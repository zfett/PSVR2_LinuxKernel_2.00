/*******************************************************************************
*
* Copyright (c) 2010, Media Teck.inc
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public Licence,
* version 2, as publish by the Free Software Foundation.
*
* This program is distributed and in hope it will be useful, but WITHOUT
* ANY WARRNTY; without even the implied warranty of MERCHANTABITLITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
*/

/**
 * @file mt_pwm_reg.h
 * Heander of mt_pwm.c
 */

#ifndef __PWM_REG_H__
#define __PWM_REG_H__

#include <linux/types.h>
#include <mt_pwm_hal.h>

/** @ingroup IP_group_pwm_internal_def
 * @{
 */
/* ----------------- Register Definitions ------------------- */
#define PWM_ENABLE					0x00000000
#define PWM_CON_0					0x00000010
	#define CLKDIV					GENMASK(2, 0)
	#define CLKSEL					BIT(3)
	#define CLKSEL_OLD				BIT(4)
	#define SRCSEL					BIT(5)
	#define MODE					BIT(6)
	#define IDLE_VALUE				BIT(7)
	#define GUARD_VALUE				BIT(8)
	#define STOP_BITPOS				GENMASK(14, 9)
	#define STOP_BITS_OFFSET		9
	#define OLD_PWM_MODE				BIT(15)
#define PWM_CH_REG(CH, RG)		((PWM_CON_0 + CH * 0x40) + 4 * RG)
#define PWM_3DLCM					0x00000810
	#define PWM_3DLCM_EN				BIT(0)
#define PWM_INT_ENABLE					0x00000814
	#define PWM_INT_FINISH_EN			BIT(0)
	#define PWM_INT_UNDERFLOW_EN			BIT(1)
#define PWM_INT_ENABLE_1				0x00000818
#define PWM_INT_ACK					0x00000824
#define PWM_INT_ACK_1					0x00000828
#define PWM_CK_26M_SEL					0x00000830
	#define PWM_26M_SEL				BIT(0)
#define PWM_SEQ_MODE					0x00000834
	#define PWM_SEQ_MODE_EN				BIT(0)
#define PWM1_DELAY					0x00000838
	#define PWM_DELAY_DURATION			GENMASK(15, 0)
	#define DELAY_CLKSEL				BIT(16)
#define PWM2_DELAY					0x0000083c
#define PWM3_DELAY					0x00000840
#define PWM4_DELAY					0x00000844
#define PWM5_DELAY					0x00000848
#define PWM6_DELAY					0x0000084c
#define PWM7_DELAY					0x00000850
#define PWM_CLK_NAME_MAIN   "groupclk"
#define PWM_CLK_NAME_SEL   "fpwmclk"
#define PWM_CLK_NAME_PWM    "pwmclk"
#define PWM_CLK_NAME_PWM1   "pwm1clk"
#define PWM_CLK_NAME_PWM2   "pwm2clk"
#define PWM_CLK_NAME_PWM3   "pwm3clk"
#define PWM_CLK_NAME_PWM4   "pwm4clk"
#define PWM_CLK_NAME_PWM5   "pwm5clk"
#define PWM_CLK_NAME_PWM6   "pwm6clk"
#define PWM_CLK_NAME_PWM7   "pwm7clk"
#define PWM_CLK_NAME_PWM8   "pwm8clk"
#define PWM_CLK_NAME_PWM9   "pwm9clk"
#define PWM_CLK_NAME_PWM10   "pwm10clk"
#define PWM_CLK_NAME_PWM11   "pwm11clk"
#define PWM_CLK_NAME_PWM12   "pwm12clk"
#define PWM_CLK_NAME_PWM13   "pwm13clk"
#define PWM_CLK_NAME_PWM14   "pwm14clk"
#define PWM_CLK_NAME_PWM15   "pwm15clk"
#define PWM_CLK_NAME_PWM16   "pwm16clk"
#define PWM_CLK_NAME_PWM17   "pwm17clk"
#define PWM_CLK_NAME_PWM18   "pwm18clk"
#define PWM_CLK_NAME_PWM19   "pwm19clk"
#define PWM_CLK_NAME_PWM20   "pwm20clk"
#define PWM_CLK_NAME_PWM21   "pwm21clk"
#define PWM_CLK_NAME_PWM22   "pwm22clk"
#define PWM_CLK_NAME_PWM23   "pwm23clk"
#define PWM_CLK_NAME_PWM24   "pwm24clk"
#define PWM_CLK_NAME_PWM25   "pwm25clk"
#define PWM_CLK_NAME_PWM26   "pwm26clk"
#define PWM_CLK_NAME_PWM27   "pwm27clk"
#define PWM_CLK_NAME_PWM28   "pwm28clk"
#define PWM_CLK_NAME_PWM29   "pwm29clk"
#define PWM_CLK_NAME_PWM30   "pwm30clk"
#define PWM_CLK_NAME_PWM31   "pwm31clk"
#define PWM_CLK_NAME_PWM32   "pwm32clk"
#define BLOCK_CLK			(66UL * 1000 * 1000)
#define PWM_26M_CLK			(26UL * 1000 * 1000)
#define PWM_100M_CLK			(100UL * 1000 * 1000)
/**
 *@}
 */

/** @ingroup IP_group_pwm_internal_struct
 * @brief General config structure.
 */
struct pwm_easy_config {
	/** pwm channel num */
	u32 pwm_no;
	/** pwm duty cycle */
	u32 duty;
	/** pwm clk source */
	u32 clk_src;
	/** pwm clk scale */
	u32 clk_div;
	/** pwm waveform duration  */
	u16 duration;
};

/** @ingroup IP_group_pwm_internal_struct
 * @brief Special config structure.
 */
struct pwm_spec_config {
	/** pwm channel num */
	u32 pwm_no;
	/** pwm mode sel */
	u32 mode;
	/** pwm clk scale */
	u32 clk_div;
	/** pwm clk source */
	u32 clk_src;
	/** pwm interrupt ack */
	u8 intr;

	union {
		/** for old mode */
		struct _PWM_OLDMODE_REGS {
			u16 idle_value;
			u16 guard_value;
			u16 gduration;
			u16 wave_num;
			u16 data_width;
			u16 thresh;
		} PWM_MODE_OLD_REGS;

		/** for fifo mode */
		struct _PWM_MODE_FIFO_REGS {
			u32 idle_value;
			u32 guard_value;
			u32 stop_bitpos_value;
			u16 high_duration;
			u16 low_duration;
			u32 gduration;
			u32 send_data0;
			u32 send_data1;
			u32 wave_num;
		} PWM_MODE_FIFO_REGS;

		/** for memory mode */
		struct _PWM_MODE_MEMORY_REGS {
			u32 idle_value;
			u32 guard_value;
			u32 stop_bitpos_value;
			u16 high_duration;
			u16 low_duration;
			u16 gduration;
			u32 buf0_base_addr;
			u32 buf0_size;
			u16 wave_num;
		} PWM_MODE_MEMORY_REGS;

		/** for RANDOM mode */
		struct _PWM_MODE_RANDOM_REGS {
			u16 idle_value;
			u16 guard_value;
			u32 stop_bitpos_value;
			u16 high_duration;
			u16 low_duration;
			u16 gduration;
			u32 buf0_base_addr;
			u32 buf0_size;
			u32 buf1_base_addr;
			u32 buf1_size;
			u16 wave_num;
			u32 valid;
		} PWM_MODE_RANDOM_REGS;

		/** for seq mode */
		struct _PWM_MODE_DELAY_REGS {
			/** u32 ENABLE_DELAY_VALUE; */
			u16 pwm1_delay_dur;
			bool pwm1_delay_clk;
			/** 0: block clock source, 1: block/1625 clock source */
			u16 pwm2_delay_dur;
			bool pwm2_delay_clk;
			u16 pwm3_delay_dur;
			bool pwm3_delay_clk;
			u16 pwm4_delay_dur;
			bool pwm4_delay_clk;
			u16 pwm5_delay_dur;
			bool pwm5_delay_clk;
			u16 pwm6_delay_dur;
			bool pwm6_delay_clk;
			u16 pwm7_delay_dur;
			bool pwm7_delay_clk;
		} PWM_MODE_DELAY_REGS;

		/** for 3DLCM mode */
		struct _PWM_MODE_3DLCM_REGS {
			bool pwm8_as_base_channel;
			bool pwm9_as_base_channel;
			bool pwm10_as_base_channel;
			bool pwm11_as_base_channel;
			bool pwm12_as_base_channel;
			bool pwm13_as_base_channel;
			bool pwm14_as_base_channel;
			bool pwm15_as_base_channel;
			bool pwm8_as_aux_channel;
			bool pwm9_as_aux_channel;
			bool pwm10_as_aux_channel;
			bool pwm11_as_aux_channel;
			bool pwm12_as_aux_channel;
			bool pwm13_as_aux_channel;
			bool pwm14_as_aux_channel;
			bool pwm15_as_aux_channel;
			bool pwm8_inverse_base_channel;
			bool pwm9_inverse_base_channel;
			bool pwm10_inverse_base_channel;
			bool pwm11_inverse_base_channel;
			bool pwm12_inverse_base_channel;
			bool pwm13_inverse_base_channel;
			bool pwm14_inverse_base_channel;
			bool pwm15_inverse_base_channel;
		} PWM_MODE_3DLCM_REGS;
	};
};

#endif /*__PWM_REG_H__*/
