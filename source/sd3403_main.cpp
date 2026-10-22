#include "sd3403_manage.hpp"
#include "sd3403_common.hpp"

int main(int argc, const char **argv)
{
    hi_s32 ret = 0;
    SD3403_Manage manage;

    ret = manage.enable_sys();
    if (ret != HI_SUCCESS) {
        std::cout << "enable sys error !" <<std::endl;
        goto start_venc_failed;
    }
    ret = manage.enable_vi_vpss();
    if (ret != HI_SUCCESS) {
        std::cout << "enable vi and vpss error !" <<std::endl;
        goto start_venc_failed;
    }
    ret = manage.enable_venc();
    if (ret != HI_SUCCESS) {
        std::cout << "enable venc error !" <<std::endl;
        goto start_venc_failed;
    }

    manage.Run_Fifo_Thread();
    manage.start_rtsp();
    manage.register_signal();

    manage.get_char();
    // printf("===========================================1=====================================\n");
start_venc_failed:
    manage.stop_all_thread();
    manage.disable_venc();
    manage.disable_rgn();
    manage.disable_vpss();
    manage.disable_vi();
    manage.disable_sys();
    manage.clean_rtsp();
    return 0;
}


