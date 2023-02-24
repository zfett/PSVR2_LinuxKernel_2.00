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
#define pr_fmt(fmt) "%s:%d:" fmt, __func__, __LINE__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "cammem.h"

static struct camera_mem_device_group *cammem_grp;

static inline struct camera_mem_device *to_cammdev(int id)
{
	struct camera_mem_device *cammdev;

	list_for_each_entry(cammdev, &cammem_grp->camdev_list_head, list) {
		if (cammdev->id == id)
			return cammdev;
	}

	return ERR_PTR(-ENODEV);
}

static int mtk_cammem_open(struct inode *inode, struct file *filp)
{
	struct camera_mem_device *cammdev = to_cammdev(iminor(inode));
	void *data = cammdev->private_data;

	if (atomic_inc_return(&cammdev->refcnt) == 1) {
		if (cammdev->cmem_ops->cammem_open)
			cammdev->cmem_ops->cammem_open(data);
	}

	filp->private_data = (void *)cammdev;

	pr_debug("cammdev(%d) is open\n", cammdev->id);

	return 0;
}

static int mtk_cammem_release(struct inode *inode, struct file *filp)
{
	struct camera_mem_device *cammdev = to_cammdev(iminor(inode));
	void *data = cammdev->private_data;

	if (!atomic_dec_return(&cammdev->refcnt)) {
		if (cammdev->cmem_ops->cammem_close)
			cammdev->cmem_ops->cammem_close(data);
	}

	pr_debug("cammdev(%d) is closed\n", cammdev->id);

	return 0;
}

static long mtk_cammem_ioctl(
	struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct camera_mem_device *cammdev = filp->private_data;
	void *data = cammdev->private_data;
	struct ISP_MEMORY_STRUCT isp_mem;
	unsigned int dir;
	int rc;

	dir = _IOC_DIR(cmd);

	if (_IOC_SIZE(cmd) > sizeof(isp_mem))
		return -EINVAL;

	if (dir & _IOC_WRITE)
		if (copy_from_user(&isp_mem, (void __user *)arg,
				   _IOC_SIZE(cmd)))
			return -EFAULT;

	if (cammdev->cmem_ops->cammem_ioctl) {
		rc = cammdev->cmem_ops->cammem_ioctl(data, cmd, &isp_mem);
		if (rc)
			goto cam_ioctl_err;
	} else {
		WARN_ON(1);
	}

	if (dir & _IOC_READ)
		if (copy_to_user((void __user *)arg, &isp_mem, _IOC_SIZE(cmd)))
			return -EFAULT;

	pr_debug("nr:%d\n",  _IOC_NR(cmd));

	return 0;

cam_ioctl_err:
	return rc;
}

static int mtk_cammem_mmap(struct file *filp, struct vm_area_struct *vm)
{
	struct camera_mem_device *cammdev = filp->private_data;
	void *data = cammdev->private_data;

	if (!cammdev->cmem_ops->cammem_mmap)
		return -EINVAL;

	return cammdev->cmem_ops->cammem_mmap(data, vm);
}

static const struct file_operations mtk_cammem_fops = {
	.owner = THIS_MODULE,
	.open = mtk_cammem_open,
	.release = mtk_cammem_release,
	.unlocked_ioctl = mtk_cammem_ioctl,
	.mmap = mtk_cammem_mmap,
};

struct cammem_device_data {
	struct camera_mem_device_ops *ops;
	char *camera_device_name;
};

struct cammem_device_data sd0_data = {
	.ops = &cammem_ion_ops,
	.camera_device_name = "cammem-side0",
};

struct cammem_device_data sd1_data = {
	.ops = &cammem_ion_ops,
	.camera_device_name = "cammem-side1",
};

struct cammem_device_data gz0_data = {
	.ops = &cammem_ion_ops,
	.camera_device_name = "cammem-gaze0",
};

struct cammem_device_data gz1_data = {
	.ops = &cammem_ion_ops,
	.camera_device_name = "cammem-gaze1",
};

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
struct cammem_device_data rsvd_data = {
	.ops = &cammem_rsvd_ops,
	.camera_device_name = "cammem-rsvd",
};
#endif

static const struct of_device_id cammem_of_ids[] = {
	{.compatible = "mediatek,mt3612-cammem-side0", .data = &sd0_data},
	{.compatible = "mediatek,mt3612-cammem-side1", .data = &sd1_data},
	{.compatible = "mediatek,mt3612-cammem-gaze0", .data = &gz0_data},
	{.compatible = "mediatek,mt3612-cammem-gaze1", .data = &gz1_data},
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	{.compatible = "mediatek,mt3612-cammem-rsv", .data = &rsvd_data},
#endif
	{}
};

static int mtk_cammem_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *of_id;
	struct camera_mem_device *cammdev;
	const struct cammem_device_data *device_data;
	int major, minor;
	int rc;

	cammdev = kzalloc(sizeof(struct camera_mem_device), GFP_KERNEL);
	if (!cammdev)
		return -ENOMEM;

	of_id = of_match_node(cammem_of_ids, np);
	if (!of_id) {
		rc = -EINVAL;
		goto device_create_err;
	}

	device_data = of_id->data;

	major = MAJOR(cammem_grp->devno);
	minor = cammem_grp->num_devs;
	cammdev->dev = device_create(cammem_grp->cam_class, NULL,
			MKDEV(major, minor), cammdev,
			device_data->camera_device_name);
	if (IS_ERR(cammdev->dev)) {
		rc = PTR_ERR(cammdev->dev);
		goto device_create_err;
	}

	cammdev->id = cammem_grp->num_devs;
	cammdev->cmem_ops = device_data->ops;
	atomic_set(&cammdev->refcnt, 0);
	cammdev->private_data = cammdev->cmem_ops->cammem_probe(pdev);
	if (IS_ERR(cammdev->private_data)) {
		rc = PTR_ERR(cammdev->private_data);
		goto cammem_probe_err;
	}

	list_add(&cammdev->list, &cammem_grp->camdev_list_head);
	cammem_grp->num_devs++;
	platform_set_drvdata(pdev, cammdev);

	return 0;

cammem_probe_err:
	device_destroy(cammem_grp->cam_class, MKDEV(major, minor));
device_create_err:
	kfree(cammdev);
	return rc;
}

static int mtk_cammem_remove(struct platform_device *pdev)
{
	struct camera_mem_device *cammdev = platform_get_drvdata(pdev);
	void *data = cammdev->private_data;
	int major, minor;

	if (cammdev->cmem_ops->cammem_remove)
		cammdev->cmem_ops->cammem_remove(data);

	major = MAJOR(cammem_grp->devno);
	minor = cammdev->id;
	device_destroy(cammem_grp->cam_class, MKDEV(major, minor));
	kfree(cammdev);

	return 0;
}

static struct platform_driver mtk_cammem = {
	.probe = mtk_cammem_probe,
	.remove = mtk_cammem_remove,
	.driver = {
		.name = "mtk-cammem",
		.owner = THIS_MODULE,
		.of_match_table = cammem_of_ids,
	}
};

static int __init mtk_cammem_init(void)
{
	int rc;

	cammem_grp = kzalloc(sizeof(struct camera_mem_device_group),
			     GFP_KERNEL);
	if (!cammem_grp)
		return -ENOMEM;

	INIT_LIST_HEAD(&cammem_grp->camdev_list_head);

	rc = alloc_chrdev_region(&cammem_grp->devno, 0,
				 CAMMEM_MAX_DEVICES, "mtk-cammem");
	if (rc < 0)
		goto alloc_cdev_err;

	cammem_grp->cdev = cdev_alloc();
	cammem_grp->cdev->owner = THIS_MODULE;
	cammem_grp->cdev->ops = &mtk_cammem_fops;
	rc = cdev_add(cammem_grp->cdev,
		cammem_grp->devno, CAMMEM_MAX_DEVICES);
	if (rc < 0)
		goto cdev_add_err;

	cammem_grp->cam_class = class_create(THIS_MODULE, "mtk-cammem");
	if (IS_ERR(cammem_grp->cam_class)) {
		rc = PTR_ERR(cammem_grp->cam_class);
		goto class_err;
	}

	rc = platform_driver_register(&mtk_cammem);
	if (rc < 0)
		goto class_err;

	return 0;

class_err:
	class_destroy(cammem_grp->cam_class);
cdev_add_err:
	unregister_chrdev_region(cammem_grp->devno, CAMMEM_MAX_DEVICES);
alloc_cdev_err:
	kfree(cammem_grp);
	return rc;
}

static void __exit mtk_cammem_exit(void)
{
	platform_driver_unregister(&mtk_cammem);
	unregister_chrdev_region(cammem_grp->devno, CAMMEM_MAX_DEVICES);
	class_destroy(cammem_grp->cam_class);
	kfree(cammem_grp);
}

module_init(mtk_cammem_init);
module_exit(mtk_cammem_exit);
MODULE_LICENSE("GPL");
