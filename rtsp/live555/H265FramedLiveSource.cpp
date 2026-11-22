// 仿照参考项目 WW_ByteStreamFileSource.cpp 实现
#include <sys/time.h>

#include "H265FramedLiveSource.hh"

////////// H265FramedLiveSource //////////
#define MAX_VIDEO_FRAME_SIZE 1000000

H265FramedLiveSource* H265FramedLiveSource::createNew(UsageEnvironment& env, VideoGetFrame *pGetFrame)
{
    return new H265FramedLiveSource(env, pGetFrame);
}

H265FramedLiveSource::H265FramedLiveSource(UsageEnvironment& env, VideoGetFrame *pGetFrame) : FramedSource(env)
{
    m_pGetFrame = pGetFrame;
}

H265FramedLiveSource::~H265FramedLiveSource()
{
    if (m_pGetFrame != NULL)
        m_pGetFrame->RemoveClient((unsigned int)(unsigned long)this);
}

unsigned int H265FramedLiveSource::maxFrameSize() const
{
    return MAX_VIDEO_FRAME_SIZE;
}

void H265FramedLiveSource::doGetNextFrame()
{
    doReadFromFile();
}

void H265FramedLiveSource::doStopGettingFrames()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
}

void H265FramedLiveSource::doGetNextFrameStatic(FramedSource* source)
{
    source->doGetNextFrame();
}

void H265FramedLiveSource::requestResync()
{
    if (m_pGetFrame != NULL) {
        m_pGetFrame->Reset();
    }
    fWaitingForKey = true;
    fPosterNext = true;
}

void H265FramedLiveSource::doReadFromFile()
{
    struct timeval oldPresentationTime = fPresentationTime;
    fFrameSize = 0;

    if (m_pGetFrame == NULL) {
        handleClosure();
        return;
    }

    uint8_t* frame = NULL;
    int size = 0;
    bool key = false;
    unsigned long cid = (unsigned long)this;
    int iFrameSize = m_pGetFrame->Get_Video_Frame(&frame, size, key, cid);

    if (iFrameSize < 0) {
        handleClosure();
        return;
    } else if (size == 0) {
        if (frame != NULL) {
            m_pGetFrame->Release_Video_Frame(frame);
        }
        // 暂无数据：1ms 后重试（与参考项目一致，不能交付 0 字节）
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)doGetNextFrameStatic, this);
        return;
    }

    if (fPosterNext) {
        fPosterNext = false;
        if (key) {
            fWaitingForKey = false;
        } else if (fWaitingForKey) {
            m_pGetFrame->Release_Video_Frame(frame);
            nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
                (TaskFunc*)doGetNextFrameStatic, this);
            return;
        }
    } else if (fWaitingForKey && !key) {
        m_pGetFrame->Release_Video_Frame(frame);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)doGetNextFrameStatic, this);
        return;
    } else if (key) {
        fWaitingForKey = false;
    }

    // 整个编码包一次交付，从不截断（BANK_SIZE 1.5MB 装得下 VENC 整包）
    memcpy(fTo, frame, size);
    fFrameSize = (unsigned int)size;
    m_pGetFrame->Release_Video_Frame(frame);

    if (((fPresentationTime.tv_sec == 0) && (fPresentationTime.tv_usec == 0))
        || ((oldPresentationTime.tv_sec == fPresentationTime.tv_sec)
            && (oldPresentationTime.tv_usec == fPresentationTime.tv_usec))) {
        gettimeofday(&fPresentationTime, NULL);
    }

    FramedSource::afterGetting(this);
}