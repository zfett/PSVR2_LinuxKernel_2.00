/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Peggy Jao <peggy.jao@mediatek.com>
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

#ifndef MTK_SBRC_H
#define MTK_SBRC_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

int mtk_sbrc_enable(const struct device *dev);
int mtk_sbrc_disable(const struct device *dev);
int mtk_sbrc_drv_init(const struct device *dev);
int mtk_sbrc_set_buffer_num(const struct device *dev, u32 buf_num);
int mtk_sbrc_set_buffer_depth(
	const struct device *dev, u32 frame_height, u32 depth);
int mtk_sbrc_set_buffer_size(const struct device *dev, u32 width);
int mtk_sbrc_clr_bypass(const struct device *dev, struct cmdq_pkt *handle);
#ifdef CONFIG_COMMON_CLK_MT3612
int mtk_sbrc_power_on(struct device *dev);
int mtk_sbrc_power_off(struct device *dev);
#endif

int mtk_sbrc_set_sw_control_read_idx(const struct device *dev, u32 idx,
				     struct cmdq_pkt *handle);
int mtk_sbrc_sw_control_read(const struct device *dev,
				    struct cmdq_pkt *handle);
#endif
