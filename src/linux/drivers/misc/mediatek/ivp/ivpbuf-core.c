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

/**
 * @file ivpbuf_core.c
 * Handle about IVP memory management.
 */

#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/freezer.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "ivpbuf-core.h"
#include "ivpbuf-dma-contig.h"
#include "ivpbuf-dma-memcpy.h"

#define IVP_LAYOUT_CONFIG_FILE  "ivp_layout.bin"

DECLARE_VLIST(ivp_dbg_buf);

static int __ivp_buf_mmap_alloc(struct ivp_buffer *ivpb)
{
	struct ivp_manage *mng = ivpb->ivp_manage;
	enum dma_data_direction dma_dir = DMA_BIDIRECTIONAL;
	void *mem_priv;

	/*
	 * Allocate memory for all planes in this buffer
	 * NOTE: mmapped areas should be page aligned
	 */
	unsigned long size = PAGE_ALIGN(ivpb->length);

	mem_priv = mng->mem_ops->alloc(mng->alloc_ctx, size, dma_dir, 0,
				       ivpb->is_cached);
	if (IS_ERR_OR_NULL(mem_priv))
		goto free;

	/* Associate allocator private data with this plane */
	ivpb->mem_priv = mem_priv;
	ivpb->dma_addr = *(int *)mng->mem_ops->cookie(mem_priv);
	ivpb->vaddr = mng->mem_ops->vaddr(mem_priv);

	return 0;
free:
	ivpb->mem_priv = NULL;

	return -ENOMEM;
}

static void __ivp_buf_mmap_free(struct ivp_buffer *ivpb)
{
	struct ivp_manage *mng = ivpb->ivp_manage;

	if (!ivpb->mem_priv)
		return;

	mng->mem_ops->put(ivpb->mem_priv);
	ivpb->mem_priv = NULL;
}

static void __ivp_buf_dmabuf_put(struct ivp_buffer *ivpb)
{
	struct ivp_manage *mng = ivpb->ivp_manage;

	if (!ivpb->mem_priv)
		return;

	if (ivpb->dbuf_mapped)
		mng->mem_ops->unmap_dmabuf(ivpb->mem_priv);

	mng->mem_ops->detach_dmabuf(ivpb->mem_priv);
	dma_buf_put(ivpb->dbuf);
	ivpb->mem_priv = NULL;
	ivpb->dbuf = NULL;
	ivpb->dbuf_mapped = 0;
}

static int __ivp_buf_mem_import(struct ivp_buffer *ivpb,
				unsigned int dma_fd)
{
	struct ivp_manage *mng = ivpb->ivp_manage;
	void *mem_priv;
	int ret;
	enum dma_data_direction dma_dir = DMA_BIDIRECTIONAL;
	struct dma_buf *dbuf = dma_buf_get(dma_fd);

	if (IS_ERR_OR_NULL(dbuf)) {
		pr_err("invalid dmabuf fd\n");
		ret = -EINVAL;
		goto err;
	}

	/* use DMABUF size if length is not provided */
	ivpb->length = dbuf->size;

	/* Acquire each plane's memory */
	mem_priv =
	    mng->mem_ops->attach_dmabuf(mng->alloc_ctx, dbuf, ivpb->length,
					dma_dir);
	if (IS_ERR(mem_priv)) {
		pr_err("failed to attach dmabuf\n");
		ret = PTR_ERR(mem_priv);
		dma_buf_put(dbuf);
		goto err;
	}

	ivpb->dbuf = dbuf;
	ivpb->mem_priv = mem_priv;

	/* TODO: This pins the buffer(s) with  dma_buf_map_attachment()).. but
	 * really we want to do this just before the DMA, not while queueing
	 * the buffer(s)..
	 */
	ret = mng->mem_ops->map_dmabuf(ivpb->mem_priv);
	if (ret) {
		pr_err("failed to map dmabuf\n");
		goto err;
	}
	ivpb->dbuf_mapped = 1;

	/*
	 * Now that everything is in order, copy relevant information
	 * provided by userspace.
	 */
	ivpb->dma_fd = dma_fd;
	ivpb->dma_addr = *(int *)mng->mem_ops->cookie(mem_priv);
	ivpb->vaddr = mng->mem_ops->vaddr(mem_priv);

	return 0;
err:
	/* In case of errors, release planes that were already acquired */
	__ivp_buf_dmabuf_put(ivpb);

	return ret;
}

static void __ivp_mem_free_layout(struct ivp_device *ivp_device)
{
	int core_id;
	struct device *dev = ivp_device->dev;
	struct ivp_manage *mng = ivp_device->ivp_mng;
	struct ivp_mem_seg *algo = &mng->algo_mem;
	struct ivp_mem_seg *share = &mng->share_mem;

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		struct ivp_core_mem *core_m = &mng->core_mem[core_id];

		if (core_m->fw.vaddr)
			dma_free_coherent_fix_iova(dev,
						   core_m->fw.vaddr,
						   core_m->fw.dma_addr,
						   core_m->fw.size);
		if (core_m->sdbg.vaddr)
			dma_free_coherent_fix_iova(dev,
						   core_m->sdbg.vaddr,
						   core_m->sdbg.dma_addr,
						   core_m->sdbg.size);
	}

	if (algo->vaddr)
		dma_free_coherent_fix_iova(dev,
					   algo->vaddr,
					   algo->dma_addr,
					   algo->size);
	if (share->vaddr)
		dma_free_coherent_fix_iova(dev,
					   share->vaddr,
					   share->dma_addr,
					   share->size);
}

static int __ivp_mem_alloc_layout(struct ivp_device *ivp_device,
				  char *layout_name)
{
	struct device *dev = ivp_device->dev;
	int ret, core_id, layout_num;
	const struct firmware *ivp_layout;
	const u8 *layout_base = NULL;
	struct ivp_mem_layout *mem_layout = NULL;
	struct ivp_manage *mng = ivp_device->ivp_mng;
	struct ivp_mem_seg *algo = &mng->algo_mem;
	struct ivp_mem_seg *share = &mng->share_mem;
	struct ivp_mem_block2 *fw;

	/* request layout */
	ret = request_firmware(&ivp_layout, layout_name, dev);
	if (ret < 0) {
		pr_err("Failed to load %s, %d\n", layout_name, ret);
		return ret;
	}

	/* static buffer */
	layout_base = ivp_layout->data;
	layout_num = ivp_device->ivp_core_num * 4 + 4;
	if (ivp_layout->size / sizeof(int) < layout_num) {
		dev_err(ivp_device->dev, "wrong memory layout, num (%ld) < %d\n",
			ivp_layout->size / sizeof(int), layout_num);
		release_firmware(ivp_layout);
		return -EINVAL;
	}
	mem_layout = (struct ivp_mem_layout *)layout_base;
	fw = &mem_layout->fw;

	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		struct ivp_core_mem *core_m = &mng->core_mem[core_id];

		core_m->fw.vaddr = NULL;
		core_m->sdbg.vaddr = NULL;

		core_m->fw.dma_addr = fw->iaddr1;
		core_m->fw.size = fw->size1;
		core_m->fw.vaddr =
		    dma_alloc_coherent_fix_iova(dev, core_m->fw.dma_addr,
						core_m->fw.size, GFP_KERNEL);
		if (!core_m->fw.vaddr) {
			ret = -ENOMEM;
			goto fini;
		}

		core_m->sdbg.dma_addr = fw->iaddr2;
		core_m->sdbg.size = fw->size2;
		core_m->sdbg.vaddr =
		    dma_alloc_coherent_fix_iova(dev, core_m->sdbg.dma_addr,
						core_m->sdbg.size, GFP_KERNEL);
		core_m->sdbg_org = core_m->sdbg;
		if (!core_m->sdbg.vaddr) {
			ret = -ENOMEM;
			goto fini;
		}

		fw = fw + 1;
	}

	algo->vaddr = NULL;

	algo->dma_addr = mem_layout->algo.iaddr;
	algo->size = mem_layout->algo.size;
	algo->vaddr =
	    dma_alloc_coherent_fix_iova(dev, algo->dma_addr, algo->size,
					GFP_KERNEL);
	if (!algo->vaddr) {
		ret = -ENOMEM;
		goto fini;
	}

	share->dma_addr = mem_layout->sbuf.iaddr;
	share->size = mem_layout->sbuf.size;
	share->vaddr =
	    dma_alloc_coherent_fix_iova(dev, share->dma_addr, share->size,
					GFP_KERNEL);
	if (!share->vaddr) {
		ret = -ENOMEM;
		goto fini;
	}

	ret = 0;

fini:
	if (ret)
		__ivp_mem_free_layout(ivp_device);
	/* release fw */
	release_firmware(ivp_layout);
	return ret;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     To create buffer.
 * @param[out]
 *     mng: pointer to the ivp memory manage.
 * @param[out]
 *     alloc_ctx: pointer to the alloc buffer contex.
 * @return
 *     0, successful to creat buffer. \n
 *     -ENOMEM, out of memory.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If allocating buffer memory fails, return -ENOMEM.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_manage_alloc(struct ivp_device *ivp_device,
		     struct ivp_alloc_buf *alloc_ctx)
{
	struct ivp_manage *mng = ivp_device->ivp_mng;
	struct ivp_buffer *ivpb;
	int id;
	int ret;

	/* Allocate videobuf buffer structures */
	ivpb = kzalloc(sizeof(*ivpb), GFP_KERNEL);
	if (!ivpb)
		return -ENOMEM;

	mutex_lock(&mng->buf_mutex);

	ivpb->ivp_manage = mng;
	ivpb->length = alloc_ctx->req_size;
	ivpb->memory = IVP_MEMORY_MMAP;
	ivpb->is_cached = alloc_ctx->is_cached;

	ret = __ivp_buf_mmap_alloc(ivpb);
	if (ret) {
		pr_err("failed allocating memory\n");
		kfree(ivpb);
		mutex_unlock(&mng->buf_mutex);
		return ret;
	}

	id = idr_alloc(&mng->ivp_idr, ivpb, 1, INT_MAX / PAGE_SIZE, GFP_KERNEL);
	ivpb->offset = id * PAGE_SIZE;

	mutex_unlock(&mng->buf_mutex);

	alloc_ctx->handle = id;
	alloc_ctx->iova_addr = ivpb->dma_addr;
	alloc_ctx->iova_size = PAGE_ALIGN(ivpb->length);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     To return offset value for mmap() function.
 * @param[in]
 *     mng: pointer to the ivp memory manage.
 * @param[out]
 *     mmap_ctx: pointer to the mmap buffer.
 * @return
 *     0, successful to query ivp buffer offset value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_manage_query(struct ivp_manage *mng, struct ivp_mmap_buf *mmap_ctx)
{
	struct ivp_buffer *ivpb;

	mutex_lock(&mng->buf_mutex);

	ivpb = idr_find(&mng->ivp_idr, mmap_ctx->handle);

	if (!ivpb) {
		pr_err("%s fail, ivp_buffer is invalid\n", __func__);
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	mmap_ctx->offset = ivpb->offset;

	mutex_unlock(&mng->buf_mutex);

	return 0;
}

int ivp_manage_mmap(struct ivp_manage *mng, struct vm_area_struct *vma)
{
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
	struct ivp_buffer *ivpb = NULL;
	int ret;
	unsigned long length;
	struct ivp_buffer *ivpb_t;
	s32 id;

	/*
	 * Check memory area access mode.
	 */
	if (!(vma->vm_flags & VM_SHARED)) {
		pr_err("invalid vma flags, VM_SHARED needed\n");
		return -EINVAL;
	}
	if (mng->is_output) {
		if (!(vma->vm_flags & VM_WRITE)) {
			pr_err("invalid vma flags, VM_WRITE needed\n");
			return -EINVAL;
		}
	} else {
		if (!(vma->vm_flags & VM_READ)) {
			pr_err("invalid vma flags, VM_READ needed\n");
			return -EINVAL;
		}
	}

	/* find the buffer queue */
	mutex_lock(&mng->buf_mutex);
	idr_for_each_entry(&mng->ivp_idr, ivpb_t, id) {
		if (ivpb_t->offset == off) {
			ivpb = ivpb_t;
			break;
		}
	}

	if (!ivpb) {
		pr_err("MMAP invalid, as offset value is invalid\n");
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	/*
	 * MMAP requires page_aligned buffers.
	 * The buffer length was page_aligned at __ivp_buf_mmap_alloc(),
	 * so, we need to do the same here.
	 */
	length = PAGE_ALIGN(ivpb->length);
	if (length < (vma->vm_end - vma->vm_start)) {
		pr_err("MMAP invalid, as it would overflow buffer length\n");
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	ret = mng->mem_ops->mmap(ivpb->mem_priv, vma);
	if (ret) {
		mutex_unlock(&mng->buf_mutex);
		return ret;
	}

	mutex_unlock(&mng->buf_mutex);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     To free the created buffer.
 * @param[out]
 *     mng: pointer to the ivp memory manage.
 * @param[out]
 *     free_ctx: pointer to the free buffer.
 * @return
 *     0, successful to free the created buffer. \n
 *     -EINVAL, wrong input parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If enter an wrong memory, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_manage_free(struct ivp_manage *mng, struct ivp_free_buf *free_ctx)
{
	struct ivp_buffer *ivpb;

	mutex_lock(&mng->buf_mutex);

	ivpb = idr_find(&mng->ivp_idr, free_ctx->handle);
	if (!ivpb) {
		pr_err("%s fail, ivp_buffer is invalid\n", __func__);
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	if (ivpb->memory == IVP_MEMORY_MMAP) {
		__ivp_buf_mmap_free(ivpb);
	} else if (ivpb->memory == IVP_MEMORY_DMABUF) {
		__ivp_buf_dmabuf_put(ivpb);
	} else {
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	idr_remove(&mng->ivp_idr, free_ctx->handle);
	kfree(ivpb);

	mutex_unlock(&mng->buf_mutex);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     To export the allocated memory for other user space applications.
 * @param[in]
 *     mng: pointer to the ivp memory manage.
 * @param[out]
 *     export_ctx: pointer to the export buffer.
 * @return
 *     0, successful to export the allocated memory. \n
 *     -EINVAL, wrong input parameter. \n
 *     error code from dma_buf_fd().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there are some wrong params, return -EINVAL. \n
 *     If export buffer fails, return the error code of dma_buf_fd().
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_manage_export(struct ivp_manage *mng,
		      struct ivp_export_buf_to_fd *export_ctx)
{
	struct ivp_buffer *ivpb = NULL;
	struct dma_buf *dbuf;
	unsigned int flags;
	int ret = 0;

	mutex_lock(&mng->buf_mutex);

	flags = O_RDWR | O_CLOEXEC;

	if (!mng->mem_ops->get_dmabuf) {
		pr_err("queue does not support DMA buffer exporting\n");
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	ivpb = idr_find(&mng->ivp_idr, export_ctx->handle);

	if (!ivpb) {
		pr_err("%s fail, ivp_buffer is invalid\n", __func__);
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	if (ivpb->memory != IVP_MEMORY_MMAP) {
		pr_err("buffer is not currently set up for mmap\n");
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	dbuf = mng->mem_ops->get_dmabuf(ivpb->mem_priv, flags & O_ACCMODE);
	if (IS_ERR_OR_NULL(dbuf)) {
		pr_err("failed to export buffer %d\n", export_ctx->handle);
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	ret = dma_buf_fd(dbuf, flags & ~O_ACCMODE);
	if (ret < 0) {
		pr_err("buffer %d, failed to export (%d)\n",
			export_ctx->handle, ret);
		dma_buf_put(dbuf);
		mutex_unlock(&mng->buf_mutex);
		return ret;
	}

	export_ctx->dma_fd = ret;

	mutex_unlock(&mng->buf_mutex);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     To create buffer through using the memory \n
 *     which is allocated by other drivers.
 * @param[out]
 *     mng: pointer to the ivp memory manage.
 * @param[out]
 *     import_ctx: pointer to the import buffer.
 * @return
 *     0, successful to import buffer. \n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there are some wrong params, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_manage_import(struct ivp_manage *mng,
		      struct ivp_import_fd_to_buf *import_ctx)
{
	struct ivp_buffer *ivpb;
	int ret;
	int id;

	mutex_lock(&mng->buf_mutex);

	/* Allocate videobuf buffer structures */
	ivpb = kzalloc(sizeof(*ivpb), GFP_KERNEL);
	if (!ivpb) {
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	ivpb->ivp_manage = mng;
	ivpb->length = 0;
	ivpb->memory = IVP_MEMORY_DMABUF;

	ret = __ivp_buf_mem_import(ivpb, import_ctx->dma_fd);
	if (ret) {
		pr_err("failed importing memory\n");
		kfree(ivpb);
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	id = idr_alloc(&mng->ivp_idr, ivpb, 1, INT_MAX / PAGE_SIZE, GFP_KERNEL);
	ivpb->offset = id * PAGE_SIZE;

	import_ctx->handle = id;
	import_ctx->size = ivpb->length;
	import_ctx->iova_addr = ivpb->dma_addr;

	mutex_unlock(&mng->buf_mutex);

	return 0;
}

int ivp_manage_sync_cache(struct ivp_manage *mng,
			  struct ivp_sync_cache_buf *sync_ctx)
{
	struct ivp_buffer *ivpb;

	mutex_lock(&mng->buf_mutex);

	ivpb = idr_find(&mng->ivp_idr, sync_ctx->handle);
	if (!ivpb) {
		pr_err("%s fail, ivp_buffer is invalid\n", __func__);
		mutex_unlock(&mng->buf_mutex);
		return -EINVAL;
	}

	if (sync_ctx->direction == IVP_SYNC_TO_DEVICE)
		mng->mem_ops->prepare(ivpb->mem_priv);
	else
		mng->mem_ops->finish(ivpb->mem_priv);

	mutex_unlock(&mng->buf_mutex);

	return 0;
}

int ivp_buf_manage_init(struct ivp_device *ivp_device)
{
	struct ivp_alloc_ctx *alloc_ctx;
	struct ivp_manage *mng;
	char *layout_name = IVP_LAYOUT_CONFIG_FILE;
	int ret = 0;

	ivp_device->ivp_mng = kzalloc(sizeof(*ivp_device->ivp_mng), GFP_KERNEL);

	mng = ivp_device->ivp_mng;

	alloc_ctx = ivp_dma_contig_init_ctx(ivp_device->dev);
	if (IS_ERR(alloc_ctx))
		return -ENOMEM;

	mng->mem_ops = &ivp_dma_contig_memops;

	mutex_init(&mng->buf_mutex);
	idr_init(&mng->ivp_idr);
	mng->alloc_ctx = alloc_ctx;
	mng->is_output = 1;

	ret = __ivp_mem_alloc_layout(ivp_device, layout_name);

	return ret;
}

void ivp_buf_manage_deinit(struct ivp_device *ivp_device)
{
	struct ivp_manage *mng;

	mng = ivp_device->ivp_mng;

	__ivp_mem_free_layout(ivp_device);
	ivp_dma_contig_cleanup_ctx(mng->alloc_ctx);
	mng->alloc_ctx = NULL;
	idr_destroy(&mng->ivp_idr);
	kfree(mng);
}

void ivp_add_dbg_buf(struct ivp_user *user, s32 buf_handle)
{
	struct ivp_dbg_buf *dbgbuf;

	dbgbuf = kzalloc(sizeof(vlist_type(struct ivp_dbg_buf)), GFP_KERNEL);
	if (!dbgbuf)
		return;

	dbgbuf->handle = buf_handle;

	mutex_lock(&user->dbgbuf_mutex);
	list_add_tail(vlist_link(dbgbuf, struct ivp_dbg_buf),
		      &user->dbgbuf_list);
	mutex_unlock(&user->dbgbuf_mutex);
}

void ivp_delete_dbg_buf(struct ivp_user *user, s32 buf_handle)
{
	struct list_head *head, *temp;
	struct ivp_dbg_buf *dbgbuf;

	mutex_lock(&user->dbgbuf_mutex);
	list_for_each_safe(head, temp, &user->dbgbuf_list) {
		dbgbuf = vlist_node_of(head, struct ivp_dbg_buf);
		if (dbgbuf->handle == buf_handle) {
			list_del_init(vlist_link(dbgbuf, struct ivp_dbg_buf));
			kfree(dbgbuf);
			break;
		}
	}
	mutex_unlock(&user->dbgbuf_mutex);
}

void ivp_check_dbg_buf(struct ivp_user *user)
{
	struct list_head *head, *temp;
	struct ivp_dbg_buf *dbgbuf;
	struct ivp_device *ivp_device = dev_get_drvdata(user->dev);

	mutex_lock(&user->dbgbuf_mutex);
	list_for_each_safe(head, temp, &user->dbgbuf_list) {
		dbgbuf = vlist_node_of(head, struct ivp_dbg_buf);
		pr_debug("[IVP BUF] free buffer leak : %d\n", dbgbuf->handle);
		ivp_manage_free(ivp_device->ivp_mng,
				(struct ivp_free_buf *)dbgbuf);
	}
	mutex_unlock(&user->dbgbuf_mutex);
}

