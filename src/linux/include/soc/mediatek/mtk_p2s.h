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

#ifndef MTK_P2S_H
#define MTK_P2S_H

#include <linux/soc/mediatek/mtk-cmdq.h>

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif

/** @ingroup IP_group_p2s_external_struct
 * @brief p2s pattern generator pattern configuration data structure.
 */
struct p2s_pg_pat_color {
	/** Initial value, range from 0 to 4095 */
	u16 init_value;
	/** Vertical addend, range from 0 to 4095 */
	u16 addend;
};

/** @ingroup IP_group_p2s_external_struct
 * @brief P2S pattern generator pattern configuration data structure.
 */
struct p2s_pg_pat_cfg {
	/** Struct for pattern R configuration */
	struct p2s_pg_pat_color color_r;
	/** Struct for pattern G configuration */
	struct p2s_pg_pat_color color_g;
	/** Struct for pattern B configuration */
	struct p2s_pg_pat_color color_b;
	/** Color bar width, range from 0 to 8191 */
	u16 cbar_width;
};

/** @ingroup IP_group_p2s_external_struct
 * @brief P2S pattern generator pattern configuration data structure.
 */
struct p2s_pg_cfg {
	/** Struct for pattern Left configuration */
	struct p2s_pg_pat_cfg left;
	/** Struct for pattern Right configuration */
	struct p2s_pg_pat_cfg right;
	/** pattern width configuration */
	u32 width;
	/** pattern height configuration */
	u32 height;
};

/** @ingroup IP_group_p2s_external_def
 * @brief p2s processing mode
 * @{
 */
#define MTK_P2S_PROC_MODE_L_ONLY	0
#define MTK_P2S_PROC_MODE_LR		1

#define MTK_P2S_UNFINISHED_FRAME	BIT(0)

int mtk_p2s_set_region(const struct device *dev, struct cmdq_pkt *handle,
		       u32 w, u32 h);
int mtk_p2s_patgen_config(const struct device *dev, struct cmdq_pkt *handle,
			   const struct p2s_pg_cfg *cfg);
int mtk_p2s_patgen_enable(const struct device *dev, struct cmdq_pkt *handle);
int mtk_p2s_start(const struct device *dev, struct cmdq_pkt *handle);
int mtk_p2s_stop(const struct device *dev, struct cmdq_pkt *handle);
int mtk_p2s_reset(const struct device *dev, struct cmdq_pkt *handle);
int mtk_p2s_power_on(struct device *dev);
int mtk_p2s_power_off(struct device *dev);
int mtk_p2s_set_proc_mode(const struct device *dev, struct cmdq_pkt *handle,
			  u32 proc_mode);

#endif
