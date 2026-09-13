#ifndef _SD3403_VENC_HPP_
#define _SD3403_VENC_HPP_

#include "def.h"

class SD3403_VENC{
public:
    SD3403_VENC();
    ~SD3403_VENC();
    hi_s32 start_venc(hi_venc_chn venc_chn[], hi_u32 chn_num, const hi_size in_size);
    hi_pic_size sys_get_pic_enum(const hi_size size);
    hi_s32 venc_start(hi_venc_chn venc_chn, sd3403_venc_chn_param *chn_param);
    hi_s32 venc_create(hi_venc_chn venc_chn, sd3403_venc_chn_param *chn_param);
    hi_s32 venc_channel_param_init(sd3403_venc_chn_param *chn_param, hi_venc_chn_attr *chn_attr);
    hi_s32 venc_h265_param_init(hi_venc_chn_attr *chn_attr,sd3403_venc_chn_param *chn_param);
    hi_void venc_h265_cbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h265_fixqp_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate);
    hi_void venc_h265_vbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h265_avbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h265_cvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_set_h265_cvbr_bit_rate(hi_venc_h264_cvbr *h265_cvbr,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h265_qvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h265_qpmap_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate,hi_u32 stats_time);
    hi_s32 venc_h264_param_init(hi_venc_chn_attr *chn_attr, sd3403_venc_chn_param *chn_param);
    hi_void venc_h264_cbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h264_fixqp_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate);
    hi_void venc_h264_vbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h264_avbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h264_cvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_set_h264_cvbr_bit_rate(hi_venc_h264_cvbr *h264_cvbr, hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h264_qvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_h264_qpmap_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate,hi_u32 stats_time);
    hi_s32 venc_mjpeg_param_init(hi_venc_chn_attr *venc_chn_attr,sd3403_venc_chn_param *venc_create_chn_param);
    hi_void venc_mjpeg_fixqp_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 frame_rate);
    hi_void venc_mjpeg_cbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 stats_time,hi_u32 frame_rate, hi_pic_size size);
    hi_void venc_mjpeg_vbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size);
    hi_s32 venc_jpeg_param_init(hi_venc_chn_attr *venc_chn_attr);
    hi_void venc_set_gop_attr(hi_payload_type type, hi_venc_chn_attr *chn_attr,hi_venc_gop_attr *gop_attr);
    hi_s32 venc_close_reencode(hi_venc_chn venc_chn);
    hi_s32 venc_start_get_stream(hi_venc_chn *venc_chn, hi_s32 cnt);
    hi_void venc_get_venc_stream_proc(hi_void);
    hi_s32 set_name_save_stream(sd3403_venc_stream_proc_info *stream_proc_info, hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type, sd3403_venc_getstream_para *para, hi_s32 venc_max_chn);
    hi_s32 venc_get_file_postfix(hi_payload_type payload, hi_char *file_postfix, hi_u8 len);
    hi_s32 set_file_name(hi_s32 index, hi_venc_chn venc_chn,sd3403_venc_stream_proc_info *stream_proc_info);
    hi_void fd_isset(sd3403_venc_stream_proc_info *stream_proc_info, fd_set *read_fds,hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type, sd3403_venc_getstream_para *para);
    hi_s32 get_stream_from_one_channl(sd3403_venc_stream_proc_info *stream_proc_info, \
                    hi_s32 index, hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type);
    hi_s32 save_frame_to_file(hi_s32 index, sd3403_venc_stream_proc_info *stream_proc_info, \
                hi_venc_stream *stream, hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type);

    int getSensor_Frame(char *data, uint64_t *pts, int ms, bool &key, int venc_chn);
    hi_s32 venc_save_stream(FILE *fd, hi_venc_stream *stream);
    hi_s32 venc_save_stream_phys_addr(FILE *fd, hi_venc_stream_buf_info *stream_buf, hi_venc_stream *stream);
    hi_s32 venc_phys_addr_retrace(FILE *fd, hi_venc_stream_buf_info *stream_buf, hi_venc_stream *stream,hi_u32 i, hi_u32 j);
    hi_void stop_venc(hi_venc_chn venc_chn[], hi_u32 chn_num);
    hi_s32 venc_stop_get_stream(hi_s32 chn_num);
    hi_s32 venc_stop(hi_venc_chn venc_chn);
    int request_key_frame(int venc_chn);

    bool sensor0_keyFrame = false;
    bool sensor1_keyFrame = false;

private:
    pthread_t g_venc_pid;
    sd3403_venc_getstream_para  g_para;
    std::thread m_thread;
    sd3403_venc_chn_param g_venc_chn_param = {30, 1, 30, {1920, 1080}, PIC_1080P, 0, HI_FALSE, {HI_VENC_GOP_MODE_NORMAL_P, {2}}, HI_PT_H265, SD3403_RC_VBR};
    hi_size g_sample_pic_size[PIC_BUTT] = {
        { 352,  288  },  /* PIC_CIF */
        { 640,  360  },  /* PIC_360P */
        { 720,  576  },  /* PIC_D1_PAL */
        { 720,  480  },  /* PIC_D1_NTSC */
        { 960,  576  },  /* PIC_960H */
        { 1280, 720  },  /* PIC_720P */
        { 1920, 1080 },  /* PIC_1080P */
        { 720,  480  },  /* PIC_480P */
        { 720,  576  },  /* PIC_576P */
        { 800,  600  },  /* PIC_800X600 */
        { 1024, 768  },  /* PIC_1024X768 */
        { 1280, 1024 },  /* PIC_1280X1024 */
        { 1366, 768  },  /* PIC_1366X768 */
        { 1440, 900  },  /* PIC_1440X900 */
        { 1280, 800  },  /* PIC_1280X800 */
        { 1600, 1200 },  /* PIC_1600X1200 */
        { 1680, 1050 },  /* PIC_1680X1050 */
        { 1920, 1200 },  /* PIC_1920X1200 */
        { 640,  480  },  /* PIC_640X480 */
        { 1920, 2160 },  /* PIC_1920X2160 */
        { 2560, 1440 },  /* PIC_2560X1440 */
        { 2560, 1600 },  /* PIC_2560X1600 */
        { 2592, 1520 },  /* PIC_2592X1520 */
        { 2592, 1944 },  /* PIC_2592X1944 */
        { 3840, 2160 },  /* PIC_3840X2160 */
        { 4096, 2160 },  /* PIC_4096X2160 */
        { 3000, 3000 },  /* PIC_3000X3000 */
        { 4000, 3000 },  /* PIC_4000X3000 */
        { 6080, 2800 },  /* PIC_6080X2800 */
        { 7680, 4320 },  /* PIC_7680X4320 */
        { 3840, 8640 }   /* PIC_3840X8640 */
    };
    

};



#endif