/*
 * Copyright (c) 2018 MediaTek Inc.
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
 * @file mtk_frmtrk.c
 * MTK FRAME TRACK Linux Driver.\n
 * This driver is used to configure MTK frame track hardware module.\n
 * It includes frame track target line, locking window size,\n
 * turbo window size and enable/disable cross vsync function.
 */

/**
 * @defgroup IP_group_frmtrk FRMTRK
 *   Frame track is a hardware engine that compare HDMI/DP input vsync\n
 *   and dsi output vsync signal. If two vsync signal are not synchronized,\n
 *   it will inform hardware to accelerate or decelerate dsi clock.
 *
 *   @{
 *	  @defgroup IP_group_frmtrk_external EXTERNAL
 *	    The external API document for frmtrk.\n
 *	    The frmtrk driver follows native Linux framework.
 *
 *	    @{
 *		@defgroup IP_group_frmtrk_external_function 1.function
 *		  External function for frmtrk.
 *		@defgroup IP_group_frmtrk_external_struct 2.structure
 *		  None. No external structure used in frmtrk driver.
 *		@defgroup IP_group_frmtrk_external_typedef 3.typedef
 *		  None. No external typedef used in frmtrk driver.
 *		@defgroup IP_group_frmtrk_external_enum 4.enumeration
 *		  None. No external enumeration used in frmtrk driver.
 *		@defgroup IP_group_frmtrk_external_def 5.define
 *		  None. No external define used in frmtrk driver.
 *	    @}
 *
 *	  @defgroup IP_group_frmtrk_internal INTERNAL
 *	    The internal API document for frmtrk.
 *
 *	    @{
 *		@defgroup IP_group_frmtrk_internal_function 1.function
 *		  Internal function for frmtrk.
 *		@defgroup IP_group_frmtrk_internal_struct 2.structure
 *		  Internal structure used for frmtrk.
 *		@defgroup IP_group_frmtrk_internal_typedef 3.typedef
 *		  None. No internal typedef used in frmtrk driver.
 *		@defgroup IP_group_frmtrk_internal_enum 4.enumeration
 *		  None. No internal enumeration used in frmtrk driver.
 *		@defgroup IP_group_frmtrk_internal_def 5.define
 *		  Internal definition used for frmtrk.
 *	    @}
 *   @}
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include "mtk_frmtrk_reg.h"

/** @ingroup IP_group_frmtrk_internal_def
 * @brief FRMTRK driver use definition
 * @{
 */
/* FRMTRACK_01 */
#define SC_FRM_MASK_I_VAL(x)			((x) << 20)
/* FRMTRACK_02 */
#define SC_DDDS_TURBO_RGN_VAL(x)		((x) << 0)
#define SC_FRM_LOCK_WIN_VAL(x)			((x) << 16)
#define SC_FRM_LOCK_TOR_VAL(x)			((x) << 24)
/* FRMTRACK_03 */
#define SC_FRM_MASK_I_SEL_VAL(x)		((x) << 16)
/**
 * @}
 */

/** @ingroup IP_group_frmtrk_internal_struct
 * @brief FRMTRK Register Target Data Structure.
 */
struct reg_trg {
	/** target register base */
	void __iomem *regs;
	/** target register start address */
	u32 reg_base;
	/** cmdq packet */
	struct cmdq_pkt *pkt;
	/** gce subsys */
	int subsys;
};

/** @ingroup IP_group_frmtrk_internal_function
 * @par Description
 *     FRMTRK reigster mask write common function.
 * @param[in] trg: register target.
 * @param[in] offset: register offset.
 * @param[in] value: write data value.
 * @param[in] mask: write mask value.
 * @return
 *     0, write to register success.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int reg_write_mask(struct reg_trg *trg, u32 offset, u32 value,
		u32 mask)
{
	int ret = 0;

	if (trg->pkt) {
		ret = cmdq_pkt_write_value(trg->pkt, trg->subsys,
				(trg->reg_base & 0xffff) | offset,
				value, mask);
	} else {
		u32 reg;

		reg = readl(trg->regs + offset);
		reg &= ~mask;
		reg |= value;
		writel(reg, trg->regs + offset);
	}

	return ret;
}

/** @ingroup IP_group_frmtrk_internal_struct
 * @brief FRMTRK Driver Data Structure.
 */
struct mtk_frmtrk {
	/** frmtrk device node */
	struct device *dev;
	/** frmtrk clock node */
	struct clk *clk;
	/** frmtrk mutex lock */
	struct mutex lock;
	/** frmtrk register base */
	void __iomem *regs;
	/** frmtrk register start address */
	u32 reg_base;
	/** gce subsys */
	u32 subsys;
};

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Start frame tracking.
 * @param[in] dev: FRMTRK device node
 * @param[in] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_start(const struct device *dev, struct cmdq_pkt *handle)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = handle;
	trg.subsys = frmtrk->subsys;

	reg_write_mask(&trg, FRMTRACK_01, FRMTRACK_01_SC_DDDS_TRK_INV,
		       FRMTRACK_01_SC_DDDS_TRK_INV);
	reg_write_mask(&trg, FRMTRACK_04, FRMTRACK_04_SC_FRM_TRK_DDDS_EN,
		       FRMTRACK_04_SC_FRM_TRK_DDDS_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_start);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Stop frame tracking.
 * @param[in] dev: FRMTRK device node
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_stop(const struct device *dev)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = NULL;
	trg.subsys = frmtrk->subsys;

	reg_write_mask(&trg, FRMTRACK_04, 0, FRMTRACK_04_SC_FRM_TRK_DDDS_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_stop);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Set frame tracking target line.
 * @param[in] dev: FRMTRK device node
 * @param[in] target_line: target line number
 * @param[in] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_target_line(const struct device *dev, u16 target_line,
			   struct cmdq_pkt *handle)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	if (target_line > 3000) {
		dev_err(dev, "invalid target_line = %d!\n", target_line);
		return -EINVAL;
	}

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = handle;
	trg.subsys = frmtrk->subsys;

	reg_write_mask(&trg, FRMTRACK_01, target_line,
		       FRMTRACK_01_SC_FRM_TRK_LINE);

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_target_line);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Set frame tracking locking window size.
 * @param[in] dev: FRMTRK device node
 * @param[in] lock_win: lock window size
 * @param[in] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_lock_window(const struct device *dev, u8 lock_win,
			   struct cmdq_pkt *handle)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	if (lock_win > 100) {
		dev_err(dev, "invalid lock_win = %d!\n", lock_win);
		return -EINVAL;
	}

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = handle;
	trg.subsys = frmtrk->subsys;

	reg_write_mask(&trg, FRMTRACK_02, SC_FRM_LOCK_WIN_VAL(lock_win),
		       FRMTRACK_02_SC_FRM_LOCK_WIN);
	reg_write_mask(&trg, FRMTRACK_02, SC_FRM_LOCK_TOR_VAL(lock_win),
		       FRMTRACK_02_SC_FRM_LOCK_TOR);

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_lock_window);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Set frame tracking turbo window size.
 * @param[in] dev: FRMTRK device node
 * @param[in] turbo_win: ddds fastest tracking window size
 * @param[in] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_turbo_window(const struct device *dev, u16 turbo_win,
			    struct cmdq_pkt *handle)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	if (turbo_win > 200) {
		dev_err(dev, "invalid turbo_win = %d!\n", turbo_win);
		return -EINVAL;
	}

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = handle;
	trg.subsys = frmtrk->subsys;

	reg_write_mask(&trg, FRMTRACK_02, SC_DDDS_TURBO_RGN_VAL(turbo_win),
		       FRMTRACK_02_SC_DDDS_TURBO_RGN);

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_turbo_window);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Check frame tracking is locked or not.
 * @param[in] dev: FRMTRK device node
 * @return
 *     true for lock, else for unlock
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
bool mtk_frmtrk_locked(const struct device *dev)
{
	struct mtk_frmtrk *frmtrk;
	u32 frm_trk_dis;
	u32 lock_win;

	if (!dev)
		return false;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return false;

	frm_trk_dis = (readl(frmtrk->regs + FRMTRACK_06) &
		       FRMTRACK_06_SC_STA_FRM_TRK_ABS_DIS) >> 16;
	lock_win = (readl(frmtrk->regs + FRMTRACK_02) &
		    FRMTRACK_02_SC_FRM_LOCK_WIN) >> 16;

	return (frm_trk_dis <= lock_win);
}
EXPORT_SYMBOL(mtk_frmtrk_locked);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Get current target distance.
 * @param[in] dev: FRMTRK device node
 * @param[out] frm_trk_dis: target distance
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_get_trk_dist(const struct device *dev, u32 *frm_trk_dis)
{
	struct mtk_frmtrk *frmtrk;

	if (!dev)
		return -EFAULT;

	if (!frm_trk_dis)
		return -EINVAL;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	*frm_trk_dis = (readl(frmtrk->regs + FRMTRACK_06) &
			FRMTRACK_06_SC_STA_FRM_TRK_ABS_DIS) >> 16;

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_get_trk_dist);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Enable or disable cross vsync function.
 * @param[in] dev: FRMTRK device node
 * @param[in] enable: enable this function or not
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_cross_vsync(const struct device *dev, bool enable)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = NULL;
	trg.subsys = frmtrk->subsys;

	if (enable)
		reg_write_mask(&trg, FRMTRACK_04,
			       FRMTRACK_04_SC_FRM_TRK_CROSS_VSYNC_EN,
			       FRMTRACK_04_SC_FRM_TRK_CROSS_VSYNC_EN);
	else
		reg_write_mask(&trg, FRMTRACK_04, 0,
			       FRMTRACK_04_SC_FRM_TRK_CROSS_VSYNC_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_cross_vsync);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Set input vsync mask number. \n
 *     This module sample output line count every mask number. \n
 *     Example, mask = 2, and sel = 1, means input vsync counter is 0,1,0,1...\n
 *     and latch output line count at input vsync counter is 1.\n
 *     The default value of mask and sel are both 0, it means the behavior of \n
 *     input vsync counter is 0,1,2,..,14,15,0,1,2... \n
 *     If user want to keep the input vsync counter = 0 in initial stage like \n
 *     cinematic use, please set mask = 1, and sel = 0.
 * @param[in] dev: FRMTRK device node
 * @param[in] mask: Input vsync mask number.
 * @param[in] sel: Input vsync mask select.
 * @param[in] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_set_mask(const struct device *dev, u32 mask, u32 sel,
			struct cmdq_pkt *handle)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = handle;
	trg.subsys = frmtrk->subsys;

	if (mask < 16) {
		reg_write_mask(&trg, FRMTRACK_01, SC_FRM_MASK_I_VAL(mask),
			       FRMTRACK_01_SC_FRM_MASK_I);
	} else {
		dev_err(dev, "invalid mask = %d!\n", mask);
		return -EINVAL;
	}

	if (sel < 16) {
		reg_write_mask(&trg, FRMTRACK_03, SC_FRM_MASK_I_SEL_VAL(sel),
			       FRMTRACK_03_SC_FRM_MASK_I_SEL);
	} else {
		dev_err(dev, "invalid sel = %d!\n", sel);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_set_mask);

/** @ingroup IP_group_frmtrk_external_function
 * @par Description
 *     Configure display output vtotal for frmtrk using.
 * @param[in] dev: FRMTRK device node
 * @param[in] vtotal: Output vtotal
 * @param[in] handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_frmtrk_config_vtotal(const struct device *dev, u16 vtotal,
			     struct cmdq_pkt *handle)
{
	struct mtk_frmtrk *frmtrk;
	struct reg_trg trg;

	if (!dev)
		return -EFAULT;

	frmtrk = dev_get_drvdata(dev);

	if (!frmtrk)
		return -EFAULT;

	if (vtotal > 3000) {
		dev_err(dev, "invalid vtotal = %d!\n", vtotal);
		return -EINVAL;
	}

	trg.regs = frmtrk->regs;
	trg.reg_base = frmtrk->reg_base;
	trg.pkt = handle;
	trg.subsys = frmtrk->subsys;

	reg_write_mask(&trg, FRMTRACK_03, vtotal, FRMTRACK_03_SC_PNL_VTOTAL_DB);

	return 0;
}
EXPORT_SYMBOL(mtk_frmtrk_config_vtotal);

/** @ingroup IP_group_frmtrk_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in] pdev: platform device node.
 * @return
 *     If return value is 0, function success.\n
 *     Otherwise, FRMTRK probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_frmtrk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_frmtrk *frmtrk;
	struct resource *regs;
	const void *ptr;

	frmtrk = devm_kzalloc(dev, sizeof(*frmtrk), GFP_KERNEL);
	if (!frmtrk)
		return -ENOMEM;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	frmtrk->regs = devm_ioremap(dev, regs->start, resource_size(regs));
	if (IS_ERR(frmtrk->regs)) {
		dev_err(dev, "Failed to map frmtrk registers\n");
		return PTR_ERR(frmtrk->regs);
	}
	frmtrk->reg_base = (u32)regs->start;

	ptr = of_get_property(dev->of_node, "gce-subsys", NULL);
	if (ptr)
		frmtrk->subsys = be32_to_cpup(ptr);
	else
		frmtrk->subsys = 0;

#ifdef CONFIG_COMMON_CLK
	pm_runtime_enable(dev);
#endif

	of_node_put(dev->of_node);

	mutex_init(&frmtrk->lock);

	platform_set_drvdata(pdev, frmtrk);

	return 0;
}

/** @ingroup IP_group_frmtrk_internal_function
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
static int mtk_frmtrk_remove(struct platform_device *pdev)
{
#ifdef CONFIG_COMMON_CLK
	pm_runtime_disable(&pdev->dev);
#endif

	return 0;
}

/** @ingroup IP_group_frmtrk_internal_struct
 * @brief FRMTRK Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id frmtrk_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-frmtrk" },
	{},
};
MODULE_DEVICE_TABLE(of, frmtrk_driver_dt_match);

/** @ingroup IP_group_frmtrk_internal_struct
 * @brief FRMTRK platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_frmtrk_driver = {
	.probe		= mtk_frmtrk_probe,
	.remove		= mtk_frmtrk_remove,
	.driver		= {
		.name	= "mediatek-frmtrk",
		.owner	= THIS_MODULE,
		.of_match_table = frmtrk_driver_dt_match,
	},
};

module_platform_driver(mtk_frmtrk_driver);
MODULE_LICENSE("GPL");
