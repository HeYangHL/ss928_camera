#include "GetFrame.hpp"
#include <netinet/in.h>
#include <sys/socket.h>

GetFrame g_rtsp_get_frame;

int GetFrame::Get_Video_Frame(uint8_t **pack, int &size) {
    if(pack)
        frame_cb(pack, size);
    else
        return -1;

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

void GetFrame::Release_Video_Frame(uint8_t *pack) {
    if (release_cb && pack) {
        release_cb(pack); // 调用 SD3403_Manage::release_rtsp_stream
    }
}