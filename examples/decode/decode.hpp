#ifndef _SD3403_DECODE_HPP_
#define _SD3403_DECODE_HPP_

#include "hi_common.h"
#include "hi_common_video.h"
#include "hi_common_vdec.h"
#include "hi_mpi_vdec.h"
#include "hi_mpi_sys.h"
#include "hi_mpi_vb.h"
#include "hi_buffer.h"
#include <string>
#include <cstdint>
#include <thread>

class SD3403_VDEC_Decoder {
public:
    SD3403_VDEC_Decoder();
    ~SD3403_VDEC_Decoder();

    hi_s32 Init(hi_u32 width, hi_u32 height, hi_payload_type type);
    hi_s32 DeInit();
    hi_s32 DecodeFile(const char* inputFile, const char* outputFile, hi_u32 maxFrames = 0);
    hi_s32 DecodeStream(const char* inputFile, const char* outputFile);
    hi_void get_default_vb_config(hi_size *size, hi_vb_cfg *vb_cfg, hi_vi_video_mode video_mode, hi_u32 yuv_cnt, hi_u32 raw_cnt);
    hi_s32 sys_init_with_vb_supplement(const hi_vb_cfg *vb_conf, hi_u32 supplement_config);
    hi_u32 vdec_cal_vb_size(hi_u32 chn_num, hi_payload_type type, hi_u32 &pic_size, hi_u32 &tmv_size);
    hi_void vdec_print_chn_status(hi_s32 chn, hi_vdec_chn_status status);
    hi_s32 vdec_config_vb_pool(hi_bool *pic_buf_alloc, hi_bool *tmv_buf_alloc, hi_u32 pic_buf_size, hi_u32 tmv_buf_size, hi_vb_cfg *vb_conf);

private:
    hi_s32 InitSysAndVb(hi_u32 width, hi_u32 height);
    hi_s32 InitVdec(hi_u32 width, hi_u32 height, hi_payload_type type);
    // hi_s32 SaveYuvFrame(const hi_video_frame_info* frameInfo, FILE* yuvFd);
    hi_s32 ReadOneFrame(FILE* fp, hi_u8* buf, hi_u32 bufSize, hi_s32* outLen);
    hi_s32 CutH264Frame(hi_u8* buf, hi_s32* readLen);
    hi_s32 CutH265Frame(hi_u8* buf, hi_s32* readLen);
    hi_s32 init_module_vdec_vb(void);
    void recv_stream(FILE *outputFp);
    void send_stream(FILE *inputFp);

    hi_vdec_chn_attr m_chnAttr;
    hi_vdec_chn m_vdecChn;
    hi_bool m_isInit;
    hi_payload_type m_payloadType;
    hi_u32 m_width;
    hi_u32 m_height;
    int chn_num = 1;
    hi_vb_src g_vdec_vb_src = HI_VB_SRC_MOD;
public:
    hi_bool send_run = HI_FALSE;
    hi_bool recv_run = HI_FALSE;
    std::thread *send_thread = NULL;
    std::thread *recv_thread = NULL;
    hi_bool send_flag = HI_FALSE;
    hi_bool recv_flag = HI_FALSE;
};

#endif
