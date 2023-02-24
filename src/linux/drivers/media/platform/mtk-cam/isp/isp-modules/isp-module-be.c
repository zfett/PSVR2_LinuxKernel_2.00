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

#include "../isp.h"
#include "../isp-registers.h"

enum isp_module_be_config {
	ISP_MODULE_BE_ENGINE_EN = 0,
	ISP_MODULE_BE_CG_DISABLE = 8,
};

struct module_be_events {
	unsigned long be_int_status;
};

struct module_be_context {
	struct module_be_events be_event_slot[MAX_SLOT_SIZE];
	int flush_trig;
	spinlock_t be_slot_lock;
	unsigned int work_frame_cnt;
	unsigned int poll_event_mask;
	struct file *poll_target;
};

static inline int mtk_isp_module_be_init(const struct isp_module *module)
{
	int idx = 0;
	struct module_be_context *be_context;

	if (!module)
		return -EFAULT;

	be_context = module->private_data;
	be_context->flush_trig = 0;
	be_context->work_frame_cnt = 0;
	for (idx = 0; idx < MAX_SLOT_SIZE; idx++)
		be_context->be_event_slot[idx].be_int_status = 0;

	return 0;
}

static inline int mtk_isp_module_be_stop(struct isp_module *module)
{
	if (!module)
		return -EFAULT;

	return 0;
}

static inline void mtk_isp_module_be_reset(const struct isp_module *module)
{
	if (!module)
		return;

	ISP_WR32(module->hw_info.base_addr + BE_REG_CTL_SW_RST, 0x1);
	mdelay(1);
	ISP_WR32(module->hw_info.base_addr + BE_REG_CTL_SW_RST, 0x0);

	pr_debug("%s virt:0x%lx phy:0x%lx off(0x%lx)\n",
		module->name,
		(unsigned long)module->hw_info.base_addr,
		(unsigned long)module->hw_info.start,
		(unsigned long)ISP_RD32(
		module->hw_info.base_addr + BE_REG_CTL_SW_RST)
	);
}

static inline int module_be_check_event(struct module_be_context *be_context,
	int idx, int type, int event_shifter)
{
	int result = 0;
	unsigned long flags;

	spin_lock_irqsave(&be_context->be_slot_lock, flags);
	if (type == ISP_MODULE_EVENT_TYPE_ISP)
		result = (be_context->be_event_slot[idx].be_int_status >>
			 event_shifter) & 1;
	spin_unlock_irqrestore(&be_context->be_slot_lock, flags);

	/* pr_debug("event: 0x%x, result: %d\n", event_shifter, result); */

	return result;
}

static int mtk_isp_module_be_wait_event(struct isp_module *module,
	const struct event_req *e)
{
	struct module_be_context *be_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = e->user_key;
	unsigned long flags;
	int ret;

	if (event_module != ISP_MODULE_BE)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP)
		return  -EINVAL;

	spin_lock_irqsave(&be_context->be_slot_lock, flags);
	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT &&
		!be_context->flush_trig) {
		be_context->be_event_slot[idx].be_int_status &=
			~(1L << event_shifter);
	}
	spin_unlock_irqrestore(&be_context->be_slot_lock, flags);

	ret = wait_event_interruptible_timeout(
			module->wait_queue,
			module_be_check_event(
				be_context,
				idx,
				event_type,
				event_shifter
			),
			mtk_isp_ms_to_jiffies(e->timeout)
		);

	if (ret == 0) {
		pr_err("wait event timeout\n");
		return -EIO;
	} else if (ret == -ERESTARTSYS) {
		pr_err("wait is interrupted by a signal\n");
		return -ERESTARTSYS;
	}

	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT ||
				e->mode == ISP_IRQ_MODE_CLEAR_NONE) {
		spin_lock_irqsave(&be_context->be_slot_lock, flags);
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			be_context->be_event_slot[idx].be_int_status &=
				~(1L << event_shifter);
		spin_unlock_irqrestore(&be_context->be_slot_lock, flags);
	}

	return 0;
}

static int mtk_isp_module_be_flush_event(struct isp_module *module,
					  const struct event_req *e)
{
	struct module_be_context *be_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = 0;
	unsigned long flags;

	if (event_module != ISP_MODULE_BE)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP)
		return -EINVAL;

	spin_lock_irqsave(&be_context->be_slot_lock, flags);

	for (idx = 0; idx < MAX_SLOT_SIZE; idx++) {
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			be_context->be_event_slot[idx].be_int_status |=
				(1L << event_shifter);
	}

	be_context->flush_trig = 1;

	spin_unlock_irqrestore(&be_context->be_slot_lock, flags);
	wake_up_interruptible(&module->wait_queue);
	return 0;
}

static int mtk_isp_module_be_set_poll(struct file *filp, struct isp_module *module,
									  const struct event_req *e)
{
	struct module_be_context *be_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);

	if (event_module != ISP_MODULE_BE)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP)
		return -EINVAL;

	be_context->poll_event_mask |= (1L << event_shifter);
	be_context->poll_target = filp;
	return 0;
}

static int mtk_isp_module_be_check_poll(struct file *filp, struct isp_module *module)
{
	struct module_be_context *be_context = module->private_data;
	int idx = 0;
	int ret = 0;
	unsigned long flags;

	if (filp == be_context->poll_target) {
		spin_lock_irqsave(&be_context->be_slot_lock, flags);
		ret = (be_context->be_event_slot[idx].be_int_status
			   & be_context->poll_event_mask);
		spin_unlock_irqrestore(&be_context->be_slot_lock, flags);
	}
	return ret;
}

static int mtk_isp_module_be_read(struct file *filp, struct isp_module *module)
{
	struct module_be_context *be_context = module->private_data;
	int idx = 0;
	unsigned long flags;
	int ret;

	ret = mtk_isp_module_be_check_poll(filp, module);
	if (ret == 0)
		return 0;

	ret = (ret&(ret*(-1)));
	spin_lock_irqsave(&be_context->be_slot_lock, flags);
	be_context->be_event_slot[idx].be_int_status &= ~(ret);
	spin_unlock_irqrestore(&be_context->be_slot_lock, flags);

	return __ISP_MODULE_EVENT_CMD(ISP_MODULE_BE,
				      ISP_MODULE_EVENT_TYPE_ISP,
				      ffs(ret)-1);
}

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
static int mtk_isp_module_be_ioctl(const struct isp_module *module,
				unsigned int cmd, unsigned long arg)
{
	return 0;
}
#endif

static irqreturn_t mtk_isp_module_be_isr(int irq, void *dev_id)
{
	struct isp_module *module = (struct isp_module *)dev_id;
	struct module_be_context *be_context = module->private_data;
	unsigned long be_status, be_cfg;
	unsigned int i, j;
	unsigned int be_addr;

	spin_lock(&be_context->be_slot_lock);
	be_status = ISP_RD32(module->hw_info.base_addr + BE_REG_CTL_INT_STATUS);
	be_cfg = ISP_RD32(module->hw_info.base_addr + BE_REG_CTL_CFG);

	/* BE done, work_frame_cnt release */
	if ((be_status >> ISP_MODULE_BE_EVENT_OUT_DONE) & 1)
		be_context->work_frame_cnt--;

	if ((be_status >> ISP_MODULE_BE_EVENT_SOF) & 1) {
		/* judge if occur incomplete frame, should drop */
		if ((be_status >> ISP_MODULE_BE_EVENT_DB_ERR) & 1)
			be_status =
				be_status & (~(1 << ISP_MODULE_BE_EVENT_SOF));

		/* detect no BE done if work_frame_cnt is not zero */
		if (be_context->work_frame_cnt) {
			pr_err("BE incomplete frame\n");
			be_context->work_frame_cnt = 0;
		}
		/* BE sof, work_frame_cnt add */
		be_context->work_frame_cnt++;
	}

	/* P1 sof, clear error status */
	if ((be_status >> ISP_MODULE_BE_EVENT_P1_SOF) & 1)
		ISP_WR32(module->hw_info.base_addr + BE_SW_REG_14, 0);

	if ((be_status >> ISP_MODULE_BE_EVENT_LABEL_ERR) & 1)
		pr_debug("BE label > 1023, over limitation\n");

	if ((be_status >> ISP_MODULE_BE_EVENT_MERGE_STACK_ERR) & 1)
		pr_debug("BE merge stack > 64, over limitation\n");

	if ((be_status >> ISP_MODULE_BE_EVENT_DATA_TABEL_ERR) & 1)
		pr_debug("BE data table > 256, over limitation\n");

	if (be_status & ((1 << ISP_MODULE_BE_EVENT_LABEL_ERR) |
					(1 << ISP_MODULE_BE_EVENT_MERGE_STACK_ERR) |
					(1 << ISP_MODULE_BE_EVENT_DATA_TABEL_ERR))) {
		/* keep error status */
		ISP_WR32(module->hw_info.base_addr + BE_SW_REG_14,
			be_status & ((1 << ISP_MODULE_BE_EVENT_LABEL_ERR) |
					(1 << ISP_MODULE_BE_EVENT_MERGE_STACK_ERR) |
					(1 << ISP_MODULE_BE_EVENT_DATA_TABEL_ERR)));
	}

	/* subsmaple change buffer */
	if (ISP_RD32(module->hw_info.base_addr + BE_SW_REG_00)) {

		if ((be_status >> ISP_MODULE_BE_EVENT_HW_PASS1_DONE) & 1) {
			be_addr = ISP_RD32(module->hw_info.base_addr +
				BE_SW_REG_02);
			ISP_WR32(module->hw_info.base_addr +
				BE_WDMA_CTRL0, be_addr);
			be_addr = ISP_RD32(module->hw_info.base_addr +
				BE_SW_REG_06);
			ISP_WR32(module->hw_info.base_addr +
				BE_WDMA_CTRL1, be_addr);
			be_addr = ISP_RD32(module->hw_info.base_addr +
				BE_SW_REG_10);
			ISP_WR32(module->hw_info.base_addr +
				BE_WDMA_CTRL2, be_addr);
		}

		if ((be_status >> ISP_MODULE_BE_EVENT_SW_PASS1_DONE) & 1) {
			/* swap buffer */
			be_addr = ISP_RD32(module->hw_info.base_addr +
					BE_SW_REG_03);
			ISP_WR32(module->hw_info.base_addr +
					BE_SW_REG_01, be_addr);
			/* update addr */
			ISP_WR32(module->hw_info.base_addr +
					BE_WDMA_CTRL0, be_addr);
			be_addr = ISP_RD32(module->hw_info.base_addr +
					BE_SW_REG_04);
			ISP_WR32(module->hw_info.base_addr +
					BE_SW_REG_02, be_addr);

			be_addr = ISP_RD32(module->hw_info.base_addr +
					BE_SW_REG_07);
			ISP_WR32(module->hw_info.base_addr +
					BE_SW_REG_05, be_addr);
			/* update addr */
			ISP_WR32(module->hw_info.base_addr +
					BE_WDMA_CTRL1, be_addr);
			be_addr = ISP_RD32(module->hw_info.base_addr +
					BE_SW_REG_08);
			ISP_WR32(module->hw_info.base_addr +
					BE_SW_REG_06, be_addr);

			be_addr = ISP_RD32(module->hw_info.base_addr +
					BE_SW_REG_11);
			ISP_WR32(module->hw_info.base_addr +
					BE_SW_REG_09, be_addr);
			/* update addr */
			ISP_WR32(module->hw_info.base_addr +
					BE_WDMA_CTRL2, be_addr);
			be_addr = ISP_RD32(module->hw_info.base_addr +
					BE_SW_REG_12);
			ISP_WR32(module->hw_info.base_addr +
					BE_SW_REG_10, be_addr);

		}
	}

	for (i = 0; i < MAX_SLOT_SIZE; i++) {
		for (j = 0; j <= ISP_MODULE_BE_EVENT_SW_PASS1_DONE; j++) {
			if ((be_status >> j) & 1)
				be_context->be_event_slot[i].be_int_status |=
					(1L << j);
		}
	}

	spin_unlock(&be_context->be_slot_lock);
	wake_up_interruptible(&module->wait_queue);

	return IRQ_HANDLED;
}

static struct isp_module_ops be_ops = {
	.init = mtk_isp_module_be_init,
	.stop = mtk_isp_module_be_stop,
	.wait_module_event = mtk_isp_module_be_wait_event,
	.flush_module_event = mtk_isp_module_be_flush_event,
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	.custom_ioctl = mtk_isp_module_be_ioctl,
#endif
	.reset = mtk_isp_module_be_reset,
	.set_poll_trigger = mtk_isp_module_be_set_poll,
	.check_poll_trigger = mtk_isp_module_be_check_poll,
	.read_poll_trigger = mtk_isp_module_be_read,
};

static const struct of_device_id isp_be_of_ids[] = {
	{ .compatible = "mediatek,isp-be", .data = "BE"},
	{}
};


static int mtk_isp_module_be_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct isp_module *module_be;
	struct module_be_context *be_context;
	int ret = 0;
	int num_items = 0;
	int i;

	if (!np) {
		pr_err("No device node for mtk isp module driver\n");
		return -ENODEV;
	}

	of_id = of_match_node(isp_be_of_ids, np);
	if (!of_id)
		return -ENODEV;

	if (of_property_read_u32(np, "num-items", &num_items))
		return -EINVAL;

	for (i = 0; i < num_items; i++) {
		module_be = _isp_module_probe_index(pdev, i, of_id->data);
		if (IS_ERR(module_be)) {
			ret = PTR_ERR(module_be);
			goto err;
		}

		module_be->module_dev = dev;

		be_context = kzalloc(sizeof(struct module_be_context),
								GFP_KERNEL);
		if (!be_context) {
			ret = -ENOMEM;
			goto err;
		}

		spin_lock_init(&be_context->be_slot_lock);

		init_waitqueue_head(&module_be->wait_queue);
		module_be->ops = &be_ops;
		module_be->private_data = be_context;

		if (module_be->hw_info.irq) {
			ret = request_irq(
				module_be->hw_info.irq,
				mtk_isp_module_be_isr,
				IRQF_TRIGGER_LOW | IRQF_SHARED,
				module_be->long_name,
				module_be
			 );
			if (ret) {
				pr_err("failed to request_irq with irq %d for %s\n",
					module_be->hw_info.irq,
					module_be->name);
				goto err;
			}
		}

		mtk_isp_register_isp_module(
			module_be->parent_id, module_be);

		platform_set_drvdata(pdev, module_be);

		pr_debug("(map_addr=0x%lx irq=%d 0x%llx->0x%llx)\n",
			(unsigned long)module_be->hw_info.base_addr,
			module_be->hw_info.irq,
			module_be->hw_info.start,
			module_be->hw_info.end
		);
	}

	return 0;

err:
	return ret;
}

static int mtk_isp_module_be_remove(struct platform_device *pdev)
{
	struct isp_module *module_be = platform_get_drvdata(pdev);
	struct module_be_context *be_context = module_be->private_data;

	mtk_isp_unregister_isp_module(module_be);
	free_irq(module_be->hw_info.irq, module_be);

	kfree(be_context);
	kfree(module_be);

	return 0;
}

static struct platform_driver isp_module_be_driver = {
	.probe = mtk_isp_module_be_probe,
	.remove = mtk_isp_module_be_remove,
	.driver = {
		.name = "isp-be",
		.owner = THIS_MODULE,
		.of_match_table = isp_be_of_ids,
	}
};

module_platform_driver(isp_module_be_driver);
MODULE_DESCRIPTION("MTK Camera ISP Driver");
MODULE_LICENSE("GPL");
