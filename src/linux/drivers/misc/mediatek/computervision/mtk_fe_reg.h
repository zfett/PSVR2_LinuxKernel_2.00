/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Shan Lin <Shan.Lin@mediatek.com>
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
 * @file mtk_fe_reg.h
 * Register definition header of mtk_fe.c
 */

#ifndef __FE_REG_H__
#define __FE_REG_H__

/* ----------------- Register Definitions ------------------- */
#define FE_CTRL1					0x00000000
	#define FE_MODE					GENMASK(1, 0)
	#define FE_PARAM				GENMASK(8, 2)
	#define FE_FLT_EN				BIT(9)
	#define FE_TH_G					GENMASK(17, 10)
	#define FE_TH_C					GENMASK(31, 18)
#define FE_IDX_CTRL					0x00000004
	#define FE_XIDX					GENMASK(13, 0)
	#define FE_YIDX					GENMASK(29, 16)
#define FE_CROP_CTRL1					0x00000008
	#define FE_START_X				GENMASK(13, 0)
	#define FE_START_Y				GENMASK(29, 16)
#define FE_CROP_CTRL2					0x0000000c
	#define FE_CROP_WD				GENMASK(13, 0)
	#define FE_CROP_HT				GENMASK(29, 16)
#define FE_CTRL2					0x00000010
	#define FE_TDR_EDGE				GENMASK(3, 0)
	#define FE_DESCR_EN				BIT(4)
	#define FE_EN					BIT(5)
	#define FE_CR_VALUE_SEL				BIT(6)
	#define FE_MERGE_MODE				GENMASK(8, 7)
#define FE_SIZE						0x00000014
	#define FE_IN_WD				GENMASK(13, 0)
	#define FE_IN_HT				GENMASK(29, 16)
#define FE_RST						0x00000018
	#define DMA_STOP				BIT(0)
	#define DMA_STOP_STATUS				BIT(1)
	#define SW_RST					BIT(4)
	#define FEO_FE_DONE				BIT(8)
	#define FE_ERR_ST				BIT(9)
	#define FE_RDY_ST				BIT(10)
	#define FE_REQ_ST				BIT(11)
#define FE_DBG						0x0000001c
	#define IN_FE_Y_CNT				GENMASK(13, 0)
	#define IN_FE_X_CONT				GENMASK(29, 16)
#define FE_WDMA_CTRL					0x00000020
	#define FE_WDMA_EN				BIT(0)
	#define FE_WDMA_STOP				BIT(1)
	#define FE_WDMA_PRE_EN				BIT(2)
	#define FE_WDMA_BURST_BND128B			BIT(3)
	#define FE_WDMA_ULTRA_EN			BIT(4)
	#define SMI_CRC_RESET				BIT(5)
	#define SMI_CRC_EN				BIT(6)
#define FE_WDMA_BASE_ADDR0_CFG				0x00000024
	#define FE_WDMA_BASE_ADDR0			GENMASK(31, 0)
#define FE_WDMA_BASE_ADDR1_CFG				0x00000028
	#define FE_WDMA_BASE_ADDR1			GENMASK(31, 0)
#define FE_WDMA_BASE_ADDR2_CFG				0x0000002c
	#define FE_WDMA_BASE_ADDR2			GENMASK(31, 0)
#define FE_WDMA_BASE_ADDR3_CFG				0x00000030
	#define FE_WDMA_BASE_ADDR3			GENMASK(31, 0)
#define FE_WDMA_BASE_ADDR4_CFG				0x00000034
	#define FE_WDMA_BASE_ADDR4			GENMASK(31, 0)
#define FE_WDMA_BASE_ADDR5_CFG				0x00000038
	#define FE_WDMA_BASE_ADDR5			GENMASK(31, 0)
#define FE_WDMA_BASE_ADDR6_CFG				0x0000003c
	#define FE_WDMA_BASE_ADDR6			GENMASK(31, 0)
#define FE_WDMA_BASE_ADDR7_CFG				0x00000040
	#define FE_WDMA_BASE_ADDR7			GENMASK(31, 0)
#define FE_WDMA_PRE_TH01				0x00000044
	#define FE_WDMA_PRE_TH_HI1			GENMASK(7, 0)
	#define FE_WDMA_PRE_TH_LO1			GENMASK(15, 8)
	#define FE_WDMA_PRE_TH_HI0			GENMASK(23, 16)
	#define FE_WDMA_PRE_TH_LO0			GENMASK(31, 24)
#define FE_WDMA_PRE_TH23				0x00000048
	#define FE_WDMA_PRE_TH_HI3			GENMASK(7, 0)
	#define FE_WDMA_PRE_TH_LO3			GENMASK(15, 8)
	#define FE_WDMA_PRE_TH_HI2			GENMASK(23, 16)
	#define FE_WDMA_PRE_TH_LO2			GENMASK(31, 24)
#define FE_WDMA_PRE_TH45				0x0000004c
	#define FE_WDMA_PRE_TH_HI5			GENMASK(7, 0)
	#define FE_WDMA_PRE_TH_LO5			GENMASK(15, 8)
	#define FE_WDMA_PRE_TH_HI4			GENMASK(23, 16)
	#define FE_WDMA_PRE_TH_LO4			GENMASK(31, 24)
#define FE_WDMA_PRE_TH67				0x00000050
	#define FE_WDMA_PRE_TH_HI7			GENMASK(7, 0)
	#define FE_WDMA_PRE_TH_LO7			GENMASK(15, 8)
	#define FE_WDMA_PRE_TH_HI6			GENMASK(23, 16)
	#define FE_WDMA_PRE_TH_LO6			GENMASK(31, 24)
#define FE_WDMA_ULTRA_TH01				0x00000054
	#define FE_WDMA_ULTRA_TH_HI1			GENMASK(7, 0)
	#define FE_WDMA_ULTRA_TH_LO1			GENMASK(15, 8)
	#define FE_WDMA_ULTRA_TH_HI0			GENMASK(23, 16)
	#define FE_WDMA_ULTRA_TH_LO0			GENMASK(31, 24)
#define FE_WDMA_ULTRA_TH23				0x00000058
	#define FE_WDMA_ULTRA_TH_HI3			GENMASK(7, 0)
	#define FE_WDMA_ULTRA_TH_LO3			GENMASK(15, 8)
	#define FE_WDMA_ULTRA_TH_HI2			GENMASK(23, 16)
	#define FE_WDMA_ULTRA_TH_LO2			GENMASK(31, 24)
#define FE_WDMA_ULTRA_TH45				0x0000005c
	#define FE_WDMA_ULTRA_TH_HI5			GENMASK(7, 0)
	#define FE_WDMA_ULTRA_TH_LO5			GENMASK(15, 8)
	#define FE_WDMA_ULTRA_TH_HI4			GENMASK(23, 16)
	#define FE_WDMA_ULTRA_TH_LO4			GENMASK(31, 24)
#define FE_WDMA_ULTRA_TH67				0x00000060
	#define FE_WDMA_ULTRA_TH_HI7			GENMASK(7, 0)
	#define FE_WDMA_ULTRA_TH_LO7			GENMASK(15, 8)
	#define FE_WDMA_ULTRA_TH_HI6			GENMASK(23, 16)
	#define FE_WDMA_ULTRA_TH_LO6			GENMASK(31, 24)
#define FE_CRC_SEL0					0x00000064
	#define SMI_CRC_RESULT_PRE			GENMASK(31, 0)
#define FE_CRC_SEL1					0x00000068
	#define SMI_CRC_RESULT_CUR			GENMASK(31, 0)

#endif /*__FE_REG_H__*/
