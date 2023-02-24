/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *      Peggy Jao <peggy.jao@mediatek.com>
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
 * @file mtk_sbrc_int.h
 * Internal header for mtk_sbrc.c
 */

#ifndef _MTK_SBRC_INT_H_
#define _MTK_SBRC_INT_H_

#ifdef CONFIG_MTK_DEBUGFS
#include <linux/debugfs.h>
#endif

/** @ingroup IP_group_sbrc_internal_def
 * @{
 */
#define SET_ON                  1
#define SET_OFF                 0
#define BUFFER_NUM              3
#define BUFFER_DEPTH            128
#define GPU_W_OVER_TH           128
#define GPIO_TIMEOUT            12800
#define GPU_FRAME_HEIGHT        2040
#define WRITE_OVER_READ_TH      128
#define THROTTLING_H            5
#define THROTTLING_L            5
#define GCE_EVENT_MASK          0x1BF
#define MUTEX_EVENT_MASK        0x16
#define IRQ_ENABLE_MASK         0x27
#define IRQ_CLR_MASK            0x3F0000
/** @} */

/** @ingroup IP_group_sbrc_internal_struct
 * @brief sbrc Driver Data Structure.
 */
struct mtk_sbrc {
	/** device node */
	struct device *dev;
#ifdef CONFIG_COMMON_CLK_MT3612
	/* clk */
	struct clk *sbrc_clk;
#endif
	/** register base */
	void __iomem *regs;
	/** spinlock for irq control */
	spinlock_t spinlock_irq;
	/** irq number */
	int irq;
	/** irq status mask */
	u32 irq_mask;

	/** data use by gce driver */
	u32 gce_subsys;
	struct cmdq_client *cmdq_client;
	struct cmdq_pkt *cmdq_pkt;

	/** sbrc callback related */
	mtk_mmsys_cb cb_func;
	u32 cb_status;
	u32 reg_base;
	void *cb_priv_data;

#ifdef CONFIG_MTK_DEBUGFS
	/** sbrc debugfs data */
	struct dentry *debugfs;
#endif
};

#ifdef CONFIG_MTK_DEBUGFS
extern struct dentry *mtk_debugfs_root;
int mtk_sbrc_debugfs_init(struct mtk_sbrc *sbrc);
void mtk_sbrc_debugfs_exit(struct mtk_sbrc *sbrc);
#endif

int reg_read_mask(void __iomem *regs, u32 offset, u32 mask);
int mtk_sbrc_set_bypass(void __iomem *regs, u32 value);
int mtk_sbrc_irq_clear(void __iomem *regs, u32 mask);
#endif

