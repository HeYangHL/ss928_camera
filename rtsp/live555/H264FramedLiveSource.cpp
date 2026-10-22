// 仿照参考项目 WW_ByteStreamFileSource.cpp 实现
#include <sys/time.h>

#include "H264FramedLiveSource.hh"

////////// H264FramedLiveSource //////////
#define MAX_VIDEO_FRAME_SIZE 1000000

H264FramedLiveSource* H264FramedLiveSource::createNew(UsageEnvironment& env)
{
    return new H264FramedLiveSource(env);
}

H264FramedLiveSource::H264FramedLiveSource(UsageEnvironment& env) : FramedSource(env)
{
}

H264FramedLiveSource::~H264FramedLiveSource()
{
}

unsigned int H264FramedLiveSource::maxFrameSize() const
{
    return MAX_VIDEO_FRAME_SIZE;
}

void H264FramedLiveSource::doGetNextFrame()
{
    doReadFromFile();
}

void H264FramedLiveSource::doStopGettingFrames()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
}

void H264FramedLiveSource::doGetNextFrameStatic(FramedSource* source)
{
    source->doGetNextFrame();
}

void H264FramedLiveSource::doReadFromFile()
{
    struct timeval oldPresentationTime = fPresentationTime;
    fFrameSize = 0;

    uint8_t* frame = NULL;
    int size = 0;
    int iFrameSize = g_rtsp_get_frame.Get_Video_Frame(&frame, size);

    if (iFrameSize < 0) {
        handleClosure();
        return;
    } else if (size == 0) {
        if (frame != NULL) {
            g_rtsp_get_frame.Release_Video_Frame(frame);
        }
        // 暂无数据：1ms 后重试（与参考项目一致，不能交付 0 字节）
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)doGetNextFrameStatic, this);
        return;
    }

    // 整个编码包一次交付，从不截断（BANK_SIZE 1.5MB 装得下 VENC 整包）
    memcpy(fTo, frame, size);
    fFrameSize = (unsigned int)size;
    g_rtsp_get_frame.Release_Video_Frame(frame);

    if (((fPresentationTime.tv_sec == 0) && (fPresentationTime.tv_usec == 0))
        || ((oldPresentationTime.tv_sec == fPresentationTime.tv_sec)
            && (oldPresentationTime.tv_usec == fPresentationTime.tv_usec))) {
        gettimeofday(&fPresentationTime, NULL);
    }

    FramedSource::afterGetting(this);
}