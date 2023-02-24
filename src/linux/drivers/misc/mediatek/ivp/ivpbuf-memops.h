/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Fish Wu <fish.wu@mediatek.com>
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

#ifndef _IVPBUF_MEMOPS_H
#define _IVPBUF_MEMOPS_H

#include <linux/mm.h>

#include "ivpbuf-core.h"

/**
 * struct ivp_vmarea_handler - common vma refcount tracking handler
 *
 * @refcount:	pointer to refcount entry in the buffer
 * @put:	callback to function that decreases buffer refcount
 * @arg:	argument for @put callback
 */
struct ivp_vmarea_handler {
	atomic_t *refcount;
	void (*put)(void *arg);
	void *arg;
};

extern const struct vm_operations_struct ivp_common_vm_ops;

struct frame_vector *ivp_create_framevec(unsigned long start,
					 unsigned long length, bool write);
void ivp_destroy_framevec(struct frame_vector *vec);

#endif
