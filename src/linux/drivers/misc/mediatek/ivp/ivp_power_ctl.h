/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Mao Lin <Zih-Ling.Lin@mediatek.com>
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
#ifndef __IVP_POWER_CTL_H__
#define __IVP_POWER_CTL_H__

#include "ivp_driver.h"

int ivp_setup_clk_resource(struct device *dev, struct ivp_device *ivp_device);
int ivp_device_power_on(struct ivp_device *ivp_device);
void ivp_device_power_off(struct ivp_device *ivp_device);

int ivp_core_setup_clk_resource(struct device *dev, struct ivp_core *ivp_core);
int ivp_core_power_on(struct ivp_core *ivp_core);
void ivp_core_power_off(struct ivp_core *ivp_core);
void ivp_core_reset(struct ivp_core *ivp_core);

int ivp_device_set_init_frequency(struct ivp_device *ivp_device);
int ivp_device_set_init_voltage(struct ivp_device *ivp_device);

unsigned long ivp_get_sram_voltage(unsigned long volt);
int ivp_adjust_voltage(struct ivp_device *ivp_device);

#endif /* __IVP_POWER_CTL_H__ */

