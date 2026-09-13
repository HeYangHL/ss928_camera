#include "sd3403_manage.hpp"

int SD3403_Manage::g_sig_flag = 0;
sem_t SD3403_Manage::mutex;

SD3403_Manage::SD3403_Manage()
{
}
SD3403_Manage::~SD3403_Manage()
{
}

int SD3403_Manage::Run_Fifo_Thread(void)
{
    for(int i = 0; i < grp_num; i++)
    {
        Venc_Thread = new std::thread(&SD3403_Manage::sensor_venc_stream, this, venc_chn[i]);
    }
    return 0;
}
#ifndef RTSP_LIVE555_ON
int SD3403_Manage::sensor0_rv(void *opaque, uint8_t *data, bool keyFrame, uint64_t *pts)
{
    int ret = 0;
    int framesize = 0;
    uint64_t ts_pts = 0;
    uint64_t rtp_pts = 0;
    uint64_t pts_nal = 0;
    int8_t *frame= nullptr;
    bool key = false;
    SD3403_Manage *manage = (SD3403_Manage *)opaque;

    while (1)
    {
        
        if(keyFrame)
        {
            manage->sd3403_venc.sensor0_keyFrame = true;
            manage->get_sensor0_fifo().clean_enc_fifo();
        }
        ret = manage->get_sensor0_fifo().read_enc(&frame, &framesize, &ts_pts, &key, 100);
        if (ret > 0)
        {
            if(keyFrame)
                if(!key)
                    continue;
                    
            memcpy(data, frame, framesize);
            *pts = ts_pts;
            if(frame != nullptr)
            {
                delete [] frame;
                frame = NULL;
            }
            break;
        }
        else
            usleep(1000);
    }

    return ret;
}

int SD3403_Manage::sensor1_rv(void *opaque, uint8_t *data, bool keyFrame, uint64_t *pts)
{
    int ret = 0;
    int framesize = 0;
    uint64_t ts_pts = 0;
    uint64_t rtp_pts = 0;
    uint64_t pts_nal = 0;
    int8_t *frame= nullptr;
    bool key = false;
    SD3403_Manage *manage = (SD3403_Manage *)opaque;

    while (1)
    {
        if(keyFrame)
        {
            manage->sd3403_venc.sensor1_keyFrame = true;
            manage->get_sensor1_fifo().clean_enc_fifo();
        }
        ret = manage->get_sensor1_fifo().read_enc(&frame, &framesize, &ts_pts, &key, 100);
        if (ret > 0)
        {
            memcpy(data, frame, framesize);
            *pts = ts_pts;
            if(frame != NULL)
            {
                delete [] frame;
                frame = NULL;
            }
            break;
        }
        else
            usleep(1000);
    }

    return ret;
}
#endif
hi_void SD3403_Manage::start_rtsp(hi_void)
{
    printf("[DEBUG]===>start rtsp server!\n");
#ifndef RTSP_LIVE555_ON
    rtsp_s.start();
    rtsp_s.add_media_source("sensor0_media", sensor0_rv, this, NULL, NULL, 30, 4800, "h265", "aac");
    rtsp_s.add_media_source("sensor1_media", sensor1_rv, this, NULL, NULL, 30, 4800, "h265", "aac");
#else
    live555_rtsp = new std::thread(&SD3403_Manage::create_live555_rtsp, this);
    manager_rtsp_stream_init();
#endif
}

void SD3403_Manage::get_points_coll(hi_video_frame_info *frame_info, std::vector<CenterPoint> &center)
{
    int ret = 0;
    std::vector<objinfo> output;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char name[1024] = {0};
    long seconds = ts.tv_sec;           // 秒
    long nanoseconds = ts.tv_nsec;       // 纳秒
    long microseconds = nanoseconds / 1000;  // 转换为微秒

    printf("get vpss channel 1 yuv, w : %d, h : %d\n", frame_info->video_frame.width, frame_info->video_frame.height);
    ret = svp.sd3403_rfcn_svp_npu_model_inference((const unsigned char *)frame_info->video_frame.virt_addr[0], output);
    // sprintf(name, "%ld_%ld.yuv", seconds, microseconds);
    // int fd = open(name, O_CREAT | O_WRONLY);
    // write(fd, frame_info->video_frame.virt_addr[0], 640*640*1.5);
    // close(fd);
    printf("output num : %d\n", output.size());
    for (int i = 0; i < output.size(); i++)
    {
        cv::Point p1, p2;
        float w = float(WIDTH_3840) / 640;
        float h = float(HEIGHT_2160) / 640;
        // printf("x1 : %f, y1 : %f, x2 : %f, y2: %f\n", output[i].x1, output[i].y1, output[i].x2, output[i].y2);
        p1.x = output[i].x1 * w;
        p1.y = output[i].y1 * h;
        p2.x = output[i].x2 * w;
        p2.y = output[i].y2 * h;
        // printf("2 x1 : %d, 2 y1 : %d, 2 x2 : %d, 2 y2: %d\n", p1.x, p1.y, p2.x, p2.y);
        CenterPoint cer;
        cer.x = (p2.x - p1.x) / 2.0 + p1.x;
        cer.y = (p2.y - p1.y) / 2.0 + p1.y;
        cer.w = p2.x - p1.x;
        cer.h = p2.y - p1.y;
        center.push_back(cer);
    }

    return;
}

/**********************************************************
函数描述：
    获取框框的信息集合，目的是获取车辆id
参数：
    center：框框的中心点信息集合
返回值：
    框框的左上角坐标，长宽 id 类型等信息集合
************************************************************/
std::vector<RetureInfo> SD3403_Manage::get_track_id(void *handle, std::vector<CenterPoint> &center)
{
    std::vector<float> carsPoint;
    int id = 0;
    int type = 0;

    for (int i = 0; i < center.size(); i++)
    {
        carsPoint.push_back(id++);
        carsPoint.push_back(center[i].x - center[i].w / 2);
        carsPoint.push_back(center[i].y - center[i].h / 2);
        carsPoint.push_back(center[i].w);
        carsPoint.push_back(center[i].h);
        carsPoint.push_back(type);
    }

    std::vector<RetureInfo> returninfos = GetTrackInfos(handle, carsPoint);
    printf("--->3 returninfos size : %d\n", returninfos.size());
    return returninfos;
}

void SD3403_Manage::send_yuv_to_model_pthread(void)
{
    hi_video_frame_info vpss_frame;
    int ret = 0;
    const char *det_vl_modelPath = "./model/yolov5s_yuv_original.om";
    TrackParam param;
    param.max_false_age = 1; // 误检的车辆存在的最大周期限制
    param.max_lost_age = 5;  // 长期丢失的车辆存在的最大周期限制
    param.min_hits = 1;
    param.iouThreshold = 0.3;

    void *handle = InitPara(param);
    hi_vpss_grp *vpss_grp = sd3403_vpss.get_vpss_grp();
    ret = svp.sd3403_rfcn_svp_npu_model_init(det_vl_modelPath);

    model_run = true;
    while(model_run)
    {
        std::vector<CenterPoint> center;
        std::vector<RetureInfo> returninfos;
        ss_print("======>%d, %d\n", sizeof(center), sizeof(returninfos));
        ret = sd3403_vpss.get_model_yuv(vpss_grp[0], vpss_frame);
        if(ret < 0)
        {
            usleep(500000);
            continue;
        }
        sd3403_sys.sys_mmap_cached(vpss_frame);
        get_points_coll(&vpss_frame, center);
        ss_print("--->center size : %d\n", center.size());
        returninfos = get_track_id(handle, center);
        ss_print("--->returninfos size : %d\n", returninfos.size());
        sd3403_rgn.show_rgn(returninfos);
        sd3403_vpss.release_model_vpss_chn(vpss_grp[0], vpss_frame);
        usleep(500000);
    }
    printf("=====>deint svp model start!\n");
    svp.sd3403_rfcn_svp_npu_model_deinit();
    printf("====>end yuv thread!\n");
    Release(handle);

    return;
}

hi_s32 SD3403_Manage::start_multi_vi_vpss(hi_s32 dev_num, hi_s32 grp_num)
{
    hi_s32 ret;
    hi_s32 i, j;
    hi_size in_size;
    sd3403_vi_cfg *vi_cfg = sd3403_vi.get_vi_cfg();
    hi_vpss_grp *vpss_grp = sd3403_vpss.get_vpss_grp();

    if (dev_num != grp_num)
    {
        return HI_FAILURE;
    }

    sdCom.get_size_by_sns_type(&in_size);

    for (i = 0; i < dev_num; i++)
    {
        ret = sd3403_vi.start_vi(&vi_cfg[i]);
        if (ret != HI_SUCCESS)
        {
            goto start_vi_failed;
        }
    }

    for (i = 0; i < grp_num; i++)
    {
        sd3403_sys.vi_bind_vpss(i, 0, vpss_grp[i], 0);
    }

    for (i = 0; i < grp_num; i++)
    {
        ret = sd3403_vpss.start_vpss(vpss_grp[i], &in_size);
        if (ret != HI_SUCCESS)
        {
            goto start_vpss_failed;
        }
    }

    return HI_SUCCESS;

start_vpss_failed:
    for (j = i - 1; j >= 0; j--)
    {
        sd3403_vpss.stop_vpss(vpss_grp[j]);
    }

    for (i = 0; i < grp_num; i++)
    {
        sd3403_sys.vi_un_bind_vpss(i, 0, vpss_grp[i], 0);
    }

start_vi_failed:
    for (j = i - 1; j >= 0; j--)
    {
        sd3403_vi.stop_vi(&vi_cfg[j]);
    }

    return HI_FAILURE;
}
hi_s32 SD3403_Manage::enable_sys(hi_void)
{
    hi_s32 ret = 0;

    ret = sd3403_sys.sys_init(HI_VI_OFFLINE_VPSS_OFFLINE, HI_VI_VIDEO_MODE_NORM, VB_DOUBLE_YUV_CNT, VB_WDR_RAW_CNT);
    if (ret != HI_SUCCESS)
    {
        ss_print("sys init failed !\n");
        return HI_FAILURE;
    }
}

hi_s32 SD3403_Manage::enable_vi_vpss(hi_void)
{
    hi_s32 ret = 0;

    sd3403_vi.get_default_cfg();
    Yuvto_Model = new std::thread(&SD3403_Manage::send_yuv_to_model_pthread, this);
    ret = start_multi_vi_vpss(2, 2);
    if(ret != HI_SUCCESS)
    {
        ss_print("start_multi_vi_vpss error!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

int SD3403_Manage::request_key_frame(int venc_chn)
{
    int s32Ret = 0;

    if (sd3403_venc.sensor0_keyFrame)
    {
        if (venc_chn == 0)
        {
            printf("======>start get sensor0 key frame!\n");
            s32Ret = sd3403_venc.request_key_frame(venc_chn);
            if (HI_SUCCESS != s32Ret)
            {
                printf("chn : %d get IDR frame error!\n", venc_chn);
                return -1;
            }
            sd3403_venc.sensor0_keyFrame = false;
            // Sensor0_Fifo.clean_enc_fifo();
        }
    }
    if (sd3403_venc.sensor1_keyFrame)
    {
        if (venc_chn == 1)
        {
            s32Ret = sd3403_venc.request_key_frame(venc_chn);
            if (HI_SUCCESS != s32Ret)
            {
                printf("chn : %d get IDR frame error!\n", venc_chn);
                return -1;
            }
            sd3403_venc.sensor1_keyFrame = false;
            // Sensor1_Fifo.clean_enc_fifo();
        }
    }
    return 0;
}

hi_void SD3403_Manage::sensor_venc_stream(int chn)
{
    bool keyFrame = false;
    int enc_len = 0;
    uint64_t o_pts = 0;
    int ret = 0;
    char thread_name[32] = {0};

    sprintf(thread_name, "thread_chn_%d", chn);
    pthread_setname_np(pthread_self(), thread_name);
    _enc_run = true;
    while (_enc_run)
    {
        char *outFrame = nullptr;
        outFrame = new char[1024 * 1024];
        ret = request_key_frame(chn);
        if(ret < 0)
        {
            delete[] outFrame;
            outFrame = nullptr;
            continue;
        }
        enc_len = sd3403_venc.getSensor_Frame((char *)outFrame, &o_pts, 100, keyFrame, chn);
        if (enc_len <= 0)
        {
            delete[] outFrame;
            outFrame = nullptr;
            continue;
        }
        if (chn == 0)
        {
            ret = Sensor0_Fifo.write_enc((int8_t *)outFrame, enc_len, o_pts, keyFrame, 10);
            if (ret <= 0)
            {
                delete[] outFrame;
                outFrame = nullptr;
                continue;
            }
        }
        else if (chn == 1)
        {
            ret = Sensor1_Fifo.write_enc((int8_t *)outFrame, enc_len, o_pts, keyFrame, 10);
            if (ret <= 0)
            {
                delete[] outFrame;
                outFrame = nullptr;
                continue;
            }
        }
    }
    return;
}

hi_s32 SD3403_Manage::enable_venc(hi_void)
{
    hi_size in_size;
    const hi_vpss_chn vpss_chn = 0;
    int i = 0;

    hi_vpss_grp *vpss_grp = sd3403_vpss.get_vpss_grp();

    sdCom.get_size_by_sns_type(&in_size);

    sd3403_venc.start_venc(venc_chn, grp_num, in_size);

    for (i = 0; i < grp_num; i++)
    {
        sd3403_sys.vpss_bind_venc(vpss_grp[i], vpss_chn, venc_chn[i]);
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_Manage::disable_rgn(hi_void)
{
    hi_s32 ret = 0;

    ret = sd3403_rgn.clean_all_rgn();
    if(ret != HI_SUCCESS)
    {
        ss_print("clean all rgn error!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_Manage::disable_venc(hi_void)
{
    hi_u32 i;
    const hi_vpss_chn vpss_chn = 0;
    hi_vpss_grp *vpss_grp = sd3403_vpss.get_vpss_grp();

    for (i = 0; i < grp_num; i++)
    {
        sd3403_sys.vpss_un_bind_venc(vpss_grp[i], vpss_chn, venc_chn[i]);
    }

    sd3403_venc.stop_venc(venc_chn, grp_num);
}

hi_s32 SD3403_Manage::disable_vpss(hi_void)
{
    hi_vpss_grp *vpss_grp = sd3403_vpss.get_vpss_grp();
    sd3403_vpss.stop_vpss(vpss_grp[0]);
    sd3403_vpss.stop_vpss(vpss_grp[1]);
    for (int i = 0; i < 2; i++)
    {
        sd3403_sys.vi_un_bind_vpss(i, 0, vpss_grp[i], 0);
    }
}
hi_s32 SD3403_Manage::disable_vi(hi_void)
{
    sd3403_vi_cfg *vi_cfg = sd3403_vi.get_vi_cfg();
    sd3403_vi.stop_vi(&vi_cfg[0]);
    sd3403_vi.stop_vi(&vi_cfg[1]);
}

hi_s32 SD3403_Manage::disable_sys(hi_void)
{
    sd3403_sys.sys_exit();
}

hi_s32 SD3403_Manage::get_char(hi_void)
{
    // if (sdCom.get_sig_flag() == 1)
    // {
    //     return -1;
    // }
    if (g_sig_flag == 1)
    {
        return -1;
    }
    ss_pause();
}

hi_void SD3403_Manage::sd3403_signal_handle(hi_void (*sig_handle)(hi_s32))
{
    struct sigaction sa;

    (hi_void)memset_s(&sa, sizeof(struct sigaction), 0, sizeof(struct sigaction));
    sa.sa_handler = sig_handle;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, HI_NULL);
    sigaction(SIGTERM, &sa, HI_NULL);
    
}
hi_void SD3403_Manage::handle_sig(hi_s32 signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        g_sig_flag = 1;
        sem_post(&mutex);
        while (1)
            pause();
    }
    return;
}

hi_void SD3403_Manage::signal_handle_thread(void)
{
    signal_run = true;

    while(1)
    {
        sem_wait(&mutex);
        printf("=====>start handle thread!\n");
        clean_rtsp();
        printf("1\n");
        stop_all_thread();
        printf("2\n");
        // svp.sd3403_rfcn_svp_npu_model_deinit();
        disable_rgn();
        printf("3\n");
        disable_venc();
        printf("4\n");
        disable_vpss();
        printf("5\n");
        disable_vi();
        printf("6\n");
        disable_sys();
        printf("=====>end handle thread!\n");
        if (g_sig_flag)
            _exit(EXIT_FAILURE);

    }
    sem_destroy(&mutex);
    return;
}

hi_void SD3403_Manage::destroy_signal_thread(void)
{
    sem_post(&mutex);
    return;
}

hi_void SD3403_Manage::register_signal(hi_void)
{
    // sdCom.sd3403_signal_handle();
    sem_init(&mutex, 0, 0);
    sd3403_signal_handle(handle_sig);
    Sig_Thread = new std::thread(&SD3403_Manage::signal_handle_thread, this);
}
hi_void get_venc_strream(hi_void)
{
}

hi_void SD3403_Manage::stop_all_thread(void)
{
    //release signal thread
    if(!g_sig_flag)
        destroy_signal_thread();
    printf("11\n");
    // if(Sig_Thread->joinable())
    //     Sig_Thread->join();

    //release model thread
    model_run = false;
    if(Yuvto_Model->joinable())
        Yuvto_Model->join();

    //release venc thread
    _enc_run = false;
    if(Venc_Thread->joinable())
        Venc_Thread->join();
#ifdef RTSP_LIVE555_ON
    rtsp.Stop();
    if(live555_rtsp->joinable())
        live555_rtsp->join();
#endif
    return;
}

hi_void SD3403_Manage::clean_rtsp(void)
{
#ifndef RTSP_LIVE555_ON
    rtsp_s.stop();
#else
    rtsp.Stop();
#endif
    return;
}

void SD3403_Manage::create_live555_rtsp(void)
{
    rtsp.Init();
    return;
}


hi_s32 SD3403_Manage::manager_rtsp_stream_init(void)
{
    g_rtsp_get_frame.Set_Video_Frame(
        [this](uint8_t **pack, int &size) -> int {
            return this->get_rtsp_stream(pack, size);
        }
    );
    g_rtsp_get_frame.Set_Release_Frame(
        [this](uint8_t *pack) {
            this->release_rtsp_stream(pack);
        }
    );
    return 0;
}

int SD3403_Manage::get_rtsp_stream(uint8_t **pack, int &size)
{
    int ret = 0;
    uint64_t ts_pts = 0;
    bool key = false;

    ret = Sensor0_Fifo.read_enc((int8_t **)pack, &size, &ts_pts, &key, 100);
    if(ret < 0)
    {
        printf("[DEBUG]===>get video stream error!\n");
        return -1;
    }

    return 0;
}

void SD3403_Manage::release_rtsp_stream(uint8_t *pack)
{
    if (pack != nullptr)
    {
        // 因为底层是 new char[] 分配的，所以这里必须用 delete[]
        delete[] (char*)pack;
        pack = nullptr;
    }
}

