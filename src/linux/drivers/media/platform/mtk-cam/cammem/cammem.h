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

#ifndef __CAMMEM_H__
#define __CAMMEM_H__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/atomic.h>

#include <soc/mediatek/mtk_cammem.h>

#define CAMMEM_MAX_DEVICES 5

struct camera_mem_device_ops {
	void* (*cammem_probe)(struct platform_device *pdev);
	int (*cammem_remove)(void *private_data);
	long (*cammem_ioctl)(void *private_data, unsigned int cmd,
			     struct ISP_MEMORY_STRUCT *isp_mem);
	int (*cammem_open)(void *private_data);
	void (*cammem_close)(void *private_data);
	int (*cammem_mmap)(void *private_data, struct vm_area_struct *vm);
};

struct camera_mem_device {
	struct device *dev;
	struct list_head list;
	struct camera_mem_device_ops *cmem_ops;
	int id;
	atomic_t refcnt;
	void *private_data;
};

struct camera_mem_device_group {
	struct cdev *cdev;
	dev_t devno;
	int num_devs;
	struct class *cam_class;
	struct list_head camdev_list_head;
};

extern struct camera_mem_device_ops cammem_ion_ops;
extern struct camera_mem_device_ops cammem_rsvd_ops;

#endif /* __CAMMEM_H__ */
