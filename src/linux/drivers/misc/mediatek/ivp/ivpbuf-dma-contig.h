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

#ifndef __IVPBUF_DMA_CONTIG_H__
#define __IVPBUF_DMA_CONTIG_H__

#include <linux/dma-mapping.h>

#include "ivpbuf-core.h"

void *ivp_dma_contig_init_ctx(struct device *dev);
void ivp_dma_contig_cleanup_ctx(void *alloc_ctx);

extern const struct ivp_mem_ops ivp_dma_contig_memops;

#endif
