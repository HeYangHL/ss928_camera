#ifndef __SD3403_AI_HPP__
#define __SD3403_AI_HPP__

#include <iostream>
#include "sd3403_aenc.hpp"
#include "def.h"
#include "hi_common_aio.h"

class SD3403_AI{
public:
    SD3403_AI(){};
    ~SD3403_AI(){};

    int Sd343_Ai_Init(void);
    int DevAttr(ot_aio_attr &attr);
    int Sd343_Ai_Deinit(void);
    int Sd343_Ai_Set_Input_Volume(hi_s32 volume_db);
    int Sd343_Ai_Get_Input_Volume(hi_s32 *volume_db);
    int Sd343_Ai_Set_Vqe(hi_bool enable);
    int Sd343_Ai_Set_Vqe_Agc(hi_s32 target_level, hi_s32 max_gain);

private:
    int Sd343_Ai_Acodec_Cfg(void);
    int Sd343_Ai_Vqe_Register(void);
    int Sd343_Ai_Vqe_Init(void);

    hi_audio_dev audioDev = 0;
    hi_audio_sample_rate audioRate = HI_AUDIO_SAMPLE_RATE_48000;
    hi_audio_bit_width audioBt = HI_AUDIO_BIT_WIDTH_16;
    hi_audio_snd_mode audioSndMode = HI_AUDIO_SOUND_MODE_MONO;
    hi_aio_mode audioWorkMode = HI_AIO_MODE_I2S_MASTER;
    hi_ai_chn audioChn = 0;
    hi_s32 inputVolume = 30;//取值范围[-78 80]
    hi_bool vqeEnable = HI_TRUE;
    hi_bool vqeRunning = HI_FALSE;
    hi_ai_record_vqe_cfg vqeCfg = {};
};

#endif