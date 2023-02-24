/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Jim.Wu <jim.wu@mediatek.com>, Ted.Yeh <ted.yeh@mediatek.com>
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

/**
 * @file mtk-dma.h
 * This header file is used for mt-cqdma.c & mt-apdma.c
 */


#ifndef _MTK_DMA_H_
#define _MTK_DMA_H_

/** @ingroup IP_group_cqdma_internal_struct
 * @brief MTK channel struct.
 */
struct mtk_chan {
	struct virt_dma_chan vc;
	struct list_head node;
	struct dma_slave_config cfg;
	struct mtk_desc *desc;
	void __iomem *channel_base;
	int dma_ch;
	int irq_number;
	char irq_name[20];
	struct tasklet_struct task;
	spinlock_t lock;
};

/** @ingroup IP_group_cqdma_internal_struct
 * @brief MTK dma control block struct.
 */
struct mtk_dma_cb {
	unsigned int info;
	dma_addr_t src;
	dma_addr_t dst;
	unsigned int length;
	unsigned int next;
};

/** @ingroup IP_group_cqdma_internal_struct
 * @brief MTK scatter gather struct.
 */
struct mtk_sg {
	struct mtk_dma_cb *cb;
	dma_addr_t cb_addr;
};

/** @ingroup IP_group_cqdma_internal_struct
 * @brief MTK descriptior struct.
 */
struct mtk_desc {
	struct mtk_chan *c;
	struct virt_dma_desc vd;
	enum dma_transfer_direction dir;
	unsigned int sg_num;
	unsigned int sg_idx;
	struct mtk_sg *sg;
};
#endif
