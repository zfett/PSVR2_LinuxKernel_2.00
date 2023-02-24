/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
#define pr_fmt(fmt) "%s:%d:" fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#include <linux/of_reserved_mem.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>

#include "cammem.h"

#define CAMMEM_MEM_SLOT_MAX 512
struct cammem_slot {
	void *virt_addr_kernel;
	void *virt_addr_user;
	dma_addr_t phy_addr;
	unsigned int size;
	int in_use;
};

struct cammem_rsvd_device {
	struct device *rdev;
	struct dma_attrs dma_attrs;
	struct cammem_slot mems[CAMMEM_MEM_SLOT_MAX];
	struct mutex mutex_mems;
};

static int mtk_cammem_get_free_slot(struct cammem_rsvd_device *rsvd_dev)
{
	int i;

	for (i = 0; i < CAMMEM_MEM_SLOT_MAX; i++) {
		if (rsvd_dev->mems[i].in_use == 0) {
			rsvd_dev->mems[i].in_use = 1;
			return i;
		}
	}

	return 0;
}

static int mtk_cammem_rsvd_alloc(
	struct cammem_rsvd_device *rsvd_dev, struct ISP_MEMORY_STRUCT *isp_mem)
{
	int free_slot_idx, alloc_size;
	struct cammem_slot *free_slot;
	int rc;

	mutex_lock(&rsvd_dev->mutex_mems);
	free_slot_idx = mtk_cammem_get_free_slot(rsvd_dev);
	free_slot = &rsvd_dev->mems[free_slot_idx];
	alloc_size = roundup(isp_mem->size, PAGE_SIZE);

	free_slot->virt_addr_kernel = dma_alloc_attrs(
				rsvd_dev->rdev, alloc_size,
				&free_slot->phy_addr, GFP_KERNEL,
				&rsvd_dev->dma_attrs);
	if (!free_slot->virt_addr_kernel) {
		pr_err("dma_alloc_attrs va size %u fail\n",
					(unsigned int)alloc_size);
		rc = -ENOMEM;
		goto dma_alloc_err;
	} else {
		memset(free_slot->virt_addr_kernel, 0x0, alloc_size);
		free_slot->size = alloc_size;
	}

	isp_mem->phyAddr = (int *) free_slot->phy_addr;
	isp_mem->KerVirtAddr = free_slot->virt_addr_kernel;
	isp_mem->size = alloc_size;
	isp_mem->memID = free_slot_idx;
	mutex_unlock(&rsvd_dev->mutex_mems);

	pr_debug("phy=0x%p virt=0x%p %d\n",
		isp_mem->phyAddr, isp_mem->KerVirtAddr, isp_mem->size);

	return 0;

dma_alloc_err:
	mutex_unlock(&rsvd_dev->mutex_mems);
	return rc;
}

static int mtk_cammem_rsvd_dealloc(
	struct cammem_rsvd_device *rsvd_dev, struct ISP_MEMORY_STRUCT *isp_mem)
{
	int dealloc_slot_idx = isp_mem->memID;
	struct cammem_slot *dealloc_slot;

	if (dealloc_slot_idx < 0 || dealloc_slot_idx >= CAMMEM_MEM_SLOT_MAX)
		return -EINVAL;

	mutex_lock(&rsvd_dev->mutex_mems);
	dealloc_slot = &rsvd_dev->mems[dealloc_slot_idx];
	if (!dealloc_slot->virt_addr_kernel) {
		mutex_unlock(&rsvd_dev->mutex_mems);
		return -EFAULT;
	}

	pr_debug("phy=0x%x virt=0x%p size=%d\n",
		(unsigned int)dealloc_slot->phy_addr,
		dealloc_slot->virt_addr_kernel, dealloc_slot->size);

	dma_free_attrs(rsvd_dev->rdev, dealloc_slot->size,
		       dealloc_slot->virt_addr_kernel,
		       dealloc_slot->phy_addr, &rsvd_dev->dma_attrs);
	dealloc_slot->in_use = 0;

	mutex_unlock(&rsvd_dev->mutex_mems);

	return 0;
}

static long mtk_cammem_rsvd_ioctl(
	void *private_data, unsigned int cmd, struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct cammem_rsvd_device *rsvd_dev =
				(struct cammem_rsvd_device *)private_data;
	int rc = 0;

	switch (cmd) {
	case ISP_RSVD_ALLOC:
		rc = mtk_cammem_rsvd_alloc(rsvd_dev, isp_mem);
		break;
	case ISP_RSVD_DEALLOC:
		rc = mtk_cammem_rsvd_dealloc(rsvd_dev, isp_mem);
		break;
	default:
		rc = -EPERM;
		break;
	}

	return rc;
}

static int mtk_cammem_rsvd_mmap(void *private_data, struct vm_area_struct *vma)
{
	struct cammem_rsvd_device *rsvd_dev =
				(struct cammem_rsvd_device *)private_data;
	int slot_idx = vma->vm_pgoff >> 4;
	struct cammem_slot *allocated_slot = &rsvd_dev->mems[slot_idx];
	int rc = 0;

	vma->vm_pgoff = 0;
	rc = dma_mmap_attrs(rsvd_dev->rdev, vma,
			    allocated_slot->virt_addr_kernel,
			    allocated_slot->phy_addr,
			    allocated_slot->size,
			    &rsvd_dev->dma_attrs);
	if (rc < 0)
		pr_err("dma_mmap_attrs fail (%d)", rc);

	pr_debug("phy=0x%x virt=0x%p size=%d\n",
		(unsigned int)allocated_slot->phy_addr,
		allocated_slot->virt_addr_kernel, allocated_slot->size);

	return rc;
}

static void *mtk_cammem_rsvd_probe(struct platform_device *pdev)
{
	struct cammem_rsvd_device *rsvd_dev;
	int rc;

	rsvd_dev = kzalloc(sizeof(struct cammem_rsvd_device), GFP_KERNEL);
	if (!rsvd_dev)
		return ERR_PTR(-ENOMEM);

	rsvd_dev->rdev = &pdev->dev;

	rc =  of_reserved_mem_device_init(rsvd_dev->rdev);
	if (rc)
		pr_debug("init reserved memory failed\n");

	memset(rsvd_dev->mems, 0, sizeof(rsvd_dev->mems));
	mutex_init(&rsvd_dev->mutex_mems);
	init_dma_attrs(&rsvd_dev->dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE | DMA_ATTR_FORCE_CONTIGUOUS,
		     &rsvd_dev->dma_attrs);

	return rsvd_dev;
}
static int mtk_cammem_rsvd_remove(void *rsvd_dev)
{
	kfree(rsvd_dev);

	return 0;
}

struct camera_mem_device_ops cammem_rsvd_ops = {
	.cammem_probe = mtk_cammem_rsvd_probe,
	.cammem_remove = mtk_cammem_rsvd_remove,
	.cammem_ioctl = mtk_cammem_rsvd_ioctl,
	.cammem_mmap = mtk_cammem_rsvd_mmap,
};
#endif
