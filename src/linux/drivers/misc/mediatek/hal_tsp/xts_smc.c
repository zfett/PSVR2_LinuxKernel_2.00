/*
 * Copyright (C) 2015 MediaTek Inc.
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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/arm-smccc.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <uapi/mediatek/mtk_xts_ioctl.h>

#define	TSP_XTS_ENC		0xf2002018
#define	TSP_XTS_DEC		0xf2002019
#define TSP_MEM			0xf2002020
#define XTS_ENC			0x1
#define XTS_DEC			0x0
#define XTS_DATA_BUFF_SIZE	0x10000
#define XTS_MAJOR_NUM		67
#define XTS_NAME		"spixts"

static uint8_t *vaddr;

int xts_data_process(unsigned int cmd, struct xts_ioc_param *param)
{
	u64 cnt, xts_len, blk_addr;
	int ret = 0;
	struct arm_smccc_res res;

	pr_info("%s start!!!\n", __func__);

	blk_addr = param->addr;
	for (cnt = 0; cnt < param->data_len; cnt += XTS_DATA_BUFF_SIZE) {
		if ((param->data_len - cnt) < XTS_DATA_BUFF_SIZE)
			xts_len = param->data_len - cnt;
		else
			xts_len = XTS_DATA_BUFF_SIZE;

		ret = copy_from_user(vaddr, param->data_src + cnt, xts_len);
		if (ret)
			break;
		arm_smccc_smc(cmd, blk_addr + cnt, xts_len,
			0, 0, 0, 0, 0, &res);
		ret = copy_to_user(param->data_dst + cnt, vaddr, xts_len);
		if (ret)
			break;
	}

	pr_info("%s end!!!\n", __func__);

	return ret;
}

static long xts_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct xts_ioc_param param;

	ret = copy_from_user(&param, (void *)arg, sizeof(param));
	if (ret < 0) {
		pr_err("%s, err=%x\n", __func__, ret);
		return -1;
	}

	switch (cmd) {
	case XTS_IOCTL_ENC:
		ret = xts_data_process(TSP_XTS_ENC, &param);
		break;
	case XTS_IOCTL_DEC:
		ret = xts_data_process(TSP_XTS_DEC, &param);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

const struct file_operations xts_drv_fops = {
	.read = NULL,
	.write = NULL,
	.unlocked_ioctl = xts_ioctl,
	.open = NULL,
	.release = NULL
};

static int xts_dev_init(void)
{
	struct arm_smccc_res res;

	pr_info("[ATF]Initial XTS_DEV HAL");

	if (register_chrdev(XTS_MAJOR_NUM, XTS_NAME, &xts_drv_fops) < 0) {
		pr_info("<1>%s: can't get major %d\n", XTS_NAME, XTS_MAJOR_NUM);
		return (-EBUSY);
	}

	arm_smccc_smc(TSP_MEM, 0x0, 0, 0, 0, 0, 0, 0, &res);
	pr_info("[ATF]Get PA:0x%lx, size:0x%lx\n", res.a1, res.a2);
	vaddr = (uint8_t *)memremap(res.a1, res.a2, MEMREMAP_WB);
	pr_info("[ATF]Remap VA:0x%p\n", vaddr);

	return 0;
}

static void xts_dev_exit(void)
{
	unregister_chrdev(XTS_MAJOR_NUM, XTS_NAME);
	pr_info("Exit XTS_DEV HAL");
}

module_init(xts_dev_init);
module_exit(xts_dev_exit);
