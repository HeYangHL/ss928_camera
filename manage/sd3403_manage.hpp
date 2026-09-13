#ifndef _SD3403_MANAGE_HPP_
#define _SD3403_MANAGE_HPP_

#include "def.h"
#include "sd3403_common.hpp"
#include "sd3403_vi.hpp"
#include "sd3403_venc.hpp"
#include "sd3403_vpss.hpp"
#include "sd3403_sys.hpp"
#include "sd3403_rgn.hpp"
#include "Fifo_Buffer.hpp"
#include "rtspserver.hpp"
#include "svp_npu.hpp"
#include <semaphore.h>
#ifdef RTSP_LIVE555_ON
#include "GetFrame.hpp"
#include "Rtsp_Server.hpp"
#endif

class SD3403_Manage{
public:
    SD3403_Manage();
    ~SD3403_Manage();
    hi_s32 manager_rtsp_stream_init(void);
    void release_rtsp_stream(uint8_t *pack);
    hi_s32 get_rtsp_stream(uint8_t **pack, int &size);
    hi_s32 start_multi_vi_vpss(hi_s32 dev_num, hi_s32 grp_num);
    hi_s32 enable_sys(hi_void);
    hi_s32 enable_vi_vpss(hi_void);
    hi_s32 enable_venc(hi_void);
    hi_s32 disable_venc(hi_void);
    hi_s32 disable_vpss(hi_void);
    hi_s32 disable_vi(hi_void);
    hi_s32 disable_sys(hi_void);
    hi_s32 disable_rgn(hi_void);
    hi_s32 get_char(hi_void);
    hi_void register_signal(hi_void);
    hi_void signal_handle_thread(void);
    hi_void sensor_venc_stream(int chn);
    hi_void start_rtsp(hi_void);
    int Run_Fifo_Thread(void);
#ifndef RTSP_LIVE555_ON
    List &get_sensor0_fifo(void){return Sensor0_Fifo;}
    List &get_sensor1_fifo(void){return Sensor1_Fifo;}
#endif
    static int sensor0_rv(void *opaque, uint8_t *data, bool keyFrame, uint64_t *pts);
    static int sensor1_rv(void *opaque, uint8_t *data, bool keyFrame, uint64_t *pts);
    int request_key_frame(int venc_chn);
    hi_void sd3403_signal_handle(hi_void (*sig_handle)(hi_s32));
    static hi_void handle_sig(hi_s32 signo);
    void get_points_coll(hi_video_frame_info *frame_info, std::vector<CenterPoint> &center);
    void send_yuv_to_model_pthread(void);
    std::vector<RetureInfo> get_track_id(void *handle, std::vector<CenterPoint> &center);
    hi_void stop_all_thread(void);
    hi_void clean_rtsp(void);
    hi_void destroy_signal_thread(void);
    hi_s32 enable_rtsp(hi_void);
    void create_live555_rtsp(void);


private:
    SD3403_SYS sd3403_sys;
    SD3403_VI sd3403_vi;
    SD3403_VPSS sd3403_vpss;
    SD3403_VENC sd3403_venc;
    SD3403_RGN sd3403_rgn;
    SD3403_Common sdCom;
    hi_venc_chn venc_chn[4] = {0, 1, 2, 3};
    hi_u32 grp_num = 2;
    List Sensor0_Fifo;
    List Sensor1_Fifo;

    bool _enc_run = false;
    bool model_run = false;
    bool signal_run = false;
    std::thread *Send_Thread = nullptr;
    std::thread *Venc_Thread = nullptr;
    std::thread *Yuvto_Model = nullptr;
    std::thread *Sig_Thread = nullptr;
    std::thread *live555_rtsp = nullptr;
    static int g_sig_flag;
    SVP_NNN svp;
    static sem_t mutex;
#ifdef RTSP_LIVE555_ON
    RtspServer rtsp;
#else
    RTSP_S rtsp_s;
#endif
};


#endif