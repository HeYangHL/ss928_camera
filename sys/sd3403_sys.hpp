#ifndef _SD3403_SYS_HPP_
#define _SD3403_SYS_HPP_

#include "def.h"
#include "sd3403_common.hpp"

class SD3403_SYS{
public:
    SD3403_SYS();
    ~SD3403_SYS();
    hi_void get_default_vb_config(hi_size *size, hi_vb_cfg *vb_cfg, hi_vi_video_mode video_mode, hi_u32 yuv_cnt, hi_u32 raw_cnt);
    hi_s32 sys_init_with_vb_supplement(const hi_vb_cfg *vb_conf, hi_u32 supplement_config);
    hi_s32 set_vi_vpss_mode(hi_void);
    hi_s32 sys_init(hi_vi_vpss_mode_type mode_type, hi_vi_video_mode video_mode, hi_u32 yuv_cnt, hi_u32 raw_cnt);
    hi_s32 vi_bind_vpss(hi_vi_pipe vi_pipe, hi_vi_chn vi_chn, hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn);
    hi_s32 vi_un_bind_vpss(hi_vi_pipe vi_pipe, hi_vi_chn vi_chn, hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn);
    hi_s32 vpss_bind_venc(hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn, hi_venc_chn venc_chn);
    hi_s32 vpss_un_bind_venc(hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn, hi_venc_chn venc_chn);
    hi_s32 sys_exit(hi_void);
    hi_s32 sys_mmap_cached(hi_video_frame_info &vpss_frame);

private:
    hi_vi_vpss_mode_type mode_type = HI_VI_OFFLINE_VPSS_OFFLINE;
    hi_vi_video_mode video_mode = HI_VI_VIDEO_MODE_NORM;
    SD3403_Common sdCom;

};

#endif
