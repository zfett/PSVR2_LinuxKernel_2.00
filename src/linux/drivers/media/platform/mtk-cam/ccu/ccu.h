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

#ifndef __CCU_H__
#define __CCU_H__

#include <linux/list.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/completion.h>

#include <soc/mediatek/ccu-ext.h>

#include "ccu-mailbox.h"
#include "ccu-i2c.h"

#define MAX_CCU_DEVICES 4

union cmd_data_t {
	struct ccu_hw_info ccu_info;
	struct ccu_power ccu_pow;
	struct ccu_cmd ccu_cmd;
	struct ccu_log ccu_log;
	struct ccu_wait_irq wait_irq;
};

struct ccu_device_hw_info {
	void __iomem *ccu_base;
	phys_addr_t ccu_hw_base;
	unsigned int range;

	void __iomem *dmem_base;
	phys_addr_t ccu_dmem_base;
	unsigned int ccu_dmem_size;
	unsigned int ccu_dmem_offset;
	unsigned int ccu_pmem_base;
	unsigned int ccu_pmem_size;
	unsigned int ccu_log_base;
	unsigned int ccu_log_size;
	unsigned int ccu_l_cam_a;
	unsigned int ccu_l_cam_b;
	unsigned int ccu_r_cam_a;
	unsigned int ccu_r_cam_b;
	unsigned int irq_num;
	unsigned int i2c_adap_l_nr;
	unsigned int i2c_adap_r_nr;
};

struct ccu_user {
	struct ccu_device *ccu_dev;
	struct mutex enque_mutex;
	struct mutex deque_mutex;
	struct list_head enque_ccu_cmd_list;
	struct list_head deque_ccu_cmd_list;
	struct completion deque_done;
	struct completion log_done;
	atomic_t log_idx;
};

struct ccu_device {
	struct device *dev;
	struct ccu_user *user;
	struct workqueue_struct *p_ccu_wq;
	struct work_struct work;
	struct task_struct *enque_task;
	wait_queue_head_t cmd_wait;
	struct ccu_mailbox mbox;
	struct ccu_device_hw_info hw_info;
	struct list_head list;
	int id;
	char name[5];
	atomic_t refcnt;
};

struct ccu_device_group {

	/* All of ccu device use the same queue. */
	struct workqueue_struct *ccu_shared_wq;
	/* This list is used to chain all of ccu devices in system. */
	struct list_head ccu_head;

	/* All of ccu device share the same major number.*/
	dev_t ccu_devno;
	struct cdev *ccu_cdev;
	struct class *ccu_class;
	int ccu_device_index;
};

struct ccu_cmd_list {
	struct ccu_cmd node;
	struct list_head link;
};

extern struct ccu_device_group *ccu_group;
void ccu_write_reg_bit(struct ccu_device *cdev, unsigned int reg_offset,
					   unsigned int field, int value);
void ccu_write_reg(struct ccu_device *cdev,	unsigned int offset, int value);
unsigned int ccu_read_reg(struct ccu_device *cdev, unsigned int offset);
struct ccu_device *mtk_ccu_find_ccu_dev_by_id(int id);
int mtk_ccu_init_hw(struct ccu_device *cdev);
int mtk_ccu_run(struct ccu_device *cdev);
int mtk_ccu_power_control(struct ccu_device *cdev, struct ccu_power *power);
int mtk_ccu_set_log_addr(struct ccu_device *cdev, struct ccu_log *log);

#endif /* __CCU_H__ */
