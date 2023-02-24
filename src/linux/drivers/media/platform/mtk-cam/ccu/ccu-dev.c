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

struct ccu_device_group *ccu_group;

static ssize_t mtk_ccu_sysfs_show_info(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ccu_device *ccu_dev = dev_get_drvdata(dev);
	char *out = buf;

	out += sprintf(out, "virt_base 0x%p\n", ccu_dev->hw_info.ccu_base);
	out += sprintf(out, "phy_base 0x%x\n",
				(unsigned int) ccu_dev->hw_info.ccu_hw_base);
	out += sprintf(out, "pmem_base 0x%x\n", ccu_dev->hw_info.ccu_pmem_base);
	out += sprintf(out, "pmem_size 0x%x\n", ccu_dev->hw_info.ccu_pmem_size);
	out += sprintf(out, "dmem_base 0x%x\n",
				(unsigned int) ccu_dev->hw_info.ccu_dmem_base);
	out += sprintf(out, "dmem_size 0x%x\n", ccu_dev->hw_info.ccu_dmem_size);
	out += sprintf(out, "dmem_offset 0x%x\n",
				ccu_dev->hw_info.ccu_dmem_offset);
	out += sprintf(out, "log_base 0x%x\n", ccu_dev->hw_info.ccu_log_base);
	out += sprintf(out, "log_size 0x%x\n", ccu_dev->hw_info.ccu_log_size);
	out += sprintf(out, "ccu_l_cam_a 0x%x\n", ccu_dev->hw_info.ccu_l_cam_a);
	out += sprintf(out, "ccu_l_cam_b 0x%x\n", ccu_dev->hw_info.ccu_l_cam_b);
	out += sprintf(out, "ccu_r_cam_a 0x%x\n", ccu_dev->hw_info.ccu_r_cam_a);
	out += sprintf(out, "ccu_r_cam_b 0x%x\n", ccu_dev->hw_info.ccu_r_cam_b);

	return out - buf;
}

static DEVICE_ATTR(info, S_IRUGO, mtk_ccu_sysfs_show_info, NULL);

static int mtk_ccu_flush_command_from_queue(struct ccu_user *user)
{
	struct ccu_cmd_list *cmd_list;

	if (!user)
		return -EINVAL;

	mutex_lock(&user->enque_mutex);
	mutex_lock(&user->deque_mutex);
	list_for_each_entry(cmd_list, &user->enque_ccu_cmd_list, link) {
		list_del_init(&cmd_list->link);
		cmd_list->node.status = CCU_ENG_STATUS_FLUSH;
		list_add_tail(&cmd_list->link, &user->deque_ccu_cmd_list);
		complete(&user->deque_done);
	}
	mutex_unlock(&user->deque_mutex);
	mutex_unlock(&user->enque_mutex);

	return 0;
}

static int mtk_ccu_pop_command_from_queue(
		struct ccu_user *user, struct ccu_cmd **rcmd)
{
	struct ccu_device *dev = user->ccu_dev;
	struct ccu_cmd_list *cmd_list;
	int rc;

	if (!dev)
		return -EINVAL;

	rc = wait_for_completion_interruptible_timeout(
		&user->deque_done, msecs_to_jiffies(500));
	if (rc > 0) {
		/* get first node from deque list */
		mutex_lock(&user->deque_mutex);
		cmd_list = list_first_entry(
			&user->deque_ccu_cmd_list, struct ccu_cmd_list, link);
		list_del_init(&cmd_list->link);
		mutex_unlock(&user->deque_mutex);

		/* if this condition is true, that means the cmd would flush
		 * via mtk_ccu_flush_command_from_queue(). so do not change
		 * its status here.
		 */
		if (unlikely(cmd_list->node.status == CCU_ENG_STATUS_FLUSH))
			pr_debug("msg_is:%d was flushed\n",
					cmd_list->node.task.msg_id);
		else
			cmd_list->node.status = CCU_ENG_STATUS_SUCCESS;
	} else {
		cmd_list = kzalloc(sizeof(struct ccu_cmd_list), GFP_KERNEL);
		if (rc == 0)
			cmd_list->node.status = CCU_ENG_STATUS_TIMEOUT;
		else
			cmd_list->node.status = rc;
	}

	*rcmd = (struct ccu_cmd *) cmd_list;

	pr_debug("ccu%d rc:%d\n", dev->id, rc);

	return 0;
}

static int mtk_ccu_push_command_to_queue(
	struct ccu_user *user, struct ccu_cmd *cmd)
{
	struct ccu_device *dev = user->ccu_dev;
	struct ccu_cmd_list *cmd_list = (struct ccu_cmd_list *)cmd;

	if (!dev)
		return -EINVAL;

	if (!cmd)
		return -EINVAL;

	mutex_lock(&user->enque_mutex);
	list_add_tail(&cmd_list->link, &user->enque_ccu_cmd_list);
	mutex_unlock(&user->enque_mutex);

	wake_up_interruptible(&dev->cmd_wait);

	pr_debug("ccu%d\n", dev->id);

	return 0;
}

static int mtk_ccu_open(struct inode *inode, struct file *flip)
{
	struct ccu_device *cdev = mtk_ccu_find_ccu_dev_by_id(iminor(inode));

	atomic_inc(&cdev->refcnt);
	flip->private_data = (void *)cdev;
	pr_debug("ccu device id:%d\n", cdev->id);

	return 0;
}

static int mtk_ccu_mmap(struct file *flip, struct vm_area_struct *vm)
{
	struct ccu_device *dev = flip->private_data;
	unsigned long length;
	unsigned int phy;

	length = (vm->vm_end - vm->vm_start);
	phy = vm->vm_pgoff << PAGE_SHIFT;

	if (phy == dev->hw_info.ccu_pmem_base) {
		if (length > dev->hw_info.ccu_pmem_size)
			return -EINVAL;
	} else if (phy == dev->hw_info.ccu_dmem_base) {
		if (length > dev->hw_info.ccu_dmem_size)
			return -EINVAL;
	} else if (phy == dev->hw_info.ccu_hw_base) {
		if (length > dev->hw_info.range)
			return -EINVAL;
	} else
		return -EINVAL;

	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	if (remap_pfn_range(
		vm, vm->vm_start, vm->vm_pgoff,
		vm->vm_end - vm->vm_start, vm->vm_page_prot))
		return -EAGAIN;

	pr_debug("ccu device id:%d 0x%x, pmem:0x%x, dmem:0x%x, hw:0x%x\n",
		dev->id, phy, dev->hw_info.ccu_pmem_base,
		(unsigned int) dev->hw_info.ccu_dmem_base,
		(unsigned int) dev->hw_info.ccu_hw_base);

	return 0;
}

static int mtk_ccu_release(struct inode *inode, struct file *flip)
{
	struct ccu_device *cdev = flip->private_data;

	atomic_dec(&cdev->refcnt);

	pr_debug("ccu device id:%d\n", cdev->id);

	return 0;
}

static long mtk_ccu_ioctl(
	struct file *flip, unsigned int cmd, unsigned long arg)
{
	struct ccu_device *dev = flip->private_data;
	struct ccu_user *user = dev->user;
	struct ccu_cmd *cmd_list;
	unsigned int dir;
	int rc = 0;
	union cmd_data_t data;

	dir = _IOC_DIR(cmd);

	if (!user)
		return -EINVAL;

	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (dir & _IOC_WRITE)
		if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;

	switch (cmd) {
	case CCU_QUERY_HW_INFO:
		data.ccu_info.ccu_hw_base = dev->hw_info.ccu_hw_base;
		data.ccu_info.ccu_hw_size = dev->hw_info.range;
		data.ccu_info.ccu_pmem_base = dev->hw_info.ccu_pmem_base;
		data.ccu_info.ccu_pmem_size = dev->hw_info.ccu_pmem_size;
		data.ccu_info.ccu_dmem_base = dev->hw_info.ccu_dmem_base;
		data.ccu_info.ccu_dmem_size = dev->hw_info.ccu_dmem_size;
		data.ccu_info.ccu_dmem_offset = dev->hw_info.ccu_dmem_offset;
		data.ccu_info.ccu_log_base = dev->hw_info.ccu_log_base;
		data.ccu_info.ccu_log_size = dev->hw_info.ccu_log_size;

		pr_debug("Query HW Info: 0x%x, 0x%x, 0x%x\n",
			(unsigned int) dev->hw_info.ccu_hw_base,
			dev->hw_info.ccu_pmem_base,
			(unsigned int) dev->hw_info.ccu_dmem_base);
		break;

	case CCU_SET_POWER:
		mtk_ccu_power_control(dev, &data.ccu_pow);
		break;

	case CCU_SET_LOG_ADDR:
		mtk_ccu_set_log_addr(dev, &data.ccu_log);
		break;

	case CCU_ENQUE_COMMAND:
		cmd_list = kzalloc(sizeof(struct ccu_cmd_list), GFP_KERNEL);
		if (!cmd_list) {
			rc = -ENOMEM;
			goto ioctl_err;
		}
		memcpy(cmd_list, &data, sizeof(struct ccu_cmd));
		rc = mtk_ccu_push_command_to_queue(user, cmd_list);
		break;

	case CCU_DEQUE_COMMAND:
		rc = mtk_ccu_pop_command_from_queue(user, &cmd_list);
		if (rc < 0)
			goto ioctl_err;

		memcpy(&data, cmd_list, sizeof(struct ccu_cmd));
		kfree(cmd_list);
		break;

	case CCU_FLUSH_COMMAND:
		mtk_ccu_flush_command_from_queue(user);
		break;

	case CCU_FLUSH_LOG:
		complete(&user->log_done);
		break;

	case CCU_WAIT_IRQ:

		/*add time out for avoid inifinity waiting.*/
		rc = wait_for_completion_io_timeout(
			&user->log_done, msecs_to_jiffies(500));
		if (rc == 0)
			goto ioctl_err;

		data.wait_irq.log_idx = atomic_read(&user->log_idx);
		break;

	case CCU_SET_RUN:
		mtk_ccu_run(dev);
		break;

	default:
		break;
	}

	if (dir & _IOC_READ)
		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)))
			return -EFAULT;

	/*pr_debug("ccu device id:%d nr:%d\n", dev->id, _IOC_NR(cmd));*/

	return 0;

ioctl_err:
	return rc;
}

static const struct file_operations mtk_ccu_fops = {
	.owner = THIS_MODULE,
	.open = mtk_ccu_open,
	.release = mtk_ccu_release,
	.unlocked_ioctl = mtk_ccu_ioctl,
	.mmap = mtk_ccu_mmap,
};

static int mtk_ccu_device_tree_parsing(
	struct ccu_device *dev,	struct device_node *np)
{
	if (of_property_read_u32_index(
		np, "ccu_pmem_base", 0, &dev->hw_info.ccu_pmem_base))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_pmem_size", 0, &dev->hw_info.ccu_pmem_size))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_dmem_offset", 0, &dev->hw_info.ccu_dmem_offset))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_log_base", 0, &dev->hw_info.ccu_log_base))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_log_size", 0, &dev->hw_info.ccu_log_size))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_isp_l_cam_a_base", 0, &dev->hw_info.ccu_l_cam_a))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_isp_l_cam_b_base", 0, &dev->hw_info.ccu_l_cam_b))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_isp_r_cam_a_base", 0, &dev->hw_info.ccu_r_cam_a))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_isp_r_cam_b_base", 0, &dev->hw_info.ccu_r_cam_b))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_i2c_adap_l_nr", 0, &dev->hw_info.i2c_adap_l_nr))
		return -EINVAL;

	if (of_property_read_u32_index(
		np, "ccu_i2c_adap_r_nr", 0, &dev->hw_info.i2c_adap_r_nr))
		return -EINVAL;

	dev->hw_info.irq_num = irq_of_parse_and_map(np, 0);
	if (!dev->hw_info.irq_num)
		return -EINVAL;

	return 0;
}

static int mtk_ccu_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ccu_device *dev = NULL;
	struct ccu_user *user = NULL;
	int major, minor;
	struct resource res;
	int rc;

	if (!np) {
		pr_err("No device node for mtk ccu driver\n");
		return -ENODEV;
	}

	if (ccu_group->ccu_device_index > MAX_CCU_DEVICES)
		return -ENOMEM;

	dev = kzalloc(sizeof(struct ccu_device), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->hw_info.ccu_base = of_iomap(np, 0);
	if (!dev->hw_info.ccu_base) {
		pr_err("Unable to iomap registers\n");
		rc = -ENOMEM;
		goto of_iomap_ccu_base_err;
	}

	rc = of_address_to_resource(np, 0, &res);
	if (rc < 0) {
		pr_err("Unable to get resource\n");
		goto of_address_ccu_base_err;
	}

	dev->hw_info.ccu_hw_base = res.start;
	dev->hw_info.range = res.end - res.start + 1;

	dev->hw_info.dmem_base = of_iomap(np, 1);
	if (!dev->hw_info.dmem_base) {
		pr_err("Unable to iomap registers\n");
		rc = -ENOMEM;
		goto of_iomap_dmem_err;
	}

	rc = of_address_to_resource(np, 1, &res);
	if (rc < 0) {
		pr_err("Unable to get resource\n");
		goto of_address_dmem_err;
	}

	dev->hw_info.ccu_dmem_base = res.start;
	dev->hw_info.ccu_dmem_size = res.end - res.start + 1;

	mtk_ccu_device_tree_parsing(dev, np);

	init_waitqueue_head(&dev->cmd_wait);
	atomic_set(&dev->refcnt, 0);

	major = MAJOR(ccu_group->ccu_devno);
	minor = ccu_group->ccu_device_index;
	dev->dev = device_create(
			ccu_group->ccu_class, NULL,
			MKDEV(major, minor), dev,
			"ccu%d", minor);
	if (IS_ERR(dev->dev)) {
		rc = PTR_ERR(dev->dev);
		pr_err("failed to create device: ccu%d", minor);
		goto device_create_err;
	}

	dev_set_drvdata(dev->dev, dev);
	rc = device_create_file(dev->dev, &dev_attr_info);
	if (rc)
		goto kalloc_ccu_user_err;

	dev->id = ccu_group->ccu_device_index;
	ccu_group->ccu_device_index++;
	list_add(&dev->list, &ccu_group->ccu_head);
	platform_set_drvdata(pdev, dev);
	dev->p_ccu_wq = ccu_group->ccu_shared_wq;

	user = kzalloc(sizeof(struct ccu_user), GFP_KERNEL);
	if (!user) {
		rc = -ENOMEM;
		goto kalloc_ccu_user_err;
	}

	mutex_init(&user->enque_mutex);
	mutex_init(&user->deque_mutex);
	INIT_LIST_HEAD(&user->enque_ccu_cmd_list);
	INIT_LIST_HEAD(&user->deque_ccu_cmd_list);
	init_completion(&user->deque_done);
	init_completion(&user->log_done);
	atomic_set(&user->log_idx, 0);

	dev->user = user;
	user->ccu_dev = dev;

	rc = mtk_ccu_init_hw(dev);
	if (rc < 0)
		goto mtk_ccu_init_hw_err;

	pr_info("(map_addr=0x%lx irq=%d 0x%llx->0x%llx)\n",
		(unsigned long)dev->hw_info.ccu_base,
		dev->hw_info.irq_num,
		dev->hw_info.ccu_hw_base,
		dev->hw_info.ccu_hw_base + dev->hw_info.range);

	return 0;

mtk_ccu_init_hw_err:
	kfree(user);
kalloc_ccu_user_err:
	device_destroy(ccu_group->ccu_class, MKDEV(major, minor));
device_create_err:
of_address_dmem_err:
	iounmap(dev->hw_info.dmem_base);
of_iomap_dmem_err:
of_address_ccu_base_err:
	iounmap(dev->hw_info.ccu_base);
of_iomap_ccu_base_err:
	kfree(dev);

	return rc;
}

static int mtk_ccu_remove(struct platform_device *pdev)
{
	struct ccu_device *dev = platform_get_drvdata(pdev);
	int major, minor;

	major = MAJOR(ccu_group->ccu_devno);
	minor = dev->id;
	device_destroy(ccu_group->ccu_class, MKDEV(major, minor));
	iounmap(dev->hw_info.ccu_base);
	iounmap(dev->hw_info.dmem_base);
	kfree(dev);

	return 0;
}

static const struct of_device_id ccu_of_ids[] = {
	{ .compatible = "mediatek,ccu,m", },
	{}
};

static struct platform_driver ccu_driver = {
	.probe = mtk_ccu_probe,
	.remove = mtk_ccu_remove,
	.driver = {
		.name = "ccu-drv",
		.owner = THIS_MODULE,
		.of_match_table = ccu_of_ids,
	}
};

static int __init mtk_ccu_init(void)
{
	int rc;

	ccu_group = kzalloc(sizeof(struct ccu_device_group), GFP_KERNEL);
	if (!ccu_group)
		return -ENOMEM;

	INIT_LIST_HEAD(&ccu_group->ccu_head);

	rc = alloc_chrdev_region(
		&ccu_group->ccu_devno, 0,
		MAX_CCU_DEVICES, "ccu");
	if (rc < 0)
		goto alloc_chrdev_region_err;

	ccu_group->ccu_device_index = 0;
	ccu_group->ccu_cdev = cdev_alloc();
	ccu_group->ccu_cdev->owner = THIS_MODULE;
	ccu_group->ccu_cdev->ops = &mtk_ccu_fops;
	rc = cdev_add(ccu_group->ccu_cdev,
		ccu_group->ccu_devno, MAX_CCU_DEVICES);
	if (rc < 0)
		goto cdev_add_err;

	ccu_group->ccu_class = class_create(THIS_MODULE, "mtk_ccu");
	if (IS_ERR(ccu_group->ccu_class)) {
		rc = PTR_ERR(ccu_group->ccu_class);
		goto cdev_add_err;
	}

	ccu_group->ccu_shared_wq = create_workqueue("mtk_ccu_wq");
	if (!ccu_group->ccu_shared_wq) {
		rc = -ENOMEM;
		goto create_workqueue_err;
	}

	rc = platform_driver_register(&ccu_driver);
	if (rc)
		goto platform_reg_err;

	return 0;

platform_reg_err:
	destroy_workqueue(ccu_group->ccu_shared_wq);
create_workqueue_err:
	class_destroy(ccu_group->ccu_class);
cdev_add_err:
	unregister_chrdev_region(ccu_group->ccu_devno, MAX_CCU_DEVICES);
alloc_chrdev_region_err:
	kfree(ccu_group);

	return rc;
}

static void __exit mtk_ccu_exit(void)
{
	platform_driver_unregister(&ccu_driver);
	destroy_workqueue(ccu_group->ccu_shared_wq);
	class_destroy(ccu_group->ccu_class);
	unregister_chrdev_region(ccu_group->ccu_devno, MAX_CCU_DEVICES);
	kfree(ccu_group);
}

module_init(mtk_ccu_init);
module_exit(mtk_ccu_exit);
MODULE_DESCRIPTION("MTK CCU Driver");
MODULE_LICENSE("GPL");
