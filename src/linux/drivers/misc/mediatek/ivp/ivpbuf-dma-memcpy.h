/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: chiawen Lee <chiawen.lee@mediatek.com>
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

#ifndef _IVPBUF_DMA_MEMCPY_H
#define _IVPBUF_DMA_MEMCPY_H

#include <linux/dmaengine.h>

#define IVP_IOVA_DATARAM0 (0x7fe00000)
#define IVP_IOVA_DATARAM1 (0x7fe80000)
#define IVP_IOVA_INSTRAM  (0x7ff80000)

#define IVP_SIZE_DATARAM  (0x80000)

#define IVP_MCU_DATARAM(i) (0x20100000 + i * 0x200000)
#define IVP_MCU_INSTRAM(i) (0x20280000 + i * 0x200000)
#define IVP_SIZE_INSTRAM  (0x8000)

struct ivp_dma_memcpy_buf;
struct ivpbuf_dma_chan {
	struct dma_chan *dma_chan;
	struct completion tx_completion;
	struct device *dev;

	/* notify enque thread */
	wait_queue_head_t dma_req_wait;

	/* to enque/deque dma_request must have mutex protection */
	struct mutex dma_mutex;
	struct list_head dma_enque_list;
	struct list_head dma_deque_list;

	struct task_struct *dma_task;
};

enum ivp_memory_type_e {
	IVP_MEMTYPE_SYSTEM,
	IVP_MEMTYPE_DATARAM0,
	IVP_MEMTYPE_DATARAM1,
	IVP_MEMTYPE_INSTRAM,
};

struct ivp_mem_para {
	enum ivp_memory_type_e mem_type;
	int coreid;
	int start_byte_offset;

	s32 buffer_handle;
};

int  ivpdma_request_dma_channel(struct ivp_device *pdev);
void ivpdma_release_dma_channel(struct ivp_device *pdev);
int  ivpdma_submit_request(struct ivp_device *pdev,
			   struct ivp_dma_memcpy_buf *buf);
int  ivpdma_check_request_status(struct ivp_device *pdev, s32 handle);
#endif
