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
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/of_address.h>
#include <linux/fs.h>

struct mtk_chipinfo_device {
	struct miscdevice mdev;
	unsigned int phy_base;
	unsigned int phy_length;
};

static int mtk_chipinfo_mmap(struct file *flip, struct vm_area_struct *vm)
{
	struct mtk_chipinfo_device *chipinfo = container_of(
		flip->private_data, struct mtk_chipinfo_device, mdev);

	if ((vm->vm_end - vm->vm_start) > chipinfo->phy_length)
		return -EINVAL;

	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	if (remap_pfn_range(
		vm, vm->vm_start, chipinfo->phy_base >> PAGE_SHIFT,
		vm->vm_end - vm->vm_start, vm->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static const struct file_operations mtk_chipinfo_fops = {
	.owner = THIS_MODULE,
	.mmap = mtk_chipinfo_mmap,
};

static int mtk_chipinfo_probe(struct platform_device *pdev)
{
	struct mtk_chipinfo_device *chipinfo;
	struct resource res;
	int rc;

	chipinfo = kzalloc(sizeof(struct mtk_chipinfo_device), GFP_KERNEL);
	if (!chipinfo)
		return -ENOMEM;

	rc = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (rc < 0) {
		pr_err("Unable to get resource\n");
		goto of_address_to_res_error;
	}

	chipinfo->phy_base = res.start;
	chipinfo->phy_length = res.end - res.start + 1;

	chipinfo->mdev.minor  = MISC_DYNAMIC_MINOR;
	chipinfo->mdev.name = "mtk_chipinfo";
	chipinfo->mdev.fops = &mtk_chipinfo_fops;
	rc = misc_register(&chipinfo->mdev);
	if (rc) {
		pr_err("can't misc_register\n");
		goto of_address_to_res_error;
	}

	platform_set_drvdata(pdev, chipinfo);

	pr_info("chipinfo base address (0x%x->0x%x)\n",
		chipinfo->phy_base,
		chipinfo->phy_base + chipinfo->phy_length);

	return 0;

of_address_to_res_error:
	kfree(chipinfo);
	return rc;
}

static int mtk_chipinfo_remove(struct platform_device *pdev)
{
	struct mtk_chipinfo_device *chipinfo = platform_get_drvdata(pdev);

	misc_deregister(&chipinfo->mdev);
	kfree(chipinfo);

	return 0;
}

static const struct of_device_id mtk_chipinfo_of_ids[] = {
	{ .compatible = "mediatek,chipinfo", },
	{}
};

static struct platform_driver mtk_chipinfo_driver = {
	.probe = mtk_chipinfo_probe,
	.remove = mtk_chipinfo_remove,
	.driver = {
		.name  = "mtk_chipinfo",
		.owner = THIS_MODULE,
		.of_match_table = mtk_chipinfo_of_ids,
	}
};
module_platform_driver(mtk_chipinfo_driver);
MODULE_DESCRIPTION("MTK Chip Info Driver");
MODULE_LICENSE("GPL");
