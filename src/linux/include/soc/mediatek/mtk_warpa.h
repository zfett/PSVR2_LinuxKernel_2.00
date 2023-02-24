/*
 * Copyright (c) 2016 MediaTek Inc.
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

#ifndef MTK_WARPA_H
#define MTK_WARPA_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif


/** @ingroup IP_group_warpa_external_def
 * @brief Warpa coefficient table related
 * @{
 */
#define MTK_WARPA_COEF_TABLE_SIZE	65
#define MTK_WARPA_USER_COEF_TABLE	5
/**
 * @}
 */

/** @ingroup IP_group_warpa_external_def
 * @brief Warpa processing mode
 * @{
 */
#define MTK_WARPA_PROC_MODE_L_ONLY	0
#define MTK_WARPA_PROC_MODE_LR		1
#define MTK_WARPA_PROC_MODE_QUAD    3
/**
 * @}
 */

/** @ingroup IP_group_warpa_external_def
 * @brief Warpa output mode
 * @{
 */
#define MTK_WARPA_OUT_MODE_ALL		0
#define MTK_WARPA_OUT_MODE_DRAM		1
#define MTK_WARPA_OUT_MODE_DIRECT_LINK	2
/**
 * @}
 */

/** @ingroup IP_group_warpa_external_struct
 * @brief warpa border color and unmmaped color information
 */
struct mtk_warpa_border_color {
	/**  If enable is true, turn on border color and unmapped color\n
	 * function. Otherwise, turn off the function.
	 */
	bool enable;
	/**  border color. When a output pixel is mapped to where is\n
	 * out of input range, this pixel is filled with border color.\n
	 * The value format is according to output buffer format.
	 */
	u16 border_color;
	/**  unmapped_color. When a output pixel is not in the mapping\n
	 * range, this pixel is filled with unmapped color.\n
	 * The value format is according to output buffer format.
	 */
	u16 unmapped_color;
};

/** @ingroup IP_group_warpa_external_struct
 * @brief warpa coefficient table
 */
struct mtk_warpa_coef_tbl {
	/**  the index of pre-defined coefficient table. If it is\n
	 * MTK_WARPA_USER_COEF_TABLE, use user_coef_tbl.
	 */
	u32 idx;
	/** this is the user-defined coefficient table. */
	u16 user_coef_tbl[MTK_WARPA_COEF_TABLE_SIZE];
};

/** @ingroup IP_group_warpa_external_enum
 * @brief warpa interrupts
 */
enum mtk_warpa_cb_status {
	MTK_WARPA_CB_STATUS_DONE		= BIT(0),
	MTK_WARPA_CB_STATUS_EOF_NOT_IDLE	= BIT(1),
	MTK_WARPA_CB_STATUS_SOF_NOT_IDLE	= BIT(2),
};

int mtk_warpa_start(const struct device *dev, struct cmdq_pkt *handle);
int mtk_warpa_stop(const struct device *dev, struct cmdq_pkt *handle);
int mtk_warpa_reset(const struct device *dev);
int mtk_warpa_set_region_map(const struct device *dev, u32 w, u32 h);
int mtk_warpa_set_region_in(const struct device *dev, u32 w, u32 h);
int mtk_warpa_set_region_out(const struct device *dev, u32 w, u32 h);
int mtk_warpa_set_buf_map_no0(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_map_no1(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_map_no2(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_map_no3(const struct device *dev, struct cmdq_pkt *handle,
			    dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_in_no0(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_in_no1(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_in_no2(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_in_no3(const struct device *dev, struct cmdq_pkt *handle,
			   dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_out_img(const struct device *dev,
			      struct cmdq_pkt *handle,
			      dma_addr_t addr, u32 pitch);
int mtk_warpa_set_buf_out_vld(const struct device *dev,
			      struct cmdq_pkt *handle,
			      dma_addr_t addr, u32 pitch);
int mtk_warpa_power_on(struct device *dev);
int mtk_warpa_power_off(struct device *dev);
int mtk_warpa_get_wpe_done(const struct device *dev);
int mtk_warpa_set_coef_tbl(const struct device *dev, struct cmdq_pkt *handle,
			   struct mtk_warpa_coef_tbl *coef_tbl);
int mtk_warpa_set_border_color(const struct device *dev,
			       struct cmdq_pkt *handle,
			       struct mtk_warpa_border_color *color);
int mtk_warpa_set_proc_mode(const struct device *dev, struct cmdq_pkt *handle,
			    u32 proc_mode);
int mtk_warpa_set_out_mode(const struct device *dev, u32 out_mode);
int mtk_warpa_register_cb(const struct device *dev, mtk_mmsys_cb func,
			  enum mtk_warpa_cb_status status, void *priv_data);

#endif
