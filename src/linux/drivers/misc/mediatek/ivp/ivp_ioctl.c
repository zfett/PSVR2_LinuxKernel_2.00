/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
 *         and Fish Wu <fish.wu@mediatek.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include <uapi/mediatek/ivp_ioctl.h>

#include "ivp_driver.h"
#include "ivp_fw_interface.h"
#include "ivp_queue.h"
#include "ivpbuf-core.h"
#include "ivpbuf-dma-memcpy.h"
#include "ivp_kservice.h"

/**
 * @file ivp_ioctl.c
 * Define 13 ioctl request.
 */


/** @ingroup IP_group_ivp_external_function
 * @par Description
 *     Service an ioctl request.
 * @param[in]
 *     flip: pointer to associated file object for device.
 * @param[in]
 *     cmd: the command code of ivp ioctl.
 * @param[out]
 *     arg: command argument/context including the information from user space.
 * @return
 *     0, successful to complete ioctl request. \n
 *     -ENOMEM, out of memory. \n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If it cannot alloc request or buffer memory, return -ENOMEM. \n
 *     If getting cmd handle failed, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
long ivp_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct ivp_user *user;
	struct ivp_device *ivp_device;
	struct ivp_core *ivp_core;
	struct device *dev;

	if (!flip || !flip->private_data)
		return -ENODEV;

	user = flip->private_data;
	ivp_device = dev_get_drvdata(user->dev);
	if (!ivp_device || !ivp_device->dev)
		return -ENODEV;

	dev = ivp_device->dev;
	switch (cmd) {
	case IVP_IOCTL_GET_CORE_NUM:{
			u32 core_num = ivp_device->ivp_core_num;

			ret =
			    copy_to_user((void *)arg, &core_num,
					 sizeof(u32));
			if (ret) {
				dev_err(dev,
					"[GET_CORE_NUM] return params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_GET_CORE_INFO:{
			u32 core = 0;
			struct ivp_fw_info fw_info;

			ret =
			    copy_from_user((void *)&fw_info, (void *)arg,
					   sizeof(struct ivp_fw_info));
			if (ret) {
				dev_err(dev,
					"[GET_INFO] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			if ((fw_info.core >= ivp_device->ivp_core_num) ||
			    (!ivp_device->ivp_core[fw_info.core])) {
				dev_err(dev, "[GET_INFO] core=%d error\n",
					fw_info.core);
				goto out;
			}

			core = fw_info.core;
			ivp_core = ivp_device->ivp_core[core];

			/* get fw version */
			fw_info.ver_major = ivp_core->ver_major;
			fw_info.ver_minor = ivp_core->ver_minor;

			/* get fw state */
			ret =
			    ivp_fw_get_ivp_status(ivp_core->ivpctrl_base,
						  &fw_info.state);

			ret =
			    copy_to_user((void *)arg, &fw_info,
					 sizeof(struct ivp_fw_info));
			if (ret) {
				dev_err(dev,
					"[GET_INFO] return params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_REQ_ENQUE:{
			struct ivp_request *req;
			struct ivp_cmd_enque cmd_enque;

			ret =
			    copy_from_user((void *)&cmd_enque, (void *)arg,
					   sizeof(struct ivp_cmd_enque));
			if (ret) {
				dev_err(dev,
					"[REQ_ENQUE] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			if (cmd_enque.cmd_type >= IVP_PROC_MAX ||
			    cmd_enque.core >= ivp_device->ivp_core_num) {
				dev_err(dev,
					"[REQ_ENQUE] cmd=%d core=%d error\n",
					cmd_enque.cmd_type, cmd_enque.core);
				goto out;
			}

			if (cmd_enque.cmd_type == IVP_PROC_LOAD_ALGO &&
			    cmd_enque.addr_handle) {
#if !defined(CONFIG_MTK_IVP_LOAD_ALGO_FROM_USER)
				dev_err(dev,
					"[REQ_ENQUE] not support load algo from user\n");
				goto out;
#endif
			}

			ret = ivp_alloc_request(&req);
			if (ret) {
				dev_err(dev,
					"[REQ_ENQUE] alloc request failed, ret=%d\n",
					ret);
				goto out;
			}
			req->user = user;

			ivp_setup_enqueue_request(req, &cmd_enque);

			if (ivp_enque_pre_process(ivp_device, req)) {
				dev_err(dev,
					"[REQ_ENQUE] process algo ctx error\n");
				ivp_free_request(req);
				goto out;
			}

			ret = ivp_push_request_to_queue(user, req);
			if (ret) {
				dev_err(dev,
					"[REQ_ENQUE] push to user's queue failed, ret=%d\n",
					ret);
				ivp_free_request(req);
				goto out;
			}

			ret = ivp_enque_post_process(ivp_device, req,
						     &cmd_enque);
			if (ret) {
				dev_err(dev,
					"[REQ_ENQUE] enqueue post process failed\n");
				ivp_free_request(req);
				goto out;
			}

			ret =
			    copy_to_user((void *)arg, (void *)&cmd_enque,
					 sizeof(struct ivp_cmd_enque));
			if (ret) {
				dev_err(dev,
					"[REQ_ENQUE] return params failed, ret=%d\n",
					ret);
				ivp_free_request(req);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_REQ_DEQUE:{
			struct ivp_request *req = NULL;
			struct ivp_cmd_deque cmd_deque;
			int status = IVP_REQ_STATUS_RUN;
			u32 core;

			ret =
			    copy_from_user((void *)&cmd_deque, (void *)arg,
					   sizeof(struct ivp_cmd_deque));
			if (ret) {
				dev_err(dev,
					"[REQ_DEQUE] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			core = cmd_deque.core;
			if ((cmd_deque.cmd_handle == 0) ||
			    (core >= ivp_device->ivp_core_num)) {
				dev_err(dev,
					"[REQ_DEQUE] get invalid cmd\n");
				ret = -EINVAL;
				goto out;
			}

			status =
			    ivp_pop_request_from_queue(cmd_deque.cmd_handle,
						       core, user, &req);
			if (req && status == IVP_REQ_STATUS_DEQUEUE) {
				cmd_deque.cmd_result = req->cmd_result;
				cmd_deque.cmd_status = req->cmd_status;
				cmd_deque.submit_cyc = req->submit_cyc;
				cmd_deque.execute_start_cyc =
					req->execute_start_cyc;
				cmd_deque.execute_end_cyc =
					req->execute_end_cyc;
				cmd_deque.submit_nsec = req->submit_nsec;
				cmd_deque.execute_start_nsec =
					req->execute_start_nsec;
				cmd_deque.execute_end_nsec =
					req->execute_end_nsec;

				ivp_deque_post_process(ivp_device, req);
				ivp_free_request(req);
			}

			ret =
			    copy_to_user((void *)arg, &cmd_deque,
					 sizeof(struct ivp_cmd_deque));
			if (ret) {
				dev_err(dev,
					"[REQ_DEQUE] return params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_FLUSH_QUE:{
			struct ivp_flush_que *flush_que;
			u32 core = 0;

			flush_que = (struct ivp_flush_que *)arg;
			ret = get_user(core, &flush_que->core);
			if (ret) {
				dev_err(dev,
					"[FLUSH_REQ] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			if (core >= ivp_device->ivp_core_num)
				goto out;

			ret = ivp_flush_requests_from_queue(core, user);
			if (ret) {
				dev_err(dev,
					"[FLUSH_REQ] flush request failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_ALLOC_BUF:{
			struct ivp_alloc_buf alloc_buf;

			ret =
			    copy_from_user((void *)&alloc_buf, (void *)arg,
					   sizeof(struct ivp_alloc_buf));
			if (ret) {
				dev_err(dev,
					"[ALLOC_BUF] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret = ivp_manage_alloc(ivp_device, &alloc_buf);
			if (ret) {
				dev_err(dev,
					"[ALLOC_BUF] alloc buf failed, ret=%d\n",
					ret);
				goto out;
			}

			ivp_add_dbg_buf(user, alloc_buf.handle);

			ret =
			    copy_to_user((void *)arg, (void *)&alloc_buf,
					 sizeof(struct ivp_alloc_buf));
			if (ret) {
				dev_err(dev,
					"[ALLOC_BUF] update params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_MMAP_BUF:{
			struct ivp_mmap_buf mmap_buf;

			ret =
			    copy_from_user((void *)&mmap_buf, (void *)arg,
					   sizeof(struct ivp_mmap_buf));
			if (ret) {
				dev_err(dev,
					"[QUERY_BUF] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret = ivp_manage_query(ivp_device->ivp_mng, &mmap_buf);
			if (ret) {
				dev_err(dev,
					"[QUERY_BUF] query buf failed, ret=%d\n",
					ret);
				goto out;
			}

			ret =
			    copy_to_user((void *)arg, (void *)&mmap_buf,
					 sizeof(struct ivp_mmap_buf));
			if (ret) {
				dev_err(dev,
					"[QUERY_BUF] update params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_EXPORT_BUF_TO_FD:{
			struct ivp_export_buf_to_fd export_buf;

			ret =
			    copy_from_user((void *)&export_buf, (void *)arg,
					   sizeof(struct ivp_export_buf_to_fd));
			if (ret) {
				dev_err(dev,
					"[EXPORT_BUF] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret =
			    ivp_manage_export(ivp_device->ivp_mng, &export_buf);
			if (ret) {
				dev_err(dev,
					"[EXPORT_BUF] export buf failed, ret=%d\n",
					ret);
				goto out;
			}

			ret =
			    copy_to_user((void *)arg, (void *)&export_buf,
					 sizeof(struct ivp_export_buf_to_fd));
			if (ret) {
				dev_err(dev,
					"[EXPORT_BUF] update params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_IMPORT_FD_TO_BUF:{
			struct ivp_import_fd_to_buf import_buf;

			ret =
			    copy_from_user((void *)&import_buf, (void *)arg,
					   sizeof(struct ivp_import_fd_to_buf));
			if (ret) {
				dev_err(dev,
					"[IMPORT_BUF] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret =
			    ivp_manage_import(ivp_device->ivp_mng, &import_buf);
			if (ret) {
				dev_err(dev,
					"[IMPORT_BUF] import buf failed, ret=%d\n",
					ret);
				goto out;
			}

			ivp_add_dbg_buf(user, import_buf.handle);

			ret =
			    copy_to_user((void *)arg, &import_buf,
					 sizeof(struct ivp_import_fd_to_buf));
			if (ret) {
				dev_err(dev,
					"[IMPORT_BUF] update params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_FREE_BUF:{
			struct ivp_free_buf free_buf;

			ret =
			    copy_from_user((void *)&free_buf, (void *)arg,
					   sizeof(struct ivp_free_buf));
			if (ret) {
				dev_err(dev,
					"[FREE_BUF] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret = ivp_manage_free(ivp_device->ivp_mng, &free_buf);
			if (ret) {
				dev_err(dev,
					"[FREE_BUF] free buf failed, ret=%d\n",
					ret);
				goto out;
			}

			ivp_delete_dbg_buf(user, free_buf.handle);

			ret =
			    copy_to_user((void *)arg, (void *)&free_buf,
					 sizeof(struct ivp_free_buf));
			if (ret) {
				dev_err(dev,
					"[FREE_BUF] update params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_SYNC_CACHE:{
			struct ivp_sync_cache_buf sync_buf;

			ret =
			    copy_from_user((void *)&sync_buf, (void *)arg,
					   sizeof(struct ivp_sync_cache_buf));
			if (ret) {
				dev_err(dev,
					"[SYNC_CACHE_BUF] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret = ivp_manage_sync_cache(ivp_device->ivp_mng,
						    &sync_buf);
			if (ret) {
				dev_err(dev,
					"[SYNC_CACHE_BUF] flush buf failed, ret=%d\n",
					ret);
				goto out;
			}

			ret =
			    copy_to_user((void *)arg, (void *)&sync_buf,
					 sizeof(struct ivp_sync_cache_buf));
			if (ret) {
				dev_err(dev,
					"[SYNC_CACHE_BUF] update params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_ENQUE_DMA_REQ:{
			struct ivp_dma_memcpy_buf cpy_buf;

			ret = copy_from_user((void *)&cpy_buf, (void *)arg,
					     sizeof(struct ivp_dma_memcpy_buf));
			if (ret ||
			    cpy_buf.core >= ivp_device->ivp_core_num) {
				dev_err(dev,
					"[DMA_MEMCPY_ENQUE] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret = ivpdma_submit_request(ivp_device, &cpy_buf);
			if (ret == -EPERM) {
				dev_err(dev, "[DMA_MEMCPY_ENQUE] cqidma not ready\n");
				return ret;
			}
			if (ret) {
				dev_err(dev,
					"[DMA_MEMCPY_ENQUE] ivp_dma_memcpy failed, ret=%d\n",
					ret);
				goto out;
			}

			ret =
			    copy_to_user((void *)arg, (void *)&cpy_buf,
					 sizeof(struct ivp_dma_memcpy_buf));
			if (ret) {
				dev_err(dev,
					"[DMA_MEMCPY_ENQUE] update params failed, ret=%d\n",
					ret);
				goto out;
			}
			break;
		}
	case IVP_IOCTL_DEQUE_DMA_REQ:{
			struct ivp_cmd_deque buf;

			ret = copy_from_user((void *)&buf, (void *)arg,
					     sizeof(struct ivp_cmd_deque));
			if (ret) {
				dev_err(dev,
					"[DMA_MEMCPY_DEQUE] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret = ivpdma_check_request_status(ivp_device,
							  buf.cmd_handle);
			buf.cmd_result = ret;

			ret = copy_to_user((void *)arg, (void *)&buf,
					    sizeof(struct ivp_cmd_deque));
			if (ret) {
				dev_err(dev,
					"[DMA_MEMCPY_DEQUE] update params failed, ret=%d\n",
					ret);
				goto out;
			}
			break;
		}

	case IVP_IOCTL_KSERVICE_REQ:{
			struct ivp_kservice_req req;

			ret =
			    copy_from_user((void *)&req, (void *)arg,
					   sizeof(struct ivp_kservice_req));
			if (ret) {
				dev_err(dev,
					"[LOCK_KSERVICE] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			if ((req.core >= ivp_device->ivp_core_num) ||
			    (!ivp_device->ivp_core[req.core])) {
				ret = -EINVAL;
				goto out;
			}

			ivp_core = ivp_device->ivp_core[req.core];
			if (req.lock == 1)
				ret = ivp_lock_kservice(ivp_core);
			else if (req.lock == 0)
				ret = ivp_unlock_kservice(ivp_core);
			else
				ret = -EINVAL;

			if (ret) {
				dev_err(dev,
					"[LOCK_KSERVICE] lock failed, ret=%d\n",
					ret);
				goto out;
			}
			break;
		}

	case IVP_IOCTL_KSERVICE_REQ_PERPARE:{
			struct ivp_kservice_request *kervice_req;
			struct ivp_request *req;
			struct ivp_cmd_enque cmd_enque;

			ret =
			    copy_from_user((void *)&cmd_enque, (void *)arg,
					   sizeof(struct ivp_cmd_enque));
			if (ret) {
				dev_err(dev,
					"[KSERVICE] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			if (cmd_enque.cmd_type >= IVP_PROC_MAX ||
			    cmd_enque.core >= ivp_device->ivp_core_num) {
				dev_err(dev,
					"[REQ_ENQUE] cmd=%d core=%d error\n",
					cmd_enque.cmd_type, cmd_enque.core);
				goto out;
			}

			ret = ivp_alloc_kservice_request(&kervice_req,
							 ivp_device);
			if (ret) {
				dev_err(dev,
					"[KSERVICE] alloc request failed, ret=%d\n",
					ret);
				goto out;
			}

			req = &kervice_req->base;
			req->user = user;
			ivp_setup_enqueue_request(req, &cmd_enque);

			if (ivp_enque_pre_process(ivp_device, req)) {
				dev_err(dev,
					"[KSERVICE] process algo ctx error\n");
				ivp_free_kservice_request(kervice_req);
				goto out;
			}

			ret = ivp_push_request_to_kservice_queue(kervice_req,
								 ivp_device);
			if (ret) {
				dev_err(dev,
					"[KSERVICE] push to queue failed, ret=%d\n",
					ret);
				ivp_free_kservice_request(kervice_req);
				goto out;
			}

			ret = ivp_enque_post_process(ivp_device, req,
						     &cmd_enque);
			if (ret) {
				dev_err(dev,
					"[REQ_ENQUE] enqueue post process failed\n");
				ivp_free_kservice_request(kervice_req);
				goto out;
			}

			ret =
			    copy_to_user((void *)arg, (void *)&cmd_enque,
					 sizeof(struct ivp_cmd_enque));
			if (ret) {
				dev_err(dev,
					"[REQ_ENQUE] return params failed, ret=%d\n",
					ret);
				ivp_free_kservice_request(kervice_req);
				goto out;
			}

			break;
		}

	case IVP_IOCTL_KSERVICE_REQ_UNPERPARE:{
			struct ivp_kservice_request *kervice_req;
			struct ivp_cmd_deque cmd_deque;
			u32 core;

			ret =
			    copy_from_user((void *)&cmd_deque, (void *)arg,
					   sizeof(struct ivp_cmd_deque));
			if (ret) {
				dev_err(dev,
					"[REQ_DEQUE] get params failed, ret=%d\n",
					ret);
				goto out;
			}

			core = cmd_deque.core;
			if ((cmd_deque.cmd_handle == 0) ||
			    (core >= ivp_device->ivp_core_num)) {
				dev_err(dev,
					"[REQ_DEQUE] get invalid cmd\n");
				ret = -EINVAL;
				goto out;
			}

			ret =
			    ivp_pop_request_from_kservice_queue(
						cmd_deque.cmd_handle,
						core, ivp_device,
						&kervice_req);
			if (kervice_req)
				ivp_free_kservice_request(kervice_req);

			ret =
			    copy_to_user((void *)arg, &cmd_deque,
					 sizeof(struct ivp_cmd_deque));
			if (ret) {
				dev_err(dev,
					"[REQ_DEQUE] return params failed, ret=%d\n",
					ret);
				goto out;
			}

			break;
		}
	case IVP_IOCTL_KSERVICE_AUTO_TEST:{
			u32 core_num;

			ret =
			    copy_from_user(&core_num, (void *)arg,
					   sizeof(u32));
			if (ret || core_num >= ivp_device->ivp_core_num) {
				dev_err(dev,
					"[AUTO_TEST] return params failed, ret=%d\n",
					ret);
				goto out;
			}

			ret = ivp_test_auto_run(core_num, ivp_device);
			break;
		}
	default:
		dev_warn(dev, "ioctl: no such command!\n");
		ret = -EINVAL;
		break;
	}
out:
	if (ret) {
		dev_err(dev,
			"fail, cmd(%d), pid(%d), (process, pid, tgid)=(%s, %d, %d)\n",
			cmd, user->open_pid, current->comm, current->pid,
			current->tgid);
	}

	return ret;
}
