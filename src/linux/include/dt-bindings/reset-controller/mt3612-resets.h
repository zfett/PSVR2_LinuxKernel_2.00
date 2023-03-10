/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Flora Fu, MediaTek
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

#ifndef _DT_BINDINGS_RESET_CONTROLLER_MT3612
#define _DT_BINDINGS_RESET_CONTROLLER_MT3612

#define MT3612_TOPRGU_OFS	0x18
#define MT3612_INFRACFG_OFS	0x120
#define MT3612_PERICFG_OFS	0x300

/* TOPRGU resets IDs */
#define MT3612_TOPRGU_MM_RST			1
#define MT3612_TOPRGU_EMI_RST			2
#define MT3612_TOPRGU_MFG_RST			3
#define MT3612_TOPRGU_DP_APB_RST		6
#define MT3612_TOPRGU_DP_RST			7
#define MT3612_TOPRGU_INFRA_AO_RST		8
#define MT3612_TOPRGU_APMIXED_RST		10
#define MT3612_TOPRGU_SIDECAM0_RST		12
#define MT3612_TOPRGU_SIDECAM1_RST		13
#define MT3612_TOPRGU_DRAM_CH0_RST		14
#define MT3612_TOPRGU_DRAM_CH1_RST		15
#define MT3612_TOPRGU_GAZECAM0_RST		16
#define MT3612_TOPRGU_GAZECAM1_RST		17
#define MT3612_TOPRGU_PERI_RST			18
#define MT3612_TOPRGU_PERI_AO_RST		19
#define MT3612_TOPRGU_MMSYS_CORE_RST	32
#define MT3612_TOPRGU_MMSYS_GAZE_RST	33
#define MT3612_TOPRGU_SYSRAM_STRIP_RST	34
#define MT3612_TOPRGU_SYSRAM_GAZE_RST	35
#define MT3612_TOPRGU_SYSRAM_VR_RST		36
#define MT3612_TOPRGU_IMG_SIDE0_RST		37
#define MT3612_TOPRGU_IMG_SIDE1_RST		38
#define MT3612_TOPRGU_CAM_SIDE0_RST		39
#define MT3612_TOPRGU_CAM_SIDE1_RST		40
#define MT3612_TOPRGU_ISP_GAZE0_RST		41
#define MT3612_TOPRGU_ISP_GAZE1_RST		42
#define MT3612_TOPRGU_VPU_CONN_RST		43
#define MT3612_TOPRGU_VPU0_RST			44
#define MT3612_TOPRGU_DRAM_CH0_AO_RST	45
#define MT3612_TOPRGU_DRAM_CH1_AO_RST	46
#define MT3612_TOPRGU_VPU1_RST			47
#define MT3612_TOPRGU_VPU2_RST			48
#define MT3612_TOPRGU_SSPXTP0_GRST		130
#define MT3612_TOPRGU_SSPXTP2_GRST		131
#define MT3612_TOPRGU_XTP_CKM_DP_GRST	132

/* INFRACFG resets IDs */
#define MT3612_INFRA_THERMAL_CTRL_RST		0
#define MT3612_INFRA_GCE_RST				1
#define MT3612_INFRA_SRAMROM_RST			12
#define MT3612_INFRA_L2C_RST				16
#define MT3612_INFRA_AUXADC_RST				17
#define MT3612_INFRA_GCE5_RST				42
#define MT3612_INFRA_DDDS_RST				49
#define MT3612_INFRA_AUDIO_SYS_RST			51
#define MT3612_INFRA_GCE4_RST				53
#define MT3612_INFRA_PMIC_WRAP_RST			64
#define MT3612_INFRA_SCP_RST				67
#define MT3612_INFRA_DVFSRC_RST				68
#define MT3612_INFRA_PMIC_GSPI_RST			69
#define MT3612_INFRA_SCP_SECURE_RST			70
#define MT3612_INFRA_SSHUB_APB_ASYNC_RST	71
#define MT3612_INFRA_SPM_APB_ASYNC_RST		72
#define MT3612_INFRA_PWR_MCU_APB_ASYNC_RST	73
#define MT3612_INFRA_HW_DVFS_APB_ASYNC_RST	74
#define MT3612_INFRA_APXGPT_APB_ASYNC_RST	75

/*  PERICFG resets IDs */
#define MT3612_PERI_IOMMU_SW_RST		2
#define MT3612_PERI_MSDC0_SW_RST		8
#define MT3612_PERI_BTIF_SW_RST			13
#define MT3612_PERI_DISP_PWM0_SW_RST	37
#define MT3612_PERI_DISP_PWM1_SW_RST	38
#define MT3612_PERI_I2C0_SW_RST			39
#define MT3612_PERI_I2C1_SW_RST			40
#define MT3612_PERI_I2C2_SW_RST			41
#define MT3612_PERI_I2C3_SW_RST			42
#define MT3612_PERI_I2C4_SW_RST			43
#define MT3612_PERI_I2C5_SW_RST			44
#define MT3612_PERI_I2C6_SW_RST			45
#define MT3612_PERI_I2C7_SW_RST			46
#define MT3612_PERI_PWM_RST				47
#define MT3612_PERI_DX_CC_AO_SW_RST		71
#define MT3612_PERI_I2C2_IMM_SW_RST		96
#define MT3612_PERI_I2C3_IMM_SW_RST		98
#define MT3612_PERI_SPI0_SW_RST			101
#define MT3612_PERI_SPI1_SW_RST			102
#define MT3612_PERI_SPI2_SW_RST			103
#define MT3612_PERI_SPI3_SW_RST			104
#define MT3612_PERI_SPI4_SW_RST			105
#define MT3612_PERI_SPI5_SW_RST			106
#define MT3612_PERI_UART0_SW_RST		107
#define MT3612_PERI_UART1_SW_RST		108
#define MT3612_PERI_UART2_SW_RST		109
#define MT3612_PERI_UART3_SW_RST		110
#define MT3612_PERI_UART4_SW_RST		111
#define MT3612_PERI_UART5_SW_RST		112
#define MT3612_PERI_SSUSB_TOP_SW_RST	113
#define MT3612_PERI_SFLASHIF_SW_RST		115
#define MT3612_PERI_UART6_SW_RST		117
#define MT3612_PERI_UART7_SW_RST		118
#define MT3612_PERI_UART8_SW_RST		121
#define MT3612_PERI_UART9_SW_RST		122
#define MT3612_PERI_SPI6_SW_RST			123
#define MT3612_PERI_SPI7_SW_RST			124
#define MT3612_PERI_SPI8_SW_RST			125
#define MT3612_PERI_SPI9_SW_RST			126
#define MT3612_PERI_I2C10_SW_RST		128
#define MT3612_PERI_I2C11_SW_RST		129
#define MT3612_PERI_I2C12_SW_RST		130
#define MT3612_PERI_I2C13_SW_RST		131
#define MT3612_PERI_I2C14_SW_RST		132
#define MT3612_PERI_I2C15_SW_RST		133
#define MT3612_PERI_I2C16_SW_RST		134
#define MT3612_PERI_I2C17_SW_RST		135
#define MT3612_PERI_I2C18_SW_RST		136
#define MT3612_PERI_I2C19_SW_RST		137
#define MT3612_PERI_I2C20_SW_RST		138
#define MT3612_PERI_I2C21_SW_RST		139
#define MT3612_PERI_I2C22_SW_RST		140
#define MT3612_PERI_IRQ_EN_APDMA3_SW_RST	146
#define MT3612_PERI_IRQ_EN_APDMA2_SW_RST	147
#define MT3612_PERI_IRQ_EN_APDMA1_SW_RST	148
#define MT3612_PERI_IRQ_EN_APDMA0_SW_RST	149
#define MT3612_PERI_IRQ_EN_I2C3_SW_RST		150
#define MT3612_PERI_IRQ_EN_I2C2_SW_RST		151
#define MT3612_PERI_IRQ_EN_I2C1_SW_RST		152
#define MT3612_PERI_IRQ_EN_I2C0_SW_RST		153
#endif  /* _DT_BINDINGS_RESET_CONTROLLER_MT3612 */
