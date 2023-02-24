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

#ifndef _MTK_WRAPPER_MUTEX_
#define _MTK_WRAPPER_MUTEX_

#include <linux/ioctl.h>
#include <linux/types.h>
#include "mtk_wrapper_common.h"

#ifndef __KERNEL__
#include <stdint.h>
#endif

#define MUTEX_DEVICE_IOC_TYPE 'M'

enum MTK_WRAPPER_MUTEX_INDEX {
	/* display mutex use 8-16 */
	MTK_WRAPPER_MUTEX_DISP = 8,
	MTK_WRAPPER_MUTEX_DP,
	MTK_WRAPPER_MUTEX_SBRC,
	MTK_WRAPPER_MUTEX_MAX_NUM
};

enum MTK_WRAPPER_MUTEX_SOF {
	MTK_WRAPPER_MUTEX_SOF_SINGLE,
	MTK_WRAPPER_MUTEX_SOF_DSI,
	MTK_WRAPPER_MUTEX_SOF_DP,
	MTK_WRAPPER_MUTEX_SOF_DP_DSC,
	MTK_WRAPPER_MUTEX_SOF_SYNC_DELAY,
};

struct args_mutex_set_config {
	enum MTK_WRAPPER_MUTEX_INDEX idx;
	enum MTK_WRAPPER_MUTEX_SOF  sof_component;
	DISPLAY_COMPONENT  sof_timer_component;
	__u32 src_time;
	__u32 ref_time;
};

struct args_mutex_add_component {
	enum MTK_WRAPPER_MUTEX_INDEX idx;
	DISPLAY_COMPONENT component;
};

struct args_mutex_remove_component {
	enum MTK_WRAPPER_MUTEX_INDEX idx;
	DISPLAY_COMPONENT component;
};

struct args_mutex_enable {
	enum MTK_WRAPPER_MUTEX_INDEX idx;
	bool enable;
};

#define MUTEX_GET_RESOURCE _IOW(MUTEX_DEVICE_IOC_TYPE, 1, enum MTK_WRAPPER_MUTEX_INDEX)
#define MUTEX_PUT_RESOURCE _IOW(MUTEX_DEVICE_IOC_TYPE, 2, enum MTK_WRAPPER_MUTEX_INDEX)
#define MUTEX_SET_CONFIG   _IOW(MUTEX_DEVICE_IOC_TYPE, 3, struct args_mutex_set_config)
#define MUTEX_ADD_COMPONENT _IOW(MUTEX_DEVICE_IOC_TYPE, 4, struct args_mutex_add_component)
#define MUTEX_REMOVE_COMPONENT _IOW(MUTEX_DEVICE_IOC_TYPE, 5, struct args_mutex_remove_component)
#define MUTEX_ENABLE     _IOW(MUTEX_DEVICE_IOC_TYPE, 6, struct args_mutex_enable)
#define MUTEX_POWER_ON   _IOW(MUTEX_DEVICE_IOC_TYPE, 7, enum MTK_WRAPPER_MUTEX_INDEX)
#define MUTEX_POWER_OFF  _IOW(MUTEX_DEVICE_IOC_TYPE, 8, enum MTK_WRAPPER_MUTEX_INDEX)

#endif /* _MTK_WRAPPER_MUTEX_ */
