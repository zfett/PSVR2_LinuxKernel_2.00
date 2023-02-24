/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *     Mandy Liu <mandyjh.liu@mediatek.com>
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
 * @file mtk_mmsys_cmmn_top_reg.h
 * Register definition header of mtk_mmsys_cmmn_top.c
 */

#ifndef __MTK_MMSYS_CMMN_TOP_REG_H__
#define __MTK_MMSYS_CMMN_TOP_REG_H__

/* ----------------- Register Definitions ------------------- */
#define MMSYS_SW0_RST_B				0x00000170

#define MDP_RSZ_0_MOUT_EN			0x00000488
	#define RSZ_0_2_2ND_PADDING		BIT(0)/*3612 2ND PADDING*/
	#define RSZ_0_2_3RD_PADDING		BIT(1)/*3612 3RD PADDING*/
#define WPEA_1_MOUT_EN				0x0000048c
	#define WPEA_2_FE			BIT(0)
	#define WPEA_2_WDMA			BIT(1)
	#define WPEA_2_PADDING			BIT(2)
#define MMSYS_COMMON_FM_SPLITTER		0X00000044
	#define MMSYS_COMMON_FM_SPLITTER_MMSYS_COMMON_FM_SPLITTER GENMASK(4, 0)
	#define FM_IMG				BIT(0)
	#define FM_FMO				BIT(1)
	#define FM_FD				BIT(2)
	#define FM_FP				BIT(3)
	#define FM_ZNCC				BIT(4)
#define MMSYS_COMMON_SPLITTER			0X0000004C
	#define MMSYS_COMMON_SPLITTER_MMSYS_COMMON_SPLITTER GENMASK(6, 0)
	#define FE				BIT(0)
	#define WDMA_0				BIT(1)
	#define WDMA_1				BIT(2)
	#define WDMA_2				BIT(3)
	#define WPE_1_RDMA			BIT(5)
	#define WPE_1_WDMA			BIT(6)

#endif /*__MTK_MMSYS_CMMN_TOP_REG_H__*/
