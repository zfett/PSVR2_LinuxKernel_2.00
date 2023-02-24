/*
* Copyright (C) 2017 MediaTek Inc.
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

#include "../isp.h"
#include "../isp-registers.h"
#include <dt-bindings/camera.h>
#include <linux/nvmem-consumer.h>

static int mtk_csi_config_by_efuse(const struct isp_module *module)
{
	struct nvmem_cell *cell;
	size_t len;
	u32 *buf;
	int ret, cnt, num_lanes = 0;
	struct device *dev = NULL;
	struct device_node *np = NULL;
	u8 efuse_buf[6]; /* ck_p, ck_n, d0_p, d0_n, d1_p, d1_n */
	u32 csia_ana08_code, csia_ana0c_code, buf_temp;

	pr_debug("set csi rx impedance by efuse information\n");

	if (IS_ERR(module->private_data)) {
		ret = PTR_ERR(module->private_data);
		pr_err("failed to get isp_module\n");
		return -ENODEV;
	}
	dev = module->private_data;

	if (IS_ERR(dev->of_node)) {
		ret = PTR_ERR(dev->of_node);
		pr_err("failed to get device node\n");
		return -ENODEV;
	}
	np = dev->of_node;
	of_property_read_u32(np, "num-lanes", &num_lanes);

	for (cnt = 0; cnt < 6; cnt++)
		efuse_buf[cnt] = 0x10;

	cell = nvmem_cell_get(dev, "phy-csi");
	if (IS_ERR_OR_NULL(cell)) {
		ret = PTR_ERR(cell);
		pr_err("failed to get nvmem_cell\n");
		goto err;
	}

	buf = (u32 *)nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);

	if (IS_ERR(buf)) {
		ret = PTR_ERR(buf);
		pr_err("failed to get efuse value\n");
		goto err;
	}

	switch (module->module_id) {
	case MTK_ISP_ISP_CSI0: /* SIDE_LL */
		pr_debug("sensor: SIDE_LL, num_lanes:%d\n", num_lanes);
		buf_temp = buf[0];
		efuse_buf[0] = (u8)((buf_temp >> 0) & 0x0000001f);
		efuse_buf[1] = (u8)((buf_temp >> 5) & 0x0000001f);
		efuse_buf[2] = (u8)((buf_temp >> 10) & 0x0000001f);
		efuse_buf[3] = (u8)((buf_temp >> 15) & 0x0000001f);
		efuse_buf[4] = (u8)((buf_temp >> 20) & 0x0000001f);
		efuse_buf[5] = (u8)((buf_temp >> 25) & 0x0000001f);
		break;

	case MTK_ISP_ISP_CSI1: /* SIDE_LR */
		pr_debug("sensor: SIDE_LR, num_lanes:%d\n", num_lanes);
		buf_temp = buf[1];
		efuse_buf[0] = (u8)((buf_temp >> 0) & 0x0000001f);
		efuse_buf[1] = (u8)((buf_temp >> 5) & 0x0000001f);
		efuse_buf[2] = (u8)((buf_temp >> 10) & 0x0000001f);
		efuse_buf[3] = (u8)((buf_temp >> 15) & 0x0000001f);
		efuse_buf[4] = (u8)((buf_temp >> 20) & 0x0000001f);
		efuse_buf[5] = (u8)((buf_temp >> 25) & 0x0000001f);
		break;

	case MTK_ISP_ISP_CSI2: /* SIDE_RL */
		pr_debug("sensor: SIDE_RL, num_lanes:%d\n", num_lanes);
		buf_temp = buf[2];
		efuse_buf[0] = (u8)((buf_temp >> 0) & 0x0000001f);
		efuse_buf[1] = (u8)((buf_temp >> 5) & 0x0000001f);
		efuse_buf[2] = (u8)((buf_temp >> 10) & 0x0000001f);
		efuse_buf[3] = (u8)((buf_temp >> 15) & 0x0000001f);
		efuse_buf[4] = (u8)((buf_temp >> 20) & 0x0000001f);
		efuse_buf[5] = (u8)((buf_temp >> 25) & 0x0000001f);
		break;

	case MTK_ISP_ISP_CSI3: /* SIDE_RR */
		pr_debug("sensor: SIDE_RR, num_lanes:%d\n", num_lanes);
		buf_temp = buf[3];
		efuse_buf[0] = (u8)((buf_temp >> 0) & 0x0000001f);
		efuse_buf[1] = (u8)((buf_temp >> 5) & 0x0000001f);
		efuse_buf[2] = (u8)((buf_temp >> 10) & 0x0000001f);
		efuse_buf[3] = (u8)((buf_temp >> 15) & 0x0000001f);
		efuse_buf[4] = (u8)((buf_temp >> 20) & 0x0000001f);
		efuse_buf[5] = (u8)((buf_temp >> 25) & 0x0000001f);
		break;

	case MTK_ISP_ISP_CSI4: /* GAZE_L */
		pr_debug("sensor: GAZE_L, num_lanes:%d\n", num_lanes);
		buf_temp = buf[4];
		efuse_buf[0] = (u8)((buf_temp >> 0) & 0x0000001f);
		efuse_buf[1] = (u8)((buf_temp >> 5) & 0x0000001f);
		efuse_buf[2] = (u8)((buf_temp >> 10) & 0x0000001f);
		efuse_buf[3] = (u8)((buf_temp >> 15) & 0x0000001f);
		break;

	case MTK_ISP_ISP_CSI5: /* GAZE_R */
		pr_debug("sensor: GAZE_R, num_lanes:%d\n", num_lanes);
		buf_temp = buf[5];
		efuse_buf[0] = (u8)((buf_temp >> 0) & 0x0000001f);
		efuse_buf[1] = (u8)((buf_temp >> 5) & 0x0000001f);
		efuse_buf[2] = (u8)((buf_temp >> 10) & 0x0000001f);
		efuse_buf[3] = (u8)((buf_temp >> 15) & 0x0000001f);
		break;

	default:
		pr_err("invalid module_id\n");
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);

	csia_ana08_code = ((u32)efuse_buf[3] << 24)|
		((u32)efuse_buf[2] << 16)|((u32)efuse_buf[1] << 8)|
		((u32)efuse_buf[0] << 0);

	/*Efuse empty protection  */
	if (csia_ana08_code == 0)
		csia_ana08_code = 0x10101010; /* set to default value */

	ISP_WR32(module->hw_info.base_addr + MIPI_REG_ANA08_CSI0A,
			csia_ana08_code);

	if (num_lanes > 1) { /* for 2D1C case */
		csia_ana0c_code = ((u32)efuse_buf[5] << 8)|
						  ((u32)efuse_buf[4] << 0);
		/*Efuse empty protection  */
		if (csia_ana0c_code == 0)
			csia_ana0c_code = 0x00001010; /* set to default value */

		ISP_WR32(module->hw_info.base_addr + MIPI_REG_ANA0C_CSI0A,
				csia_ana0c_code);
	}

	return 0;
err:
	return ret;
}

static inline int mtk_isp_module_seninf_init(const struct isp_module *module)
{
	if (!module)
		return -EFAULT;

	return 0;
}

static inline int mtk_isp_module_seninf_stop(struct isp_module *module)
{
	if (!module)
		return -EFAULT;

	return 0;
}

static int mtk_isp_module_seninf_ioctl(const struct isp_module *module,
					unsigned int cmd, unsigned long arg)
{
	if (!module)
		return -EINVAL;

	pr_debug("custom_ioctl, module_id:%d\n", module->module_id);

	switch (cmd) {
	case ISP_CONFIG_CSI_BY_EFUSE:
		mtk_csi_config_by_efuse(module);
		break;

	default:
		break;
	}

	return 0;
}

static irqreturn_t mtk_isp_module_seninf_isr(int irq, void *dev_id)
{
	/* struct isp_module *module = (struct isp_module *)dev_id; */

	/* pr_info("%s seninf-isp:0x%lx\n", module->long_name, isp_status); */

	return IRQ_HANDLED;
}

static struct isp_module_ops seninf_ops = {
	.init = mtk_isp_module_seninf_init,
	.stop = mtk_isp_module_seninf_stop,
	.custom_ioctl = mtk_isp_module_seninf_ioctl,
};

static const struct of_device_id isp_seninf_of_ids[] = {
	{.compatible = "mediatek,isp-seninf", .data = "SENINF"},
	{}
};

static int mtk_isp_module_seninf_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct isp_module *module_seninf;
	int ret = 0;
	int num_items = 0;
	int i;

	if (!np) {
		pr_err("No device node for mtk isp module driver\n");
		return -ENODEV;
	}

	of_id = of_match_node(isp_seninf_of_ids, np);
	if (!of_id)
		return -ENODEV;

	if (of_property_read_u32(np, "num-items", &num_items))
		return -EINVAL;

	for (i = 0; i < num_items; i++) {
		module_seninf = _isp_module_probe_index(pdev, i, of_id->data);
		if (IS_ERR(module_seninf)) {
			ret = PTR_ERR(module_seninf);
			goto err;
		}
		module_seninf->module_dev = dev;

		module_seninf->ops = &seninf_ops;
		init_waitqueue_head(&module_seninf->wait_queue);

		if (module_seninf->hw_info.irq) {
			ret = request_irq(module_seninf->hw_info.irq,
					  mtk_isp_module_seninf_isr,
					  IRQF_TRIGGER_LOW | IRQF_SHARED,
					  module_seninf->long_name,
					  module_seninf);
			if (ret) {
				pr_err
				("failed to request_irq w/ irq %d for %s\n",
				module_seninf->hw_info.irq,
				module_seninf->name);
				goto err;
			}
		}

		mtk_isp_register_isp_module(module_seninf->parent_id,
						module_seninf);
		platform_set_drvdata(pdev, module_seninf);
		module_seninf->private_data = &pdev->dev;

		pr_debug("isp module %s is probe successfully. (map_addr=0x%lx irq=%d 0x%llx->0x%llx)\n",
			module_seninf->name,
			(unsigned long)module_seninf->hw_info.base_addr,
			module_seninf->hw_info.irq,
			module_seninf->hw_info.start,
			module_seninf->hw_info.end);
	}

	return 0;

err:
	return ret;
}

static int mtk_isp_module_seninf_remove(struct platform_device *pdev)
{
	struct isp_module *module_seninf = platform_get_drvdata(pdev);

	mtk_isp_unregister_isp_module(module_seninf);
	kfree(module_seninf);

	return 0;
}

static struct platform_driver isp_module_seninf_drvier = {
	.probe = mtk_isp_module_seninf_probe,
	.remove = mtk_isp_module_seninf_remove,
	.driver = {
		   .name = "isp-seninf",
		   .owner = THIS_MODULE,
		   .of_match_table = isp_seninf_of_ids,
		   }
};

module_platform_driver(isp_module_seninf_drvier);
MODULE_DESCRIPTION("MTK Camera Seninf Driver");
MODULE_LICENSE("GPL");
