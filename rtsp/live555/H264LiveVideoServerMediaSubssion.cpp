#include "H264LiveVideoServerMediaSubssion.hh"
#include "H264VideoStreamFramer.hh"
#include "H264VideoRTPSink.hh"

H264LiveVideoServerMediaSubssion *H264LiveVideoServerMediaSubssion::createNew(UsageEnvironment &env, Boolean reuseFirstSource, GetFrame *pGetFrame) {
  return new H264LiveVideoServerMediaSubssion(env, reuseFirstSource, pGetFrame);
}


H264LiveVideoServerMediaSubssion::H264LiveVideoServerMediaSubssion(
    UsageEnvironment &env, Boolean reuseFirstSource, GetFrame *pGetFrame)
    : OnDemandServerMediaSubsession(env, reuseFirstSource)
{
  m_pGetFrame = pGetFrame;
  m_pActiveClients = NULL;
}

H264LiveVideoServerMediaSubssion::~H264LiveVideoServerMediaSubssion() {}

FramedSource *H264LiveVideoServerMediaSubssion::createNewStreamSource(
    unsigned clientSessionId, unsigned &estBitrate) {
  /* Remain to do : assign estBitrate */
  estBitrate = 1000; // kbps, estimate

  H264FramedLiveSource *liveSource = H264FramedLiveSource::createNew(envir(), m_pGetFrame);
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
  bool firstClient = true;
  if (m_pGetFrame != NULL) {
    int cnt = m_pGetFrame->m_activeCount.load();
    firstClient = (cnt == 0);
    m_pGetFrame->m_activeCount.fetch_add(1);
  } else if (m_pActiveClients != NULL) {
    firstClient = (*m_pActiveClients == 0);
    (*m_pActiveClients)++;
  }
  if (fLiveSource != NULL) {
    if (m_pGetFrame != NULL) {
      if (firstClient) {
        m_pGetFrame->ClearRingHistory();
      }
      m_pGetFrame->AddClient(clientSessionId);
    }
    if (fLiveSource != NULL) {
      fLiveSource->requestResync();
    }
  }
  OnDemandServerMediaSubsession::startStream(clientSessionId, streamToken,
      rtcpRRHandler, rtcpRRHandlerClientData, rtpSeqNum, rtpTimestamp,
      serverRequestAlternativeByteHandler, serverRequestAlternativeByteHandlerClientData);
}

void H264LiveVideoServerMediaSubssion::deleteStream(unsigned clientSessionId, void*& streamToken) {
  if (m_pGetFrame != NULL) {
    if (m_pGetFrame->m_activeCount.load() > 0) {
      m_pGetFrame->m_activeCount.fetch_sub(1);
    }
    m_pGetFrame->RemoveClient(clientSessionId);
  } else if (m_pActiveClients != NULL && *m_pActiveClients > 0) {
    (*m_pActiveClients)--;
  }
  OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}

RTPSink *H264LiveVideoServerMediaSubssion::createNewRTPSink(Groupsock *rtpGroupsock,
                                  unsigned char rtpPayloadTypeIfDynamic,
                                  FramedSource *inputSource) {
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}