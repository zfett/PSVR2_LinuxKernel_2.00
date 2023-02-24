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

#ifndef MTK_CROP_H
#define MTK_CROP_H

#include <linux/soc/mediatek/mtk-cmdq.h>

/** default value of active eye number */

int mtk_crop_start(const struct device *dev, struct cmdq_pkt **handle);
int mtk_crop_stop(const struct device *dev, struct cmdq_pkt **handle);
int mtk_crop_reset(const struct device *dev, struct cmdq_pkt **handle);
int mtk_crop_set_region(const struct device *dev, struct cmdq_pkt **handle,
		u32 in_w, u32 in_h, u32 out_w, u32 out_h,
		u32 crop_x, u32 crop_y);
int mtk_crop_power_on(struct device *dev);
int mtk_crop_power_off(struct device *dev);

#endif
