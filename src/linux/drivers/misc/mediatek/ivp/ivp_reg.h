/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
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
#ifndef __IVP_REG_H__
#define __IVP_REG_H__

#include <linux/io.h>
#include <linux/kernel.h>

#include "vpusys_core_config_reg.h"

#define IVP_VCORE_CG_CON			0x00
#define IVP_VCORE_CG_SET			0x04
#define IVP_VCORE_CG_CLR			0x08
#define IVP_VCORE_SW_RST			0x0C

#define IVP_CG_CON				0x00
#define IVP_CG_SET				0x04
#define IVP_CG_CLR				0x08
#define IVP_SW_RST				0x0C

#define IVP_VCORE_RESET0			0x0E
#define IVP_VCORE_RESET1			0x70

#define IVP_CTRL_OFFSET				0x90000
#define IVPSYS_CORE_OFFSET			0xB0000

#define IVP_CTRL_REG_RESET			IVP_SW_RST
#define IVP_CTRL_REG_DONE_ST			VPU_DONE_ST
#define IVP_CTRL_REG_CTRL			VPU_CTRL
#define IVP_CTRL_REG_XTENSA_INT			VPU_XTENSA_INT
#define IVP_INT_XTENSA				VPU_XTENSA_INT_APMCU_INT
#define IVP_CTRL_REG_CTL_XTENSA_INT		VPU_CTL_XTENSA_INT
#define IVP_CTRL_REG_INT_MASK			VPU_INT_MASK
#define IVP_CTRL_REG_TOP_SPARE			VPU_IR_MITIGATION
#define IVP_CTRL_REG_AXI_DEFAULT0		VPU_AXI_DEFAULT0
#define IVP_CTRL_REG_AXI_DEFAULT3		VPU_AXI_DEFAULT3
#define IVP_CTRL_REG_CABGEN_CTRL		VPU_CABGEN_CTRL
#define IVP_CTRL_REG_XTENSA_INFO00		VPU_XTENSA_INFO00
#define IVP_CTRL_REG_XTENSA_INFO01		VPU_XTENSA_INFO01
#define IVP_CTRL_REG_XTENSA_INFO02		VPU_XTENSA_INFO02
#define IVP_CTRL_REG_XTENSA_INFO03		VPU_XTENSA_INFO03
#define IVP_CTRL_REG_XTENSA_INFO04		VPU_XTENSA_INFO04
#define IVP_CTRL_REG_XTENSA_INFO05		VPU_XTENSA_INFO05
#define IVP_CTRL_REG_XTENSA_INFO06		VPU_XTENSA_INFO06
#define IVP_CTRL_REG_XTENSA_INFO07		VPU_XTENSA_INFO07
#define IVP_CTRL_REG_XTENSA_INFO08		VPU_XTENSA_INFO08
#define IVP_CTRL_REG_XTENSA_INFO09		VPU_XTENSA_INFO09
#define IVP_CTRL_REG_XTENSA_INFO10		VPU_XTENSA_INFO10
#define IVP_CTRL_REG_XTENSA_INFO11		VPU_XTENSA_INFO11
#define IVP_CTRL_REG_XTENSA_INFO12		VPU_XTENSA_INFO12
#define IVP_CTRL_REG_XTENSA_INFO13		VPU_XTENSA_INFO13
#define IVP_CTRL_REG_XTENSA_INFO14		VPU_XTENSA_INFO14
#define IVP_CTRL_REG_XTENSA_INFO15		VPU_XTENSA_INFO15
#define IVP_CTRL_REG_XTENSA_INFO16		VPU_XTENSA_INFO16
#define IVP_CTRL_REG_XTENSA_INFO17		VPU_XTENSA_INFO17
#define IVP_CTRL_REG_XTENSA_INFO18		VPU_XTENSA_INFO18
#define IVP_CTRL_REG_XTENSA_INFO19		VPU_XTENSA_INFO19
#define IVP_CTRL_REG_XTENSA_INFO20		VPU_XTENSA_INFO20
#define IVP_CTRL_REG_XTENSA_INFO21		VPU_XTENSA_INFO21
#define IVP_CTRL_REG_XTENSA_INFO22		VPU_XTENSA_INFO22
#define IVP_CTRL_REG_XTENSA_INFO23		VPU_XTENSA_INFO23
#define IVP_CTRL_REG_XTENSA_INFO24		VPU_XTENSA_INFO24
#define IVP_CTRL_REG_XTENSA_INFO25		VPU_XTENSA_INFO25
#define IVP_CTRL_REG_XTENSA_INFO26		VPU_XTENSA_INFO26
#define IVP_CTRL_REG_XTENSA_INFO27		VPU_XTENSA_INFO27
#define IVP_CTRL_REG_XTENSA_INFO28		VPU_XTENSA_INFO28
#define IVP_CTRL_REG_XTENSA_INFO29		VPU_XTENSA_INFO29
#define IVP_CTRL_REG_XTENSA_INFO30		VPU_XTENSA_INFO30
#define IVP_CTRL_REG_XTENSA_INFO31		VPU_XTENSA_INFO31
#define IVP_CTRL_REG_DEBUG_INFO01		VPU_DEBUG_INFO01
#define IVP_CTRL_REG_DEBUG_INFO02		VPU_DEBUG_INFO02
#define IVP_CTRL_REG_DEBUG_INFO03		VPU_DEBUG_INFO03
#define IVP_CTRL_REG_DEBUG_INFO04		VPU_DEBUG_INFO04
#define IVP_CTRL_REG_DEBUG_INFO05		VPU_DEBUG_INFO05
#define IVP_CTRL_REG_XTENSA_ALTRESETVEC		VPU_XTENSA_ALTRESETVEC

/* IVP_CTRL_REG_AXI_DEFAULT0 */
#define SHT_AR_USER_CORE_8_4	25
#define SHT_AW_USER_CORE_8_4	20
#define SHT_AR_USER_IDMA_8_4	13
#define SHT_AW_USER_IDMA_8_4	8

enum ivp_interrupt {
	INT00_LEVEL_L1 = 0,
	INT01_LEVEL_L1 = 1,
	INT02_LEVEL_L1 = 2,
	INT03_LEVEL_L1 = 3,
	INT04_LEVEL_L1 = 4,
	INT05_LEVEL_L1 = 5,
	INT06_LEVEL_L1 = 6,
	INT07_LEVEL_L1 = 7,

	INT10_LEVEL_L2 = 8,
	INT11_LEVEL_L2 = 9,
	INT12_LEVEL_L2 = 10,
	INT13_LEVEL_L2 = 11,
	INT16_EDGE_L1 = 12,
	INT17_EDGE_L2 = 13,

	INT22_EDGE_L2 = 14,

	INT24_NMI_L2 = 15,
	INT25_LEVEL_L1 = 16,
	INT26_LEVEL_L1 = 17,
	INT27_EDGE_L1 = 18,
	INT28_EDGE_L1 = 19,
	INT29_LEVEL_L2 = 20,
	INT30_EDGE_L2 = 21,
	INT31_EDGE_L2 = 22,
	INT_NUM_TOTAL_COUNT,
};

static inline unsigned int ivp_read_reg32(void __iomem *ivp_base,
					  unsigned int offset)
{
	return readl(ivp_base + offset);
}

static inline void ivp_write_reg32(void __iomem *ivp_base, unsigned int offset,
				   unsigned int val)
{
	writel(val, ivp_base + offset);
}

static inline void ivp_set_reg32(void __iomem *ivp_base, unsigned int offset,
			  unsigned int bits)
{
	ivp_write_reg32(ivp_base, offset,
			(ivp_read_reg32(ivp_base, offset) | bits));
}

static inline void ivp_clear_reg32(void __iomem *ivp_base, unsigned int offset,
			    unsigned int bits)
{
	ivp_write_reg32(ivp_base, offset,
			(ivp_read_reg32(ivp_base, offset) & ~bits));
}

#endif /* __IVP_REG_H__ */
