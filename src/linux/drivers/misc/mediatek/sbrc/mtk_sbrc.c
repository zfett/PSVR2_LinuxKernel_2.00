/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Peggy Jao <peggy.jao@mediatek.com>
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
 * @file mtk_sbrc.c
 *     sbrc driver.
 *     This sbrc driver is used to control MTK sbrc hardware module.
 */

/**
 * @defgroup IP_group_sbrc SBRC
 *     Strip Buffer Rendering Control hardware. It is proposed to\n
 *     maintain the fullness and emptiness of strip buffers between\n
 *     GPU (GPIO) and Ring Buffer Controller (RBFC_REN to RDMA).
 *
 *     @{
 *         @defgroup IP_group_sbrc_external EXTERNAL
 *             The external API document for sbrc.\n
 *
 *             @{
 *                 @defgroup IP_group_sbrc_external_function 1.function
 *                     External function for sbrc.
 *                 @defgroup IP_group_sbrc_external_struct 2.structure
 *                     None.
 *                 @defgroup IP_group_sbrc_external_typedef 3.typedef
 *                     None.
 *                 @defgroup IP_group_sbrc_external_enum 4.enumeration
 *                     None.
 *                 @defgroup IP_group_sbrc_external_def 5.define
 *                     None.
 *             @}
 *
 *         @defgroup IP_group_sbrc_internal INTERNAL
 *             The internal API document for sbrc.\n
 *
 *             @{
 *                 @defgroup IP_group_sbrc_internal_function 1.function
 *                     Internal function for sbrc and moudle init.
 *                 @defgroup IP_group_sbrc_internal_struct 2.structure
 *                     Internal structure used for sbrc.
 *                 @defgroup IP_group_sbrc_internal_typedef 3.typedef
 *                     None. Follow Linux coding style, no typedef used.
 *                 @defgroup IP_group_sbrc_internal_enum 4.enumeration
 *                     None. No enumeration in sbrc driver.
 *                 @defgroup IP_group_sbrc_internal_def 5.define
 *                     Internal definition used for sbrc.
 *             @}
 *     @}
 */

#include <asm-generic/io.h>
#ifdef CONFIG_MTK_DEBUGFS
#include <linux/debugfs.h>
#endif
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>
#include <soc/mediatek/mtk_sbrc.h>
#include "mtk_sbrc_int.h"
#include "sbrc_reg.h"

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     sbrc register mask write common function.
 * @param[in]
 *     regs: register base.
 * @param[in]
 *     offset: register offset.
 * @param[in]
 *     value: write data value.
 * @param[in]
 *     mask: write mask value.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int reg_write_mask(void __iomem *regs, u32 offset, u32 value, u32 mask)
{
	u32 reg;

	reg = readl(regs + offset);
	reg &= ~mask;
	reg |= value;
	writel(reg, regs + offset);

	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     sbrc register mask read common function.
 * @param[in]
 *     regs: register base.
 * @param[in]
 *     offset: register offset.
 * @param[in]
 *     mask: read mask value.
 * @return
 *     reg value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int reg_read_mask(void __iomem *regs, u32 offset, u32 mask)
{
	u32 reg;

	reg = readl(regs + offset);
	reg &= mask;

	return reg;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC read buffer status query.
 * @param[in]
 *     regs: SBRC register base.
 * @return
 *     read buffer index.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
#if 0 /* Todo */
static int mtk_sbrc_read_buffer_sta(void __iomem *regs)
{
	u32 reg;

	reg = reg_read_mask(regs, SBRC_BUFFER_STA, SBRC_READ_BUFFER_STA);
	return (reg >> 8);
}
#endif

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC write buffer status query.
 * @param[in]
 *     regs: SBRC register base.
 * @return
 *     write buffer index.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
#if 0 /* ToDo */
static int mtk_sbrc_write_buffer_sta(void __iomem *regs)
{
	return reg_read_mask(regs, SBRC_BUFFER_STA, SBRC_WRITE_BUFFER_STA);
}
#endif

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC set bypass mode by SW.
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     value: 1: enter bypass mode/0: exit bypass mode
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_set_bypass(void __iomem *regs, u32 value)
{
	reg_write_mask(regs, SBRC_BYPASS_MODE, value, SBRC_BYPASS_MODE_SET);
	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC clear bypass mode by SW.
 * @param[in]
 *     dev: SBRC device.
 * @param[in]
 *     handle: gce support for SBRC
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_clr_bypass(const struct device *dev, struct cmdq_pkt *handle)
{
	u32 status;
	int ret = 0;
	struct mtk_sbrc *sbrc;
	void __iomem *regs;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

	regs = sbrc->regs;
	status = reg_read_mask(regs, SBRC_BYPASS_MODE, SBRC_BYPASS_MODE_SET);
	/* if SW bypass is set, clear it first. */
	if (status == 1)
		mtk_sbrc_set_bypass(regs, 0);

	/* Toggle clear bit to exit bypass mode. */
	if (!handle) {
		reg_write_mask(regs, SBRC_BYPASS_MODE, 1 << 1,
			       SBRC_BYPASS_MODE_CLR);
		reg_write_mask(regs, SBRC_BYPASS_MODE, 0 << 1,
			       SBRC_BYPASS_MODE_CLR);
	} else {
		ret = cmdq_pkt_write_value(handle, sbrc->gce_subsys,
				(sbrc->reg_base & 0xffff) | SBRC_BYPASS_MODE,
				1 << 1, SBRC_BYPASS_MODE_CLR);
		ret = cmdq_pkt_write_value(handle, sbrc->gce_subsys,
				(sbrc->reg_base & 0xffff) | SBRC_BYPASS_MODE,
				0 << 1, SBRC_BYPASS_MODE_CLR);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_sbrc_clr_bypass);

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC set write over read threshold value.
 *     if the write agent to be threshold lines ahead of read agent,
 *     SBRC will notify write agent to speed up writing.
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     threshold: write over read threshold value
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_set_w_over_r_th(void __iomem *regs, u32 threshold)
{
	reg_write_mask(regs, SBRC_URGENT_TH_VAL, threshold,
		       SBRC_WRITE_OVER_READ_VAL);

	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC urgent signal config.
 *     SBRC will send urgent signal to EMI to arrange more bandwidth for GPU
 *     while GPU cannot render in time.
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     value: urgent signal enable bit.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_urgent_config(void __iomem *regs, u32 value)
{
	reg_write_mask(regs, SBRC_URGENT_ENABLE, value, SBRC_URGENT_EN);

	return 0;
}

#ifdef CONFIG_COMMON_CLK_MT3612
/** @ingroup IP_group_sbrc_external_function
 ** @par Description
 *  Turn on the power of SBRC.
 * @param[in]
 *    dev: sbrc device node.
 * @return
 *    0, successfully append command into the cmdq command buffer.
 *    -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_power_on(struct device *dev)
{
	struct mtk_sbrc *sbrc;
	int ret = 0;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	/* enable power domain */
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev,
		"Failed to enable power domain: %d\n", ret);
		return ret;
	}

	/* clear clock gating */
	ret = clk_prepare_enable(sbrc->sbrc_clk);
	if (ret) {
		clk_disable_unprepare(sbrc->sbrc_clk);
		dev_err(dev, "Failed to enable clk:%d\n", ret);
		return ret;
	}
#endif

	return ret;
}
EXPORT_SYMBOL(mtk_sbrc_power_on);

/** @ingroup IP_group_sbrc_external_function
 ** @par Description
 *  Turn off the power of SBRC.
 * @param[in]
 *    dev: sbrc device node.
 * @return
 *    0, successfully append command into the cmdq command buffer.
 *    -EINVAL, invalid parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If fe dev is NULL, return -EINVAL
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_power_off(struct device *dev)
{
	struct mtk_sbrc *sbrc;
	int ret = 0;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	clk_disable_unprepare(sbrc->sbrc_clk);
	pm_runtime_put(dev);
#endif
	return ret;
}
EXPORT_SYMBOL(mtk_sbrc_power_off);
#endif

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC GCE event config.
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     value: GCE event config.
 * @return
 *     0, configured successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_gce_config(void __iomem *regs, u32 value)
{
	reg_write_mask(regs, SBRC_GCE_CONFIG, value, GCE_EVENT_MASK);
	return 0;
}

#if 0
/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC software reset.
 * @param[in]
 *     regs: SBRC register base.
 * @return
 *     0, write successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_sw_reset(void __iomem *regs)
{
	reg_write_mask(regs, SBRC_SW_RESET, 1, SBRC_SW_RST);
	reg_write_mask(regs, SBRC_SW_RESET, 0, SBRC_SW_RST);

	return 0;
}
#endif

/** @ingroup IP_group_sbrc_external_function
 * @par Description
 *     SBRC set enable.
 * @param[in]
 *     dev: sbrc device node.
 * @return
 *     0, write successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_enable(const struct device *dev)
{
	struct mtk_sbrc *sbrc;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;
	reg_write_mask(sbrc->regs, SBRC_ENABLE, 1, SBRC_EN);

	/* setup Qos/priority value */
	mtk_sbrc_set_w_over_r_th(sbrc->regs, WRITE_OVER_READ_TH);
	mtk_sbrc_urgent_config(sbrc->regs, SET_ON);

	return 0;
}
EXPORT_SYMBOL(mtk_sbrc_enable);

/** @ingroup IP_group_sbrc_external_function
 * @par Description
 *     SBRC set disable.
 * @param[in]
 *     dev: sbrc device node.
 * @return
 *     0, write successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_disable(const struct device *dev)
{
	struct mtk_sbrc *sbrc;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;
	reg_write_mask(sbrc->regs, SBRC_ENABLE, 0, SBRC_EN);

	return 0;
}
EXPORT_SYMBOL(mtk_sbrc_disable);

/** @ingroup IP_group_sbrc_external_function
 * @par Description
 *     set SBRC SW control read buffer index.
 * @param[in]
 *     dev: sbrc device node.
 * @param[in]
 *     idx: forced read buffer index
 * @param[in]
 *     handle: gce support for SBRC
 * @return
 *     0, write successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_set_sw_control_read_idx(const struct device *dev, u32 idx,
				    struct cmdq_pkt *handle)
{
	struct mtk_sbrc *sbrc;
	int ret = 0;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

	if (idx > 256) {
		dev_err(dev, "[Fail] read buffer index: %d should < 256!\n",
			idx);
		return -EINVAL;
	}

	if (!handle) {
		reg_write_mask(sbrc->regs, SBRC_SW_CTRL_READ_BUFFER, idx << 1,
					   SW_CTRL_READ_BUF_INDEX);

	} else {
		ret = cmdq_pkt_write_value(handle, sbrc->gce_subsys,
		      (sbrc->reg_base & 0xffff) | SBRC_SW_CTRL_READ_BUFFER,
		      idx << 1, SW_CTRL_READ_BUF_INDEX);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_sbrc_set_sw_control_read_idx);

/** @ingroup IP_group_sbrc_external_function
 * @par Description
 *     SBRC SW control read buffer index.
 * @param[in]
 *     dev: sbrc device node.
 * @param[in]
 *     handle: gce support for SBRC
 * @return
 *     0, write successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_sw_control_read(const struct device *dev,
				    struct cmdq_pkt *handle)
{
	struct mtk_sbrc *sbrc;
	int ret = 0;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

	if (!handle) {
		reg_write_mask(sbrc->regs, SBRC_SW_CTRL_READ_BUFFER, 1,
					   SW_CTRL_READ_BUF_EN);
		reg_write_mask(sbrc->regs, SBRC_SW_CTRL_READ_BUFFER, 0,
					   SW_CTRL_READ_BUF_EN);
	} else {
		ret = cmdq_pkt_write_value(handle, sbrc->gce_subsys,
		      (sbrc->reg_base & 0xffff) | SBRC_SW_CTRL_READ_BUFFER,
		      1, SW_CTRL_READ_BUF_EN);
		ret = cmdq_pkt_write_value(handle, sbrc->gce_subsys,
		      (sbrc->reg_base & 0xffff) | SBRC_SW_CTRL_READ_BUFFER,
		      0, SW_CTRL_READ_BUF_EN);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_sbrc_sw_control_read);

/** @ingroup IP_group_sbrc_external_function
 * @par Description
 *     SBRC set buffer number.
 * @param[in]
 *     dev: sbrc device node.
 * @param[in]
 *     buf_num: buffer number
 *     buffer number should be in the range of 2 to 256
 * @return
 *     -EINVAL, wrong parameter.
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     buf_num should be in the range of 2 to 256.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_set_buffer_num(const struct device *dev, u32 buf_num)
{
	struct mtk_sbrc *sbrc;

	if (!dev)
		return -EINVAL;

	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

	if ((buf_num < 2) || (buf_num > 256)) {
		dev_err(dev, "[Fail] buffer num: %d should be within 2-256!\n",
			buf_num);
		return -EINVAL;
	}

	reg_write_mask(sbrc->regs, SBRC_BUFFER_NUM_CFG, buf_num,
			SBRC_BUFFER_NUM);
	return 0;
}
EXPORT_SYMBOL(mtk_sbrc_set_buffer_num);

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC set strip number.
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     strip_num: strip number
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_set_strip_num(void __iomem *regs, u32 strip_num)
{
	reg_write_mask(regs, SBRC_TOTAL_STRIP_NUM_CFG, strip_num,
			SBRC_TOTAL_STRIP_NUM);
	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC set line remainder.
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     line_remainder: line remainder
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_set_line_remainder(void __iomem *regs, u32 line_remainder)
{
	reg_write_mask(regs, SBRC_LINE_REMAINDER_CFG, line_remainder,
			SBRC_LINE_REMAINDER);
	return 0;
}

/** @ingroup IP_group_sbrc_external_function
 * @par Description
 *     SBRC set buffer depth.
 * @param[in]
 *     dev: sbrc device node.
 * @param[in]
 *     frame_height: GPU frame height
 * @param[in]
 *     depth: strip buffer depth(default: 128 lines)
 *     depth should be in the range of 2 to 256.
 *     depth should be smaller than frame_height.
 * @return
 *     0, write successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     depth should be in the range of 2 to 256.
 *     depth should be smaller than frame_height.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_set_buffer_depth(
	const struct device *dev, u32 frame_height, u32 depth)
{
	struct mtk_sbrc *sbrc;
	u32 strip_num;
	u32 line_remainder;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

	if (frame_height == 0) {
		dev_err(dev, "[Fail] frame_height: %d should be > 0\n",
			frame_height);
		return -EINVAL;
	}

	if ((depth > 256) || (depth < 2)) {
		dev_err(dev, "[Fail] depth: %d should be within 2-256!\n",
			depth);
		return -EINVAL;
	}

	if (depth > frame_height) {
		dev_err(dev, "[Fail] depth: %d is > frame_height: %d!\n",
			depth, frame_height);
		return -EINVAL;
	}

	strip_num = frame_height/depth;
	line_remainder = frame_height - strip_num * depth;
	if (line_remainder) /* strip_num = ceil(frame_height/depth) */
		strip_num = strip_num + 1;

	mtk_sbrc_set_strip_num(sbrc->regs, strip_num);
	mtk_sbrc_set_line_remainder(sbrc->regs, line_remainder);

	reg_write_mask(sbrc->regs, SBRC_BUFFER_DEPTH_CFG, depth,
		       SBRC_BUFFER_DEPTH);
	return 0;
}
EXPORT_SYMBOL(mtk_sbrc_set_buffer_depth);

/** @ingroup IP_group_sbrc_external_function
 * @par Description
 *     SBRC set buffer size.
 * @param[in]
 *     dev: sbrc device node.
 * @param[in]
 *     width: GPU image width in bytes. (ex: 4000 32bbp = 4000*32/8)
 *     width should be smaller than 32767.
 * @return
 *     0, write successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     width should be smaller than 32767.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_set_buffer_size(const struct device *dev, u32 width)
{
	struct mtk_sbrc *sbrc;
	u32 buffer_depth;
	u32 buffer_size;

	if (!dev)
		return -EINVAL;
	sbrc = dev_get_drvdata(dev);

	if (!sbrc)
		return -EINVAL;

	if ((width > 32767) || (width < 1)) {
		dev_err(dev, "[Fail] width: %d should be within 1-32767!\n",
			width);
		return -EINVAL;
	}

	buffer_depth = reg_read_mask(sbrc->regs, SBRC_BUFFER_DEPTH_CFG,
				SBRC_BUFFER_DEPTH);

	if ((buffer_depth > 256) || (buffer_depth < 2)) {
		dev_err(dev, "[Fail] depth: %d should be within 2-256!\n",
			buffer_depth);
		return -EINVAL;
	}

	buffer_size = width * buffer_depth;

	reg_write_mask(sbrc->regs, SBRC_BUFFER_SIZE_CFG, buffer_size,
		       SBRC_BUFFER_SIZE);
	return 0;
}
EXPORT_SYMBOL(mtk_sbrc_set_buffer_size);

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC set GPU write over threshold.
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     value: GPU write over threshold
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_set_w_over_th(void __iomem *regs, u32 value)
{
	reg_write_mask(regs, SBRC_W_OVER_TH_VAL, value, SBRC_W_OVER_TH_VALUE);
	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC set timeout
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     value: delay cycle for GPIO timeout mechanism
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_set_timeout(void __iomem *regs, u32 value)
{
	reg_write_mask(regs, SBRC_GPIO_TIMEOUT_VAL, value,
		       SBRC_GPIO_TIMEOUT_VALUE);
	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC mutex config
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     value: MUTEX event config.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_mutex_config(void __iomem *regs, u32 value)
{
	reg_write_mask(regs, SBRC_MUTEX_CONFIG, value, MUTEX_EVENT_MASK);
	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC irq config
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     value: IRQ event enable.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_irq_config(void __iomem *regs, u32 value)
{
	reg_write_mask(regs, SBRC_IRQ_CONFIG, value, IRQ_ENABLE_MASK);
	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC irq clear
 * @param[in]
 *     regs: SBRC register base.
 * @param[in]
 *     mask: IRQ event mask.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_irq_clear(void __iomem *regs, u32 mask)
{
	reg_write_mask(regs, SBRC_IRQ_CONFIG, mask, mask);
	reg_write_mask(regs, SBRC_IRQ_CONFIG, 0, mask);
	return 0;
}

#if 0
/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     sbrc get bypass mode status
 * @param[in]
 *     sbrc: struct mtk_sbrc
 * @return
 *     sbrc bypass mode.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int sbrc_is_bypass_mode(struct mtk_sbrc *sbrc)
{
	return reg_read_mask(sbrc->regs, SBRC_MON, ALL_BYPASS_MODE_STA);
}
#endif


/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC callback function for IRQ handle.
 * @param[in]
 *     data: sbrc callback data.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_sbrc_irq_cb(struct mtk_mmsys_cb_data *data)
{
	struct mtk_sbrc *sbrc = data->priv_data;

	sbrc->irq_mask = data->status; /* irq status */

	/* SBRC write over threshold */
	if (sbrc->irq_mask & IRQ_W_OVER_TH_STA)
		mtk_sbrc_irq_clear(sbrc->regs, IRQ_W_OVER_TH_CLR);

	/* SBRC urgent signal trigger */
	if (sbrc->irq_mask & IRQ_URGENT_STA)
		mtk_sbrc_irq_clear(sbrc->regs, IRQ_URGENT_CLR);

	/* SBRC read over write */
	if (sbrc->irq_mask & IRQ_W_INCOMPLETE_STA) {
		dev_dbg(sbrc->dev, "[SBRC_IRQ] W_INCOMPLETE\n");
		mtk_sbrc_irq_clear(sbrc->regs, IRQ_W_INCOMPLETE_CLR);
	}

	/* SBRC GPU render fault */
	if (sbrc->irq_mask & IRQ_GPU_RENDER_FAULT_STA) {
		dev_dbg(sbrc->dev, "[SBRC_IRQ] GPU_RENDER_FAULT\n");
		mtk_sbrc_irq_clear(sbrc->regs, IRQ_GPU_RENDER_FAULT_CLR);
	}

	/* SBRC and GPU buffer index differs */
	if (sbrc->irq_mask & IRQ_GPU_N_SBRC_RW_CLASH_STA) {
		dev_dbg(sbrc->dev, "[SBRC_IRQ] GPU_N_SBRC_RW_CLASH\n");
		mtk_sbrc_irq_clear(sbrc->regs, IRQ_GPU_N_SBRC_RW_CLASH_CLR);
	}

	/* SBRC GPIO timeout */
	if (sbrc->irq_mask & IRQ_GPIO_TIMEOUT_STA) {
		dev_dbg(sbrc->dev, "[SBRC_IRQ] GPIO_TIMEOUT\n");
		mtk_sbrc_irq_clear(sbrc->regs, IRQ_GPIO_TIMEOUT_CLR);
	}
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     Register callback function for SBRC IRQ.
 * @param[in]
 *     dev: pointer of sbrc device structure.
 * @param[in]
 *     func: callback function which is called by SBRC interrupt handler.
 * @param[in]
 *     status: status that user want to monitor.
 * @param[in]
 *     priv_data: caller may have multiple instance, so it could use this\n
 *                private data to distinguish which instance is callbacked.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_register_cb(const struct device *dev, mtk_mmsys_cb func,
			u32 status, void *priv_data)
{
	struct mtk_sbrc *sbrc = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&sbrc->spinlock_irq, flags);
	sbrc->cb_func = func;
	sbrc->cb_status = status;
	sbrc->cb_priv_data = priv_data;
	spin_unlock_irqrestore(&sbrc->spinlock_irq, flags);

	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *    sbrc driver irq handler.
 * @param[in] irq: irq information.
 * @param[in] dev_id: sbrc driver data.
 * @return
 *     irqreturn_t.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_sbrc_irq_handler(int irq, void *dev_id)
{
	struct mtk_sbrc *sbrc = dev_id;
	u32 status;
	struct mtk_mmsys_cb_data cb_data;
	mtk_mmsys_cb func;
	unsigned long flags;

	status = readl(sbrc->regs + SBRC_IRQ_STATUS);

	spin_lock_irqsave(&sbrc->spinlock_irq, flags);
	func = sbrc->cb_func;
	cb_data.status = status;
	cb_data.priv_data = sbrc->cb_priv_data;
	spin_unlock_irqrestore(&sbrc->spinlock_irq, flags);

	cb_data.part_id = 0;

	if (func && cb_data.status)
		func(&cb_data);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC initialization.
 * @param[in]
 *     dev: sbrc device node.
 * @return
 *     0, configured successfully.
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_drv_init(const struct device *dev)
{
	struct mtk_sbrc *sbrc;
	void __iomem *regs;
	u32 width;

	if (!dev)
		return -EINVAL;

	sbrc = dev_get_drvdata(dev);
	regs = sbrc->regs;
	/* set default GPU width 4000 32bpp */
	width = 4000*32/8;

	mtk_sbrc_set_buffer_num(dev, BUFFER_NUM);
	mtk_sbrc_set_buffer_depth(dev, GPU_FRAME_HEIGHT, BUFFER_DEPTH);
	mtk_sbrc_set_buffer_size(dev, width);
	mtk_sbrc_set_bypass(regs, SET_OFF);
	mtk_sbrc_set_w_over_th(regs, GPU_W_OVER_TH);
	mtk_sbrc_set_w_over_r_th(regs, WRITE_OVER_READ_TH);
	mtk_sbrc_set_timeout(regs, GPIO_TIMEOUT);

	mtk_sbrc_gce_config(regs, 0x1bf);
	mtk_sbrc_mutex_config(regs, 0x16);
	mtk_sbrc_irq_config(regs, 0x27);
	mtk_sbrc_urgent_config(regs, SET_ON);
	mtk_sbrc_disable(dev);

	mtk_sbrc_register_cb(dev, &mtk_sbrc_irq_cb, 0, sbrc);

	return 0;
}
EXPORT_SYMBOL(mtk_sbrc_drv_init);

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, probe succeeds.\n
 *     Otherwise, sbrc probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_sbrc *sbrc;
	struct resource *regs;
	int err;

	sbrc = devm_kzalloc(dev, sizeof(*sbrc), GFP_KERNEL);
	if (!sbrc)
		return -ENOMEM;

	sbrc->dev = dev;

#ifdef CONFIG_COMMON_CLK_MT3612
	sbrc->sbrc_clk = devm_clk_get(dev, "mmsys_core_sbrc");
	if (IS_ERR(sbrc->sbrc_clk)) {
		dev_err(dev, "Failed to get sbrc clk\n");
		return PTR_ERR(sbrc->sbrc_clk);
	}
#endif

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sbrc->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(sbrc->regs)) {
		dev_err(dev, "Failed to map sbrc registers\n");
		return PTR_ERR(sbrc->regs);
	}
	sbrc->reg_base = (u32)regs->start;

	spin_lock_init(&sbrc->spinlock_irq);
	sbrc->irq = platform_get_irq(pdev, 0);
	if (sbrc->irq < 0) {
		dev_err(dev, "Failed to get sbrc irq\n");
		return sbrc->irq;
	}

	err = devm_request_irq(dev, sbrc->irq, mtk_sbrc_irq_handler,
			       IRQF_TRIGGER_LOW, dev_name(dev), sbrc);
	if (err < 0) {
		dev_err(dev, "Failed to request sbrc irq\n");
		return err;
	}

	of_node_put(dev->of_node);

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_enable(dev);
#endif

	platform_set_drvdata(pdev, sbrc);

	err = mtk_sbrc_drv_init(dev);
	if (err < 0) {
		dev_err(dev, "Failed to initialize sbrc driver\n");
		return err;
	}

	/* GCE Init */
	err = of_property_read_u32(dev->of_node, "gce-subsys",
				&sbrc->gce_subsys);
	if (err) {
		dev_err(dev, "parsing gce-subsys error: %d\n", err);
		return -EINVAL;
	}

	dev_dbg(dev, "sbrc probe done!!\n");

#ifdef CONFIG_MTK_DEBUGFS
	err = mtk_sbrc_debugfs_init(sbrc);
	if (err < 0)
		dev_err(dev, "Failed to initialize sbrc debugfs\n");
#endif

	return 0;
}

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     platform_driver remove function.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, remove succeeds.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_remove(struct platform_device *pdev)
{
	struct mtk_sbrc *sbrc = platform_get_drvdata(pdev);

#ifdef CONFIG_MTK_DEBUGFS
	mtk_sbrc_debugfs_exit(sbrc);
#endif
	devm_free_irq(&pdev->dev, sbrc->irq, sbrc);
	mtk_sbrc_disable(sbrc->dev);

#ifdef CONFIG_COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif

	return 0;
}

static const struct of_device_id sbrc_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-sbrc"},
	{},
};
MODULE_DEVICE_TABLE(of, sbrc_driver_dt_match);

struct platform_driver mtk_sbrc_driver = {
	.probe		= mtk_sbrc_probe,
	.remove		= mtk_sbrc_remove,
	.driver		= {
		.name	= "mediatek-sbrc",
		.owner	= THIS_MODULE,
		.of_match_table = sbrc_driver_dt_match,
	},
};

module_platform_driver(mtk_sbrc_driver);
MODULE_LICENSE("GPL");
