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
 * @file mtk_mmsys_cmmn_top.h
 * Header of mtk_mmsys_cmmn_top.c
 */
#ifndef MTK_MMSYS_CMMN_TOP_H
#define MTK_MMSYS_CMMN_TOP_H

#include <linux/platform_device.h>

/** @ingroup IP_group_mmsys_cmmn_top_external_enum
 * @brief mmsys_cmmn_top memory type ID\n
 * value is from 0 to 1
 */
enum mtk_mmsys_cmmn_mem_type {
	MTK_MMSYS_CMMN_DRAM = 0,
	MTK_MMSYS_CMMN_SYSRAM,
};

/** @ingroup IP_group_mmsys_cmmn_top_external_enum
 * @brief mmsys_cmmn_top token ID\n
 * value is from 0 to 10
 * MTK_MMSYS_CMMN_FM_IMG:read image
 * MTK_MMSYS_CMMN_FM_FD: read FE descriptor
 * MTK_MMSYS_CMMN_FM_FP: read FE point
 * MTK_MMSYS_CMMN_FM_FMO: FM output WDMA
 * MTK_MMSYS_CMMN_FM_ZNCC: FM output WDMA for Zero Mean Normalized\n
 * Cross-Correlation.
 */

enum mtk_mmsys_cmmn_token {
	MTK_MMSYS_CMMN_WDMA_0 = 0,
	MTK_MMSYS_CMMN_WDMA_1,
	MTK_MMSYS_CMMN_WDMA_2,
	MTK_MMSYS_CMMN_WPE_1_RDMA,
	MTK_MMSYS_CMMN_WPE_1_WDMA,

	MTK_MMSYS_CMMN_FM_IMG,
	MTK_MMSYS_CMMN_FM_FD,
	MTK_MMSYS_CMMN_FM_FP,
	MTK_MMSYS_CMMN_FM_FMO,
	MTK_MMSYS_CMMN_FM_ZNCC,
	MTK_MMSYS_CMMN_FE,
};

/** @ingroup IP_group_mmsys_cmmn_top_external_enum
 * @brief mmsys_cmmn_top Component ID\n
 * value is from 0 to 6
 */
enum mtk_mmsys_cmmn_mod {
	MTK_MMSYS_CMMN_MOD_WPEA = 0,
	MTK_MMSYS_CMMN_MOD_FE,
	MTK_MMSYS_CMMN_MOD_WDMA,
	MTK_MMSYS_CMMN_MOD_PADDING_0,
	MTK_MMSYS_CMMN_MOD_RSZ_0,
	MTK_MMSYS_CMMN_MOD_PADDING_1,
	MTK_MMSYS_CMMN_MOD_PADDING_2,
};

/** @ingroup IP_group_mmsys_cmmn_top_external_enum
 * @brief mmsys_cmmn_top Reset component ID\n
 */
enum mtk_mmsys_cmmn_reset_mod {
	padding_0 = 0,
	padding_1 = 1,
	MDP_WPEA_1 = 2,
	MDP_FE = 3,
	padding_2 = 4,
	DISP_WDMA_2 = 5,
	DISP_WDMA_3 =  6,
	DISP_WDMA_1 = 7,
	MDP_FM = 8,
	RESIZER_0 = 14,
	RESIZER_1 = 15,
	DISP_MUTEX = 16,
};

int mtk_mmsys_cmmn_top_power_on(struct device *dev);
int mtk_mmsys_cmmn_top_power_off(struct device *dev);
int mtk_mmsys_cmmn_top_conn(const struct device *dev,
		enum mtk_mmsys_cmmn_mod from_mod,
		enum mtk_mmsys_cmmn_mod to_mod);
int mtk_mmsys_cmmn_top_disconn(const struct device *dev,
		enum mtk_mmsys_cmmn_mod from_mod,
		enum mtk_mmsys_cmmn_mod to_mod);
int mtk_mmsys_cmmn_top_sel_mem(const struct device *dev,
	enum mtk_mmsys_cmmn_token token,
	enum mtk_mmsys_cmmn_mem_type type);
int mtk_mmsys_cmmn_top_mod_reset(const struct device *dev,
		enum mtk_mmsys_cmmn_reset_mod mod);
#endif
