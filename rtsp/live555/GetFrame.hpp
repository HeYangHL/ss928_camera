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

class GetFrame{
    public:
    // void Init_Socket(void){};
    using FrameCallback = std::function<int(uint8_t**, int&)>;
    using ReleaseCallback = std::function<void(uint8_t*)>;
    int Set_Video_Frame(FrameCallback cb);
    int Set_Release_Frame(ReleaseCallback cb);
    int Get_Video_Frame(uint8_t **pack, int &size);
    void Release_Video_Frame(uint8_t *pack);

    private:
    int fd = 0;
    struct sockaddr_in client, server;
    FileStream fstream;
    FrameCallback frame_cb;
    ReleaseCallback release_cb;
};

extern GetFrame g_rtsp_get_frame;

#endif