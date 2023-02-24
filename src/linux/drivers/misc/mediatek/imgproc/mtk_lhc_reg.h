/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Houlong Wei <houlong.wei@mediatek.com>
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
 * @file mtk_lhc_reg.h
 * Register definition header of mtk_lhc.c
 */
#ifndef __LHC_REG_H__
#define __LHC_REG_H__

/* ----------------- Register Definitions ------------------- */
#define LHC_EN						0x00000000
	#define REG_LHC_EN				BIT(0)
#define LHC_RESET					0x00000004
	#define REG_LHC_RESET				BIT(0)
#define LHC_INTEN					0x00000008
	#define FRAME_DONE_INT_EN			BIT(0)
	#define READ_FINISH_INT_EN			BIT(1)
	#define IF_UNFINISH_INT_EN			BIT(2)
	#define OF_UNFINISH_INT_EN			BIT(3)
	#define SRAM_RW_ERR_INT_EN			BIT(4)
	#define APB_RW_ERR_INT_EN			BIT(5)
#define LHC_INTSTA					0x0000000c
	#define FRAME_DONE_INT_STA			BIT(0)
	#define READ_FINISH_INT_STA			BIT(1)
	#define IF_UNFINISH_INT_STA			BIT(2)
	#define OF_UNFINISH_INT_STA			BIT(3)
	#define SRAM_RW_ERR_INT_STA			BIT(4)
	#define APB_RW_ERR_INT_STA			BIT(5)
#define LHC_STATUS					0x00000010
	#define HANDSHAKE				GENMASK(13, 2)
#define LHC_CFG						0x00000014
	#define RELAY_MODE				BIT(0)
	#define LHC_CG_DISABLE				BIT(1)
	#define LHC_INPUT_SWAP				BIT(3)
#define LHC_INPUT_COUNT					0x00000018
	#define INP_PIX_CNT				GENMASK(12, 0)
	#define INP_LINE_CNT				GENMASK(28, 16)
#define LHC_OUTPUT_COUNT				0x0000001c
	#define OUTP_PIX_CNT				GENMASK(12, 0)
	#define OUTP_LINE_CNT				GENMASK(28, 16)
#define LHC_SIZE					0x00000024
	#define VSIZE					GENMASK(12, 0)
	#define HSIZE					GENMASK(28, 16)
#define LHC_DUMMY_REG					0x00000028
	#define DUMMY_REG				GENMASK(31, 0)
#define LHC_DRE_BLOCK_INFO_00				0x0000002c
	#define ACT_WIN_X_START				GENMASK(12, 0)
	#define ACT_WIN_X_END				GENMASK(28, 16)
#define LHC_DRE_BLOCK_INFO_01				0x00000030
	#define DRE_BLK_X_NUM				GENMASK(4, 0)
	#define DRE_BLK_Y_NUM				GENMASK(20, 16)
#define LHC_DRE_BLOCK_INFO_02				0x00000034
	#define DRE_BLK_WIDTH				GENMASK(12, 0)
	#define DRE_BLK_HEIGHT				GENMASK(28, 16)
#define LHC_Y2R_00					0x00000038
	#define Y2R_PRE_ADD_0_S				GENMASK(10, 0)
	#define Y2R_PRE_ADD_1_S				GENMASK(26, 16)
#define LHC_Y2R_01					0x0000003c
	#define Y2R_PRE_ADD_2_S				GENMASK(10, 0)
	#define Y2R_C00_S				GENMASK(31, 16)
#define LHC_Y2R_02					0x00000040
	#define Y2R_C01_S				GENMASK(15, 0)
	#define Y2R_C02_S				GENMASK(31, 16)
#define LHC_Y2R_03					0x00000044
	#define Y2R_C10_S				GENMASK(15, 0)
	#define Y2R_C11_S				GENMASK(31, 16)
#define LHC_Y2R_04					0x00000048
	#define Y2R_C12_S				GENMASK(15, 0)
	#define Y2R_C20_S				GENMASK(31, 16)
#define LHC_Y2R_05					0x0000004c
	#define Y2R_C21_S				GENMASK(15, 0)
	#define Y2R_C22_S				GENMASK(31, 16)
#define LHC_Y2R_06					0x00000050
	#define Y2R_POST_ADD_0_S			GENMASK(10, 0)
	#define Y2R_POST_ADD_1_S			GENMASK(26, 16)
#define LHC_Y2R_07					0x00000054
	#define Y2R_EN					BIT(0)
	#define Y2R_INT_TABLE_SEL			GENMASK(3, 1)
	#define Y2R_EXT_TABLE_EN			BIT(4)
	#define Y2R_POST_ADD_2_S			GENMASK(26, 16)
#define LHC_SRAM_CFG					0x00000058
	#define FORCE_HIST_SRAM_EN			BIT(0)
	#define FORCE_HIST_SRAM_APB			BIT(1)
	#define FORCE_HIST_SRAM_INT			BIT(2)
	#define SRAM_RREQ_EN				GENMASK(5, 3)
	#define SRAM_WREQ_EN				GENMASK(8, 6)
#define LHC_SRAM_STATUS					0x0000005c
	#define RB_HIST_SRAM_IDX			BIT(0)
	#define HIST_SRAM_WRDY_R			BIT(16)
	#define HIST_SRAM_RRDY_R			BIT(17)
	#define HIST_SRAM_WRDY_G			BIT(18)
	#define HIST_SRAM_RRDY_G			BIT(19)
	#define HIST_SRAM_WRDY_B			BIT(20)
	#define HIST_SRAM_RRDY_B			BIT(21)
#define LHC_SRAM_RW_IF_0_0				0x00000060
	#define HIST_SRAM_WADDR_R			GENMASK(12, 0)
#define LHC_SRAM_RW_IF_0_1				0x00000064
	#define HIST_SRAM_WDATA_R			GENMASK(31, 0)
#define LHC_SRAM_RW_IF_0_2				0x00000068
	#define HIST_SRAM_RADDR_R			GENMASK(12, 0)
#define LHC_SRAM_RW_IF_0_3				0x0000006c
	#define HIST_SRAM_RDATA_R			GENMASK(31, 0)
#define LHC_SRAM_RW_IF_1_0				0x00000070
	#define HIST_SRAM_WADDR_G			GENMASK(12, 0)
#define LHC_SRAM_RW_IF_1_1				0x00000074
	#define HIST_SRAM_WDATA_G			GENMASK(31, 0)
#define LHC_SRAM_RW_IF_1_2				0x00000078
	#define HIST_SRAM_RADDR_G			GENMASK(12, 0)
#define LHC_SRAM_RW_IF_1_3				0x0000007c
	#define HIST_SRAM_RDATA_G			GENMASK(31, 0)
#define LHC_SRAM_RW_IF_2_0				0x00000080
	#define HIST_SRAM_WADDR_B			GENMASK(12, 0)
#define LHC_SRAM_RW_IF_2_1				0x00000084
	#define HIST_SRAM_WDATA_B			GENMASK(31, 0)
#define LHC_SRAM_RW_IF_2_2				0x00000088
	#define HIST_SRAM_RADDR_B			GENMASK(12, 0)
#define LHC_SRAM_RW_IF_2_3				0x0000008c
	#define HIST_SRAM_RDATA_B			GENMASK(31, 0)
#define LHC_SRAM_READ_CNT				0x00000090
	#define HIST_SRAM_TOTAL_CNT			GENMASK(15, 0)
	#define HIST_SRAM_READ_CNT_CORE_SEL		GENMASK(17, 16)
#define LHC_SRAM_READ_CNT_STA				0x00000094
	#define HIST_SRAM_READ_CNT_STA			GENMASK(15, 0)
#define LHC_ATPG					0x00000098
#define LHC_SHADOW_CTRL					0x0000009c

#endif /*__LHC_REG_H__*/
