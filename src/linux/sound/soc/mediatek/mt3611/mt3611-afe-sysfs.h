/**
 * @file mt3611-afe-sysfs.h
 * Mediatek audio debug function
 *
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MT_AFE_SYSFS_H__
#define __MT_AFE_SYSFS_H__

struct mtk_afe;
#ifdef DPTOI2S_SYSFS
int mt3611_afe_init_sysfs(struct mtk_afe *afe);
int mt3611_afe_exit_sysfs(struct mtk_afe *afe);
extern void i2s_setting(int mode, unsigned int sysclk);
#endif

#endif
