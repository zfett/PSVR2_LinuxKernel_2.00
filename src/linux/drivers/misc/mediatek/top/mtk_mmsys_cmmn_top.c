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
#include <soc/mediatek/mtk_mmsys_cmmn_top.h>
#include "mtk_mmsys_cmmn_top_reg.h"

/**
 * @file mtk_mmsys_cmmn_top.c
 * This driver is used to config MTK mmsys common top hardware module.\n
 * It includes controlling mux, mout and provides reset function for sub\n
 * modules which are belong to mmsys common partition.
 */

/** @ingroup IP_group_mmsys_cmmn_top_internal_def
 * @brief mmsys cmmn top driver use definition
 * @{
 */
#define MTK_MMSYS_CMMN_TOP_CLK_SET_NUM				8
/**
 * @}
 */
struct reg_trg {
	void __iomem *regs;
};

/**
 * @defgroup IP_group_mmsys_cmmn_top MMSYS_CMMN_TOP engine
 *     There are several mux and mout modules in mmsys top in MT3611.\n
 *     By setting the mux and mout modules, we can control the data flow\n
 *     in mmsys common partition.\n
 *     @{
 *         @defgroup IP_group_mmsys_cmmn_top_external EXTERNAL
 *             The external API document for mmsys_cmmn_top.\n
 *             The mmsys_cmmn_top driver follow native Linux framework
 *             @{
 *               @defgroup IP_group_mmsys_cmmn_top_external_function 1.function
 *                   External function for mmsys_cmmn_top.
 *               @defgroup IP_group_mmsys_cmmn_top_external_struct 2.structure
 *                   none. Native Linux Driver.
 *               @defgroup IP_group_mmsys_cmmn_top_external_typedef 3.typedef
 *                   none. Native Linux Driver.
 *               @defgroup IP_group_mmsys_cmmn_top_external_enum 4.enumeration
 *                   External enumeration for mmsys_cmmn_top.
 *               @defgroup IP_group_mmsys_cmmn_top_external_def 5.define
 *                   none. Native Linux Driver.
 *             @}
 *
 *         @defgroup IP_group_mmsys_cmmn_top_internal INTERNAL
 *             The internal API document for mmsys_cmmn_top.
 *             @{
 *               @defgroup IP_group_mmsys_cmmn_top_internal_function 1.function
 *                   Internal function for mmsys_cmmn_top and module init.
 *               @defgroup IP_group_mmsys_cmmn_top_internal_struct 2.structure
 *                   Internal structure used for mmsys_cmmn_top.
 *               @defgroup IP_group_mmsys_cmmn_top_internal_typedef 3.typedef
 *                   none. Follow Linux coding style, no typedef used.
 *               @defgroup IP_group_mmsys_cmmn_top_internal_enum 4.enumeration
 *                   none. Follow Linux coding style, no typedef used.
 *               @defgroup IP_group_gce_internal_def 5.define
 *                   none. No extra define in mmsys_cmmn_top driver.
 *             @}
 *     @}
 */


/** @ingroup IP_group_mmsys_cmmn_top_internal_struct
 * @brief MMSYS_CMMN_TOP driver data structure.
 */
struct mtk_mmsys_cmmn_top {
	/** mmsys_cmmn_top device node */
	struct device *dev;
	/** mmsys_cmmn_top clock node */
	struct clk *clk_sync[MTK_MMSYS_CMMN_TOP_CLK_SET_NUM];
	/** mmsys_cmmn_top clock node */
	struct clk *clk_dl[MTK_MMSYS_CMMN_TOP_CLK_SET_NUM];
	/** mmsys_cmmn_top register base */
	void __iomem *regs;
};

/** @ingroup IP_group_mmsys_cmmn_top_internal_struct
 * @brief MMSYS_CMMN_TOP connection configuration structure.
 */
struct mtk_mmsys_cmmn_top_conn_cfg {
	/** source module */
	u32 from_mod;
	/** target module */
	u32 to_mod;
	/** register offset */
	u32 reg;
	/** register field mask */
	u32 mask;
	/** register field value */
	u32 val;
};

/** @ingroup IP_group_mmsys_cmmn_top_internal_struct
 * @brief MMSYS_CMMN_TOP routing path structure.
 */
static struct mtk_mmsys_cmmn_top_conn_cfg mmsys_cmmn_conn_cfg[] = {
	/*3612*/
	/** WPEA to FE */
	{
		.from_mod = MTK_MMSYS_CMMN_MOD_WPEA,
		.to_mod = MTK_MMSYS_CMMN_MOD_FE,
		.reg = WPEA_1_MOUT_EN,
		.mask = WPEA_2_FE,
		.val = WPEA_2_FE,
	},
	/** WPEA to WDMA */
	{
		.from_mod = MTK_MMSYS_CMMN_MOD_WPEA,
		.to_mod = MTK_MMSYS_CMMN_MOD_WDMA,
		.reg = WPEA_1_MOUT_EN,
		.mask = WPEA_2_WDMA,
		.val = WPEA_2_WDMA,
	},
	/** WPEA to PADDING */
	{
		.from_mod = MTK_MMSYS_CMMN_MOD_WPEA,
		.to_mod = MTK_MMSYS_CMMN_MOD_PADDING_0,
		.reg = WPEA_1_MOUT_EN,
		.mask = WPEA_2_PADDING,
		.val = WPEA_2_PADDING,
	},
	/** RSZ to 2ND PADDING */
	{
		.from_mod = MTK_MMSYS_CMMN_MOD_RSZ_0,
		.to_mod = MTK_MMSYS_CMMN_MOD_PADDING_1,
		.reg = MDP_RSZ_0_MOUT_EN,
		.mask = RSZ_0_2_2ND_PADDING,
		.val = RSZ_0_2_2ND_PADDING,
	},
	/** RSZ to 3RD PADDING */
	{
		.from_mod = MTK_MMSYS_CMMN_MOD_RSZ_0,
		.to_mod = MTK_MMSYS_CMMN_MOD_PADDING_2,
		.reg = MDP_RSZ_0_MOUT_EN,
		.mask = RSZ_0_2_3RD_PADDING,
		.val = RSZ_0_2_3RD_PADDING,
	},
};

static int reg_write_mask(struct reg_trg *trg, u32 offset, u32 value,
		u32 mask)
{
		u32 reg;

		reg = readl(trg->regs + offset);
		reg &= ~mask;
		reg |= value;
		writel(reg, trg->regs + offset);

	return 0;
}
static int mtk_mmsys_cmmn_top_mem_sel(const struct device *dev,
	struct reg_trg *trg, enum mtk_mmsys_cmmn_token token,
	enum mtk_mmsys_cmmn_mem_type type)
{
	if (!dev)
	return -EINVAL;

	if ((type != 0) && (type != 1)) {
		dev_err(dev, "Invalid mem type: %d\n", type);
		return -EINVAL;
	}

	switch (token) {
	case MTK_MMSYS_CMMN_FM_IMG:
		reg_write_mask(trg, MMSYS_COMMON_FM_SPLITTER,
			type ? FM_IMG : 0, FM_IMG);
		break;
	case MTK_MMSYS_CMMN_WDMA_0:
		reg_write_mask(trg, MMSYS_COMMON_SPLITTER,
			type ? WDMA_0 : 0, WDMA_0);
		break;
	case MTK_MMSYS_CMMN_FM_FD:
		reg_write_mask(trg, MMSYS_COMMON_FM_SPLITTER,
			type ? FM_FD : 0, FM_FD);
		break;
	case MTK_MMSYS_CMMN_FM_FP:
		reg_write_mask(trg, MMSYS_COMMON_FM_SPLITTER,
			type ? FM_FP : 0, FM_FP);
		break;
	case MTK_MMSYS_CMMN_FE:
		reg_write_mask(trg, MMSYS_COMMON_SPLITTER,
			type ? FE : 0, FE);
		break;
	case MTK_MMSYS_CMMN_FM_FMO:
		reg_write_mask(trg, MMSYS_COMMON_FM_SPLITTER,
			type ? FM_FMO : 0, FM_FMO);
		break;
	case MTK_MMSYS_CMMN_FM_ZNCC:
		reg_write_mask(trg, MMSYS_COMMON_FM_SPLITTER,
			type ? FM_ZNCC : 0, FM_ZNCC);
		break;
	case MTK_MMSYS_CMMN_WDMA_1:
		reg_write_mask(trg, MMSYS_COMMON_SPLITTER,
			type ? WDMA_1 : 0, WDMA_1);
		break;
	case MTK_MMSYS_CMMN_WDMA_2:
		reg_write_mask(trg, MMSYS_COMMON_SPLITTER,
			type ? WDMA_2 : 0, WDMA_2);
		break;
	case MTK_MMSYS_CMMN_WPE_1_RDMA:
		reg_write_mask(trg, MMSYS_COMMON_FM_SPLITTER,
			type ? WPE_1_RDMA : 0, WPE_1_RDMA);
		break;
	case MTK_MMSYS_CMMN_WPE_1_WDMA:
		reg_write_mask(trg, MMSYS_COMMON_FM_SPLITTER,
			type ? WPE_1_WDMA : 0, WPE_1_WDMA);
		break;
	default:
		dev_err(dev, "Splitter select failed!\n");
		return -EINVAL;
	}
	return 0;
}


/** @ingroup IP_group_mmsys_cmmn_top_external_function
 * @par Description
 *     Turn On MMSYS_CMMN Power and Clock.
 * @param[in]
 *     dev: mmsys_cmmn_top device node.
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
int mtk_mmsys_cmmn_top_power_on(struct device *dev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct mtk_mmsys_cmmn_top *cmmn_top;
	int ret = 0;

	if (!dev)
		return -ENODEV;

	cmmn_top = dev_get_drvdata(dev);

	if (!cmmn_top)
		return -EINVAL;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(cmmn_top->dev, "Failed to enable power domain: %d\n",
			ret);
		return ret;
	}
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cmmn_top_power_on);

/** @ingroup IP_group_mmsys_cmmn_top_external_function
 * @par Description
 *     Turn Off MMSYS_CMMN Power and Clock.
 * @param[in]
 *     dev: mmsys_cmmn_top device node.
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
int mtk_mmsys_cmmn_top_power_off(struct device *dev)
{
	struct mtk_mmsys_cmmn_top *cmmn_top;

	if (!dev)
		return -ENODEV;

	cmmn_top = dev_get_drvdata(dev);

	if (!cmmn_top)
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_put(dev);
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cmmn_top_power_off);

/** @ingroup IP_group_mmsys_cmmn_top_external_function
 * @par Description
 *     Connect two modules.\n
 *     Find the module pairs and enable the related mux and mout registers.
 * @param[in] dev: MMSYS_CMMN_TOP device node.
 * @param[in] from_mod: source module, #mtk_mmsys_cmmn_mod.
 * @param[in] to_mod: target module, #mtk_mmsys_cmmn_mod.
 * @return
 *     0, find matched connection pair successfully.
 *     -EINVAL, NULL dev or invalid pair that is not defined in\n
 *                   mmsys_cmmn_conn_cfg.
 * @par Boundary case and Limitation
 *     from_mod and to_mod should be in enum mtk_mmsys_cmmn_mod.
 * @par Error case and Error handling
 *     1. If mmsys_cmmn_top dev is NULL, return -EINVAL.\n.
 *     2. If the (from_mod, to_mod) pair is not defined in\n
 *         mmsys_cmmn_conn_cfg, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cmmn_top_conn(const struct device *dev,
		enum mtk_mmsys_cmmn_mod from_mod,
		enum mtk_mmsys_cmmn_mod to_mod)
{
	struct mtk_mmsys_cmmn_top *cmmn_top;
	struct mtk_mmsys_cmmn_top_conn_cfg *conn_cfg = mmsys_cmmn_conn_cfg;
	u32 reg;
	int i;
	bool match = false;

	if (!dev)
		return -ENODEV;

	cmmn_top = dev_get_drvdata(dev);

	if (!cmmn_top)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(mmsys_cmmn_conn_cfg); i++) {
		if (from_mod == conn_cfg[i].from_mod &&
				to_mod == conn_cfg[i].to_mod) {
			reg = readl(cmmn_top->regs + conn_cfg[i].reg);
			reg &= ~conn_cfg[i].mask;
			reg |= conn_cfg[i].val;
			writel(reg, cmmn_top->regs + conn_cfg[i].reg);
			match = true;
		}
	}

	if (!match) {
		dev_err(dev, "mtk_mmsys_cmmn_top_conn: invalid pair from %d to %d\n",
				from_mod, to_mod);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cmmn_top_conn);

/** @ingroup IP_group_mmsys_cmmn_top_external_function
 * @par Description
 *     Disconnect two modules.\n
 *     Find the connection and disable the related mux and mout registers.
 * @param[in] dev: MMSYS_CMMN_TOP device node.
 * @param[in] from_mod: source module, #mtk_mmsys_cmmn_mod.
 * @param[in] to_mod: target module, #mtk_mmsys_cmmn_mod.
 * @return
 *     0, find matched connection pair successfully.
 *     -EINVAL, NULL dev or invalid pair that is not defined in\n
 *                   mmsys_cmmn_conn_cfg.
 * @par Boundary case and Limitation
 *     from_mod and to_mod should be in enum mtk_mmsys_cmmn_mod.
 * @par Error case and Error handling
 *     1. If mmsys_cmmn_top dev is NULL, return -EINVAL.\n.
 *     2. If the (from_mod, to_mod) pair is not defined in\n
 *         mmsys_cmmn_conn_cfg, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cmmn_top_disconn(const struct device *dev,
		enum mtk_mmsys_cmmn_mod from_mod,
		enum mtk_mmsys_cmmn_mod to_mod)
{
	struct mtk_mmsys_cmmn_top *cmmn_top;
	struct mtk_mmsys_cmmn_top_conn_cfg *conn_cfg = mmsys_cmmn_conn_cfg;
	u32 reg;
	int i;
	bool match = false;

	if (!dev)
		return -ENODEV;

	cmmn_top = dev_get_drvdata(dev);

	if (!cmmn_top)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(mmsys_cmmn_conn_cfg); i++) {
		if (from_mod == conn_cfg[i].from_mod &&
				to_mod == conn_cfg[i].to_mod) {
			reg = readl(cmmn_top->regs + conn_cfg[i].reg);
			reg &= ~conn_cfg[i].mask;
			writel(reg, cmmn_top->regs + conn_cfg[i].reg);
			match = true;
		}
	}

	if (!match) {
		dev_err(dev, "mtk_mmsys_cmmn_top_disconn: invalid pair from %d to %d\n",
				from_mod, to_mod);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cmmn_top_disconn);

/** @ingroup IP_group_mmsys_cmmn_top_external_function
 * @par Description
 *     set mem_type.
 * @param[in]
 *     dev: MMSYS_CMMN_TOP device node.
 * @param[in]
 *     token: token selection, #mtk_mmsys_cmmn_token.
 * @param[in]
 *     type: memoery type selection, #mtk_mmsys_cmmn_mem_type.
 * @return
 *     0, sel memory successfully.
 *     -EINVAL, NULL dev.
 * @par Boundary case and Limitation
 *	token should be in enum mtk_mmsys_cmmn_token.\n
 *	type should be in enum mtk_mmsys_cmmn_mem_type.\n
 * @par Error case and Error handling
 *	If mmsys_cmmn_top dev is NULL, return -EINVAL.\n.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cmmn_top_sel_mem(const struct device *dev,
	enum mtk_mmsys_cmmn_token token,
	enum mtk_mmsys_cmmn_mem_type type)
{
	struct mtk_mmsys_cmmn_top *cmmn_top;
	struct reg_trg trg;

	if (!dev)
		return -ENODEV;

	cmmn_top = dev_get_drvdata(dev);

	if (!cmmn_top)
		return -EINVAL;

	trg.regs = cmmn_top->regs;

	return mtk_mmsys_cmmn_top_mem_sel(dev, &trg, token, type);
}
EXPORT_SYMBOL(mtk_mmsys_cmmn_top_sel_mem);

/** @ingroup IP_group_mmsys_cmmn_top_external_function
 * @par Description
 *     Reset modules.\n
 *     Find the target module and reset it.
 * @param[in] dev: MMSYS_CMMN_TOP device node.
 * @param[in] mod: target module, #mtk_mmsys_cmmn_reset_mod.
 * @return
 *     0, reset module successfully.
 *     -EINVAL, NULL dev or invalid module id that is not defined in\n
 *                   mtk_mmsys_cmmn_reset_mod.
 * @par Boundary case and Limitation
 *     the target module should be in enum mtk_mmsys_cmmn_reset_mod.
 * @par Error case and Error handling
 *     1. If mmsys_cmmn_top dev is NULL, return -EINVAL.\n.
 *     2. If the target module is not defined in\n
 *         mtk_mmsys_cmmn_reset_mod, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cmmn_top_mod_reset(const struct device *dev,
		enum mtk_mmsys_cmmn_reset_mod mod)
{
	struct mtk_mmsys_cmmn_top *cmmn_top;
	u32 reg, offset;
	u32 mask = 0;

	if (!dev)
		return -ENODEV;

	cmmn_top = dev_get_drvdata(dev);

	if (!cmmn_top)
		return -EINVAL;

	if ((mod >= padding_0 && mod <= MDP_FM) ||
		(mod >= RESIZER_0 && mod <= DISP_MUTEX)) {
		offset = MMSYS_SW0_RST_B;
	} else {
		dev_err(dev, "mtk_mmsys_cmmn_top_mod_reset: invalid mod %d\n",
		mod);
		return -EINVAL;
	}

	mask = 1UL << mod;

	reg = readl(cmmn_top->regs + offset);
	reg &= ~mask;
	writel(reg, cmmn_top->regs + offset);

	reg |= mask;
	writel(reg, cmmn_top->regs + offset);

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cmmn_top_mod_reset);

/** @ingroup IP_group_mmsys_cmmn_top_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, MMSYS_CMMN_TOP probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_cmmn_top_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mmsys_cmmn_top *cmmn_top;
	struct resource *regs;

	cmmn_top = devm_kzalloc(dev, sizeof(*cmmn_top), GFP_KERNEL);
	if (!cmmn_top)
		return -ENOMEM;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cmmn_top->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(cmmn_top->regs)) {
		dev_err(dev, "Failed to map mtk_mmsys_cmmn_top registers\n");
		return PTR_ERR(cmmn_top->regs);
	}

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_enable(dev);
#endif

	platform_set_drvdata(pdev, cmmn_top);
	dev_info(dev, "mmsys_cmmn_top probe done\n");

	return 0;
}


/** @ingroup IP_group_mmsys_cmmn_top_internal_function
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
static int mtk_mmsys_cmmn_top_remove(struct platform_device *pdev)
{
#ifdef CONFIG_COMMON_CLK
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

/** @ingroup IP_group_mmsys_cmmn_top_internal_struct
 * @brief MMSYS_CMMN_TOP Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id mmsys_cmmn_top_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-mmsys-common-top" },
	{},
};
MODULE_DEVICE_TABLE(of, mmsys_cmmn_top_driver_dt_match);

/** @ingroup IP_group_mmsys_cmmn_top_internal_struct
 * @brief MMSYS_CMMN_TOP platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_mmsys_cmmn_top_driver = {
	.probe		= mtk_mmsys_cmmn_top_probe,
	.remove		= mtk_mmsys_cmmn_top_remove,
	.driver		= {
		.name	= "mediatek-mmsys-common-top",
		.owner	= THIS_MODULE,
		.of_match_table = mmsys_cmmn_top_driver_dt_match,
	},
};

module_platform_driver(mtk_mmsys_cmmn_top_driver);
MODULE_LICENSE("GPL");
