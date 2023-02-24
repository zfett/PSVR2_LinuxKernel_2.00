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

/**
 * @file mtk_fm.h
 * Header of mtk_fm.c
 */
#ifndef MTK_FM_H
#define MTK_FM_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif


/** @ingroup IP_group_fm_external_struct
 * @brief FM Mask Table Structure
 */
struct mtk_fm_mask_tbl {
	/** table size */
	u32 size;
	/** mask table */
	u32 *mask_tbl;
};

/** @ingroup IP_group_fm_external_struct
 * @brief FM Search Center Structure
 */
struct mtk_fm_search_center {
	/** search center size */
	u32 size;
	/** search center */
	u32 *sc;
};

/** @ingroup IP_group_fm_external_enum
 * @brief FM Search Range Type\n
 * value is from 1 to 2
 */
enum mtk_fm_sr_type {
	/** 1: search range for temporal tracking */
	MTK_FM_SR_TYPE_TEMPORAL_PREDICTION = 1,
	/** 2: search range for spatial */
	MTK_FM_SR_TYPE_SPATIAL = 2,
};

/** @ingroup IP_group_fm_external_enum
 * @brief FM Block Size Type\n
 * value is from 0 to 2
 */
enum mtk_fm_blk_size_type {
	/** 0: block size 32x32 */
	MTK_FM_BLK_SIZE_TYPE_32x32 = 0,
	/** 1: block size 16x16 */
	MTK_FM_BLK_SIZE_TYPE_16x16 = 1,
	/** 2: block size 8x8 */
	MTK_FM_BLK_SIZE_TYPE_8x8 = 2,
};


/** @ingroup IP_group_fm_external_enum
 * @brief FM Input Type\n
 * value is from 0 to 2
 */
enum mtk_fm_in_buf {
	/** 0: input image */
	MTK_FM_IN_BUF_IMG = 0,
	/** 1: input of feature block's descriptors */
	MTK_FM_IN_BUF_DESCRIPTOR = 1,
	/** 2: input of feature block's point */
	MTK_FM_IN_BUF_POINT = 2,
};

/** @ingroup IP_group_fm_external_enum
 * @brief FM Output Type\n
 * value is from 0 to 1
 */
enum mtk_fm_out_buf {
	/** 0: fmo output */
	MTK_FM_OUT_BUF_FMO = 0,
	/** 1: ZNCC subpixels output */
	MTK_FM_OUT_BUF_ZNCC_SUBPIXEL = 1,
};


int mtk_fm_set_multi_in_buf(const struct device *dev,
		struct cmdq_pkt *handle,
		enum mtk_fm_in_buf idx, dma_addr_t addr, u32 pitch);
int mtk_fm_set_multi_out_buf(const struct device *dev,
		struct cmdq_pkt *handle,
		enum mtk_fm_out_buf idx, dma_addr_t addr);
int mtk_fm_get_fm_done(const struct device *dev, u32 *ret);
int mtk_fm_get_dma_idle(const struct device *dev, struct cmdq_pkt *handle);
int mtk_fm_get_mask_tbl(const struct device *dev,
		struct mtk_fm_mask_tbl *mask_tbl);
int mtk_fm_get_search_center(const struct device *dev,
		struct mtk_fm_search_center *sc);

int mtk_fm_set_blk_type(const struct device *dev, struct cmdq_pkt *handle,
		enum mtk_fm_blk_size_type blk_type);
int mtk_fm_set_sr_type(const struct device *dev, struct cmdq_pkt *handle,
		enum mtk_fm_sr_type sr_type);
int mtk_fm_set_mask_tbl(const struct device *dev, struct cmdq_pkt *handle,
		struct mtk_fm_mask_tbl *mask_tbl);
int mtk_fm_set_search_center(const struct device *dev, struct cmdq_pkt *handle,
		struct mtk_fm_search_center *sc);

int mtk_fm_set_mask_tbl_sw_idx(const struct device *dev,
		struct cmdq_pkt *handle, u32 idx);
int mtk_fm_set_mask_tbl_hw_idx(const struct device *dev,
		struct cmdq_pkt *handle, u32 idx);
int mtk_fm_set_zncc_threshold(const struct device *dev, struct cmdq_pkt *handle,
		u32 zncc_threshold);
int mtk_fm_set_region(const struct device *dev, struct cmdq_pkt *handle,
		u32 w, u32 h, u32 blk_nr);
int mtk_fm_set_req_interval(const struct device *dev, struct cmdq_pkt *handle,
	u32 img_req_interval, u32 desc_req_interval, u32 wdma_req_interval,
	u32 point_req_interval, u32 zncc_req_interval);
int mtk_fm_start(const struct device *dev, struct cmdq_pkt *handle);
int mtk_fm_stop(const struct device *dev, struct cmdq_pkt *handle);
int mtk_fm_reset(const struct device *dev);
int mtk_fm_write_register(const struct device *dev, u32 offset, u32 value);
int mtk_fm_read_register(const struct device *dev, u32 offset, u32 *value);
int mtk_fm_power_on(struct device *dev);
int mtk_fm_power_off(struct device *dev);
int mtk_fm_register_cb(const struct device *dev, mtk_mmsys_cb func,
		void *priv_data);
int dump_fm_reg(struct device *dev);
#endif
