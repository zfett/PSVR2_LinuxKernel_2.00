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

/*****************************************************************
 * camera_gce.c - MT3612 Linux GCE Device Driver
 *
 * DESCRIPTION:
 *     This file provids GCE functionality which is implemented
 *     by using CMDQ driver APIs.
 *
 ******************************************************************/

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/compat.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <dt-bindings/gce/mt3612-gce.h>
#include "camera_gce.h"

/********************************
 **   Definitions and Macros   **
 ********************************/
#define CAMGCE_WR32(addr, data) iowrite32(data, (void *)(addr))
#define CAMGCE_RD32(addr) ioread32((void *)(addr))
#define CAMGCE_DEV_NAME "mtk-camera-gce"

/***************************************
 **    Enumerations and Structures    **
 ***************************************/
struct camgce_user_id {
	pid_t pid;
	pid_t tid;
};

struct camgce_user_info {
	spinlock_t spinlock_ref; /*spinlock for open/release gce device*/
	int usercount;
	int irq;
};

/* camera device priv info. */
struct camgce_priv {
	const char *name;
	unsigned int dev_idx;
};

static const struct of_device_id camgce_of_ids[] = {
	{.compatible = "mediatek,mt3612-camgce"},
	{}
};

/*
 * camera gce device information
 */
#define SPRMAX (2)
struct sprmem {
	u32 *va;
	dma_addr_t pa;
};

struct camgce_platform {
	struct cdev cdev;
	dev_t dev_t;
};

struct camgce_cbdata {
	struct camgce_device *dev;
	struct cmdq_pkt *pkt;
};

struct cmdq_async_info {
	u32 flush_event;
	u32 flush_count;
	u32 cb_done;
	spinlock_t spinlock_cb; /*for callback data r/w*/
};

struct cam_cmdq {
	struct cmdq_client *cmdq_client;
	u32 subsys;
	u32 subsys_base;
	struct cmdq_async_info async_info;
	/*struct sprmem g_sprval[SPRMAX];*/
};

struct camgce_device {
	struct device *dev;
	struct camgce_user_info uinfo;
	struct camgce_user_id uid;
	void __iomem *regs[CAMGCE_SGZ_MAX_NUM];
	int occupied;

	/* character device structure */
	struct camgce_platform platform;

	/* priv data for each cam mem device */
	struct camgce_priv *priv;

	/* cmdq client driver */
	struct cam_cmdq *cmdq[CAMGCE_SGZ_MAX_NUM];

	/* register ranges */
	struct resource res[CAMGCE_SGZ_MAX_NUM];
	unsigned int res_num;
};

static struct class *camgce_cls;
static struct camgce_device *camgce_devs;
static int camgce_dev_nr;

static long
of_get_cam_pa(struct device_node *dev, int index)
{
	struct resource res;

	if (of_address_to_resource(dev, index, &res))
		return 0;
	return res.start;
}

static int
mtk_camgce_pkt_create(struct cmdq_pkt **cmdq)
{
	int ret = 0;

	ret = cmdq_pkt_create(cmdq);
	if (ret) {
		pr_err("cmdq_pkt_create fail, ret(%d)\n", ret);
		return ret;
	}
	pr_debug("cmdq_pkt_create(%p) Success\n", cmdq);
	return ret;
}

static void
mtk_camgce_pkt_destroy(struct cmdq_pkt *cmdq)
{
	cmdq_pkt_destroy(cmdq);
}

static int
mtk_camgce_pkt_flush(struct cam_cmdq *cmdq, struct cmdq_pkt *pkt)
{
	int ret = 0;

	ret = cmdq_pkt_flush(cmdq->cmdq_client, pkt);

	return ret;
}

static void
mtk_camgce_flush_async_cb(struct cmdq_cb_data data)
{
	struct camgce_cbdata *cb_data = data.data;
	struct camgce_device *camgce = cb_data->dev;
	struct cmdq_async_info *async_info;

	async_info = &camgce->cmdq[camgce->priv->dev_idx]->async_info;
	spin_lock(&async_info->spinlock_cb);
	async_info->cb_done++;
	spin_unlock(&async_info->spinlock_cb);

	cmdq_pkt_destroy(cb_data->pkt);
	kfree(cb_data);
}

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
static int
mtk_camgce_get_cb_result(struct camgce_device *data)
{
	struct cmdq_async_info *async_info;
	u32 event_no, flush_count, cb_done;

	async_info = &data->cmdq[data->priv->dev_idx]->async_info;
	spin_lock(&async_info->spinlock_cb);
	event_no = async_info->flush_event;
	flush_count = async_info->flush_count;
	cb_done = async_info->cb_done;
	/* read clear */
	async_info->flush_count = 0;
	async_info->cb_done = 0;
	spin_unlock(&async_info->spinlock_cb);

	/* result analyzing */
	if (flush_count == 1 && cb_done == 1)
		return CAMGCE_CB_PERFECT_DONE;
	else if (cb_done > 0 && flush_count > cb_done)
		return CAMGCE_CB_DONE_LESS;
	else if (flush_count > 0 && cb_done > flush_count)
		return CAMGCE_CB_DONE_MORE;
	else if (cb_done > 0 && flush_count == cb_done)
		return CAMGCE_CB_DONE_EQUAL;
	else if (cb_done == 0)
		return CAMGCE_CB_NONE;
	else
		return -EFAULT;
}
#endif

static int
mtk_camgce_pkt_flush_async
(struct cam_cmdq *cmdq, struct cmdq_pkt *pkt, struct camgce_device *data)
{
	int ret = 0;
	struct camgce_cbdata *cb_data;
	struct cmdq_async_info *async_info;

	cb_data = kmalloc(sizeof(*cb_data), GFP_KERNEL);
	cb_data->dev = data;
	cb_data->pkt = pkt;

	/* record number of async calls */
	async_info = &data->cmdq[data->priv->dev_idx]->async_info;
	spin_lock(&async_info->spinlock_cb);
	async_info->flush_count++;
	spin_unlock(&async_info->spinlock_cb);

	ret = cmdq_pkt_flush_async
		(cmdq->cmdq_client, pkt,
		mtk_camgce_flush_async_cb, cb_data);

	return ret;
}

static int
mtk_camgce_waitevent(struct cmdq_pkt *cmdq, u32 user_event,
		     struct camgce_device *data)
{
	struct cmdq_async_info *async_info;

	if (user_event >= CMDQ_EVENT_MAX_NUM_GCE4_5) {
		pr_err("Invalid GCE event:%d\n", user_event);
		return -EINVAL;
	}

	/* store event number into async_info */
	async_info = &data->cmdq[data->priv->dev_idx]->async_info;
	spin_lock(&async_info->spinlock_cb);
	async_info->flush_event = user_event;
	spin_unlock(&async_info->spinlock_cb);

	cmdq_pkt_clear_event(cmdq, user_event);
	return cmdq_pkt_wfe(cmdq, user_event);
}

#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
static int
mtk_camgce_sendevent(struct cmdq_pkt *cmdq, u32 user_event)
{
	if (user_event >= CMDQ_EVENT_MAX_NUM_GCE4_5) {
		pr_err("Invalid GCE event:%d\n", user_event);
		return -EINVAL;
	}
	cmdq_pkt_clear_token(cmdq, user_event, SUBSYS_102CXXXX);
	return cmdq_pkt_set_token(cmdq, user_event, SUBSYS_102CXXXX);
}

static void
reg_write_mask
(struct cam_cmdq *cmdq, u32 offset, u32 value, u32 mask, struct cmdq_pkt *pkt)
{
	u32 subsys, addr;

	if (!pkt) {
		pr_err("cmdq packet handle is NULL\n");
		return;
	}
	subsys = cmdq->subsys;
	addr = (cmdq->subsys_base & 0xffff) | offset;
	pr_debug("subsys=%d, Addr=0x%x,", subsys, addr);
	pr_debug("val=0x%x, mask=0x%x\n", value, mask);
	cmdq_pkt_write_value(pkt, subsys, addr, value, mask);
}

static void
reg_write(struct cam_cmdq *cmdq, u32 offset, u32 value, struct cmdq_pkt *pkt)
{
	pr_debug("addr:0x%x, value:0x%x\n", offset, value);
	if (!pkt) {
		pr_err("cmdq packet handle is NULL\n");
		return;
	}
	reg_write_mask(cmdq, offset, value, 0xffffffff, pkt);
}
#endif

int
mtk_camgce_write_reg(
	int dev_idx, int hw_module, unsigned long offset, unsigned long data)
{
	int ret = 0;
	struct camgce_device *camgce_dev;

	pr_debug("dev_idx:%d, module:%d,", dev_idx, hw_module);
	pr_debug("addr=0x%lx, data=0x%lx\n", offset, data);

	if (dev_idx < 0 || dev_idx >= CAMGCE_DEV_MAX_NUM)
		return -EFAULT;

	if (hw_module < 0 || hw_module >= CAMGCE_SGZ_MAX_NUM)
		return -EFAULT;

	camgce_dev = &camgce_devs[dev_idx];

	if (camgce_dev->regs[hw_module] && offset < PAGE_SIZE)
		CAMGCE_WR32(camgce_dev->regs[hw_module] + offset, data);
	else
		ret = -EFAULT;

	return ret;
}
EXPORT_SYMBOL(mtk_camgce_write_reg);

int mtk_camgce_read_reg(int dev_idx, int hw_module, unsigned long offset)
{
	int ret = 0;
	struct camgce_device *camgce_dev;

	pr_debug("dev_idx:%d, module:%d,", dev_idx, hw_module);
	pr_debug("addr=0x%lx\n", offset);

	if (dev_idx < 0 || dev_idx >= CAMGCE_DEV_MAX_NUM)
		return -EFAULT;
	if (hw_module < 0 || hw_module >= CAMGCE_SGZ_MAX_NUM)
		return -EFAULT;

	camgce_dev = &camgce_devs[dev_idx];

	if (camgce_dev->regs[hw_module] && offset < PAGE_SIZE)
		ret = CAMGCE_RD32(camgce_dev->regs[hw_module] + offset);
	else
		ret = -EFAULT;

	return ret;
}
EXPORT_SYMBOL(mtk_camgce_read_reg);

static irqreturn_t
mtk_camgce_irq(int irq, void *device_id)
{
	/* Read irq status */

	return IRQ_HANDLED;
}

static int
mtk_camgce_mmap(struct file *file, struct vm_area_struct *p_vma)
{
	unsigned long length = 0;
	unsigned long pfn = 0x0;
	struct camgce_device *camgce_dev;
	int found = 0;
	unsigned int i = 0;

	camgce_dev = (struct camgce_device *)file->private_data;
	if (!camgce_dev) {
		pr_err("file->private_data is NULL,");
		pr_err("process=%s,", current->comm);
		pr_err("pid=%d, tgid=%d\n", current->pid, current->tgid);
		return -EFAULT;
	}

	length = (p_vma->vm_end - p_vma->vm_start);
	pfn = p_vma->vm_pgoff << PAGE_SHIFT;

	for (i = 0; i < camgce_dev->res_num; i++) {
		if ((camgce_dev->res[i].start == pfn) &&
			((camgce_dev->res[i].start + length) <=
				(camgce_dev->res[i].end + 1))) {
			found = 1;
			break;
		}
	}

	if (found == 0)
		return -EAGAIN;

	p_vma->vm_page_prot = pgprot_noncached(p_vma->vm_page_prot);

	pr_debug("vm_pgoff(0x%lx),pfn(0x%lx),phy(0x%lx),",
		p_vma->vm_pgoff, pfn, p_vma->vm_pgoff << PAGE_SHIFT);
	pr_debug("vm_start(0x%lx),vm_end(0x%lx),length(0x%lx)\n",
		p_vma->vm_start, p_vma->vm_end, length);

	if (remap_pfn_range
		(p_vma, p_vma->vm_start, p_vma->vm_pgoff,
		p_vma->vm_end - p_vma->vm_start,
		p_vma->vm_page_prot))
		return -EAGAIN;

	/*  */
	return 0;
}

static long
mtk_camgce_ioctl(struct file *file, unsigned int cmd, unsigned long param)
{
	int ret = 0;
	struct camgce_reg camgce_data;
	struct camgce_device *camgce_dev;
	struct cmdq_pkt *pkt;
	unsigned int module_maxnum = 0;
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	unsigned int read_reg = 0;
#endif
	u32 subsys, addr, value, mask;

	camgce_dev = (struct camgce_device *)file->private_data;

	if (!camgce_dev) {
		pr_err("file->private_data is NULL,");
		pr_err("process=%s,", current->comm);
		pr_err("pid=%d, tgid=%d\n", current->pid, current->tgid);
		return -EFAULT;
	}

	module_maxnum = camgce_module_num;

	switch (cmd) {
	case CAMGCE_DEBUG_WRITE:/*especially for write to any subsys*/
		/* camgce_data.module --> subsys num
		 * camgce_data.addr ----> (reg address - sybsys base)
		 * camgce_data.value ---> same, reg value
		 * camgce_data.bitmask -> same, bit mask
		 * camgce_data.pkt_handle -> same, packet handle
		*/
		pr_debug("[CAMGCE_DEBUG_WRITE]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			sizeof(struct camgce_reg)) == 0) {
			if (camgce_data.pkt_handle) {
				pkt = (struct cmdq_pkt *)camgce_data.pkt_handle;
				subsys = camgce_data.module;
				addr = camgce_data.addr;
				mask = 0xffffffff;
				value = camgce_data.value;
				pr_debug("subsys=%d, Addr=0x%x, val=0x%x\n",
					 subsys, addr, value);
				cmdq_pkt_write_value(pkt, subsys, addr,
						     value, mask);
			} else {
				pr_err("NULL packet handle\n");
				ret = -EFAULT;
			}
		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	case CAMGCE_WRITE:
		pr_debug("[CAMGCE_WRITE]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			sizeof(struct camgce_reg)) == 0) {
			/**/
			ret = mtk_camgce_write_reg
				(camgce_dev->priv->dev_idx,
				 camgce_data.module, camgce_data.addr,
				 camgce_data.value);
			if (ret < 0) {
				pr_err("camgce_write_reg failed:%d\n", ret);
				return ret;
			}
		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
	case CAMGCE_READ:
		pr_debug("[CAMGCE_READ]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			 /**/
			read_reg =
				mtk_camgce_read_reg
				(camgce_dev->priv->dev_idx,
				 camgce_data.module,
				 camgce_data.addr);
			pr_debug("reg value:0x%x\n", read_reg);

		} else {
			pr_err("copy_from_user fail\n");
			ret = -EFAULT;
			goto EXIT;
		}
		/*if(put_user(regValue,			*/
		/*(MUINT32 *) &(camgce_data.reg.value))*/
		/*!= 0) {						*/
		/*	LOG_ERR("put_user failed\n");	*/
		/*	ret = -EFAULT;				*/
		/*	goto EXIT;				*/
		/*}							*/
		camgce_data.value = read_reg;
		if (copy_to_user
			((void *)param, (void *)&camgce_data,
			 sizeof(struct camgce_reg)) != 0) {
			pr_err("copy to user fail failed\n");
			/* ret = -EFAULT; */
			goto EXIT;
		}
		break;
#endif
	/*
	 *  Camera GCE ioctls
	 */
	case CAMGCE_CMDQ_CREATE:
		pr_debug("[CAMGCE_CMDQ_CREATE]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("subssys(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}

			ret = mtk_camgce_pkt_create(&pkt);
			camgce_data.pkt_handle =
					(camgce_cmdq_pkt_hndl)(pkt);

			if (copy_to_user
				((void *)param, (void *)&camgce_data,
				 sizeof(struct camgce_reg)) != 0) {
				pr_err("copy to user fail failed\n");
				/* ret = -EFAULT; */
				goto EXIT;
			}

		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
	case CAMGCE_CMDQ_DESTROY:
		pr_debug("[CAMGCE_CMDQ_DESTROY]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("subssys(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}

			mtk_camgce_pkt_destroy
				((struct cmdq_pkt *)camgce_data.pkt_handle);

		} else {
			pr_err("copy_from_user fail\n");
			ret = -EFAULT;
		}
		break;
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	case CAMGCE_CMDQ_MASK_WRITE:
		pr_debug("[CAMGCE_CMDQ_MASK_WRITE]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("module(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}
			reg_write_mask
				(camgce_dev->cmdq[camgce_data.module],
				camgce_data.addr,
				camgce_data.value,
				camgce_data.bitmask,
				(struct cmdq_pkt *)camgce_data.pkt_handle);
		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
	case CAMGCE_CMDQ_WRITE:
		pr_debug("[CAMGCE_CMDQ_WRITE]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("module(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}
			reg_write
				(camgce_dev->cmdq[camgce_data.module],
				 camgce_data.addr,
				 camgce_data.value,
				 (struct cmdq_pkt *)camgce_data.pkt_handle);

		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
	case CAMGCE_CMDQ_READ:
		pr_debug("[CAMGCE_CMDQ_READ] may be support in future\n");
		break;
#endif
	case CAMGCE_CMDQ_FLUSH:
		pr_debug("[CAMGCE_CMDQ_FLUSH]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("subssys(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}
			ret = mtk_camgce_pkt_flush
				(camgce_dev->cmdq[camgce_data.module],
				 (struct cmdq_pkt *)camgce_data.pkt_handle);
		} else {
			pr_err("copy_from_user fail\n");
			ret = -EFAULT;
		}
		break;
	case CAMGCE_CMDQ_FLUSH_ASYNC:
		pr_debug("[CAMGCE_CMDQ_FLUSH_ASYNC]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("subssys(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}
			ret = mtk_camgce_pkt_flush_async
				(camgce_dev->cmdq[camgce_data.module],
				 (struct cmdq_pkt *)camgce_data.pkt_handle,
				 camgce_dev);
		} else {
			pr_err("copy_from_user fail\n");
			ret = -EFAULT;
		}
		break;
	case CAMGCE_CMDQ_WAITEVENT:
		pr_debug("[CAMGCE_CMDQ_WAITEVENT]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("module(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}
			ret = mtk_camgce_waitevent
					(camgce_data.pkt_handle,
					 camgce_data.value, camgce_dev);
		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
#ifndef CONFIG_VIDEO_MEDIATEK_ISP_TINY
	case CAMGCE_CMDQ_SENDEVENT:
		pr_debug("[CAMGCE_CMDQ_SENDEVENT]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("module(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}
			ret = mtk_camgce_sendevent
					(camgce_data.pkt_handle,
					 camgce_data.value);
		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
	case CAMGCE_GET_CBINFO:
		pr_debug("[CAMGCE_GET_CBINFO]\n");
		if (copy_from_user
			(&camgce_data, (void *)param,
			 sizeof(struct camgce_reg)) == 0) {
			/* check if subsys idx is valid */
			if (camgce_data.module >= module_maxnum) {
				pr_err("module(%d)\n", camgce_data.module);
				ret = -EFAULT;
				break;
			}
			ret = mtk_camgce_get_cb_result(camgce_dev);
			camgce_data.value = ret;

			if (copy_to_user
				((void *)param, (void *)&camgce_data,
				 sizeof(struct camgce_reg)) != 0) {
				pr_err("copy to user fail failed\n");
				/* ret = -EFAULT; */
				goto EXIT;
			}

		} else {
			pr_err("copy_from_user failed\n");
			ret = -EFAULT;
		}
		break;
#endif
	default:
		pr_err("Unknown Cmd(0x%x)\n", cmd);
		ret = -EPERM;
		break;
	}
EXIT:
	return ret;
}

static int
mtk_camgce_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct camgce_user_info *uinfo;
	struct camgce_user_id *uid;
	struct camgce_platform *plat =
		container_of(inode->i_cdev, struct camgce_platform, cdev);
	struct camgce_device *cg_dev =
		container_of(plat, struct camgce_device, platform);

	if (!cg_dev) {
		pr_err("camera gce device is null\n");
		return -ENOMEM;
	}
	filp->private_data = cg_dev;

	uinfo = &cg_dev->uinfo;
	pr_debug("dev:%d,usr=%d\n", cg_dev->priv->dev_idx, uinfo->usercount);

	spin_lock(&uinfo->spinlock_ref);
	if (uinfo->usercount > 0) {
		uinfo->usercount++;
		spin_unlock(&uinfo->spinlock_ref);
		goto EXIT;
	} else {
		uinfo->usercount++;
		spin_unlock(&uinfo->spinlock_ref);
	}

	uid = &cg_dev->uid;
	uid->pid = current->pid;
	uid->tid = current->tgid;

EXIT:
	return ret;
}

static int
mtk_camgce_release(struct inode *inode, struct file *filp)
{
	struct camgce_device *cg_dev;
	struct camgce_user_info *uinfo;

	cg_dev = (struct camgce_device *)filp->private_data;
	uinfo = &cg_dev->uinfo;
	spin_lock(&uinfo->spinlock_ref);

	pr_debug("+ E. UserCount:%d\n", uinfo->usercount);

	uinfo->usercount--;
	pr_debug("dev:%d, usr=%d\n", cg_dev->priv->dev_idx, uinfo->usercount);
	if (uinfo->usercount > 0) {
		spin_unlock(&uinfo->spinlock_ref);
		goto EXIT;
	} else {
		spin_unlock(&uinfo->spinlock_ref);
	}

EXIT:
	pr_debug("-");

	return 0;
}

static const struct file_operations mtk_camgce_file_fops = {
	.owner = THIS_MODULE,
	.open = mtk_camgce_open,
	.release = mtk_camgce_release,
	.mmap = mtk_camgce_mmap,
	.unlocked_ioctl = mtk_camgce_ioctl,
};

static int
mtk_camgce_reg_cdev(struct camgce_device *camgce_dev)
{
	int ret = 0;
	struct camgce_platform *plat;

	pr_debug("camgce RegCharDev Start\n");

	plat = &camgce_dev->platform;

	/* Allocate driver */
	ret = alloc_chrdev_region(&plat->dev_t, 0, 1, camgce_dev->priv->name);
	if (ret < 0) {
		pr_debug("alloc_chrdev_region failed %d\n", ret);
		return ret;
	}

	/* Attatch file operation. */
	cdev_init(&plat->cdev, &mtk_camgce_file_fops);
	plat->cdev.owner = THIS_MODULE;
	plat->cdev.ops = &mtk_camgce_file_fops;

	/* Add to system */
	ret = cdev_add(&plat->cdev, plat->dev_t, 1);
	if (ret < 0) {
		pr_debug("Attatch file failed:%d\n", ret);
		goto EXIT;
	}

EXIT:
	if (ret < 0)
		cdev_del(&plat->cdev);

	pr_debug("camgce regCharDev end\n");
	return ret;
}

static int
mtk_camgce_unreg_cdev(struct camgce_device *camgce_dev)
{
	pr_debug("+ E.\n");
	/* Release char driver */
	if (!camgce_dev->occupied) {
		pr_err("cg_dev is NULL\n");
		return -EFAULT;
	}

	device_destroy(camgce_cls, camgce_dev->platform.dev_t);
	cdev_del(&camgce_dev->platform.cdev);
	unregister_chrdev_region(camgce_dev->platform.dev_t, 1);

	pr_debug("- E.\n");

	return 0;
}

static int
mtk_camgce_probe(struct platform_device *p_dev)
{
	int i = 0, j = 0, k = 0, ret = 0;
	unsigned int irq_info[3]; /* Record interrupts info from device tree */
	struct camgce_device *camgce_dev;
	struct camgce_priv *priv;
	struct device *dev = &p_dev->dev;
	struct device_node *gce_node = NULL;
	struct platform_device *gce_pdev;
	const struct of_device_id *of_id;
	int num_gceitems = 0;
	int num_modules = 0;
	int num_subsys_thr = 1;
	int subsys_id = 0;
	struct cmdq_client *cmdq_client_ptr[SUBSYS_MAX] = {NULL};
	int mbox_create_cnt = 0;

	pr_debug("camgce probe start\n");

	if (!p_dev) {
		dev_err(&p_dev->dev, "pDev is NULL\n");
		return -ENXIO;
	}
	if (camgce_dev_nr == 0) {
		camgce_devs =
			devm_kzalloc
			(dev,
			 sizeof(struct camgce_device) * CAMGCE_DEV_MAX_NUM,
			 GFP_KERNEL);
		if (!camgce_devs) {
			dev_err(&p_dev->dev, "Unable to allocate isp_devs\n");
			return -ENOMEM;
		}
	}

	/* get node name and save priv */
	of_id = of_match_node(camgce_of_ids, dev->of_node);
	if (!of_id)
		return -EINVAL;

	gce_node = of_parse_phandle(dev->of_node, "mediatek,gce", 0);
	if (!gce_node) {
		dev_err(dev, "Failed to get gce node\n");
		of_node_put(gce_node);
		return -EINVAL;
	}

	gce_pdev = of_find_device_by_node(gce_node);
	if (!gce_pdev) {
		dev_err(dev, "Failed to get gce platform dev.\n");
		of_node_put(gce_node);
		return -EINVAL;
	}
	if (!dev_get_drvdata(&gce_pdev->dev)) {
		dev_info(dev, "Waiting for gce device %s ready\n",
			gce_node->full_name);
		of_node_put(gce_node);
		return -EPROBE_DEFER;
	}

	of_node_put(gce_node);
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	of_property_read_string(dev->of_node, "dev-name", &priv->name);
	of_property_read_u32(dev->of_node, "dev-idx", &priv->dev_idx);

	camgce_dev = &camgce_devs[priv->dev_idx];
	camgce_dev->dev = &p_dev->dev;
	camgce_dev->priv = priv;
	camgce_dev->occupied = true;

	/*LOG_DBG("cg_dev[%d]:%p, ->dev:%p", camgce_dev_nr,*/
	/* camgce_dev, camgce_dev->dev);*/
	num_modules =
	of_property_count_elems_of_size(dev->of_node, "reg", 4 * sizeof(u32));
	if (num_modules > CAMGCE_SGZ_MAX_NUM) {
		camgce_dev->res_num = 0;
		return -EINVAL;
	}
	camgce_dev->res_num = num_modules;

	num_gceitems =
	of_property_count_elems_of_size(dev->of_node,
					"gce-subsys", sizeof(u32));

	/* iomap registers : for regs accessing without using GCE */
	for (i = 0; i < num_modules; i++) {
		camgce_dev->regs[i] = of_iomap(p_dev->dev.of_node, i);
		if (!camgce_dev->regs[i]) {
			dev_err(&p_dev->dev, "of_iomap fail,name=%s,i=%d\n",
				p_dev->dev.of_node->name, i);
			return -ENOMEM;
		}
		pr_debug(" devnode(%s), map_addr=0x%lx, GCE_PA=0x%lx\n",
			p_dev->dev.of_node->name,
			(unsigned long)camgce_dev->regs[i],
			of_get_cam_pa(p_dev->dev.of_node, i));
		ret = of_address_to_resource
			(p_dev->dev.of_node, i, &camgce_dev->res[i]);
		if (ret < 0) {
			pr_err("Unable to get resource\n");
			return ret;
		}
	}
	/* get IRQ ID and request IRQ */
	camgce_dev->uinfo.irq = irq_of_parse_and_map(p_dev->dev.of_node, 0);
	pr_debug("mapped irq=%d\n", camgce_dev->uinfo.irq);
	if (camgce_dev->uinfo.irq) {
		/* Get IRQ Flag from device node */
		if (of_property_read_u32_array
			(p_dev->dev.of_node, "interrupts",
			 irq_info, ARRAY_SIZE(irq_info)))	{
			dev_err(&p_dev->dev, "get irq flags from DTS fail\n");
			return -ENODEV;
		}
		/* global irq table */
		ret = request_irq
			(camgce_dev->uinfo.irq,
			 (irq_handler_t)mtk_camgce_irq,
			 (irq_info[2] | IRQF_SHARED),
			 "camgce", NULL);
		/* IRQF_TRIGGER_NONE dose not take effect here, */
		/* real trigger mode set in dts file */
		if (ret) {
			dev_err(&p_dev->dev, "irq failed\n");
			return ret;
		}
	} else {
		dev_info(dev, "No irq registered!!\n");
	}
	/* cmdq driver create */
	for (i = 0, j = 0; j < num_gceitems; j++) {
		of_property_read_u32_index
			(dev->of_node, "subsys-thread", j, &num_subsys_thr);
		for (k = 0; k < num_subsys_thr; k++, i++) {
			camgce_dev->cmdq[i] =
				kzalloc(sizeof(struct cam_cmdq), GFP_KERNEL);

			spin_lock_init(
				&camgce_dev->cmdq[i]->async_info.spinlock_cb);

			of_property_read_u32_index
				(dev->of_node, "gce-subsys", j,
				 &camgce_dev->cmdq[i]->subsys);

			subsys_id = camgce_dev->cmdq[i]->subsys;
			if (subsys_id != SUBSYS_NONE &&
			    subsys_id < SUBSYS_MAX)	{
				/**/
				camgce_dev->cmdq[i]->subsys_base =
					of_get_cam_pa(p_dev->dev.of_node, j);

				camgce_dev->cmdq[i]->cmdq_client =
					cmdq_mbox_create
					(camgce_dev->dev, mbox_create_cnt);

				if (IS_ERR(camgce_dev->cmdq[i]->cmdq_client)) {
					dev_err(dev,
						"mbox fail: %d(thr%d)\n", i, k);
					kzfree(camgce_dev->cmdq[i]);
					return -EPROBE_DEFER;
				}
				cmdq_client_ptr[subsys_id] =
					camgce_dev->cmdq[i]->cmdq_client;
				mbox_create_cnt++;
			}
		}
	}

	camgce_dev_nr++;
	/* store camera memory device structure in device driver's private data
	 */
	platform_set_drvdata(p_dev, camgce_devs);

	ret = mtk_camgce_reg_cdev(camgce_dev);

	camgce_dev->dev = device_create
		(camgce_cls, NULL,
		 camgce_dev->platform.dev_t, NULL,
		 camgce_dev->priv->name);
	/* init spinlock */
	spin_lock_init(&camgce_dev->uinfo.spinlock_ref);

	if (ret < 0)
		mtk_camgce_unreg_cdev(camgce_dev);

	pr_debug("Camgce probe end\n");

	return ret;
}

static int
mtk_camgce_remove(struct platform_device *p_dev)
{
	int i, j, irq_num;

	for (i = 0; i < CAMGCE_DEV_MAX_NUM; i++) {
		/* cmdq mbox destroy */
		for (j = 0; j < CAMGCE_SGZ_MAX_NUM; j++) {
			cmdq_mbox_destroy(camgce_devs[i].cmdq[j]->cmdq_client);
			kfree(camgce_devs[i].cmdq[j]);
		}
		kfree(camgce_devs[i].priv);
		mtk_camgce_unreg_cdev(&camgce_devs[i]);
		disable_irq(camgce_devs[i].uinfo.irq);
		irq_num = platform_get_irq(p_dev, 0);
		free_irq(irq_num, NULL);
	}
	pr_debug("camgce remove success\n");
	return 0;
}

static struct platform_driver mtk_camgce_driver = {
	.probe = mtk_camgce_probe,
	.remove = mtk_camgce_remove,
	.driver = {
		.name = CAMGCE_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = camgce_of_ids,
	}
};

static int __init
mtk_camgce_init(void)
{
	int ret = 0;

	camgce_cls = class_create(THIS_MODULE, "mtk-camgce");

	if (IS_ERR(camgce_cls)) {
		ret = PTR_ERR(camgce_cls);
		pr_err("Unable to create class,err=%d\n", ret);
	}

	ret = platform_driver_register(&mtk_camgce_driver);
	if (ret < 0) {
		pr_err("camgce platform_driver_register fail\n");
		return ret;
	}
	pr_debug("camgce platform_driver_register success\n");

	return ret;
}

static void __exit
mtk_camgce_exit(void)
{
	platform_driver_unregister(&mtk_camgce_driver);
	pr_debug("camgce driver exit success\n");
	class_destroy(camgce_cls);
}

module_init(mtk_camgce_init);
module_exit(mtk_camgce_exit);
MODULE_LICENSE("GPL");
