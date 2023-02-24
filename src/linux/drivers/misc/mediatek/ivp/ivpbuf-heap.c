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
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "ivpbuf-heap.h"

static int _ivp_heap_check(struct ivp_block *head)
{
	struct ivp_block *cur_block = head;

	while (cur_block) {
		if (cur_block->next) {
			if (cur_block->addr + cur_block->size !=
			    cur_block->next->addr) {
				pr_err("heap list is error\n");
				return -EFAULT;
			}
		}
		cur_block = cur_block->next;
	}

	return 0;
}

struct ivp_block *ivp_heap_init(unsigned int addr, unsigned int size)
{
	struct ivp_block *list;

	list = kzalloc(sizeof(*list), GFP_KERNEL);
	if (!list)
		return ERR_PTR(-ENOMEM);

	list->addr = addr;
	list->size = size;
	list->is_alloc = 0;
	list->next = NULL;

	return list;
}

void ivp_heap_deinit(struct ivp_block *list)
{
	if (list) {
		if (list->next)
			pr_err("memory leak\n");
	}

	kfree(list);
}

int ivp_heap_alloc(struct ivp_block *head, unsigned int alloc_size,
		   unsigned int *alloc_addr)
{
	struct ivp_block *cur_block = head;
	struct ivp_block *new_node;
	unsigned int left_size = 0;

	/* page size, align to 4K size */
	alloc_size = PAGE_ALIGN(alloc_size);

	/* find the available block */
	while (cur_block) {
		if (cur_block->is_alloc == 0 && cur_block->size >= alloc_size)
			break;
		cur_block = cur_block->next;
	}

	if (!cur_block)
		return -ENOMEM;

	if (cur_block->size > alloc_size) {
		/* create new block */
		new_node = kzalloc(sizeof(*new_node), GFP_KERNEL);
		if (!new_node)
			return -ENOMEM;

		/* separate the current block */
		left_size = cur_block->size - alloc_size;
		cur_block->size = alloc_size;
		cur_block->is_alloc = 1;

		/* set the new block to record the remaining memory */
		new_node->addr = cur_block->addr + cur_block->size;
		new_node->size = left_size;
		new_node->is_alloc = 0;

		/* insert new block to heap */
		new_node->next = cur_block->next;
		cur_block->next = new_node;
	} else {
		/* use this available block */
		cur_block->is_alloc = 1;
	}

	if (_ivp_heap_check(head) != 0)
		return -EFAULT;

	*alloc_addr = cur_block->addr;

	return 0;
}

int ivp_heap_free(struct ivp_block *head, unsigned int free_addr)
{
	struct ivp_block *prev_node = NULL;
	struct ivp_block *next_node = NULL;
	struct ivp_block *cur_block = head;

	/*  find the free block */
	while (cur_block) {
		if (cur_block->addr == free_addr)
			break;
		prev_node = cur_block;
		cur_block = cur_block->next;
	}

	if (!cur_block) {
		pr_err("ivp heap free addr error\n");
		return -EINVAL;
	}

	/* change is_alloc state */
	cur_block->is_alloc = 0;
	next_node = cur_block->next;

	/* Merge current block with the next block */
	if (next_node) {
		if (next_node->is_alloc == 0) {
			/* change the cur_block  */
			cur_block->size = cur_block->size + next_node->size;
			/* remove the next block from heap */
			cur_block->next = next_node->next;
			/* release the next block */
			kfree(next_node);
		}
	}
	/* Merge current block with the prev block */
	if (prev_node) {
		if (prev_node->is_alloc == 0) {
			/* change the prev block */
			prev_node->size = prev_node->size + cur_block->size;
			/* remove the current block */
			prev_node->next = cur_block->next;
			/* release the current block */
			kfree(cur_block);
		}
	}

	if (_ivp_heap_check(head) != 0)
		return -EFAULT;

	return 0;
}
