#ifndef _MEDIA_RINGBUFFER_HPP_
#define _MEDIA_RINGBUFFER_HPP_

#include <vector>
#include <map>
#include <mutex>
#include <cstring>
#include <stdint.h>
#include <deque>
#include <functional>

// 视频帧：关键帧信息只对视频有意义
struct VideoFrame
{
    uint8_t *data;
    int len;
    bool key;
    uint64_t pts;

    VideoFrame() : data(NULL), len(0), key(false), pts(0) {}
};

// 音频帧：只保存音频需要的参数
struct AudioFrame
{
    uint8_t *data;
    int len;
    uint64_t pts;
    unsigned sampleRate;
    unsigned channels;
    unsigned adtsHeaderLen;
    int seq;

    AudioFrame()
        : data(NULL), len(0), pts(0), sampleRate(48000), channels(1), adtsHeaderLen(7), seq(0) {}
};

// 队列核心只依赖 traits，不需要在运行时判断媒体类型
template <typename T>
struct RingTraits;

template <>
struct RingTraits<VideoFrame>
{
    static bool useKeyIndex() { return true; }
    static bool isKey(const VideoFrame &frame) { return frame.key; }
};

template <>
struct RingTraits<AudioFrame>
{
    static bool useKeyIndex() { return false; }
    static bool isKey(const AudioFrame &) { return false; }
};

struct ClientMessage
{
    bool enable;
    bool catchingUp;
    uint64_t nextSeq;    // 客户端下一次要读取的帧序号（全局单调递增）

    ClientMessage() : enable(false), catchingUp(false), nextSeq(0) {}
};

template <typename T>
class CircularQueue
{
private:
    std::vector<T> buffer;
    unsigned int head;      // 环形队列中最旧帧索引
    unsigned int tail;      // 下一个写入位置
    unsigned int count;     // 当前窗口内有效帧数
    unsigned int capacity;  // 总容量
    uint64_t writeSeq;      // 已写入总帧数，用作帧序号基准
    std::vector<uint64_t> slotSeq; // 每个槽位当前保存的帧序号

    std::deque<unsigned int> keyIndexList; // 视频关键帧索引列表（音频不使用）

    std::map<unsigned int, ClientMessage> clientMsg;
    mutable std::mutex m_mutex;

    // 调用方必须已持有 m_mutex
    ClientMessage makeClientMsgLocked() const
    {
        ClientMessage msg;
        msg.enable = true;

        if (!RingTraits<T>::useKeyIndex())
        {
            // 音频没有关键帧：从实时写指针开始，避免新客户端回放历史积压
            msg.nextSeq = writeSeq;
            msg.catchingUp = false;
        }
        else if (!keyIndexList.empty())
        {
            // 视频：从最近关键帧开始
            msg.nextSeq = slotSeq[keyIndexList.back()];
            msg.catchingUp = true;
        }
        else if (count > 0)
        {
            msg.nextSeq = slotSeq[head];
            msg.catchingUp = true;
        }
        else
        {
            msg.nextSeq = writeSeq;
            msg.catchingUp = false;
        }

        return msg;
    }

    // 调用方必须已持有 m_mutex
    void freeSlotLocked(unsigned int index)
    {
        if (buffer[index].data != NULL)
        {
            delete[] buffer[index].data;
            buffer[index].data = NULL;
        }
    }

public:
    CircularQueue(size_t size = 90)
        : buffer(size), head(0), tail(0), count(0), capacity((unsigned int)size),
          writeSeq(0), slotSeq(size, 0)
    {
    }

    ~CircularQueue()
    {
        for (size_t i = 0; i < buffer.size(); i++)
        {
            if (buffer[i].data != NULL)
            {
                delete[] buffer[i].data;
                buffer[i].data = NULL;
            }
        }
    }

    // 入队：满了就覆盖最旧帧（共享历史池，不拒绝入队）
    // data 的所有权随 item 移交给队列
    bool enqueue(const T &item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (capacity == 0)
            return false;

        bool key = RingTraits<T>::isKey(item);

        if (count < capacity)
        {
            buffer[tail] = item;
            slotSeq[tail] = writeSeq;
            if (key)
            {
                keyIndexList.push_back(tail);
            }
            tail = (tail + 1) % capacity;
            count++;
        }
        else
        {
            freeSlotLocked(head);
            while (!keyIndexList.empty() && keyIndexList.front() == head)
            {
                keyIndexList.pop_front();
            }
            buffer[head] = item;
            slotSeq[head] = writeSeq;
            if (key)
            {
                keyIndexList.push_back(head);
            }
            head = (head + 1) % capacity;
            tail = (tail + 1) % capacity;
        }

        writeSeq++;
        return true;
    }

    // 注册/重置客户端（合并接口）：不存在则创建并定位，存在则重置游标
    void addClient(unsigned int clientId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clientMsg[clientId] = makeClientMsgLocked();
    }

    // 移除客户端
    void removeClient(unsigned int clientId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clientMsg.erase(clientId);
    }

    // 出队：每个客户端独立读取和推进游标，不影响全局count
    // 视频：掉队时重定位到最近关键帧；音频：掉队时丢弃积压，跳到实时写指针
    bool dequeue(T &item, int clientId, bool *isKey = NULL)
    {
        return dequeueImpl(item, clientId, isKey, NULL);
    }

    // 出队并在持有队列锁期间完成数据拷贝，避免返回后生产端覆盖同槽导致悬空指针
    // copyFn 返回 false 时表示拷贝失败，游标不推进，由调用方决定重试或关闭
    bool dequeueAndCopy(int clientId, const std::function<bool(const T&)> &copyFn,
                        bool *isKey = NULL)
    {
        T item;
        return dequeueImpl(item, clientId, isKey, &copyFn);
    }

private:
    bool dequeueImpl(T &item, int clientId, bool *isKey,
                     const std::function<bool(const T&)> *copyFn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (count == 0)
        {
            return false;
        }

        auto it = clientMsg.find((unsigned int)clientId);
        if (it == clientMsg.end())
        {
            // 未注册客户端，自动注册并按帧类型定位
            it = clientMsg.emplace((unsigned int)clientId, makeClientMsgLocked()).first;
        }

        const uint64_t oldestSeq = writeSeq - count;

        // 已追上写指针：当前没有新帧
        if (it->second.nextSeq >= writeSeq)
        {
            return false;
        }

        // 游标已被覆盖
        if (it->second.nextSeq < oldestSeq)
        {
            if (!RingTraits<T>::useKeyIndex())
            {
                // 音频不做关键帧等待，直接丢弃积压，保持低延迟
                it->second.nextSeq = writeSeq;
                it->second.catchingUp = false;
                return false;
            }

            // 视频重新定位到最近关键帧（没有关键帧则回到最旧帧）
            if (!keyIndexList.empty())
            {
                it->second.nextSeq = slotSeq[keyIndexList.back()];
            }
            else
            {
                it->second.nextSeq = oldestSeq;
            }
            it->second.catchingUp = true;

            if (it->second.nextSeq >= writeSeq)
            {
                return false;
            }
        }

        unsigned int index = (head + (unsigned int)(it->second.nextSeq - oldestSeq)) % capacity;
        if (slotSeq[index] != it->second.nextSeq)
        {
            // 理论上不会走到这里；保险起见按帧类型重定位
            if (!RingTraits<T>::useKeyIndex())
            {
                it->second.nextSeq = writeSeq;
                return false;
            }

            it->second.nextSeq = keyIndexList.empty() ? oldestSeq : slotSeq[keyIndexList.back()];
            if (it->second.nextSeq >= writeSeq)
            {
                return false;
            }
            index = (head + (unsigned int)(it->second.nextSeq - oldestSeq)) % capacity;
        }

        // 读取当前客户端游标位置的帧
        item = buffer[index];
        bool frameKey = RingTraits<T>::isKey(item);
        if (isKey != NULL)
        {
            *isKey = frameKey;
        }

        if (copyFn != NULL && !(*copyFn)(item))
        {
            // 拷贝失败：不推进游标，调用方可以重试
            return false;
        }

        // 推进客户端游标（不影响全局count）
        it->second.nextSeq++;
        if (it->second.catchingUp && frameKey)
        {
            it->second.catchingUp = false;
        }

        return true;
    }

public:
    // 获取关键帧索引（仅视频有效）
    int getIkeyIndex(void)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (keyIndexList.empty())
            return -1;
        return (int)keyIndexList.back();
    }

    // 获取最近关键帧索引（仅视频有效）
    int getLastKeyIndex(void)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (keyIndexList.empty())
            return -1;
        return (int)keyIndexList.back();
    }

    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return count == 0;
    }

    bool isFull() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return count >= capacity;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return count;
    }

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
            freeSlotLocked((unsigned int)i);
            buffer[i] = T();
            slotSeq[i] = 0;
        }
        head = tail = count = 0;
        writeSeq = 0;
        keyIndexList.clear();
        clientMsg.clear();
    }

    // 清空历史帧但保留客户端上下文
    void clearHistory()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < buffer.size(); i++)
        {
            freeSlotLocked((unsigned int)i);
            buffer[i] = T();
            slotSeq[i] = 0;
        }
        head = tail = count = 0;
        keyIndexList.clear();

        for (auto &kv : clientMsg)
        {
            kv.second = makeClientMsgLocked();
        }
    }
};

#endif
