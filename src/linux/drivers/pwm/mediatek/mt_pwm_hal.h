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
*
********************************************************************************
* Author : How  Wang (how.wang@mediatek.com)
********************************************************************************
*/

/**
 * @file mt_pwm_hal.h
 * Heander of mt_pwm_hal.c
 */

#ifndef __MT_PWM_HAL_H__
#define __MT_PWM_HAL_H__

#include <linux/types.h>
#include <linux/io.h>
#include <asm/cacheflush.h>
#include <linux/platform_device.h>

/** @ingroup IP_group_pwm_internal_def
 * @{
 */
/*********************************
*  Define Error Number
**********************************/
#define RSUCCESS	0
#define EEXCESSPWMNO	1
#define EPARMNOSUPPORT	2
#define ERROR		3
#define EBADADDR	4
#define EEXCESSBITS	5
#define EINVALID	6
#define PWM_MIN		0
#define PWM_MAX		31

#define PWM_DEBUG
#ifdef PWM_DEBUG
#define PWMDBG(fmt, args ...)	pr_debug("pwm %5d: " fmt, __LINE__, ##args)
#else
#define PWMDBG(fmt, args ...)
#endif

#define PWMMSG(fmt, args ...)	pr_debug(fmt, ##args)

#define PWM_DEVICE "mt-pwm"
#ifdef CONFIG_OF
extern void __iomem *pwm_base;
/* unsigned int pwm_irqnr; */

#undef PWM_BASE
#define PWM_BASE	pwm_base
#else
#define PWM_BASE	0x11070000
#endif

/**********************************
* Global enum data
***********************************/
/******************* Register Manipulations*****************/
#define mt_reg_sync_writel(v, a) \
	do {    \
		__raw_writel((v), (void __force __iomem *)(PWM_BASE + a));   \
		mb();   /*fix for memory barrier without comment */ \
	} while (0)

#define INREG32(reg)	__raw_readl((void *)(PWM_BASE + reg))
#define OUTREG32(reg, val)	mt_reg_sync_writel(val, reg)
#define SETREG32(reg, val)	OUTREG32(reg, INREG32(reg)|(val))
#define CLRREG32(reg, val)	OUTREG32(reg, INREG32(reg)&~(val))
#define MASKREG32(x, y, z)	OUTREG32(x, (INREG32(x)&~(y))|(z))
#define PWM_NEW_MODE_DUTY_TOTAL_BITS	64
/**
 *@}
 */
int mt_get_pwm_clk_src(struct platform_device *pdev);

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm channel nums def.
 * value is from 0 to 31.
 */
enum PWM {
	/** 0: pwm channel num 0*/
	PWM0 = 0,
	/** 1: pwm channel num 1*/
	PWM1,
	/** 2: pwm channel num 2*/
	PWM2,
	/** 3: pwm channel num 3*/
	PWM3,
	/** 4: pwm channel num 4*/
	PWM4,
	/** 5: pwm channel num 5*/
	PWM5,
	/** 6: pwm channel num 6*/
	PWM6,
	/** 7: pwm channel num 7*/
	PWM7,
	/** 8: pwm channel num 8*/
	PWM8,
	/** 9: pwm channel num 9*/
	PWM9,
	/** 10: pwm channel num 10*/
	PWM10,
	/** 11: pwm channel num 11*/
	PWM11,
	/** 12: pwm channel num 12*/
	PWM12,
	/** 13: pwm channel num 13*/
	PWM13,
	/** 14: pwm channel num 14*/
	PWM14,
	/** 15: pwm channel num 15*/
	PWM15,
	/** 16: pwm channel num 16*/
	PWM16,
	/** 17: pwm channel num 17*/
	PWM17,
	/** 18: pwm channel num 18*/
	PWM18,
	/** 19: pwm channel num 19*/
	PWM19,
	/** 20: pwm channel num 20*/
	PWM20,
	/** 21: pwm channel num 21*/
	PWM21,
	/** 22: pwm channel num 22*/
	PWM22,
	/** 23: pwm channel num 23*/
	PWM23,
	/** 24: pwm channel num 24*/
	PWM24,
	/** 25: pwm channel num 25*/
	PWM25,
	/** 26: pwm channel num 26*/
	PWM26,
	/** 27: pwm channel num 27*/
	PWM27,
	/** 28: pwm channel num 28*/
	PWM28,
	/** 29: pwm channel num 29*/
	PWM29,
	/** 30: pwm channel num 30*/
	PWM30,
	/** 31: pwm channel num 31*/
	PWM31
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm test select bit.
 * value is from 0 and 1.
 */
enum TEST_SEL_BIT {
	/** 0: pwm test not sel*/
	TEST_SEL_FALSE,
	/** 1: pwm test sel*/
	TEST_SEL_TRUE
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm mode bit.
 * value is from 0 and 1.
 */
enum PWM_CON_MODE_BIT {
	/** 0: pwm mode period*/
	PERIOD,
	/** 1: pwm mode rand*/
	RAND
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm data source bit.
 * value is from 0 and 1.
 */
enum PWM_CON_SRCSEL_BIT {
	/** 0: pwm data source from fifo*/
	PWM_FIFO,
	/** 1: pwm data source from memory*/
	MEMORY
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm idle bit.
 * value is from 0 and 2.
 */
enum PWM_CON_IDLE_BIT {
	/** 0: pwm idle value low*/
	IDLE_FALSE,
	/** 1: pwm  idle value  high*/
	IDLE_TRUE,
	/** 2: pwm  idle value max*/
	IDLE_MAX
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm guard bit.
 * value is from 0 and 2.
 */
enum PWM_CON_GUARD_BIT {
	/** 0: pwm guard value low*/
	GUARD_FALSE,
	/** 1: pwm guard value high*/
	GUARD_TRUE,
	/** 2: pwm guard value max*/
	GUARD_MAX
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm old mode enable bit.
 * value is from 0 and 1.
 */
enum OLD_MODE_BIT {
	/** 0: pwm oldmode disable*/
	OLDMODE_DISABLE,
	/** 1: pwm oldmode enable*/
	OLDMODE_ENABLE
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm buf valid bit.
 * value is from 0 and 2.
 */
enum PWM_BUF_VALID_BIT {
	/** 0: pwm buf0 valid*/
	BUF0_VALID,
	/** 1: pwm buf1 valid*/
	BUF1_VALID,
	/** 2: pwm buf0 max*/
	BUF_MAX
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm clock source bit.
 * value is from 0 and 1.
 */
enum CLOCK_SRC {
	/** 0: pwm clock Block*/
	CLK_BLOCK,
	/** 1: pwm clock BLOCK_BY_1625*/
	CLK_BLOCK_BY_1625_OR_32K
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm clock div bit.
 * value is from 0 and 8.
 */
enum PWM_CLK_DIV {
	/** 0: pwm clk scale div min*/
	CLK_DIV_MIN,
	/** 1: pwm clk scale div 1*/
	CLK_DIV1 = CLK_DIV_MIN,
	/** 2: pwm clk scale div 2*/
	CLK_DIV2,
	/** 3: pwm clk scale div 4*/
	CLK_DIV4,
	/** 4: pwm clk scale div 8*/
	CLK_DIV8,
	/** 5: pwm clk scale div 16*/
	CLK_DIV16,
	/** 6: pwm clk scale div 32*/
	CLK_DIV32,
	/** 7: pwm clk scale div 64*/
	CLK_DIV64,
	/** 8: pwm clk scale div 128*/
	CLK_DIV128,
	/** 9: pwm clk scale div max*/
	CLK_DIV_MAX
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm interrupt enable bit.
 * value is from 0 and 2.
 */
enum PWM_INT_ENABLE_BITS {
	/** 0: pwm interrupt finish enable*/
	PWM_INT_FINISH_EN = 0,
	/** 1: pwm interrupt underflow enable*/
	PWM_INT_UNDERFLOW_EN,
	/** 2: pwm interrupt enable max */
	PWM_INT_ENABLE_BITS_MAX,
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm interrupt status bit.
 * value is from 0 and 2.
 */
enum PWM_INT_STATUS_BITS {
	/** 0: pwm interrupt finish state*/
	PWM_INT_FINISH_ST = 0,
	/** 1: pwm interrupt underflow state*/
	PWM_INT_UNDERFLOW_ST,
	/** 2: pwm interrupt state max*/
	PWM_INT_STATUS_BITS_MAX,
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm interrupt ack bit.
 * value is from 0 and 2.
 */
enum PWM_INT_ACK_BITS {
	/** 0: pwm interrupt finish ack*/
	PWM_INT_FINISH_ACK = 0,
		/** 1: pwm interrupt underflow ack*/
	PWM_INT_UNDERFLOW_ACK,
		/** 2: pwm interrupt ack max*/
	PWM_INT_ACK_BITS_MAX,
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm clock source bit.
 * value is from 0 and 5.
 */
enum PWM_CLOCK_SRC_ENUM {
	/** 0: pwm clock source min*/
	PWM_CLK_SRC_MIN,
	/** 0: pwm clock source Block*/
	PWM_CLK_OLD_MODE_BLOCK = PWM_CLK_SRC_MIN,
	/** 1: pwm clock source 32K*/
	PWM_CLK_OLD_MODE_32K,
	/** 2: pwm clock source new  Block*/
	PWM_CLK_NEW_MODE_BLOCK,
	/** 3: pwm clock source new  Block div 1625*/
	PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625,
	/** 4: pwm clock source num*/
	PWM_CLK_SRC_NUM,
	/** 5: pwm clock source invalid*/
	PWM_CLK_SRC_INVALID,
};

/** @ingroup IP_group_pwm_internal_enum
 * @brief Pwm mode bit.
 * value is from 0 and 6.
 */
enum PWM_MODE_ENUM {
	/** 0: pwm mode sel min*/
	PWM_MODE_MIN,
	/** 0: pwm mode old*/
	PWM_MODE_OLD = PWM_MODE_MIN,
	/** 1: pwm mode fifo*/
	PWM_MODE_FIFO,
	/** 2: pwm mode memory*/
	PWM_MODE_MEMORY,
	/** 3: pwm mode random*/
	PWM_MODE_RANDOM,
	/** 4: pwm mode delay*/
	PWM_MODE_DELAY,
	/** 5: pwm mode 3dlcm*/
	PWM_MODE_3DLCM,
	/** 6: pwm mode invalid*/
	PWM_MODE_INVALID,
};

void mt_set_pwm_3dlcm_enable_hal(u8 enable);
void mt_set_pwm_3dlcm_inv_hal(u32 pwm_no, u8 inv);
void mt_set_pwm_3dlcm_base_hal(u32 pwm_no, u8 enable);
void mt_set_pwm_3dlcm_aux_hal(u32 pwm_no, u8 enable);
void mt_pwm_26M_clk_enable_hal(u32 enable);
void mt_set_pwm_enable_hal(u32 pwm_no);
void mt_set_pwm_disable_hal(u32 pwm_no);
void mt_set_pwm_enable_seqmode_hal(void);
void mt_set_pwm_clk_hal(u32 pwm_no, u32 clksrc, u32 div);
s32 mt_set_pwm_con_datasrc_hal(u32 pwm_no, u32 val);
s32 mt_set_pwm_con_mode_hal(u32 pwm_no, u32 val);
s32 mt_set_pwm_con_idleval_hal(u32 pwm_no, u16 val);
s32 mt_set_pwm_con_guardval_hal(u32 pwm_no, u16 val);
void mt_set_pwm_con_stpbit_hal(u32 pwm_no, u32 stpbit, u32 srcsel);
s32 mt_set_pwm_con_oldmode_hal(u32 pwm_no, u32 val);
void mt_set_pwm_hidur_hal(u32 pwm_no, u16 dur_val);
void mt_set_pwm_lowdur_hal(u32 pwm_no, u16 dur_val);
void mt_set_pwm_guarddur_hal(u32 pwm_no, u16 dur_val);
void mt_set_pwm_send_data0_hal(u32 pwm_no, u32 data);
void mt_set_pwm_send_data1_hal(u32 pwm_no, u32 data);
void mt_set_pwm_wave_num_hal(u32 pwm_no, u16 num);
void mt_set_pwm_data_width_hal(u32 pwm_no, u16 width);
void mt_set_pwm_thresh_hal(u32 pwm_no, u16 thresh);
void mt_set_intr_enable_hal(u32 pwm_no, u8 mode);
void mt_set_intr_ack_hal(u32 pwm_no, u8 mode);
void mt_pwm_dump_regs_hal(void);
void pwm_debug_store_hal(void);
void pwm_debug_show_hal(void);
void mt_set_pwm_valid_hal(u32 pwm_no, u32 buf_valid_bit);
void mt_set_pwm_delay_duration_hal(unsigned long pwm_delay_reg, u16 val);
void mt_set_pwm_delay_clock_hal(unsigned long pwm_delay_reg, u32 clksrc);
void mt_set_pwm_buf0_addr_hal(u32 pwm_no, u32 addr);
void mt_set_pwm_buf0_size_hal(u32 pwm_no, u16 size);
void mt_set_pwm_buf1_addr_hal(u32 pwm_no, u32 addr);
void mt_set_pwm_buf1_size_hal(u32 pwm_no, u16 size);

#endif
