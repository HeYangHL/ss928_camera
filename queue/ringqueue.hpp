#ifndef _RINGQUEUE_HPP_
#define _RINGQUEUE_HPP_

#include <vector>
#include <iostream>
#include <map>
#include <mutex>
#include <cstring>
#include <stdint.h>

template<typename T>
class CircularQueue {
private:
    std::vector<T> buffer;
    unsigned int head;    // 队头索引
    unsigned int tail;    // 队尾索引
    unsigned int count;   // 当前元素数量
    unsigned int capacity;// 总容量

public:
    CircularQueue(size_t size = 90) : capacity(size), head(0), tail(0), count(0) {
        buffer.resize(size);
    }

    // 入队
    bool enqueue(const T& item) {
        if (isFull()) {
            return false;  // 队列已满
        }
        buffer[tail] = item;
        tail = (tail + 1) % capacity;
        count++;
        return true;
    }

    // 出队
    bool dequeue(T& item) {
        if (isEmpty()) {
            return false;  // 队列为空
        }
        item = buffer[head];
        head = (head + 1) % capacity;
        count--;
        return true;
    }

    // 查看队头元素
    bool front(T& item) const {
        if (isEmpty()) {
            return false;
        }
        item = buffer[head];
        return true;
    }

    // 判断队列是否为空
    bool isEmpty() const {
        return count == 0;
    }

    // 判断队列是否已满
    bool isFull() const {
        return count == capacity;
    }

    // 获取当前元素数量
    size_t size() const {
        return count;
    }

    // 获取容量
    size_t getCapacity() const {
        return capacity;
    }

    // 清空队列
    void clear() {
        head = tail = count = 0;
    }

    // 打印队列内容（用于调试）
    void print() const {
        std::cout << "Queue (head->tail): ";
        for (size_t i = 0; i < count; i++) {
            size_t index = (head + i) % capacity;
            std::cout << buffer[index] << " ";
        }
        std::cout << std::endl;
    }
};

struct RtspFrameItem {
    uint8_t *data;
    int size;
    bool key;
    uint64_t pts;
};

struct RtspClientCtx {
    unsigned int clientId;
    int readOffset;
    bool catchingUp;
    bool firstConnect;
    RtspClientCtx() : clientId(0), readOffset(0), catchingUp(false), firstConnect(false) {}
};

class RtspRingBuffer {
private:
    std::vector<RtspFrameItem> m_buffer;
    size_t m_capacity;
    size_t m_head; // only moves on overwrite
    size_t m_tail;
    size_t m_count;
    std::map<unsigned int, RtspClientCtx> m_clients;
    std::mutex m_mutex;

public:
    RtspRingBuffer(size_t cap = 90) : m_capacity(cap), m_head(0), m_tail(0), m_count(0) {
        m_buffer.resize(m_capacity);
    }

    void setCapacity(size_t cap) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (cap == 0) cap = 90;
        m_capacity = cap;
        m_buffer.resize(m_capacity);
        clear();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < m_buffer.size(); i++) {
            if (m_buffer[i].data) {
                delete[] m_buffer[i].data;
                m_buffer[i].data = nullptr;
            }
            m_buffer[i].size = 0;
            m_buffer[i].key = false;
            m_buffer[i].pts = 0;
        }
        m_head = 0;
        m_tail = 0;
        m_count = 0;
        m_clients.clear();
    }

    bool push(const uint8_t *data, int size, bool key, uint64_t pts = 0) {
        if (data == nullptr || size <= 0) return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        uint8_t *copy = new uint8_t[size];
        memcpy(copy, data, size);
        if (m_count < m_capacity) {
            m_buffer[m_tail] = RtspFrameItem{copy, size, key, pts};
            m_tail = (m_tail + 1) % m_capacity;
            m_count++;
        } else {
            // overwrite oldest
            if (m_buffer[m_head].data) {
                delete[] m_buffer[m_head].data;
            }
            m_buffer[m_head] = RtspFrameItem{copy, size, key, pts};
            m_head = (m_head + 1) % m_capacity;
            m_tail = (m_tail + 1) % m_capacity;
        }
        return true;
    }

    size_t getCount() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

    void addClient(unsigned int clientId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_clients.find(clientId) == m_clients.end()) {
            m_clients[clientId] = RtspClientCtx();
            m_clients[clientId].clientId = clientId;
            m_clients[clientId].readOffset = 0;
            m_clients[clientId].catchingUp = false;
            m_clients[clientId].firstConnect = true;
        }
    }

    RtspClientCtx* getClientCtx(unsigned int clientId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_clients.find(clientId);
        if (it == m_clients.end()) return nullptr;
        return &it->second;
    }

    int getLastKeyOffset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count == 0) return -1;
        for (int i = (int)m_count - 1; i >= 0; i--) {
            size_t idx = (m_head + (size_t)i) % m_capacity;
            if (m_buffer[idx].key) return i;
        }
        return -1;
    }

    // get copy at offset from head (0-based in current window)
    bool getCopyAtOffset(int offset, uint8_t **outData, int &outSize, bool &outKey) {
        if (outData == nullptr) return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count == 0) return false;
        if (offset < 0) offset = 0;
        if (offset >= (int)m_count) offset = (int)m_count - 1;
        size_t idx = (m_head + (size_t)offset) % m_capacity;
        RtspFrameItem &item = m_buffer[idx];
        if (item.data == nullptr || item.size <= 0) return false;
        uint8_t *copy = new uint8_t[item.size];
        memcpy(copy, item.data, item.size);
        *outData = copy;
        outSize = item.size;
        outKey = item.key;
        return true;
    }
};

#endif
