#include "rtsp.hpp"

RTSP::RTSP()
{}
RTSP::~RTSP()
{}

int RTSP::sensor0_rv(void *opaque, uint8_t *data, int len, uint64_t *pts)
{
    int ret = 0;
    int framesize = 0;
    uint64_t ts_pts = 0;
    uint64_t rtp_pts = 0;
    uint64_t pts_nal = 0;
    int8_t *frame= nullptr;
    RTSP *rtsp = (RTSP *)opaque;

    printf("======>sensor 0 a = %d\n", rtsp->a);
    while (1)
    {
        ret = rtsp->get_sensor0_fifo().read(&frame, &framesize, &ts_pts, 100);
        if (ret > 0)
        {
            memcpy(data, frame, framesize);
            *pts = ts_pts;
            if(frame != NULL)
            {
                delete [] frame;
                frame = NULL;
            }
            break;
        }
    }

    return ret;
}

int RTSP::sensor1_rv(void *opaque, uint8_t *data, int len, uint64_t *pts)
{
    int ret = 0;
    int framesize = 0;
    uint64_t ts_pts = 0;
    uint64_t rtp_pts = 0;
    uint64_t pts_nal = 0;
    int8_t *frame= nullptr;
    RTSP *rtsp = (RTSP *)opaque;

    printf("======>sensor 1 a = %d\n", rtsp->a);
    while (1)
    {
        ret = rtsp->get_sensor1_fifo().read(&frame, &framesize, &ts_pts, 100);
        if (ret > 0)
        {
            memcpy(data, frame, framesize);
            *pts = 0;
            if(frame != NULL)
            {
                delete [] frame;
                frame = NULL;
            }
            break;
        }
    }

    return ret;
}

int RTSP::rtsp_setver_start(void)
{
    rtsp_s.start();
}
int RTSP::add_media_stream(void)
{
    a = 2;
    rtsp_s.add_media_source("sensor0_media", sensor0_rv, this, NULL, NULL, 30, 4800, "h264", "aac");
    rtsp_s.add_media_source("sensor1_media", sensor1_rv, this, NULL, NULL, 30, 4800, "h264", "aac");
}
