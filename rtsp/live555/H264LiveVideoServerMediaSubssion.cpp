#include "H264LiveVideoServerMediaSubssion.hh"
#include "H264FramedLiveSource.hh"
#include "H264VideoStreamFramer.hh"
#include "H264VideoRTPSink.hh"
#include "ByteStreamFileSource.hh"


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

  // Create a framer for the Video Elementary Stream:
  return H264VideoStreamFramer::createNew(envir(), liveSource);
}

RTPSink *H264LiveVideoServerMediaSubssion::createNewRTPSink(Groupsock *rtpGroupsock,
                                  unsigned char rtpPayloadTypeIfDynamic,
                                  FramedSource *inputSource) {
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}