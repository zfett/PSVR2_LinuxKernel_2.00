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
 * @file mtk_fm_reg.h
 * Register definition header of mtk_fm.c
 */

#ifndef __FM_REG_H__
#define __FM_REG_H__

/* ----------------- Register Definitions ------------------- */
#define FM_THR						0x00000200
	#define FM_FM_TH_ZNCC				GENMASK(14, 0)
#define FM_TYPE						0x00000204
	#define FM_FM_SR_TYPE				GENMASK(2, 1)
	#define FM_FM_BLK_TYPE				GENMASK(4, 3)
#define FM_CTRL0					0x00000208
	#define FM_DRAM_EN				BIT(0)
	#define FM_FM_RESET				BIT(4)
	#define FM_BURST_BND128B			BIT(6)
	#define FM_FM_START_SEL				BIT(7)
	#define FM_IF_END_INT_SEL			GENMASK(9, 8)
	#define FM_PULTRA0_EN				BIT(12)
	#define FM_ULTRA0_EN				BIT(13)
	#define FM_PULTRA1_EN				BIT(14)
	#define FM_ULTRA1_EN				BIT(15)
	#define FM_PULTRA2_EN				BIT(16)
	#define FM_ULTRA2_EN				BIT(17)
	#define FM_PULTRA4_EN				BIT(20)
	#define FM_ULTRA4_EN				BIT(21)
#define FM_CTRL1					0x0000020c
	#define FM_FM_START				BIT(0)
#define FM_INPUT_SIZE					0x00000210
	#define FM_FM_IN_WIDTH				GENMASK(10, 0)
	#define FM_FM_IN_HEIGHT				GENMASK(26, 16)
#define FM_INPUT_INFO					0x00000214
	#define FM_FM_IMG_STRIDE			GENMASK(11, 4)
	#define FM_FM_BLK_NUM				GENMASK(26, 16)
#define FM_RADDR0					0x00000218
	#define FM_FM_IN_ADDR0				GENMASK(31, 4)
#define FM_RADDR1					0x0000021c
	#define FM_FM_IN_ADDR1				GENMASK(31, 4)
#define FM_RADDR2					0x00000220
	#define FM_FM_IN_ADDR2				GENMASK(31, 4)
#define FM_WADDR0					0x00000224
	#define FM_FM_OUT_ADDR0				GENMASK(31, 4)
#define FM_WADDR1					0x00000228
	#define FM_FM_OUT_ADDR1				GENMASK(31, 4)
#define FM_MASK_SRAM_CFG				0x0000022c
	#define FM_MASK_SRAM_PP_HALT			BIT(1)
	#define FM_FORCE_MASK_SRAM_EN			BIT(4)
	#define FM_FORCE_MASK_SRAM_APB			BIT(5)
	#define FM_FORCE_MASK_SRAM_INT			BIT(6)
	#define FM_MASK_SRAM_CTRL_RST			BIT(7)
	#define FM_SC_SRAM_CTRL_RST			BIT(8)
#define FM_MASK_SRAM_STATUS				0x00000230
	#define FM_MASK_SRAM_WRDY			BIT(16)
	#define FM_MASK_SRAM_RRDY			BIT(17)
#define FM_MASK_SRAM_RW_IF_0				0x00000234
	#define FM_MASK_SRAM_WADDR			GENMASK(12, 0)
#define FM_MASK_SRAM_RW_IF_1				0x00000238
	#define FM_MASK_SRAM_WDATA			GENMASK(31, 0)
#define FM_MASK_SRAM_RW_IF_2				0x0000023c
	#define FM_MASK_SRAM_RADDR			GENMASK(12, 0)
#define FM_MASK_SRAM_RW_IF_3				0x00000240
	#define FM_MASK_SRAM_RDATA			GENMASK(31, 0)
#define FM_SC_SRAM_STATUS				0x00000244
	#define FM_SC_SRAM_WRDY				BIT(16)
	#define FM_SC_SRAM_RRDY				BIT(17)
#define FM_SC_SRAM_RW_IF_0				0x00000248
	#define FM_SC_SRAM_WADDR			GENMASK(12, 0)
#define FM_SC_SRAM_RW_IF_1				0x0000024c
	#define FM_SC_SRAM_WDATA			GENMASK(31, 0)
#define FM_SC_SRAM_RW_IF_2				0x00000250
	#define FM_SC_SRAM_RADDR			GENMASK(12, 0)
#define FM_SC_SRAM_RW_IF_3				0x00000254
	#define FM_SC_SRAM_RDATA			GENMASK(31, 0)
#define FM_INTEN					0x0000025c
	#define FM_IF_END_INT_EN			BIT(0)
	#define FM_OF_END_INT_EN			BIT(1)
#define FM_INTSTA					0x00000260
	#define FM_IF_END_INT				BIT(0)
	#define FM_OF_END_INT				BIT(1)
#define FM_STATUS					0x00000264
	#define FM_WDMA_IDLE				BIT(0)
	#define FM_RDMA0_IDLE				BIT(1)
	#define FM_RDMA1_IDLE				BIT(2)
	#define FM_RDMA2_IDLE				BIT(3)
	#define FM_FM_ACTIVE				BIT(4)
	#define FM_WDMA_SUBPIX_IDLE			BIT(5)
#define FM_SMI_CONF0					0x00000270
	#define FM_REQ0_INTERVAL			GENMASK(15, 0)
	#define FM_REQ1_INTERVAL			GENMASK(31, 16)
#define FM_SMI_CONF1					0x00000274
	#define FM_REQ2_INTERVAL			GENMASK(15, 0)
	#define FM_REQ3_INTERVAL			GENMASK(31, 16)
#define FM_SMI_CONF2					0x00000278
	#define FM_PULTRA0_H				GENMASK(2, 0)
	#define FM_PULTRA0_L				GENMASK(6, 4)
	#define FM_ULTRA0_H				GENMASK(10, 8)
	#define FM_ULTRA0_L				GENMASK(14, 12)
	#define FM_PULTRA1_H				GENMASK(18, 16)
	#define FM_PULTRA1_L				GENMASK(22, 20)
	#define FM_ULTRA1_H				GENMASK(26, 24)
	#define FM_ULTRA1_L				GENMASK(30, 28)
#define FM_SMI_CONF3					0x0000027c
	#define FM_PULTRA2_H				GENMASK(2, 0)
	#define FM_PULTRA2_L				GENMASK(6, 4)
	#define FM_ULTRA2_H				GENMASK(10, 8)
	#define FM_ULTRA2_L				GENMASK(14, 12)
#define FM_SMI_CONF4					0x00000280
	#define FM_REQ4_INTERVAL			GENMASK(15, 0)
	#define FM_PULTRA4_H				GENMASK(18, 16)
	#define FM_PULTRA4_L				GENMASK(22, 20)
	#define FM_ULTRA4_H				GENMASK(26, 24)
	#define FM_ULTRA4_L				GENMASK(30, 28)

#endif /*__FM_REG_H__*/
