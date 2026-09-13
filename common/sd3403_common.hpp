#ifndef _SD3403_COMMON_HPP_
#define _SD3403_COMMON_HPP_

#include "def.h"

class SD3403_Common{
public:
    SD3403_Common();
    ~SD3403_Common();
    hi_void get_size_by_sns_type(hi_size *size);
    // hi_void sd3403_signal_handle(hi_void (*sig_handle)(hi_s32));
    // hi_void handle_sig(hi_s32 signo);
    int get_sig_flag(hi_void){return g_sig_flag;};
    sd3403_sns_type get_sns_type(hi_void){return sns_type;};

private:
    int g_sig_flag = 0;
    sd3403_sns_type sns_type = OV_OS08A20_MIPI_8M_30FPS_12BIT;
    
};

#endif