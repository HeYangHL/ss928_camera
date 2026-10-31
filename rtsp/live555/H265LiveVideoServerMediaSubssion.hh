#ifndef _H265_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_
#define _H265_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_
#include "OnDemandServerMediaSubsession.hh"


#include "H265FramedLiveSource.hh"

class H265LiveVideoServerMediaSubssion : public OnDemandServerMediaSubsession {

public:
	static H265LiveVideoServerMediaSubssion* createNew(UsageEnvironment& env, Boolean reuseFirstSource=false, GetFrame *pGetFrame = nullptr);

protected: // we're a virtual base class
	H265LiveVideoServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource, GetFrame *pGetFrame = nullptr);
	~H265LiveVideoServerMediaSubssion();

protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
	virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource *inputSource);
	virtual void startStream(unsigned clientSessionId, void* streamToken,
		TaskFunc* rtcpRRHandler,
		void* rtcpRRHandlerClientData,
		unsigned short& rtpSeqNum,
		unsigned& rtpTimestamp,
		ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
		void* serverRequestAlternativeByteHandlerClientData);
	virtual void deleteStream(unsigned clientSessionId, void*& streamToken);
private:
	H265FramedLiveSource *fLiveSource = nullptr;
	GetFrame *m_pGetFrame = nullptr;
	int *m_pActiveClients = nullptr;
};


#endif
