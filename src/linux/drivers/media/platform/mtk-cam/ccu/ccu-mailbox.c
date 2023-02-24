/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/io.h>
#include <linux/string.h>

#include "ccu.h"
#include "ccu-regs.h"

static inline int mtk_ccu_msg_copy(
	volatile struct _ccu_msg_t *dest,
	volatile struct _ccu_msg_t *src)
{
	dest->msg_id = src->msg_id;
	dest->in_data_ptr = src->in_data_ptr;
	dest->out_data_ptr = src->out_data_ptr;
	dest->rsv = src->rsv;
	return 0;
}

static enum mb_result _ccu_mailbox_write_to_buffer(
	struct ccu_device *cdev, struct _ccu_msg_t *task)
{
	volatile struct _ccu_mailbox_t *_ccu_mailbox;
	volatile unsigned int rear;
	volatile unsigned int front;
	enum mb_result result;
	unsigned int nextWriteSlot;

	_ccu_mailbox = cdev->mbox.ccu_mb_addr;
	rear = _ccu_mailbox->rear;
	front = _ccu_mailbox->front;
	nextWriteSlot = (rear + 1) & (CCU_MAILBOX_QUEUE_SIZE - 1);

	/* Check if queue is full, full when front = rear + 1 */
	/* (modulus add: rear+1 = rear+1 % CCU_MAILBOX_QUEUE_SIZE) */
	if (nextWriteSlot == front) {
		result = MAILBOX_QUEUE_FULL;

		pr_debug("ccu mailbox queue full !!\n");
	} else {
		mtk_ccu_msg_copy(&(_ccu_mailbox->queue[nextWriteSlot]), task);

		_ccu_mailbox->rear = nextWriteSlot;

		/* memory barrier to check if mailbox value is wrote */
		/* into DRAM, not still in CPU write buffer */
		mb();

		result = MAILBOX_OK;

		pr_debug("sending cmd: f(%d), r(%d), cmd(%d), in(%x), out(%x)\n",
			_ccu_mailbox->front,
			_ccu_mailbox->rear,
			_ccu_mailbox->queue[_ccu_mailbox->rear].msg_id,
			_ccu_mailbox->queue[_ccu_mailbox->rear].in_data_ptr,
			_ccu_mailbox->queue[_ccu_mailbox->rear].out_data_ptr);
	}

	return result;
}

static inline void _memclr(void *dest, size_t length)
{
	volatile char *destPtr = dest;

	while (length--)
		*destPtr++ = 0;
}

enum mb_result mtk_ccu_mailbox_init(struct ccu_device *cdev)
{
	void __iomem *dmem_base = cdev->hw_info.dmem_base;

	cdev->mbox.apmcu_mb_addr = (struct _ccu_mailbox_t *)(dmem_base +
		ccu_read_reg(cdev, CCU_DATA_REG_MAILBOX_APMCU));
	cdev->mbox.ccu_mb_addr = (struct _ccu_mailbox_t *)(dmem_base +
		ccu_read_reg(cdev, CCU_DATA_REG_MAILBOX_CCU));

	/* there will be alignment fault if using memset. */
	_memclr(cdev->mbox.apmcu_mb_addr, sizeof(struct _ccu_mailbox_t));
	_memclr(cdev->mbox.ccu_mb_addr, sizeof(struct _ccu_mailbox_t));

	/* memory barrier to check if mailbox value is wrote into DRAM */
	mb();

	pr_debug("ccu_mailbox addr: %p\n", cdev->mbox.ccu_mb_addr);
	pr_debug("apmcu_mailbox addr: %p\n", cdev->mbox.apmcu_mb_addr);

	return MAILBOX_OK;
}

enum mb_result mtk_ccu_mailbox_send_cmd(
	struct ccu_device *cdev, struct _ccu_msg_t *task)
{
	/* Fill slot */
	enum mb_result result = _ccu_mailbox_write_to_buffer(cdev, task);

	if (result != MAILBOX_OK)
		return result;

	/* Send interrupt */
	/* MCU write this field to trigger ccu interrupt pulse */
	ccu_write_reg_bit(cdev, CTL_CCU_INT, INT_CTL_CCU, 1);

	return MAILBOX_OK;
}

enum mb_result mtk_ccu_mailbox_receive_cmd(
	struct ccu_device *cdev, struct _ccu_msg_t *task)
{
	volatile struct _ccu_mailbox_t *_apmcu_mailbox;
	volatile unsigned int rear;
	volatile unsigned int front;
	unsigned int nextReadSlot;
	enum mb_result result;

	_apmcu_mailbox = cdev->mbox.apmcu_mb_addr;
	rear = _apmcu_mailbox->rear;
	front = _apmcu_mailbox->front;

	/* Check if queue is empty, empty when front == rear */
	if (rear != front) {
		/* modulus add: rear+1 = rear+1 % CCU_MAILBOX_QUEUE_SIZE */
		nextReadSlot = (front + 1) & (CCU_MAILBOX_QUEUE_SIZE - 1);

		mtk_ccu_msg_copy(task, &(_apmcu_mailbox->queue[nextReadSlot]));

		_apmcu_mailbox->front = nextReadSlot;

		result = MAILBOX_OK;

		pr_debug("received cmd: f(%d), r(%d), cmd(%d), ",
			_apmcu_mailbox->front,
			_apmcu_mailbox->rear,
			_apmcu_mailbox->queue[nextReadSlot].msg_id);
		pr_debug("in(%x), out(%x)\n",
			_apmcu_mailbox->queue[nextReadSlot].in_data_ptr,
			_apmcu_mailbox->queue[nextReadSlot].out_data_ptr);
	} else {
		result = MAILBOX_QUEUE_EMPTY;

	}

	return result;
}
