/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Hua Yu <hua.yu@mediatek.com>
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

#include <linux/dma-iommu.h>
#include <mtk_common_util.h>

#define DBG_MODE 0

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

int mtk_common_map_handle(struct device *dev, u64 handle)
{
	struct dma_buf_attachment *attach;
	struct sg_table *orig_sg;
	struct sg_table *sg;
	struct mtk_common_buf *buf = (struct mtk_common_buf *)handle;
	int ret, nents;

	if (!dev) {
		dev_err(dev, "map handle device null!\n");
		return -EINVAL;
	}

	if (buf->dev) {
		dev_err(dev, "map handle buf device null!\n");
		return -EINVAL;
	}

	attach = dma_buf_attach(buf->dma_buf, dev);
	if (IS_ERR(attach)) {
		dev_err(dev, "map handle buf attach failed!\n");
		return PTR_ERR(attach);
	}

	orig_sg = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(orig_sg)) {
		ret = PTR_ERR(orig_sg);
		dev_err(dev, "map handle dma_buf_map_attachment failed!\n");
		goto fail_detach;
	}

	sg = dup_sg_table(orig_sg);
	if (IS_ERR(sg)) {
		ret = PTR_ERR(sg);
		dev_err(dev, "map handle dup_sg_table failed!\n");
		goto fail_unmap;
	}

	nents = dma_map_sg(dev, sg->sgl, sg->orig_nents, DMA_BIDIRECTIONAL);
	if (nents <= 0) {
		dev_err(dev, "dma_map_sg fail\n");
		ret = nents;
		goto fail_free_sg;
	}

	ret = check_sg(sg, nents);
	if (ret) {
		dev_err(dev, "sg_table is not contiguous");
		goto fail_unmap_sg;
	}

	buf->import_attach = attach;
	buf->orig_sg = orig_sg;
	buf->sg = sg;

	buf->dev = dev;
	buf->dma_addr = sg_dma_address(sg->sgl) + buf->offset;

	return 0;

fail_unmap_sg:
	dma_unmap_sg(dev, sg->sgl, sg->orig_nents, DMA_BIDIRECTIONAL);
fail_free_sg:
	sg_free_table(sg);
	kfree(sg);
fail_unmap:
	dma_buf_unmap_attachment(attach, orig_sg, DMA_BIDIRECTIONAL);
fail_detach:
	dma_buf_detach(buf->dma_buf, attach);

	return ret;
}
EXPORT_SYMBOL(mtk_common_map_handle);

int mtk_common_fd_to_handle_offset(struct device *dev, int fd, u64 *handle,
	u32 offset)
{
	struct dma_buf *dma_buf;
	struct mtk_common_buf *buf;
	int ret;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);
#if DBG_MODE
	dev_info(dev, "get dmf_buf success\n");
#endif
	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		dev_err(dev, "fd to handle kzalloc failed! %d\n", ret);
		goto out_put;
	}

	buf->dma_buf = dma_buf;
	buf->offset = offset;

	if (dev) {
		ret = mtk_common_map_handle(dev, (u64)buf);
		if (ret) {
			dev_err(dev,
			"fd to handle mtk_common_map_handle failed! %d\n",
			ret);
			goto out_free;
		}
	}

	*handle = (u64)buf;
#if DBG_MODE
	dev_info(dev, " mtk_common_fd_to_handle buf->dma_addr = %pad\n",
		 &buf->dma_addr);
	dev_info(dev, " mtk_common_fd_to_handle buf->kvaddr = %p\n",
		 (void *)buf->kvaddr);
	dev_info(dev, "get buf success\n");
#endif
	return 0;

out_free:
	kfree(buf);
out_put:
	dma_buf_put(dma_buf);
	return ret;
}
EXPORT_SYMBOL(mtk_common_fd_to_handle_offset);

int mtk_common_fd_to_handle(struct device *dev, int fd,
	u64 *handle)
{
	return mtk_common_fd_to_handle_offset(dev, fd, handle, 0);
}
EXPORT_SYMBOL(mtk_common_fd_to_handle);

int mtk_common_put_handle(u64 handle)
{
	struct mtk_common_buf *buf = (struct mtk_common_buf *)handle;

	if (!handle) {
		pr_err("mtk_common_put_handle: invalid handle!\n");
		return -EINVAL;
	}

	if (buf->dev) {
		dma_unmap_sg(buf->dev, buf->sg->sgl, buf->sg->orig_nents,
			DMA_BIDIRECTIONAL);
		sg_free_table(buf->sg);
		kfree(buf->sg);

		dma_buf_unmap_attachment(buf->import_attach, buf->orig_sg,
			DMA_BIDIRECTIONAL);
		dma_buf_detach(buf->dma_buf, buf->import_attach);
	}

	dma_buf_put(buf->dma_buf);
	kfree(buf);
	buf = NULL;
	return 0;
}
EXPORT_SYMBOL(mtk_common_put_handle);

void *pa_to_vaddr_dev(struct device *dev, struct mtk_common_buf *buftova,
	int size, bool enable)
{
	void *vaddr;

	vaddr = pa_to_vaddr(buftova, size, enable);

	if (enable) {
		dma_sync_sg_for_cpu(dev, buftova->sg->sgl,
			buftova->sg->nents, DMA_FROM_DEVICE);
		if (!vaddr)
		pr_err("pa=%pad:pa_to_vaddr failed!, return null va\n",
			&buftova->dma_addr);
	}

	return vaddr;
}
EXPORT_SYMBOL(pa_to_vaddr_dev);

void *pa_to_vaddr(struct mtk_common_buf *buftova, int size, bool enable)
{
	int ret;
#if DBG_MODE
	pr_info("buf->kvaddr1 = %p\n", (void *)buftova->kvaddr);
	pr_info("buftova->dma_addr1 = %pad\n", &buftova->dma_addr);
#endif
	if (enable) {
		if (buftova->import_attach) {
			ret = dma_buf_begin_cpu_access(
				buftova->import_attach->dmabuf, 0, size, 0);
			if (ret)
				pr_err("dma_buf_begin_cpu_access failed! %d\n",
					ret);
			if (!buftova->kvaddr) {
				buftova->kvaddr = dma_buf_vmap(
					buftova->import_attach->dmabuf);
				if (!buftova->kvaddr)
					pr_err("buffer map vaddr failed!\n");
			}
		} else {
			pr_err("pa to va: import_attach == NULL!!\n");
		}
	} else {
		dma_buf_end_cpu_access(
			buftova->import_attach->dmabuf, 0, size, 0);
		if (buftova->kvaddr) {
			dma_buf_vunmap(
			buftova->import_attach->dmabuf, buftova->kvaddr);
			buftova->kvaddr = NULL;
		} else {
			pr_err("pa to va: kvaddr == NULL before vunmap!!\n");
		}
	}
#if DBG_MODE
	pr_info("buf->kvaddr2 = %p\n", (void *)buftova->kvaddr);
	pr_info("buftova->dma_addr2 = %pad\n", &buftova->dma_addr);
#endif
	return buftova->kvaddr;
}
EXPORT_SYMBOL(pa_to_vaddr);

int move_va_to_sram(struct device *dev, void *va, u32 pa, int size)
{
	int i, j, ret = 0, aligned_size;
	u32 *dram = (u32 *)va;
	u32 tmp[4];

	if (va == NULL) {
		pr_err("move_va_to_sram: va is NULL\n");
		return -EINVAL;
	}

	size /= 4; /*use u32 as pointer, size need to /4*/
	aligned_size = roundup(size, 4);
#if DBG_MODE
	pr_debug("aligned size: %d\n", aligned_size);
#endif

	for (i = 0; i < aligned_size; i += 4) {
#if DBG_MODE
		pr_debug("start to fill %x:\n", pa + i * 4);
#endif
		for (j = 0; j < 4; j++)
			tmp[j] = i + j > size ? 0 : dram[i + j];

		ret = mtk_mm_sysram_fill_128b(dev, pa + i * 4, 16, tmp);
#if DBG_MODE
		pr_debug("%d filled\n", ret);
#endif
		if (ret < 0) {
			dev_err(dev, "sram fill fail @%d, ret = %d\n", i, ret);
			return ret;
		}
	}

	return ret;
}
EXPORT_SYMBOL(move_va_to_sram);

int move_sram_to_va(struct device *dev, u32 pa, void *va, int size)
{
	int i, j, ret = 0, aligned_size;
	u32 *dram = (u32 *)va;
	u32 tmp[4];

	if (va == NULL) {
		pr_err("move_sram_to_va: va is NULL\n");
		return -EINVAL;
	}

	size /= 4; /*use u32 as pointer, size need to / 4*/
	aligned_size = roundup(size, 4);
#if DBG_MODE
	pr_debug("aligned size: %d\n", aligned_size);
#endif
	for (i = 0; i < aligned_size; i += 4) {
#if DBG_MODE
		pr_debug("start to read %d:\n", pa + i);
#endif
		ret = mtk_mm_sysram_read(dev, pa + i * 4, tmp);
		if (ret) {
			dev_err(dev, "sram read fail @%d, ret =%d\n", i, ret);
			return ret;
		}
		for (j = 0; j < 4; j++)
			if (i + j < size)
				dram[i + j] = tmp[j];
	}

	return ret;
}
EXPORT_SYMBOL(move_sram_to_va);


/*------------------------USB dump in/dump out--------------------------*/

/*save input char stream 'char *file'  to 'char *file_path'*/
void mtk_common_file_path(char *file_path, char *file)
{
	u32 i, len = 0;

	for (i = 0; i < strlen(file); i++, len++)
		if (file[i] == ':')
			break;

	strncpy(file_path, file, len);
	file_path[len] = '\0';
#if DBG_MODE
	pr_info("mtk_common_file_path: '%s'\n", file_path);
#endif
}
EXPORT_SYMBOL(mtk_common_file_path);

/*read input path file 'char *file_path' output to  'void *va' */
void mtk_common_read_file(void *va, char *file_path, u32 size)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos = 0;
#if DBG_MODE
	pr_info("mtk_common_read_file: %s\n", file_path);
#endif
	oldfs = get_fs();
	set_fs(get_ds());

	if (va == NULL) {
		pr_err("mtk_common_read_file: va is NULL\n");
		goto out;
	}

	fp = filp_open(file_path, O_RDONLY, 0644);
	if (IS_ERR(fp)) {
	pr_err("mtk_common_read_file: open file %s fail!\n", file_path);
		goto out;
	}

	vfs_read(fp, va, size, &pos);
	filp_close(fp, 0);
#if DBG_MODE
	pr_info("mtk_common_read_file: open file %s success!\n", file_path);
#endif
out:
	set_fs(oldfs);
}
EXPORT_SYMBOL(mtk_common_read_file);

/*write input path file 'char *file_path'  with data in 'void *va' */
void mtk_common_write_file(void *va, char *file_path, u32 size)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos = 0;
#if DBG_MODE
	pr_info("mtk_common_write_file: %s\n", file_path);
#endif

	oldfs = get_fs();
	set_fs(get_ds());

	if (va == NULL) {
		pr_err("mtk_common_write_file: va is NULL\n");
		goto out;
	}

	fp = filp_open(file_path, O_WRONLY | O_CREAT, 0644);
	if (IS_ERR(fp)) {
	pr_err("mtk_common_write_file: open file %s fail!\n", file_path);
		goto out;
	}

	vfs_write(fp, va, size, &pos);
	filp_close(fp, 0);
#if DBG_MODE
	pr_info("mtk_common_write_file: open file %s success!\n", file_path);
#endif
out:
	set_fs(oldfs);
}
EXPORT_SYMBOL(mtk_common_write_file);

/*read input path  image 'char *file_path' output to  'void *va' */
void mtk_common_read_img(void *va,
	char *file_path, u32 file_offset,
	u32 line_size, u32 w, u32 h, u32 pitch)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos;
	loff_t i;

#if 0
	in_filp = filp_open(arg->input_file, O_RDONLY, 0);
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_read(in_filp, in_status.s_addr, arg->input_file_size,
		       &in_filp->f_pos);
	set_fs(oldfs);
#endif
#if DBG_MODE
	pr_info("mtk_common_read_img: %s,line_size:%d, w:%d, h:%d, pitch:%d\n",
		file_path, line_size, w, h, pitch);
#endif
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	if (va == NULL) {
		pr_err("mtk_common_read_img: va is NULL\n");
		goto out;
	}

	fp = filp_open(file_path, O_RDONLY, 0644);
	if (IS_ERR(fp)) {
	pr_err("mtk_common_read_img: open image %s fail!\n", file_path);
		goto out;
	}
	for (i = 0; i < h; i++) {
		pos = file_offset + line_size * i;
		vfs_read(fp, va + pitch * i, w, &pos);
	}
	filp_close(fp, 0);
#if DBG_MODE
	pr_info("mtk_common_read_img: open image %s success!\n", file_path);
#endif
out:
	set_fs(oldfs);
}
EXPORT_SYMBOL(mtk_common_read_img);


/*write input path image  'char *file_path'  with data in 'void *va' */
void mtk_common_write_img(void *va,
	char *file_path, u32 file_offset,
	u32 line_size, u32 w, u32 h, u32 pitch)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos;
	loff_t i;
#if DBG_MODE
	pr_info("mtk_common_write_img: %s\n", file_path);
#endif
	oldfs = get_fs();
	set_fs(get_ds());

	if (va == NULL) {
		pr_err("mtk_common_write_img: va is NULL\n");
		goto out;
	}

	fp = filp_open(file_path, O_WRONLY | O_CREAT, 0644);
	if (IS_ERR(fp)) {
	pr_err("mtk_common_write_img: open image %s fail!\n", file_path);
		goto out;
	}

	for (i = 0; i < h; i++) {
		pos = file_offset + line_size * i;
		vfs_write(fp, va + pitch * i, w, &pos);
	}
	filp_close(fp, 0);
#if DBG_MODE
	pr_info("mtk_common_write_file: open image %s success!\n",
		file_path);
#endif
out:
	set_fs(oldfs);
}
EXPORT_SYMBOL(mtk_common_write_img);

/*read input path  L+R image 'char *file_path' output to  'void *va' */
void mtk_common_read_img_lr(void *l_va,
	void *r_va, char *file_path,
	u32 line_size, u32 h, u32 pitch)
{
	struct file *fp;
	mm_segment_t oldfs;
	loff_t pos;
	loff_t i;
#if DBG_MODE
	pr_info("mtk_common_read_img_lr: %s\n", file_path);
#endif
	oldfs = get_fs();
	set_fs(get_ds());

	if (!l_va || !r_va) {
		pr_err("mtk_common_read_img_lr: va is NULL\n");
		goto out;
	}

	fp = filp_open(file_path, O_RDONLY, 0644);
	if (IS_ERR(fp)) {
	pr_err("mtk_common_read_img_lr: open file %s fail!\n", file_path);
		goto out;
	}

	for (i = 0; i < h; i++) {
		pos = line_size * i * 2;
		vfs_read(fp, l_va + pitch * i, line_size, &pos);
		pos = line_size * (i * 2 + 1);
		vfs_read(fp, r_va + pitch * i, line_size, &pos);
	}
	filp_close(fp, 0);
#if DBG_MODE
	pr_info("mtk_common_read_img_lr: open imaglr %s success!\n", file_path);
#endif
out:
	set_fs(oldfs);
}
EXPORT_SYMBOL(mtk_common_read_img_lr);

int mtk_common_compare(char *a_va, char *b_va, u32 size)
{
	u32 i;
	u32 err_cnt = 0;
	pr_err("mtk_common_compare file\n");

	if (!a_va || !b_va) {
		pr_err("mtk_common_compare fail: va is NULL\n");
		return -EIO;
	}
	for (i = 0; i < size; i++)
		if (a_va[i] != b_va[i]) {
			err_cnt++;
			pr_err("mtk_common_compare file fail: a_va[%d] = %d, b_va[%d] = %d\n",
					i, a_va[i], i, b_va[i]);
			if (err_cnt >= 20) {
				pr_err("mtk_common_compare file err_cnt over 20, fail!\n");
				goto out;
			}
	}
	if (err_cnt > 0)
		goto out;
	pr_err("mtk_common_compare file pass!\n");
	return 0;
out:
	pr_err("mtk_common_compare_img fail, err_cnt = %d\n", err_cnt);
	return -EIO;
}
EXPORT_SYMBOL(mtk_common_compare);

int mtk_common_compare_img(char *a_va,
	char *b_va,
	u32 line_size,
	u32 h,
	u32 pitch)
{
	u32 x, y;
	u32 err_cnt = 0;

	pr_err("mtk_common_compare_img\n");

	if (!a_va || !b_va) {
		pr_err("mtk_common_compare_img fail: va is NULL\n");
		return -EIO;
	}

	for (y = 0; y < h; y++) {
		for (x = 0; x < line_size; x++) {
			if (a_va[y * pitch + x] != b_va[y * pitch + x]) {
				err_cnt++;
				pr_err(
					"compare_img x = %d,y = %d,a_va[%d] = %d, b_va[%d] = %d\n",
					x, y, y * pitch + x,
					a_va[y * pitch + x], y * pitch + x,
					b_va[y * pitch + x]);
				if (err_cnt >= 20) {
					pr_err("mtk_common_compare_img err_cnt over 20, fail!\n");
					goto out;
				}
			}
		}
	}
	if (err_cnt > 0)
		goto out;
	pr_err("mtk_common_compare_img all pass!\n");
	return 0;
out:
	pr_err("mtk_common_compare_img fail, err_cnt = %d\n", err_cnt);
	return -EIO;
}
EXPORT_SYMBOL(mtk_common_compare_img);

MODULE_LICENSE("GPL");
