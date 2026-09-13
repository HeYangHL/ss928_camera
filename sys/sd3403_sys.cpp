#include "sd3403_sys.hpp"

SD3403_SYS::SD3403_SYS()
{

}
SD3403_SYS::~SD3403_SYS()
{

}

hi_void SD3403_SYS::get_default_vb_config(hi_size *size, hi_vb_cfg *vb_cfg, hi_vi_video_mode video_mode, hi_u32 yuv_cnt, hi_u32 raw_cnt)
{
    hi_vb_calc_cfg calc_cfg;
    hi_pic_buf_attr buf_attr;

    (hi_void)memset_s(vb_cfg, sizeof(hi_vb_cfg), 0, sizeof(hi_vb_cfg));
    vb_cfg->max_pool_cnt = 128; /* 128 blks */

    /* default YUV pool: SP420 + compress_seg */
    buf_attr.width         = size->width;
    buf_attr.height        = size->height;
    buf_attr.align         = HI_DEFAULT_ALIGN;
    buf_attr.bit_width     = HI_DATA_BIT_WIDTH_8;
    buf_attr.pixel_format  = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    buf_attr.compress_mode = OT_COMPRESS_MODE_SEG;
    hi_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);

    vb_cfg->common_pool[0].blk_size = calc_cfg.vb_size;
    vb_cfg->common_pool[0].blk_cnt  = yuv_cnt;

    /* default raw pool: raw12bpp + compress_line */
    buf_attr.pixel_format  = HI_PIXEL_FORMAT_RGB_BAYER_12BPP;
    buf_attr.compress_mode = (video_mode == HI_VI_VIDEO_MODE_NORM ? HI_COMPRESS_MODE_LINE : HI_COMPRESS_MODE_NONE);
    hi_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    vb_cfg->common_pool[1].blk_size = calc_cfg.vb_size;
    vb_cfg->common_pool[1].blk_cnt  = raw_cnt;
}

hi_s32 SD3403_SYS::sys_init_with_vb_supplement(const hi_vb_cfg *vb_conf, hi_u32 supplement_config)
{
    hi_s32 ret;
    hi_vb_supplement_cfg supplement_conf = {0};

    hi_mpi_sys_exit();
    hi_mpi_vb_exit();

    if (vb_conf == HI_NULL) {
        ss_print("input parameter is null, it is invalid!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_vb_set_cfg(vb_conf);
    if (ret != HI_SUCCESS) {
        ss_print("hi_mpi_vb_set_conf failed!\n");
        return HI_FAILURE;
    }

    supplement_conf.supplement_cfg = supplement_config;
    ret = hi_mpi_vb_set_supplement_cfg(&supplement_conf);
    if (ret != HI_SUCCESS) {
        ss_print("hi_mpi_vb_set_supplement_conf failed!\n");
        return HI_FAILURE;
    }
 
    ret = hi_mpi_vb_init();
    if (ret != HI_SUCCESS) {
        ss_print("hi_mpi_vb_init failed!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_sys_init();
    if (ret != HI_SUCCESS) {
        ss_print("hi_mpi_sys_init failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_SYS::set_vi_vpss_mode(hi_void)
{
    hi_u32 i;
    hi_s32 ret;
    hi_vi_vpss_mode_type other_pipe_mode_type;
    hi_vi_vpss_mode vi_vpss_mode;

    if (mode_type == HI_VI_OFFLINE_VPSS_ONLINE) {
        other_pipe_mode_type = HI_VI_OFFLINE_VPSS_ONLINE;
    } else {
        other_pipe_mode_type = HI_VI_OFFLINE_VPSS_OFFLINE;
    }

    vi_vpss_mode.mode[0] = mode_type;
    for (i = 1; i < HI_VI_MAX_PIPE_NUM; i++) {
        vi_vpss_mode.mode[i] = other_pipe_mode_type;
    }

    ret = hi_mpi_sys_set_vi_vpss_mode(&vi_vpss_mode);
    if (ret != HI_SUCCESS) {
        ss_print("set vi vpss mode failed!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_sys_set_vi_video_mode(video_mode);
    if (ret != HI_SUCCESS) {
        ss_print("set vi video mode failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_SYS::sys_init(hi_vi_vpss_mode_type mode_type, hi_vi_video_mode video_mode, hi_u32 yuv_cnt, hi_u32 raw_cnt)
{
    hi_s32 ret;
    hi_size size;
    hi_vb_cfg vb_cfg;
    hi_u32 supplement_config;
    sdCom.get_size_by_sns_type(&size);
    get_default_vb_config(&size, &vb_cfg, video_mode, yuv_cnt, raw_cnt);
    supplement_config = HI_VB_SUPPLEMENT_BNR_MOT_MASK;
    ret = sys_init_with_vb_supplement(&vb_cfg, supplement_config);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = set_vi_vpss_mode();
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 SD3403_SYS::vi_bind_vpss(hi_vi_pipe vi_pipe, hi_vi_chn vi_chn, hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn)
{
    hi_mpp_chn src_chn;
    hi_mpp_chn dest_chn;

    src_chn.mod_id = HI_ID_VI;
    src_chn.dev_id = vi_pipe;
    src_chn.chn_id = vi_chn;

    dest_chn.mod_id = HI_ID_VPSS;
    dest_chn.dev_id = vpss_grp;
    dest_chn.chn_id = vpss_chn;

    check_return(hi_mpi_sys_bind(&src_chn, &dest_chn), "hi_mpi_sys_bind(VI-VPSS)");

    return HI_SUCCESS;
}
hi_s32 SD3403_SYS::vi_un_bind_vpss(hi_vi_pipe vi_pipe, hi_vi_chn vi_chn, hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn)
{
    hi_mpp_chn src_chn;
    hi_mpp_chn dest_chn;

    src_chn.mod_id = HI_ID_VI;
    src_chn.dev_id = vi_pipe;
    src_chn.chn_id = vi_chn;

    dest_chn.mod_id = HI_ID_VPSS;
    dest_chn.dev_id = vpss_grp;
    dest_chn.chn_id = vpss_chn;

    check_return(hi_mpi_sys_unbind(&src_chn, &dest_chn), "hi_mpi_sys_unbind(VI-VPSS)");

    return HI_SUCCESS;
}

hi_s32 SD3403_SYS::vpss_bind_venc(hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn, hi_venc_chn venc_chn)
{
    hi_mpp_chn src_chn;
    hi_mpp_chn dest_chn;

    src_chn.mod_id = HI_ID_VPSS;
    src_chn.dev_id = vpss_grp;
    src_chn.chn_id = vpss_chn;

    dest_chn.mod_id = HI_ID_VENC;
    dest_chn.dev_id = 0;
    dest_chn.chn_id = venc_chn;

    check_return(hi_mpi_sys_bind(&src_chn, &dest_chn), "hi_mpi_sys_bind(VPSS-VENC)");

    return HI_SUCCESS;
}
hi_s32 SD3403_SYS::vpss_un_bind_venc(hi_vpss_grp vpss_grp, hi_vpss_chn vpss_chn, hi_venc_chn venc_chn)
{
    hi_mpp_chn src_chn;
    hi_mpp_chn dest_chn;

    src_chn.mod_id = HI_ID_VPSS;
    src_chn.dev_id = vpss_grp;
    src_chn.chn_id = vpss_chn;

    dest_chn.mod_id = HI_ID_VENC;
    dest_chn.dev_id = 0;
    dest_chn.chn_id = venc_chn;

    check_return(hi_mpi_sys_unbind(&src_chn, &dest_chn), "hi_mpi_sys_unbind(VPSS-VENC)");

    return HI_SUCCESS;
}

hi_s32 SD3403_SYS::sys_mmap_cached(hi_video_frame_info &vpss_frame)
{
    uint64_t blk_size = 0;

    blk_size = vpss_frame.video_frame.width * vpss_frame.video_frame.height * 1.5;
    vpss_frame.video_frame.virt_addr[0] = hi_mpi_sys_mmap_cached(vpss_frame.video_frame.phys_addr[0], blk_size);
    return HI_SUCCESS;
}

hi_s32 SD3403_SYS::sys_exit(hi_void)
{
    hi_mpi_sys_exit();
    hi_mpi_vb_exit_mod_common_pool(HI_VB_UID_VDEC);
    hi_mpi_vb_exit();
    return 0;
}

