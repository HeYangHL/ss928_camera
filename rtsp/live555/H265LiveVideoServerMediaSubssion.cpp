#include "H265LiveVideoServerMediaSubssion.hh"
#include "H265VideoStreamFramer.hh"
#include "H265VideoRTPSink.hh"

H265LiveVideoServerMediaSubssion *H265LiveVideoServerMediaSubssion::createNew(UsageEnvironment &env, Boolean reuseFirstSource, GetFrame *pGetFrame) {
  return new H265LiveVideoServerMediaSubssion(env, reuseFirstSource, pGetFrame);
}


H265LiveVideoServerMediaSubssion::H265LiveVideoServerMediaSubssion(
    UsageEnvironment &env, Boolean reuseFirstSource, GetFrame *pGetFrame)
    : OnDemandServerMediaSubsession(env, reuseFirstSource)
{
  m_pGetFrame = pGetFrame;
  m_pActiveClients = NULL;
}

H265LiveVideoServerMediaSubssion::~H265LiveVideoServerMediaSubssion() {}

FramedSource *H265LiveVideoServerMediaSubssion::createNewStreamSource(
    unsigned clientSessionId, unsigned &estBitrate) {
  // 4K H.265 VBR peak ~= 10240 kbps (venc_h265_vbr_param_init, PIC_3840X2160)
  estBitrate = 10000; // kbps, estimate

  H265FramedLiveSource *liveSource = H265FramedLiveSource::createNew(envir(), m_pGetFrame);
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

void H265LiveVideoServerMediaSubssion::deleteStream(unsigned clientSessionId, void*& streamToken) {
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

RTPSink *H265LiveVideoServerMediaSubssion::createNewRTPSink(Groupsock *rtpGroupsock,
                                  unsigned char rtpPayloadTypeIfDynamic,
                                  FramedSource *inputSource) {
  return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
