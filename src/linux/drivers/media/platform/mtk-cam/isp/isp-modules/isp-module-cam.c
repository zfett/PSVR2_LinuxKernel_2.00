/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
#define pr_fmt(fmt) "%s:%d:" fmt, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>

#include <dt-bindings/camera.h>

#include "../isp.h"
#include "../isp-registers.h"

static atomic_t reset_cnt[CAMSYS_NUM];

struct module_cam_event_statistic {
	unsigned long isp_raw_err_cnt[ISP_ISR_MAX_NUM];
	struct timespec latest_ts;
};

struct module_cam_events {
	unsigned long isp_raw_status;
	unsigned long dma_raw_status;
	unsigned long isp_raw_status_flush;
};

struct module_cam_context {
	struct module_cam_events cam_event_slot[MAX_SLOT_SIZE];
	struct module_cam_event_statistic stt;
	unsigned long isp_raw_status_err_mask;
	unsigned long isp_raw_status_err_disable;
	int aao_subsmple_done;
	int flush_trig;
	spinlock_t cam_slot_lock;
	unsigned int poll_event_mask_isp;
	unsigned int poll_event_mask_dma;
	struct file *poll_target;
	struct tasklet_struct done_task;
	struct list_head done_subscribe_list[ISP_CB_EVENT_MAX];
	unsigned int frame_cnt;
};

static void mtk_isp_module_cam_hw_init(const struct isp_module *module)
{
	int idx = 0;

	if (!module) {
		pr_err("module is null pointer, cam_hw_init didn't finish");
		return;
	}

	switch (module->module_id) {
	case MTK_ISP_ISP_CAMSYS_SIDE0:
	case MTK_ISP_ISP_CAMSYS_SIDE1:
	case MTK_ISP_ISP_CAMSYS_GAZE0:
	case MTK_ISP_ISP_CAMSYS_GAZE1:
		idx = module->module_id - MTK_ISP_ISP_CAMSYS_SIDE0;
		pr_debug("(%s) reset count(%d --> %d)\n",
			module->long_name,
			atomic_read(&reset_cnt[idx]),
			atomic_read(&reset_cnt[idx])+1);
		if (atomic_inc_return(&reset_cnt[idx]) == 1) {
#if !defined(CONFIG_COMMON_CLK_MT3612)
			ISP_WR32(module->hw_info.base_addr + CAMSYS_REG_CG_CON,
				 0x0);
			ISP_WR32(module->hw_info.base_addr + CAMSYS_REG_CG_CLR,
				 0xffffffff);
#endif
			pr_info("(%s) CAMSYS Reset\n", module->long_name);
			ISP_WR32(module->hw_info.base_addr + CAMSYS_REG_SW_RST,
				 0xfffffff0);
			ISP_WR32(module->hw_info.base_addr + CAMSYS_REG_SW_RST,
				0x0);
		}

		if (module->module_id == MTK_ISP_ISP_CAMSYS_SIDE0 ||
			module->module_id == MTK_ISP_ISP_CAMSYS_SIDE1) {
			ISP_WR32(module->hw_info.base_addr + CAMSYS_REG_BE_CTL,
				 0x3);
		}

		break;
		/*
		 * DMA performance, this register in CAM_B is useless,
		 * CAM_B's performance is controlled by CAM_A's special_fun_en
		 * and this register can't be modified when CAM_B is running.
		 *
		 * Enable CQ_ULTRA_BPCI_EN, CQ_ULTRA_LSCI_EN
		 * MULTI_PLANE_ID_EN should keep 0 otherwise RBFC will be fail
		 */
	case MTK_ISP_ISP_CAM1:
		ISP_WR32(module->hw_info.base_addr + CAM_REG_SPECIAL_FUN_EN,
			 0x30000000);
		/*
		 * Fallthrough
		 * Setting DMA ultra/pre-ultra function
		 */
	case MTK_ISP_ISP_CAM2:
		ISP_WR32(module->hw_info.base_addr + CAM_REG_IMGO_CON,
			0x80000180);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_IMGO_CON2,
			0xc000c0);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_IMGO_CON3,
			0x600060);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_RRZO_CON,
			0x80000280);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_RRZO_CON2,
			0x1800180);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_RRZO_CON3,
			0xc000c0);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_AAO_CON,
			0x80000040);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_AAO_CON2,
			0x200020);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_AAO_CON3,
			0x100010);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_PSO_CON,
			0x80000040);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_PSO_CON2,
			0x200020);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_PSO_CON3,
			0x100010);
		break;
	default:
		break;
	}
}

static inline int mtk_isp_module_cam_init(const struct isp_module *module)
{
	int ret = 0;
	int idx = 0;
	struct module_cam_context *cam_context;

	if (!module)
		return -EFAULT;

#if defined(CONFIG_COMMON_CLK_MT3612)
	ret = isp_module_power_on(module);
	if (ret) {
		pr_err("Power on ISP module(%d) failed, ret:%d\n",
			module->module_id, ret);
		return ret;
	}
#endif
	cam_context = module->private_data;
	/*
	 * isp err disable should be set to zero at initial, and
	 * will be modified while dma error happens.
	 */
	cam_context->isp_raw_status_err_disable = 0;
	cam_context->isp_raw_status_err_mask = CAM_INT_ST_MASK_CAM_ERR;
	cam_context->flush_trig = 0;

	for (idx = 0; idx < MAX_SLOT_SIZE; idx++) {
		cam_context->cam_event_slot[idx].isp_raw_status = 0;
		cam_context->cam_event_slot[idx].dma_raw_status = 0;
		cam_context->cam_event_slot[idx].isp_raw_status_flush = 0;
	}

	mtk_isp_module_cam_hw_init(module);

	return ret;
}

static inline int mtk_isp_module_cam_stop(struct isp_module *module)
{
	int ret = 0;
	int idx = 0;

	if (!module)
		return -EFAULT;

#if defined(CONFIG_COMMON_CLK_MT3612)
	ret = isp_module_power_off(module);
	if (ret) {
		pr_err("Power off ISP module(%d) failed, ret:%d\n",
			module->module_id, ret);
		return ret;
	}
#endif
	switch (module->module_id) {
	case MTK_ISP_ISP_CAMSYS_SIDE0:
	case MTK_ISP_ISP_CAMSYS_SIDE1:
	case MTK_ISP_ISP_CAMSYS_GAZE0:
	case MTK_ISP_ISP_CAMSYS_GAZE1:
		idx = module->module_id - MTK_ISP_ISP_CAMSYS_SIDE0;
		pr_info("(%s) reset count(%d -> %d)\n",
			module->long_name,
			atomic_read(&reset_cnt[idx]),
			atomic_read(&reset_cnt[idx])-1);
		atomic_dec(&reset_cnt[idx]);
		break;
	default:
		break;
	}

	return ret;
}

static inline int module_cam_check_event(struct module_cam_context *cam_context,
					 int idx, int type, int event_shifter)
{
	int result = 0;
	unsigned long flags;

	spin_lock_irqsave(&cam_context->cam_slot_lock, flags);
	if (type == ISP_MODULE_EVENT_TYPE_ISP) {
		result = (cam_context->cam_event_slot[idx].isp_raw_status >>
			  event_shifter) & 1;
	} else if (type == ISP_MODULE_EVENT_TYPE_DMA) {
		result = (cam_context->cam_event_slot[idx].dma_raw_status >>
			  event_shifter) & 1;
	}
	spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);

/*		pr_debug("event:0x%x, result:%d at [%lu.%06lu]\n",
*		 event_shifter, result,
*		 cam_context->stt.last_ts.tv_sec,
*		 cam_context->stt.last_ts.tv_nsec);
*/

	return result;
}

static int mtk_isp_module_cam_flush_event(struct isp_module *module,
					  const struct event_req *e)
{
	struct module_cam_context *cam_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = 0;
	unsigned long flags;

	if (event_module != ISP_MODULE_CAM)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP &&
	    event_type != ISP_MODULE_EVENT_TYPE_DMA)
		return -EINVAL;

	spin_lock_irqsave(&cam_context->cam_slot_lock, flags);

	for (idx = 0; idx < MAX_SLOT_SIZE; idx++) {
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
		cam_context->cam_event_slot[idx].isp_raw_status |=
			(1L << event_shifter);
		else if (event_type == ISP_MODULE_EVENT_TYPE_DMA)
		cam_context->cam_event_slot[idx].dma_raw_status |=
			(1L << event_shifter);
	}

	cam_context->flush_trig = 1;

	spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);

	wake_up_interruptible(&module->wait_queue);

	return 0;
}

static int mtk_isp_module_cam_clear_event(struct isp_module *module,
					  const struct event_req *e)
{
	struct module_cam_context *cam_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = e->user_key;
	unsigned long flags;

	if (event_module != ISP_MODULE_CAM)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP &&
	    event_type != ISP_MODULE_EVENT_TYPE_DMA)
		return -EINVAL;

	spin_lock_irqsave(&cam_context->cam_slot_lock, flags);
	if (event_type == ISP_MODULE_EVENT_TYPE_ISP) {
		cam_context->cam_event_slot[idx].isp_raw_status &=
		    ~(1L << event_shifter);
	} else if (event_type == ISP_MODULE_EVENT_TYPE_DMA) {
		cam_context->cam_event_slot[idx].dma_raw_status &=
		    ~(1L << event_shifter);
	}
	cam_context->aao_subsmple_done = 0;
	spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);

	return 0;
}

static int mtk_isp_module_cam_wait_event(struct isp_module *module,
					 const struct event_req *e)
{
	struct module_cam_context *cam_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = e->user_key;
	unsigned long flags;
	int ret;

	if (event_module != ISP_MODULE_CAM)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP &&
	    event_type != ISP_MODULE_EVENT_TYPE_DMA)
		return -EINVAL;

	spin_lock_irqsave(&cam_context->cam_slot_lock, flags);
	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT &&
		 !cam_context->flush_trig) {
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			cam_context->cam_event_slot[idx].isp_raw_status &=
			    ~(1L << event_shifter);
		else if (event_type == ISP_MODULE_EVENT_TYPE_DMA)
			cam_context->cam_event_slot[idx].dma_raw_status &=
			    ~(1L << event_shifter);
	}
	spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);

	ret = wait_event_interruptible_timeout(module->wait_queue,
					       module_cam_check_event
					       (cam_context, idx, event_type,
						event_shifter),
					       mtk_isp_ms_to_jiffies(e->
								     timeout));

	if (ret == 0) {
		pr_err("wait event timeout\n");
		return -EIO;
	} else if (ret == -ERESTARTSYS) {
		pr_err("wait is interrupted by a signal\n");
		return -ERESTARTSYS;
	}

	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT ||
	    e->mode == ISP_IRQ_MODE_CLEAR_NONE) {
		spin_lock_irqsave(&cam_context->cam_slot_lock, flags);
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			cam_context->cam_event_slot[idx].isp_raw_status &=
			    ~(1L << event_shifter);
		else if (event_type == ISP_MODULE_EVENT_TYPE_DMA)
			cam_context->cam_event_slot[idx].dma_raw_status &=
			    ~(1L << event_shifter);
		spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);
	}

	/* pr_debug("%s received event 0x%x\n", module->name, event_shifter); */

	return 0;
}

static void mtk_isp_module_cam_reset(const struct isp_module *module)
{
	if (!module)
		return;

	switch (module->module_id) {
	case MTK_ISP_ISP_UNI:
		/* Reset UNI with the order RAWI_HW_RST -> CTL_HW_RST
		 * 0x04 = RAWI_HW_RST
		 * 0x1000 = CTL_HW_RST
		 * 0x0 = no reset
		 */
		ISP_WR32(module->hw_info.base_addr + CAM_UNI_REG_TOP_SW_CTL,
			 0x04);
		ISP_WR32(module->hw_info.base_addr + CAM_UNI_REG_TOP_SW_CTL,
			 0x1000);
		ISP_WR32(module->hw_info.base_addr + CAM_UNI_REG_TOP_SW_CTL,
			 0x0);
		break;
	case MTK_ISP_ISP_CAMSYS_SIDE0:
	case MTK_ISP_ISP_CAMSYS_SIDE1:
	case MTK_ISP_ISP_CAMSYS_GAZE0:
	case MTK_ISP_ISP_CAMSYS_GAZE1:
		/* Do nothing here for camsys. */
		break;
	default:
		/* Reset CAM with the order
		 * (HW_RST + SW_RST_Trig) -> SW_RST_Trig
		 * 0x5 = HW_RST + SW_RST_Trig
		 * 0x1 = SW_RST_Trig
		 * 0x0 = no reset
		 */
		ISP_WR32(module->hw_info.base_addr + CAM_REG_CTL_SW_CTL, 0x5);
		mdelay(1);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_CTL_SW_CTL, 0x1);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_CTL_SW_CTL, 0x0);
		break;
	}

	pr_debug("%s virt:0x%lx phy:0x%lx off(0x%lx)\n",
		module->name,
		(unsigned long)module->hw_info.base_addr,
		(unsigned long)module->hw_info.start,
		(unsigned long)ISP_RD32(module->hw_info.base_addr +
					CAM_REG_CTL_SW_CTL));
}

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
static int mtk_isp_module_cam_ioctl(const struct isp_module *module,
				unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	if (!module)
		return -EINVAL;

	pr_debug("custom_ioctl, module_id:%d\n", module->module_id);

	switch (cmd) {
	default:
		break;
	}

	return rc;
}
#endif

static int mtk_isp_module_cam_set_poll(struct file *filp, struct isp_module *module,
					const struct event_req *e)
{
	struct module_cam_context *cam_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);

	if (event_module != ISP_MODULE_CAM)
		return -EINVAL;

	if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
		cam_context->poll_event_mask_isp |= (1L << event_shifter);
	else if (event_type == ISP_MODULE_EVENT_TYPE_DMA)
		cam_context->poll_event_mask_dma |= (1L << event_shifter);
	else
		return -EINVAL;

	cam_context->poll_target = filp;
	return 0;
}


static int mtk_isp_module_cam_check_poll(struct file *filp, struct isp_module *module)
{
	struct module_cam_context *cam_context = module->private_data;
	int idx = 0;
	int ret = 0;
	unsigned long flags;

	if (filp == cam_context->poll_target) {
		spin_lock_irqsave(&cam_context->cam_slot_lock, flags);
		ret = (cam_context->cam_event_slot[idx].isp_raw_status
			   & cam_context->poll_event_mask_isp);
		ret |= (cam_context->cam_event_slot[idx].dma_raw_status
				& cam_context->poll_event_mask_dma);
		spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);
	}
	return ret;
}

static int mtk_isp_module_cam_read(struct file *filp, struct isp_module *module)
{
	struct module_cam_context *cam_context = module->private_data;
	int idx = 0;
	unsigned long flags;
	int isp;
	int dma;

	if (filp == cam_context->poll_target) {
		spin_lock_irqsave(&cam_context->cam_slot_lock, flags);
		isp = (cam_context->cam_event_slot[idx].isp_raw_status
			   & cam_context->poll_event_mask_isp);
		isp = (isp&(isp*(-1)));
		cam_context->cam_event_slot[idx].isp_raw_status &= ~(isp);
		spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);
		if (isp) {
			return __ISP_MODULE_EVENT_CMD(ISP_MODULE_CAM,
										  ISP_MODULE_EVENT_TYPE_ISP,
										  ffs(isp)-1);
		}

		spin_lock_irqsave(&cam_context->cam_slot_lock, flags);
		dma = (cam_context->cam_event_slot[idx].dma_raw_status
			   & cam_context->poll_event_mask_dma);
		dma = (dma&(dma*(-1)));
		cam_context->cam_event_slot[idx].dma_raw_status &= ~(dma);
		spin_unlock_irqrestore(&cam_context->cam_slot_lock, flags);
		if (dma) {
			return __ISP_MODULE_EVENT_CMD(ISP_MODULE_CAM,
										  ISP_MODULE_EVENT_TYPE_DMA,
										  ffs(dma)-1);
		}
	}
	return 0;
}

static struct isp_module_ops cam_ops = {
	.init = mtk_isp_module_cam_init,
	.stop = mtk_isp_module_cam_stop,
	.wait_module_event = mtk_isp_module_cam_wait_event,
	.flush_module_event = mtk_isp_module_cam_flush_event,
	.clear_module_event = mtk_isp_module_cam_clear_event,
	.reset = mtk_isp_module_cam_reset,
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	.custom_ioctl = mtk_isp_module_cam_ioctl,
#endif
	.set_poll_trigger = mtk_isp_module_cam_set_poll,
	.check_poll_trigger = mtk_isp_module_cam_check_poll,
	.read_poll_trigger = mtk_isp_module_cam_read,
};

static void mtk_isp_module_cam_check_err_mask(struct module_cam_context
					      *cam_context,
					      unsigned long err_status,
					      struct timespec last_ts)
{
	int i;
	unsigned long cur_usec = (cam_context->stt.latest_ts.tv_nsec / 1000) +
				 (cam_context->stt.latest_ts.tv_sec *
				  1000000);
	unsigned long last_usec = (last_ts.tv_nsec / 1000) +
				  (last_ts.tv_sec * 1000000);
	for (i = 0; i < ISP_ISR_MAX_NUM; i++) {
		if ((err_status >> i) & 1) {
			cam_context->stt.isp_raw_err_cnt[i]++;
			if ((cur_usec - last_usec) < ISP_INT_ERR_TIMER_THR) {
				if (cam_context->stt.isp_raw_err_cnt[i] >
				   ISP_INT_ERR_MAX_TIME) {
					cam_context->isp_raw_status_err_disable
							|= (1 << i);
				}
			} else {
				cam_context->stt.isp_raw_err_cnt[i] = 0;
			}
		}
	}

}


static irqreturn_t mtk_isp_module_cam_isr(int irq, void *dev_id)
{
	struct isp_module *module = (struct isp_module *)dev_id;
	struct module_cam_context *cam_context = module->private_data;
	unsigned long isp_status;
	unsigned long dma_status;
	unsigned long err_status;
	unsigned long imgo_err_status, rrzo_err_status, aao_err_status;
	unsigned long subsample_en = 0;
	unsigned long irq_en_orig, irq_en_new;
	unsigned long frame_idx;
	struct timespec last_ts = cam_context->stt.latest_ts;
	int i, j;

	spin_lock(&cam_context->cam_slot_lock);
	ktime_get_ts(&cam_context->stt.latest_ts);

	isp_status =
	    ISP_RD32(module->hw_info.base_addr + CAM_REG_CTL_RAW_INT_STATUS);
	dma_status =
	    ISP_RD32(module->hw_info.base_addr + CAM_REG_CTL_RAW_INT2_STATUS);
	subsample_en =
	    ISP_RD32(module->hw_info.base_addr +
		     CAM_REG_CTL_SW_PASS1_DONE) & 0x100;
	err_status = isp_status & cam_context->isp_raw_status_err_mask;

	imgo_err_status =
	    ISP_RD32(module->hw_info.base_addr + CAM_REG_IMGO_ERR_STAT);
	rrzo_err_status =
	    ISP_RD32(module->hw_info.base_addr + CAM_REG_RRZO_ERR_STAT);
	aao_err_status =
	    ISP_RD32(module->hw_info.base_addr + CAM_REG_AAO_ERR_STAT);

	/* save frame index to AAO spar_reg */
	if ((isp_status >> ISP_MODULE_EVENT_VSYNC) & 1) {
		frame_idx =
			ISP_RD32(module->hw_info.base_addr +
				CAM_REG_PSO_FH_SPARE_11);
		ISP_WR32(module->hw_info.base_addr + CAM_REG_AAO_FH_SPARE_2,
		 frame_idx);
	}

	if (subsample_en != 0) {
		if ((isp_status >> ISP_MODULE_EVENT_VSYNC) & 1)
			cam_context->aao_subsmple_done = 0;
		if ((dma_status >> ISP_MODULE_EVENT_AAO_DONE) & 1)
			cam_context->aao_subsmple_done |= AAO_DONE;
		if ((isp_status >> ISP_MODULE_EVENT_SW_PASS1_DONE) & 1)
			cam_context->aao_subsmple_done |= SW_P1_Done;
		if (cam_context->aao_subsmple_done == (AAO_DONE | SW_P1_Done)) {
			dma_status |= (1L << ISP_MODULE_EVENT_AAO_DONE);
		} else {
			dma_status &= ~(1L << ISP_MODULE_EVENT_AAO_DONE);
		}
	}

	for (i = 0; i < MAX_SLOT_SIZE; i++) {
		/* for cam_done_task debug usage */
		cam_context->cam_event_slot[i].isp_raw_status_flush =
				isp_status;

		for (j = 0; j <= ISP_MODULE_EVENT_SW_PASS1_DONE; j++) {
			if ((isp_status >> j) & 1)
				cam_context->cam_event_slot[i].isp_raw_status |=
						(1L << j);
		}

		if ((isp_status >> ISP_MODULE_EVENT_VSYNC) & 1)
			cam_context->cam_event_slot[i].dma_raw_status = 0;
		else
			cam_context->cam_event_slot[i].dma_raw_status |=
							dma_status;
	}

	mtk_isp_module_cam_check_err_mask(cam_context, err_status, last_ts);

	irq_en_orig = ISP_RD32(module->hw_info.base_addr +
			       CAM_REG_CTL_RAW_INT_EN);
	irq_en_new = irq_en_orig & (~cam_context->isp_raw_status_err_disable);
	ISP_WR32(module->hw_info.base_addr + CAM_REG_CTL_RAW_INT_EN,
		 irq_en_new);

	if ((isp_status >> ISP_MODULE_EVENT_SOF) & 1) {
		irq_en_new |= cam_context->isp_raw_status_err_disable;
		ISP_WR32(module->hw_info.base_addr + CAM_REG_CTL_RAW_INT_EN,
				 irq_en_new);
		cam_context->isp_raw_status_err_disable = 0;
		ISP_WR32(module->hw_info.base_addr + CAM_REG_PSO_FH_SPARE_2, ++cam_context->frame_cnt);
	}

	spin_unlock(&cam_context->cam_slot_lock);

	wake_up_interruptible(&module->wait_queue);
	tasklet_schedule(&cam_context->done_task);

	pr_debug("%s isp:0x%lx, dma:0x%lx\n", module->long_name,
			isp_status, dma_status);

	return IRQ_HANDLED;
}

static int _mtk_isp_register_p1_cb(struct isp_module *module,
			    int cb_type, cbptr_t cbptr, void *data)
{
	struct module_cam_context *cam_context = module->private_data;
	struct isp_p1_cb_event_package *cb_event;

	if ((unsigned int)cb_type >= ISP_CB_EVENT_MAX)
		return -EINVAL;

	cb_event = kzalloc(sizeof(struct isp_p1_cb_event_package), GFP_KERNEL);
	if (!cb_event)
		return -ENOMEM;

	cb_event->cb_type = cb_type;
	cb_event->cbptr = cbptr;
	cb_event->private_data = data;
	list_add(&cb_event->list, &cam_context->done_subscribe_list[cb_type]);

	pr_info("%s 0x%x\n", module->long_name, cb_type);

	return 0;
}

int mtk_isp_register_p1_cb(int dev_id, int cb_type, cbptr_t cbptr, void *data)
{
	struct isp_dev *dev = mtk_isp_find_isp_dev_by_id(dev_id);
	struct isp_module *module =
			mtk_isp_find_isp_module_by_id(dev, MTK_ISP_ISP_CAM1);

	return _mtk_isp_register_p1_cb(module, cb_type, cbptr, data);
}
EXPORT_SYMBOL(mtk_isp_register_p1_cb);

static int _mtk_isp_unregister_p1_cb(struct isp_module *module,
			    int cb_type, cbptr_t cbptr, void *data)
{
	struct module_cam_context *cam_context = module->private_data;
	struct isp_p1_cb_event_package *cb_event;
	int found = 0;

	if ((cb_type >= ISP_CB_EVENT_MAX) || (cb_type < 0))
		return -EINVAL;

	list_for_each_entry(cb_event,
		&cam_context->done_subscribe_list[cb_type],
		list) {
		if ((cb_event->cb_type == cb_type) &&
			(cb_event->private_data == data) &&
			(cb_event->cbptr == cbptr)) {
			found = 1;
			break;
		}
	}

	if (found == 0)
		return -EINVAL;

	list_del(&cb_event->list);
	kfree(cb_event);

	return 0;
}

int mtk_isp_unregister_p1_cb(int dev_id, int cb_type, cbptr_t cbptr, void *data)
{
	struct isp_dev *dev = mtk_isp_find_isp_dev_by_id(dev_id);
	struct isp_module *module =
			mtk_isp_find_isp_module_by_id(dev, MTK_ISP_ISP_CAM1);

	return _mtk_isp_unregister_p1_cb(module, cb_type, cbptr, data);
}
EXPORT_SYMBOL(mtk_isp_unregister_p1_cb);

static void mtk_isp_module_cam_done_task(unsigned long data)
{
	struct isp_module *module_cam = (struct isp_module *)data;
	struct module_cam_context *cam_context = module_cam->private_data;
	struct isp_p1_cb_event_package *cb_event;
	unsigned long isp_status;
	unsigned long dma_status;

	/* using slot 0 as source because slot 0 is for all user
	* to check if we need to call the callbacks.
	*/
	isp_status = cam_context->cam_event_slot[0].isp_raw_status_flush;
	dma_status = cam_context->cam_event_slot[0].dma_raw_status;

	if ((isp_status >> ISP_MODULE_EVENT_SOF) & 1)
		list_for_each_entry(cb_event,
				    &cam_context->
				    done_subscribe_list[ISP_CB_EVENT_SOF],
				    list) {
		cb_event->cbptr(cb_event->private_data);
		}
	/*
	if (((isp_status >> ISP_MODULE_EVENT_SOF) & 1) ||
	    ((isp_status >> ISP_MODULE_EVENT_SW_PASS1_DONE) & 1))
		pr_debug("name: %s, isp_status:0x%lx dma_status:0x%lx\n",
			 module_cam->long_name, isp_status, dma_status);
	*/
}



static const struct of_device_id isp_cam_of_ids[] = {
	{.compatible = "mediatek,isp-camsys", .data = "CAMSYS"},
	{.compatible = "mediatek,isp-cam", .data = "CAM"},
	{.compatible = "mediatek,isp-uni", .data = "UNI"},
	{}
};

static int mtk_isp_module_cam_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct isp_module *module_cam;
	struct module_cam_context *cam_context;
	int ret = 0;
	int num_items = 0;
	int i, j;

	if (!np) {
		pr_err("No device node for mtk isp module driver\n");
		return -ENODEV;
	}

	of_id = of_match_node(isp_cam_of_ids, np);
	if (!of_id)
		return -ENODEV;

	if (of_property_read_u32(np, "num-items", &num_items))
		return -EINVAL;

	for (i = 0; i < num_items; i++) {
		module_cam = _isp_module_probe_index(pdev, i, of_id->data);

		if (IS_ERR(module_cam)) {
			ret = PTR_ERR(module_cam);
			goto err;
		}

		module_cam->module_dev = dev;

		cam_context = kzalloc(sizeof(struct module_cam_context),
				      GFP_KERNEL);
		if (!cam_context) {
			ret = -ENOMEM;
			goto err;
		}

		spin_lock_init(&cam_context->cam_slot_lock);

		init_waitqueue_head(&module_cam->wait_queue);
		module_cam->ops = &cam_ops;
		module_cam->private_data = cam_context;

		if (module_cam->hw_info.irq) {
			ret = request_irq(module_cam->hw_info.irq,
					  mtk_isp_module_cam_isr,
					  IRQF_TRIGGER_LOW | IRQF_SHARED,
					  module_cam->long_name, module_cam);
			if (ret) {
				pr_err
				    ("failed to request_irq w/ irq %d for %s\n",
				     module_cam->hw_info.irq, module_cam->name);
				goto err;
			}
		}

		for (j = 0; j < ISP_CB_EVENT_MAX; j++)
			INIT_LIST_HEAD(&cam_context->done_subscribe_list[j]);

		tasklet_init(&cam_context->done_task,
					mtk_isp_module_cam_done_task,
					(unsigned long)module_cam);

		mtk_isp_register_isp_module(module_cam->parent_id, module_cam);
		platform_set_drvdata(pdev, module_cam);

		pr_debug("(map_addr=0x%lx irq=%d 0x%llx->0x%llx)\n",
			(unsigned long)module_cam->hw_info.base_addr,
			module_cam->hw_info.irq,
			module_cam->hw_info.start, module_cam->hw_info.end);
	}

	for (j = 0; j < CAMSYS_NUM; j++)
		atomic_set(&reset_cnt[j], 0);

#if defined(CONFIG_COMMON_CLK_MT3612)
	isp_module_pdm_enable(dev, module_cam);
	ret = isp_module_grp_clk_get(dev, &module_cam->clklist);
	if (ret == -ENODATA) {
		return 0; /* not a problem */
	} else if (ret) {
		pr_err("failed get clk for ret:%d (-517: probe laterm)\n", ret);
		goto err_clk;
	}
#endif

	return 0;

#if defined(CONFIG_COMMON_CLK_MT3612)
err_clk:
	isp_module_grp_clk_free(module_cam);
#endif
err:
	return ret;
}

static int mtk_isp_module_cam_remove(struct platform_device *pdev)
{
	struct isp_module *module_cam = platform_get_drvdata(pdev);
	struct module_cam_context *cam_context = module_cam->private_data;

	mtk_isp_unregister_isp_module(module_cam);
	tasklet_kill(&cam_context->done_task);
	free_irq(module_cam->hw_info.irq, module_cam);
#if defined(CONFIG_COMMON_CLK_MT3612)
	isp_module_grp_clk_free(module_cam);
	isp_module_pdm_disable(&pdev->dev, module_cam);
#endif
	kfree(cam_context);
	kfree(module_cam);

	return 0;
}

static struct platform_driver isp_module_cam_drvier = {
	.probe = mtk_isp_module_cam_probe,
	.remove = mtk_isp_module_cam_remove,
	.driver = {
		   .name = "isp-cam",
		   .owner = THIS_MODULE,
		   .of_match_table = isp_cam_of_ids,
		   }
};

module_platform_driver(isp_module_cam_drvier);
MODULE_DESCRIPTION("MTK Camera ISP Driver");
MODULE_LICENSE("GPL");
