/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#pragma once
#include <linux/module.h>
#include <linux/dma-buf.h>

struct mtk_wrapper_dma_info {
	int ionfd;
	struct dma_buf *dma_buf;
	dma_addr_t dma_addr;
	struct dma_buf_attachment *attach;
	struct sg_table *orig_sg;
	struct sg_table *sg;
};

struct platform_device *pdev_find_dt(const char *compatible);
int mtk_wrapper_ionfd_to_dmainfo(struct device *dev, int fd, struct mtk_wrapper_dma_info *dma);
int mtk_wrapper_display_pipe_probe(const char *compatible, int idx, struct device **dev);
int mtk_wrapper_display_pipe_probe_pdev(const char *compatible, int idx, struct platform_device **dev);
