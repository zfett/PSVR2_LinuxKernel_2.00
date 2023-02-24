/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	Hua Yu <hua.yu@mediatek.com>
 *
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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

#include <linux/dma-iommu.h>
#include <linux/idr.h>
#include "mtk_cv_common.h"

int mtk_ion_is_gpu_sram(struct dma_buf *dma_buf);
#define SYSRAM_ADDR      0x0000000009000000ULL
#define SYSRAM_ADDR_MASK 0xFFFFFFFFFF000000ULL
#define IS_SYSRAM_ADDR(a) (((a) & SYSRAM_ADDR_MASK) == SYSRAM_ADDR)
#define SYSRAM_ADDR_TO_PHYS(a) ((a) & (~SYSRAM_ADDR_MASK))

static DEFINE_IDR(cv_common_idr);

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

struct mtk_cv_common_buf *mtk_cv_common_get_buf_from_handle(int handle)
{
	struct mtk_cv_common_buf *buf;

	buf = idr_find(&cv_common_idr, handle);
	if (!buf) {
		pr_err("%s: Can't find buf from handle:%d\n", __func__, handle);
		return NULL;
	}
	return buf;
}

static int mtk_cv_common_map_handle(struct device *dev, int handle)
{
	struct dma_buf_attachment *attach;
	struct sg_table *orig_sg;
	struct sg_table *sg = NULL;
	struct mtk_cv_common_buf *buf;
	int ret = -1;
	int nents;

	pr_debug("[%s]\n", __func__);
	if (!dev) {
		dev_err(dev, "map handle device null!\n");
		return -EINVAL;
	}

	buf = idr_find(&cv_common_idr, handle);
	if (!buf) {
		dev_err(dev, "can't find buf from handle:%d\n", handle);
		return -EINVAL;
	}

	if (buf->dev) {
		dev_err(dev, "%s: map handle buf device should be null! handle:%d buf->dev:%p\n", __func__, handle, buf->dev);
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

	ret = mtk_ion_is_gpu_sram(buf->dma_buf);
	if (ret < 0) {
		dev_err(dev, "map handle mtk_ion_is_gpu_sram failed!\n");
		goto fail_unmap;
	}
	buf->is_sysram = ret;

	if (buf->is_sysram == 0) {
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

		buf->dma_addr = sg_dma_address(sg->sgl) + buf->offset;

	} else {
		buf->dma_addr = sg_dma_address(orig_sg->sgl) + buf->offset;
		dev_dbg(dev, "SYSRAM: buf->dma_addr = %p\n", (void *)buf->dma_addr);

		if (IS_SYSRAM_ADDR(buf->dma_addr)) {
			buf->dma_addr = SYSRAM_ADDR_TO_PHYS(buf->dma_addr);
		} else {
			ret = -1;
			buf->dma_addr = 0;
			goto fail_unmap;
		}
	}

	buf->import_attach = attach;
	buf->orig_sg = orig_sg;
	buf->sg = sg;

	buf->dev = dev;

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

int mtk_cv_common_fd_to_handle_offset(struct device *dev, int fd, int *handle, u32 offset)
{
	struct dma_buf *dma_buf;
	struct mtk_cv_common_buf *buf;
	int ret;

	pr_debug("[%s]\n", __func__);
	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf)) {
		dev_err(dev, "dma_buf_get error: %d\n", fd);
		return PTR_ERR(dma_buf);
	}

	dev_dbg(dev, "get dmf_buf success\n");

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		dev_err(dev, "fd to handle kzalloc failed! %d\n", ret);
		goto out_put;
	}

	buf->dma_buf = dma_buf;
	buf->offset = offset;

	*handle = idr_alloc(&cv_common_idr, buf, 1, 0, GFP_KERNEL);
	if (*handle < 0) {
		dev_err(dev, "Can't get handle to fail idr_alloc: %d\n", *handle);
		ret = *handle;
		goto out_free;
	}

	if (dev) {
		ret = mtk_cv_common_map_handle(dev, *handle);
		if (ret) {
			dev_err(dev, "fd to handle mtk_cv_common_map_handle failed! %d\n", ret);
			goto out_free;
		}
	}

	dev_dbg(dev, " mtk_cv_common_fd_to_handle handle (mtk_cv_common_buf) = %d\n", *handle);
	dev_dbg(dev, " mtk_cv_common_fd_to_handle buf->dma_addr = %p\n", (void *)buf->dma_addr);
	dev_dbg(dev, " mtk_cv_common_fd_to_handle buf->kvaddr = %p\n", (void *)buf->kvaddr);
	dev_dbg(dev, "get buf success\n");
	return 0;

out_free:
	kfree(buf);
out_put:
	dma_buf_put(dma_buf);
	return ret;
}

int mtk_cv_common_fd_to_handle(struct device *dev, int fd, int *handle)
{
	return mtk_cv_common_fd_to_handle_offset(dev, fd, handle, 0);
}

int mtk_cv_common_put_handle(int handle)
{
	struct mtk_cv_common_buf *buf;

	buf = idr_find(&cv_common_idr, handle);
	if (!buf) {
		pr_err("Can't find buf from handle:%d\n", handle);
		return -EINVAL;
	}

	if (buf->dev) {
		if (buf->kvaddr) {
			pa_to_vaddr(buf, 0, false);
		}
		if (buf->sg) {
			dma_unmap_sg(buf->dev, buf->sg->sgl, buf->sg->orig_nents, DMA_BIDIRECTIONAL);
			sg_free_table(buf->sg);
			kfree(buf->sg);
		}
		dma_buf_unmap_attachment(buf->import_attach, buf->orig_sg, DMA_BIDIRECTIONAL);
		dma_buf_detach(buf->dma_buf, buf->import_attach);
	}

	dma_buf_put(buf->dma_buf);
	idr_remove(&cv_common_idr, handle);
	kfree(buf);

	return 0;
}

int mtk_cv_common_sync_handle(int handle, enum mtk_cv_common_sync_direction dir)
{
	struct mtk_cv_common_buf *buf;

	buf = idr_find(&cv_common_idr, handle);
	if (!buf) {
		dev_err(NULL, "%s: buf is null\n", __func__);
		return -EINVAL;
	}

	if (buf->is_sysram)
		return 0;

	if (!buf->dev) {
		dev_err(buf->dev, "%s: buf->dev is null\n", __func__);
		return -EINVAL;
	}

	if (!buf->sg) {
		dev_err(buf->dev, "buf->sg is null.\n");
		return -EINVAL;
	}

	if (dir == MTK_CV_COMMON_SYNC_FOR_DEVICE || dir == MTK_CV_COMMON_SYNC_BOTH) {
		dma_sync_sg_for_device(buf->dev, buf->sg->sgl, buf->sg->nents, DMA_TO_DEVICE);
	}

	if (dir == MTK_CV_COMMON_SYNC_FOR_CPU || dir == MTK_CV_COMMON_SYNC_BOTH) {
		dma_sync_sg_for_cpu(buf->dev, buf->sg->sgl, buf->sg->nents, DMA_FROM_DEVICE);
	}

	return 0;
}

void *pa_to_vaddr(struct mtk_cv_common_buf *buftova, int size, bool enable)
{
	int ret;

	pr_debug("buf->kvaddr1 = %p\n",  (void *)buftova->kvaddr);
	pr_debug("buftova->dma_addr1 = %p\n",  (void *)buftova->dma_addr);

	if (enable) {
		if (buftova->import_attach) {
		ret = dma_buf_begin_cpu_access(
			buftova->import_attach->dmabuf, 0, size, 0);
		if (ret)
			pr_err("dma_buf_begin_cpu_access failed! %d\n", ret);
		if (!buftova->kvaddr)
			buftova->kvaddr =
				dma_buf_vmap(buftova->import_attach->dmabuf);
		}
	} else {
		dma_buf_end_cpu_access(
			buftova->import_attach->dmabuf, 0, size, 0);
		if (buftova->kvaddr) {
			dma_buf_vunmap(
			buftova->import_attach->dmabuf, buftova->kvaddr);
			buftova->kvaddr = NULL;
		}
	}
	pr_debug("buf->kvaddr2 = %p\n",  (void *)buftova->kvaddr);
	pr_debug("buftova->dma_addr2 = %p\n",  (void *)buftova->dma_addr);
	return buftova->kvaddr;
}

MODULE_LICENSE("GPL");
