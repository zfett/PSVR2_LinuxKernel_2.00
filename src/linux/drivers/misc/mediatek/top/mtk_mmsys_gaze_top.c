/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	CK Hu <ck.hu@mediatek.com>
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

#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/mtk_mmsys_gaze_top.h>
#include "mtk_mmsys_gaze_top_reg.h"

/**
 * @file mtk_mmsys_gaze_top.c
 * This driver is used to config MTK mmsys gaze top hardware module.\n
 * It includes controlling mux, mout and provides reset function for sub\n
 * modules which are belong to mmsys gaze partition.
 */

/** @ingroup IP_group_mmsys_gaze_top_internal_def
 * @brief mmsys gaze top driver use definition
 * @{
 */
#define MTK_MMSYS_GAZE_TOP_CLK_SET_NUM				8
/**
 * @}
 */
struct reg_trg {
	void __iomem *regs;
};

/** @ingroup IP_group_mmsys_gaze_top_internal_struct
 * @brief MMSYS_GAZE_TOP driver data structure.
 */
struct mtk_mmsys_gaze_top {
	/** mmsys_gaze_top device node */
	struct device *dev;
	/** mmsys_gaze_top clock node */
	struct clk *clk_sync[MTK_MMSYS_GAZE_TOP_CLK_SET_NUM];
	/** mmsys_gaze_top clock node */
	struct clk *clk_dl[MTK_MMSYS_GAZE_TOP_CLK_SET_NUM];
	/** mmsys_gaze_top register base */
	void __iomem *regs;
};

/** @ingroup IP_group_mmsys_gaze_top_external_function
 * @par Description
 *     Get gaze wdma checksum. \n
 * @param[in] dev: mmsys gaze top device data struct.
 * @param[out] checksum: checksum value.
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev address. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, index, is not in 0 ~ 3, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_gaze_top_get_cksm(const struct device *dev, u32 *checksum)
{
	struct mtk_mmsys_gaze_top *gaze_top = dev_get_drvdata(dev);

	if (!gaze_top)
		return -EINVAL;

	*checksum = readl(gaze_top->regs + MMSYS_GAZE_WDMA_CRC);

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_gaze_top_get_cksm);


/** @ingroup IP_group_mmsys_gaze_top_external_function
 * @par Description
 *     Turn On MMSYS_GAZE Power and Clock.
 * @param[in]
 *     dev: mmsys_gaze_top device node.
 * @return
 *     0, successful to turn on power and clock.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from pm_runtime_get_sync().\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.\n
 *     2. If pm_runtime_get_sync() fails, return its error code.\n
 *     3. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_gaze_top_power_on(struct device *dev)
{
	struct mtk_mmsys_gaze_top *gaze_top = dev_get_drvdata(dev);
#ifdef CONFIG_COMMON_CLK_MT3612
	int ret = 0;
#endif

	if (!gaze_top)
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(gaze_top->dev, "Failed to enable power domain: %d\n",
			ret);
		return ret;
	}
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_gaze_top_power_on);

/** @ingroup IP_group_mmsys_gaze_top_external_function
 * @par Description
 *     Turn Off MMSYS_GAZE Power and Clock.
 * @param[in]
 *     dev: mmsys_gaze_top device node.
 * @return
 *     0, successful to turn off power and clock.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_gaze_top_power_off(struct device *dev)
{
	struct mtk_mmsys_gaze_top *gaze_top = dev_get_drvdata(dev);

	if (!gaze_top)
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_put(dev);
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_gaze_top_power_off);

/** @ingroup IP_group_mmsys_gaze_top_external_function
 * @par Description
 *     Reset modules.\n
 *     Find the target module and reset it.
 * @param[in] dev: MMSYS_GAZE_TOP device node.
 * @param[in] mod: target module, #mtk_mmsys_gaze_reset_mod.
 * @return
 *     0, reset module successfully.
 *     -EINVAL, NULL dev or invalid module id that is not defined in\n
 *                   mtk_mmsys_gaze_reset_mod.
 * @par Boundary case and Limitation
 *     the target module should be in enum mtk_mmsys_gaze_reset_mod.
 * @par Error case and Error handling
 *     1. If mmsys_gaze_top dev is NULL, return -EINVAL.\n.
 *     2. If the target module is not defined in\n
 *         mtk_mmsys_gaze_reset_mod, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_gaze_top_mod_reset(const struct device *dev,
		enum mtk_mmsys_gaze_reset_mod mod)
{
	struct mtk_mmsys_gaze_top *gaze_top = dev_get_drvdata(dev);
	u32 reg, offset;
	u32 mask = 0;

	if (!gaze_top)
		return -EINVAL;

	if (mod >= MDP_WPEA_0 || mod <= GALS_COMMON1) {
		offset = MMSYS_SW0_RST_B;
	} else {
		dev_err(dev, "mtk_mmsys_gaze_top_mod_reset: invalid mod %d\n",
		mod);
		return -EINVAL;
	}

	mask = 1UL << mod;

	reg = readl(gaze_top->regs + offset);
	reg &= ~mask;
	writel(reg, gaze_top->regs + offset);

	reg |= mask;
	writel(reg, gaze_top->regs + offset);

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_gaze_top_mod_reset);

/** @ingroup IP_group_mmsys_gaze_top_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, MMSYS_GAZE_TOP probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_gaze_top_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mmsys_gaze_top *gaze_top;
	struct resource *regs;

	gaze_top = devm_kzalloc(dev, sizeof(*gaze_top), GFP_KERNEL);
	if (!gaze_top)
		return -ENOMEM;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gaze_top->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(gaze_top->regs)) {
		dev_err(dev, "Failed to map mtk_mmsys_gaze_top registers\n");
		return PTR_ERR(gaze_top->regs);
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_enable(dev);
#endif

	platform_set_drvdata(pdev, gaze_top);
	dev_info(dev, "mmsys_gaze_top probe done\n");

	return 0;
}


/** @ingroup IP_group_mmsys_gaze_top_internal_function
 * @par Description
 *     none.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.\n.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_gaze_top_remove(struct platform_device *pdev)
{
#ifdef CONFIG_COMMON_CLK
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

/** @ingroup IP_group_mmsys_gaze_top_internal_struct
 * @brief MMSYS_GAZE_TOP Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id mmsys_gaze_top_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-mmsys-gaze-top" },
	{},
};
MODULE_DEVICE_TABLE(of, mmsys_gaze_top_driver_dt_match);

/** @ingroup IP_group_mmsys_gaze_top_internal_struct
 * @brief MMSYS_GAZE_TOP platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_mmsys_gaze_top_driver = {
	.probe		= mtk_mmsys_gaze_top_probe,
	.remove		= mtk_mmsys_gaze_top_remove,
	.driver		= {
		.name	= "mediatek-mmsys-gaze-top",
		.owner	= THIS_MODULE,
		.of_match_table = mmsys_gaze_top_driver_dt_match,
	},
};

module_platform_driver(mtk_mmsys_gaze_top_driver);
MODULE_LICENSE("GPL");
