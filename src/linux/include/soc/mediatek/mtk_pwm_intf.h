/*
 * Copyright (C) 2017 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __LINUX_MTK_PWM_INTF_H
#define __LINUX_MTK_PWM_INTF_H

/**
 * @file mtk_pwm_intf.h
 * Heander of upmu_pwm.c
 */

#include <linux/types.h>

#define VOLTAGE_FULL_RANGE	(1800)

/* =========== User Layer ================== */
/* pmic_get_pwm_value
 * if return value < 0 -> means get data fail
 */
int pmic_get_pwm_value(u8 list);
void pmic_pwm_dump_regs(char *buf);
const char *pmic_get_pwm_name(u8 list);
#define RET_SUCCESS 0
#define RET_FAIL 1
#define MAX_PARA_NUM 20
enum {
	LEDX_CFG_AUTOLOAD,
	LEDX_MODE_SEL,
	LEDX_APPLY_EN,
	LEDX_FIXED_MODE_CONFIG,
	LEDX_FLASH_MODE_CONFIG,
	IDX1_APPLY_EN,
	IDX1_ENABLE,
	IDX1_DISABLE,
	IDX1_CONFIG,
	IDX2_APPLY_EN,
	IDX2_ENABLE,
	IDX2_DISABLE,
	IDX2_CONFIG,
};

struct mtk_pwm_intf {
	struct mtk_pwm_ops *ops;
	char *name;
	int channel_num;
	const char **channel_name;
	int dbg_chl;
	int dbg_md_chl;
	int reg;
	int data;
};

struct cmd_tbl_t {
	char name[256];
	uint32_t (*cb_func)(struct mtk_pwm_intf *pwm_intf_dev,
		uint32_t argc, char **argv);
};
extern void pmic_pwm_init(struct regmap *map);
/* ============ kernel Layer =================== */
#endif /* __LINUX_MTK_PWM_INTF_H */
