/*
 * Copyright (C) 2018 MediaTek Inc.
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
* @file pmic_pwm.c
*/
/**
 * @defgroup IP_group_pmic_pwm PMIC_PWM
 *	 There are total 5 pwm channels in MT3615,\n
 *	 Follow as:pwmled1,pwmled2,pwmled3,pwm1,pwm2,ect.\n
 *
 *	 @{
 *		 @defgroup IP_group_pmic_pwm_external EXTERNAL
 *		   The external API document for pwm. \n
 *
 *		   @{
 *			   @defgroup IP_group_pwm_external_function 1.function
 *				 none. No extra function used in pwm driver.
 *			   @defgroup IP_group_pwm_external_struct 2.structure
 *				 none. No extra structure used in pwm driver.
 *			   @defgroup IP_group_pwm_external_typedef 3.typedef
 *				 none. No extra typedef used.
 *			   @defgroup IP_group_pwm_external_enum 4.enumeration
 *				 none. No extra enumeration in pwm driver.
 *			   @defgroup IP_group_pwm_external_def 5.define
 *				 none. No extra define in pwm driver.
 *		   @}
 *	 @}
 */

#include <linux/mutex.h>
#include <linux/delay.h>
#include "include/mt3615_pwm.h"

/**
 * @brief a global variable of mutex formutual exclusion.
 */
static struct mutex mt3615_pwm_mutex;

int32_t pwrap_pwm_write_reg(uint32_t reg_num, uint32_t val,
			    uint32_t mask, uint32_t shift)
{
	int32_t ret = 0;
	uint32_t pwm_reg = 0;

	pr_debug("[PMIC/PWM]reg_num = 0x%x , val = %d\n", reg_num, val);
	ret = regmap_read(mt3615_pwrap_regmap, reg_num,
					&pwm_reg);
	if (ret != 0) {
		pr_err("[PMIC/PWM]Reg[%x]= pmic_wrap read data fail\n"
			, reg_num);
		return ret;
	}
	pwm_reg &= ~(mask << shift);
	pwm_reg |= ((val & mask) << shift);
	ret = regmap_write(mt3615_pwrap_regmap, reg_num,
					pwm_reg);
	if (ret != 0) {
		pr_err("[PMIC/PWM]Reg[%x]= pmic_wrap read data fail\n"
			, reg_num);
		return ret;
	}
	return ret;
}

int32_t pwrap_pwm_read_reg(uint32_t reg_num, uint32_t *val,
				   uint32_t mask, uint32_t shift)
{
	int32_t ret = 0;
	uint32_t pwm_reg = 0;

	ret = regmap_read(mt3615_pwrap_regmap, (unsigned int)reg_num,
		&pwm_reg);
	if (ret != 0) {
		pr_err("[PMIC/PWM]Reg[%x]= pmic_wrap read data fail\n"
			, reg_num);
		return ret;
	}

	pwm_reg &= (mask << shift);
	*val = (pwm_reg >> shift);

	return ret;
}

int32_t mt3615_pwm_get_soc_ready(void)
{
	int32_t ret = 0;
	int32_t reg_val = 0x0;

	ret = pwrap_pwm_read_reg(MT3615_SOC_READY_STATUS_ADDR, &reg_val,
			MT3615_SOC_READY_STATUS_MASK,
			MT3615_SOC_READY_STATUS_SHIFT);
	if (ret < 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return reg_val;
}

int32_t mt3615_pwm_cfg_soc_ready(uint32_t flag)
{
	int32_t ret = 0;
	uint32_t reg_val = flag;

	ret = pwrap_pwm_write_reg(MT3615_SOC_READY_STATUS_ADDR, reg_val,
				MT3615_SOC_READY_STATUS_MASK,
				MT3615_SOC_READY_STATUS_SHIFT);
	pr_debug("[PMIC/PWM]Func:%s fail, reg_val = %d\n", __func__, reg_val);
	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     configure pwm led1 autoload register.
 * @param[in]
 *     flag:  0  disable autoload,
 *     1 enable autoload.
 * @param[in]
 *     scenarios: pwm mode:PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led1_cfg_autoload(uint32_t flag, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = flag;

	pr_debug("pwm_led1_cfg_autoload ,reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
	ret = pwrap_pwm_write_reg(PWR_BY_LED1_LOAD_DISABLE_ADDR, reg_val,
			PWR_BY_LED1_LOAD_DISABLE_MASK,
			PWR_BY_LED1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
	ret = pwrap_pwm_write_reg(PWR_ON_LED1_LOAD_DISABLE_ADDR, reg_val,
			PWR_ON_LED1_LOAD_DISABLE_MASK,
			PWR_ON_LED1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
	ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED1_LOAD_DISABLE_ADDR,
			reg_val,
			PWR_OFF_LP_LED1_LOAD_DISABLE_MASK,
			PWR_OFF_LP_LED1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
	ret = pwrap_pwm_write_reg(PWR_RB_LED1_LOAD_DISABLE_ADDR, reg_val,
			PWR_RB_LED1_LOAD_DISABLE_MASK,
			PWR_RB_LED1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
	ret = pwrap_pwm_write_reg(PWR_FAIL_LED1_LOAD_DISABLE_ADDR,
			reg_val,
			PWR_FAIL_LED1_LOAD_DISABLE_MASK,
			PWR_FAIL_LED1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
	ret = pwrap_pwm_write_reg(PWR_FW_LED1_LOAD_DISABLE_ADDR, reg_val,
			PWR_FW_LED1_LOAD_DISABLE_MASK,
			PWR_FW_LED1_LOAD_DISABLE_SHIFT);
		break;
	}
	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     configure pwm led2 autoload register.
 * @param[in]
 *     flag:  0  disable autoload,
 *     1 enable autoload.
 * @param[in]
 *     scenarios: pwm mode:PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led2_cfg_autoload(uint32_t flag, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = flag;

	pr_debug("pwm_led2_cfg_autoload ,reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_BY_LED2_LOAD_DISABLE_ADDR,
			reg_val, PWR_BY_LED2_LOAD_DISABLE_MASK,
			PWR_BY_LED2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_LED2_LOAD_DISABLE_ADDR,
			reg_val, PWR_ON_LED2_LOAD_DISABLE_MASK,
			PWR_ON_LED2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED2_LOAD_DISABLE_ADDR,
			reg_val,
			PWR_OFF_LP_LED2_LOAD_DISABLE_MASK,
			PWR_OFF_LP_LED2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_LED2_LOAD_DISABLE_ADDR,
			reg_val, PWR_RB_LED2_LOAD_DISABLE_MASK,
			PWR_RB_LED2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_LED2_LOAD_DISABLE_ADDR,
			reg_val,
			PWR_FAIL_LED2_LOAD_DISABLE_MASK,
			PWR_FAIL_LED2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_LED2_LOAD_DISABLE_ADDR,
			reg_val, PWR_FW_LED2_LOAD_DISABLE_MASK,
			PWR_FW_LED2_LOAD_DISABLE_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *      configure pwm led3 autoload register.
 * @param[in]
 *     flag:  0  disable autoload,
 *     1 enable autoload.
 * @param[in]
 *     scenarios: pwm mode:PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *      PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led3_cfg_autoload(uint32_t flag, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = flag;

	pr_debug("pwm_led3_cfg_autoload ");
	switch (scenarios) {
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_BY_LED3_LOAD_DISABLE_ADDR,
			reg_val, PWR_BY_LED3_LOAD_DISABLE_MASK,
			PWR_BY_LED3_LOAD_DISABLE_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_LED3_LOAD_DISABLE_ADDR,
			reg_val, PWR_ON_LED3_LOAD_DISABLE_MASK,
			PWR_ON_LED3_LOAD_DISABLE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED3_LOAD_DISABLE_ADDR,
			reg_val, PWR_OFF_LP_LED3_LOAD_DISABLE_MASK,
			PWR_OFF_LP_LED3_LOAD_DISABLE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_LED3_LOAD_DISABLE_ADDR,
			reg_val, PWR_RB_LED3_LOAD_DISABLE_MASK,
			PWR_RB_LED3_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_LED3_LOAD_DISABLE_ADDR,
			reg_val, PWR_FAIL_LED3_LOAD_DISABLE_MASK,
			PWR_FAIL_LED3_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_LED3_LOAD_DISABLE_ADDR,
			reg_val, PWR_FW_LED3_LOAD_DISABLE_MASK,
			PWR_FW_LED3_LOAD_DISABLE_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     configure pwm ledx autoload register.
 * @param[in]
 *     start_pwm_no: Start pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     end_pwm_no: End pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     flag:  0  disable autoload,
 *     1 enable autoload.
 * @param[in]
 *     scenarios: pwm mode:PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_ledx_cfg_autoload(uint32_t start_pwm_no,
	uint32_t end_pwm_no, uint32_t flag, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t index = 0x0;

	for (index = start_pwm_no; index <= end_pwm_no; index++) {
		pr_debug("pwm_ledx_cfg_autoload");
		switch (index) {
		case PWMLED1:
			pr_debug("PWMLED1\n");
			ret = mt3615_pwm_led1_cfg_autoload(flag, scenarios);
			break;
		case PWMLED2:
			pr_debug("PWMLED2\n");
			ret = mt3615_pwm_led2_cfg_autoload(flag, scenarios);
			break;
		case PWMLED3:
			pr_debug("PWMLED3\n");
			ret = mt3615_pwm_led3_cfg_autoload(flag, scenarios);
			break;
		}
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_ledx_cfg_autoload);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *      configure pwm1 autoload register.
  * @param[in]
 *     flag:  0  disable autoload,
 *     1 enable autoload.
 * @param[in]
 *     scenarios: pwm mode:PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

int32_t mt3615_pwm_Idx1_cfg_autoload(uint32_t flag, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = flag;

	pr_debug("mt3615_pwm_Idx1_config ");
	switch (scenarios) {
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_ON_PWM1_LOAD_DISABLE_ADDR,
			reg_val, PWR_ON_PWM1_LOAD_DISABLE_MASK,
			PWR_ON_PWM1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_PWM1_LOAD_DISABLE_ADDR,
			reg_val, PWR_ON_PWM1_LOAD_DISABLE_MASK,
			PWR_ON_PWM1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_PWM1_LOAD_DISABLE_ADDR,
			reg_val, PWR_OFF_LP_PWM1_LOAD_DISABLE_MASK,
			PWR_OFF_LP_PWM1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_PWM1_LOAD_DISABLE_ADDR,
			reg_val, PWR_RB_PWM1_LOAD_DISABLE_MASK,
			PWR_RB_PWM1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_PWM1_LOAD_DISABLE_ADDR,
			reg_val, PWR_FAIL_PWM1_LOAD_DISABLE_MASK,
			PWR_FAIL_PWM1_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_PWM1_LOAD_DISABLE_ADDR,
			reg_val, PWR_FW_PWM1_LOAD_DISABLE_MASK,
			PWR_FW_PWM1_LOAD_DISABLE_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx1_cfg_autoload);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     configure pwm2 autoload register.
 * @param[in]
 *     flag:  0  disable autoload,
 *     1 enable autoload.
 * @param[in]
 *     scenarios: pwm mode:  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,
 *     PWR_FAIL_WDT,PWR_ON2.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx2_cfg_autoload(uint32_t flag, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = flag;

	pr_debug("mt3615_pwm_Idx2_config ");
	switch (scenarios) {
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_PWM2_LOAD_DISABLE_ADDR,
			reg_val, PWR_ON_PWM2_LOAD_DISABLE_MASK,
			PWR_ON_PWM2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_PWM2_LOAD_DISABLE_ADDR,
			reg_val, PWR_OFF_LP_PWM2_LOAD_DISABLE_MASK,
			PWR_OFF_LP_PWM2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_PWM2_LOAD_DISABLE_ADDR,
			reg_val, PWR_RB_PWM2_LOAD_DISABLE_MASK,
			PWR_RB_PWM2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_PWM2_LOAD_DISABLE_ADDR,
			reg_val, PWR_FW_PWM2_LOAD_DISABLE_MASK,
			PWR_FW_PWM2_LOAD_DISABLE_SHIFT);
		break;
	case PWR_ON2:
		pr_debug("PWR_ON2\n");
		ret = pwrap_pwm_write_reg(PWR_ON2_PWM2_LOAD_DISABLE_ADDR,
			reg_val, PWR_ON2_PWM2_LOAD_DISABLE_MASK,
			PWR_ON2_PWM2_LOAD_DISABLE_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx2_cfg_autoload);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     configure corresponding PWMLEDX apply_en.
 * @param[in]
 *     start_pwm_no: Start pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     end_pwm_no: End pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_ledx_apply_en(uint32_t start_pwm_no, uint32_t end_pwm_no)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;
	uint32_t index = 0x0;

	for (index = start_pwm_no; index <= end_pwm_no; index++) {
		switch (index) {
		case PWMALL:
			pr_debug("No support yet,,ERROR!!!\n");
			break;
		case PWMLED1:
			ret |= pwrap_pwm_read_reg(LED1_APPLY_EN_ADDR, &reg_val,
					LED1_APPLY_EN_MASK,
					LED1_APPLY_EN_SHIFT);
			while (reg_val) {
				udelay(1000);
				pr_debug(",wait LED1_APPLY_EN 0:%d\n", reg_val);
				ret |= pwrap_pwm_read_reg(LED1_APPLY_EN_ADDR,
					&reg_val,
					LED1_APPLY_EN_MASK,
					LED1_APPLY_EN_SHIFT);
			}
			reg_val = 0x1;

			ret |= pwrap_pwm_write_reg(LED1_APPLY_EN_ADDR, reg_val,
					LED1_APPLY_EN_MASK,
					LED1_APPLY_EN_SHIFT);
			pr_debug("mt3615_pwm_led1_apply_en set\n");
			break;
		case PWMLED2:
			ret |= pwrap_pwm_read_reg(LED2_APPLY_EN_ADDR, &reg_val,
					LED2_APPLY_EN_MASK,
					LED2_APPLY_EN_SHIFT);
			while (reg_val) {
				udelay(1000);
				pr_debug(",wait LED2_APPLY_EN 0:%d\n", reg_val);
				ret |= pwrap_pwm_read_reg(LED2_APPLY_EN_ADDR,
					&reg_val,
					LED2_APPLY_EN_MASK,
					LED2_APPLY_EN_SHIFT);
			}
			reg_val = 0x1;

			ret |= pwrap_pwm_write_reg(LED2_APPLY_EN_ADDR, reg_val,
					LED2_APPLY_EN_MASK,
					LED2_APPLY_EN_SHIFT);
			pr_debug("mt3615_pwm_led2_apply_en set\n");
			break;
		case PWMLED3:
			ret |= pwrap_pwm_read_reg(LED3_APPLY_EN_ADDR, &reg_val,
					LED3_APPLY_EN_MASK,
					LED3_APPLY_EN_SHIFT);
			while (reg_val) {
				udelay(1000);
				pr_debug(",wait LED3_APPLY_EN 0:%d\n", reg_val);
				ret |= pwrap_pwm_read_reg(LED3_APPLY_EN_ADDR,
					&reg_val,
					LED3_APPLY_EN_MASK,
					LED3_APPLY_EN_SHIFT);
				}
			reg_val = 0x1;

			ret |= pwrap_pwm_write_reg(LED3_APPLY_EN_ADDR, reg_val,
					LED3_APPLY_EN_MASK,
					LED3_APPLY_EN_SHIFT);
			pr_debug("mt3615_pwm_led3_apply_en set\n");
			break;
		}
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_ledx_apply_en);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED1 mode sel.
 * @param[in]
 *     mode:  PWM_DISABLE,  PWM_FIXED, PWM_FLASH.
 * @param[in]
 *     scenarios: pwm mode:PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led1_mode_sel(uint32_t mode, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	reg_val = mode;

	pr_debug("mt3615_pwm_led1_mode_sel ,reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret = pwrap_pwm_write_reg(RG_LED1_MODE_ADDR, reg_val,
			RG_LED1_MODE_MASK,
			RG_LED1_MODE_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_BY_LED1_MODE_ADDR, reg_val,
			PWR_BY_LED1_MODE_MASK,
			PWR_BY_LED1_MODE_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_LED1_MODE_ADDR, reg_val,
			PWR_ON_LED1_MODE_MASK,
			PWR_ON_LED1_MODE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED1_MODE_ADDR, reg_val,
			PWR_OFF_LP_LED1_MODE_MASK,
			PWR_OFF_LP_LED1_MODE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_LED1_MODE_ADDR, reg_val,
			PWR_RB_LED1_MODE_MASK,
			PWR_RB_LED1_MODE_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_LED1_MODE_ADDR, reg_val,
			PWR_FAIL_LED1_MODE_MASK,
			PWR_FAIL_LED1_MODE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_LED1_MODE_ADDR, reg_val,
			PWR_FW_LED1_MODE_MASK,
			PWR_FW_LED1_MODE_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led1_mode_sel);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED2 mode sel.
 * @param[in]
 *     mode:  PWM_DISABLE,  PWM_FIXED, PWM_FLASH.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led2_mode_sel(uint32_t mode, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	reg_val = mode;

	pr_debug("mt3615_pwm_led2_mode_sel reg_val = %d\n", reg_val);

	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret = pwrap_pwm_write_reg(RG_LED2_MODE_ADDR, reg_val,
			RG_LED2_MODE_MASK,
			RG_LED2_MODE_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_BY_LED2_MODE_ADDR, reg_val,
			PWR_BY_LED2_MODE_MASK,
			PWR_BY_LED2_MODE_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_LED2_MODE_ADDR, reg_val,
			PWR_ON_LED2_MODE_MASK,
			PWR_ON_LED2_MODE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED2_MODE_ADDR, reg_val,
			PWR_OFF_LP_LED2_MODE_MASK,
			PWR_OFF_LP_LED2_MODE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_LED2_MODE_ADDR, reg_val,
			PWR_RB_LED2_MODE_MASK,
			PWR_RB_LED2_MODE_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_LED2_MODE_ADDR, reg_val,
			PWR_FAIL_LED2_MODE_MASK,
			PWR_FAIL_LED2_MODE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_LED2_MODE_ADDR, reg_val,
			PWR_FW_LED2_MODE_MASK,
			PWR_FW_LED2_MODE_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led2_mode_sel);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED3 mode sel.
 * @param[in]
 *     mode:  PWM_DISABLE,  PWM_FIXED, PWM_FLASH.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY, PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led3_mode_sel(uint32_t mode, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	reg_val = mode;

	pr_debug("mt3615_pwm_led3_mode_sel ,reg_val = %d\n", reg_val);

	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret = pwrap_pwm_write_reg(RG_LED3_MODE_ADDR, reg_val,
				  RG_LED3_MODE_MASK,
				  RG_LED3_MODE_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_BY_LED3_MODE_ADDR, reg_val,
				  PWR_BY_LED3_MODE_MASK,
				  PWR_BY_LED3_MODE_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_LED3_MODE_ADDR, reg_val,
				  PWR_ON_LED3_MODE_MASK,
				  PWR_ON_LED3_MODE_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED3_MODE_ADDR, reg_val,
				  PWR_OFF_LP_LED3_MODE_MASK,
				  PWR_OFF_LP_LED3_MODE_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_LED3_MODE_ADDR, reg_val,
				  PWR_RB_LED3_MODE_MASK,
				  PWR_RB_LED3_MODE_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_LED3_MODE_ADDR, reg_val,
				  PWR_FAIL_LED3_MODE_MASK,
				  PWR_FAIL_LED3_MODE_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_LED3_MODE_ADDR, reg_val,
				  PWR_FW_LED3_MODE_MASK,
				  PWR_FW_LED3_MODE_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led3_mode_sel);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLEDx  mode sel.
 * @param[in]
 *     start_pwm_no: Start pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     end_pwm_no: End pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     mode:  PWM_DISABLE,  PWM_FIXED, PWM_FLASH.
 * @param[in]
 *     scenarios: pwm mode:PWM_RG,PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_ledx_mode_sel(uint32_t start_pwm_no,
	uint32_t end_pwm_no, uint32_t mode, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t index = 0x0;

	for (index = start_pwm_no; index <= end_pwm_no; index++) {
		switch (index) {
		case PWMALL:
			pr_debug("mt3615_pwm_ledx_apply_en ,ERROR!!!\n");
			break;
		case PWMLED1:
			ret = mt3615_pwm_led1_mode_sel(mode, scenarios);
			pr_debug("mt3615_pwm_led1_mode_sel set\n");
			break;
		case PWMLED2:
			ret = mt3615_pwm_led2_mode_sel(mode, scenarios);
			pr_debug("mt3615_pwm_led2_mode_sel set\n");
			break;
		case PWMLED3:
			ret = mt3615_pwm_led3_mode_sel(mode, scenarios);
			pr_debug("mt3615_pwm_led3_mode_sel set\n");
			break;
		}

	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_ledx_mode_sel);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED1 fixed_mode config.
 * @param[in]
 *     duty_cycle:  dutyclcle.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT,
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led1_fixed_mode_config(uint32_t duty_cycle,
	uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	reg_val = duty_cycle * 10;

	pr_debug("mt3615_pwm_led1_fixed_mode_config, reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWM_RG:
		  pr_debug("Register mode\n");
		  ret = pwrap_pwm_write_reg(RG_LED1_DFIX_ADDR, reg_val,
					  RG_LED1_DFIX_MASK,
					  RG_LED1_DFIX_SHIFT);
		break;
	case PWR_STANDBY:
		  pr_debug("PWR_STANDBY\n");
		  ret = pwrap_pwm_write_reg(PWR_BY_LED1_DFIX_ADDR, reg_val,
					  PWR_BY_LED1_DFIX_MASK,
					  PWR_BY_LED1_DFIX_SHIFT);
		break;
	case PWR_ON:
		  pr_debug("PWR_ON\n");
		  ret = pwrap_pwm_write_reg(PWR_ON_LED1_DFIX_ADDR, reg_val,
					  PWR_ON_LED1_DFIX_MASK,
					  PWR_ON_LED1_DFIX_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		  pr_debug("PWR_OFF_LONGPRESS\n");
		  ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED1_DFIX_ADDR, reg_val,
					  PWR_OFF_LP_LED1_DFIX_MASK,
					  PWR_OFF_LP_LED1_DFIX_SHIFT);
		break;
	case PWR_REBOOT:
		  pr_debug("PWR_REBOOT\n");
		  ret = pwrap_pwm_write_reg(PWR_RB_LED1_DFIX_ADDR, reg_val,
					  PWR_RB_LED1_DFIX_MASK,
					  PWR_RB_LED1_DFIX_SHIFT);
		break;
	case PWR_FAIL:
		  pr_debug("PWR_FAIL\n");
		  ret = pwrap_pwm_write_reg(PWR_FAIL_LED1_DFIX_ADDR, reg_val,
					  PWR_FAIL_LED1_DFIX_MASK,
					  PWR_FAIL_LED1_DFIX_SHIFT);
		break;
	case PWR_FAIL_WDT:
		  pr_debug("PWR_FAIL_WDT\n");
		  ret = pwrap_pwm_write_reg(PWR_FW_LED1_DFIX_ADDR, reg_val,
					  PWR_FW_LED1_DFIX_MASK,
					  PWR_FW_LED1_DFIX_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led1_fixed_mode_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED2 fixed_mode config.
 * @param[in]
 *     duty_cycle:  dutyclcle.
 * @param[in]
 *     scenarios: pwm mode:P PWM_RG, WR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led2_fixed_mode_config(uint32_t duty_cycle,
	uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	reg_val = duty_cycle * 10;

	pr_debug("mt3615_pwm_led2_fixed_mode_config reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWM_RG:
		  ret = pwrap_pwm_write_reg(RG_LED2_DFIX_ADDR, reg_val,
					  RG_LED2_DFIX_MASK,
					  RG_LED2_DFIX_SHIFT);
		  pr_debug("Register mode\n");
		  break;
	case PWR_STANDBY:
		  pr_debug("PWR_STANDBY\n");
		  ret = pwrap_pwm_write_reg(PWR_BY_LED2_DFIX_ADDR, reg_val,
					  PWR_BY_LED2_DFIX_MASK,
					  PWR_BY_LED2_DFIX_SHIFT);
		  break;
	case PWR_ON:
		  pr_debug("PWR_ON\n");
		  ret = pwrap_pwm_write_reg(PWR_ON_LED2_DFIX_ADDR, reg_val,
					  PWR_ON_LED2_DFIX_MASK,
					  PWR_ON_LED2_DFIX_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		  pr_debug("PWR_OFF_LONGPRESS\n");
		  ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED2_DFIX_ADDR, reg_val,
					  PWR_OFF_LP_LED2_DFIX_MASK,
					  PWR_OFF_LP_LED2_DFIX_SHIFT);
		  break;
	case PWR_REBOOT:
		  pr_debug("PWR_REBOOT\n");
		  ret = pwrap_pwm_write_reg(PWR_RB_LED2_DFIX_ADDR, reg_val,
					  PWR_RB_LED2_DFIX_MASK,
					  PWR_RB_LED2_DFIX_SHIFT);
		  break;
	case PWR_FAIL:
		  pr_debug("PWR_FAIL\n");
		  ret = pwrap_pwm_write_reg(PWR_FAIL_LED2_DFIX_ADDR, reg_val,
					  PWR_FAIL_LED2_DFIX_MASK,
					  PWR_FAIL_LED2_DFIX_SHIFT);
		  break;
	case PWR_FAIL_WDT:
		  pr_debug("PWR_FAIL_WDT\n");
		  ret = pwrap_pwm_write_reg(PWR_FW_LED2_DFIX_ADDR, reg_val,
					  PWR_FW_LED2_DFIX_MASK,
					  PWR_FW_LED2_DFIX_SHIFT);
		  break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led2_fixed_mode_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED3 fixed_mode config.
 * @param[in]
 *     duty_cycle:  dutyclcle.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led3_fixed_mode_config(uint32_t duty_cycle,
	uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	reg_val = duty_cycle * 10;

	pr_debug("mt3615_pwm_led3_fixed_mode_config reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWM_RG:
		  pr_debug("Register mode\n");
		  ret = pwrap_pwm_write_reg(RG_LED3_DFIX_ADDR, reg_val,
					  RG_LED3_DFIX_MASK,
					  RG_LED3_DFIX_SHIFT);
		break;
	case PWR_STANDBY:
		  pr_debug("PWR_STANDBY\n");
		  ret = pwrap_pwm_write_reg(PWR_BY_LED3_DFIX_ADDR, reg_val,
					  PWR_BY_LED3_DFIX_MASK,
					  PWR_BY_LED3_DFIX_SHIFT);
		break;
	case PWR_ON:
		  pr_debug("PWR_ON\n");
		  ret = pwrap_pwm_write_reg(PWR_ON_LED3_DFIX_ADDR, reg_val,
					  PWR_ON_LED3_DFIX_MASK,
					  PWR_ON_LED3_DFIX_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		  pr_debug("PWR_OFF_LONGPRESS\n");
		  ret = pwrap_pwm_write_reg(PWR_OFF_LP_LED3_DFIX_ADDR, reg_val,
					  PWR_OFF_LP_LED3_DFIX_MASK,
					  PWR_OFF_LP_LED3_DFIX_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_LED3_DFIX_ADDR, reg_val,
					  PWR_RB_LED3_DFIX_MASK,
					  PWR_RB_LED3_DFIX_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_LED3_DFIX_ADDR, reg_val,
					  PWR_FAIL_LED3_DFIX_MASK,
					  PWR_FAIL_LED3_DFIX_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_LED3_DFIX_ADDR, reg_val,
					  PWR_FW_LED3_DFIX_MASK,
					  PWR_FW_LED3_DFIX_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led3_fixed_mode_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLEDx fixed_mode config.
 * @param[in]
 *     start_pwm_no: Start pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     end_pwm_no: End pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     duty_cycle:  dutyclcle.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_ledx_fixed_mode_config(uint32_t start_pwm_no,
	uint32_t end_pwm_no, uint32_t duty_cycle, uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t index = 0x0;

	for (index = start_pwm_no; index <= end_pwm_no; index++) {
		switch (index) {
		case PWMALL:
			pr_debug("Not support yet ,ERROR!!!\n");
			break;
		case PWMLED1:
			ret = mt3615_pwm_led1_fixed_mode_config(duty_cycle,
				scenarios);
			pr_debug("pwm_led1_fixed_mode set\n");
			break;
		case PWMLED2:
			ret = mt3615_pwm_led2_fixed_mode_config(duty_cycle,
				scenarios);
			pr_debug("pwm_led2_fixed_mode set\n");
			break;
		case PWMLED3:
			ret = mt3615_pwm_led3_fixed_mode_config(duty_cycle,
				scenarios);
			pr_debug("pwm_led3_fixed_mode set\n ");
			break;
		}

	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_ledx_fixed_mode_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED1 flash_mode config.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG,PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @param[in]
 *     pstart: start duty point.
 * @param[in]
 *     pend: end duty point.
 * @param[in]
 *     TH0: high duy height of start/end duty.
 * @param[in]
 *     TL0: low duty height of start/end duty.
 * @param[in]
 *     TR0: high-time length of start/end duty.
 * @param[in]
 *     TF0: low-time length of start/end duty.
 * @param[in]
 *     DH:  high duty height of repeat duty.
 * @param[in]
 *     TH: high duty time of repeat duty.
 * @param[in]
 *     DL: low duty height of repeat duty.
 * @param[in]
 *     TL: low duty time of repeat duty.
 * @param[in]
 *     TR: duty rising time of repeat duty.
 * @param[in]
 *     TF :duty falling time of repeat duty.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led1_flash_mode_config(uint32_t scenarios,
	uint32_t pstart, uint32_t pend, uint32_t TH0, uint32_t TL0,
	uint32_t TR0, uint32_t TF0, uint32_t DH, uint32_t TH,
	uint32_t DL, uint32_t TL, uint32_t TR, uint32_t TF)
{
	int32_t ret = 0;

	pr_debug("mt3615_pwm_led1_flash_mode_config ");
	switch (scenarios) {
	case PWM_RG:
		  pr_debug("Register mode\n");
		  ret |= pwrap_pwm_write_reg(RG_LED1_PSTART_ADDR, pstart,
					  RG_LED1_PSTART_MASK,
					  RG_LED1_PSTART_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_PEND_ADDR, pend,
					  RG_LED1_PEND_MASK,
					  RG_LED1_PEND_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_DH_ADDR, DH,
					  RG_LED1_DH_MASK,
					  RG_LED1_DH_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_DL_ADDR, DL,
					  RG_LED1_DL_MASK,
					  RG_LED1_DL_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TH0_ADDR, TH0,
					  RG_LED1_TH0_MASK,
					  RG_LED1_TH0_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TL0_ADDR, TL0,
					  RG_LED1_TL0_MASK,
					  RG_LED1_TL0_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TR0_ADDR, TR0,
					  RG_LED1_TR0_MASK,
					  RG_LED1_TR0_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TF0_ADDR, TF0,
					  RG_LED1_TF0_MASK,
					  RG_LED1_TF0_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TH_ADDR, TH,
					  RG_LED1_TH_MASK,
					  RG_LED1_TH_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TL_ADDR, TL,
					  RG_LED1_TL_MASK,
					  RG_LED1_TL_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TR_ADDR, TR,
					  RG_LED1_TR_MASK,
					  RG_LED1_TR_SHIFT);
		  ret |= pwrap_pwm_write_reg(RG_LED1_TF_ADDR, TF,
					  RG_LED1_TF_MASK,
					  RG_LED1_TF_SHIFT);
		  break;
	case PWR_STANDBY:
		  pr_debug("PWR_STANDBY\n");
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_PSTART_ADDR, pstart,
					  PWR_BY_LED1_PSTART_MASK,
					  PWR_BY_LED1_PSTART_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_PEND_ADDR, pend,
					  PWR_BY_LED1_PEND_MASK,
					  PWR_BY_LED1_PEND_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_DH_ADDR, DH,
					  PWR_BY_LED1_DH_MASK,
					  PWR_BY_LED1_DH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_DL_ADDR, DL,
					  PWR_BY_LED1_DL_MASK,
					  PWR_BY_LED1_DL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TH0_ADDR, TH0,
					  PWR_BY_LED1_TH0_MASK,
					  PWR_BY_LED1_TH0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TL0_ADDR, TL0,
					  PWR_BY_LED1_TL0_MASK,
					  PWR_BY_LED1_TL0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TR0_ADDR, TR0,
					  PWR_BY_LED1_TR0_MASK,
					  PWR_BY_LED1_TR0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TF0_ADDR, TF0,
					  PWR_BY_LED1_TF0_MASK,
					  PWR_BY_LED1_TF0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TH_ADDR, TH,
					  PWR_BY_LED1_TH_MASK,
					  PWR_BY_LED1_TH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TL_ADDR, TL,
					  PWR_BY_LED1_TL_MASK,
					  PWR_BY_LED1_TL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TR_ADDR, TR,
					  PWR_BY_LED1_TR_MASK,
					  PWR_BY_LED1_TR_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_BY_LED1_TF_ADDR, TF,
					  PWR_BY_LED1_TF_MASK,
					  PWR_BY_LED1_TF_SHIFT);
		  break;
	case PWR_ON:
		  pr_debug("PWR_ON\n");
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_PSTART_ADDR, pstart,
					  PWR_ON_LED1_PSTART_MASK,
					  PWR_ON_LED1_PSTART_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_PEND_ADDR, pend,
					  PWR_ON_LED1_PEND_MASK,
					 PWR_ON_LED1_PEND_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_DH_ADDR, DH,
					  PWR_ON_LED1_DH_MASK,
					  PWR_ON_LED1_DH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_DL_ADDR, DL,
					  PWR_ON_LED1_DL_MASK,
					  PWR_ON_LED1_DL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TH0_ADDR, TH0,
					  PWR_ON_LED1_TH0_MASK,
					  PWR_ON_LED1_TH0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TL0_ADDR, TL0,
					  PWR_ON_LED1_TL0_MASK,
					  PWR_ON_LED1_TL0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TR0_ADDR, TR0,
					  PWR_ON_LED1_TR0_MASK,
					  PWR_ON_LED1_TR0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TF0_ADDR, TF0,
					  PWR_ON_LED1_TF0_MASK,
					  PWR_ON_LED1_TF0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TH_ADDR, TH,
					  PWR_ON_LED1_TH_MASK,
					  PWR_ON_LED1_TH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TL_ADDR, TL,
					  PWR_ON_LED1_TL_MASK,
					  PWR_ON_LED1_TL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TR_ADDR, TR,
					  PWR_ON_LED1_TR_MASK,
					  PWR_ON_LED1_TR_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_ON_LED1_TF_ADDR, TF,
					  PWR_ON_LED1_TF_MASK,
					  PWR_ON_LED1_TF_SHIFT);
		  break;
	case PWR_OFF_LONGPRESS:
		  pr_debug("PWR_OFF_LONGPRESS\n");
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_PSTART_ADDR,
			pstart, PWR_OFF_LP_LED1_PSTART_MASK,
			PWR_OFF_LP_LED1_PSTART_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_PEND_ADDR, pend,
					  PWR_OFF_LP_LED1_PEND_MASK,
					  PWR_OFF_LP_LED1_PEND_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_DH_ADDR, DH,
					  PWR_OFF_LP_LED1_DH_MASK,
					  PWR_OFF_LP_LED1_DH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_DL_ADDR, DL,
					  PWR_OFF_LP_LED1_DL_MASK,
					  PWR_OFF_LP_LED1_DL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TH0_ADDR, TH0,
					  PWR_OFF_LP_LED1_TH0_MASK,
					  PWR_OFF_LP_LED1_TH0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TL0_ADDR, TL0,
					  PWR_OFF_LP_LED1_TL0_MASK,
					  PWR_OFF_LP_LED1_TL0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TR0_ADDR, TR0,
					  PWR_OFF_LP_LED1_TR0_MASK,
					  PWR_OFF_LP_LED1_TR0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TF0_ADDR, TF0,
					  PWR_OFF_LP_LED1_TF0_MASK,
					  PWR_OFF_LP_LED1_TF0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TH_ADDR, TH,
					  PWR_OFF_LP_LED1_TH_MASK,
					  PWR_OFF_LP_LED1_TH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TL_ADDR, TL,
					  PWR_OFF_LP_LED1_TL_MASK,
					  PWR_OFF_LP_LED1_TL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TR_ADDR, TR,
					  PWR_OFF_LP_LED1_TR_MASK,
					  PWR_OFF_LP_LED1_TR_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED1_TF_ADDR, TF,
					  PWR_OFF_LP_LED1_TF_MASK,
					  PWR_OFF_LP_LED1_TF_SHIFT);
		  break;
	case PWR_REBOOT:
		  pr_debug("PWR_REBOOT\n");
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_PSTART_ADDR, pstart,
					  PWR_RB_LED1_PSTART_MASK,
					  PWR_RB_LED1_PSTART_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_PEND_ADDR, pend,
					  PWR_RB_LED1_PEND_MASK,
					  PWR_RB_LED1_PEND_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_DH_ADDR, DH,
					  PWR_RB_LED1_DH_MASK,
					  PWR_RB_LED1_DH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_DL_ADDR, DL,
					  PWR_RB_LED1_DL_MASK,
					  PWR_RB_LED1_DL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TH0_ADDR, TH0,
					  PWR_RB_LED1_TH0_MASK,
					  PWR_RB_LED1_TH0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TL0_ADDR, TL0,
					  PWR_RB_LED1_TL0_MASK,
					  PWR_RB_LED1_TL0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TR0_ADDR, TR0,
					  PWR_RB_LED1_TR0_MASK,
					  PWR_RB_LED1_TR0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TF0_ADDR, TF0,
					  PWR_RB_LED1_TF0_MASK,
					  PWR_RB_LED1_TF0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TH_ADDR, TH,
					  PWR_RB_LED1_TH_MASK,
					  PWR_RB_LED1_TH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TL_ADDR, TL,
					  PWR_RB_LED1_TL_MASK,
					  PWR_RB_LED1_TL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TR_ADDR, TR,
					  PWR_RB_LED1_TR_MASK,
					  PWR_RB_LED1_TR_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_RB_LED1_TF_ADDR, TF,
					  PWR_RB_LED1_TF_MASK,
					  PWR_RB_LED1_TF_SHIFT);
		  break;
	case PWR_FAIL:
		  pr_debug("PWR_FAIL\n");
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_PSTART_ADDR, pstart,
					  PWR_FAIL_LED1_PSTART_MASK,
					  PWR_FAIL_LED1_PSTART_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_PEND_ADDR, pend,
					  PWR_FAIL_LED1_PEND_MASK,
					  PWR_FAIL_LED1_PEND_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_DH_ADDR, DH,
					  PWR_FAIL_LED1_DH_MASK,
					  PWR_FAIL_LED1_DH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_DL_ADDR, DL,
					  PWR_FAIL_LED1_DL_MASK,
					  PWR_FAIL_LED1_DL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TH0_ADDR, TH0,
					  PWR_FAIL_LED1_TH0_MASK,
					  PWR_FAIL_LED1_TH0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TL0_ADDR, TL0,
					  PWR_FAIL_LED1_TL0_MASK,
					  PWR_FAIL_LED1_TL0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TR0_ADDR, TR0,
					  PWR_FAIL_LED1_TR0_MASK,
					  PWR_FAIL_LED1_TR0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TF0_ADDR, TF0,
					  PWR_FAIL_LED1_TF0_MASK,
					  PWR_FAIL_LED1_TF0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TH_ADDR, TH,
					  PWR_FAIL_LED1_TH_MASK,
					  PWR_FAIL_LED1_TH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TL_ADDR, TL,
					  PWR_FAIL_LED1_TL_MASK,
					  PWR_FAIL_LED1_TL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TR_ADDR, TR,
					  PWR_FAIL_LED1_TR_MASK,
					  PWR_FAIL_LED1_TR_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FAIL_LED1_TF_ADDR, TF,
					  PWR_FAIL_LED1_TF_MASK,
					  PWR_FAIL_LED1_TF_SHIFT);
		  break;
	case PWR_FAIL_WDT:
		  pr_debug("PWR_FAIL_WDT\n");
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_PSTART_ADDR, pstart,
					  PWR_FW_LED1_PSTART_MASK,
					  PWR_FW_LED1_PSTART_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_PEND_ADDR, pend,
					  PWR_FW_LED1_PEND_MASK,
					  PWR_FW_LED1_PEND_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TH0_ADDR, TH0,
					  PWR_FW_LED1_TH0_MASK,
					  PWR_FW_LED1_TH0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TL0_ADDR, TL0,
					  PWR_FW_LED1_TL0_MASK,
					  PWR_FW_LED1_TL0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TR0_ADDR, TR0,
					  PWR_FW_LED1_TR0_MASK,
					  PWR_FW_LED1_TR0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TF0_ADDR, TF0,
					  PWR_FW_LED1_TF0_MASK,
					  PWR_FW_LED1_TF0_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_DH_ADDR, DH,
					  PWR_FW_LED1_DH_MASK,
					  PWR_FW_LED1_DH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TH_ADDR, TH,
					  PWR_FW_LED1_TH_MASK,
					  PWR_FW_LED1_TH_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_DL_ADDR, DL,
					  PWR_FW_LED1_DL_MASK,
					  PWR_FW_LED1_DL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TL_ADDR, TL,
					  PWR_FW_LED1_TL_MASK,
					  PWR_FW_LED1_TL_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TR_ADDR, TR,
					  PWR_FW_LED1_TR_MASK,
					  PWR_FW_LED1_TR_SHIFT);
		  ret |= pwrap_pwm_write_reg(PWR_FW_LED1_TF_ADDR, TF,
					  PWR_FW_LED1_TF_MASK,
					  PWR_FW_LED1_TF_SHIFT);
		  break;
	}
	pr_debug("~ config end.\n");

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led1_flash_mode_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED2 flash_mode config.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG,PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @param[in]
 *     pstart: start duty point.
 * @param[in]
 *     pend: end duty point.
 * @param[in]
 *     TH0: high duy height of start/end duty.
 * @param[in]
 *     TL0: low duty height of start/end duty.
 * @param[in]
 *     TR0: high-time length of start/end duty.
 * @param[in]
 *     TF0: low-time length of start/end duty.
 * @param[in]
 *     DH:  high duty height of repeat duty.
 * @param[in]
 *     TH: high duty time of repeat duty.
 * @param[in]
 *     DL: low duty height of repeat duty.
 * @param[in]
 *     TL: low duty time of repeat duty.
 * @param[in]
 *     TR: duty rising time of repeat duty.
 * @param[in]
 *     TF :duty falling time of repeat duty.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led2_flash_mode_config(uint32_t scenarios,
	uint32_t pstart, uint32_t pend, uint32_t TH0, uint32_t TL0,
	uint32_t TR0, uint32_t TF0, uint32_t DH, uint32_t TH,
	uint32_t DL, uint32_t TL, uint32_t TR, uint32_t TF)
{
	int32_t ret = 0;

	pr_debug("mt3615_pwm_led2_flash_mode_config ");
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret |= pwrap_pwm_write_reg(RG_LED2_PSTART_ADDR, pstart,
					RG_LED2_PSTART_MASK,
					RG_LED2_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_PEND_ADDR, pend,
					RG_LED2_PEND_MASK,
					RG_LED2_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_DH_ADDR, DH,
					RG_LED2_DH_MASK,
					RG_LED2_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_DL_ADDR, DL,
					RG_LED2_DL_MASK,
					RG_LED2_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TH0_ADDR, TH0,
					RG_LED2_TH0_MASK,
					RG_LED2_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TL0_ADDR, TL0,
					RG_LED2_TL0_MASK,
					RG_LED2_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TR0_ADDR, TR0,
					RG_LED2_TR0_MASK,
					RG_LED2_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TF0_ADDR, TF0,
					RG_LED2_TF0_MASK,
					RG_LED2_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TH_ADDR, TH,
					RG_LED2_TH_MASK,
					RG_LED2_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TL_ADDR, TL,
					RG_LED2_TL_MASK,
					RG_LED2_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TR_ADDR, TR,
					RG_LED2_TR_MASK,
					RG_LED2_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED2_TF_ADDR, TF,
					RG_LED2_TF_MASK,
					RG_LED2_TF_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_PSTART_ADDR, pstart,
					PWR_BY_LED2_PSTART_MASK,
					PWR_BY_LED2_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_PEND_ADDR, pend,
					PWR_BY_LED2_PEND_MASK,
					PWR_BY_LED2_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_DH_ADDR, DH,
					PWR_BY_LED2_DH_MASK,
					PWR_BY_LED2_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_DL_ADDR, DL,
					PWR_BY_LED2_DL_MASK,
					PWR_BY_LED2_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TH0_ADDR, TH0,
					PWR_BY_LED2_TH0_MASK,
					PWR_BY_LED2_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TL0_ADDR, TL0,
					PWR_BY_LED2_TL0_MASK,
					PWR_BY_LED2_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TR0_ADDR, TR0,
					PWR_BY_LED2_TR0_MASK,
					PWR_BY_LED2_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TF0_ADDR, TF0,
					PWR_BY_LED2_TF0_MASK,
					PWR_BY_LED2_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TH_ADDR, TH,
					PWR_BY_LED2_TH_MASK,
					PWR_BY_LED2_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TL_ADDR, TL,
					PWR_BY_LED2_TL_MASK,
					PWR_BY_LED2_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TR_ADDR, TR,
					PWR_BY_LED2_TR_MASK,
					PWR_BY_LED2_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED2_TF_ADDR, TF,
					PWR_BY_LED2_TF_MASK,
					PWR_BY_LED2_TF_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_PSTART_ADDR, pstart,
					PWR_ON_LED2_PSTART_MASK,
					PWR_ON_LED2_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_PEND_ADDR, pend,
					PWR_ON_LED2_PEND_MASK,
					PWR_ON_LED2_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_DH_ADDR, DH,
					PWR_ON_LED2_DH_MASK,
					PWR_ON_LED2_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_DL_ADDR, DL,
					PWR_ON_LED2_DL_MASK,
					PWR_ON_LED2_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TH0_ADDR, TH0,
					PWR_ON_LED2_TH0_MASK,
					PWR_ON_LED2_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TL0_ADDR, TL0,
					PWR_ON_LED2_TL0_MASK,
					PWR_ON_LED2_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TR0_ADDR, TR0,
					PWR_ON_LED2_TR0_MASK,
					PWR_ON_LED2_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TF0_ADDR, TF0,
					PWR_ON_LED2_TF0_MASK,
					PWR_ON_LED2_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TH_ADDR, TH,
					PWR_ON_LED2_TH_MASK,
					PWR_ON_LED2_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TL_ADDR, TL,
					PWR_ON_LED2_TL_MASK,
					PWR_ON_LED2_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TR_ADDR, TR,
					PWR_ON_LED2_TR_MASK,
					PWR_ON_LED2_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED2_TF_ADDR, TF,
					PWR_ON_LED2_TF_MASK,
					PWR_ON_LED2_TF_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_PSTART_ADDR, pstart,
					PWR_OFF_LP_LED2_PSTART_MASK,
					PWR_OFF_LP_LED2_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_PEND_ADDR, pend,
					PWR_OFF_LP_LED2_PEND_MASK,
					PWR_OFF_LP_LED2_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_DH_ADDR, DH,
					PWR_OFF_LP_LED2_DH_MASK,
					PWR_OFF_LP_LED2_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_DL_ADDR, DL,
					PWR_OFF_LP_LED2_DL_MASK,
					PWR_OFF_LP_LED2_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TH0_ADDR, TH0,
					PWR_OFF_LP_LED2_TH0_MASK,
					PWR_OFF_LP_LED2_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TL0_ADDR, TL0,
					PWR_OFF_LP_LED2_TL0_MASK,
					PWR_OFF_LP_LED2_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TR0_ADDR, TR0,
					PWR_OFF_LP_LED2_TR0_MASK,
					PWR_OFF_LP_LED2_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TF0_ADDR, TF0,
					PWR_OFF_LP_LED2_TF0_MASK,
					PWR_OFF_LP_LED2_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TH_ADDR, TH,
					PWR_OFF_LP_LED2_TH_MASK,
					PWR_OFF_LP_LED2_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TL_ADDR, TL,
					PWR_OFF_LP_LED2_TL_MASK,
					PWR_OFF_LP_LED2_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TR_ADDR, TR,
					PWR_OFF_LP_LED2_TR_MASK,
					PWR_OFF_LP_LED2_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED2_TF_ADDR, TF,
					PWR_OFF_LP_LED2_TF_MASK,
					PWR_OFF_LP_LED2_TF_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_PSTART_ADDR, pstart,
					PWR_RB_LED2_PSTART_MASK,
					PWR_RB_LED2_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_PEND_ADDR, pend,
					PWR_RB_LED2_PEND_MASK,
					PWR_RB_LED2_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_DH_ADDR, DH,
					PWR_RB_LED2_DH_MASK,
					PWR_RB_LED2_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_DL_ADDR, DL,
					PWR_RB_LED2_DL_MASK,
					PWR_RB_LED2_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TH0_ADDR, TH0,
					PWR_RB_LED2_TH0_MASK,
					PWR_RB_LED2_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TL0_ADDR, TL0,
					PWR_RB_LED2_TL0_MASK,
					PWR_RB_LED2_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TR0_ADDR, TR0,
					PWR_RB_LED2_TR0_MASK,
					PWR_RB_LED2_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TF0_ADDR, TF0,
					PWR_RB_LED2_TF0_MASK,
					PWR_RB_LED2_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TH_ADDR, TH,
					PWR_RB_LED2_TH_MASK,
					PWR_RB_LED2_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TL_ADDR, TL,
					PWR_RB_LED2_TL_MASK,
					PWR_RB_LED2_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TR_ADDR, TR,
					PWR_RB_LED2_TR_MASK,
					PWR_RB_LED2_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED2_TF_ADDR, TF,
					PWR_RB_LED2_TF_MASK,
					PWR_RB_LED2_TF_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_PSTART_ADDR, pstart,
					PWR_FAIL_LED2_PSTART_MASK,
					PWR_FAIL_LED2_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_PEND_ADDR, pend,
					PWR_FAIL_LED2_PEND_MASK,
					PWR_FAIL_LED2_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_DH_ADDR, DH,
					PWR_FAIL_LED2_DH_MASK,
					PWR_FAIL_LED2_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_DL_ADDR, DL,
					PWR_FAIL_LED2_DL_MASK,
					PWR_FAIL_LED2_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TH0_ADDR, TH0,
					PWR_FAIL_LED2_TH0_MASK,
					PWR_FAIL_LED2_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TL0_ADDR, TL0,
					PWR_FAIL_LED2_TL0_MASK,
					PWR_FAIL_LED2_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TR0_ADDR, TR0,
					PWR_FAIL_LED2_TR0_MASK,
					PWR_FAIL_LED2_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TF0_ADDR, TF0,
					PWR_FAIL_LED2_TF0_MASK,
					PWR_FAIL_LED2_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TH_ADDR, TH,
					PWR_FAIL_LED2_TH_MASK,
					PWR_FAIL_LED2_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TL_ADDR, TL,
					PWR_FAIL_LED2_TL_MASK,
					PWR_FAIL_LED2_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TR_ADDR, TR,
					PWR_FAIL_LED2_TR_MASK,
					PWR_FAIL_LED2_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED2_TF_ADDR, TF,
					PWR_FAIL_LED2_TF_MASK,
					PWR_FAIL_LED2_TF_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_PSTART_ADDR, pstart,
					PWR_FW_LED2_PSTART_MASK,
					PWR_FW_LED2_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_PEND_ADDR, pend,
					PWR_FW_LED2_PEND_MASK,
					PWR_FW_LED2_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TH0_ADDR, TH0,
					PWR_FW_LED2_TH0_MASK,
					PWR_FW_LED2_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TL0_ADDR, TL0,
					PWR_FW_LED2_TL0_MASK,
					PWR_FW_LED2_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TR0_ADDR, TR0,
					PWR_FW_LED2_TR0_MASK,
					PWR_FW_LED2_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TF0_ADDR, TF0,
					PWR_FW_LED2_TF0_MASK,
					PWR_FW_LED2_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_DH_ADDR, DH,
					PWR_FW_LED2_DH_MASK,
					PWR_FW_LED2_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TH_ADDR, TH,
					PWR_FW_LED2_TH_MASK,
					PWR_FW_LED2_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_DL_ADDR, DL,
					PWR_FW_LED2_DL_MASK,
					PWR_FW_LED2_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TL_ADDR, TL,
					PWR_FW_LED2_TL_MASK,
					PWR_FW_LED2_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TR_ADDR, TR,
					PWR_FW_LED2_TR_MASK,
					PWR_FW_LED2_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED2_TF_ADDR, TF,
					PWR_FW_LED2_TF_MASK,
					PWR_FW_LED2_TF_SHIFT);
		break;
		}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	pr_debug("config end.\n");

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led2_flash_mode_config);
/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLED3 flash_mode config.
 * @param[in]
 *     start_pwm_no: Start pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     end_pwm_no: End pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG,PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @param[in]
 *     pstart: start duty point.
 * @param[in]
 *     pend: end duty point.
 * @param[in]
 *     TH0: high duy height of start/end duty.
 * @param[in]
 *     TL0: low duty height of start/end duty.
 * @param[in]
 *     TR0: high-time length of start/end duty.
 * @param[in]
 *     TF0: low-time length of start/end duty.
 * @param[in]
 *     DH:  high duty height of repeat duty.
 * @param[in]
 *     TH: high duty time of repeat duty.
 * @param[in]
 *     DL: low duty height of repeat duty.
 * @param[in]
 *     TL: low duty time of repeat duty.
 * @param[in]
 *     TR: duty rising time of repeat duty.
 * @param[in]
 *     TF :duty falling time of repeat duty.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_led3_flash_mode_config(uint32_t scenarios,
	uint32_t pstart, uint32_t pend, uint32_t TH0, uint32_t TL0,
	uint32_t TR0, uint32_t TF0, uint32_t DH, uint32_t TH,
	uint32_t DL, uint32_t TL, uint32_t TR, uint32_t TF)
{
	int32_t ret = 0;

	DH = DH * 10;
	DL = DL * 10;
	pr_debug("mt3615_pwm_led3_flash_mode_config ");
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret |= pwrap_pwm_write_reg(RG_LED3_PSTART_ADDR, pstart,
					RG_LED3_PSTART_MASK,
					RG_LED3_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_PEND_ADDR, pend,
					RG_LED3_PEND_MASK,
					RG_LED3_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_DH_ADDR, DH,
					RG_LED3_DH_MASK,
					RG_LED3_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_DL_ADDR, DL,
					RG_LED3_DL_MASK,
					RG_LED3_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TH0_ADDR, TH0,
					RG_LED3_TH0_MASK,
					RG_LED3_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TL0_ADDR, TL0,
					RG_LED3_TL0_MASK,
					RG_LED3_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TR0_ADDR, TR0,
					RG_LED3_TR0_MASK,
					RG_LED3_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TF0_ADDR, TF0,
					RG_LED3_TF0_MASK,
					RG_LED3_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TH_ADDR, TH,
					RG_LED3_TH_MASK,
					RG_LED3_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TL_ADDR, TL,
					RG_LED3_TL_MASK,
					RG_LED3_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TR_ADDR, TR,
					RG_LED3_TR_MASK,
					RG_LED3_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_LED3_TF_ADDR, TF,
					RG_LED3_TF_MASK,
					RG_LED3_TF_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_PSTART_ADDR, pstart,
					PWR_BY_LED3_PSTART_MASK,
					PWR_BY_LED3_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_PEND_ADDR, pend,
					PWR_BY_LED3_PEND_MASK,
					PWR_BY_LED3_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_DH_ADDR, DH,
					PWR_BY_LED3_DH_MASK,
					PWR_BY_LED3_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_DL_ADDR, DL,
					PWR_BY_LED3_DL_MASK,
					PWR_BY_LED3_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TH0_ADDR, TH0,
					PWR_BY_LED3_TH0_MASK,
					PWR_BY_LED3_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TL0_ADDR, TL0,
					PWR_BY_LED3_TL0_MASK,
					PWR_BY_LED3_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TR0_ADDR, TR0,
					PWR_BY_LED3_TR0_MASK,
					PWR_BY_LED3_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TF0_ADDR, TF0,
					PWR_BY_LED3_TF0_MASK,
					PWR_BY_LED3_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TH_ADDR, TH,
					PWR_BY_LED3_TH_MASK,
					PWR_BY_LED3_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TL_ADDR, TL,
					PWR_BY_LED3_TL_MASK,
					PWR_BY_LED3_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TR_ADDR, TR,
					PWR_BY_LED3_TR_MASK,
					PWR_BY_LED3_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_LED3_TF_ADDR, TF,
					PWR_BY_LED3_TF_MASK,
					PWR_BY_LED3_TF_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_PSTART_ADDR, pstart,
					PWR_ON_LED3_PSTART_MASK,
					PWR_ON_LED3_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_PEND_ADDR, pend,
					PWR_ON_LED3_PEND_MASK,
					PWR_ON_LED3_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_DH_ADDR, DH,
					PWR_ON_LED3_DH_MASK,
					PWR_ON_LED3_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_DL_ADDR, DL,
					PWR_ON_LED3_DL_MASK,
					PWR_ON_LED3_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TH0_ADDR, TH0,
					PWR_ON_LED3_TH0_MASK,
					PWR_ON_LED3_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TL0_ADDR, TL0,
					PWR_ON_LED3_TL0_MASK,
					PWR_ON_LED3_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TR0_ADDR, TR0,
					PWR_ON_LED3_TR0_MASK,
					PWR_ON_LED3_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TF0_ADDR, TF0,
					PWR_ON_LED3_TF0_MASK,
					PWR_ON_LED3_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TH_ADDR, TH,
					PWR_ON_LED3_TH_MASK,
					PWR_ON_LED3_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TL_ADDR, TL,
					PWR_ON_LED3_TL_MASK,
					PWR_ON_LED3_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TR_ADDR, TR,
					PWR_ON_LED3_TR_MASK,
					PWR_ON_LED3_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_LED3_TF_ADDR, TF,
					PWR_ON_LED3_TF_MASK,
					PWR_ON_LED3_TF_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_PSTART_ADDR, pstart,
					PWR_OFF_LP_LED3_PSTART_MASK,
					PWR_OFF_LP_LED3_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_PEND_ADDR, pend,
					PWR_OFF_LP_LED3_PEND_MASK,
					PWR_OFF_LP_LED3_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_DH_ADDR, DH,
					PWR_OFF_LP_LED3_DH_MASK,
					PWR_OFF_LP_LED3_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_DL_ADDR, DL,
					PWR_OFF_LP_LED3_DL_MASK,
					PWR_OFF_LP_LED3_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TH0_ADDR, TH0,
					PWR_OFF_LP_LED3_TH0_MASK,
					PWR_OFF_LP_LED3_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TL0_ADDR, TL0,
					PWR_OFF_LP_LED3_TL0_MASK,
					PWR_OFF_LP_LED3_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TR0_ADDR, TR0,
					PWR_OFF_LP_LED3_TR0_MASK,
					PWR_OFF_LP_LED3_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TF0_ADDR, TF0,
					PWR_OFF_LP_LED3_TF0_MASK,
					PWR_OFF_LP_LED3_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TH_ADDR, TH,
					PWR_OFF_LP_LED3_TH_MASK,
					PWR_OFF_LP_LED3_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TL_ADDR, TL,
					PWR_OFF_LP_LED3_TL_MASK,
					PWR_OFF_LP_LED3_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TR_ADDR, TR,
					PWR_OFF_LP_LED3_TR_MASK,
					PWR_OFF_LP_LED3_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_LED3_TF_ADDR, TF,
					PWR_OFF_LP_LED3_TF_MASK,
					PWR_OFF_LP_LED3_TF_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_PSTART_ADDR, pstart,
					PWR_RB_LED3_PSTART_MASK,
					PWR_RB_LED3_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_PEND_ADDR, pend,
					PWR_RB_LED3_PEND_MASK,
					PWR_RB_LED3_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_DH_ADDR, DH,
					PWR_RB_LED3_DH_MASK,
					PWR_RB_LED3_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_DL_ADDR, DL,
					PWR_RB_LED3_DL_MASK,
					PWR_RB_LED3_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TH0_ADDR, TH0,
					PWR_RB_LED3_TH0_MASK,
					PWR_RB_LED3_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TL0_ADDR, TL0,
					PWR_RB_LED3_TL0_MASK,
					PWR_RB_LED3_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TR0_ADDR, TR0,
					PWR_RB_LED3_TR0_MASK,
					PWR_RB_LED3_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TF0_ADDR, TF0,
					  PWR_RB_LED3_TF0_MASK,
					  PWR_RB_LED3_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TH_ADDR, TH,
					PWR_RB_LED3_TH_MASK,
					PWR_RB_LED3_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TL_ADDR, TL,
					PWR_RB_LED3_TL_MASK,
					PWR_RB_LED3_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TR_ADDR, TR,
					PWR_RB_LED3_TR_MASK,
					PWR_RB_LED3_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_LED3_TF_ADDR, TF,
					PWR_RB_LED3_TF_MASK,
					PWR_RB_LED3_TF_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_PSTART_ADDR, pstart,
					PWR_FAIL_LED3_PSTART_MASK,
					PWR_FAIL_LED3_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_PEND_ADDR, pend,
					PWR_FAIL_LED3_PEND_MASK,
					PWR_FAIL_LED3_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_DH_ADDR, DH,
					PWR_FAIL_LED3_DH_MASK,
					PWR_FAIL_LED3_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_DL_ADDR, DL,
					PWR_FAIL_LED3_DL_MASK,
					PWR_FAIL_LED3_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TH0_ADDR, TH0,
					PWR_FAIL_LED3_TH0_MASK,
					PWR_FAIL_LED3_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TL0_ADDR, TL0,
					PWR_FAIL_LED3_TL0_MASK,
					PWR_FAIL_LED3_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TR0_ADDR, TR0,
					PWR_FAIL_LED3_TR0_MASK,
					PWR_FAIL_LED3_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TF0_ADDR, TF0,
					PWR_FAIL_LED3_TF0_MASK,
					PWR_FAIL_LED3_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TH_ADDR, TH,
					PWR_FAIL_LED3_TH_MASK,
					PWR_FAIL_LED3_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TL_ADDR, TL,
					PWR_FAIL_LED3_TL_MASK,
					PWR_FAIL_LED3_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TR_ADDR, TR,
					PWR_FAIL_LED3_TR_MASK,
					PWR_FAIL_LED3_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_LED3_TF_ADDR, TF,
					PWR_FAIL_LED3_TF_MASK,
					PWR_FAIL_LED3_TF_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_PSTART_ADDR, pstart,
					PWR_FW_LED3_PSTART_MASK,
					PWR_FW_LED3_PSTART_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_PEND_ADDR, pend,
					PWR_FW_LED3_PEND_MASK,
					PWR_FW_LED3_PEND_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TH0_ADDR, TH0,
					PWR_FW_LED3_TH0_MASK,
					PWR_FW_LED3_TH0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TL0_ADDR, TL0,
					PWR_FW_LED3_TL0_MASK,
					PWR_FW_LED3_TL0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TR0_ADDR, TR0,
					PWR_FW_LED3_TR0_MASK,
					PWR_FW_LED3_TR0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TF0_ADDR, TF0,
					PWR_FW_LED3_TF0_MASK,
					PWR_FW_LED3_TF0_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_DH_ADDR, DH,
					PWR_FW_LED3_DH_MASK,
					PWR_FW_LED3_DH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TH_ADDR, TH,
					PWR_FW_LED3_TH_MASK,
					PWR_FW_LED3_TH_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_DL_ADDR, DL,
					PWR_FW_LED3_DL_MASK,
					PWR_FW_LED3_DL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TL_ADDR, TL,
					PWR_FW_LED3_TL_MASK,
					PWR_FW_LED3_TL_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TR_ADDR, TR,
					PWR_FW_LED3_TR_MASK,
					PWR_FW_LED3_TR_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_LED3_TF_ADDR, TF,
					PWR_FW_LED3_TF_MASK,
					PWR_FW_LED3_TF_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	pr_debug("config end.\n");
	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_led3_flash_mode_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     setting corresponding PWMLEDx flash_mode config.
 * @param[in]
 *     start_pwm_no: Start pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     end_pwm_no: End pwm channel:PWMLED1,PWMLED2,PWMLED3.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG,PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @param[in]
 *     pstart: start duty point.
* @param[in]
 *     pend: end duty point.
 * @param[in]
 *     TH0: high duy height of start/end duty.
 * @param[in]
 *     TL0: low duty height of start/end duty.
 * @param[in]
 *     TR0: high-time length of start/end duty.
 * @param[in]
 *     TF0: low-time length of start/end duty.
 * @param[in]
 *     DH:  high duty height of repeat duty.
 * @param[in]
 *     TH: high duty time of repeat duty.
 * @param[in]
 *     DL: low duty height of repeat duty.
 * @param[in]
 *     TL: low duty time of repeat duty.
 * @param[in]
 *     TR: duty rising time of repeat duty.
 * @param[in]
 *     TF :duty falling time of repeat duty.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_ledx_flash_mode_config(uint32_t start_pwm_no,
	uint32_t end_pwm_no, uint32_t scenarios, uint32_t pstart,
	uint32_t pend, uint32_t TH0, uint32_t TL0, uint32_t TR0,
	uint32_t TF0, uint32_t DH, uint32_t TH, uint32_t DL,
	uint32_t TL, uint32_t TR, uint32_t TF)
{
	int32_t ret = 0;
	uint32_t index = 0x0;

	TH0 = TH0 / 5;
	TL0 = TL0 / 5;
	TR0 = TR0 / 5;
	TF0 = TF0 / 5;
	DH = DH * 10;
	TH = TH / 5;
	DL = DL * 10;
	TL = TL / 5;
	TR = TR / 5;
	TF = TF / 5;

	for (index = start_pwm_no; index <= end_pwm_no; index++) {
		switch (index) {
		case PWMALL:
			pr_debug("mt3615_pwm_ledx_flash_mode_config ,ERROR!!!\n");
			break;
		case PWMLED1:
			ret = mt3615_pwm_led1_flash_mode_config(scenarios,
				pstart, pend, TH0, TL0, TR0, TF0,
				DH, TH, DL, TL, TR, TF);
			pr_debug("pwm_led1_flash_mode set\n");
			break;
		case PWMLED2:
			ret = mt3615_pwm_led2_flash_mode_config(scenarios,
				pstart, pend, TH0, TL0, TR0, TF0,
				DH, TH, DL, TL, TR, TF);
			pr_debug("pwm_led2_flash_mode set\n");
			break;
		case PWMLED3:
			ret = mt3615_pwm_led3_flash_mode_config(scenarios,
				pstart, pend, TH0, TL0, TR0, TF0,
				DH, TH, DL, TL, TR, TF);
			pr_debug("pwm_led3_flash_mode set\n");
			break;
		}
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_ledx_flash_mode_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     Cofig corresponding PWM1 apply_en.
 * @par Parameters
 *     none.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx1_apply_en(void)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	reg_val = 0x1;

	ret = pwrap_pwm_write_reg(PWM1_APPLY_EN_ADDR, reg_val,
				PWM1_APPLY_EN_MASK,
				PWM1_APPLY_EN_SHIFT);
	pr_debug("mt3615_pwm_Idx1_apply_en , reg_val = %d\n", reg_val);
	mutex_unlock(&mt3615_pwm_mutex);

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx1_apply_en);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     corresponding PWM1 enable.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx1_enable(uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	if (scenarios == PWM_RG)
	mutex_lock(&mt3615_pwm_mutex);
	pr_debug("mt3615_pwm_Idx1_enable , reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret = pwrap_pwm_write_reg(RG_PWM1_EN_ADDR, reg_val,
					RG_PWM1_EN_MASK,
					RG_PWM1_EN_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_BY_PWM1_EN_ADDR, reg_val,
					PWR_BY_PWM1_EN_MASK,
					PWR_BY_PWM1_EN_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_PWM1_EN_ADDR, reg_val,
					PWR_ON_PWM1_EN_MASK,
					PWR_ON_PWM1_EN_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_PWM1_EN_ADDR, reg_val,
					PWR_OFF_LP_PWM1_EN_MASK,
					PWR_OFF_LP_PWM1_EN_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_PWM1_EN_ADDR, reg_val,
					PWR_RB_PWM1_EN_MASK,
					PWR_RB_PWM1_EN_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_PWM1_EN_ADDR, reg_val,
					PWR_FAIL_PWM1_EN_MASK,
					PWR_FAIL_PWM1_EN_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_PWM1_EN_ADDR, reg_val,
					PWR_FW_PWM1_EN_MASK,
					PWR_FW_PWM1_EN_SHIFT);
		break;
	}
	if (scenarios == PWM_RG)
	mutex_unlock(&mt3615_pwm_mutex);

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx1_enable);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     Config corresponding PWM1 disable.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx1_disable(uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x0;

	if (scenarios == PWM_RG)
	mutex_lock(&mt3615_pwm_mutex);
	pr_debug("mt3615_pwm_Idx1_disable , reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret = pwrap_pwm_write_reg(RG_PWM1_EN_ADDR, reg_val,
					RG_PWM1_EN_MASK,
					RG_PWM1_EN_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret = pwrap_pwm_write_reg(PWR_BY_PWM1_EN_ADDR, reg_val,
					PWR_BY_PWM1_EN_MASK,
					PWR_BY_PWM1_EN_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_PWM1_EN_ADDR, reg_val,
					PWR_ON_PWM1_EN_MASK,
					PWR_ON_PWM1_EN_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_PWM1_EN_ADDR, reg_val,
					PWR_OFF_LP_PWM1_EN_MASK,
					PWR_OFF_LP_PWM1_EN_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_PWM1_EN_ADDR, reg_val,
					PWR_RB_PWM1_EN_MASK,
					PWR_RB_PWM1_EN_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret = pwrap_pwm_write_reg(PWR_FAIL_PWM1_EN_ADDR, reg_val,
					PWR_FAIL_PWM1_EN_MASK,
					PWR_FAIL_PWM1_EN_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_PWM1_EN_ADDR, reg_val,
					PWR_FW_PWM1_EN_MASK,
					PWR_FW_PWM1_EN_SHIFT);
		break;
	}
	if (scenarios == PWM_RG)
	mutex_unlock(&mt3615_pwm_mutex);

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx1_disable);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     configure corresponding PWM1 config(ex:DUTY,FREQ).
 * @param[in]
 *     pwm_freq_index: freq select setting.
 * @param[in]
 *     duty_step: duty setp setting.
 * @param[in]
 *     scenarios: pwm mode: PWM_RG, PWR_STANDBY,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL,
 *     PWR_FAIL_WDT.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx1_config(uint32_t pwm_freq_index,
			uint32_t duty_step, uint32_t scenarios)
{
	int32_t ret = 0;

	if (scenarios == PWM_RG)
	mutex_lock(&mt3615_pwm_mutex);

	pr_debug("mt3615_pwm_Idx1_config ");
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret |= pwrap_pwm_write_reg(RG_PWM1_FREQ_ADDR,
			pwm_freq_index, RG_PWM1_FREQ_MASK,
			RG_PWM1_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_PWM1_DUTY_ADDR,
			duty_step, RG_PWM1_DUTY_MASK,
			RG_PWM1_DUTY_SHIFT);
		break;
	case PWR_STANDBY:
		pr_debug("PWR_STANDBY\n");
		ret |= pwrap_pwm_write_reg(PWR_BY_PWM1_FREQ_ADDR,
			pwm_freq_index, PWR_BY_PWM1_FREQ_MASK,
			PWR_BY_PWM1_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_BY_PWM1_DUTY_ADDR,
			duty_step, PWR_BY_PWM1_DUTY_MASK,
			PWR_BY_PWM1_DUTY_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret |= pwrap_pwm_write_reg(PWR_ON_PWM1_FREQ_ADDR,
			pwm_freq_index, PWR_ON_PWM1_FREQ_MASK,
			PWR_ON_PWM1_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_PWM1_DUTY_ADDR,
			duty_step, PWR_ON_PWM1_DUTY_MASK,
			PWR_ON_PWM1_DUTY_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_PWM1_FREQ_ADDR,
			pwm_freq_index, PWR_OFF_LP_PWM1_FREQ_MASK,
			PWR_OFF_LP_PWM1_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_PWM1_DUTY_ADDR,
			duty_step, PWR_OFF_LP_PWM1_DUTY_MASK,
			PWR_OFF_LP_PWM1_DUTY_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret |= pwrap_pwm_write_reg(PWR_RB_PWM1_FREQ_ADDR,
			pwm_freq_index, PWR_RB_PWM1_FREQ_MASK,
			PWR_RB_PWM1_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_PWM1_DUTY_ADDR,
			duty_step, PWR_RB_PWM1_DUTY_MASK,
			PWR_RB_PWM1_DUTY_SHIFT);
		break;
	case PWR_FAIL:
		pr_debug("PWR_FAIL\n");
		ret |= pwrap_pwm_write_reg(PWR_FAIL_PWM1_FREQ_ADDR,
			pwm_freq_index, PWR_FAIL_PWM1_FREQ_MASK,
			PWR_FAIL_PWM1_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FAIL_PWM1_DUTY_ADDR,
			duty_step, PWR_FAIL_PWM1_DUTY_MASK,
			PWR_FAIL_PWM1_DUTY_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret |= pwrap_pwm_write_reg(PWR_FW_PWM1_FREQ_ADDR,
			pwm_freq_index, PWR_FW_PWM1_FREQ_MASK,
			PWR_FW_PWM1_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_PWM1_DUTY_ADDR,
			duty_step, PWR_FW_PWM1_DUTY_MASK,
			PWR_FW_PWM1_DUTY_SHIFT);
		break;
	}
	if (scenarios == PWM_RG)
	mutex_unlock(&mt3615_pwm_mutex);

	pr_debug("pwm_freq_index = %d , duty_cycle = %d, scenarios = %d\n",
		pwm_freq_index, duty_step, scenarios);

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx1_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     Cofig corresponding PWM2 apply_en.
 * @par Parameters
 *     none.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx2_apply_en(uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret = pwrap_pwm_read_reg(RG_PWM2_EN_ADDR, &reg_val,
					RG_PWM2_EN_MASK,
					RG_PWM2_EN_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_read_reg(PWR_ON_PWM2_EN_ADDR, &reg_val,
					RG_PWM2_EN_MASK,
					RG_PWM2_EN_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_read_reg(PWR_OFF_LP_PWM2_EN_ADDR, &reg_val,
					PWR_OFF_LP_PWM2_EN_MASK,
					PWR_OFF_LP_PWM2_EN_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_read_reg(PWR_RB_PWM2_EN_ADDR, &reg_val,
					PWR_RB_PWM2_EN_MASK,
					PWR_RB_PWM2_EN_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_read_reg(PWR_FW_PWM2_EN_ADDR, &reg_val,
					PWR_FW_PWM2_EN_MASK,
					PWR_FW_PWM2_EN_SHIFT);
		break;
	case PWR_ON2:
		pr_debug("PWR_ON2\n");
		ret = pwrap_pwm_read_reg(PWR_ON2_PWM2_EN_ADDR, &reg_val,
					PWR_ON2_PWM2_EN_MASK,
					PWR_ON2_PWM2_EN_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_info("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	if (reg_val == 0) {

		reg_val = 0x1;

		pr_debug(" set PWM2_EN to 1 , reg_val = %d\n",
			reg_val);
		switch (scenarios) {
		case PWM_RG:
			pr_debug("Register mode\n");
			ret = pwrap_pwm_write_reg(RG_PWM2_EN_ADDR, reg_val,
						RG_PWM2_EN_MASK,
						RG_PWM2_EN_SHIFT);
			break;
		case PWR_ON:
			pr_debug("PWR_ON\n");
			ret = pwrap_pwm_write_reg(PWR_ON_PWM2_EN_ADDR, reg_val,
						RG_PWM2_EN_MASK,
						RG_PWM2_EN_SHIFT);
			break;
		case PWR_OFF_LONGPRESS:
			pr_debug("PWR_OFF_LONGPRESS\n");
			ret = pwrap_pwm_write_reg(PWR_OFF_LP_PWM2_EN_ADDR,
						reg_val,
						PWR_OFF_LP_PWM2_EN_MASK,
						PWR_OFF_LP_PWM2_EN_SHIFT);
			break;
		case PWR_REBOOT:
			pr_debug("PWR_REBOOT\n");
			ret = pwrap_pwm_write_reg(PWR_RB_PWM2_EN_ADDR, reg_val,
						PWR_RB_PWM2_EN_MASK,
						PWR_RB_PWM2_EN_SHIFT);
			break;
		case PWR_FAIL_WDT:
			pr_debug("PWR_FAIL_WDT\n");
			ret = pwrap_pwm_write_reg(PWR_FW_PWM2_EN_ADDR, reg_val,
						PWR_FW_PWM2_EN_MASK,
						PWR_FW_PWM2_EN_SHIFT);
			break;
		case PWR_ON2:
			pr_debug("PWR_ON2\n");
			ret = pwrap_pwm_write_reg(PWR_ON2_PWM2_EN_ADDR, reg_val,
						PWR_ON2_PWM2_EN_MASK,
						PWR_ON2_PWM2_EN_SHIFT);
			break;
		}

		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}
	}
	reg_val = 0x1;

	pr_debug("Register mode enable\n");
	ret = pwrap_pwm_write_reg(RG_PWM2_EN_ADDR, reg_val,
				RG_PWM2_EN_MASK,
				RG_PWM2_EN_SHIFT);

	ret = pwrap_pwm_write_reg(PWM2_APPLY_EN_ADDR, reg_val,
				PWM2_APPLY_EN_MASK,
				PWM2_APPLY_EN_SHIFT);
	pr_debug("mt3615_pwm_Idx2_apply_en , reg_val = %d\n", reg_val);

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx2_apply_en);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     corresponding PWM2 enable.
 * @param[in]
 *     scenarios: pwm mode:PWM_RG,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL_WDT,  PWR_ON2.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx2_enable(uint32_t scenarios)
{
	int32_t ret = 0;
	uint32_t reg_val = 0x1;

	pr_debug("mt3615_pwm_Idx2_enable reg_val = %d\n", reg_val);
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n");
		ret = pwrap_pwm_write_reg(RG_PWM2_EN_ADDR, reg_val,
					RG_PWM2_EN_MASK,
					RG_PWM2_EN_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret = pwrap_pwm_write_reg(PWR_ON_PWM2_EN_ADDR, reg_val,
					PWR_ON_PWM2_EN_MASK,
					PWR_ON_PWM2_EN_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret = pwrap_pwm_write_reg(PWR_OFF_LP_PWM2_EN_ADDR, reg_val,
					PWR_OFF_LP_PWM2_EN_MASK,
					PWR_OFF_LP_PWM2_EN_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret = pwrap_pwm_write_reg(PWR_RB_PWM2_EN_ADDR, reg_val,
					PWR_RB_PWM2_EN_MASK,
					PWR_RB_PWM2_EN_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret = pwrap_pwm_write_reg(PWR_FW_PWM2_EN_ADDR, reg_val,
					PWR_FW_PWM2_EN_MASK,
					PWR_FW_PWM2_EN_SHIFT);
		break;
	case PWR_ON2:
		pr_debug("PWR_ON2\n");
		ret = pwrap_pwm_write_reg(PWR_ON2_PWM2_EN_ADDR, reg_val,
					PWR_ON2_PWM2_EN_MASK,
					PWR_ON2_PWM2_EN_SHIFT);
		break;
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx2_enable);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     configure corresponding PWM2 config,
 *     (ex:DUTY,FREQ,TREPEAT,TON,TOFF).
 * @param[in]
 *     pwm_freq_index: select work freq.
 * @param[in]
 *     scenarios: pwm mode:PWM_RG,  PWR_ON,
 *     PWR_OFF_LONGPRESS,  PWR_REBOOT,  PWR_FAIL_WDT,  PWR_ON2.
 * @param[in]
 *     off_time:TOFF time.
 * @param[in]
 *     on_time:TON time.
 * @param[in]
 *     TREPEAT:pwm2 repeat time.
 * @return
 *     0 ,success,
 *     not equal 0 ,fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t mt3615_pwm_Idx2_config(uint32_t pwm_freq_index, uint32_t scenarios,
		uint32_t off_time, uint32_t on_time, uint32_t repeat_time)
{
	int32_t ret = 0;

	pr_debug("mt3615_pwm_Idx2_config ");
	switch (scenarios) {
	case PWM_RG:
		pr_debug("Register mode\n ");
		ret |= pwrap_pwm_write_reg(RG_PWM2_FREQ_ADDR, pwm_freq_index,
					RG_PWM2_FREQ_MASK,
					RG_PWM2_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_PWM2_TON_ADDR, on_time,
					RG_PWM2_TON_MASK,
					RG_PWM2_TON_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_PWM2_TOFF_ADDR, off_time,
					RG_PWM2_TOFF_MASK,
					RG_PWM2_TOFF_SHIFT);
		ret |= pwrap_pwm_write_reg(RG_PWM2_TREPEAT_ADDR, repeat_time,
					RG_PWM2_TREPEAT_MASK,
					RG_PWM2_TREPEAT_SHIFT);
		break;
	case PWR_ON:
		pr_debug("PWR_ON\n");
		ret |= pwrap_pwm_write_reg(PWR_ON_PWM2_FREQ_ADDR,
			pwm_freq_index, PWR_ON_PWM2_FREQ_MASK,
			PWR_ON_PWM2_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_PWM2_TON_ADDR, on_time,
					PWR_ON_PWM2_TON_MASK,
					PWR_ON_PWM2_TON_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_PWM2_TOFF_ADDR, off_time,
					PWR_ON_PWM2_TOFF_MASK,
					PWR_ON_PWM2_TOFF_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON_PWM2_TREPEAT_ADDR,
			repeat_time, PWR_ON_PWM2_TREPEAT_MASK,
			PWR_ON_PWM2_TREPEAT_SHIFT);
		break;
	case PWR_OFF_LONGPRESS:
		pr_debug("PWR_OFF_LONGPRESS\n");
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_PWM2_FREQ_ADDR,
			pwm_freq_index, PWR_OFF_LP_PWM2_FREQ_MASK,
			PWR_OFF_LP_PWM2_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_PWM2_TON_ADDR, on_time,
					PWR_OFF_LP_PWM2_TON_MASK,
					PWR_OFF_LP_PWM2_TON_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_PWM2_TOFF_ADDR, off_time,
					PWR_OFF_LP_PWM2_TOFF_MASK,
					PWR_OFF_LP_PWM2_TOFF_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_OFF_LP_PWM2_TREPEAT_ADDR,
					repeat_time,
					PWR_OFF_LP_PWM2_TREPEAT_MASK,
					PWR_OFF_LP_PWM2_TREPEAT_SHIFT);
		break;
	case PWR_REBOOT:
		pr_debug("PWR_REBOOT\n");
		ret |= pwrap_pwm_write_reg(PWR_RB_PWM2_FREQ_ADDR,
			pwm_freq_index, PWR_RB_PWM2_FREQ_MASK,
			PWR_RB_PWM2_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_PWM2_TON_ADDR, on_time,
					PWR_RB_PWM2_TON_MASK,
					PWR_RB_PWM2_TON_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_PWM2_TOFF_ADDR, off_time,
					PWR_RB_PWM2_TOFF_MASK,
					PWR_RB_PWM2_TOFF_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_RB_PWM2_TREPEAT_ADDR,
			repeat_time, PWR_RB_PWM2_TREPEAT_MASK,
			PWR_RB_PWM2_TREPEAT_SHIFT);
		break;
	case PWR_FAIL_WDT:
		pr_debug("PWR_FAIL_WDT\n");
		ret |= pwrap_pwm_write_reg(PWR_FW_PWM2_FREQ_ADDR,
			pwm_freq_index, PWR_FW_PWM2_FREQ_MASK,
			PWR_FW_PWM2_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_PWM2_TON_ADDR, on_time,
					PWR_FW_PWM2_TON_MASK,
					PWR_FW_PWM2_TON_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_PWM2_TOFF_ADDR, off_time,
					PWR_FW_PWM2_TOFF_MASK,
					PWR_FW_PWM2_TOFF_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_FW_PWM2_TREPEAT_ADDR,
			repeat_time, PWR_FW_PWM2_TREPEAT_MASK,
			PWR_FW_PWM2_TREPEAT_SHIFT);
		break;
	case PWR_ON2:
		pr_debug("PWR_ON2\n");
		ret |= pwrap_pwm_write_reg(PWR_ON2_PWM2_FREQ_ADDR,
			pwm_freq_index, PWR_ON2_PWM2_FREQ_MASK,
			PWR_ON2_PWM2_FREQ_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON2_PWM2_TON_ADDR, on_time,
					 PWR_ON2_PWM2_TON_MASK,
					PWR_ON2_PWM2_TON_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON2_PWM2_TOFF_ADDR, off_time,
					PWR_ON2_PWM2_TOFF_MASK,
					PWR_ON2_PWM2_TOFF_SHIFT);
		ret |= pwrap_pwm_write_reg(PWR_ON2_PWM2_TREPEAT_ADDR,
			repeat_time, PWR_ON2_PWM2_TREPEAT_MASK,
			PWR_ON2_PWM2_TREPEAT_SHIFT);
		break;
	}
	pr_debug("pwm_freq_index = %d , scenarios = %d\n", pwm_freq_index,
		scenarios);

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mt3615_pwm_Idx2_config);

/** @ingroup IP_group_pwm_external_function
 * @par Description
 *     pmic pwm init.
 * @param[in]
 *     map: pwrap map base addr.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt3615_pwm_init(struct regmap *map)
{
	mt3615_pwrap_regmap = map;

	pr_debug("%s\n", __func__);
	mutex_init(&mt3615_pwm_mutex);

	pr_debug("****[%s] DONE\n", __func__);

}
EXPORT_SYMBOL(mt3615_pwm_init);

