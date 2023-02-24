/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	leon liang <leon.liang@mediatek.com>
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
 * @file mtk_p2s_reg.h
 * Register definition header of mtk_p2s.c
 */

#ifndef __MTK_P2S_REG_H__
#define __MTK_P2S_REG_H__

/* ----------------- Register Definitions ------------------- */
#define CP2S_ENABLE					0x00000000
	#define CP2S_EN					BIT(0)
#define CP2S_RESET					0x00000004
	#define CP2S_RST				BIT(0)
#define CP2S_DCM_0					0x00000008
	#define LEFT_DCM_EN				BIT(0)
	#define RIGHT_DCM_EN				BIT(1)
	#define INOUT_DCM_EN				BIT(2)
	#define FDONE_DCM_EN				BIT(3)
	#define PATG_DCM_EN				BIT(4)
#define CP2S_CTRL_0					0x00000010
	#define FWIDTH_0				GENMASK(12, 0)
	#define LEFT_EN					BIT(16)
#define CP2S_CTRL_1					0x00000014
	#define FWIDTH_1				GENMASK(12, 0)
	#define RIGHT_EN				BIT(16)
#define CP2S_CTRL_2					0x00000018
	#define FHEIGHT					GENMASK(12, 0)
	#define SRAM_SHARE				BIT(16)
#define CP2S_PATG_EN				0x00000020
	#define PATG_EN					BIT(0)
#define CP2S_PATG_RST				0x00000024
	#define PATG_RST				BIT(0)
#define CP2S_PATG_0					0x00000030
	#define PATG_MODE				GENMASK(1, 0)
	#define PATG_SW_TRIGGER				BIT(4)
	#define PATG_0_FRAME_DONE			BIT(16)
	#define PATG_1_FRAME_DONE			BIT(20)
	#define PATG_0_FRAME_DONE_CLR			BIT(24)
	#define PATG_1_FRAME_DONE_CLR			BIT(28)
#define CP2S_PATG_1					0x00000034
	#define PATG_0_FWIDTH				GENMASK(12, 0)
#define CP2S_PATG_2					0x00000038
	#define PATG_1_FWIDTH				GENMASK(12, 0)
#define CP2S_PATG_3					0x0000003c
	#define PATG_FHEIGHT				GENMASK(12, 0)
#define CP2S_PATG_4					0x00000040
	#define PATG_0_BAR_WIDTH			GENMASK(12, 0)
	#define PATG_1_BAR_WIDTH			GENMASK(28, 16)
#define CP2S_PATG_5					0x00000044
	#define PATG_0_DATA_R_INIT			GENMASK(9, 0)
	#define PATG_0_DATA_R_ADDEND			GENMASK(25, 16)
#define CP2S_PATG_6					0x00000048
	#define PATG_0_DATA_G_INIT			GENMASK(9, 0)
	#define PATG_0_DATA_G_ADDEND			GENMASK(25, 16)
#define CP2S_PATG_7					0x0000004c
	#define PATG_0_DATA_B_INIT			GENMASK(9, 0)
	#define PATG_0_DATA_B_ADDEND			GENMASK(25, 16)
#define CP2S_PATG_8					0x00000050
	#define PATG_1_DATA_R_INIT			GENMASK(9, 0)
	#define PATG_1_DATA_R_ADDEND			GENMASK(25, 16)
#define CP2S_PATG_9					0x00000054
	#define PATG_1_DATA_G_INIT			GENMASK(9, 0)
	#define PATG_1_DATA_G_ADDEND			GENMASK(25, 16)
#define CP2S_PATG_10					0x00000058
	#define PATG_1_DATA_B_INIT			GENMASK(9, 0)
	#define PATG_1_DATA_B_ADDEND			GENMASK(25, 16)
#define CP2S_DBG_0					0x00000060
	#define DBG_IN_0_H_SIZE				GENMASK(12, 0)
	#define DBG_IN_0_V_SIZE				GENMASK(28, 16)
#define CP2S_DBG_1					0x00000064
	#define DBG_IN_0_H_COUNT			GENMASK(12, 0)
	#define DBG_IN_0_V_COUNT			GENMASK(28, 16)
#define CP2S_DBG_2					0x00000068
	#define DBG_IN_0_IF_DATA_R			GENMASK(9, 0)
	#define DBG_IN_0_IF_VLD				BIT(24)
	#define DBG_IN_0_IF_RDY				BIT(28)
#define CP2S_DBG_4					0x00000070
	#define DBG_IN_1_H_SIZE				GENMASK(12, 0)
	#define DBG_IN_1_V_SIZE				GENMASK(28, 16)
#define CP2S_DBG_5					0x00000074
	#define DBG_IN_1_H_COUNT			GENMASK(12, 0)
	#define DBG_IN_1_V_COUNT			GENMASK(28, 16)
#define CP2S_DBG_6					0x00000078
	#define DBG_IN_1_IF_DATA_R			GENMASK(9, 0)
	#define DBG_IN_1_IF_VLD				BIT(24)
	#define DBG_IN_1_IF_RDY				BIT(28)
#define CP2S_DBG_8					0x00000080
	#define DBG_OUT_H_SIZE				GENMASK(12, 0)
	#define DBG_OUT_V_SIZE				GENMASK(28, 16)
#define CP2S_DBG_9					0x00000084
	#define DBG_OUT_H_COUNT				GENMASK(12, 0)
	#define DBG_OUT_V_COUNT				GENMASK(28, 16)
#define CP2S_DBG_10					0x00000088
	#define DBG_OUT_IF_DATA_R			GENMASK(9, 0)
	#define DBG_OUT_IF_VLD				BIT(24)
	#define DBG_OUT_IF_RDY				BIT(28)

#endif /*__MTK_P2S_REG_H__*/
