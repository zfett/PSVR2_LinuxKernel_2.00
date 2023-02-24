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

#include <linux/dma-mapping.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "ivpbuf-core.h"
#include "ivpbuf-memops.h"

/**
 * ivp_common_vm_open() - increase refcount of the vma
 * @vma:	virtual memory region for the mapping
 *
 * This function adds another user to the provided vma. It expects
 * struct ivp_vmarea_handler pointer in vma->vm_private_data.
 */
static void ivp_common_vm_open(struct vm_area_struct *vma)
{
	struct ivp_vmarea_handler *h = vma->vm_private_data;

	pr_debug("%s: %p, refcount: %d, vma: %08lx-%08lx\n",
		 __func__, h, atomic_read(h->refcount), vma->vm_start,
		 vma->vm_end);

	atomic_inc(h->refcount);
}

/**
 * ivp_common_vm_close() - decrease refcount of the vma
 * @vma:	virtual memory region for the mapping
 *
 * This function releases the user from the provided vma. It expects
 * struct ivp_vmarea_handler pointer in vma->vm_private_data.
 */
static void ivp_common_vm_close(struct vm_area_struct *vma)
{
	struct ivp_vmarea_handler *h = vma->vm_private_data;

	pr_debug("%s: %p, refcount: %d, vma: %08lx-%08lx\n",
		 __func__, h, atomic_read(h->refcount), vma->vm_start,
		 vma->vm_end);

	h->put(h->arg);
}

/**
 * ivp_common_vm_ops - common vm_ops used for tracking refcount of mmaped
 * video buffers
 */
const struct vm_operations_struct ivp_common_vm_ops = {
	.open = ivp_common_vm_open,
	.close = ivp_common_vm_close,
};
