/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: zm.Chen, MediaTek
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

#ifndef __MFD_MT3615_API_H__
#define __MFD_MT3615_API_H__

int pmic_config_reg(unsigned int reg_num, unsigned int val,
				   unsigned int mask, unsigned int shift);

int pmic_read_reg(unsigned int reg_num, unsigned int *val,
				 unsigned int mask, unsigned int shift);

void mt3615_pmic_reboot(void);
void mt3615_pmic_poweroff(void);
unsigned int mt3615_get_soc_ready(void);
int mt3615_set_soc_ready(unsigned int en);
int mt3615_enable_shutdown_pad(enum SHUTDOWN_IDX idx
						, unsigned int en);
int mt3615_enable_timerout(unsigned int idx, unsigned int en,
					unsigned int set);
extern int call_pmic_ut(struct mt3615_chip *ut_dev, char *buf);
#endif /* __MFD_MT3615_API_H__ */
