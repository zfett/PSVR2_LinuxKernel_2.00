/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Peng Liu <peng.liu@mediatek.com>
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

#ifndef MTK_PADDING_H
#define MTK_PADDING_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

/** @ingroup IP_group_padding_external_enum
 * @brief padding mode index
 */
enum mtk_padding_mode {
	/** 0: padding 3 times in 1/4,2/4,3/4 */
	MTK_PADDING_MODE_QUARTER = 0,
	/** 1: padding 1 times in 2/4 */
	MTK_PADDING_MODE_HALF = 1,
	/** 2: padding mode error setting */
	MTK_PADDING_MODE_MAX = 2,
};

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
#endif

int mtk_padding_start(const struct device *dev, struct cmdq_pkt *handle);
int mtk_padding_stop(const struct device *dev, struct cmdq_pkt *handle);
int mtk_padding_set_bypass(const struct device *dev,
			struct cmdq_pkt *handle, bool bypass);
int mtk_padding_reset(const struct device *dev, struct cmdq_pkt *handle);
int mtk_padding_config(const struct device *dev, struct cmdq_pkt *handle,
		enum mtk_padding_mode mode, u32 num, u32 width, u32 height);
int mtk_padding_power_on(struct device *dev);
int mtk_padding_power_off(struct device *dev);
int mtk_padding_register_cb(const struct device *dev, mtk_mmsys_cb func,
			u32 status, void *priv_data);
#endif
