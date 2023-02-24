/*
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/dma-mapping.h>
#include <uapi/drm/drm_fourcc.h>

#include "mtk_common_util.h"
#include "mtk_disp_dev.h"
#include "mtk_disp.h"

#define DISP_DEV_DEVNAME "mtk_disp"

static struct device *dispsys_dev;
static dev_t disp_devno;
static struct cdev *disp_cdev;
static struct class *disp_class;
static struct device *disp_device;

struct mtk_disp_para disp_para;
struct mtk_dispsys_buf disp_buf[MAX_FILE_NUM] = {0};

static int mtk_disp_format_to_bpp(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_AYUV2101010:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB8888:
		return 32;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;
	case DRM_FORMAT_YUYV:
		return 16;
	}

	return -EINVAL;
}

static int mtk_disp_format_to_bpp_for_dsc(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
		return 30;
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;
	}

	return -EINVAL;
}

static int disp_set_parameter(struct device *dev, void *para)
{
	struct disp_parameter *input_para;
	int ret = 0;

	if (!dev || !para)
		return -EINVAL;

	input_para = (struct disp_parameter *)para;

	disp_para.scenario = (enum mtk_dispsys_scenario)input_para->scenario;
	disp_para.input_format = input_para->input_format;
	disp_para.output_format = input_para->output_format;
	disp_para.input_height = input_para->input_height;
	disp_para.input_width = input_para->input_width;

	disp_para.input_fps = input_para->input_fps;
	disp_para.disp_fps = input_para->disp_fps;
	disp_para.dp_enable = input_para->dp_enable;
	disp_para.frmtrk = input_para->frmtrk_enable;

	disp_para.dsc_enable = (bool)input_para->dsc_enable;
	disp_para.encode_rate = input_para->encode_rate;
	disp_para.dsc_passthru = (bool)input_para->dsc_passthru;
	disp_para.lhc_enable = (bool)input_para->lhc_enable;
	disp_para.dump_enable = (bool)input_para->dump_enable;
	disp_para.fbdc_enable = (bool)input_para->fbdc_enable;

	disp_para.edid_sel = input_para->edid_sel;

	if (disp_para.dsc_enable && !disp_para.encode_rate)
		return -1;

	ret = mtk_dispsys_config(dev, &disp_para);

	return ret;
}

static int disp_load_image(struct device *dev, void *para)
{
	struct disp_img_info *img;
	u32 width, height;
	u32 size, f_size;
	u32 temp, offset, header;
	int i, fbdc_enable = 0;
	int ret = 0;

	if (!dev || !para)
		return -EINVAL;

	img = (struct disp_img_info *)para;

	if (img->file[0][0] == '\0')
		return -EINVAL;

	if (disp_buf[0].va_lcd || disp_buf[1].va_lcd)
		return -EMFILE;

	if (((img->height == 0) || (img->height > 2160)) ||
	    ((img->width == 0) || (img->width > 4320)))
		return -EINVAL;

	size = img->height * img->width *
	       (mtk_disp_format_to_bpp(img->format) / 8);
	width = img->width * (mtk_disp_format_to_bpp(img->format) / 8);
	height = img->height;

	if (disp_para.scenario == DISP_DRAM_MODE ||
	    disp_para.scenario == DISP_SRAM_MODE) {
		if (disp_para.fbdc_enable) {
			/* compressed fbdc data is 4Byte per pixel */
			size = img->height * img->width * 4;

			temp = (img->height * img->width + 127) / 128;
			header = roundup(temp, 64);
			offset = header - temp;

			f_size = size + header;
			size = roundup(f_size, PAGE_SIZE);

			fbdc_enable = 1;
		}
	}

	for (i = 0; i < MAX_FILE_NUM; i++) {
		if (img->file[i][0] == '\0')
			return 0;

		disp_buf[i].size = size;
		ret = mtk_dispsys_buf_alloc_ext(dev, &disp_buf[i]);

		if (ret)
			return ret;

		if (fbdc_enable) {
			mtk_common_read_file(disp_buf[i].va_lcd + offset,
					     img->file[i], f_size);
		} else {
			mtk_common_read_img(disp_buf[i].va_lcd, img->file[i], 0,
					    width, width, height, width);
		}

		pr_debug("display_dev : img%d addr = 0x%llx\n",
			 i, disp_buf[i].pa_lcd);
	}

	return 0;
}

static int disp_save_image(struct device *dev, void *para)
{
	struct disp_img_info *img;
	u32 width;
	u32 size, header;
	int fbdc_enable = 0;
	u32 format = disp_para.output_format;
#ifdef CONFIG_MACH_MT3612_A0
	u32 hw_nr = 4;
#else
	u32 hw_nr = 1;
#endif
	if (!dev || !para)
		return -EINVAL;

	img = (struct disp_img_info *)para;

	if (img->out_file[0] == '\0')
		return -EINVAL;

	if (disp_para.dsc_enable || disp_para.dsc_passthru) {
		width = disp_para.input_width / hw_nr;

		width = (width *
			mtk_disp_format_to_bpp_for_dsc(format) /
			8 / disp_para.encode_rate + 2) / 3;

		width = (width * 24 / 8) * hw_nr;
		size = disp_para.input_height * width;
	} else if (disp_para.fbdc_enable) {
		width = disp_para.input_width *
			mtk_disp_format_to_bpp(format) / 8;

		header = (disp_para.input_height * disp_para.input_width + 127)
			/ 128;
		header = roundup(header, 64);
		size = disp_para.input_height * width + header;
		fbdc_enable = 1;
	} else {
		width = disp_para.input_width *
			mtk_disp_format_to_bpp(format) / 8;
		size = disp_para.input_height * width;
	}

	if (fbdc_enable) {
		mtk_common_write_file(mtk_dispsys_get_dump_vaddr(dev),
				      img->out_file, size);
	} else {
		mtk_common_write_img(mtk_dispsys_get_dump_vaddr(dev),
				     img->out_file, 0, width, width,
				     disp_para.input_height, width);
	}

	return 0;
}

static void disp_unload_image(struct device *dev)
{
	int i;

	for (i = 0; i < MAX_FILE_NUM; i++) {
		if (disp_buf[i].va_lcd) {
			pr_debug("display_dev : release img%d addr = %llx\n",
				 i, disp_buf[i].pa_lcd);
			mtk_dispsys_buf_free_ext(dev, &disp_buf[i]);
		}
	}
}

static int disp_dev_open(struct inode *inode, struct file *file)
{
	pr_debug("display_dev :open successful\n");
	return 0;
}

static int disp_dev_release(struct inode *inode, struct file *file)
{
	pr_debug("display_dev :release successful\n");
	return 0;
}

static ssize_t disp_dev_read(struct file *file, char *buf, size_t count,
			     loff_t *ptr)
{
	return count;
}

static ssize_t disp_dev_write(struct file *file, const char *buf, size_t count,
			      loff_t *ppos)
{
	return count;
}

static long disp_dev_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	void *buf = NULL;
	int i;
	struct device *rdma_dev;
	struct mtk_common_buf_handle *handle;
	struct mtk_common_buf *ion_buf;
	int ret = 0;
	struct cmdq_pkt **pkt;

	if (_IOC_DIR(cmd) != _IOC_NONE) {
		buf = kzalloc(_IOC_SIZE(cmd), GFP_KERNEL);

		if (buf == NULL) {
			ret = -ENOMEM;
			goto ioctl_exit;
		}

		if (_IOC_WRITE & _IOC_DIR(cmd)) {
			if (copy_from_user(buf, (void *)arg, _IOC_SIZE(cmd))) {
				kfree(buf);
				pr_err(
				   "[display_dev] ioctl copy from user failed");
				ret = -EFAULT;
				goto ioctl_exit;
			}
		}
	}

	switch (cmd) {
	case IOCTL_DISP_SET_PARAMETER:
		ret = disp_set_parameter(dispsys_dev, buf);
		break;
	case IOCTL_DISP_START:
#ifdef CONFIG_MACH_MT3612_A0
		if (disp_para.dp_enable)
			ret = mtk_dispsys_vr_start(dispsys_dev);
		else
			ret = mtk_dispsys_enable(dispsys_dev, 0, 0);
#else
		ret = mtk_dispsys_enable(dispsys_dev, 0, 0);
#endif
		break;
	case IOCTL_DISP_STOP:
#ifdef CONFIG_MACH_MT3612_A0
		if (disp_para.dp_enable)
			ret = mtk_dispsys_vr_stop(dispsys_dev);
		else
			mtk_dispsys_disable(dispsys_dev);
#else
		mtk_dispsys_disable(dispsys_dev);
#endif
		break;
	case IOCTL_DISP_LHC:
		ret = mtk_dispsys_read_lhc(dispsys_dev, buf);
		if (ret != 0) {
			kfree(buf);
			return ret;
		}
		break;
	case IOCTL_DISP_LOAD_IMG:
		if (!buf) {
			ret = -EINVAL;
			break;
		}
		ret = disp_load_image(dispsys_dev, buf);
		if (ret)
			break;
		ret = mtk_dispsys_config_srcaddr(dispsys_dev,
						 disp_buf[0].pa_lcd, NULL);
		break;
	case IOCTL_DISP_SAVE_IMG:
		ret = disp_save_image(dispsys_dev, buf);
		break;
	case IOCTL_DISP_UNLOAD_IMG:
		disp_unload_image(dispsys_dev);
		break;
	case IOCTL_DISP_IMG_IDX:
		if (!buf) {
			ret = -EINVAL;
			break;
		}
		i = *(int *)buf;
		pr_debug("display_dev :display image index=%d\n", i);
		if ((i >= MAX_FILE_NUM) || (i < 0)) {
			ret = -EINVAL;
			break;
		}

		pkt = mtk_dispsys_cmdq_pkt_create(dispsys_dev);

		ret = mtk_dispsys_config_srcaddr(dispsys_dev,
						 disp_buf[i].pa_lcd, pkt);

		mtk_dispsys_cmdq_pkt_flush(dispsys_dev, pkt);
		mtk_dispsys_cmdq_pkt_destroy(dispsys_dev, pkt);
		break;
	case IOCTL_DISP_ION_GET_HANDLE:
		if (!buf) {
			ret = -EINVAL;
			break;
		}
		rdma_dev = mtk_dispsys_get_rdma(dispsys_dev);
		handle = (struct mtk_common_buf_handle *)buf;
		ret = mtk_common_fd_to_handle_offset(rdma_dev,
						     handle->fd,
						     &(handle->handle),
						     handle->offset);
		if (ret) {
			dev_dbg(dispsys_dev,
				"FD_TO_HANDLE: import buf failed,ret=%d\n",
				ret);
			break;
		}

		break;
	case IOCTL_DISP_ION_SET_BUFFER:
		if (!buf) {
			ret = -EINVAL;
			break;
		}
		handle = (struct mtk_common_buf_handle *)buf;
		ion_buf = (struct mtk_common_buf *)handle->handle;

		dev_dbg(dispsys_dev, "kernel set buf dma_addr = %p\n",
			(void *)ion_buf->dma_addr);

		disp_buf[0].pa_lcd = ion_buf->dma_addr;

		ret = mtk_dispsys_config_srcaddr(dispsys_dev,
						 disp_buf[0].pa_lcd, NULL);
		break;
	case IOCTL_DISP_ION_PUT_HANDLE:
		if (!buf) {
			ret = -EINVAL;
			break;
		}
		handle = (struct mtk_common_buf_handle *)buf;

		disp_buf[0].pa_lcd = (dma_addr_t)NULL;

		ret = mtk_common_put_handle(handle->handle);

		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if ((_IOC_READ & _IOC_DIR(cmd)) && buf) {
		if (copy_to_user((void __user *)arg, buf, _IOC_SIZE(cmd))) {
			kfree(buf);
			pr_info("[display_dev] ioctl copy to user failed");
			ret = -EFAULT;
			goto ioctl_exit;
		}
	}

	kfree(buf);

ioctl_exit:

	if (ret != 0)
		pr_info("[display_dev] ioctl fail(%d)", ret);

	return ret;
}

const struct file_operations disp_dev_fops = {
	.owner = THIS_MODULE,
	.read = disp_dev_read,
	.write = disp_dev_write,
	.unlocked_ioctl = disp_dev_ioctl,
	.open = disp_dev_open,
	.release = disp_dev_release,
};

static int mtk_disp_init(void)
{
	struct device_node *node;
	struct platform_device *ctrl_pdev;
	int ret = 0;

	pr_debug("display_dev_init\n");

	/* find display pipe device */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt3612-display");
	if (!node) {
		pr_err("Failed to get display pipe node\n");
		of_node_put(node);
		return -ENODEV;
	}
	ctrl_pdev = of_find_device_by_node(node);
	if (!ctrl_pdev) {
		pr_err("Device dose not enable %s\n", node->full_name);
		of_node_put(node);
		return -ENODEV;
	}
	if (!dev_get_drvdata(&ctrl_pdev->dev)) {
		pr_err("Waiting for display pipe device %s\n", node->full_name);
		of_node_put(node);
		return -ENODEV;
	}
	of_node_put(node);

	dispsys_dev = &ctrl_pdev->dev;

	/* register character dev */
	ret = alloc_chrdev_region(&disp_devno, 0, 1, DISP_DEV_DEVNAME);
	if (ret) {
		pr_err("[disp_dev] alloc_chrdev_region failed (ret=%d)\n", ret);
		goto err_alloc;
	}

	disp_cdev = cdev_alloc();
	disp_cdev->owner = THIS_MODULE;
	disp_cdev->ops = &disp_dev_fops;
	ret = cdev_add(disp_cdev, disp_devno, 1);
	if (ret) {
		pr_err("[disp_dev] cdev_add fail (ret=%d)\n", ret);
		goto err_add;
	}

	/* register class */
	disp_class = class_create(THIS_MODULE, DISP_DEV_DEVNAME);
	if (IS_ERR(disp_class)) {
		ret = PTR_ERR(disp_class);
		pr_err("[disp_dev] class_create fail (ret=%d)\n", ret);
		goto err_add;
	}

	disp_device =
	    device_create(disp_class, NULL, disp_devno, NULL, DISP_DEV_DEVNAME);

	if (IS_ERR(disp_device)) {
		ret = PTR_ERR(disp_device);
		pr_err("[disp_dev] device_create fail (ret=%d)\n", ret);
		goto err_device;
	}

	pr_debug("display_dev_init completed.\n");

	return 0;

err_device:
	class_destroy(disp_class);

err_add:
	cdev_del(disp_cdev);
	unregister_chrdev_region(disp_devno, 1);

err_alloc:
	return ret;
}
module_init(mtk_disp_init);

static void mtk_disp_exit(void)
{
	pr_debug("display_dev_exit\n");

	device_destroy(disp_class, disp_devno);
	class_destroy(disp_class);
	disp_class = NULL;

	cdev_del(disp_cdev);
	unregister_chrdev_region(disp_devno, 1);
}
module_exit(mtk_disp_exit);

/*---------------------------------------------------------------------------*/
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("reference");
MODULE_LICENSE("GPL");
