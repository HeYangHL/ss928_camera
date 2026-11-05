#include "GetFrame.hpp"
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>



int GetFrame::Get_Video_Frame(uint8_t **pack, int &size, bool &key) {
    if (pack == nullptr)
        return -1;
    if (frame_cb)
        return frame_cb(pack, size, key);
    return -1;
}

int GetFrame::Get_Video_Frame(uint8_t **pack, int &size, bool &key, unsigned long clientSessionId) {
    if (pack == nullptr)
        return -1;
    // 优先从环形队列读取（多客户端各自游标）
    if (m_ringQueue.size() > 0) {
        MediaFrame mf;
        bool mfKey = false;
        if (m_ringQueue.dequeue(mf, (int)clientSessionId, mfKey)) {
            if (mf.data != NULL && mf.len > 0) {
                // 深拷贝一份给live555使用
                *pack = (uint8_t*)malloc(mf.len);
                if (*pack == NULL) {
                    return -1;
                }
                memcpy(*pack, mf.data, mf.len);
                size = mf.len;
                key = mf.key || mfKey;
                return 0;
            }
        }
    } else {
        // 环形队列为空，暂时返回0大小让source重试，不要关闭
        size = 0;
        key = false;
        if (pack) *pack = NULL;
        return 0;
    }
    if (frame_cb)
        return frame_cb(pack, size, key);
    size = 0;
    key = false;
    if (pack) *pack = NULL;
    return 0;
}

int GetFrame::Set_Video_Frame(FrameCallback cb)
{
    frame_cb = cb;
    return 0;
}

// GetFrame.cpp
int GetFrame::Set_Release_Frame(ReleaseCallback cb) {
    release_cb = cb;
    return 0;
}

int GetFrame::Set_Reset_Callback(ResetCallback cb) {
    reset_cb = cb;
    return 0;
}

void GetFrame::Release_Video_Frame(uint8_t *pack) {
    if (release_cb && pack) {
        release_cb(pack); // 调用 SD3403_Manage::release_rtsp_stream
    }
}

void GetFrame::Reset(void) {
    if (reset_cb) {
        reset_cb();
    }
}

void GetFrame::AddClient(unsigned int clientSessionId) {
    m_ringQueue.addClient(clientSessionId);
    if (reset_cb) {
        reset_cb();
    }
}

void GetFrame::RemoveClient(unsigned int clientSessionId) {
    m_ringQueue.removeClient(clientSessionId);
}

void GetFrame::ClearRingHistory() {
    m_ringQueue.clearHistory();
}

void GetFrame::ClearRingBuffer() {
    m_ringQueue.clear();
}

int GetFrame::PushFrame(uint8_t *data, int size, bool key, uint64_t pts) {
    if (data == NULL || size <= 0) return -1;
    MediaFrame mf;
    mf.data = data;
    mf.len = size;
    mf.key = key;
    mf.pts = pts;
    return m_ringQueue.enqueue(mf, key) ? 0 : -1;
}