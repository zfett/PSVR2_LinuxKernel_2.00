/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	CK Hu <ck.hu@mediatek.com>
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

#ifndef MTK_DEV_CV_COMMON_H
#define MTK_DEV_CV_COMMON_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <uapi/mediatek/mtk_vr_tracking_common.h>
#include <linux/completion.h>
#include <dt-bindings/gce/mt3612-gce.h>

struct cmdq_flush_completion {
	/** completion */
	struct completion cmplt;
	/** error status */
	bool err;
};

int kern_log_level;

#define LOG(level, ...)			\
do {					\
	if (level <= kern_log_level) {		\
		pr_err(__VA_ARGS__);	\
	}				\
} while (0)

int gaze_kern_log_level;

#define GZ_LOG(level, ...)			\
do {					\
	if (level <= gaze_kern_log_level) {		\
		pr_err(__VA_ARGS__);	\
	}				\
} while (0)


/*#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))*/
/*#define load_pitch(w, align)	roundup(w, align)*/
#define load_buf_size(w, align, h)	((roundup(w, align)) * h)

#endif /* MTK_DEV_CV_COMMON_H */
