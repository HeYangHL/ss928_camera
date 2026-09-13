#include <vector>
#include <iostream>

template<typename T>
class CircularQueue {
private:
    std::vector<T> buffer;
    unsigned int head;    // 队头索引
    unsigned int tail;    // 队尾索引
    unsigned int count;   // 当前元素数量
    unsigned int capacity;// 总容量

public:
    CircularQueue(size_t size) : capacity(size), head(0), tail(0), count(0) {
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