#ifndef _SVP_NPU_HPP_
#define _SVP_NPU_HPP_

#include <vector>
#include <algorithm>
#include <fstream>
#include <cmath> 
#include "def.h"
#include "svp_acl_rt.h"
#include "svp_acl.h"
#include "svp_acl_ext.h"
#include "svp_acl_mdl.h"

using namespace std;

typedef struct
{
    td_u32 max_batch_num;
    td_u32 dynamic_batch_num;
    td_u32 total_t;
    td_bool is_cached;
    td_u32 model_idx;
} svp_npu_task_cfg;

typedef struct
{
    svp_npu_task_cfg cfg;
    svp_acl_mdl_dataset *input_dataset;
    svp_acl_mdl_dataset *output_dataset;
    td_void *task_buf_ptr;
    size_t task_buf_size;
    size_t task_buf_stride;
    td_void *work_buf_ptr;
    size_t work_buf_size;
    size_t work_buf_stride;
} svp_npu_task_info;

typedef struct
{
    hi_void *work_buf_ptr;
    size_t work_buf_size;
    size_t work_buf_stride;
} svp_npu_shared_work_buf;

typedef struct
{
    hi_u32 model_id;
    hi_bool is_load_flag;
    hi_ulong model_mem_size;
    hi_void *model_mem_ptr;
    svp_acl_mdl_desc *model_desc;
    size_t input_num;
    size_t output_num;
    size_t dynamic_batch_idx;
} sd3403_svp_npu_model_info;
typedef struct {
    hi_float score;
    hi_u32 class_id;
} sd3403_svp_npu_top_n_result;

typedef struct {
    hi_float nms_threshold;
    hi_float score_threshold;
    hi_float min_height;
    hi_float min_width;
} sd3403_svp_npu_threshold;
typedef struct {
    hi_char *num_name;
    hi_char *roi_name;
    hi_bool has_background;
    hi_u32 roi_offset;
} sd3403_svp_npu_detection_info;

typedef struct {
    hi_point point[HI_POINT_NUM];
    float lx;
    float ly;
    float rx;
    float ry;
    float score;
    int classId;
} sd3403_svp_rect;

typedef struct {
    hi_u16 num;
    sd3403_svp_rect rect[HI_SVP_RECT_NUM];
} sd3403_svp_rect_info;

typedef struct tag_Objinfo
{
    /* data */
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    uint32_t  classNum;
    int ID;
    int trackID;
    float area;
}objinfo;

struct DetectionInnerParam {
    float *outData { nullptr };
    size_t detectIdx { 0 };
    size_t wStrideOffset { 0 };
    float scoreThr { 0.0f};
    uint32_t outWidth { 0 };
    uint32_t chnStep { 0 };
    uint32_t outHeightIdx { 0 };
    uint32_t objScoreOffset { 0 };
};

enum VaildBoxId {
    SCORE_IDX    = 0,
    XCENTER_IDX  = 1,
    YCENTER_IDX  = 2,
    W_IDX        = 3,
    H_IDX        = 4,
    CLSAA_ID_IDX = 5
};

class SVP_NNN
{
public:
    int sd3403_svp_npu_acl_init(const td_char *config_path);
    int sd3403_svp_npu_acl_deinit(void);
    int sd3403_svp_npu_load_model(const td_char *model_path, hi_u32 model_index);
    int sd3403_svp_npu_read_model(const td_char *model_path, hi_u32 model_index);
    td_s32 sd3403_svp_npu_acl_init_task(td_bool is_share_work_buf, td_u32 shared_work_buf_idx);
    td_s32 svp_npu_get_work_buf_info(const svp_npu_task_info *task, td_u32 *work_buf_size, td_u32 *work_buf_stride);
    hi_s32 svp_npu_acl_create_shared_work_buf(td_u32 shared_work_buf_idx);
    td_s32 sd3403_svp_check_task_cfg(const svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_acl_dataset_init(hi_u32 task_idx);
    hi_void sd3403_svp_npu_destroy_input(svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_create_input(svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_create_output(svp_npu_task_info *task);
    hi_void svp_npu_destroy_data_buffer(svp_acl_data_buffer *input_data);
    hi_void sd3403_svp_npu_destroy_output(svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_create_task_buf(svp_npu_task_info *task);
    svp_acl_data_buffer *sd3403_svp_npu_create_input_data_buffer(svp_npu_task_info *task, hi_u32 idx);
    hi_s32 sd3403_svp_npu_malloc_mem(hi_void **buffer, hi_u32 buffer_size, hi_bool is_cached);
    hi_s32 sd3403_svp_npu_share_work_buf(const svp_npu_shared_work_buf *shared_work_buf, const svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_check_has_aicpu_task(const svp_npu_task_info *task, hi_bool *has_aicpu_task);
    hi_void *sd3403_svp_npu_acl_aicpu_thread(hi_void);
    hi_void sd3403_svp_npu_acl_deinit_task(hi_u32 task_num, hi_u32 shared_work_buf_idx);
    hi_void sd3403_svp_npu_destroy_task_buf(svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_get_input_data(const hi_char *src, hi_u32 num, const svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_create_work_buf(svp_npu_task_info *task);
    hi_s32 sd3403_svp_npu_create_desc(hi_u32 model_index);
    hi_s32 sd3403_svp_npu_get_model_base_info(hi_u32 model_index);
    hi_s32 ad3403_svp_npu_model_execute(const svp_npu_task_info *task);
    hi_void sd3403_svp_npu_output_classification_result(const svp_npu_task_info *task, std::vector<sd3403_svp_npu_top_n_result> &output);
    hi_void sd3403_svp_npu_sort_output_result(const hi_float *src, hi_u32 src_len,sd3403_svp_npu_top_n_result *dst, hi_u32 dst_len);
    hi_void sd3403_svp_npu_destroy_work_buf(svp_npu_task_info *task);
    hi_void sd3403_svp_npu_acl_dataset_deinit(hi_u32 task_idx);
    hi_void sd3403_svp_npu_unload_model(hi_u32 model_index);
    hi_s32 sd3403_svp_npu_get_line_num_and_line_byte_num(const svp_npu_task_info *task, hi_u32 idx, \
                                                              hi_bool is_input, hi_u32 *total_line_num, hi_u32 *line_byte_num);
    hi_s32 sd3403_svp_npu_model_execute(const svp_npu_task_info *task);
    /*外部接口*/
    int Init_ACL();
    int Init_AI(const char *Det_VL_model);
    int Det(unsigned char *frame, std::vector<sd3403_svp_npu_top_n_result> &output);
    int Uninit_AI();
    int Uninit_ACL();

private:
    const td_char *acl_config_path = "";
    const td_char *om_model_path = "./model/resnet50.om";
    td_s32 g_svp_npu_dev_id = 0;
    td_bool is_cached = TD_FALSE;
    // td_void *model_mem_ptr = nullptr;
    // svp_acl_mdl_desc *model_desc = nullptr;
    // td_u32 model_id = 0;
    // uint64_t model_size = 0;
    // size_t input_num = 0;
    // size_t output_num = 0;
    // size_t dynamic_batch_idx = 0;
    td_bool is_load_flag = TD_FALSE;
    td_u32 task_num = 1;
    svp_npu_task_info g_svp_npu_task[SVP_NPU_MAX_TASK_NUM] = {0};
    svp_npu_shared_work_buf g_svp_npu_shared_work_buf[SVP_NPU_SHAERD_WORK_BUF_NUM] = {0};
    sd3403_svp_npu_model_info g_svp_npu_model[SD3403_SVP_NPU_MAX_MODEL_NUM] = {0};
    std::thread aicpu_thread;
    hi_bool g_svp_npu_aicpu_process_signal = HI_FALSE;
    sd3403_svp_rect_info g_svp_npu_rect_info = {0};

/*****
 * ****************************************************************************************
 * 目标检测模型加载代码
 * ****************************************************************************************
 */
public:
    hi_s32 sd3403_rfcn_svp_npu_acl_init(void);
    hi_s32 sd3403_rfcn_svp_npu_load_model(const hi_char *model_path, hi_u32 model_index, hi_bool is_cached);
    hi_s32 sd3403_rfcn_svp_npu_read_model(const hi_char *model_path, hi_u32 model_index, hi_bool is_cached);
    hi_s32 sd3403_rfcn_svp_npu_get_model_base_info(hi_u32 model_index);
    hi_s32 sd3403_rfcn_svp_npu_set_threshold(sd3403_svp_npu_threshold threshold[], hi_u32 threshold_num,const svp_npu_task_info *task);
    hi_void sd3403_rfcn_svp_npu_acl_vdec_to_vo(hi_void *args);
    hi_s32 sd3403_rfcn_svp_npu_get_input_data_buffer_info(const svp_npu_task_info *task, hi_u32 idx, \
                                                        hi_u8 **virt_addr, hi_u32 *size, hi_u32 *stride);
    hi_s32 sd3403_rfcn_svp_npu_update_input_data_buffer_info(const unsigned char *yuv_frame, hi_u32 idx, const svp_npu_task_info *task);
    hi_s32 sd3403_rfcn_svp_npu_model_execute(const svp_npu_task_info *task);
    hi_s32 sd3403_rfcn_svp_npu_model_init(const hi_char *om_model_path);
    hi_s32 sd3403_rfcn_svp_npu_acl_frame_handle(const unsigned char *yuv_frame);
    hi_s32 sd3403_rfcn_svp_npu_roi_to_rect(const svp_npu_task_info *task, sd3403_svp_rect_info *rect_info);
    hi_s32 sd3403_rfcn_svp_npu_acl_frame_proc(const unsigned char *yuv_frame);
    hi_s32 sd3403_rfcn_svp_npu_acl_deinit(void);
    hi_void sd3403_rfcn_svp_npu_unload_model(hi_u32 model_index);
    hi_s32 sd3403_rfcn_svp_npu_create_work_buf(svp_npu_task_info *task);
    hi_s32 sd3403_rfcn_svp_npu_share_work_buf(const svp_npu_shared_work_buf *shared_work_buf, const svp_npu_task_info *task);
    hi_s32 sd3403_rfcn_svp_npu_check_has_aicpu_task(const svp_npu_task_info *task, hi_bool *has_aicpu_task);
    hi_void sd3403_rfcn_svp_npu_acl_deinit_task(hi_u32 task_num, hi_u32 shared_work_buf_idx);
    hi_void sd3403_rfcn_svp_npu_destroy_work_buf(svp_npu_task_info *task);
    hi_void sd3403_rfcn_svp_npu_destroy_task_buf(svp_npu_task_info *task);
    hi_void sd3403_rfcn_svp_npu_acl_dataset_deinit(hi_u32 task_idx);
    hi_void sd3403_rfcn_svp_npu_destroy_input(svp_npu_task_info *task);
    hi_void sd3403_rfcn_svp_npu_destroy_output(svp_npu_task_info *task);
    hi_s32 sd3403_rfcn_svp_npu_model_inference(const unsigned char *yuv_frame, std::vector<objinfo> &output);
    hi_s32 sd3403_rfcn_svp_npu_acl_dataset_init(hi_u32 task_idx);
    hi_s32 sd3403_rfcn_svp_npu_create_input(svp_npu_task_info *task);
    svp_acl_data_buffer *sd3403_rfcn_svp_npu_create_input_data_buffer(svp_npu_task_info *task, hi_u32 idx);
    hi_s32 sd3403_rfcn_svp_npu_create_output(svp_npu_task_info *task);
    td_s32 sd3403_rfcn_svp_check_task_cfg(const svp_npu_task_info *task);
    hi_s32 sd3403_rfcn_svp_npu_malloc_mem(hi_void **buffer, hi_u32 buffer_size, hi_bool is_cached);
    hi_void *sd3403_rfcn_svp_npu_acl_aicpu_thread(hi_void);
    td_s32 sd3403_rfcn_svp_npu_acl_init_task(td_bool is_share_work_buf, td_u32 shared_work_buf_idx);
    hi_s32 sd3403_rfcn_svp_npu_model_deinit(void);
    hi_s32 sd3403_rfcn_svp_npu_get_roi_num_by_index(const svp_npu_task_info *task, sd3403_svp_rect_info *rect_info);
    static bool sd3403_rfcn_cmp(const std::vector<float>& veci, const std::vector<float>& vecj);
    void sd3403_rfcn_parse_result(const std::vector<std::vector<float>>& boxValue, sd3403_svp_rect_info *rect_info);
    // void PrintResult(const std::vector<std::vector<float>>& boxValue);
    hi_s32 sd3403_rfcn_svp_npu_get_roi_by_index(const svp_npu_task_info *task, sd3403_svp_rect_info *rect_info);
    hi_s32 sd3403_rfcn_get_output_result(std::vector<objinfo> &output);
    hi_s32 sd3403_rfcn_svp_npu_create_task_buf(svp_npu_task_info *task);
    hi_s32 read_rpn_file(const std::string& fileName, std::vector<float>& detParas);
    hi_s32 sd3403_rfcn_output_result(const svp_npu_task_info *task);
    void FilterYolov5v7Box(const svp_npu_task_info *task, std::vector<vector<float>>& vaildBox);
    void ProcessPerDectection(size_t detectIdx, vector<vector<float>>& vaildBox,vector<vector<uint32_t>>& anchorGrids, const svp_npu_task_info *task);
    void ProcessPerDectectionInner(const DetectionInnerParam& innerParam, const vector<float>& gridsX, \
                const vector<float>& gridsY, const vector<vector<uint32_t>>& anchorGrids, vector<vector<float>>& vaildBox);
    void InitData(int8_t* data, size_t dataSize);


    //test
    void *ReadBinFileWithStride(const std::string& fileName, const svp_acl_mdl_io_dims& dims,size_t stride, size_t dataSize);

private:
    sd3403_svp_npu_model_info rfcn_svp_npu_model[SD3403_SVP_NPU_MAX_MODEL_NUM] = {0};
    std::thread rfcn_svp_npu_thread;
    svp_npu_task_info rfcn_svp_npu_task[SVP_NPU_MAX_TASK_NUM] = {0};
    sd3403_svp_npu_threshold g_svp_npu_rfcn_threshold[SD3403_SVP_NPU_RFCN_THRESHOLD_NUM] = {
    {0.45, 0.25, 1.0, 1.0}, {0.3, 0.9, 1.0, 1.0} };
    sd3403_svp_rect_info g_svp_npu_rfcn_rect_info = {0};
    float scoreThr_ { 0.5 };
    svp_acl_rt_context context_ = nullptr;
    svp_acl_rt_stream stream_ = nullptr;

};

#endif