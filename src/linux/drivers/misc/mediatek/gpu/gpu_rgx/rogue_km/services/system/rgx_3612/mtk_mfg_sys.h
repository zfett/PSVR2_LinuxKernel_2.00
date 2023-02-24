/*
 * Copyright (c) 2019 MediaTek Inc.
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

#ifndef MTK_MFG_SYS_H
#define MTK_MFG_SYS_H

#include "pvrsrv_device.h"

int __init mtk_mfg_pm0_init(void);
int __init mtk_mfg_pm1_init(void);
int __init mtk_mfg_pm2_init(void);
int __init mtk_mfg_pm3_init(void);

void mtk_mfg_pm0_deinit(void);
void mtk_mfg_pm1_deinit(void);
void mtk_mfg_pm2_deinit(void);
void mtk_mfg_pm3_deinit(void);

PVRSRV_ERROR mtk_mfg_device_init(void *pvOSDevice);
void mtk_mfg_device_deinit(void *pvOSDevice);
PVRSRV_ERROR mtk_mfg_prepower_state(IMG_HANDLE hSysData,
			      PVRSRV_DEV_POWER_STATE eNewPowerState,
			      PVRSRV_DEV_POWER_STATE eCurrentPowerState,
			      IMG_BOOL bForced);

PVRSRV_ERROR mtk_mfg_postpower_state(IMG_HANDLE hSysData,
			      PVRSRV_DEV_POWER_STATE eNewPowerState,
			      PVRSRV_DEV_POWER_STATE eCurrentPowerState,
			      IMG_BOOL bForced);
void mtk_mfg_set_frequency(IMG_UINT32 ui32Frequency);
void mtk_mfg_set_voltage(IMG_UINT32 ui32Voltage);
void mtk_mfg_avs_config(struct device *dev);
void mtk_mfg_show_opp_tbl(struct seq_file *sf);
void mtk_mfg_show_cur_opp(struct seq_file *sf);
extern int mtk_mfg_get_loading(void);
extern unsigned long mtk_mfg_get_freq(void);
#endif /* MTK_MFG_SYS_H */
