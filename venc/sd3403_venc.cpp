#include "sd3403_venc.hpp"

SD3403_VENC::SD3403_VENC()
{
}
SD3403_VENC::~SD3403_VENC()
{
}

hi_s32 SD3403_VENC::start_venc(hi_venc_chn venc_chn[], hi_u32 chn_num, const hi_size in_size)
{
    hi_s32 i, ret;

    g_venc_chn_param.venc_size.width = in_size.width;
    g_venc_chn_param.venc_size.height = in_size.height;
    g_venc_chn_param.size = sys_get_pic_enum(in_size);

    for (i = 0; i < (hi_s32)chn_num; i++)
    {
        ret = venc_start(venc_chn[i], &g_venc_chn_param);
        if (ret != HI_SUCCESS)
        {
            goto exit;
        }
    }

    // ret = venc_start_get_stream(venc_chn, chn_num);
    // if (ret != HI_SUCCESS)
    // {
    //     goto exit;
    // }

    return HI_SUCCESS;

exit:
    for (i = i - 1; i >= 0; i--)
    {
        venc_stop(venc_chn[i]);
    }
    return HI_FAILURE;
}

int SD3403_VENC::request_key_frame(int venc_chn)
{
    hi_s32 s32Ret = 0;

    s32Ret = hi_mpi_venc_request_idr(venc_chn, HI_FALSE);
    if (HI_SUCCESS != s32Ret)
    {
        printf("chn : %d get IDR frame error!\n", venc_chn);
        return -1;
    }

    return 0;
}

int SD3403_VENC::getSensor_Frame(char *data, uint64_t *pts, int ms, bool &key, int venc_chn)
{
    hi_s32 s32Ret = 0;
    hi_venc_chn_attr venc_chn_attr;
    hi_venc_stream_buf_info stream_buf_info;
    int VencFd = 0;
    hi_venc_chn_status stStat;
    hi_venc_stream stStream;
    int size = 0;

    s32Ret = hi_mpi_venc_get_chn_attr(venc_chn, &venc_chn_attr);
    if (s32Ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_get_chn_attr chn[%d] failed with %#x!\n", venc_chn, s32Ret);
        return SAMPLE_RETURN_NULL;
    }
    VencFd = hi_mpi_venc_get_fd(venc_chn);
    if (VencFd < 0)
    {
        ss_print("hi_mpi_venc_get_fd failed with %#x!\n", VencFd);
        return SAMPLE_RETURN_NULL;
    }
    s32Ret = hi_mpi_venc_get_stream_buf_info(venc_chn, &stream_buf_info);
    if (s32Ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_get_stream_buf_info failed with %#x!\n", s32Ret);
        return SAMPLE_RETURN_FAILURE;
    }
    fd_set read_fds;
    struct timeval TimeoutVal;
    FD_ZERO(&read_fds);
    FD_SET(VencFd, &read_fds);
    TimeoutVal.tv_sec = 0;
    TimeoutVal.tv_usec = 500000;

    s32Ret = select(VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        ss_print("select failed!\n");
        return -1;
    }
    else if (s32Ret == 0)
    {

        ss_print("get venc chn_%d stream time out.\n", venc_chn);
        return 0;
    }
    else
    {
        if (FD_ISSET(VencFd, &read_fds))
        {
            s32Ret = hi_mpi_venc_query_status(venc_chn, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                ss_print("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", venc_chn, s32Ret);
                return -1;
            }
            if (0 == stStat.cur_packs)
            {
                ss_print("NOTE: Current  frame is NULL!\n");
                return -1;
            }
            stStream.pack = (hi_venc_pack *)malloc(sizeof(hi_venc_pack) * stStat.cur_packs);
            if (NULL == stStream.pack)
            {
                ss_print("malloc stream pack failed!\n");
                return -1;
            }

            stStream.pack_cnt = stStat.cur_packs;
            s32Ret = hi_mpi_venc_get_stream(venc_chn, &stStream, ms);
            if (HI_SUCCESS != s32Ret)
            {
                free(stStream.pack);
                stStream.pack = NULL;
                ss_print("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                return -1;
            }
            hi_s32 i = 0;
            char *p = data;

            for (i = 0; i < stStream.pack_cnt; i++)
            {
                memcpy(p, stStream.pack[i].addr + stStream.pack[i].offset,
                       stStream.pack[i].len - stStream.pack[i].offset);

                p += stStream.pack[i].len - stStream.pack[i].offset;
                size += stStream.pack[i].len - stStream.pack[i].offset;
            }

            if (venc_chn_attr.venc_attr.type == HI_PT_H264)
            {
                if (stStream.pack->data_type.h264_type == HI_VENC_H264_NALU_IDR_SLICE ||
                    stStream.pack->data_type.h264_type == HI_VENC_H264_NALU_I_SLICE ||
                    stStream.pack->data_type.h264_type == HI_VENC_H264_NALU_SPS)
                {
                    key = true;
                }
                else
                {
                    key = false;
                }
            }
            else if (venc_chn_attr.venc_attr.type == HI_PT_H265)
            {
                if (stStream.pack->data_type.h265_type == HI_VENC_H265_NALU_IDR_SLICE ||
                    stStream.pack->data_type.h265_type == HI_VENC_H265_NALU_I_SLICE ||
                    stStream.pack->data_type.h265_type == HI_VENC_H265_NALU_VPS)
                {
                    printf("===>get chn : %d stream, len : %d, type : %d\n", venc_chn, size, stStream.pack->data_type.h265_type);
                    char buffer[1024] = {0};
                    sprintf(buffer, "venc_%d.h265", venc_chn);
                    FILE *fp = fopen(buffer, "wb");
                    fwrite(data, size, 1, fp);
                    fclose(fp);
                    key = true;
                }
                else
                {
                    key = false;
                }
            }
            if (pts)
            {
                *pts = stStream.pack->pts;
            }

            s32Ret = hi_mpi_venc_release_stream(venc_chn, &stStream);
            if (HI_SUCCESS != s32Ret)
            {
                free(stStream.pack);
                stStream.pack = NULL;
                return -1;
            }

            if (stStream.pack)
            {
                free(stStream.pack);
                stStream.pack = NULL;
            }
        }
    }
    return size;
}

hi_s32 SD3403_VENC::venc_start_get_stream(hi_venc_chn *venc_chn, hi_s32 cnt)
{
    hi_s32 i;

    g_para.thread_start = HI_TRUE;
    g_para.cnt = cnt;
    for (i = 0; (i < cnt) && (i < HI_VENC_MAX_CHN_NUM); i++)
    {
        g_para.venc_chn[i] = venc_chn[i];
    }

    // return pthread_create(&g_venc_pid, 0, venc_get_venc_stream_proc, (hi_void *)&g_para);
    m_thread = std::thread(&SD3403_VENC::venc_get_venc_stream_proc, this);
    return 0;
}

hi_void SD3403_VENC::venc_get_venc_stream_proc(hi_void)
{
    hi_s32 i, ret;
    sd3403_venc_getstream_para *para = HI_NULL;
    struct timeval timeout_val;
    fd_set read_fds;
    hi_payload_type payload_type[HI_VENC_MAX_CHN_NUM];
    hi_venc_stream_buf_info stream_buf_info[HI_VENC_MAX_CHN_NUM];
    sd3403_venc_stream_proc_info stream_proc_info = {0};

    memset(payload_type, 0, sizeof(hi_payload_type) * HI_VENC_MAX_CHN_NUM);
    prctl(PR_SET_NAME, "get_venc_stream", 0, 0, 0);

    para = (sd3403_venc_getstream_para *)&g_para;
    stream_proc_info.chn_total = para->cnt;
    /* step 1:  check & prepare save-file & venc-fd */
    if (stream_proc_info.chn_total >= HI_VENC_MAX_CHN_NUM)
    {
        ss_print("input count invalid\n");
        return;
    }

    ret = set_name_save_stream(&stream_proc_info, stream_buf_info, payload_type, para, HI_VENC_MAX_CHN_NUM);
    if (ret == SAMPLE_RETURN_NULL)
    {
        return;
    }
    else if (ret == SAMPLE_RETURN_FAILURE)
    {
        return;
    }

    /* step 2:  start to get streams of each channel. */
    while (para->thread_start == HI_TRUE)
    {
        FD_ZERO(&read_fds);
        for (i = 0; (i < stream_proc_info.chn_total) && (i < HI_VENC_MAX_CHN_NUM); i++)
        {
            FD_SET(stream_proc_info.venc_fd[i], &read_fds);
        }

        timeout_val.tv_sec = 2; /* 2 is a number */
        timeout_val.tv_usec = 0;
        ret = select(stream_proc_info.maxfd + 1, &read_fds, HI_NULL, HI_NULL, &timeout_val);
        if (ret < 0)
        {
            ss_print("select failed!\n");
            break;
        }
        else if (ret == 0)
        {
            ss_print("get venc stream time out, exit thread\n");
            continue;
        }
        else
        {
            fd_isset(&stream_proc_info, &read_fds, stream_buf_info, payload_type, para);
        }
    }

    /* step 3 : close save-file */
    for (i = 0; i < stream_proc_info.chn_total; i++)
    {
        if (payload_type[i] != HI_PT_JPEG)
        {
            fclose(stream_proc_info.file[i]);
        }
    }

    return;
}
hi_s32 SD3403_VENC::set_name_save_stream(sd3403_venc_stream_proc_info *stream_proc_info,
                                         hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type,
                                         sd3403_venc_getstream_para *para, hi_s32 venc_max_chn)
{
    hi_s32 i, ret, fd;
    hi_venc_chn_attr venc_chn_attr;
    hi_unused(venc_max_chn);

    for (i = 0; (i < stream_proc_info->chn_total) && (i < HI_VENC_MAX_CHN_NUM); i++)
    {
        /* decide the stream file name, and open file to save stream */
        hi_venc_chn venc_chn = para->venc_chn[i];
        ret = hi_mpi_venc_get_chn_attr(venc_chn, &venc_chn_attr);
        if (ret != HI_SUCCESS)
        {
            ss_print("hi_mpi_venc_get_chn_attr chn[%d] failed with %#x!\n", venc_chn, ret);
            return SAMPLE_RETURN_NULL;
        }
        payload_type[i] = venc_chn_attr.venc_attr.type;

        ret = venc_get_file_postfix(payload_type[i], stream_proc_info->file_postfix,
                                    sizeof(stream_proc_info->file_postfix));
        if (ret != HI_SUCCESS)
        {
            ss_print("sample_comm_venc_get_file_postfix [%d] failed with %#x!\n",
                     venc_chn_attr.venc_attr.type, ret);
            return SAMPLE_RETURN_NULL;
        }

        if (payload_type[i] != HI_PT_JPEG)
        {
            ret = set_file_name(i, venc_chn, stream_proc_info);
            if (ret != HI_SUCCESS)
            {
                return ret;
            }

            stream_proc_info->file[i] = fopen(stream_proc_info->real_file_name[i], "wb");
            if (!stream_proc_info->file[i])
            {
                ss_print("open file[%s] failed!\n", stream_proc_info->real_file_name[i]);
                return SAMPLE_RETURN_NULL;
            }
            fd = fileno(stream_proc_info->file[i]);
            fchmod(fd, S_IRUSR | S_IWUSR);
        }
        /* set venc fd. */
        stream_proc_info->venc_fd[i] = hi_mpi_venc_get_fd(i);
        if (stream_proc_info->venc_fd[i] < 0)
        {
            ss_print("hi_mpi_venc_get_fd failed with %#x!\n", stream_proc_info->venc_fd[i]);
            return SAMPLE_RETURN_NULL;
        }

        if (stream_proc_info->maxfd <= stream_proc_info->venc_fd[i])
        {
            stream_proc_info->maxfd = stream_proc_info->venc_fd[i];
        }

        ret = hi_mpi_venc_get_stream_buf_info(i, &stream_buf_info[i]);
        if (ret != HI_SUCCESS)
        {
            ss_print("hi_mpi_venc_get_stream_buf_info failed with %#x!\n", ret);
            return SAMPLE_RETURN_FAILURE;
        }
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VENC::venc_get_file_postfix(hi_payload_type payload, hi_char *file_postfix, hi_u8 len)
{
    if (payload == HI_PT_H264)
    {
        if (strcpy_s(file_postfix, len, ".h264") != EOK)
        {
            return HI_FAILURE;
        }
    }
    else if (payload == HI_PT_H265)
    {
        if (strcpy_s(file_postfix, len, ".h265") != EOK)
        {
            return HI_FAILURE;
        }
    }
    else if (payload == HI_PT_JPEG)
    {
        if (strcpy_s(file_postfix, len, ".jpg") != EOK)
        {
            return HI_FAILURE;
        }
    }
    else if (payload == HI_PT_MJPEG)
    {
        if (strcpy_s(file_postfix, len, ".mjp") != EOK)
        {
            return HI_FAILURE;
        }
    }
    else
    {
        ss_print("payload type err!\n");
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 SD3403_VENC::set_file_name(hi_s32 index, hi_venc_chn venc_chn, sd3403_venc_stream_proc_info *stream_proc_info)
{
    if (snprintf_s(stream_proc_info->file_name[index], FILE_NAME_LEN, FILE_NAME_LEN - 1, "./") < 0)
    {
        return SAMPLE_RETURN_NULL;
    }

    if (realpath(stream_proc_info->file_name[index], stream_proc_info->real_file_name[index]) == HI_NULL)
    {
        ss_print("chn[%d] stream file path error\n", venc_chn);
        return SAMPLE_RETURN_NULL;
    }

    if (snprintf_s(stream_proc_info->real_file_name[index], FILE_NAME_LEN, FILE_NAME_LEN - 1,
                   "stream_chn%d%s", index, stream_proc_info->file_postfix) < 0)
    {
        return SAMPLE_RETURN_NULL;
    }

    return HI_SUCCESS;
}

hi_void SD3403_VENC::fd_isset(sd3403_venc_stream_proc_info *stream_proc_info, fd_set *read_fds,
                              hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type, sd3403_venc_getstream_para *para)
{
    hi_s32 i, ret;

    // printf("chn_total: %d, HI_VENC_MAX_CHN_NUM : %d\n", stream_proc_info->chn_total, HI_VENC_MAX_CHN_NUM);
    for (i = 0; (i < stream_proc_info->chn_total) && (i < HI_VENC_MAX_CHN_NUM); i++)
    {
        if (FD_ISSET(stream_proc_info->venc_fd[i], read_fds))
        {
            stream_proc_info->venc_chn = para->venc_chn[i];
            ret = get_stream_from_one_channl(stream_proc_info, i, stream_buf_info, payload_type);
            if (ret == SAMPLE_RETURN_CONTINUE)
            {
                continue;
            }
            else if (ret == SAMPLE_RETURN_BREAK)
            {
                break;
            }
        }
    }
}
hi_s32 SD3403_VENC::get_stream_from_one_channl(sd3403_venc_stream_proc_info *stream_proc_info,
                                               hi_s32 index, hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type)
{
    hi_s32 ret;
    hi_venc_stream stream;
    hi_venc_chn_status stat;

    /* step 2.1 : query how many packs in one-frame stream. */
    if (memset_s(&stream, sizeof(stream), 0, sizeof(stream)) != EOK)
    {
        printf("call memset_s error\n");
    }

    ret = hi_mpi_venc_query_status(index, &stat);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_query_status chn[%d] failed with %#x!\n", index, ret);
        return SAMPLE_RETURN_BREAK;
    }

    if (stat.cur_packs == 0)
    {
        ss_print("NOTE: current  frame is HI_NULL!\n");
        return SAMPLE_RETURN_CONTINUE;
    }
    /* step 2.3 : malloc corresponding number of pack nodes. */
    stream.pack = (hi_venc_pack *)malloc(sizeof(hi_venc_pack) * stat.cur_packs);
    if (stream.pack == HI_NULL)
    {
        ss_print("malloc stream pack failed!\n");
        return SAMPLE_RETURN_BREAK;
    }

    /* step 2.4 : call mpi to get one-frame stream */
    stream.pack_cnt = stat.cur_packs;
    ret = hi_mpi_venc_get_stream(index, &stream, HI_TRUE);
    if (ret != HI_SUCCESS)
    {
        free(stream.pack);
        stream.pack = HI_NULL;
        ss_print("hi_mpi_venc_get_stream failed with %#x!\n", ret);
        return SAMPLE_RETURN_BREAK;
    }

    /* step 2.5 : save frame to file */
    // ret = save_frame_to_file(index, stream_proc_info, &stream, stream_buf_info, payload_type);
    // if (ret != HI_SUCCESS) {
    //     return ret;
    // }

    /* step 2.6 : release stream */
    ret = hi_mpi_venc_release_stream(index, &stream);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_release_stream failed!\n");
        free(stream.pack);
        stream.pack = HI_NULL;
        return SAMPLE_RETURN_BREAK;
    }

    /* step 2.7 : free pack nodes */
    free(stream.pack);
    stream.pack = HI_NULL;
    stream_proc_info->picture_cnt[index]++;
    if (payload_type[index] == HI_PT_JPEG)
    {
        fclose(stream_proc_info->file[index]);
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VENC::save_frame_to_file(hi_s32 index, sd3403_venc_stream_proc_info *stream_proc_info,
                                       hi_venc_stream *stream, hi_venc_stream_buf_info *stream_buf_info, hi_payload_type *payload_type)
{
    hi_s32 ret, fd;
    if (payload_type[index] == HI_PT_JPEG)
    {
        if (snprintf_s(stream_proc_info->file_name[index], FILE_NAME_LEN, FILE_NAME_LEN - 1, "./") < 0)
        {
            free(stream->pack);
            return SAMPLE_RETURN_NULL;
        }
        if (realpath(stream_proc_info->file_name[index], stream_proc_info->real_file_name[index]) == HI_NULL)
        {
            free(stream->pack);
            ss_print("chn[%d] stream file path error\n", stream_proc_info->venc_chn);
            return SAMPLE_RETURN_NULL;
        }

        if (snprintf_s(stream_proc_info->real_file_name[index], FILE_NAME_LEN, FILE_NAME_LEN - 1,
                       "stream_chn%d_%d%s", index, stream_proc_info->picture_cnt[index], stream_proc_info->file_postfix) < 0)
        {
            free(stream->pack);
            return SAMPLE_RETURN_NULL;
        }
        stream_proc_info->file[index] = fopen(stream_proc_info->real_file_name[index], "wb");
        if (!stream_proc_info->file[index])
        {
            free(stream->pack);
            ss_print("open file err!\n");
            return SAMPLE_RETURN_NULL;
        }
        fd = fileno(stream_proc_info->file[index]);
        fchmod(fd, S_IRUSR | S_IWUSR);
    }

#ifndef __LITEOS__
    hi_unused(stream_buf_info);
    ret = venc_save_stream(stream_proc_info->file[index], stream);
#else
    ret = venc_save_stream_phys_addr(stream_proc_info->file[index], &stream_buf_info[index], stream);
#endif
    if (ret != HI_SUCCESS)
    {
        free(stream->pack);
        stream->pack = HI_NULL;
        ss_print("save stream failed!\n");
        return SAMPLE_RETURN_BREAK;
    }

    return HI_SUCCESS;
}
hi_s32 SD3403_VENC::venc_save_stream(FILE *fd, hi_venc_stream *stream)
{
    hi_u32 i;

    for (i = 0; i < stream->pack_cnt; i++)
    {
        fwrite(stream->pack[i].addr + stream->pack[i].offset, stream->pack[i].len - stream->pack[i].offset, 1, fd);

        fflush(fd);
    }

    return HI_SUCCESS;
}
hi_s32 SD3403_VENC::venc_save_stream_phys_addr(FILE *fd, hi_venc_stream_buf_info *stream_buf, hi_venc_stream *stream)
{
    hi_u32 i, j;
    hi_s32 ret;

    for (i = 0; i < stream->pack_cnt; i++)
    {
        for (j = 0; j < HI_VENC_MAX_TILE_NUM; j++)
        {
            if ((stream->pack[i].phys_addr > stream_buf->phys_addr[j]) &&
                (stream->pack[i].phys_addr <= stream_buf->phys_addr[j] + stream_buf->buf_size[j]))
            {
                break;
            }
        }

        if (j < HI_VENC_MAX_TILE_NUM && stream->pack[i].phys_addr + stream->pack[i].len >=
                                            stream_buf->phys_addr[j] + stream_buf->buf_size[j])
        {
            ret = venc_phys_addr_retrace(fd, stream_buf, stream, i, j);
            if (ret < 0)
            {
                return ret;
            }
        }
        else
        {
            /* physical address retrace does not happen */
            ret = fwrite((hi_void *)(hi_uintptr_t)(stream->pack[i].phys_addr + stream->pack[i].offset),
                         stream->pack[i].len - stream->pack[i].offset, 1, fd);
            if (ret < 0)
            {
                ss_print("fwrite err %d\n", ret);
                return ret;
            }
        }
        fflush(fd);
    }

    return HI_SUCCESS;
}
hi_s32 SD3403_VENC::venc_phys_addr_retrace(FILE *fd, hi_venc_stream_buf_info *stream_buf, hi_venc_stream *stream, hi_u32 i, hi_u32 j)
{
    hi_u64 src_phys_addr;
    hi_u32 left;
    hi_s32 ret;

    if (stream->pack[i].phys_addr + stream->pack[i].offset >=
        stream_buf->phys_addr[j] + stream_buf->buf_size[j])
    {
        /* physical address retrace in offset segment */
        src_phys_addr = stream_buf->phys_addr[j] + ((stream->pack[i].phys_addr + stream->pack[i].offset) -
                                                    (stream_buf->phys_addr[j] + stream_buf->buf_size[j]));

        ret = fwrite((hi_void *)(hi_uintptr_t)src_phys_addr, stream->pack[i].len - stream->pack[i].offset, 1, fd);
        if (ret >= 0)
        {
            ss_print("fwrite err %d\n", ret);
            return ret;
        }
    }
    else
    {
        /* physical address retrace in data segment */
        left = (stream_buf->phys_addr[j] + stream_buf->buf_size[j]) - stream->pack[i].phys_addr;

        ret = fwrite((hi_void *)(hi_uintptr_t)(stream->pack[i].phys_addr + stream->pack[i].offset),
                     left - stream->pack[i].offset, 1, fd);
        if (ret < 0)
        {
            ss_print("fwrite err %d\n", ret);
            return ret;
        }

        ret = fwrite((hi_void *)(hi_uintptr_t)stream_buf->phys_addr[j], stream->pack[i].len - left, 1, fd);
        if (ret < 0)
        {
            ss_print("fwrite err %d\n", ret);
            return ret;
        }
    }

    return HI_SUCCESS;
}

hi_pic_size SD3403_VENC::sys_get_pic_enum(const hi_size size)
{
    // hi_pic_size i;
    int i = 0;

    for (i = PIC_CIF; i < PIC_BUTT; i++)
    {
        hi_pic_size j = static_cast<hi_pic_size>(i);
        if ((g_sample_pic_size[j].width == size.width) &&
            (g_sample_pic_size[j].height == size.height))
        {
            return j;
        }
    }

    return PIC_1080P;
}

hi_s32 SD3403_VENC::venc_start(hi_venc_chn venc_chn, sd3403_venc_chn_param *chn_param)
{
    hi_s32 ret;
    hi_venc_start_param start_param;

    /* step 1: create encode chnl */
    if ((ret = venc_create(venc_chn, chn_param)) != HI_SUCCESS)
    {
        ss_print("sample_comm_venc_create failed with%#x! \n", ret);
        return HI_FAILURE;
    }
    /* step 2:  start recv venc pictures */
    start_param.recv_pic_num = -1;
    if ((ret = hi_mpi_venc_start_chn(venc_chn, &start_param)) != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_start_recv_pic failed with%#x! \n", ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 SD3403_VENC::venc_create(hi_venc_chn venc_chn, sd3403_venc_chn_param *chn_param)
{
    hi_s32 ret;
    hi_venc_chn_attr venc_chn_attr;
    hi_pic_size size = chn_param->size;

    chn_param->frame_rate = 30; /* 30 is a number */
    chn_param->gop = 30;        /* 30 is a number */

    if (size >= PIC_BUTT)
    {
        ss_print("illegal size!\n");
        return HI_FAILURE;
    }

    chn_param->venc_size.width = g_sample_pic_size[size].width;
    chn_param->venc_size.height = g_sample_pic_size[size].height;

    /* step 1:  create venc channel */
    if ((ret = venc_channel_param_init(chn_param, &venc_chn_attr)) != HI_SUCCESS)
    {
        ss_print("venc_channel_param_init failed!\n");
        return ret;
    }

    if ((ret = hi_mpi_venc_create_chn(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_create_chn [%d] failed with %#x! ===\n", venc_chn, ret);
        return ret;
    }

    if (chn_param->type == HI_PT_JPEG)
    {
        return HI_SUCCESS;
    }

    if ((ret = venc_close_reencode(venc_chn)) != HI_SUCCESS)
    {
        hi_mpi_venc_destroy_chn(venc_chn);
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VENC::venc_close_reencode(hi_venc_chn venc_chn)
{
    hi_s32 ret;
    hi_venc_rc_param rc_param;
    hi_venc_chn_attr chn_attr;

    ret = hi_mpi_venc_get_chn_attr(venc_chn, &chn_attr);
    if (ret != HI_SUCCESS)
    {
        ss_print("GetChnAttr failed!\n");
        return HI_FAILURE;
    }

    ret = hi_mpi_venc_get_rc_param(venc_chn, &rc_param);
    if (ret != HI_SUCCESS)
    {
        ss_print("GetRcParam failed!\n");
        return HI_FAILURE;
    }

    if (chn_attr.rc_attr.rc_mode == HI_VENC_RC_MODE_H264_CBR)
    {
        rc_param.h264_cbr_param.max_reencode_times = 0;
    }
    else if (chn_attr.rc_attr.rc_mode == HI_VENC_RC_MODE_H264_VBR)
    {
        rc_param.h264_vbr_param.max_reencode_times = 0;
    }
    else if (chn_attr.rc_attr.rc_mode == HI_VENC_RC_MODE_H265_CBR)
    {
        rc_param.h265_cbr_param.max_reencode_times = 0;
    }
    else if (chn_attr.rc_attr.rc_mode == HI_VENC_RC_MODE_H265_VBR)
    {
        rc_param.h265_vbr_param.max_reencode_times = 0;
    }
    else
    {
        return HI_SUCCESS;
    }

    ret = hi_mpi_venc_set_rc_param(venc_chn, &rc_param);
    if (ret != HI_SUCCESS)
    {
        ss_print("SetRcParam failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SD3403_VENC::venc_channel_param_init(sd3403_venc_chn_param *chn_param, hi_venc_chn_attr *chn_attr)
{
    hi_s32 ret;
    hi_venc_gop_attr *gop_attr = &chn_param->gop_attr;
    hi_u32 profile = chn_param->profile;
    hi_payload_type type = chn_param->type;
    hi_size venc_size = chn_param->venc_size;

    chn_attr->venc_attr.type = type;
    chn_attr->venc_attr.max_pic_width = venc_size.width;
    chn_attr->venc_attr.max_pic_height = venc_size.height;
    chn_attr->venc_attr.pic_width = venc_size.width;   /* the picture width */
    chn_attr->venc_attr.pic_height = venc_size.height; /* the picture height */

    if (type == HI_PT_MJPEG || type == HI_PT_JPEG)
    {
        chn_attr->venc_attr.buf_size =
            HI_ALIGN_UP(venc_size.width, 16) * HI_ALIGN_UP(venc_size.height, 16) * 4; /* 16 4 is a number */
    }
    else
    {
        chn_attr->venc_attr.buf_size =
            HI_ALIGN_UP(venc_size.width * venc_size.height * 3 / 4, 64); /*  3  4 64 is a number */
    }
    chn_attr->venc_attr.profile = profile;
    chn_attr->venc_attr.is_by_frame = HI_TRUE; /* get stream mode is slice mode or frame mode? */

    if (gop_attr->gop_mode == HI_VENC_GOP_MODE_SMART_P)
    {
        chn_param->stats_time = gop_attr->smart_p.bg_interval / chn_param->gop;
    }
    else
    {
        chn_param->stats_time = 1;
    }

    switch (type)
    {
    case HI_PT_H265:
        ret = venc_h265_param_init(chn_attr, chn_param);
        break;

    case HI_PT_H264:
        ret = venc_h264_param_init(chn_attr, chn_param);
        break;

    case HI_PT_MJPEG:
        ret = venc_mjpeg_param_init(chn_attr, chn_param);
        break;

    case HI_PT_JPEG:
        ret = venc_jpeg_param_init(chn_attr);
        break;

    default:
        ss_print("can't support this type (%d) in this version!\n", type);
        return HI_ERR_VENC_NOT_SUPPORT;
    }

    venc_set_gop_attr(type, chn_attr, gop_attr);
    return ret;
}

hi_s32 SD3403_VENC::venc_h265_param_init(hi_venc_chn_attr *chn_attr, sd3403_venc_chn_param *chn_param)
{
    sd3403_rc rc_mode = chn_param->rc_mode;
    hi_u32 gop = chn_param->gop;
    hi_u32 stats_time = chn_param->stats_time;
    hi_u32 frame_rate = chn_param->frame_rate;
    hi_pic_size size = chn_param->size;

    chn_attr->venc_attr.h265_attr.frame_buf_ratio = SD3403_FRAME_BUF_RATIO_MIN;
    if (rc_mode == SD3403_RC_CBR)
    {
        venc_h265_cbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_FIXQP)
    {
        venc_h265_fixqp_param_init(chn_attr, gop, frame_rate);
    }
    else if (rc_mode == SD3403_RC_VBR)
    {
        venc_h265_vbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_AVBR)
    {
        venc_h265_avbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_CVBR)
    {
        venc_h265_cvbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_QVBR)
    {
        venc_h265_qvbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_QPMAP)
    {
        venc_h265_qpmap_param_init(chn_attr, gop, frame_rate, stats_time);
    }
    else
    {
        ss_print("%s,%d,rc_mode(%d) not support\n", __FUNCTION__, __LINE__, rc_mode);
        return HI_FAILURE;
    }
    chn_attr->venc_attr.h265_attr.rcn_ref_share_buf_en = chn_param->is_rcn_ref_share_buf;

    return HI_SUCCESS;
}
hi_void SD3403_VENC::venc_h265_cbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h265_cbr h265_cbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H265_CBR;
    h265_cbr.gop = gop;
    h265_cbr.stats_time = stats_time;     /* stream rate statics time(s) */
    h265_cbr.src_frame_rate = frame_rate; /* input (vi) frame rate */
    h265_cbr.dst_frame_rate = frame_rate; /* target frame rate */
    switch (size)
    {
    case PIC_D1_NTSC:
        h265_cbr.bit_rate = 1024 * frame_rate / 30; /* 1024 is a number 30 is a number */
        break;

    case PIC_720P:
        h265_cbr.bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        break;

    case PIC_1080P:
        h265_cbr.bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h265_cbr.bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h265_cbr.bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h265_cbr.bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h265_cbr.bit_rate = 1024 * 20 + 5120 * frame_rate / 30; /* 1024 20 5120 30 is a number */
        break;

    default:
        h265_cbr.bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;
    }
    venc_chn_attr->rc_attr.h265_cbr = h265_cbr;
}

hi_void SD3403_VENC::venc_h265_fixqp_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate)
{
    hi_venc_h265_fixqp h265_fixqp;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H265_FIXQP;
    h265_fixqp.gop = gop;
    h265_fixqp.src_frame_rate = frame_rate;
    h265_fixqp.dst_frame_rate = frame_rate;
    h265_fixqp.i_qp = 25; /* 25 is a number */
    h265_fixqp.p_qp = 30; /* 30 is a number */
    h265_fixqp.b_qp = 32; /* 32 is a number */
    venc_chn_attr->rc_attr.h265_fixqp = h265_fixqp;
}

hi_void SD3403_VENC::venc_h265_vbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h265_vbr h265_vbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H265_VBR;
    h265_vbr.gop = gop;
    h265_vbr.stats_time = stats_time;
    h265_vbr.src_frame_rate = frame_rate;
    h265_vbr.dst_frame_rate = frame_rate;
    switch (size)
    {
    case PIC_D1_NTSC:
        h265_vbr.max_bit_rate = 1024 * frame_rate / 30; /* 1024 is a number 30 is a number */
        break;

    case PIC_720P:
        h265_vbr.max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        break;

    case PIC_1080P:
        h265_vbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h265_vbr.max_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 is 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h265_vbr.max_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h265_vbr.max_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h265_vbr.max_bit_rate = 1024 * 20 + 5120 * frame_rate / 30; /* 1024 20 5120 30 is a number */
        break;

    default:
        h265_vbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;
    }
    venc_chn_attr->rc_attr.h265_vbr = h265_vbr;
}

hi_void SD3403_VENC::venc_h265_avbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h265_avbr h265_avbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H265_AVBR;
    h265_avbr.gop = gop;
    h265_avbr.stats_time = stats_time;
    h265_avbr.src_frame_rate = frame_rate;
    h265_avbr.dst_frame_rate = frame_rate;

    switch (size)
    {
    case PIC_D1_NTSC:
        h265_avbr.max_bit_rate = 1024 * frame_rate / 30; /* 1024 is a number 30 is a number */
        break;

    case PIC_720P:
        h265_avbr.max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        break;

    case PIC_1080P:
        h265_avbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h265_avbr.max_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h265_avbr.max_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h265_avbr.max_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h265_avbr.max_bit_rate = 1024 * 20 + 5120 * frame_rate / 30; /* 1024 20 5120 30 is a number */
        break;

    default:
        h265_avbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;
    }
    venc_chn_attr->rc_attr.h265_avbr = h265_avbr;
}

hi_void SD3403_VENC::venc_h265_cvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h265_cvbr h265_cvbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H265_CVBR;
    h265_cvbr.gop = gop;
    h265_cvbr.stats_time = stats_time;
    h265_cvbr.src_frame_rate = frame_rate;
    h265_cvbr.dst_frame_rate = frame_rate;
    h265_cvbr.long_term_stats_time = 1;
    h265_cvbr.short_term_stats_time = stats_time;

    venc_set_h265_cvbr_bit_rate(&h265_cvbr, frame_rate, size);

    venc_chn_attr->rc_attr.h265_cvbr = h265_cvbr;
}
hi_void SD3403_VENC::venc_set_h265_cvbr_bit_rate(hi_venc_h264_cvbr *h265_cvbr, hi_u32 frame_rate, hi_pic_size size)
{
    switch (size)
    {
    case PIC_D1_NTSC:
        h265_cvbr->max_bit_rate = 1024 + 512 * frame_rate / 30;           /* 1024 512 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 + 512 * frame_rate / 30; /* 1024 512 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 256;                          /* 256 is a number */
        break;

    case PIC_720P:
        h265_cvbr->max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30;           /* 1024 2 1024 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 512;                               /* 512 is a number */
        break;

    case PIC_1080P:
        h265_cvbr->max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30;           /* 1024 2 2048 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 1024;                              /* 1024 is a number */
        break;

    case PIC_2592X1944:
        h265_cvbr->max_bit_rate = 1024 * 4 + 3072 * frame_rate / 30;           /* 1024 4 3072 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 1024 * 2;                          /* 1024 2 is a number */
        break;

    case PIC_3840X2160:
        h265_cvbr->max_bit_rate = 1024 * 8 + 5120 * frame_rate / 30;           /* 1024 8 5120 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 1024 * 3;                          /* 1024 3 is a number */
        break;

    case PIC_4000X3000:
        h265_cvbr->max_bit_rate = 1024 * 12 + 5120 * frame_rate / 30;           /* 1024 12 5120 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 1024 * 4;                           /* 1024 4 is a number */
        break;

    case PIC_7680X4320:
        h265_cvbr->max_bit_rate = 1024 * 24 + 5120 * frame_rate / 30;           /* 1024 24 5120 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 * 15 + 5120 * frame_rate / 30; /* 1024 15 5120 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 1024 * 5;                           /* 1024 5 is a number */
        break;

    default:
        h265_cvbr->max_bit_rate = 1024 * 24 + 2048 * frame_rate / 30;           /* 1024  24  2048 30 is a number */
        h265_cvbr->long_term_max_bit_rate = 1024 * 15 + 2048 * frame_rate / 30; /* 1024 15 2048 30 is a number */
        h265_cvbr->long_term_min_bit_rate = 1024 * 5;                           /* 1024 5 is a number */
        break;
    }
}

hi_void SD3403_VENC::venc_h265_qvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h265_qvbr h265_qvbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H265_QVBR;
    h265_qvbr.gop = gop;
    h265_qvbr.stats_time = stats_time;
    h265_qvbr.src_frame_rate = frame_rate;
    h265_qvbr.dst_frame_rate = frame_rate;
    switch (size)
    {
    case PIC_D1_NTSC:
        h265_qvbr.target_bit_rate = 1024 + 512 * frame_rate / 30; /* 1024 512 30 is a number */
        break;

    case PIC_720P:
        h265_qvbr.target_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        break;

    case PIC_1080P:
        h265_qvbr.target_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h265_qvbr.target_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h265_qvbr.target_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h265_qvbr.target_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h265_qvbr.target_bit_rate = 1024 * 20 + 5120 * frame_rate / 30; /* 1024 20 5120 30 is a number */
        break;

    default:
        h265_qvbr.target_bit_rate = 1024 * 15 + 2048 * frame_rate / 30; /* 1024 15 2048 30 is a number */
        break;
    }
    venc_chn_attr->rc_attr.h265_qvbr = h265_qvbr;
}

hi_void SD3403_VENC::venc_h265_qpmap_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate, hi_u32 stats_time)
{
    hi_venc_h265_qpmap h265_qpmap;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H265_QPMAP;
    h265_qpmap.gop = gop;
    h265_qpmap.stats_time = stats_time;
    h265_qpmap.src_frame_rate = frame_rate;
    h265_qpmap.dst_frame_rate = frame_rate;
    h265_qpmap.qpmap_mode = HI_VENC_RC_QPMAP_MODE_MEAN_QP;
    venc_chn_attr->rc_attr.h265_qpmap = h265_qpmap;
}

hi_s32 SD3403_VENC::venc_h264_param_init(hi_venc_chn_attr *chn_attr, sd3403_venc_chn_param *chn_param)
{
    sd3403_rc rc_mode = chn_param->rc_mode;
    hi_u32 gop = chn_param->gop;
    hi_u32 stats_time = chn_param->stats_time;
    hi_u32 frame_rate = chn_param->frame_rate;
    hi_pic_size size = chn_param->size;

    chn_attr->venc_attr.h264_attr.frame_buf_ratio = SD3403_FRAME_BUF_RATIO_MIN;
    if (rc_mode == SD3403_RC_CBR)
    {
        venc_h264_cbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_FIXQP)
    {
        venc_h264_fixqp_param_init(chn_attr, gop, frame_rate);
    }
    else if (rc_mode == SD3403_RC_VBR)
    {
        venc_h264_vbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_AVBR)
    {
        venc_h264_avbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_CVBR)
    {
        venc_h264_cvbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_QVBR)
    {
        venc_h264_qvbr_param_init(chn_attr, gop, stats_time, frame_rate, size);
    }
    else if (rc_mode == SD3403_RC_QPMAP)
    {
        venc_h264_qpmap_param_init(chn_attr, gop, frame_rate, stats_time);
    }
    else
    {
        ss_print("%s,%d,rc_mode(%d) not support\n", __FUNCTION__, __LINE__, rc_mode);
        return HI_FAILURE;
    }
    chn_attr->venc_attr.h264_attr.rcn_ref_share_buf_en = chn_param->is_rcn_ref_share_buf;

    return HI_SUCCESS;
}

hi_void SD3403_VENC::venc_h264_cbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h264_cbr h264_cbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H264_CBR;
    h264_cbr.gop = gop;
    h264_cbr.stats_time = stats_time;     /* stream rate statics time(s) */
    h264_cbr.src_frame_rate = frame_rate; /* input (vi) frame rate */
    h264_cbr.dst_frame_rate = frame_rate; /* target frame rate */
    switch (size)
    {
    case PIC_360P:
        h264_cbr.bit_rate = 1024 * 1 + 1024 * frame_rate / 30; /* 1024 1024 30 is a number */
        break;

    case PIC_D1_NTSC:
        h264_cbr.bit_rate = 1024 * frame_rate / 30; /* 1024 is 30 is a number */
        break;

    case PIC_720P:
        h264_cbr.bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        break;

    case PIC_1080P:
        h264_cbr.bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h264_cbr.bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h264_cbr.bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h264_cbr.bit_rate = 1024 * 12 + 5120 * frame_rate / 30; /* 1024 12 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h264_cbr.bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    default:
        h264_cbr.bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2  2048 30 is a number */
        break;
    }

    venc_chn_attr->rc_attr.h264_cbr = h264_cbr;
}

hi_void SD3403_VENC::venc_h264_fixqp_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate)
{
    hi_venc_h264_fixqp h264_fixqp;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H264_FIXQP;
    h264_fixqp.gop = gop;
    h264_fixqp.src_frame_rate = frame_rate;
    h264_fixqp.dst_frame_rate = frame_rate;
    h264_fixqp.i_qp = 25; /* 25 is a number */
    h264_fixqp.p_qp = 30; /* 30 is a number */
    h264_fixqp.b_qp = 32; /* 32 is a number */
    venc_chn_attr->rc_attr.h264_fixqp = h264_fixqp;
}

hi_void SD3403_VENC::venc_h264_vbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h264_vbr h264_vbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H264_VBR;
    h264_vbr.gop = gop;
    h264_vbr.stats_time = stats_time;
    h264_vbr.src_frame_rate = frame_rate;
    h264_vbr.dst_frame_rate = frame_rate;
    switch (size)
    {
    case PIC_360P:
        h264_vbr.max_bit_rate = 1024 * 1 + 1024 * frame_rate / 30; /* 1024 30 is a number */
        break;

    case PIC_D1_NTSC:
        h264_vbr.max_bit_rate = 1024 * frame_rate / 30; /* 1024 30 is a number */
        break;

    case PIC_720P:
        h264_vbr.max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 30 is a number */
        break;

    case PIC_1080P:
        h264_vbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h264_vbr.max_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h264_vbr.max_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h264_vbr.max_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h264_vbr.max_bit_rate = 1024 * 20 + 5120 * frame_rate / 30; /* 1024 20 5120 30 is a number */
        break;

    default:
        h264_vbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;
    }

    venc_chn_attr->rc_attr.h264_vbr = h264_vbr;
}

hi_void SD3403_VENC::venc_h264_avbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h264_avbr h264_avbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H264_AVBR;
    h264_avbr.gop = gop;
    h264_avbr.stats_time = stats_time;
    h264_avbr.src_frame_rate = frame_rate;
    h264_avbr.dst_frame_rate = frame_rate;
    switch (size)
    {
    case PIC_360P:
        h264_avbr.max_bit_rate = 1024 * 1 + 1024 * frame_rate / 30; /* 1024 1024 30 is a number */
        break;

    case PIC_D1_NTSC:
        h264_avbr.max_bit_rate = 1024 * frame_rate / 30; /* 1024 30 is a number */
        break;

    case PIC_720P:
        h264_avbr.max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        break;

    case PIC_1080P:
        h264_avbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h264_avbr.max_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h264_avbr.max_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h264_avbr.max_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h264_avbr.max_bit_rate = 1024 * 20 + 5120 * frame_rate / 30; /* 1024 20 5120 30 is a number */
        break;

    default:
        h264_avbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;
    }

    venc_chn_attr->rc_attr.h264_avbr = h264_avbr;
}

hi_void SD3403_VENC::venc_h264_cvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h264_cvbr h264_cvbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H264_CVBR;
    h264_cvbr.gop = gop;
    h264_cvbr.stats_time = stats_time;
    h264_cvbr.src_frame_rate = frame_rate;
    h264_cvbr.dst_frame_rate = frame_rate;
    h264_cvbr.long_term_stats_time = 1;
    h264_cvbr.short_term_stats_time = stats_time;

    venc_set_h264_cvbr_bit_rate(&h264_cvbr, frame_rate, size);

    venc_chn_attr->rc_attr.h264_cvbr = h264_cvbr;
}

hi_void SD3403_VENC::venc_set_h264_cvbr_bit_rate(hi_venc_h264_cvbr *h264_cvbr, hi_u32 frame_rate, hi_pic_size size)
{
    switch (size)
    {
    case PIC_D1_NTSC:
        h264_cvbr->max_bit_rate = 1024 + 512 * frame_rate / 30;           /* 1024 512 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 + 512 * frame_rate / 30; /* 1024 512 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 256;                          /* 256 is a number */
        break;
    case PIC_720P:
        h264_cvbr->max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30;           /* 1024 2 1024 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 512;                               /* 512 is a number */
        break;
    case PIC_1080P:
        h264_cvbr->max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30;           /* 1024 2 2048 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 1024;                              /* 1024 is a number */
        break;
    case PIC_2592X1944:
        h264_cvbr->max_bit_rate = 1024 * 4 + 3072 * frame_rate / 30;           /* 1024 4 3072 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 1024 * 2;                          /* 1024 2 is a number */
        break;
    case PIC_3840X2160:
        h264_cvbr->max_bit_rate = 1024 * 8 + 5120 * frame_rate / 30;           /* 1024 8 5120 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 1024 * 3;                          /* 1024 3 is a number */
        break;
    case PIC_4000X3000:
        h264_cvbr->max_bit_rate = 1024 * 12 + 5120 * frame_rate / 30;           /* 1024 12 5120 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 1024 * 4;                           /* 1024 4 is a number */
        break;
    case PIC_7680X4320:
        h264_cvbr->max_bit_rate = 1024 * 24 + 5120 * frame_rate / 30;           /* 1024 24 5120 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 * 15 + 5120 * frame_rate / 30; /* 1024 15 5120 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 1024 * 5;                           /* 1024 5 is a number */
        break;
    default:
        h264_cvbr->max_bit_rate = 1024 * 24 + 2048 * frame_rate / 30;           /* 1024 24 2048 30 is a number */
        h264_cvbr->long_term_max_bit_rate = 1024 * 15 + 2048 * frame_rate / 30; /* 1024 15 2048 30 is a number */
        h264_cvbr->long_term_min_bit_rate = 1024 * 5;                           /* 1024 5 is a number */
        break;
    }
}

hi_void SD3403_VENC::venc_h264_qvbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_h264_qvbr h264_qvbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H264_QVBR;
    h264_qvbr.gop = gop;
    h264_qvbr.stats_time = stats_time;
    h264_qvbr.src_frame_rate = frame_rate;
    h264_qvbr.dst_frame_rate = frame_rate;
    switch (size)
    {
    case PIC_D1_NTSC:
        h264_qvbr.target_bit_rate = 1024 + 512 * frame_rate / 30; /* 1024 512 30 is a number */
        break;

    case PIC_720P:
        h264_qvbr.target_bit_rate = 1024 * 2 + 1024 * frame_rate / 30; /* 1024 2 1024 30 is a number */
        break;

    case PIC_1080P:
        h264_qvbr.target_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        h264_qvbr.target_bit_rate = 1024 * 3 + 3072 * frame_rate / 30; /* 1024 3 3072 30 is a number */
        break;

    case PIC_3840X2160:
        h264_qvbr.target_bit_rate = 1024 * 5 + 5120 * frame_rate / 30; /* 1024 5 5120 30 is a number */
        break;

    case PIC_4000X3000:
        h264_qvbr.target_bit_rate = 1024 * 10 + 5120 * frame_rate / 30; /* 1024 10 5120 30 is a number */
        break;

    case PIC_7680X4320:
        h264_qvbr.target_bit_rate = 1024 * 20 + 5120 * frame_rate / 30; /* 1024  20 5120 30 is a number */
        break;

    default:
        h264_qvbr.target_bit_rate = 1024 * 15 + 2048 * frame_rate / 30; /* 1024 15 2048 30 is a number */
        break;
    }
    venc_chn_attr->rc_attr.h264_qvbr = h264_qvbr;
}

hi_void SD3403_VENC::venc_h264_qpmap_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 gop, hi_u32 frame_rate, hi_u32 stats_time)
{
    hi_venc_h264_qpmap h264_qpmap;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_H264_QPMAP;
    h264_qpmap.gop = gop;
    h264_qpmap.stats_time = stats_time;
    h264_qpmap.src_frame_rate = frame_rate;
    h264_qpmap.dst_frame_rate = frame_rate;

    venc_chn_attr->rc_attr.h264_qpmap = h264_qpmap;
}
hi_s32 SD3403_VENC::venc_mjpeg_param_init(hi_venc_chn_attr *venc_chn_attr, sd3403_venc_chn_param *venc_create_chn_param)
{
    sd3403_rc rc_mode = venc_create_chn_param->rc_mode;
    hi_u32 stats_time = venc_create_chn_param->stats_time;
    hi_u32 frame_rate = venc_create_chn_param->frame_rate;
    hi_pic_size size = venc_create_chn_param->size;

    if (rc_mode == SD3403_RC_FIXQP)
    {
        venc_mjpeg_fixqp_param_init(venc_chn_attr, frame_rate);
    }
    else if (rc_mode == SD3403_RC_CBR)
    {
        venc_mjpeg_cbr_param_init(venc_chn_attr, stats_time, frame_rate, size);
    }
    else if ((rc_mode == SD3403_RC_VBR) || (rc_mode == SD3403_RC_AVBR))
    {
        if (rc_mode == SD3403_RC_AVBR)
        {
            ss_print("mjpege not support AVBR, so change rcmode to VBR!\n");
        }
        venc_mjpeg_vbr_param_init(venc_chn_attr, stats_time, frame_rate, size);
    }
    else
    {
        ss_print("can't support other mode(%d) in this version!\n", rc_mode);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}
hi_void SD3403_VENC::venc_mjpeg_fixqp_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 frame_rate)
{
    hi_venc_mjpeg_fixqp mjpege_fixqp;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_MJPEG_FIXQP;
    mjpege_fixqp.qfactor = 95; /* 95 is a number */
    mjpege_fixqp.src_frame_rate = frame_rate;
    mjpege_fixqp.dst_frame_rate = frame_rate;

    venc_chn_attr->rc_attr.mjpeg_fixqp = mjpege_fixqp;
}

hi_void SD3403_VENC::venc_mjpeg_cbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_mjpeg_cbr mjpege_cbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_MJPEG_CBR;
    mjpege_cbr.stats_time = stats_time;
    mjpege_cbr.src_frame_rate = frame_rate;
    mjpege_cbr.dst_frame_rate = frame_rate;

    switch (size)
    {
    case PIC_360P:
        mjpege_cbr.bit_rate = 1024 * 3 + 1024 * frame_rate / 30; /* 1024 3 1024 30 is a number */
        break;

    case PIC_D1_NTSC:
        mjpege_cbr.bit_rate = 1024 + 1024 * frame_rate / 30; /* 1024 1024 30 is a number */
        break;

    case PIC_720P:
        mjpege_cbr.bit_rate = 1024 * 3 + 1024 * frame_rate / 30; /* 1024 3 1024 30 is a number */
        break;

    case PIC_1080P:
        mjpege_cbr.bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        mjpege_cbr.bit_rate = 1024 * 20 + 3072 * frame_rate / 30; /* 1024 20 3072 30 is a number */
        break;

    case PIC_3840X2160:
        mjpege_cbr.bit_rate = 1024 * 25 + 5120 * frame_rate / 30; /* 1024 25 5120 30 is a number */
        break;

    case PIC_4000X3000:
        mjpege_cbr.bit_rate = 1024 * 30 + 5120 * frame_rate / 30; /* 1024 30 5120 30 is a number */
        break;

    case PIC_7680X4320:
        mjpege_cbr.bit_rate = 1024 * 40 + 5120 * frame_rate / 30; /* 1024 40 5120 30 is a number */
        break;

    default:
        mjpege_cbr.bit_rate = 1024 * 5 + 2048 * frame_rate / 30; /* 1024 5 2048 30 is a number */
        break;
    }

    venc_chn_attr->rc_attr.mjpeg_cbr = mjpege_cbr;
}
hi_void SD3403_VENC::venc_mjpeg_vbr_param_init(hi_venc_chn_attr *venc_chn_attr, hi_u32 stats_time, hi_u32 frame_rate, hi_pic_size size)
{
    hi_venc_mjpeg_vbr mjpeg_vbr;

    venc_chn_attr->rc_attr.rc_mode = HI_VENC_RC_MODE_MJPEG_VBR;
    mjpeg_vbr.stats_time = stats_time;
    mjpeg_vbr.src_frame_rate = frame_rate;
    mjpeg_vbr.dst_frame_rate = 5; /* 5 is a number */

    switch (size)
    {
    case PIC_360P:
        mjpeg_vbr.max_bit_rate = 1024 * 3 + 1024 * frame_rate / 30; /* 1024 3 1024 30 is a number */
        break;

    case PIC_D1_NTSC:
        mjpeg_vbr.max_bit_rate = 1024 + 1024 * frame_rate / 30; /* 1024 30 is a number */
        break;

    case PIC_720P:
        mjpeg_vbr.max_bit_rate = 1024 * 3 + 1024 * frame_rate / 30; /* 1024 3 30 is a number */
        break;

    case PIC_1080P:
        mjpeg_vbr.max_bit_rate = 1024 * 2 + 2048 * frame_rate / 30; /* 1024 2 2048 30 is a number */
        break;

    case PIC_2592X1944:
        mjpeg_vbr.max_bit_rate = 1024 * 20 + 3072 * frame_rate / 30; /* 1024 20 3072 30 is a number */
        break;

    case PIC_3840X2160:
        mjpeg_vbr.max_bit_rate = 1024 * 25 + 5120 * frame_rate / 30; /* 1024 25 5120 30 is a number */
        break;

    case PIC_4000X3000:
        mjpeg_vbr.max_bit_rate = 1024 * 30 + 5120 * frame_rate / 30; /* 1024 30 5120 30 is a number */
        break;

    case PIC_7680X4320:
        mjpeg_vbr.max_bit_rate = 1024 * 40 + 5120 * frame_rate / 30; /* 1024 40 5120 30 is a number */
        break;

    default:
        mjpeg_vbr.max_bit_rate = 1024 * 5 + 2048 * frame_rate / 30; /* 1024 5 2048 30 is a number */
        break;
    }

    venc_chn_attr->rc_attr.mjpeg_vbr = mjpeg_vbr;
}
hi_s32 SD3403_VENC::venc_jpeg_param_init(hi_venc_chn_attr *venc_chn_attr)
{
    hi_venc_jpeg_attr jpeg_attr;
    jpeg_attr.dcf_en = HI_FALSE;
    jpeg_attr.mpf_cfg.large_thumbnail_num = 0;
    jpeg_attr.recv_mode = HI_VENC_PIC_RECV_SINGLE;

    venc_chn_attr->venc_attr.jpeg_attr = jpeg_attr;

    return HI_SUCCESS;
}
hi_void SD3403_VENC::venc_set_gop_attr(hi_payload_type type, hi_venc_chn_attr *chn_attr, hi_venc_gop_attr *gop_attr)
{
    if (type == HI_PT_MJPEG || type == HI_PT_JPEG)
    {
        chn_attr->gop_attr.gop_mode = HI_VENC_GOP_MODE_NORMAL_P;
        chn_attr->gop_attr.normal_p.ip_qp_delta = 0;
    }
    else
    {
        chn_attr->gop_attr = *gop_attr;
        if ((gop_attr->gop_mode == HI_VENC_GOP_MODE_BIPRED_B) && (type == HI_PT_H264))
        {
            if (chn_attr->venc_attr.profile == 0)
            {
                chn_attr->venc_attr.profile = 1;

                ss_print("H.264 base profile not support BIPREDB, so change profile to main profile!\n");
            }
        }
    }
}

hi_void SD3403_VENC::stop_venc(hi_venc_chn venc_chn[], hi_u32 chn_num)
{
    hi_u32 i;

    venc_stop_get_stream(chn_num);

    for (i = 0; i < chn_num; i++)
    {
        venc_stop(venc_chn[i]);
    }
}

hi_s32 SD3403_VENC::venc_stop_get_stream(hi_s32 chn_num)
{
    hi_s32 i;
    for (i = 0; i < chn_num; i++)
    {
        if (hi_mpi_venc_stop_chn(i) != HI_SUCCESS)
        {
            ss_print("chn %d hi_mpi_venc_stop_recv_pic failed!\n", i);
            return HI_FAILURE;
        }
    }

    if (g_para.thread_start == HI_TRUE)
    {
        g_para.thread_start = HI_FALSE;
        // pthread_join(g_venc_pid, 0);
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }
    return HI_SUCCESS;
}

hi_s32 SD3403_VENC::venc_stop(hi_venc_chn venc_chn)
{
    hi_s32 ret;

    /* stop venc chn */
    ret = hi_mpi_venc_stop_chn(venc_chn);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_stop_chn vechn[%d] failed with %#x!\n", venc_chn, ret);
    }

    /* distroy venc channel */
    ret = hi_mpi_venc_destroy_chn(venc_chn);
    if (ret != HI_SUCCESS)
    {
        ss_print("hi_mpi_venc_destroy_chn vechn[%d] failed with %#x!\n", venc_chn, ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}
