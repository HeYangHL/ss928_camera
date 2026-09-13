#include "sd3403_vpss.hpp"

SD3403_VPSS::SD3403_VPSS()
{
}
SD3403_VPSS::~SD3403_VPSS()
{
}

hi_s32 SD3403_VPSS::start_vpss(hi_vpss_grp grp, hi_size *in_size)
{
    hi_s32 ret;
    hi_low_delay_info low_delay_info;
    hi_vpss_grp_attr grp_attr;
    hi_vpss_chn_attr chn_attr[2];
    hi_bool chn_enable[HI_VPSS_MAX_PHYS_CHN_NUM] = {HI_TRUE, HI_TRUE, HI_FALSE, HI_FALSE};

    get_default_grp_attr(&grp_attr);
    grp_attr.max_width = in_size->width;
    grp_attr.max_height = in_size->height;
    get_default_chn_attr(&chn_attr[0]);
    get_default_chn_attr(&chn_attr[1]);
    chn_attr[0].width = in_size->width;
    chn_attr[0].height = in_size->height;
    chn_attr[0].compress_mode = HI_COMPRESS_MODE_SEG_COMPACT;
    chn_attr[1].width = 640;
    chn_attr[1].height = 640;
    chn_attr[1].depth = 2;
    chn_attr[1].compress_mode = HI_COMPRESS_MODE_NONE;
    chn_attr[1].pixel_format = HI_PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    ret = vpss_start(grp, chn_enable, &grp_attr, chn_attr, HI_VPSS_MAX_PHYS_CHN_NUM);
    if (ret != HI_SUCCESS)
    {
        return ret;
    }

    low_delay_info.enable = HI_TRUE;
    low_delay_info.line_cnt = 200; /* 200: lowdelay line */
    low_delay_info.one_buf_en = HI_FALSE;
    ret = hi_mpi_vpss_set_low_delay_attr(grp, 0, &low_delay_info);
    if (ret != HI_SUCCESS)
    {
        vpss_stop(grp, chn_enable, HI_VPSS_MAX_PHYS_CHN_NUM);
        return ret;
    }

    return HI_SUCCESS;
}

hi_void SD3403_VPSS::get_default_grp_attr(hi_vpss_grp_attr *grp_attr)
{
    grp_attr->nr_en = HI_TRUE;
    grp_attr->ie_en = HI_FALSE;
    grp_attr->dci_en = HI_FALSE;
    grp_attr->buf_share_en = HI_FALSE;
    grp_attr->mcf_en = HI_FALSE;
    grp_attr->max_width = VPSS_DEFAULT_WIDTH;
    grp_attr->max_height = VPSS_DEFAULT_HEIGHT;
    grp_attr->max_dei_width = 0;
    grp_attr->max_dei_height = 0;
    grp_attr->dynamic_range = HI_DYNAMIC_RANGE_SDR8;
    grp_attr->pixel_format = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    grp_attr->dei_mode = HI_VPSS_DEI_MODE_OFF;
    grp_attr->buf_share_chn = HI_VPSS_CHN0;
    grp_attr->nr_attr.nr_type = HI_VPSS_NR_TYPE_VIDEO_NORM;
    grp_attr->nr_attr.compress_mode = HI_COMPRESS_MODE_FRAME;
    grp_attr->nr_attr.nr_motion_mode = HI_VPSS_NR_MOTION_MODE_NORM;
    grp_attr->frame_rate.src_frame_rate = -1;
    grp_attr->frame_rate.dst_frame_rate = -1;
}

hi_void SD3403_VPSS::get_default_chn_attr(hi_vpss_chn_attr *chn_attr)
{
    chn_attr->mirror_en = HI_FALSE;
    chn_attr->flip_en = HI_FALSE;
    chn_attr->border_en = HI_FALSE;
    chn_attr->width = VPSS_DEFAULT_WIDTH;
    chn_attr->height = VPSS_DEFAULT_HEIGHT;
    chn_attr->depth = 0;
    chn_attr->chn_mode = HI_VPSS_CHN_MODE_USER;
    chn_attr->video_format = HI_VIDEO_FORMAT_LINEAR;
    chn_attr->dynamic_range = HI_DYNAMIC_RANGE_SDR8;
    chn_attr->pixel_format = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    chn_attr->compress_mode = HI_COMPRESS_MODE_SEG;
    chn_attr->aspect_ratio.mode = HI_ASPECT_RATIO_NONE;
    chn_attr->frame_rate.src_frame_rate = -1;
    chn_attr->frame_rate.dst_frame_rate = -1;
}

hi_s32 SD3403_VPSS::vpss_start(hi_vpss_grp grp, const hi_bool *chn_enable, const hi_vpss_grp_attr *grp_attr, const hi_vpss_chn_attr *chn_attr, hi_u32 chn_array_size)
{
    hi_s32 ret;
    if (chn_array_size < HI_VPSS_MAX_PHYS_CHN_NUM)
    {
        ss_print("array size(%u) of chn_enable and chn_attr need > %u!\n",
                 chn_array_size, HI_VPSS_MAX_PHYS_CHN_NUM);
        return HI_FAILURE;
    }
    ret = hi_mpi_vpss_create_grp(grp, grp_attr);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_vpss_create_grp(grp:%d) failed with %#x!\n", grp, ret);
        return HI_FAILURE;
    }
    ret = hi_mpi_vpss_start_grp(grp);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_vpss_start_grp failed with %#x\n", ret);
        goto destroy_grp;
    }

    ret = vpss_start_chn(grp, chn_enable, chn_attr, HI_VPSS_MAX_PHYS_CHN_NUM);
    if (ret != HI_SUCCESS)
    {
        goto stop_grp;
    }

    return HI_SUCCESS;

stop_grp:
    ret = hi_mpi_vpss_stop_grp(grp);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_vpss_stop_grp failed with %#x!\n", ret);
    }
destroy_grp:
    ret = hi_mpi_vpss_destroy_grp(grp);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_vpss_destroy_grp failed with %#x!\n", ret);
    }
    return HI_FAILURE;
}
hi_s32 SD3403_VPSS::vpss_start_chn(hi_vpss_grp grp, const hi_bool *chn_enable, const hi_vpss_chn_attr *chn_attr, hi_u32 chn_array_size)
{
    hi_vpss_chn vpss_chn;
    hi_s32 ret, i;

    for (i = 0; i < (hi_s32)chn_array_size; ++i)
    {
        if (chn_enable[i] == HI_TRUE)
        {
            vpss_chn = i;
            ret = hi_mpi_vpss_set_chn_attr(grp, vpss_chn, &chn_attr[vpss_chn]);
            if (ret != HI_SUCCESS)
            {
                ss_print("hi_mpi_vpss_set_chn_attr failed with %#x\n", ret);
                goto disable_chn;
            }
            ret = hi_mpi_vpss_enable_chn(grp, vpss_chn);
            if (ret != HI_SUCCESS)
            {
                ss_print("hi_mpi_vpss_enable_chn failed with %#x\n", ret);
                goto disable_chn;
            }
        }
    }
    return HI_SUCCESS;

disable_chn:
    for (i = i - 1; i >= 0; i--)
    {
        if (chn_enable[i] == HI_TRUE)
        {
            vpss_chn = i;
            ret = hi_mpi_vpss_disable_chn(grp, vpss_chn);
            if (ret != HI_SUCCESS)
            {
                ss_print("hi_mpi_vpss_disable_chn failed with %#x!\n", ret);
            }
        }
    }
    return HI_FAILURE;
}

hi_s32 SD3403_VPSS::vpss_stop(hi_vpss_grp grp, const hi_bool *chn_enable, hi_u32 chn_array_size)
{
    hi_s32 i;
    hi_s32 ret;
    hi_vpss_chn vpss_chn;

    if (chn_array_size < HI_VPSS_MAX_PHYS_CHN_NUM)
    {
        ss_print("array size(%u) of chn_enable need > %u!\n", chn_array_size, HI_VPSS_MAX_PHYS_CHN_NUM);
        return HI_FAILURE;
    }

    for (i = 0; i < HI_VPSS_MAX_PHYS_CHN_NUM; ++i)
    {
        if (chn_enable[i] == HI_TRUE)
        {
            vpss_chn = i;
            ret = hi_mpi_vpss_disable_chn(grp, vpss_chn);
            if (ret != HI_SUCCESS)
            {
                ss_print("hi_mpi_vpss_disable_chn failed with %#x!\n", ret);
            }
        }
    }

    ret = hi_mpi_vpss_stop_grp(grp);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_vpss_stop_grp failed with %#x!\n", ret);
    }

    ret = hi_mpi_vpss_destroy_grp(grp);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_vpss_destroy_grp failed with %#x!\n", ret);
    }

    return HI_SUCCESS;
}

hi_void SD3403_VPSS::stop_vpss(hi_vpss_grp grp)
{

    hi_bool chn_enable[HI_VPSS_MAX_PHYS_CHN_NUM];

    if (grp == 1)
    {
        chn_enable[0] = HI_TRUE;
        chn_enable[1] = HI_FALSE;
        chn_enable[2] = HI_FALSE;
        chn_enable[3] = HI_FALSE;
    }
    else
    {
        chn_enable[0] = HI_TRUE;
        chn_enable[1] = HI_TRUE;
        chn_enable[2] = HI_FALSE;
        chn_enable[3] = HI_FALSE;
    }

    vpss_stop(grp, chn_enable, HI_VPSS_MAX_PHYS_CHN_NUM);
}

hi_s32 SD3403_VPSS::get_model_yuv(hi_vpss_grp grp, hi_video_frame_info &vpss_frame)
{

    int vpss_fd = 0;
    int ret = 0;
    int get_frame_ts = 100;

    vpss_fd = hi_mpi_vpss_get_chn_fd(grp, 1);
    fd_set vi_read_fds;
    struct timeval TimeoutVal;
    FD_ZERO(&vi_read_fds);
    FD_SET(vpss_fd, &vi_read_fds);
    TimeoutVal.tv_sec = 0;
    TimeoutVal.tv_usec = 500000;
    ret = select(vpss_fd + 1, &vi_read_fds, NULL, NULL, &TimeoutVal);
    if (ret < 0)
    {
        ss_print("vpss select failed!\n");
        return -1; // 失败，外部continue
    }
    else if (ret == 0)
    {
        ss_print("get vpss chn_%d stream time out.\n", 0);
        //            ss_mpi_vpss_close_fd();
        return -2; // 超时，外部continue
    }
    else
    {
        if (FD_ISSET(vpss_fd, &vi_read_fds))
        {
            ret = hi_mpi_vpss_get_chn_frame(grp, 1, &vpss_frame, get_frame_ts);
            if (ret != HI_SUCCESS)
            {
                ss_print("==========>get vpss chn frame error!\n");
                //                    ss_mpi_vpss_close_fd();
                hi_mpi_vpss_release_chn_frame(grp, 1, &vpss_frame);
            }
        }
    }

    return 0;
}

hi_s32 SD3403_VPSS::release_model_vpss_chn(hi_vpss_grp grp, hi_video_frame_info &vpss_frame)
{
    hi_s32 ret = 0;

    ret = hi_mpi_vpss_release_chn_frame(grp, 1, &vpss_frame);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_vpss_release_chn_frame failed with %#x!\n", ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
