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

#ifndef MTK_EMI_CTRL_H_
#define MTK_EMI_CTRL_H_

#include <linux/types.h>
#include <linux/device.h>

enum {
	EMI_PORT_M0 = 0,
	EMI_PORT_M1,
	EMI_PORT_M2,
	EMI_PORT_M3,
	EMI_PORT_M4,
	EMI_PORT_M5,
	EMI_PORT_M6,
	EMI_PORT_M7,
	_EMI_PORT_MAX
};

enum {
	EMI_CONF_P1 = 1,
	EMI_CONF_P2,
	EMI_CONF_P3,
	EMI_CONF_P4,
	EMI_CONF_P5,
	EMI_CONF_P6,
	EMI_CONF_P7,
	EMI_CONF_P8,
	_EMI_CONF_MAX
};

int mtk_emi_ctrl_set_bandwidth(struct device *dev, const u8 port,
						const u8 value);
int mtk_emi_ctrl_change_config(struct device *dev, const u8 p_num);

#endif /* MTK_EMI_CTRL_H_ */
