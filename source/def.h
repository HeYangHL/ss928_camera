#ifndef _DEF_H_
#define _DEF_H_

#include <stdio.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "hi_common.h"
#include "securec.h"
#include "hi_common_video.h"
#include "hi_common_isp.h"
#include "ot_common_isp.h"
#include "hi_mpi_isp.h"
#include "ot_common_vpss.h"
#include "hi_mpi_vpss.h"
#include "ot_common_vi.h"
#include "hi_mpi_vi.h"
#include "ot_common_venc.h"
#include "ot_common_rc.h"
#include "hi_mpi_venc.h"
#include "ot_common_sys.h"
#include "hi_mpi_sys.h"
#include "ot_common_vb.h"
#include "hi_mpi_vb.h"
#include "hi_buffer.h"
#include "ot_mipi_rx.h"
#include "hi_mipi_rx.h"
#include "hi_sns_ctrl.h"
#include "hi_common_awb.h"
#include "hi_mpi_awb.h"
#include "ot_common_awb.h"
#include "hi_common_ae.h"
#include "hi_mpi_ae.h"
#include "ot_common_ae.h"
#include "hi_sns_ctrl.h"
#include "hi_common_region.h"
#include "hi_mpi_region.h"
#include <iostream>
#include <thread>
#include "Fifo_Buffer.hpp"


// #ifdef __cplusplus
// #if __cplusplus
// extern "C"{
// #endif
// #endif /* __cplusplus */

#define ss_print(fmt...)                             \
    do                                               \
    {                                                \
        printf("[%s]-%d: ", __FUNCTION__, __LINE__); \
        printf(fmt);                                 \
    } while (0)

#define ss_pause()                                                          \
    do                                                                      \
    {                                                                       \
        printf("---------------press enter key to exit!---------------\n"); \
        getchar();                                                          \
    } while (0)

#define check_return(express, name)                                             \
    do                                                                          \
    {                                                                           \
        hi_s32 ret_ = (express);                                                \
        if (ret_ != HI_SUCCESS)                                                 \
        {                                                                       \
            printf("\033[0;31m%s failed at %s: LINE: %d with %#x!\033[0;39m\n", \
                   (name), __FUNCTION__, __LINE__, ret_);                       \
            return ret_;                                                        \
        }                                                                       \
    } while (0)

#define SD3403_PATH_MAX 256
#define FILE_NAME_LEN 128
#define FILE_PATH_LEN 128
#define RGN_RGB888_BLUE 0x0000ff
#define RGN_RGB888_RED 0xff0000

typedef struct
{
    hi_bool thread_start;
    hi_venc_chn venc_chn[HI_VENC_MAX_CHN_NUM];
    hi_s32 cnt;
} sd3403_venc_getstream_para;

typedef enum
{
    PIC_CIF,
    PIC_360P,    /* 640 * 360 */
    PIC_D1_PAL,  /* 720 * 576 */
    PIC_D1_NTSC, /* 720 * 480 */
    PIC_960H,    /* 960 * 576 */
    PIC_720P,    /* 1280 * 720 */
    PIC_1080P,   /* 1920 * 1080 */
    PIC_480P,
    PIC_576P,
    PIC_800X600,
    PIC_1024X768,
    PIC_1280X1024,
    PIC_1366X768,
    PIC_1440X900,
    PIC_1280X800,
    PIC_1600X1200,
    PIC_1680X1050,
    PIC_1920X1200,
    PIC_640X480,
    PIC_1920X2160,
    PIC_2560X1440,
    PIC_2560X1600,
    PIC_2592X1520,
    PIC_2592X1944,
    PIC_3840X2160,
    PIC_4096X2160,
    PIC_3000X3000,
    PIC_4000X3000,
    PIC_6080X2800,
    PIC_7680X4320,
    PIC_3840X8640,
    PIC_BUTT
} hi_pic_size;

typedef enum
{
    SD3403_RC_CBR = 0,
    SD3403_RC_VBR,
    SD3403_RC_AVBR,
    SD3403_RC_CVBR,
    SD3403_RC_QVBR,
    SD3403_RC_QPMAP,
    SD3403_RC_FIXQP
} sd3403_rc;

enum InputOutputId
{
    INPUT_IMG_ID = 0,
    OUTPUT_NUM_ID = 0,
    OUTPUT_BBOX_ID = 1
};
enum BoxValue
{
    TOP_LEFT_X = 0,
    TOP_LEFT_Y = 1,
    BOTTOM_RIGHT_X = 2,
    BOTTOM_RIGHT_Y = 3,
    SCORE = 4,
    CLASS_ID = 5,
    BBOX_SIZE = 6
};

enum DetParaEnum
{
    NMS_THR = 0,
    SCORE_THR = 1,
    MIN_HEIGHT = 2,
    MIN_WIDTH = 3,
};

typedef enum
{
    OV_OS08A20_MIPI_8M_30FPS_12BIT,
    OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1,
    OV_OS04A10_MIPI_4M_30FPS_12BIT,
    OV_OS08B10_MIPI_8M_30FPS_12BIT,
    OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1,
    OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT,
    SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT,
    SONY_IMX485_MIPI_8M_30FPS_12BIT,
    SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1,
    SNS_TYPE_BUTT,
} sd3403_sns_type;

typedef struct
{
    hi_u32 frame_rate;
    hi_u32 stats_time;
    hi_u32 gop;
    hi_size venc_size;
    hi_pic_size size;
    hi_u32 profile;
    hi_bool is_rcn_ref_share_buf;
    hi_venc_gop_attr gop_attr;
    hi_payload_type type;
    sd3403_rc rc_mode;
} sd3403_venc_chn_param;

typedef struct
{
    sd3403_sns_type sns_type;
    hi_u32 sns_clk_src;
    hi_u32 sns_rst_src;
    hi_u32 bus_id;
} sd3403_sns_info;

typedef struct
{
    hi_s32 mipi_dev;
    lane_divide_mode_t divide_mode;
    combo_dev_attr_t combo_dev_attr;
    ext_data_type_t ext_data_type_attr;
} sd3403_mipi_info;

typedef struct
{
    hi_vi_dev vi_dev;
    hi_vi_dev_attr dev_attr;
    hi_vi_bas_attr bas_attr;
} sd3403_vi_dev_info;

typedef struct
{
    hi_u32 grp_num;
    hi_vi_grp fusion_grp[HI_VI_MAX_WDR_FUSION_GRP_NUM];
    hi_vi_wdr_fusion_grp_attr fusion_grp_attr[HI_VI_MAX_WDR_FUSION_GRP_NUM];
} sd3403_vi_grp_info;

typedef struct
{
    hi_vi_chn vi_chn;
    hi_vi_chn_attr chn_attr;
} sd3403_vi_chn_info;
typedef struct
{
    hi_isp_pub_attr isp_pub_attr;
} sd3403_isp_info;

typedef struct
{
    hi_vi_pipe_attr pipe_attr;

    hi_bool pipe_need_start;
    hi_bool isp_need_run;
    sd3403_isp_info isp_info;

    hi_u32 chn_num;
    sd3403_vi_chn_info chn_info[HI_VI_MAX_PHYS_CHN_NUM];
} sd3403_vi_pipe_info;

typedef struct
{
    sd3403_sns_info sns_info;
    sd3403_mipi_info mipi_info;
    sd3403_vi_dev_info dev_info;
    hi_vi_bind_pipe bind_pipe;
    sd3403_vi_grp_info grp_info;
    sd3403_vi_pipe_info pipe_info[HI_VI_MAX_PHYS_PIPE_NUM];
} sd3403_vi_cfg;

typedef struct
{
    FILE *file[HI_VENC_MAX_CHN_NUM];
    hi_s32 venc_fd[HI_VENC_MAX_CHN_NUM];
    hi_s32 maxfd;
    hi_u32 picture_cnt[HI_VENC_MAX_CHN_NUM];
    hi_char file_name[HI_VENC_MAX_CHN_NUM][FILE_NAME_LEN];
    hi_char real_file_name[HI_VENC_MAX_CHN_NUM][SD3403_PATH_MAX];
    hi_venc_chn venc_chn;
    hi_char file_postfix[10]; /* 10 :file_postfix number */
    hi_s32 chn_total;
} sd3403_venc_stream_proc_info;

//svp struct
typedef struct center_point{
    float x;
    float y;
    float w;
    float h;
}CenterPoint;

typedef struct car_info{
    int id;
    int type;
    int handle;
}CarInfo;

typedef struct coor_point
{
    float x;
    float y;
}COOR_POINT;

#define SD3403_FRAME_BUF_RATIO_MAX 100
#define SD3403_FRAME_BUF_RATIO_MIN 70
#define SAMPLE_RETURN_CONTINUE 1
#define SAMPLE_RETURN_BREAK 2
#define SAMPLE_RETURN_NULL 3
#define SAMPLE_RETURN_GOTO 4
#define SAMPLE_RETURN_FAILURE (-1)
#define MIPI_DEV_NAME "/dev/ot_mipi_rx"
#define OB_HEIGHT_END 24
#define OB_HEIGHT_START 0
#define IMX347_OB_HEIGHT_END 20
#define IMX485_OB_HEIGHT_END 20
#define WIDTH_2688 2688
#define WIDTH_2592 2592
#define HEIGHT_1520 1520
#define WIDTH_1920 1920
#define HEIGHT_1080 1080
#define WIDTH_3840 3840
#define HEIGHT_2160 2160
#define MIPI_NUM 3
#define VPSS_DEFAULT_WIDTH 3840
#define VPSS_DEFAULT_HEIGHT 2160
#define VB_RAW_CNT_NONE 0
#define VB_LINEAR_RAW_CNT 5
#define VB_WDR_RAW_CNT 8
#define VB_MULTI_RAW_CNT 15
#define VB_YUV_ROUTE_CNT 10
#define VB_DOUBLE_YUV_CNT 15
#define VB_MULTI_YUV_CNT 30
#define HI_SVP_RECT_NUM 64
#define HI_POINT_NUM 4

#define svp_printf_red(level_str, msg, ...)                                                                              \
    do                                                                                                                   \
    {                                                                                                                    \
        fprintf(stderr, "\033[0;31m [level]:%s,[func]:%s [line]:%d [info]:" msg "\033[0;39m\n", level_str, __FUNCTION__, \
                __LINE__, ##__VA_ARGS__);                                                                                \
    } while (0)

#define svp_trace_err(msg, ...) svp_printf_red("Error", msg, ##__VA_ARGS__)

#define svp_check_exps_return(exps, ret, level, msg, ...) \
    do                                                    \
    {                                                     \
        if ((exps))                                       \
        {                                                 \
            svp_trace_err(msg, ##__VA_ARGS__);            \
            return (ret);                                 \
        }                                                 \
    } while (0)

#define svp_printf(level_str, msg, ...)                                                                                  \
    do                                                                                                                   \
    {                                                                                                                    \
        fprintf(stderr, "[level]:%s,[func]:%s,[line]:%d,[info]:" msg, level_str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#define svp_trace_info(msg, ...) svp_printf("Info", msg, ##__VA_ARGS__)

/* exps is true, goto */
#define svp_check_exps_goto(exps, label, level, msg, ...) \
    do                                                    \
    {                                                     \
        if ((exps))                                       \
        {                                                 \
            svp_trace_err(msg, ##__VA_ARGS__);            \
            goto label;                                   \
        }                                                 \
    } while (0)

#define svp_check_exps_return_void(exps, level, msg, ...) \
    do                                                    \
    {                                                     \
        if ((exps))                                       \
        {                                                 \
            svp_trace_err(msg, ##__VA_ARGS__);            \
            return;                                       \
        }                                                 \
    } while (0)

#define SVP_NPU_MAX_TASK_NUM 16
#define SVP_NPU_SHAERD_WORK_BUF_NUM 1
#define SVP_NPU_EXTRA_INPUT_NUM 2
#define SD3403_SVP_NPU_MAX_THREAD_NUM 16
#define SD3403_SVP_NPU_MAX_TASK_NUM 16
#define SD3403_SVP_NPU_MAX_MODEL_NUM 1
#define SD3403_SVP_NPU_EXTRA_INPUT_NUM 2
#define SD3403_SVP_NPU_BYTE_BIT_NUM 8
#define SD3403_SVP_NPU_SHOW_TOP_NUM 5
#define SD3403_SVP_NPU_MAX_NAME_LEN 32
#define SD3403_SVP_NPU_MAX_MEM_SIZE 0xFFFFFFFF
#define SD3403_SVP_NPU_RECT_LEFT_TOP 0
#define SD3403_SVP_NPU_RECT_RIGHT_TOP 1
#define SD3403_SVP_NPU_RECT_RIGHT_BOTTOM 2
#define SD3403_SVP_NPU_RECT_LEFT_BOTTOM 3
#define SD3403_SVP_NPU_THRESHOLD_NUM 4
#define SD3403_SVP_NPU_RESNET50_INPUT_FILE_NUM 1
#define SD3403_SVP_NPU_RFCN_THRESHOLD_NUM 2
#define SD3403_SVP_NPU_AICPU_WAIT_TIME 1000

typedef enum
{
    SVP_ERR_LEVEL_DEBUG = 0x0,   /* debug-level                                  */
    SVP_ERR_LEVEL_INFO = 0x1,    /* informational                                */
    SVP_ERR_LEVEL_NOTICE = 0x2,  /* normal but significant condition             */
    SVP_ERR_LEVEL_WARNING = 0x3, /* warning conditions                           */
    SVP_ERR_LEVEL_ERROR = 0x4,   /* error conditions                             */
    SVP_ERR_LEVEL_CRIT = 0x5,    /* critical conditions                          */
    SVP_ERR_LEVEL_ALERT = 0x6,   /* action must be taken immediately             */
    SVP_ERR_LEVEL_FATAL = 0x7,   /* just for compatibility with previous version */

    SVP_ERR_LEVEL_BUTT
} sd3403_svp_err_level;

constexpr uint8_t SCALE_SIZE = 3;
constexpr uint8_t CLASS_NUM = 80;
constexpr uint8_t OUT_PARM_NUM = 85;

// typedef struct Resnet{

// }objinfo;

#define SVP_NPU_MAX_MEM_SIZE 0xFFFFFFFF
#define SVP_ACL_ERROR_RT_REPORT_TIMEOUT 507012
#define YUV420_SIZE 614400
#define YUV_STRIDE 640
#define YOLO_VERSION 5

#define PROC_OLD


// #ifdef __cplusplus
// #if __cplusplus
// }
// #endif
// #endif /* __cplusplus */
#endif