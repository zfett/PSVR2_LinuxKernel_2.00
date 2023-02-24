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

#include <soc/mediatek/mtk_dip_warp_rbsync.h>

static atomic_t reset_cnt[IMGSYS_NUM];
static spinlock_t rbsync_handle_lock[2];

struct module_dip_events {
	unsigned long isp_raw_status;
	unsigned long cq_int_status;
	unsigned long cq_int2_status;
	unsigned long cq_int3_status;
	unsigned long dma_raw_status;
};

struct module_dip_context {
	struct module_dip_events dip_event_slot[MAX_SLOT_SIZE];
	struct rb_sync_handle *rbsync_handle;
	int wbuf_idx;
	int flush_trig;
	spinlock_t dip_slot_lock;
	unsigned int poll_event_mask;
	struct file *poll_target;
};

static int mtk_isp_module_dip_hw_init(const struct isp_module *module)
{
	int idx = 0;

	if (!module)
		return -EINVAL;

	switch (module->module_id) {
	case MTK_ISP_ISP_IMGSYS_SIDE0:
	case MTK_ISP_ISP_IMGSYS_SIDE1:
	case MTK_ISP_ISP_IMGSYS_GAZE0:
	case MTK_ISP_ISP_IMGSYS_GAZE1:
		idx = module->module_id - MTK_ISP_ISP_IMGSYS_SIDE0;
		pr_debug("(%s) reset count(%d --> %d)\n",
			module->long_name,
			atomic_read(&reset_cnt[idx]),
			atomic_read(&reset_cnt[idx])+1);
		if (atomic_inc_return(&reset_cnt[idx]) == 1) {
#if !defined(CONFIG_COMMON_CLK_MT3612)
			ISP_WR32(
				module->hw_info.base_addr + IMGSYS_REG_CG_CON,
				0x0);
			ISP_WR32(
				module->hw_info.base_addr + IMGSYS_REG_CG_CLR,
				0xffffffff);
#endif
			pr_info("(%s) IMGSYS Reset\n", module->long_name);
			ISP_WR32(
				module->hw_info.base_addr + IMGSYS_REG_SW_RST,
				0xfffffcf0);
			ISP_WR32(
				module->hw_info.base_addr + IMGSYS_REG_SW_RST,
				0x0);
		}
		break;
	case MTK_ISP_ISP_DIPA:
		ISP_WR32(
			module->hw_info.base_addr + DIP_REG_IMG3O_CON,
			0x80000040);
		ISP_WR32(
			module->hw_info.base_addr + DIP_REG_IMG3O_CON2,
			0x18000c0);
		ISP_WR32(
			module->hw_info.base_addr + DIP_REG_IMG3O_CON3,
			0xc00030);
		break;
	default:
		break;
	}

	return 0;
}

static inline int mtk_isp_module_dip_init(const struct isp_module *module)
{
	int ret = 0;
	int idx = 0;
	struct module_dip_context *dip_context;
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

	mtk_isp_module_dip_hw_init(module);

	dip_context = module->private_data;

	dip_context->flush_trig = 0;
	for (idx = 0; idx < MAX_SLOT_SIZE; idx++) {
		dip_context->dip_event_slot[idx].isp_raw_status = 0;
		dip_context->dip_event_slot[idx].cq_int_status = 0;
		dip_context->dip_event_slot[idx].cq_int2_status = 0;
		dip_context->dip_event_slot[idx].cq_int3_status = 0;
		dip_context->dip_event_slot[idx].dma_raw_status = 0;
	}

	return ret;
}

static inline int mtk_isp_module_dip_stop(struct isp_module *module)
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
	case MTK_ISP_ISP_IMGSYS_SIDE0:
	case MTK_ISP_ISP_IMGSYS_SIDE1:
	case MTK_ISP_ISP_IMGSYS_GAZE0:
	case MTK_ISP_ISP_IMGSYS_GAZE1:
		idx = module->module_id - MTK_ISP_ISP_IMGSYS_SIDE0;
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


static void mtk_isp_module_dip_reset(const struct isp_module *module)
{
	pr_debug("%s\n", module->name);

	if (!module)
		return;

	switch (module->module_id) {
	case MTK_ISP_ISP_IMGSYS_SIDE0:
	case MTK_ISP_ISP_IMGSYS_SIDE1:
	case MTK_ISP_ISP_IMGSYS_GAZE0:
	case MTK_ISP_ISP_IMGSYS_GAZE1:
		/* Do nothing here for imgsys.*/
		break;
	default:
		/* Reset DIP with the order
		 * (HW_RST + SW_RST_Trig) -> SW_RST_Trig
		 * 0x5 = HW_RST + SW_RST_Trig
		 * 0x1 = SW_RST_Trig
		 * 0x0 = no reset
		 */
		ISP_WR32(
			module->hw_info.base_addr + DIP_REG_CTL_SW_CTL,
			0x5);
		mdelay(1);
		ISP_WR32(
			module->hw_info.base_addr + DIP_REG_CTL_SW_CTL,
			0x1);
		ISP_WR32(
			module->hw_info.base_addr + DIP_REG_CTL_SW_CTL,
			0x0);
		break;
	}
}


static inline int module_dip_check_event(
	struct module_dip_context *dip_context,
	int idx,
	int type,
	int event_shifter)
{
	int result = 0;
	unsigned long flags;

	spin_lock_irqsave(&dip_context->dip_slot_lock, flags);
	if (type == ISP_MODULE_EVENT_TYPE_ISP)
		result = (dip_context->dip_event_slot[idx].isp_raw_status >>
				event_shifter) & 1;
	else if (type == ISP_MODULE_EVENT_TYPE_DMA)
		result = (dip_context->dip_event_slot[idx].dma_raw_status >>
				event_shifter) & 1;
	spin_unlock_irqrestore(&dip_context->dip_slot_lock, flags);

/*	pr_debug("event:0x%x, result:%d\n",
*		event_shifter,
*		result
*	);
*/
	return result;
}

static int mtk_isp_module_dip_flush_event(
	struct isp_module *module,
	const struct event_req *e)
{
	struct module_dip_context *dip_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = 0;
	unsigned long flags;

	if (event_module != ISP_MODULE_DIP)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP &&
		event_type != ISP_MODULE_EVENT_TYPE_DMA)
		return -EINVAL;

	spin_lock_irqsave(&dip_context->dip_slot_lock, flags);
	for (idx = 0; idx < MAX_SLOT_SIZE; idx++) {
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			dip_context->dip_event_slot[idx].isp_raw_status |=
				(1L << event_shifter);
		else if (event_type == ISP_MODULE_EVENT_TYPE_DMA)
			dip_context->dip_event_slot[idx].dma_raw_status |=
				(1L << event_shifter);
	}

	dip_context->flush_trig = 1;

	spin_unlock_irqrestore(&dip_context->dip_slot_lock, flags);

	wake_up_interruptible(&module->wait_queue);

	return 0;
}

static int mtk_isp_module_dip_wait_event(
	struct isp_module *module,
	const struct event_req *e)
{
	struct module_dip_context *dip_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = e->user_key;
	unsigned long flags;
	int rc;

	if (event_module != ISP_MODULE_DIP)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP &&
		event_type != ISP_MODULE_EVENT_TYPE_DMA)
		return -EINVAL;

	spin_lock_irqsave(&dip_context->dip_slot_lock, flags);
	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT &&
		!dip_context->flush_trig) {
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			dip_context->dip_event_slot[idx].isp_raw_status &=
				~(1L << event_shifter);
		else if (event_type == ISP_MODULE_EVENT_TYPE_DMA)
			dip_context->dip_event_slot[idx].dma_raw_status &=
				~(1L << event_shifter);
	}
	spin_unlock_irqrestore(&dip_context->dip_slot_lock, flags);

	rc = wait_event_interruptible_timeout(
		module->wait_queue,
		module_dip_check_event(
			dip_context,
			idx,
			event_type,
			event_shifter
		),
		mtk_isp_ms_to_jiffies(e->timeout)
	);
	if (rc == 0) {
		pr_debug("wait event timeout\n");
		return -EIO;
	} else if (rc == -ERESTARTSYS) {
		pr_debug("wait is interrupted by a signal\n");
		return -ERESTARTSYS;
	}

	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT ||
				e->mode == ISP_IRQ_MODE_CLEAR_NONE) {
		spin_lock_irqsave(&dip_context->dip_slot_lock, flags);
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			dip_context->dip_event_slot[idx].isp_raw_status &=
				~(1L << event_shifter);
		else if (event_type == ISP_MODULE_EVENT_TYPE_DMA)
			dip_context->dip_event_slot[idx].dma_raw_status &=
				~(1L << event_shifter);
		spin_unlock_irqrestore(&dip_context->dip_slot_lock, flags);
	}

	/*
	pr_debug("%s received event 0x%x\n",
		module->long_name,
		event_shifter
	);
	*/
	return 0;
}
static irqreturn_t mtk_isp_module_dip_isr(int irq, void *dev_id)
{
	struct isp_module *module = (struct isp_module *)dev_id;
	struct module_dip_context *dip_context = module->private_data;
	unsigned long isp_status, cq_int_status, cq_int2_status, cq_int3_status;
	unsigned long img3o_err_status, imgi_err_status;
	int i, j;
	struct rb_sync_handle *rbsync_hdle;
	int idx;
	unsigned int err_frm_thres;
	int *buf_map;
	int *wbuf_idx;

	spin_lock(&dip_context->dip_slot_lock);
	isp_status = ISP_RD32(
			module->hw_info.base_addr + DIP_REG_CTL_INT_STATUS);
	cq_int_status = ISP_RD32(
			module->hw_info.base_addr + DIP_REG_CTL_CQ_INT_STATUS);
	cq_int2_status = ISP_RD32(
			module->hw_info.base_addr + DIP_REG_CTL_CQ_INT2_STATUS);
	cq_int3_status = ISP_RD32(
			module->hw_info.base_addr + DIP_REG_CTL_CQ_INT3_STATUS);
	img3o_err_status = ISP_RD32(
			module->hw_info.base_addr + DIP_REG_IMG3O_ERR_STAT);
	imgi_err_status = ISP_RD32(
			module->hw_info.base_addr + DIP_REG_IMGI_ERR_STAT);

	for (i = 0; i < MAX_SLOT_SIZE; i++) {
		for (j = 0; j <= ISP_MODULE_APB_INTERFERE_STATUS; j++) {
			if ((isp_status >> j) & 1)
				dip_context->dip_event_slot[i].isp_raw_status |=
						(1L << j);
		}
		dip_context->dip_event_slot[i].cq_int_status = cq_int_status;
		dip_context->dip_event_slot[i].cq_int2_status = cq_int2_status;
		dip_context->dip_event_slot[i].cq_int3_status = cq_int3_status;
	}

	spin_unlock(&dip_context->dip_slot_lock);

	if ((module->parent_id == MTK_ISP_ISP2) ||
		(module->parent_id == MTK_ISP_ISP4)) {
		idx = ((module->parent_id ==  MTK_ISP_ISP2) ? 0 : 1);
		rbsync_hdle = dip_context->rbsync_handle;
		if (rbsync_hdle != NULL) {
			spin_lock(&rbsync_handle_lock[idx]);

			err_frm_thres =
			(unsigned int)(rbsync_hdle->error_frm_intervals);
			buf_map = &(rbsync_hdle->buf_map[0]);

			/* Pass2 done */
			if (isp_status & 0x00010000) {
				pr_debug("dev(%d), wt_frm(%x), rd_frm(%x)\n",
					module->parent_id,
					rbsync_hdle->wr.frm_idx,
					rbsync_hdle->rd.frm_idx);

				if ((rbsync_hdle->wr.frm_idx -
					 rbsync_hdle->rd.frm_idx) >
					err_frm_thres) {
					rbsync_hdle->status = -1;
					pr_err("Err:dev(%d) bufchase over %d ",
						module->parent_id,
						err_frm_thres);
					pr_err("wt_frm(%x), rd_frm(%x)\n",
						rbsync_hdle->wr.frm_idx,
						rbsync_hdle->rd.frm_idx);

				}
				rbsync_hdle->wr.frm_idx++;

				wbuf_idx = &dip_context->wbuf_idx;
				*wbuf_idx =
				((*wbuf_idx) + 1) % rbsync_hdle->buf_num;
				rbsync_hdle->wr.cur_buf_loc =
					buf_map[*wbuf_idx];
				pr_debug("next wt loc(%x)\n",
					rbsync_hdle->wr.cur_buf_loc);
			}
			spin_unlock(&rbsync_handle_lock[idx]);
		}
	}

	wake_up_interruptible(&module->wait_queue);
/* pr_info("%s isp:0x%lx, cq_int:0x%lx, cq_int2:0x%lx, cq_int3:0x%lx\n",
 *		module->long_name,
 *		isp_status,
 *		cq_int_status,
 *		cq_int2_status,
 *		cq_int3_status);
 */
	return IRQ_HANDLED;
}

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
static int mtk_isp_module_set_sgz_rbsync_handle(
	const struct isp_module *module, long kva)
{
	struct module_dip_context *dip_context = module->private_data;

	if (dip_context == NULL)
		return -EFAULT;

	dip_context->rbsync_handle = (struct rb_sync_handle *)kva;
	pr_debug("rbsync_handle kva=%lx\n", kva);

	if (dip_context->rbsync_handle) {
		pr_debug("rbsync_handle bufnum=%d\n",
			dip_context->rbsync_handle->buf_num);
		dip_context->wbuf_idx = 0;
	}

	return 0;
};

static int mtk_isp_module_dip_ioctl(const struct isp_module *module,
				unsigned int cmd, unsigned long arg)
{
	struct rbsync_info sgz_rbsyncinfo;
	int rc = 0;

	if (!module)
		return -EINVAL;

	pr_debug("custom_ioctl, module_id:%d\n", module->module_id);

	switch (cmd) {
	case ISP_SET_DIP_WARP_SB_INFO:
		if (copy_from_user(
			&sgz_rbsyncinfo, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;

		rc = mtk_isp_module_set_sgz_rbsync_handle(
					module, sgz_rbsyncinfo.kva);
		break;
	default:
		break;
	}

	return rc;
}
#endif

static int mtk_isp_module_dip_set_poll(struct file *filp, struct isp_module *module,
				       const struct event_req *e)
{
	struct module_dip_context *dip_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);

	if (event_module != ISP_MODULE_DIP)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP)
		return -EINVAL;

	dip_context->poll_event_mask |= (1L << event_shifter);
	dip_context->poll_target = filp;
	return 0;
}


static int mtk_isp_module_dip_check_poll(struct file *filp, struct isp_module *module)
{
	struct module_dip_context *dip_context = module->private_data;
	int idx = 0;
	int ret = 0;
	unsigned long flags;

	if (filp == dip_context->poll_target) {
		spin_lock_irqsave(&dip_context->dip_slot_lock, flags);
		ret = (dip_context->dip_event_slot[idx].isp_raw_status
			   & dip_context->poll_event_mask);
		spin_unlock_irqrestore(&dip_context->dip_slot_lock, flags);
	}
	return ret;
}

static int mtk_isp_module_dip_read(struct file *filp, struct isp_module *module)
{
	struct module_dip_context *dip_context = module->private_data;
	int idx = 0;
	unsigned long flags;
	int ret;

	ret = mtk_isp_module_dip_check_poll(filp, module);
	if (ret == 0)
		return 0;

	ret = (ret&(ret*(-1)));
	spin_lock_irqsave(&dip_context->dip_slot_lock, flags);
	dip_context->dip_event_slot[idx].isp_raw_status &= ~(ret);
	spin_unlock_irqrestore(&dip_context->dip_slot_lock, flags);

	return __ISP_MODULE_EVENT_CMD(ISP_MODULE_DIP,
				      ISP_MODULE_EVENT_TYPE_ISP,
				      ffs(ret)-1);
}

static struct isp_module_ops dip_ops = {
	.init = mtk_isp_module_dip_init,
	.stop = mtk_isp_module_dip_stop,
	.reset = mtk_isp_module_dip_reset,
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	.custom_ioctl = mtk_isp_module_dip_ioctl,
#endif
	.wait_module_event = mtk_isp_module_dip_wait_event,
	.flush_module_event = mtk_isp_module_dip_flush_event,
	.set_poll_trigger = mtk_isp_module_dip_set_poll,
	.check_poll_trigger = mtk_isp_module_dip_check_poll,
	.read_poll_trigger = mtk_isp_module_dip_read,
};

static const struct of_device_id isp_dip_of_ids[] = {
	{ .compatible = "mediatek,isp-imgsys", .data = "IMGSYS"},
	{ .compatible = "mediatek,isp-dip", .data = "DIP"},
	{}
};

static int mtk_isp_module_dip_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct isp_module *module_dip;
	struct module_dip_context *dip_context;
	int ret = 0;
	int num_items = 0;
	int i;

	if (!np) {
		pr_err("No device node for mtk isp module driver\n");
		return -ENODEV;
	}

	of_id = of_match_node(isp_dip_of_ids, np);
	if (!of_id)
		return -ENODEV;

	if (of_property_read_u32(np, "num-items", &num_items))
		return -EINVAL;

	for (i = 0; i < num_items; i++)	{
		module_dip = _isp_module_probe_index(pdev, i, of_id->data);
		if (IS_ERR(module_dip)) {
			ret = PTR_ERR(module_dip);
			goto err;
		}

		module_dip->module_dev = dev;

		dip_context = kzalloc(sizeof(struct module_dip_context),
								GFP_KERNEL);
		if (!dip_context) {
			ret = -ENOMEM;
			goto err;
		}

		spin_lock_init(&dip_context->dip_slot_lock);

		if (module_dip->parent_id ==  MTK_ISP_ISP2)
			spin_lock_init(&rbsync_handle_lock[0]);
		if (module_dip->parent_id ==  MTK_ISP_ISP4)
			spin_lock_init(&rbsync_handle_lock[1]);

		if (module_dip->hw_info.irq) {
			ret = request_irq(
				module_dip->hw_info.irq,
				mtk_isp_module_dip_isr,
				IRQF_TRIGGER_LOW | IRQF_SHARED,
				module_dip->long_name,
				module_dip);
			if (ret) {
				pr_err("failed to request_irq with irq %d for %s\n",
				module_dip->hw_info.irq, module_dip->name);
				goto err;
			}
		}

		module_dip->ops = &dip_ops;
		module_dip->private_data = dip_context;
		init_waitqueue_head(&module_dip->wait_queue);

		mtk_isp_register_isp_module(
			module_dip->parent_id,
			module_dip
		);

		platform_set_drvdata(pdev, module_dip);

		pr_debug("(map_addr=0x%lx irq=%d 0x%llx->0x%llx)\n",
			(unsigned long)module_dip->hw_info.base_addr,
			module_dip->hw_info.irq,
			module_dip->hw_info.start,
			module_dip->hw_info.end);
	}

	for (i = 0; i < IMGSYS_NUM; i++)
		atomic_set(&reset_cnt[i], 0);

#if defined(CONFIG_COMMON_CLK_MT3612)
	isp_module_pdm_enable(dev, module_dip);
	ret = isp_module_grp_clk_get(dev, &module_dip->clklist);
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
	isp_module_grp_clk_free(module_dip);
#endif
err:
	return ret;
}

static int mtk_isp_module_dip_remove(struct platform_device *pdev)
{
	struct isp_module *module_dip = platform_get_drvdata(pdev);
	struct module_dip_context *dip_context = module_dip->private_data;

	kfree(dip_context);
	mtk_isp_unregister_isp_module(module_dip);
	free_irq(module_dip->hw_info.irq, module_dip);
#if defined(CONFIG_COMMON_CLK_MT3612)
	isp_module_grp_clk_free(module_dip);
	isp_module_pdm_disable(&pdev->dev, module_dip);
#endif
	kfree(module_dip);

	return 0;
}

static struct platform_driver isp_module_dip_drvier = {
	.probe   = mtk_isp_module_dip_probe,
	.remove  = mtk_isp_module_dip_remove,
	.driver  = {
		.name  = "isp-dip",
		.owner = THIS_MODULE,
		.of_match_table = isp_dip_of_ids,
	}
};
module_platform_driver(isp_module_dip_drvier);
MODULE_DESCRIPTION("MTK Camera ISP Driver");
MODULE_LICENSE("GPL");
