/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Fish Wu <fish.wu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file ivp_queue.c
 * Handle IVP queue thread and user.
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
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/wait.h>

#include "ivp_cmd_hnd.h"
#include "ivp_driver.h"
#include "ivp_dvfs.h"
#include "ivp_fw_interface.h"
#include "ivp_queue.h"
#include "ivp_systrace.h"
#include "ivp_utilization.h"
#include "ivpbuf-core.h"

DECLARE_VLIST(ivp_user);
DECLARE_VLIST(ivp_request);

/* need mutex protect before use (ivp_device->user_mutex) */
static bool users_queue_are_empty(u32 core, struct ivp_device *ivp_device)
{
	struct list_head *head;
	struct ivp_user *user;
	bool is_empty = true;

	list_for_each(head, &ivp_device->user_list) {
		user = vlist_node_of(head, struct ivp_user);
		if (!list_empty(&user->enque_list[core])) {
			is_empty = false;
			break;
		}
	}

	return is_empty;
}

int ivp_create_user(struct ivp_user **user, struct ivp_device *ivp_device)
{
	struct ivp_user *u;
	u32 core = 0;

	u = kzalloc(sizeof(vlist_type(struct ivp_user)), GFP_KERNEL);
	if (!u)
		return -ENOMEM;

	u->dev = ivp_device->dev;
	u->id = ++(ivp_device->ivp_num_users);
	u->open_pid = current->pid;
	u->open_tgid = current->tgid;

	for (core = 0; core < IVP_CORE_NUM; core++) {
		mutex_init(&u->data_mutex[core]);
		INIT_LIST_HEAD(&u->enque_list[core]);
		INIT_LIST_HEAD(&u->deque_list[core]);
		init_waitqueue_head(&u->deque_wait[core]);
		mutex_init(&u->algo_mutex[core]);
		idr_init(&u->algo_idr[core]);
		idr_init(&u->req_idr[core]);
	}
	init_waitqueue_head(&u->poll_wait_queue);
	mutex_init(&u->dbgbuf_mutex);
	INIT_LIST_HEAD(&u->dbgbuf_list);


	mutex_lock(&ivp_device->user_mutex);
	list_add_tail(vlist_link(u, struct ivp_user), &ivp_device->user_list);
	mutex_unlock(&ivp_device->user_mutex);

	*user = u;
	return 0;
}

int ivp_delete_user(struct ivp_user *user, struct ivp_device *ivp_device)
{
	struct list_head *head, *temp;
	struct ivp_request *req;
	struct ivp_algo_context *algo_ctx;
	struct ivp_core *ivp_core;
	void *entry;
	u32 core = 0, id, cnt = 0;
	bool running = false;

	for (core = 0; core < IVP_CORE_NUM; core++) {
		ivp_flush_requests_from_queue(core, user);

		/* clear the list of deque */
		mutex_lock(&user->data_mutex[core]);
		list_for_each_safe(head, temp, &user->deque_list[core]) {
			req = vlist_node_of(head, struct ivp_request);
			list_del(head);
			ivp_free_request(req);
		}
		mutex_unlock(&user->data_mutex[core]);

		mutex_lock(&user->algo_mutex[core]);
		idr_for_each_entry(&user->algo_idr[core], entry, id) {
			algo_ctx = entry;
			ivp_core = ivp_device->ivp_core[algo_ctx->core];
			ivp_free_algo(user, algo_ctx);
			ivp_core_set_auto_suspend(ivp_core, 0);
			idr_remove(&user->algo_idr[core], id);
			kfree(algo_ctx);
		}
		mutex_unlock(&user->algo_mutex[core]);
		idr_destroy(&user->req_idr[core]);
		idr_destroy(&user->algo_idr[core]);
	}

	mutex_lock(&ivp_device->user_mutex);
	list_del(vlist_link(user, struct ivp_user));
	mutex_unlock(&ivp_device->user_mutex);

	do {
		running = false;
		for (core = 0; core < IVP_CORE_NUM; core++)
			running = running || user->running[core];
		if (cnt > ivp_device->timeout) {
			pr_err("%s: waiting user(%d) to stop running timeout\n",
			       __func__, user->open_pid);
			break;
		}
		usleep_range(100, 1000);
		cnt++;
	} while (running);
	kfree(user);

	return 0;
}

static int ivp_enque_handler(struct ivp_request *req,
			     struct ivp_device *ivp_device)
{
	struct ivp_core *ivp_core = ivp_device->ivp_core[req->core];
	int ret = 0;

	if (ivp_core->state == IVP_UNPREPARE) {
		ivp_reset_hw(ivp_core);
		ivp_reset_algo_ctx(ivp_core);
		ivp_core->state = IVP_IDLE;
	}

	if (!req->duration)
		req->duration = ivp_device->timeout;

	ivp_utilization_compute_enter(ivp_device, req->core);

	req->execute_start_cyc = ivp_read_counter(ivp_core);
	req->execute_start_nsec = ktime_get().tv64;

	switch (req->cmd) {
	case IVP_PROC_EXEC_ALGO:
		req->cmd_status = IVP_REQ_STATUS_RUN;
		ivp_atrace_begin("ivp_exec_algo");
		ret = ivp_exec_algo(req, ivp_device);
		ivp_atrace_end("ivp_exec_algo");
		break;
	case IVP_PROC_LOAD_ALGO:
		req->cmd_status = IVP_REQ_STATUS_RUN;
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
		if (!req->addr_handle)
			ret = ivp_request_algo(req, ivp_device);
#endif
		if (!ret) {
			ivp_atrace_begin("ivp_load_algo");
			ret = ivp_load_algo(req, ivp_device);
			ivp_atrace_end("ivp_load_algo");
		}
		break;
	case IVP_PROC_UNLOAD_ALGO:
		req->cmd_status = IVP_REQ_STATUS_RUN;
		ret = ivp_unload_algo(req, ivp_device);
		break;
	case IVP_PROC_RESET:
		break;
	default:
		pr_warn("%s: bad command!\n", __func__);
		ret = -EINVAL;
	}

	req->execute_end_cyc = ivp_read_counter(ivp_core);
	req->execute_end_nsec = ktime_get().tv64;

	ivp_utilization_compute_leave(ivp_device, req->core);

	if (req->cmd == IVP_PROC_UNLOAD_ALGO ||
	    (req->cmd == IVP_PROC_LOAD_ALGO && req->cmd_result))
		ivp_core_set_auto_suspend(ivp_core, 0);
	else if ((ret == -ETIMEDOUT) && (ivp_device->timeout2reset)) {
		ivp_reset_hw(ivp_core);
		ivp_reset_algo_ctx(ivp_core);
		ivp_core->state = IVP_IDLE;
	}

	return ret;
}

int ivp_enque_routine_loop(void *arg)
{
	struct ivp_core *ivp_core = (struct ivp_core *)arg;
	struct list_head *head;
	struct ivp_user *user;
	struct ivp_request *req;
	u32 core = ivp_core->core;
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	for (; !kthread_should_stop();) {
		/* wait for requests if there is no one in user's queue */
		add_wait_queue(&ivp_core->req_wait, &wait);
		while (1) {
			mutex_lock(&ivp_device->user_mutex);
			if (!users_queue_are_empty(core, ivp_device)) {
				mutex_unlock(&ivp_device->user_mutex);
				break;
			}
			mutex_unlock(&ivp_device->user_mutex);
			if (ivp_core->state != IVP_UNPREPARE)
				wait_woken(&wait, TASK_INTERRUPTIBLE,
					   MAX_SCHEDULE_TIMEOUT);
		}
		remove_wait_queue(&ivp_core->req_wait, &wait);

		/* consume the user's queue */
		mutex_lock(&ivp_device->user_mutex);
		list_for_each(head, &ivp_device->user_list) {
			user = vlist_node_of(head, struct ivp_user);
			mutex_lock(&user->data_mutex[core]);
			/* thread will handle the remaining queue if flush */
			if (user->flush ||
			    list_empty(&user->enque_list[core])) {
				mutex_unlock(&user->data_mutex[core]);
				continue;
			}

			/* get first node from enque list */
			req =
			    vlist_node_of(user->enque_list[core].next,
					  struct ivp_request);
			list_del_init(vlist_link(req, struct ivp_request));
			user->running[core] = true;
			mutex_unlock(&ivp_device->user_mutex);
			ivp_enque_handler(req, ivp_device);
			mutex_lock(&ivp_device->user_mutex);
			list_add_tail(vlist_link(req, struct ivp_request),
				      &user->deque_list[core]);
			wake_up_interruptible_all(&user->poll_wait_queue);
			user->running[core] = false;
			mutex_unlock(&user->data_mutex[core]);

			wake_up_interruptible_all(&user->deque_wait[core]);
			if (ivp_core->state == IVP_SLEEP)
				break;
		}
		mutex_unlock(&ivp_device->user_mutex);
	}

	return 0;
}

int ivp_alloc_request(struct ivp_request **rreq)
{
	struct ivp_request *req;

	req = kzalloc(sizeof(vlist_type(struct ivp_request)), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	*rreq = req;

	return 0;
}

int ivp_free_request(struct ivp_request *req)
{
	kfree(req);

	return 0;
}

void ivp_setup_enqueue_request(struct ivp_request *req,
			       struct ivp_cmd_enque *cmd_enque)
{
	switch (cmd_enque->cmd_type) {
	case IVP_PROC_LOAD_ALGO:
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
		if (!cmd_enque->addr_handle &&
		    cmd_enque->algo_name[0] != '\0') {
			strncpy(req->algo_name, cmd_enque->algo_name,
				strlen(cmd_enque->algo_name));
		}
		/* fallthrough */
#endif
	case IVP_PROC_EXEC_ALGO:
	case IVP_PROC_RESET:
	case IVP_PROC_UNLOAD_ALGO:
		req->handle = idr_alloc(&req->user->req_idr[cmd_enque->core], req, 1, 0, GFP_KERNEL);
		req->cmd = cmd_enque->cmd_type;
		req->core = cmd_enque->core;
		req->drv_algo_hnd = cmd_enque->algo_hnd;
		req->argv = cmd_enque->argv;
		req->addr_handle = cmd_enque->addr_handle;
		req->size = cmd_enque->size;
		req->duration = cmd_enque->duration;
		req->cmd_result = 0;
		req->cmd_status = IVP_REQ_STATUS_ENQUEUE;
		break;
	}
}

int ivp_enque_pre_process(struct ivp_device *ivp_device,
			  struct ivp_request *req)
{
	int ret = 0;

	switch (req->cmd) {
	case IVP_PROC_LOAD_ALGO:
		ret = ivp_create_algo_ctx(ivp_device, req);
		break;
	default:
		break;
	}

	return ret;
}

int ivp_enque_post_process(struct ivp_device *ivp_device,
	struct ivp_request *req, struct ivp_cmd_enque *cmd_enque)
{
	int ret = 0;

	cmd_enque->cmd_handle = req->handle;
	cmd_enque->algo_hnd = req->drv_algo_hnd;

	return ret;
}

void ivp_deque_post_process(struct ivp_device *ivp_device,
			    struct ivp_request *req)
{
	switch (req->cmd) {
	case IVP_PROC_LOAD_ALGO:
		if (req->cmd_result != 0)
			ivp_free_algo_ctx(req);
		break;
	case IVP_PROC_UNLOAD_ALGO:
		if (req->cmd_result == 0)
			ivp_free_algo_ctx(req);
		break;
	default:
		break;
	}
}

int ivp_create_algo_ctx(struct ivp_device *ivp_device,
			struct ivp_request *req)
{
	struct ivp_algo_context *algo_ctx;
	struct ivp_user *ivp_user = req->user;

	algo_ctx = kzalloc(sizeof(struct ivp_algo_context), GFP_KERNEL);
	if (!algo_ctx)
		return -ENOMEM;

	/* fill in the algo info */
	mutex_lock(&ivp_user->algo_mutex[req->core]);
	algo_ctx->drv_algo_hnd
		= idr_alloc(&ivp_user->algo_idr[req->core], algo_ctx, 1, 0,
			    GFP_KERNEL);
	mutex_unlock(&ivp_user->algo_mutex[req->core]);

	algo_ctx->core = req->core;
	algo_ctx->state = IVP_ALGO_STATE_INIT;
	algo_ctx->algo_buf_hnd = 0;
	algo_ctx->algo_load_duration = req->duration;

	/* return drv_algo_hnd to user mode */
	req->drv_algo_hnd = algo_ctx->drv_algo_hnd;

	return 0;
}

void ivp_free_algo_ctx(struct ivp_request *req)
{
	struct ivp_algo_context *algo_ctx;
	struct ivp_user *ivp_user = req->user;

	algo_ctx = idr_find(&ivp_user->algo_idr[req->core], req->drv_algo_hnd);

	mutex_lock(&ivp_user->algo_mutex[req->core]);
	idr_remove(&ivp_user->algo_idr[req->core], req->drv_algo_hnd);
	kfree(algo_ctx);
	mutex_unlock(&ivp_user->algo_mutex[req->core]);
}

int ivp_show_algo(struct ivp_device *ivp_device, char *buf, int count)
{
	struct ivp_user *user;
	struct list_head *user_head;
	struct ivp_algo_context *algo_ctx;
	void *entry;
	int core, id;

	list_for_each(user_head, &ivp_device->user_list) {
		user = vlist_node_of(user_head, struct ivp_user);
		for (core = 0; core < ivp_device->ivp_core_num; core++) {
			mutex_lock(&user->algo_mutex[core]);
			idr_for_each_entry(&user->algo_idr[core], entry, id) {
				algo_ctx = entry;
				count += scnprintf(buf + count, PAGE_SIZE,
						   "-------------------------------\n");
				count += scnprintf(buf + count, PAGE_SIZE,
						   "user-pid:%d\n",
						   user->open_pid);
				count += scnprintf(buf + count, PAGE_SIZE,
						   "core:0x%x\n",
						   algo_ctx->core);
				count += scnprintf(buf + count, PAGE_SIZE,
						   "drv_algo_hnd:%d\n",
						   algo_ctx->drv_algo_hnd);
				count += scnprintf(buf + count, PAGE_SIZE,
						   "fw_algo_hnd:0x%x\n",
						   algo_ctx->fw_algo_hnd);
				count += scnprintf(buf + count, PAGE_SIZE,
						   "algo_flags:0x%x(%s)\n",
						   algo_ctx->algo_flags,
						   algo_ctx->algo_flags ?
						   "fix" : "dyn");
				count += scnprintf(buf + count, PAGE_SIZE,
						   "code_mem:0x%x\n",
						   algo_ctx->algo_code_mem);
				count += scnprintf(buf + count, PAGE_SIZE,
						   "data_mem:0x%x\n",
						   algo_ctx->algo_data_mem);
			}
			mutex_unlock(&user->algo_mutex[core]);
		}
	}

	return count;
}

/* need mutex protect before use (ivp_device->user_mutex) */
void ivp_reset_algo_ctx(struct ivp_core *ivp_core)
{
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	struct ivp_user *user;
	struct list_head *user_head;
	struct ivp_algo_context *algo_ctx;
	void *entry;
	int id;

	list_for_each(user_head, &ivp_device->user_list) {
		user = vlist_node_of(user_head, struct ivp_user);
		mutex_lock(&user->algo_mutex[ivp_core->core]);
		idr_for_each_entry(&user->algo_idr[ivp_core->core], entry, id) {
			algo_ctx = entry;
			if (algo_ctx->core == ivp_core->core) {
				algo_ctx->state = IVP_ALGO_STATE_RESET;
				pr_debug("%s, algo_ctx->fw_algo_hnd=%x\n",
					__func__, algo_ctx->fw_algo_hnd);
			}
		}
		mutex_unlock(&user->algo_mutex[ivp_core->core]);
	}
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Push a request into IVP command enqueue list.
 * @param[in]
 *     user: pointer to the user that call a request.
 * @param[in]
 *     req: pointer to the request that have summit to process.
 * @return
 *     0, successful to push request. \n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If enter an empty user, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_push_request_to_queue(struct ivp_user *user,
			      struct ivp_request *req)
{
	u32 core = req->core;
	struct ivp_device *ivp_device;
	struct ivp_core *ivp_core;
	int ret = 0;

	if (!user) {
		pr_err("empty user");
		return -EINVAL;
	}

	ivp_device = dev_get_drvdata(user->dev);
	ivp_core = ivp_device->ivp_core[core];

	if (req->cmd == IVP_PROC_LOAD_ALGO) {
		ret = pm_runtime_get_sync(ivp_core->dev);
		if (ret < 0) {
			dev_err(ivp_core->dev,
				"%s: pm_runtime_get_sync fail.\n", __func__);
			return ret;
		}
	}

	req->submit_cyc = ivp_read_counter(ivp_core);
	req->submit_nsec = ktime_get().tv64;

	mutex_lock(&user->data_mutex[core]);
	list_add_tail(vlist_link(req, struct ivp_request),
		      &user->enque_list[core]);
	mutex_unlock(&user->data_mutex[core]);

	wake_up(&ivp_core->req_wait);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Pop a request from IVP command dequeue list.
 * @param[in]
 *     handle: command handle.
 * @param[in]
 *     core: core id of IVP device.
 * @param[in]
 *     user: pointer to the user that query a result of request.
 * @param[out]
 *     rreq: pointer of pointer to request that have summit to process.
 * @return
 *     return the status of request.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_pop_request_from_queue(s32 handle, u32 core,
			       struct ivp_user *user,
			       struct ivp_request **rreq)
{
	struct list_head *head, *temp;
	struct ivp_request *req;
	int status = IVP_REQ_STATUS_RUN;

	mutex_lock(&user->data_mutex[core]);
	/* This part should not be happened */
	if (list_empty(&user->deque_list[core])) {
		mutex_unlock(&user->data_mutex[core]);
		*rreq = NULL;
		return status;
	};

	status = IVP_REQ_STATUS_INVALID;
	list_for_each_safe(head, temp, &user->deque_list[core]) {
		req = vlist_node_of(head, struct ivp_request);
		if (req->handle == handle) {
			list_del_init(vlist_link(req, struct ivp_request));
			idr_remove(&user->req_idr[core], handle);
			status = IVP_REQ_STATUS_DEQUEUE;
			req->cmd_status = IVP_REQ_STATUS_DEQUEUE;
			break;
		}
	}
	mutex_unlock(&user->data_mutex[core]);
	*rreq = req;

	return status;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Flush IVP command enqueue list.\n
 *     Wait processing request finish and move it to the dequeue list.\n
 *     Move the rest requests to depueue list and mark error.
 * @param[in]
 *     core: core id of IVP device, denote which queue to flush.
 * @param[out]
 *     user: pointer to the user, one of the user's queue will be flushed.
 * @return
 *     0, successful to flush request.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_flush_requests_from_queue(u32 core, struct ivp_user *user)
{
	struct list_head *head, *temp;
	struct ivp_request *req;

	mutex_lock(&user->data_mutex[core]);

	if (!user->running[core] && list_empty(&user->enque_list[core])) {
		mutex_unlock(&user->data_mutex[core]);
		return 0;
	}

	user->flush = true;
	mutex_unlock(&user->data_mutex[core]);

	/* the running request will add to the deque before interrupt */
	wait_event_interruptible(user->deque_wait[core], !user->running[core]);

	mutex_lock(&user->data_mutex[core]);
	/* push the remaining enque to the deque */
	list_for_each_safe(head, temp, &user->enque_list[core]) {
		req = vlist_node_of(head, struct ivp_request);
		req->cmd_status = IVP_REQ_STATUS_FLUSH;
		list_del_init(head);
		list_add_tail(head, &user->deque_list[core]);
	}

	user->flush = false;
	mutex_unlock(&user->data_mutex[core]);

	return 0;
}

