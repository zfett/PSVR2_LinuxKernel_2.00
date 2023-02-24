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

#ifndef __IVPBUF_CORE_H__
#define __IVPBUF_CORE_H__

#include <linux/dma-buf.h>
#include <linux/mm_types.h>
#include <linux/mutex.h>
#include <linux/poll.h>

#include <uapi/mediatek/ivp_ioctl.h>

#include "ivp_driver.h"

#define DECLARE_VLIST(type) \
struct type ## _list { \
	struct type node; \
	struct list_head link; \
}

/*
 * vlist_node_of - get the pointer to the node which has specific vlist
 * @ptr:	the pointer to struct list_head
 * @type:	the type of list node
 */
#define vlist_node_of(ptr, type) ({ \
	const struct list_head *mptr = (ptr); \
	(type *)((char *)mptr - offsetof(type ## _list, link)); })

/*
 * vlist_link - get the pointer to struct list_head
 * @ptr:	the pointer to struct vlist
 * @type:	the type of list node
 */
#define vlist_link(ptr, type) \
	((struct list_head *)((char *)ptr + offsetof(type ## _list, link)))

/*
 * vlist_type - get the type of struct vlist
 * @type:	the type of list node
 */
#define vlist_type(type) type ## _list

/*
 * vlist_node - get the pointer to the node of vlist
 * @ptr:	the pointer to struct vlist
 * @type:	the type of list node
 */
#define vlist_node(ptr, type) ((type *)ptr)

struct ivp_alloc_ctx;

enum ivp_memory {
	IVP_MEMORY_UNKNOWN = 0,
	IVP_MEMORY_MMAP = 1,
	IVP_MEMORY_DMABUF = 2,
};

struct ivp_mem_ops {
	void *(*alloc)(void *alloc_ctx, unsigned long size,
		       enum dma_data_direction dma_dir, gfp_t gfp_flags,
		       unsigned int is_cached);
	void (*put)(void *buf_priv);
	struct dma_buf *(*get_dmabuf)(void *buf_priv, unsigned long flags);

	void *(*get_userptr)(void *alloc_ctx, unsigned long vaddr,
			     unsigned long size,
			     enum dma_data_direction dma_dir);
	void (*put_userptr)(void *buf_priv);

	void (*prepare)(void *buf_priv);
	void (*finish)(void *buf_priv);

	void *(*attach_dmabuf)(void *alloc_ctx, struct dma_buf *dbuf,
			       unsigned long size,
			       enum dma_data_direction dma_dir);
	void (*detach_dmabuf)(void *buf_priv);
	int (*map_dmabuf)(void *buf_priv);
	void (*unmap_dmabuf)(void *buf_priv);

	void *(*vaddr)(void *buf_priv);
	void *(*cookie)(void *buf_priv);

	unsigned int (*num_users)(void *buf_priv);

	int (*mmap)(void *buf_priv, struct vm_area_struct *vma);
};

struct ivp_buffer {
	struct ivp_manage *ivp_manage;
	unsigned int memory;
	unsigned int is_cached;

	void *mem_priv;
	unsigned int dma_addr;	/* IOVA */
	void *vaddr;		/* kernel virtual address */
	struct dma_buf *dbuf;
	unsigned int dbuf_mapped;
	unsigned int length;	/* buffer size */
	unsigned int offset;	/* for MMAP */
	int dma_fd;		/* for DMABUF */
};

struct ivp_mem_seg {
	dma_addr_t dma_addr;
	u32 size;
	void *vaddr;
};

/* static memory layout */
struct ivp_core_mem {
	struct ivp_mem_seg fw;
	struct ivp_mem_seg sdbg;
	struct ivp_mem_seg sdbg_org;
};

struct ivp_manage {
	const struct ivp_mem_ops *mem_ops;

	/* static memory layout */
	struct ivp_core_mem core_mem[IVP_CORE_NUM];
	struct ivp_mem_seg algo_mem;
	struct ivp_mem_seg share_mem;

	/* dynamic memory allocation */
	struct mutex buf_mutex;
	struct idr ivp_idr;

	void *alloc_ctx;
	unsigned int is_output:1;
};

/* For IOCTL */
int ivp_manage_alloc(struct ivp_device *ivp_device,
		     struct ivp_alloc_buf *alloc_ctx);
int ivp_manage_query(struct ivp_manage *mng,
		     struct ivp_mmap_buf *mmap_ctx);
int ivp_manage_mmap(struct ivp_manage *mng,
		    struct vm_area_struct *vma);
int ivp_manage_free(struct ivp_manage *mng,
		    struct ivp_free_buf *free_ctx);
int ivp_manage_export(struct ivp_manage *mng,
		      struct ivp_export_buf_to_fd *export_ctx);
int ivp_manage_import(struct ivp_manage *mng,
		      struct ivp_import_fd_to_buf *import_ctx);
int ivp_manage_sync_cache(struct ivp_manage *mng,
			  struct ivp_sync_cache_buf *sync_ctx);
int ivp_buf_manage_init(struct ivp_device *ivp_device);
void ivp_buf_manage_deinit(struct ivp_device *ivp_device);
void ivp_add_dbg_buf(struct ivp_user *user, s32 buf_handle);
void ivp_delete_dbg_buf(struct ivp_user *user, s32 buf_handle);
void ivp_check_dbg_buf(struct ivp_user *user);
#endif
