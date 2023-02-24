/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Shan Lin <Shan.Lin@mediatek.com>
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

#ifndef MTK_FE_H
#define MTK_FE_H

#include <linux/soc/mediatek/mtk-cmdq.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif


/** @ingroup IP_group_fe_external_enum
 * @brief fe output buffer index
 */
enum mtk_fe_out_buf {
	/** 0: feature block's point output */
	MTK_FE_OUT_BUF_POINT = 0,
	/** 1: feature block's descriptor output */
	MTK_FE_OUT_BUF_DESCRIPTOR = 1,
};

/** @ingroup IP_group_fm_external_enum
 * @brief FE input mask type\n
 * value is from 0 to 2
 */
enum mtk_fe_block_size {
	/** 0: fe 32x32 block size */
	MTK_FE_BLK_SZ_32x32 = 0,
	/** 1: fe 16x16 block size */
	MTK_FE_BLK_SZ_16x16 = 1,
	/** 2: fe 8x8 block size */
	MTK_FE_BLK_SZ_8x8 = 2,
};

/** @ingroup IP_group_fm_external_enum
 * @brief FE Input frame merge number \n
 * value is from 0 to 2
 */
enum mtk_fe_frame_merge_mode {
	/** 0: no merge, 1 frame input */
	MTK_FE_MERGE_SINGLE_FRAME = 0,
	/** 1: frame merge, 2 frame input */
	MTK_FE_MERGE_DOUBLE_FRAME = 1,
	/** 2: frame merge, 4 frame input */
	MTK_FE_MERGE_QUAD_FRAME = 2,
	/** 3: dummy index for specify max index */
	MTK_FE_MERGE_MAX = 3,
};

int mtk_fe_start(const struct device *dev, struct cmdq_pkt *handle);
int mtk_fe_stop(const struct device *dev, struct cmdq_pkt *handle);
int mtk_fe_reset(const struct device *dev);
int mtk_fe_set_params(const struct device *dev, struct cmdq_pkt *handle,
		u32 flat_enable, u32 harris_kappa, u32 th_grad, u32 th_cr,
		u32 cr_val_sel_mode);
int mtk_fe_set_region(const struct device *dev, struct cmdq_pkt *handle,
		u32 w, u32 h, enum mtk_fe_frame_merge_mode merge_mode);
int mtk_fe_set_multi_out_buf(const struct device *dev,
		struct cmdq_pkt *handle,
		u32 idx, enum mtk_fe_out_buf type, dma_addr_t addr, u32 pitch);
int mtk_fe_write_register(const struct device *dev, u32 offset, u32 value);
int mtk_fe_read_register(const struct device *dev, u32 offset, u32 *value);
int mtk_fe_power_on(struct device *dev);
int mtk_fe_power_off(struct device *dev);
bool mtk_fe_get_fe_done(const struct device *dev);
int mtk_fe_set_block_size(const struct device *dev, struct cmdq_pkt *handle,
		enum mtk_fe_block_size blk_sz);

#endif
