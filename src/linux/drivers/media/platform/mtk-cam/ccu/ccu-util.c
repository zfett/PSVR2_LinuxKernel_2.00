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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "ccu.h"

#define CCU_WR32(addr, data) iowrite32(data, (void *)addr)
#define CCU_RD32(addr) ioread32((void *)addr)

inline void ccu_write_reg_bit(
	struct ccu_device *cdev, unsigned int reg_offset,
	unsigned int field, int value) {
	unsigned int tmp = ccu_read_reg(cdev, reg_offset);

	if (value == 1)
		tmp |= (1 << field);
	else
		tmp &= ~(1 << field);
	ccu_write_reg(cdev, reg_offset, tmp);
}

inline void ccu_write_reg(struct ccu_device *cdev,
	unsigned int offset, int value) {
	CCU_WR32(cdev->hw_info.ccu_base + offset, value);
}

inline unsigned int ccu_read_reg(struct ccu_device *cdev,
	unsigned int offset) {
	return CCU_RD32(cdev->hw_info.ccu_base + offset);
}

struct ccu_device *mtk_ccu_find_ccu_dev_by_id(int id)
{
	struct ccu_device *dev;

	list_for_each_entry(dev, &ccu_group->ccu_head, list) {
		if (dev->id == id)
			return dev;
	}

	return ERR_PTR(-ENODEV);
}
