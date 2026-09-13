#include "decode.hpp"
#include "hi_mpi_sys.h"
#include "hi_mpi_vb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK_RETURN(ret, name)                                                 \
    do                                                                          \
    {                                                                           \
        if (ret != HI_SUCCESS)                                                  \
        {                                                                       \
            printf("\033[0;31m%s failed at %s: LINE: %d with %#x!\033[0;39m\n", \
                   name, __FUNCTION__, __LINE__, ret);                          \
            return ret;                                                         \
        }                                                                       \
    } while (0)

#define VDEC_CHN 0
#define MAX_STREAM_BUF_SIZE (10 * 1024 * 1024)
#define FRAME_BUF_CNT 8
#define REF_FRAME_NUM 2
#define DISPLAY_FRAME_NUM 2

SD3403_VDEC_Decoder::SD3403_VDEC_Decoder()
    : m_vdecChn(VDEC_CHN), m_isInit(HI_FALSE), m_payloadType(HI_PT_H265), m_width(1920), m_height(1080)
{
    memset(&m_chnAttr, 0, sizeof(m_chnAttr));
}

SD3403_VDEC_Decoder::~SD3403_VDEC_Decoder()
{
    if (m_isInit)
    {
        DeInit();
    }
}

hi_s32 SD3403_VDEC_Decoder::Init(hi_u32 width, hi_u32 height, hi_payload_type type)
{
    hi_s32 ret;

    m_width = width;
    m_height = height;
    m_payloadType = type;

    ret = InitSysAndVb(width, height);
    CHECK_RETURN(ret, "InitSysAndVb");

    ret = InitVdec(width, height, type);
    CHECK_RETURN(ret, "InitVdec");

    m_isInit = HI_TRUE;
    return HI_SUCCESS;
}

hi_void SD3403_VDEC_Decoder::get_default_vb_config(hi_size *size, hi_vb_cfg *vb_cfg, hi_vi_video_mode video_mode, hi_u32 yuv_cnt, hi_u32 raw_cnt)
{
    hi_vb_calc_cfg calc_cfg;
    hi_pic_buf_attr buf_attr;

    (hi_void) memset_s(vb_cfg, sizeof(hi_vb_cfg), 0, sizeof(hi_vb_cfg));
    vb_cfg->max_pool_cnt = 1; /* 128 blks */

    /* default YUV pool: SP420 + compress_seg */
    buf_attr.width = size->width;
    buf_attr.height = size->height;
    buf_attr.align = HI_DEFAULT_ALIGN;
    buf_attr.bit_width = HI_DATA_BIT_WIDTH_8;
    buf_attr.pixel_format = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    buf_attr.compress_mode = OT_COMPRESS_MODE_SEG;
    hi_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);

    vb_cfg->common_pool[0].blk_size = calc_cfg.vb_size;
    vb_cfg->common_pool[0].blk_cnt = yuv_cnt;

    /* default raw pool: raw12bpp + compress_line */
    buf_attr.pixel_format = HI_PIXEL_FORMAT_RGB_BAYER_12BPP;
    buf_attr.compress_mode = (video_mode == HI_VI_VIDEO_MODE_NORM ? HI_COMPRESS_MODE_LINE : HI_COMPRESS_MODE_NONE);
    hi_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    vb_cfg->common_pool[1].blk_size = calc_cfg.vb_size;
    vb_cfg->common_pool[1].blk_cnt = raw_cnt;
}

hi_s32 SD3403_VDEC_Decoder::sys_init_with_vb_supplement(const hi_vb_cfg *vb_conf, hi_u32 supplement_config)
{
    hi_s32 ret;
    hi_vb_supplement_cfg supplement_conf = {0};

    hi_mpi_sys_exit();
    hi_mpi_vb_exit();

    if (vb_conf == HI_NULL)
    {
        printf("input parameter is null, it is invalid!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_vb_set_cfg(vb_conf);
    if (ret != HI_SUCCESS)
    {
        printf("hi_mpi_vb_set_conf failed!\n");
        return HI_FAILURE;
    }

    supplement_conf.supplement_cfg = supplement_config;
    ret = hi_mpi_vb_set_supplement_cfg(&supplement_conf);
    if (ret != HI_SUCCESS)
    {
        printf("hi_mpi_vb_set_supplement_conf failed!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_vb_init();
    if (ret != HI_SUCCESS)
    {
        printf("hi_mpi_vb_init failed!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_sys_init();
    if (ret != HI_SUCCESS)
    {
        printf("hi_mpi_sys_init failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VDEC_Decoder::vdec_config_vb_pool(hi_bool *pic_buf_alloc, hi_bool *tmv_buf_alloc, hi_u32 pic_buf_size, hi_u32 tmv_buf_size, hi_vb_cfg *vb_conf)
{
    hi_u32 i, j;
    hi_bool find_flag;
    hi_s32 pos = 0;
    hi_s32 frame_buf_cnt = (m_payloadType == HI_PT_JPEG) ? 3 : 5;
    hi_s32 ref_frame_num = (m_payloadType == HI_PT_JPEG) ? 0 : 2;

    /* pic_buffer */
    for (j = 0; j < HI_VB_MAX_COMMON_POOLS; j++)
    {
        find_flag = HI_FALSE;
        for (i = 0; (i < chn_num) && (i < HI_VDEC_MAX_CHN_NUM); i++)
        {
            if ((find_flag == HI_FALSE) && (pic_buf_size != 0) && (*pic_buf_alloc == HI_FALSE))
            {
                vb_conf->common_pool[j].blk_size = pic_buf_size;
                vb_conf->common_pool[j].blk_cnt = frame_buf_cnt;
                *pic_buf_alloc = HI_TRUE;
                find_flag = HI_TRUE;
                pos = j;
            }

            if ((find_flag == HI_TRUE) && (*pic_buf_alloc == HI_FALSE) &&
                (vb_conf->common_pool[j].blk_size == pic_buf_size))
            {
                vb_conf->common_pool[j].blk_cnt += frame_buf_cnt;
                *pic_buf_alloc = HI_TRUE;
            }
        }
    }

    /* tmv_buffer */
    for (j = pos + 1; j < HI_VB_MAX_COMMON_POOLS; j++)
    {
        find_flag = HI_FALSE;
        for (i = 0; (i < chn_num) && (i < HI_VDEC_MAX_CHN_NUM); i++)
        {
            if ((find_flag == HI_FALSE) && (tmv_buf_size != 0) && (*tmv_buf_alloc == HI_FALSE))
            {
                vb_conf->common_pool[j].blk_size = tmv_buf_size;
                vb_conf->common_pool[j].blk_cnt = ref_frame_num + 1;
                *tmv_buf_alloc = HI_TRUE;
                find_flag = HI_TRUE;
                pos = j;
            }

            if ((find_flag == HI_TRUE) && (*tmv_buf_alloc == HI_FALSE) &&
                (vb_conf->common_pool[j].blk_size == tmv_buf_size))
            {
                vb_conf->common_pool[j].blk_cnt += ref_frame_num + 1;
                *tmv_buf_alloc = HI_TRUE;
            }
        }
    }
    vb_conf->max_pool_cnt = pos + 1;
    return i - 1;
}

hi_u32 SD3403_VDEC_Decoder::vdec_cal_vb_size(hi_u32 chn_num, hi_payload_type type, hi_u32 &pic_size, hi_u32 &tmv_size)
{
    hi_u32 i;
    hi_pic_buf_attr buf_attr = {0};
    for (i = 0; (i < chn_num); i++)
    {
        buf_attr.align = 0;
        buf_attr.height = 3840;
        buf_attr.width = 2160;
        if (type == HI_PT_H265)
        {
            buf_attr.bit_width = HI_DATA_BIT_WIDTH_8;
            buf_attr.pixel_format = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
            pic_size = hi_vdec_get_pic_buf_size(type, &buf_attr);
            tmv_size =
                hi_vdec_get_tmv_buf_size(type, buf_attr.width, buf_attr.height);
        }
        else if (type == HI_PT_H264)
        {
            buf_attr.bit_width = HI_DATA_BIT_WIDTH_8;
            buf_attr.pixel_format = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
            pic_size = hi_vdec_get_pic_buf_size(type, &buf_attr);
        }
        else
        {
            buf_attr.bit_width = HI_DATA_BIT_WIDTH_8;
            buf_attr.pixel_format = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
            pic_size = hi_vdec_get_pic_buf_size(type, &buf_attr);
        }
    }
    return 0;
}

hi_s32 SD3403_VDEC_Decoder::init_module_vdec_vb(void)
{
    hi_u32 pic_size, tmv_size;
    hi_bool pic_buf_alloc = HI_FALSE, tmv_buf_alloc = HI_FALSE;
    hi_vb_cfg vb_conf;
    hi_s32 ret = 0;

    (hi_void) memset_s(&vb_conf, sizeof(hi_vb_cfg), 0, sizeof(hi_vb_cfg));
    vdec_cal_vb_size(1, HI_PT_H265, pic_size, tmv_size);
    vdec_config_vb_pool(&pic_buf_alloc, &tmv_buf_alloc, pic_size, tmv_size, &vb_conf);

    if (g_vdec_vb_src == HI_VB_SRC_MOD)
    {
        hi_mpi_vb_exit_mod_common_pool(HI_VB_UID_VDEC);
        ret = hi_mpi_vb_set_mod_pool_cfg(HI_VB_UID_VDEC, &vb_conf);
        if (ret != HI_SUCCESS)
        {
            printf("vb set mod pool error ! code : %x\n", ret);
        }
        ret = hi_mpi_vb_init_mod_common_pool(HI_VB_UID_VDEC);
        if (ret != HI_SUCCESS)
        {
            printf("vb exit mod common pool fail for 0x%x\n", ret);
            hi_mpi_vb_exit_mod_common_pool(HI_VB_UID_VDEC);
            return HI_FAILURE;
        }
    }
}

hi_s32 SD3403_VDEC_Decoder::InitSysAndVb(hi_u32 width, hi_u32 height)
{
    hi_s32 ret;
    hi_size size;
    hi_vb_cfg vb_cfg;
    hi_u32 supplement_config;
    size.width = 1920;
    size.height = 1080;
    get_default_vb_config(&size, &vb_cfg, HI_VI_VIDEO_MODE_NORM, 15, 8);
    supplement_config = HI_VB_SUPPLEMENT_BNR_MOT_MASK;
    ret = sys_init_with_vb_supplement(&vb_cfg, supplement_config);
    if (ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }
    init_module_vdec_vb();

    return HI_SUCCESS;
}

hi_s32 SD3403_VDEC_Decoder::InitVdec(hi_u32 width, hi_u32 height, hi_payload_type type)
{
    hi_s32 ret;
    hi_vdec_chn_attr m_chnAttr;
    // hi_payload_type type = OT_PT_H265;
    hi_vdec_send_mode mode = HI_VDEC_SEND_MODE_STREAM;
    hi_vdec_mod_param mod_param;
    hi_vdec_chn_param chn_param;
    hi_pic_buf_attr buf_attr = {0};

    hi_mpi_vdec_get_mod_param(&mod_param);
    mod_param.vb_src = g_vdec_vb_src;
    hi_mpi_vdec_set_mod_param(&mod_param);

    buf_attr.align = 0;
    buf_attr.height = 3840;
    buf_attr.width = 2160;

    m_chnAttr.type = type;
    m_chnAttr.mode = mode;
    m_chnAttr.pic_width = width;
    m_chnAttr.pic_height = height;
    m_chnAttr.stream_buf_size = width * height * 20;

    if (type == HI_PT_H264 || type == HI_PT_H265)
    {
        m_chnAttr.frame_buf_cnt = 6;
        m_chnAttr.video_attr.ref_frame_num = 2;
        m_chnAttr.video_attr.temporal_mvp_en = HI_TRUE;
        if (type == HI_PT_H264)
            m_chnAttr.video_attr.temporal_mvp_en = HI_FALSE;

        buf_attr.bit_width = HI_DATA_BIT_WIDTH_8;
        buf_attr.pixel_format = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        m_chnAttr.frame_buf_size = hi_vdec_get_pic_buf_size(m_chnAttr.type, &buf_attr);
    }
    else
    {
        printf("Unsupported payload type %d\n", type);
        return HI_FAILURE;
    }

    ret = hi_mpi_vdec_create_chn(0, &m_chnAttr);
    CHECK_RETURN(ret, "hi_mpi_vdec_create_chn");

    hi_mpi_vdec_get_chn_param(0, &chn_param);
    chn_param.video_param.dec_mode = HI_VIDEO_DEC_MODE_IP;
    chn_param.video_param.compress_mode = HI_COMPRESS_MODE_NONE;
    chn_param.video_param.video_format = HI_VIDEO_FORMAT_LINEAR;
    chn_param.video_param.out_order = HI_VIDEO_OUT_ORDER_DEC;
    chn_param.video_param.slice_input_en = HI_FALSE;
    chn_param.display_frame_num = 2;
    hi_mpi_vdec_set_chn_param(0, &chn_param);
    hi_mpi_vdec_start_recv_stream(0);

    return HI_SUCCESS;
}

hi_s32 SD3403_VDEC_Decoder::DeInit()
{
    hi_s32 ret;

    if (m_isInit)
    {
        ret = hi_mpi_vdec_stop_recv_stream(m_vdecChn);
        if (ret != HI_SUCCESS)
        {
            printf("hi_mpi_vdec_stop_recv_stream failed!\n");
        }

        ret = hi_mpi_vdec_destroy_chn(m_vdecChn);
        if (ret != HI_SUCCESS)
        {
            printf("hi_mpi_vdec_destroy_chn failed!\n");
        }

        hi_mpi_vb_exit();
        hi_mpi_sys_exit();

        m_isInit = HI_FALSE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VDEC_Decoder::CutH264Frame(hi_u8 *buf, hi_s32 *readLen)
{
    hi_s32 i;
    hi_bool findStart = HI_FALSE;
    hi_bool findEnd = HI_FALSE;

    for (i = 0; i < *readLen - 8; i++)
    {
        hi_u32 tmp = buf[i + 3] & 0x1F;
        bool newPic = (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1 &&
                       (((tmp == 0x5 || tmp == 0x1) && ((buf[i + 4] & 0x80) == 0x80)) ||
                        (tmp == 20 && (buf[i + 7] & 0x80) == 0x80)));

        if (newPic)
        {
            findStart = HI_TRUE;
            i += 8;
            break;
        }
    }

    for (; i < *readLen - 8; i++)
    {
        hi_u32 tmp = buf[i + 3] & 0x1F;
        bool newPic = (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1 &&
                       (tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 ||
                        ((tmp == 5 || tmp == 1) && ((buf[i + 4] & 0x80) == 0x80)) ||
                        (tmp == 20 && (buf[i + 7] & 0x80) == 0x80)));

        if (newPic)
        {
            findEnd = HI_TRUE;
            break;
        }
    }

    if (i > 0)
    {
        *readLen = i;
    }

    if (findStart == HI_FALSE)
    {
        printf("Cannot find H264 start code!\n");
    }

    if (findEnd == HI_FALSE)
    {
        *readLen = i + 8;
    }

    return 0;
}

hi_s32 SD3403_VDEC_Decoder::CutH265Frame(hi_u8 *buf, hi_s32 *readLen)
{
    hi_s32 i;
    hi_bool findStart = HI_FALSE;
    hi_bool findEnd = HI_FALSE;

    for (i = 0; i < *readLen - 6; i++)
    {
        hi_u32 tmp = (buf[i + 3] & 0x7E) >> 1;
        bool newPic = (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1 &&
                       (tmp <= 21) && ((buf[i + 5] & 0x80) == 0x80));

        if (newPic)
        {
            findStart = HI_TRUE;
            i += 6;
            break;
        }
    }

    for (; i < *readLen - 6; i++)
    {
        hi_u32 tmp = (buf[i + 3] & 0x7E) >> 1;
        bool newPic = (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1 &&
                       (tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39 || tmp == 40 ||
                        ((tmp <= 21) && (buf[i + 5] & 0x80) == 0x80)));

        if (newPic)
        {
            findEnd = HI_TRUE;
            break;
        }
    }

    if (i > 0)
    {
        *readLen = i;
    }

    if (findStart == HI_FALSE)
    {
        printf("Cannot find H265 start code!\n");
    }

    if (findEnd == HI_FALSE)
    {
        *readLen = i + 6;
    }

    return 0;
}

// hi_s32 SD3403_VDEC_Decoder::ReadOneFrame(FILE *fp, hi_u8 *buf, hi_u32 bufSize, hi_s32 *outLen)
// {
//     hi_s32 readLen = fread(buf, 1, bufSize, fp);
//     if (readLen <= 0)
//     {
//         *outLen = 0;
//         return HI_FAILURE;
//     }

//     if (m_payloadType == HI_PT_H264)
//     {
//         CutH264Frame(buf, &readLen);
//     }
//     else if (m_payloadType == HI_PT_H265)
//     {
//         CutH265Frame(buf, &readLen);
//     }

//     *outLen = readLen;
//     return HI_SUCCESS;
// }

// hi_s32 SD3403_VDEC_Decoder::SaveYuvFrame(const hi_video_frame_info* frameInfo, FILE* yuvFd)
// {
//     hi_s32 ret;
//     hi_u64 physAddr;
//     hi_u8* virtAddr;
//     hi_u32 width = frameInfo->video_frame.width;
//     hi_u32 height = frameInfo->video_frame.height;
//     hi_u64 yStride = frameInfo->video_frame.virt_mode.clr_fmt.clr_plannar.y_stride;
//     hi_u64 uvStride = frameInfo->video_frame.virt_mode.clr_fmt.clr_semi_planar.uv_stride;

//     physAddr = frameInfo->video_frame.phys_addr[0];
//     virtAddr = (hi_u8*)hi_mpi_sys_mmap(physAddr, width * height * 3 / 2);
//     if (virtAddr == nullptr) {
//         printf("Failed to mmap frame buffer\n");
//         return HI_FAILURE;
//     }

//     hi_u8* yData = virtAddr;
//     hi_u8* uvData = virtAddr + yStride * height;

//     fwrite(yData, 1, width * height, yuvFd);

//     for (hi_u32 i = 0; i < height / 2; i++) {
//         fwrite(uvData + i * uvStride, 1, width, yuvFd);
//     }

//     hi_mpi_sys_munmap(virtAddr, width * height * 3 / 2);

//     return HI_SUCCESS;
// }

hi_void SD3403_VDEC_Decoder::vdec_print_chn_status(hi_s32 chn, hi_vdec_chn_status status)
{
    printf("\033[0;33m ---------------------------------------------------------------\033[0;39m\n");
    printf("\033[0;33m chn:%d, type:%d, start:%d, decode_frames:%d, left_pics:%d, left_bytes:%d, "
           "left_frames:%d, recv_frames:%d  \033[0;39m\n",
           chn, (status).type, (status).is_started, (status).dec_stream_frames, (status).left_decoded_frames,
           (status).left_stream_bytes, (status).left_stream_frames, (status).recv_stream_frames);
    printf("\033[0;33m format_err:%d,    pic_size_err_set:%d,  stream_unsprt:%d,  pack_err:%d, "
           "set_pic_size_err:%d,  ref_err_set:%d,  pic_buf_size_err_set:%d  \033[0;39m\n",
           (status).dec_err.format_err, (status).dec_err.set_pic_size_err, (status).dec_err.stream_unsupport,
           (status).dec_err.pack_err, (status).dec_err.set_protocol_num_err, (status).dec_err.set_ref_num_err,
           (status).dec_err.set_pic_buf_size_err);
    printf("\033[0;33m -----------------------------------------------------------------\033[0;39m\n");
    return;
}

void SD3403_VDEC_Decoder::send_stream(FILE *inputFp)
{
    hi_u8 *streamBuf = nullptr;

    hi_u32 frameCount = 0;
    hi_bool endOfStream = HI_FALSE;
    hi_s32 usedBytes = 0;
    hi_s32 readLen;
    hi_vdec_stream stream;

    hi_s32 ret;

    streamBuf = (hi_u8 *)malloc(MAX_STREAM_BUF_SIZE);
    if (!streamBuf)
    {
        printf("Failed to allocate stream buffer\n");
        fclose(inputFp);
        return;
    }

    printf("Resolution: %ux%u, Type: %s\n", m_width, m_height,
           m_payloadType == HI_PT_H264 ? "H264" : "H265");

    while (!send_run)
    {

        // fseek(inputFp, usedBytes, SEEK_SET);
        readLen = 0;
        readLen = fread(streamBuf, 1, 64 * 1024, inputFp);
        printf("read input file len : %d\n", readLen);
        if (readLen <= 0)
        {
            printf("====>read input file error!\n");
            send_run = HI_TRUE;
        }
        else
        {

            if (m_payloadType == HI_PT_H264)
            {
                printf("====> video type is h264\n");
                // CutH264Frame(streamBuf, &readLen);
            }
            else if (m_payloadType == HI_PT_H265)
            {
                printf("====> video type is h265\n");
                // CutH265Frame(streamBuf, &readLen);
            }

            stream.addr = streamBuf;
            stream.len = readLen;
            stream.pts = frameCount * 33333;
            stream.end_of_frame = HI_FALSE;
            stream.end_of_stream = HI_FALSE;
            stream.need_display = HI_TRUE;

            ret = hi_mpi_vdec_send_stream(m_vdecChn, &stream, 1000);
            if (ret != HI_SUCCESS)
            {
                printf("=========>send frame error !error coede : %x\n", ret);
                usleep(10000);
                continue;
            }

            usedBytes += readLen;
        }

        printf("====>usedBytes : %d\n", usedBytes);
        usleep(33000);
    }

    (hi_void) memset_s(&stream, sizeof(hi_vdec_stream), 0, sizeof(hi_vdec_stream));
    stream.addr = nullptr;
    stream.len = 0;
    stream.pts = 0;
    stream.end_of_frame = HI_FALSE;
    stream.end_of_stream = HI_TRUE;
    stream.need_display = HI_TRUE;
    hi_mpi_vdec_send_stream(m_vdecChn, &stream, -1);

    printf("Decoding completed! Total frames: %u\n", frameCount);
    send_flag = HI_TRUE;
    free(streamBuf);
    fclose(inputFp);

    return;
}

void SD3403_VDEC_Decoder::recv_stream(FILE *outputFp)
{
    hi_s32 ret;
    hi_vdec_chn_status status;
    hi_video_frame_info frameInfo;
    int count = 0;

    ret = hi_mpi_vdec_query_status(m_vdecChn, &status);
    if (ret != HI_SUCCESS)
    {
        printf(" hi_mpi_vdec_query_status error ! code : %x\n", ret);
    }
    vdec_print_chn_status(m_vdecChn, status);
    printf("status.left_decoded_frames : %d\n", status.left_decoded_frames);
    if (status.dec_err.pack_err > 0)
    {
        printf("Stream packet error count: %d\n", status.dec_err.pack_err);
    }

    while (!recv_run)
    {
        ret = hi_mpi_vdec_query_status(m_vdecChn, &status);
        if (ret != HI_SUCCESS)
        {
            printf(" hi_mpi_vdec_query_status error ! code : %x\n", ret);
        }
        vdec_print_chn_status(m_vdecChn, status);
        memset(&frameInfo, 0, sizeof(frameInfo));
        ret = hi_mpi_vdec_get_frame(m_vdecChn, &frameInfo, nullptr, 1000);
        if (ret == HI_SUCCESS)
        {
            // ret = SaveYuvFrame(&frameInfo, outputFp);
            // if (ret == HI_SUCCESS) {
            //     frameCount++;
            //     if (frameCount % 30 == 0) {
            //         printf("Decoded %u frames...\n", frameCount);
            //     }
            // }
            printf("===>get dec frame success!\n");
            if (!outputFp)
            {
                printf("open file failed\n");
                hi_mpi_vdec_release_frame(m_vdecChn, &frameInfo);
                return;
            }

            ot_video_frame *vf = &frameInfo.video_frame;
            td_u32 width = vf->width;
            td_u32 height = vf->height;
            td_u32 stride_y = vf->stride[0];
            td_u32 stride_uv = vf->stride[1];

            if (vf->virt_addr[0] == NULL)
            {
                vf->virt_addr[0] = hi_mpi_sys_mmap(vf->phys_addr[0], stride_y * height);
            }
            if (vf->virt_addr[1] == NULL)
            {
                vf->virt_addr[1] = hi_mpi_sys_mmap(vf->phys_addr[1], stride_uv * (height / 2));
            }

            // 写入 Y 分量（按实际宽度）
            for (td_u32 i = 0; i < height; i++)
            {
                fwrite(vf->virt_addr[0] + i * stride_y, 1, width, outputFp);
            }

            // 写入 UV 分量（按实际宽度，高度为 height/2）
            for (td_u32 i = 0; i < height / 2; i++)
            {
                fwrite(vf->virt_addr[1] + i * stride_uv, 1, width, outputFp);
            }
            // fclose(outputFp);

            hi_mpi_vdec_release_frame(m_vdecChn, &frameInfo);
            count = 0;
        }
        else
        {
            count++;
            printf("get dec frame error! error code : %x\n", ret);
            usleep(1000);
        }
        if (count >= 3)
        {
            recv_run = HI_TRUE;
        }
    };
    printf("=================>recv ok!\n");
    recv_flag = HI_TRUE;
    fclose(outputFp);
    hi_mpi_vdec_release_frame(m_vdecChn, &frameInfo);

    return;
}

hi_s32 SD3403_VDEC_Decoder::DecodeFile(const char *inputFile, const char *outputFile, hi_u32 maxFrames)
{
    hi_s32 ret;
    FILE *inputFp = nullptr;
    FILE *outputFp = nullptr;

    inputFp = fopen(inputFile, "rb");
    if (!inputFp)
    {
        printf("Failed to open input file: %s\n", inputFile);
        return HI_FAILURE;
    }

    outputFp = fopen(outputFile, "wb");
    if (!outputFp)
    {
        printf("Failed to open output file: %s\n", outputFile);
        fclose(inputFp);
        return HI_FAILURE;
    }

    send_thread = new std::thread(&SD3403_VDEC_Decoder::send_stream, this, inputFp);
    recv_thread = new std::thread(&SD3403_VDEC_Decoder::recv_stream, this, outputFp);

    return HI_SUCCESS;
}

hi_s32 SD3403_VDEC_Decoder::DecodeStream(const char *inputFile, const char *outputFile)
{
    return DecodeFile(inputFile, outputFile, 0);
}

static void PrintUsage(const char *progName)
{
    printf("\n==========================================\n");
    printf("VDEC Decoder Usage:\n");
    printf("==========================================\n");
    printf("Usage: %s <input_file> <output_file> [width] [height] [type]\n", progName);
    printf("\nParameters:\n");
    printf("  input_file   : Input H.264/H.265 bitstream file\n");
    printf("  output_file  : Output YUV file (NV12 format)\n");
    printf("  width        : Video width (default: 1920)\n");
    printf("  height       : Video height (default: 1080)\n");
    printf("  type         : Codec type 0=H264, 1=H265 (default: 1)\n");
    printf("\nExamples:\n");
    printf("  %s 1.h264 1.yuv 1920 1080 0\n", progName);
    printf("  %s 2.h265 2.yuv 1920 1080 1\n", progName);
    printf("==========================================\n\n");
}

int main(int argc, char *argv[])
{
    hi_s32 ret;
    const char *inputFile = nullptr;
    const char *outputFile = nullptr;
    hi_u32 width = 1920;
    hi_u32 height = 1080;
    hi_payload_type type = HI_PT_H265;

    if (argc < 3)
    {
        PrintUsage(argv[0]);
        return -1;
    }

    inputFile = argv[1];
    outputFile = argv[2];

    if (argc >= 4)
    {
        width = atoi(argv[3]);
    }
    if (argc >= 5)
    {
        height = atoi(argv[4]);
    }
    if (argc >= 6)
    {
        type = (atoi(argv[5]) == 0) ? HI_PT_H264 : HI_PT_H265;
    }

    printf("Decoder Configuration:\n");
    printf("  Input : %s\n", inputFile);
    printf("  Output: %s\n", outputFile);
    printf("  Size  : %ux%u\n", width, height);
    printf("  Type  : %s\n", type == HI_PT_H264 ? "H.264" : "H.265");
    printf("\n");

    SD3403_VDEC_Decoder decoder;
    ret = decoder.Init(width, height, type);
    if (ret != HI_SUCCESS)
    {
        printf("Decoder init failed!\n");
        return -1;
    }

    ret = decoder.DecodeStream(inputFile, outputFile);
    if (ret != HI_SUCCESS)
    {
        printf("Decode failed!\n");
        return -1;
    }

    while (!decoder.send_flag || !decoder.recv_flag)
        ;
    if (decoder.send_thread->joinable())
    {
        decoder.send_thread->join();
        decoder.send_thread = nullptr; // 重置为空线程
    }
    if (decoder.recv_thread->joinable())
    {
        decoder.recv_thread->join();
        decoder.recv_thread = nullptr; // 重置为空线程
    }

    printf("Done!\n");
    return 0;
}
