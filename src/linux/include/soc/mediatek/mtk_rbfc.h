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
/**
 * @file mtk_rbfc.h
 * Header of mtk_rbfc.c
 */

#ifndef MTK_RBFC_H
#define MTK_RBFC_H

#include <linux/soc/mediatek/mtk-cmdq.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif

/** @ingroup IP_group_rbfc_external_enum
 * @brief RBFC Target Buffer Area\n
 * value is from 0 to 1.
 */
enum MTK_RBFC_TARGET {
/** 0: target buffer in DRAM */
	MTK_RBFC_TO_DRAM = 0,
/** 1: target buffer in SYSRAM */
	MTK_RBFC_TO_SYSRAM,
};

/** @ingroup IP_group_rbfc_external_enum
 * @brief RBFC operation mode\n
 * value is from 0 to 3.
 */
enum MTK_RBFC_MODE {
/** 0: normal mode */
	MTK_RBFC_NORMAL_MODE,
/** 1: FRAME ID mode */
	MTK_RBFC_FID_MODE,
/** 2: disabled mode */
	MTK_RBFC_DISABLE_MODE,
/** 3: bypass mode */
	MTK_RBFC_BYPASS_MODE
};

int mtk_rbfc_power_on(struct device *dev);
int mtk_rbfc_power_off(struct device *dev);
int mtk_rbfc_finish(const struct device *dev);
int mtk_rbfc_set_ring_buf_multi(const struct device *dev,
				struct cmdq_pkt **handle,
				u32 idx, dma_addr_t addr, u32 pitch,
				u32 row_num);
int mtk_rbfc_set_2nd_ring_buf_multi(const struct device *dev,
				    struct cmdq_pkt **handle, u32 idx,
				    dma_addr_t addr, u32 pitch, u32 row_num);
int mtk_rbfc_set_region_multi(const struct device *dev,
			      struct cmdq_pkt **handle,
			      u32 idx, u32 w, u32 h);
int mtk_rbfc_set_target(const struct device *dev, struct cmdq_pkt **handle,
			enum MTK_RBFC_TARGET target);
int mtk_rbfc_set_read_thd(const struct device *dev,
			  struct cmdq_pkt **handle, const u32 *thd);
int mtk_rbfc_set_write_thd(const struct device *dev,
			   struct cmdq_pkt **handle, const u32 *thd);
int mtk_rbfc_set_active_num(const struct device *dev,
			    struct cmdq_pkt **handle, const u32 *active);
int mtk_rbfc_set_row_cnt_init(const struct device *dev,
			      struct cmdq_pkt **handle, const u32 *row_cnt);
int mtk_rbfc_set_w_ov_th(const struct device *dev,
			 struct cmdq_pkt **handle,
			 u16 w_ov_th, bool unmask_only);
int mtk_rbfc_start_mode(const struct device *dev, struct cmdq_pkt **handle,
			enum MTK_RBFC_MODE mode);
int mtk_rbfc_get_w_op_cnt(const struct device *dev, u32 *data);
int mtk_rbfc_get_r_op_cnt(const struct device *dev, u32 *data);
int mtk_rbfc_set_plane_num(const struct device *dev, u8 data);
int mtk_rbfc_set_ufo_mode(const struct device *dev, u8 data);
#endif
