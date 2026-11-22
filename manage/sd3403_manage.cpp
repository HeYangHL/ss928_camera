#include "sd3403_manage.hpp"
#include <new>

#define RTSP_LISTEN_PORT 8554

extern int g_h265_active_clients;
extern int g_h264_active_clients;

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
    for(hi_u32 i = 0; i < grp_num; i++)
    {
        Venc_Thread = new std::thread(&SD3403_Manage::sensor_venc_stream, this, venc_chn[i]);
    }
    Aenc_Thread = new std::thread(&SD3403_Manage::audio_aenc_stream, this);
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
    Live555_RTSPInit();
#endif
}

void SD3403_Manage::get_points_coll(hi_video_frame_info *frame_info, std::vector<CenterPoint> &center)
{
    std::vector<objinfo> output;

    printf("get vpss channel 1 yuv, w : %d, h : %d\n", frame_info->video_frame.width, frame_info->video_frame.height);
    svp.sd3403_rfcn_svp_npu_model_inference((const unsigned char *)frame_info->video_frame.virt_addr[0], output);
    // sprintf(name, "%ld_%ld.yuv", seconds, microseconds);
    // int fd = open(name, O_CREAT | O_WRONLY);
    // write(fd, frame_info->video_frame.virt_addr[0], 640*640*1.5);
    // close(fd);
    printf("output num : %zu\n", output.size());
    for (size_t i = 0; i < output.size(); i++)
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

    for (size_t i = 0; i < center.size(); i++)
    {
        carsPoint.push_back(id++);
        carsPoint.push_back(center[i].x - center[i].w / 2);
        carsPoint.push_back(center[i].y - center[i].h / 2);
        carsPoint.push_back(center[i].w);
        carsPoint.push_back(center[i].h);
        carsPoint.push_back(type);
    }

    std::vector<RetureInfo> returninfos = GetTrackInfos(handle, carsPoint);
    printf("--->3 returninfos size : %zu\n", returninfos.size());
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
        ss_print("======>%zu, %zu\n", sizeof(center), sizeof(returninfos));
        ret = sd3403_vpss.get_model_yuv(vpss_grp[0], vpss_frame);
        if(ret < 0)
        {
            usleep(500000);
            continue;
        }
        sd3403_sys.sys_mmap_cached(vpss_frame);
        get_points_coll(&vpss_frame, center);
        ss_print("--->center size : %zu\n", center.size());
        returninfos = get_track_id(handle, center);
        ss_print("--->returninfos size : %zu\n", returninfos.size());
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
    return HI_SUCCESS;
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
    _venc_run = true;
    while (_venc_run)
    {
        uint8_t *outFrame = nullptr;
        outFrame = new uint8_t[1024 * 1024];
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
            if (keyFrame) {
                Sensor0_IdrCache.Store((const int8_t *)outFrame, enc_len, o_pts);
            }
            // 由环形队列代替FIFO
            m_sensor0_vrb.PushVideoFrame(outFrame, enc_len, keyFrame, o_pts);
            outFrame = nullptr;
            continue;
        }
        else if (chn == 1)
        {
            if (keyFrame) {
                Sensor1_IdrCache.Store((const int8_t *)outFrame, enc_len, o_pts);
            }
            m_sensor1_vrb.PushVideoFrame(outFrame, enc_len, keyFrame, o_pts);
            outFrame = nullptr;
            continue;
        }
    }
    return;
}

hi_void SD3403_Manage::audio_aenc_stream(void)
{
    int enc_len = 0;
    uint64_t o_pts = 0;
    int seq = 0;
    char thread_name[32] = {0};
    char *outFrame = nullptr;

    sprintf(thread_name, "thread_aenc");
    pthread_setname_np(pthread_self(), thread_name);
    _aenc_run = true;

    outFrame = new char[MAX_AAC_MAINBUF_SIZE];
    if (outFrame == nullptr)
    {
        _aenc_run = false;
        return;
    }

    while(_aenc_run)
    {
        enc_len = sd3403_aenc.Sd3403_Get_Aenc_Stream(
                      outFrame, MAX_AAC_MAINBUF_SIZE, &o_pts, &seq, 100);
        if (enc_len <= 0)
            continue;

        // 音频帧需要拷贝后入队，outFrame 会被下一帧复用
        uint8_t *audioFrame = new (std::nothrow) uint8_t[enc_len];
        if (audioFrame == nullptr)
            continue;

        // ADTS: protection_absent=1 时头长 7 字节，否则 9 字节
        unsigned adtsHeaderLen = 7;
        uint8_t *adts = reinterpret_cast<uint8_t*>(outFrame);
        if (enc_len >= 2 && adts[0] == 0xFF && (adts[1] & 0xF0) == 0xF0)
        {
            adtsHeaderLen = (adts[1] & 0x01) ? 7 : 9;
        }

        memcpy(audioFrame, outFrame, enc_len);
        if (m_arb.PushAudioFrame(audioFrame, enc_len, o_pts, 48000, 1,
                                 adtsHeaderLen, seq) != 0)
        {
            delete[] audioFrame;
        }
    }

    delete[] outFrame;
}

hi_s32 SD3403_Manage::enable_venc(hi_void)
{
    hi_size in_size;
    const hi_vpss_chn vpss_chn = 0;
    hi_u32 i = 0;

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
    return HI_SUCCESS;
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
    return HI_SUCCESS;
}
hi_s32 SD3403_Manage::disable_vi(hi_void)
{
    sd3403_vi_cfg *vi_cfg = sd3403_vi.get_vi_cfg();
    sd3403_vi.stop_vi(&vi_cfg[0]);
    sd3403_vi.stop_vi(&vi_cfg[1]);
    return HI_SUCCESS;
}

hi_s32 SD3403_Manage::disable_sys(hi_void)
{
    sd3403_sys.sys_exit();
    return HI_SUCCESS;
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
    return HI_SUCCESS;
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
        clean_rtsp();
        stop_all_thread();
        // svp.sd3403_rfcn_svp_npu_model_deinit();
        disable_ai_aenc();
        disable_rgn();
        disable_venc();
        disable_vpss();
        disable_vi();
        disable_sys();
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
    if(signal_run && !g_sig_flag)
        destroy_signal_thread();

    // if(Sig_Thread->joinable())
    //     Sig_Thread->join();

    //release model thread
    model_run = false;
    if(Yuvto_Model != nullptr && Yuvto_Model->joinable())
        Yuvto_Model->join();

    //release venc thread
    _venc_run = false;
    if(Venc_Thread != nullptr && Venc_Thread->joinable())
        Venc_Thread->join();

    _aenc_run = false;
    if(Aenc_Thread != nullptr && Aenc_Thread->joinable())
        Aenc_Thread->join();
#ifdef RTSP_LIVE555_ON
    rtsp.Stop();
    rtsp.ThreadJoin();
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

int SD3403_Manage::Live555_RTSPInit(void)
{
    unsigned int rtspServerPortNum = RTSP_LISTEN_PORT;

    if (rtsp.SetRTSPServerPort(rtspServerPortNum) != 0) {
        printf("SetRTSPServerPort fail ...\n");
        return -1;
    }

    char *urlPrefix = rtsp.rtspURLPrefix();
    if (urlPrefix != NULL) {
        printf("=================URL===================\n");
        printf("%s%s\n", urlPrefix, "sensor0_media");
        printf("%s%s\n", urlPrefix, "sensor1_media");
        printf("=================URL===================\n");
    }

    manager_rtsp_sensor0_stream_init();
    manager_rtsp_sensor1_stream_init();

    if (rtsp.addVideoStream("sensor0_media", &m_sensor0_vrb, true, false, &m_arb, 48000, 1) != 0) {
        printf("addVideoStream sensor0_media fail ...\n");
        return -1;
    }
    if (rtsp.addVideoStream("sensor1_media", &m_sensor1_vrb, true, false, &m_arb, 48000, 1) != 0) {
        printf("addVideoStream sensor1_media fail ...\n");
        return -1;
    }

    if (rtsp.Start() != 0) {
        printf("rtsp Start fail ...\n");
        return -1;
    }

    return 0;
}


hi_s32 SD3403_Manage::manager_rtsp_sensor0_stream_init(void)
{
    m_sensor0_vrb.Set_Video_Frame(
        [this](uint8_t **pack, int &size, bool &key) -> int {
            return this->get_rtsp_sensor0_stream(pack, size, key);
        }
    );
    m_sensor0_vrb.Set_Release_Frame(
        [this](uint8_t *pack) {
            this->release_rtsp_stream(pack);
        }
    );
    m_sensor0_vrb.Set_Reset_Callback([this]() {
        if (replay_sensor0_idr != nullptr) {
            delete[] replay_sensor0_idr;
            replay_sensor0_idr = nullptr;
        }
        replay_sensor0_idr = Sensor0_IdrCache.GetCopy(&replay_sensor0_idr_len, &replay_sensor0_idr_pts);
        replay_sensor0_pending = (replay_sensor0_idr != nullptr);
        sd3403_venc.sensor0_keyFrame = true;
        sd3403_venc.request_key_frame(0);
        sd3403_venc.sensor0_keyFrame = false;
    });
    return 0;
}

void SD3403_Manage::manager_rtsp_sensor1_stream_init(void)
{
    m_sensor1_vrb.Set_Video_Frame(
        [this](uint8_t **pack, int &size, bool &key) -> int {
            return this->get_rtsp_sensor1_stream(pack, size, key);
        }
    );
    m_sensor1_vrb.Set_Release_Frame(
        [this](uint8_t *pack) {
            this->release_rtsp_stream(pack);
        }
    );
    m_sensor1_vrb.Set_Reset_Callback([this]() {
        if (replay_sensor1_idr != nullptr) {
            delete[] replay_sensor1_idr;
            replay_sensor1_idr = nullptr;
        }
        replay_sensor1_idr = Sensor1_IdrCache.GetCopy(&replay_sensor1_idr_len, &replay_sensor1_idr_pts);
        replay_sensor1_pending = (replay_sensor1_idr != nullptr);
        sd3403_venc.sensor1_keyFrame = true;
        sd3403_venc.request_key_frame(1);
        sd3403_venc.sensor1_keyFrame = false;
    });
}

int SD3403_Manage::get_rtsp_sensor0_stream(uint8_t **pack, int &size, bool &key)
{
    if (replay_sensor0_pending) {
        replay_sensor0_pending = false;
        *pack = replay_sensor0_idr;
        size = replay_sensor0_idr_len;
        key = true;
        replay_sensor0_idr = nullptr;
        return 0;
    }
    key = false;
    if (pack == nullptr) return -1;
    *pack = nullptr;
    size = 0;
    return -1;
}

int SD3403_Manage::get_rtsp_sensor1_stream(uint8_t **pack, int &size, bool &key)
{
    if (replay_sensor1_pending) {
        replay_sensor1_pending = false;
        *pack = replay_sensor1_idr;
        size = replay_sensor1_idr_len;
        key = true;
        replay_sensor1_idr = nullptr;
        return 0;
    }
    key = false;
    if (pack == nullptr) return -1;
    *pack = nullptr;
    size = 0;
    return -1;
}



void SD3403_Manage::release_rtsp_stream(uint8_t *pack)
{
    if (pack != nullptr)
    {
        // GetFrame 深拷贝与 IDR 缓存均使用 new uint8_t[] 分配
        delete[] pack;
    }
}

hi_s32 SD3403_Manage::set_audio_input_volume(hi_s32 volume_db)
{
    return sd3403_ai.Sd343_Ai_Set_Input_Volume(volume_db);
}

hi_s32 SD3403_Manage::get_audio_input_volume(hi_s32 *volume_db)
{
    return sd3403_ai.Sd343_Ai_Get_Input_Volume(volume_db);
}

hi_s32 SD3403_Manage::set_audio_vqe(hi_bool enable)
{
    return sd3403_ai.Sd343_Ai_Set_Vqe(enable);
}

hi_s32 SD3403_Manage::set_audio_vqe_agc(hi_s32 target_level, hi_s32 max_gain)
{
    return sd3403_ai.Sd343_Ai_Set_Vqe_Agc(target_level, max_gain);
}

hi_s32 SD3403_Manage::enable_ai_aenc(hi_void)
{
    hi_s32 ret = 0;

    ret = sd3403_ai.Sd343_Ai_Init();
    if(ret)
    {
        ss_print("[%s %d] ai init failed\n", __func__, __LINE__);
        return -1;
    }

    ret = sd3403_aenc.Sd343_Aenc_Aac_Init();
    if(ret)
    {
        ss_print("[%s %d] aac register failed\n", __func__, __LINE__);
        sd3403_ai.Sd343_Ai_Deinit();
        return -1;
    }

    ret = sd3403_aenc.Sd343_Aenc_Init();
    if(ret)
    {
        ss_print("[%s %d] aenc init failed\n", __func__, __LINE__);
        sd3403_aenc.Sd343_Aenc_Aac_DeInit();
        sd3403_ai.Sd343_Ai_Deinit();
        return -1;
    }

    return 0;
}

hi_s32 SD3403_Manage::disable_ai_aenc(hi_void)
{
    hi_s32 ret = 0;

    ret = sd3403_aenc.Sd343_Aenc_Deinit();
    if(ret)
        ss_print("[%s %d] aenc deinit failed\n", __func__, __LINE__);

    ret = sd3403_ai.Sd343_Ai_Deinit();
    if(ret)
        ss_print("[%s %d] ai deinit failed\n", __func__, __LINE__);

    ret = sd3403_aenc.Sd343_Aenc_Aac_DeInit();
    if(ret)
        ss_print("[%s %d] aac deinit failed\n", __func__, __LINE__);

    return 0;
}