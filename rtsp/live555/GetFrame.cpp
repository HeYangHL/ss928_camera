#include "GetFrame.hpp"
#include <cstring>
#include <new>


// ==================== VideoGetFrame ====================

int VideoGetFrame::Get_Video_Frame(uint8_t **pack, int &size, bool &key)
{
    if (pack == NULL)
        return -1;
    if (frame_cb)
        return frame_cb(pack, size, key);
    return -1;
}

int VideoGetFrame::Get_Video_Frame(uint8_t **pack, int &size, bool &key,
                                    unsigned long clientSessionId)
{
    if (pack == NULL)
        return -1;

    *pack = NULL;
    size = 0;
    key = false;

    bool frameKey = false;
    uint8_t *copy = NULL;
    int copyLen = 0;
    bool copyFailed = false;

    bool got = m_ringQueue.dequeueAndCopy((int)clientSessionId,
        [&](const VideoFrame &vf) -> bool {
            if (vf.data == NULL || vf.len <= 0)
                return true;

            uint8_t *p = new (std::nothrow) uint8_t[vf.len];
            if (p == NULL)
            {
                copyFailed = true;
                return false;
            }
            memcpy(p, vf.data, vf.len);
            copy = p;
            copyLen = vf.len;
            return true;
        }, &frameKey);

    if (got)
    {
        if (copy != NULL && copyLen > 0)
        {
            *pack = copy;
            size = copyLen;
            key = frameKey;
            return 0;
        }
        // 无效/空帧已消费，让source稍后重试
        return 0;
    }
    if (copyFailed)
        return -1;

    // 队列为空时，尝试一次历史IDR回放（仅视频路径使用）
    if (m_ringQueue.size() == 0 && frame_cb)
    {
        uint8_t *cbPack = NULL;
        int cbSize = 0;
        bool cbKey = false;
        if (frame_cb(&cbPack, cbSize, cbKey) == 0 && cbPack != NULL && cbSize > 0)
        {
            *pack = cbPack;
            size = cbSize;
            key = cbKey;
            return 0;
        }
    }

    // 环形队列非空但客户端已追上写指针，或暂时无数据：
    // 返回0大小让source延时重试，不能走handleClosure
    return 0;
}

int VideoGetFrame::Set_Video_Frame(FrameCallback cb)
{
    frame_cb = cb;
    return 0;
}

int VideoGetFrame::Set_Release_Frame(ReleaseCallback cb)
{
    release_cb = cb;
    return 0;
}

int VideoGetFrame::Set_Reset_Callback(ResetCallback cb)
{
    reset_cb = cb;
    return 0;
}

void VideoGetFrame::Release_Video_Frame(uint8_t *pack)
{
    if (pack == NULL)
        return;

    if (release_cb)
        release_cb(pack);
    else
        delete[] pack;
}

void VideoGetFrame::Reset(void)
{
    if (reset_cb)
        reset_cb();
}

void VideoGetFrame::AddClient(unsigned int clientSessionId)
{
    m_ringQueue.addClient(clientSessionId);
    if (reset_cb)
        reset_cb();
}

void VideoGetFrame::RemoveClient(unsigned int clientSessionId)
{
    m_ringQueue.removeClient(clientSessionId);
}

void VideoGetFrame::ClearRingHistory()
{
    m_ringQueue.clearHistory();
}

void VideoGetFrame::ClearRingBuffer()
{
    m_ringQueue.clear();
}

int VideoGetFrame::PushVideoFrame(uint8_t *data, int size, bool key, uint64_t pts)
{
    if (data == NULL || size <= 0)
        return -1;

    VideoFrame vf;
    vf.data = data;
    vf.len = size;
    vf.key = key;
    vf.pts = pts;
    return m_ringQueue.enqueue(vf) ? 0 : -1;
}


// ==================== AudioGetFrame ====================

int AudioGetFrame::Get_Audio_Frame(uint8_t **pack, int &size, unsigned long clientSessionId,
                                    AudioFrame *info)
{
    if (pack == NULL)
        return -1;

    *pack = NULL;
    size = 0;

    AudioFrame frameInfo;
    uint8_t *copy = NULL;
    int copyLen = 0;
    bool copyFailed = false;

    bool got = m_ringQueue.dequeueAndCopy((int)clientSessionId,
        [&](const AudioFrame &af) -> bool {
            if (af.data == NULL || af.len <= 0)
                return true;

            uint8_t *p = new (std::nothrow) uint8_t[af.len];
            if (p == NULL)
            {
                copyFailed = true;
                return false;
            }
            memcpy(p, af.data, af.len);
            copy = p;
            copyLen = af.len;
            frameInfo = af;
            frameInfo.data = p;
            frameInfo.len = af.len;
            return true;
        });

    if (got)
    {
        if (copy != NULL && copyLen > 0)
        {
            *pack = copy;
            size = copyLen;
            if (info != NULL)
                *info = frameInfo;
            return 0;
        }
        return 0;
    }
    if (copyFailed)
        return -1;

    // 空队列或客户端已追上写指针：返回0大小让source延时重试
    return 0;
}

int AudioGetFrame::PushAudioFrame(uint8_t *data, int size, uint64_t pts,
                                   unsigned sampleRate, unsigned channels,
                                   unsigned adtsHeaderLen, int seq)
{
    if (data == NULL || size <= 0)
        return -1;

    AudioFrame af;
    af.data = data;
    af.len = size;
    af.pts = pts;
    af.sampleRate = sampleRate;
    af.channels = channels;
    af.adtsHeaderLen = adtsHeaderLen;
    af.seq = seq;
    return m_ringQueue.enqueue(af) ? 0 : -1;
}

void AudioGetFrame::Release_Audio_Frame(uint8_t *pack)
{
    if (pack != NULL)
        delete[] pack;
}

void AudioGetFrame::AddClient(unsigned int clientSessionId)
{
    m_ringQueue.addClient(clientSessionId);
}

void AudioGetFrame::RemoveClient(unsigned int clientSessionId)
{
    m_ringQueue.removeClient(clientSessionId);
}

void AudioGetFrame::ClearRingBuffer()
{
    m_ringQueue.clear();
}
