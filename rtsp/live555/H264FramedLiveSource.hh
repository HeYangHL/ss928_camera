#ifndef _H264FRAMEDLIVESOURCE_HH
#define _H264FRAMEDLIVESOURCE_HH

#include <FramedSource.hh>
#include "GetFrame.hpp"


class H264FramedLiveSource : public FramedSource
{
public:
    static H264FramedLiveSource* createNew(UsageEnvironment& env);

protected:
    H264FramedLiveSource(UsageEnvironment& env);
    virtual ~H264FramedLiveSource();

    void doReadFromFile();
    virtual unsigned int maxFrameSize() const;

private:
    // 重定义虚函数（结构对齐参考项目 WW_ByteStreamFileSource）
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();
    static void doGetNextFrameStatic(FramedSource* source);
};

#endif