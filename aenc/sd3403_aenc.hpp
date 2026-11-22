#ifndef _SD3403_AENC_HPP_
#define _SD3403_AENC_HPP_

#include <iostream>
#include "sd3403_audio.hpp"
#include "def.h"

extern "C" {
    hi_s32 hi_mpi_aenc_aac_init(hi_void);
    hi_s32 hi_mpi_aenc_aac_deinit(hi_void);
}

class SD3403_AENC{
public:
    SD3403_AENC(){};
    ~SD3403_AENC(){};
    int Sd343_Aenc_Init(void);
    int Sd343_Aenc_Deinit(void);
    int Sd3403_Ai_Bind_Aenc(void);
    int Sd3403_Ai_UnBind_Aenc(void);
    int Sd3403_Get_Aenc_Stream(char *data, int cap, uint64_t *pts, int *seq, int ms);
    hi_u32 get_point_num_per_frame(void){return point_num_per_frame;}
    int Sd343_Aenc_Aac_Init(void);
    int Sd343_Aenc_Aac_DeInit(void);
private:
    hi_payload_type encType = HI_PT_AAC;
    hi_u32 point_num_per_frame =
        (encType == HI_PT_AAC)   ? 1024 :
        (encType == HI_PT_G726)  ? 480  : 1024;
    hi_aac_type aacType = HI_AAC_TYPE_AACLC;
    hi_aac_bps aacBps = HI_AAC_BPS_96K;
    hi_audio_sample_rate audioRate = HI_AUDIO_SAMPLE_RATE_48000;
    hi_audio_snd_mode audioSndMode = HI_AUDIO_SOUND_MODE_MONO;
    hi_aac_transport_type aacTransportType = HI_AAC_TRANSPORT_TYPE_ADTS;
    hi_aenc_chn aencChn = 0;
};

#endif
