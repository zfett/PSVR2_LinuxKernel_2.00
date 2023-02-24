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

#include <linux/of_platform.h>
#include <linux/slab.h>

#include "mtk_wrapper_util.h"

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
		pr_err("[cv-common]sg_alloc_table new table failed!\n");
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sg(table->sgl, sg, table->nents, i) {
		memcpy(new_sg, sg, sizeof(*sg));
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static int check_sg(struct sg_table *sg, int nents)
{
	struct scatterlist *s;
	unsigned int i;
	dma_addr_t expected;

	expected = sg_dma_address(sg->sgl);
	for_each_sg(sg->sgl, s, nents, i) {
		if (sg_dma_address(s) != expected) {
			pr_err("[cv-common]check_sg table failed!\n");
			return -EINVAL;
		}

		expected = sg_dma_address(s) + sg_dma_len(s);
	}

	return 0;
}

static int mtk_wrapper_release_dma_info(struct device *dev, struct mtk_wrapper_dma_info *dma)
{
	dma_unmap_sg(dev, dma->sg->sgl, dma->sg->orig_nents, DMA_BIDIRECTIONAL);
	sg_free_table(dma->sg);
	kfree(dma->sg);
	dma_buf_unmap_attachment(dma->attach, dma->orig_sg, DMA_BIDIRECTIONAL);
	dma_buf_detach(dma->dma_buf, dma->attach);
	dma_buf_put(dma->dma_buf);
	dma->attach = NULL;
	return 0;
}

int mtk_wrapper_ionfd_to_dmainfo(struct device *dev, int fd, struct mtk_wrapper_dma_info *dma)
{
	struct dma_buf_attachment *attach;
	struct sg_table *orig_sg;
	struct sg_table *sg;
	int ret, nents;

	struct dma_buf *dma_buf;

	if (fd == dma->ionfd) {
		/*already mapped.*/
		return 0;
	}

	if (dma->attach) {
		/* release prev buffer */
		mtk_wrapper_release_dma_info(dev, dma);
	}

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf)) {
		return PTR_ERR(dma_buf);
	}

	attach = dma_buf_attach(dma_buf, dev);
	if (IS_ERR(attach)) {
		pr_err("map handle buf attach failed!\n");
		return PTR_ERR(attach);
	}

	orig_sg = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(orig_sg)) {
		ret = PTR_ERR(orig_sg);
		pr_err("map handle dma_buf_map_attachment failed!\n");
		goto fail_detach;
	}

	sg = dup_sg_table(orig_sg);
	if (IS_ERR(sg)) {
		ret = PTR_ERR(sg);
		pr_err("map handle dup_sg_table failed!\n");
		goto fail_unmap;
	}

	nents = dma_map_sg(dev, sg->sgl, sg->orig_nents, DMA_BIDIRECTIONAL);
	if (nents <= 0) {
		pr_err("dma_map_sg fail\n");
		ret = nents;
		goto fail_free_sg;
	}

	ret = check_sg(sg, nents);
	if (ret) {
		pr_err("sg_table is not contiguous");
		goto fail_unmap_sg;
	}

	dma->dma_buf = dma_buf;
	dma->attach = attach;
	dma->orig_sg = orig_sg;
	dma->sg = sg;
	dma->ionfd  = fd;
	dma->dma_addr = sg_dma_address(sg->sgl);

	return 0;

fail_unmap_sg:
	dma_unmap_sg(dev, sg->sgl, sg->orig_nents, DMA_BIDIRECTIONAL);
fail_free_sg:
	sg_free_table(sg);
	kfree(sg);
fail_unmap:
	dma_buf_unmap_attachment(attach, orig_sg, DMA_BIDIRECTIONAL);
fail_detach:
	dma_buf_detach(dma_buf, attach);

	return ret;
}

struct platform_device *pdev_find_dt(const char *compatible)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, compatible);
	if (!node) {
		pr_err("[%s] can't find %s\n", __func__, compatible);
		return NULL;
	}
	pdev = of_find_device_by_node(node);
	if (!pdev) {
		pr_err("Failed to get %s pdev\n", compatible);
		return NULL;
	}
	return pdev;
}

int mtk_wrapper_display_pipe_probe_pdev(const char *compatible, int idx, struct platform_device **platform_dev)
{
	static struct platform_device *s_disp_pipe;
	struct device_node *node;
	struct platform_device *pdev;

	if (!s_disp_pipe) {
		s_disp_pipe = pdev_find_dt("sie,disp-pipe");
		if (!s_disp_pipe) {
			return -EFAULT;
		}
	}

	node = of_parse_phandle(s_disp_pipe->dev.of_node, compatible, idx);
	if (!node) {
		return -EINVAL;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		return -EINVAL;
	}

	*platform_dev = pdev;

	return 0;
}

int mtk_wrapper_display_pipe_probe(const char *compatible, int idx, struct device **dev)
{
	struct platform_device *pdev = NULL;
	int ret;

	ret = mtk_wrapper_display_pipe_probe_pdev(compatible, idx, &pdev);
	if (ret < 0) {
		return ret;
	}

	*dev = &pdev->dev;
	return ret;
}
