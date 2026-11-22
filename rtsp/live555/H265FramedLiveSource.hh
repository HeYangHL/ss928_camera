#ifndef _H265FRAMEDLIVESOURCE_HH
#define _H265FRAMEDLIVESOURCE_HH

#include <FramedSource.hh>
#include "GetFrame.hpp"


class H265FramedLiveSource : public FramedSource
{
public:
    static H265FramedLiveSource* createNew(UsageEnvironment& env, VideoGetFrame *pGetFrame = nullptr);
    void requestResync();

protected:
    H265FramedLiveSource(UsageEnvironment& env, VideoGetFrame *pGetFrame = nullptr);
    virtual ~H265FramedLiveSource();

    void doReadFromFile();
    virtual unsigned int maxFrameSize() const;

private:
    // 重定义虚函数（结构对齐参考项目 WW_ByteStreamFileSource）
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();
    static void doGetNextFrameStatic(FramedSource* source);
    VideoGetFrame *m_pGetFrame;
    bool fWaitingForKey = false;
    bool fPosterNext = false;
};

#endif