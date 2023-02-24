/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

struct mtk_pmic_p2p {
	struct device *dev;
	void __iomem *base;
	uint32_t vpu_200m_dac;
	uint32_t vpu_312m_dac;
	uint32_t vpu_457m_dac;
	uint32_t vpu_604m_dac;
	uint32_t vpu_756m_dac;
	uint32_t gpu_300m_dac;
	uint32_t gpu_400m_dac;
	uint32_t gpu_570m_dac;
	uint32_t gpu_665m_dac;
	uint32_t gpu_750m_dac;
	uint32_t gpu_815m_dac;
	uint32_t gpu_870m_dac;
};

/* PMICWRAP_P2P */
#define PWRAP_DVFS_ADR0			0x00000014
#define PWRAP_DVFS_WDATA0		0x00000018
#define PWRAP_DVFS_ADR1			0x0000001c
#define PWRAP_DVFS_WDATA1		0x00000020
#define PWRAP_DVFS_ADR2			0x00000024
#define PWRAP_DVFS_WDATA2		0x00000028
#define PWRAP_DVFS_ADR3			0x0000002c
#define PWRAP_DVFS_WDATA3		0x00000030
#define PWRAP_DVFS_ADR4			0x00000034
#define PWRAP_DVFS_WDATA4		0x00000038
#define PWRAP_DVFS_ADR5			0x0000003c
#define PWRAP_DVFS_WDATA5		0x00000040
#define PWRAP_DVFS_ADR6			0x00000044
#define PWRAP_DVFS_WDATA6		0x00000048
#define PWRAP_DVFS_ADR7			0x0000004c
#define PWRAP_DVFS_WDATA7		0x00000050
#define PWRAP_DVFS_ADR8			0x00000054
#define PWRAP_DVFS_WDATA8		0x00000058
#define PWRAP_DVFS_ADR9			0x0000005c
#define PWRAP_DVFS_WDATA9		0x00000060
#define PWRAP_DVFS_ADR10		0x00000064
#define PWRAP_DVFS_WDATA10		0x00000068
#define PWRAP_DVFS_ADR11		0x0000006c
#define PWRAP_DVFS_WDATA11		0x00000070
#define PWRAP_DVFS_ADR12		0x00000074
#define PWRAP_DVFS_WDATA12		0x00000078
#define PWRAP_DVFS_ADR13		0x0000007c
#define PWRAP_DVFS_WDATA13		0x00000080
#define PWRAP_DVFS_ADR14		0x00000084
#define PWRAP_DVFS_WDATA14		0x00000088
#define PWRAP_DVFS_ADR15		0x0000008c
#define PWRAP_DVFS_WDATA15		0x00000090
#define PWRAP_DVFS_ADR16		0x00000094
#define PWRAP_DVFS_WDATA16		0x00000098
#define PWRAP_DVFS_ADR17		0x0000009c
#define PWRAP_DVFS_WDATA17		0x000000a0
#define PWRAP_DVFS_ADR18		0x000000a4
#define PWRAP_DVFS_WDATA18		0x000000a8
#define PWRAP_DVFS_ADR19		0x000000ac
#define PWRAP_DVFS_WDATA19		0x000000b0
#define PWRAP_DVFS_ADR20		0x000000b4
#define PWRAP_DVFS_WDATA20		0x000000b8
#define PWRAP_DVFS_ADR21		0x000000bc
#define PWRAP_DVFS_WDATA21		0x000000c0
#define PWRAP_DVFS_ADR22		0x000000c4
#define PWRAP_DVFS_WDATA22		0x000000c8
#define PWRAP_DVFS_ADR23		0x000000cc
#define PWRAP_DVFS_WDATA23		0x000000d0
#define PWRAP_DVFS_ADR24		0x000000d4
#define PWRAP_DVFS_WDATA24		0x000000d8
#define PWRAP_DVFS_ADR25		0x000000dc
#define PWRAP_DVFS_WDATA25		0x000000e0
#define PWRAP_DVFS_ADR26		0x000000e4
#define PWRAP_DVFS_WDATA26		0x000000e8
#define PWRAP_DVFS_ADR27		0x000000ec
#define PWRAP_DVFS_WDATA27		0x000000f0
#define PWRAP_DVFS_ADR28		0x000000f4
#define PWRAP_DVFS_WDATA28		0x000000f8
#define PWRAP_DVFS_ADR29		0x000000fc
#define PWRAP_DVFS_WDATA29		0x00000100
#define PWRAP_DVFS_ADR30		0x00000104
#define PWRAP_DVFS_WDATA30		0x00000108
#define PWRAP_DVFS_ADR31		0x0000010c
#define PWRAP_DVFS_WDATA31		0x00000110

uint32_t mtk_get_vpu_avs_dac(struct platform_device *pdev, uint8_t idx);
uint32_t mtk_get_gpu_avs_dac(struct platform_device *pdev, uint8_t idx);

