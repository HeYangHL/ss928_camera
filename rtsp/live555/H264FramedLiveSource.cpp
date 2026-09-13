#include "H264FramedLiveSource.hh"
#include "Fifo_Buffer.hpp"


H264FramedLiveSource::H264FramedLiveSource(UsageEnvironment& env): FramedSource(env)
{
	// g_rtsp_get_frame.Init_Socket();
}

H264FramedLiveSource* H264FramedLiveSource::createNew(UsageEnvironment& env)
{
	H264FramedLiveSource* newSource = new H264FramedLiveSource(env);
	return newSource;
}

H264FramedLiveSource::~H264FramedLiveSource()
{
}

// void H264FramedLiveSource::doGetNextFrame()
// {
	
// 	uint8_t *frame = new uint8_t[1024*1024];
// 	int size = 0;
// 	int ret = 0;
// 	ret = g_rtsp_get_frame.Get_Video_Frame(&frame, size);
// 	printf("live555 get video frame\n");
// 	if(ret != -1)
// 	{
// 		fFrameSize = size;
// 		memcpy(fTo, frame, fFrameSize);
// 	}
// 	else {
// 		handleClosure();
// 	}
	
// 	gettimeofday(&fPresentationTime, NULL);
//     afterGetting(this);
	
// 	return;
// }
void H264FramedLiveSource::doGetNextFrame()
{
    uint8_t *frame = nullptr;
    int size = 0;
    int ret = 0;
    
    ret = g_rtsp_get_frame.Get_Video_Frame(&frame, size);
    
    if(ret != -1 && frame != nullptr && size > 0)
    {
        if (size > fMaxSize) {
            fFrameSize = fMaxSize;
            printf("[WARNING] Frame truncated! size:%d, max:%d\n", size, fMaxSize);
        } else {
            fFrameSize = size;
        }
        
        memcpy(fTo, frame, fFrameSize);
		g_rtsp_get_frame.Release_Video_Frame(frame);
    }
    else {
		if(frame)
			g_rtsp_get_frame.Release_Video_Frame(frame);
        fFrameSize = 0; 
    }
    
    gettimeofday(&fPresentationTime, NULL);
    afterGetting(this);
    
    return;
}