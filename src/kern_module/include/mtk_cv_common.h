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

#ifndef MTK_CV_COMMON_H
#define MTK_CV_COMMON_H

#ifdef __KERNEL__
#include <linux/dma-buf.h>
#include <linux/module.h>
#endif

struct mtk_cv_common_set_buf {
	int handle;
	__u32 buf_type;
	__u32 pitch;
	__u32 offset;
	__u32 format;
};

struct mtk_cv_common_buf_handle {
	int handle;
	__u32 buf_type;
	__s32 fd;
	__u32 offset;
};

struct mtk_cv_common_put_handle {
	int handle;
	__u32 buf_type;
};

enum mtk_cv_common_sync_direction {
	MTK_CV_COMMON_SYNC_BOTH,
	MTK_CV_COMMON_SYNC_FOR_DEVICE,
	MTK_CV_COMMON_SYNC_FOR_CPU,
};

struct mtk_cv_common_sync_handle {
	int handle;
	enum mtk_cv_common_sync_direction dir;
};

#ifdef __KERNEL__
struct mtk_cv_common_buf {
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *import_attach;
	void *cookie;
	void *kvaddr;
	struct device *dev;
	dma_addr_t dma_addr;
	u32 offset;
	unsigned long dma_attrs;
	struct sg_table *orig_sg;
	struct sg_table *sg;
	u32 pitch;
	u32 format;
	u32 is_sysram;
};

struct mtk_cv_common_buf *mtk_cv_common_get_buf_from_handle(int handle);
int mtk_cv_common_fd_to_handle(struct device *dev, int fd, int *handle);
int mtk_cv_common_fd_to_handle_offset(struct device *dev, int fd, int *handle, u32 offset);
int mtk_cv_common_put_handle(int handle);
int mtk_cv_common_sync_handle(int handle, enum mtk_cv_common_sync_direction dir);
void *pa_to_vaddr(struct mtk_cv_common_buf *buftova, int size, bool enable);
#endif
#endif /*MTK_CV_COMMON_H*/
