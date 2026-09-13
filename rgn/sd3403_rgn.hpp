#ifndef _SD3403_RGN_HPP_
#define _SD3403_RGN_HPP_

#include "def.h"
#include "pch.h"
#include <unordered_map>
#include <unordered_set>

class SD3403_RGN{
public:
    SD3403_RGN(){};
    ~SD3403_RGN(){};

    hi_s32 create_rgn(hi_s32 handle);
    hi_s32 start_rgn(hi_s32 handle, hi_s32 u32X, hi_s32 u32Y, hi_s32 u32Width, hi_s32 u32Height);
    hi_s32 update_rgn(hi_s32 handle, hi_s32 u32X, hi_s32 u32Y, hi_s32 u32Width, hi_s32 u32Height);
    hi_s32 destroy_rgn(hi_s32 handle);
    hi_s32 clean_all_rgn(void);
    hi_s32 show_rgn(std::vector<RetureInfo> rgninfo);
    hi_s32 get_rgn_handle(void);
    void delete_handle(hi_s32 handle);

private:
    hi_rgn_type type = HI_RGN_COVER;
    std::unordered_map<int, CarInfo> _carInfo;
    bool handle_flag[1024] = {0};
    hi_mpp_chn mpp_chn;
};


#endif