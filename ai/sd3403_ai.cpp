#include "sd3403_ai.hpp"
#include "hi_acodec.h"
#ifdef OT_VQE_USE_STATIC_MODULE_REGISTER
#include "hi_vqe_register.h"
#endif

#define ACODEC_FILE "/dev/acodec"
#define ACODEC_INPUT_VOL_MIN (-78)
#define ACODEC_INPUT_VOL_MAX 80

int SD3403_AI::Sd343_Ai_Acodec_Cfg(void)
{
    hi_s32 fd = -1;
    hi_acodec_fs i2s_fs;
    hi_acodec_mixer input_mode = HI_ACODEC_MIXER_IN_D;
    hi_s32 volume = inputVolume;

    switch (audioRate) {
        case HI_AUDIO_SAMPLE_RATE_8000:  i2s_fs = HI_ACODEC_FS_8000;  break;
        case HI_AUDIO_SAMPLE_RATE_16000: i2s_fs = HI_ACODEC_FS_16000; break;
        case HI_AUDIO_SAMPLE_RATE_32000: i2s_fs = HI_ACODEC_FS_32000; break;
        case HI_AUDIO_SAMPLE_RATE_44100: i2s_fs = HI_ACODEC_FS_44100; break;
        case HI_AUDIO_SAMPLE_RATE_48000: i2s_fs = HI_ACODEC_FS_48000; break;
        default:
            ss_print("[%s %d] unsupported sample rate: %d\n", __func__, __LINE__, audioRate);
            return -1;
    }

    fd = open(ACODEC_FILE, O_RDWR);
    if (fd < 0) {
        ss_print("[%s %d] open %s failed\n", __func__, __LINE__, ACODEC_FILE);
        return -1;
    }

    if (ioctl(fd, HI_ACODEC_SOFT_RESET_CTRL))
        ss_print("[%s %d] acodec soft reset failed\n", __func__, __LINE__);

    if (ioctl(fd, HI_ACODEC_SET_I2S1_FS, &i2s_fs)) {
        ss_print("[%s %d] set acodec sample rate failed\n", __func__, __LINE__);
        close(fd);
        return -1;
    }

    if (ioctl(fd, HI_ACODEC_SET_MIXER_MIC, &input_mode)) {
        ss_print("[%s %d] select acodec input mode failed\n", __func__, __LINE__);
        close(fd);
        return -1;
    }

    /* input volume range [-78, 80] dB, recommended [20, 50], larger is louder */
    if (ioctl(fd, HI_ACODEC_SET_INPUT_VOLUME, &volume)) {
        ss_print("[%s %d] set acodec input volume failed\n", __func__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    ss_print("[%s %d] acodec input volume: %d dB\n", __func__, __LINE__, volume);
    return 0;
}

int SD3403_AI::Sd343_Ai_Set_Input_Volume(hi_s32 volume_db)
{
    hi_s32 fd = -1;
    hi_s32 volume = volume_db;

    if (volume_db < ACODEC_INPUT_VOL_MIN || volume_db > ACODEC_INPUT_VOL_MAX) {
        ss_print("[%s %d] invalid volume: %d, range is [%d, %d] dB\n",
                 __func__, __LINE__, volume_db, ACODEC_INPUT_VOL_MIN, ACODEC_INPUT_VOL_MAX);
        return -1;
    }

    fd = open(ACODEC_FILE, O_RDWR);
    if (fd < 0) {
        ss_print("[%s %d] open %s failed\n", __func__, __LINE__, ACODEC_FILE);
        return -1;
    }

    if (ioctl(fd, HI_ACODEC_SET_INPUT_VOLUME, &volume)) {
        ss_print("[%s %d] set acodec input volume failed\n", __func__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    inputVolume = volume_db;
    ss_print("[%s %d] acodec input volume set to %d dB\n", __func__, __LINE__, volume_db);
    return 0;
}

int SD3403_AI::Sd343_Ai_Get_Input_Volume(hi_s32 *volume_db)
{
    hi_s32 fd = -1;
    hi_s32 volume = 0;

    if (volume_db == nullptr) {
        ss_print("[%s %d] invalid param\n", __func__, __LINE__);
        return -1;
    }

    fd = open(ACODEC_FILE, O_RDWR);
    if (fd < 0) {
        ss_print("[%s %d] open %s failed\n", __func__, __LINE__, ACODEC_FILE);
        return -1;
    }

    if (ioctl(fd, HI_ACODEC_GET_INPUT_VOLUME, &volume)) {
        ss_print("[%s %d] get acodec input volume failed\n", __func__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    *volume_db = volume;
    inputVolume = volume;
    return 0;
}

int SD3403_AI::Sd343_Ai_Vqe_Register(void)
{
#ifdef OT_VQE_USE_STATIC_MODULE_REGISTER
    static hi_bool registered = HI_FALSE;
    hi_audio_vqe_register vqeReg = {};
    hi_s32 ret;

    if (registered == HI_TRUE)
        return 0;

    vqeReg.record_mod_cfg.handle = hi_vqe_record_get_handle();
    vqeReg.hpf_mod_cfg.handle = hi_vqe_hpf_get_handle();
    vqeReg.anr_mod_cfg.handle = hi_vqe_anr_get_handle();
    vqeReg.agc_mod_cfg.handle = hi_vqe_agc_get_handle();
    vqeReg.eq_mod_cfg.handle = hi_vqe_eq_get_handle();

    ret = hi_mpi_audio_register_vqe_mod(&vqeReg);
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] register vqe module failed; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    registered = HI_TRUE;
#endif
    return 0;
}

int SD3403_AI::Sd343_Ai_Vqe_Init(void)
{
    hi_s32 ret;

    ret = Sd343_Ai_Vqe_Register();
    if (ret != 0)
        return -1;

    if (vqeCfg.work_sample_rate == 0) {
        vqeCfg.work_sample_rate = audioRate;
        vqeCfg.frame_sample = AACLC_SAMPLES_PER_FRAME;
        vqeCfg.work_state = HI_VQE_WORK_STATE_COMMON;
        vqeCfg.in_chn_num = 1;
        vqeCfg.out_chn_num = 1;
        vqeCfg.record_type = HI_VQE_RECORD_NORMAL;

        vqeCfg.hpf_cfg.usr_mode = HI_TRUE;
        vqeCfg.hpf_cfg.hpf_freq = HI_AUDIO_HPF_FREQ_80;

        vqeCfg.rnr_cfg.usr_mode = HI_FALSE;

        vqeCfg.agc_cfg.usr_mode = HI_TRUE;
        vqeCfg.agc_cfg.target_level = -6;
        vqeCfg.agc_cfg.noise_floor = -50;
        vqeCfg.agc_cfg.max_gain = 20;
        vqeCfg.agc_cfg.adjust_speed = 5;
        vqeCfg.agc_cfg.improve_snr = 0;
        vqeCfg.agc_cfg.use_hpf = 0;
        vqeCfg.agc_cfg.output_mode = 0;
        vqeCfg.agc_cfg.noise_suppress_switch = 1;

        vqeCfg.open_mask = HI_AI_RECORDVQE_MASK_HPF |
                           HI_AI_RECORDVQE_MASK_RNR |
                           HI_AI_RECORDVQE_MASK_AGC;
    }

    ret = hi_mpi_ai_set_record_vqe_attr(audioDev, audioChn, &vqeCfg);
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] set record vqe attr failed; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    ret = hi_mpi_ai_enable_vqe(audioDev, audioChn);
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] enable ai vqe failed; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    vqeRunning = HI_TRUE;
    ss_print("[%s %d] record vqe enabled, agc target:%d dB, max gain:%d dB\n",
             __func__, __LINE__, vqeCfg.agc_cfg.target_level, vqeCfg.agc_cfg.max_gain);
    return 0;
}

int SD3403_AI::Sd343_Ai_Set_Vqe(hi_bool enable)
{
    hi_s32 ret;

    if (enable == HI_TRUE) {
        vqeEnable = HI_TRUE;
        if (vqeRunning == HI_TRUE)
            return 0;
        ret = Sd343_Ai_Vqe_Init();
        if (ret != 0)
            return -1;
        return 0;
    }

    vqeEnable = HI_FALSE;
    if (vqeRunning == HI_FALSE)
        return 0;

    ret = hi_mpi_ai_disable_vqe(audioDev, audioChn);
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] disable ai vqe failed; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    vqeRunning = HI_FALSE;
    return 0;
}

int SD3403_AI::Sd343_Ai_Set_Vqe_Agc(hi_s32 target_level, hi_s32 max_gain)
{
    hi_s32 ret;

    if (target_level < -40 || target_level > -1 || max_gain < 0 || max_gain > 30) {
        ss_print("[%s %d] invalid agc param, target:%d [-40,-1] dB, max gain:%d [0,30] dB\n",
                 __func__, __LINE__, target_level, max_gain);
        return -1;
    }

    vqeCfg.agc_cfg.usr_mode = HI_TRUE;
    vqeCfg.agc_cfg.target_level = (hi_s8)target_level;
    vqeCfg.agc_cfg.max_gain = (hi_s8)max_gain;
    vqeCfg.agc_cfg.adjust_speed = 5;
    vqeCfg.open_mask |= HI_AI_RECORDVQE_MASK_AGC;

    if (vqeRunning == HI_FALSE)
        return 0;

    ret = hi_mpi_ai_disable_vqe(audioDev, audioChn);
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] disable ai vqe failed; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }
    vqeRunning = HI_FALSE;

    ret = hi_mpi_ai_set_record_vqe_attr(audioDev, audioChn, &vqeCfg);
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] set record vqe attr failed; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    ret = hi_mpi_ai_enable_vqe(audioDev, audioChn);
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] enable ai vqe failed; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    vqeRunning = HI_TRUE;
    ss_print("[%s %d] vqe agc set to target:%d dB, max gain:%d dB\n",
             __func__, __LINE__, target_level, max_gain);
    return 0;
}

int SD3403_AI::Sd343_Ai_Init(void)
{
    hi_s32 ret = 0;
    hi_aio_attr devAttr = {};
    hi_ai_chn_param chnParam;
    hi_ai_chn_attr chnAttr;

    devAttr.bit_width          = audioBt;
    devAttr.sample_rate        = audioRate;
    devAttr.snd_mode           = audioSndMode;
    devAttr.work_mode          = audioWorkMode;   // 或 I2S_MASTER
    devAttr.expand_flag        = 0;                        // 8bit→16bit 扩展
    devAttr.frame_num          = 30;
    devAttr.point_num_per_frame= AACLC_SAMPLES_PER_FRAME;
    devAttr.chn_cnt            = 1;
    devAttr.clk_share          = 1;                        // 与 AO0 共用时钟
    devAttr.i2s_type           = OT_AIO_I2STYPE_INNERCODEC;// 按板级对接填

    /* 2. 必须先 set（初始化属性） */
    ret = hi_mpi_ai_set_pub_attr(audioDev, &devAttr);
    if(ret != HI_SUCCESS)
    {
        ss_print("[%s %d] get dev attr error; errcode : %x\n", __func__, __LINE__, ret);
        return ret;
    }
    printf("==============================================================================\n");
    printf("rate : %d\n", devAttr.sample_rate);
    printf("bit width : %d\n", devAttr.bit_width);
    printf("mode : %d\n", devAttr.work_mode);
    printf("snd mode : %d\n", devAttr.snd_mode);
    printf("expand_flag : %d\n", devAttr.expand_flag);
    printf("frame_num : %d\n", devAttr.frame_num);
    printf("point_num_per_frame : %d\n", devAttr.point_num_per_frame);
    printf("chn_cnt : %d\n", devAttr.chn_cnt);
    printf("clk_share : %d\n", devAttr.clk_share);
    printf("i2s_type : %d\n", devAttr.i2s_type);
    printf("==============================================================================\n");
    
    ret = hi_mpi_ai_enable(audioDev);
    if(ret != HI_SUCCESS)
    {
        ss_print("[%s %d] ai dev enable error; errcode : %x\n", __func__, __LINE__, ret);
        return ret;
    }
    ret = hi_mpi_ai_enable_chn(audioDev, audioChn);
    if(ret != HI_SUCCESS)
    {
        ss_print("[%s %d] ai chn enable error; errcode : %x\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = Sd343_Ai_Acodec_Cfg();
    if(ret != 0)
    {
        ss_print("[%s %d] config acodec error\n", __func__, __LINE__);
        hi_mpi_ai_disable_chn(audioDev, audioChn);
        hi_mpi_ai_disable(audioDev);
        return ret;
    }

    if (vqeEnable == HI_TRUE) {
        ret = Sd343_Ai_Vqe_Init();
        if (ret != 0) {
            ss_print("[%s %d] record vqe init failed, continue without vqe\n", __func__, __LINE__);
            vqeRunning = HI_FALSE;
        }
    }

    ret = hi_mpi_ai_get_chn_param(audioDev, audioChn, &chnParam);
    if(ret != HI_SUCCESS)
    {
        ss_print("[%s %d] get chn param error; errcode : %x\n", __func__, __LINE__, ret);
    }
    else
    {
        printf("usr_frame_depth : %d\n", chnParam.usr_frame_depth);
    }

    ret = hi_mpi_ai_get_chn_attr(audioDev, audioChn, &chnAttr);
    if(ret != HI_SUCCESS)
    {
        ss_print("[%s %d] get chn attr error; errcode : %x\n", __func__, __LINE__, ret);
    }
    else
    {
        printf("mode : %d\n", chnAttr.mode);
    }

    return 0;
}

int SD3403_AI::DevAttr(ot_aio_attr &attr)
{
    attr.sample_rate = audioRate;
    attr.bit_width = audioBt;
    attr.snd_mode = audioSndMode;

    return 0;
}

int SD3403_AI::Sd343_Ai_Deinit(void)
{
    hi_s32 ret = 0;

    if (vqeRunning == HI_TRUE) {
        ret = hi_mpi_ai_disable_vqe(audioDev, audioChn);
        if(ret != HI_SUCCESS)
            ss_print("[%s %d] ai vqe disable error; errcode : %x\n", __func__, __LINE__, ret);
        vqeRunning = HI_FALSE;
    }

    ret = hi_mpi_ai_disable_chn(audioDev, audioChn);
    if(ret != HI_SUCCESS)
    {
        ss_print("[%s %d] ai chn disable error; errcode : %x\n", __func__, __LINE__, ret);
        return ret;
    }
    ret = hi_mpi_ai_disable(audioDev);
    if(ret != HI_SUCCESS)
    {
        ss_print("[%s %d] ai dev disable error; errcode : %x\n", __func__, __LINE__, ret);
        return ret;
    }
    return 0;
}