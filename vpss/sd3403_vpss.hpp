#ifndef _SD3403_VPSS_HPP_
#define _SD3403_VPSS_HPP_

#include "def.h"

class SD3403_VPSS{
public:
    SD3403_VPSS();
    ~SD3403_VPSS();
    hi_s32 start_vpss(hi_vpss_grp grp, hi_size *in_size);
    hi_void get_default_grp_attr(hi_vpss_grp_attr *grp_attr);
    hi_void get_default_chn_attr(hi_vpss_chn_attr *chn_attr);
    hi_s32 vpss_start(hi_vpss_grp grp, const hi_bool *chn_enable,const hi_vpss_grp_attr *grp_attr, const hi_vpss_chn_attr *chn_attr, hi_u32 chn_array_size);
    hi_s32 vpss_start_chn(hi_vpss_grp grp, const hi_bool *chn_enable,const hi_vpss_chn_attr *chn_attr, hi_u32 chn_array_size);
    hi_s32 vpss_stop(hi_vpss_grp grp, const hi_bool *chn_enable, hi_u32 chn_array_size);
    hi_void stop_vpss(hi_vpss_grp grp);
    hi_s32 get_model_yuv(hi_vpss_grp grp, hi_video_frame_info &vpss_frame);
    hi_s32 release_model_vpss_chn(hi_vpss_grp grp, hi_video_frame_info &vpss_frame);
    hi_vpss_grp *get_vpss_grp(hi_void)
    {
        return vpss_grp;
    }

private:
    hi_vpss_grp vpss_grp[2] = {0, 1};

};


#endif