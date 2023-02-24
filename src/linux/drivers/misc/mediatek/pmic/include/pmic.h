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

#ifndef _PMIC_SW_H_
#define _PMIC_SW_H_


#include <linux/regulator/consumer.h>

/*
 * Debugfs
 */
#define PMICTAG                "[PMIC] "
#define PMIC_DEBUG_LV		0

#define PMIC_LOG_DBG     4
#define PMIC_LOG_INFO    3
#define PMIC_LOG_NOT     2
#define PMIC_LOG_WARN    1
#define PMIC_LOG_ERR     0

#define PMICLOG(fmt, arg...) do { \
	if (PMIC_DEBUG_LV >= PMIC_LOG_DBG) \
		pr_notice(PMICTAG "%s: " fmt, __func__, ##arg); \
} while (0)

#define PMIC_ENTRY(reg) {reg, reg##_ADDR, reg##_MASK, reg##_SHIFT}

#endif				/* _PMIC_SW_H_ */
