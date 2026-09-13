#include "svp_npu.hpp"

/*INIT*/
int SVP_NNN::sd3403_svp_npu_acl_init(const td_char *config_path)
{
    svp_acl_error ret;
    uint32_t dev_count = 0;

    ret = svp_acl_init(config_path);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, TD_FAILURE, SVP_ERR_LEVEL_ERROR, "acl init failed!\n");
    svp_trace_info("init svp init success!\n");
    ret = svp_acl_rt_get_device_count(&dev_count);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, TD_FAILURE, SVP_ERR_LEVEL_ERROR, "acl get device count failed!\n");
    if (dev_count > 0)
    {
        ret = svp_acl_rt_set_device(g_svp_npu_dev_id);
        svp_check_exps_return(ret != SVP_ACL_SUCCESS, TD_FAILURE, SVP_ERR_LEVEL_ERROR, "acl set device failed!\n");
        if (ret != SVP_ACL_SUCCESS)
        {
            (td_void) svp_acl_finalize();
            svp_trace_err("svp acl open device %d failed!\n", g_svp_npu_dev_id);
            return TD_FAILURE;
        }
        svp_trace_info("set device %d success!\n", g_svp_npu_dev_id);
    }
    svp_acl_rt_run_mode run_mode;
    ret = svp_acl_rt_get_run_mode(&run_mode);
    if ((ret != SVP_ACL_SUCCESS) || (run_mode != SVP_ACL_DEVICE))
    {
        (td_void) svp_acl_rt_reset_device(g_svp_npu_dev_id);
        (td_void) svp_acl_finalize();
        svp_trace_err("acl get run mode failed!\n");
        return TD_FAILURE;
    }
    svp_trace_info("get run mode success!\n");
}

int SVP_NNN::sd3403_svp_npu_read_model(const td_char *model_path, hi_u32 model_index)
{
    svp_acl_error ret;
    FILE *fp = TD_NULL;

    fp = fopen(model_path, "rb");
    if (fp == nullptr)
    {
        svp_trace_err("open model error!\n");
        return -1;
    }
    ret = fseek(fp, 0L, SEEK_END);
    if (ret < 0)
    {
        svp_trace_err("fseek model error!\n");
        goto end_0;
    }
    g_svp_npu_model[model_index].model_mem_size = ftell(fp);
    if (g_svp_npu_model[model_index].model_mem_size <= 0)
    {
        svp_trace_err("ftell model error!\n");
        goto end_0;
    }
    ret = fseek(fp, 0L, SEEK_SET);
    if (ret < 0)
    {
        svp_trace_err("fseek model error!\n");
        goto end_0;
    }

    if (is_cached == TD_TRUE)
    {
        ret = svp_acl_rt_malloc_cached(&g_svp_npu_model[model_index].model_mem_ptr, g_svp_npu_model[model_index].model_mem_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    else
    {
        ret = svp_acl_rt_malloc(&g_svp_npu_model[model_index].model_mem_ptr, g_svp_npu_model[model_index].model_mem_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("acl malloc error!\n");
        goto end_0;
    }

    ret = fread(g_svp_npu_model[model_index].model_mem_ptr, g_svp_npu_model[model_index].model_mem_size, 1, fp);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("read model to mem error!\n");
        goto end_1;
    }
    if (is_cached == TD_TRUE)
    {
        ret = svp_acl_rt_mem_flush(g_svp_npu_model[model_index].model_mem_ptr, g_svp_npu_model[model_index].model_mem_size);
        if (ret != SVP_ACL_SUCCESS)
        {
            svp_trace_err("mem flush error!\n");
            goto end_1;
        }
    }
    (td_void) fclose(fp);
    return TD_SUCCESS;

end_1:
    (td_void) svp_acl_rt_free(g_svp_npu_model[model_index].model_mem_ptr);
end_0:
    (td_void) fclose(fp);
    return TD_FAILURE;
}

hi_s32 SVP_NNN::sd3403_svp_npu_create_desc(hi_u32 model_index)
{
    svp_acl_error ret;

    g_svp_npu_model[model_index].model_desc = svp_acl_mdl_create_desc();
    svp_check_exps_return(g_svp_npu_model[model_index].model_desc == HI_NULL, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "create model description failed!\n");

    ret = svp_acl_mdl_get_desc(g_svp_npu_model[model_index].model_desc, g_svp_npu_model[model_index].model_id);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "get model description failed, error code is %d!\n", ret);

    svp_trace_info("create model description success!\n");

    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_svp_npu_get_model_base_info(hi_u32 model_index)
{
    svp_acl_error ret;

    g_svp_npu_model[model_index].input_num = svp_acl_mdl_get_num_inputs(g_svp_npu_model[model_index].model_desc);
    svp_check_exps_return(g_svp_npu_model[model_index].input_num < SVP_NPU_EXTRA_INPUT_NUM + 1,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get input num failed!\n");

    g_svp_npu_model[model_index].output_num = svp_acl_mdl_get_num_outputs(g_svp_npu_model[model_index].model_desc);
    svp_check_exps_return(g_svp_npu_model[model_index].output_num < 1,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get output num failed!\n");

    ret = svp_acl_mdl_get_input_index_by_name(g_svp_npu_model[model_index].model_desc,
                                              SVP_ACL_DYNAMIC_TENSOR_NAME, &g_svp_npu_model[model_index].dynamic_batch_idx);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get dynamic batch idx failed, model id is %u, error code is %d!\n", model_index, ret);

    return HI_SUCCESS;
}
/*INIT*/
int SVP_NNN::sd3403_svp_npu_load_model(const td_char *model_path, hi_u32 model_index)
{
    td_s32 ret = 0;

    ret = sd3403_svp_npu_read_model(model_path, model_index);
    if (ret != TD_SUCCESS)
    {
        svp_trace_err("svp npu read model error!\n");
        return TD_FAILURE;
    }
    ret = svp_acl_mdl_load_from_mem(g_svp_npu_model[model_index].model_mem_ptr, g_svp_npu_model[model_index].model_mem_size,
                                    &g_svp_npu_model[model_index].model_id);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("load from mem error!\n");
        goto end_0;
    }

    ret = sd3403_svp_npu_create_desc(model_index);
    svp_check_exps_goto(ret != SVP_ACL_SUCCESS, end_0, SVP_ERR_LEVEL_ERROR, "create desc failed, model file is %s!\n", model_path);

    svp_trace_info("create model description success!\n");

    ret = sd3403_svp_npu_get_model_base_info(model_index);
    svp_check_exps_goto(ret != SVP_ACL_SUCCESS, end_1,
                        SVP_ERR_LEVEL_ERROR, "get model base info failed, model file is %s!\n", model_path);
    svp_trace_info("load mem_size:%lu, id:%d!\n", g_svp_npu_model[model_index].model_mem_size,
                   g_svp_npu_model[model_index].model_id);

    g_svp_npu_model[model_index].is_load_flag = HI_TRUE;
    svp_trace_info("load model %s success!\n", model_path);

end_1:
    (td_void) svp_acl_mdl_destroy_desc(g_svp_npu_model[model_index].model_desc);
    g_svp_npu_model[model_index].model_desc = TD_NULL;
end_0:
    (td_void) svp_acl_rt_free(g_svp_npu_model[model_index].model_mem_ptr);
    g_svp_npu_model[model_index].model_mem_ptr = TD_NULL;
    g_svp_npu_model[model_index].model_mem_size = 0;
    return HI_FAILURE;
}
/*INIT*/
td_s32 SVP_NNN::sd3403_svp_npu_acl_init_task(td_bool is_share_work_buf, td_u32 shared_work_buf_idx)
{
    hi_u32 task_idx;
    hi_s32 ret;
    hi_bool has_aicpu_task = HI_FALSE;

    if (is_share_work_buf == HI_TRUE)
    {
        ret = svp_npu_acl_create_shared_work_buf(shared_work_buf_idx);
        svp_check_exps_return(ret != HI_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create shared work buf failed!\n");
    }
    for (task_idx = 0; task_idx < task_num; task_idx++)
    {
        ret = sd3403_svp_npu_acl_dataset_init(task_idx);
        if (ret != HI_SUCCESS)
        {
            goto task_init_end_0;
        }
        ret = sd3403_svp_npu_create_task_buf(&g_svp_npu_task[task_idx]);
        if (ret != HI_SUCCESS)
        {
            svp_trace_err("create task buf failed.\n");
            goto task_init_end_0;
        }
        if (is_share_work_buf == HI_FALSE)
        {
            ret = sd3403_svp_npu_create_work_buf(&g_svp_npu_task[task_idx]);
        }
        else
        {
            /* if all tasks are on the same stream, work buf can be shared */
            ret = sd3403_svp_npu_share_work_buf(&g_svp_npu_shared_work_buf[shared_work_buf_idx],
                                                &g_svp_npu_task[task_idx]);
        }
        if (ret != HI_SUCCESS)
        {
            svp_trace_err("create work buf failed.\n");
            goto task_init_end_0;
        }
        // 创建aicpu线程
        if (g_svp_npu_aicpu_process_signal == HI_FALSE)
        {
            ret = sd3403_svp_npu_check_has_aicpu_task(&g_svp_npu_task[task_idx], &has_aicpu_task);
            if (ret != HI_SUCCESS)
            {
                svp_trace_err("check has aicpu task failed.\n");
                goto task_init_end_0;
            }
            if (has_aicpu_task == HI_TRUE)
            {
                g_svp_npu_aicpu_process_signal = HI_TRUE;
                // ret = pthread_create(&g_svp_npu_aicpu_thread, 0, sd3403_svp_npu_acl_aicpu_thread, HI_NULL);
                try
                {
                    aicpu_thread = std::thread(&SVP_NNN::sd3403_svp_npu_acl_aicpu_thread, this);
                }
                catch (const std::system_error &e)
                {
                    svp_trace_err("create aicpu task thread failed.\n");
                    goto task_init_end_0;
                }
            }
        }
    }
    return HI_SUCCESS;
task_init_end_0:
    (hi_void) sd3403_svp_npu_acl_deinit_task(task_num, shared_work_buf_idx);
    return ret;
}

hi_void SVP_NNN::sd3403_svp_npu_acl_deinit_task(hi_u32 task_num, hi_u32 shared_work_buf_idx)
{
    hi_u32 task_idx;

    if (g_svp_npu_aicpu_process_signal == HI_TRUE)
    {
        g_svp_npu_aicpu_process_signal = HI_FALSE;
        if (aicpu_thread.joinable())
        {
            aicpu_thread.join();
        }
    }

    for (task_idx = 0; task_idx < task_num; task_idx++)
    {
        (hi_void) sd3403_svp_npu_destroy_work_buf(&g_svp_npu_task[task_idx]);
        (hi_void) sd3403_svp_npu_destroy_task_buf(&g_svp_npu_task[task_idx]);
        (hi_void) sd3403_svp_npu_acl_dataset_deinit(task_idx);
        (hi_void) memset_s(&g_svp_npu_task[task_idx], sizeof(svp_npu_task_cfg), 0,
                           sizeof(svp_npu_task_cfg));
    }
    if (g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr != HI_NULL)
    {
        (hi_void) svp_acl_rt_free(g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr);
        g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr = HI_NULL;
        g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_size = 0;
        g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_stride = 0;
    }
}

hi_void SVP_NNN::sd3403_svp_npu_destroy_work_buf(svp_npu_task_info *task)
{
    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->work_buf_ptr == HI_NULL)
    {
        return;
    }
    (hi_void) svp_acl_rt_free(task->work_buf_ptr);
    task->work_buf_ptr = HI_NULL;
    task->work_buf_stride = 0;
    task->work_buf_size = 0;
}

hi_void SVP_NNN::sd3403_svp_npu_destroy_task_buf(svp_npu_task_info *task)
{
    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->task_buf_ptr == HI_NULL)
    {
        return;
    }
    (hi_void) svp_acl_rt_free(task->task_buf_ptr);
    task->task_buf_ptr = HI_NULL;
    task->task_buf_stride = 0;
    task->task_buf_size = 0;
}

hi_void SVP_NNN::sd3403_svp_npu_acl_dataset_deinit(hi_u32 task_idx)
{
    (hi_void) sd3403_svp_npu_destroy_input(&g_svp_npu_task[task_idx]);
    (hi_void) sd3403_svp_npu_destroy_output(&g_svp_npu_task[task_idx]);
}

hi_s32 SVP_NNN::sd3403_svp_npu_check_has_aicpu_task(const svp_npu_task_info *task, hi_bool *has_aicpu_task)
{
    hi_u32 aicpu_task_num;
    svp_acl_error ret;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(has_aicpu_task == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "has_aicpu_task is NULL!\n");

    ret = svp_acl_ext_get_mdl_aicpu_task_num(g_svp_npu_model[task->cfg.model_idx].model_id, &aicpu_task_num);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get aicpu task num failed, error code is %d!\n", ret);

    *has_aicpu_task = (aicpu_task_num == 0) ? HI_FALSE : HI_TRUE;
    return HI_SUCCESS;
}

hi_void *SVP_NNN::sd3403_svp_npu_acl_aicpu_thread(hi_void)
{
    svp_acl_error ret;

    while (g_svp_npu_aicpu_process_signal == HI_TRUE)
    {
        ret = svp_acl_ext_process_aicpu_task(SD3403_SVP_NPU_AICPU_WAIT_TIME);
        if (ret != SVP_ACL_SUCCESS && ret != SVP_ACL_ERROR_RT_REPORT_TIMEOUT)
        {
            svp_trace_err("aicpu porcess failed\n");
            break;
        }
    }
    return HI_NULL;
}

hi_s32 SVP_NNN::sd3403_svp_npu_share_work_buf(const svp_npu_shared_work_buf *shared_work_buf, const svp_npu_task_info *task)
{
    svp_acl_error ret;
    svp_acl_data_buffer *work_buf = HI_NULL;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }

    svp_check_exps_return(shared_work_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "shared_work_buf is NULL!\n");

    svp_check_exps_return(shared_work_buf->work_buf_ptr == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "work_buf_ptr is NULL!\n");

    svp_check_exps_return(task->work_buf_ptr != HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "task has created work buf!\n");

    work_buf = svp_acl_create_data_buffer(shared_work_buf->work_buf_ptr, shared_work_buf->work_buf_size,
                                          shared_work_buf->work_buf_stride);
    svp_check_exps_return(work_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "create work buf failed!\n");

    ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, work_buf);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("add work buf failed!\n");
        (hi_void) svp_acl_destroy_data_buffer(work_buf);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_svp_npu_create_work_buf(svp_npu_task_info *task)
{
    size_t num;
    svp_acl_data_buffer *work_buf = HI_NULL;
    svp_acl_error ret;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }

    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "input_dataset is NULL!\n");

    num = svp_acl_mdl_get_dataset_num_buffers(task->input_dataset);
    svp_check_exps_return(num != g_svp_npu_model[task->cfg.model_idx].input_num - 1,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "num of data buffer(%lu) should be %lu when create work buf!\n",
                          num, g_svp_npu_model[task->cfg.model_idx].input_num - 1);

    work_buf = sd3403_svp_npu_create_input_data_buffer(task, g_svp_npu_model[task->cfg.model_idx].input_num - 1);
    svp_check_exps_return(work_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "create work buf failed!\n");

    ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, work_buf);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("add work buf failed!\n");
        (hi_void) svp_npu_destroy_data_buffer(work_buf);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_svp_npu_create_task_buf(svp_npu_task_info *task)
{
    size_t num;
    svp_acl_data_buffer *task_buf = HI_NULL;
    svp_acl_error ret;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "input_dataset is NULL!\n");

    num = svp_acl_mdl_get_dataset_num_buffers(task->input_dataset);
    svp_check_exps_return(num != g_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "num of data buffer(%lu) should be %lu when create task buf!\n",
                          num, g_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM);

    task_buf = sd3403_svp_npu_create_input_data_buffer(task, g_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM);
    svp_check_exps_return(task_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create task buf failed!\n");

    ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, task_buf);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("add task buf failed!\n");
        (hi_void) svp_npu_destroy_data_buffer(task_buf);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

svp_acl_data_buffer *SVP_NNN::sd3403_svp_npu_create_input_data_buffer(svp_npu_task_info *task, hi_u32 idx)
{
    size_t buffer_size, stride;
    hi_void *input_buffer = HI_NULL;
    svp_acl_data_buffer *input_data = HI_NULL;

    stride = svp_acl_mdl_get_input_default_stride(g_svp_npu_model[task->cfg.model_idx].model_desc, idx);
    svp_check_exps_return(stride == 0, input_data, SVP_ERR_LEVEL_ERROR,
                          "get %u-th input stride failed!\n", idx);

    buffer_size = svp_acl_mdl_get_input_size_by_index(g_svp_npu_model[task->cfg.model_idx].model_desc, idx) *
                  (hi_u64)task->cfg.max_batch_num;
    svp_check_exps_return((buffer_size == 0 || buffer_size > SVP_NPU_MAX_MEM_SIZE), input_data,
                          SVP_ERR_LEVEL_ERROR, "buffer_size(%lu) can't be 0 and should be less than %u!\n",
                          buffer_size, SVP_NPU_MAX_MEM_SIZE);

    if (sd3403_svp_npu_malloc_mem(&input_buffer, (hi_u32)buffer_size, task->cfg.is_cached) != HI_SUCCESS)
    {
        svp_trace_err("%u-th input malloc mem failed!\n", idx);
        return input_data;
    }

    input_data = svp_acl_create_data_buffer(input_buffer, buffer_size, stride);
    if (input_data == HI_NULL)
    {
        svp_trace_err("can't create %u-th input data buffer!\n", idx);
        (hi_void) svp_acl_rt_free(input_buffer);
        return input_data;
    }
    if (idx == g_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM)
    {
        task->task_buf_ptr = input_buffer;
        task->task_buf_size = buffer_size;
        task->task_buf_stride = stride;
    }
    else if (idx == g_svp_npu_model[task->cfg.model_idx].input_num - 1)
    {
        task->work_buf_ptr = input_buffer;
        task->work_buf_size = buffer_size;
        task->work_buf_stride = stride;
    }
    return input_data;
}

hi_s32 SVP_NNN::sd3403_svp_npu_malloc_mem(hi_void **buffer, hi_u32 buffer_size, hi_bool is_cached)
{
    svp_acl_error ret;

    if (is_cached == HI_TRUE)
    {
        ret = svp_acl_rt_malloc_cached(buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    else
    {
        ret = svp_acl_rt_malloc(buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "can't malloc buffer, size is %u, error code is %d!\n", buffer_size, ret);

    (hi_void) memset_s(*buffer, buffer_size, 0, buffer_size);
    if (is_cached == HI_TRUE)
    {
        (hi_void) svp_acl_rt_mem_flush(*buffer, buffer_size);
    }
    return ret;
}

hi_s32 SVP_NNN::sd3403_svp_npu_acl_dataset_init(hi_u32 task_idx)
{
    hi_s32 ret = sd3403_svp_npu_create_input(&g_svp_npu_task[task_idx]);
    if (ret != HI_SUCCESS)
    {
        svp_trace_err("svp npu create input error!\n");
        return HI_FAILURE;
    }
    ret = sd3403_svp_npu_create_output(&g_svp_npu_task[task_idx]);
    if (ret != HI_SUCCESS)
    {
        svp_trace_err("svp npu create output error!\n");
        sd3403_svp_npu_destroy_input(&g_svp_npu_task[task_idx]);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_void SVP_NNN::sd3403_svp_npu_destroy_output(svp_npu_task_info *task)
{
    hi_u32 i;
    size_t output_num;
    svp_acl_data_buffer *data_buffer = HI_NULL;
    hi_void *data = HI_NULL;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->output_dataset == HI_NULL)
    {
        return;
    }

    output_num = svp_acl_mdl_get_dataset_num_buffers(task->output_dataset);

    for (i = 0; i < output_num; i++)
    {
        data_buffer = svp_acl_mdl_get_dataset_buffer(task->output_dataset, i);
        data = svp_acl_get_data_buffer_addr(data_buffer);
        (hi_void) svp_acl_rt_free(data);
        (hi_void) svp_acl_destroy_data_buffer(data_buffer);
    }

    (hi_void) svp_acl_mdl_destroy_dataset(task->output_dataset);
    task->output_dataset = HI_NULL;
}

hi_s32 SVP_NNN::sd3403_svp_npu_create_output(svp_npu_task_info *task)
{
    svp_acl_error ret;
    hi_u32 i;
    svp_acl_data_buffer *output_data = HI_NULL;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    task->output_dataset = svp_acl_mdl_create_dataset();
    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create output dataset failed!\n");
    for (i = 0; i < g_svp_npu_model[task->cfg.model_idx].output_num; i++)
    {
        size_t buffer_size, stride;
        hi_void *output_buffer = HI_NULL;
        svp_acl_data_buffer *output_data = HI_NULL;

        stride = svp_acl_mdl_get_output_default_stride(g_svp_npu_model[task->cfg.model_idx].model_desc, i);
        svp_check_exps_return(stride == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get %u-th output stride failed!\n", i);

        buffer_size = svp_acl_mdl_get_output_size_by_index(g_svp_npu_model[task->cfg.model_idx].model_desc, i) * (hi_u64)task->cfg.max_batch_num;
        svp_check_exps_return((buffer_size == 0 || buffer_size > SVP_NPU_MAX_MEM_SIZE), HI_FAILURE,
                              SVP_ERR_LEVEL_ERROR, "buffer_size(%lu) can't be 0 and should be less than %u!\n",
                              buffer_size, SVP_NPU_MAX_MEM_SIZE);

        // malloc mem
        if (sd3403_svp_npu_malloc_mem(&output_buffer, buffer_size, task->cfg.is_cached) != HI_SUCCESS)
        {
            svp_trace_err("%u-th output malloc mem failed!\n", i);
            return HI_FAILURE;
        }

        output_data = svp_acl_create_data_buffer(output_buffer, buffer_size, stride);
        if (output_data == HI_NULL)
        {
            svp_trace_err("can't create %u-th output data buffer!\n", i);
            (hi_void) svp_acl_rt_free(output_buffer);
            return HI_FAILURE;
        }

        ret = svp_acl_mdl_add_dataset_buffer(task->output_dataset, output_data);
        if (ret != SVP_ACL_SUCCESS)
        {
            svp_trace_err("add %u-th output data buffer failed!\n", i);
            (hi_void) svp_npu_destroy_data_buffer(output_data);
            (hi_void) sd3403_svp_npu_destroy_output(task);
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_svp_npu_create_input(svp_npu_task_info *task)
{
    svp_acl_error ret;
    hi_u32 i;
    svp_acl_data_buffer *input_data = HI_NULL;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    task->input_dataset = svp_acl_mdl_create_dataset();
    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create input dataset failed!\n");
    for (i = 0; i < g_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM; i++)
    {
        input_data = sd3403_svp_npu_create_input_data_buffer(task, i);
        if (input_data == HI_NULL)
        {
            svp_trace_err("create %u-th input data buffer failed!\n", i);
            (hi_void) sd3403_svp_npu_destroy_input(task);
            return HI_FAILURE;
        }

        ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, input_data);
        if (ret != SVP_ACL_SUCCESS)
        {
            svp_trace_err("add %u-th input data buffer failed!\n", i);
            (hi_void) svp_npu_destroy_data_buffer(input_data);
            (hi_void) sd3403_svp_npu_destroy_input(task);
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

hi_void SVP_NNN::svp_npu_destroy_data_buffer(svp_acl_data_buffer *input_data)
{
    hi_void *data = svp_acl_get_data_buffer_addr(input_data);
    (hi_void) svp_acl_rt_free(data);
    (hi_void) svp_acl_destroy_data_buffer(input_data);
}

hi_void SVP_NNN::sd3403_svp_npu_destroy_input(svp_npu_task_info *task)
{
    hi_u32 i;
    size_t input_num;
    svp_acl_data_buffer *data_buffer = HI_NULL;
    hi_void *data = HI_NULL;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->input_dataset == HI_NULL)
    {
        return;
    }

    input_num = svp_acl_mdl_get_dataset_num_buffers(task->input_dataset);

    for (i = 0; i < input_num; i++)
    {
        data_buffer = svp_acl_mdl_get_dataset_buffer(task->input_dataset, i);
        if (i < input_num - SVP_NPU_EXTRA_INPUT_NUM)
        {
            data = svp_acl_get_data_buffer_addr(data_buffer);
            (hi_void) svp_acl_rt_free(data);
        }
        (hi_void) svp_acl_destroy_data_buffer(data_buffer);
    }

    (hi_void) svp_acl_mdl_destroy_dataset(task->input_dataset);
    task->input_dataset = HI_NULL;
}

hi_s32 SVP_NNN::svp_npu_acl_create_shared_work_buf(td_u32 shared_work_buf_idx)
{
    hi_u32 task_idx, work_buf_size, work_buf_stride;
    hi_s32 ret;

    for (task_idx = 0; task_idx < task_num; task_idx++)
    {
        ret = svp_npu_get_work_buf_info(&g_svp_npu_task[task_idx], &work_buf_size, &work_buf_stride);
        if (ret != SVP_ACL_SUCCESS)
        {
            svp_trace_err("task %d get work info error!\n");
            return TD_FAILURE;
        }
        if (g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_size < work_buf_size)
        {
            g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_size = work_buf_size;
            g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_stride = work_buf_stride;
        }
    }
    ret = svp_acl_rt_malloc_cached(&g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr,
                                   g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != HI_SUCCESS)
    {
        svp_trace_err("acl rt malloc error!\n");
        return TD_FAILURE;
    }

    (hi_void) svp_acl_rt_mem_flush(g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr,
                                   g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_size);
    return HI_SUCCESS;
}

td_s32 SVP_NNN::svp_npu_get_work_buf_info(const svp_npu_task_info *task, td_u32 *work_buf_size, td_u32 *work_buf_stride)
{
    if (sd3403_svp_check_task_cfg(task) != TD_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return TD_FAILURE;
    }
    *work_buf_stride = (td_u32)svp_acl_mdl_get_input_default_stride(g_svp_npu_model[task->cfg.model_idx].model_desc,
                                                                    g_svp_npu_model[task->cfg.model_idx].input_num - 1);
    if (*work_buf_stride == 0)
    {
        svp_trace_err("get work buf stride failed!\n");
        return TD_FAILURE;
    }

    *work_buf_size = (td_u32)svp_acl_mdl_get_input_size_by_index(g_svp_npu_model[task->cfg.model_idx].model_desc,
                                                                 g_svp_npu_model[task->cfg.model_idx].input_num - 1);
    if (*work_buf_size == 0)
    {
        svp_trace_err("get work buf size failed!\n");
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

td_s32 SVP_NNN::sd3403_svp_check_task_cfg(const svp_npu_task_info *task)
{
    svp_check_exps_return(task == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "task is NULL!\n");

    svp_check_exps_return(task->cfg.max_batch_num == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "max_batch_num(%u) is 0!\n", task->cfg.max_batch_num);

    svp_check_exps_return(task->cfg.dynamic_batch_num == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "dynamic_batch_num(%u) is 0!\n", task->cfg.dynamic_batch_num);

    svp_check_exps_return(task->cfg.total_t != 0 && task->cfg.dynamic_batch_num != 1, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "dynamic_batch_num(%u) should be 1 when total_t(%u) is not 0!\n",
                          task->cfg.dynamic_batch_num, task->cfg.total_t);

    svp_check_exps_return((task->cfg.is_cached != HI_TRUE && task->cfg.is_cached != HI_FALSE), HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "is_cached(%u) should be [%u, %u]!\n", task->cfg.is_cached, HI_FALSE, HI_TRUE);

    svp_check_exps_return(task->cfg.model_idx >= SD3403_SVP_NPU_MAX_MODEL_NUM, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "model_idx(%u) should be less than %u!\n",
                          task->cfg.model_idx, SD3403_SVP_NPU_MAX_MODEL_NUM);

    svp_check_exps_return(g_svp_npu_model[task->cfg.model_idx].model_desc == HI_NULL, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "%u-th model_desc is NULL!\n", task->cfg.model_idx);
    return HI_SUCCESS;
}

int SVP_NNN::sd3403_svp_npu_acl_deinit(void)
{
    svp_acl_error ret;

    ret = svp_acl_rt_reset_device(g_svp_npu_dev_id);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("reset device fail\n");
    }
    svp_trace_info("end to reset device is %d\n", g_svp_npu_dev_id);
    ret = svp_acl_finalize();
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("finalize acl fail\n");
    }
    svp_trace_info("end to finalize acl\n");
}
/*INIT*/
hi_s32 SVP_NNN::sd3403_svp_npu_get_input_data(const hi_char *src, hi_u32 num, const svp_npu_task_info *task)
{
    hi_s32 ret;
    hi_u32 i, line, total_line_num, line_byte_num;
    hi_char path[SD3403_PATH_MAX] = {0};
    size_t stride, size, input_num;
    FILE *fp = HI_NULL;
    int8_t *data = HI_NULL;
    svp_acl_data_buffer *data_buffer = HI_NULL;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(src == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "src is NULL!\n");
    input_num = g_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM;
    svp_check_exps_return(input_num != num, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "input file num(%u) should be equal to %lu!\n", num, input_num);
    for (i = 0; i < num; i++)
    {
        ret = sd3403_svp_npu_get_line_num_and_line_byte_num(task, i, HI_TRUE, &total_line_num, &line_byte_num);
        svp_check_exps_return(ret != HI_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get line num failed!\n");
        data_buffer = svp_acl_mdl_get_dataset_buffer(task->input_dataset, i);
        svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get data buffer NULL!\n");
        data = (int8_t *)svp_acl_get_data_buffer_addr(data_buffer);
        svp_check_exps_return(data == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get data addr NULL!\n");
        stride = svp_acl_get_data_buffer_stride(data_buffer);
        svp_check_exps_return(stride == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get data stride failed!\n");

        size = svp_acl_get_data_buffer_size(data_buffer);
        if (size < (hi_u64)task->cfg.dynamic_batch_num * total_line_num * stride)
        {
            printf("get buffer size error!\n");
            return HI_FAILURE;
        }
        for (line = 0; line < task->cfg.dynamic_batch_num * total_line_num; line++)
        {
            memcpy(data + line * stride, src + line * stride, line_byte_num);
            // return HI_FAILURE;
        }
        if (task->cfg.is_cached == HI_TRUE)
        {
            (hi_void) svp_acl_rt_mem_flush((const void *)data, task->cfg.dynamic_batch_num * total_line_num * stride);
        }
    }
    return HI_SUCCESS;
}
/*INIT*/
hi_s32 SVP_NNN::sd3403_svp_npu_model_execute(const svp_npu_task_info *task)
{
    svp_acl_data_buffer *data_buffer = HI_NULL;
    hi_void *data = HI_NULL;
    size_t size;
    hi_u32 i;
    svp_acl_error ret;

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }

    ret = svp_acl_mdl_execute(g_svp_npu_model[task->cfg.model_idx].model_id, task->input_dataset, task->output_dataset);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "svp_acl_mdl_execute failed, model_id is %u, error code is %d!\n",
                          g_svp_npu_model[task->cfg.model_idx].model_id, ret);

    if (task->cfg.is_cached == HI_TRUE)
    {
        for (i = 0; i < g_svp_npu_model[task->cfg.model_idx].output_num; i++)
        {
            data_buffer = svp_acl_mdl_get_dataset_buffer(task->output_dataset, i);
            svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                                  "get %u-th output data_buffer is NULL!\n", i);

            data = svp_acl_get_data_buffer_addr(data_buffer);
            svp_check_exps_return(data == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                                  "get %u-th output data is NULL!\n", i);

            size = svp_acl_get_data_buffer_size(data_buffer) / task->cfg.max_batch_num * task->cfg.dynamic_batch_num;
            svp_check_exps_return(size == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                                  "get %u-th output data size is 0!\n", i);

            ret = svp_acl_rt_mem_flush(data, size);
            svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                                  "flush %u-th output data failed, error code is %d!\n", i, ret);
        }
    }
    return ret;
}

hi_s32 SVP_NNN::sd3403_svp_npu_get_line_num_and_line_byte_num(const svp_npu_task_info *task, hi_u32 idx,
                                                              hi_bool is_input, hi_u32 *total_line_num, hi_u32 *line_byte_num)
{
    hi_s32 ret;
    hi_u32 i;
    svp_acl_mdl_io_dims dims;
    svp_acl_data_type data_type;
    size_t data_size;

    if (is_input == HI_TRUE)
    {
        ret = svp_acl_mdl_get_input_dims(g_svp_npu_model[task->cfg.model_idx].model_desc, idx, &dims);
    }
    else
    {
        ret = svp_acl_mdl_get_output_dims(g_svp_npu_model[task->cfg.model_idx].model_desc, idx, &dims);
    }
    svp_check_exps_return(ret != HI_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get %u-th input/output dims failed!\n", idx);

    if (is_input == HI_TRUE)
    {
        data_type = svp_acl_mdl_get_input_data_type(g_svp_npu_model[task->cfg.model_idx].model_desc, idx);
    }
    else
    {
        data_type = svp_acl_mdl_get_output_data_type(g_svp_npu_model[task->cfg.model_idx].model_desc, idx);
    }
    svp_check_exps_return(ret != HI_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get %u-th input/output data type failed!\n", idx);

    data_size = svp_acl_data_type_size(data_type);
    svp_check_exps_return(data_size == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get data size failed!\n");

    *line_byte_num = dims.dims[dims.dim_count - 1] *
                     ((data_size + SD3403_SVP_NPU_BYTE_BIT_NUM - 1) / SD3403_SVP_NPU_BYTE_BIT_NUM);

    *total_line_num = 1;
    for (i = 0; i < dims.dim_count - 1; i++)
    {
        *total_line_num *= dims.dims[i];
    }
    /* lstm xt line num */
    if ((task->cfg.total_t != 0) && (idx == 0))
    {
        svp_check_exps_return(task->cfg.total_t > dims.dims[0], HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                              "total t(%u) can't be greater than max total t(%ld)!\n", task->cfg.total_t, dims.dims[0]);
        *total_line_num /= dims.dims[0];
        *total_line_num *= task->cfg.total_t;
    }
    return HI_SUCCESS;
}

hi_void SVP_NNN::sd3403_svp_npu_sort_output_result(const hi_float *src, hi_u32 src_len,
                                                   sd3403_svp_npu_top_n_result *dst, hi_u32 dst_len)
{
    hi_u32 i, j, index;
    hi_bool charge;

    printf("====>src_len : %d\n", src_len);
    for (i = 0; i < src_len; i++)
    {
        charge = HI_FALSE;

        for (j = 0; j < dst_len; j++)
        {
            // printf("===>i:%d, j : %d, src : %f\n", i, j, src[i]);
            if (src[i] > dst[j].score)
            {
                index = j;
                charge = HI_TRUE;
                break;
            }
        }

        if (charge == HI_TRUE)
        {
            for (j = dst_len - 1; j > index; j--)
            {
                dst[j].score = dst[j - 1].score;
                dst[j].class_id = dst[j - 1].class_id;
            }
            dst[index].score = src[i];
            dst[index].class_id = i;
        }
    }
}

hi_void SVP_NNN::sd3403_svp_npu_output_classification_result(const svp_npu_task_info *task, std::vector<sd3403_svp_npu_top_n_result> &output)
{
    svp_acl_data_buffer *data_buffer = HI_NULL;
    int8_t *data = HI_NULL;
    hi_u32 i, j, n;
    svp_acl_error ret;
    size_t stride;
    svp_acl_mdl_io_dims dims;
    sd3403_svp_npu_top_n_result top[SD3403_SVP_NPU_SHOW_TOP_NUM] = {0};

    if (sd3403_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    svp_check_exps_return_void(task->output_dataset == HI_NULL, SVP_ERR_LEVEL_ERROR,
                               "task->output_dataset is NULL!\n");

    printf("==========================> output num : %d\n", svp_acl_mdl_get_dataset_num_buffers(task->output_dataset));
    for (i = 0; i < svp_acl_mdl_get_dataset_num_buffers(task->output_dataset); i++)
    {
        data_buffer = svp_acl_mdl_get_dataset_buffer(task->output_dataset, i);
        svp_check_exps_return_void(data_buffer == HI_NULL, SVP_ERR_LEVEL_ERROR,
                                   "get %u-th data buffer is NULL!\n", i);

        data = (int8_t *)svp_acl_get_data_buffer_addr(data_buffer);
        svp_check_exps_return_void(data == HI_NULL, SVP_ERR_LEVEL_ERROR,
                                   "get %u-th data addr is NULL!\n", i);

        stride = svp_acl_get_data_buffer_stride(data_buffer);
        svp_check_exps_return_void(data == HI_NULL, SVP_ERR_LEVEL_ERROR,
                                   "get %u-th data stride is 0!\n", i);
        printf("===>stride : %d\n", stride);

        ret = svp_acl_mdl_get_output_dims(g_svp_npu_model[task->cfg.model_idx].model_desc, i, &dims);
        svp_check_exps_return_void(data == HI_NULL, SVP_ERR_LEVEL_ERROR,
                                   "get %u-th output dims failed, error code is %d!\n", i, ret);

        svp_trace_info("dims count : %d, name : %s\n", dims.dim_count, dims.name);
        for (int k = 0; k < dims.dim_count; k++)
        {
            svp_trace_info("k : %d, dims.dims[i] : %d\n", k, dims.dims[k]);
        }

        printf("===>task->cfg.dynamic_batch_num : %d\n", task->cfg.dynamic_batch_num);
        for (n = 0; n < task->cfg.dynamic_batch_num; n++)
        {
            (hi_void) sd3403_svp_npu_sort_output_result((const hi_float *)data, (hi_u32)dims.dims[dims.dim_count - 1],
                                                        top, SD3403_SVP_NPU_SHOW_TOP_NUM);
            svp_trace_info("%u-th batch result:\n", n);
            // 获取最高的置信度最高的五个类别
            for (j = 0; j < SD3403_SVP_NPU_SHOW_TOP_NUM; j++)
            {
                svp_trace_info("top %d: value[%lf], class_id[%u]!\n", j, top[j].score, top[j].class_id);
                output.push_back(top[j]);
            }
            data += stride;
            (hi_void) memset_s(top, sizeof(top), 0, sizeof(top));
        }
    }

    svp_trace_info("output data success!\n");
    return;
}
hi_void SVP_NNN::sd3403_svp_npu_unload_model(hi_u32 model_index)
{
    svp_acl_error ret;

    svp_check_exps_return_void(g_svp_npu_model[model_index].is_load_flag != HI_TRUE,
                               SVP_ERR_LEVEL_ERROR, "%u-th node has not loaded a model!\n", model_index);

    ret = svp_acl_mdl_unload(g_svp_npu_model[model_index].model_id);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("unload model failed, model_id is %u, error code is %d!\n",
                      g_svp_npu_model[model_index].model_id, ret);
    }

    if (g_svp_npu_model[model_index].model_desc != HI_NULL)
    {
        (hi_void) svp_acl_mdl_destroy_desc(g_svp_npu_model[model_index].model_desc);
        g_svp_npu_model[model_index].model_desc = HI_NULL;
    }

    if (g_svp_npu_model[model_index].model_mem_ptr != HI_NULL)
    {
        (hi_void) svp_acl_rt_free(g_svp_npu_model[model_index].model_mem_ptr);
        g_svp_npu_model[model_index].model_mem_ptr = HI_NULL;
        g_svp_npu_model[model_index].model_mem_size = 0;
    }

    g_svp_npu_model[model_index].is_load_flag = HI_FALSE;
    svp_trace_info("unload model SUCCESS, model id is %u!\n", g_svp_npu_model[model_index].model_id);
}

int SVP_NNN::Init_ACL()
{
    hi_s32 ret;

    ret = sd3403_svp_npu_acl_init("");
    svp_check_exps_return(ret != HI_SUCCESS, HI_FALSE, SVP_ERR_LEVEL_ERROR, "init failed!\n");
    return 0;
}
int SVP_NNN::Init_AI(const char *Det_VL_model)
{
    hi_s32 ret;
    int model_idx = 0;

    ret = sd3403_svp_npu_load_model(Det_VL_model, 0);
    svp_check_exps_goto(ret != HI_SUCCESS, process_end0, SVP_ERR_LEVEL_ERROR, "load model failed!\n");
    g_svp_npu_task[0].cfg.max_batch_num = 1;
    g_svp_npu_task[0].cfg.dynamic_batch_num = 1;
    g_svp_npu_task[0].cfg.total_t = 0;
    g_svp_npu_task[0].cfg.is_cached = HI_FALSE;
    g_svp_npu_task[0].cfg.model_idx = model_idx;

    ret = sd3403_svp_npu_acl_init_task(HI_FALSE, 0);
    svp_check_exps_goto(ret != HI_SUCCESS, process_end1, SVP_ERR_LEVEL_ERROR, "init task failed!\n");
    return 0;

process_end1:
    (hi_void) sd3403_svp_npu_unload_model(model_idx);
process_end0:
    (hi_void) sd3403_svp_npu_acl_deinit();

    return -1;
}
int SVP_NNN::Det(unsigned char *frame, std::vector<sd3403_svp_npu_top_n_result> &output)
{
    hi_s32 ret;
    int model_idx = 0;

    ret = sd3403_svp_npu_get_input_data((const hi_char *)frame, SD3403_SVP_NPU_RESNET50_INPUT_FILE_NUM, &g_svp_npu_task[0]);
    svp_check_exps_goto(ret != HI_SUCCESS, process_end2, SVP_ERR_LEVEL_ERROR, "get data failed!\n");
    ret = sd3403_svp_npu_model_execute(&g_svp_npu_task[0]);
    svp_check_exps_goto(ret != HI_SUCCESS, process_end2, SVP_ERR_LEVEL_ERROR, "execute failed!\n");
    (hi_void) sd3403_svp_npu_output_classification_result(&g_svp_npu_task[0], output);

    return 0;

process_end2:
    (hi_void) sd3403_svp_npu_acl_deinit_task(1, 0);
process_end1:
    (hi_void) sd3403_svp_npu_unload_model(model_idx);
process_end0:
    (hi_void) sd3403_svp_npu_acl_deinit();

    return -1;
}
int SVP_NNN::Uninit_AI()
{
}
int SVP_NNN::Uninit_ACL()
{
}

/*****
 * ****************************************************************************************
 * 目标检测模型加载代码
 * ****************************************************************************************
 */

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_acl_init(void)
{
    /* svp acl init */
    svp_acl_rt_run_mode run_mode;
    svp_acl_error ret;
    hi_bool is_mpi_init;

    ret = svp_acl_init("");
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "acl init failed!\n");

    /* open device */
    ret = svp_acl_rt_set_device(g_svp_npu_dev_id);
    if (ret != SVP_ACL_SUCCESS)
    {
        (hi_void) svp_acl_finalize();
        svp_trace_err("svp acl open device %d failed!\n", g_svp_npu_dev_id);
        return HI_FAILURE;
    }
    svp_trace_info("open device %d success!\n", g_svp_npu_dev_id);
    ret = svp_acl_rt_set_op_wait_timeout(0);
    if (ret != SVP_ACL_SUCCESS)
    {
        printf("acl set op wait time failed");
        return HI_FAILURE;
    }
    svp_trace_info("set op wait time success");

    ret = svp_acl_rt_create_context(&context_, g_svp_npu_dev_id);
    if (ret != SVP_ACL_SUCCESS)
    {
        printf("acl create context failed");
        return HI_FAILURE;
    }
    printf("create context success\n");

    // create stream
    ret = svp_acl_rt_create_stream(&stream_);
    if (ret != SVP_ACL_SUCCESS)
    {
        printf("acl create stream failed");
        return HI_FAILURE;
    }
    printf("=============================2=====stream : %p. context : %p===================\n", stream_, context_);
    /* get run mode */
    ret = svp_acl_rt_get_run_mode(&run_mode);
    if ((ret != SVP_ACL_SUCCESS) || (run_mode != SVP_ACL_DEVICE))
    {
        (hi_void) svp_acl_rt_reset_device(g_svp_npu_dev_id);
        (hi_void) svp_acl_finalize();
        svp_trace_err("acl get run mode failed!\n");
        return HI_FAILURE;
    }

    svp_trace_info("get run mode success!\n");

    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_read_model(const hi_char *model_path, hi_u32 model_index, hi_bool is_cached)
{
    FILE *fp = HI_NULL;
    hi_s32 ret;

    /* Get model file size */
    fp = fopen(model_path, "rb");
    svp_check_exps_return(fp == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "open model file failed, model file is %s!\n", model_path);

    ret = fseek(fp, 0L, SEEK_END);
    svp_check_exps_goto(ret == -1, end_0, SVP_ERR_LEVEL_ERROR, "fseek failed!\n");

    rfcn_svp_npu_model[model_index].model_mem_size = ftell(fp);
    svp_check_exps_goto(rfcn_svp_npu_model[model_index].model_mem_size <= 0, end_0,
                        SVP_ERR_LEVEL_ERROR, "ftell failed!\n");

    ret = fseek(fp, 0L, SEEK_SET);
    svp_check_exps_goto(ret == -1, end_0, SVP_ERR_LEVEL_ERROR, "fseek failed!\n");

    /* malloc model file mem */
    printf("===>rfcn_svp_npu_model[model_index].model_mem_size : %d\n", rfcn_svp_npu_model[model_index].model_mem_size);
    if (is_cached == HI_TRUE)
    {
        ret = svp_acl_rt_malloc_cached(&rfcn_svp_npu_model[model_index].model_mem_ptr,
                                       rfcn_svp_npu_model[model_index].model_mem_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    else
    {
        ret = svp_acl_rt_malloc(&rfcn_svp_npu_model[model_index].model_mem_ptr,
                                rfcn_svp_npu_model[model_index].model_mem_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    svp_check_exps_goto(ret != SVP_ACL_SUCCESS, end_0, SVP_ERR_LEVEL_ERROR,
                        "malloc mem failed, erroe code %d!\n", ret);

    ret = fread(rfcn_svp_npu_model[model_index].model_mem_ptr, rfcn_svp_npu_model[model_index].model_mem_size, 1, fp);
    svp_check_exps_goto(ret != 1, end_1, SVP_ERR_LEVEL_ERROR, "read model file failed!\n");

    if (is_cached == HI_TRUE)
    {
        ret = svp_acl_rt_mem_flush(rfcn_svp_npu_model[model_index].model_mem_ptr,
                                   rfcn_svp_npu_model[model_index].model_mem_size);
        svp_check_exps_goto(ret != SVP_ACL_SUCCESS, end_1, SVP_ERR_LEVEL_ERROR,
                            "flush mem failed!, error code is %d\n", ret);
    }
    (hi_void) fclose(fp);
    return HI_SUCCESS;
end_1:
    (hi_void) svp_acl_rt_free(rfcn_svp_npu_model[model_index].model_mem_ptr);
end_0:
    (hi_void) fclose(fp);
    return HI_FAILURE;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_load_model(const hi_char *model_path, hi_u32 model_index, hi_bool is_cached)
{
    hi_s32 ret;

    ret = sd3403_rfcn_svp_npu_read_model(model_path, model_index, is_cached);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "query model failed, model file is %s!\n", model_path);

    ret = svp_acl_mdl_load_from_mem(rfcn_svp_npu_model[model_index].model_mem_ptr,
                                    rfcn_svp_npu_model[model_index].model_mem_size, &rfcn_svp_npu_model[model_index].model_id);
    svp_check_exps_goto(ret != SVP_ACL_SUCCESS, end_0,
                        SVP_ERR_LEVEL_ERROR, "load model from mem failed, error code is %d!\n", ret);

    // create desc
    rfcn_svp_npu_model[model_index].model_desc = svp_acl_mdl_create_desc();
    svp_check_exps_return(rfcn_svp_npu_model[model_index].model_desc == HI_NULL, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "create model description failed!\n");

    ret = svp_acl_mdl_get_desc(rfcn_svp_npu_model[model_index].model_desc, rfcn_svp_npu_model[model_index].model_id);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "get model description failed, error code is %d!\n", ret);

    svp_trace_info("create model description success!\n");
    // 没这步
    ret = sd3403_rfcn_svp_npu_get_model_base_info(model_index);
    svp_check_exps_goto(ret != SVP_ACL_SUCCESS, end_1,
                        SVP_ERR_LEVEL_ERROR, "get model base info failed, model file is %s!\n", model_path);

    return SVP_ACL_SUCCESS;
end_1:
    (hi_void) svp_acl_mdl_destroy_desc(rfcn_svp_npu_model[model_index].model_desc);
    rfcn_svp_npu_model[model_index].model_desc = HI_NULL;
end_0:
    (hi_void) svp_acl_rt_free(rfcn_svp_npu_model[model_index].model_mem_ptr);
    rfcn_svp_npu_model[model_index].model_mem_ptr = HI_NULL;
    rfcn_svp_npu_model[model_index].model_mem_size = 0;
    return HI_FAILURE;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_get_model_base_info(hi_u32 model_index)
{
    svp_acl_error ret;

    rfcn_svp_npu_model[model_index].input_num = svp_acl_mdl_get_num_inputs(rfcn_svp_npu_model[model_index].model_desc);
    svp_check_exps_return(rfcn_svp_npu_model[model_index].input_num < SVP_NPU_EXTRA_INPUT_NUM + 1,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get input num failed!\n");

    rfcn_svp_npu_model[model_index].output_num = svp_acl_mdl_get_num_outputs(rfcn_svp_npu_model[model_index].model_desc);
    svp_check_exps_return(rfcn_svp_npu_model[model_index].output_num < 1,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get output num failed!\n");

    // ret = svp_acl_mdl_get_input_index_by_name(rfcn_svp_npu_model[model_index].model_desc,
    //                                           SVP_ACL_DYNAMIC_TENSOR_NAME, &rfcn_svp_npu_model[model_index].dynamic_batch_idx);
    // svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
    //                       "get dynamic batch idx failed, model id is %u, error code is %d!\n", model_index, ret);

    return HI_SUCCESS;
}

hi_s32 SVP_NNN::read_rpn_file(const std::string &fileName, std::vector<float> &detParas)
{
    struct stat sBuf;
    int fileStatus = stat(fileName.data(), &sBuf);
    if (fileStatus == -1)
    {
        printf("failed to get file %s", fileName.c_str());
        return HI_FAILURE;
    }
    if (S_ISREG(sBuf.st_mode) == 0)
    {
        printf("%s is not a file, please enter a file", fileName.c_str());
        return HI_FAILURE;
    }
    std::ifstream txtFile;
    txtFile.open(fileName);
    if (txtFile.is_open() == false)
    {
        printf("open file %s failed", fileName.c_str());
        return HI_FAILURE;
    }
    float c;
    while (!txtFile.eof())
    {
        if (!txtFile.good())
        {
            return HI_FAILURE;
        }
        txtFile >> c;
        detParas.push_back(c);
    }
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_set_threshold(sd3403_svp_npu_threshold threshold[], hi_u32 threshold_num, const svp_npu_task_info *task)
{
    hi_u32 i, n;
    svp_acl_error ret;
    svp_acl_data_buffer *data_buffer = HI_NULL;
    hi_float *data = HI_NULL;
    size_t idx, size;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(threshold == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "threshold is NULL!\n");
    svp_check_exps_return(threshold_num == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "threshold_num is 0!\n");

    data_buffer = svp_acl_mdl_get_dataset_buffer(task->input_dataset, 1);
    svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get roi data_buffer is NULL!\n");

    size = svp_acl_get_data_buffer_size(data_buffer);
    svp_check_exps_return(size < SD3403_SVP_NPU_THRESHOLD_NUM * sizeof(hi_float), HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "get size(%lu) is less than %lu!\n",
                          size, SD3403_SVP_NPU_THRESHOLD_NUM * sizeof(hi_float));

    data = (hi_float *)svp_acl_get_data_buffer_addr(data_buffer);
    svp_check_exps_return(data == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get roi data is NULL!\n");
    n = 0;
    data[n++] = threshold[0].nms_threshold;
    data[n++] = threshold[0].score_threshold;
    data[n++] = threshold[0].min_height;
    data[n++] = threshold[0].min_width;

    return HI_SUCCESS;
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_acl_vdec_to_vo(hi_void *args)
{
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_get_input_data_buffer_info(const svp_npu_task_info *task, hi_u32 idx,
                                                               hi_u8 **virt_addr, hi_u32 *size, hi_u32 *stride)
{
    svp_acl_data_buffer *data_buffer = HI_NULL;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(virt_addr == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "virt_addr is NULL!\n");
    svp_check_exps_return(size == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "size is NULL!\n");
    svp_check_exps_return(stride == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "size is NULL!\n");

    data_buffer = svp_acl_mdl_get_dataset_buffer(task->input_dataset, idx);
    svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get 0-th data_buffer failed!\n");
    *size = (hi_u32)svp_acl_get_data_buffer_size(data_buffer);
    *stride = (hi_u32)svp_acl_get_data_buffer_stride(data_buffer);
    *virt_addr = (hi_u8 *)svp_acl_get_data_buffer_addr(data_buffer);
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_update_input_data_buffer_info(const unsigned char *yuv_frame, hi_u32 idx,
                                                                  const svp_npu_task_info *task)
{
    svp_acl_data_buffer *data_buffer = HI_NULL;
    svp_acl_error ret;

    void* binFileBufferData = nullptr;
    ret = svp_acl_rt_malloc(&binFileBufferData, YUV420_SIZE, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    memcpy(binFileBufferData, yuv_frame, YUV420_SIZE);

    data_buffer = svp_acl_mdl_get_dataset_buffer(task->input_dataset, idx);
    svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get %u-th data_buffer failed!\n", idx);
    void *old_data = svp_acl_get_data_buffer_addr(data_buffer);
    ret = svp_acl_update_data_buffer(data_buffer, (hi_void *)binFileBufferData, YUV420_SIZE, YUV_STRIDE);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "update data buffer failed!\n");
    if (old_data && old_data != yuv_frame)
    {
        svp_acl_rt_free(old_data); // 使用对应的内存释放接口
    }
    return HI_SUCCESS;
}

void* SVP_NNN::ReadBinFileWithStride(const std::string& fileName, const svp_acl_mdl_io_dims& dims,
    size_t stride, size_t dataSize)
{
    struct stat sBuf;
    int fileStatus = stat(fileName.data(), &sBuf);
    if (fileStatus == -1) {
        printf("failed to get file %s\n", fileName.c_str());
        return nullptr;
    }

    if (S_ISREG(sBuf.st_mode) == 0) {
        printf("%s is not a file, please enter a file\n", fileName.c_str());
        return nullptr;
    }

    std::ifstream binFile(fileName, std::ifstream::binary);
    if (binFile.is_open() == false) {
        printf("open file %s failed\n", fileName.c_str());
        return nullptr;
    }
    binFile.seekg(0, binFile.end);
    int binFileBufferLen = binFile.tellg();
    if (binFileBufferLen == 0) {
        printf("binfile is empty, filename is %s\n", fileName.c_str());
        binFile.close();
        return nullptr;
    }
    printf("===>binFileBufferLen : %d\n", binFileBufferLen);
    binFile.seekg(0, binFile.beg);
    void* binFileBufferData = nullptr;
    int64_t loopTimes = 1;
    for (size_t loop = 0; loop < dims.dim_count - 1; loop++) {
        loopTimes *= dims.dims[loop];
    }
    size_t bufferSize = loopTimes * stride;
    printf("=====>malloc bufferSize : %d, loopTimes : %d, stride : %d\n", bufferSize, loopTimes, stride);
    svp_acl_error ret = svp_acl_rt_malloc(&binFileBufferData, bufferSize, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != SVP_ACL_SUCCESS) {
        printf("malloc device buffer failed. size is %u\n", binFileBufferLen);
        binFile.close();
        return nullptr;
    }
    InitData(static_cast<int8_t*>(binFileBufferData), bufferSize);

    int64_t dimValue = dims.dims[dims.dim_count - 1];
    size_t lineSize = dimValue * dataSize;
    size_t allsize = 0;
    printf("===>dimValue : %d, dataSize : %d\n", dimValue, dataSize);
    int fd = open("svp_711_VL_1012.yuv", O_CREAT | O_RDWR);
    for (int64_t loop = 0; loop < loopTimes; loop++) {
        allsize += lineSize;
        // printf("===>loop * stride : %d, all size : %d\n", loop * stride, allsize);
        binFile.read((static_cast<char *>(binFileBufferData) + loop * stride), lineSize);
        write(fd, (static_cast<char *>(binFileBufferData) + loop * stride), lineSize);
    }
    close(fd);

    binFile.close();
    return binFileBufferData;
}


hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_model_execute(const svp_npu_task_info *task)
{
    svp_acl_data_buffer *data_buffer = HI_NULL;
    hi_void *data = HI_NULL;
    size_t size;
    hi_u32 i;
    svp_acl_error ret;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }

    ret = svp_acl_mdl_execute(rfcn_svp_npu_model[task->cfg.model_idx].model_id, task->input_dataset, task->output_dataset);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "svp_acl_mdl_execute failed, model_id is %u, error code is %d!\n",
                          rfcn_svp_npu_model[task->cfg.model_idx].model_id, ret);
    size_t output_buf_num = svp_acl_mdl_get_dataset_num_buffers(task->output_dataset);

    // if (task->cfg.is_cached == HI_TRUE)
    // {
    //     for (i = 0; i < rfcn_svp_npu_model[task->cfg.model_idx].output_num; i++)
    //     {
    //         data_buffer = svp_acl_mdl_get_dataset_buffer(task->output_dataset, i);
    //         svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
    //                               "get %u-th output data_buffer is NULL!\n", i);

    //         data = svp_acl_get_data_buffer_addr(data_buffer);
    //         svp_check_exps_return(data == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
    //                               "get %u-th output data is NULL!\n", i);

    //         size = svp_acl_get_data_buffer_size(data_buffer) / task->cfg.max_batch_num * task->cfg.dynamic_batch_num;
    //         svp_check_exps_return(size == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
    //                               "get %u-th output data size is 0!\n", i);

    //         ret = svp_acl_rt_mem_flush(data, size);
    //         svp_check_exps_return(data_buffer == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
    //                               "flush %u-th output data failed, error code is %d!\n", i, ret);

    //     }
    // }
    return ret;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_roi_to_rect(const svp_npu_task_info *task, sd3403_svp_rect_info *rect_info)
{
    hi_s32 ret;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(rect_info == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "rect_info is NULL!\n");

    // get roi num
    ret = sd3403_rfcn_svp_npu_get_roi_num_by_index(task, rect_info);
    svp_check_exps_return(ret == HI_FAILURE, ret, SVP_ERR_LEVEL_ERROR,
                          "get roi num failed!\n");
    ret = sd3403_rfcn_svp_npu_get_roi_by_index(task, rect_info);
    svp_check_exps_return(ret != HI_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get roi failed!\n");

    return HI_SUCCESS;
}

static void GetMaxScoreAndIdx(uint32_t objScoreIdx, uint32_t chnStep, const float *outData,
                              float &maxclsSCore, uint32_t &maxClsInx)
{
    uint32_t clsScoreIdx = objScoreIdx + chnStep;
    for (uint32_t c = 0; c < CLASS_NUM; c++)
    {
        float clsScoreVal = outData[clsScoreIdx];
        if (clsScoreVal > maxclsSCore)
        {
            maxclsSCore = clsScoreVal;
            maxClsInx = c;
        }
        clsScoreIdx += chnStep;
    }
}
inline static float Sigmod(float a)
{
    return 1.0f / (1.0f + exp(-a));
}

static float CalcIou(const vector<float> &box1, const vector<float> &box2)
{
    float area1 = box1[6];
    float area2 = box2[6];
    float xx1 = max(box1[0], box2[0]);
    float yy1 = max(box1[1], box2[1]);
    float xx2 = min(box1[2], box2[2]);
    float yy2 = min(box1[3], box2[3]);
    float w = max(0.0f, xx2 - xx1 + 1);
    float h = max(0.0f, yy2 - yy1 + 1);
    float inter = w * h;
    float ovr = inter / (area1 + area2 - inter);
    return ovr;
}

void MulticlassNms(vector<vector<float>> &bboxes, const vector<vector<float>> &vaildBox, float nmsThr)
{
    for (auto &item : vaildBox)
    { /* score, xcenter, ycenter, w, h, classId */
        float boxXCenter = item[XCENTER_IDX];
        float boxYCenter = item[YCENTER_IDX];
        float boxWidth = item[W_IDX];
        ;
        float boxHeight = item[H_IDX];
        ;

        float x1 = (boxXCenter - boxWidth / 2);
        float y1 = (boxYCenter - boxHeight / 2);
        float x2 = (boxXCenter + boxWidth / 2);
        float y2 = (boxYCenter + boxHeight / 2);
        float area = (x2 - x1 + 1) * (y2 - y1 + 1);
        bool keep = true;
        /* lx, ly, rx, ry, score, class id, area */
        vector<float> bbox{x1, y1, x2, y2, item[SCORE_IDX], item[CLSAA_ID_IDX], area};
        for (size_t j = 0; j < bboxes.size(); j++)
        {
            if (CalcIou(bbox, bboxes[j]) > nmsThr)
            {
                keep = false;
                break;
            }
        }
        if (keep)
        {
            bboxes.push_back(bbox);
        }
    }
}

bool Cmp(const std::vector<float> &veci, const vector<float> &vecj)
{
    if (veci[CLASS_ID] < vecj[CLASS_ID])
    {
        return true;
    }
    else if (veci[CLASS_ID] == vecj[CLASS_ID])
    {
        return veci[SCORE] > vecj[SCORE];
    }
    return false;
}

static void PrintResult(const std::vector<std::vector<float>> &boxValue)
{
    if (boxValue.empty())
    {
        printf("input box empty\n");
        return;
    }
    std::vector<int> clsNum;
    float cId = boxValue[0][CLASS_ID];
    int validNum = 0;
    for (size_t loop = 0; loop < boxValue.size(); loop++)
    {
        printf("====> loop : %d, class : %f\n", loop, boxValue[loop][CLASS_ID]);
        if (boxValue[loop][CLASS_ID] == cId)
        {
            validNum++;
        }
        else
        {
            clsNum.push_back(validNum);
            cId = boxValue[loop][CLASS_ID];
            validNum = 1;
        }
    }
    clsNum.push_back(validNum);
    int idx = 0;
    int sumNum = 0;
    printf("current class valid box number is: %d\n", clsNum[idx]);
    sumNum += clsNum[idx];
    size_t totalBoxNum = boxValue.size();
    for (size_t loop = 0; loop < totalBoxNum; loop++)
    {
        if (loop == static_cast<size_t>(sumNum))
        {
            idx++;
            printf("current class valid box number is: %d\n", clsNum[idx]);
            sumNum += clsNum[idx];
        }
        printf("lx: %lf, ly: %lf, rx: %lf, ry: %lf, score: %lf; class id: %d\n",
               boxValue[loop][TOP_LEFT_X], boxValue[loop][TOP_LEFT_Y], boxValue[loop][BOTTOM_RIGHT_X],
               boxValue[loop][BOTTOM_RIGHT_Y], boxValue[loop][SCORE], (int)boxValue[loop][CLASS_ID]);
    }
}

//=========新的输出结果
hi_s32 SVP_NNN::sd3403_rfcn_output_result(const svp_npu_task_info *task)
{
    svp_acl_mdl_io_dims inDims;
    svp_acl_error ret = svp_acl_mdl_get_input_dims(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, INPUT_IMG_ID, &inDims);
    if (ret != SVP_ACL_SUCCESS || inDims.dim_count <= 2)
    { // 2: dim count
        printf("svp_acl_mdl_get_input_dims error!\n");
    }
    vector<vector<float>> vaildBox;
    FilterYolov5v7Box(task, vaildBox);
    std::sort(vaildBox.begin(), vaildBox.end(), [](const vector<float> &veci, const vector<float> &vecj)
              {
        if (veci[0] > vecj[0]) {
            return true;
        }
        return false; });
    vector<vector<float>> bboxes;
    const float nmsThr = 0.45; // 0.45
    MulticlassNms(bboxes, vaildBox, nmsThr);
    if (bboxes.size() == 0)
    {
        printf("total valid num is zero\n");
        return HI_FAILURE;
    }

    std::sort(bboxes.begin(), bboxes.end(), Cmp);
    PrintResult(bboxes);
}

void SVP_NNN::FilterYolov5v7Box(const svp_npu_task_info *task, std::vector<vector<float>> &vaildBox)
{
    size_t detectionOutNum = svp_acl_mdl_get_num_outputs(rfcn_svp_npu_model[task->cfg.model_idx].model_desc);
    vector<vector<uint32_t>> anchorGrids;
    if (YOLO_VERSION == 5)
    {
        anchorGrids = {
            {116, 90, 156, 198, 373, 326}, // p5/32
            {30, 61, 62, 45, 59, 119},     // p4/16
            {10, 13, 16, 30, 33, 23}       // p3/8
        };
    }
    else
    { /* yolov7 */
        anchorGrids = {
            {142, 110, 192, 243, 459, 401}, // p5/32
            {36, 75, 76, 55, 72, 146},      // p4/16
            {12, 16, 19, 36, 40, 28}        // p3/8
        };
    }
    /* gen box */
    for (size_t n = 0; n < detectionOutNum; n++)
    {
        ProcessPerDectection(n, vaildBox, anchorGrids, task);
    }
}

void SVP_NNN::ProcessPerDectection(size_t detectIdx, vector<vector<float>> &vaildBox, vector<vector<uint32_t>> &anchorGrids, const svp_npu_task_info *task)
{
    svp_acl_mdl_io_dims outDims;
    svp_acl_mdl_get_output_dims(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, detectIdx, &outDims);
    svp_acl_data_buffer *dataBuffer = svp_acl_mdl_get_dataset_buffer(task->output_dataset, detectIdx);
    DetectionInnerParam innerParam;
    innerParam.scoreThr = scoreThr_;
    innerParam.detectIdx = detectIdx;
    innerParam.outData = reinterpret_cast<float *>(svp_acl_get_data_buffer_addr(dataBuffer));

    innerParam.wStrideOffset = svp_acl_mdl_get_output_default_stride(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, detectIdx) / sizeof(float);
    uint32_t outHeight = outDims.dims[outDims.dim_count - 2];
    innerParam.outWidth = outDims.dims[outDims.dim_count - 1];
    innerParam.chnStep = outHeight * innerParam.wStrideOffset;
    vector<uint32_t> expandedStrides{32, 16, 8}; /* 8: 16 : 32: anchor size */
    vector<uint32_t> hSizes{20, 40, 80};         // imgh / expandedStrides
    vector<uint32_t> wSizes{20, 40, 80};         // imgw / expandedStrides

    /* gen grids */
    vector<float> gridsX(wSizes[detectIdx]);
    vector<float> gridsY(hSizes[detectIdx]);
    for (uint32_t i = 0; i < hSizes[detectIdx]; i++)
    {
        gridsY[i] = i - 0.5; // 0.5: alg param
    }
    for (uint32_t i = 0; i < wSizes[detectIdx]; i++)
    {
        gridsX[i] = i - 0.5; // 0.5: alg param
    }
    innerParam.objScoreOffset = 4 * innerParam.chnStep; // 4: offset
    for (uint32_t i = 0; i < outHeight; i++)
    {
        innerParam.outHeightIdx = i;
        ProcessPerDectectionInner(innerParam, gridsX, gridsY, anchorGrids, vaildBox);
    }
}

void SVP_NNN::ProcessPerDectectionInner(const DetectionInnerParam &innerParam, const vector<float> &gridsX,
                                        const vector<float> &gridsY, const vector<vector<uint32_t>> &anchorGrids, vector<vector<float>> &vaildBox)
{
    vector<uint32_t> expandedStrides{32, 16, 8}; /* 8: 16 : 32: anchor size */
    uint32_t outHeightIdx = innerParam.outHeightIdx;
    uint32_t chnStep = innerParam.chnStep;
    float scoreThr = innerParam.scoreThr;
    float *outData = innerParam.outData;
    size_t wStrideOffset = innerParam.wStrideOffset;
    uint32_t objScoreOffset = innerParam.objScoreOffset;
    uint32_t offset = outHeightIdx * innerParam.wStrideOffset;
    for (uint32_t j = 0; j < innerParam.outWidth; j++)
    {
        for (uint32_t k = 0; k < SCALE_SIZE; k++)
        {
            offset = j + outHeightIdx * wStrideOffset + k * chnStep * OUT_PARM_NUM;
            uint32_t objScoreIdx = offset + objScoreOffset;
            float objScoreVal = Sigmod(outData[objScoreIdx]);
            if (objScoreVal <= scoreThr)
            {
                continue;
            }
            /* max score */
            float maxclsSCore = 0.0f;
            uint32_t maxClsInx = 0;
            GetMaxScoreAndIdx(objScoreIdx, chnStep, outData, maxclsSCore, maxClsInx);

            float confidenceScore = Sigmod(maxclsSCore) * objScoreVal;
            if (confidenceScore > scoreThr)
            {
                /* gen box  info */
                uint32_t xCenterIdx = offset;
                uint32_t yCenterIdx = xCenterIdx + chnStep;
                uint32_t boxWidthIdx = yCenterIdx + chnStep;
                uint32_t boxHieghtIdx = boxWidthIdx + chnStep;
                float xCenter = (Sigmod(outData[xCenterIdx]) * 2 + gridsX[j]) *            // 2: alg param
                                expandedStrides[innerParam.detectIdx];                     // 2: alg param
                float yCenter = (Sigmod(outData[yCenterIdx]) * 2 + gridsY[outHeightIdx]) * // 2: alg param
                                expandedStrides[innerParam.detectIdx];
                float tmpValue = Sigmod(outData[boxWidthIdx]) * 2; // 2: alg param
                float boxWidth = tmpValue * tmpValue * anchorGrids[innerParam.detectIdx][(k << 1)];
                tmpValue = Sigmod(outData[boxHieghtIdx]) * 2; // 2: alg param
                float boxHieght = tmpValue * tmpValue * anchorGrids[innerParam.detectIdx][(k << 1) + 1];

                vaildBox.push_back({confidenceScore, xCenter, yCenter, boxWidth,
                                    boxHieght, static_cast<float>(maxClsInx)});
            }
        }
    }
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_get_roi_num_by_index(const svp_npu_task_info *task, sd3403_svp_rect_info *rect_info)
{
    svp_acl_mdl_io_dims aclDims;
    std::vector<int> validBoxNum;
    svp_acl_error ret = 0;
    ret = svp_acl_mdl_get_output_dims(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, OUTPUT_NUM_ID, &aclDims);
    if(ret != SVP_ACL_SUCCESS)
    {
        ss_print("svp_acl_mdl_get_output_dims error!\n");
        return HI_FAILURE;
    }
    svp_acl_data_buffer *dataBuffer = svp_acl_mdl_get_dataset_buffer(task->output_dataset, OUTPUT_NUM_ID);
    auto outData = reinterpret_cast<float *>(svp_acl_get_data_buffer_addr(dataBuffer));
    printf("roi num : %d\n", static_cast<uint32_t>(aclDims.dims[aclDims.dim_count - 1]));
    for (uint32_t loop = 0; loop < static_cast<uint32_t>(aclDims.dims[aclDims.dim_count - 1]); loop++)
    {
        // printf("====> push : %f\n", *(outData + loop));
        validBoxNum.push_back(*(outData + loop));
    }

    for (size_t loop = 0; loop < validBoxNum.size(); loop++)
    {
        // printf("validBoxNum[loop] : %d\n", validBoxNum[loop]);
        rect_info->num += validBoxNum[loop];
    }
    if (rect_info->num == 0)
    {
        ss_print("total valid num is zero\n");
        return -2;
    }

    return HI_SUCCESS;
}

bool SVP_NNN::sd3403_rfcn_cmp(const std::vector<float> &veci, const std::vector<float> &vecj)
{
    if (veci[CLASS_ID] < vecj[CLASS_ID])
    {
        return true;
    }
    else if (veci[CLASS_ID] == vecj[CLASS_ID])
    {
        return veci[SCORE] > vecj[SCORE];
    }
    return false;
}

void SVP_NNN::sd3403_rfcn_parse_result(const std::vector<std::vector<float>> &boxValue, sd3403_svp_rect_info *rect_info)
{
    if (boxValue.empty())
    {
        printf("input box empty\n");
        return;
    }
    std::vector<int> clsNum;
    float cId = boxValue[0][CLASS_ID];
    int validNum = 0;
    for (size_t loop = 0; loop < boxValue.size(); loop++)
    {
        if (boxValue[loop][CLASS_ID] == cId)
        {
            validNum++;
        }
        else
        {
            clsNum.push_back(validNum);
            cId = boxValue[loop][CLASS_ID];
            validNum = 1;
        }
    }
    clsNum.push_back(validNum);
    int idx = 0;
    int sumNum = 0;
    printf("current class valid box number is: %d\n", clsNum[idx]);
    sumNum += clsNum[idx];
    size_t totalBoxNum = boxValue.size();
    // printf("========>totalBoxNum : %d\n", totalBoxNum);
    for (size_t loop = 0; loop < totalBoxNum; loop++)
    {
        // printf("===>loop : %d\n", loop);
        if (loop == static_cast<size_t>(sumNum))
        {
            idx++;
            printf("==>current class valid box number is: %d\n", clsNum[idx]);
            sumNum += clsNum[idx];
        }
        rect_info->rect[loop].lx = boxValue[loop][TOP_LEFT_X];
        rect_info->rect[loop].ly = boxValue[loop][TOP_LEFT_Y];
        rect_info->rect[loop].rx = boxValue[loop][BOTTOM_RIGHT_X];
        rect_info->rect[loop].ry = boxValue[loop][BOTTOM_RIGHT_Y];
        rect_info->rect[loop].score = boxValue[loop][SCORE];
        rect_info->rect[loop].classId = (int)boxValue[loop][CLASS_ID];
        printf("lx: %lf, ly: %lf, rx: %lf, ry: %lf, score: %lf; class id: %d\n",
               boxValue[loop][TOP_LEFT_X], boxValue[loop][TOP_LEFT_Y], boxValue[loop][BOTTOM_RIGHT_X],
               boxValue[loop][BOTTOM_RIGHT_Y], boxValue[loop][SCORE], (int)boxValue[loop][CLASS_ID]);
    }
    return;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_get_roi_by_index(const svp_npu_task_info *task, sd3403_svp_rect_info *rect_info)
{
    svp_acl_mdl_io_dims aclDims;

    // get x y score
    svp_acl_data_buffer *dataBufferValue = svp_acl_mdl_get_dataset_buffer(task->output_dataset, OUTPUT_BBOX_ID);
    auto outDataValue = reinterpret_cast<float *>(svp_acl_get_data_buffer_addr(dataBufferValue));
    svp_acl_mdl_get_output_dims(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, OUTPUT_BBOX_ID, &aclDims);
    if (aclDims.dim_count <= 0)
    {
        printf("aclrtOutputDims error\n");
        return HI_FAILURE;
    }

    svp_acl_error ret = svp_acl_mdl_get_input_dims(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, INPUT_IMG_ID, &aclDims);
    if (ret != SVP_ACL_SUCCESS || aclDims.dim_count <= 2)
    { // 2: dim count
        printf("svp_acl_mdl_get_input_dims error!\n");
    }
    // input data shape is nchw, 2 is stand h
    int imgHeight = aclDims.dims[aclDims.dim_count - 2];
    int imgWidth = aclDims.dims[aclDims.dim_count - 1];
    printf("input image width[%d]; height[%d]\n", imgWidth, imgHeight);

    size_t wStrideOffset = svp_acl_mdl_get_output_default_stride(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, OUTPUT_BBOX_ID) / sizeof(float);
    // box include 6 part which is lx, ly, rx, ry, score, class id
    std::vector<std::vector<float>> bboxes;
    for (int inx = 0; inx < rect_info->num; inx++)
    {
        float classId = (*(outDataValue + inx + CLASS_ID * wStrideOffset));
        // if (classId == 0.0f) {
        // continue; // skip class 0 back ground
        //}
        std::vector<float> bbox(BBOX_SIZE, 0.0f);
        for (size_t loop = 0; loop < BBOX_SIZE; loop++)
        {
            bbox[loop] = (*(outDataValue + inx + loop * wStrideOffset));
        }
        bboxes.push_back(bbox);
    }
    printf("=====> boxs : %d\n", bboxes.size());
    std::sort(bboxes.begin(), bboxes.end(), sd3403_rfcn_cmp);
    sd3403_rfcn_parse_result(bboxes, rect_info);
    // size_t buffer_size = svp_acl_mdl_get_input_size_by_index(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, OUTPUT_BBOX_ID) * \
    //             (hi_u64)task->cfg.max_batch_num;
    // memset(outDataValue, 0, buffer_size);

    // PrintResult(bboxes);
    return HI_SUCCESS;
}


hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_acl_frame_proc(const unsigned char *yuv_frame)
{
    hi_s32 ret;

    ret = sd3403_rfcn_svp_npu_update_input_data_buffer_info(yuv_frame, 0, &g_svp_npu_task[0]);
    svp_check_exps_return(ret != HI_SUCCESS, ret, SVP_ERR_LEVEL_ERROR, "update data buffer failed!\n");

    ret = sd3403_rfcn_svp_npu_model_execute(&g_svp_npu_task[0]);
    svp_check_exps_return(ret != HI_SUCCESS, ret, SVP_ERR_LEVEL_ERROR, "model execute failed!\n");

    ret = sd3403_rfcn_svp_npu_roi_to_rect(&g_svp_npu_task[0], &g_svp_npu_rfcn_rect_info);
    svp_check_exps_return(ret == HI_FAILURE, ret, SVP_ERR_LEVEL_ERROR, "roi to rect failed!\n");
    // sd3403_rfcn_output_result(&g_svp_npu_task[0]);

    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_acl_frame_handle(const unsigned char *yuv_frame)
{
    hi_s32 ret = 0;
    // need change
    sd3403_svp_npu_detection_info detection_info = {0};
    // detection_info.num_name = "detection_filter_3_0";
    // detection_info.roi_name = "detection_filter_3_";
    detection_info.has_background = HI_TRUE;

    // ret = svp_acl_rt_set_device(g_svp_npu_dev_id);
    // svp_check_exps_return(ret != HI_SUCCESS, HI_NULL, SVP_ERR_LEVEL_ERROR, "open device failed!\n");
    // ret = sd3403_rfcn_svp_npu_get_input_data_buffer_info(&g_svp_npu_task[0], 0, &data, &size, &stride);
    // svp_check_exps_return(ret != HI_SUCCESS, HI_NULL, SVP_ERR_LEVEL_ERROR,"Error(%#x),get_input_data_buffer_info failed!\n", ret);

    ret = sd3403_rfcn_svp_npu_acl_frame_proc(yuv_frame);
    svp_check_exps_return(ret == HI_FAILURE, ret, SVP_ERR_LEVEL_ERROR, "Error(%#x),sample_svp_npu_acl_frame_proc failed!\n", ret);
    // ret = sd3403_rfcn_svp_npu_update_input_data_buffer_info(data, size, stride, 0, &g_svp_npu_task[0]);
    // svp_check_exps_return(ret != HI_SUCCESS, HI_NULL, SVP_ERR_LEVEL_ERROR, "update buffer failed!\n");
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_malloc_mem(hi_void **buffer, hi_u32 buffer_size, hi_bool is_cached)
{
    svp_acl_error ret;

    if (is_cached == HI_TRUE)
    {
        ret = svp_acl_rt_malloc_cached(buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    else
    {
        ret = svp_acl_rt_malloc(buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
    }
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "can't malloc buffer, size is %u, error code is %d!\n", buffer_size, ret);

    (hi_void) memset_s(*buffer, buffer_size, 0, buffer_size);
    if (is_cached == HI_TRUE)
    {
        (hi_void) svp_acl_rt_mem_flush(*buffer, buffer_size);
    }
    return ret;
}

svp_acl_data_buffer *SVP_NNN::sd3403_rfcn_svp_npu_create_input_data_buffer(svp_npu_task_info *task, hi_u32 idx)
{
    size_t buffer_size, stride;
    hi_void *input_buffer = HI_NULL;
    svp_acl_data_buffer *input_data = HI_NULL;

    stride = svp_acl_mdl_get_input_default_stride(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, idx);
    svp_check_exps_return(stride == 0, input_data, SVP_ERR_LEVEL_ERROR,
                          "get %u-th input stride failed!\n", idx);

    buffer_size = svp_acl_mdl_get_input_size_by_index(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, idx) *
                  (hi_u64)task->cfg.max_batch_num;
    svp_check_exps_return((buffer_size == 0 || buffer_size > SVP_NPU_MAX_MEM_SIZE), input_data,
                          SVP_ERR_LEVEL_ERROR, "buffer_size(%lu) can't be 0 and should be less than %u!\n",
                          buffer_size, SVP_NPU_MAX_MEM_SIZE);

    printf("=====> input num : %d, size : %d, stride : %d\n", idx, buffer_size, stride);
    if (sd3403_rfcn_svp_npu_malloc_mem(&input_buffer, (hi_u32)buffer_size, task->cfg.is_cached) != HI_SUCCESS)
    {
        svp_trace_err("%u-th input malloc mem failed!\n", idx);
        return input_data;
    }

    input_data = svp_acl_create_data_buffer(input_buffer, buffer_size, stride);
    if (input_data == HI_NULL)
    {
        svp_trace_err("can't create %u-th input data buffer!\n", idx);
        (hi_void) svp_acl_rt_free(input_buffer);
        return input_data;
    }

    printf("========>idx : %d, 1 : %d, 2 : %d\n", idx, rfcn_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM, rfcn_svp_npu_model[task->cfg.model_idx].input_num - 1);
    if (idx == rfcn_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM)
    {
        task->task_buf_ptr = input_buffer;
        task->task_buf_size = buffer_size;
        task->task_buf_stride = stride;
    }
    else if (idx == rfcn_svp_npu_model[task->cfg.model_idx].input_num - 1)
    {
        task->work_buf_ptr = input_buffer;
        task->work_buf_size = buffer_size;
        task->work_buf_stride = stride;
    }
    return input_data;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_create_work_buf(svp_npu_task_info *task)
{
    size_t num;
    svp_acl_data_buffer *work_buf = HI_NULL;
    svp_acl_error ret;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }

    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "input_dataset is NULL!\n");

    num = svp_acl_mdl_get_dataset_num_buffers(task->input_dataset);
    svp_check_exps_return(num != rfcn_svp_npu_model[task->cfg.model_idx].input_num - 1,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "num of data buffer(%lu) should be %lu when create work buf!\n",
                          num, rfcn_svp_npu_model[task->cfg.model_idx].input_num - 1);

    work_buf = sd3403_rfcn_svp_npu_create_input_data_buffer(task, rfcn_svp_npu_model[task->cfg.model_idx].input_num - 1);
    svp_check_exps_return(work_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "create work buf failed!\n");

    ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, work_buf);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("add work buf failed!\n");
        (hi_void) svp_npu_destroy_data_buffer(work_buf);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_share_work_buf(const svp_npu_shared_work_buf *shared_work_buf, const svp_npu_task_info *task)
{
    svp_acl_error ret;
    svp_acl_data_buffer *work_buf = HI_NULL;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }

    svp_check_exps_return(shared_work_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "shared_work_buf is NULL!\n");

    svp_check_exps_return(shared_work_buf->work_buf_ptr == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "work_buf_ptr is NULL!\n");

    svp_check_exps_return(task->work_buf_ptr != HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "task has created work buf!\n");

    work_buf = svp_acl_create_data_buffer(shared_work_buf->work_buf_ptr, shared_work_buf->work_buf_size,
                                          shared_work_buf->work_buf_stride);
    svp_check_exps_return(work_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "create work buf failed!\n");

    ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, work_buf);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("add work buf failed!\n");
        (hi_void) svp_acl_destroy_data_buffer(work_buf);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_check_has_aicpu_task(const svp_npu_task_info *task, hi_bool *has_aicpu_task)
{
    hi_u32 aicpu_task_num;
    svp_acl_error ret;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(has_aicpu_task == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "has_aicpu_task is NULL!\n");

    ret = svp_acl_ext_get_mdl_aicpu_task_num(rfcn_svp_npu_model[task->cfg.model_idx].model_id, &aicpu_task_num);
    svp_check_exps_return(ret != SVP_ACL_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "get aicpu task num failed, error code is %d!\n", ret);

    *has_aicpu_task = (aicpu_task_num == 0) ? HI_FALSE : HI_TRUE;
    return HI_SUCCESS;
}

hi_void *SVP_NNN::sd3403_rfcn_svp_npu_acl_aicpu_thread(hi_void)
{
    svp_acl_error ret;

    while (g_svp_npu_aicpu_process_signal == HI_TRUE)
    {
        ret = svp_acl_ext_process_aicpu_task(SD3403_SVP_NPU_AICPU_WAIT_TIME);
        if (ret != SVP_ACL_SUCCESS && ret != SVP_ACL_ERROR_RT_REPORT_TIMEOUT)
        {
            svp_trace_err("aicpu porcess failed\n");
            break;
        }
    }
    return HI_NULL;
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_destroy_work_buf(svp_npu_task_info *task)
{
    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->work_buf_ptr == HI_NULL)
    {
        return;
    }
    (hi_void) svp_acl_rt_free(task->work_buf_ptr);
    task->work_buf_ptr = HI_NULL;
    task->work_buf_stride = 0;
    task->work_buf_size = 0;
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_destroy_output(svp_npu_task_info *task)
{
    hi_u32 i;
    size_t output_num;
    svp_acl_data_buffer *data_buffer = HI_NULL;
    hi_void *data = HI_NULL;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->output_dataset == HI_NULL)
    {
        return;
    }

    output_num = svp_acl_mdl_get_dataset_num_buffers(task->output_dataset);

    for (i = 0; i < output_num; i++)
    {
        data_buffer = svp_acl_mdl_get_dataset_buffer(task->output_dataset, i);
        data = svp_acl_get_data_buffer_addr(data_buffer);
        (hi_void) svp_acl_rt_free(data);
        (hi_void) svp_acl_destroy_data_buffer(data_buffer);
    }

    (hi_void) svp_acl_mdl_destroy_dataset(task->output_dataset);
    task->output_dataset = HI_NULL;
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_acl_dataset_deinit(hi_u32 task_idx)
{
    (hi_void) sd3403_rfcn_svp_npu_destroy_input(&g_svp_npu_task[task_idx]);
    (hi_void) sd3403_rfcn_svp_npu_destroy_output(&g_svp_npu_task[task_idx]);
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_destroy_task_buf(svp_npu_task_info *task)
{
    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->task_buf_ptr == HI_NULL)
    {
        return;
    }
    (hi_void) svp_acl_rt_free(task->task_buf_ptr);
    task->task_buf_ptr = HI_NULL;
    task->task_buf_stride = 0;
    task->task_buf_size = 0;
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_acl_deinit_task(hi_u32 task_num, hi_u32 shared_work_buf_idx)
{
    hi_u32 task_idx;

    // if (g_svp_npu_aicpu_process_signal == HI_TRUE)
    // {
    //     g_svp_npu_aicpu_process_signal = HI_FALSE;
    //     if (aicpu_thread.joinable())
    //     {
    //         aicpu_thread.join();
    //     }
    // }

    for (task_idx = 0; task_idx < task_num; task_idx++)
    {
        (hi_void) sd3403_rfcn_svp_npu_destroy_work_buf(&g_svp_npu_task[task_idx]);
        (hi_void) sd3403_rfcn_svp_npu_destroy_task_buf(&g_svp_npu_task[task_idx]);
        (hi_void) sd3403_rfcn_svp_npu_acl_dataset_deinit(task_idx);
        (hi_void) memset_s(&g_svp_npu_task[task_idx], sizeof(svp_npu_task_cfg), 0,
                           sizeof(svp_npu_task_cfg));
    }
    if (g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr != HI_NULL)
    {
        (hi_void) svp_acl_rt_free(g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr);
        g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_ptr = HI_NULL;
        g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_size = 0;
        g_svp_npu_shared_work_buf[shared_work_buf_idx].work_buf_stride = 0;
    }
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_create_task_buf(svp_npu_task_info *task)
{
    size_t num;
    svp_acl_data_buffer *task_buf = HI_NULL;
    svp_acl_error ret;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "input_dataset is NULL!\n");

    num = svp_acl_mdl_get_dataset_num_buffers(task->input_dataset);
    svp_check_exps_return(num != rfcn_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM,
                          HI_FAILURE, SVP_ERR_LEVEL_ERROR, "num of data buffer(%lu) should be %lu when create task buf!\n",
                          num, rfcn_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM);

    task_buf = sd3403_rfcn_svp_npu_create_input_data_buffer(task, rfcn_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM);
    svp_check_exps_return(task_buf == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create task buf failed!\n");

    ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, task_buf);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("add task buf failed!\n");
        (hi_void) svp_npu_destroy_data_buffer(task_buf);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

td_s32 SVP_NNN::sd3403_rfcn_svp_npu_acl_init_task(td_bool is_share_work_buf, td_u32 shared_work_buf_idx)
{
    hi_u32 task_idx = 0;
    hi_s32 ret = 0;
    hi_bool has_aicpu_task = HI_FALSE;

    if (is_share_work_buf == HI_TRUE)
    {
        ret = svp_npu_acl_create_shared_work_buf(shared_work_buf_idx);
        svp_check_exps_return(ret != HI_SUCCESS, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create shared work buf failed!\n");
    }

    for (task_idx = 0; task_idx < task_num; task_idx++)
    {
        ret = sd3403_rfcn_svp_npu_acl_dataset_init(task_idx);
        if (ret != HI_SUCCESS)
        {
            printf("[%s] : %d - init error\n", __func__, __LINE__);
            goto task_init_end_0;
        }
        // 看到这一步
        ret = sd3403_rfcn_svp_npu_create_task_buf(&g_svp_npu_task[task_idx]);
        if (ret != HI_SUCCESS)
        {
            svp_trace_err("create task buf failed.\n");
            goto task_init_end_0;
        }

        if (is_share_work_buf == HI_FALSE)
        {
            ret = sd3403_rfcn_svp_npu_create_work_buf(&g_svp_npu_task[task_idx]);
        }
        else
        {
            /* if all tasks are on the same stream, work buf can be shared */
            ret = sd3403_rfcn_svp_npu_share_work_buf(&g_svp_npu_shared_work_buf[shared_work_buf_idx],
                                                     &g_svp_npu_task[task_idx]);
        }

        if (ret != HI_SUCCESS)
        {
            svp_trace_err("create work buf failed.\n");
            goto task_init_end_0;
        }
        // 创建aicpu线程
        // if (g_svp_npu_aicpu_process_signal == HI_FALSE)
        // {

        //     ret = sd3403_rfcn_svp_npu_check_has_aicpu_task(&g_svp_npu_task[task_idx], &has_aicpu_task);
        //     if (ret != HI_SUCCESS)
        //     {
        //         svp_trace_err("check has aicpu task failed.\n");
        //         goto task_init_end_0;
        //     }

        //     if (has_aicpu_task == HI_TRUE)
        //     {
        //         g_svp_npu_aicpu_process_signal = HI_TRUE;
        //         // ret = pthread_create(&g_svp_npu_aicpu_thread, 0, sd3403_svp_npu_acl_aicpu_thread, HI_NULL);
        //         try
        //         {
        //             aicpu_thread = std::thread(&SVP_NNN::sd3403_rfcn_svp_npu_acl_aicpu_thread, this);
        //         }
        //         catch (const std::system_error &e)
        //         {
        //             svp_trace_err("create aicpu task thread failed.\n");
        //             goto task_init_end_0;
        //         }
        //     }
        // }
    }


    return HI_SUCCESS;
task_init_end_0:
    (hi_void) sd3403_rfcn_svp_npu_acl_deinit_task(task_num, shared_work_buf_idx);
    return ret;
}

void SVP_NNN::InitData(int8_t *data, size_t dataSize)
{
    for (size_t i = 0; i < dataSize; i++)
    {
        data[i] = 0;
    }
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_destroy_input(svp_npu_task_info *task)
{
    hi_u32 i;
    size_t input_num;
    svp_acl_data_buffer *data_buffer = HI_NULL;
    hi_void *data = HI_NULL;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return;
    }

    if (task->input_dataset == HI_NULL)
    {
        return;
    }

    input_num = svp_acl_mdl_get_dataset_num_buffers(task->input_dataset);
    printf("intput num : %d\n", input_num);
    for (i = 0; i < input_num; i++)
    {
        data_buffer = svp_acl_mdl_get_dataset_buffer(task->input_dataset, i);
        if (i < rfcn_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM)
        {
            data = svp_acl_get_data_buffer_addr(data_buffer);
            (hi_void) svp_acl_rt_free(data);
        }
        (hi_void) svp_acl_destroy_data_buffer(data_buffer);
    }

    (hi_void) svp_acl_mdl_destroy_dataset(task->input_dataset);
    task->input_dataset = HI_NULL;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_create_input(svp_npu_task_info *task)
{
    svp_acl_error ret;
    hi_u32 i;
    svp_acl_data_buffer *input_data = HI_NULL;
    hi_void *input_buffer = HI_NULL;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    task->input_dataset = svp_acl_mdl_create_dataset();
    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create input dataset failed!\n");

    for (i = 0; i < rfcn_svp_npu_model[task->cfg.model_idx].input_num - SVP_NPU_EXTRA_INPUT_NUM; i++)
    {

        input_data = sd3403_rfcn_svp_npu_create_input_data_buffer(task, i);
        if (input_data == HI_NULL)
        {
            svp_trace_err("create %u-th input data buffer failed!\n", i);
            (hi_void) sd3403_rfcn_svp_npu_destroy_input(task);
            return HI_FAILURE;
        }

        ret = svp_acl_mdl_add_dataset_buffer(task->input_dataset, input_data);
        if (ret != SVP_ACL_SUCCESS)
        {
            svp_trace_err("add %u-th input data buffer failed!\n", i);
            (hi_void) sd3403_rfcn_svp_npu_destroy_input(task);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_create_output(svp_npu_task_info *task)
{
    svp_acl_error ret;
    hi_u32 i;
    svp_acl_data_buffer *output_data = HI_NULL;

    if (sd3403_rfcn_svp_check_task_cfg(task) != HI_SUCCESS)
    {
        svp_trace_err("check task cfg failed!\n");
        return HI_FAILURE;
    }
    task->output_dataset = svp_acl_mdl_create_dataset();
    svp_check_exps_return(task->input_dataset == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "create output dataset failed!\n");
    for (i = 0; i < rfcn_svp_npu_model[task->cfg.model_idx].output_num; i++)
    {
        size_t buffer_size, stride;
        hi_void *output_buffer = HI_NULL;
        svp_acl_data_buffer *output_data = HI_NULL;

        stride = svp_acl_mdl_get_output_default_stride(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, i);
        svp_check_exps_return(stride == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "get %u-th output stride failed!\n", i);

        buffer_size = svp_acl_mdl_get_output_size_by_index(rfcn_svp_npu_model[task->cfg.model_idx].model_desc, i) * (hi_u64)task->cfg.max_batch_num;
        svp_check_exps_return((buffer_size == 0 || buffer_size > SVP_NPU_MAX_MEM_SIZE), HI_FAILURE,
                              SVP_ERR_LEVEL_ERROR, "buffer_size(%lu) can't be 0 and should be less than %u!\n",
                              buffer_size, SVP_NPU_MAX_MEM_SIZE);

        // malloc mem

        if (sd3403_rfcn_svp_npu_malloc_mem(&output_buffer, buffer_size, task->cfg.is_cached) != HI_SUCCESS)
        {
            svp_trace_err("%u-th output malloc mem failed!\n", i);
            return HI_FAILURE;
        }

        output_data = svp_acl_create_data_buffer(output_buffer, buffer_size, stride);
        if (output_data == HI_NULL)
        {
            svp_trace_err("can't create %u-th output data buffer!\n", i);
            (hi_void) svp_acl_rt_free(output_buffer);
            return HI_FAILURE;
        }

        ret = svp_acl_mdl_add_dataset_buffer(task->output_dataset, output_data);
        if (ret != SVP_ACL_SUCCESS)
        {
            svp_trace_err("add %u-th output data buffer failed!\n", i);
            (hi_void) svp_npu_destroy_data_buffer(output_data);
            (hi_void) sd3403_rfcn_svp_npu_destroy_output(task);
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_acl_dataset_init(hi_u32 task_idx)
{
    /*这步有些不同，yolo上是直接创建的有图像数据的输入，并且在创建输出之后创建的； yolo上显示，有几个图像输入就创建几个输入*/
    hi_s32 ret = sd3403_rfcn_svp_npu_create_input(&g_svp_npu_task[task_idx]);
    if (ret != HI_SUCCESS)
    {
        svp_trace_err("svp npu create input error!\n");
        return HI_FAILURE;
    }
    /**/
    ret = sd3403_rfcn_svp_npu_create_output(&g_svp_npu_task[task_idx]);
    if (ret != HI_SUCCESS)
    {
        svp_trace_err("svp npu create output error!\n");
        sd3403_rfcn_svp_npu_destroy_input(&g_svp_npu_task[task_idx]);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

td_s32 SVP_NNN::sd3403_rfcn_svp_check_task_cfg(const svp_npu_task_info *task)
{
    svp_check_exps_return(task == HI_NULL, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "task is NULL!\n");

    svp_check_exps_return(task->cfg.max_batch_num == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "max_batch_num(%u) is 0!\n", task->cfg.max_batch_num);

    svp_check_exps_return(task->cfg.dynamic_batch_num == 0, HI_FAILURE, SVP_ERR_LEVEL_ERROR,
                          "dynamic_batch_num(%u) is 0!\n", task->cfg.dynamic_batch_num);

    svp_check_exps_return(task->cfg.total_t != 0 && task->cfg.dynamic_batch_num != 1, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "dynamic_batch_num(%u) should be 1 when total_t(%u) is not 0!\n",
                          task->cfg.dynamic_batch_num, task->cfg.total_t);

    svp_check_exps_return((task->cfg.is_cached != HI_TRUE && task->cfg.is_cached != HI_FALSE), HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "is_cached(%u) should be [%u, %u]!\n", task->cfg.is_cached, HI_FALSE, HI_TRUE);

    svp_check_exps_return(task->cfg.model_idx >= SD3403_SVP_NPU_MAX_MODEL_NUM, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "model_idx(%u) should be less than %u!\n",
                          task->cfg.model_idx, SD3403_SVP_NPU_MAX_MODEL_NUM);

    svp_check_exps_return(rfcn_svp_npu_model[task->cfg.model_idx].model_desc == HI_NULL, HI_FAILURE,
                          SVP_ERR_LEVEL_ERROR, "%u-th model_desc is NULL!\n", task->cfg.model_idx);
    return HI_SUCCESS;
}

/*init svp nnn*/
hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_model_init(const hi_char *om_model_path)
{
    hi_s32 ret = 0;
    const hi_u32 model_idx = 0;
    g_svp_npu_task[0].cfg.max_batch_num = 1;
    g_svp_npu_task[0].cfg.dynamic_batch_num = 1;
    g_svp_npu_task[0].cfg.total_t = 0;
    g_svp_npu_task[0].cfg.is_cached = HI_TRUE;
    g_svp_npu_task[0].cfg.model_idx = model_idx;

    ret = sd3403_rfcn_svp_npu_acl_init();
    svp_check_exps_return(ret != HI_SUCCESS, HI_FALSE, SVP_ERR_LEVEL_ERROR, "init failed!\n");

    ret = sd3403_rfcn_svp_npu_load_model(om_model_path, model_idx, HI_FALSE);
    svp_check_exps_goto(ret != HI_SUCCESS, process_end1, SVP_ERR_LEVEL_ERROR, "load model failed!\n");

    ret = sd3403_rfcn_svp_npu_acl_init_task(HI_FALSE, 0);
    svp_check_exps_goto(ret != HI_SUCCESS, process_end2, SVP_ERR_LEVEL_ERROR, "init task failed!\n");

    /*这步应该和input一起，有多少个图片就创建多少个input和设置多少个阈值， 但是需要更按照yolo改代码*/
    ret = sd3403_rfcn_svp_npu_set_threshold(g_svp_npu_rfcn_threshold, SD3403_SVP_NPU_RFCN_THRESHOLD_NUM, &g_svp_npu_task[0]);
    svp_check_exps_goto(ret != HI_SUCCESS, process_end3, SVP_ERR_LEVEL_ERROR, "set threshold failed!\n");

    return HI_SUCCESS;
process_end1:
    sd3403_rfcn_svp_npu_acl_deinit_task(1, 0);
process_end2:
    sd3403_rfcn_svp_npu_unload_model(model_idx);
process_end3:
    printf("==============================yhtest================================================================\n");
    sd3403_rfcn_svp_npu_acl_deinit();
    return HI_FALSE;
}

hi_s32 SVP_NNN::sd3403_rfcn_get_output_result(std::vector<objinfo> &output)
{

    for (int i = 0; i < g_svp_npu_rfcn_rect_info.num; i++)
    {
        objinfo info;
        info.x1 = g_svp_npu_rfcn_rect_info.rect[i].lx;
        info.y1 = g_svp_npu_rfcn_rect_info.rect[i].ly;
        info.x2 = g_svp_npu_rfcn_rect_info.rect[i].rx;
        info.y2 = g_svp_npu_rfcn_rect_info.rect[i].ry;
        info.score = g_svp_npu_rfcn_rect_info.rect[i].score;
        info.trackID = g_svp_npu_rfcn_rect_info.rect[i].classId;
        output.push_back(info);
    }
    memset(&g_svp_npu_rfcn_rect_info, 0, sizeof(g_svp_npu_rfcn_rect_info));
    return HI_SUCCESS;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_model_inference(const unsigned char *yuv_frame, std::vector<objinfo> &output)
{
    hi_s32 ret = 0;

    ret = sd3403_rfcn_svp_npu_acl_frame_handle(yuv_frame);
    // svp_check_exps_goto(ret != 0, exit, SVP_ERR_LEVEL_ERROR, "create thread failed!\n");
    svp_check_exps_return(ret == HI_FAILURE, HI_FAILURE, SVP_ERR_LEVEL_ERROR, "Error(%#x),sd3403_rfcn_svp_npu_acl_frame_handle failed!\n", ret);

    ret = sd3403_rfcn_get_output_result(output);
    // svp_check_exps_goto(ret != 0, exit, SVP_ERR_LEVEL_ERROR, "get output result failed!\n");

    return HI_SUCCESS;
// exit:
//     sd3403_rfcn_svp_npu_model_deinit();
    // return HI_FALSE;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_model_deinit(void)
{
    const hi_u32 model_idx = 0;
    sd3403_rfcn_svp_npu_acl_deinit_task(1, 0);
    sd3403_rfcn_svp_npu_unload_model(model_idx);
    printf("=\n");
    sd3403_rfcn_svp_npu_acl_deinit();
    printf("-=\n");
    return 0;
}

hi_s32 SVP_NNN::sd3403_rfcn_svp_npu_acl_deinit(void)
{
    svp_acl_error ret;
    // ret = svp_acl_rt_set_device(g_svp_npu_dev_id);
    
    printf("=============================1=====stream : %p. context : %p===================\n", stream_, context_);
    ret = svp_acl_rt_synchronize_stream(stream_);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("synchronize stream fail, error : %d\n", ret);
    }
    ret = svp_acl_rt_destroy_stream(stream_);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("destroy stream fail, error : %d\n", ret);
    }

    ret = svp_acl_rt_destroy_context(context_);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("reset context fail\n");
    }

    ret = svp_acl_rt_reset_device(g_svp_npu_dev_id);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("reset device fail\n");
    }
    svp_trace_info("end to reset device is %d\n", g_svp_npu_dev_id);

    ret = svp_acl_finalize();
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("finalize acl fail\n");
    }

    return 0;
}

hi_void SVP_NNN::sd3403_rfcn_svp_npu_unload_model(hi_u32 model_index)
{
    svp_acl_error ret;

    ret = svp_acl_mdl_unload(rfcn_svp_npu_model[model_index].model_id);
    if (ret != SVP_ACL_SUCCESS)
    {
        svp_trace_err("unload model failed, model_id is %u, error code is %d!\n",
                      rfcn_svp_npu_model[model_index].model_id, ret);
    }

    if (rfcn_svp_npu_model[model_index].model_desc != HI_NULL)
    {
        (hi_void) svp_acl_mdl_destroy_desc(rfcn_svp_npu_model[model_index].model_desc);
        rfcn_svp_npu_model[model_index].model_desc = HI_NULL;
    }

    if (rfcn_svp_npu_model[model_index].model_mem_ptr != HI_NULL)
    {
        (hi_void) svp_acl_rt_free(rfcn_svp_npu_model[model_index].model_mem_ptr);
        rfcn_svp_npu_model[model_index].model_mem_ptr = HI_NULL;
        rfcn_svp_npu_model[model_index].model_mem_size = 0;
    }

    rfcn_svp_npu_model[model_index].is_load_flag = HI_FALSE;
    svp_trace_info("unload model SUCCESS, model id is %u!\n", rfcn_svp_npu_model[model_index].model_id);

    return;
}