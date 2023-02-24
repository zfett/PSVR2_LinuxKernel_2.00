/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef _MTK_SYSRAM_SMI_COMMON_REG_H_
#define _MTK_SYSRAM_SMI_COMMON_REG_H_

/* -------- sysram2_sys2x1_smi_common_u0_reg Register Definitions -------- */
#define SMI_L1LEN					0x00000100
	#define M4U_ARB_WDATA				BIT(0)
	#define BW_OVSHT_CTRL				GENMASK(2, 1)
	#define FLOW_CTRL				BIT(3)
#define SMI_L1ARB0					0x00000104
	#define MAX_GNT_CNT				GENMASK(11, 0)
	#define BW_FILTER_EN				BIT(12)
	#define BW_HFILTER_EN				BIT(13)
	#define OUTS_LIMIT_EN				BIT(14)
	#define OUTS_RD_LIMIT_TH			GENMASK(21, 16)
	#define OUTS_LIMIT_RSOFT_EN			BIT(23)
	#define OUTS_WR_LIMIT_TH			GENMASK(29, 24)
	#define OUTS_LIMIT_WSOFT_EN			BIT(31)
#define SMI_L1ARB1					0x00000108
	#define MAX_GNT_CNT				GENMASK(11, 0)
	#define BW_FILTER_EN				BIT(12)
	#define BW_HFILTER_EN				BIT(13)
	#define OUTS_LIMIT_EN				BIT(14)
	#define OUTS_RD_LIMIT_TH			GENMASK(21, 16)
	#define OUTS_LIMIT_RSOFT_EN			BIT(23)
	#define OUTS_WR_LIMIT_TH			GENMASK(29, 24)
	#define OUTS_LIMIT_WSOFT_EN			BIT(31)
#define SMI_MON_AXI_ENA					0x000001a0
	#define MONAXIENA				BIT(0)
#define SMI_MON_AXI_CLR					0x000001a4
	#define MONAXICLR				BIT(0)
#define SMI_MON_AXI_TYPE				0x000001ac
	#define MON_TYPE_RW_SEL				BIT(0)
#define SMI_MON_AXI_CON					0x000001b0
	#define AXI_BUS_SELECT				GENMASK(7, 4)
#define SMI_MON_AXI_ACT_CNT				0x000001c0
	#define ACT_CNT					GENMASK(27, 0)
#define SMI_MON_AXI_REQ_CNT				0x000001c4
	#define REQ_CNT					GENMASK(27, 0)
#define SMI_MON_AXI_BEA_CNT				0x000001cc
	#define BEA_CNT					GENMASK(27, 0)
#define SMI_MON_AXI_BYT_CNT				0x000001d0
	#define BYT_CNT					GENMASK(27, 0)
#define SMI_WRR_REG0					0x00000228
	#define SMI_WRR_REG0_0				GENMASK(5, 0)
	#define SMI_WRR_REG0_1				GENMASK(11, 6)
#define SMI_READ_FIFO_TH				0x00000230
	#define SLICE_TH				GENMASK(1, 0)
	#define OUTS_LIMITER_OF_EACH_AXI_CH		GENMASK(13, 8)
	#define NON_ULTRA_OUTS_LIMITER_OF_EACH_AXI_CH	GENMASK(21, 16)
	#define RD_FLUSH_ENTER_CMD_THROT_EN		BIT(24)
	#define CH_OUTS_LIMITER_EN			BIT(25)
	#define STD_AXI_MODE				BIT(26)
	#define ARB_ORI					BIT(27)
	#define W_ULTRA_WRR				BIT(28)
	#define R_ULTRA_WRR				BIT(29)
	#define W_GCLAST_EN				BIT(30)
	#define R_GCLAST_EN				BIT(31)
#define SMI_M4U_TH					0x00000234
	#define TOTAL_CMD_NUM_M1			GENMASK(5, 0)
	#define TOTAL_NONULTRA_CMD_NUM_M1		GENMASK(13, 8)
#define SMI_FIFO_TH1					0x00000238
	#define TOTAL_CMD_NUM_RD_M1			GENMASK(5, 0)
	#define TOTAL_NONULTRA_CMD_NUM_RD_M1		GENMASK(13, 8)
	#define TOTAL_CMD_NUM_WR_M1			GENMASK(21, 16)
	#define TOTAL_NONULTRA_CMD_NUM_WR_M1		GENMASK(29, 24)
#define SMI_DCM						0x00000300
	#define SMI_STALL_DIS				GENMASK(7, 1)
	#define SMI_STALL_CNT				GENMASK(13, 8)
#define SMI_M1_RULTRA_WRR0				0x00000308
	#define SMI_WRR_REG0_0				GENMASK(5, 0)
	#define SMI_WRR_REG0_1				GENMASK(11, 6)
#define SMI_M1_WULTRA_WRR0				0x00000310
	#define SMI_WRR_REG0_0				GENMASK(5, 0)
	#define SMI_WRR_REG0_1				GENMASK(11, 6)
#define SMI_COMMON_CLAMP_EN				0x000003c0
#define SMI_COMMON_CLAMP_EN_SET				0x000003c4
#define SMI_COMMON_CLAMP_EN_CLR				0x000003c8

#endif /* #ifndef _MTK_SYSRAM_SMI_COMMON_REG_H_ */
