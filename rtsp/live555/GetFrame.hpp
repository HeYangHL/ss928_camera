#ifndef _GETFRAME_HPP_
#define _GETFRAME_HPP_

#include <stdint.h>
#include <functional>
#include <atomic>

#include "MediaRingBuffer.hpp"


// 视频帧队列封装：只提供视频相关接口
class VideoGetFrame
{
public:
    explicit VideoGetFrame(size_t capacity = 90)
        : m_ringQueue(capacity) {}

    using FrameCallback = std::function<int(uint8_t**, int&, bool&)>;
    using ReleaseCallback = std::function<void(uint8_t*)>;
    using ResetCallback = std::function<void()>;

    int Set_Video_Frame(FrameCallback cb);
    int Set_Release_Frame(ReleaseCallback cb);
    int Set_Reset_Callback(ResetCallback cb);

    int Get_Video_Frame(uint8_t **pack, int &size, bool &key);
    int Get_Video_Frame(uint8_t **pack, int &size, bool &key, unsigned long clientSessionId);

    int PushVideoFrame(uint8_t *data, int size, bool key, uint64_t pts = 0);

    void Release_Video_Frame(uint8_t *pack);
    void Reset(void);
    void AddClient(unsigned int clientSessionId);
    void RemoveClient(unsigned int clientSessionId);
    void ClearRingHistory();
    void ClearRingBuffer();

    int GetActiveCount() const { return m_activeCount.load(); }
    CircularQueue<VideoFrame> *GetRingQueue() { return &m_ringQueue; }

private:
    friend class H265LiveVideoServerMediaSubssion;
    friend class H264LiveVideoServerMediaSubssion;

    FrameCallback frame_cb;
    ReleaseCallback release_cb;
    ResetCallback reset_cb;
    std::atomic<int> m_activeCount{0};
    CircularQueue<VideoFrame> m_ringQueue;
};


// 音频帧队列封装：只提供音频相关接口
class AudioGetFrame
{
public:
    explicit AudioGetFrame(size_t capacity = 64)
        : m_ringQueue(capacity) {}

    int Get_Audio_Frame(uint8_t **pack, int &size, unsigned long clientSessionId,
                        AudioFrame *info = NULL);

    int PushAudioFrame(uint8_t *data, int size, uint64_t pts = 0,
                       unsigned sampleRate = 48000, unsigned channels = 1,
                       unsigned adtsHeaderLen = 7, int seq = 0);

    void Release_Audio_Frame(uint8_t *pack);
    void AddClient(unsigned int clientSessionId);
    void RemoveClient(unsigned int clientSessionId);
    void ClearRingBuffer();

    CircularQueue<AudioFrame> *GetRingQueue() { return &m_ringQueue; }

private:
    CircularQueue<AudioFrame> m_ringQueue;
};

#endif
