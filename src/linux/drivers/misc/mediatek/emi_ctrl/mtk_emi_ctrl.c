/*
 * Copyright (C) 2019 MediaTek Inc.
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
 * @file mtk_emi_ctrl.c
 *  This EMI control driver is used to control MTK EMI hardware module.
 */

/**
 * @defgroup IP_group_EMI_CTRL EMI_CTRL
 *     EMI_CTRL module
 *     @{
 *         @defgroup IP_group_emi_ctrl_external EXTERNAL
 *             The external API document for emi_ctrl.\n
 *             The emi_ctrl driver follows native Linux framework
 *             @{
 *               @defgroup IP_group_emi_ctrl_external_function 1.function
 *                   External function for emi_ctrl.
 *               @defgroup IP_group_emi_ctrl_external_struct 2.structure
 *                   External structure used for emi_ctrl.
 *               @defgroup IP_group_emi_ctrl_external_typedef 3.typedef
 *                   none. Native Linux Driver.
 *               @defgroup IP_group_emi_ctrl_external_enum 4.enumeration
 *                   External enumeration used for emi_ctrl.
 *               @defgroup IP_group_emi_ctrl_external_def 5.define
 *                   External definition used for emi_ctrl.
 *             @}
 *
 *         @defgroup IP_group_emi_ctrl_internal INTERNAL
 *             The internal API document for emi_ctrl.
 *             @{
 *               @defgroup IP_group_emi_ctrl_internal_function 1.function
 *                   Internal function for emi_ctrl and module init.
 *               @defgroup IP_group_emi_ctrl_internal_struct 2.structure
 *                   Internal structure used for emi_ctrl.
 *               @defgroup IP_group_emi_ctrl_internal_typedef 3.typedef
 *                   none. Follow Linux coding style, no typedef used.
 *               @defgroup IP_group_emi_ctrl_internal_enum 4.enumeration
 *                   Internal enumeration used for emi_ctrl.
 *               @defgroup IP_group_emi_ctrl_internal_def 5.define
 *                   Internal definition used for emi_ctrl.
 *             @}
 *     @}
 */

#include <linux/types.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <soc/mediatek/mtk_emi_ctrl.h>

#include "emi_reg.h"

/**
 * @ingroup IP_group_emi_ctrl_internal_def
 * @{
 */
#define BW_VALUE_P3_M2 0x0b
#define BW_VALUE_P3_M5 0x08
#define BW_VALUE_PN_M2 0x05
#define BW_VALUE_PN_M5 0x03
/**
 * @}
 */

/**
 * @ingroup IP_group_emi_ctrl_internal_struct
 * @brief EMI Control Driver Data Structure.
 */
struct emi_ctrl {
	/** EMI register base */
	void __iomem *base;

	/** emi_ctrl mutex lock */
	struct mutex lock;
};

/**
 * @ingroup IP_group_emi_ctrl_internal_function
 * @par Description
 *      Base function for setting the bandwidth filter value.\n
 *	There is no sanity check and lock protection in this function.\n
 *	The input checking and protection must be done by caller.
 * @param[in] emi_ctrl: emi_ctrl structucture
 * @param[in] port: the master port to be set
 * @param[in] value: the value to be set
 * @return
 *      none
 */
static void set_bandwidth(struct emi_ctrl *emi_ctrl, const u8 port,
						const u8 value)
{
	void *reg;

	/** Get the bandwidth filter register address */
	reg = emi_ctrl->base;

	switch (port) {
	case EMI_PORT_M0:
	case EMI_PORT_M1:
		reg += EMI_ARBA;
		break;
	case EMI_PORT_M2:
		reg += EMI_ARBC;
		break;
	case EMI_PORT_M3:
		reg += EMI_ARBD;
		break;
	case EMI_PORT_M4:
		reg += EMI_ARBE;
		break;
	case EMI_PORT_M5:
		reg += EMI_ARBF;
		break;
	case EMI_PORT_M6:
		reg += EMI_ARBG;
		break;
	case EMI_PORT_M7:
		reg += EMI_ARBH;
		break;
	default:
		WARN_ON(1);
		return;
	}

	/** Update the bandwidth value */
	writel((readl(reg) & ~M0_BANDWIDTH) | (value & M0_BANDWIDTH), reg);
}

/**
 * @ingroup IP_group_emi_ctrl_external_function
 * @par Description
 *	Set the bandwidth filter value.
 * @param[in] dev: emi_ctrl device node.
 * @param[in] port: the master port to be set.
 * @param[in] value: the value to be set.
 * @return
 *	0, configuration successful.\n
 *	-EINVAL, invalid argument.\n
 *	-ENODEV, no such device.
 * @par Boundary case and Limitation
 *	port must between 0 ~ 7.
 *	value must between 0 ~ 63.
 * @par Error case and Error handling
 *	If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_emi_ctrl_set_bandwidth(struct device *dev, const u8 port,
						const u8 value)
{
	struct emi_ctrl *emi_ctrl;

	/** Sanity check */
	if (!dev)
		return -EINVAL;

	if (port >= _EMI_PORT_MAX) {
		dev_err(dev, "Invalid master port: %d\n", port);
		return -EINVAL;
	}

	if ((value & M0_BANDWIDTH) != value) {
		dev_err(dev, "Invalid filter value: %d\n", value);
		return -EINVAL;
	}

	/** Get driver data */
	emi_ctrl = dev_get_drvdata(dev);

	if (!emi_ctrl)
		return -ENODEV;

	/** lock */
	mutex_lock(&emi_ctrl->lock);

	/** Set the bandwidth filter value */
	set_bandwidth(emi_ctrl, port, value);

	/** Unlock */
	mutex_unlock(&emi_ctrl->lock);

	return 0;
}
EXPORT_SYMBOL(mtk_emi_ctrl_set_bandwidth);

/**
 * @ingroup IP_group_emi_ctrl_external_function
 * @par Description
 *	Change the configuration of the EMI.
 * @param[in] dev: emi_ctrl device node.
 * @param[in] p_num: P number, P1 ~ P8 is available.
 * @return
 *	0, configuration successful.\n
 *	-EINVAL, invalid argument.\n
 *	-ENODEV, no such device.
 * @par Boundary case and Limitation
 *	p_num must between 1 ~ 8.
 * @par Error case and Error handling
 *	If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_emi_ctrl_change_config(struct device *dev, const u8 p_num)
{
	struct emi_ctrl *emi_ctrl;

	/** Sanity check */
	if (!dev)
		return -EINVAL;

	if (p_num <= 0 || p_num >= _EMI_CONF_MAX) {
		dev_err(dev, "Invalid p_num: %d\n", p_num);
		return -EINVAL;
	}

	/** Get driver data and lock */
	emi_ctrl = dev_get_drvdata(dev);

	if (!emi_ctrl)
		return -ENODEV;

	mutex_lock(&emi_ctrl->lock);

	/** Set bandwidth filter value */
	if (p_num == EMI_CONF_P3) {
		set_bandwidth(emi_ctrl, EMI_PORT_M2, BW_VALUE_P3_M2);
		set_bandwidth(emi_ctrl, EMI_PORT_M5, BW_VALUE_P3_M5);
	} else {
		set_bandwidth(emi_ctrl, EMI_PORT_M2, BW_VALUE_PN_M2);
		set_bandwidth(emi_ctrl, EMI_PORT_M5, BW_VALUE_PN_M5);
	}

	/** Unlock */
	mutex_unlock(&emi_ctrl->lock);

	return 0;
}
EXPORT_SYMBOL(mtk_emi_ctrl_change_config);

/**
 * @ingroup IP_group_emi_ctrl_internal_function
 * @par Description
 *	Probe emi_ctrl according to device tree.
 * @param[in] pdev: platform device node.
 * @return
 *	0, if successful.\n
 *	Otherwise, emi_ctrl probe failed.
 * @par Boundary case and Limitation
 *	none.
 * @par Error case and Error handling
 *	If there is any error during probe flow,\n
 *	system error number will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_emi_ctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct emi_ctrl *emi_ctrl;
	struct resource *regs;
	int ret;

	emi_ctrl = devm_kzalloc(dev, sizeof(*emi_ctrl), GFP_KERNEL);
	if (!emi_ctrl)
		return -ENOMEM;

	platform_set_drvdata(pdev, emi_ctrl);

	/** Map register */
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(dev, "Failed to get EMI IORESOURCE_MEM\n");
		ret = -ENODEV;
		goto err_get_io_resource;
	}

	emi_ctrl->base = devm_ioremap_resource(dev, regs);
	if (!emi_ctrl->base) {
		dev_err(dev, "Failed to get EMI register mapping\n");
		ret = PTR_ERR(emi_ctrl->base);
		goto err_map_register;
	}

	/** Initialize mutex lock */
	mutex_init(&emi_ctrl->lock);

	/** Log */
	dev_info(dev, "EMI control driver initialized\n");

	return 0;

err_map_register:
err_get_io_resource:
	devm_kfree(dev, emi_ctrl);

	return ret;
}

/**
 * @ingroup IP_group_emi_ctrl_internal_function
 * @par Description
 *	Release memory.
 * @param[in] pdev: platform device node.
 * @return
 *	0, if successful.\n.
 *	-ENODEV, no such device.\n
 *	-EBUSY, if this emi_ctrl needed by another device.
 * @par Boundary case and Limitation
 *	none.
 * @par Error case and Error handling
 *	none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_emi_ctrl_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct emi_ctrl *emi_ctrl;

	/** Sanity check */
	if (!dev)
		return -ENODEV;

	/** Get driver data */
	emi_ctrl = dev_get_drvdata(dev);

	if (!emi_ctrl)
		return 0;

	/** Clean up */
	devm_kfree(dev, emi_ctrl);

	/** Log */
	dev_info(dev, "EMI control driver removed\n");

	return 0;
}

/**
 * @ingroup IP_group_emi_ctrl_internal_struct
 * @brief Open Framework Device ID for emi_ctrl.\n
 *	This structure is used to attach specific names to\n
 *	platform device for use with device tree.
 */
static const struct of_device_id of_match_table_emi_ctrl[] = {
	{ .compatible = "mediatek,mt3612-emi-ctrl" },
	{},
};
MODULE_DEVICE_TABLE(of, of_match_table_emi_ctrl);

/**
 * @ingroup type_group_emi_ctrl_struct
 * @brief emi_ctrl platform driver structure.\n
 *	This structure is used to register itself.
 */
struct platform_driver platform_driver_emi_ctrl = {
	.probe = mtk_emi_ctrl_probe,
	.remove = mtk_emi_ctrl_remove,
	.driver = {
		.name = "mediatek-emi-ctrl",
		.owner = THIS_MODULE,
		.of_match_table = of_match_table_emi_ctrl,
	},
};

module_platform_driver(platform_driver_emi_ctrl);

MODULE_AUTHOR("Chun-Chi Lan");
MODULE_DESCRIPTION("MediaTek EMI Control Driver");
MODULE_LICENSE("GPL");
