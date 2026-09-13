#ifndef _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_
#define _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_
#include "H264VideoFileServerMediaSubsession.hh"


class H264LiveVideoServerMediaSubssion : public OnDemandServerMediaSubsession {

public:
	static H264LiveVideoServerMediaSubssion* createNew(UsageEnvironment& env, Boolean reuseFirstSource);

protected: // we're a virtual base class
	H264LiveVideoServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource);
	~H264LiveVideoServerMediaSubssion();

protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
	virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource *inputSource);
public:
	char fFileName[100];

	int *Server_datasize;//数据区大小指针
	unsigned char*  Server_databuf;//数据区指针
	bool *Server_dosent;//发送标示
};


#endif