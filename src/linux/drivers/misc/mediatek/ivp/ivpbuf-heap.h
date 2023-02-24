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

#ifndef __IVPBUF_HEAP_H__
#define __IVPBUF_HEAP_H__

struct ivp_block {
	unsigned int addr;
	unsigned int size;
	unsigned int is_alloc;
	struct ivp_block *next;
};

struct ivp_block *ivp_heap_init(unsigned int addr, unsigned int size);
void ivp_heap_deinit(struct ivp_block *list);
int ivp_heap_alloc(struct ivp_block *head, unsigned int alloc_size,
		   unsigned int *alloc_addr);
int ivp_heap_free(struct ivp_block *head, unsigned int free_addr);

#endif
