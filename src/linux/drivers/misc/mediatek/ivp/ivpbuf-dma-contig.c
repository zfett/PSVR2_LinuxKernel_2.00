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

#include <linux/dma-buf.h>
#include <linux/dma-iommu.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "ivpbuf-core.h"
#include "ivpbuf-dma-contig.h"
#include "ivpbuf-heap.h"
#include "ivpbuf-memops.h"

int mtk_ion_is_gpu_sram(struct dma_buf *dmabuf);
#define SYSRAM_ADDR      0x0000000009000000ULL
#define SYSRAM_ADDR_MASK 0xFFFFFFFFFF000000ULL
#define IS_SYSRAM_ADDR(a) (((a) & SYSRAM_ADDR_MASK) == SYSRAM_ADDR)
#define VPU_SYSRAM_ADDR  0x000000007F000000ULL
#define SYSRAM_ADDR_TO_VPU_ADDR(a) (((a) & (~SYSRAM_ADDR_MASK)) | VPU_SYSRAM_ADDR)

#define IVP_DYNAMIC_MEMORY_BASE 0x60000000
#define IVP_DYNAMIC_MEMORY_SIZE 0x10000000

static struct ivp_block *g_ivp_heap;

struct ivp_dc_conf {
	struct device *dev;
};

struct ivp_dc_buf {
	struct device *dev;
	void *vaddr;
	unsigned long size;
	dma_addr_t dma_addr;
	enum dma_data_direction dma_dir;
	struct sg_table *dma_sgt;
	struct frame_vector *vec;
	struct sg_table *remap_sgt;

	/* MMAP related */
	struct ivp_vmarea_handler handler;
	atomic_t refcount;
	struct sg_table *sgt_base;

	/* cache related */
	unsigned int is_cached;

	/* DMABUF related */
	struct dma_buf_attachment *db_attach;

	/* SYSRAM related */
	int is_sysram;
};

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static struct sg_table *ivp_dc_get_base_sgt(struct ivp_dc_buf *buf)
{
	int ret;
	struct sg_table *sgt;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return NULL;

	ret = dma_get_sgtable(buf->dev, sgt, buf->vaddr, buf->dma_addr,
			      buf->size);
	if (ret < 0) {
		dev_err(buf->dev, "failed to get scatterlist from DMA API\n");
		kfree(sgt);
		return NULL;
	}

	return sgt;
}

static void *ivp_dc_cookie(void *buf_priv)
{
	struct ivp_dc_buf *buf = buf_priv;

	return &buf->dma_addr;
}

static void *ivp_dc_vaddr(void *buf_priv)
{
	struct ivp_dc_buf *buf = buf_priv;

	if (!buf->vaddr && buf->db_attach)
		buf->vaddr = dma_buf_vmap(buf->db_attach->dmabuf);

	return buf->vaddr;
}

static void ivp_dc_prepare(void *buf_priv)
{
	struct ivp_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	/* DMABUF exporter will flush the cache for us */
	if (!sgt || buf->db_attach)
		return;

	if (buf->is_cached)
		dma_sync_sg_for_device(buf->dev, sgt->sgl, sgt->orig_nents,
				       buf->dma_dir);
}

static void ivp_dc_finish(void *buf_priv)
{
	struct ivp_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	/* DMABUF exporter will flush the cache for us */
	if (!sgt || buf->db_attach)
		return;

	if (buf->is_cached)
		dma_sync_sg_for_cpu(buf->dev, sgt->sgl, sgt->orig_nents,
				    buf->dma_dir);
}

/*********************************************/
/*        callbacks for MMAP buffers         */
/*********************************************/

static void ivp_dc_put(void *buf_priv)
{
	struct ivp_dc_buf *buf = buf_priv;

	if (!atomic_dec_and_test(&buf->refcount))
		return;

	if (buf->dma_sgt) {
		sg_free_table(buf->dma_sgt);
		kfree(buf->dma_sgt);
	}

	if (buf->sgt_base) {
		sg_free_table(buf->sgt_base);
		kfree(buf->sgt_base);
	}
	/* ivp free iova memory */
	dma_free_coherent_fix_iova(buf->dev, buf->vaddr, buf->dma_addr,
				   buf->size);

	ivp_heap_free(g_ivp_heap, buf->dma_addr);

	put_device(buf->dev);
	kfree(buf);
}

static void *ivp_dc_alloc(void *alloc_ctx, unsigned long size,
			  enum dma_data_direction dma_dir, gfp_t gfp_flags,
			  unsigned int is_cached)
{
	struct ivp_dc_conf *conf = alloc_ctx;
	struct device *dev = conf->dev;
	struct ivp_dc_buf *buf;
	unsigned int alloc_addr;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	/* ivp alloc iova memory */
	if (ivp_heap_alloc(g_ivp_heap, size, &alloc_addr)) {
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}
	buf->dma_addr = (dma_addr_t)alloc_addr;
	buf->vaddr = dma_alloc_coherent_fix_iova(dev, buf->dma_addr, size,
						 GFP_KERNEL | gfp_flags);

	if (!buf->vaddr) {
		dev_err(dev, "dma_alloc_coherent of size %ld failed\n", size);
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	/* Prevent the device from being released while the buffer is used */
	buf->dev = get_device(dev);
	buf->size = size;
	buf->dma_dir = dma_dir;
	buf->is_cached = is_cached;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = ivp_dc_put;
	buf->handler.arg = buf;

	if (!buf->dma_sgt)
		buf->dma_sgt = ivp_dc_get_base_sgt(buf);

	atomic_inc(&buf->refcount);

	return buf;
}

static int ivp_dc_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct ivp_dc_buf *buf = buf_priv;
	int ret;

	if (!buf)
		return -EINVAL;

	/*
	 * dma_mmap_* uses vm_pgoff as in-buffer offset, but we want to
	 * map whole buffer
	 */
	vma->vm_pgoff = 0;

	if (buf->is_cached) {
		ret = dma_mmap_attrs_cached(buf->dev, vma, buf->vaddr,
						  buf->dma_addr, buf->size,
						  NULL);
		if (ret) {
			pr_err("Remapping memory failed, error: %d\n", ret);
			return ret;
		}
	} else {
		ret = dma_mmap_coherent(buf->dev, vma, buf->vaddr,
					buf->dma_addr, buf->size);
		if (ret) {
			pr_err("Remapping memory failed, error: %d\n", ret);
			return ret;
		}
	}

	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = &buf->handler;
	vma->vm_ops = &ivp_common_vm_ops;

	vma->vm_ops->open(vma);

	pr_debug("%s: mapped dma addr 0x%08lx at 0x%08lx, size %ld\n",
		 __func__, (unsigned long)buf->dma_addr, vma->vm_start,
		 buf->size);

	return 0;
}

/*********************************************/
/*         DMABUF ops for exporters          */
/*********************************************/

struct ivp_dc_attachment {
	struct sg_table sgt;
	enum dma_data_direction dma_dir;
};

static int ivp_dc_dmabuf_ops_attach(struct dma_buf *dbuf, struct device *dev,
				    struct dma_buf_attachment *dbuf_attach)
{
	struct ivp_dc_attachment *attach;
	unsigned int i;
	struct scatterlist *rd, *wr;
	struct sg_table *sgt;
	struct ivp_dc_buf *buf = dbuf->priv;
	int ret;

	attach = kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach)
		return -ENOMEM;

	sgt = &attach->sgt;
	/* Copy the buf->base_sgt scatter list to the attachment, as we can't
	 * map the same scatter list to multiple attachments at the same time.
	 */
	ret = sg_alloc_table(sgt, buf->sgt_base->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(attach);
		return -ENOMEM;
	}

	rd = buf->sgt_base->sgl;
	wr = sgt->sgl;
	for (i = 0; i < sgt->orig_nents; ++i) {
		sg_set_page(wr, sg_page(rd), rd->length, rd->offset);
		rd = sg_next(rd);
		wr = sg_next(wr);
	}

	attach->dma_dir = DMA_NONE;
	dbuf_attach->priv = attach;

	return 0;
}

static void ivp_dc_dmabuf_ops_detach(struct dma_buf *dbuf,
				     struct dma_buf_attachment *db_attach)
{
	struct ivp_dc_attachment *attach = db_attach->priv;
	struct sg_table *sgt;

	if (!attach)
		return;

	sgt = &attach->sgt;

	/* release the scatterlist cache */
	if (attach->dma_dir != DMA_NONE)
		dma_unmap_sg(db_attach->dev, sgt->sgl, sgt->orig_nents,
			     attach->dma_dir);
	sg_free_table(sgt);
	kfree(attach);
	db_attach->priv = NULL;
}

static struct sg_table *ivp_dc_dmabuf_ops_map(struct dma_buf_attachment
					      *db_attach,
					      enum dma_data_direction dma_dir)
{
	struct ivp_dc_attachment *attach = db_attach->priv;
	/* stealing dmabuf mutex to serialize map/unmap operations */
	struct mutex *lock = &db_attach->dmabuf->lock;
	struct sg_table *sgt;

	mutex_lock(lock);

	sgt = &attach->sgt;
	/* return previously mapped sg table */
	if (attach->dma_dir == dma_dir) {
		mutex_unlock(lock);
		return sgt;
	}

	/* release any previous cache */
	if (attach->dma_dir != DMA_NONE) {
		dma_unmap_sg(db_attach->dev, sgt->sgl, sgt->orig_nents,
			     attach->dma_dir);
		attach->dma_dir = DMA_NONE;
	}

	/* mapping to the client with new direction */
	sgt->nents = dma_map_sg(db_attach->dev, sgt->sgl, sgt->orig_nents,
				dma_dir);
	if (!sgt->nents) {
		pr_err("failed to map scatterlist\n");
		mutex_unlock(lock);
		return ERR_PTR(-EIO);
	}

	attach->dma_dir = dma_dir;

	mutex_unlock(lock);

	return sgt;
}

static void ivp_dc_dmabuf_ops_unmap(struct dma_buf_attachment *db_attach,
				    struct sg_table *sgt,
				    enum dma_data_direction dma_dir)
{
	/* nothing to be done here */
}

static void ivp_dc_dmabuf_ops_release(struct dma_buf *dbuf)
{
	/* drop reference obtained in ivp_dc_get_dmabuf */
	ivp_dc_put(dbuf->priv);
}

static void *ivp_dc_dmabuf_ops_kmap(struct dma_buf *dbuf, unsigned long pgnum)
{
	struct ivp_dc_buf *buf = dbuf->priv;

	return buf->vaddr + pgnum * PAGE_SIZE;
}

static void *ivp_dc_dmabuf_ops_vmap(struct dma_buf *dbuf)
{
	struct ivp_dc_buf *buf = dbuf->priv;

	return buf->vaddr;
}

static int ivp_dc_dmabuf_ops_mmap(struct dma_buf *dbuf,
				  struct vm_area_struct *vma)
{
	return ivp_dc_mmap(dbuf->priv, vma);
}

static struct dma_buf_ops ivp_dc_dmabuf_ops = {
	.attach = ivp_dc_dmabuf_ops_attach,
	.detach = ivp_dc_dmabuf_ops_detach,
	.map_dma_buf = ivp_dc_dmabuf_ops_map,
	.unmap_dma_buf = ivp_dc_dmabuf_ops_unmap,
	.kmap = ivp_dc_dmabuf_ops_kmap,
	.kmap_atomic = ivp_dc_dmabuf_ops_kmap,
	.vmap = ivp_dc_dmabuf_ops_vmap,
	.mmap = ivp_dc_dmabuf_ops_mmap,
	.release = ivp_dc_dmabuf_ops_release,
};



static struct dma_buf *ivp_dc_get_dmabuf(void *buf_priv, unsigned long flags)
{
	struct ivp_dc_buf *buf = buf_priv;
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	/* coherent memory is suitable for export */
	buf->is_cached = 0;

	exp_info.ops = &ivp_dc_dmabuf_ops;
	exp_info.size = buf->size;
	exp_info.flags = flags;
	exp_info.priv = buf;

	if (!buf->sgt_base)
		buf->sgt_base = ivp_dc_get_base_sgt(buf);

	if (WARN_ON(!buf->sgt_base))
		return NULL;

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR(dbuf))
		return NULL;

	/* dmabuf keeps reference to ivp buffer */
	atomic_inc(&buf->refcount);

	return dbuf;
}

/*********************************************/
/*       callbacks for DMABUF buffers        */
/*********************************************/
static struct sg_table *ivp_dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sg(table->sgl, sg, table->nents, i) {
		memcpy(new_sg, sg, sizeof(*sg));
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static void ivp_del_dup_sg_table(struct sg_table *table)
{
	sg_free_table(table);
	kfree(table);
}

static int ivp_dc_map_dmabuf(void *mem_priv)
{
	struct ivp_dc_buf *buf = mem_priv;
	struct sg_table *sgt, *dup_sgt;
	unsigned int alloc_addr;

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to pin a non attached buffer\n");
		return -EINVAL;
	}

	if (WARN_ON(buf->dma_sgt)) {
		pr_err("dmabuf buffer is already pinned\n");
		return 0;
	}

	/* get the associated scatterlist for this buffer */
	sgt = dma_buf_map_attachment(buf->db_attach, buf->dma_dir);
	if (IS_ERR(sgt)) {
		pr_err("Error getting dmabuf scatterlist\n");
		return -EINVAL;
	}

	if (buf->is_sysram) {
		// SYSRAM
		buf->dma_addr = sg_dma_address(sgt->sgl);
		if (!IS_SYSRAM_ADDR(buf->dma_addr)) {
			dma_buf_unmap_attachment(buf->db_attach, sgt,buf->dma_dir);
			return -EINVAL;
		}
		buf->dma_addr = SYSRAM_ADDR_TO_VPU_ADDR(buf->dma_addr);
		dup_sgt = NULL;
	} else {
		// DRAM
		if (ivp_heap_alloc(g_ivp_heap, buf->size, &alloc_addr)) {
			dma_buf_unmap_attachment(buf->db_attach, sgt,buf->dma_dir);
			return -ENOMEM;
		}
		buf->dma_addr = (dma_addr_t)alloc_addr;

		/* dup sgt */
		dup_sgt = ivp_dup_sg_table(sgt);
		/* use dup_sgt to do remap */
		dup_sgt->nents = dma_map_sg_within_reserved_iova(buf->dev,
								 dup_sgt->sgl,
								 dup_sgt->orig_nents,
								 IOMMU_READ |
								 IOMMU_WRITE,
								 buf->dma_addr);
		if (!dup_sgt->nents) {
			pr_err("failed to map scatterlist\n");
			ivp_del_dup_sg_table(dup_sgt);
			ivp_heap_free(g_ivp_heap, buf->dma_addr);
			return -EIO;
		}
	}

	pr_debug("%s: addr=%pad, size=%lx", __func__, &buf->dma_addr,
		buf->size);

	buf->dma_sgt = sgt;
	buf->remap_sgt = dup_sgt;
	buf->vaddr = NULL;

	return 0;
}

static void ivp_dc_unmap_dmabuf(void *mem_priv)
{
	struct ivp_dc_buf *buf = mem_priv;
	struct sg_table *sgt = buf->dma_sgt;
	struct sg_table *dup_sgt = buf->remap_sgt;

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to unpin a not attached buffer\n");
		return;
	}

	if (WARN_ON(!sgt)) {
		pr_err("dmabuf buffer is already unpinned\n");
		return;
	}

	if (buf->vaddr) {
		dma_buf_vunmap(buf->db_attach->dmabuf, buf->vaddr);
		buf->vaddr = NULL;
	}

	if (dup_sgt) {
		/* unmap dup_sgt */
		dma_unmap_sg_within_reserved_iova(buf->dev, dup_sgt->sgl,
						  dup_sgt->nents, buf->dma_dir,
						  buf->size);
		/* del dup_sgt */
		ivp_del_dup_sg_table(dup_sgt);

		ivp_heap_free(g_ivp_heap, buf->dma_addr);
	}

	dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);

	buf->dma_addr = 0;
	buf->dma_sgt = NULL;
	buf->remap_sgt = NULL;
}

static void ivp_dc_detach_dmabuf(void *mem_priv)
{
	struct ivp_dc_buf *buf = mem_priv;

	/* if ivp works correctly you should never detach mapped buffer */
	if (WARN_ON(buf->dma_addr))
		ivp_dc_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static void *ivp_dc_attach_dmabuf(void *alloc_ctx, struct dma_buf *dbuf,
				  unsigned long size,
				  enum dma_data_direction dma_dir)
{
	struct ivp_dc_conf *conf = alloc_ctx;
	struct ivp_dc_buf *buf;
	struct dma_buf_attachment *dba;
	int ret;

	if (dbuf->size < size)
		return ERR_PTR(-EFAULT);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = conf->dev;
	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		pr_err("failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = dma_dir;
	buf->size = size;
	buf->db_attach = dba;
	buf->is_cached = 0;
	ret = mtk_ion_is_gpu_sram(dbuf);
	if (ret < 0)
		buf->is_sysram = 0;
	else
		buf->is_sysram = ret;

	return buf;
}

/*********************************************/
/*       DMA CONTIG exported functions       */
/*********************************************/

const struct ivp_mem_ops ivp_dma_contig_memops = {
	.alloc = ivp_dc_alloc,
	.put = ivp_dc_put,
	.get_dmabuf = ivp_dc_get_dmabuf,
	.cookie = ivp_dc_cookie,
	.vaddr = ivp_dc_vaddr,
	.mmap = ivp_dc_mmap,
	.prepare = ivp_dc_prepare,
	.finish = ivp_dc_finish,
	.map_dmabuf = ivp_dc_map_dmabuf,
	.unmap_dmabuf = ivp_dc_unmap_dmabuf,
	.attach_dmabuf = ivp_dc_attach_dmabuf,
	.detach_dmabuf = ivp_dc_detach_dmabuf,
};

void *ivp_dma_contig_init_ctx(struct device *dev)
{
	struct ivp_dc_conf *conf;

	conf = kzalloc(sizeof(*conf), GFP_KERNEL);
	if (!conf)
		return ERR_PTR(-ENOMEM);

	conf->dev = dev;

	g_ivp_heap =
	    ivp_heap_init(IVP_DYNAMIC_MEMORY_BASE, IVP_DYNAMIC_MEMORY_SIZE);
	if (!g_ivp_heap)
		return ERR_PTR(-ENOMEM);

	return conf;
}

void ivp_dma_contig_cleanup_ctx(void *alloc_ctx)
{
	if (!IS_ERR_OR_NULL(alloc_ctx))
		kfree(alloc_ctx);

	ivp_heap_deinit(g_ivp_heap);
}
