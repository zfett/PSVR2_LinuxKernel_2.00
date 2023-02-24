/*
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include "../ion.h"
#include "../ion_priv.h"
#include "gpu_sram_heap.h"


int num_heaps;
struct ion_heap **mtkionheaps;
struct ion_device *g_ion_device;
EXPORT_SYMBOL(g_ion_device);

#ifndef ION_CARVEOUT_MEM_BASE
#define ION_CARVEOUT_MEM_BASE 0
#endif

#ifndef ION_CARVEOUT_MEM_SIZE
#define ION_CARVEOUT_MEM_SIZE 0
#endif

#ifndef ION_GPU_FW_MEM_SIZE
#ifdef CONFIG_MTK_GPU_DEDICATED_FW_MEMORY
#define ION_GPU_FW_MEM_START (CONFIG_MTK_GPU_FW_MEMORY_START)
#define ION_GPU_FW_MEM_SIZE (CONFIG_MTK_GPU_FW_MEMORY_SIZE)
#else
#define ION_GPU_FW_MEM_START 0
#define ION_GPU_FW_MEM_SIZE 0
#endif
#endif

#define ION_GPU_SRAM_MEM_OFFSET (ION_GPU_FW_MEM_START + ION_GPU_FW_MEM_SIZE)

#ifndef ION_GPU_SRAM_MEM_BASE
#ifdef CONFIG_MACH_MT3612_A0
#define ION_GPU_SRAM_MEM_BASE (0x9000000 + ION_GPU_SRAM_MEM_OFFSET)
#else
#define ION_GPU_SRAM_MEM_BASE (0x0 + ION_GPU_SRAM_MEM_OFFSET)
#endif
#endif

#ifndef ION_GPU_SRAM_MEM_SIZE
#ifdef CONFIG_MACH_MT3612_A0
#define ION_GPU_SRAM_MEM_SIZE ((6 * 1024 * 1024) - (ION_GPU_SRAM_MEM_OFFSET))
#else
#define ION_GPU_SRAM_MEM_SIZE 0
#endif
#endif

long
mtk_ion_ioctl(struct ion_client *client, unsigned int cmd, unsigned long arg)
{
	return 0;
}

struct ion_heap *ion_mtk_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_heap *heap = NULL;

	heap = ion_heap_create(heap_data);
	if (IS_ERR_OR_NULL(heap)) {
		pr_err("%s: error creating heap %s type %d base %lu size %zu\n",
		       __func__, heap_data->name, heap_data->type,
		       heap_data->base, heap_data->size);
		return ERR_PTR(-EINVAL);
	}

	heap->name = heap_data->name;
	heap->id = heap_data->id;
	return heap;
}

int mtk_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	int err;
	int i;
	/*
	*unsigned int addr = 0;
	*unsigned int ionsize = 0;
	*/
	num_heaps = pdata->nr;

	mtkionheaps =
	    kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);

	pr_info("ion_device_create  num_heaps=%d\n", num_heaps);

	g_ion_device = ion_device_create(&mtk_ion_ioctl);
	if (IS_ERR_OR_NULL(g_ion_device)) {
		kfree(mtkionheaps);
		return PTR_ERR(g_ion_device);
	}

	/* create the heaps as specified in the board file */
	for (i = 0; i < num_heaps; i++) {
		struct ion_platform_heap *heap_data = &pdata->heaps[i];

		if (heap_data->type == ION_HEAP_TYPE_CARVEOUT) {
			if (heap_data->size == 0) {
				heap_data->base = 0;
				continue;
			}
		}

		if (heap_data->type == ION_HEAP_TYPE_GPU_SRAM) {
			if (heap_data->size == 0) {
				heap_data->base = 0;
				continue;
			}
		}

		if (heap_data->type == ION_HEAP_TYPE_DMA)
			heap_data->priv = (void *)g_ion_device->dev.this_device;

		mtkionheaps[i] = ion_mtk_heap_create(heap_data);

		if (IS_ERR_OR_NULL(mtkionheaps[i])) {
			err = PTR_ERR(mtkionheaps[i]);
			goto err;
		}
		ion_device_add_heap(g_ion_device, mtkionheaps[i]);
	}
	platform_set_drvdata(pdev, g_ion_device);
	g_ion_device->dev.this_device->archdata.dma_ops = NULL;
	arch_setup_dma_ops(g_ion_device->dev.this_device, 0, 0, NULL, false);
	return 0;
err:
	for (i = 0; i < num_heaps; i++) {
		if (mtkionheaps[i])
			ion_heap_destroy(mtkionheaps[i]);
	}
	kfree(mtkionheaps);
	return err;
}

int mtk_ion_remove(struct platform_device *pdev)
{
	struct ion_device *idev = platform_get_drvdata(pdev);
	int i;

	ion_device_destroy(idev);
	for (i = 0; i < num_heaps; i++)
		ion_heap_destroy(mtkionheaps[i]);
	kfree(mtkionheaps);
	return 0;
}

struct ion_device *get_ion_device(void)
{
	return g_ion_device;
}

int mtk_ion_is_gpu_sram(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer;
	int ret;

	ret = ion_check_dma_buf(dmabuf);
	if (ret) return ret;

	buffer = dmabuf->priv;

	return (buffer->heap->type == ION_HEAP_TYPE_GPU_SRAM);
}
EXPORT_SYMBOL(mtk_ion_is_gpu_sram);

static struct platform_driver ion_driver = {
	.probe = mtk_ion_probe,
	.remove = mtk_ion_remove,
	.driver = {.name = "ion-mtk"}
};

static struct ion_platform_heap gs_heap[] = {
	{
	 .type = ION_HEAP_TYPE_SYSTEM,
	 .name = "system",
	 .id = ION_HEAP_TYPE_SYSTEM,
	 },
	{
	 .type = ION_HEAP_TYPE_SYSTEM_CONTIG,
	 .name = "system_contig",
	 .id = ION_HEAP_TYPE_SYSTEM_CONTIG,
	 },
	 {
	 .type = ION_HEAP_TYPE_DMA,
	 .name = "dma",
	 .id = ION_HEAP_TYPE_DMA,
	 },
	 {
	 .type = ION_HEAP_TYPE_CARVEOUT,
	 .name = "carveout",
	 .id   = ION_HEAP_TYPE_CARVEOUT,
	 .base = ION_CARVEOUT_MEM_BASE,
	 .size = ION_CARVEOUT_MEM_SIZE,
	 },
	 {
	 .type = ION_HEAP_TYPE_GPU_SRAM,
	 .name = "gpu_sram",
	 .id   = ION_HEAP_TYPE_GPU_SRAM,
	 .base = ION_GPU_SRAM_MEM_BASE,
	 .size = ION_GPU_SRAM_MEM_SIZE,
	 }
};

static struct ion_platform_data gs_generic_config = {
	.nr = ARRAY_SIZE(gs_heap),
	.heaps = gs_heap,
};

static struct platform_device gs_ion_device = {
	.name = "ion-mtk",
	.id = 0,
	.num_resources = 0,
	.dev = {
		.platform_data = &gs_generic_config,
		}
};

static int __init ion_init(void)
{
	int ret;

	ret = platform_driver_register(&ion_driver);

	if (!ret) {
		ret = platform_device_register(&gs_ion_device);
		if (ret)
			platform_driver_unregister(&ion_driver);
	}
	return ret;
}

static void __exit ion_exit(void)
{
	platform_driver_unregister(&ion_driver);
	platform_device_unregister(&gs_ion_device);
}

module_init(ion_init);
module_exit(ion_exit);
