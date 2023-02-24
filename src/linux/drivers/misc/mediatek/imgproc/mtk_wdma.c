/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	leon liang <leon.liang@mediatek.com>
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
 * @defgroup IP_group_wdma WDMA
 *     Wdma is the driver of WDMA which is dedicated for DMA writing\n
 *     function. The interface includes parameter setting and control\n
 *     function. Parameter setting includes setting of region, output\n
 *     buffer, and callback function. Control function includes power on\n
 *     and off, start, and stop.
 *
 *     @{
 *         @defgroup IP_group_wdma_external EXTERNAL
 *             The external API document for wdma.\n
 *
 *             @{
 *                 @defgroup IP_group_wdma_external_function 1.function
 *                     Exported function of wdma
 *                 @defgroup IP_group_wdma_external_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_wdma_external_typedef 3.typedef
 *                     Exported type definition of wdma
 *                 @defgroup IP_group_wdma_external_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_wdma_external_def 5.define
 *                     none.
 *             @}
 *
 *         @defgroup IP_group_wdma_internal INTERNAL
 *             The internal API document for wdma.\n
 *
 *             @{
 *                 @defgroup IP_group_wdma_internal_function 1.function
 *                     none.
 *                 @defgroup IP_group_wdma_internal_struct 2.structure
 *                     none.
 *                 @defgroup IP_group_wdma_internal_typedef 3.typedef
 *                     none.
 *                 @defgroup IP_group_wdma_internal_enum 4.enumeration
 *                     none.
 *                 @defgroup IP_group_wdma_internal_def 5.define
 *                     none.
 *             @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_drv_def.h>
#include <soc/mediatek/smi.h>
#include <uapi/drm/drm_fourcc.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include "mtk_wdma_reg.h"

#define MODULE_MAX_NUM		1

#define WDMA_OF_RGB565			0
#define WDMA_OF_RGB888			1
#define WDMA_OF_BGRA8888		2
#define WDMA_OF_ABGR8888		3
#define WDMA_OF_UYVY			4
#define WDMA_OF_YUY2			5
#define WDMA_OF_Y_ONLY			7
#define WDMA_OF_I420			8
#define WDMA_OF_BGRA1010102		11
#define WDMA_OF_NV12			12
#define WDMA_OF_10B_Y_ONLY		13
#define WDMA_OF_10B_NV12		14
#define WDMA_OF_ARGB2101010		15

#define INT_MTX_SEL_RGB_TO_JPEG		0
#define INT_MTX_SEL_RGB_TO_BT601	2
#define INT_MTX_SEL_RGB_TO_BT709	3
#define INT_MTX_SEL_JPEG_TO_RGB		4
#define INT_MTX_SEL_BT601_TO_RGB	6
#define INT_MTX_SEL_BT709_TO_RGB	7
#define INT_MTX_SEL_JPEG_TO_BT601	8
#define INT_MTX_SEL_JPEG_TO_BT709	9
#define INT_MTX_SEL_BT601_TO_JPEG	10
#define INT_MTX_SEL_BT709_TO_JPEG	11
#define INT_MTX_SEL_BT709_TO_BT601	12
#define INT_MTX_SEL_BT601_TO_BT709	13
#define INT_MTX_SEL_RGB_TO_BT709_FULL	14
#define INT_MTX_SEL_BT709_FULL_TO_RGB	15

#define REG_ENABLE			1
#define REG_DISABLE			0

#ifdef CONFIG_MTK_DEBUGFS
struct dentry *mtk_wdma_debugfs_root;
#endif

#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))
#define GET_BIT(var, pos) ((var>>pos) & 1)
#define GET_VAL(reg, mask) ((reg & mask) >> __builtin_ctz(mask))
#define SET_VAL(value, mask) (value << __builtin_ctz(mask))

struct reg_trg {
	void __iomem *regs;
	u32 reg_base;
	struct cmdq_pkt *pkt;
	int subsys;
};

static void reg_write_mask(struct reg_trg *trg, u32 offset, u32 value,
		u32 mask)
{
	if (trg->pkt) {
		cmdq_pkt_write_value(trg->pkt, trg->subsys,
				(trg->reg_base & 0xffff) | offset,
				value, mask);
	} else {
		u32 reg;

		reg = readl(trg->regs + offset);
		reg &= ~mask;
		reg |= value;
		writel(reg, trg->regs + offset);
	}
}

static void reg_set_val(struct reg_trg *trg, u32 offset, u32 value, u32 mask)
{
	reg_write_mask(trg, offset, SET_VAL(value, mask), mask);
}

static void reg_get_val(struct reg_trg *trg, u32 offset, u32 *pValue, u32 mask)
{
	u32 reg;

	reg = readl(trg->regs + offset);
	*pValue = GET_VAL(reg, mask);
}

static void mtk_wdma_enable(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_EN, REG_ENABLE, ENABLE);

#if 1 /* priority configuration */
	reg_set_val(trg, WDMA_BUF_CON5,  0x20, WDMA_PRE_ULTRA_LOW_Y);
	reg_set_val(trg, WDMA_BUF_CON5,  0x30, WDMA_ULTRA_LOW_Y);
	reg_set_val(trg, WDMA_BUF_CON6,  0x30, WDMA_PRE_ULTRA_HIGH_Y);
	reg_set_val(trg, WDMA_BUF_CON6,  0x40, WDMA_ULTRA_HIGH_Y);
	reg_set_val(trg, WDMA_BUF_CON7,  0x20, WDMA_PRE_ULTRA_LOW_U);
	reg_set_val(trg, WDMA_BUF_CON7,  0x30, WDMA_ULTRA_LOW_U);
	reg_set_val(trg, WDMA_BUF_CON8,  0x30, WDMA_PRE_ULTRA_HIGH_U);
	reg_set_val(trg, WDMA_BUF_CON8,  0x40, WDMA_ULTRA_HIGH_U);
	reg_set_val(trg, WDMA_BUF_CON9,  0x20, WDMA_PRE_ULTRA_LOW_V);
	reg_set_val(trg, WDMA_BUF_CON9,  0x30, WDMA_ULTRA_LOW_V);
	reg_set_val(trg, WDMA_BUF_CON10, 0x30, WDMA_PRE_ULTRA_HIGH_V);
	reg_set_val(trg, WDMA_BUF_CON10, 0x40, WDMA_ULTRA_HIGH_V);

	reg_set_val(trg, WDMA_BUF_CON1, ENABLE, FRAME_END_ULTRA);
	reg_set_val(trg, WDMA_BUF_CON1, ENABLE, PRE_ULTRA_ENABLE);
	reg_set_val(trg, WDMA_BUF_CON1, ENABLE, ULTRA_ENABLE);
#endif
}

static void mtk_wdma_disable(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_EN, REG_DISABLE, ENABLE);
}

static void mtk_wdma_fbdc_cr_ch0123_val0(struct reg_trg *trg, u32 value)
{
	reg_set_val(trg, WDMA_FBC_FBDC_CR_CH0123_VAL0, value,
			FBC_FBDC_CR_CH0123_VAL0);
}

static void mtk_wdma_fbdc_cr_ch0123_val1(struct reg_trg *trg, u32 value)
{
	reg_set_val(trg, WDMA_FBC_FBDC_CR_CH0123_VAL1, value,
			FBC_FBDC_CR_CH0123_VAL1);
}

static void mtk_wdma_pvric_on(struct reg_trg *trg)
{
	mtk_wdma_fbdc_cr_ch0123_val0(trg, 0x00000000);
	mtk_wdma_fbdc_cr_ch0123_val1(trg, 0x00000000);

	reg_set_val(trg, WDMA_EN, REG_ENABLE, PVRIC_EN);
}

static void mtk_wdma_pvric_off(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_EN, REG_DISABLE, PVRIC_EN);
}

static void mtk_wdma_bypass_shadow_on(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_EN, REG_ENABLE, BYPASS_SHADOW);
}

static void mtk_wdma_bypass_shadow_off(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_EN, REG_DISABLE, BYPASS_SHADOW);
}

static void mtk_wdma_crop(struct reg_trg *trg, u32 in_w, u32 in_h,
		u32 crop_x, u32 crop_y, u32 out_w, u32 out_h)
{
	reg_set_val(trg, WDMA_SRC_SIZE, in_w, WIDTH);
	reg_set_val(trg, WDMA_SRC_SIZE, in_h, HEIGHT);
	reg_set_val(trg, WDMA_CLIP_SIZE, out_w, WIDTH);
	reg_set_val(trg, WDMA_CLIP_SIZE, out_h, HEIGHT);
	reg_set_val(trg, WDMA_CLIP_COORD, crop_x, X_COORD);
	reg_set_val(trg, WDMA_CLIP_COORD, crop_y, Y_COORD);
}

static void mtk_wdma_out_addr_0(struct reg_trg *trg, dma_addr_t addr)
{
	reg_set_val(trg, WDMA_DST_ADDR0, (u32)addr, ADDRESS0);
}

static void mtk_wdma_out_addr_1(struct reg_trg *trg, dma_addr_t addr)
{
	reg_set_val(trg, WDMA_DST_ADDR1, (u32)addr, ADDRESS1);
}

static void mtk_wdma_out_addr_2(struct reg_trg *trg, dma_addr_t addr)
{
	reg_set_val(trg, WDMA_DST_ADDR2, (u32)addr, ADDRESS2);
}

static void mtk_wdma_out_pitch_0(struct reg_trg *trg, u32 pitch)
{
	reg_set_val(trg, WDMA_DST_W_IN_BYTE, pitch, DST_W_IN_BYTE);
}

static void mtk_wdma_target_line(struct reg_trg *trg, u32 line)
{
	reg_set_val(trg, WDMA_TARGET_LINE, line, TARGET_LINE);
}

static int mtk_wdma_format_to_bpp(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_AYUV2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB8888:
		return 32;
	case DRM_FORMAT_NV12:
		return 8;
	case DRM_FORMAT_NV12_10BIT:
		return 10;
	case DRM_FORMAT_C8:
	case DRM_FORMAT_R8:
		return 8;
	case DRM_FORMAT_R10:
		return 10;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;
	case DRM_FORMAT_YUYV:
		return 16;
	}

	return -EINVAL;
}

static void mtk_wdma_out_pitch_1_2(struct reg_trg *trg, u32 pitch)
{
	reg_set_val(trg, WDMA_DST_UV_PITCH, pitch, UV_DST_W_IN_BYTE);
}

static void mtk_wdma_out_format(struct reg_trg *trg, u32 fmt, bool pvric_en)
{
	u32 wdma_fmt;
	u32 wdma_swap = 0;
	u32 yuv444_out = 0;

	switch (fmt) {
	default:
	case DRM_FORMAT_ABGR2101010:
		wdma_swap = 1;
		/* fall through */
	case DRM_FORMAT_ARGB2101010:
		wdma_fmt = WDMA_OF_BGRA1010102;
		break;
	case DRM_FORMAT_AYUV2101010:
		/* for special YUV, use proper foortprint and need swap */
		wdma_fmt = WDMA_OF_BGRA1010102;
		wdma_swap = 1;
		if (pvric_en)
			yuv444_out = 1;
		break;
	case DRM_FORMAT_ABGR8888:
		wdma_swap = 1;
		/* fall through */
	case DRM_FORMAT_ARGB8888:
		wdma_fmt = WDMA_OF_BGRA8888;
		break;
	case DRM_FORMAT_NV12:
		wdma_fmt = WDMA_OF_NV12;
		break;
	case DRM_FORMAT_NV12_10BIT:
		wdma_fmt = WDMA_OF_10B_NV12;
		break;
	case DRM_FORMAT_C8:
		wdma_fmt = WDMA_OF_Y_ONLY;
		break;
	case DRM_FORMAT_R8:
		wdma_fmt = WDMA_OF_Y_ONLY;
		break;
	case DRM_FORMAT_R10:
		wdma_fmt = WDMA_OF_10B_Y_ONLY;
		break;
	case DRM_FORMAT_BGR888:
		wdma_swap = 1;
		/* fall through */
	case DRM_FORMAT_RGB888:
		wdma_fmt = WDMA_OF_RGB888;
		break;
	case DRM_FORMAT_YUYV:
		wdma_fmt = WDMA_OF_YUY2;
		break;
	case DRM_FORMAT_RGBA1010102:
		wdma_fmt = WDMA_OF_ARGB2101010;
		wdma_swap = 1;
		break;
	}
	reg_set_val(trg, WDMA_CFG, wdma_fmt, OUT_FORMAT);
	reg_set_val(trg, WDMA_CFG, wdma_swap, SWAP);
	reg_set_val(trg, WDMA_CFG, yuv444_out, YUV444_OUT);
}

static void mtk_wdma_color_transform_enable(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_CFG, REG_ENABLE, CT_EN);
}

static void mtk_wdma_color_transform_disable(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_CFG, REG_DISABLE, CT_EN);
}

static int mtk_wdma_int_color_transform_matrix(struct reg_trg *trg,
		enum mtk_wdma_internal_color_trans_matrix int_matrix)
{
	u32 ct_type;

	switch (int_matrix) {
	case MTK_WDMA_INT_COLOR_TRANS_RGB_TO_JPEG:
		ct_type = INT_MTX_SEL_RGB_TO_JPEG;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_RGB_TO_BT601:
		ct_type = INT_MTX_SEL_RGB_TO_BT601;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_RGB_TO_BT709:
		ct_type = INT_MTX_SEL_RGB_TO_BT709;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_JPEG_TO_RGB:
		ct_type = INT_MTX_SEL_JPEG_TO_RGB;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_BT601_TO_RGB:
		ct_type = INT_MTX_SEL_BT601_TO_RGB;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_BT709_TO_RGB:
		ct_type = INT_MTX_SEL_BT709_TO_RGB;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_JPEG_TO_BT601:
		ct_type = INT_MTX_SEL_JPEG_TO_BT601;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_JPEG_TO_BT709:
		ct_type = INT_MTX_SEL_JPEG_TO_BT709;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_BT601_TO_JPEG:
		ct_type = INT_MTX_SEL_BT601_TO_JPEG;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_BT709_TO_JPEG:
		ct_type = INT_MTX_SEL_BT709_TO_JPEG;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_BT709_TO_BT601:
		ct_type = INT_MTX_SEL_BT709_TO_BT601;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_BT601_TO_BT709:
		ct_type = INT_MTX_SEL_BT601_TO_BT709;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_RGB_TO_BT709_FULL:
		ct_type = INT_MTX_SEL_RGB_TO_BT709_FULL;
		break;
	case MTK_WDMA_INT_COLOR_TRANS_BT709_FULL_TO_RGB:
		ct_type = INT_MTX_SEL_BT709_FULL_TO_RGB;
		break;
	default:
		return -EINVAL;
	}

	reg_set_val(trg, WDMA_CFG, ct_type, INT_MTX_SEL);
	reg_set_val(trg, WDMA_CFG, 0, EXT_MTX_EN);

	return 0;
}

static void mtk_wdma_ext_color_transform_matrix(struct reg_trg *trg,
		u32 c00, u32 c01, u32 c02,
		u32 c10, u32 c11, u32 c12,
		u32 c20, u32 c21, u32 c22,
		u32 pre_add_0, u32 pre_add_1, u32 pre_add_2,
		u32 post_add_0, u32 post_add_1, u32 post_add_2)
{
	reg_set_val(trg, WDMA_C00, c00, C00);
	reg_set_val(trg, WDMA_C00, c01, C01);
	reg_set_val(trg, WDMA_C02, c02, C02);
	reg_set_val(trg, WDMA_C10, c10, C10);
	reg_set_val(trg, WDMA_C10, c11, C11);
	reg_set_val(trg, WDMA_C12, c12, C12);
	reg_set_val(trg, WDMA_C20, c20, C20);
	reg_set_val(trg, WDMA_C20, c21, C21);
	reg_set_val(trg, WDMA_C22, c22, C22);
	reg_set_val(trg, WDMA_PRE_ADD0, pre_add_0, PRE_ADD_0);
	reg_set_val(trg, WDMA_PRE_ADD0, pre_add_1, PRE_ADD_1);
	reg_set_val(trg, WDMA_PRE_ADD2, pre_add_2, PRE_ADD_2);
	reg_set_val(trg, WDMA_POST_ADD0, post_add_0, POST_ADD_0);
	reg_set_val(trg, WDMA_POST_ADD0, post_add_1, POST_ADD_1);
	reg_set_val(trg, WDMA_POST_ADD2, post_add_2, POST_ADD_2);

	reg_set_val(trg, WDMA_CFG, REG_ENABLE, EXT_MTX_EN);
}

static void mtk_wdma_smi_byte_mask_enable(struct reg_trg *trg)
{
	reg_set_val(trg, WDMA_SMI_CON1, REG_DISABLE, MASK_DIS);
}

enum MTK_WDMA_TYPE {
	/** 1: mt3612 disp0wdma support pvric */
	MTK_3612_DISP_WDMA_0_PVRIC,
	/** 2: mt3612 disp1wdma */
	MTK_3612_DISP_WDMA_1,
	/** 3: mt3612 disp2wdma */
	MTK_3612_DISP_WDMA_2,
	/** 4: mt3612 disp3wdma */
	MTK_3612_DISP_WDMA_3,
	/** 5: mt3612 wdma0 */
	MTK_3612_WDMA_0,
	/** 6: mt3612 wdma1 */
	MTK_3612_WDMA_1,
	/** 7: mt3612 wdma2 */
	MTK_3612_WDMA_2,
	/** 8: mt3612 wdma gaze */
	MTK_3612_WDMA_GAZE,
};

struct mtk_wdma {
	struct device *dev;
	struct clk *clk[MODULE_MAX_NUM];
	void __iomem *regs[MODULE_MAX_NUM];
	u32 irq[MODULE_MAX_NUM];
	spinlock_t spinlock_irq;
	mtk_mmsys_cb cb_func;
	u32 cb_status;
	void *cb_priv_data;
	u32 reg_base[MODULE_MAX_NUM];
	u32 subsys[MODULE_MAX_NUM];
	struct device *dev_larb[MODULE_MAX_NUM];
	struct device *dev_s_larb[MODULE_MAX_NUM];
	u32 out_w;
	u32 hw_nr;
	u32 eye_nr;
	u32 eye_hw_nr;
	u32 active_eye;
	dma_addr_t addr[MODULE_MAX_NUM][MTK_WDMA_OUT_BUF_MAX];
	u32 addr_offset[MODULE_MAX_NUM][MTK_WDMA_OUT_BUF_MAX];
	u32 out_mode;
	u32 irq_status[MODULE_MAX_NUM];
	u64 wdma_type;
	bool pvric_en;
	bool bypass_shadow_disable;
};

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Reset the functon of WDMA. You can reset in stop state.\n
 *     The state won't change will keep in stop state.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 *     4. -EPERM, Operation not permitted.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_reset(const struct device *dev)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 rst_status;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = NULL;
			trg.subsys = wdma->subsys[hw_id];

			reg_set_val(&trg, WDMA_RST, REG_ENABLE, SOFT_RESET);
			if (readl_poll_timeout_atomic(
				wdma->regs[hw_id] + WDMA_RST, rst_status,
				((rst_status & SOFT_RESET) == 0), 2, 10)) {
				pr_err("mtk_wdma_reset timeout\n");
				return -EPERM;
			}
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_reset);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     get hardware state.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[out]
 *     state: h/w state.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter or not support.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_get_hardware_state(const struct device *dev, u32 *state)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	if (!state)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = NULL;
			trg.subsys = wdma->subsys[hw_id];

			reg_get_val(&trg,
				WDMA_FLOW_CTRL_DBG, state, WDMA_STATE);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_get_hardware_state);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Enable PVRIC. WDMA driver will know whether this WDMA support\n
 *     PVRIC or NOT. If not, this function will return err. Preventing\n
 *     incorrect operation from user.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     enable: enable pvric or not
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter or not support.\n
 *     3. -ENODEV, No such device.\n
 *     4. -EPERM, operation not permitted.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_pvric_enable(const struct device *dev, bool enable)
{
	struct mtk_wdma *wdma;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->wdma_type != MTK_3612_DISP_WDMA_0_PVRIC)
		return -EPERM;

	wdma->pvric_en = enable;

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_pvric_enable);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     get pvric current frame compression byte count
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[out]
 *     count: pvric frame compression byte count.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter or not support.\n
 *     3. -ENODEV, No such device.\n
 *     4. -EPERM, operation not permitted.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_get_pvric_frame_comp_cur(const struct device *dev, u32 *count)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	if (!count)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->wdma_type != MTK_3612_DISP_WDMA_0_PVRIC)
		return -EPERM;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = NULL;
			trg.subsys = wdma->subsys[hw_id];

			reg_get_val(&trg,
				WDMA_PVRIC_FRAME_COMP_MON_CUR,
				count, PVRIC_FRAME_COMP_MON_CUR);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_get_pvric_frame_comp_cur);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     get pvric previous frame compression byte count
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[out]
 *     count: pvric frame compression byte count.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter or not support.\n
 *     3. -ENODEV, No such device.\n
 *     4. -EPERM, operation not permitted.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_get_pvric_frame_comp_pre(const struct device *dev, u32 *count)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	if (!count)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->wdma_type != MTK_3612_DISP_WDMA_0_PVRIC)
		return -EPERM;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = NULL;
			trg.subsys = wdma->subsys[hw_id];

			reg_get_val(&trg,
				WDMA_PVRIC_FRAME_COMP_MON_PRE, count,
				PVRIC_FRAME_COMP_MON_PRE);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_get_pvric_frame_comp_pre);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Disable BYPASS SHADOW.\n
 *     Bypass shadow is default ON.\n
 *     If you want to operate those shadow registers, you need to disable it.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     disable: disable bypass register
 * @return
 *     1. 0, successfully disable bypass register.\n
 *     2. -EINVAL, invalid parameter or not support.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_bypass_shadow_disable(const struct device *dev, bool disable)
{
	struct mtk_wdma *wdma;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	wdma->bypass_shadow_disable = disable;

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_bypass_shadow_disable);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     To Set FBC FDBC CR CH0123 VAL0.\n
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     value: register value
 * @return
 *     1. 0, successfully disable bypass register.\n
 *     2. -EINVAL, invalid parameter or not support.\n
 *     3. -ENODEV, No such device.\n
 *     4. -EPERM, operation not permitted.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_fbdc_cr_ch0123_val0(const struct device *dev, u32 value)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->wdma_type != MTK_3612_DISP_WDMA_0_PVRIC)
		return -EPERM;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = NULL;
			trg.subsys = wdma->subsys[hw_id];

			mtk_wdma_fbdc_cr_ch0123_val0(&trg, value);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_fbdc_cr_ch0123_val0);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     To Set FBC FDBC CR CH0123 VAL1.\n
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     value: register value
 * @return
 *     1. 0, successfully disable bypass register.\n
 *     2. -EINVAL, invalid parameter or not support.\n
 *     3. -ENODEV, No such device.\n
 *     4. -EPERM, operation not permitted.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_fbdc_cr_ch0123_val1(const struct device *dev, u32 value)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->wdma_type != MTK_3612_DISP_WDMA_0_PVRIC)
		return -EPERM;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = NULL;
			trg.subsys = wdma->subsys[hw_id];

			mtk_wdma_fbdc_cr_ch0123_val1(&trg, value);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_fbdc_cr_ch0123_val1);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Start the functon of WDMA.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_start(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = handle ? handle[hw_id] : NULL;
			trg.subsys = wdma->subsys[hw_id];

			mtk_wdma_smi_byte_mask_enable(&trg);

			if (wdma->pvric_en)
				mtk_wdma_pvric_on(&trg);

			if (wdma->bypass_shadow_disable)
				mtk_wdma_bypass_shadow_off(&trg);

			mtk_wdma_enable(&trg);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_start);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Stop the functon of WDMA.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_stop(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = handle ? handle[hw_id] : NULL;
			trg.subsys = wdma->subsys[hw_id];

			mtk_wdma_disable(&trg);

			if (wdma->pvric_en)
				mtk_wdma_pvric_off(&trg);

			if (wdma->bypass_shadow_disable)
				mtk_wdma_bypass_shadow_on(&trg);
		}
	return 0;
}
EXPORT_SYMBOL(mtk_wdma_stop);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set the region of wdma. It includes input region, output region, and\n
 *         crop position. For a input image with the resolution of (w, h),\n
 *         to crop a rectange whose top-left position is (x1,y1) and\n
 *         bottom-right position is (x2, y2), the setting should be\n
 *         (in_w, in_h) = (w, h), (out_w, out_h) = (x2 - x1 + 1, y2 - y1 + 1)\n
 *         , and (crop_x, crop_y) = (x1, y1).
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     in_w: input width of wdma.\n
 *     For DISP_WDMA0, DISP_WDMA1, DISP_WDMA2, DISP_WDMA3: 1~1920.\n
 *     For WDMA_0, WDMA_1, WDMA_2: 1~2560.\n
 *     For WDMA_GAZE: 1~1280.\n
 *     DISP_WDMA0 with PVRIC enable, should be in the range of 1 to 1920\n
 *     and multiple of 8.
 * @param[in]
 *     in_h: input height of wdma.\n
 *     With PVRIC enabled, height should be a multiple of 8\n
 *     and in the range 1~2184.\n
 *     With PVRIC disabled, height should be in the range 1~2205.\n
 *     For WDMA_0, WDMA_1, WDMA_2, WDMA_GAZE: 1~640.
 * @param[in]
 *     out_w: output width of wdma (cropped width). The cropped region should\n
 *     be in the range of input image.
 * @param[in]
 *     out_h: output height of wdma (cropped height). The cropped region\n
 *     should be in the range of input image.
 * @param[in]
 *     crop_x: horizontal position of the top-left corner of the cropped\n
 *         region.\n
 *     Range: 1 ~ 2,160
 * @param[in]
 *     crop_y: verticall position of the top-left corner of the cropped\n
 *         region.\n
 *     Range: 1 ~ 1,280
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     in_w should be in the range of 1 to 2560. in_h should be in the range\n
 *         of 1 to 2160. out_w should be in the range of 1 to 2560. out_h\n
 *         should be in the range of 1 to 2205. The cropped region should be\n
 *         in the range of input image.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_region(const struct device *dev, struct cmdq_pkt **handle,
		u32 in_w, u32 in_h, u32 out_w, u32 out_h,
		u32 crop_x, u32 crop_y)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->wdma_type == MTK_3612_DISP_WDMA_0_PVRIC
		|| wdma->wdma_type == MTK_3612_DISP_WDMA_1
		|| wdma->wdma_type == MTK_3612_DISP_WDMA_2
		|| wdma->wdma_type == MTK_3612_DISP_WDMA_3) {
		if (in_w > 1920)
			return -EINVAL;
		if (in_h > 2205)
			return -EINVAL;

		if (wdma->pvric_en) {
			if (in_w % 8)
				return -EINVAL;
			if (in_h % 8)
				return -EINVAL;
			if (out_w % 8)
				return -EINVAL;
			if (out_h % 8)
				return -EINVAL;
		}
	} else if (wdma->wdma_type == MTK_3612_WDMA_0
		|| wdma->wdma_type == MTK_3612_WDMA_1
		|| wdma->wdma_type == MTK_3612_WDMA_2) {
		if (in_w > 2560)
			return -EINVAL;
		if (in_h > 640)
			return -EINVAL;
	} else if (wdma->wdma_type == MTK_3612_WDMA_GAZE) {
		if (in_w > 1280)
			return -EINVAL;
		if (in_h > 640)
			return -EINVAL;
	}

	if (out_w > in_w)
		return -EINVAL;
	if (out_h > in_h)
		return -EINVAL;

	if (crop_x > 1280)
		return -EINVAL;
	if (crop_y > 2160)
		return -EINVAL;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = handle ? handle[hw_id] : NULL;
			trg.subsys = wdma->subsys[hw_id];

			mtk_wdma_crop(&trg, in_w / wdma->eye_hw_nr, in_h,
					crop_x, crop_y,
					out_w / wdma->eye_hw_nr, out_h);
		}

	wdma->out_w = out_w;
	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_region);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set address, pitch, and format of output buffer with index.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     idx: the index of output buffer. This index is used to indicate\n
 *     which buffer address you want WDMA to write (multi-plane,\n
 *     separated y and uv for example). In epcot Next project, you can ignore.
 * @param[in]
 *     addr: the address of this output buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of output buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @param[in]
 *     fmt: the color format of output buffer. It should follow the\n
 *         definition defined in drm_fourcc.h.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     For output 10 bit color format, the address of output buffer should\n
 *         be multiple of 16.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_multi_out_buf(const struct device *dev,
		struct cmdq_pkt **handle, u32 idx, dma_addr_t addr, u32 pitch,
		u32 fmt)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;
	int bpp;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	bpp = mtk_wdma_format_to_bpp(fmt);
	if (bpp < 0) {
		dev_err(dev, "mtk_wdma_set_multi_out_buf: not supported fmt %u\n",
				fmt);
		return -EINVAL;
	}

	if (idx > MTK_WDMA_OUT_BUF_2) {
		dev_err(dev, "mtk_wdma_set_multi_out_buf: idx %d error\n",
				idx);
		return -EINVAL;
	}

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = handle ? handle[hw_id] : NULL;
			trg.subsys = wdma->subsys[hw_id];

			switch (idx) {
			case MTK_WDMA_OUT_BUF_0:
				mtk_wdma_out_addr_0(&trg, addr
						+ wdma->addr_offset[hw_id][idx]
						+ eye_hw_id * wdma->out_w
						* bpp / wdma->eye_hw_nr / 8);
				mtk_wdma_out_pitch_0(&trg, pitch);
				mtk_wdma_out_format(&trg, fmt, wdma->pvric_en);
				break;
			case MTK_WDMA_OUT_BUF_1:
				mtk_wdma_out_addr_1(&trg, addr
						+ wdma->addr_offset[hw_id][idx]
						+ eye_hw_id * wdma->out_w
						* bpp / wdma->eye_hw_nr / 8);
				mtk_wdma_out_pitch_1_2(&trg, pitch);
				break;
			case MTK_WDMA_OUT_BUF_2:
				mtk_wdma_out_addr_2(&trg, addr
						+ wdma->addr_offset[hw_id][idx]
						);
				break;
			default:
				dev_err(dev, "[%s] idx %d error\n", __func__,
					idx);
				return -EINVAL;
			}

			wdma->addr[hw_id][idx] = addr;
		}
	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_multi_out_buf);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set address, pitch, and format of output buffer.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     addr: the address of this output buffer. It is physical address\n
 *         without iommu. It is iova with iommu.
 * @param[in]
 *     pitch: the pitch of output buffer. The pitch means the distance\n
 *         (in bytes) between the first pixel of two consecutive line.
 * @param[in]
 *     fmt: the color format of output buffer. It should follow the\n
 *         definition defined in drm_fourcc.h.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 * @par Boundary case and Limitation
 *     For output 10 bit color format, the address of output buffer should\n
 *         be multiple of 16.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_out_buf(const struct device *dev, struct cmdq_pkt **handle,
		dma_addr_t addr, u32 pitch, u32 fmt)
{
	return mtk_wdma_set_multi_out_buf(dev, handle, MTK_WDMA_OUT_BUF_0,
					  addr, pitch, fmt);
}
EXPORT_SYMBOL(mtk_wdma_set_out_buf);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set address offset of output buffer with index. This is just a work\n
 *     around function, so it's not suggested to use this function.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     idx: the index of output buffer.
 * @param[in]
 *     addr_offset: the address offset of this output buffer.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     For output 10 bit color format, the address of output buffer should\n
 *         be multiple of 16.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_multi_out_buf_addr_offset(const struct device *dev,
		struct cmdq_pkt **handle, u32 idx, u32 addr_offset)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (idx > MTK_WDMA_OUT_BUF_2) {
		dev_err(dev, "[%s] idx %d error\n", __func__, idx);
		return -EINVAL;
	}

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = handle ? handle[hw_id] : NULL;
			trg.subsys = wdma->subsys[hw_id];

			switch (idx) {
			case MTK_WDMA_OUT_BUF_0:
				mtk_wdma_out_addr_0(
					&trg,
					wdma->addr[hw_id][idx] + addr_offset);
				break;
			case MTK_WDMA_OUT_BUF_1:
				mtk_wdma_out_addr_1(
					&trg,
					wdma->addr[hw_id][idx] + addr_offset);
				break;
			case MTK_WDMA_OUT_BUF_2:
				mtk_wdma_out_addr_2(
					&trg,
					wdma->addr[hw_id][idx] + addr_offset);
				break;
			default:
				dev_err(dev, "[%s] idx %d error\n", __func__,
					idx);
				return -EINVAL;
			}

			wdma->addr_offset[hw_id][idx] = addr_offset;
		}
	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_multi_out_buf_addr_offset);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set WDMA color transform
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     color_trans: transform struct for user specified.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_color_transform(const struct device *dev,
		struct cmdq_pkt **handle,
		struct mtk_wdma_color_trans *color_trans)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;
	int ret;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = handle ? handle[hw_id] : NULL;
			trg.subsys = wdma->subsys[hw_id];

			if (!color_trans->enable) {
				mtk_wdma_color_transform_disable(&trg);
				continue;
			}

			if (color_trans->external_matrix) {
				mtk_wdma_ext_color_transform_matrix(&trg,
						color_trans->c00,
						color_trans->c01,
						color_trans->c02,
						color_trans->c10,
						color_trans->c11,
						color_trans->c12,
						color_trans->c20,
						color_trans->c21,
						color_trans->c22,
						color_trans->pre_add_0,
						color_trans->pre_add_1,
						color_trans->pre_add_2,
						color_trans->post_add_0,
						color_trans->post_add_1,
						color_trans->post_add_2);
			} else {
				ret = mtk_wdma_int_color_transform_matrix(&trg,
						color_trans->int_matrix);
				if (ret < 0) {
					dev_err(dev,
						"[%s] err, int_matrix = %d\n",
						__func__,
						color_trans->int_matrix);
					return ret;
				}
			}

			mtk_wdma_color_transform_enable(&trg);
		}
	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_color_transform);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set active eye to which the following setting would apply.\n
 *     This function is not used for epcot next. We keep this function was\n
 *     for the stability of early porting.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     active_eye: the active eye number.
 * @return
 *     1. 0, success.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     Active eye number should be less than eye number.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_active_eye(const struct device *dev, u32 active_eye)
{
	struct mtk_wdma *wdma;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if ((active_eye != MTK_WDMA_ACTIVE_EYE_ALL) &&
			(active_eye >= wdma->eye_nr)) {
		dev_err(dev, "Invalid active eye nr %d\n", active_eye);
		return -EINVAL;
	}

	wdma->active_eye = active_eye;
	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_active_eye);

static int mtk_wdma_smi_larb_get_(const struct device *dev, u32 hw_nr,
		struct device **dev_larb)
{
	int i, ret;

	for (i = 0; i < hw_nr; i++) {
		ret = mtk_smi_larb_get(dev_larb[i]);
		if (ret < 0) {
			dev_err(dev, "Failed to get larb %d: %d\n", i, ret);
			goto err;
		}
	}

	return 0;

err:
	while (--i >= 0)
		mtk_smi_larb_put(dev_larb[i]);

	return ret;
}

static int mtk_wdma_smi_larb_get(const struct device *dev)
{
	struct mtk_wdma *wdma;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->out_mode == MTK_WDMA_OUT_MODE_DRAM)
		return mtk_wdma_smi_larb_get_(dev, wdma->hw_nr,
				wdma->dev_larb);
	else
		return mtk_wdma_smi_larb_get_(dev, wdma->hw_nr,
				wdma->dev_s_larb);
}

static void mtk_wdma_smi_larb_put_(const struct device *dev, u32 hw_nr,
		struct device **dev_larb)
{
	int i;

	for (i = 0; i < hw_nr; i++)
		mtk_smi_larb_put(dev_larb[i]);
}

static int mtk_wdma_smi_larb_put(const struct device *dev)
{
	struct mtk_wdma *wdma;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->out_mode == MTK_WDMA_OUT_MODE_DRAM)
		mtk_wdma_smi_larb_put_(dev, wdma->hw_nr, wdma->dev_larb);
	else
		mtk_wdma_smi_larb_put_(dev, wdma->hw_nr, wdma->dev_s_larb);

	return 0;
}

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set WDMA Interrupt target line.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     handle: command queue packet. If it is NULL, write the configuration\n
 *         to register directly. Otherwise, write the configuration to\n
 *         command queue packet.
 * @param[in]
 *     line: Set WDMA target line, should be in the range of 1 to 2560.
 * @return
 *     1. 0, successfully set target line.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     line should be in the range of 1 to 2560.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_target_line(const struct device *dev,
			     struct cmdq_pkt **handle, u32 line)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	if (line < 1 || line > 2560)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = handle ? handle[hw_id] : NULL;
			trg.subsys = wdma->subsys[hw_id];

			mtk_wdma_target_line(&trg, line);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_set_target_line);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Get WDMA Interrupt target line.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[out]
 *     line: Get current line count.
 * @return
 *     1. 0, successfully get current line.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     None.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_get_target_line(const struct device *dev, u32 *line)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	u32 hw_id, eye_id, eye_hw_id;
	u32 eye_id_start, eye_id_end;

	if (!dev)
		return -EINVAL;

	if (!line)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (wdma->active_eye == MTK_WDMA_ACTIVE_EYE_ALL) {
		eye_id_start = 0;
		eye_id_end = wdma->eye_nr;
	} else {
		eye_id_start = wdma->active_eye;
		eye_id_end = wdma->active_eye + 1;
	}

	for (eye_id = eye_id_start; eye_id < eye_id_end; eye_id++)
		for (eye_hw_id = 0; eye_hw_id < wdma->eye_hw_nr; eye_hw_id++) {
			hw_id = eye_id * wdma->eye_hw_nr + eye_hw_id;
			trg.regs = wdma->regs[hw_id];
			trg.reg_base = wdma->reg_base[hw_id];
			trg.pkt = NULL;
			trg.subsys = wdma->subsys[hw_id];

			reg_get_val(&trg,
				WDMA_TARGET_LINE, line, TARGET_LINE);
		}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_get_target_line);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Set output mode. Output to dram or sram.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     out_mode: MTK_WDMA_OUT_MODE_DRAM for output to dram,\n
 *         MTK_WDMA_OUT_MODE_SRAM for output to sram.
 * @return
 *     1. 0, successfully set outpu mode.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     1. Call this function in stop-state or idle-state.
 *     2. out_mode should be MTK_WARPA_OUT_MODE_ALL, MTK_WDMA_OUT_MODE_DRAM,\n
 *         or MTK_WDMA_OUT_MODE_SRAM.
 * @par Error case and Error handling
 *     Same as return part.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_set_out_mode(const struct device *dev, u32 out_mode)
{
	struct mtk_wdma *wdma;
	int ret;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	if (out_mode > 1)
		return -EINVAL;

	if (out_mode == wdma->out_mode)
		return 0;

	ret = mtk_wdma_smi_larb_put(dev);
	if (ret)
		return ret;

	wdma->out_mode = out_mode;

	return mtk_wdma_smi_larb_get(dev);
}
EXPORT_SYMBOL(mtk_wdma_set_out_mode);

static int mtk_wdma_clock_on(struct device *dev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_wdma *wdma;
	int i, ret;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	for (i = 0; i < wdma->hw_nr; i++) {
		ret = clk_prepare_enable(wdma->clk[i]);
		if (ret) {
			dev_err(dev, "Failed to enable clock hw %d: ret %d\n",
					i, ret);
			goto err;
		}
	}
	return 0;
err:
	while (--i >= 0)
		clk_disable_unprepare(wdma->clk[i]);

	return ret;
#else
	return 0;
#endif
}

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Turn on the power of WDMA. Before any configuration, you should\n
 *         turn on the power of WDMA.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_power_on(struct device *dev)
{
#if CONFIG_PM
	struct mtk_wdma *wdma;
	int ret;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to get sync of power domain: %d\n", ret);
		return ret;
	}

	ret = mtk_wdma_clock_on(dev);
	if (ret) {
		pm_runtime_put(dev);
		return ret;
	}
#endif
	return mtk_wdma_smi_larb_get(dev);
}
EXPORT_SYMBOL(mtk_wdma_power_on);

static int mtk_wdma_clock_off(struct device *dev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_wdma *wdma;
	int i;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	for (i = 0; i < wdma->hw_nr; i++)
		clk_disable_unprepare(wdma->clk[i]);
#endif
	return 0;
}

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Turn off the power of WDMA.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_power_off(struct device *dev)
{
#if CONFIG_PM
	struct mtk_wdma *wdma;
	int ret;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	ret = pm_runtime_put_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed to put sync of power domain: %d\n", ret);
		return ret;
	}

	ret = mtk_wdma_clock_off(dev);

	if (!ret) {
		pm_runtime_put(dev);
		return ret;
	}
#endif
	return mtk_wdma_smi_larb_put(dev);
}
EXPORT_SYMBOL(mtk_wdma_power_off);

/** @ingroup IP_group_wdma_external_function
 * @par Description
 *     Register callback function for WDMA frame done.
 * @param[in]
 *     dev: pointer of wdma device structure.
 * @param[in]
 *     func: callback function which is called by WDMA interrupt handler.
 * @param[in]
 *     status: status that user want to monitor.
 * @param[in]
 *     priv_data: caller may have multiple instance, so it could use this\n
 *         private data to distinguish which instance is callbacked.
 * @return
 *     1. 0, successfully start wdma hardware.\n
 *     2. -EINVAL, invalid parameter.\n
 *     3. -ENODEV, No such device.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_wdma_register_cb(const struct device *dev, mtk_mmsys_cb func,
		u32 status, void *priv_data)
{
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	unsigned long flags;
	int i;

	if (!dev)
		return -EINVAL;

	wdma = dev_get_drvdata(dev);

	if (!wdma)
		return -ENODEV;

	spin_lock_irqsave(&wdma->spinlock_irq, flags);
	for (i = 0; i < wdma->hw_nr; i++)
		wdma->irq_status[i] &= ~status;
	wdma->cb_func = func;
	wdma->cb_status = status;
	wdma->cb_priv_data = priv_data;
	spin_unlock_irqrestore(&wdma->spinlock_irq, flags);

	for (i = 0; i < wdma->hw_nr; i++) {
		trg.regs = wdma->regs[i];
		trg.reg_base = wdma->reg_base[i];
		trg.pkt = NULL;
		trg.subsys = wdma->subsys[i];

		reg_set_val(&trg, WDMA_INTEN, status, GENMASK(3, 0));
	}

	return 0;
}
EXPORT_SYMBOL(mtk_wdma_register_cb);

static irqreturn_t mtk_wdma_irq_handler(int irq, void *dev_id)
{
	struct mtk_wdma *wdma = dev_id;
	struct reg_trg trg;
	struct mtk_mmsys_cb_data cb_data;
	mtk_mmsys_cb func;
	unsigned long flags;
	u32 merge_status = MTK_WDMA_INT_ALL;
	u32 reg;
	int i;

	for (i = 0; i < wdma->hw_nr; i++)
		if (irq == wdma->irq[i])
			break;

	cb_data.part_id = i;

	trg.regs = wdma->regs[cb_data.part_id];
	trg.reg_base = wdma->reg_base[cb_data.part_id];
	trg.pkt = NULL;
	trg.subsys = wdma->subsys[cb_data.part_id];
	reg_get_val(&trg, WDMA_INTSTA, &reg, GENMASK(3, 0));
	reg_set_val(&trg, WDMA_INTSTA, 0x0, GENMASK(3, 0));

	spin_lock_irqsave(&wdma->spinlock_irq, flags);
	wdma->irq_status[cb_data.part_id] = reg;
	for (i = 0; i < wdma->hw_nr; i++)
		merge_status &= wdma->irq_status[i];
	if (merge_status)
		for (i = 0; i < wdma->hw_nr; i++)
			wdma->irq_status[i] &= ~merge_status;
	func = wdma->cb_func;
	cb_data.status = wdma->cb_status & reg;
	cb_data.priv_data = wdma->cb_priv_data;
	spin_unlock_irqrestore(&wdma->spinlock_irq, flags);

	if (func && cb_data.status)
		func(&cb_data);

	return IRQ_HANDLED;
}

static const struct of_device_id wdma_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-disp_wdma0-pvric",
	  .data = (void *)MTK_3612_DISP_WDMA_0_PVRIC},
	{ .compatible = "mediatek,mt3612-disp_wdma1",
	  .data = (void *)MTK_3612_DISP_WDMA_1},
	{ .compatible = "mediatek,mt3612-disp_wdma2",
	  .data = (void *)MTK_3612_DISP_WDMA_2},
	{ .compatible = "mediatek,mt3612-disp_wdma3",
	  .data = (void *)MTK_3612_DISP_WDMA_3},
	{ .compatible = "mediatek,mt3612-common_wdma0",
	  .data = (void *)MTK_3612_WDMA_0},
	{ .compatible = "mediatek,mt3612-common_wdma1",
	  .data = (void *)MTK_3612_WDMA_1},
	{ .compatible = "mediatek,mt3612-common_wdma2",
	  .data = (void *)MTK_3612_WDMA_2},
	{ .compatible = "mediatek,mt3612-gaze_wdma0",
	  .data = (void *)MTK_3612_WDMA_GAZE},
	{},
};
MODULE_DEVICE_TABLE(of, wdma_driver_dt_match);

#ifdef CONFIG_MTK_DEBUGFS
static int wdma_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_wdma *wdma = s->private;
	u32 reg;
	u32 format;
	u32 state;

	seq_puts(s, "-------------------- WDMA DEUBG --------------------\n");

	reg = readl(wdma->regs[0] + WDMA_FLOW_CTRL_DBG);

	state = GET_VAL(reg, WDMA_STATE);
	seq_puts(s, "STATE:         :");
	switch (state) {
	case BIT(0):
		seq_puts(s, "00_0000_0001: Idle state\n");
		break;
	case BIT(1):
		seq_puts(s, "00_0000_0010: Clear state\n");
		break;
	case BIT(2):
		seq_puts(s, "00_0000_0100: Prepare state 1\n");
		break;
	case BIT(3):
		seq_puts(s, "00_0000_1000: Prepare state 2\n");
		break;
	case BIT(4):
		seq_puts(s, "00_0001_0000: Data transmit state\n");
		break;
	case BIT(5):
		seq_puts(s, "00_0010_0000: EOF prepare state\n");
		break;
	case BIT(6):
		seq_puts(s, "00_0100_0000: SW reset prepare state\n");
		break;
	case BIT(7):
		seq_puts(s, "00_1000_0000: EOF wait idle state\n");
		break;
	case BIT(8):
		seq_puts(s, "01_0000_0000: SW reset wait idle state\n");
		break;
	case BIT(9):
		seq_puts(s, "10_0000_0000: Frame completed state\n");
		break;
	default:
		break;
	}

	seq_printf(s, "FIFO FULL      :%12ld\n", GET_VAL(reg, WDMA_FIFO_FULL));
	seq_printf(s, "GREQ           :%12ld\n", GET_VAL(reg, WDMA_GREQ));
	seq_printf(s, "IN READY       :%12ld\n", GET_VAL(reg, WDMA_IN_READY));
	seq_printf(s, "IN REQ         :%12ld\n", GET_VAL(reg, WDMA_IN_REQ));
	seq_printf(s, "FRAME DONE     :%12ld\n", GET_VAL(reg, FRAME_COMPLETE));
	seq_printf(s, "FRAME INC      :%12ld\n", GET_VAL(reg, FRAME_UNDERRUN));

	reg = readl(wdma->regs[0] + WDMA_INTEN);

	seq_printf(s, "IRQ DONE EN    :%12ld\n",
		   GET_VAL(reg, FRAME_COMPLETE_EN));
	seq_printf(s, "IRQ UNDER EN   :%12ld\n",
		   GET_VAL(reg, FRAME_UNDERRUN_EN));
	seq_printf(s, "IRQ FULL EN    :%12ld\n",
		   GET_VAL(reg, FIFO_FULL_INT_EN));
	seq_printf(s, "IRQ LINE EN    :%12ld\n",
		   GET_VAL(reg, TARGET_LINE_INT_EN));

	reg = readl(wdma->regs[0] + WDMA_INTSTA);

	seq_printf(s, "IRQ DONE STA   :%12ld\n",
		   GET_VAL(reg, FRAME_COMPLETE_STA));
	seq_printf(s, "IRQ UNDER STA  :%12ld\n",
		   GET_VAL(reg, FRAME_UNDERRUN_STA));
	seq_printf(s, "IRQ FULL STA   :%12ld\n",
		   GET_VAL(reg, FIFO_FULL_INT_STA));
	seq_printf(s, "IRQ LINE STA   :%12ld\n",
		   GET_VAL(reg, TARGET_LINE_INT_STA));

	reg = readl(wdma->regs[0] + WDMA_EN);

	seq_printf(s, "ENABLE         :%12ld\n", GET_VAL(reg, ENABLE));
	seq_printf(s, "BYPASS SHADOW  :%12ld\n", GET_VAL(reg, BYPASS_SHADOW));
	seq_printf(s, "IN  CRC EN     :%12ld\n",
		   GET_VAL(reg, INPUT_CRC_ENABLE));
	seq_printf(s, "OUT CRC EN     :%12ld\n",
		   GET_VAL(reg, OUTPUT_CRC_ENABLE));
	seq_printf(s, "PVRIC ENABLE   :%12ld\n", GET_VAL(reg, PVRIC_EN));

	reg = readl(wdma->regs[0] + WDMA_CFG);
	format = GET_VAL(reg, OUT_FORMAT);

	switch (format) {
	case 0:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "RGB565");
		break;
	case 1:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "RGB888");
		break;
	case 2:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "BGRA8888");
		break;
	case 3:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "ABGR8888");
		break;
	case 7:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "Y 8-bit");
		break;
	case 11:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "BGRA1010102 (BGRA10/10/10/2)");
		break;
	case 15:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "ARGB2101010 (ARGB2/10/10/10)");
		break;
	default:
		seq_printf(s, "OUT FORMAT     :%12d: %s\n",
			   format, "");
		break;
	}

	seq_printf(s, "SWAP           :%12ld\n", GET_VAL(reg, SWAP));
	seq_printf(s, "YUV444 OUT     :%12ld\n", GET_VAL(reg, YUV444_OUT));

	reg = readl(wdma->regs[0] + WDMA_SRC_SIZE);

	seq_printf(s, "SRC Width      :%12ld\n", GET_VAL(reg, WIDTH));
	seq_printf(s, "SRC Height     :%12ld\n", GET_VAL(reg, HEIGHT));

	reg = readl(wdma->regs[0] + WDMA_CLIP_SIZE);

	seq_printf(s, "CLIP Width     :%12ld\n", GET_VAL(reg, WIDTH));
	seq_printf(s, "CLIP Height    :%12ld\n", GET_VAL(reg, HEIGHT));

	reg = readl(wdma->regs[0] + WDMA_DST_W_IN_BYTE);

	seq_printf(s, "PITCH          :%12ld\n",
		   GET_VAL(reg, DST_W_IN_BYTE));

	reg = readl(wdma->regs[0] + WDMA_EXEC_DBG);

	seq_printf(s, "SOF_COUNT      :%12ld\n",
		   GET_VAL(reg, WDMA_FRAME_RUN_CNT));
	seq_printf(s, "COMPLETE_COUNT :%12ld\n",
		   GET_VAL(reg, WDMA_FRAME_COMPLETE_CNT));

	reg = readl(wdma->regs[0] + WDMA_CT_DBG);

	seq_printf(s, "INPUT CNT X    :%12ld\n",
		   GET_VAL(reg, WDMA_INPUT_CNT_X));
	seq_printf(s, "INPUT CNT Y    :%12ld\n",
		   GET_VAL(reg, WDMA_INPUT_CNT_Y));

	reg = readl(wdma->regs[0] + WDMA_TARGET_LINE);

	seq_printf(s, "TARGET LINE    :%12ld\n",
		   GET_VAL(reg, TARGET_LINE));

	reg = readl(wdma->regs[0] + WDMA_CRC0);

	seq_printf(s, "CRC 0 IN  PRE  :%12ld\n",
		   GET_VAL(reg, WDMA_CRC_0));

	reg = readl(wdma->regs[0] + WDMA_CRC1);

	seq_printf(s, "CRC 1 IN CUR   :%12ld\n",
		   GET_VAL(reg, WDMA_CRC_1));

	reg = readl(wdma->regs[0] + WDMA_CRC2);

	seq_printf(s, "CRC 2 OUT PRE  :%12ld\n",
		   GET_VAL(reg, WDMA_CRC_2));

	reg = readl(wdma->regs[0] + WDMA_CRC3);

	seq_printf(s, "CRC 3 OUT CUR  :%12ld\n",
		   GET_VAL(reg, WDMA_CRC_3));

	reg = readl(wdma->regs[0] + WDMA_DST_ADDR0);

	seq_printf(s, "DST_ADDR0      :  0x%08lx\n",
		   GET_VAL(reg, ADDRESS0));

	reg = readl(wdma->regs[0] + WDMA_DST_ADDR1);

	seq_printf(s, "DST_ADDR1      :  0x%08lx\n",
		   GET_VAL(reg, ADDRESS1));

	reg = readl(wdma->regs[0] + WDMA_DST_ADDR2);

	seq_printf(s, "DST_ADDR2      :  0x%08lx\n",
		   GET_VAL(reg, ADDRESS2));

	reg = readl(wdma->regs[0] + WDMA_FBC_FBDC_CR_CH0123_VAL0);

	seq_printf(s, "FBDC CH0123 0  :  0x%08lx\n",
		   GET_VAL(reg, FBC_FBDC_CR_CH0123_VAL0));

	reg = readl(wdma->regs[0] + WDMA_FBC_FBDC_CR_CH0123_VAL1);

	seq_printf(s, "FBDC CH0123 1  :  0x%08lx\n",
		   GET_VAL(reg, FBC_FBDC_CR_CH0123_VAL1));

	reg = readl(wdma->regs[0] + WDMA_PVRIC_FRAME_COMP_MON_CUR);

	seq_printf(s, "COMP_MON_CUR   :%12lu\n",
		   GET_VAL(reg, PVRIC_FRAME_COMP_MON_CUR));

	reg = readl(wdma->regs[0] + WDMA_PVRIC_FRAME_COMP_MON_PRE);

	seq_printf(s, "COMP_MON_PRE   :%12lu\n",
		   GET_VAL(reg, PVRIC_FRAME_COMP_MON_PRE));

	seq_puts(s, "----------------------------------------------------\n");
	return 0;
}

static int mtk_wdma_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, wdma_debug_show, inode->i_private);
}

/* echo reg r 8 8 8 > /sys/kernel/debug/mediatek/wdma/14018000.dispwdma */
/* echo reg w 8 8 8 1 > /sys/kernel/debug/mediatek/wdma/14018000.dispwdma */
static ssize_t mtk_wdma_debugfs_write(struct file *file,
					   const char __user *ubuf,
					   size_t count, loff_t *ppos)
{
	char *buf = vmalloc(count+1);
	char *vbuf = buf;
	struct mtk_wdma *wdma;
	struct reg_trg trg;
	struct seq_file *seq_ptr;
	const char sp[] = " ";
	char *token;
	char op[2] = {0};
	u32 offset;
	u32 mask_msb;
	u32 mask_lsb;
	u32 value;

	if (!buf)
		goto out;

	if (copy_from_user(buf, ubuf, count))
		goto out;

	seq_ptr = (struct seq_file *)file->private_data;

	pr_err("[wdma][debugfs] %s\n", buf);
	buf[count] = '\0';

	wdma = (struct mtk_wdma *)seq_ptr->private;

	if (!wdma)
		goto out;

	trg.regs = wdma->regs[0];
	trg.reg_base = wdma->reg_base[0];
	trg.pkt = NULL;
	trg.subsys = wdma->subsys[0];

	token = strsep(&buf, sp);

	if (strncmp(token, "reg", 3) == 0) {
		token = strsep(&buf, sp);
		strncpy(op, token, 1);

		token = strsep(&buf, sp);
		if (strncmp(token, "0x", 2) != 0)
			goto out;
		if (kstrtouint(token+2, 16, &offset))
			goto out;
		token = strsep(&buf, sp);
		if (kstrtouint(token, 10, &mask_msb))
			goto out;
		token = strsep(&buf, sp);
		if (kstrtouint(token, 10, &mask_lsb))
			goto out;
		token = strsep(&buf, sp);
		if (strncmp(op, "w", 1) == 0)
			if (kstrtouint(token, 10, &value))
				goto out;

		if (strncmp(op, "w", 1) == 0) {
			reg_set_val(&trg, offset, value,
					GENMASK(mask_msb, mask_lsb));
		} else if (strncmp(op, "r", 1) == 0) {
			reg_get_val(&trg, offset, &value,
					GENMASK(mask_msb, mask_lsb));
		}
		pr_err("reg '%s' 0x%03x [%d:%d] 0x%x\n",
			op, offset, mask_msb, mask_lsb, value);
	}

out:
	vfree(vbuf);
	return count;
}

static const struct file_operations wdma_debug_fops = {
	.open = mtk_wdma_debugfs_open,
	.read = seq_read,
	.write = mtk_wdma_debugfs_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int mtk_wdma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_wdma *wdma;
	struct resource *regs;
	const struct of_device_id *of_id;
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry *debug_dentry;
#endif
	struct platform_device *larb_pdev;
	struct device_node *larb_node;
	int i, irq, ret;

	wdma = devm_kzalloc(dev, sizeof(*wdma), GFP_KERNEL);
	if (!wdma)
		return -ENOMEM;

	of_id = of_match_device(wdma_driver_dt_match, &pdev->dev);
	if (!of_id)
		return -ENODEV;
	wdma->wdma_type = (u64)of_id->data;

	wdma->active_eye = MTK_WDMA_ACTIVE_EYE_ALL;

	wdma->eye_nr = 1;
	wdma->eye_hw_nr = 1;
	wdma->hw_nr = MODULE_MAX_NUM;

#if defined(CONFIG_COMMON_CLK_MT3612)
	for (i = 0; i < wdma->hw_nr; i++) {
		if (wdma->wdma_type == MTK_3612_DISP_WDMA_0_PVRIC)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_core_disp_wdma0");
		else if (wdma->wdma_type == MTK_3612_DISP_WDMA_1)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_core_disp_wdma1");
		else if (wdma->wdma_type == MTK_3612_DISP_WDMA_2)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_core_disp_wdma2");
		else if (wdma->wdma_type == MTK_3612_DISP_WDMA_3)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_core_disp_wdma3");
		else if (wdma->wdma_type == MTK_3612_WDMA_0)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_cmm_wdma_0");
		else if (wdma->wdma_type == MTK_3612_WDMA_1)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_cmm_wdma_1");
		else if (wdma->wdma_type == MTK_3612_WDMA_2)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_cmm_wdma_2");
		else if (wdma->wdma_type == MTK_3612_WDMA_GAZE)
			wdma->clk[i] =
				devm_clk_get(dev, "mmsys_gaze_wdma_gaze");

		if (IS_ERR(wdma->clk[i])) {
			dev_err(dev, "Failed to get clk\n");
			return PTR_ERR(wdma->clk[i]);
		}
	}
#endif

	spin_lock_init(&wdma->spinlock_irq);

	for (i = 0; i < wdma->hw_nr; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		wdma->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(wdma->regs[i])) {
			dev_err(dev, "Failed to map wdma registers\n");
			return PTR_ERR(wdma->regs[i]);
		}
		wdma->reg_base[i] = (u32)regs->start;

		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(dev, "Failed to get wdma %d irq.\n", i);
			return irq;
		}

		ret = devm_request_irq(dev, irq, mtk_wdma_irq_handler,
			       IRQF_TRIGGER_LOW, dev_name(dev), wdma);
		if (ret < 0) {
			dev_err(dev, "Failed to request wdma irq %d: %d\n",
					irq, ret);
			return ret;
		}
		wdma->irq[i] = irq;
	}

	of_property_read_u32_array(dev->of_node, "gce-subsys",
			wdma->subsys, wdma->hw_nr);

	of_node_put(dev->of_node);

	for (i = 0; i < wdma->hw_nr; i++) {
		larb_node = of_parse_phandle(dev->of_node, "mediatek,larbs", i);
		if (!larb_node) {
			dev_err(dev, "parse phandle of mediatek,larbs fail.\n");
			return -EINVAL;
		}

		if (!of_device_is_available(larb_node))
			continue;

		larb_pdev = of_find_device_by_node(larb_node);
		if (!dev_get_drvdata(&larb_pdev->dev)) {
			dev_err(dev, "Wdma is waiting for larb device %s\n",
				larb_node->full_name);
			of_node_put(larb_node);
			return -EPROBE_DEFER;
		}
		wdma->dev_larb[i] = &larb_pdev->dev;
		of_node_put(larb_node);
	}

#if CONFIG_PM
	pm_runtime_enable(dev);
#endif

	platform_set_drvdata(pdev, wdma);

#ifdef CONFIG_MTK_DEBUGFS
	if (wdma->hw_nr > 1 || !mtk_debugfs_root)
		goto debugfs_done;

	/* debug info create */
	if (!mtk_wdma_debugfs_root)
		mtk_wdma_debugfs_root = debugfs_create_dir("wdma",
							   mtk_debugfs_root);

	if (!mtk_wdma_debugfs_root) {
		dev_dbg(dev, "failed to create wdma debugfs root directory.\n");
		goto debugfs_done;
	}

	debug_dentry = debugfs_create_file(pdev->name, 0664,
					   mtk_wdma_debugfs_root,
					   wdma, &wdma_debug_fops);
debugfs_done:
	if (!debug_dentry)
		dev_dbg(dev, "Failed to create wdma debugfs %s\n", pdev->name);
#endif

	dev_dbg(dev, "wdma probe done, hw type = %lld\n", wdma->wdma_type);

	return 0;
}

static int mtk_wdma_remove(struct platform_device *pdev)
{
#if CONFIG_PM
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
#endif

#ifdef CONFIG_MTK_DEBUGFS
	debugfs_remove_recursive(mtk_wdma_debugfs_root);
	mtk_wdma_debugfs_root = NULL;
#endif
	return 0;
}

struct platform_driver mtk_wdma_driver = {
	.probe		= mtk_wdma_probe,
	.remove		= mtk_wdma_remove,
	.driver		= {
		.name		= "mediatek-wdma",
		.owner		= THIS_MODULE,
		.of_match_table = wdma_driver_dt_match,
	},
};

module_platform_driver(mtk_wdma_driver);
MODULE_LICENSE("GPL");
