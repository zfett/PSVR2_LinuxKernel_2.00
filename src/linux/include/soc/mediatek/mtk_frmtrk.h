/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk_frmtrk.h
 * Header of mtk_frmtrk.c
 */

#ifndef _MTK_FRMTRK_H_
#define _MTK_FRMTRK_H_

#include <linux/soc/mediatek/mtk-cmdq.h>

bool mtk_frmtrk_locked(const struct device *dev);
int mtk_frmtrk_turbo_window(const struct device *dev, u16 turbo_win,
			    struct cmdq_pkt *handle);
int mtk_frmtrk_lock_window(const struct device *dev, u8 lock_win,
			   struct cmdq_pkt *handle);
int mtk_frmtrk_target_line(const struct device *dev, u16 target_line,
			   struct cmdq_pkt *handle);
int mtk_frmtrk_stop(const struct device *dev);
int mtk_frmtrk_start(const struct device *dev, struct cmdq_pkt *handle);
int mtk_frmtrk_cross_vsync(const struct device *dev, bool enable);
int mtk_frmtrk_set_mask(const struct device *dev, u32 mask, u32 sel,
			struct cmdq_pkt *handle);
int mtk_frmtrk_get_trk_dist(const struct device *dev, u32 *frm_trk_dis);
int mtk_frmtrk_config_vtotal(const struct device *dev, u16 vtotal,
			     struct cmdq_pkt *handle);

#endif
