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
 * @file mtk_mmsys_gaze_top.h
 * Header of mtk_mmsys_gaze_top.c
 */
#ifndef MTK_MMSYS_GAZE_TOP_H
#define MTK_MMSYS_GAZE_TOP_H

/** @ingroup IP_group_mmsys_cmmn_top_external_enum
 * @brief mmsys_cmmn_top Reset component ID\n
 * value is from 0 to 6
 * GALS_COMMON1 is for SMI(Smart Multimedia Interface) bridge to\n
 * EMI(External Memory Interface), GALS is short for Global Async\n
 * Local Sync, a circut design.
 * GALS_LARB22 is for SMI_LARB22 bridge to SDRAM.
 */
enum mtk_mmsys_gaze_reset_mod {
	MDP_WPEA_0 = 0,
	WDMA_GAZE,
	GALS_LARB22,
	SMI_LARB22,
	SMI_LARB4,
	DISP_MUTEX,
	GALS_COMMON1,
};

int mtk_mmsys_gaze_top_power_on(struct device *dev);
int mtk_mmsys_gaze_top_power_off(struct device *dev);
int mtk_mmsys_gaze_top_mod_reset(const struct device *dev,
		enum mtk_mmsys_gaze_reset_mod mod);
int mtk_mmsys_gaze_top_get_cksm(const struct device *dev, u32 *checksum);

#endif
