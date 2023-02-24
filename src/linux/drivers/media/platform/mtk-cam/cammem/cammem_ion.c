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

#define pr_fmt(fmt) "%s:%d:" fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/dma-buf.h>
#include <linux/idr.h>
#include <linux/scatterlist.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>

#include <soc/mediatek/smi.h>
#include <soc/mediatek/mtk_cammem.h>

#include "cammem.h"

#define ISP_MEMORY_MAX_USER 512

enum CAMERA_LARB_ENUM {
	LARB2,
	LARB5,
	CAMERA_LARB_MAX_NUM
};

struct isp_ion_dma_buf {
	int ion_buf_sfd;
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *import_attach;
	void *kvaddr;
	dma_addr_t dma_addr;
	struct sg_table *orig_sg;
	struct sg_table *sg;
};

struct cammem_ion_device {
	struct device *iondev;
	int larb_nr;
	struct device *larb_dev[CAMERA_LARB_MAX_NUM];
	struct idr mems;
	spinlock_t mems_lock;
};

static int mtk_cammem_ion_open(void *private_data)
{
	struct cammem_ion_device *ion_dev =
			(struct cammem_ion_device *)private_data;
	int i, rc;

	idr_init(&ion_dev->mems);

	for (i = 0; i < ion_dev->larb_nr; i++) {
		rc = mtk_smi_larb_get(ion_dev->larb_dev[i]);
		if (rc != 0)
			pr_err("mtk_smi_larb_get(larb_dev[%d]) fail(%d)\n",
			       i, rc);
	}

	pr_debug("\n");

	return 0;
}

static void mtk_cammem_ion_close(void *private_data)
{
	struct cammem_ion_device *ion_dev =
			(struct cammem_ion_device *)private_data;
	int i, key;
	struct isp_ion_dma_buf *buf;

	spin_lock(&ion_dev->mems_lock);
	if (!idr_is_empty(&ion_dev->mems)) {
		pr_debug("There are un-freed buffers");
		idr_for_each_entry(&ion_dev->mems, buf, key) {
			pr_debug("Force free isp buf key(%d), PA(%pad)",
				key, &buf->dma_addr);
			/* detach dma_buf w/ iova */
			dma_buf_unmap_attachment(buf->import_attach, buf->sg,
						 DMA_BIDIRECTIONAL);
			dma_buf_detach(buf->dma_buf, buf->import_attach);
			dma_buf_put(buf->dma_buf);
			kfree(buf);

			/* delete from map & device's context */
			idr_remove(&ion_dev->mems, key);
		}
	}
	spin_unlock(&ion_dev->mems_lock);

	idr_destroy(&ion_dev->mems);

	for (i = 0; i < ion_dev->larb_nr; i++) {
		if (!ion_dev->larb_dev[i])
			pr_err("ion_dev->larb_dev[%d] is null\n", i);

		mtk_smi_larb_put(ion_dev->larb_dev[i]);
	}
}

static struct isp_ion_dma_buf *isp_buf_init(void)
{
	struct isp_ion_dma_buf *buf;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	return buf;
}

static struct sg_table *dup_sg_table(struct sg_table *table)
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

static struct isp_ion_dma_buf *isp_import_sg_table(
		struct device *dev, struct dma_buf_attachment *attach,
		struct sg_table *orig_sg)
{
	struct isp_ion_dma_buf *buf;
	int ret, nents;
	struct scatterlist *s;
	unsigned int i;
	dma_addr_t expected;
	struct sg_table *sg = dup_sg_table(orig_sg);

	buf = isp_buf_init();

	if (IS_ERR(buf)) {
		sg_free_table(sg);
		kfree(sg);
		return ERR_PTR(PTR_ERR(buf));
	}

	nents = dma_map_sg(dev, sg->sgl, sg->orig_nents, DMA_BIDIRECTIONAL);
	if (nents <= 0) {
		pr_err("dma_map_sg fail\n");
		ret = -EINVAL;
		goto err_buf_free;
	}

	expected = sg_dma_address(sg->sgl);
	pr_debug("expected PA(%pad)\n", &expected);
	for_each_sg(sg->sgl, s, nents, i) {
		if (sg_dma_address(s) != expected) {
			pr_err("sg_table is not contiguous\n");
			ret = -EINVAL;
			goto err_unmap;
		}
		expected = sg_dma_address(s) + sg_dma_len(s);
		pr_debug("sgt(%d): PA(%pad) + len(0x%x) -> expectPA(%pad)\n",
			i, &sg_dma_address(s), sg_dma_len(s), &expected);
	}


	buf->dma_addr = sg_dma_address(sg->sgl);
	buf->orig_sg = orig_sg;
	buf->sg = sg;

	return buf;

err_unmap:
	dma_unmap_sg(dev, sg->sgl, sg->orig_nents, DMA_BIDIRECTIONAL);
err_buf_free:
	sg_free_table(sg);
	kfree(sg);
	kfree(buf);
	return ERR_PTR(ret);
}

static struct isp_ion_dma_buf *isp_import_fd(struct device *dev,
		struct dma_buf *dma_buf)
{
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct isp_ion_dma_buf *buf;
	int ret;

	attach = dma_buf_attach(dma_buf, dev);
	if (IS_ERR(attach))
		return ERR_CAST(attach);

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		goto fail_detach;
	}

	buf = isp_import_sg_table(dev, attach, sgt);
	if (IS_ERR(buf)) {
		ret = PTR_ERR(buf);
		goto fail_unmap;
	}

	buf->import_attach = attach;

	return buf;

fail_unmap:
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
fail_detach:
	dma_buf_detach(dma_buf, attach);
	dma_buf_put(dma_buf);

	return ERR_PTR(ret);
}

static int mtk_cammem_ion_mapphy(struct cammem_ion_device *ion_dev,
			struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct dma_buf *dma_buf;
	struct isp_ion_dma_buf *buf;
	int fd = isp_mem->memID;
	int rc;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);

	/* Create isp buf by importing ion fd */
	buf = isp_import_fd(ion_dev->iondev, dma_buf);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	/* record all dma buffers for management */
	buf->ion_buf_sfd = fd;
	buf->dma_buf = dma_buf;
	isp_mem->phyAddr = (int *)buf->dma_addr;

	idr_preload(GFP_KERNEL);
	spin_lock(&ion_dev->mems_lock);
	rc = idr_alloc(&ion_dev->mems, buf, 0,
			ISP_MEMORY_MAX_USER, GFP_NOWAIT);
	if (rc < 0) {
		pr_err("No free buffers can be allocated\n");
		spin_unlock(&ion_dev->mems_lock);
		goto idr_alloc_err;
	}
	isp_mem->key = rc;
	spin_unlock(&ion_dev->mems_lock);
	idr_preload_end();

	return 0;

idr_alloc_err:
	dma_buf_put(dma_buf);
	return rc;
}

static int mtk_cammem_ion_unmapphy(struct cammem_ion_device *ion_dev,
			struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct isp_ion_dma_buf *buf;

	spin_lock(&ion_dev->mems_lock);
	buf = idr_find(&ion_dev->mems, isp_mem->key);
	if (!buf) {
		pr_err("Can't find isp buf: key(%d), PA(%p)",
			isp_mem->key, isp_mem->phyAddr);
		spin_unlock(&ion_dev->mems_lock);
		return -EINVAL;
	}
	/* delete from map & device's context */
	idr_remove(&ion_dev->mems, isp_mem->key);
	spin_unlock(&ion_dev->mems_lock);

	dma_unmap_sg(ion_dev->iondev, buf->sg->sgl, buf->sg->orig_nents,
		DMA_BIDIRECTIONAL);
	sg_free_table(buf->sg);
	kfree(buf->sg);

	/* detach dma_buf w/ iova */
	dma_buf_unmap_attachment(buf->import_attach, buf->orig_sg,
		DMA_BIDIRECTIONAL);
	dma_buf_detach(buf->dma_buf, buf->import_attach);
	dma_buf_put(buf->dma_buf);
	kfree(buf);

	return 0;
}

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
static int mtk_cammem_ion_map_kervrt(
	struct cammem_ion_device *ion_dev, struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct dma_buf *dma_buf;
	struct isp_ion_dma_buf *buf;
	int fd = isp_mem->memID;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);

	/* Create isp buf by importing ion fd */
	buf = isp_import_fd(ion_dev->iondev, dma_buf);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	/* record all dma buffers for management */
	buf->ion_buf_sfd = fd;
	buf->dma_buf = dma_buf;

	idr_preload(GFP_KERNEL);
	spin_lock(&ion_dev->mems_lock);

	dma_buf_begin_cpu_access(
		buf->import_attach->dmabuf, 0, isp_mem->size, 0);
	if (buf->kvaddr == NULL)  {
		buf->kvaddr = dma_buf_vmap(buf->import_attach->dmabuf);
		isp_mem->KerVirtAddr = (int *)buf->kvaddr;
		pr_info("mapping kvaddr = %p\n",  (void *)buf->kvaddr);
	} else {
		pr_err("kvaddr had been mapped = %p\n",  (void *)buf->kvaddr);
	}

	spin_unlock(&ion_dev->mems_lock);
	idr_preload_end();

	return 0;
}

static int mtk_cammem_ion_unmap_kervrt(
	struct cammem_ion_device *ion_dev, struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct dma_buf *dma_buf;
	struct isp_ion_dma_buf *buf;
	int fd = isp_mem->memID;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);

	/* Create isp buf by importing ion fd */
	buf = isp_import_fd(ion_dev->iondev, dma_buf);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	/* record all dma buffers for management */
	buf->ion_buf_sfd = fd;
	buf->dma_buf = dma_buf;

	idr_preload(GFP_KERNEL);
	spin_lock(&ion_dev->mems_lock);

	buf->kvaddr = isp_mem->KerVirtAddr;

	pr_debug("unmap kvaddr = %p\n",  (void *)buf->kvaddr);

	dma_buf_end_cpu_access(
		buf->import_attach->dmabuf, 0, isp_mem->size, 0);
	if (buf->kvaddr) {
		dma_buf_vunmap(buf->import_attach->dmabuf, buf->kvaddr);
		buf->kvaddr = NULL;
		isp_mem->KerVirtAddr = NULL;
	}

	spin_unlock(&ion_dev->mems_lock);
	idr_preload_end();

	return 0;
}

static int mtk_cammem_ion_cache_flush(
	struct cammem_ion_device *ion_dev, struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct dma_buf *dma_buf;
	struct isp_ion_dma_buf *buf;

	int fd = isp_mem->memID;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);

	spin_lock(&ion_dev->mems_lock);
	buf = idr_find(&ion_dev->mems, isp_mem->key);
	if (!buf) {
		pr_err("Can't find isp buf: key(%d), PA(%p)",
			   isp_mem->key, isp_mem->phyAddr);
		spin_unlock(&ion_dev->mems_lock);
		return -EINVAL;
	}
	spin_unlock(&ion_dev->mems_lock);

	dma_sync_sg_for_device(ion_dev->iondev,
			       buf->sg->sgl,
			       buf->sg->nents,
			       DMA_TO_DEVICE);

	return 0;
}

static int mtk_cammem_ion_cache_invalid(
	struct cammem_ion_device *ion_dev, struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct dma_buf *dma_buf;
	struct isp_ion_dma_buf *buf;

	int fd = isp_mem->memID;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);

	spin_lock(&ion_dev->mems_lock);
	buf = idr_find(&ion_dev->mems, isp_mem->key);
	if (!buf) {
		pr_err("Can't find isp buf: key(%d), PA(%p)",
			   isp_mem->key, isp_mem->phyAddr);
		spin_unlock(&ion_dev->mems_lock);
		return -EINVAL;
	}
	spin_unlock(&ion_dev->mems_lock);

	dma_sync_sg_for_cpu(ion_dev->iondev,
			    buf->sg->sgl,
			    buf->sg->nents,
			    DMA_FROM_DEVICE);

	return 0;
}
#endif

static int mtk_cammem_ion_sram_phy_addr(struct cammem_ion_device *ion_dev, struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct dma_buf *dma_buf;
	int fd = isp_mem->memID;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	int rc = 0;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);
	attach = dma_buf_attach(dma_buf, ion_dev->iondev);
	if (IS_ERR(attach))
		return -1;
	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		rc = PTR_ERR(sgt);
		goto fail_detach;
	}
	isp_mem->phyAddr = (int *)sg_dma_address(sgt->sgl);
fail_detach:
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(dma_buf, attach);
	dma_buf_put(dma_buf);

	return rc;
}

static long mtk_cammem_ion_ioctl(
	void *private_data, unsigned int cmd, struct ISP_MEMORY_STRUCT *isp_mem)
{
	struct cammem_ion_device *ion_dev =
				(struct cammem_ion_device *)private_data;
	int rc = 0;

	switch (cmd) {
	case ISP_ION_MAPPHY:
		rc = mtk_cammem_ion_mapphy(ion_dev, isp_mem);
		if (rc)
			pr_err("ISP_ION_MAPPHY: import buf failed\n");

		pr_debug("IonBufFd(%d)-key(%d): PA(0x%p), size(0x%x)",
				isp_mem->memID, isp_mem->key,
				isp_mem->phyAddr, isp_mem->size);
		break;
	case ISP_ION_UNMAPPHY:
		pr_debug("[%s] IonBufFd(%d)-key(%d): size:0x%x,UVA:%p,PA:%p)",
			isp_mem->name, isp_mem->memID,
			isp_mem->key, isp_mem->size,
			isp_mem->UserVirtAddr, isp_mem->phyAddr);

		rc = mtk_cammem_ion_unmapphy(ion_dev, isp_mem);
		if (rc)
			pr_err("ISP_ION_UNMAPPHY: export buf failed\n");
		break;
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	case ISP_ION_MAP_KERVRT:
		pr_debug("ISP_ION_MAP_KERVRT\n");
		rc = mtk_cammem_ion_map_kervrt(ion_dev, isp_mem);
		if (rc)
			pr_err("ISP_ION_MAP_KERVRT: import buf failed\n");
		pr_debug("[%s] IonBufFd(%d)-key(%d): size:0x%x,UVA:%p,KVA:%p)",
			isp_mem->name, isp_mem->memID,
			isp_mem->key, isp_mem->size,
			isp_mem->UserVirtAddr, isp_mem->KerVirtAddr);
		break;
	case ISP_ION_UNMAP_KERVRT:
		pr_debug("ISP_ION_UNMAP_KERVRT\n");
		rc = mtk_cammem_ion_unmap_kervrt(ion_dev, isp_mem);
		if (rc)
			pr_err("ISP_ION_UNMAP_KERVRT: import buf failed\n");
		break;
	case ISP_ION_CACHE_FLUSH:
		pr_debug("ISP_ION_CACHE_FLUSH\n");
		rc = mtk_cammem_ion_cache_flush(ion_dev, isp_mem);
		if (rc)
			pr_err("ISP_ION_CACHE_FLUSH: failed\n");
		break;
	case ISP_ION_CACHE_INVALID:
		pr_debug("ISP_ION_CACHE_INVALID\n");
		rc = mtk_cammem_ion_cache_invalid(ion_dev, isp_mem);
		if (rc)
			pr_err("ISP_ION_CACHE_INVALID: failed\n");
		break;
#endif
	case ISP_ION_SRAM_PHY_ADDR:
		pr_debug("ISP_ION_SRAM_PHY_ADDR\n");
		rc = mtk_cammem_ion_sram_phy_addr(ion_dev, isp_mem);
		if (rc)
			pr_err("ISP_ION_SRAM_PHY_ADDR: failed\n");
		break;
	default:
		rc = -EPERM;
		break;
	}

	return rc;
}

static void *mtk_cammem_ion_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *larb_node = NULL;
	struct platform_device *plat_dev = NULL;
	struct cammem_ion_device *ion_dev;
	int i, larb_nr;

	ion_dev = kzalloc(sizeof(struct cammem_ion_device), GFP_KERNEL);
	if (!ion_dev)
		return ERR_PTR(-ENOMEM);

	ion_dev->iondev = &pdev->dev;

	larb_nr = of_count_phandle_with_args(np, "mediatek,larb", NULL);
	if ((larb_nr <= 0) || (larb_nr > CAMERA_LARB_MAX_NUM)) {
		pr_err("No larb or larb# mismatch with dts !!");
		/* do something error handle here. */
	} else {
		ion_dev->larb_nr = larb_nr;
	}

	for (i = 0; i < larb_nr; i++) {
		larb_node = of_parse_phandle(np, "mediatek,larb", i);
		if (!larb_node)
			pr_err("can not find \"mediatek,larb\" node");
		else {
			plat_dev = of_find_device_by_node(larb_node);
			if (!plat_dev)
				pr_err(" larb_dev[%d] NULL", i);
			else {
				/* wait for smi larb probe done */
				if (!plat_dev->dev.driver) {
					kfree(ion_dev);
					return ERR_PTR(-EPROBE_DEFER);
				}

				ion_dev->larb_dev[i] = &plat_dev->dev;
				pr_debug("cam smi larb dev(%d):%p is found",
					 i, ion_dev->larb_dev[i]);
			}
		}
	}

	spin_lock_init(&ion_dev->mems_lock);

	return ion_dev;
}

static int mtk_cammem_ion_remove(void *ion_dev)
{
	kfree(ion_dev);

	return 0;
}

struct camera_mem_device_ops cammem_ion_ops = {
	.cammem_probe = mtk_cammem_ion_probe,
	.cammem_remove = mtk_cammem_ion_remove,
	.cammem_open = mtk_cammem_ion_open,
	.cammem_close = mtk_cammem_ion_close,
	.cammem_ioctl = mtk_cammem_ion_ioctl,
};
