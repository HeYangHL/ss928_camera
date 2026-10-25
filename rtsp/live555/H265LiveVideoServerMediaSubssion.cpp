#include "H265LiveVideoServerMediaSubssion.hh"
#include "H265FramedLiveSource.hh"
#include "H265VideoStreamFramer.hh"
#include "H265VideoRTPSink.hh"


H265LiveVideoServerMediaSubssion *H265LiveVideoServerMediaSubssion::createNew(UsageEnvironment &env, Boolean reuseFirstSource) {
  return new H265LiveVideoServerMediaSubssion(env, reuseFirstSource);
}


H265LiveVideoServerMediaSubssion::H265LiveVideoServerMediaSubssion(
    UsageEnvironment &env, Boolean reuseFirstSource)
    : OnDemandServerMediaSubsession(env, reuseFirstSource) {}

H265LiveVideoServerMediaSubssion::~H265LiveVideoServerMediaSubssion() {}

int g_h265_active_clients = 0;

FramedSource *H265LiveVideoServerMediaSubssion::createNewStreamSource(
    unsigned clientSessionId, unsigned &estBitrate) {
  // 4K H.265 VBR peak ~= 10240 kbps (venc_h265_vbr_param_init, PIC_3840X2160)
  estBitrate = 10000; // kbps, estimate

  H265FramedLiveSource *liveSource = H265FramedLiveSource::createNew(envir());
  if (liveSource == NULL) {
    return NULL;
  }
  fLiveSource = liveSource;

  // Create a framer for the Video Elementary Stream:
  return H265VideoStreamFramer::createNew(envir(), liveSource);
}

void H265LiveVideoServerMediaSubssion::startStream(unsigned clientSessionId, void* streamToken,
    TaskFunc* rtcpRRHandler,
    void* rtcpRRHandlerClientData,
    unsigned short& rtpSeqNum,
    unsigned& rtpTimestamp,
    ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
    void* serverRequestAlternativeByteHandlerClientData) {
  bool firstClient = (g_h265_active_clients == 0);
  g_h265_active_clients++;
  if (fLiveSource != NULL && firstClient) {
    fLiveSource->requestResync();
  }
  OnDemandServerMediaSubsession::startStream(clientSessionId, streamToken,
      rtcpRRHandler, rtcpRRHandlerClientData, rtpSeqNum, rtpTimestamp,
      serverRequestAlternativeByteHandler, serverRequestAlternativeByteHandlerClientData);
}

void H265LiveVideoServerMediaSubssion::deleteStream(unsigned clientSessionId, void*& streamToken) {
  if (g_h265_active_clients > 0) {
    g_h265_active_clients--;
  }
  OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}

RTPSink *H265LiveVideoServerMediaSubssion::createNewRTPSink(Groupsock *rtpGroupsock,
                                  unsigned char rtpPayloadTypeIfDynamic,
                                  FramedSource *inputSource) {
  return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
