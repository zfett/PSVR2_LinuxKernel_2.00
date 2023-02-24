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
 * @file mtk_dsi_reg.h
 * Register definition header of mtk_dsi.c
 */

#ifndef __MTK_DSI_REG_H__
#define __MTK_DSI_REG_H__

/* ----------------- Register Definitions ------------------- */
#define DSI_START					0x00000000
	#define SLEEPOUT_START				BIT(2)
	#define SKEWCAL_START				BIT(4)
	#define VM_CMD_START				BIT(16)
#define DSI_STATUS					0x00000004
	#define ESC_ENTRY_ERR				BIT(4)
	#define ESC_SYNC_ERR				BIT(5)
	#define FALSE_CTRL_ERR				BIT(6)
	#define CONTENTION_ERR0				BIT(8)
	#define CONTENTION_ERR1				BIT(9)
	#define HW_MUTE_STATE				BIT(12)
	#define HW_MUTE_ON				BIT(13)
#define DSI_INTEN					0x00000008
	#define LPRX_RD_RDY_INT_EN			BIT(0)
	#define CMD_DONE_INT_EN				BIT(1)
	#define TE_RDY_INT_EN				BIT(2)
	#define VM_DONE_INT_EN				BIT(3)
	#define FRAME_DONE_INT_EN			BIT(4)
	#define VM_CMD_DONE_INT_EN			BIT(5)
	#define SLEEPOUT_DONE_INT_EN			BIT(6)
	#define TE_TIMEOUT_INT_EN			BIT(7)
	#define VM_VBP_STR_INT_EN			BIT(8)
	#define VM_VACT_STR_INT_EN			BIT(9)
	#define VM_VFP_STR_INT_EN			BIT(10)
	#define SKEWCAL_DONE_INT_EN			BIT(11)
	#define BUFFER_UNDERRUN_INT_EN			BIT(12)
	#define ERRCONTENTIONLP0_INT_EN			BIT(13)
	#define ERRCONTENTIONLP1_INT_EN			BIT(14)
	#define DSI_EOF_INT_EN				BIT(15)
	#define LPRX_RD_RDY_EVENT_EN			BIT(16)
	#define CMD_DONE_EVENT_EN			BIT(17)
	#define TE_RDY_EVENT_EN				BIT(18)
	#define VM_DONE_EVENT_EN			BIT(19)
	#define FRAME_DONE_EVENT_EN			BIT(20)
	#define VM_CMD_DONE_EVENT_EN			BIT(21)
	#define SLEEPOUT_DONE_EVENT_EN			BIT(22)
	#define TE_TIMEOUT_EVENT_EN			BIT(23)
	#define VM_VBP_STR_EVENT_EN			BIT(24)
	#define VM_VACT_STR_EVENT_EN			BIT(25)
	#define VM_VFP_STR_EVENT_EN			BIT(26)
	#define SKEWCAL_DONE_EVENT_EN			BIT(27)
	#define BUFFER_UNDERRUN_EVENT_EN		BIT(28)
	#define ERRCONTENTIONLP0_EVENT_EN		BIT(29)
	#define ERRCONTENTIONLP1_EVENT_EN		BIT(30)
	#define DSI_EOF_EVENT_EN			BIT(31)
#define DSI_INTSTA					0x0000000c
	#define LPRX_RD_RDY_INT_FLAG			BIT(0)
	#define CMD_DONE_INT_FLAG			BIT(1)
	#define TE_RDY_INT_FLAG				BIT(2)
	#define VM_DONE_INT_FLAG			BIT(3)
	#define FRAME_DONE_INT_FLAG			BIT(4)
	#define VM_CMD_DONE_INT_FLAG			BIT(5)
	#define SLEEPOUT_DONE_INT_FLAG			BIT(6)
	#define TE_TIMEOUT_INT_FLAG			BIT(7)
	#define VM_VBP_STR_INT_FLAG			BIT(8)
	#define VM_VACT_STR_INT_FLAG			BIT(9)
	#define VM_VFP_STR_INT_FLAG			BIT(10)
	#define SKEWCAL_DONE_INT_FLAG			BIT(11)
	#define BUFFER_UNDERRUN_INT_FLAG		BIT(12)
	#define ERRCONTENTIONLP0_INT_FLAG		BIT(13)
	#define ERRCONTENTIONLP1_INT_FLAG		BIT(14)
	#define DSI_EOF_INT_FLAG			BIT(15)
	#define HW_MUTE_ON_INT_FLAG			BIT(16)
	#define HW_MUTE_OFF_INT_FLAG			BIT(17)
	#define DSI_BUSY				BIT(31)
#define DSI_COM_CON					0x00000010
	#define DSI_RESET				BIT(0)
	#define DPHY_RESET				BIT(2)
	#define DSI_DUAL_EN				BIT(4)
	#define DSI_RESET_FF				BIT(8)
	#define DPHY_RST_DSIRST_DIS			BIT(12)
#define DSI_MODE_CON					0x00000014
	#define MODE_CON				GENMASK(1, 0)
	#define FRAME_MODE				BIT(16)
	#define MIX_MODE				BIT(17)
	#define V2C_SWITCH_ON				BIT(18)
	#define C2V_SWITCH_ON				BIT(19)
	#define SLEEP_MODE				BIT(20)
#define DSI_TXRX_CON					0x00000018
	#define VC_NUM					GENMASK(1, 0)
	#define LANE_NUM				GENMASK(5, 2)
	#define HSTX_DIS_EOT				BIT(6)
	#define HSTX_BLLP_EN				BIT(7)
	#define TE_FREERUN				BIT(8)
	#define EXT_TE_EN				BIT(9)
	#define EXT_TE_EDGE_SEL				BIT(10)
	#define TE_AUTO_SYNC				BIT(11)
	#define MAX_RTN_SIZE				GENMASK(15, 12)
	#define HSTX_CKLP_EN				BIT(16)
	#define TYPE1_BTA_SEL				BIT(17)
	#define TE_WITH_CMD_EN				BIT(18)
	#define TE_TIMEOUT_CHK_EN			BIT(19)
	#define EXT_TE_TIME_VM				GENMASK(23, 20)
	#define LP_ONLY_VBLK				BIT(28)
#define DSI_PSCON					0x0000001c
	#define DSI_PS_WC				GENMASK(14, 0)
	#define DSI_PS_SEL				GENMASK(18, 16)
	#define RGB_SWAP				BIT(24)
	#define BYTE_SWAP				BIT(25)
	#define CUSTOM_HEADER				GENMASK(31, 26)
#define DSI_VSA_NL					0x00000020
	#define VSA_NL					GENMASK(11, 0)
#define DSI_VBP_NL					0x00000024
	#define VBP_NL					GENMASK(11, 0)
#define DSI_VFP_NL					0x00000028
	#define VFP_NL					GENMASK(11, 0)
#define DSI_VACT_NL					0x0000002c
	#define VACT_NL					GENMASK(14, 0)
#define DSI_LFR_CON					0x00000030
	#define DSI_LFR_MODE				GENMASK(1, 0)
	#define DSI_LFR_TYPE				GENMASK(3, 2)
	#define DSI_LFR_EN				BIT(4)
	#define DSI_LFR_UPDATE				BIT(5)
	#define DSI_LFR_VSE_DIS				BIT(6)
	#define DSI_LFR_SKIP_NUM			GENMASK(13, 8)
#define DSI_LFR_STA					0x00000034
	#define DSI_LFR_SKIP_CNT			GENMASK(5, 0)
	#define DSI_LFR_SKIP_STA			BIT(8)
#define DSI_SIZE_CON					0x00000038
	#define DSI_WIDTH				GENMASK(14, 0)
	#define DSI_HEIGHT				GENMASK(30, 16)
#define DSI_HSA_WC					0x00000050
#define DSI_HBP_WC					0x00000054
#define DSI_HFP_WC					0x00000058
#define DSI_BLLP_WC					0x0000005c
#define DSI_CMDQ_CON					0x00000060
	#define CMDQ_SIZE				GENMASK(7, 0)
#define DSI_HSTX_CKLP_WC				0x00000064
	#define HSTX_CKLP_WC				GENMASK(15, 2)
	#define HSTX_CKLP_WC_AUTO			BIT(16)
#define DSI_HSTX_CKLP_WC_AUTO_RESULT			0x00000068
	#define HSTX_CKLP_WC_AUTO_RESULT		GENMASK(15, 0)
#define DSI_RX_DATA03					0x00000074
	#define BYTE0					GENMASK(7, 0)
	#define BYTE1					GENMASK(15, 8)
	#define BYTE2					GENMASK(23, 16)
	#define BYTE3					GENMASK(31, 24)
#define DSI_RX_DATA47					0x00000078
	#define BYTE4					GENMASK(7, 0)
	#define BYTE5					GENMASK(15, 8)
	#define BYTE6					GENMASK(23, 16)
	#define BYTE7					GENMASK(31, 24)
#define DSI_RX_DATA8B					0x0000007c
	#define BYTE8					GENMASK(7, 0)
	#define BYTE9					GENMASK(15, 8)
	#define BYTEA					GENMASK(23, 16)
	#define BYTEB					GENMASK(31, 24)
#define DSI_RX_DATAC					0x00000080
	#define BYTEC					GENMASK(7, 0)
	#define BYTED					GENMASK(15, 8)
	#define BYTEE					GENMASK(23, 16)
	#define BYTEF					GENMASK(31, 24)
#define DSI_RX_RACK					0x00000084
	#define RACK					BIT(0)
	#define RACK_BYPASS				BIT(1)
#define DSI_RX_TRIG_STA					0x00000088
	#define RX_TRIG_0				BIT(0)
	#define RX_TRIG_1				BIT(1)
	#define RX_TRIG_2				BIT(2)
	#define RX_TRIG_3				BIT(3)
	#define RX_ULPS					BIT(4)
	#define DIRECTION				BIT(5)
	#define RX_LPDT					BIT(6)
	#define RX_POINTER				GENMASK(11, 8)
#define DSI_MEM_CONTI					0x00000090
	#define DSI_RWMEM_CONTI				GENMASK(15, 0)
#define DSI_FRM_BC					0x00000094
#define DSI_TIME_CON0					0x000000a0
	#define ULPS_WAKEUP_PRD				GENMASK(15, 0)
	#define SKEWCAL_PRD				GENMASK(31, 16)
#define DSI_TIME_CON1					0x000000a4
	#define TE_TIMEOUT_PRD				GENMASK(15, 0)
	#define PREFETCH_TIME				GENMASK(30, 16)
	#define PREFETCH_EN				BIT(31)
#define DSI_PHY_LCPAT					0x00000100
	#define LC_HSTX_CK_PAT				GENMASK(7, 0)
#define DSI_PHY_LCCON					0x00000104
	#define LC_HSTX_EN				BIT(0)
	#define LC_ULPM_EN				BIT(1)
	#define LC_WAKEUP_EN				BIT(2)
#define DSI_PHY_LD0CON					0x00000108
	#define L0_RM_TRIG_EN				BIT(0)
	#define L0_ULPM_EN				BIT(1)
	#define L0_WAKEUP_EN				BIT(2)
	#define LX_ULPM_AS_L0				BIT(3)
	#define L0_RX_FILTER_EN				BIT(4)
#define DSI_PHY_SYNCON					0x0000010c
	#define HS_SYNC_CODE				GENMASK(7, 0)
	#define HS_SYNC_CODE2				GENMASK(15, 8)
	#define HS_SKEWCAL_PAT				GENMASK(23, 16)
	#define HS_DB_SYNC_EN				BIT(24)
#define DSI_PHY_TIMCON0					0x00000110
	#define LPX					GENMASK(7, 0)
	#define DA_HS_PREP				GENMASK(15, 8)
	#define DA_HS_ZERO				GENMASK(23, 16)
	#define DA_HS_TRAIL				GENMASK(31, 24)
#define DSI_PHY_TIMCON1					0x00000114
	#define TA_GO					GENMASK(7, 0)
	#define TA_SURE					GENMASK(15, 8)
	#define TA_GET					GENMASK(23, 16)
	#define DA_HS_EXIT				GENMASK(31, 24)
#define DSI_PHY_TIMCON2					0x00000118
	#define CONT_DET				GENMASK(7, 0)
	#define DA_HS_SYNC				GENMASK(15, 8)
	#define CLK_HS_ZERO				GENMASK(23, 16)
	#define CLK_HS_TRAIL				GENMASK(31, 24)
#define DSI_PHY_TIMCON3					0x0000011c
	#define CLK_HS_PREP				GENMASK(7, 0)
	#define CLK_HS_POST				GENMASK(15, 8)
	#define CLK_HS_EXIT				GENMASK(25, 16)
#define DSI_CPHY_CON0					0x00000120
	#define CPHY_EN					BIT(0)
	#define SETTLE_SKIP_EN				BIT(1)
	#define PROGSEQ_SKIP_EN				BIT(2)
	#define CPHY_PROGSEQMSB				GENMASK(13, 4)
	#define CPHY_INIT_STATE				GENMASK(24, 16)
	#define CPHY_CONTI_CLK				GENMASK(31, 28)
#define DSI_CPHY_CON1					0x00000124
	#define CPHY_PROGSEQLSB				GENMASK(31, 0)
#define DSI_CPHY_DBG0					0x00000128
	#define CPHYHS_STATE_DA0			GENMASK(8, 0)
	#define CPHYHS_STATE_DA1			GENMASK(24, 16)
#define DSI_CPHY_DBG1					0x0000012c
	#define CPHYHS_STATE_DA2			GENMASK(8, 0)
#define DSI_VM_CMD_CON					0x00000130
	#define VM_CMD_EN				BIT(0)
	#define LONG_PKT				BIT(1)
	#define TIME_SEL				BIT(2)
	#define TS_VSA_EN				BIT(3)
	#define TS_VBP_EN				BIT(4)
	#define TS_VFP_EN				BIT(5)
	#define CM_DATA_ID				GENMASK(15, 8)
	#define CM_DATA_0				GENMASK(23, 16)
	#define CM_DATA_1				GENMASK(31, 24)
#define DSI_VM_CMD_DATA0				0x00000134
#define DSI_VM_CMD_DATA4				0x00000138
#define DSI_VM_CMD_DATA8				0x0000013c
#define DSI_VM_CMD_DATAC				0x00000140
#define DSI_CKSM_OUT					0x00000144
	#define PKT_CHKSUM				GENMASK(15, 0)
	#define ACC_CHKSUM				GENMASK(31, 16)
#define DSI_STATE_DBG6					0x00000160
	#define CMCTL_STATE				GENMASK(14, 0)
	#define CMDQ_STATE				GENMASK(22, 16)
#define DSI_STATE_DBG7					0x00000164
	#define VMCTL_STATE				GENMASK(10, 0)
	#define VFP_PERIOD				BIT(12)
	#define VACT_PERIOD				BIT(13)
	#define VBP_PERIOD				BIT(14)
	#define VSA_PERIOD				BIT(15)
	#define VM_FRAME_CKSUM				GENMASK(31, 16)
#define DSI_DEBUG_SEL					0x00000170
	#define DEBUG_OUT_SEL				GENMASK(4, 0)
	#define CHKSUM_REC_EN				BIT(8)
	#define C2V_START_CON				BIT(9)
	#define VM_FRAME_CKSM_EN			BIT(10)
	#define FIFO_MEMORY_TYPE			GENMASK(13, 12)
	#define DYNAMIC_CG_CON				GENMASK(31, 14)
#define DSI_BIST_PATTERN				0x00000178
	#define SELF_PAT_PATTERN			GENMASK(31, 0)
#define DSI_BIST_CON					0x0000017c
	#define BIST_MODE				BIT(0)
	#define SELF_PAT_PRE_MODE			BIT(5)
	#define SELF_PAT_POST_MODE			BIT(6)
	#define SW_MUTE_EN				BIT(7)
	#define HW_MUTE_EN				BIT(8)
	#define CONT_DET_ENABLE				BIT(9)
	#define SELF_PAT_VSYNC_ON			BIT(10)
	#define SELF_PAT_READY_ON			BIT(11)
	#define FIFOPRE_CNT_CLR				BIT(13)
	#define MM_RST_SEL				BIT(14)
	#define SELF_PAT_SEL				GENMASK(27, 24)
#define DSI_VM_CMD_DATA10				0x00000180
#define DSI_VM_CMD_DATA14				0x00000184
#define DSI_VM_CMD_DATA18				0x00000188
#define DSI_VM_CMD_DATA1C				0x0000018c
#define DSI_SHADOW_DEBUG				0x00000190
	#define FORCE_COMMIT				BIT(0)
	#define BYPASS_SHADOW				BIT(1)
	#define READ_WORKING				BIT(2)
	#define SAVE_POWER				BIT(3)
	#define SOFEOF_SEL				GENMASK(29, 28)
#define DSI_SHADOW_STA					0x00000194
	#define VACT_UPDATE_ERR				BIT(0)
	#define VFP_UPDATE_ERR				BIT(1)
	#define SOFEOF_CNT				GENMASK(31, 16)
#define DSI_LINE_DEBUG					0x00000198
	#define VM_TOTAL_LINE_CNT			GENMASK(14, 0)
	#define DSI_TOP_BUG_STATUS			GENMASK(31, 16)
#define DSI_VM_CMD_DATA20				0x000001a0
#define DSI_VM_CMD_DATA24				0x000001a4
#define DSI_VM_CMD_DATA28				0x000001a8
#define DSI_VM_CMD_DATA2C				0x000001ac
#define DSI_VM_CMD_DATA30				0x000001b0
#define DSI_VM_CMD_DATA34				0x000001b4
#define DSI_VM_CMD_DATA38				0x000001b8
#define DSI_VM_CMD_DATA3C				0x000001bc
#define DSI_LINE_DEBUG3					0x000001c0
	#define FIFO_WIDTH_CNT				GENMASK(15, 0)
	#define FIFO_HEIGHT_CNT				GENMASK(30, 16)
#define DSI_DEBUG_MBIST					0x000001c4
	#define PHY_CHECKSUM				GENMASK(31, 16)
#define DSI_FRAME_CNT					0x000001c8
	#define FRAME_RATE				GENMASK(30, 0)
	#define FRAME_CNT_EN				BIT(31)
#define DSI_VACTL_EVENT					0x000001cc
	#define VACTL_EVENT_OFFSET			GENMASK(14, 0)
	#define VACTL_EVENT_PERIOD			GENMASK(30, 16)
	#define VACTL_EVENT_EN				BIT(31)
#define DSI_CMDQ					0x00000200
	#define TYPE					GENMASK(1, 0)
	#define BTA					BIT(2)
	#define HS					BIT(3)
	#define CL					BIT(4)
	#define TE					BIT(5)
	#define RESV					GENMASK(7, 6)
	#define DATA_ID					GENMASK(15, 8)
	#define DATA_0					GENMASK(23, 16)
	#define DATA_1					GENMASK(31, 24)

#endif /*__MTK_DSI_REG_H__*/
