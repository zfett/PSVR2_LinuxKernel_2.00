/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk-cmdq-mailbox.h
 * header of mtk-cmdq-mailbox.c
 */

#ifndef __MTK_CMDQ_MAILBOX_H__
#define __MTK_CMDQ_MAILBOX_H__

#include <linux/mailbox_controller.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/mtk-cmdq.h>

/** @ingroup IP_group_gce_external_def
 * @brief constant definition used for gce helper function.
 * @{
 */
/** The maximum of gce hardware thread is 32 */
#define CMDQ_THR_MAX_COUNT		32
/** cmdq event maximum */
#define CMDQ_MAX_EVENT				0x3ff
/** The gce hardware interrupt mask */
#define CMDQ_IRQ_MASK			(u32)((1ULL << CMDQ_THR_MAX_COUNT) - 1)
/** The macro used for check the numbers of commands */
#define CMDQ_NUM_CMD(cmd_buf_size)	((cmd_buf_size) / CMDQ_INST_SIZE)
/**
 * @}
 */

/** @ingroup IP_group_gce_external_struct
 * @brief cmdq thread structure. All the information about the thread was\n
 * stored in this structure.
 */
struct cmdq_thread {
	/** software representation of a communication chan */
	struct mbox_chan	*chan;
	/** gce hardware thread register base */
	void __iomem		*base;
	/** linux timer list, for the software protection of gce */
	struct timer_list	timeout;
	/** timeout time in micro seconds */
	u32			timeout_ms;
	/** client can set the priority for every gce hardware thread */
	u32			priority;
	/** cmdq structure */
	struct cmdq		*cmdq;
	/** every gce hardware thread has only one command buffer at a time */
	dma_addr_t		pa_base;
	/** command buffer, the packet sent from mailbox client */
	struct cmdq_pkt		*pkt;
};

/** @ingroup IP_group_gce_external_struct
 * @brief CMDQ structure.
 */
struct cmdq {
	/** controller of a class of communication channels */
	struct mbox_controller	mbox;
	/** gce hardware register base */
	void __iomem		*base;
	/** gce hardware interrupt number */
	u32			irq;
	/** gce hardware thread information */
	struct cmdq_thread	thread[CMDQ_THR_MAX_COUNT];
	/** suspend enable/disable */
	bool			suspended;
};

/** @ingroup IP_group_gce_external_struct
 * @brief cmdq task callback structure.
 */
struct cmdq_task_cb {
	/** callback function */
	cmdq_async_flush_cb	cb;
	/** callback data */
	void			*data;
};

/** @ingroup IP_group_gce_external_struct
 * @brief cmdq packet structure. Anything about the command buffer\n
 * was stored in this structure.
 */
struct cmdq_pkt {
	/** virtual base address */
	void			*va_base;
	/** command occupied size */
	size_t			cmd_buf_size;
	/** real buffer size */
	size_t			buf_size;
	/** task callback function */
	struct cmdq_task_cb	cb;
};

#endif /* __MTK_CMDQ_MAILBOX_H__ */
