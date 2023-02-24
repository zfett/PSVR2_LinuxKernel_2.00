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

#include <dt-bindings/camera.h>

#include "../isp.h"
#include "../isp-registers.h"


static int mtk_isp_module_rbfc_init(const struct isp_module *module)
{
	int ret = 0;

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
	if (module->parent_id == MTK_ISP_ISP4 ||
		module->parent_id == MTK_ISP_ISP5) {
		switch (module->module_id) {
		case MTK_ISP_ISP_RBFC_WEN_A:
		case MTK_ISP_ISP_RBFC_WEN_B:
			ISP_WR32(module->hw_info.base_addr + RBFC_DCM_CON,
				0xEF);
			break;
		}
	}
	return ret;
}

static inline int mtk_isp_module_rbfc_stop(struct isp_module *module)
{
	int ret = 0;

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

	return ret;
}

static void mtk_isp_module_rbfc_reset(const struct isp_module *module)
{
	if (!module)
		return;

	switch (module->module_id) {
	case MTK_ISP_ISP_RBFC_RRZO_A:
	case MTK_ISP_ISP_RBFC_RRZO_B:
	case MTK_ISP_ISP_RBFC_REN_A:
	case MTK_ISP_ISP_RBFC_REN_B:
	case MTK_ISP_ISP_RBFC_WEN_A:
	case MTK_ISP_ISP_RBFC_WEN_B:
		/* Reset ISP RBFC. 0x1: RBFC reset, 0x0: no reset */
		ISP_WR32(module->hw_info.base_addr + RBFC_RSTCTL, 0x1);
		mdelay(1);
		ISP_WR32(module->hw_info.base_addr + RBFC_RSTCTL, 0x0);
		break;
	}
}

static struct isp_module_ops rbfc_ops = {
	.init = mtk_isp_module_rbfc_init,
	.stop = mtk_isp_module_rbfc_stop,
	.reset = mtk_isp_module_rbfc_reset,
};

static const struct of_device_id isp_rbfc_of_ids[] = {
	{ .compatible = "mediatek,isp-rbfc", .data = "RBFC"},
	{ .compatible = "mediatek,isp-rbfc-wpe", .data = "RBFC-WPE"},
	{}
};

static int mtk_isp_module_rbfc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct isp_module *module_rbfc;
	int ret = 0;
	int num_items = 0;
	int i;

	if (!np) {
		pr_err("No device node for mtk isp module driver\n");
		return -ENODEV;
	}

	of_id = of_match_node(isp_rbfc_of_ids, np);
	if (!of_id)
		return -ENODEV;

	if (of_property_read_u32(np, "num-items", &num_items))
		return -EINVAL;

	for (i = 0; i < num_items; i++)	{
		module_rbfc = _isp_module_probe_index(pdev, i, of_id->data);
		if (IS_ERR(module_rbfc)) {
			ret = PTR_ERR(module_rbfc);
			goto err;
		}
		module_rbfc->module_dev = dev;

		module_rbfc->ops = &rbfc_ops;
		init_waitqueue_head(&module_rbfc->wait_queue);

		mtk_isp_register_isp_module(
			module_rbfc->parent_id,
			module_rbfc);
		platform_set_drvdata(pdev, module_rbfc);

		pr_debug("(map_addr=0x%lx irq=%d 0x%llx->0x%llx)\n",
			(unsigned long)module_rbfc->hw_info.base_addr,
			module_rbfc->hw_info.irq,
			module_rbfc->hw_info.start,
			module_rbfc->hw_info.end);
	}

#if defined(CONFIG_COMMON_CLK_MT3612)
	isp_module_pdm_enable(dev, module_rbfc);
#endif

	return 0;

err:
	return ret;
}

static int mtk_isp_module_rbfc_remove(struct platform_device *pdev)
{
	struct isp_module *module_rbfc = platform_get_drvdata(pdev);

	mtk_isp_unregister_isp_module(module_rbfc);
#if defined(CONFIG_COMMON_CLK_MT3612)
	isp_module_pdm_disable(&pdev->dev, module_rbfc);
#endif
	kfree(module_rbfc);

	return 0;
}

static struct platform_driver isp_module_rbfc_drvier = {
	.probe   = mtk_isp_module_rbfc_probe,
	.remove  = mtk_isp_module_rbfc_remove,
	.driver  = {
		.name  = "isp-rbfc",
		.owner = THIS_MODULE,
		.of_match_table = isp_rbfc_of_ids,
	}
};
module_platform_driver(isp_module_rbfc_drvier);
MODULE_DESCRIPTION("MTK Camera ISP Driver");
MODULE_LICENSE("GPL");
