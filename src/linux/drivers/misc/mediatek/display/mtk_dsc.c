/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	Randy Lin <randy.lin@mediatek.com>
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
 *	@file mtk_dsc.c
 *	MTK DSC Linux Driver. \n
 *	This driver is used to configure MTK DSC hardware module. \n
 *	It includes stream format, frame size, data compression \n
 *	and DSC core number setting.
 */

/**
 * @defgroup IP_group_dsc DSC
 *   There are 2 warppers and 2 cores(each warpper) in MT3612. \n
 *   DSC engine can support RGB888/RGB101010 compression data format. \n
 *   And also support bypass or relay mode.
 *
 *   @{
 *       @defgroup IP_group_dsc_external EXTERNAL
 *         The external API document for dsc.
 *
 *         @{
 *             @defgroup IP_group_dsc_external_function 1.function
 *               This is dsc external function.
 *             @defgroup IP_group_dsc_external_struct 2.structure
 *               External structure used for dsc.
 *             @defgroup IP_group_dsc_external_typedef 3.typedef
 *               None. No external typedef used in dsc driver.
 *             @defgroup IP_group_dsc_external_enum 4.enumeration
 *               None. No external enumeration used in dsc driver.
 *             @defgroup IP_group_dsc_external_def 5.define
 *               External define used for dsc.
 *         @}
 *
 *       @defgroup IP_group_dsc_internal INTERNAL
 *         The internal API document for dsc.
 *
 *         @{
 *             @defgroup IP_group_dsc_internal_function 1.function
 *               This is dsc internal function.
 *             @defgroup IP_group_dsc_internal_struct 2.structure
 *               Internal structure used for dsc.
 *             @defgroup IP_group_dsc_internal_typedef 3.typedef
 *               None. No internal typedef used in dsc driver.
 *             @defgroup IP_group_dsc_internal_enum 4.enumeration
 *               None. No internal enumeration used in dsc driver.
 *             @defgroup IP_group_dsc_internal_def 5.define
 *               Internal define used for dsc.
 *         @}
 *   @}
 */

#include <asm-generic/io.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/mtk_dsc.h>
#include <soc/mediatek/mtk_drv_def.h>
#include <soc/mediatek/mtk_dsi.h>
#include "mtk_dsc_reg.h"

/**	@ingroup IP_group_dsc_internal_def
 *	@{
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define CORE_OFFSET			0x400

#define DSC_EN_SET(x)			((x) << 0)
#define DSC_DUAL_INOUT_SET(x)		((x) << 2)
#define DSC_IN_SRC_SEL_SET(x)		((x) << 3)
#define DSC_BYPASS_SET(x)		((x) << 4)
#define DSC_RELAY_SET(x)		((x) << 5)
#define DSC_SW_RESET_SET(x)		((x) << 8)
#define STALL_CLK_GATE_EN_SET(x)	((x) << 9)

#define DSC_DONE_INTEN_SET(x)		((x) << 0)
#define DSC_ERR_INTEN_SET(x)		((x) << 1)
#define ZERO_FIFO_ERR_INTEN_SET(x)	((x) << 2)
#define ABNORMAL_EOF_INTEN_SET(x)	((x) << 3)
#define UNMUTE_INTEN_SET(x)		((x) << 4)
#define MUTE_INTEN_SET(x)		((x) << 5)

#define DSC_DONE_INTACK_SET(x)		((x) << 0)
#define DSC_ERR_INTACK_SET(x)		((x) << 1)
#define ZERO_FIFO_ERR_INTACK_SET(x)	((x) << 2)
#define ABNORMAL_EOF_INTACK_SET(x)	((x) << 3)
#define UNMUTE_INTACK_SET(x)		((x) << 4)
#define MUTE_INTACK_SET(x)		((x) << 5)

#define PIC_WIDTH_SET(x)		((x) << 0)
#define PIC_GROUP_WIDTH_M1_SET(x)	((x) << 16)

#define PIC_HEIGHT_M1_SET(x)		((x) << 0)
#define PIC_HEIGHT_EXT_M1_SET(x)	((x) << 16)

#define SLICE_WIDTH_SET(x)		((x) << 0)
#define SLICE_GROUP_WIDTH_M1_SET(x)	((x) << 16)

#define SLICE_HEIGHT_M1_SET(x)		((x) << 0)
#define SLICE_NUM_M1_SET(x)		((x) << 16)
#define SLICE_WIDTH_MOD3_SET(x)		((x) << 30)

#define CHUNK_SIZE_SET(x)		((x) << 0)

#define BUF_SIZE_SET(x)			((x) << 0)

#define SLICE_MODE_SET(x)		((x) << 0)
#define PIX_TYPE_SET(x)			((x) << 1)
#define RGB_SWAP_SET(x)			((x) << 2)
#define INIT_DELAY_HEIGHT_SET(x)	((x) << 8)
#define OBUF_STR_IF_BUF_FULL_SET(x)	((x) << 16)

#define DSC_CFG_SET(x)			((x) << 0)
#define ICH_LINE_CLEAR_SET(x)		((x) << 6)

#define PAD_NUM_SET(x)			((x) << 0)

#define CKSM_CAL_EN_SET(x)		((x) << 9)

#define DSC_MUTE_VALUE_SET(x)		((x) << 0)
#define SW_MUTE_CONTROL_SET(x)		((x) << 30)
#define HW_MUTE_ENABLE_SET(x)		(((unsigned)x) << 31)

#define OBUF_SIZE_SET(x)		((x) << 0)
#define OBUF_SW_SET(x)			(((unsigned)x) << 31)

#define UP_LINE_BUF_DEPTH_SET(x)	((x) << 0)
#define BIT_PER_CHANNEL_SET(x)		((x) << 4)
#define BIT_PER_PIXEL_SET(x)		((x) << 8)
#define RCT_ON_SET(x)			((x) << 18)
#define BP_ENABLE_SET(x)		((x) << 19)

#define INITIAL_XMIT_DELAY_SET(x)	((x) << 0)
#define INITIAL_DEC_DELAY_SET(x)	((x) << 16)

#define INITIAL_SCALE_VALUE_SET(x)	((x) << 0)
#define SCALE_INCREMENT_INTERVAL_SET(x)	((x) << 16)

#define SCALE_DECREMENT_INTERVAL_SET(x)	((x) << 0)
#define FIRST_LINE_BPG_OFFSET_SET(x)	((x) << 16)

#define NFL_BPG_OFFSET_SET(x)		((x) << 0)
#define SLICE_BPG_OFFSET_SET(x)		((x) << 16)

#define INITIAL_OFFSET_SET(x)		((x) << 0)
#define FINAL_OFFSET_SET(x)		((x) << 16)

#define FLATNESS_MIN_QP_SET(x)		((x) << 0)
#define FLATNESS_MAX_QP_SET(x)		((x) << 8)
#define RC_MODEL_SIZE_SET(x)		((x) << 16)

#define RC_EDGE_FACTOR_SET(x)		((x) << 0)
#define RC_QUANT_INCR_LIMIT0_SET(x)	((x) << 8)
#define RC_QUANT_INCR_LIMIT1_SET(x)	((x) << 16)
#define RC_TGT_OFFSET_HI_SET(x)		((x) << 24)
#define RC_TGT_OFFSET_LO_SET(x)		((x) << 28)

#define RC_BUF_THR0_SET(x)		((x) << 0)
#define RC_BUF_THR1_SET(x)		((x) << 8)
#define RC_BUF_THR2_SET(x)		((x) << 16)
#define RC_BUF_THR3_SET(x)		((x) << 24)

#define RC_BUF_THR4_SET(x)		((x) << 0)
#define RC_BUF_THR5_SET(x)		((x) << 8)
#define RC_BUF_THR6_SET(x)		((x) << 16)
#define RC_BUF_THR7_SET(x)		((x) << 24)

#define RC_BUF_THR8_SET(x)		((x) << 0)
#define RC_BUF_THR9_SET(x)		((x) << 8)
#define RC_BUF_THR10_SET(x)		((x) << 16)
#define RC_BUF_THR11_SET(x)		((x) << 24)

#define RC_BUF_THR12_SET(x)		((x) << 0)
#define RC_BUF_THR13_SET(x)		((x) << 8)

#define RANGE_MIN_QP0_SET(x)		((x) << 0)
#define RANGE_MAX_QP0_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET0_SET(x)	((x) << 10)
#define RANGE_MIN_QP1_SET(x)		((x) << 16)
#define RANGE_MAX_QP1_SET(x)		((x) << 21)
#define RANGE_BPG_OFFSET1_SET(x)	((x) << 26)

#define RANGE_MIN_QP2_SET(x)		((x) << 0)
#define RANGE_MAX_QP2_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET2_SET(x)	((x) << 10)
#define RANGE_MIN_QP3_SET(x)		((x) << 16)
#define RANGE_MAX_QP3_SET(x)		((x) << 21)
#define RANGE_BPG_OFFSET3_SET(x)	((x) << 26)

#define RANGE_MIN_QP4_SET(x)		((x) << 0)
#define RANGE_MAX_QP4_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET4_SET(x)	((x) << 10)
#define RANGE_MIN_QP5_SET(x)		((x) << 16)
#define RANGE_MAX_QP5_SET(x)		((x) << 21)
#define RANGE_BPG_OFFSET5_SET(x)	((x) << 26)

#define RANGE_MIN_QP6_SET(x)		((x) << 0)
#define RANGE_MAX_QP6_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET6_SET(x)	((x) << 10)
#define RANGE_MIN_QP7_SET(x)		((x) << 16)
#define RANGE_MAX_QP7_SET(x)		((x) << 21)
#define RANGE_BPG_OFFSET7_SET(x)	((x) << 26)

#define RANGE_MIN_QP8_SET(x)		((x) << 0)
#define RANGE_MAX_QP8_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET8_SET(x)	((x) << 10)
#define RANGE_MIN_QP9_SET(x)		((x) << 16)
#define RANGE_MAX_QP9_SET(x)		((x) << 21)
#define RANGE_BPG_OFFSET9_SET(x)	((x) << 26)

#define RANGE_MIN_QP10_SET(x)		((x) << 0)
#define RANGE_MAX_QP10_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET10_SET(x)	((x) << 10)
#define RANGE_MIN_QP11_SET(x)		((x) << 16)
#define RANGE_MAX_QP11_SET(x)		((x) << 21)
#define RANGE_BPG_OFFSET11_SET(x)	((x) << 26)

#define RANGE_MIN_QP12_SET(x)		((x) << 0)
#define RANGE_MAX_QP12_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET12_SET(x)	((x) << 10)
#define RANGE_MIN_QP13_SET(x)		((x) << 16)
#define RANGE_MAX_QP13_SET(x)		((x) << 21)
#define RANGE_BPG_OFFSET13_SET(x)	((x) << 26)

#define RANGE_MIN_QP14_SET(x)		((x) << 0)
#define RANGE_MAX_QP14_SET(x)		((x) << 5)
#define RANGE_BPG_OFFSET14_SET(x)	((x) << 10)

#define FORCE_COMMIT_SET(x)		((x) << 0)
#define BYPASS_SHADOW_SET(x)		((x) << 1)
#define READ_WORKING_SET(x)		((x) << 2)
#define MUTE_HIGH_TO_LOW_SYNC_SOF_SET(x)((x) << 3)
#define MUTE_LOW_TO_HIGH_SYNC_SOF_SET(x)((x) << 4)
#define DSC_VERSION_MINOR_SET(x)	((x) << 5)
#define RELAY_FIFO_MODE_OFF_SET(x)	((x) << 9)

#define DSC_CFG_10BPC			0x0000d028
#define DSC_CFG_8BPC			0x0000d022

#define DSC_PPS6_10BPC_10BPP		0x20001007
#define DSC_PPS6_10BPC_15BPP		0x20001007
#define DSC_PPS6_8BPC_8BPP		0x20000c03
#define DSC_PPS6_8BPC_12BPP		0x20000c03

#define DSC_PPS7_10BPC_10BPP		0x330f0f06
#define DSC_PPS7_10BPC_15BPP		0x330f0f06
#define DSC_PPS7_8BPC_8BPP		0x330b0b06
#define DSC_PPS7_8BPC_12BPP		0x330b0b06

#define DSC_PPS8_10BPC_10BPP		0x382a1c0e
#define DSC_PPS8_10BPC_15BPP		0x382a1c0e
#define DSC_PPS8_8BPC_8BPP		0x382a1c0e
#define DSC_PPS8_8BPC_12BPP		0x382a1c0e

#define DSC_PPS9_10BPC_10BPP		0x69625446
#define DSC_PPS9_10BPC_15BPP		0x69625446
#define DSC_PPS9_8BPC_8BPP		0x69625446
#define DSC_PPS9_8BPC_12BPP		0x69625446

#define DSC_PPS10_10BPC_10BPP		0x7b797770
#define DSC_PPS10_10BPC_15BPP		0x7b797770
#define DSC_PPS10_8BPC_8BPP		0x7b797770
#define DSC_PPS10_8BPC_12BPP		0x7b797770

#define DSC_PPS11_10BPC_10BPP		0x00007e7d
#define DSC_PPS11_10BPC_15BPP		0x00007e7d
#define DSC_PPS11_8BPC_8BPP		0x00007e7d
#define DSC_PPS11_8BPC_12BPP		0x00007e7d

#define DSC_PPS12_10BPC_10BPP		0x010408e0
#define DSC_PPS12_10BPC_15BPP		0x20a22840
#define DSC_PPS12_8BPC_8BPP		0x00800880
#define DSC_PPS12_8BPC_12BPP		0x00800840

#define DSC_PPS13_10BPC_10BPP		0xf9460125
#define DSC_PPS13_10BPC_15BPP		0x10c418a3
#define DSC_PPS13_8BPC_8BPP		0xf8c100a1
#define DSC_PPS13_8BPC_12BPP		0xf8c100a1

#define DSC_PPS14_10BPC_10BPP		0xe967f167
#define DSC_PPS14_10BPC_15BPP		0x00e508c5
#define DSC_PPS14_8BPC_8BPP		0xe8e3f0e3
#define DSC_PPS14_8BPC_12BPP		0xe8e3f0e3

#define DSC_PPS15_10BPC_10BPP		0xe187e167
#define DSC_PPS15_10BPC_15BPP		0xf106f8e5
#define DSC_PPS15_8BPC_8BPP		0xe103e0e3
#define DSC_PPS15_8BPC_12BPP		0xe103e0e3

#define DSC_PPS16_10BPC_10BPP		0xd9a7e1a7
#define DSC_PPS16_10BPC_15BPP		0xe127e926
#define DSC_PPS16_8BPC_8BPP		0xd943e123
#define DSC_PPS16_8BPC_12BPP		0xd923e103

#define DSC_PPS17_10BPC_10BPP		0xd9c9d9c9
#define DSC_PPS17_10BPC_15BPP		0xd968d948
#define DSC_PPS17_8BPC_8BPP		0xd165d945
#define DSC_PPS17_8BPC_12BPP		0xd125d925

#define DSC_PPS18_10BPC_10BPP		0xd1ebd1e9
#define DSC_PPS18_10BPC_15BPP		0xd18bd169
#define DSC_PPS18_8BPC_8BPP		0xd189d165
#define DSC_PPS18_8BPC_12BPP		0xd147d125

#define DSC_PPS19_10BPC_10BPP		0x0000d20f
#define DSC_PPS19_10BPC_15BPP		0x0000d1ac
#define DSC_PPS19_8BPC_8BPP		0x0000d1ac
#define DSC_PPS19_8BPC_12BPP		0x0000d16a
/**
 * @}
 */

/** @ingroup IP_group_dsc_internal_struct
 * @brief Data Struct used for DSC Driver information.
 */
struct mtk_dsc {
	/** store 2 dsc register base address */
	void __iomem *regs[DSC_MAX_WRAPPER_NUM];
	/** data use by gce driver */
	u32 subsys[DSC_MAX_WRAPPER_NUM];
	/** data use by gce driver */
	u32 subsys_offset[DSC_MAX_WRAPPER_NUM];
	/** dsc active hardware wrapper number, range is 1~2 */
	int wrapper_cnt;
	/** dsc active hardware number, range is 1~2 */
	int core_cnt;
	/** dsc configure struct */
	struct dsc_config config;
#ifdef CONFIG_COMMON_CLK_MT3612
	/** point to dsc0 clock struct */
	struct clk *clk;
	/** point to dsc1 clock struct */
	struct clk *clk1;
#endif
};

struct reg_info {
	void __iomem *regs;
	u32 subsys;
	u32 subsys_offset;
	struct cmdq_pkt *pkt;
};

void mtk_dsc_reg_write_mask(struct reg_info rif, u32 value, u32 offset,
			    u32 mask)
{
	if (rif.pkt) {
		cmdq_pkt_write_value(rif.pkt, rif.subsys,
				     rif.subsys_offset | offset, value, mask);
	} else {
		u32 reg;

		reg = readl(rif.regs + offset);
		reg &= ~mask;
		reg |= (value & mask);
		writel(reg, rif.regs + offset);
	}
}

void mtk_dsc_reg_write(struct reg_info rif, u32 value, u32 offset)
{
	if (rif.pkt)
		mtk_dsc_reg_write_mask(rif, value, offset, 0xffffffff);
	else
		writel(value, rif.regs + offset);
}

/** @ingroup IP_group_dsc_internal_function
 * @par Description
 *     DSC reigster multi write common function.
 * @param[in] mtkdsc: DSC driver data struct.
 * @param[in] value: write data value.
 * @param[in] offset: DSC register offset.
 * @param[in] handle: command queue handler.
 * @return none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsc_reg_multi_write(const struct mtk_dsc *mtkdsc,
				    u32 value, u32 offset,
				    struct cmdq_pkt **handle)
{
	int i, j;
	struct reg_info rif;

	for (i = 0; i < mtkdsc->wrapper_cnt; i++) {
		if (handle == NULL)
			rif.pkt = NULL;
		else
			rif.pkt = handle[i];

		rif.regs = mtkdsc->regs[i];
		rif.subsys_offset = mtkdsc->subsys_offset[i];
		rif.subsys = mtkdsc->subsys[i];
		for (j = 0; j < mtkdsc->core_cnt; j++)
			mtk_dsc_reg_write(rif, value, offset + j * CORE_OFFSET);
	}
}

/** @ingroup IP_group_dsc_internal_function
 * @par Description
 *     DSC reigster multi mask write common function.
 * @param[in] mtkdsc: DSC driver data struct.
 * @param[in] value: write data value.
 * @param[in] offset: DSC register offset.
 * @param[in] mask: write data mask value.
 * @param[in] handle: command queue handler.
 * @return none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dsc_reg_multi_mask_write(const struct mtk_dsc *mtkdsc,
					 u32 value, u32 offset,
					 u32 mask,
					 struct cmdq_pkt **handle)
{
	int i, j;
	struct reg_info rif;

	for (i = 0; i < mtkdsc->wrapper_cnt; i++) {
		if (handle == NULL)
			rif.pkt = NULL;
		else
			rif.pkt = handle[i];

		rif.regs = mtkdsc->regs[i];
		rif.subsys_offset = mtkdsc->subsys_offset[i];
		rif.subsys = mtkdsc->subsys[i];
		for (j = 0; j < mtkdsc->core_cnt; j++) {
			mtk_dsc_reg_write_mask(rif, value,
					       offset + j * CORE_OFFSET, mask);
		}
	}
}

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Reset DSC HW engine. \n
 *     The function can be called to reset DSC HW when HW is abnormal.
 * @param[in] dev: DSC device data struct.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is invalid, return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_reset(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 reg = 0, mask = 0;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);
	reg = DSC_SW_RESET_SET(1) | DSC_RELAY_SET(0) |
	      DSC_BYPASS_SET(1) | DSC_EN_SET(0);
	mask = DSC_SW_RESET_SET(1) | DSC_RELAY_SET(1) |
	       DSC_BYPASS_SET(1) | DSC_EN_SET(1);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg, DISP_DSC_CON, mask, handle);

	reg = DSC_SW_RESET_SET(0) | DSC_RELAY_SET(0) |
	      DSC_BYPASS_SET(1) | DSC_EN_SET(0);
	mask = DSC_SW_RESET_SET(1) | DSC_RELAY_SET(1) |
	       DSC_BYPASS_SET(1) | DSC_EN_SET(1);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg, DISP_DSC_CON, mask, handle);

	return 0;
}
EXPORT_SYMBOL(dsc_reset);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Bypass DSC engine.
 * @param[in] dev: DSC device data struct.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is invalid, return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_bypass(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 reg = 0, mask = 0;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);
	reg = DSC_RELAY_SET(0) | DSC_BYPASS_SET(1) | DSC_EN_SET(0);
	mask = DSC_RELAY_SET(1) | DSC_BYPASS_SET(1) | DSC_EN_SET(1);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg, DISP_DSC_CON, mask, handle);

#ifdef CONFIG_COMMON_CLK_MT3612
	clk_disable_unprepare(mtkdsc->clk);
	clk_disable_unprepare(mtkdsc->clk1);
#endif
	return 0;
}
EXPORT_SYMBOL(dsc_bypass);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Set DSC engine as relay mode.
 * @param[in] dev: DSC device data struct.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device. \n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is invalid, return -ENODEV. \n
 *     2. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_relay(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 reg = 0, mask = 0;
	int ret = 0;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);
#ifdef CONFIG_COMMON_CLK_MT3612
	ret = clk_prepare_enable(mtkdsc->clk);
	if (ret != 0)
		return ret;
	if (mtkdsc->wrapper_cnt == 2) {
		ret = clk_prepare_enable(mtkdsc->clk1);
		if (ret != 0) {
			clk_disable_unprepare(mtkdsc->clk);
			return ret;
		}
	}
#endif
	reg = DSC_RELAY_SET(1) | DSC_BYPASS_SET(0) | DSC_EN_SET(1);
	mask = DSC_RELAY_SET(1) | DSC_BYPASS_SET(1) | DSC_EN_SET(1);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg, DISP_DSC_CON, mask, handle);

	return ret;
}
EXPORT_SYMBOL(dsc_relay);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Enable DSC engine to start encoding.
 * @param[in] dev: DSC device data struct.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device. \n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is invalid, return -ENODEV. \n
 *     2. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_start(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 reg = 0, mask = 0;
	int ret = 0;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);
#ifdef CONFIG_COMMON_CLK_MT3612
	ret = clk_prepare_enable(mtkdsc->clk);
	if (ret != 0)
		return ret;
	if (mtkdsc->wrapper_cnt == 2) {
		ret = clk_prepare_enable(mtkdsc->clk1);
		if (ret != 0) {
			clk_disable_unprepare(mtkdsc->clk);
			return ret;
		}
	}
#endif
	reg = DSC_RELAY_SET(0) | DSC_BYPASS_SET(0) | DSC_EN_SET(1);
	mask = DSC_RELAY_SET(1) | DSC_BYPASS_SET(1) | DSC_EN_SET(1);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg, DISP_DSC_CON, mask, handle);

	return ret;
}
EXPORT_SYMBOL(dsc_start);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Set dispaly color of DSC mute mode.
 * @param[in] dev: DSC device data struct.
 * @param[in] color: write color value(30 bit resolution).
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device.
 * @par Boundary case and Limitation
 *     Input color should be limited to 30 bit.
 * @par Error case and Error handling
 *     Device struct is invalid, return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_mute_color(const struct device *dev,
		   u32 color, struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 reg = 0, mask = 0x3fffffff;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);
	color = color & mask;
	reg = DSC_MUTE_VALUE_SET(color);
	mask = DSC_MUTE_VALUE_SET(mask);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg,
				     DISP_DSC_MUTE_CON, mask, handle);

	return 0;
}
EXPORT_SYMBOL(dsc_mute_color);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Set DSC software mute mode. \n
 *     The function can be called to enable or disable SW mute function. \n
 *     After enable this function, DSC will output single color pattern. \n
 *     The color is setted by dsc_mute_color().
 * @param[in] dev: DSC device data struct.
 * @param[in] enable: Set 1 to enable software mute mode, \n
 *     set 0 to disable.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is invalid, return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_sw_mute(const struct device *dev,
		bool enable, struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 reg = 0, mask = 0;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);
	reg = SW_MUTE_CONTROL_SET(enable);
	mask = SW_MUTE_CONTROL_SET(1);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg,
				     DISP_DSC_MUTE_CON, mask, handle);

	return 0;
}
EXPORT_SYMBOL(dsc_sw_mute);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Set DSC hardware mute mode. \n
 *     The function can be called to enable or disable HW mute function. \n
 *     After enable this function, DSC will output single color pattern. \n
 *     The color is setted by dsc_mute_color().
 * @param[in] dev: DSC device data struct.
 * @param[in] enable: Set 1 to enable hardware mute mode, \n
 *     set 0 to disable.
 * @param[in] handle: command queue handler.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is invalid, return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_hw_mute(const struct device *dev,
		bool enable, struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 reg = 0, mask = 0;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);
	reg = HW_MUTE_ENABLE_SET(enable);
	mask = HW_MUTE_ENABLE_SET(1);

	mtk_dsc_reg_multi_mask_write(mtkdsc, reg,
				     DISP_DSC_MUTE_CON, mask, handle);

	return 0;
}
EXPORT_SYMBOL(dsc_hw_mute);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Configure DSC hardware initial value.
 * @param[in] dev: DSC device data struct.
 * @param[in] config: DSC configure struct.
 * @param[in] external_pps: pps parameter.
 * @param[in] handle: command queue handler.
 * @return
 *      0, No error, DSC HW init done. \n
 *     -1, DSC input core_cnt error. \n
 *     -2, DSC input format error. \n
 *     -3, DSC input pic_width or pic_height error. \n
 *     -4, DSC input slice_w or slice_h error. \n
 *     -5, DSC input ich_line_clear error. \n
 *     -6, DSC input slice_height error. \n
 *     -ENODEV, no such device.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is invalid, return -ENODEV. \n
 *     2. If DSC input parameters check error, it returns error code (-1 ~ -6).
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_hw_init(const struct device *dev,
		struct dsc_config config,
		const u32 *external_pps,
		struct cmdq_pkt **handle)
{
	struct mtk_dsc *mtkdsc;
	u32 tmp_reg = 0, pps0;
	unsigned int format, bit_per_channel, bit_per_pixel,
	    slice_height, slice_mode, pic_width, pic_height, slice_width,
	    slice_group_width, chunk_size, pad_num, init_delay_height,
	    pic_group_width, pic_height_ext, rbs_min, hrd_delay,
	    initial_xmit_delay, rc_model_size, initial_offset, dsc_version,
	    first_line_bpg_offset, slice_num, slice_bits, mux_word_size,
	    num_extra_mux_bits, initial_scale_value, final_offset,
	    final_scale, group_total, nfl_bpg_offset, slice_bgp_offset,
	    scale_increment_interval, scale_decrement_interval, ich_line_clear;

	if (!dev)
		return -ENODEV;

	mtkdsc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s starts\n", __func__);

	mtkdsc->config = config;

	tmp_reg = BYPASS_SHADOW_SET(1);
	mtk_dsc_reg_multi_mask_write(mtkdsc, tmp_reg,
				     DISP_DSC_SHADOW,
				     BYPASS_SHADOW_SET(1),
				     handle);

	if (mtkdsc->config.inout_sel == DSC_1_IN_1_OUT) {
		tmp_reg = DSC_IN_SRC_SEL_SET(0) | DSC_DUAL_INOUT_SET(0);
		mtkdsc->core_cnt = 1;
		slice_mode = 0;
	} else if (mtkdsc->config.inout_sel == DSC_2_IN_1_OUT) {
		tmp_reg = DSC_IN_SRC_SEL_SET(1) | DSC_DUAL_INOUT_SET(0);
		mtkdsc->core_cnt = 2;
		slice_mode = 1;
	} else if (mtkdsc->config.inout_sel == DSC_2_IN_2_OUT) {
		tmp_reg = DSC_IN_SRC_SEL_SET(0) | DSC_DUAL_INOUT_SET(1);
		mtkdsc->core_cnt = 2;
		slice_mode = 0;
		mtkdsc->config.disp_pic_w /= 2;
	} else {
		dev_err(dev, "DSC input core_cnt error!\n");
		return -1;
	}

	mtk_dsc_reg_multi_mask_write(mtkdsc, tmp_reg,
				     DISP_DSC_CON,
				     DSC_IN_SRC_SEL_SET(1) |
				     DSC_DUAL_INOUT_SET(1),
				     handle);

	format = mtkdsc->config.pic_format;
	pic_width = mtkdsc->config.disp_pic_w;
	pic_height = mtkdsc->config.disp_pic_h;
	slice_height = mtkdsc->config.slice_h;
	ich_line_clear = mtkdsc->config.ich_line_clear;

	if (format == MTK_DSI_FMT_COMPRESSION_30_15) {
		bit_per_channel = DSC_BPC_10_BIT;
		bit_per_pixel = DSC_BPP_15_BIT;
	} else if (format == MTK_DSI_FMT_COMPRESSION_30_10) {
		bit_per_channel = DSC_BPC_10_BIT;
		bit_per_pixel = DSC_BPP_10_BIT;
	} else if (format == MTK_DSI_FMT_COMPRESSION_24_12) {
		bit_per_channel = DSC_BPC_8_BIT;
		bit_per_pixel = DSC_BPP_12_BIT;
	} else if (format == MTK_DSI_FMT_COMPRESSION_24_8) {
		bit_per_channel = DSC_BPC_8_BIT;
		bit_per_pixel = DSC_BPP_8_BIT;
	} else if (format == MTK_DSI_FMT_RGB101010) {
		bit_per_channel = DSC_BPC_10_BIT;
		bit_per_pixel = DSC_NO_COMPRESS;
	} else if (format == MTK_DSI_FMT_RGB888) {
		bit_per_channel = DSC_BPC_8_BIT;
		bit_per_pixel = DSC_NO_COMPRESS;
	} else {
		dev_err(dev, "DSC input format error!\n");
		return -2;
	}

	if (mtkdsc->config.version == DSC_V1P1)
		dsc_version = 1;
	else
		dsc_version = 2;

	slice_width = pic_width / (slice_mode + 1);
	tmp_reg = slice_width * slice_height;

	dev_dbg(dev, "version = %d, pic_w = %d, pic_h = %d, bpc = %d, bpp = %d\n",
		dsc_version, pic_width, pic_height, bit_per_channel,
		bit_per_pixel);

	dev_dbg(dev, "slice_w = %d, slice_h = %d, slice_size = %d, core_cnt = %d\n",
		slice_width, slice_height, tmp_reg, mtkdsc->core_cnt);

	if (pic_width > 0 && pic_height > 0) {
	} else {
		dev_err(dev, "DSC input pic_width or pic_height error!\n");
		return -3;
	}

	if (tmp_reg < 15000 || tmp_reg > 3111000) {
		dev_err(dev, "DSC input slice_w or slice_h error!\n");
		return -4;
	}

	if (ich_line_clear > 2) {
		dev_err(dev, "DSC input ich_line_clear error!\n");
		return -5;
	}

	if (slice_height > pic_height) {
		dev_err(dev, "DSC input slice_h > pic_height error!\n");
		return -6;
	}

	first_line_bpg_offset =
	    (slice_height >= 8) ?
	    (12 + (MIN(34, (slice_height - 8)) * 9 / 100)) :
	    (2 * (slice_height - 1));

	first_line_bpg_offset = first_line_bpg_offset < 6 ?
				6 : first_line_bpg_offset;

	pic_group_width = (pic_width + 2) / 3;
	tmp_reg = PIC_GROUP_WIDTH_M1_SET(pic_group_width - 1) |
		  PIC_WIDTH_SET(pic_width);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PIC_W, handle);

	pic_height_ext = ((pic_height + (slice_height - 1)) / slice_height) *
			 slice_height;
	tmp_reg = PIC_HEIGHT_EXT_M1_SET(pic_height_ext - 1) |
		  PIC_HEIGHT_M1_SET(pic_height - 1);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PIC_H, handle);

	slice_group_width = (slice_width + 2) / 3;
	tmp_reg = SLICE_GROUP_WIDTH_M1_SET(slice_group_width - 1) |
		  SLICE_WIDTH_SET(slice_width);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_SLICE_W, handle);

	slice_num = (pic_height + (slice_height - 1)) / slice_height;
	tmp_reg = SLICE_WIDTH_MOD3_SET(slice_width % 3) |
		  SLICE_NUM_M1_SET(slice_num - 1) |
		  SLICE_HEIGHT_M1_SET(slice_height - 1);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_SLICE_H, handle);

	chunk_size = (slice_width * bit_per_pixel + 7) / 8;
	tmp_reg = CHUNK_SIZE_SET(chunk_size);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_CHUNK_SIZE, handle);

	tmp_reg = BUF_SIZE_SET(chunk_size * slice_height);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_BUF_SIZE, handle);

	initial_xmit_delay = bit_per_channel == DSC_BPC_10_BIT ?
			     (bit_per_pixel == DSC_BPP_10_BIT ? 410 : 273) :
			     (bit_per_pixel == DSC_BPP_8_BIT ? 512 : 341);
	initial_offset = bit_per_channel == DSC_BPC_10_BIT ?
			 (bit_per_pixel == DSC_BPP_10_BIT ? 5632 : 2048) :
			 (bit_per_pixel == DSC_BPP_8_BIT ? 6144 : 2048);

	init_delay_height = ((128 + (initial_xmit_delay + 2) / 3) * 3 +
			     pic_width - 1) / pic_width;
	init_delay_height = (init_delay_height > 15) ?
			    15 :
			    ((init_delay_height > 6) ? init_delay_height : 6);

	tmp_reg = OBUF_STR_IF_BUF_FULL_SET(1) |
		  INIT_DELAY_HEIGHT_SET(init_delay_height) |
		  RGB_SWAP_SET(0) | PIX_TYPE_SET(0) |
		  SLICE_MODE_SET(slice_mode);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_MODE, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  DSC_CFG_10BPC : DSC_CFG_8BPC;
	tmp_reg |= ICH_LINE_CLEAR_SET(ich_line_clear);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_CFG, handle);

	pad_num = (3 - (chunk_size * (slice_mode + 1)) % 3) % 3;
	pad_num = pad_num > 0 ? pad_num + 4 : pad_num;
	tmp_reg = PAD_NUM_SET(pad_num);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PAD, handle);

	dev_dbg(dev, "ich_line_clear = %d, init_delay_height = %d, first_line_bpg_offset = %d, pad_num = %d\n",
		ich_line_clear, init_delay_height, first_line_bpg_offset,
		pad_num);

	tmp_reg = OBUF_SW_SET(0);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_OBUF, handle);

	pps0 = BP_ENABLE_SET(1) | RCT_ON_SET(1) |
	       BIT_PER_PIXEL_SET(bit_per_pixel << 4) |
	       BIT_PER_CHANNEL_SET(bit_per_channel) |
	       UP_LINE_BUF_DEPTH_SET(bit_per_channel + 1);
	mtk_dsc_reg_multi_write(mtkdsc, pps0, DISP_DSC_PPS0, handle);

	rc_model_size = 8192;
	rbs_min = rc_model_size - initial_offset +
		  (initial_xmit_delay * bit_per_pixel) +
		  (slice_group_width * first_line_bpg_offset);
	hrd_delay = (rbs_min + (bit_per_pixel - 1)) / bit_per_pixel;

	if (external_pps != NULL) {
		int i;
		static uint32_t regs[] = {
			DISP_DSC_PPS1, DISP_DSC_PPS2, DISP_DSC_PPS3,
			DISP_DSC_PPS4, DISP_DSC_PPS5, DISP_DSC_PPS6,
			DISP_DSC_PPS7, DISP_DSC_PPS8, DISP_DSC_PPS9,
			DISP_DSC_PPS10, DISP_DSC_PPS11, DISP_DSC_PPS12,
			DISP_DSC_PPS13, DISP_DSC_PPS14, DISP_DSC_PPS15,
			DISP_DSC_PPS16,	DISP_DSC_PPS17, DISP_DSC_PPS18,
			DISP_DSC_PPS19,
		};
		pr_info("use external PPS.");
		for (i = 0; i < ARRAY_SIZE(regs); i++) {
			mtk_dsc_reg_multi_write(mtkdsc, external_pps[i],
						regs[i], handle);
		}

		tmp_reg = FORCE_COMMIT_SET(1) |
			  BYPASS_SHADOW_SET(1) |
			  READ_WORKING_SET(1) |
			  DSC_VERSION_MINOR_SET(dsc_version) |
			  RELAY_FIFO_MODE_OFF_SET(0);

		mtk_dsc_reg_multi_mask_write(mtkdsc, tmp_reg,
					     DISP_DSC_SHADOW,
					     FORCE_COMMIT_SET(1) |
					     BYPASS_SHADOW_SET(1) |
					     READ_WORKING_SET(1) |
					     DSC_VERSION_MINOR_SET(0xf) |
					     RELAY_FIFO_MODE_OFF_SET(1),
					     handle);

		tmp_reg = readl(mtkdsc->regs[0] + DISP_DSC_SHADOW);
		pps0 = readl(mtkdsc->regs[0] + DISP_DSC_CON);
		pr_info("DISP_DSC_SHADOW = 0x%X, DISP_DSC_CON = 0x%X\n",
			tmp_reg, pps0);

		return 0;
	}

	tmp_reg = INITIAL_DEC_DELAY_SET(hrd_delay - initial_xmit_delay) |
		  INITIAL_XMIT_DELAY_SET(initial_xmit_delay);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS1, handle);

	mux_word_size = 48;
	slice_bits = 8 * chunk_size * slice_height;

	if (pps0 & 0x00040000) {
		num_extra_mux_bits =
		    3 * (mux_word_size + 4 * (bit_per_channel + 1) - 2);
/*
 *	} else {
 *		num_extra_mux_bits =
 *		    3 * mux_word_size + 4 * (bit_per_channel + 1) +
 *		    8 * bit_per_channel - 2;
 */
	}

	tmp_reg = num_extra_mux_bits;
	while ((num_extra_mux_bits > 0) &&
	       ((slice_bits - num_extra_mux_bits) % mux_word_size != 0)) {
		num_extra_mux_bits--;
	}
	dev_info(dev, "RCT_ON = 0x%X, num_extra_mux_bits = %d->%d, slice_bits = %d\n",
		 (pps0 & 0x00040000), tmp_reg, num_extra_mux_bits, slice_bits);

	final_offset = rc_model_size + num_extra_mux_bits -
		       ((initial_xmit_delay * (bit_per_pixel << 4) + 8) >> 4);
	final_scale = 8 * rc_model_size / (rc_model_size - final_offset);

	initial_scale_value = 8 * rc_model_size /
			      (rc_model_size - initial_offset);
	initial_scale_value = initial_scale_value > (slice_group_width + 8) ?
			      (initial_scale_value + 8) : initial_scale_value;

	group_total = slice_group_width * slice_height;

	nfl_bpg_offset = ((first_line_bpg_offset << 11) +
			  (slice_height - 1) - 1) / (slice_height - 1);

	slice_bgp_offset =
	    (2048 * (rc_model_size - initial_offset + num_extra_mux_bits) +
	     group_total - 1) / group_total;

	scale_increment_interval =
	    (final_scale > 9) ?
	    2048 * final_offset /
	    ((final_scale - 9) * (nfl_bpg_offset + slice_bgp_offset)) : 0;

	scale_increment_interval = scale_increment_interval & 0xffff;

	scale_decrement_interval =
	    (initial_scale_value > 8) ?
	    slice_group_width / (initial_scale_value - 8) : 4095;

	tmp_reg = SCALE_INCREMENT_INTERVAL_SET(scale_increment_interval) |
		  INITIAL_SCALE_VALUE_SET(initial_scale_value);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS2, handle);

	tmp_reg = FIRST_LINE_BPG_OFFSET_SET(first_line_bpg_offset) |
		  SCALE_DECREMENT_INTERVAL_SET(scale_decrement_interval);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS3, handle);

	tmp_reg = SLICE_BPG_OFFSET_SET(slice_bgp_offset) |
		  NFL_BPG_OFFSET_SET(nfl_bpg_offset);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS4, handle);

	tmp_reg = FINAL_OFFSET_SET(final_offset) |
		  INITIAL_OFFSET_SET(initial_offset);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS5, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS6_10BPC_10BPP : DSC_PPS6_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS6_8BPC_8BPP : DSC_PPS6_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS6, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS7_10BPC_10BPP : DSC_PPS7_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS7_8BPC_8BPP : DSC_PPS7_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS7, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS8_10BPC_10BPP : DSC_PPS8_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS8_8BPC_8BPP : DSC_PPS8_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS8, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS9_10BPC_10BPP : DSC_PPS9_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS9_8BPC_8BPP : DSC_PPS9_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS9, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS10_10BPC_10BPP : DSC_PPS10_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS10_8BPC_8BPP : DSC_PPS10_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS10, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS11_10BPC_10BPP : DSC_PPS11_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS11_8BPC_8BPP : DSC_PPS11_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS11, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS12_10BPC_10BPP : DSC_PPS12_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS12_8BPC_8BPP : DSC_PPS12_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS12, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS13_10BPC_10BPP : DSC_PPS13_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS13_8BPC_8BPP : DSC_PPS13_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS13, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS14_10BPC_10BPP : DSC_PPS14_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS14_8BPC_8BPP : DSC_PPS14_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS14, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS15_10BPC_10BPP : DSC_PPS15_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS15_8BPC_8BPP : DSC_PPS15_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS15, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS16_10BPC_10BPP : DSC_PPS16_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS16_8BPC_8BPP : DSC_PPS16_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS16, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS17_10BPC_10BPP : DSC_PPS17_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS17_8BPC_8BPP : DSC_PPS17_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS17, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS18_10BPC_10BPP : DSC_PPS18_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS18_8BPC_8BPP : DSC_PPS18_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS18, handle);

	tmp_reg = bit_per_channel == DSC_BPC_10_BIT ?
		  (bit_per_pixel == DSC_BPP_10_BIT ?
		   DSC_PPS19_10BPC_10BPP : DSC_PPS19_10BPC_15BPP) :
		  (bit_per_pixel == DSC_BPP_8_BIT ?
		   DSC_PPS19_8BPC_8BPP : DSC_PPS19_8BPC_12BPP);
	mtk_dsc_reg_multi_write(mtkdsc, tmp_reg, DISP_DSC_PPS19, handle);

	tmp_reg = FORCE_COMMIT_SET(1) |
		  BYPASS_SHADOW_SET(1) |
		  READ_WORKING_SET(1) |
		  DSC_VERSION_MINOR_SET(dsc_version) |
		  RELAY_FIFO_MODE_OFF_SET(0);

	mtk_dsc_reg_multi_mask_write(mtkdsc, tmp_reg,
				     DISP_DSC_SHADOW,
				     FORCE_COMMIT_SET(1) |
				     BYPASS_SHADOW_SET(1) |
				     READ_WORKING_SET(1) |
				     DSC_VERSION_MINOR_SET(0xf) |
				     RELAY_FIFO_MODE_OFF_SET(1),
				     handle);

	tmp_reg = readl(mtkdsc->regs[0] + DISP_DSC_SHADOW);
	pps0 = readl(mtkdsc->regs[0] + DISP_DSC_CON);
	dev_info(dev, "DISP_DSC_SHADOW = 0x%X, DISP_DSC_CON = 0x%X\n",
		 tmp_reg, pps0);

	return 0;
}
EXPORT_SYMBOL(dsc_hw_init);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Set DSC power off. \n
 *     The function can be called to set DSC power off.
 * @param[in] dev: DSC device data struct.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     Device struct is invalid, return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_power_off(struct device *dev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	if (!dev)
		return -ENODEV;

	pm_runtime_put(dev);
#endif
	return 0;
}
EXPORT_SYMBOL(dsc_power_off);

/** @ingroup IP_group_dsc_external_function
 * @par Description
 *     Set DSC power on. \n
 *     The function can be called to set DSC power on.
 * @param[in] dev: DSC device data struct.
 * @return
 *     0, success. \n
 *     -ENODEV, no such device. \n
 *     error code from pm_runtime_get_sync().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Device struct is invalid, return -ENODEV. \n
 *     2. If pm_runtime_get_sync() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dsc_power_on(struct device *dev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	int ret = 0;

	if (!dev)
		return -ENODEV;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to enable power domain: %d\n", ret);
		return ret;
	}
#endif
	return 0;
}
EXPORT_SYMBOL(dsc_power_on);

static int mtk_dsc_probe(struct platform_device *pdev)
{
	struct mtk_dsc *dsc;
	struct device *dev = &pdev->dev;
	struct resource *regs;
	int i;

	dsc = devm_kzalloc(dev, sizeof(*dsc), GFP_KERNEL);
	if (!dsc)
		return -ENOMEM;

	of_property_read_u32(dev->of_node, "hw-number", &dsc->wrapper_cnt);
	if (dsc->wrapper_cnt <= 0 ||
	    dsc->wrapper_cnt > DSC_MAX_WRAPPER_NUM) {
		dev_err(dev, "Invalid DCS wrapper cnt = %d\n",
			dsc->wrapper_cnt);
		return -EINVAL;
	}
	/* Clock Init */
#ifdef CONFIG_COMMON_CLK_MT3612
	dsc->clk = devm_clk_get(dev, "dsc-clk");
	if (IS_ERR(dsc->clk)) {
		dev_err(dev, "cannot get dsc clock\n");
		return PTR_ERR(dsc->clk);
	}

	dsc->clk1 = devm_clk_get(dev, "dsc-clk1");
	if (IS_ERR(dsc->clk1)) {
		dev_err(dev, "cannot get dsc1 clock\n");
		return PTR_ERR(dsc->clk1);
	}

	pm_runtime_enable(dev);
#endif
/*
 * dsc->regs[0] = 0x14600000 / 0x1401E000
 * dsc->regs[1] = 0x14601000 / 0x1401F000
 */
	for (i = 0; i < dsc->wrapper_cnt; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		dsc->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(dsc->regs[i])) {
			dev_err(dev, "Failed to map disp_dsc registers\n");
			return PTR_ERR(dsc->regs[i]);
		}
		dsc->subsys_offset[i] = regs->start & 0xFFFF;
	}

	of_property_read_u32_array(dev->of_node, "gce-subsys",
				   dsc->subsys, dsc->wrapper_cnt);
	dev_dbg(dev,
		"%s probe wrapper_cnt=%d, subsys[0]=0x%08X, subsys_offset[0]=0x%08X\n",
		dev->of_node->full_name,
		dsc->wrapper_cnt, dsc->subsys[0], dsc->subsys_offset[0]);

	platform_set_drvdata(pdev, dsc);

	dev_dbg(dev, "mtk dsc driver installed.\n");

	return 0;
}

static int mtk_dsc_remove(struct platform_device *pdev)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);
#endif
	return 0;
}

static const struct of_device_id mtk_dsc_of_match[] = {
	{.compatible = "mediatek,mt3612-dsc"},
	{},
};

static struct platform_driver mtk_dsc_driver = {
	.probe = mtk_dsc_probe,
	.remove = mtk_dsc_remove,
	.driver = {
		   .name = "mtk-dsc",
		   .owner = THIS_MODULE,
		   .of_match_table = mtk_dsc_of_match,
		   },
};

static int mtk_dsc_init(void)
{
	int ret = 0;

	pr_debug("mtk dsc drv init!\n");

	ret = platform_driver_register(&mtk_dsc_driver);
	if (ret) {
		pr_err(" platform_driver_register failed!\n");
		return ret;
	}
	pr_debug("mtk dsc drv init ok!\n");
	return ret;
}

static void mtk_dsc_exit(void)
{
	pr_debug("mtk dsc drv exit!\n");
	platform_driver_unregister(&mtk_dsc_driver);
}

module_init(mtk_dsc_init);
module_exit(mtk_dsc_exit);

MODULE_LICENSE("GPL v2");
