#ifndef _MEDIA_RINGBUFFER_HPP_
#define _MEDIA_RINGBUFFER_HPP_

#include <vector>
#include <iostream>
#include <map>
#include <mutex>
#include <cstring>
#include <stdint.h>
#include <deque>

struct MediaFrame
{
    uint8_t *data;
    int len;
    bool key;
    uint64_t pts;
    MediaFrame() : data(NULL), len(0), key(false), pts(0) {}
};

struct ClientMessage
{
    bool enable;
    bool catchingUp;
    unsigned int readIndex;  // 客户端当前读索引（指向ringbuffer中的索引）
    ClientMessage() : enable(false), catchingUp(false), readIndex(0) {}
};

template <typename T>
class CircularQueue
{
private:
    std::vector<T> buffer;
    unsigned int head;     // 环形队列中最旧帧索引（满了覆盖时用）
    unsigned int tail;     // 下一个写入位置
    unsigned int count;    // 当前窗口内有效帧数
    unsigned int capacity; // 总容量
    std::deque<unsigned int> keyIndexList; // 环形窗口内关键帧索引列表

    std::map<unsigned int, ClientMessage> clientMsg;
    std::mutex m_mutex;

public:
    CircularQueue(size_t size = 90) : capacity(size), head(0), tail(0), count(0)
    {
        buffer.resize(size);
    }

    // 入队：满了就覆盖最旧帧（共享历史池，不拒绝入队）
    bool enqueue(const T &item, bool key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (capacity == 0)
            return false;

        if (count < capacity)
        {
            // 队列未满
            buffer[tail] = item;
            if (key)
            {
                keyIndexList.push_back(tail);
            }
            tail = (tail + 1) % capacity;
            count++;
        }
        else
        {
            // 队列已满，覆盖最旧帧
            if (buffer[head].data != NULL)
            {
                // 共享池：覆盖最旧帧时需要释放旧帧内存，避免泄漏
                // 但如果有客户端正好停在被覆盖的位置，getFrame会重新定位
                delete[] buffer[head].data;
                buffer[head].data = NULL;
            }
            // 如果最旧帧是关键帧，从key列表中移除
            while (!keyIndexList.empty() && keyIndexList.front() == head)
            {
                keyIndexList.pop_front();
            }
            buffer[head] = item;
            if (key)
            {
                keyIndexList.push_back(head);
            }
            head = (head + 1) % capacity;
            tail = (tail + 1) % capacity;
            // count 保持不变
        }
        return true;
    }

    // 注册/重置客户端（合并接口）：不存在则创建并定位，存在则重置游标
    void addClient(unsigned int clientId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = clientMsg.find(clientId);
        ClientMessage msg;
        msg.enable = true;
        msg.catchingUp = true;
        if (!keyIndexList.empty())
        {
            msg.readIndex = keyIndexList.back();
        }
        else if (count > 0)
        {
            msg.readIndex = head;
        }
        else
        {
            msg.readIndex = tail;
            msg.catchingUp = false;
        }
        if (it != clientMsg.end())
        {
            it->second = msg;
        }
        else
        {
            clientMsg[clientId] = msg;
        }
    }

    // 移除客户端
    void removeClient(unsigned int clientId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clientMsg.erase(clientId);
    }

    // 出队：每个客户端独立读取和推进游标，不影响全局count
    bool dequeue(T &item, int clientId, bool &isKey)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (count == 0)
        {
            return false;
        }
        auto it = clientMsg.find((unsigned int)clientId);
        if (it == clientMsg.end())
        {
            // 未注册客户端，自动注册并定位到最近关键帧
            ClientMessage msg;
            msg.enable = true;
            msg.catchingUp = true;
            if (!keyIndexList.empty())
            {
                msg.readIndex = keyIndexList.back();
            }
            else if (count > 0)
            {
                msg.readIndex = head;
            }
            else
            {
                msg.readIndex = tail;
                msg.catchingUp = false;
            }
            clientMsg[(unsigned int)clientId] = msg;
            it = clientMsg.find((unsigned int)clientId);
        }

        // 检查客户端游标是否已被覆盖（队列已满且游标正好是head位置，或者游标已经被覆盖）
        if (count == capacity)
        {
            // 判断游标是否已被覆盖：如果readIndex == head，说明该位置已被最新入队覆盖
            if (it->second.readIndex == head)
            {
                // 重新定位到最近关键帧
                if (!keyIndexList.empty())
                {
                    it->second.readIndex = keyIndexList.back();
                    it->second.catchingUp = true;
                }
                else
                {
                    it->second.readIndex = head;
                    it->second.catchingUp = true;
                }
            }
        }

        // 确保游标在有效范围内
        if (buffer[it->second.readIndex].data == NULL && count > 0)
        {
            // 跳到下一个有效位置
            it->second.readIndex = head;
        }

        // 读取当前客户端游标位置的帧
        item = buffer[it->second.readIndex];
        isKey = buffer[it->second.readIndex].key;
        // 推进客户端游标（不影响全局count）
        it->second.readIndex = (it->second.readIndex + 1) % capacity;
        return true;
    }

    // 获取关键帧索引
    int getIkeyIndex(void)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (keyIndexList.empty()) return -1;
        return (int)keyIndexList.back();
    }

    // 获取最近关键帧索引
    int getLastKeyIndex(void)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (keyIndexList.empty()) return -1;
        return (int)keyIndexList.back();
    }

    // 判断队列是否为空（全局状态，用于生产端参考）
    bool isEmpty() const
    {
        return count == 0;
    }

    // 判断队列是否已满
    bool isFull() const
    {
        return count >= capacity;
    }

    // 获取当前元素数量
    size_t size() const
    {
        return count;
    }

    // 获取容量
    size_t getCapacity() const
    {
        return capacity;
    }

    // 清空队列（释放所有历史帧内存）
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < buffer.size(); i++)
        {
            if (buffer[i].data != NULL)
            {
                delete[] buffer[i].data;
                buffer[i].data = NULL;
            }
            buffer[i].len = 0;
            buffer[i].key = false;
            buffer[i].pts = 0;
        }
        head = tail = count = 0;
        keyIndexList.clear();
        clientMsg.clear();
    }

    // 清空历史帧但保留客户端上下文
    void clearHistory()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < buffer.size(); i++)
        {
            if (buffer[i].data != NULL)
            {
                delete[] buffer[i].data;
                buffer[i].data = NULL;
            }
            buffer[i].len = 0;
            buffer[i].key = false;
            buffer[i].pts = 0;
        }
        head = tail = count = 0;
        keyIndexList.clear();
        // 保留clientMsg，由addClient重新定位
    }


};

#endif
