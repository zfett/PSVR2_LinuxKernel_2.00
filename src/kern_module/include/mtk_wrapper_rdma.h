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

#ifndef _MTK_WRAPPER_RDMA_
#define _MTK_WRAPPER_RDMA_

#include <linux/ioctl.h>
#include <linux/types.h>

#include "mtk_wrapper_common.h"

#define RDMA_DEVICE_IOC_TYPE 'R'
#define RDMA_MAX_BUFFER_NUM (3)

struct args_rdma_set_config {
	MTK_WRAPPER_FRAME_FORMAT format;
	int ionfd[RDMA_MAX_BUFFER_NUM];
	int buffer_num;
	__u32 pitch;
	__u32 width;
	__u32 height;
	__u32 pvric_header_offset;
	bool     rbfc_setting;
	bool     use_sram;
};

enum rdma_device_id {
	RDMA0,
	RDMA1,
	RDMA2,
	RDMA_ID_MAX
};

enum MTK_WRAPPER_RDMA_TYPE {
	MTK_WRAPPER_RDMA_TYPE_MDP,
	MTK_WRAPPER_RDMA_TYPE_PVRIC,
	MTK_WRAPPER_RDMA_TYPE_DISP,
	MTK_WRAPPER_RDMA_TYPE_MAX
};

#define RDMA_POWER_ON     _IO(RDMA_DEVICE_IOC_TYPE, 1)
#define RDMA_POWER_OFF    _IO(RDMA_DEVICE_IOC_TYPE, 2)
#define RDMA_RESET        _IO(RDMA_DEVICE_IOC_TYPE, 3)
#define RDMA_START        _IO(RDMA_DEVICE_IOC_TYPE, 4)
#define RDMA_STOP         _IO(RDMA_DEVICE_IOC_TYPE, 5)
#define RDMA_CONFIG_FRAME _IOW(RDMA_DEVICE_IOC_TYPE, 6, struct args_rdma_set_config)
#define RDMA_LARB_GET       _IOW(RDMA_DEVICE_IOC_TYPE, 8, bool)
#define RDMA_LARB_PUT       _IOW(RDMA_DEVICE_IOC_TYPE, 9, bool)
#define RDMA_TYPE_SEL      _IOW(RDMA_DEVICE_IOC_TYPE, 11, __u8)
#define RDMA_UPDATE_WP     _IOW(RDMA_DEVICE_IOC_TYPE, 12, int)

#endif /* _MTK_WRAPPER_RDMA_ */
