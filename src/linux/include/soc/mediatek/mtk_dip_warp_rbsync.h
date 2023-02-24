/*
* Copyright (C) 2017 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#ifndef __MTK_DIP_WARP_RBSYNC_H
#define __MTK_DIP_WARP_RBSYNC_H

#define DIP_WARP_RBSYNC_FRM_ERROR_THRESHOLD (1)
#define MAX_WBUF_NUM (32)

struct rb_sync_param {
	unsigned int cur_buf_loc;
	unsigned int frm_idx;
};

struct rb_sync_handle {
	struct rb_sync_param wr;
	struct rb_sync_param rd;
	unsigned int buf_num;
	int error_frm_intervals;
	int status;
	int buf_map[MAX_WBUF_NUM];
};

#endif /* __MTK_DIP_WARP_RBSYNC_H */
