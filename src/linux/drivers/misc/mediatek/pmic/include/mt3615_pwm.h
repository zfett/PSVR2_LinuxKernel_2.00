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
#ifndef __PWM_MT3615_API_H__
#define __PWM_MT3615_API_H__
/**
 * @file mt3615_pwm.h
 * Heander of pmic_pwm.c
 */
#include <linux/types.h>
#include <linux/regmap.h>
#include "../include/mt3615_pwm_reg.h"

extern struct regmap *mt3615_pwrap_regmap;
int32_t pwrap_pwm_write_reg(uint32_t reg_num, uint32_t val,
		uint32_t mask, uint32_t shift);
int32_t pwrap_pwm_read_reg(uint32_t reg_num, uint32_t *val,
		uint32_t mask, uint32_t shift);
void mt3615_pwm_init(struct regmap *map);
int32_t mt3615_pwm_get_soc_ready(void);
int32_t mt3615_pwm_cfg_soc_ready(uint32_t u4flag);
int32_t mt3615_pwm_ledx_cfg_autoload(uint32_t start_pwm_no,
		uint32_t end_pwm_no, uint32_t u4flag, uint32_t u4OPScenarios);
int32_t mt3615_pwm_Idx1_cfg_autoload(uint32_t u4flag, uint32_t u4OPScenarios);
int32_t mt3615_pwm_Idx2_cfg_autoload(uint32_t u4flag, uint32_t u4OPScenarios);
int32_t mt3615_pwm_ledx_apply_en(uint32_t start_pwm_no, uint32_t end_pwm_no);
int32_t mt3615_pwm_led1_mode_sel(uint32_t u4mode, uint32_t u4OPScenarios);
int32_t mt3615_pwm_led2_mode_sel(uint32_t u4mode, uint32_t u4OPScenarios);
int32_t mt3615_pwm_led3_mode_sel(uint32_t u4mode, uint32_t u4OPScenarios);
int32_t mt3615_pwm_ledx_mode_sel(uint32_t start_pwm_no,
		uint32_t end_pwm_no, uint32_t u4mode, uint32_t u4OPScenarios);
int32_t mt3615_pwm_led1_fixed_mode_config(uint32_t u4DutyCycle,
		uint32_t u4OPScenarios);
int32_t mt3615_pwm_led2_fixed_mode_config(uint32_t u4DutyCycle,
		uint32_t u4OPScenarios);
int32_t mt3615_pwm_led3_fixed_mode_config(uint32_t u4DutyCycle,
		uint32_t u4OPScenarios);
int32_t mt3615_pwm_ledx_fixed_mode_config(uint32_t start_pwm_no,
	uint32_t end_pwm_no, uint32_t u4DutyCycle, uint32_t u4OPScenarios);
int32_t mt3615_pwm_led1_flash_mode_config(uint32_t u4OPScenarios,
		uint32_t u4PSTAx, uint32_t u4PENDx, uint32_t u4TH0,
		uint32_t u4TL0, uint32_t u4TR0, uint32_t u4TF0, uint32_t u4DH,
		uint32_t u4TH, uint32_t u4DL, uint32_t u4TL, uint32_t u4TR,
		uint32_t u4TF);
int32_t mt3615_pwm_led2_flash_mode_config(uint32_t u4OPScenarios,
		uint32_t u4PSTAx, uint32_t u4PENDx, uint32_t u4TH0,
		uint32_t u4TL0, uint32_t u4TR0, uint32_t u4TF0, uint32_t u4DH,
		uint32_t u4TH, uint32_t u4DL, uint32_t u4TL, uint32_t u4TR,
		uint32_t u4TF);
int32_t mt3615_pwm_led3_flash_mode_config(uint32_t u4OPScenarios,
		uint32_t u4PSTAx, uint32_t u4PENDx, uint32_t u4TH0,
		uint32_t u4TL0, uint32_t u4TR0, uint32_t u4TF0, uint32_t u4DH,
		uint32_t u4TH, uint32_t u4DL, uint32_t u4TL, uint32_t u4TR,
		uint32_t u4TF);
int32_t mt3615_pwm_ledx_flash_mode_config(uint32_t start_pwm_no,
		uint32_t end_pwm_no, uint32_t u4OPScenarios, uint32_t u4PSTAx,
		uint32_t u4PENDx, uint32_t u4TH0, uint32_t u4TL0,
		uint32_t u4TR0, uint32_t u4TF0, uint32_t u4DH, uint32_t u4TH,
		uint32_t u4DL, uint32_t u4TL, uint32_t u4TR, uint32_t u4TF);
int32_t mt3615_pwm_Idx1_apply_en(void);
int32_t mt3615_pwm_Idx1_enable(uint32_t u4OPScenarios);
int32_t mt3615_pwm_Idx1_disable(uint32_t u4OPScenarios);
int32_t mt3615_pwm_Idx1_config(uint32_t u4PwmFreqIdx,
		uint32_t u4DutyStep, uint32_t u4OPScenarios);
int32_t mt3615_pwm_Idx2_apply_en(uint32_t u4OPScenarios);
int32_t mt3615_pwm_Idx2_enable(uint32_t u4OPScenarios);
int32_t mt3615_pwm_Idx2_config(uint32_t u4PwmFreqIdx,
		uint32_t u4OPScenarios, uint32_t u4TOFF, uint32_t u4TON,
		uint32_t u4TREPEAT);

#endif
