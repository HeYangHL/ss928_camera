#include "sd3403_vi.hpp"

SD3403_VI::SD3403_VI()
{

}
SD3403_VI::~SD3403_VI()
{
    
}


static hi_isp_pub_attr g_isp_pub_attr_os08a20_mipi_8m_30fps = {
    {0, 24, 3840, 2160},
    {3840, 2160},
    30,
    HI_ISP_BAYER_BGGR,
    HI_WDR_MODE_NONE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 3840, 2180},
    },
};

static hi_isp_pub_attr g_isp_pub_attr_os08a20_mipi_8m_30fps_wdr2to1 = {
    {0, 24, 3840, 2160},
    {3840, 2160},
    30,
    HI_ISP_BAYER_BGGR,
    HI_WDR_MODE_2To1_LINE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 3840, 2180},
    },
};

static hi_isp_pub_attr g_isp_pub_attr_os04a10_mipi_4m_30fps = {
    {0, 0, WIDTH_2688, HEIGHT_1520},
    {WIDTH_2688, HEIGHT_1520},
    30,
    HI_ISP_BAYER_RGGB,
    HI_WDR_MODE_NONE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 2688, 1520},
    },
};

static hi_isp_pub_attr g_isp_pub_attr_os08b10_mipi_8m_30fps_wdr2to1 = {
    {0, 0, 3840, 2160},
    {3840, 2160},
    30,
    HI_ISP_BAYER_RGGB,
    HI_WDR_MODE_2To1_LINE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 3840, 2160},
    },
};

static hi_isp_pub_attr g_isp_pub_attr_os08b10_mipi_8m_30fps = {
    {0, 0, 3840, 2160},
    {3840, 2160},
    30,
    HI_ISP_BAYER_RGGB,
    HI_WDR_MODE_NONE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 3840, 2160},
    },
};

static hi_isp_pub_attr g_isp_pub_attr_os05a10_2l_mipi_4m_30fps = {
    {0, 0, 2688, 1520},
    {2688, 1520},
    30,
    HI_ISP_BAYER_BGGR,
    HI_WDR_MODE_NONE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 2688, 1520},
    },
};

static hi_isp_pub_attr g_isp_pub_attr_imx347_slave_mipi_4m_30fps = {
    {0, 20, WIDTH_2592, HEIGHT_1520},
    {WIDTH_2592, HEIGHT_1520},
    30,
    HI_ISP_BAYER_RGGB,
    HI_WDR_MODE_NONE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 2592, 1540},
    },
};

static hi_isp_pub_attr g_isp_pub_attr_imx485_mipi_8m_30fps = {
    {0, 20, 3840, 2160},
    {3840, 2160},
    30,
    HI_ISP_BAYER_RGGB,
    HI_WDR_MODE_NONE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 3840, 2180},
    },
};

hi_isp_pub_attr g_isp_pub_attr_imx485_mipi_8m_30fps_wdr3to1 = {
    {0, 0, 3840, 2160},
    {3840, 2160},
    30,
    HI_ISP_BAYER_RGGB,
    HI_WDR_MODE_3To1_LINE,
    0,
    HI_FALSE,
    HI_FALSE,
    {
        HI_FALSE,
        {0, 0, 3840, 2160},
    },
};


hi_s32 SD3403_VI::start_vi(const sd3403_vi_cfg *vi_cfg)
{
    hi_s32 ret;
    ret = start_mipi_rx(&vi_cfg->sns_info, &vi_cfg->mipi_info);
    if (ret != HI_SUCCESS) {
        ss_print("start mipi rx failed!\n");
        goto start_mipi_rx_failed;
    }
    ret = start_dev(vi_cfg->dev_info.vi_dev, &vi_cfg->dev_info.dev_attr, &vi_cfg->dev_info.bas_attr);
    if (ret != HI_SUCCESS) {
        ss_print("start dev failed!\n");
        goto start_dev_failed;
    }
    ret = dev_bind_pipe(vi_cfg->dev_info.vi_dev, &vi_cfg->bind_pipe);
    if (ret != HI_SUCCESS) {
        ss_print("dev bind pipe failed!\n");
        goto dev_bind_pipe_failed;
    }
    ret = set_grp_info(&vi_cfg->grp_info);
    if (ret != HI_SUCCESS) {
        ss_print("set grp info failed!\n");
        goto set_grp_info_failed;
    }
    ret = start_pipe(&vi_cfg->bind_pipe, vi_cfg->pipe_info);
    if (ret != HI_SUCCESS) {
        ss_print("start pipe failed!\n");
        goto start_pipe_failed;
    }
    ret = start_isp(vi_cfg);
    if (ret != HI_SUCCESS) {
        ss_print("start isp failed!\n");
        goto start_isp_failed;
    }
    return HI_SUCCESS;

start_isp_failed:
    stop_pipe(&vi_cfg->bind_pipe, vi_cfg->pipe_info);
start_pipe_failed: /* fall through */
set_grp_info_failed:
    dev_unbind_pipe(vi_cfg->dev_info.vi_dev, &vi_cfg->bind_pipe);
dev_bind_pipe_failed:
    stop_dev(vi_cfg->dev_info.vi_dev);
start_dev_failed:
    stop_mipi_rx(&vi_cfg->sns_info, &vi_cfg->mipi_info);
start_mipi_rx_failed:
    return HI_FAILURE;

}

hi_s32 SD3403_VI::start_isp(const sd3403_vi_cfg *vi_cfg)
{
    hi_s8 i, j;
    hi_s32 ret;
    hi_vi_pipe vi_pipe;
    for (i = 0; i < (hi_u8)vi_cfg->bind_pipe.pipe_num; i++) {
        vi_pipe = vi_cfg->bind_pipe.pipe_id[i];
        if ((vi_cfg->grp_info.fusion_grp_attr[0].wdr_mode != HI_WDR_MODE_NONE) &&
            (vi_cfg->grp_info.fusion_grp_attr[0].wdr_mode != HI_WDR_MODE_BUILT_IN) &&
            (i > 0)) {
            continue;
        }
        ret = start_one_pipe_isp(vi_pipe, i, vi_cfg);
        if (ret != HI_SUCCESS) {
            for (j = i - 1; (j >= 0) && (i != 0); j--) {
                vi_pipe = vi_cfg->bind_pipe.pipe_id[j];
                stop_one_pipe_isp(vi_pipe);
            }

            return ret;
        }
    }

    return HI_SUCCESS;
}

hi_void SD3403_VI::stop_one_pipe_isp(hi_vi_pipe vi_pipe)
{
    hi_mpi_isp_exit(vi_pipe);
    isp_stop(vi_pipe);
    deregister_sensor_lib(vi_pipe);

    g_start_isp[vi_pipe] = HI_FALSE;
}

hi_void SD3403_VI::isp_stop(hi_isp_dev isp_dev)
{
    // if (g_isp_pid[isp_dev]) {
    //     pthread_join(g_isp_pid[isp_dev], NULL);
    //     g_isp_pid[isp_dev] = 0;
    // }
    if (m_thread[isp_dev].joinable()) {
        m_thread[isp_dev].join();
    }

    return;
}

hi_s32 SD3403_VI::start_one_pipe_isp(hi_vi_pipe vi_pipe, hi_u8 pipe_index, const sd3403_vi_cfg *vi_cfg)
{
    hi_s32 ret;
    ret = register_sensor_lib(vi_pipe, pipe_index, vi_cfg);
    if (ret != HI_SUCCESS) {
        printf("register sensor to ISP %d failed\n", vi_pipe);
        return HI_FAILURE;
    }
    ret = hi_mpi_isp_mem_init(vi_pipe);
    if (ret != HI_SUCCESS) {
        printf("HI_MPI_ISP_MemInit failed with 0x%x!\n", ret);
        goto exit0;
    }
    ret = hi_mpi_isp_set_pub_attr(vi_pipe, &vi_cfg->pipe_info[pipe_index].isp_info.isp_pub_attr);
    if (ret != HI_SUCCESS) {
        printf("HI_MPI_ISP_SetPubAttr failed with 0x%x!\n", ret);
        goto exit1;
    }
    ret = hi_mpi_isp_init(vi_pipe);
    if (ret != HI_SUCCESS) {
        printf("HI_MPI_ISP_Init failed with 0x%x!\n", ret);
        return -1;
    }
    ret = isp_sensor_founction_cfg(vi_pipe, vi_cfg->sns_info.sns_type);
    if (ret != HI_SUCCESS) {
        printf("sensor founction cfg failed with 0x%x!\n", ret);
        return -1;
    }
    if ((vi_pipe < HI_VI_MAX_PHYS_PIPE_NUM) &&
        (vi_cfg->pipe_info[pipe_index].isp_need_run == HI_TRUE)) {
        ret = isp_run(vi_pipe);
        if (ret != HI_SUCCESS) {
            printf("ISP Run failed with 0x%x!\n", ret);
            goto exit1;
        }
    }
    g_start_isp[vi_pipe] = HI_TRUE;

    return HI_SUCCESS;

exit1:
    hi_mpi_isp_exit(vi_pipe);
exit0:
    deregister_sensor_lib(vi_pipe);
    return ret;
}

hi_void SD3403_VI::deregister_sensor_lib(hi_vi_pipe vi_pipe)
{
    isp_awb_lib_uncallback(vi_pipe);
    isp_ae_lib_uncallback(vi_pipe);
    isp_sensor_unregiter_callback(vi_pipe);
}

hi_s32 SD3403_VI::isp_run(hi_isp_dev isp_dev)
{
    hi_s32 ret;
    pthread_attr_t *thread_attr = NULL;

    // ret = pthread_create(&g_isp_pid[isp_dev], thread_attr, sample_comm_isp_thread, (hi_void*)(hi_ulong)isp_dev);
    // if (ret != 0) {
    //     printf("create isp running thread failed!, error: %d\r\n", ret);
    // }
    m_thread[isp_dev] = std::thread(&SD3403_VI::isp_thread, this, isp_dev);

    // if (thread_attr != HI_NULL) {
    //     pthread_attr_destroy(thread_attr);
    // }
    return ret;
}

hi_void SD3403_VI::isp_thread(hi_isp_dev isp_dev)
{
    hi_s32 ret;
    // hi_isp_dev isp_dev;
    hi_char thread_name[20];
    // isp_dev = (hi_isp_dev)(hi_ulong)param;
    errno_t err;

    err = snprintf_s(thread_name, sizeof(thread_name)*20, 19, "ISP%d_RUN", isp_dev); /* 20,19 chars */
    if (err < 0) {
        return;
    }
    prctl(PR_SET_NAME, thread_name, 0, 0, 0);

    printf("ISP Dev %d running !\n", isp_dev);
    ret = hi_mpi_isp_run(isp_dev);
    if (ret != HI_SUCCESS) {
        printf("HI_MPI_ISP_Run failed with %#x!\n", ret);
        return;
    }

    return;
}


hi_s32 SD3403_VI::isp_sensor_founction_cfg(hi_vi_pipe vi_pipe, sd3403_sns_type sns_type)
{
    hi_isp_sns_blc_clamp sns_blc_clamp;

    sns_blc_clamp.blc_clamp_en = HI_FALSE;

    switch (sns_type) {
        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
            g_sns_imx485_obj.pfn_set_blc_clamp(vi_pipe, sns_blc_clamp);
            break;
        default:
            break;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VI::register_sensor_lib(hi_vi_pipe vi_pipe, hi_u8 pipe_index, const sd3403_vi_cfg *vi_cfg)
{
    hi_s32 ret;
    hi_u32 bus_id;

    ret = isp_sensor_regiter_callback(vi_pipe);
    if (ret != HI_SUCCESS) {
        printf("register sensor to ISP %d failed\n", vi_pipe);
        return HI_FAILURE;
    }
    if (pipe_index > 0) {
        bus_id = -1;
    } else {
        bus_id = vi_cfg->sns_info.bus_id;
    }

    ret = isp_bind_sns(vi_pipe, bus_id);
    if (ret != HI_SUCCESS) {
        printf("register sensor bus id %d failed\n", bus_id);
        goto exit0;
    }
    ret = isp_ae_lib_callback(vi_pipe);
    if (ret != HI_SUCCESS) {
        printf("isp_mst_comm_ae_lib_callback failed\n");
        goto exit0;
    }
    ret = isp_awb_lib_callback(vi_pipe);
    if (ret != HI_SUCCESS) {
        printf("isp_mst_comm_awb_lib_callback failed\n");
        goto exit1;
    }

    return HI_SUCCESS;
    exit1:
    isp_ae_lib_uncallback(vi_pipe);
exit0:
    isp_sensor_unregiter_callback(vi_pipe);
    return ret;


}

hi_s32 SD3403_VI::isp_sensor_unregiter_callback(hi_isp_dev isp_dev)
{
    hi_isp_3a_alg_lib ae_lib;
    hi_isp_3a_alg_lib awb_lib;
    hi_isp_sns_obj *sns_obj;
    hi_s32 ret;
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    // sns_obj = isp_get_sns_obj(g_sns_type[isp_dev]);
    sns_obj = isp_get_sns_obj(sns_type);
    if (sns_obj == HI_NULL) {
        printf("sensor %d not exist!\n", sns_type);
        return HI_FAILURE;
    }

    ae_lib.id = isp_dev;
    awb_lib.id = isp_dev;
    strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    if (sns_obj->pfn_un_register_callback != HI_NULL) {
        ret = sns_obj->pfn_un_register_callback(isp_dev, &ae_lib, &awb_lib);
        if (ret != HI_SUCCESS) {
            printf("sensor_unregister_callback failed with %#x!\n", ret);
            return ret;
        }
    } else {
        printf("sensor_unregister_callback failed with HI_NULL!\n");
    }

    // g_sns_type[isp_dev] = SNS_TYPE_BUTT;

    return HI_SUCCESS;
}

hi_s32 SD3403_VI::isp_awb_lib_uncallback(hi_isp_dev isp_dev)
{
    hi_s32 ret;
    hi_isp_3a_alg_lib awb_lib;

    awb_lib.id = isp_dev;
    strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    ret = hi_mpi_awb_unregister(isp_dev, &awb_lib);
    if (ret != HI_SUCCESS) {
        printf("hi_mpi_awb_unregister failed with %#x!\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}


hi_s32 SD3403_VI::isp_ae_lib_uncallback(hi_isp_dev isp_dev)
{
    hi_s32 ret;
    hi_isp_3a_alg_lib ae_lib;

    ae_lib.id = isp_dev;
    strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));

    ret = hi_mpi_ae_unregister(isp_dev, &ae_lib);
    if (ret != HI_SUCCESS) {
        printf("hi_mpi_ae_unregister failed with %#x!\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}


hi_s32 SD3403_VI::isp_awb_lib_callback(hi_isp_dev isp_dev)
{
    hi_s32 ret;
    hi_isp_3a_alg_lib awb_lib;

    awb_lib.id = isp_dev;
    strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));

    ret = hi_mpi_awb_register(isp_dev, &awb_lib);
    if (ret != HI_SUCCESS) {
        printf("hi_mpi_awb_register failed with %#x!\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VI::isp_ae_lib_callback(hi_isp_dev isp_dev)
{
    hi_s32 ret;
    hi_isp_3a_alg_lib ae_lib;

    ae_lib.id = isp_dev;
    strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));

    ret = hi_mpi_ae_register(isp_dev, &ae_lib);
    if (ret != HI_SUCCESS) {
        printf("hi_mpi_ae_register failed with %#x!\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}


hi_s32 SD3403_VI::isp_bind_sns(hi_isp_dev isp_dev, hi_s8 sns_dev)
{
    hi_isp_sns_commbus sns_bus_info;
    hi_isp_sns_type    bus_type;
    hi_isp_sns_obj    *sns_obj;
    hi_s32 ret;
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    sns_obj = isp_get_sns_obj(sns_type);
    if (sns_obj == HI_NULL) {
        printf("sensor %d not exist!\n", sns_type);
        return HI_FAILURE;
    }
    bus_type = get_sns_bus_type();
    if (bus_type == HI_ISP_SNS_I2C_TYPE) {
        sns_bus_info.i2c_dev = sns_dev;
    } else {
        sns_bus_info.ssp_dev.bit4_ssp_dev = sns_dev;
        sns_bus_info.ssp_dev.bit4_ssp_cs  = 0;
    }

    if (sns_obj->pfn_set_bus_info != HI_NULL) {
        ret = sns_obj->pfn_set_bus_info(isp_dev, sns_bus_info);
        if (ret != HI_SUCCESS) {
            printf("set sensor bus info failed with %#x!\n", ret);
            return ret;
        }
    } else {
        printf("not support set sensor bus info!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;

}

hi_isp_sns_type SD3403_VI::get_sns_bus_type(hi_void)
{
    sd3403_sns_type sns_type = sdCom.get_sns_type();
    hi_unused(sns_type);
    return HI_ISP_SNS_I2C_TYPE; 
}

hi_s32 SD3403_VI::isp_sensor_regiter_callback(hi_isp_dev isp_dev)
{
    hi_s32 ret;
    hi_isp_3a_alg_lib ae_lib;
    hi_isp_3a_alg_lib awb_lib;
    hi_isp_sns_obj *sns_obj;
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    sns_obj = isp_get_sns_obj(sns_type);
    if (sns_obj == HI_NULL) {
        printf("sensor %d not exist!\n", sns_type);
        return HI_FAILURE;
    }

    ae_lib.id = isp_dev;
    awb_lib.id = isp_dev;
    strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    if (sns_obj->pfn_register_callback != HI_NULL) {
        ret = sns_obj->pfn_register_callback(isp_dev, &ae_lib, &awb_lib);
        if (ret != HI_SUCCESS) {
            printf("sensor_register_callback failed with %#x!\n", ret);
            return ret;
        }
    } else {
        printf("sensor_register_callback failed with HI_NULL!\n");
    }

    // g_sns_type[isp_dev] = sns_type;

    return HI_SUCCESS;
}

hi_isp_sns_obj *SD3403_VI::isp_get_sns_obj(sd3403_sns_type sns_type)
{
    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
            return &g_sns_os08a20_obj;
        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
            return &g_sns_os04a10_obj;
        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
            return &g_sns_os08b10_obj;
        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
            return &g_sns_os05a10_2l_slave_obj;
        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            return &g_sns_imx347_slave_obj;
        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
            return &g_sns_imx485_obj;
        default:
            return HI_NULL;
    }
}


hi_s32 SD3403_VI::start_pipe(const hi_vi_bind_pipe *bind_pipe, const sd3403_vi_pipe_info pipe_info[])
{
    hi_s32 i;
    hi_s32 ret;

    for (i = 0; i < (hi_s32)bind_pipe->pipe_num; i++) {
        hi_vi_pipe vi_pipe = bind_pipe->pipe_id[i];
        ret = start_one_pipe(vi_pipe, &pipe_info[i]);
        if (ret != HI_SUCCESS) {
            goto exit;
        }
    }

    return HI_SUCCESS;

exit:
    for (i = i - 1; i >= 0; i--) {
        hi_vi_pipe vi_pipe = bind_pipe->pipe_id[i];
        stop_one_pipe(vi_pipe, &pipe_info[i]);
    }
    return HI_FAILURE;
}

hi_void SD3403_VI::stop_pipe(const hi_vi_bind_pipe *bind_pipe, const sd3403_vi_pipe_info pipe_info[])
{
    hi_s32 i;
    for (i = bind_pipe->pipe_num - 1; i >= 0; i--) {
        hi_vi_pipe vi_pipe = bind_pipe->pipe_id[i];
        stop_one_pipe(vi_pipe, &pipe_info[i]);
    }
}
hi_void SD3403_VI::dev_unbind_pipe(hi_vi_dev vi_dev, const hi_vi_bind_pipe *bind_pipe)
{
    hi_u32 i;
    hi_s32 ret;

    for (i = 0; i < bind_pipe->pipe_num; i++) {
        ret = hi_mpi_vi_unbind(vi_dev, bind_pipe->pipe_id[i]);
        if (ret != HI_SUCCESS) {
            ss_print("vi dev(%d) unbind pipe(%d) failed!\n", vi_dev, bind_pipe->pipe_id[i]);
        }
    }
}

hi_void SD3403_VI::stop_dev(hi_vi_dev vi_dev)
{
    hi_s32 ret;

    ret = hi_mpi_vi_disable_dev(vi_dev);
    if (ret != HI_SUCCESS) {
        ss_print("vi disable dev failed with 0x%x!\n", ret);
    }
}

hi_void SD3403_VI::stop_mipi_rx(const sd3403_sns_info *sns_info, const sd3403_mipi_info *mipi_info)
{
    hi_s32 ret;

    ret = mipi_ctrl_cmd(mipi_info->mipi_dev, HI_MIPI_RESET_MIPI);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d reset mipi rx failed!\n", mipi_info->mipi_dev);
    }

    ret = mipi_ctrl_cmd(mipi_info->mipi_dev, HI_MIPI_DISABLE_MIPI_CLOCK);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d disable mipi rx clock failed!\n", mipi_info->mipi_dev);
    }

    ret = mipi_ctrl_cmd(sns_info->sns_rst_src, HI_MIPI_RESET_SENSOR);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d reset sensor failed!\n", sns_info->sns_rst_src);
    }

    ret = mipi_ctrl_cmd(sns_info->sns_clk_src, HI_MIPI_DISABLE_SENSOR_CLOCK);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d disable sensor clock failed!\n", sns_info->sns_clk_src);
    }
}

hi_void SD3403_VI::stop_isp(const sd3403_vi_cfg *vi_cfg)
{
    hi_u32     i;
    hi_bool    start_pipe;
    hi_vi_pipe vi_pipe;

    for (i = 0; i < vi_cfg->bind_pipe.pipe_num; i++) {
        if ((vi_cfg->pipe_info[i].isp_info.isp_pub_attr.wdr_mode == HI_WDR_MODE_NONE) ||
            (vi_cfg->pipe_info[i].isp_info.isp_pub_attr.wdr_mode == HI_WDR_MODE_BUILT_IN)) {
            start_pipe = HI_TRUE;
        } else {
            start_pipe = (i > 0) ? HI_FALSE : HI_TRUE;
        }

        if (start_pipe != HI_TRUE) {
            continue;
        }

        vi_pipe = vi_cfg->bind_pipe.pipe_id[i];
        stop_one_pipe_isp(vi_pipe);
    }
}

hi_void SD3403_VI::stop_vi(const sd3403_vi_cfg *vi_cfg)
{
    hi_vi_dev vi_dev = vi_cfg->dev_info.vi_dev;

    stop_isp(vi_cfg);
    stop_pipe(&vi_cfg->bind_pipe, vi_cfg->pipe_info);
    dev_unbind_pipe(vi_dev, &vi_cfg->bind_pipe);
    stop_dev(vi_dev);
    stop_mipi_rx(&vi_cfg->sns_info, &vi_cfg->mipi_info);
}

hi_void SD3403_VI::stop_one_pipe(hi_vi_pipe vi_pipe, const sd3403_vi_pipe_info *pipe_info)
{
     hi_s32 ret;

    ret = stop_chn(vi_pipe, pipe_info->chn_info, pipe_info->chn_num);
    if (ret != HI_SUCCESS) {
        ss_print("vi pipe(%d) stop chn failed!\n", vi_pipe);
    }

    ret = hi_mpi_vi_stop_pipe(vi_pipe);
    if (ret != HI_SUCCESS) {
        ss_print("vi stop pipe(%d) failed with 0x%x!\n", vi_pipe, ret);
    }

    ret = hi_mpi_vi_destroy_pipe(vi_pipe);
    if (ret != HI_SUCCESS) {
        ss_print("vi destroy pipe(%d) failed with 0x%x!\n", vi_pipe, ret);
    }
}

hi_s32 SD3403_VI::start_one_pipe(hi_vi_pipe vi_pipe, const sd3403_vi_pipe_info *pipe_info)
{
     hi_s32 ret;

    ret = hi_mpi_vi_create_pipe(vi_pipe, &pipe_info->pipe_attr);
    if (ret != HI_SUCCESS) {
        ss_print("vi create pipe(%d) failed with 0x%x!\n", vi_pipe, ret);
        return HI_FAILURE;
    }

    if (pipe_info->pipe_need_start == HI_TRUE) {
        ret = hi_mpi_vi_start_pipe(vi_pipe);
        if (ret != HI_SUCCESS) {
            ss_print("vi start pipe(%d) failed with 0x%x!\n", vi_pipe, ret);
            goto start_pipe_failed;
        }
    }

    ret = start_chn(vi_pipe, pipe_info->chn_info, pipe_info->chn_num);
    if (ret != HI_SUCCESS) {
        ss_print("vi pipe(%d) start chn failed! error code : %x\n", vi_pipe, ret);
        goto start_chn_failed;
    }

    return HI_SUCCESS;

start_chn_failed:
    hi_mpi_vi_stop_pipe(vi_pipe);
start_pipe_failed:
    hi_mpi_vi_destroy_pipe(vi_pipe);
    return HI_FAILURE;
}

hi_s32 SD3403_VI::stop_chn(hi_vi_pipe vi_pipe, const sd3403_vi_chn_info chn_info[], hi_u32 chn_num)
{
    hi_u32 i;
    hi_s32 ret;

    for (i = 0; i < chn_num; i++) {
        hi_vi_chn vi_chn = chn_info[i].vi_chn;

        ret = hi_mpi_vi_disable_chn(vi_pipe, vi_chn);
        if (ret != HI_SUCCESS) {
            ss_print("vi disable chn(%d) failed with 0x%x!\n", vi_chn, ret);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VI::start_chn(hi_vi_pipe vi_pipe, const sd3403_vi_chn_info chn_info[], hi_u32 chn_num)
{
    hi_u32 i;
    hi_s32 ret;

    for (i = 0; i < chn_num; i++) {
        hi_vi_chn vi_chn = chn_info[i].vi_chn;
        const hi_vi_chn_attr *chn_attr = &chn_info[i].chn_attr;

        ret = hi_mpi_vi_set_chn_attr(vi_pipe, vi_chn, chn_attr);
        if (ret != HI_SUCCESS) {
            ss_print("vi set chn(%d) attr failed with 0x%x!\n", vi_chn, ret);
            return HI_FAILURE;
        }

        ret = hi_mpi_vi_enable_chn(vi_pipe, vi_chn);
        if (ret != HI_SUCCESS) {
            ss_print("vi enable chn(%d) failed with 0x%x!\n", vi_chn, ret);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VI::set_grp_info(const sd3403_vi_grp_info *grp_info)
{
    hi_s32 ret;
    hi_u32 i;
    for (i = 0; i < grp_info->grp_num; i++) {
        ret = hi_mpi_vi_set_wdr_fusion_grp_attr(grp_info->fusion_grp[i], &grp_info->fusion_grp_attr[i]);
        if (ret != HI_SUCCESS) {
            ss_print("vi set wdr fusion grp attr failed!\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}   


hi_s32 SD3403_VI::dev_bind_pipe(hi_vi_dev vi_dev, const hi_vi_bind_pipe *bind_pipe)
{
    hi_u32 i;
    hi_s32 j;
    hi_s32 ret;

    for (i = 0; i < bind_pipe->pipe_num; i++) {
        ret = hi_mpi_vi_bind(vi_dev, bind_pipe->pipe_id[i]);
        if (ret != HI_SUCCESS) {
            ss_print("vi dev(%d) bind pipe(%d) failed!\n", vi_dev, bind_pipe->pipe_id[i]);
            goto exit;
        }
    }

    return HI_SUCCESS;

exit:
    for (j = i - 1; j >= 0; j--) {
        ret = hi_mpi_vi_unbind(vi_dev, bind_pipe->pipe_id[j]);
        if (ret != HI_SUCCESS) {
            ss_print("vi dev(%d) unbind pipe(%d) failed!\n", vi_dev, bind_pipe->pipe_id[j]);
        }
    }
    return HI_FAILURE;
}

hi_s32 SD3403_VI::start_dev(hi_vi_dev vi_dev, const hi_vi_dev_attr *dev_attr, const hi_vi_bas_attr *bas_attr)
{
    hi_s32 ret;

    ret = hi_mpi_vi_set_dev_attr(vi_dev, dev_attr);
    if (ret != HI_SUCCESS) {
        ss_print("vi set dev attr failed with 0x%x!\n", ret);
        return HI_FAILURE;
    }

    if ((bas_attr->enable == HI_TRUE) && (vi_dev == 0)) {
        ret = hi_mpi_vi_set_bas_attr(vi_dev, bas_attr);
        if (ret != HI_SUCCESS) {
            ss_print("vi set bas attr failed with 0x%x!\n", ret);
            return HI_FAILURE;
        }
    }

    ret = hi_mpi_vi_enable_dev(vi_dev);
    if (ret != HI_SUCCESS) {
        ss_print("vi enable dev failed with 0x%x!\n", ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VI::start_mipi_rx(const sd3403_sns_info *sns_info, const sd3403_mipi_info *mipi_info)
{
    hi_s32 ret;

    ret = set_mipi_hs_mode(mipi_info->divide_mode);
    if (ret != HI_SUCCESS) {
        ss_print("mipi rx set hs_mode failed!\n");
        return HI_FAILURE;
    }

    ret = mipi_ctrl_cmd(mipi_info->mipi_dev, HI_MIPI_ENABLE_MIPI_CLOCK);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d enable mipi rx clock failed!\n", mipi_info->mipi_dev);
        return HI_FAILURE;
    }

    ret = mipi_ctrl_cmd(mipi_info->mipi_dev, HI_MIPI_RESET_MIPI);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d reset mipi rx failed!\n",mipi_info->mipi_dev);
        return HI_FAILURE;
    }

    ret = mipi_ctrl_cmd(sns_info->sns_clk_src, HI_MIPI_ENABLE_SENSOR_CLOCK);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d enable sensor clock failed!\n", sns_info->sns_clk_src);
        return HI_FAILURE;
    }

    ret = mipi_ctrl_cmd(sns_info->sns_rst_src, HI_MIPI_RESET_SENSOR);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d reset sensor failed!\n", sns_info->sns_rst_src);
        return HI_FAILURE;
    }

    ret = set_mipi_combo_attr(&mipi_info->combo_dev_attr);
    if (ret != HI_SUCCESS) {
        ss_print("mipi rx set combo attr failed!\n");
        return HI_FAILURE;
    }

    ret = set_mipi_ext_data_type_attr(&mipi_info->ext_data_type_attr);
    if (ret != HI_SUCCESS) {
        ss_print("mipi rx set ext data attr failed!\n");
        return HI_FAILURE;
    }

    ret = mipi_ctrl_cmd(mipi_info->mipi_dev, HI_MIPI_UNRESET_MIPI);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d unreset mipi rx failed!\n", mipi_info->mipi_dev);
        return HI_FAILURE;
    }

    ret = mipi_ctrl_cmd(sns_info->sns_rst_src, HI_MIPI_UNRESET_SENSOR);
    if (ret != HI_SUCCESS) {
        ss_print("devno %d unreset sensor failed!\n", sns_info->sns_rst_src);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_void SD3403_VI::get_default_cfg(hi_void)
{
    memset(vi_cfg, 0, sizeof(sd3403_vi_cfg)*2);
    const hi_vi_dev vi_dev = 2; /* dev2 for sensor1 */
    const hi_vi_pipe vi_pipe = 1; /* dev2 bind pipe1 */

    get_default_vi_cfg(&vi_cfg[0]);
    get_default_vi_cfg(&vi_cfg[1]);

    vi_cfg[0].mipi_info.divide_mode = LANE_DIVIDE_MODE_1;

    vi_cfg[1].sns_info.bus_id = 2; /* i2c5 */
    vi_cfg[1].sns_info.sns_clk_src = 1;
    vi_cfg[1].sns_info.sns_rst_src = 1;

    get_mipi_info_by_dev_id(vi_dev, &vi_cfg[1].mipi_info);
    vi_cfg[1].dev_info.vi_dev = vi_dev;
    vi_cfg[1].bind_pipe.pipe_id[0] = vi_pipe;
    vi_cfg[1].grp_info.grp_num = 1;
    vi_cfg[1].grp_info.fusion_grp[0] = 1;
    vi_cfg[1].grp_info.fusion_grp_attr[0].pipe_id[0] = vi_pipe;

}

hi_void SD3403_VI::get_mipi_info_by_dev_id(hi_vi_dev vi_dev, sd3403_mipi_info *mipi_info)
{
    mipi_info->mipi_dev    = vi_dev;
    mipi_info->divide_mode = LANE_DIVIDE_MODE_1;
    get_mipi_attr_by_dev_id(vi_dev, &mipi_info->combo_dev_attr);
    get_mipi_ext_data_attr(&mipi_info->ext_data_type_attr);
    mipi_info->ext_data_type_attr.devno = vi_dev;
}

hi_void SD3403_VI::get_mipi_attr_by_dev_id(hi_vi_dev vi_dev,combo_dev_attr_t *combo_attr)
{
    hi_u32 ob_height = OB_HEIGHT_START;
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
            ob_height = OB_HEIGHT_END;
            if (vi_dev == 0) {
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 2) { /* dev2 */
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_nowdr_dev2_attr, sizeof(combo_dev_attr_t));
            }
            break;

        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
            if (vi_dev == 0) {
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_os08b10_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 2) { /* dev2 */
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_os08b10_12bit_8m_nowdr_dev2_attr, sizeof(combo_dev_attr_t));
            }
            break;

        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
            get_os05a10_mipi_attr(vi_dev, combo_attr);
            break;

        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
            if (vi_dev == 0) {
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 2) { /* dev2 */
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_dev2_attr, sizeof(combo_dev_attr_t));
            }
            break;

        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            ob_height = IMX347_OB_HEIGHT_END;
            if (vi_dev == 0) {
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_imx347_slave_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 2) { /* dev2 */
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_imx347_slave_12bit_4m_nowdr_dev2_attr, sizeof(combo_dev_attr_t));
            }
            break;

        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
            ob_height = IMX485_OB_HEIGHT_END;
            if (vi_dev == 0) {
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_imx485_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 2) { /* dev2 */
                (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn2_sensor_imx485_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
            }
            break;

        default:
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
    }
    combo_attr->img_rect.height = combo_attr->img_rect.height + ob_height;
}

hi_void SD3403_VI::get_os05a10_mipi_attr(hi_vi_dev vi_dev, combo_dev_attr_t *combo_attr)
{
    if (vi_dev == 0) {
        (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
            &g_mipi_2lane_chn0_sensor_os05a10_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
    } else if (vi_dev == 1) {
        (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
            &g_mipi_2lane_chn1_sensor_os05a10_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
    } else if (vi_dev == 2) { /* dev2 */
        (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
            &g_mipi_2lane_chn2_sensor_os05a10_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
    } else if (vi_dev == 3) { /* dev3 */
        (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
            &g_mipi_2lane_chn3_sensor_os05a10_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
    }
}


hi_void SD3403_VI::get_default_vi_cfg(sd3403_vi_cfg *vi_cfg)
{
    (hi_void)memset_s(vi_cfg, sizeof(sd3403_vi_cfg), 0, sizeof(sd3403_vi_cfg));

    /* sensor info */
    get_default_sns_info(&vi_cfg->sns_info);
    /* mipi info */
    get_default_mipi_info(&vi_cfg->mipi_info);
    /* dev info */
    get_default_dev_info(&vi_cfg->dev_info);
    /* bind info */
    get_default_bind_info(&vi_cfg->bind_pipe);
    /* grp info */
    get_default_grp_info(&vi_cfg->grp_info);
    /* pipe info */
    get_default_pipe_info(&vi_cfg->bind_pipe, vi_cfg->pipe_info);
}


hi_void SD3403_VI::get_default_sns_info(sd3403_sns_info *sns_info)
{
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    sns_info->sns_type=sns_type;
    sns_info->sns_clk_src = 0;
    sns_info->sns_rst_src = 0;
    sns_info->bus_id = 5; //i2c2
}

hi_void SD3403_VI::get_default_mipi_info(sd3403_mipi_info *mipi_info)
{
    mipi_info->mipi_dev    = 0;
    mipi_info->divide_mode = LANE_DIVIDE_MODE_0;
    get_mipi_attr(&mipi_info->combo_dev_attr);
    get_mipi_ext_data_attr(&mipi_info->ext_data_type_attr);
}

hi_void SD3403_VI::get_mipi_attr(combo_dev_attr_t *combo_attr)
{
    hi_u32 ob_height = OB_HEIGHT_START;
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
            ob_height = OB_HEIGHT_END;
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
            break;

        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
            ob_height = OB_HEIGHT_END;
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_wdr2to1_attr, sizeof(combo_dev_attr_t));
            break;

        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
            break;

        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_os08b10_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
            break;

        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_os08b10_12bit_8m_wdr2to1_attr, sizeof(combo_dev_attr_t));
            break;

        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_2lane_chn0_sensor_os05a10_12bit_4m_nowdr_single_attr, sizeof(combo_dev_attr_t));
            break;

        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            ob_height = IMX347_OB_HEIGHT_END;
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_imx347_slave_12bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
            break;

        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
            ob_height = IMX485_OB_HEIGHT_END;
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_imx485_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
            break;

        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_8lane_chn0_sensor_imx485_10bit_8m_wdr3to1_attr, sizeof(combo_dev_attr_t));
            break;

        default:
            (hi_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_nowdr_attr, sizeof(combo_dev_attr_t));
    }
    combo_attr->img_rect.height = combo_attr->img_rect.height + ob_height;
}

hi_void SD3403_VI::get_mipi_ext_data_attr(ext_data_type_t *ext_data_attr)
{
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            (hi_void)memcpy_s(ext_data_attr, sizeof(ext_data_type_t),
                &g_mipi_ext_data_type_os08a20_12bit_8m_nowdr_attr, sizeof(ext_data_type_t));
            break;

        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
            (hi_void)memcpy_s(ext_data_attr, sizeof(ext_data_type_t),
                &g_mipi_ext_data_type_default_attr, sizeof(ext_data_type_t));
            break;

        default:
            (hi_void)memcpy_s(ext_data_attr, sizeof(ext_data_type_t),
                &g_mipi_ext_data_type_default_attr, sizeof(ext_data_type_t));
    }
}

hi_s32 SD3403_VI::set_mipi_hs_mode(lane_divide_mode_t hs_mode)
{
    hi_s32 fd;
    hi_s32 ret;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ss_print("open %s failed!\n", MIPI_DEV_NAME);
        return HI_FAILURE;
    }

    ret = ioctl(fd, HI_MIPI_SET_HS_MODE, &hs_mode);

    close(fd);

    return ret;
}


hi_void SD3403_VI::get_default_dev_info(sd3403_vi_dev_info *dev_info)
{
    hi_size size;
    hi_u32 ob_height;
    hi_vi_intf_mode intf_mode = HI_VI_INTF_MODE_MIPI;
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    dev_info->vi_dev = 0;
    switch (intf_mode) {
        case HI_VI_INTF_MODE_MIPI:
            (hi_void)memcpy_s(&dev_info->dev_attr, sizeof(hi_vi_dev_attr), &g_mipi_raw_dev_attr, sizeof(hi_vi_dev_attr));
            break;

        default:
            (hi_void)memcpy_s(&dev_info->dev_attr, sizeof(hi_vi_dev_attr), &g_mipi_raw_dev_attr, sizeof(hi_vi_dev_attr));
            break;
    }
    if (sns_type == SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1) {
        dev_info->dev_attr.data_rate = HI_DATA_RATE_X2;
    }
    sdCom.get_size_by_sns_type(&size);
    ob_height = get_obheight_by_sns_type();
    dev_info->dev_attr.in_size.width  = size.width;
    dev_info->dev_attr.in_size.height = size.height + ob_height;
    dev_info->bas_attr.enable = HI_FALSE;
    return;
}

hi_s32 SD3403_VI::get_obheight_by_sns_type(hi_void)
{
    hi_u32 ob_height = OB_HEIGHT_START;
    sd3403_sns_type sns_type = sdCom.get_sns_type();


    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
            ob_height = OB_HEIGHT_END;
            break;
        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
            ob_height = OB_HEIGHT_END;
            break;
        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            ob_height = IMX347_OB_HEIGHT_END;
            break;
        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
            ob_height = IMX485_OB_HEIGHT_END;
            break;
        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
            ob_height = OB_HEIGHT_START;
            break;
        default:
            break;
    }

    return ob_height;
}

hi_void SD3403_VI::get_default_bind_info(hi_vi_bind_pipe *bind_pipe)
{
    hi_u32 i;
    bind_pipe->pipe_num = get_pipe_num_by_sns_type();
    for (i = 0; i < bind_pipe->pipe_num; i++) {
        bind_pipe->pipe_id[i] = i;
    }
    return;
}

hi_void SD3403_VI::get_default_grp_info(sd3403_vi_grp_info *grp_info)
{
    hi_u32 i;
    hi_u32 pipe_num;
    hi_size size;

    sdCom.get_size_by_sns_type(&size);
    grp_info->grp_num = 1;
    grp_info->fusion_grp[0] = 0;
    grp_info->fusion_grp_attr[0].wdr_mode = get_wdr_mode_by_sns_type();
    grp_info->fusion_grp_attr[0].cache_line = size.height;
    pipe_num = get_pipe_num_by_sns_type();
    for (i = 0; i < pipe_num; i++) {
        grp_info->fusion_grp_attr[0].pipe_id[i] = i;
    }

}

hi_wdr_mode SD3403_VI::get_wdr_mode_by_sns_type(hi_void)
{
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            return HI_WDR_MODE_NONE;

        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
            return HI_WDR_MODE_2To1_LINE;

        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
            return HI_WDR_MODE_3To1_LINE;

        default:
            return HI_WDR_MODE_NONE;
    }
}

hi_s32 SD3403_VI::get_pipe_num_by_sns_type(hi_void)
{
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
            return 1;

        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
            return 2; /* 2 pipe */

        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
            return 3; /* 3 pipe */

        default:
            return 1;
    }
}

hi_s32 SD3403_VI::isp_get_pub_attr_by_sns(hi_isp_pub_attr *pub_attr)
{
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_os08a20_mipi_8m_30fps, sizeof(hi_isp_pub_attr));
            break;

        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_os08a20_mipi_8m_30fps_wdr2to1, sizeof(hi_isp_pub_attr));
            break;

        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_os04a10_mipi_4m_30fps, sizeof(hi_isp_pub_attr));
            break;

        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_os08b10_mipi_8m_30fps, sizeof(hi_isp_pub_attr));
            break;

        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_os08b10_mipi_8m_30fps_wdr2to1, sizeof(hi_isp_pub_attr));
            break;

        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_os05a10_2l_mipi_4m_30fps, sizeof(hi_isp_pub_attr));
            break;

        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_imx347_slave_mipi_4m_30fps, sizeof(hi_isp_pub_attr));
            break;

        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_imx485_mipi_8m_30fps, sizeof(hi_isp_pub_attr));
            break;

        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_imx485_mipi_8m_30fps_wdr3to1, sizeof(hi_isp_pub_attr));
            break;

        default:
            (hi_void)memcpy_s(pub_attr, sizeof(hi_isp_pub_attr),
                &g_isp_pub_attr_os08a20_mipi_8m_30fps, sizeof(hi_isp_pub_attr));
            break;
    }

    return HI_SUCCESS;
}

hi_void SD3403_VI::get_default_pipe_info(hi_vi_bind_pipe *bind_pipe,sd3403_vi_pipe_info pipe_info[])
{
    hi_u32 i;
    hi_size size;
    sd3403_sns_type sns_type = sdCom.get_sns_type();

    sdCom.get_size_by_sns_type(&size);
    for (i = 0; i < bind_pipe->pipe_num; i++) {
        /* pipe attr */
        pipe_info[i].pipe_attr.pipe_bypass_mode               = HI_VI_PIPE_BYPASS_NONE;
        pipe_info[i].pipe_attr.isp_bypass                     = HI_FALSE;
        pipe_info[i].pipe_attr.size.width                     = size.width;
        pipe_info[i].pipe_attr.size.height                    = size.height;
        pipe_info[i].pipe_attr.pixel_format                   = HI_PIXEL_FORMAT_RGB_BAYER_12BPP;
        pipe_info[i].pipe_attr.compress_mode                  = HI_COMPRESS_MODE_LINE;
        pipe_info[i].pipe_attr.bit_width                      = HI_DATA_BIT_WIDTH_8;
        pipe_info[i].pipe_attr.bit_align_mode                 = HI_VI_BIT_ALIGN_MODE_HIGH;
        pipe_info[i].pipe_attr.frame_rate_ctrl.src_frame_rate = -1;
        pipe_info[i].pipe_attr.frame_rate_ctrl.dst_frame_rate = -1;

        if (sns_type == SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1) {
            pipe_info[i].pipe_attr.pixel_format  = HI_PIXEL_FORMAT_RGB_BAYER_10BPP;
            pipe_info[i].pipe_attr.compress_mode = HI_COMPRESS_MODE_NONE;
        }

        pipe_info[i].pipe_need_start = HI_TRUE;
        pipe_info[i].isp_need_run = HI_TRUE;

        /* pub attr */
        isp_get_pub_attr_by_sns(&pipe_info[i].isp_info.isp_pub_attr);

        /* chn info */
        pipe_info[i].chn_num = 1;
        pipe_info[i].chn_info[0].vi_chn                                  = 0;
        pipe_info[i].chn_info[0].chn_attr.size.width                     = size.width;
        pipe_info[i].chn_info[0].chn_attr.size.height                    = size.height;
        pipe_info[i].chn_info[0].chn_attr.pixel_format                   = HI_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        pipe_info[i].chn_info[0].chn_attr.dynamic_range                  = HI_DYNAMIC_RANGE_SDR8;
        pipe_info[i].chn_info[0].chn_attr.video_format                   = HI_VIDEO_FORMAT_LINEAR;
        pipe_info[i].chn_info[0].chn_attr.compress_mode                  = HI_COMPRESS_MODE_NONE;
        pipe_info[i].chn_info[0].chn_attr.mirror_en                      = HI_FALSE;
        pipe_info[i].chn_info[0].chn_attr.flip_en                        = HI_FALSE;
        pipe_info[i].chn_info[0].chn_attr.depth                          = 0;
        pipe_info[i].chn_info[0].chn_attr.frame_rate_ctrl.src_frame_rate = -1;
        pipe_info[i].chn_info[0].chn_attr.frame_rate_ctrl.dst_frame_rate = -1;
    }
}


hi_s32 SD3403_VI::mipi_ctrl_cmd(hi_u32 devno, hi_u32 cmd)
{
    hi_s32 ret;
    hi_s32 fd;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ss_print("open %s failed!\n", MIPI_DEV_NAME);
        return HI_FAILURE;
    }

    ret = ioctl(fd, cmd, &devno);

    close(fd);

    return ret;
}

hi_s32 SD3403_VI::set_mipi_combo_attr(const combo_dev_attr_t *combo_dev_attr)
{
    hi_s32 fd;
    hi_s32 ret;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ss_print("open %s failed!\n", MIPI_DEV_NAME);
        return HI_FAILURE;
    }

    ret = ioctl(fd, HI_MIPI_SET_DEV_ATTR, combo_dev_attr);

    close(fd);

    return ret;
}

hi_s32 SD3403_VI::set_mipi_ext_data_type_attr(const ext_data_type_t *ext_data_type_attr)
{
    hi_s32 fd;
    hi_s32 ret;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ss_print("open %s failed!\n", MIPI_DEV_NAME);
        return HI_FAILURE;
    }

    ret = ioctl(fd, HI_MIPI_SET_EXT_DATA_TYPE, ext_data_type_attr);

    close(fd);

    return ret;
}



