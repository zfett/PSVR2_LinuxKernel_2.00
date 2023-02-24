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

#ifndef _SIE_DATA_H_
#define _SIE_DATA_H_

#include <linux/usb/composite.h>
#include "usb.h"

#define FS_BULK_MAX_PACKET_SIZE		(64)

int usb_data_init(struct class *class, int index, struct usb_function **interface);
void usb_data_destroy(int index);

#if defined(DATA_DEBUG_LOG_ENABLE)
#define GADGET_DATA_LOG(LEVEL, FORMAT, ...)				\
	if ((LEVEL) <= LOG_LEVEL) {					\
		const char *file = __FILE__;				\
		if (strlen(file) > 20) {				\
			file = file + strlen(file) - 20;		\
		}							\
		printk("%s:%d (%s) - " FORMAT,				\
		       file, __LINE__, __FUNCTION__, ##__VA_ARGS__);	\
	}
#else
#define GADGET_DATA_LOG(LEVEL, FORMAT, ...)
#endif

#endif
