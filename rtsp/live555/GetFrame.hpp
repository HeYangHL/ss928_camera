#ifndef _GETFRAME_HPP_
#define _GETFRAME_HPP_

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "FileStream.h"
#include <functional>

#include <atomic>
#include "MediaRingBuffer.hpp"

class GetFrame{
    public:
    GetFrame() = default;
    ~GetFrame() = default;
    // void Init_Socket(void){};
    using FrameCallback = std::function<int(uint8_t**, int&, bool&)>;
    using ReleaseCallback = std::function<void(uint8_t*)>;
    using ResetCallback = std::function<void()>;
    int Set_Video_Frame(FrameCallback cb);
    int Set_Release_Frame(ReleaseCallback cb);
    int Set_Reset_Callback(ResetCallback cb);
    int Get_Video_Frame(uint8_t **pack, int &size, bool &key);
    int Get_Video_Frame(uint8_t **pack, int &size, bool &key, unsigned long clientSessionId);
    void Release_Video_Frame(uint8_t *pack);
    void Reset(void);
    void AddClient(unsigned int clientSessionId);
    void RemoveClient(unsigned int clientSessionId);
    void ClearRingHistory();
    void ClearRingBuffer();
    int PushFrame(uint8_t *data, int size, bool key, uint64_t pts = 0);
    int GetActiveCount() const { return m_activeCount.load(); }
    CircularQueue<MediaFrame> *GetRingQueue() { return &m_ringQueue; }

    private:
    friend class H265LiveVideoServerMediaSubssion;
    friend class H264LiveVideoServerMediaSubssion;
    int fd = 0;
    struct sockaddr_in client, server;
    FileStream fstream;
    FrameCallback frame_cb;
    ReleaseCallback release_cb;
    ResetCallback reset_cb;
    std::atomic<int> m_activeCount{0};
    CircularQueue<MediaFrame> m_ringQueue;
};

#endif