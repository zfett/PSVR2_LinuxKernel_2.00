/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Wilson Huang <wilson.huang@mediatek.com>
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
 * @file mtk_slicer_reg.h
 * Slicer register definition
 */

#ifndef __SLCR_REG_H__
#define __SLCR_REG_H__

/** @ingroup IP_group_slicer_internal_def
 * @brief Slicer register definition
 * @{
 */
/* ----------------- Register Definitions ------------------- */
#define SLCR_ENABLE					0x00000000
	#define SLCR_EN					BIT(0)
#define SLCR_RESET					0x00000004
	#define SLCR_RST				BIT(0)
#define SLCR_DCM_0					0x00000010
	#define VID_DCM_EN				BIT(0)
	#define DSC_DCM_EN				BIT(1)
	#define PATG_DCM_EN				BIT(2)
	#define DITHER_DCM_EN				BIT(5)
	#define CONVERSION_DCM_EN			BIT(6)
	#define SRAM_DCM_EN				BIT(7)
	#define DBG_DCM_EN				BIT(8)
	#define RS_DCM_EN				BIT(9)
	#define FDONE_DCM_EN				BIT(10)
	#define GCE_DCM_EN				BIT(11)
	#define IRQ_DCM_EN				BIT(12)
	#define CRC_DCM_EN				BIT(13)
#define SLCR_CTRL_0					0x00000014
	#define SRAM_SHARE				BIT(0)
#define SLCR_3D_STA_0					0x0000001c
	#define STA_3D_LR_FLAG				BIT(0)
#define SLCR_3D_CTRL_0					0x00000020
	#define RS_EN					BIT(0)
#define SLCR_3D_CTRL_1					0x00000024
	#define RS_MODE					BIT(0)
#define SLCR_3D_CTRL_2					0x00000028
	#define RS_VSYNC				GENMASK(12, 0)
	#define RS_VFP					GENMASK(28, 16)
#define SLCR_3D_CTRL_3					0x0000002c
	#define RS_VACTV				GENMASK(12, 0)
	#define RS_VBP					GENMASK(28, 16)
#define SLCR_VID_CTRL_0					0x00000040
	#define VID_EN					BIT(0)
#define SLCR_VID_CTRL_1					0x00000044
	#define VID_FWIDTH				GENMASK(12, 0)
	#define VID_FHEIGHT				GENMASK(28, 16)
#define SLCR_VID_CTRL_2					0x00000048
	#define VID_IN_INV_HS				BIT(4)
	#define VID_IN_INV_VS				BIT(5)
	#define VID_OUT_INV_HS				BIT(8)
	#define VID_OUT_INV_VS				BIT(9)
#define SLCR_VID_CTRL_3					0x0000004c
	#define VID_IN_2CS_CFG				BIT(0)
	#define VID_IN_3CS_CFG_0			GENMASK(5, 4)
	#define VID_IN_3CS_CFG_1			GENMASK(9, 8)
	#define VID_IN_3CS_CFG_2			GENMASK(13, 12)
	#define VID_OUT_3CS_CFG_0			GENMASK(17, 16)
	#define VID_OUT_3CS_CFG_1			GENMASK(21, 20)
	#define VID_OUT_3CS_CFG_2			GENMASK(25, 24)
	#define VID_SYNC_OUT				BIT(28)
#define SLCR_VID_CTRL_4					0x00000050
	#define VID_0_SOP				GENMASK(12, 0)
	#define VID_0_EOP				GENMASK(28, 16)
#define SLCR_VID_CTRL_5					0x00000054
	#define VID_1_SOP				GENMASK(12, 0)
	#define VID_1_EOP				GENMASK(28, 16)
#define SLCR_VID_CTRL_6					0x00000058
	#define VID_2_SOP				GENMASK(12, 0)
	#define VID_2_EOP				GENMASK(28, 16)
#define SLCR_VID_CTRL_7					0x0000005c
	#define VID_3_SOP				GENMASK(12, 0)
	#define VID_3_EOP				GENMASK(28, 16)
#define SLCR_VID_CTRL_8					0x00000060
	#define VID_0_EN				GENMASK(3, 0)
	#define VID_1_EN				GENMASK(7, 4)
	#define VID_2_EN				GENMASK(11, 8)
	#define VID_3_EN				GENMASK(15, 12)
#define SLCR_DSC_CTRL_0					0x00000070
	#define DSC_EN					BIT(0)
#define SLCR_DSC_CTRL_1					0x00000074
	#define DSC_FWIDTH				GENMASK(12, 0)
	#define DSC_FHEIGHT				GENMASK(28, 16)
#define SLCR_DSC_CTRL_2					0x00000078
	#define DSC_IN_INV_HS				BIT(4)
	#define DSC_IN_INV_VS				BIT(5)
	#define DSC_OUT_INV_HS				BIT(8)
	#define DSC_OUT_INV_VS				BIT(9)
	#define DSC_LIT					BIT(12)
	#define DSC_CHUNK_NUM				GENMASK(22, 20)
	#define DSC_BIT_RATE				GENMASK(29, 24)
#define SLCR_DSC_CTRL_3					0x0000007c
	#define DSC_IN_2CS_CFG				BIT(0)
	#define DSC_VALID_BYTE				GENMASK(21, 16)
	#define DSC_SYNC_OUT				BIT(28)
#define SLCR_DSC_CTRL_4					0x00000080
	#define DSC_0_SOP				GENMASK(12, 0)
	#define DSC_0_EOP				GENMASK(28, 16)
#define SLCR_DSC_CTRL_5					0x00000084
	#define DSC_1_SOP				GENMASK(12, 0)
	#define DSC_1_EOP				GENMASK(28, 16)
#define SLCR_DSC_CTRL_6					0x00000088
	#define DSC_2_SOP				GENMASK(12, 0)
	#define DSC_2_EOP				GENMASK(28, 16)
#define SLCR_DSC_CTRL_7					0x0000008c
	#define DSC_3_SOP				GENMASK(12, 0)
	#define DSC_3_EOP				GENMASK(28, 16)
#define SLCR_DSC_CTRL_8					0x00000090
	#define DSC_0_EN				GENMASK(3, 0)
	#define DSC_1_EN				GENMASK(7, 4)
	#define DSC_2_EN				GENMASK(11, 8)
	#define DSC_3_EN				GENMASK(15, 12)
#define SLCR_DITHER_0					0x000000b0
	#define ROUNDED_EN				BIT(0)
	#define RDITHER_EN				BIT(4)
	#define LFSR_EN					BIT(8)
	#define EDITHER_EN				BIT(12)
#define SLCR_DITHER_1					0x000000b4
	#define SHIFT_EN				BIT(0)
	#define DRMOD_R					GENMASK(5, 4)
	#define DRMOD_G					GENMASK(9, 8)
	#define DRMOD_B					GENMASK(13, 12)
	#define SUBPIX_EN				BIT(16)
	#define SUBPIX_R				GENMASK(21, 20)
	#define SUBPIX_G				GENMASK(25, 24)
	#define SUBPIX_B				GENMASK(29, 28)
#define SLCR_DITHER_2					0x000000b8
	#define LEFT_EN					BIT(0)
	#define LSB_OFF					BIT(4)
	#define INK					BIT(8)
	#define INK_DATA_R				GENMASK(27, 16)
#define SLCR_DITHER_3					0x000000bc
	#define INK_DATA_G				GENMASK(11, 0)
	#define INK_DATA_B				GENMASK(27, 16)
#define SLCR_DITHER_4					0x000000c0
	#define SEED					GENMASK(23, 0)
	#define INIT					BIT(24)
#define SLCR_DITHER_5					0x000000c4
	#define FPHASE_EN				BIT(0)
	#define FPHASE_R				BIT(4)
	#define FPHASE					GENMASK(13, 8)
	#define TABLE_EN				GENMASK(17, 16)
	#define FPHASE_CTRL				GENMASK(21, 20)
	#define FPHASE_SEL				GENMASK(25, 24)
	#define FPHASE_BIT				GENMASK(30, 28)
#define SLCR_DITHER_6					0x000000c8
#define SLCR_DITHER_7					0x000000cc
#define SLCR_CSC_0					0x000000d0
	#define CONVERSION_EN				BIT(0)
	#define EXT_MATRIX_EN				BIT(4)
	#define INT_MATRIX_SEL				GENMASK(19, 16)
#define SLCR_CSC_1					0x000000d4
	#define EXT_C_00				GENMASK(14, 0)
	#define EXT_C_01				GENMASK(30, 16)
#define SLCR_CSC_2					0x000000d8
	#define EXT_C_02				GENMASK(14, 0)
	#define EXT_C_10				GENMASK(30, 16)
#define SLCR_CSC_3					0x000000dc
	#define EXT_C_11				GENMASK(14, 0)
	#define EXT_C_12				GENMASK(30, 16)
#define SLCR_CSC_4					0x000000e0
	#define EXT_C_20				GENMASK(14, 0)
	#define EXT_C_21				GENMASK(30, 16)
#define SLCR_CSC_5					0x000000e4
	#define EXT_C_22				GENMASK(14, 0)
#define SLCR_CSC_6					0x000000e8
	#define EXT_IN_OFFSET_0				GENMASK(10, 0)
	#define EXT_IN_OFFSET_1				GENMASK(26, 16)
#define SLCR_CSC_7					0x000000ec
	#define EXT_IN_OFFSET_2				GENMASK(10, 0)
	#define EXT_OUT_OFFSET_0			GENMASK(26, 16)
#define SLCR_CSC_8					0x000000f0
	#define EXT_OUT_OFFSET_1			GENMASK(10, 0)
	#define EXT_OUT_OFFSET_2			GENMASK(26, 16)
#define SLCR_PATG_0					0x00000100
	#define PATG_EN					BIT(0)
#define SLCR_PATG_1					0x00000104
	#define PATG_RST				BIT(0)
#define SLCR_PATG_2					0x00000108
	#define PATG_MODE				GENMASK(1, 0)
	#define PATG_SW_TRIGGER				BIT(4)
	#define PATG_SW_CLEAR				BIT(5)
	#define PATG_SW_VID_FDONE			BIT(6)
	#define PATG_SW_DSC_FDONE			BIT(7)
	#define PATG_CHUNK_NUM				GENMASK(22, 20)
	#define PATG_BIT_RATE				GENMASK(29, 24)
#define SLCR_PATG_3					0x0000010c
	#define PATG_TIMING_PARA			GENMASK(3, 0)
	#define PATG_H_POLARITY				BIT(8)
	#define PATG_V_POLARITY				BIT(12)
#define SLCR_PATG_4					0x00000110
	#define PATG_H_SYNC				GENMASK(12, 0)
	#define PATG_V_SYNC				GENMASK(28, 16)
#define SLCR_PATG_5					0x00000114
	#define PATG_H_BACK				GENMASK(12, 0)
	#define PATG_V_BACK				GENMASK(28, 16)
#define SLCR_PATG_6					0x00000118
	#define PATG_H_ACTV				GENMASK(12, 0)
	#define PATG_V_ACTV				GENMASK(28, 16)
#define SLCR_PATG_7					0x0000011c
	#define PATG_H_FROT				GENMASK(12, 0)
	#define PATG_V_FROT				GENMASK(28, 16)
#define SLCR_PATG_8					0x00000120
	#define PATG_R_CBAR_WIDTH			GENMASK(12, 0)
	#define PATG_R_CBAR_HEIGHT			GENMASK(28, 16)
#define SLCR_PATG_9					0x00000124
	#define PATG_R_INIT				GENMASK(11, 0)
#define SLCR_PATG_10					0x00000128
	#define PATG_R_H_ADDEND				GENMASK(11, 0)
	#define PATG_R_V_ADDEND				GENMASK(27, 16)
#define SLCR_PATG_12					0x00000130
	#define PATG_G_CBAR_WIDTH			GENMASK(12, 0)
	#define PATG_G_CBAR_HEIGHT			GENMASK(28, 16)
#define SLCR_PATG_13					0x00000134
	#define PATG_G_INIT				GENMASK(11, 0)
#define SLCR_PATG_14					0x00000138
	#define PATG_G_H_ADDEND				GENMASK(11, 0)
	#define PATG_G_V_ADDEND				GENMASK(27, 16)
#define SLCR_PATG_16					0x00000140
	#define PATG_B_CBAR_WIDTH			GENMASK(12, 0)
	#define PATG_B_CBAR_HEIGHT			GENMASK(28, 16)
#define SLCR_PATG_17					0x00000144
	#define PATG_B_INIT				GENMASK(11, 0)
#define SLCR_PATG_18					0x00000148
	#define PATG_B_H_ADDEND				GENMASK(11, 0)
	#define PATG_B_V_ADDEND				GENMASK(27, 16)
#define SLCR_PATG_19					0x0000014c
	#define PATG_STA_H_POLARITY			BIT(8)
	#define PATG_STA_V_POLARITY			BIT(12)
#define SLCR_PATG_20					0x00000150
	#define PATG_STA_H_SYNC				GENMASK(12, 0)
	#define PATG_STA_V_SYNC				GENMASK(28, 16)
#define SLCR_PATG_21					0x00000154
	#define PATG_STA_H_BACK				GENMASK(12, 0)
	#define PATG_STA_V_BACK				GENMASK(28, 16)
#define SLCR_PATG_22					0x00000158
	#define PATG_STA_H_ACTV				GENMASK(12, 0)
	#define PATG_STA_V_ACTV				GENMASK(28, 16)
#define SLCR_PATG_23					0x0000015c
	#define PATG_STA_H_FROT				GENMASK(12, 0)
	#define PATG_STA_V_FROT				GENMASK(28, 16)
#define SLCR_GCE_0					0x00000170
	#define GCE_VID_EVENT_SEL			GENMASK(3, 0)
#define SLCR_GCE_1					0x00000174
	#define GCE_VID_WIDTH				GENMASK(12, 0)
	#define GCE_VID_HEIGHT				GENMASK(28, 16)
#define SLCR_GCE_2					0x00000180
	#define GCE_DSC_EVENT_SEL			GENMASK(3, 0)
#define SLCR_GCE_3					0x00000184
	#define GCE_DSC_WIDTH				GENMASK(12, 0)
	#define GCE_DSC_HEIGHT				GENMASK(28, 16)
#define SLCR_IRQ_EN_0					0x000001a0
	#define IRQ_VID_BUF_OVFL_0_EN			BIT(0)
	#define IRQ_VID_BUF_OVFL_1_EN			BIT(1)
	#define IRQ_VID_BUF_OVFL_2_EN			BIT(2)
	#define IRQ_VID_BUF_OVFL_3_EN			BIT(3)
	#define IRQ_VID_OSIZE_0_EN			BIT(4)
	#define IRQ_VID_OSIZE_1_EN			BIT(5)
	#define IRQ_VID_OSIZE_2_EN			BIT(6)
	#define IRQ_VID_OSIZE_3_EN			BIT(7)
	#define IRQ_VID_ISIZE_EN			BIT(8)
	#define IRQ_VID_HSYNC_POLA_EN			BIT(12)
	#define IRQ_VID_VSYNC_POLA_EN			BIT(16)
	#define IRQ_VID_GCE_EN				BIT(20)
	#define IRQ_VID_FDONE_0_EN			BIT(24)
	#define IRQ_VID_FDONE_1_EN			BIT(25)
	#define IRQ_VID_FDONE_2_EN			BIT(26)
	#define IRQ_VID_FDONE_3_EN			BIT(27)
#define SLCR_IRQ_STA_0					0x000001a4
	#define IRQ_VID_BUF_OVFL_0_STA			BIT(0)
	#define IRQ_VID_BUF_OVFL_1_STA			BIT(1)
	#define IRQ_VID_BUF_OVFL_2_STA			BIT(2)
	#define IRQ_VID_BUF_OVFL_3_STA			BIT(3)
	#define IRQ_VID_OSIZE_0_STA			BIT(4)
	#define IRQ_VID_OSIZE_1_STA			BIT(5)
	#define IRQ_VID_OSIZE_2_STA			BIT(6)
	#define IRQ_VID_OSIZE_3_STA			BIT(7)
	#define IRQ_VID_ISIZE_STA			BIT(8)
	#define IRQ_VID_HSYNC_POLA_STA			BIT(12)
	#define IRQ_VID_VSYNC_POLA_STA			BIT(16)
	#define IRQ_VID_GCE_STA				BIT(20)
	#define IRQ_VID_FDONE_0_STA			BIT(24)
	#define IRQ_VID_FDONE_1_STA			BIT(25)
	#define IRQ_VID_FDONE_2_STA			BIT(26)
	#define IRQ_VID_FDONE_3_STA			BIT(27)
#define SLCR_IRQ_RST_0					0x000001a8
	#define IRQ_VID_BUF_OVFL_0_RST			BIT(0)
	#define IRQ_VID_BUF_OVFL_1_RST			BIT(1)
	#define IRQ_VID_BUF_OVFL_2_RST			BIT(2)
	#define IRQ_VID_BUF_OVFL_3_RST			BIT(3)
	#define IRQ_VID_OSIZE_0_RST			BIT(4)
	#define IRQ_VID_OSIZE_1_RST			BIT(5)
	#define IRQ_VID_OSIZE_2_RST			BIT(6)
	#define IRQ_VID_OSIZE_3_RST			BIT(7)
	#define IRQ_VID_ISIZE_RST			BIT(8)
	#define IRQ_VID_HSYNC_POLA_RST			BIT(12)
	#define IRQ_VID_VSYNC_POLA_RST			BIT(16)
	#define IRQ_VID_GCE_RST				BIT(20)
	#define IRQ_VID_FDONE_0_RST			BIT(24)
	#define IRQ_VID_FDONE_1_RST			BIT(25)
	#define IRQ_VID_FDONE_2_RST			BIT(26)
	#define IRQ_VID_FDONE_3_RST			BIT(27)
#define SLCR_IRQ_EN_1					0x000001b0
	#define IRQ_DSC_BUF_OVFL_0_EN			BIT(0)
	#define IRQ_DSC_BUF_OVFL_1_EN			BIT(1)
	#define IRQ_DSC_BUF_OVFL_2_EN			BIT(2)
	#define IRQ_DSC_BUF_OVFL_3_EN			BIT(3)
	#define IRQ_DSC_OSIZE_0_EN			BIT(4)
	#define IRQ_DSC_OSIZE_1_EN			BIT(5)
	#define IRQ_DSC_OSIZE_2_EN			BIT(6)
	#define IRQ_DSC_OSIZE_3_EN			BIT(7)
	#define IRQ_DSC_ISIZE_EN			BIT(8)
	#define IRQ_DSC_HSYNC_POLA_EN			BIT(12)
	#define IRQ_DSC_VSYNC_POLA_EN			BIT(16)
	#define IRQ_DSC_GCE_EN				BIT(20)
	#define IRQ_DSC_FDONE_0_EN			BIT(24)
	#define IRQ_DSC_FDONE_1_EN			BIT(25)
	#define IRQ_DSC_FDONE_2_EN			BIT(26)
	#define IRQ_DSC_FDONE_3_EN			BIT(27)
#define SLCR_IRQ_STA_1					0x000001b4
	#define IRQ_DSC_BUF_OVFL_0_STA			BIT(0)
	#define IRQ_DSC_BUF_OVFL_1_STA			BIT(1)
	#define IRQ_DSC_BUF_OVFL_2_STA			BIT(2)
	#define IRQ_DSC_BUF_OVFL_3_STA			BIT(3)
	#define IRQ_DSC_OSIZE_0_STA			BIT(4)
	#define IRQ_DSC_OSIZE_1_STA			BIT(5)
	#define IRQ_DSC_OSIZE_2_STA			BIT(6)
	#define IRQ_DSC_OSIZE_3_STA			BIT(7)
	#define IRQ_DSC_ISIZE_STA			BIT(8)
	#define IRQ_DSC_HSYNC_POLA_STA			BIT(12)
	#define IRQ_DSC_VSYNC_POLA_STA			BIT(16)
	#define IRQ_DSC_GCE_STA				BIT(20)
	#define IRQ_DSC_FDONE_0_STA			BIT(24)
	#define IRQ_DSC_FDONE_1_STA			BIT(25)
	#define IRQ_DSC_FDONE_2_STA			BIT(26)
	#define IRQ_DSC_FDONE_3_STA			BIT(27)
#define SLCR_IRQ_RST_1					0x000001b8
	#define IRQ_DSC_BUF_OVFL_0_RST			BIT(0)
	#define IRQ_DSC_BUF_OVFL_1_RST			BIT(1)
	#define IRQ_DSC_BUF_OVFL_2_RST			BIT(2)
	#define IRQ_DSC_BUF_OVFL_3_RST			BIT(3)
	#define IRQ_DSC_OSIZE_0_RST			BIT(4)
	#define IRQ_DSC_OSIZE_1_RST			BIT(5)
	#define IRQ_DSC_OSIZE_2_RST			BIT(6)
	#define IRQ_DSC_OSIZE_3_RST			BIT(7)
	#define IRQ_DSC_ISIZE_RST			BIT(8)
	#define IRQ_DSC_HSYNC_POLA_RST			BIT(12)
	#define IRQ_DSC_VSYNC_POLA_RST			BIT(16)
	#define IRQ_DSC_GCE_RST				BIT(20)
	#define IRQ_DSC_FDONE_0_RST			BIT(24)
	#define IRQ_DSC_FDONE_1_RST			BIT(25)
	#define IRQ_DSC_FDONE_2_RST			BIT(26)
	#define IRQ_DSC_FDONE_3_RST			BIT(27)
#define SLCR_CRC_0					0x000001c0
	#define CRC_EN					BIT(0)
#define SLCR_CRC_1					0x000001c4
	#define CRC_RST					BIT(0)
#define SLCR_CRC_4					0x000001d0
	#define CRC_VID_NUM				GENMASK(31, 0)
#define SLCR_CRC_5					0x000001d4
	#define CRC_VID_VAL				GENMASK(31, 0)
#define SLCR_CRC_6					0x000001d8
	#define CRC_DSC_NUM				GENMASK(31, 0)
#define SLCR_CRC_7					0x000001dc
	#define CRC_DSC_VAL				GENMASK(31, 0)
#define SLCR_DMYL_0					0x000001e0
	#define VID_0_DMYL				GENMASK(12, 0)
	#define VID_0_DMYL_DATA_R			GENMASK(25, 16)
#define SLCR_DMYL_1					0x000001e4
	#define VID_0_DMYL_DATA_G			GENMASK(9, 0)
	#define VID_0_DMYL_DATA_B			GENMASK(25, 16)
#define SLCR_DMYL_2					0x000001e8
	#define VID_1_DMYL				GENMASK(12, 0)
	#define VID_1_DMYL_DATA_R			GENMASK(25, 16)
#define SLCR_DMYL_3					0x000001ec
	#define VID_1_DMYL_DATA_G			GENMASK(9, 0)
	#define VID_1_DMYL_DATA_B			GENMASK(25, 16)
#define SLCR_DMYL_4					0x000001f0
	#define VID_2_DMYL				GENMASK(12, 0)
	#define VID_2_DMYL_DATA_R			GENMASK(25, 16)
#define SLCR_DMYL_5					0x000001f4
	#define VID_2_DMYL_DATA_G			GENMASK(9, 0)
	#define VID_2_DMYL_DATA_B			GENMASK(25, 16)
#define SLCR_DMYL_6					0x000001f8
	#define VID_3_DMYL				GENMASK(12, 0)
	#define VID_3_DMYL_DATA_R			GENMASK(25, 16)
#define SLCR_DMYL_7					0x000001fc
	#define VID_3_DMYL_DATA_G			GENMASK(9, 0)
	#define VID_3_DMYL_DATA_B			GENMASK(25, 16)
#define SLCR_DBG_EN					0x00000200
	#define DBG_EN					BIT(0)
#define SLCR_DBG_RST					0x00000204
	#define DBG_RST					BIT(0)
#define SLCR_DBG_CLK					0x00000208
	#define DBG_VID_FREE_RUN			GENMASK(7, 0)
	#define DBG_DSC_FREE_RUN			GENMASK(15, 8)
	#define DBG_OUT_FREE_RUN			GENMASK(23, 16)
	#define DBG_APB_FREE_RUN			GENMASK(31, 24)
#define SLCR_DBG_VID_0					0x00000210
	#define DBG_VID_IN_HSIZE			GENMASK(12, 0)
	#define DBG_VID_IN_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_VID_1					0x00000214
	#define DBG_VID_IN_HCNT				GENMASK(12, 0)
	#define DBG_VID_IN_VCNT				GENMASK(28, 16)
#define SLCR_DBG_VID_2					0x00000218
	#define DBG_VID_IN_IF_DE			BIT(0)
	#define DBG_VID_IN_IF_HS			BIT(4)
	#define DBG_VID_IN_IF_VS			BIT(8)
#define SLCR_DBG_VID_3					0x0000021c
	#define DBG_VID_MONITOR_IN_SEL			GENMASK(3, 0)
	#define DBG_VID_3D_HADDEND			GENMASK(28, 16)
#define SLCR_DBG_VID_4					0x00000220
	#define DBG_VID_PQ_HSIZE			GENMASK(12, 0)
	#define DBG_VID_PQ_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_VID_5					0x00000224
	#define DBG_VID_PQ_HCNT				GENMASK(12, 0)
	#define DBG_VID_PQ_VCNT				GENMASK(28, 16)
#define SLCR_DBG_VID_6					0x00000228
	#define DBG_VID_PQ_IF_DE			BIT(0)
	#define DBG_VID_PQ_IF_HS			BIT(4)
	#define DBG_VID_PQ_IF_VS			BIT(8)
#define SLCR_DBG_VID_12					0x00000240
	#define DBG_VID_OUT_0_HSIZE			GENMASK(12, 0)
	#define DBG_VID_OUT_0_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_VID_13					0x00000244
	#define DBG_VID_OUT_1_HSIZE			GENMASK(12, 0)
	#define DBG_VID_OUT_1_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_VID_14					0x00000248
	#define DBG_VID_OUT_2_HSIZE			GENMASK(12, 0)
	#define DBG_VID_OUT_2_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_VID_15					0x0000024c
	#define DBG_VID_OUT_3_HSIZE			GENMASK(12, 0)
	#define DBG_VID_OUT_3_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_VID_16					0x00000250
	#define DBG_VID_OUT_0_HCNT			GENMASK(12, 0)
	#define DBG_VID_OUT_0_VCNT			GENMASK(28, 16)
#define SLCR_DBG_VID_17					0x00000254
	#define DBG_VID_OUT_1_HCNT			GENMASK(12, 0)
	#define DBG_VID_OUT_1_VCNT			GENMASK(28, 16)
#define SLCR_DBG_VID_18					0x00000258
	#define DBG_VID_OUT_2_HCNT			GENMASK(12, 0)
	#define DBG_VID_OUT_2_VCNT			GENMASK(28, 16)
#define SLCR_DBG_VID_19					0x0000025c
	#define DBG_VID_OUT_3_HCNT			GENMASK(12, 0)
	#define DBG_VID_OUT_3_VCNT			GENMASK(28, 16)
#define SLCR_DBG_VID_20					0x00000260
	#define DBG_VID_OUT_0_IF_RDY			BIT(0)
	#define DBG_VID_OUT_0_IF_VLD			BIT(4)
#define SLCR_DBG_VID_21					0x00000264
	#define DBG_VID_OUT_1_IF_RDY			BIT(0)
	#define DBG_VID_OUT_1_IF_VLD			BIT(4)
#define SLCR_DBG_VID_22					0x00000268
	#define DBG_VID_OUT_2_IF_RDY			BIT(0)
	#define DBG_VID_OUT_2_IF_VLD			BIT(4)
#define SLCR_DBG_VID_23					0x0000026c
	#define DBG_VID_OUT_3_IF_RDY			BIT(0)
	#define DBG_VID_OUT_3_IF_VLD			BIT(4)
#define SLCR_DBG_DSC_0					0x00000270
	#define DBG_DSC_IN_HSIZE			GENMASK(12, 0)
	#define DBG_DSC_IN_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_DSC_1					0x00000274
	#define DBG_DSC_IN_HCNT				GENMASK(12, 0)
	#define DBG_DSC_IN_VCNT				GENMASK(28, 16)
#define SLCR_DBG_DSC_2					0x00000278
	#define DBG_DSC_IN_IF_DE			BIT(0)
	#define DBG_DSC_IN_IF_HS			BIT(4)
	#define DBG_DSC_IN_IF_VS			BIT(8)
#define SLCR_DBG_DSC_3					0x0000027c
	#define DBG_DSC_MONITOR_IN_SEL			GENMASK(3, 0)
#define SLCR_DBG_DSC_4					0x00000280
	#define DBG_DSC_PQ_HSIZE			GENMASK(12, 0)
	#define DBG_DSC_PQ_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_DSC_5					0x00000284
	#define DBG_DSC_PQ_HCNT				GENMASK(12, 0)
	#define DBG_DSC_PQ_VCNT				GENMASK(28, 16)
#define SLCR_DBG_DSC_6					0x00000288
	#define DBG_DSC_PQ_IF_DE			BIT(0)
	#define DBG_DSC_PQ_IF_HS			BIT(4)
	#define DBG_DSC_PQ_IF_VS			BIT(8)
#define SLCR_DBG_DSC_12					0x000002a0
	#define DBG_DSC_OUT_0_HSIZE			GENMASK(12, 0)
	#define DBG_DSC_OUT_0_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_DSC_13					0x000002a4
	#define DBG_DSC_OUT_1_HSIZE			GENMASK(12, 0)
	#define DBG_DSC_OUT_1_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_DSC_14					0x000002a8
	#define DBG_DSC_OUT_2_HSIZE			GENMASK(12, 0)
	#define DBG_DSC_OUT_2_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_DSC_15					0x000002ac
	#define DBG_DSC_OUT_3_HSIZE			GENMASK(12, 0)
	#define DBG_DSC_OUT_3_VSIZE			GENMASK(28, 16)
#define SLCR_DBG_DSC_16					0x000002b0
	#define DBG_DSC_OUT_0_HCNT			GENMASK(12, 0)
	#define DBG_DSC_OUT_0_VCNT			GENMASK(28, 16)
#define SLCR_DBG_DSC_17					0x000002b4
	#define DBG_DSC_OUT_1_HCNT			GENMASK(12, 0)
	#define DBG_DSC_OUT_1_VCNT			GENMASK(28, 16)
#define SLCR_DBG_DSC_18					0x000002b8
	#define DBG_DSC_OUT_2_HCNT			GENMASK(12, 0)
	#define DBG_DSC_OUT_2_VCNT			GENMASK(28, 16)
#define SLCR_DBG_DSC_19					0x000002bc
	#define DBG_DSC_OUT_3_HCNT			GENMASK(12, 0)
	#define DBG_DSC_OUT_3_VCNT			GENMASK(28, 16)
#define SLCR_DBG_DSC_20					0x000002c0
	#define DBG_DSC_OUT_0_IF_RDY			BIT(0)
	#define DBG_DSC_OUT_0_IF_VLD			BIT(4)
#define SLCR_DBG_DSC_21					0x000002c4
	#define DBG_DSC_OUT_1_IF_RDY			BIT(0)
	#define DBG_DSC_OUT_1_IF_VLD			BIT(4)
#define SLCR_DBG_DSC_22					0x000002c8
	#define DBG_DSC_OUT_2_IF_RDY			BIT(0)
	#define DBG_DSC_OUT_2_IF_VLD			BIT(4)
#define SLCR_DBG_DSC_23					0x000002cc
	#define DBG_DSC_OUT_3_IF_RDY			BIT(0)
	#define DBG_DSC_OUT_3_IF_VLD			BIT(4)
#define SLCR_DBG_OUT_0					0x000002f0
	#define DBG_OUT_SEL				GENMASK(7, 0)
/**
 * @}
 */
#endif /*__SLCR_REG_H__*/
