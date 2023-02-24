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

struct module_camsv_events {
	unsigned long isp_raw_status;
};

struct module_camsv_context {
	struct module_camsv_events camsv_event_slot[MAX_SLOT_SIZE];
	int flush_trig;
	spinlock_t camsv_slot_lock;
	unsigned int poll_event_mask;
	struct file *poll_target;
};

static inline int mtk_isp_module_camsv_init(const struct isp_module *module)
{
	struct module_camsv_context *camsv_context;
	if (!module)
		return -EFAULT;
	camsv_context = module->private_data;
	camsv_context->flush_trig = 0;
	return 0;
}

static inline int mtk_isp_module_camsv_stop(struct isp_module *module)
{
	if (!module)
		return -EFAULT;

	return 0;
}

static inline int module_camsv_check_event(
	struct module_camsv_context *camsv_context,
	int idx,
	int type,
	int event_shifter)
{
	int result = 0;
	unsigned long flags;

	spin_lock_irqsave(&camsv_context->camsv_slot_lock, flags);
	if (type == ISP_MODULE_EVENT_TYPE_ISP)
		result = (camsv_context->camsv_event_slot[idx].isp_raw_status >>
			  event_shifter) & 1;
	spin_unlock_irqrestore(&camsv_context->camsv_slot_lock, flags);

	/* pr_debug("event:0x%x, result:%d\n", event_shifter, result); */

	return result;
}

static int mtk_isp_module_camsv_flush_event(struct isp_module *module,
					  const struct event_req *e)
{
	struct module_camsv_context *camsv_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = 0;
	unsigned long flags;

	if (event_module != ISP_MODULE_CAMSV)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP)
		return -EINVAL;

	spin_lock_irqsave(&camsv_context->camsv_slot_lock, flags);

	for (idx = 0; idx < MAX_SLOT_SIZE; idx++) {
		camsv_context->camsv_event_slot[idx].isp_raw_status |=
			(1L << event_shifter);
	}

	camsv_context->flush_trig = 1;

	spin_unlock_irqrestore(&camsv_context->camsv_slot_lock, flags);

	wake_up_interruptible(&module->wait_queue);

	return 0;
}

static int mtk_isp_module_camsv_wait_event(
	struct isp_module *module,
	const struct event_req *e)
{
	struct module_camsv_context *camsv_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);
	int idx = e->user_key;
	unsigned long flags;
	int ret;

	if (event_module != ISP_MODULE_CAMSV)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP)
		return -EINVAL;

	spin_lock_irqsave(&camsv_context->camsv_slot_lock, flags);
	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT &&
		!camsv_context->flush_trig) {
		camsv_context->camsv_event_slot[idx].isp_raw_status &=
			~(1L << event_shifter);
	}
	spin_unlock_irqrestore(&camsv_context->camsv_slot_lock, flags);

	if (e->mode == ISP_IRQ_MODE_CLEAR_NONE) {
		if (camsv_context->camsv_event_slot[idx].isp_raw_status &
			(1L << event_shifter)) {
			goto NON_CLEAR_WAIT;
		}
	}

	ret = wait_event_interruptible_timeout(
		module->wait_queue,
		module_camsv_check_event(
			camsv_context,
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

NON_CLEAR_WAIT:

	if (e->mode == ISP_IRQ_MODE_CLEAR_WAIT ||
				e->mode == ISP_IRQ_MODE_CLEAR_NONE) {
		spin_lock_irqsave(&camsv_context->camsv_slot_lock, flags);
		if (event_type == ISP_MODULE_EVENT_TYPE_ISP)
			camsv_context->camsv_event_slot[idx].isp_raw_status &=
				~(1L << event_shifter);
		spin_unlock_irqrestore(&camsv_context->camsv_slot_lock, flags);
	}

	pr_debug("%s received event 0x%x\n",
			module->name, event_shifter);

	return 0;
}

static void mtk_isp_module_camsv_reset(const struct isp_module *module)
{
	if (!module)
		return;

	ISP_WR32(module->hw_info.base_addr + CAMSV_REG_CTL_SW_CTL,
				0x5);
	mdelay(1);
	ISP_WR32(module->hw_info.base_addr + CAMSV_REG_CTL_SW_CTL,
				0x1);
	ISP_WR32(module->hw_info.base_addr + CAMSV_REG_CTL_SW_CTL,
				0x0);

	pr_debug("%s virt:0x%lx phy:0x%lx off(0x%lx)\n",
		module->name,
		(unsigned long)module->hw_info.base_addr,
		(unsigned long)module->hw_info.start,
		(unsigned long)ISP_RD32(
		module->hw_info.base_addr + CAMSV_REG_CTL_SW_CTL)
	);
}

static int mtk_isp_module_camsv_set_poll(struct file *filp, struct isp_module *module,
					 const struct event_req *e)
{
	struct module_camsv_context *camsv_context = module->private_data;
	int event_module = DECODE_EVENT_CMD_EVENT_MODULE(e->event);
	int event_type = DECODE_EVENT_CMD_EVENT_TYPE(e->event);
	int event_shifter = DECODE_EVENT_CMD_EVENT(e->event);

	if (event_module != ISP_MODULE_CAMSV)
		return -EINVAL;

	if (event_type != ISP_MODULE_EVENT_TYPE_ISP)
		return -EINVAL;

	camsv_context->poll_event_mask |= (1L << event_shifter);
	camsv_context->poll_target     = filp;
	return 0;
}


static int mtk_isp_module_camsv_check_poll(struct file *filp, struct isp_module *module)
{
	struct module_camsv_context *camsv_context = module->private_data;
	int idx = 0;
	int ret = 0;
	unsigned long flags;

	if (filp == camsv_context->poll_target) {
		spin_lock_irqsave(&camsv_context->camsv_slot_lock, flags);
		ret = (camsv_context->camsv_event_slot[idx].isp_raw_status
			   & camsv_context->poll_event_mask);
		spin_unlock_irqrestore(&camsv_context->camsv_slot_lock, flags);
	}

	return ret;
}

static int mtk_isp_module_camsv_read(struct file *filp, struct isp_module *module)
{
	struct module_camsv_context *camsv_context = module->private_data;
	int idx = 0;
	unsigned long flags;
	int ret;

	ret = mtk_isp_module_camsv_check_poll(filp, module);
	if (ret == 0)
		return 0;

	ret = (ret&(ret*(-1)));
	spin_lock_irqsave(&camsv_context->camsv_slot_lock, flags);
	camsv_context->camsv_event_slot[idx].isp_raw_status &= ~(ret);
	spin_unlock_irqrestore(&camsv_context->camsv_slot_lock, flags);

	return __ISP_MODULE_EVENT_CMD(ISP_MODULE_CAMSV,
				      ISP_MODULE_EVENT_TYPE_ISP,
				      ffs(ret)-1);
}

static struct isp_module_ops camsv_ops = {
	.init = mtk_isp_module_camsv_init,
	.stop = mtk_isp_module_camsv_stop,
	.wait_module_event = mtk_isp_module_camsv_wait_event,
	.flush_module_event = mtk_isp_module_camsv_flush_event,
	.reset = mtk_isp_module_camsv_reset,
	.set_poll_trigger = mtk_isp_module_camsv_set_poll,
	.check_poll_trigger = mtk_isp_module_camsv_check_poll,
	.read_poll_trigger = mtk_isp_module_camsv_read,
};

static const struct of_device_id isp_camsv_of_ids[] = {
	{ .compatible = "mediatek,isp-camsv", .data = "CAMSV"},
	{}
};

static irqreturn_t mtk_isp_module_camsv_isr(int irq, void *dev_id)
{
	struct isp_module *module = (struct isp_module *)dev_id;
	struct module_camsv_context *camsv_context = module->private_data;
	unsigned long isp_status;
	unsigned long addr_org, addr;
	int i, j;

	spin_lock(&camsv_context->camsv_slot_lock);
	isp_status = ISP_RD32(
		module->hw_info.base_addr + CAMSV_REG_CTL_INT_STATUS);

	if ((isp_status >> ISP_MODULE_CAMSV_EVENT_SW_PASS1_DONE) & 1) {
		addr_org = ISP_RD32(module->hw_info.base_addr +
			CAMSV_REG_SPARE1);
		addr = ISP_RD32(module->hw_info.base_addr + CAMSV_REG_SPARE0);
		if (addr_org == addr) {
			isp_status = isp_status &
				(~(1 << ISP_MODULE_CAMSV_EVENT_SW_PASS1_DONE));
			/* pr_info("Mask P1Done(0x%x_0x%x), status(0x%x)\n", */
			/* addr_org, addr, isp_status); */
		} else {
			ISP_WR32(module->hw_info.base_addr + CAMSV_REG_SPARE1,
				addr);
			/* pr_info("P1_Done update spare1:0x%lx\n", addr); */
		}
	}

	if ((isp_status >> ISP_MODULE_CAMSV_EVENT_TG_SOF) & 1) {
		addr = ISP_RD32(module->hw_info.base_addr +
			CAMSV_IMGO_BASE_ADDR);
		ISP_WR32(module->hw_info.base_addr + CAMSV_REG_SPARE0, addr);
		/* pr_info("SOF update spare0:0x%lx\n", addr); */
	}

	for (i = 0; i < MAX_SLOT_SIZE; i++) {
		for (j = 0; j <= ISP_MODULE_CAMSV_EVENT_TG_SOF_WAIT; j++) {
			if ((isp_status >> j) & 1)
				camsv_context->camsv_event_slot[i].
						isp_raw_status |= (1L << j);
		}
	}
	spin_unlock(&camsv_context->camsv_slot_lock);

	wake_up_interruptible(&module->wait_queue);

	/* pr_info("%s camsv-isp:0x%lx\n", module->long_name, isp_status); */

	return IRQ_HANDLED;
}


static int mtk_isp_module_camsv_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct isp_module *module_camsv;
	struct module_camsv_context *camsv_context;
	int ret = 0;
	int num_items = 0;
	int i;

	if (!np) {
		pr_err("No device node for mtk isp module driver\n");
		return -ENODEV;
	}

	of_id = of_match_node(isp_camsv_of_ids, np);
	if (!of_id)
		return -ENODEV;

	if (of_property_read_u32(np, "num-items", &num_items))
		return -EINVAL;

	for (i = 0; i < num_items; i++)	{
		module_camsv = _isp_module_probe_index(pdev, i, of_id->data);
		if (IS_ERR(module_camsv)) {
			ret = PTR_ERR(module_camsv);
			goto err;
		}

		module_camsv->module_dev = dev;

		camsv_context = kzalloc(sizeof(struct module_camsv_context),
								GFP_KERNEL);
		if (!camsv_context) {
			ret = -ENOMEM;
			goto err;
		}

		spin_lock_init(&camsv_context->camsv_slot_lock);

		init_waitqueue_head(&module_camsv->wait_queue);
		module_camsv->ops = &camsv_ops;
		module_camsv->private_data = camsv_context;

		if (module_camsv->hw_info.irq) {
			ret = request_irq(
				module_camsv->hw_info.irq,
				mtk_isp_module_camsv_isr,
				IRQF_TRIGGER_LOW | IRQF_SHARED,
				module_camsv->long_name,
				module_camsv
			 );
			if (ret) {
				pr_err("failed to request_irq with irq %d for %s\n",
					module_camsv->hw_info.irq,
					module_camsv->name);
				goto err;
			}
		}

		mtk_isp_register_isp_module(
			module_camsv->parent_id, module_camsv);

		platform_set_drvdata(pdev, module_camsv);

		pr_info("(map_addr=0x%lx irq=%d 0x%llx->0x%llx)\n",
			(unsigned long)module_camsv->hw_info.base_addr,
			module_camsv->hw_info.irq,
			module_camsv->hw_info.start,
			module_camsv->hw_info.end
		);
	}

	return 0;

err:
	return ret;
}

static int mtk_isp_module_camsv_remove(struct platform_device *pdev)
{
	struct isp_module *module_camsv = platform_get_drvdata(pdev);
	struct module_camsv_context *camsv_context = module_camsv->private_data;

	mtk_isp_unregister_isp_module(module_camsv);
	free_irq(module_camsv->hw_info.irq, module_camsv);
	kfree(camsv_context);
	kfree(module_camsv);

	return 0;
}

static struct platform_driver isp_module_camsv_drvier = {
	.probe   = mtk_isp_module_camsv_probe,
	.remove  = mtk_isp_module_camsv_remove,
	.driver  = {
		.name  = "isp-camsv",
		.owner = THIS_MODULE,
		.of_match_table = isp_camsv_of_ids,
	}
};
module_platform_driver(isp_module_camsv_drvier);
MODULE_DESCRIPTION("MTK Camera ISP Driver");
MODULE_LICENSE("GPL");
