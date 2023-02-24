/*******************************************************************************
*
* Copyright (c) 2012, Media Teck.inc
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public Licence,
* version 2, as publish by the Free Software Foundation.
*
* This program is distributed and in hope it will be useful, but WITHOUT
* ANY WARRNTY; without even the implied warranty of MERCHANTABITLITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*/

/**
 * @file mt_pwm_hal.c
 */

/**
 * @defgroup IP_group_pwm PWM
 *   There are total 32 pwm channels in MT3611,\n
 *   Each channel has it's own Clock Source,ect.\n
 *
 *   @{
 *       @defgroup IP_group_pwm_external EXTERNAL
 *         The external API document for pwm. \n
 *
 *         @{
 *             @defgroup IP_group_pwm_external_function 1.function
 *               External function used in pwm driver.
 *             @defgroup IP_group_pwm_external_struct 2.structure
 *               External structure used in pwm driver.
 *             @defgroup IP_group_pwm_external_typedef 3.typedef
 *               none. No extra typedef used.
 *             @defgroup IP_group_pwm_external_enum 4.enumeration
 *               none. No extra enumeration in pwm driver.
 *             @defgroup IP_group_pwm_external_def 5.define
 *               none. No extra define in pwm driver.
 *         @}
 *
 *       @defgroup IP_group_pwm_internal INTERNAL
 *         The internal API document for pwm. \n
 *
 *         @{
 *             @defgroup IP_group_pwm_internal_function 1.function
 *               Internal function used in pwm driver.
 *             @defgroup IP_group_pwm_internal_struct 2.structure
 *               Internal structure used in pwm driver.
 *             @defgroup IP_group_pwm_internal_typedef 3.typedef
 *               none. No extra typedef used  in pwm driver.
 *             @defgroup IP_group_pwm_internal_enum 4.enumeration
 *               Internal enumeration used in pwm driver.
 *             @defgroup IP_group_pwm_internal_def 5.define
 *               Internal define used in pwm driver.
 *         @}
 *   @}
 */

#include <linux/types.h>

#include "mt_pwm_hal.h"
#include "mt_pwm_reg.h"

/**
 * @brief a global pwm register offset define.
 */

enum {
	/** 0: pwm con reg offset */
	PWM_CON,
	/** 1: pwm high duration reg offset */
	PWM_HDURATION,
	/** 2: pwm low durationreg offset */
	PWM_LDURATION,
	/** 3: pwm gduration reg offset */
	PWM_GDURATION,
	/** 4: pwm buf0 reg offset */
	PWM_BUF0_BASE_ADDR,
	/** 5: pwm buf0 size reg offset */
	PWM_BUF0_SIZE,
	/** 6: pwm buf1 reg offset */
	PWM_BUF1_BASE_ADDR,
	/** 7: pwm buf1 size reg offset */
	PWM_BUF1_SIZE,
	/** 8: pwm send data0 reg offset */
	PWM_SEND_DATA0,
	/** 9: pwm send data1 reg offset */
	PWM_SEND_DATA1,
	/** 10: pwm wave num reg offset */
	PWM_WAVE_NUM,
	/** 11: pwm data width reg offset */
	PWM_DATA_WIDTH,
	/** 12: pwm thresh reg offset */
	PWM_THRESH,
	/** 13: pwm thresh reg offset */
	PWM_SEND_WAVENUM,
	/** 14: pwm send wavenum reg offset */
	PWM_VALID
} PWM_REG_OFF;

/**************************************************************/

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm enable reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_enable_hal(u32 pwm_no)
{
	SETREG32(PWM_ENABLE, 1 << pwm_no);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm disable reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_disable_hal(u32 pwm_no)
{
	CLRREG32(PWM_ENABLE, 1 << pwm_no);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm enable seqmode reg operation.
 * @par Parameters
 *	  none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_enable_seqmode_hal(void)
{
	SETREG32(PWM_SEQ_MODE, PWM_SEQ_MODE_EN);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm setting clk reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     clksrc: 26M/1625 = 16KHz,32K,Block.
 * @param[in]
 *     div: Clock division.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_clk_hal(u32 pwm_no, u32 clksrc, u32 div)
{
	unsigned long reg_con;

	reg_con = (unsigned long)PWM_CH_REG(pwm_no, PWM_CON);
	MASKREG32(reg_con, CLKDIV, div);
	if ((clksrc & 0x80000000) != 0) {
		clksrc &= ~(0x80000000);
		if (clksrc == CLK_BLOCK_BY_1625_OR_32K) {
			/* old mode: 26M/1625 = 16KHz */
			CLRREG32(reg_con, CLKSEL_OLD);
			/* bit 4: 0 */
			SETREG32(reg_con, CLKSEL);
			/* bit 3: 1 */
		} else {	/* old mode 32k clk */
			SETREG32(reg_con, CLKSEL_OLD);
			SETREG32(reg_con, CLKSEL);
		}
	} else {
		CLRREG32(reg_con, CLKSEL_OLD);
		if (clksrc == CLK_BLOCK)
			CLRREG32(reg_con, CLKSEL);
		else if (clksrc == CLK_BLOCK_BY_1625_OR_32K)
			SETREG32(reg_con, CLKSEL);
	}
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm data src reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 is fifo mode, 1 is memory mode.
 * @return
 *     0, function success.\n
 *     1, not support data source type.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
s32 mt_set_pwm_con_datasrc_hal(u32 pwm_no, u32 val)
{
	unsigned long reg_con;

	reg_con = (unsigned long)PWM_CH_REG(pwm_no, PWM_CON);
	if (val == PWM_FIFO)
		CLRREG32(reg_con, SRCSEL);
	else if (val == MEMORY)
		SETREG32(reg_con, SRCSEL);
	else
		return 1;
	return 0;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm mode reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 is period mode,1 is random mode.
 * @return
 *     0, function success.\n
 *     1, not support pwm mode.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
s32 mt_set_pwm_con_mode_hal(u32 pwm_no, u32 val)
{
	unsigned long reg_con;

	reg_con = (unsigned long)PWM_CH_REG(pwm_no, PWM_CON);
	if (val == PERIOD)
		CLRREG32(reg_con, MODE);
	else if (val == RAND)
		SETREG32(reg_con, MODE);
	else
		return 1;
	return 0;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  idle val reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 means that  idle state is not put out,idle_false.
 *	 1 means that idle state is put out,idle_true.
 * @return
 *     0, function success.\n
 *     1, wrong idle bit.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
s32 mt_set_pwm_con_idleval_hal(u32 pwm_no, u16 val)
{
	unsigned long reg_con;

	reg_con = (unsigned long)PWM_CH_REG(pwm_no, PWM_CON);
	if (val == IDLE_TRUE)
		SETREG32(reg_con, IDLE_VALUE);
	else if (val == IDLE_FALSE)
		CLRREG32(reg_con, IDLE_VALUE);
	else
		return 1;
	return 0;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm guard val reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 means that  guard state is not put out,guard_false.
 *	 1 means that guard state is put out,guard_true.
 * @return
 *     0, function success.\n
 *     1, wrong pwm guard value bit.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
s32 mt_set_pwm_con_guardval_hal(u32 pwm_no, uint16_t val)
{
	unsigned long reg_con;

	reg_con = (unsigned long)PWM_CH_REG(pwm_no, PWM_CON);
	if (val == GUARD_TRUE)
		SETREG32(reg_con, GUARD_VALUE);
	else if (val == GUARD_FALSE)
		CLRREG32(reg_con, GUARD_VALUE);
	else
		return 1;
	return 0;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm stopbits reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     stpbit: Stop bits.
 * @param[in]
 *     srcsel: 0 is fifo mode, 1 is memory mode.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     stop bits should be less then 0x3f
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_con_stpbit_hal(u32 pwm_no, u32 stpbit, u32 srcsel)
{
	unsigned long reg_con;

	reg_con = (unsigned long)PWM_CH_REG(pwm_no, PWM_CON);
	if (srcsel == PWM_FIFO)
		MASKREG32(reg_con, STOP_BITPOS,
			  stpbit << STOP_BITS_OFFSET);
	if (srcsel == MEMORY)
		MASKREG32(reg_con,
			  STOP_BITPOS & (0x1f <<
						    STOP_BITS_OFFSET),
			  stpbit << STOP_BITS_OFFSET);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm oldmode enable or not.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 means disable oldmode,oldmode_disable.
 *	 1 means enable oldmode,odlmode_enable.
 * @return
 *     0, function success.\n
 *     1, wrong pwm mode enable bit.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
s32 mt_set_pwm_con_oldmode_hal(u32 pwm_no, u32 val)
{
	unsigned long reg_con;

	reg_con = (unsigned long)PWM_CH_REG(pwm_no, PWM_CON);
	if (val == OLDMODE_DISABLE)
		CLRREG32(reg_con, OLD_PWM_MODE);
	else if (val == OLDMODE_ENABLE)
		SETREG32(reg_con, OLD_PWM_MODE);
	else
		return 1;
	return 0;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm high duration reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     dur_val: High duration value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_hidur_hal(u32 pwm_no, uint16_t dur_val)
{				/* only low 16 bits are valid */
	unsigned long reg_hi_dur;

	reg_hi_dur = (unsigned long)PWM_CH_REG(pwm_no, PWM_HDURATION);
	OUTREG32(reg_hi_dur, dur_val);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm low duration reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     dur_val: Low duration value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_lowdur_hal(u32 pwm_no, uint16_t dur_val)
{
	unsigned long reg_low_dur;

	reg_low_dur = (unsigned long)PWM_CH_REG(pwm_no, PWM_LDURATION);
	OUTREG32(reg_low_dur, dur_val);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  guard duration reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     dur_val: Guard duration value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_guarddur_hal(u32 pwm_no, uint16_t dur_val)
{
	unsigned long reg_guard_dur;

	reg_guard_dur = (unsigned long)PWM_CH_REG(pwm_no, PWM_GDURATION);
	OUTREG32(reg_guard_dur, dur_val);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  send data0 reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     data:  Send data0 value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_send_data0_hal(u32 pwm_no, u32 data)
{
	unsigned long reg_data0;

	reg_data0 = (unsigned long)PWM_CH_REG(pwm_no, PWM_SEND_DATA0);
	OUTREG32(reg_data0, data);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  send data1 reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     data:  Send data1 value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_send_data1_hal(u32 pwm_no, u32 data)
{
	unsigned long reg_data1;

	reg_data1 = (unsigned long)PWM_CH_REG(pwm_no, PWM_SEND_DATA1);
	OUTREG32(reg_data1, data);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  wave num reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     num:  Wave num value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_wave_num_hal(u32 pwm_no, uint16_t num)
{
	unsigned long reg_wave_num;

	reg_wave_num = (unsigned long)PWM_CH_REG(pwm_no, PWM_WAVE_NUM);
	OUTREG32(reg_wave_num, num);
}

/** @ingroup IP_group_pwm_internal_function
 *     Pwm  data width config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     width: The data width of the wave.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_data_width_hal(u32 pwm_no, uint16_t width)
{
	unsigned long reg_data_width;

	reg_data_width = (unsigned long)PWM_CH_REG(pwm_no, PWM_DATA_WIDTH);
	OUTREG32(reg_data_width, width);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  thresh reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     thresh:  The thresh of the wave.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_thresh_hal(u32 pwm_no, uint16_t thresh)
{
	unsigned long reg_thresh;

	reg_thresh =  (unsigned long)PWM_CH_REG(pwm_no, PWM_THRESH);
	OUTREG32(reg_thresh, thresh);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm Set intr enable register.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     mode: Finish_en or underflow_en
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_intr_enable_hal(u32 pwm_no, u8 mode)
{
	if (pwm_no > 15) {
		if (mode == PWM_INT_FINISH_EN)
			SETREG32(PWM_INT_ENABLE_1, 1 << ((pwm_no % 16) * 2));
		else if (mode == PWM_INT_UNDERFLOW_EN)
			SETREG32(PWM_INT_ENABLE_1, 1 <<
			((pwm_no % 16) * 2 + 1));
	} else {
		if (mode == PWM_INT_FINISH_EN)
			SETREG32(PWM_INT_ENABLE, 1 << (pwm_no * 2));
		else if (mode == PWM_INT_UNDERFLOW_EN)
			SETREG32(PWM_INT_ENABLE, 1 << (pwm_no * 2 + 1));
	}
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm Set intr ack register.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     mode: Finish_ack or underflow_ack.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_intr_ack_hal(u32 pwm_no, u8 mode)
{
	if (pwm_no > 15) {
		if (mode == PWM_INT_FINISH_ACK)
			SETREG32(PWM_INT_ACK_1, 1 << ((pwm_no % 16) * 2));
		else if (mode == PWM_INT_UNDERFLOW_ACK)
			SETREG32(PWM_INT_ACK_1, 1 << ((pwm_no % 16) * 2 + 1));
	} else {
		if (mode == PWM_INT_FINISH_ACK)
			SETREG32(PWM_INT_ACK, 1 << (pwm_no * 2));
		else if (mode == PWM_INT_UNDERFLOW_ACK)
			SETREG32(PWM_INT_ACK, 1 << (pwm_no * 2 + 1));
	}
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf0 addr reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     addr:  Buf0 data address value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_buf0_addr_hal(u32 pwm_no, u32 addr)
{
	unsigned long reg_buff0_addr;

	reg_buff0_addr = (unsigned long)PWM_CH_REG(pwm_no, PWM_BUF0_BASE_ADDR);
	OUTREG32(reg_buff0_addr, addr);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf0 size reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     size:  Buf0 size value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_buf0_size_hal(u32 pwm_no, uint16_t size)
{
	unsigned long reg_buff0_size;

	reg_buff0_size = (unsigned long)PWM_CH_REG(pwm_no, PWM_BUF0_SIZE);
	OUTREG32(reg_buff0_size, size);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf1 addr reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     addr:  Buf1 data address value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_buf1_addr_hal(u32 pwm_no, u32 addr)
{
	unsigned long reg_buff0_addr;

	reg_buff0_addr = (unsigned long)PWM_CH_REG(pwm_no, PWM_BUF1_BASE_ADDR);
	OUTREG32(reg_buff0_addr, addr);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf1 size reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     size:  Buf1 size value.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_buf1_size_hal(u32 pwm_no, uint16_t size)
{
	unsigned long reg_buff0_size;

	reg_buff0_size = (unsigned long)PWM_CH_REG(pwm_no, PWM_BUF1_SIZE);
	OUTREG32(reg_buff0_size, size);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  set pwm valid  reg operation.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     buf_valid_bit: For buf0: bit0 and bit1 should be set 1.
 *     For buf1: bit2 and bit3 should be set 1.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_valid_hal(u32 pwm_no, u32 buf_valid_bit)
{
	unsigned long reg_thresh;

	reg_thresh = (unsigned long)PWM_CH_REG(pwm_no, PWM_VALID);
	SETREG32(reg_thresh, 1 << buf_valid_bit);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  set delayduration  reg operation.
 * @param[in]
 *     pwm_delay_reg: Register of delay SEQ mode .
 * @param[in]
 *     val:  Delay duration value when using SEQ mode.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_delay_duration_hal(unsigned long pwm_delay_reg, u16 val)
{
	MASKREG32(pwm_delay_reg, PWM_DELAY_DURATION, val);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  set delay clock config.
 * @param[in]
 *     pwm_delay_reg: Register of delay SEQ mode .
 * @param[in]
 *     clksrc: Pwm delay clock src when using SEQ mode.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_delay_clock_hal(unsigned long pwm_delay_reg, u32 clksrc)
{
	MASKREG32(pwm_delay_reg, DELAY_CLKSEL, clksrc << 16);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm dump regs reg operation.
 * @par Parameters
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_pwm_dump_regs_hal(void)
{
	int i;
	unsigned long reg_val, reg_con;

	reg_val = INREG32(PWM_ENABLE);
	PWMMSG("\r\n[PWM_ENABLE is:%lx]\n\r ", reg_val);
	reg_val = INREG32(PWM_CK_26M_SEL);
	PWMMSG("\r\n[PWM_26M_SEL is:%lx]\n\r ", reg_val);
	/* PWMDBG("peri pdn0 clock: 0x%x\n", INREG32(INFRA_PDN_STA0)); */

	for (i = PWM_MIN; i <= PWM_MAX; i++) {
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_CON);
		reg_val = INREG32(reg_con);
		PWMMSG("\r\n[PWM%d_CON is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_HDURATION);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_HDURATION is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_LDURATION);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_LDURATION is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_GDURATION);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_GDURATION is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_BUF0_BASE_ADDR);
		reg_val = INREG32(reg_con);
		PWMMSG("\r\n[PWM%d_BUF0_BASE_ADDR is:%lx]\r\n", i, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_BUF0_SIZE);
		reg_val = INREG32(reg_con);
		PWMMSG("\r\n[PWM%d_BUF0_SIZE is:%lx]\r\n", i, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_BUF1_BASE_ADDR);
		reg_val = INREG32(reg_con);
		PWMMSG("\r\n[PWM%d_BUF1_BASE_ADDR is:%lx]\r\n", i, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_BUF1_SIZE);
		reg_val = INREG32(reg_con);
		PWMMSG("\r\n[PWM%d_BUF1_SIZE is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_SEND_DATA0);
		reg_val = INREG32(reg_con);

		PWMMSG("[PWM%d_SEND_DATA0 is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_SEND_DATA1);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_PWM_SEND_DATA1 is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_WAVE_NUM);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_WAVE_NUM is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_DATA_WIDTH);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_WIDTH is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_THRESH);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_THRESH is:%lx]\r\n", i + 1, reg_val);
		reg_con = (unsigned long)PWM_CH_REG(i, PWM_SEND_WAVENUM);
		reg_val = INREG32(reg_con);
		PWMMSG("[PWM%d_SEND_WAVENUM is:%lx]\r\n", i + 1, reg_val);

	}
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm used by store API for self test.
 * @par Parameters
 *	   none.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void pwm_debug_store_hal(void)
{
	/* dump clock status */
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm used by show API for self test.
 * @par Parameters
 *	   none.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void pwm_debug_show_hal(void)
{
	mt_pwm_dump_regs_hal();
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm 3dlcm Eable reg operation.
 * @param[in]
 *     enable: 3dlcm enable true or false.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_3dlcm_enable_hal(u8 enable)
{
	SETREG32(PWM_3DLCM, PWM_3DLCM_EN);
}


/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm 3dlcm channel Auxiliary Inverse or  Same as Base reg operation.
 * @param[in]
 *     pwm_no: Auxiliary Pwm channel,pwm0~pwm31.
 * @param[in]
 *     inv: Inverse or the Same as Base.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_3dlcm_inv_hal(u32 pwm_no, u8 inv)
{
	/* set "pwm_no" auxiliary inverse or the same as base */
	if (inv)
		SETREG32(PWM_3DLCM, 1 << (pwm_no - 7));
	else
		CLRREG32(PWM_3DLCM, 1 << (pwm_no - 7));
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm 3dlcm Setting Pwm channel as Auxiliary reg operation.
 * @param[in]
 *     pwm_no: Auxiliary Pwm channel,pwm0~pwm31.
 * @param[in]
 *     enable: 3dlcm Enable True or False.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_3dlcm_aux_hal(u32 pwm_no, u8 enable)
{
	/* set "pwm_no" as auxiliary */
	if (enable)
		SETREG32(PWM_3DLCM, 1 << (pwm_no + 9));
	else
		CLRREG32(PWM_3DLCM, 1 << (pwm_no + 9));
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm 3dlcm Setting Pwm channel as Base reg operation.
 * @param[in]
 *     pwm_no: Base Pwm channel,pwm0~pwm31.
 * @param[in]
 *     enable: 3dlcm Enable True or False.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_set_pwm_3dlcm_base_hal(u32 pwm_no, u8 enable)
{
	CLRREG32(PWM_3DLCM, 0xFF << 8);
	if (enable)
		SETREG32(PWM_3DLCM, 1 << (pwm_no + 1));
	else
		CLRREG32(PWM_3DLCM, 1 << (pwm_no + 1));
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm clk enable or disable reg operation.
 * @param[in]
 *     enable: Pwm 26M clk enable or not.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_pwm_26M_clk_enable_hal(u32 enable)
{
	unsigned long reg_con;

	/* select 66M or 26M */
	reg_con = (unsigned long)PWM_CK_26M_SEL;
	if (enable)
		SETREG32(reg_con, PWM_26M_SEL);
	else
		CLRREG32(reg_con, PWM_26M_SEL);

}
