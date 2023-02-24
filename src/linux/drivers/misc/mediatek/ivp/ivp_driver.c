/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

 /**
  * @file ivp_driver.c
  * This ivp driver is used to control IVP module.\n
  * It includes IVP firmware, ioctl, buffer manage and power contral.
  */

 /**
  * @defgroup IP_group_IVP IVP
  *   IVP driver initialize IVP firmware and defines 13 ioctl to control \n
  *   IVP device.
  *
  *   @{
  *       @defgroup IP_group_ivp_external EXTERNAL
  *         The external API document for ivp.
  *
  *         @{
  *             @defgroup IP_group_ivp_external_function 1.function
  *               This is ivp external API.
  *             @defgroup IP_group_ivp_external_struct 2.structure
  *               This is ivp external structure.
  *             @defgroup IP_group_ivp_external_typedef 3.typedef
  *               None.
  *             @defgroup IP_group_ivp_external_enum 4.enumeration
  *               This is ivp external enumeration.
  *             @defgroup IP_group_ivp_external_def 5.define
  *               This is ivp external definition used.
  *         @}
  *
  *       @defgroup IP_group_ivp_internal INTERNAL
  *         The internal API document for ivp.
  *
  *         @{
  *             @defgroup IP_group_ivp_internal_function 1.function
  *               This is ivp internal function and module init.
  *             @defgroup IP_group_ivp_internal_struct 2.structure
  *               None.
  *             @defgroup IP_group_ivp_internal_typedef 3.typedef
  *               None.
  *             @defgroup IP_group_ivp_internal_enum 4.enumeration
  *               None.
  *             @defgroup IP_group_ivp_internal_def 5.define
  *               This is ivp internal definition used.
  *         @}
  *   @}
  */

#include <linux/platform_device.h>

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/pm.h>
#include <linux/pm_opp.h>
#include <linux/pm_runtime.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/regulator/consumer.h>

#include <uapi/mediatek/ivp_ioctl.h>

#include "ivp_cmd_hnd.h"
#include "ivp_dbgfs.h"
#include "ivp_driver.h"
#include "ivp_dvfs.h"
#include "ivp_err_hnd.h"
#include "ivp_fw_interface.h"
#include "ivp_opp_ctl.h"
#include "ivp_profile.h"
#include "ivp_power_ctl.h"
#include "ivp_queue.h"
#include "ivp_reg.h"
#include "ivp_systrace.h"
#include "ivp_utilization.h"
#include "ivpbuf-core.h"
#include "ivpbuf-dma-memcpy.h"

/** @ingroup IP_group_ivp_internal_def
 *  @brief IVP definitions
 * @{
 */
#define IVP_DEV_NAME		"ivp"
#define CMD_WAIT_TIME_MS	(1000 * 1000)
#define IVP_CORE_SUSPEND_DELAY_MS 100

#define ENABLE_PROFIKER_TIMER   0
/**
 * @}
 */

static struct class *ivp_class;

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     IVP devcie open callback function.
 * @param[in]
 *     inode: a data structure that describes a filesystem object.
 * @param[in]
 *     flip: the file structure which include ivp_user.
 * @return
 *     0, successful to create ivp user. \n
 *     -ENOMEM, out of memory.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If creating IVP user fails, return -ENOMEM.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_open(struct inode *inode, struct file *flip)
{
	int ret = 0;
	struct ivp_user *user;
	struct ivp_device *ivp_device;

	ivp_device =
	    container_of(inode->i_cdev, struct ivp_device, ivp_chardev);

	mutex_lock(&ivp_device->init_mutex);
	if (!ivp_device->ivp_init_done) {
		ret = ivp_initialize(ivp_device);
		if (ret) {
			pr_err("fail to initialize ivp");
			mutex_unlock(&ivp_device->init_mutex);
			return ret;
		}
	}
	mutex_unlock(&ivp_device->init_mutex);

	ivp_create_user(&user, ivp_device);
	if (IS_ERR_OR_NULL(user)) {
		pr_err("fail to create user\n");
		return -ENOMEM;
	}

	flip->private_data = user;

	return ret;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     IVP devcie release callback function.
 * @param[in]
 *     inode: no use.
 * @param[in]
 *     flip: the file structure which include ivp_user.
 * @return
 *     0, successful to release. \n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_release(struct inode *inode, struct file *flip)
{
	struct ivp_user *user = flip->private_data;
	struct ivp_device *ivp_device;

	if (user) {
		ivp_device = dev_get_drvdata(user->dev);
		ivp_check_dbg_buf(user);
		ivp_delete_user(user, ivp_device);
	} else {
		pr_err("delete empty user!\n");
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     mmap callback function of IVP.
 * @param[in]
 *     flip: the file structure which include ivp_user.
 * @param[in]
 *     vma: The structure of a memory VMM memory area.
 * @return
 *     0, successful to do mmap. \n
 *     error code from ivp_manage_mmap().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If ivp_manage_mmap() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_mmap(struct file *flip, struct vm_area_struct *vma)
{
	int ret = 0;
	struct ivp_user *user = flip->private_data;
	struct ivp_device *ivp_device = dev_get_drvdata(user->dev);

	ret = ivp_manage_mmap(ivp_device->ivp_mng, vma);
	if (ret)
		return ret;

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     poll callback function of IVP.
 * @param[in]
 *     flip: the file structure which include ivp_user.
 * @param[in]
 *     wait: the poll_table to be added the wait_queue.
 * @return
 *     0, nothing to deque. \n
 *     POLLIN | POLLPRI, something to deque.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If nothing to deque, return 0.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int ivp_poll(struct file *flip, poll_table *wait)
{
	struct ivp_user *user = flip->private_data;
	struct ivp_device *ivp_device;
	unsigned int mask = 0;
	int core;
	bool deque = false;

	if (!user)
		return POLLNVAL;
	ivp_device = dev_get_drvdata(user->dev);

	poll_wait(flip, &user->poll_wait_queue, wait);

	for (core = 0; core < ivp_device->ivp_core_num; core++) {
		mutex_lock(&user->data_mutex[core]);
		if (!list_empty(&user->deque_list[core])) {
			deque = true;
			mutex_unlock(&user->data_mutex[core]);
			break;
		}
		mutex_unlock(&user->data_mutex[core]);
	}

	if (deque)
		mask = POLLIN | POLLPRI;

	return mask;

}

/** @ingroup IP_group_ivp_internal_struct
 * @brief This is file operations of IVP device.
 */
static const struct file_operations ivp_fops = {
	.owner = THIS_MODULE,
	.open = ivp_open,
	.release = ivp_release,
	.mmap = ivp_mmap,
	.poll = ivp_poll,
	.unlocked_ioctl = ivp_ioctl
};

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Unregister IVP character device.
 * @param[out]
 *     ivp_device: the structure of ivp device.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline void ivp_unreg_chardev(struct ivp_device *ivp_device)
{
	cdev_del(&ivp_device->ivp_chardev);
	unregister_chrdev_region(ivp_device->ivp_devt, 1);
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Register IVP character device.
 * @param[out]
 *     ivp_device: the structure of ivp device.
 * @return
 *     0, successful to register IVP character device. \n
 *     error code from alloc_chrdev_region(). \n
 *     error code from cdev_add().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If alloc_chrdev_region() fails, return its error code. \n
 *     If cdev_add() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline int ivp_reg_chardev(struct ivp_device *ivp_device)
{
	int ret = 0;

	ret = alloc_chrdev_region(&ivp_device->ivp_devt, 0, 1, IVP_DEV_NAME);
	if ((ret) < 0) {
		pr_err("alloc_chrdev_region failed, %d\n", ret);
		return ret;
	}

	/* Attatch file operation. */
	cdev_init(&ivp_device->ivp_chardev, &ivp_fops);
	ivp_device->ivp_chardev.owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(&ivp_device->ivp_chardev, ivp_device->ivp_devt, 1);
	if ((ret) < 0) {
		pr_err("Attatch file operation failed, %d\n", ret);
		goto out;
	}

out:
	if (ret < 0)
		ivp_unreg_chardev(ivp_device);

	return ret;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Initialize IVP core firmware.
 * @param[out]
 *     ivp_core: the structure of ivp core.
 * @return
 *     0, successful to initialize ivp core firmware. \n
 *     -ENOENT, fail to get resource. \n
 *     -EINVAL, fail to get ivp core fiwrmware information.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If loading IVP core firmware fails, return -ENOENT. \n
 *     If getting IVP core firmware information fails, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_init_fw(struct ivp_core *ivp_core)
{
	int ret = 0;
	char fw_name[16];

	/* init mutex and cmd waitqueue, need to remove form fw load */
	mutex_init(&ivp_core->cmd_mutex);
	mutex_init(&ivp_core->dbg_mutex);
	init_waitqueue_head(&ivp_core->cmd_wait);
	init_waitqueue_head(&ivp_core->req_wait);

	mutex_init(&ivp_core->power_mutex);

	ret = sprintf(fw_name, "%s.bin", ivp_core->core_name);
	if (ret < 0)
		return -EINVAL;
	if (ivp_load_firmware(ivp_core, fw_name))
		return -ENOENT;

	ivp_core->prepared = true;

	ret = pm_runtime_get_sync(ivp_core->dev);
	if (ret < 0) {
		dev_err(ivp_core->dev,
			"%s: pm_runtime_get_sync fail.\n", __func__);
		return ret;
	}

	if (ivp_get_fwinfo(ivp_core)) {
		dev_err(ivp_core->dev, "ivp get fwinfo fail.\n");
		ret = -EINVAL;
		goto err_core_power_off;
	}

	ivp_core->state = IVP_IDLE;
	return 0;

err_core_power_off:
	ivp_core_set_auto_suspend(ivp_core, 1);

	return ret;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Initialize IVP core enque task.
 * @param[out]
 *     ivp_core: the structure of ivp core.
 * @return
 *     0, successful to initialize ivp core enque task. \n
 *     -ENOENT, fail to get resource.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If creating ivp core enque thread fails, return -ENOENT.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_init_queue_task(struct ivp_core *ivp_core)
{
	ivp_core->enque_task = kthread_create(ivp_enque_routine_loop,
					      ivp_core,
					      ivp_core->core_name);
	if (IS_ERR(ivp_core->enque_task)) {
		ivp_core->enque_task = NULL;
		dev_err(ivp_core->dev, "create enque_task kthread error.\n");
		return -ENOENT;
	}

	{
		struct sched_param param = {.sched_priority = 20 };
		sched_setscheduler(ivp_core->enque_task, SCHED_RR, &param);
	}

	wake_up_process(ivp_core->enque_task);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Initialize IVP kservice.
 * @param[out]
 *     ivp_core: the structure of ivp core.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void ivp_init_kservice(struct ivp_core *ivp_core)
{
	ivp_core->kservice_state = IVP_KSERVICE_STOP;
	INIT_LIST_HEAD(&ivp_core->kservice_request_list);
	mutex_init(&ivp_core->kservice_mutex);
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Initialize IVP device and firmware.
 * @param[out]
 *     ivp_device: the structure of ivp device.
 * @return
 *     0, successful to initialize ivp device. \n
 *     error code from ivp_buf_manage_init(). \n
 *     error code from ivpdma_request_dma_channel(). \n
 *     error code from ivp_device_power_on(). \n
 *     error code from ivp_init_fw(). \n
 *     error code from ivp_init_devfreq().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If ivp_buf_manage_init() fails, return its error code. \n
 *     If ivpdma_request_dma_channel() fails, return its error code. \n
 *     If ivp_device_power_on() fails, return its error code. \n
 *     If ivp_init_fw() fails, return its error code. \n
 *     If ivp_init_devfreq() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int ivp_initialize(struct ivp_device *ivp_device)
{
	int ret, core_id;
	struct device *dev = ivp_device->dev;

	if (ivp_device->ivp_init_done)
		return 0;

	ret = ivp_buf_manage_init(ivp_device);
	if (ret < 0) {
		dev_err(dev, "Failed to do ivp buffer init %d\n", ret);
		return ret;
	}

	ret = ivpdma_request_dma_channel(ivp_device);
	if (ret)
		dev_err(dev, "Failed to request dma_channel %d\n", ret);

	mutex_init(&ivp_device->power_mutex);
	ret = ivp_device_power_on(ivp_device);
	if (ret) {
		dev_err(dev, "fail to ivp device power on : %d\n", ret);
		goto err_release_dma_channel;
	}

	ivp_device_set_init_voltage(ivp_device);
	ivp_device_set_init_frequency(ivp_device);

	ivp_init_profiler(ivp_device, ENABLE_PROFIKER_TIMER);
	/* init hw and create task */
	for (core_id = 0; core_id < ivp_device->ivp_core_num; core_id++) {
		struct ivp_core *ivp_core = ivp_device->ivp_core[core_id];

		if (!ivp_core)
			continue;
		ivp_core->ivp_device = ivp_device;
		ivp_core->core = core_id;
		sprintf(ivp_core->core_name, "ivp%d_%s", ivp_core->core,
			ivp_device->board_name);
		ret = ivp_init_fw(ivp_core);
		if (ret < 0) {
			dev_err(ivp_core->dev,
				"Failed to do ivp_init_fw %d : %d\n",
				core_id, ret);
			goto err_power_off;
		}
		ivp_core_set_init_frequency(ivp_core);
		ivp_init_queue_task(ivp_core);
		ivp_init_kservice(ivp_core);
#ifdef CONFIG_MTK_IVP_DVFS
		/* init devfreq */
		ret = ivp_init_devfreq(ivp_core);
		if (ret) {
			dev_err(ivp_core->dev,
				"Failed to do ivp_init_devfreq: %d\n",
				ret);
			goto err_power_off;
		}
#endif /* CONFIG_MTK_IVP_DVFS */
		if (ivp_device->set_autosuspend)
			ivp_core_set_auto_suspend(ivp_core, 0);
	}

	ivp_device->ivp_init_done = true;

	return 0;

err_power_off:
	ivp_device_power_off(ivp_device);
err_release_dma_channel:
	ivpdma_release_dma_channel(ivp_device);
	ivp_buf_manage_deinit(ivp_device);

	return ret;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     This is the probe function of IVP core \n
 *     for initializing IVP core driver and device resource.
 * @param[in]
 *     pdev: platform device that denotes the information of IVP core.
 * @return
 *     0, successful to probe ivp core. \n
 *     -ENOMEM, out of memory. \n
 *     -ENOENT, fail to getting resource. \n
 *     error code from platform_get_irq(). \n
 *     error code from devm_request_irq().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If device memory allocate fails, return -ENOMEM. \n
 *     If getting resource fails, return -ENOENT. \n
 *     If platform_get_irq() fails, return its error code. \n
 *     If devm_request_irq() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_ivp_core_probe(struct platform_device *pdev)
{
	int iomem_id, irq, ret;
	struct resource *mem;
	struct ivp_core *ivp_core;
	struct ivp_err_inform *ivp_err;
	void __iomem *iomem[IVP_IOMEM_MAX];
	struct device *dev = &pdev->dev;
	const char *board_ver;
	const void *ptr;

	ivp_core = devm_kzalloc(dev, sizeof(*ivp_core), GFP_KERNEL);
	if (!ivp_core)
		return -ENOMEM;

	/* init ivp error*/
	ivp_err = devm_kzalloc(dev, sizeof(*ivp_err), GFP_KERNEL);
	if (!ivp_err)
		return -ENOMEM;
	ivp_err->uf = devm_kzalloc(dev, sizeof(*ivp_err->uf), GFP_KERNEL);
	if (!ivp_err->uf)
		return -ENOMEM;

	of_property_read_string(dev->of_node, "compatible", &board_ver);

	for (iomem_id = 0; iomem_id < IVP_IOMEM_MAX; iomem_id++) {
		mem = platform_get_resource(pdev, IORESOURCE_MEM, iomem_id);
		if (!mem) {
			dev_err(dev, "cannot get IORESOUCE_MEM\n");
			return -ENOENT;
		}

		if (iomem_id == IVP_IOMEM_CTRL)
			ivp_core->trg.reg_base = (u32)mem->start;

		iomem[iomem_id] = devm_ioremap_resource(dev, mem);
		if (IS_ERR((const void *)(iomem[iomem_id]))) {
			dev_err(dev, "cannot get ioremap %d\n",
				iomem_id);
			return -ENOENT;
		}
	}

	ptr = of_get_property(dev->of_node, "gce-subsys", NULL);
	if (ptr)
		ivp_core->trg.subsys = be32_to_cpup(ptr);
	else
		ivp_core->trg.subsys = 0;

	ivp_core->ivpctrl_base = iomem[IVP_IOMEM_CTRL];
	ivp_core->ivpdebug_mem = platform_get_resource(pdev,
				 IORESOURCE_MEM, IVP_IOMEM_CTRL + 1);
	if (!ivp_core->ivpdebug_mem) {
		dev_err(dev, "cannot get IORESOUCE_MEM(DEBUG)\n");
		ivp_core->ivpdebug_mem = NULL;
	} else {
		ivp_core->ivpdebug_base =
			devm_ioremap_resource(dev,
					      ivp_core->ivpdebug_mem);
		if (IS_ERR(ivp_core->ivpdebug_base))
			dev_err(dev, "cannot map ivpdebug_base\n");
		else
			ivp_core->enable_profiler = 1;
	}

	ivp_core->state = IVP_UNPREPARE;
	ivp_core->prepared = false;
	ivp_core->ptrace_log_start = 0;

	ret = ivp_core_setup_clk_resource(dev, ivp_core);
	if (ret)
		return ret;

	/* add opp table */
	if (dev_pm_opp_of_add_table(dev))
		dev_warn(dev, "no OPP table for ivp_core\n");

	/* interrupt resource */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ivp_core->irq = irq;

	pm_runtime_set_autosuspend_delay(dev, IVP_CORE_SUSPEND_DELAY_MS);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);

	ivp_core->dev = &pdev->dev;
	ivp_core->ivp_err = ivp_err;
	platform_set_drvdata(pdev, ivp_core);
	dev_set_drvdata(dev, ivp_core);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     This is the remove function of IVP core \n
 *     for removing IVP core driver and device resource.
 * @param[in]
 *     pdev: platform device that denotes the information of IVP core.
 * @return
 *     0, successful to remove ivp driver.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_ivp_core_remove(struct platform_device *pdev)
{
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	dev_pm_opp_of_remove_table(&pdev->dev);
	return 0;
}

/** @ingroup IP_group_ivp_internal_struct
 * @brief IVP core Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id mtk_ivp_core_of_ids[] = {
	{.compatible = "mediatek,mt3612-ivp-core",},
	{}
};

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     runtime suspend callback function of IVP core.
 * @param[in]
 *     dev: IVP core device node.
 * @return
 *     0, successful to resume IVP core.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_core_runtime_suspend(struct device *dev)
{
	struct ivp_core *ivp_core = dev_get_drvdata(dev);

	if (!ivp_core->prepared)
		return 0;

	devm_free_irq(dev, ivp_core->irq, ivp_core);

	mutex_lock(&ivp_core->power_mutex);
	ivp_core_power_off(ivp_core);
	ivp_core->state = IVP_UNPREPARE;
	ivp_core->is_suspended = true;
	mutex_unlock(&ivp_core->power_mutex);

	ivp_disable_counter(ivp_core);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     runtime resume callback function of IVP core.
 * @param[in]
 *     dev: IVP core device node.
 * @return
 *     0, successful to resume IVP core. \n
 *     error code from ivp_check_fw_boot_status().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If ivp_check_fw_boot_status() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_core_runtime_resume(struct device *dev)
{
	struct ivp_core *ivp_core = dev_get_drvdata(dev);
	int ret = 0;

	if (!ivp_core->prepared)
		return 0;

	ivp_atrace_init();

	mutex_lock(&ivp_core->power_mutex);
	ivp_atrace_begin("ivp_core_power_on");
	ret = ivp_core_power_on(ivp_core);
	ivp_atrace_end("ivp_core_power_on");
	if (ret) {
		dev_err(dev, "ivp_core_power_on fail: %d.\n", ret);
		goto err_mutex_unlock;
	}

	ivp_set_reg32(ivp_core->ivpctrl_base, VPU_XTENSA_INT, IVP_INT_XTENSA);
	ret = devm_request_irq(dev, ivp_core->irq, ivp_isr_handler,
			       IRQF_TRIGGER_NONE,
			       dev_name(dev),
			       ivp_core);
	if (ret < 0) {
		dev_err(dev, "Failed to request irq %d: %d\n", ivp_core->irq,
			ret);
		goto err_core_power_off;
	}

	ivp_core_reset(ivp_core);
	ivp_atrace_begin("ivp_hw_boot_sequence");
	ivp_hw_boot_sequence(ivp_core);
	ivp_atrace_end("ivp_hw_boot_sequence");

	ivp_atrace_begin("ivp_check_fw_boot_status");
	ret = ivp_check_fw_boot_status(ivp_core);
	ivp_atrace_end("ivp_check_fw_boot_status");
	if (ret) {
		dev_err(dev, "ivp firmware boot fail: %d.\n", ret);
		goto err_core_power_off;
	}
	ivp_core->state = IVP_IDLE;
	ivp_core->is_suspended = false;

	mutex_unlock(&ivp_core->power_mutex);

	return ret;

err_core_power_off:
	ivp_core_power_off(ivp_core);
	ivp_core->state = IVP_UNPREPARE;
	ivp_core->is_suspended = true;

err_mutex_unlock:
	mutex_unlock(&ivp_core->power_mutex);

	return ret;
}

/** @ingroup IP_group_ivp_internal_struct
 * @brief IVP core device PM callbacks.
 */
static const struct dev_pm_ops ivp_core_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
	.runtime_suspend = ivp_core_runtime_suspend,
	.runtime_resume = ivp_core_runtime_resume,
	.runtime_idle = NULL,
};

/** @ingroup IP_group_ivp_internal_struct
 * @brief IVP core platform driver structure. \n
 * This structure is used to register IVP core.
 */
static struct platform_driver mtk_ivp_core_driver = {
	.probe = mtk_ivp_core_probe,
	.remove = mtk_ivp_core_remove,
	.driver = {
		   .name = "mediatek,ivp-core",
		   .of_match_table = mtk_ivp_core_of_ids,
		   .pm = &ivp_core_pm_ops,
		  }
};

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     Set up the resource of IVP device and get IVP core data.
 * @param[in]
 *     pdev: platform device that denotes the information of IVP device.
 * @param[out]
 *     ivp_device: the structure of IVP device.
 * @return
 *     0, successful to set IVP resource. \n
 *     -ENOENT, fail to get resource. \n
 *     -ENOMEM, out of memory. \n
 *     -EINVAL, wrong input parameters. \n
 *     -EPROBE_DEFER, driver requests probe retry
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If getting resource fails, return -ENOENT. \n
 *     If creat pll_data fails, return -ENOMEM. \n
 *     If getting the device or the number of IVP core fail, return -EINVAL. \n
 *     If getting IVP core fails, return -EPROBE_DEFER.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_setup_resource(struct platform_device *pdev,
			      struct ivp_device *ivp_device)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct device_node *core_node;
	struct platform_device *core_pdev;
	int i, ret;

	ret = of_property_read_string(dev->of_node, "board_name",
				      &ivp_device->board_name);
	if (ret) {
		dev_err(dev, "parsing board_name error: %d\n", ret);
		return -EINVAL;
	}

	/* vcore_base */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "cannot get IORESOUCE_MEM 0\n");
		return -ENOENT;
	}
	ivp_device->vcore_base = devm_ioremap_resource(dev, res);
	if (IS_ERR((const void *)(ivp_device->vcore_base))) {
		dev_err(dev, "cannot get ioremap vcore_base\n");
		return -ENOENT;
	}

	/* conn_bass */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(dev, "cannot get IORESOUCE_MEM 1\n");
		return -ENOENT;
	}
	ivp_device->conn_base = devm_ioremap_resource(dev, res);
	if (IS_ERR((const void *)(ivp_device->conn_base))) {
		dev_err(dev, "cannot get ioremap conn_base\n");
		return -ENOENT;
	}

	ret = ivp_setup_clk_resource(dev, ivp_device);
	if (ret)
		return ret;

#if defined(CONFIG_REGULATOR)
	ivp_device->regulator = devm_regulator_get(dev, "vivp");
	if (IS_ERR(ivp_device->regulator)) {
		dev_err(dev, "cannot get regulator vivp\n");
		return PTR_ERR(ivp_device->regulator);
	}
	ivp_device->sram_regulator = devm_regulator_get(dev, "vsram");
	if (IS_ERR(ivp_device->sram_regulator)) {
		if (PTR_ERR(ivp_device->sram_regulator) != -EPROBE_DEFER)
			dev_err(dev, "cannot get regulator vsram\n");
		return PTR_ERR(ivp_device->sram_regulator);
	}
#endif

	ret = of_property_read_u32(dev->of_node, "sub_hw_nr",
				   &ivp_device->ivp_core_num);
	if (ret) {
		dev_err(dev, "parsing sub_hw_nr error: %d\n", ret);
		return -EINVAL;
	}

	if (ivp_device->ivp_core_num > IVP_CORE_NUM)
		return -EINVAL;

	for (i = 0; i < ivp_device->ivp_core_num; i++) {
		struct ivp_core *ivp_core = NULL;

		core_node = of_parse_phandle(dev->of_node,
					     "mediatek,ivpcore", i);
		if (!core_node) {
			dev_err(dev,
				"Missing <mediatek,ivpcore> phandle\n");
			return -EINVAL;
		}

		core_pdev = of_find_device_by_node(core_node);
		if (core_pdev)
			ivp_core = platform_get_drvdata(core_pdev);

		if (!ivp_core) {
			dev_warn(dev, "Waiting for ivp core %s\n",
				 core_node->full_name);
			of_node_put(core_node);
			return -EPROBE_DEFER;
		}
		of_node_put(core_node);
		/* attach ivp_core */
		ivp_device->ivp_core[i] = ivp_core;
	}

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     This is the probe function of IVP device \n
 *     for initializing IVP driver and device resource.
 * @param[in]
 *     pdev: platform device that denotes the information of IVP device.
 * @return
 *     0, successful to probe ivp driver. \n
 *     -ENOMEM, out of memory. \n
 *     error code from ivp_setup_resource(). \n
 *     error code from ivp_init_fw().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If device memory allocate fails, return -ENOMEM. \n
 *     If ivp_setup_resource() fails, return its error code. \n
 *     If ivp_init_fw() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_probe(struct platform_device *pdev)
{
	struct ivp_device *ivp_device;
	struct device *dev = &pdev->dev;
	int ret;

	ivp_device = devm_kzalloc(dev, sizeof(*ivp_device), GFP_KERNEL);
	if (!ivp_device)
		return -ENOMEM;

	ret = ivp_setup_resource(pdev, ivp_device);
	if (ret)
		return ret;

	/* command execution timeout setting */
	ivp_device->timeout = CMD_WAIT_TIME_MS;
	ivp_device->timeout2reset = true;

	ivp_device->set_autosuspend = true;

	mutex_init(&ivp_device->init_mutex);
	ivp_device->ivp_init_done = false;

	INIT_LIST_HEAD(&ivp_device->user_list);
	mutex_init(&ivp_device->user_mutex);
	ivp_device->ivp_num_users = 0;
	ivp_device->dev = &pdev->dev;

	/* init hrtimer */
	ret = ivp_init_util(ivp_device);
	if (ret) {
		dev_err(dev, "Failed to do ivp_init_util: %d\n", ret);
		return ret;
	}

	/* add opp table */
	ret = dev_pm_opp_of_add_table(dev);
	if (ret) {
		ivp_device->opp_clk_table = NULL;
		dev_warn(dev, "no OPP table for ivp\n");
	} else {
		/* create opp-clock table */
		ret = ivp_init_opp_clk_table(ivp_device);
		if (ret) {
			dev_err(dev, "Failed to initialize opp_clk_table:%d\n",
				ret);
		} else {
			ret = ivp_adjust_voltage(ivp_device);
			if (ret == -EPROBE_DEFER) {
				dev_warn(dev, "wait pmicw p2p\n");
				return ret;
			}
		}
	}

	if (ivp_reg_chardev(ivp_device) == 0) {
		/* Create class register */
		ivp_class = class_create(THIS_MODULE, IVP_DEV_NAME);
		if (IS_ERR(ivp_class)) {
			ret = PTR_ERR(ivp_class);
			dev_err(dev, "Unable to create class, err = %d\n", ret);
			goto dev_out;
		}

		dev = device_create(ivp_class, NULL, ivp_device->ivp_devt,
				    NULL, IVP_DEV_NAME);
		if (IS_ERR(dev)) {
			ret = PTR_ERR(dev);
			dev_err(dev,
				"Failed to create device: /dev/%s, err = %d",
				IVP_DEV_NAME, ret);
			goto dev_out;
		}

		platform_set_drvdata(pdev, ivp_device);
		dev_set_drvdata(dev, ivp_device);
		ivp_create_sysfs(dev);
	}

	return 0;

dev_out:
	ivp_unreg_chardev(ivp_device);
	ivp_deinit_util(ivp_device);

	return ret;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     This is the remove function of IVP device \n
 *     for removing IVP driver and device resource.
 * @param[in]
 *     pdev: platform device that denotes the information of IVP device.
 * @return
 *     0, successful to remove ivp driver.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_remove(struct platform_device *pdev)
{
	struct ivp_device *ivp_device = platform_get_drvdata(pdev);
	int core;

	if (ivp_device->ivp_init_done) {
		ivp_deinit_profiler(ivp_device);
		ivp_device_power_off(ivp_device);
		ivpdma_release_dma_channel(ivp_device);
		ivp_buf_manage_deinit(ivp_device);
		for (core = 0; core < ivp_device->ivp_core_num; core++) {
			struct ivp_core *ivp_core = ivp_device->ivp_core[core];

#ifdef CONFIG_MTK_IVP_DVFS
			ivp_deinit_devfreq(ivp_core);
#endif /* CONFIG_MTK_IVP_DVFS */
			ivp_core->prepared = false;
		}
	}

	ivp_device->ivp_init_done = false;

	ivp_deinit_util(ivp_device);
	ivp_unreg_chardev(ivp_device);
	dev_pm_opp_of_remove_table(ivp_device->dev);
	device_destroy(ivp_class, ivp_device->ivp_devt);
	class_destroy(ivp_class);

	ivp_remove_sysfs(&pdev->dev);

	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     This is the suspend function of IVP device.
 * @param[in]
 *     pdev: platform device that denotes the information of IVP device.
 * @param[in]
 *     mesg: platform device message.
 * @return
 *     0, successful to suspend ivp driver.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     This is the resume function of IVP device.
 * @param[in]
 *     pdev: platform device that denotes the information of IVP device.
 * @return
 *     0, successful to resume ivp driver.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int ivp_resume(struct platform_device *pdev)
{
	return 0;
}

/** @ingroup IP_group_ivp_internal_struct
 * @brief IVP Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for using with device tree.
 */
static const struct of_device_id ivp_of_ids[] = {
	{.compatible = "mediatek,mt3612-ivp",},
	{}
};

/** @ingroup IP_group_ivp_internal_struct
 * @brief IVP platform driver structure. \n
 * This structure is used to register IVP.
 */
static struct platform_driver ivp_driver = {
	.probe = ivp_probe,
	.remove = ivp_remove,
	.suspend = ivp_suspend,
	.resume = ivp_resume,
	.driver = {
		   .name = IVP_DEV_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = ivp_of_ids,
	}
};

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     IVP driver initialization entry function.\n
 *     To be run at kernel boot time or module insertion.
 * @par Parameters
 *     none.
 * @return
 *     0, IVP initialize success. \n
 *     error code from platform_driver_register().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If register IVP core driver fails, return -ENODEV.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __init IVP_INIT(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_ivp_core_driver);
	if (ret != 0) {
		pr_err("Failed to register ivp core driver\n");
		return -ENODEV;
	}

	ret = platform_driver_register(&ivp_driver);
	if (ret != 0) {
		pr_err("failed to register IVP driver");
		goto err_unreg_ivp_core;
	}

	return ret;
err_unreg_ivp_core:
	platform_driver_unregister(&mtk_ivp_core_driver);
	return ret;
}

/** @ingroup IP_group_ivp_internal_function
 * @par Description
 *     IVP driver exit entry function.\n
 *     To be run when driver is removed.
 * @par Parameters
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __exit IVP_EXIT(void)
{
	platform_driver_unregister(&ivp_driver);
	platform_driver_unregister(&mtk_ivp_core_driver);
}

module_init(IVP_INIT);
module_exit(IVP_EXIT);
MODULE_LICENSE("GPL");

