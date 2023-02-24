/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef _POWER_CONTROLLER_H_
#define _POWER_CONTROLLER_H_

#include <linux/io.h>
#include <asm/bug.h>
#include <linux/device.h>
#include <linux/list.h>

#include "mtk_mm_sysram_reg.h"

/**/
enum {
	PWR_CTRL_TYPE_NORMAL,
	PWR_CTRL_TYPE_3612_SRAM2,
	_PWR_CTRL_TYPE_NUM
};

/* Data Types */
struct addr_node {
	int begin;
	int end;
	struct list_head list;
};

struct power_controller {
	int type;
	struct device *dev;
	int size;
	struct list_head addr_list;
	void *base;
};

/* Function Prototypes */
int pwr_ctrl_init(struct power_controller *ctrl, struct device *dev, int size,
							void *base, int type);
int pwr_ctrl_exit(struct power_controller *ctrl);
int pwr_ctrl_require(struct power_controller *ctrl, int addr, int size);
int pwr_ctrl_release(struct power_controller *ctrl, int addr, int size);

#endif /* #ifndef _POWER_CONTROLLER_H_ */
