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

#ifndef MTK_WDMA_H
#define MTK_WDMA_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif

/** default value of active eye number */
#define MTK_WDMA_ACTIVE_EYE_ALL		0xffffffff

/** @ingroup IP_group_wdma_external_enum
 * @brief WDMA color transform type
 */
enum mtk_wdma_internal_color_trans_matrix {
	MTK_WDMA_INT_COLOR_TRANS_RGB_TO_JPEG = 0,
	MTK_WDMA_INT_COLOR_TRANS_RGB_TO_BT601,
	MTK_WDMA_INT_COLOR_TRANS_RGB_TO_BT709,
	MTK_WDMA_INT_COLOR_TRANS_JPEG_TO_RGB,
	MTK_WDMA_INT_COLOR_TRANS_BT601_TO_RGB,
	MTK_WDMA_INT_COLOR_TRANS_BT709_TO_RGB,
	MTK_WDMA_INT_COLOR_TRANS_JPEG_TO_BT601,
	MTK_WDMA_INT_COLOR_TRANS_JPEG_TO_BT709,
	MTK_WDMA_INT_COLOR_TRANS_BT601_TO_JPEG,
	MTK_WDMA_INT_COLOR_TRANS_BT709_TO_JPEG,
	MTK_WDMA_INT_COLOR_TRANS_BT709_TO_BT601,
	MTK_WDMA_INT_COLOR_TRANS_BT601_TO_BT709,
	MTK_WDMA_INT_COLOR_TRANS_RGB_TO_BT709_FULL,
	MTK_WDMA_INT_COLOR_TRANS_BT709_FULL_TO_RGB,
};

/** @ingroup IP_group_wdma_external_struct
 * @brief WDMA color transform structure
 */
struct mtk_wdma_color_trans {
	/**  Enable or disable color transform */
	bool enable;
	/**  Use external matrix or internal matrix */
	bool external_matrix;
	/**  Internal color transform matrix */
	enum mtk_wdma_internal_color_trans_matrix int_matrix;
	/** External color transform matirx */
	u32 c00;
	u32 c01;
	u32 c02;
	u32 c10;
	u32 c11;
	u32 c12;
	u32 c20;
	u32 c21;
	u32 c22;
	u32 pre_add_0;
	u32 pre_add_1;
	u32 pre_add_2;
	u32 post_add_0;
	u32 post_add_1;
	u32 post_add_2;
};

enum mtk_wdma_out_buf {
	MTK_WDMA_OUT_BUF_0 = 0,
	MTK_WDMA_OUT_BUF_1,
	MTK_WDMA_OUT_BUF_2,
	MTK_WDMA_OUT_TDLR_BUF_0,
	MTK_WDMA_OUT_BUF_MAX,
};

/** @ingroup IP_group_wdma_external_def
 * @brief Wdma output mode
 * @{
 */
#define MTK_WDMA_OUT_MODE_DRAM		0
#define MTK_WDMA_OUT_MODE_SRAM		1
/**
 * @}
 */

#define MTK_WDMA_FRAME_COMPLETE		BIT(0)
#define MTK_WDMA_FRAME_UNDERRUN		BIT(1)
#define MTK_WDMA_FIFO_FULL_INT		BIT(2)
#define MTK_WDMA_TARGET_LINE_INT	BIT(3)
#define MTK_WDMA_INT_ALL		GENMASK(3, 0)

int mtk_wdma_reset(const struct device *dev);
int mtk_wdma_start(const struct device *dev, struct cmdq_pkt **handle);
int mtk_wdma_stop(const struct device *dev, struct cmdq_pkt **handle);
int mtk_wdma_set_region(
		const struct device *dev, struct cmdq_pkt **handle,
		u32 in_w, u32 in_h, u32 out_w, u32 out_h,
		u32 crop_x, u32 crop_y);
int mtk_wdma_set_out_buf(
		const struct device *dev, struct cmdq_pkt **handle,
		dma_addr_t addr, u32 pitch, u32 fmt);
int mtk_wdma_set_multi_out_buf(
		const struct device *dev, struct cmdq_pkt **handle,
		u32 idx, dma_addr_t addr, u32 pitch, u32 fmt);
int mtk_wdma_set_multi_out_buf_addr_offset(
		const struct device *dev, struct cmdq_pkt **handle,
		u32 idx, u32 addr_offset);
int mtk_wdma_set_color_transform(
		const struct device *dev, struct cmdq_pkt **handle,
		struct mtk_wdma_color_trans *color_trans);
int mtk_wdma_set_active_eye(const struct device *dev, u32 active_eye);
int mtk_wdma_set_out_mode(const struct device *dev, u32 out_mode);
int mtk_wdma_power_on(struct device *dev);
int mtk_wdma_power_off(struct device *dev);
int mtk_wdma_register_cb(
		const struct device *dev, mtk_mmsys_cb func,
		u32 status, void *priv_data);
int mtk_wdma_pvric_enable(const struct device *dev, bool enable);
int mtk_wdma_bypass_shadow_disable(const struct device *dev, bool disable);
int mtk_wdma_set_target_line(
		const struct device *dev, struct cmdq_pkt **handle,
		u32 line);
int mtk_wdma_get_target_line(const struct device *dev, u32 *line);
int mtk_wdma_get_pvric_frame_comp_cur(const struct device *dev, u32 *count);
int mtk_wdma_get_pvric_frame_comp_pre(const struct device *dev, u32 *count);
int mtk_wdma_get_hardware_state(const struct device *dev, u32 *state);
int mtk_wdma_set_fbdc_cr_ch0123_val0(const struct device *dev, u32 value);
int mtk_wdma_set_fbdc_cr_ch0123_val1(const struct device *dev, u32 value);
#endif
