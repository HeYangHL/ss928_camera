#ifndef _H265FRAMEDLIVESOURCE_HH
#define _H265FRAMEDLIVESOURCE_HH

#include <FramedSource.hh>
#include "GetFrame.hpp"


class H265FramedLiveSource : public FramedSource
{
public:
    static H265FramedLiveSource* createNew(UsageEnvironment& env);

protected:
    H265FramedLiveSource(UsageEnvironment& env);
    virtual ~H265FramedLiveSource();

    void doReadFromFile();
    virtual unsigned int maxFrameSize() const;

private:
    // 重定义虚函数（结构对齐参考项目 WW_ByteStreamFileSource）
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();
    static void doGetNextFrameStatic(FramedSource* source);
};

#endif