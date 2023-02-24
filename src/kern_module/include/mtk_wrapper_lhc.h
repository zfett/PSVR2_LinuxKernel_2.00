/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _MTK_WRAPPER_LHC_
#define _MTK_WRAPPER_LHC_

#include <linux/ioctl.h>
#include <linux/types.h>

#define LHC_DEVICE_IOC_TYPE 'L'

#define LHC_NUM (4)

typedef struct _mtk_wrapper_lhc_slice {
	__u32 w;
	__u32 h;
	__u32 start;
	__u32 end;
} mtk_wrapper_lhc_slice;

typedef struct _args_lhc_config_slice {
	mtk_wrapper_lhc_slice slice[LHC_NUM];
} args_lhc_config_slice;

#define LHC_BLK_X_NUM       4
#define LHC_BLK_Y_NUM       16
#define LHC_BLK_BIN_NUM     17
#define MTK_MM_MODULE_MAX_NUM	4

#define LHC_BLK_X_NUMS		(LHC_BLK_X_NUM * MTK_MM_MODULE_MAX_NUM)

typedef struct _mtk_wrapper_lhc_hist {
	__u8 module_num;
	__u8 r[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
	__u8 g[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
	__u8 b[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
} mtk_wrapper_lhc_hist;

#define LHC_POWER_ON     _IO(LHC_DEVICE_IOC_TYPE, 1)
#define LHC_POWER_OFF    _IO(LHC_DEVICE_IOC_TYPE, 2)
#define LHC_CONFIG_SLICE _IOW(LHC_DEVICE_IOC_TYPE, 3, args_lhc_config_slice)
#define LHC_START        _IO(LHC_DEVICE_IOC_TYPE, 4)
#define LHC_STOP         _IO(LHC_DEVICE_IOC_TYPE, 5)
#endif /* _MTK_WRAPPER_LHC_*/
