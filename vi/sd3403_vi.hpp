#ifndef _SD3403_VI_HPP_
#define _SD3403_VI_HPP_

#include "def.h"
#include "sd3403_common.hpp"

extern ot_isp_sns_obj g_sns_os04a10_obj;

class SD3403_VI{
public:
    SD3403_VI();
    ~SD3403_VI();
    hi_s32 start_vi(const sd3403_vi_cfg *vi_cfg);
    hi_void get_default_cfg(hi_void);
    hi_void get_default_vi_cfg(sd3403_vi_cfg *vi_cfg);
    hi_void get_mipi_info_by_dev_id(hi_vi_dev vi_dev, sd3403_mipi_info *mipi_info);
    hi_void get_mipi_attr_by_dev_id(hi_vi_dev vi_dev,combo_dev_attr_t *combo_attr);
    hi_void get_os05a10_mipi_attr(hi_vi_dev vi_dev, combo_dev_attr_t *combo_attr);
    hi_s32 isp_get_pub_attr_by_sns(hi_isp_pub_attr *pub_attr);
    hi_void get_default_sns_info(sd3403_sns_info *sns_info);
    hi_void get_default_mipi_info(sd3403_mipi_info *mipi_info);
    hi_void get_mipi_attr(combo_dev_attr_t *combo_attr);
    hi_void get_mipi_ext_data_attr(ext_data_type_t *ext_data_attr);
    hi_s32 set_mipi_hs_mode(lane_divide_mode_t hs_mode);
    hi_void get_default_dev_info(sd3403_vi_dev_info *dev_info);
    hi_s32 get_obheight_by_sns_type(hi_void);
    hi_void get_default_bind_info(hi_vi_bind_pipe *bind_pipe);
    hi_void get_default_grp_info(sd3403_vi_grp_info *grp_info);
    hi_wdr_mode get_wdr_mode_by_sns_type(hi_void);
    hi_s32 get_pipe_num_by_sns_type(hi_void);
    hi_void get_default_pipe_info(hi_vi_bind_pipe *bind_pipe,sd3403_vi_pipe_info pipe_info[]);
    hi_s32 mipi_ctrl_cmd(hi_u32 devno, hi_u32 cmd);
    hi_s32 set_mipi_combo_attr(const combo_dev_attr_t *combo_dev_attr);
    hi_s32 set_mipi_ext_data_type_attr(const ext_data_type_t *ext_data_type_attr);
    hi_s32 start_mipi_rx(const sd3403_sns_info *sns_info, const sd3403_mipi_info *mipi_info);
    hi_s32 start_dev(hi_vi_dev vi_dev, const hi_vi_dev_attr *dev_attr, const hi_vi_bas_attr *bas_attr);
    hi_s32 dev_bind_pipe(hi_vi_dev vi_dev, const hi_vi_bind_pipe *bind_pipe);
    hi_s32 set_grp_info(const sd3403_vi_grp_info *grp_info);
    hi_s32 start_pipe(const hi_vi_bind_pipe *bind_pipe, const sd3403_vi_pipe_info pipe_info[]);
    hi_s32 start_one_pipe(hi_vi_pipe vi_pipe, const sd3403_vi_pipe_info *pipe_info);
    hi_void stop_one_pipe(hi_vi_pipe vi_pipe, const sd3403_vi_pipe_info *pipe_info);
    hi_s32 start_isp(const sd3403_vi_cfg *vi_cfg);
    hi_s32 start_one_pipe_isp(hi_vi_pipe vi_pipe, hi_u8 pipe_index, const sd3403_vi_cfg *vi_cfg);
    hi_s32 register_sensor_lib(hi_vi_pipe vi_pipe, hi_u8 pipe_index, const sd3403_vi_cfg *vi_cfg);
    hi_s32 isp_sensor_regiter_callback(hi_isp_dev isp_dev);
    hi_s32 start_chn(hi_vi_pipe vi_pipe, const sd3403_vi_chn_info chn_info[], hi_u32 chn_num);
    hi_s32 stop_chn(hi_vi_pipe vi_pipe, const sd3403_vi_chn_info chn_info[], hi_u32 chn_num);
    hi_isp_sns_obj *isp_get_sns_obj(sd3403_sns_type sns_type);
    hi_s32 isp_bind_sns(hi_isp_dev isp_dev, hi_s8 sns_dev);
    hi_isp_sns_type get_sns_bus_type(hi_void);
    hi_s32 isp_ae_lib_callback(hi_isp_dev isp_dev);
    hi_s32 isp_awb_lib_callback(hi_isp_dev isp_dev);
    hi_s32 isp_ae_lib_uncallback(hi_isp_dev isp_dev);
    hi_s32 isp_sensor_unregiter_callback(hi_isp_dev isp_dev);
    hi_s32 isp_sensor_founction_cfg(hi_vi_pipe vi_pipe, sd3403_sns_type sns_type);
    hi_s32 isp_run(hi_isp_dev isp_dev);
    // void *isp_thread(hi_void *param);
    hi_void isp_thread(hi_isp_dev isp_dev);
    hi_s32 isp_awb_lib_uncallback(hi_isp_dev isp_dev);
    hi_void deregister_sensor_lib(hi_vi_pipe vi_pipe);
    hi_void stop_one_pipe_isp(hi_vi_pipe vi_pipe);
    hi_void isp_stop(hi_isp_dev isp_dev);
    hi_void stop_pipe(const hi_vi_bind_pipe *bind_pipe, const sd3403_vi_pipe_info pipe_info[]);
    hi_void dev_unbind_pipe(hi_vi_dev vi_dev, const hi_vi_bind_pipe *bind_pipe);
    hi_void stop_dev(hi_vi_dev vi_dev);
    hi_void stop_mipi_rx(const sd3403_sns_info *sns_info, const sd3403_mipi_info *mipi_info);
    hi_void stop_isp(const sd3403_vi_cfg *vi_cfg);
    hi_void stop_vi(const sd3403_vi_cfg *vi_cfg);
    sd3403_vi_cfg *get_vi_cfg(hi_void)
    {
        return vi_cfg;
    }
    

private:
    sd3403_vi_cfg vi_cfg[2];
    hi_isp_sns_type g_sns_type[HI_VI_MAX_PIPE_NUM] = {OT_ISP_SNS_TYPE_BUTT};
    hi_bool g_start_isp[HI_VI_MAX_PIPE_NUM] = {HI_FALSE};
    std::thread m_thread[HI_VI_MAX_DEV_NUM];

    SD3403_Common sdCom;

    hi_isp_pub_attr g_isp_pub_attr_imx485_mipi_8m_30fps_wdr3to1 = {{0, 0, 3840, 2160},{3840, 2160},30,HI_ISP_BAYER_RGGB,HI_WDR_MODE_3To1_LINE, \
                                                                0,HI_FALSE,HI_FALSE,{HI_FALSE,{0, 0, 3840, 2160},},};
   
    ext_data_type_t g_mipi_ext_data_type_os08a20_12bit_8m_nowdr_attr = {0, MIPI_NUM, {12, 12, 12}, {0x37, 0x2c, 0x2c}};
    ext_data_type_t g_mipi_ext_data_type_default_attr = {0, MIPI_NUM, {12, 12, 12}, {0x2c, 0x2c, 0x2c}};
    hi_vi_dev_attr g_mipi_raw_dev_attr = {HI_VI_INTF_MODE_MIPI, HI_VI_WORK_MODE_MULTIPLEX_1, {0xfff00000, 0x00000000}, HI_VI_SCAN_PROGRESSIVE, \
                        {-1, -1, -1, -1}, HI_VI_DATA_SEQ_YVYU, {HI_VI_VSYNC_FIELD, HI_VI_VSYNC_NEG_HIGH, HI_VI_HSYNC_VALID_SIG, HI_VI_HSYNC_NEG_HIGH, HI_VI_VSYNC_VALID_SIG, HI_VI_VSYNC_VALID_NEG_HIGH,  \
                        {0,0,0,0,0,0,0,0,0}}, HI_VI_DATA_TYPE_RAW, HI_FALSE, {WIDTH_3840, HEIGHT_2160}, HI_DATA_RATE_X1};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_nowdr_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_3840, HEIGHT_2160}, \
                       {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{0, 1, 2, 3, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_nowdr_dev2_attr = {2, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{4, 5, 6, 7, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os08a20_12bit_8m_wdr2to1_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_VC,{0, 1, 2, 3, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os08b10_12bit_8m_nowdr_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{0, 1, 2, 3, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os08b10_12bit_8m_wdr2to1_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_VC,{0, 1, 2, 3, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os08b10_12bit_8m_nowdr_dev2_attr = {2, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{4, 5, 6, 7, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_2lane_chn0_sensor_os05a10_12bit_4m_nowdr_single_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2688, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{0, 1, -1, -1, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_2lane_chn0_sensor_os05a10_12bit_4m_nowdr_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2688, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{0, 2, -1, -1, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_2lane_chn1_sensor_os05a10_12bit_4m_nowdr_attr = {1, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2688, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{1, 3, -1, -1, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_2lane_chn2_sensor_os05a10_12bit_4m_nowdr_attr = {2, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2688, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{4, 6, -1, -1, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_2lane_chn3_sensor_os05a10_12bit_4m_nowdr_attr = {3, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2688, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{5, 7, -1, -1, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2688, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{0, 1, 2, 3, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_dev2_attr = {2, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2688, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{4, 5, 6, 7, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_imx347_slave_12bit_4m_nowdr_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 0, WIDTH_2592, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{0, 1, 2, 3, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_imx347_slave_12bit_4m_nowdr_dev2_attr = {2, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 2, WIDTH_2592, HEIGHT_1520}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{4, 5, 6, 7, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn0_sensor_imx485_12bit_8m_nowdr_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 2, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{0, 1, 2, 3, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_4lane_chn2_sensor_imx485_12bit_8m_nowdr_attr = {2, INPUT_MODE_MIPI, MIPI_DATA_RATE_X1, {0, 2, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_12BIT,HI_MIPI_WDR_MODE_NONE,{4, 5, 6, 7, -1, -1, -1, -1}}};
    combo_dev_attr_t g_mipi_8lane_chn0_sensor_imx485_10bit_8m_wdr3to1_attr = {0, INPUT_MODE_MIPI, MIPI_DATA_RATE_X2, {0, 2, WIDTH_3840, HEIGHT_2160}, \
                        {DATA_TYPE_RAW_10BIT,HI_MIPI_WDR_MODE_NONE,{0, 1, 2, 3, 4, 5, 6, 7}}};

    
    
};


#endif
