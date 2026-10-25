#include "H264LiveVideoServerMediaSubssion.hh"
#include "H264VideoStreamFramer.hh"
#include "H264VideoRTPSink.hh"

int g_h264_active_clients = 0;

H264LiveVideoServerMediaSubssion *H264LiveVideoServerMediaSubssion::createNew(UsageEnvironment &env, Boolean reuseFirstSource) {
  return new H264LiveVideoServerMediaSubssion(env, reuseFirstSource);
}


H264LiveVideoServerMediaSubssion::H264LiveVideoServerMediaSubssion(
    UsageEnvironment &env, Boolean reuseFirstSource)
    : OnDemandServerMediaSubsession(env, reuseFirstSource) {}

H264LiveVideoServerMediaSubssion::~H264LiveVideoServerMediaSubssion() {}

FramedSource *H264LiveVideoServerMediaSubssion::createNewStreamSource(
    unsigned clientSessionId, unsigned &estBitrate) {
  /* Remain to do : assign estBitrate */
  estBitrate = 1000; // kbps, estimate

  H264FramedLiveSource *liveSource = H264FramedLiveSource::createNew(envir());
  if (liveSource == NULL) {
    return NULL;
  }
  fLiveSource = liveSource;

  // Create a framer for the Video Elementary Stream:
  return H264VideoStreamFramer::createNew(envir(), liveSource);
}

void H264LiveVideoServerMediaSubssion::startStream(unsigned clientSessionId, void* streamToken,
    TaskFunc* rtcpRRHandler,
    void* rtcpRRHandlerClientData,
    unsigned short& rtpSeqNum,
    unsigned& rtpTimestamp,
    ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
    void* serverRequestAlternativeByteHandlerClientData) {
  bool firstClient = (g_h264_active_clients == 0);
  g_h264_active_clients++;
  if (fLiveSource != NULL && firstClient) {
    fLiveSource->requestResync();
  }
  OnDemandServerMediaSubsession::startStream(clientSessionId, streamToken,
      rtcpRRHandler, rtcpRRHandlerClientData, rtpSeqNum, rtpTimestamp,
      serverRequestAlternativeByteHandler, serverRequestAlternativeByteHandlerClientData);
}

void H264LiveVideoServerMediaSubssion::deleteStream(unsigned clientSessionId, void*& streamToken) {
  if (g_h264_active_clients > 0) {
    g_h264_active_clients--;
  }
  OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}

RTPSink *H264LiveVideoServerMediaSubssion::createNewRTPSink(Groupsock *rtpGroupsock,
                                  unsigned char rtpPayloadTypeIfDynamic,
                                  FramedSource *inputSource) {
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}