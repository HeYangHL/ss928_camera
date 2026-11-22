#include "sd3403_aenc.hpp"

int SD3403_AENC::Sd343_Aenc_Init(void)
{
    hi_s32 ret = 0;
    hi_aenc_chn_attr aencAttr = {};
    hi_aenc_attr_aac aenc_aac = {};
    hi_aenc_attr_g726 aenc_g726 = {};

    aencAttr.type = encType;
    aencAttr.buf_size = 30;
    aencAttr.point_num_per_frame = point_num_per_frame;

    if (encType == HI_PT_AAC)
    {
        aenc_aac.aac_type = aacType;
        aenc_aac.bit_rate = aacBps;
        aenc_aac.bit_width = HI_AUDIO_BIT_WIDTH_16;
        aenc_aac.sample_rate = audioRate;
        aenc_aac.snd_mode = audioSndMode;
        aenc_aac.transport_type = aacTransportType;
        aenc_aac.band_width = 0;
        aencAttr.value = &aenc_aac;
    }
    else if (encType == HI_PT_G726)
    {
        aenc_g726.g726bps = HI_G726_40K;
        aencAttr.value = &aenc_g726;
    }
    else
    {
        ss_print("[%s %d] unsupported payload type: %d\n", __func__, __LINE__, encType);
        return -1;
    }

    ret = hi_mpi_aenc_create_chn(aencChn, &aencAttr);
    if (ret != HI_SUCCESS)
    {
        ss_print("[%s %d] create aenc chn error; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }
    ret = Sd3403_Ai_Bind_Aenc();
    if (ret != 0)
        return -1;

    return 0;
}
int SD3403_AENC::Sd343_Aenc_Deinit(void)
{
    hi_s32 ret = 0;

    ret = Sd3403_Ai_UnBind_Aenc();
    if (ret != 0)
        ss_print("[%s %d] AI unbind Aenc failed, continue destroy\n", __func__, __LINE__);

    ret = hi_mpi_aenc_destroy_chn(aencChn);
    if (ret != HI_SUCCESS)
    {
        ss_print("[%s %d] Aenc destroy chn error; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }
    return 0;
}

int SD3403_AENC::Sd3403_Ai_Bind_Aenc(void)
{
    hi_s32 ret = 0;
    hi_mpp_chn srcChn;
    hi_mpp_chn dstChn;

    srcChn.mod_id = HI_ID_AI;
    srcChn.dev_id = 0;
    srcChn.chn_id = 0;

    dstChn.mod_id = HI_ID_AENC;
    dstChn.dev_id = 0;
    dstChn.chn_id = 0;

    ret = hi_mpi_sys_bind(&srcChn, &dstChn);
    if (ret != HI_SUCCESS)
    {
        ss_print("[%s %d] AI bind Aenc error; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }
    return 0;
}

int SD3403_AENC::Sd3403_Ai_UnBind_Aenc(void)
{
    hi_s32 ret = 0;
    hi_mpp_chn srcChn;
    hi_mpp_chn dstChn;

    srcChn.mod_id = HI_ID_AI;
    srcChn.dev_id = 0;
    srcChn.chn_id = 0;

    dstChn.mod_id = HI_ID_AENC;
    dstChn.dev_id = 0;
    dstChn.chn_id = 0;

    ret = hi_mpi_sys_unbind(&srcChn, &dstChn);
    if (ret != HI_SUCCESS)
    {
        ss_print("[%s %d] AI unbind Aenc error; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }
    return 0;
}

int SD3403_AENC::Sd3403_Get_Aenc_Stream(char *data, int cap, uint64_t *pts, int *seq, int ms)
{
    hi_s32 ret = 0;
    int AencFd = 0;
    hi_audio_stream audioStream;

    if (data == HI_NULL || cap <= 0)
        return -1;

    AencFd = hi_mpi_aenc_get_fd(aencChn);
    if (AencFd < 0)
    {
        ss_print("[%s %d] get aenc fd error; fd : %d\n", __func__, __LINE__, AencFd);
        return -1;
    }
    fd_set read_fds;
    struct timeval timeoutVal;
    FD_ZERO(&read_fds);
    FD_SET(AencFd, &read_fds);
    timeoutVal.tv_sec = 0;
    timeoutVal.tv_usec = 50000;
    ret = select(AencFd + 1, &read_fds, NULL, NULL, &timeoutVal);
    if (ret < 0)
    {
        ss_print("select failed!\n");
        return -1;
    }
    else if (ret == 0)
    {
        ss_print("get aenc chn_%d stream time out.\n", aencChn);
        return 0;
    }
    else
    {
        if (FD_ISSET(AencFd, &read_fds))
        {
            ret = hi_mpi_aenc_get_stream(aencChn, &audioStream, ms);
            if (ret != HI_SUCCESS)
            {
                ss_print("[%s %d] get aenc stream error; errcode : %x\n", __func__, __LINE__, ret);
                return -1;
            }
            if (audioStream.len > (hi_u32)cap)
            {
                ss_print("[%s %d] aenc frame too large: %u > %d\n",
                         __func__, __LINE__, audioStream.len, cap);
                hi_mpi_aenc_release_stream(aencChn, &audioStream);
                return -1;
            }
            memcpy(data, audioStream.stream, audioStream.len);
            *pts = audioStream.time_stamp;
            *seq = audioStream.seq;
            ret = hi_mpi_aenc_release_stream(aencChn, &audioStream);
            if (ret != HI_SUCCESS)
            {
                ss_print("[%s %d] release aenc stream error; errcode : %x\n", __func__, __LINE__, ret);
                return -1;
            }
            return audioStream.len;
        }
    }

    return 0;
}

int SD3403_AENC::Sd343_Aenc_Aac_Init(void)
{
    hi_s32 ret;

    ret = hi_mpi_aenc_aac_init();
    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] aac init error; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    return 0;
}

int SD3403_AENC::Sd343_Aenc_Aac_DeInit(void)
{
    hi_s32 ret = hi_mpi_aenc_aac_deinit();

    if (ret != HI_SUCCESS) {
        ss_print("[%s %d] aac deinit error; errcode : %x\n", __func__, __LINE__, ret);
        return -1;
    }

    return 0;
}
