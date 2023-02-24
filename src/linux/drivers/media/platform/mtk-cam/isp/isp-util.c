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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/delay.h>

#include "isp.h"

inline unsigned long mtk_isp_ms_to_jiffies(unsigned long ms)
{
	return msecs_to_jiffies(ms);
}
EXPORT_SYMBOL(mtk_isp_ms_to_jiffies);

inline unsigned long mtk_isp_us_to_jiffies(unsigned long us)
{
	return usecs_to_jiffies(us);
}
EXPORT_SYMBOL(mtk_isp_us_to_jiffies);

struct isp_dev *mtk_isp_find_isp_dev_by_id(int id)
{
	struct isp_dev *dev;

	list_for_each_entry(dev, &isp_group->isp_dev_head, list) {
		if (dev->dev_id == id)
			return dev;
	}

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(mtk_isp_find_isp_dev_by_id);

inline struct isp_dev *mtk_isp_find_isp_dev_by_module(
	const struct isp_module *module)
{
	return module->parent_dev;
}
EXPORT_SYMBOL(mtk_isp_find_isp_dev_by_module);

struct isp_module *mtk_isp_find_isp_module_by_name(
	const struct isp_dev *dev, const char *m_name)
{
	struct isp_module *module;

	list_for_each_entry(module, &dev->isp_module_head, list) {
		if (!strcmp(module->name, m_name))
			return module;
	}

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(mtk_isp_find_isp_module_by_name);

struct isp_module *mtk_isp_find_isp_module_by_id(
	const struct isp_dev *dev, int id)
{
	struct isp_module *module;

	list_for_each_entry(module, &dev->isp_module_head, list) {
		if (module->module_id == id)
			return module;
	}

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(mtk_isp_find_isp_module_by_id);
