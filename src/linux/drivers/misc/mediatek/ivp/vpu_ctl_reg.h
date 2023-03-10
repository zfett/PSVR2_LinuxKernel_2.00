/*
 * Copyright (C) 2017 MediaTek Inc.
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
#ifndef __VPU_CTL_REG_H__
#define __VPU_CTL_REG_H__

/* ----------------- Register Definitions ------------------- */
#define VPU_RESET					0x00000000
	#define VPU_B_RST				BIT(4)
	#define VPU_D_RST				BIT(8)
	#define VPU_OCD_HALT_ON_RST			BIT(12)
#define VPU_DONE_ST					0x0000000c
	#define XOCD_MODE				BIT(4)
	#define P_WAIT_MODE				BIT(7)
#define VPU_CTRL					0x00000010
	#define BUS_PIF_GATED				BIT(17)
	#define STAT_VECTOR_SEL				BIT(19)
	#define RUN_STALL				BIT(23)
	#define PBCLK_EN				BIT(26)
	#define P_DEBUG_ENABLE				BIT(31)
#define VPU_XTENSA_INT					0x00000014
	#define INT_XTENSA				BIT(0)
#define VPU_CTL_XTENSA_INT				0x00000018
	#define INT_CTL_XTENSA00			BIT(0)
	#define INT_CTL_XTENSA01			BIT(1)
	#define INT_CTL_XTENSA02			BIT(2)
	#define INT_CTL_XTENSA03			BIT(3)
	#define INT_CTL_XTENSA04			BIT(4)
	#define INT_CTL_XTENSA05			BIT(5)
	#define INT_CTL_XTENSA06			BIT(6)
	#define INT_CTL_XTENSA07			BIT(7)
	#define INT_CTL_XTENSA08			BIT(8)
	#define INT_CTL_XTENSA09			BIT(9)
	#define INT_CTL_XTENSA10			BIT(10)
	#define INT_CTL_XTENSA11			BIT(11)
	#define INT_CTL_XTENSA12			BIT(12)
	#define INT_CTL_XTENSA13			BIT(13)
	#define INT_CTL_XTENSA14			BIT(14)
	#define INT_CTL_XTENSA15			BIT(15)
	#define INT_CTL_XTENSA16			BIT(16)
	#define INT_CTL_XTENSA17			BIT(17)
	#define INT_CTL_XTENSA18			BIT(18)
	#define INT_CTL_XTENSA19			BIT(19)
	#define INT_CTL_XTENSA20			BIT(20)
	#define INT_CTL_XTENSA21			BIT(21)
	#define INT_CTL_XTENSA22			BIT(22)
#define VPU_INT_MASK					0x0000002c
	#define CTL_XTENSA_INT_MASK_22_0		GENMASK(22, 0)
	#define APMCU_INT_MASK				BIT(24)
#define VPU_INT_CLR					0x00000030
	#define INT_CLR_XTENSA0				BIT(0)
	#define INT_CLR_XTENSA1				BIT(1)
	#define INT_CLR_XTENSA2				BIT(2)
	#define INT_CLR_XTENSA3				BIT(3)
	#define INT_CLR_XTENSA4				BIT(4)
	#define INT_CLR_XTENSA5				BIT(5)
	#define INT_CLR_XTENSA6				BIT(6)
	#define INT_CLR_XTENSA7				BIT(7)
	#define INT_CLR_XTENSA8				BIT(8)
	#define INT_CLR_XTENSA9				BIT(9)
	#define INT_CLR_XTENSA10			BIT(10)
	#define INT_CLR_XTENSA11			BIT(11)
	#define INT_CLR_XTENSA16			BIT(16)
	#define INT_CLR_XTENSA17			BIT(17)
	#define INT_CLR_XTENSA20			BIT(20)
#define VPU_IR_MITIGATION				0x00000038
	#define VPU_IR_MITIGATION_MODE			GENMASK(1, 0)
	#define VPU_IR_MITIGATION_AGGRESSIVE		BIT(8)
#define VPU_AXI_DEFAULT0				0x0000003c
	#define AW_USER_IDMA_8_4			GENMASK(12, 8)
	#define AR_USER_IDMA_8_4			GENMASK(17, 13)
	#define AW_USER_CORE_8_4			GENMASK(24, 20)
	#define AR_USER_CORE_8_4			GENMASK(29, 25)
#define VPU_AXI_DEFAULT3				0x00000048
	#define DBG_EN					BIT(16)
	#define NIDEN					BIT(17)
	#define SPIDEN					BIT(18)
	#define SPNIDEN					BIT(19)
#define VPU_CABGEN_CTRL					0x0000004c
	#define APB_MULTI_ALT_CORE_EN			BIT(18)
	#define APB_MULTI_ALT_CORE_EN_STS		BIT(19)
#define VPU_XTENSA_INFO00				0x00000050
	#define XTENSA_INFO0				GENMASK(31, 0)
#define VPU_XTENSA_INFO01				0x00000054
	#define XTENSA_INFO1				GENMASK(31, 0)
#define VPU_XTENSA_INFO02				0x00000058
	#define XTENSA_INFO2				GENMASK(31, 0)
#define VPU_XTENSA_INFO03				0x0000005c
	#define XTENSA_INFO3				GENMASK(31, 0)
#define VPU_XTENSA_INFO04				0x00000060
	#define XTENSA_INFO4				GENMASK(31, 0)
#define VPU_XTENSA_INFO05				0x00000064
	#define XTENSA_INFO5				GENMASK(31, 0)
#define VPU_XTENSA_INFO06				0x00000068
	#define XTENSA_INFO6				GENMASK(31, 0)
#define VPU_XTENSA_INFO07				0x0000006c
	#define XTENSA_INFO7				GENMASK(31, 0)
#define VPU_XTENSA_INFO08				0x00000070
	#define XTENSA_INFO8				GENMASK(31, 0)
#define VPU_XTENSA_INFO09				0x00000074
	#define XTENSA_INFO9				GENMASK(31, 0)
#define VPU_XTENSA_INFO10				0x00000078
	#define XTENSA_INFO10				GENMASK(31, 0)
#define VPU_XTENSA_INFO11				0x0000007c
	#define XTENSA_INFO11				GENMASK(31, 0)
#define VPU_XTENSA_INFO12				0x00000080
	#define XTENSA_INFO12				GENMASK(31, 0)
#define VPU_XTENSA_INFO13				0x00000084
	#define XTENSA_INFO13				GENMASK(31, 0)
#define VPU_XTENSA_INFO14				0x00000088
	#define XTENSA_INFO14				GENMASK(31, 0)
#define VPU_XTENSA_INFO15				0x0000008c
	#define XTENSA_INFO15				GENMASK(31, 0)
#define VPU_XTENSA_INFO16				0x00000090
	#define XTENSA_INFO16				GENMASK(31, 0)
#define VPU_XTENSA_INFO17				0x00000094
	#define XTENSA_INFO17				GENMASK(31, 0)
#define VPU_XTENSA_INFO18				0x00000098
	#define XTENSA_INFO18				GENMASK(31, 0)
#define VPU_XTENSA_INFO19				0x0000009c
	#define XTENSA_INFO19				GENMASK(31, 0)
#define VPU_XTENSA_INFO20				0x000000a0
	#define XTENSA_INFO20				GENMASK(31, 0)
#define VPU_XTENSA_INFO21				0x000000a4
	#define XTENSA_INFO21				GENMASK(31, 0)
#define VPU_XTENSA_INFO22				0x000000a8
	#define XTENSA_INFO22				GENMASK(31, 0)
#define VPU_XTENSA_INFO23				0x000000ac
	#define XTENSA_INFO23				GENMASK(31, 0)
#define VPU_XTENSA_INFO24				0x000000b0
	#define XTENSA_INFO24				GENMASK(31, 0)
#define VPU_XTENSA_INFO25				0x000000b4
	#define XTENSA_INFO25				GENMASK(31, 0)
#define VPU_XTENSA_INFO26				0x000000b8
	#define XTENSA_INFO26				GENMASK(31, 0)
#define VPU_XTENSA_INFO27				0x000000bc
	#define XTENSA_INFO27				GENMASK(31, 0)
#define VPU_XTENSA_INFO28				0x000000c0
	#define XTENSA_INFO28				GENMASK(31, 0)
#define VPU_XTENSA_INFO29				0x000000c4
	#define XTENSA_INFO29				GENMASK(31, 0)
#define VPU_XTENSA_INFO30				0x000000c8
	#define XTENSA_INFO30				GENMASK(31, 0)
#define VPU_XTENSA_INFO31				0x000000cc
	#define XTENSA_INFO31				GENMASK(31, 0)
#define VPU_DEBUG_INFO01				0x000000d4
	#define PDEBUG_INB_PIF				GENMASK(7, 0)
	#define PDEBUG_STATUS				GENMASK(23, 16)
#define VPU_DEBUG_INFO02				0x000000d8
	#define PDEBUG_INST				GENMASK(31, 0)
#define VPU_DEBUG_INFO03				0x000000dc
	#define PDEBUG_LS0_STAT				GENMASK(31, 0)
#define VPU_DEBUG_INFO04				0x000000e0
	#define PDEBUG_LS1_STAT				GENMASK(31, 0)
#define VPU_DEBUG_INFO05				0x000000e4
	#define PDEBUG_PC				GENMASK(31, 0)
#define VPU_XTENSA_ALTRESETVEC				0x00000100

#endif /*__VPU_CTL_REG_H__*/
