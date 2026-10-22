#ifndef _H265_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_
#define _H265_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_
#include "OnDemandServerMediaSubsession.hh"


class H265LiveVideoServerMediaSubssion : public OnDemandServerMediaSubsession {

public:
	static H265LiveVideoServerMediaSubssion* createNew(UsageEnvironment& env, Boolean reuseFirstSource=false);

protected: // we're a virtual base class
	H265LiveVideoServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource);
	~H265LiveVideoServerMediaSubssion();

protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
	virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource *inputSource);
};


#endif
