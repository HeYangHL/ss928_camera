#ifndef _AACFRAMEDLIVESOURCE_HH
#define _AACFRAMEDLIVESOURCE_HH

#include <FramedSource.hh>
#include "GetFrame.hpp"


class AACFramedLiveSource : public FramedSource
{
public:
    static AACFramedLiveSource* createNew(UsageEnvironment& env, AudioGetFrame *pGetFrame = nullptr,
                                          unsigned sampleRate = 48000, unsigned numChannels = 1);

    unsigned samplingFrequency() const { return fSamplingFrequency; }
    unsigned numChannels() const { return fNumChannels; }
    char const* configStr() const { return fConfigStr; }

protected:
    AACFramedLiveSource(UsageEnvironment& env, AudioGetFrame *pGetFrame,
                        unsigned sampleRate, unsigned numChannels);
    virtual ~AACFramedLiveSource();

    virtual unsigned int maxFrameSize() const;

private:
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();
    static void doGetNextFrameStatic(FramedSource* source);

    AudioGetFrame *m_pGetFrame;
    unsigned fSamplingFrequency;
    unsigned fNumChannels;
    char fConfigStr[5];
    unsigned fFrameDurationUs;
    unsigned long m_clientId;
};

#endif
