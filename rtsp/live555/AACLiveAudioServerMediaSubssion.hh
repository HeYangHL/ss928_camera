#ifndef _AAC_LIVE_AUDIO_SERVER_MEDIA_SUBSESSION_HH_
#define _AAC_LIVE_AUDIO_SERVER_MEDIA_SUBSESSION_HH_

#include "OnDemandServerMediaSubsession.hh"
#include "AACFramedLiveSource.hh"


class AACLiveAudioServerMediaSubssion : public OnDemandServerMediaSubsession
{
public:
    static AACLiveAudioServerMediaSubssion* createNew(UsageEnvironment& env,
                                                      Boolean reuseFirstSource = False,
                                                      AudioGetFrame *pGetFrame = nullptr,
                                                      unsigned sampleRate = 48000,
                                                      unsigned numChannels = 1);

protected:
    AACLiveAudioServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource,
                                    AudioGetFrame *pGetFrame, unsigned sampleRate,
                                    unsigned numChannels);
    ~AACLiveAudioServerMediaSubssion();

protected:
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock *rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource *inputSource);

private:
    AudioGetFrame *m_pGetFrame;
    AACFramedLiveSource *fLiveSource;
    unsigned fSampleRate;
    unsigned fNumChannels;
};

#endif
