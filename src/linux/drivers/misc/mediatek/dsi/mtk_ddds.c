/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk_ddds.c
 * MTK Linux Driver for DDDS.\n
 * This driver is used to config MTK ddds hardware module. \n
 * DDDS can generate a reference clock to mipi_tx pll. \n
 * This reference clock will tracking HDMI/DP input hsync.
 */

/**
 * @defgroup IP_group_ddds DDDS
 *   This is ddds linux driver. \n
 *   It can config MTK ddds hardware module to output reference clock
 *   to mipi_tx pll.
 *
 *   @{
 *       @defgroup IP_group_ddds_external EXTERNAL
 *         The external API document for ddds.
 *
 *         @{
 *             @defgroup IP_group_ddds_external_function 1.function
 *               This is ddds external function.
 *             @defgroup IP_group_ddds_external_struct 2.structure
 *               None. No external structure used in ddds driver.
 *             @defgroup IP_group_ddds_external_typedef 3.typedef
 *               None. No external typedef used in ddds driver.
 *             @defgroup IP_group_ddds_external_enum 4.enumeration
 *               This is ddds external enumeration.
 *             @defgroup IP_group_ddds_external_def 5.define
 *               None. No external define used in ddds driver.
 *         @}
 *
 *       @defgroup IP_group_ddds_internal INTERNAL
 *         The internal API document for ddds.
 *
 *         @{
 *             @defgroup IP_group_ddds_internal_function 1.function
 *               This is ddds internal function and module init.
 *             @defgroup IP_group_ddds_internal_struct 2.structure
 *               Internal structure used for ddds.
 *             @defgroup IP_group_ddds_internal_typedef 3.typedef
 *               None. No internal typedef used in ddds driver.
 *             @defgroup IP_group_ddds_internal_enum 4.enumeration
 *               None. No internal enumeration used in ddds driver.
 *             @defgroup IP_group_ddds_internal_def 5.define
 *               This is ddds internal define.
 *         @}
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

#include <soc/mediatek/mtk_ddds.h>
#include "mtk_ddds_reg.h"

/** @ingroup IP_group_ddds_internal_def
 * @brief ddds configuration register
 * @{
 */
#define DDDS_FREQ_CW_MASK		DDDS_FREQ_CW
#define DDDS_FREQ_CW_SET(x)		((x) << 0)

#define INIT_SEL_MASK			INIT_SEL
#define INIT_SEL_SET(x)			((x) << 28)
#define HLEN_NUM_MASK			HLEN_NUM
#define HLEN_NUM_SET(x)			((x) << 16)
#define HLEN_INT_MASK			HLEN_INT
#define HLEN_INT_SET(x)			((x) << 0)

#define HLEN_DEN_MASK			HLEN_DEN
#define HLEN_DEN_SET(x)			((x) << 16)

#define DDDS_ERR_LIM_MASK		DDDS_ERR_LIM
#define DDDS_ERR_LIM_SET(x)		((x) << 0)

#define HLEN_DEN_E1_MASK		HLEN_DEN_E1
#define HLEN_DEN_E1_SET(x)		((x) << 16)
#define HLEN_INT_E1_MASK		HLEN_INT_E1
#define HLEN_INT_E1_SET(x)		((x) << 0)

#define HLEN_INT_E2_MASK		HLEN_INT_E2
#define HLEN_INT_E2_SET(x)		((x) << 16)
#define HLEN_NUM_E1_MASK		HLEN_NUM_E1
#define HLEN_NUM_E1_SET(x)		((x) << 0)

#define HLEN_NUM_E2_MASK		HLEN_NUM_E2
#define HLEN_NUM_E2_SET(x)		((x) << 16)
#define HLEN_DEN_E2_MASK		HLEN_DEN_E2
#define HLEN_DEN_E2_SET(x)		((x) << 0)

#define HLEN_DEN_L1_MASK		HLEN_DEN_L1
#define HLEN_DEN_L1_SET(x)		((x) << 16)
#define HLEN_INT_L1_MASK		HLEN_INT_L1
#define HLEN_INT_L1_SET(x)		((x) << 0)

#define HLEN_INT_L2_MASK		HLEN_INT_L2
#define HLEN_INT_L2_SET(x)		((x) << 16)
#define HLEN_NUM_L1_MASK		HLEN_NUM_L1
#define HLEN_NUM_L1_SET(x)		((x) << 0)

#define HLEN_NUM_L2_MASK		HLEN_NUM_L2
#define HLEN_NUM_L2_SET(x)		((x) << 16)
#define HLEN_DEN_L2_MASK		HLEN_DEN_L2
#define HLEN_DEN_L2_SET(x)		((x) << 0)

#define DDDS_CTRL_PARA			GENMASK(11, 0)

#define DDDS_ERR_LIM_LOW_MASK		DDDS_ERR_LIM_LOW
/**
 * @}
 */

/** @ingroup IP_group_ddds_internal_def
 * @brief DDDS system clock
 * @{
 */
#define SRC_CLK		624
/**
 * @}
 */

/** @ingroup IP_group_ddds_internal_struct
 * @brief Data Struct for DDDS Driver
 */
struct mtk_ddds {
	/** point to ddds device struct */
	struct device *dev;
	/** store ddds register base address */
	void __iomem *regs;
	/** target frequency for ddds output */
	u32 target_freq;
	/** ddds digital system clock */
	struct clk *sys_clk;
	/** ddds sys clock source select */
	struct clk *sys_sel;
	/** ddds1 clock */
	struct clk *ddds1_clk;
	/** ddds digital feedback clock */
	struct clk *fb_clk;
	/** ddds feedback clock source select */
	struct clk *fb_sel;
	/** ddds vsp clock */
	struct clk *ddds_vsp_clk;
	/** ddds reference output clock to dsi mipi tx pll */
	struct clk *ref_clk;
	/** ddds reference output clock select */
	struct clk *ref_sel;
	/** ddds reference output clock select x'tal 26MHz */
	struct clk *ref_clk26m;
	/** ddds reference output clock select ddds output clock */
	struct clk *ref_pll_ck;
	/** ddds ddds output clock select */
	struct clk *ref_pll_sel;
	/** ddds hw enable status */
	bool enabled;
};

/** @ingroup IP_group_ddds_internal_function
 * @par Description
 *     DDDS reigster mask write common function.
 * @param[out] reg: ddds register base address
 * @param[in] offset: ddds register offset
 * @param[in] mask: write data mask value
 * @param[in] data: write data value
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_ddds_mask(void __iomem *reg, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(reg + offset);

	writel((temp & ~mask) | (data & mask), reg + offset);
}

/** @ingroup IP_group_ddds_internal_function
 * @par Description
 *     Frequency to DDDS control word translation function. \n
 *     Use this function to translate frequency to control word. \n
 *     DDDS use it to setup target frequency and calculate error limit. \n
 *     FREQ_CW = SRC_CLK / (TARGET_FREQ * 2) * 2^24, where 2^24 = 0x1000000
 * @param[in] freq: translation frequency, unit is MHz
 * @return ddds control word
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u32 mtk_ddds_freq_cw(u32 freq)
{
	u32 freq_cw;

	/*(0x1000000 / (2 * freq) * SRC_CLK)*/
	freq_cw = ((0x400000 * (u32)SRC_CLK) / freq) * 2;

	return freq_cw;
}

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     Target frequency setting function.\n
 *     This function is used to set target output frequency. \n
 *     User set target frequence, it will be translated to cw and set into hw.
 * @param[in] dev: device data struct
 * @param[in] target_freq: target output frequency, unit is MHz
 * @return
 *     0, success. \n
 *     -EFAULT, invalid dev point. \n
 *     -EINVAL, invalid parameter. \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Parameter, target_freq, default is 26MHz for MIPI TX PLL. \n
 *     And its range should be in 20MHz ~ 30MHz.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, target_freq, over spec, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_set_target_freq(const struct device *dev, u32 target_freq)
{
	struct mtk_ddds *ddds;
	u32 freq_cw;

	if (!dev)
		return -EFAULT;

	if (WARN_ON(target_freq > 30) || WARN_ON(target_freq < 20))
		return -EINVAL;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (!ddds->enabled)
		return -EIO;

	ddds->target_freq = target_freq;

	freq_cw = mtk_ddds_freq_cw(target_freq);
	mtk_ddds_mask(ddds->regs, DDDS_00, DDDS_FREQ_CW_MASK, freq_cw);

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_set_target_freq);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     Target frequency adjust function.\n
 *     This function is used to fine tune target output frequency. \n
 *     If base target frequence is 26MHz, \n
 *     1 cw can adjust 0.13Hz at its target frequence.
 * @param[in] dev: device data struct
 * @param[in] adj_cw: adjust value, unit is control word
 * @return
 *     0, success
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Parameter, adj_cw, range is -10 ~ 10.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, adj_cw, over spec, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_target_freq_adjust(const struct device *dev, s8 adj_cw)
{
	struct mtk_ddds *ddds;
	u32 freq_cw;

	if (!dev)
		return -EFAULT;

	if (WARN_ON(adj_cw > 10) || WARN_ON(adj_cw < -10))
		return -EINVAL;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (!ddds->enabled)
		return -EIO;

	freq_cw = readl(ddds->regs + DDDS_00) & DDDS_FREQ_CW_MASK;
	freq_cw += adj_cw;
	mtk_ddds_mask(ddds->regs, DDDS_00, DDDS_FREQ_CW_MASK, freq_cw);

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_target_freq_adjust);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     Error limit setting function. \n
 *     Setup output frequency upper and lower boundary. \n
 *     If user set target freq to 26MHz and max_freq to 28MHz, \n
 *     it means the upper boundary is 28MHz and lower boundary is 24MHz.
 * @param[in] dev: device data struct
 * @param[in] max_freq: max tracking frequency
 * @return
 *     0, success
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Setup target freq fisrt (mtk_ddds_set_target_freq()), \n
 *     then set error limit. \n
 *     And parameter, max_freq, range should be in 20MHz ~ 30MHz.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, max_freq, over spec, return -EINVAL. \n
 *     3. The device not enable, return -EIO. \n
 *     4. The target freq dose not set, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_set_err_limit(const struct device *dev, u32 max_freq)
{
	struct mtk_ddds *ddds;
	u32 freq_cw;
	u32 target_freq_cw;
	u32 err_lim_cw;

	if (!dev)
		return -EFAULT;

	if (WARN_ON(max_freq > 30) || WARN_ON(max_freq < 20))
		return -EINVAL;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (!ddds->enabled)
		return -EIO;

	freq_cw = mtk_ddds_freq_cw(max_freq);
	target_freq_cw = readl(ddds->regs + DDDS_00) & DDDS_FREQ_CW_MASK;

	if (!target_freq_cw)
		return -EINVAL;

	if (target_freq_cw > freq_cw)
		err_lim_cw = target_freq_cw - freq_cw;
	else
		err_lim_cw = freq_cw - target_freq_cw;

	mtk_ddds_mask(ddds->regs, DDDS_03, DDDS_ERR_LIM_MASK,
		      (err_lim_cw >> 16));
	mtk_ddds_mask(ddds->regs, DDDS_1C, DDDS_ERR_LIM_LOW_MASK, err_lim_cw);

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_set_err_limit);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     hlen setting function. \n
 *     Set hlen if you want to enable close loop (tracking HDMI/DP hsync). \n
 *     hlen = target_frq / hsync
 * @param[in] dev: device data struct
 * @param[in] rgn: configure which tracking region parameter
 * @param[in] hsync_freq: source hsync frequency, unit is Hz
 * @return
 *     0, success
 *     -EFAULT, invalid address point \n
 *     -EINVAL, invalid parameter \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Setup target freq fisrt (mtk_ddds_set_target_freq()), then set hlen.\n
 *     And input/output vtotal should be the same. \n
 *     Parameter, rgn, is defined in #mtk_ddds_frmtck_region.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The parameter, hsync_freq, is zero, return -EINVAL. \n
 *     3. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_set_hlen(const struct device *dev,
		      enum mtk_ddds_frmtck_region rgn, u32 hsync_freq)
{
	struct mtk_ddds *ddds;
	u32 target_freq;
	u32 hlen_int;
	u32 hlen_num;
	const u32 hlen_den = 4095;

	if (!dev)
		return -EFAULT;

	if (!hsync_freq || (hsync_freq > 300000)) {
		dev_err(dev, "invalid hsync_freq = %d!\n", hsync_freq);
		return -EINVAL;
	}

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (!ddds->enabled)
		return -EIO;

	target_freq = ddds->target_freq * 1000000;
	hlen_int = target_freq / hsync_freq;
	hlen_num = (target_freq - (hlen_int * hsync_freq)) * hlen_den /
		   hsync_freq;

	switch (rgn) {
	case DDDS_FRAMETRACK_LOCK:
		mtk_ddds_mask(ddds->regs, DDDS_01, HLEN_INT_MASK,
			      HLEN_INT_SET(hlen_int));
		mtk_ddds_mask(ddds->regs, DDDS_01, HLEN_NUM_MASK,
			      HLEN_NUM_SET(hlen_num));
		mtk_ddds_mask(ddds->regs, DDDS_02, HLEN_DEN_MASK,
			      HLEN_DEN_SET(hlen_den));
		break;
	case DDDS_FRAMETRACK_FAST1:
		mtk_ddds_mask(ddds->regs, DDDS_0C, HLEN_INT_E1_MASK,
			      HLEN_INT_E1_SET(hlen_int));
		mtk_ddds_mask(ddds->regs, DDDS_0D, HLEN_NUM_E1_MASK,
			      HLEN_NUM_E1_SET(hlen_num));
		mtk_ddds_mask(ddds->regs, DDDS_0C, HLEN_DEN_E1_MASK,
			      HLEN_DEN_E1_SET(hlen_den));
		break;
	case DDDS_FRAMETRACK_FAST2:
		mtk_ddds_mask(ddds->regs, DDDS_0D, HLEN_INT_E2_MASK,
			      HLEN_INT_E2_SET(hlen_int));
		mtk_ddds_mask(ddds->regs, DDDS_0E, HLEN_NUM_E2_MASK,
			      HLEN_NUM_E2_SET(hlen_num));
		mtk_ddds_mask(ddds->regs, DDDS_0E, HLEN_DEN_E2_MASK,
			      HLEN_DEN_E2_SET(hlen_den));
		break;
	case DDDS_FRAMETRACK_SLOW1:
		mtk_ddds_mask(ddds->regs, DDDS_0F, HLEN_INT_L1_MASK,
			      HLEN_INT_L1_SET(hlen_int));
		mtk_ddds_mask(ddds->regs, DDDS_10, HLEN_NUM_L1_MASK,
			      HLEN_NUM_L1_SET(hlen_num));
		mtk_ddds_mask(ddds->regs, DDDS_0F, HLEN_DEN_L1_MASK,
			      HLEN_DEN_L1_SET(hlen_den));
		break;
	case DDDS_FRAMETRACK_SLOW2:
		mtk_ddds_mask(ddds->regs, DDDS_10, HLEN_INT_L2_MASK,
			      HLEN_INT_L2_SET(hlen_int));
		mtk_ddds_mask(ddds->regs, DDDS_11, HLEN_NUM_L2_MASK,
			      HLEN_NUM_L2_SET(hlen_num));
		mtk_ddds_mask(ddds->regs, DDDS_11, HLEN_DEN_L2_MASK,
			      HLEN_DEN_L2_SET(hlen_den));
		break;
	default:
		dev_err(dev, "invalid mtk_ddds_frmtck_region = %d!\n", rgn);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_set_hlen);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     Enable DDDS frame track mode. \n
 *     Enable this function when you want to track input (hdmi/dp) vsync.
 * @param[in] dev: device data struct
 * @param[in] enable: Enable cowork with frame track or not
 * @return
 *     0, success
 *     -EFAULT, invalid address point \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Eanble frame track module before enable this mode.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_frmtrk_mode(const struct device *dev, bool enable)
{
	struct mtk_ddds *ddds;

	if (!dev)
		return -EFAULT;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (!ddds->enabled)
		return -EIO;

	if (enable)
		mtk_ddds_mask(ddds->regs, DDDS_0C, VSYNC_TRACK_EN,
			      VSYNC_TRACK_EN);
	else
		mtk_ddds_mask(ddds->regs, DDDS_0C, VSYNC_TRACK_EN, 0);

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_frmtrk_mode);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     Enable DDDS close loop function. \n
 *     Enable this function when you want to track input (hdmi/dp) hsync.
 * @param[in] dev: device data struct
 * @param[in] enable: Enable close loop or not
 * @return
 *     0, success
 *     -EFAULT, invalid address point \n
 *     -EIO, device not enable.
 * @par Boundary case and Limitation
 *     Input signal (hdmi/dp) should be stable before enable this function. \n
 *     And also set hlen (mtk_ddds_set_hlen()) before enable this function.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EFAULT. \n
 *     2. The device not enable, return -EIO.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_close_loop(const struct device *dev, bool enable)
{
	struct mtk_ddds *ddds;

	if (!dev)
		return -EFAULT;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (!ddds->enabled)
		return -EIO;

	if (enable) {
		mtk_ddds_mask(ddds->regs, DDDS_02, DDDS_CTRL_PARA, 0x713);
		mtk_ddds_mask(ddds->regs, DDDS_03, SC_MJC_TRACK_SEL,
			      SC_MJC_TRACK_SEL);
		mtk_ddds_mask(ddds->regs, DDDS_00, (DISP_EN | FIX_FS_DDDS_SEL),
			      (DISP_EN | FIX_FS_DDDS_SEL));
	} else
		mtk_ddds_mask(ddds->regs, DDDS_00, (DISP_EN | FIX_FS_DDDS_SEL),
			      0);

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_close_loop);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     Check ddds locked or not. \n
 *     You can check ddds is lock or not after enable close loop.
 * @param[in] dev: device data struct
 * @return
 *     True, DDDS is locked \n
 *     False, DDDS is unlock or dev is null or hw not enable.
 * @par Boundary case and Limitation
 *     Check lock status after enable close loop (mtk_ddds_close_loop()).
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return false. \n
 *     2. The device not enable, return false.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
bool mtk_ddds_locked(const struct device *dev)
{
	struct mtk_ddds *ddds;
	u32 ddds_lock;

	if (!dev)
		return false;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return false;

	if (!ddds->enabled)
		return false;

	ddds_lock = readl(ddds->regs + STA_DDDS_00) & DDDS_LOCK;

	return ddds_lock ? true : false;
}
EXPORT_SYMBOL(mtk_ddds_locked);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     DDDS enable/disable function. \n
 *     Enable/disable ddds hw function.\n
 *     This function enable related clock for ddds.
 * @param[in] dev: device data struct
 * @param[in] enable: Enable ddds or not
 * @return
 *     0, function success.
 *     -EFAULT, invalid point.
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     Please call this function to \n
 *     enable ddds before you want to use this hw or \n
 *     disable ddds after you do not use this hw.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_enable(const struct device *dev, bool enable)
{
	struct mtk_ddds *ddds;
	int ret;

	if (!dev)
		return -EFAULT;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (enable) {
		if (!ddds->enabled) {
			ret = clk_set_parent(ddds->sys_sel,
					     ddds->ddds1_clk);
			if (ret)
				return ret;

			ret = clk_set_parent(ddds->fb_sel,
					     ddds->ddds_vsp_clk);
			if (ret)
				return ret;

			ret = clk_prepare_enable(ddds->sys_clk);
			if (ret)
				return ret;
			ret = clk_prepare_enable(ddds->fb_clk);
			if (ret)
				return ret;

			mtk_ddds_mask(ddds->regs, DDDS_03, SPREAD_INIT,
				      SPREAD_INIT);
			mtk_ddds_mask(ddds->regs, DDDS_03, DATA_SYNC_AUTO, 0);
			usleep_range(1000, 1200);
			mtk_ddds_mask(ddds->regs, DDDS_03, DATA_SYNC_AUTO,
				      DATA_SYNC_AUTO);
			mtk_ddds_mask(ddds->regs, DDDS_03, MUTE_FUNC_OFF,
				      MUTE_FUNC_OFF);

			ddds->enabled = true;
		}
	} else {
		if (ddds->enabled) {
			mtk_ddds_mask(ddds->regs, DDDS_03, DATA_SYNC_AUTO, 0);
			clk_disable_unprepare(ddds->fb_clk);
			clk_disable_unprepare(ddds->sys_clk);
			ddds->enabled = false;
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_enable);

/** @ingroup IP_group_ddds_external_function
 * @par Description
 *     DDDS reference clock output function. \n
 *     This output clock is used as reference clock of mipi tx pll. \n
 *     User can enable or disable this clock and also can select x'tal or \n
 *     ddds output clock as reference clock. \n
 *     Please call this api to select and enable clock before enable dsi.
 * @param[in] dev: device data struct
 * @param[in] xtal: output x'tal or ddds output clock as reference clock
 * @param[in] enable: Enable output clock to mipi tx pll
 * @return
 *     0, function success. \n
 *     -EFAULT, invalid address. \n
 *     -EINVAL, invalid parameter. \n
 *     error code from clk_prepare_enable() or clk_set_parent().
 * @par Boundary case and Limitation
 *     Please call this function to enable ddds output reference clock before \n
 *     you want to use mipi tx and dsi.
 * @par Error case and Error handling
 *     1. Device struct is not assigned, return -EFAULT. \n
 *     2. If output clock is from ddds and ddds not enable, return -EINVAL. \n
 *     3. If clk_prepare_enable() fails, return its error code. \n
 *     4. If clk_set_parent() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ddds_refclk_out(const struct device *dev, bool xtal, bool enable)
{
	struct mtk_ddds *ddds;
	int ret;

	if (!dev)
		return -EFAULT;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	if (!ddds->enabled && !xtal && enable)
		return -EINVAL;

	if (enable) {
		ret = clk_set_parent(ddds->ref_pll_sel, ddds->ddds_vsp_clk);
		if (ret)
			return ret;

		ret = clk_set_parent(
				ddds->ref_sel,
				((xtal) ? ddds->ref_clk26m : ddds->ref_pll_ck));
		if (ret)
			return ret;

		ret = clk_prepare_enable(ddds->ref_clk);
		if (ret)
			return ret;
	} else {
		ret = clk_set_parent(ddds->ref_sel, ddds->ref_clk26m);
		if (ret)
			return ret;

		clk_disable_unprepare(ddds->ref_clk);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_refclk_out);

/** @ingroup IP_group_ddds_external_function
  * @par Description
  *     Dump all ddds reigsters on the console.
  * @param[in] dev: device data struct
  * @return none
  * @par Boundary case and Limitation
  *     none
  * @par Error case and Error handling
  *     none
  * @par Call graph and Caller graph (refer to the graph below)
  * @par Refer to the source code
  */
int mtk_ddds_dump_registers(const struct device *dev)
{
	struct mtk_ddds *ddds;
	int i;

	if (!dev)
		return -EFAULT;

	ddds = dev_get_drvdata(dev);

	if (!ddds)
		return -EFAULT;

	for (i = 0; i < 0x80; i += 0x10) {
		dev_dbg(dev, "DDDS+%04x: %08x %08x %08x %08x\n", i,
			readl(ddds->regs + i),
			readl(ddds->regs + (i + 0x4)),
			readl(ddds->regs + (i + 0x8)),
			readl(ddds->regs + (i + 0xc)));
	}

	return 0;
}
EXPORT_SYMBOL(mtk_ddds_dump_registers);

/** @ingroup IP_group_ddds_internal_function
 * @par Description
 *     Platform device probe.
 * @param[in] pdev: platform device
 * @return
 *     0, success
 *     Other number, failure
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_ddds_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ddds *ddds;
	struct resource *regs;
	int ret = 0;

	dev_dbg(dev, "ddds probe starts\n");

	ddds = devm_kzalloc(dev, sizeof(*ddds), GFP_KERNEL);
	if (!ddds)
		return -ENOMEM;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddds->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(ddds->regs)) {
		ret = PTR_ERR(ddds->regs);
		dev_err(dev, "Failed to ioremap ddds registers: %d\n", ret);
		return ret;
	}

	ddds->sys_clk = devm_clk_get(dev, "sys-clk");
	if (IS_ERR(ddds->sys_clk)) {
		dev_err(dev, "cannot get ddds system clock\n");
		return PTR_ERR(ddds->sys_clk);
	}
	ddds->sys_sel = devm_clk_get(dev, "sys-sel");
	if (IS_ERR(ddds->sys_sel)) {
		dev_err(dev, "cannot get ddds sys_sel clock\n");
		return PTR_ERR(ddds->sys_sel);
	}
	ddds->ddds1_clk = devm_clk_get(dev, "ddds1-clk");
	if (IS_ERR(ddds->ddds1_clk)) {
		dev_err(dev, "cannot get ddds ddds1_clk clock\n");
		return PTR_ERR(ddds->ddds1_clk);
	}

	ddds->fb_clk = devm_clk_get(dev, "fb-clk");
	if (IS_ERR(ddds->fb_clk)) {
		dev_err(dev, "cannot get ddds feedback clock\n");
		return PTR_ERR(ddds->fb_clk);
	}
	ddds->fb_sel = devm_clk_get(dev, "fb-sel");
	if (IS_ERR(ddds->fb_sel)) {
		dev_err(dev, "cannot get ddds fb_sel clock\n");
		return PTR_ERR(ddds->fb_sel);
	}
	ddds->ddds_vsp_clk = devm_clk_get(dev, "ddds-vsp-clk");
	if (IS_ERR(ddds->ddds_vsp_clk)) {
		dev_err(dev, "cannot get ddds ddds_vsp_clk clock\n");
		return PTR_ERR(ddds->ddds_vsp_clk);
	}

	ddds->ref_clk = devm_clk_get(dev, "dsi-ref-clk");
	if (IS_ERR(ddds->ref_clk)) {
		dev_err(dev, "cannot get ddds output reference clock\n");
		return PTR_ERR(ddds->ref_clk);
	}
	ddds->ref_sel = devm_clk_get(dev, "dsi-ref-sel");
	if (IS_ERR(ddds->ref_sel)) {
		dev_err(dev, "cannot get ddds output reference clock mux\n");
		return PTR_ERR(ddds->ref_sel);
	}
	ddds->ref_clk26m = devm_clk_get(dev, "dsi-ref-clk26m");
	if (IS_ERR(ddds->ref_clk26m)) {
		dev_err(dev, "cannot get ddds output reference clock mux\n");
		return PTR_ERR(ddds->ref_clk26m);
	}
	ddds->ref_pll_ck = devm_clk_get(dev, "dsi-ref-pll-ck");
	if (IS_ERR(ddds->ref_pll_ck)) {
		dev_err(dev, "cannot get ddds output reference clock mux\n");
		return PTR_ERR(ddds->ref_pll_ck);
	}
	ddds->ref_pll_sel = devm_clk_get(dev, "dsi-ref-pll-sel");
	if (IS_ERR(ddds->ref_pll_sel)) {
		dev_err(dev, "cannot get ddds ref_pll_sel mux\n");
		return PTR_ERR(ddds->ref_pll_sel);
	}

	ddds->dev = dev;
	platform_set_drvdata(pdev, ddds);

	dev_dbg(dev, "ddds probe is done\n");
	return 0;
}

/** @ingroup IP_group_ddds_internal_function
 * @par Description
 *     Platform device remove.
 * @param[in] pdev: platform device
 * @return
 *     0, success
 *     Other number, failure
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_ddds_remove(struct platform_device *pdev)
{
	int ret = 0;

	return ret;
}

/**
 * @brief a device id structure for ddds driver
 */
static const struct of_device_id mtk_ddds_of_match[] = {
	{.compatible = "mediatek,mt3612-ddds"},
	{},
};
MODULE_DEVICE_TABLE(of, mtk_ddds_of_match);

/**
 * @brief a platform driver structure for ddds driver
 */
struct platform_driver mtk_ddds_driver = {
	.probe = mtk_ddds_probe,
	.remove = mtk_ddds_remove,
	.driver = {
		   .name = "mediatek-ddds",
		   .owner = THIS_MODULE,
		   .of_match_table = mtk_ddds_of_match,
		  },
};

module_platform_driver(mtk_ddds_driver);
MODULE_LICENSE("GPL");
