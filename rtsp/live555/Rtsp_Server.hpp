#ifndef _RTSP_SERVER_HPP_
#define _RTSP_SERVER_HPP_


#include <iostream>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "H264LiveVideoServerMediaSubssion.hh"
#include "H264FramedLiveSource.hh"


class RtspServer{
public:
    void Init(void);
    void Stop(void);
    void announceURL(RTSPServer* rtspServer, ServerMediaSession* sms);
    void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms, char const* streamName, char const* inputFileName);
private:
    UsageEnvironment* env;
    TaskScheduler *scheduler = NULL;
    UserAuthenticationDatabase* authDB = NULL;
    Boolean reuseFirstSource;
    char fWatchVariable;
};

#endif