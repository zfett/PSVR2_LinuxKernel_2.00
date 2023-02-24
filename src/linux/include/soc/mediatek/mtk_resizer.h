/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	Bibby Hsieh <bibby.hsieh@mediatek.com>
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
 * @file mtk_resizer.h
 * Header of mtk_resizer.c
 */

#ifndef MTK_RESIZER_H
#define MTK_RESIZER_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif

/** @ingroup IP_group_resizer_external_def
 * @brief Resizer interrupt status definition
 * @{
 */
/** Frame start interrupt */
#define RSZ_FRAME_START_INT	BIT(28)
/** Frame end interrupt */
#define RSZ_FRAME_END_INT	BIT(29)
/** Size error interrupt */
#define RSZ_ERR_INT		BIT(30)
/**
 * @}
 */

/** @ingroup IP_group_resizer_external_struct
 * @brief Resizer General Config Structure.
 */
struct mtk_resizer_config {
	/** input width */
	u16 in_width;
	/** input height */
	u16 in_height;
	/** output width */
	u16 out_width;
	/** output height */
	u16 out_height;
	/** overlap pix for tile mode */
	u16 overlap;
	/** resizer input format, RGB/YUV */
	u16 rsz_in_format;
};

/** @ingroup IP_group_resizer_external_struct
 * @brief Resizer General Config Ex Structure.
 */
struct mtk_resizer_config_test {
	/** input width */
	u16 in_width;
	/** input height */
	u16 in_height;
	/** output width */
	u16 out_width;
	/** output height */
	u16 out_height;
	/** overlap pix for tile mode */
	u16 overlap;
	/** in-band-signal-enhancement enable */
	bool ibse_enable;
	/** in-band-signal-enhancement gain */
	u16 ibse_gaincontrol_gain;
	/** tap-adaptive enable */
	bool tap_adapt_enable;
	/** resizer tap-adaptive slope  */
	u16 tap_adapt_slope;
	/** resizer input format, RGB/YUV */
	u16 rsz_in_format;
};

/** @ingroup IP_group_resizer_external_struct
 * @brief Resizer Quarter Config Structure.
 */
struct mtk_resizer_quarter_config {
	/** input start positions */
	u16 in_x_left[4];
	/** input end positions */
	u16 in_x_right[4];
	/** input height */
	u16 in_height;

	/** output start positions */
	u16 out_x_left[4];
	/** output end positions */
	u16 out_x_right[4];
	/** output height */
	u16 out_height;

	/** luma start positions integer offset */
	u16 luma_x_int_off[4];
	/** luma start positions sub-pixel offset */
	u32 luma_x_sub_off[4];
};

enum mtk_resizer_input_format {
	RESIZER_INPUT_RGB,
	RESIZER_INPUT_YUV,
};
int mtk_resizer_power_on(struct device *dev);
int mtk_resizer_power_off(struct device *dev);
int mtk_resizer_start(const struct device *dev, struct cmdq_pkt **cmdq_handle);
int mtk_resizer_stop(const struct device *dev, struct cmdq_pkt **cmdq_handle);
int mtk_resizer_bypass(const struct device *dev, struct cmdq_pkt **cmdq_handle);
int mtk_resizer_config(const struct device *dev, struct cmdq_pkt **cmdq_handle,
				const struct mtk_resizer_config *config);
int mtk_resizer_config_test(const struct device *dev,
					struct cmdq_pkt **cmdq_handle,
					struct mtk_resizer_config_test *config);
int mtk_resizer_config_quarter(const struct device *dev,
			       struct cmdq_pkt **cmdq_handle,
			       const struct mtk_resizer_quarter_config *config);
int mtk_resizer_out_bit_clip(const struct device *dev,
			     struct cmdq_pkt **cmdq_handle,
			     bool enable, u8 threshold);
int mtk_resizer_register_cb(struct device *dev, mtk_mmsys_cb cb, u32 status,
			    void *priv_data);

#endif

