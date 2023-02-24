/*
* Copyright (C) 2017 MediaTek Inc.
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
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/poll.h>
#include <linux/debugfs.h>

#include <dt-bindings/camera.h>

#include <soc/mediatek/mtk_dip_warp_rbsync.h>

#include "isp.h"
#include "isp-registers.h"

struct isp_dev_group *isp_group;

static int mtk_isp_open(struct inode *inode, struct file *filp)
{
	struct isp_dev *dev = mtk_isp_find_isp_dev_by_id(iminor(inode));
	struct isp_module *module = NULL;

	if (IS_ERR(dev))
		return PTR_ERR(dev);

	/* Since there might two paths (ex. sidell_IR and sidell_W) open
	 * the same device concurrently, we need mutex to make sure both
	 * paths have device init completed as expected.
	 */
	mutex_lock(&dev->dev_lock);
	if (atomic_inc_return(&dev->refcnt) == 1) {
		list_for_each_entry(module, &dev->isp_module_head, list) {
			if (module->ops->init)
				module->ops->init(module);
		}
	}
	mutex_unlock(&dev->dev_lock);

	filp->private_data = (void *)dev;
	pr_debug("id :%d\n", dev->dev_id);

	return 0;
}

static int mtk_isp_release(struct inode *inode, struct file *filp)
{
	struct isp_dev *dev = mtk_isp_find_isp_dev_by_id(iminor(inode));
	struct isp_module *module = NULL;
	int i;

	if (IS_ERR(dev))
		return PTR_ERR(dev);

	if (!atomic_dec_return(&dev->refcnt)) {
		list_for_each_entry(module, &dev->isp_module_head, list) {
			if (module->ops->stop)
				module->ops->stop(module);

			mutex_lock(&dev->slot_lock);
			for (i = 1; i < MAX_SLOT_SIZE; i++)
				module->event_slot[i].in_use = 0;
			mutex_unlock(&dev->slot_lock);
		}
	}

	filp->private_data = NULL;
	pr_debug("id :%d\n", dev->dev_id);

	return 0;
}

static int mtk_isp_mmap(struct file *filp, struct vm_area_struct *vm)
{
	struct isp_dev *dev = filp->private_data;
	struct isp_module *module;
	unsigned long length;
	phys_addr_t phy;
	int found = 0;

	length = (vm->vm_end - vm->vm_start);
	phy = vm->vm_pgoff << PAGE_SHIFT;

	list_for_each_entry(module, &dev->isp_module_head, list) {
		if (module->hw_info.start == phy &&
			module->hw_info.range >= length) {
			found = 1;
			break;
		}
	}

	if (found == 0)
		return -EAGAIN;

	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	if (remap_pfn_range(
		vm, vm->vm_start, vm->vm_pgoff,
		vm->vm_end - vm->vm_start, vm->vm_page_prot))
		return -EAGAIN;

	pr_debug("isp%d 0x%lx\n", dev->dev_id, (unsigned long)phy);

	return 0;
}

static unsigned int mtk_isp_poll(struct file *filp, poll_table *wait)
{
	struct isp_dev *dev = filp->private_data;
	struct isp_module *module = NULL;
	int ret = 0;

	if (IS_ERR(dev))
		return PTR_ERR(dev);

	list_for_each_entry(module, &dev->isp_module_head, list) {
		if (module->ops->check_poll_trigger) {
			poll_wait(filp, &module->wait_queue, wait);
			ret = module->ops->check_poll_trigger(filp, module);
			if (ret != 0)
				return (POLLIN|POLLRDNORM);
		}
	}
	return 0;
}

static ssize_t mtk_isp_read(struct file *filp, char __user *buff,
			    size_t count, loff_t *pos)
{
	struct isp_dev *dev = filp->private_data;
	struct isp_module *module = NULL;
	ssize_t cur = 0;
	unsigned int event;

	if (IS_ERR(dev))
		return PTR_ERR(dev);

	list_for_each_entry(module, &dev->isp_module_head, list) {
		if (module->ops->read_poll_trigger) {
			if (cur+sizeof(event) > count)
				return cur;
			event = module->ops->read_poll_trigger(filp, module);
			if (event == 0)
				continue;
			if (copy_to_user(buff+cur, &event, sizeof(event)) == 0)
				cur += sizeof(event);
			else {
				pr_err("mtk_isp_read copy_to_user failed\n");
				return -EFAULT;
			}
		}
	}
	return cur;
}

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
static int mtk_isp_ioctl_find_free_userkey(const struct isp_dev *dev)
{
	struct isp_module *module;
	int found = 0;
	int i;

	/* Please notice that the i starts from one rather than zero.
	 * The reason is that if upper layer does not call ioctl
	 * with ISP_REGISTER_IRQ_USER_KEY to obtain a key, it's still
	 * okay to call wait irq with key zero, because zero is for
	 * all of users by default.
	 */
	for (i = 1; i < MAX_SLOT_SIZE; i++) {
		list_for_each_entry(module, &dev->isp_module_head, list) {
			if (module->event_slot[i].in_use == 0)
				found = 1;
			else
				found = 0;
		}

		if (found == 1)
			return i;
	}

	return -1;
}
#endif

static inline int mtk_isp_ioctl_cmd_check(unsigned int cmd)
{
	switch (cmd) {
	case ISP_QUERY_HWMODULE_INFO:
	case ISP_RESET_BY_HWMODULE:
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	case ISP_WAIT_IRQ:
	case ISP_FLUSH_IRQ_REQUEST:
	case ISP_REGISTER_IRQ_USER_KEY:
	case ISP_QUERY_NUM_USER_KEY:
	case ISP_CLEAR_IRQ_REQUEST:
#endif
	case ISP_SET_POLL_TRIGGER:
			return 1;
	}

	return 0;
}

static long mtk_isp_ioctl(
	struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct isp_dev *dev = filp->private_data;
	struct isp_module *module = NULL;
	struct event_req *event = NULL;
	unsigned int dir;
	union cmd_data_t data;
	int ret = 0;
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	int i = 0;
#endif

	if (mtk_isp_ioctl_cmd_check(cmd)) {
		dir = _IOC_DIR(cmd);

		if (_IOC_SIZE(cmd) > sizeof(data))
			return -EINVAL;

		if (dir & _IOC_WRITE)
			if (copy_from_user(
				&data,
				(void __user *)arg,
				_IOC_SIZE(cmd))
			)
				return -EFAULT;

		switch (cmd) {
		case ISP_QUERY_HWMODULE_INFO:
			module = mtk_isp_find_isp_module_by_id(
					dev,
					data.module_id);
			if (IS_ERR(module)) {
				pr_err("failed to find module %d\n",
					data.module_id);
				return -EINVAL;
			}

			data.module_info.base = module->hw_info.start;
			data.module_info.range = module->hw_info.range;

			/*
			*pr_debug("%d 0x%lx 0x%lx\n",
			*	data.module_id, data.module_info.base,
			*	data.module_info.range);
			*/
			break;

		case ISP_RESET_BY_HWMODULE:
			module = mtk_isp_find_isp_module_by_id(
					dev,
					data.module_id);

			if (IS_ERR(module)) {
				pr_err("failed to find module %d\n",
					data.module_id);
				return -EINVAL;
			}

			if (module->ops->reset)
				module->ops->reset(module);

			pr_debug("reset module %d:%d\n",
				dev->dev_id, data.module_id);
			break;

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
		case ISP_WAIT_IRQ:
			event = &data.e_req;
			module = mtk_isp_find_isp_module_by_id(
					dev,
					event->module_id);

			if (IS_ERR(module))
				return -EINVAL;

			if (event->user_key >= MAX_SLOT_SIZE)
				return -EINVAL;

			if (event->user_key > 0 &&
				module->event_slot[event->user_key].slot_owner
				!= current->pid)
				return -EINVAL;

			/*
			*pr_debug("%s wait event:0x%lx with key:%d\n",
			*module->long_name, event->event, event->user_key);
			*/

			if (module->ops->wait_module_event)
				ret = module->ops->wait_module_event(
					module,
					event);
			break;

		case ISP_FLUSH_IRQ_REQUEST:
			event = &data.e_req;
			module = mtk_isp_find_isp_module_by_id(
					dev,
					event->module_id);

			if (IS_ERR(module))
				return -EINVAL;

			if (event->user_key >= MAX_SLOT_SIZE)
				return -EINVAL;

			if (event->user_key > 0 &&
				module->event_slot[event->user_key].slot_owner
				!= current->pid)
				return -EINVAL;

			pr_debug(
				"%s flush event:0x%lx with key:%d\n",
				module->name, event->event,
				event->user_key);

			if (module->ops->flush_module_event)
				ret = module->ops->flush_module_event(
					module,
					event);
			break;

		case ISP_CLEAR_IRQ_REQUEST:
			event = &data.e_req;
			module = mtk_isp_find_isp_module_by_id(
					dev,
					event->module_id);

			if (IS_ERR(module))
				return -EINVAL;

			if (event->user_key >= MAX_SLOT_SIZE)
				return -EINVAL;

			if (event->user_key > 0 &&
				module->event_slot[event->user_key].slot_owner
				!= current->pid)
				return -EINVAL;

			pr_debug(
				"%s clear event:0x%lx with key:%d\n",
				module->name, event->event,
				event->user_key);

			if (module->ops->clear_module_event)
				ret = module->ops->clear_module_event(
					module,
					event);
			break;

		case ISP_REGISTER_IRQ_USER_KEY:
			mutex_lock(&dev->slot_lock);
			i = mtk_isp_ioctl_find_free_userkey(dev);
			data.userkey_struct.userKey = i;
			if (i > 0) {
				list_for_each_entry(module,
					&dev->isp_module_head, list) {
					module->event_slot[i].in_use = 1;
					strncpy(
						module->event_slot[i].user_name,
						data.userkey_struct.userName,
						MAX_STR_SIZE
					);
					module->event_slot[i].slot_owner =
						current->pid;

					/*
					* pr_debug("%s %s:%d\n",
					*	module->long_name,
					*	module->event_slot[i].user_name,
					*	i);
					*/
				}
			}
			mutex_unlock(&dev->slot_lock);
			break;
		case ISP_QUERY_NUM_USER_KEY:
			data.num_slots = MAX_SLOT_SIZE;
			break;
#endif
		case ISP_SET_POLL_TRIGGER:
			event = &data.e_req;
			module = mtk_isp_find_isp_module_by_id(
					dev,
					event->module_id);
			if (IS_ERR(module))
				return -EINVAL;

			if (module->ops->set_poll_trigger)
				module->ops->set_poll_trigger(
						filp,
						module,
						event);
			break;
		}

		if (dir & _IOC_READ)
			if (copy_to_user(
				    (void __user *)arg, &data, _IOC_SIZE(cmd)))
				return -EFAULT;
	} else {
		/* Please make sure that module_id is put at the fisrt
		 * in the struct of your arg. and module developer needs
		 * to implement copy_from_user again in your custom_ioctl
		 * to get the correct and whole struture.
		 */
		if (copy_from_user(&data,
				(void __user *)arg, sizeof(unsigned int)))
			return -EFAULT;

		module = mtk_isp_find_isp_module_by_id(dev,
						data.module_id);
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
		ret = -EPERM;
#endif
		if (module->ops->custom_ioctl)
			ret = module->ops->custom_ioctl(module, cmd, arg);
	}

	return ret;
}

const struct file_operations mtk_isp_fops = {
	.owner = THIS_MODULE,
	.open = mtk_isp_open,
	.release = mtk_isp_release,
	.mmap = mtk_isp_mmap,
	.poll = mtk_isp_poll,
	.read = mtk_isp_read,
	.unlocked_ioctl = mtk_isp_ioctl,
};

#ifdef CONFIG_DEBUG_FS
static ssize_t mtk_isp_sysfs_show_info(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct isp_dev *isp_dev_ptr = dev_get_drvdata(dev);
	struct isp_module *module;
	char *out = buf;

	out += sprintf(out,
	"name\tvirt_base_addr\t\t\tphy_start_addr - phy_end_addr\trange\tIRQ\tID\n"
	);

	list_for_each_entry(module, &isp_dev_ptr->isp_module_head, list) {
		out += sprintf(out,
			"%s\t0x%lx\t\t0x%llx - 0x%llx\t\t0x%x\t%d\t0x%x\n",
			module->name, (unsigned long)module->hw_info.base_addr,
			module->hw_info.start, module->hw_info.end,
			module->hw_info.range, module->hw_info.irq,
			module->module_id);
	}

	return out - buf;
}

static DEVICE_ATTR(info, S_IRUGO, mtk_isp_sysfs_show_info, NULL);
#endif

static ssize_t mtk_isp_debugfs_reg_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct isp_module *module = file->private_data;
	unsigned long reg_off, reg_val;
	char buf[count];
	char *cur_buf = buf;

	if (simple_write_to_buffer(buf, count, ppos, user_buf, count) < 0)
		return -EINVAL;

	buf[count - 1] = ' ';

	/* echo "0x40 0x04" > reg is an example for the following parsing code.
	 */
	if (kstrtoul(strsep(&cur_buf, " "), 16, &reg_off))
		return -EINVAL;

	if (kstrtoul(strsep(&cur_buf, " "), 16, &reg_val))
		return -EINVAL;

	pr_debug("%s %s offset:0x%lx val:0x%lx\n", __func__, module->name,
		reg_off, reg_val);

	ISP_WR32(module->hw_info.base_addr + reg_off, reg_val);

	return count;
}

static const struct file_operations mtk_isp_debugfs_reg_rw_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.llseek = no_llseek,
	.write = mtk_isp_debugfs_reg_write,
};

static void *regs_dump_seq_start(struct seq_file *s, loff_t *pos)
{
	struct isp_module *module = s->private;

	if (*pos >= module->hw_info.range - ISP_REGISTER_WIDTH)
		return NULL;
	else if (*pos > 0)
		(*pos) -= 1;

	return pos;
}

static void *regs_dump_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct isp_module *module = s->private;

	if (*pos >= module->hw_info.range - ISP_REGISTER_WIDTH)
		return NULL;

	(*pos) += ISP_REGISTER_WIDTH;

	return pos;
}

static int regs_dump_seq_show(struct seq_file *s, void *v)
{
	struct isp_module *module = s->private;
	loff_t *pos = v;

	seq_printf(s, "0x%lx + 0x%llx : 0x%lx\n",
		(unsigned long)module->hw_info.start, *pos,
		(unsigned long)ISP_RD32(module->hw_info.base_addr + *pos));

	return 0;
}

static void regs_dump_seq_stop(struct seq_file *s, void *v)
{
	/* nothing for now */
}

static const struct seq_operations regs_dump_seq_ops = {
	.start = regs_dump_seq_start,
	.next = regs_dump_seq_next,
	.show = regs_dump_seq_show,
	.stop = regs_dump_seq_stop,
};

static int mtk_isp_debugfs_regs_dump_open(
	struct inode *inode, struct file *file)
{
	seq_open(file, &regs_dump_seq_ops);

	if (inode->i_private)
		((struct seq_file *)file->private_data)->private =
			inode->i_private;

	return 0;
}

static const struct file_operations mtk_isp_debugfs_regs_dump_fops = {
	.owner = THIS_MODULE,
	.open = mtk_isp_debugfs_regs_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int mtk_isp_unregister_isp_module(struct isp_module *module)
{
	struct isp_dev *dev;

	if (!module) {
		pr_err("failed to unregister isp_module to isp-core\n");
		return -EINVAL;
	}

	dev = mtk_isp_find_isp_dev_by_module(module);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	spin_lock(&dev->list_lock);
	list_del(&module->list);
	spin_unlock(&dev->list_lock);

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(module->root_dir);
#endif

	pr_info("%s is unregistered successfully to isp_dev %d\n",
		module->name, dev->dev_id);
	return 0;
}
EXPORT_SYMBOL(mtk_isp_unregister_isp_module);

int mtk_isp_register_isp_module(int id, struct isp_module *module)
{
	struct isp_dev *dev;
#ifdef CONFIG_DEBUG_FS
	struct dentry *dir;
	int ret = 0;
#endif

	if (!module) {
		pr_err("failed to register isp_module to isp-core\n");
		return -EINVAL;
	}

	if (id > isp_group->num) {
		pr_err("failed to register isp_module. id is incorrect. id:%d isp_group->num:%d\n",
		       id, isp_group->num);
		return -EINVAL;
	}

	dev = mtk_isp_find_isp_dev_by_id(id);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	spin_lock(&dev->list_lock);
	list_add_tail(&module->list, &dev->isp_module_head);
	spin_unlock(&dev->list_lock);

	module->event_slot[0].in_use = 1;
	snprintf(module->event_slot[0].user_name, sizeof(module->event_slot[0].user_name), "all_users");
	module->parent_dev = dev;
	spin_lock_init(&module->pdm_lock);

#ifdef CONFIG_DEBUG_FS
	dir = debugfs_create_dir(module->name, dev->root_dir);
	if (IS_ERR(dir)) {
		ret = PTR_ERR(dir);
		pr_err("Unable to create debugfs for module %s\n",
			module->name);
		goto debugfs_err;
	}

	debugfs_create_file(
		"dump_regs", 0444, dir,	module,
		&mtk_isp_debugfs_regs_dump_fops);
	debugfs_create_file(
		"reg", 0222, dir, module,
		&mtk_isp_debugfs_reg_rw_fops);

	module->root_dir = dir;
#endif

	pr_debug("%s is registered successfully to isp_dev %d\n",
		module->name, dev->dev_id);

	return 0;

#ifdef CONFIG_DEBUG_FS
/* Although there is something wrong on creating debugfs,
 * it does not impact the call flow between isp-dev and
 * isp-module.
 */
debugfs_err:
	return ret;
#endif
}
EXPORT_SYMBOL(mtk_isp_register_isp_module);

static int mtk_isp_register_cdev(struct isp_dev_group *group)
{
	int ret = 0;

	ret = alloc_chrdev_region(&group->isp_devno, 0, MAX_ISP_DEVICES, "isp");
	if (ret < 0) {
		pr_err("failed to alloc chrdev region\n");
		return ret;
	}

	group->isp_cdev = cdev_alloc();
	if (!group->isp_cdev) {
		ret = -ENOMEM;
		pr_err("cdev_alloc failed\n");
		goto cdev_add_err;
	}
	group->isp_cdev->owner = THIS_MODULE;
	group->isp_cdev->ops = &mtk_isp_fops;

	ret = cdev_add(group->isp_cdev, group->isp_devno, MAX_ISP_DEVICES);
	if (ret < 0) {
		pr_err("failed to add cdev\n");
		goto cdev_add_err;
	}

	return 0;

cdev_add_err:
	unregister_chrdev_region(group->isp_devno, MAX_ISP_DEVICES);

	return ret;
}

static int mtk_isp_unregister_cdev(struct isp_dev_group *group)
{
	cdev_del(group->isp_cdev);
	unregister_chrdev_region(group->isp_devno, MAX_ISP_DEVICES);

	return 0;
}

static int mtk_isp_probe(struct platform_device *pdev)
{
#ifdef CONFIG_DEBUG_FS
	char isp_name[MAX_STR_SIZE];
#endif
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	int major, minor;
	struct isp_dev *dev = NULL;
	int id;

	if (!np) {
		pr_err("No device node for mtk camera isp driver\n");
		return -ENODEV;
	}

	if (!isp_group) {
		pr_err("there are something wrong on module_init\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "id", &id)) {
		pr_err("Property 'mediatek,id' missing or invalid\n");
		return -EINVAL;
	}

	dev = kzalloc(sizeof(struct isp_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->dev_id = id;
	INIT_LIST_HEAD(&dev->isp_module_head);
	atomic_set(&dev->refcnt, 0);
	spin_lock_init(&dev->list_lock);
	mutex_init(&dev->slot_lock);
	mutex_init(&dev->dev_lock);

	list_add(&dev->list, &isp_group->isp_dev_head);
	isp_group->num++;

	major = MAJOR(isp_group->isp_devno);
	minor = dev->dev_id;
	dev->dev = device_create(isp_group->dev_class, NULL,
		MKDEV(major, minor), dev, "isp%d", minor);
	if (IS_ERR(dev->dev)) {
		ret = PTR_ERR(dev->dev);
		pr_err("failed to create device: isp%d", minor);
		goto dev_create_err;
	}

	dev_set_drvdata(dev->dev, dev);
#ifdef CONFIG_DEBUG_FS
	ret = device_create_file(dev->dev, &dev_attr_info);
	if (ret)
		goto dev_create_file_err;
#endif

#ifdef CONFIG_DEBUG_FS
	memset((void *)isp_name, '\0', MAX_STR_SIZE);
	sprintf(isp_name, "isp%d", dev->dev_id);

	dev->root_dir = debugfs_create_dir(isp_name, isp_group->debugfs_root);
	if (IS_ERR(dev->root_dir)) {
		ret = PTR_ERR(dev->root_dir);
		pr_err("Unable to create debugfs\n");
		goto debugfs_create_err;
	}
#endif

	platform_set_drvdata(pdev, dev);

	pr_info("isp%d probe successfully\n", dev->dev_id);

	return 0;

#ifdef CONFIG_DEBUG_FS
debugfs_create_err:
	device_remove_file(dev->dev, &dev_attr_info);
dev_create_file_err:
	device_destroy(isp_group->dev_class, MKDEV(major, minor));
#endif
dev_create_err:
	kfree(dev);

	return ret;
}

static int mtk_isp_remove(struct platform_device *pdev)
{
	struct isp_dev *dev = platform_get_drvdata(pdev);
	int major, minor;

	if (!list_empty(&dev->isp_module_head))
		return -EBUSY;

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(dev->root_dir);
	device_remove_file(dev->dev, &dev_attr_info);
#endif
	major = MAJOR(isp_group->isp_devno);
	minor = dev->dev_id;
	device_destroy(isp_group->dev_class, MKDEV(major, minor));

	kfree(dev);

	pr_info("isp%d is removed successfully\n", minor);

	return 0;
}

static const struct of_device_id isp_of_ids[] = {
	{ .compatible = "mediatek,isp",	},
	{}
};

static struct platform_driver isp_drvier = {
	.probe = mtk_isp_probe,
	.remove = mtk_isp_remove,
	.driver = {
		.name = ISP_DEV_GROUP_NAME,
		.owner = THIS_MODULE,
		.of_match_table = isp_of_ids,
	}
};

static int __init mtk_isp_init(void)
{
	int ret = 0;

	isp_group = kzalloc(sizeof(struct isp_dev_group), GFP_KERNEL);
	if (!isp_group)
		return -ENOMEM;

	isp_group->num = 0;
	isp_group->name = ISP_DEV_GROUP_NAME;
	INIT_LIST_HEAD(&isp_group->isp_dev_head);

	ret = mtk_isp_register_cdev(isp_group);
	if (ret < 0)
		goto reg_err;

	isp_group->dev_class = class_create(THIS_MODULE, "mtk_isp");
	if (IS_ERR(isp_group->dev_class)) {
		ret = PTR_ERR(isp_group->dev_class);
		pr_err("Unable to create class\n");
		goto class_err;
	}

#ifdef CONFIG_DEBUG_FS
	isp_group->debugfs_root = debugfs_create_dir("isp_group", NULL);
	if (IS_ERR(isp_group->debugfs_root)) {
		ret = PTR_ERR(isp_group->debugfs_root);
		pr_err("Unable to create debugfs\n");
		goto debugfs_err;
	}
#endif

	return platform_driver_register(&isp_drvier);

#ifdef CONFIG_DEBUG_FS
debugfs_err:
	class_destroy(isp_group->dev_class);
#endif
class_err:
	mtk_isp_unregister_cdev(isp_group);
reg_err:
	kfree(isp_group);

	return ret;
}

static void __exit mtk_isp_exit(void)
{
	platform_driver_unregister(&isp_drvier);
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(isp_group->debugfs_root);
#endif
	class_destroy(isp_group->dev_class);
	mtk_isp_unregister_cdev(isp_group);
	kfree(isp_group);
}

module_init(mtk_isp_init);
module_exit(mtk_isp_exit);
MODULE_DESCRIPTION("MTK Camera ISP Driver");
MODULE_LICENSE("GPL");
