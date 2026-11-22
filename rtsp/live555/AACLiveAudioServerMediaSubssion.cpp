#include "AACLiveAudioServerMediaSubssion.hh"
#include "MPEG4GenericRTPSink.hh"

AACLiveAudioServerMediaSubssion* AACLiveAudioServerMediaSubssion::createNew(UsageEnvironment& env,
                                                                           Boolean reuseFirstSource,
                                                                           AudioGetFrame *pGetFrame,
                                                                           unsigned sampleRate,
                                                                           unsigned numChannels)
{
    return new AACLiveAudioServerMediaSubssion(env, reuseFirstSource, pGetFrame, sampleRate, numChannels);
}

AACLiveAudioServerMediaSubssion::AACLiveAudioServerMediaSubssion(UsageEnvironment& env,
                                                                 Boolean reuseFirstSource,
                                                                 AudioGetFrame *pGetFrame,
                                                                 unsigned sampleRate,
                                                                 unsigned numChannels)
    : OnDemandServerMediaSubsession(env, reuseFirstSource),
      m_pGetFrame(pGetFrame),
      fLiveSource(NULL),
      fSampleRate(sampleRate),
      fNumChannels(numChannels)
{
}

AACLiveAudioServerMediaSubssion::~AACLiveAudioServerMediaSubssion()
{
}

FramedSource* AACLiveAudioServerMediaSubssion::createNewStreamSource(unsigned clientSessionId,
                                                                     unsigned& estBitrate)
{
    estBitrate = 96; // kbps

    fLiveSource = AACFramedLiveSource::createNew(envir(), m_pGetFrame, fSampleRate, fNumChannels);
    return fLiveSource;
}

RTPSink* AACLiveAudioServerMediaSubssion::createNewRTPSink(Groupsock *rtpGroupsock,
                                                           unsigned char rtpPayloadTypeIfDynamic,
                                                           FramedSource *inputSource)
{
    AACFramedLiveSource *source = (AACFramedLiveSource *)inputSource;
    if (source == NULL)
        return NULL;

    return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
                                          source->samplingFrequency(), "audio", "AAC-hbr",
                                          source->configStr(), source->numChannels());
}
