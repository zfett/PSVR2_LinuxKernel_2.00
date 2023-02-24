/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Mao Lin <Zih-Ling.Lin@mediatek.com>
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include "ivpbuf-core.h"
#include "ivp_cmd_hnd.h"
#include "ivp_fw_interface.h"
#include "ivp_kservice.h"
#include "ivp_reg.h"
#include "ivp_queue.h"
#include "ivp_utilization.h"

DECLARE_VLIST(ivp_kservice_request);

int ivp_lock_kservice(struct ivp_core *ivp_core)
{
	if (!ivp_core || ivp_core->kservice_state != IVP_KSERVICE_STOP)
		return -EINVAL;

	ivp_core->kservice_state = IVP_KSERVICE_INIT;
	return 0;
}

int ivp_unlock_kservice(struct ivp_core *ivp_core)
{
	if (!ivp_core)
		return -EINVAL;

	ivp_core->kservice_state = IVP_KSERVICE_STOP;
	return 0;
}

int ivp_alloc_kservice_request(struct ivp_kservice_request **rreq,
			       struct ivp_device *ivp_device)
{
	struct ivp_kservice_request *req;
	unsigned int *req_addr;

	req = kzalloc(sizeof(vlist_type(struct ivp_kservice_request)),
		      GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	req_addr = (unsigned int *)&req;
	req->submit_stamp = (*req_addr & 0xffff) << 16;
	*rreq = req;

	return 0;
}

int ivp_free_kservice_request(struct ivp_kservice_request *req)
{
	kfree(req);

	return 0;
}


int ivp_push_request_to_kservice_queue(struct ivp_kservice_request *kreq,
				       struct ivp_device *ivp_device)
{
	struct ivp_request *req = &kreq->base;
	struct ivp_core *ivp_core = ivp_device->ivp_core[req->core];

	mutex_lock(&ivp_core->kservice_mutex);
	list_add_tail(vlist_link(kreq, struct ivp_kservice_request),
		      &ivp_core->kservice_request_list);
	mutex_unlock(&ivp_core->kservice_mutex);

	return 0;
}

static int
ivp_validate_request_from_kservice_queue(u64 handle, u32 core,
					 struct ivp_device *ivp_device,
					 struct ivp_kservice_request **rreq)
{
	struct list_head *head, *temp;
	struct ivp_kservice_request *req;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];

	/* This part should not be happened */
	if (list_empty(&ivp_core->kservice_request_list)) {
		*rreq = NULL;
		return -EINVAL;
	}

	list_for_each_safe(head, temp, &ivp_core->kservice_request_list) {
		req = vlist_node_of(head, struct ivp_kservice_request);
		if (req->base.handle == handle)
			break;
	}
	if ((req->submit_stamp & 0xffff) == 0xffff)
		req->submit_stamp = req->submit_stamp & 0xffff0000;
	else
		req->submit_stamp = req->submit_stamp + 1;
	*rreq = req;

	return 0;
}

int ivp_pop_request_from_kservice_queue(u64 handle, u32 core,
					struct ivp_device *ivp_device,
					struct ivp_kservice_request **rreq)
{
	struct ivp_kservice_request *req;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	int ret = 0;

	mutex_lock(&ivp_core->kservice_mutex);

	ret = ivp_validate_request_from_kservice_queue(handle, core,
						       ivp_device, rreq);
	if (ret) {
		mutex_unlock(&ivp_core->kservice_mutex);
		return ret;
	}

	req = *rreq;
	list_del_init(vlist_link(req, struct ivp_kservice_request));

	mutex_unlock(&ivp_core->kservice_mutex);

	return ret;
}

static inline void lock_command(struct ivp_core *ivp_core)
{
	mutex_lock(&ivp_core->cmd_mutex);
	ivp_core->is_cmd_done = false;
}

static inline int wait_command(struct ivp_core *ivp_core, u32 timeout)
{
	return (wait_event_interruptible_timeout(ivp_core->cmd_wait,
						 ivp_core->is_cmd_done,
						 msecs_to_jiffies
						 (timeout)) > 0)
	    ? 0 : -ETIMEDOUT;
}

static inline int check_command(IVP_UINT32 timestamp,
				void __iomem *ivpctrl_base)
{
	IVP_UINT32 tmp;

	tmp = (IVP_UINT32)ivp_read_reg32(ivpctrl_base,
					 IVP_CTRL_REG_XTENSA_INFO23);

	return (tmp == timestamp) ? 0 : -EINVAL;
}

static inline void unlock_command(struct ivp_core *ivp_core)
{
	mutex_unlock(&ivp_core->cmd_mutex);
}

static int ivp_check_cmd_send(struct ivp_core *ivp_core,
			      IVP_UINT32 timestamp)
{
	struct ivp_device *ivp_device = ivp_core->ivp_device;
	void __iomem *ivpctrl_base = ivp_core->ivpctrl_base;
	ktime_t time = ktime_get();
	bool is_timeout = false;

	while (check_command(timestamp, ivpctrl_base)) {
		if (ktime_ms_delta(ktime_get(), time) >
		    ivp_device->timeout) {
			enum ivp_state fw_state =
				(enum ivp_state)ivp_read_reg32(ivpctrl_base,
						IVP_CTRL_REG_XTENSA_INFO00);

			if (is_timeout)
				return -ETIMEDOUT;
			if (fw_state == STATE_WAIT_ISR) {
				time = ktime_get();
				ivp_write_reg32(ivpctrl_base,
						IVP_CTRL_REG_XTENSA_INFO15,
						timestamp);

				ivp_set_reg32(ivpctrl_base,
					      IVP_CTRL_REG_CTL_XTENSA_INT,
					      (0x1 << INT00_LEVEL_L1));
			}

			is_timeout = true;
		}
	}

	return 0;
}

static int
ivp_fw_exec_algo_with_gce(struct apmcu_exec_cmd *exec_cmd,
			  struct ivp_core *ivp_core,
			  struct cmdq_pkt *pkt)
{
	int ret;
	u32 base = ivp_core->trg.reg_base & 0xffff;

	ret = cmdq_pkt_write_value(pkt, ivp_core->trg.subsys,
				   base | IVP_CTRL_REG_XTENSA_INFO16,
				   exec_cmd->apmcu_cmd, 0xffffffff);

	ret |= cmdq_pkt_write_value(pkt, ivp_core->trg.subsys,
				    base | IVP_CTRL_REG_XTENSA_INFO17,
				    exec_cmd->algo_hnd, 0xffffffff);

	ret |= cmdq_pkt_write_value(pkt, ivp_core->trg.subsys,
				    base | IVP_CTRL_REG_XTENSA_INFO18,
				    exec_cmd->algo_arg_size, 0xffffffff);

	ret |= cmdq_pkt_write_value(pkt, ivp_core->trg.subsys,
				    base | IVP_CTRL_REG_XTENSA_INFO19,
				    exec_cmd->algo_arg_base, 0xffffffff);

	ret |= cmdq_pkt_write_value(pkt, ivp_core->trg.subsys,
				    base | IVP_CTRL_REG_XTENSA_INFO15,
				    exec_cmd->timestamp, 0xffffffff);

	if (ret)
		return ret;

	/* trigger dsp */
	ret = cmdq_pkt_write_value(pkt, ivp_core->trg.subsys,
				   base | IVP_CTRL_REG_CTL_XTENSA_INT,
				   (0x1 << INT00_LEVEL_L1),
				   (0x1 << INT00_LEVEL_L1));

	return ret;
}

static int ivp_kservice_exec_algo(struct ivp_device *ivp_device,
				  u64 handle, u32 core,
				  struct cmdq_pkt *pkt)
{
	struct ivp_kservice_request *requsest;
	struct ivp_request *req;
	int ret;
	void __iomem *ivpctrl_base;
	struct apmcu_exec_cmd exec_cmd;
	struct apmcu_exec_cmd_rtn exec_cmd_rtn;
	struct ivp_user *ivp_user;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	struct ivp_algo_context *algo_ctx;

	mutex_lock(&ivp_core->kservice_mutex);

	ret = ivp_validate_request_from_kservice_queue(handle, core,
						       ivp_device,
						       &requsest);
	if (ret) {
		mutex_unlock(&ivp_core->kservice_mutex);
		return ret;
	}

	req = (struct ivp_request *)&requsest->base;
	ivp_user = req->user;
	algo_ctx = idr_find(&ivp_user->algo_idr[core], req->drv_algo_hnd);
	if (!algo_ctx) {
		dev_err(ivp_core->dev, "no algo can excute.\n");
		mutex_unlock(&ivp_core->kservice_mutex);
		return ret;
	}

	ivpctrl_base = ivp_core->ivpctrl_base;

	exec_cmd.apmcu_cmd = CMD_EXEC_PROC;
	exec_cmd.algo_hnd = algo_ctx->fw_algo_hnd;
	exec_cmd.algo_arg_size = req->size;
	exec_cmd.algo_arg_base = req->addr_handle;
	exec_cmd.timestamp = requsest->submit_stamp;

	if (pkt) {
		ivp_core->kservice_state = IVP_KSERVICE_BUSY_WITH_GCE;
		/*pr_debug("ivp execution with gce  = 0x%x ,subsys = 0x%x)\n",*/
		/*         ivp_core->trg.reg_base, ivp_core->trg.subsys);     */
		ret = ivp_fw_exec_algo_with_gce(&exec_cmd, ivp_core, pkt);
	} else {
		ivp_core->kservice_state = IVP_KSERVICE_BUSY;
		ivp_fw_exec_algo(&exec_cmd, ivpctrl_base);

		ret = ivp_check_cmd_send(ivp_core, exec_cmd.timestamp);
		if (ret) {
			pr_err("%s: fail to send command\n", __func__);
			mutex_unlock(&ivp_core->kservice_mutex);
			return ret;
		}

		ret = wait_command(ivp_core, ivp_device->timeout);
		if (ret)
			pr_err("timeout to do execution\n");
		else
			ivp_fw_exec_algo_rtn(&exec_cmd_rtn, ivpctrl_base);
	}

	mutex_unlock(&ivp_core->kservice_mutex);
	return ret;
}

static struct ivp_device *get_ivp_device(struct platform_device *pdev)
{
	if (!pdev)
		return NULL;

	return (struct ivp_device *)platform_get_drvdata(pdev);
}

int mtk_send_ivp_request(struct platform_device *pdev,
			 u64 handle, u32 core, struct cmdq_pkt *pkt)
{
	int ret;
	struct ivp_device *ivp_device = get_ivp_device(pdev);

	if (!ivp_device)
		return -ENODEV;
	ret = ivp_kservice_exec_algo(ivp_device, handle, core, pkt);
	return ret;
}
EXPORT_SYMBOL(mtk_send_ivp_request);

int mtk_clr_ivp_request(struct platform_device *pdev,
			u64 handle, u32 core, struct cmdq_pkt *pkt)
{
	int ret = 0;
	struct ivp_device *ivp_device = get_ivp_device(pdev);
	struct ivp_core *ivp_core;
	u32 base;

	if (!ivp_device)
		return -ENODEV;

	if (core >= ivp_device->ivp_core_num || !ivp_device->ivp_core[core])
		return -ENODEV;

	ivp_core = ivp_device->ivp_core[core];
	base = ivp_core->trg.reg_base & 0xffff;
	if (pkt) {
		ret = cmdq_pkt_write_value(pkt, ivp_core->trg.subsys,
					   base | IVP_CTRL_REG_XTENSA_INT,
					   IVP_INT_XTENSA, IVP_INT_XTENSA);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_clr_ivp_request);

static int ivp_kservice_reload_algo(struct ivp_device *ivp_device,
				    u64 handle, u32 core)
{
	struct ivp_kservice_request *requsest;
	struct ivp_request *req;
	int ret = 0;
	void __iomem *ivpctrl_base;
	struct apmcu_load_cmd load_cmd;
	struct apmcu_load_cmd_rtn load_cmd_rtn;
	struct ivp_user *ivp_user;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	struct ivp_algo_context *algo_ctx;

	mutex_lock(&ivp_core->kservice_mutex);

	ret = ivp_validate_request_from_kservice_queue(handle, core,
						       ivp_device,
						       &requsest);
	if (ret) {
		mutex_unlock(&ivp_core->kservice_mutex);
		return ret;
	}

	ivpctrl_base = ivp_core->ivpctrl_base;
	req = (struct ivp_request *)&requsest->base;
	ivp_user = req->user;
	algo_ctx = idr_find(&ivp_user->algo_idr[core], req->drv_algo_hnd);
	if (!algo_ctx) {
		pr_err("%s: fail to get algo_ctx\n", __func__);
		return -EINVAL;
	}

	load_cmd.apmcu_cmd = CMD_LOAD_PROC;
	load_cmd.flags = algo_ctx->algo_flags;
	load_cmd.algo_size = algo_ctx->algo_size;
	load_cmd.algo_base = algo_ctx->algo_iova;
	load_cmd.timestamp = requsest->submit_stamp;

	ivp_fw_load_algo(&load_cmd, ivpctrl_base);

	ret = ivp_check_cmd_send(ivp_core, load_cmd.timestamp);
	if (ret) {
		pr_err("%s: fail to send command\n", __func__);
		mutex_unlock(&ivp_core->kservice_mutex);
		return ret;
	}

	ret = wait_command(ivp_core, ivp_device->timeout);
	if (ret) {
		pr_err("%s: timeout to re-load algo\n", __func__);
		mutex_unlock(&ivp_core->kservice_mutex);
		return ret;
	}

	ivp_fw_load_algo_rtn(&load_cmd_rtn, ivpctrl_base);
	algo_ctx->fw_algo_hnd = load_cmd_rtn.algo_hnd;

	ivp_core->kservice_state = IVP_KSERVICE_INIT;
	mutex_unlock(&ivp_core->kservice_mutex);

	return ret;
}

int mtk_reset_ivp_kservice(struct platform_device *pdev,
			   u64 handle, u32 core)
{
	int ret;
	struct ivp_device *ivp_device = get_ivp_device(pdev);
	struct ivp_core *ivp_core;

	if (!ivp_device)
		return -ENODEV;

	if (core >= ivp_device->ivp_core_num || !ivp_device->ivp_core[core])
		return -ENODEV;

	ivp_core = ivp_device->ivp_core[core];

	ivp_reset_hw(ivp_core);
	ret = ivp_kservice_reload_algo(ivp_device, handle, core);

	return ret;
}
EXPORT_SYMBOL(mtk_reset_ivp_kservice);

static int ivp_lock_command(struct ivp_device *ivp_device,
			    u32 core, u32 enable)
{
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];

	if (ivp_core->kservice_state == IVP_KSERVICE_STOP)
		return -EINVAL;

	/* Temp solution : set core for utilization calculation */
	if (enable) {
		ivp_utilization_compute_enter(ivp_device, core);
		lock_command(ivp_core);
	} else {
		unlock_command(ivp_core);
		ivp_utilization_compute_leave(ivp_device, core);
	}

	return 0;
}

int mtk_lock_ivp_command(struct platform_device *pdev,
			 u32 core, u32 enable)
{
	struct ivp_device *ivp_device = get_ivp_device(pdev);

	if (!ivp_device)
		return -ENODEV;
	if (core >= ivp_device->ivp_core_num || !ivp_device->ivp_core[core])
		return -ENODEV;
	return ivp_lock_command(ivp_device, core, enable);
}
EXPORT_SYMBOL(mtk_lock_ivp_command);


int ivp_test_auto_run(u32 core, struct ivp_device *ivp_device)
{
	const int handle_max = 10;
	const int loop_count = 20;
	struct ivp_core *ivp_core = ivp_device->ivp_core[core];
	struct list_head *head, *temp;
	struct ivp_kservice_request *req;
	u64 test_handle[handle_max];
	int test_cnt = 0, i, ret = 0;

	if (list_empty(&ivp_core->kservice_request_list))
		return -EINVAL;

	list_for_each_safe(head, temp, &ivp_core->kservice_request_list) {
		req = vlist_node_of(head, struct ivp_kservice_request);
		test_handle[test_cnt++] = (u64) req;
		if (test_cnt == handle_max)
			break;
	}

	if (test_cnt == 0)
		return -EINVAL;

	for (i = 0; i < loop_count; i++) {
		pr_err("auto run (%d) execute %llx\n",
		       i, test_handle[i % test_cnt]);
		ret = ivp_lock_command(ivp_device, core, 1);
		if (ret)
			break;
		ret = ivp_kservice_exec_algo(ivp_device,
					     test_handle[i % test_cnt],
					     core, NULL);
		ret |= ivp_lock_command(ivp_device, core, 0);
		if (ret)
			break;
	}

	pr_err("auto run : test reset flow\n");
	ivp_reset_hw(ivp_core);
	ret = ivp_lock_command(ivp_device, core, 1);
	ret = ivp_kservice_reload_algo(ivp_device, test_handle[0], core);
	ret |= ivp_lock_command(ivp_device, core, 0);

	/* continue run */
	for (i = 0; i < loop_count; i++) {
		pr_err("auto run (%d) execute %llx\n",
		       i, test_handle[i % test_cnt]);
		ret = ivp_lock_command(ivp_device, core, 1);
		if (ret)
			break;
		ret = ivp_kservice_exec_algo(ivp_device,
					     test_handle[i % test_cnt],
					     core, NULL);
		ret |= ivp_lock_command(ivp_device, core, 0);
		if (ret)
			break;
	}
	return ret;
}
