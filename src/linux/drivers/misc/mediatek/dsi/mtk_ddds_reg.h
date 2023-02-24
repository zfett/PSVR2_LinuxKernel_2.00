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
 * @file mtk_ddds_reg.h
 * Register definition header of mtk_ddds.c
 */

#ifndef __MTK_DDDS_REG_H__
#define __MTK_DDDS_REG_H__

/* ----------------- Register Definitions ------------------- */
#define DDDS_00						0x00000000
	#define DDDS_FREQ_CW				GENMASK(28, 0)
	#define FIX_FS_DDDS_SEL				BIT(29)
	#define H_PRE_LOCK				BIT(30)
	#define DISP_EN					BIT(31)
#define DDDS_01						0x00000004
	#define HLEN_INT				GENMASK(15, 0)
	#define HLEN_NUM				GENMASK(27, 16)
	#define INIT_SEL				GENMASK(31, 28)
#define DDDS_02						0x00000008
	#define RESERVED1				GENMASK(15, 12)
	#define HLEN_DEN				GENMASK(27, 16)
#define DDDS_03						0x0000000c
	#define DDDS_ERR_LIM				GENMASK(8, 0)
	#define SPREAD_INIT				BIT(9)
	#define DATA_SYNC_AUTO				BIT(10)
	#define RESERVED2				BIT(11)
	#define RESERVED3				GENMASK(14, 13)
	#define SC_MJC_TRACK_SEL			BIT(15)
	#define RESERVED4				GENMASK(30, 25)
	#define MUTE_FUNC_OFF				BIT(31)
#define DDDS_0C						0x00000030
	#define HLEN_INT_E1				GENMASK(15, 0)
	#define HLEN_DEN_E1				GENMASK(27, 16)
	#define RESERVED13				GENMASK(29, 28)
	#define VSYNC_TRACK_EN				BIT(30)
	#define BYPASS_VSYN_EN				BIT(31)
#define DDDS_0D						0x00000034
	#define HLEN_NUM_E1				GENMASK(11, 0)
	#define RESERVED14				GENMASK(15, 12)
	#define HLEN_INT_E2				GENMASK(31, 16)
#define DDDS_0E						0x00000038
	#define HLEN_DEN_E2				GENMASK(11, 0)
	#define RESERVED15				GENMASK(15, 12)
	#define HLEN_NUM_E2				GENMASK(27, 16)
	#define RESERVED16				GENMASK(31, 28)
#define DDDS_0F						0x0000003c
	#define HLEN_INT_L1				GENMASK(15, 0)
	#define HLEN_DEN_L1				GENMASK(27, 16)
	#define RESERVED17				GENMASK(31, 28)
#define DDDS_10						0x00000040
	#define HLEN_NUM_L1				GENMASK(11, 0)
	#define RESERVED18				GENMASK(15, 12)
	#define HLEN_INT_L2				GENMASK(31, 16)
#define DDDS_11						0x00000044
	#define HLEN_DEN_L2				GENMASK(11, 0)
	#define RESERVED19				GENMASK(15, 12)
	#define HLEN_NUM_L2				GENMASK(27, 16)
	#define RESERVED20				GENMASK(31, 28)
#define STA_DDDS_00					0x0000004c
	#define DDDS_FREQ_CW_RB				GENMASK(28, 0)
	#define DDDS_LOCK				BIT(30)
#define STA_DDDS_01					0x00000050
	#define ERR_STATUS				GENMASK(28, 0)
#define STA_DDDS_02					0x00000054
	#define FREQ_CW					GENMASK(28, 0)
#define DDDS_16						0x00000058
	#define RESERVED23				GENMASK(29, 17)
	#define DDDS_LOCK_TH				GENMASK(31, 30)
#define DDDS_1C						0x00000070
	#define DDDS_ERR_LIM_LOW			GENMASK(15, 0)
	#define RESERVED28				GENMASK(28, 19)


#endif /*__MTK_DDDS_REG_H__*/
