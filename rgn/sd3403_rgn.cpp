#include "sd3403_rgn.hpp"

hi_s32 SD3403_RGN::create_rgn(hi_s32 handle)
{
    hi_rgn_attr rgn_attr;
    hi_s32 ret = 0;

    memset(&rgn_attr, 0, sizeof(hi_rgn_attr));
    rgn_attr.type = type;

    ret = hi_mpi_rgn_create(handle, &rgn_attr);
    if (ret != HI_SUCCESS)
    {
        ss_print("create rgn error, error code : %x\n!\n", ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
hi_s32 SD3403_RGN::start_rgn(hi_s32 handle, hi_s32 u32X, hi_s32 u32Y, hi_s32 u32Width, hi_s32 u32Height)
{
     hi_s32 ret = 0;

    COOR_POINT first_point;
    COOR_POINT second_point;
    COOR_POINT third_point;
    COOR_POINT fourth_point;

    first_point.x = u32X;
    first_point.y = u32Y;

    second_point.x = u32X + u32Width;
    second_point.y = u32Y;

    third_point.x = u32X + u32Width;
    third_point.y = u32Y + u32Height;

    fourth_point.x = u32X;
    fourth_point.y = u32Y + u32Height;

    ot_rgn_chn_attr chn_attr;
    memset(&chn_attr, 0, sizeof(ot_rgn_chn_attr));
    chn_attr.is_show = HI_TRUE;
    chn_attr.type = HI_RGN_COVER;
    chn_attr.attr.cover_chn.layer = 1;
    chn_attr.attr.cover_chn.coord = (ot_coord)0;
    chn_attr.attr.cover_chn.cover.color = RGN_RGB888_RED;
    chn_attr.attr.cover_chn.cover.type = HI_COVER_QUAD;

    chn_attr.attr.cover_chn.cover.quad.is_solid = (td_bool)0;
    chn_attr.attr.cover_chn.cover.quad.thick = 4;
#if 1
    chn_attr.attr.cover_chn.cover.quad.point[0].x = (td_s32)first_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[0].y = (td_s32)first_point.y;
    chn_attr.attr.cover_chn.cover.quad.point[1].x = (td_s32)second_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[1].y = (td_s32)second_point.y;
    chn_attr.attr.cover_chn.cover.quad.point[2].x = (td_s32)third_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[2].y = (td_s32)third_point.y;
    chn_attr.attr.cover_chn.cover.quad.point[3].x = (td_s32)fourth_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[3].y = (td_s32)fourth_point.y;
    if (chn_attr.attr.cover_chn.cover.quad.point[0].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[0].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[0].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[0].y -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[1].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[1].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[1].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[1].y -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[2].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[2].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[2].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[2].y -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[3].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[3].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[3].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[3].y -= 1;
        // printf("first_point.x : %d, first_point.y : %d\n", chn_attr.attr.cover_chn.cover.quad.point[0].x, chn_attr.attr.cover_chn.cover.quad.point[0].y);
        // printf("second_point.x : %d, second_point.y : %d\n", chn_attr.attr.cover_chn.cover.quad.point[1].x, chn_attr.attr.cover_chn.cover.quad.point[1].y);
        // printf("third_point.x : %d, third_point.y : %d\n", chn_attr.attr.cover_chn.cover.quad.point[2].x, chn_attr.attr.cover_chn.cover.quad.point[2].y);
        // printf("fourth_point.x : %d, fourth_point.y : %d\n", chn_attr.attr.cover_chn.cover.quad.point[3].x, chn_attr.attr.cover_chn.cover.quad.point[3].y);
#else
    // chn_attr.attr.cover_chn.cover.rect.x = 0;
    // chn_attr.attr.cover_chn.cover.rect.y = 0;
    // chn_attr.attr.cover_chn.cover.rect.height = 500;
    // chn_attr.attr.cover_chn.cover.rect.width = 500;
#endif

    mpp_chn.mod_id = HI_ID_VPSS;
    mpp_chn.dev_id = 0;
    mpp_chn.chn_id = 0;

    ret = create_rgn(handle);
    if (ret != HI_SUCCESS)
    {
        ss_print("create rgn error!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_rgn_attach_to_chn(handle, &mpp_chn, &chn_attr);
    if (ret != HI_SUCCESS)
    {
        ss_print("attach rgn to chn error! error code : %x, delete handle : %d\n", ret, handle);
        delete_handle(handle);
        destroy_rgn(handle);
        return HI_FAILURE;
    }
    handle_flag[handle] = true;

    return HI_SUCCESS;
}

hi_s32 SD3403_RGN::update_rgn(hi_s32 handle, hi_s32 u32X, hi_s32 u32Y, hi_s32 u32Width, hi_s32 u32Height)
{
    hi_s32 ret = 0;
    ot_rgn_chn_attr chn_attr;
    COOR_POINT first_point;
    COOR_POINT second_point;
    COOR_POINT third_point;
    COOR_POINT fourth_point;

    first_point.x = u32X;
    first_point.y = u32Y;
    second_point.x = u32X + u32Width;
    second_point.y = u32Y;
    third_point.x = u32X + u32Width;
    third_point.y = u32Y + u32Height;
    fourth_point.x = u32X;
    fourth_point.y = u32Y + u32Height;
    // chenge yh
    ret = hi_mpi_rgn_get_display_attr(handle, &mpp_chn, &chn_attr);
    if (ret != HI_SUCCESS)
    {
        ss_print("get rgn attr error! error code : %x\n", ret);
        return HI_FAILURE;
    }

    chn_attr.attr.cover_chn.cover.quad.point[0].x = first_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[0].y = first_point.y;
    chn_attr.attr.cover_chn.cover.quad.point[1].x = second_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[1].y = second_point.y;
    chn_attr.attr.cover_chn.cover.quad.point[2].x = third_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[2].y = third_point.y;
    chn_attr.attr.cover_chn.cover.quad.point[3].x = fourth_point.x;
    chn_attr.attr.cover_chn.cover.quad.point[3].y = fourth_point.y;
    if (chn_attr.attr.cover_chn.cover.quad.point[0].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[0].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[0].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[0].y -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[1].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[1].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[1].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[1].y -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[2].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[2].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[2].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[2].y -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[3].x % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[3].x -= 1;
    if (chn_attr.attr.cover_chn.cover.quad.point[3].y % 2 != 0)
        chn_attr.attr.cover_chn.cover.quad.point[3].y -= 1;
    ret = hi_mpi_rgn_set_display_attr(handle, &mpp_chn, &chn_attr);
    if (ret != HI_SUCCESS)
    {
        delete_handle(handle);
        ss_print("set rgn attr error! error code : %x, handle : %d\n", ret, handle);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_RGN::destroy_rgn(hi_s32 handle)
{
    hi_s32 ret = 0;

    ret = hi_mpi_rgn_detach_from_chn(handle, &mpp_chn);
    if (ret != HI_SUCCESS)
    {
        ss_print("detach rgn from chn error! code : %x\n", ret);
        return HI_FAILURE;
    }

    ret = hi_mpi_rgn_destroy(handle);
    if(ret != HI_SUCCESS)
    {
        ss_print("destroy rgn error, handle : %d, error code : %d\n", handle, ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_RGN::clean_all_rgn(void)
{
    hi_s32 ret = 0;

    for(auto it = _carInfo.begin(); it != _carInfo.end(); )
    {
        ret = hi_mpi_rgn_destroy(it->second.handle);
        if(ret != HI_SUCCESS)
        {
            ss_print("claen all rgn handle : %d error!\n", it->second.handle);
            return HI_FAILURE;
        }
        it = _carInfo.erase(it);
    }
    return HI_SUCCESS;
}


hi_s32 SD3403_RGN::show_rgn(std::vector<RetureInfo> rgninfo)
{
    hi_s32 ret = 0;
    //收集当前帧的所有 ID

    ss_print("===========================show rgn=========================info : %d\n", rgninfo.size());
    std::unordered_set<int> currentIDs;
    for (const auto& info : rgninfo) {
        currentIDs.insert(info.detectID);
    }
    for (const auto& info : rgninfo) {
        auto it = _carInfo.find(info.detectID);
        if (it != _carInfo.end()) {
            // 更新已有 RGN（例如移动位置）
            ret = update_rgn(it->second.handle, info.x, info.y, info.width, info.height);
        } else {
            // 创建新 RGN
            hi_s32 handle = get_rgn_handle();
            ss_print("===>create new rgn, handle : %d, delete id : %d\n", handle, info.detectID);
            start_rgn(handle, info.x, info.y, info.width, info.height);
            _carInfo[info.detectID] = CarInfo{info.detectID, info.type, handle};
        }
    }
    //删除消失的目标
    for (auto it = _carInfo.begin(); it != _carInfo.end(); ) {
        ss_print("====>find delete id : %d, num : %d\n", it->first, currentIDs.size());
        if (currentIDs.find(it->first) == currentIDs.end()) {
            int free_handle = it->second.handle;
            destroy_rgn(it->second.handle);
            it = _carInfo.erase(it);
            handle_flag[free_handle] = false;
            ss_print("delete old rgn, handle : %d\n", free_handle);
        } else {
            ++it;
        }
    }


    return HI_SUCCESS;
}

hi_s32 SD3403_RGN::get_rgn_handle(void)
{
    int i = 0;

    for(i = 0; i < sizeof(handle_flag); i++)
    {
        if(handle_flag[i] == false)
            return i;
    }
    if(i>sizeof(handle_flag))
    {
        printf("rgn handle exceeds the specified maximum value!\n");
        return -1;
    }
}

void SD3403_RGN::delete_handle(hi_s32 handle)
{
    for (auto iter = _carInfo.begin(); iter != _carInfo.end(); iter++)
    {
        if (iter->second.handle == handle)
        {
            _carInfo.erase(iter);
            break;
        }
    }
    handle_flag[handle] = false;
    return;
}