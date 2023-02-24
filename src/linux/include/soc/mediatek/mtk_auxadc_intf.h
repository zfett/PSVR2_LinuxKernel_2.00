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

#ifndef __LINUX_MTK_AUXADC_INTF_H
#define __LINUX_MTK_AUXADC_INTF_H

#define VOLTAGE_FULL_RANGE	(1800)

/* =========== User Layer ================== */
/* pmic_get_auxadc_value
 * if return value < 0 -> means get data fail
 */
int pmic_get_auxadc_value(u8 list);
void pmic_auxadc_dump_regs(char *buf);
const char *pmic_get_auxadc_name(u8 list);

enum {
	/* mt6355 */
	AUXADC_LIST_BATADC,
	AUXADC_LIST_MT6355_START = AUXADC_LIST_BATADC,
	AUXADC_LIST_VCDT,
	AUXADC_LIST_BATTEMP,
	AUXADC_LIST_BATID,
	AUXADC_LIST_VBIF,
	AUXADC_LIST_MT6355_CHIP_TEMP,
	AUXADC_LIST_DCXO,
	AUXADC_LIST_ACCDET,
	AUXADC_LIST_TSX,
	AUXADC_LIST_HPOFS_CAL,
	AUXADC_LIST_MT6355_END = AUXADC_LIST_HPOFS_CAL,
	AUXADC_LIST_MAX,
};

extern void pmic_auxadc_init(struct regmap *map);
/* ============ kernel Layer =================== */

#endif /* __LINUX_MTK_AUXADC_INTF_H */
