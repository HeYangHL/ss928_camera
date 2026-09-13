#include "sd3403_common.hpp"

SD3403_Common::SD3403_Common()
{}
SD3403_Common::~SD3403_Common()
{}

hi_void SD3403_Common::get_size_by_sns_type(hi_size *size)
{
    switch (sns_type) {
        case OV_OS08A20_MIPI_8M_30FPS_12BIT:
        case OV_OS08A20_MIPI_8M_30FPS_12BIT_WDR2TO1:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT:
        case OV_OS08B10_MIPI_8M_30FPS_12BIT_WDR2TO1:
        case SONY_IMX485_MIPI_8M_30FPS_12BIT:
        case SONY_IMX485_MIPI_8M_30FPS_10BIT_WDR3TO1:
            size->width  = WIDTH_3840;
            size->height = HEIGHT_2160;
            break;
        case OV_OS04A10_MIPI_4M_30FPS_12BIT:
            size->width  = WIDTH_2688;
            size->height = HEIGHT_1520;
            break;
        case SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT:
            size->width  = WIDTH_2592;
            size->height = HEIGHT_1520;
            break;

        case OV_OS05A10_SLAVE_MIPI_4M_30FPS_12BIT:
            size->width = WIDTH_2688;
            size->height = HEIGHT_1520;
            break;

        default:
            size->width  = WIDTH_1920;
            size->height = HEIGHT_1080;
            break;
    }
}

// hi_void SD3403_Common::sd3403_signal_handle(hi_void (*sig_handle)(hi_s32))
// {
//     struct sigaction sa;

//     (hi_void)memset_s(&sa, sizeof(struct sigaction), 0, sizeof(struct sigaction));
//     sa.sa_handler = sig_handle;
//     sa.sa_flags = 0;
//     sigaction(SIGINT, &sa, HI_NULL);
//     sigaction(SIGTERM, &sa, HI_NULL);
    
// }
// hi_void SD3403_Common::handle_sig(hi_s32 signo)
// {
//     if (signo == SIGINT || signo == SIGTERM) {
//         g_sig_flag = 1;
//     }
// }

