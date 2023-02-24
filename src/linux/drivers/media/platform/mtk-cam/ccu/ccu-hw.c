/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "%s:%d:" fmt, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>

#include <soc/mediatek/ccu_ext_interface/ccu_ext_interface.h>

#include "ccu.h"
#include "ccu-regs.h"

static int mtk_ccu_enque_cmd_loop(void *arg)
{
	struct ccu_device *cdev = (struct ccu_device *) arg;
	struct ccu_user *user = cdev->user;
	struct ccu_cmd_list *cmd_list;
	int t = 0;

	while (1) {
		t = wait_event_interruptible_timeout(
			cdev->cmd_wait,
			!list_empty_careful(&user->enque_ccu_cmd_list),
			MAX_SCHEDULE_TIMEOUT);

		if (kthread_should_stop())
			break;

		if (t == 0)
			continue;

		mutex_lock(&user->enque_mutex);
		cmd_list = list_first_entry(
			&user->enque_ccu_cmd_list, struct ccu_cmd_list, link);
		list_del_init(&cmd_list->link);
		mutex_unlock(&user->enque_mutex);

		pr_debug("start to send msg:%d to ccu%d\n",
			cmd_list->node.task.msg_id, cdev->id);
		mtk_ccu_mailbox_send_cmd(cdev, &cmd_list->node.task);
		kfree(cmd_list);
	}

	pr_debug("%s ccu%d exit\n", __func__, cdev->id);

	return 0;
}

static void mtk_ccu_process_log_msg(
	struct ccu_device *cdev, struct _ccu_msg_t *rec_msg)
{
	struct ccu_user *user = cdev->user;

	if (rec_msg->msg_id == MSG_TO_APMCU_FLUSH_LOG)
		/* in_data_ptr would be the value either
		 * CCU_LOG_FLUSH_LOG_BUF_1 or CCU_LOG_FLUSH_LOG_BUF_1
		 */
		atomic_set(&user->log_idx, rec_msg->in_data_ptr);
	else if (rec_msg->msg_id == MSG_TO_APMCU_CCU_ASSERT)
		atomic_set(&user->log_idx, CCU_LOG_ASSERT);
	else if (rec_msg->msg_id == MSG_TO_APMCU_CCU_WARNING)
		atomic_set(&user->log_idx, CCU_LOG_WARNING);
	else
		WARN_ON(1);

	complete(&user->log_done);
}

static void mtk_ccu_process_ack_msg(
	struct ccu_device *cdev, struct _ccu_msg_t *rec_msg)
{
	struct ccu_user *user = cdev->user;
	struct ccu_cmd_list *cmd_list;

	cmd_list = kzalloc(sizeof(struct ccu_cmd_list), GFP_KERNEL);
	if (!cmd_list)
		return;

	memcpy(cmd_list, rec_msg, sizeof(struct _ccu_msg_t));

	mutex_lock(&user->deque_mutex);
	list_add_tail(&cmd_list->link, &user->deque_ccu_cmd_list);
	mutex_unlock(&user->deque_mutex);

	complete(&user->deque_done);
}

static void mtk_ccu_wq_handler(struct work_struct *w)
{
	struct ccu_device *cdev = container_of(w, struct ccu_device, work);
	enum mb_result mailboxRet;
	struct _ccu_msg_t receivedCcuCmd;

	while (1) {
		mailboxRet = mtk_ccu_mailbox_receive_cmd(cdev, &receivedCcuCmd);

		if (mailboxRet == MAILBOX_QUEUE_EMPTY) {
			break;
		}

		switch (receivedCcuCmd.msg_id) {
		case MSG_TO_APMCU_FLUSH_LOG:
		case MSG_TO_APMCU_CCU_ASSERT:
		case MSG_TO_APMCU_CCU_WARNING:
			mtk_ccu_process_log_msg(cdev, &receivedCcuCmd);
			break;
		default:
			mtk_ccu_process_ack_msg(cdev, &receivedCcuCmd);
			break;
		}
	}
}

irqreturn_t mtk_ccu_isr_handler(int irq, void *dev_id)
{
	struct ccu_device *cdev = (struct ccu_device *) dev_id;

	/*write clear mode*/
	ccu_write_reg(cdev, EINTC_CLR, 0xFF);

	ccu_read_reg(cdev, EINTC_ST);

	if (unlikely(ccu_read_reg(cdev, CCU_DATA_REG_MAILBOX_CCU) == 0))
		;
	else
		queue_work(cdev->p_ccu_wq, &cdev->work);

	return IRQ_HANDLED;
}

static int mtk_ccu_wait_init_done(struct ccu_device *cdev, unsigned int value)
{
	int timeout = 100;

	ccu_write_reg(cdev, CCU_STA_REG_SW_INIT_DONE, 0);

	while ((timeout-- >= 0) &&
		(ccu_read_reg(cdev, CCU_STA_REG_SW_INIT_DONE) != value))
		udelay(100);

	if (timeout <= 0) {
		pr_debug("timeout: ccu initial debug info28: %x\n",
			ccu_read_reg(cdev, CCU_INFO28));
		return -ETIMEDOUT;
	}

	return 0;
}

int mtk_ccu_init_hw(struct ccu_device *cdev)
{
	INIT_WORK(&cdev->work, mtk_ccu_wq_handler);

	if (cdev->hw_info.irq_num > 0) {
		sprintf(cdev->name, "ccu%d", cdev->id);
		if (request_irq(cdev->hw_info.irq_num, mtk_ccu_isr_handler,
			IRQF_TRIGGER_NONE | IRQF_SHARED, cdev->name, cdev)) {
			pr_info("fail to request ccu irq!\n");
			return -ENODEV;
		}
	}

	cdev->enque_task = kthread_create(
		mtk_ccu_enque_cmd_loop, cdev, "ccu-enque");
	if (IS_ERR(cdev->enque_task))
		return PTR_ERR(cdev->enque_task);

	wake_up_process(cdev->enque_task);

	pr_debug("ccu%d init success\n", cdev->id);

	return 0;
}

int mtk_ccu_run(struct ccu_device *cdev)
{
	int rc;

	ccu_write_reg_bit(cdev, CCU_RESET, MD32_HW_RST, 0);

	rc = mtk_ccu_wait_init_done(cdev, CCU_STATUS_INIT_DONE);
	if (rc)
		return rc;

	mtk_ccu_mailbox_init(cdev);

	rc = mtk_ccu_wait_init_done(cdev, CCU_STATUS_INIT_DONE_2);
	if (rc)
		return rc;

	pr_debug("ccu%d starts to run\n", cdev->id);

	return 0;
}

int mtk_ccu_set_log_addr(
	struct ccu_device *cdev, struct ccu_log *log)
{
	if (!log)
		return -EINVAL;

	ccu_write_reg(cdev, CCU_DATA_REG_LOG_BUF0, log->log_addr[0]);
	ccu_write_reg(cdev, CCU_DATA_REG_LOG_BUF1, log->log_addr[1]);

	return 0;
}

int mtk_ccu_power_control(
	struct ccu_device *cdev, struct ccu_power *power)
{
	int rc = 0;

	switch (power->control) {
	case CCU_POWER_ON:
		/* set CCU_A_RESET. CCU_HW_RST=1*/
		ccu_write_reg(cdev, CCU_RESET, 0xFF1FFCFF);
		ccu_write_reg(cdev, CCU_RESET, 0x00010000);
		break;
	case CCU_POWER_OFF:
		break;
	case CCU_POWER_RESTART:
		/* Restart CCU, no need to release CG */
		break;
	case CCU_POWER_PAUSE:
		/* Pause CCU, but don't pullup CG */
		break;
	case CCU_POWER_ENABLE_CG_ONLY:
		/* CCU boot fail, just enable CG */
		break;
	default:
		pr_info("unknown power control(%d)\n", power->control);
		rc = -EINVAL;
	}

	pr_debug("ccu%d power control(%d) success\n", cdev->id, power->control);

	return rc;
}
