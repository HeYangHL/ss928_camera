#include <sys/time.h>
#include <stdio.h>
#include <string.h>

#include "AACFramedLiveSource.hh"

#define MAX_AUDIO_FRAME_SIZE 4096
#define AAC_LC_SAMPLES_PER_FRAME 1024
#define AUDIO_EMPTY_RETRY_US 5000

static unsigned int adtsSamplingFrequencyIndex(unsigned int sampleRate)
{
    switch (sampleRate)
    {
        case 96000: return 0;
        case 88200: return 1;
        case 64000: return 2;
        case 48000: return 3;
        case 44100: return 4;
        case 32000: return 5;
        case 24000: return 6;
        case 22050: return 7;
        case 16000: return 8;
        case 12000: return 9;
        case 11025: return 10;
        case 8000:  return 11;
        case 7350:  return 12;
        default:    return 3; // 默认按 48kHz 处理
    }
}

AACFramedLiveSource* AACFramedLiveSource::createNew(UsageEnvironment& env, AudioGetFrame *pGetFrame,
                                                    unsigned sampleRate, unsigned numChannels)
{
    return new AACFramedLiveSource(env, pGetFrame, sampleRate, numChannels);
}

AACFramedLiveSource::AACFramedLiveSource(UsageEnvironment& env, AudioGetFrame *pGetFrame,
                                         unsigned sampleRate, unsigned numChannels)
    : FramedSource(env),
      m_pGetFrame(pGetFrame),
      fSamplingFrequency(sampleRate),
      fNumChannels(numChannels),
      fConfigStr(),
      fFrameDurationUs(0),
      m_clientId((unsigned long)this)
{
    // AudioSpecificConfig: AAC-LC(objectType=2), 48kHz(sfi=3), mono(ch=1) => 0x1188
    unsigned int sfi = adtsSamplingFrequencyIndex(fSamplingFrequency);
    unsigned int config = (2u << 11) | ((sfi & 0x0F) << 7) | ((fNumChannels & 0x0F) << 3);
    snprintf(fConfigStr, sizeof(fConfigStr), "%04X", config);

    if (fSamplingFrequency > 0)
    {
        fFrameDurationUs = (unsigned int)((AAC_LC_SAMPLES_PER_FRAME * 1000000ULL +
                                           fSamplingFrequency / 2) / fSamplingFrequency);
    }

    if (m_pGetFrame != NULL)
    {
        // 音频队列按实时写指针定位，不需要关键帧
        m_pGetFrame->AddClient((unsigned int)m_clientId);
    }
}

AACFramedLiveSource::~AACFramedLiveSource()
{
    if (m_pGetFrame != NULL)
    {
        m_pGetFrame->RemoveClient((unsigned int)m_clientId);
    }
}

unsigned int AACFramedLiveSource::maxFrameSize() const
{
    return MAX_AUDIO_FRAME_SIZE;
}

void AACFramedLiveSource::doGetNextFrame()
{
    if (m_pGetFrame == NULL)
    {
        handleClosure();
        return;
    }

    uint8_t *frame = NULL;
    int size = 0;
    AudioFrame af;
    int ret = m_pGetFrame->Get_Audio_Frame(&frame, size, m_clientId, &af);
    if (ret < 0)
    {
        handleClosure();
        return;
    }

    if (frame == NULL || size <= 0)
    {
        if (frame != NULL)
        {
            m_pGetFrame->Release_Audio_Frame(frame);
        }
        nextTask() = envir().taskScheduler().scheduleDelayedTask(AUDIO_EMPTY_RETRY_US,
            (TaskFunc*)doGetNextFrameStatic, this);
        return;
    }

    // AENC 输出为 ADTS：0xFFF 同步字，protection_absent=1 时头长 7 字节，否则 9 字节
    if (size < 7 || frame[0] != 0xFF || (frame[1] & 0xF0) != 0xF0)
    {
        m_pGetFrame->Release_Audio_Frame(frame);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)doGetNextFrameStatic, this);
        return;
    }

    // AudioFrame 中携带 AENC 阶段计算好的 ADTS 头长；异常时回退到实时解析
    unsigned int headerSize = af.adtsHeaderLen;
    if (headerSize < 7 || headerSize >= (unsigned int)size)
    {
        headerSize = (frame[1] & 0x01) ? 7 : 9;
    }
    if ((unsigned int)size <= headerSize)
    {
        m_pGetFrame->Release_Audio_Frame(frame);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)doGetNextFrameStatic, this);
        return;
    }

    unsigned int rawSize = (unsigned int)size - headerSize;
    unsigned int copySize = rawSize;
    if (copySize > fMaxSize)
    {
        fNumTruncatedBytes = copySize - fMaxSize;
        copySize = fMaxSize;
    }
    else
    {
        fNumTruncatedBytes = 0;
    }

    if (copySize == 0)
    {
        m_pGetFrame->Release_Audio_Frame(frame);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)doGetNextFrameStatic, this);
        return;
    }

    // MPEG4GenericRTPSink(AAC-hbr) 需要裸 AAC 帧，这里去掉 ADTS 头
    memcpy(fTo, frame + headerSize, copySize);
    fFrameSize = copySize;
    m_pGetFrame->Release_Audio_Frame(frame);

    gettimeofday(&fPresentationTime, NULL);
    unsigned int sampleRate = (af.sampleRate != 0) ? af.sampleRate : fSamplingFrequency;
    if (sampleRate > 0)
    {
        fDurationInMicroseconds = (unsigned int)((AAC_LC_SAMPLES_PER_FRAME * 1000000ULL +
                                                   sampleRate / 2) / sampleRate);
    }
    else
    {
        fDurationInMicroseconds = fFrameDurationUs;
    }

    FramedSource::afterGetting(this);
}

void AACFramedLiveSource::doStopGettingFrames()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
}

void AACFramedLiveSource::doGetNextFrameStatic(FramedSource* source)
{
    source->doGetNextFrame();
}
