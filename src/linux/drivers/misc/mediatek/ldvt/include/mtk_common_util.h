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

#ifndef MTK_COMMON_UTIL_H
#define MTK_COMMON_UTIL_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <uapi/mediatek/mtk_vr_tracking_common.h>
#include <linux/mailbox/mtk-cmdq-mailbox.h>
#include <soc/mediatek/mtk_mm_sysram.h>

/** ivp related data structure **/
enum mtkivp_req_status {
	MTKIVP_REQ_STATUS_NONE,
	MTKIVP_REQ_STATUS_DONE,
	MTKIVP_REQ_STATUS_BUSY,
	MTKIVP_REQ_STATUS_ERROR,
};

struct mtkivp_request {
	uint64_t handle;
	uint8_t	cmd;
	uint8_t	coreid;
	enum mtkivp_req_status status;

	struct {
		uint32_t flag;
		uint32_t size;
		uint32_t addr_handle;
	} in;

	struct {
		uint32_t result;
		uint64_t algo_handle;
	} out;
};

struct mtk_common_buf {
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
};

/*ion handle mapping*/
int mtk_common_map_handle(struct device *dev, u64 handle);
int mtk_common_fd_to_handle(struct device *dev, int fd, u64 *handle);
int mtk_common_fd_to_handle_offset(struct device *dev, int fd, u64 *handle,
	u32 offset);
int mtk_common_put_handle(u64 handle);

/*doing pa to va for cpu access data*/
void *pa_to_vaddr(struct mtk_common_buf *buftova, int size, bool enable);
void *pa_to_vaddr_dev(struct device *dev, struct mtk_common_buf *buftova,
	int size, bool enable);

/* doing dram va to and from sram dev by apb interface */
int move_va_to_sram(struct device *dev, void *va, u32 pa, int size);
int move_sram_to_va(struct device *dev, u32 pa, void *va, int size);

/*save input char stream 'char *file'  to 'char *file_path'*/
void mtk_common_file_path(char *file_path, char *file);
/*read input path file 'char *file_path' output to  'void *va' */
void mtk_common_read_file(void *va, char *file_path, u32 size);
/*write input path file 'char *file_path'  with data in 'void *va' */
void mtk_common_write_file(void *va, char *file_path, u32 size);
/*read input path  image 'char *file_path' output to  'void *va' */
void mtk_common_read_img(void *va,
	char *file_path, u32 file_offset,
	u32 line_size, u32 w, u32 h, u32 pitch);
/*write input path image  'char *file_path'  with data in 'void *va' */
void mtk_common_write_img(void *va,
	char *file_path, u32 file_offset,
	u32 line_size, u32 w, u32 h, u32 pitch);
/*read input path  L+R image 'char *file_path' output to  'void *va' */
void mtk_common_read_img_lr(void *l_va,
	void *r_va, char *file_path,
	u32 line_size, u32 h, u32 pitch);

/*doing bit true compare for golden file compare*/
int mtk_common_compare(char *a_va, char *b_va, u32 size);
int mtk_common_compare_img(char *a_va, char *b_va, u32 line_size,
	u32 h, u32 pitch);
#endif /*MTK_COMMON_UTIL_H*/
