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

#ifndef __ISP_H
#define __ISP_H

#include <linux/list.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <soc/mediatek/mtk_isp.h>

#define ISP_DEV_GROUP_NAME		"mtk-camera-isp"
#define MAX_STR_SIZE		20

#ifdef CONFIG_VIDEO_MEDIATEK_ISP_TINY
#define MAX_SLOT_SIZE		1
#else
#define MAX_SLOT_SIZE		32
#endif

#define MAX_ISP_DEVICES	8

#define ISP_WR32(addr, data) iowrite32(data, (void *)addr)
#define ISP_RD32(addr) ioread32((void *)addr)
#define ISP_SET_BIT(reg, bit) ((*(MUINT32 *)(reg)) |= (MUINT32)(1 << (bit)))
#define ISP_CLR_BIT(reg, bit) ((*(MUINT32 *)(reg)) &= ~((MUINT32)(1 << (bit))))

union cmd_data_t {
	int module_id;
	struct isp_userkey userkey_struct;
	struct event_req e_req;
	struct isp_module_info module_info;
	unsigned int num_slots;
};

struct isp_module_hw_info {
	phys_addr_t start;
	phys_addr_t end;
	int range;
	void __iomem *base_addr;
	int irq;
};

struct isp_module_event {
	int in_use;
	char user_name[MAX_STR_SIZE];
	pid_t slot_owner;
};

struct isp_clk {
	struct clk *clk;
	bool en;
	struct list_head list;
};

struct isp_clk_list {
	struct list_head head;
	int clkcnt;
};

struct isp_module {
	char name[MAX_STR_SIZE];
	char long_name[MAX_STR_SIZE];
	int module_id;
	int parent_id;
	int pdm_cnt;
	spinlock_t pdm_lock;

	struct list_head list;
	wait_queue_head_t wait_queue;

	struct device *module_dev;
	struct isp_clk_list *clklist;
	struct isp_dev *parent_dev;
	struct isp_module_hw_info hw_info;
	struct isp_module_ops *ops;
	struct isp_module_event event_slot[MAX_SLOT_SIZE];
	struct dentry *root_dir;
	void *private_data;
};

struct isp_module_ops {
	int (*init)(const struct isp_module *module);
	int (*restart)(struct isp_module *module);
	int (*stop)(struct isp_module *module);
	int (*wait_module_event)(struct isp_module *module,
				const struct event_req *event);
	int (*flush_module_event)(struct isp_module *module,
				const struct event_req *event);
	int (*clear_module_event)(struct isp_module *module,
				const struct event_req *event);
	int (*custom_ioctl)(const struct isp_module *module,
				unsigned int cmd, unsigned long arg);
	void (*reset)(const struct isp_module *module);
	int (*set_poll_trigger)(struct file *filp,
				struct isp_module *module,
				const struct event_req *event);
	int (*check_poll_trigger)(struct file *filp,
				struct isp_module *module);
	int (*read_poll_trigger)(struct file *filp,
				struct isp_module *module);
};

struct isp_dev {
	atomic_t refcnt;
	int dev_id;
	struct list_head list;
	struct list_head isp_module_head;
	spinlock_t list_lock;
	struct mutex slot_lock;
	struct mutex dev_lock;

	struct device *dev;
	struct dentry *root_dir;
};

struct isp_dev_group {
	const char *name;
	int num;
	struct list_head isp_dev_head;
	struct class *dev_class;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_root;
#endif
	struct cdev *isp_cdev;
	dev_t isp_devno;
};

typedef void (*cbptr_t)(void *data);

struct isp_p1_cb_event_package {
	int cb_type;
	cbptr_t cbptr;
	struct list_head list;
	void *private_data;
};

extern struct isp_dev_group *isp_group;

int mtk_isp_register_isp_module(int id, struct isp_module *module);

int mtk_isp_unregister_isp_module(struct isp_module *module);

int mtk_isp_register_p1_cb(int dev_id, int cb_type, cbptr_t cbptr, void *data);

int mtk_isp_unregister_p1_cb(int dev_id, int cb_type,
	cbptr_t cbptr, void *data);

struct isp_dev *mtk_isp_find_isp_dev_by_id(int id);

struct isp_dev *mtk_isp_find_isp_dev_by_module(
	const struct isp_module *module);

struct isp_module *mtk_isp_find_isp_module_by_name(
	const struct isp_dev *dev, const char *m_name);

struct isp_module *mtk_isp_find_isp_module_by_id(
	const struct isp_dev *dev, int id);

struct isp_module *_isp_module_probe_index(const struct platform_device *pdev,
	int index, const char *module_name);

/* CCF & PM */
int isp_module_grp_clk_get(struct device *dev, struct isp_clk_list **cl);
int isp_module_grp_clk_free(struct isp_module *module);
int isp_module_power_on(const struct isp_module *module);
int isp_module_power_off(const struct isp_module *module);
int isp_module_pdm_enable(struct device *dev, struct isp_module *module);
int isp_module_pdm_disable(struct device *dev, struct isp_module *module);


unsigned long mtk_isp_ms_to_jiffies(unsigned long ms);

unsigned long mtk_isp_us_to_jiffies(unsigned long us);

#endif /* __ISP_H */
