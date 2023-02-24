/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>

#include <sce_km_defs.h>

#include <linux/mailbox/mtk-cmdq-mailbox.h>

#include "mtk_wrapper_util.h"

#define GCE_NUM                 (3)
#define CMDQ_THR_ENABLE_TASK	0x04
#define CMDQ_THR_IRQ_STATUS		0x10
#define CMDQ_THR_CURR_ADDR		0x20
#define CMDQ_THR_END_ADDR		0x24

#define CMDQ_THR_IRQ_DONE		0x1
#define CMDQ_THR_IRQ_ERROR		0x12
#define CMDQ_THR_IRQ_EN			(CMDQ_THR_IRQ_ERROR | CMDQ_THR_IRQ_DONE)

#define CMD_DUMP_GCE "dump"

static struct cmdq *s_cmdq[GCE_NUM];
static const char *s_gce_label[GCE_NUM] = {"gce0", "gce4", "gce5"};

static int mtk_wrapper_gce_dump(void)
{
	int i, j;
	unsigned long curr_pa;
	unsigned long end_pa;
	u32 irq_flags, enable;

	for (i = 0; i < GCE_NUM; i++) {
		if (!s_cmdq[i]) {
			return 0;
		}
		pr_err("------- %s ------\n", s_gce_label[i]);
		pr_err("      STATE   IRQ\n");
		for (j = 0; j < CMDQ_THR_MAX_COUNT; j++) {
			struct cmdq_thread *thread;

			thread = &s_cmdq[i]->thread[j];
			curr_pa = readl(thread->base + CMDQ_THR_CURR_ADDR);
			end_pa  = readl(thread->base + CMDQ_THR_END_ADDR);
			irq_flags = readl(thread->base + CMDQ_THR_IRQ_STATUS);
			enable  = readl(thread->base + CMDQ_THR_ENABLE_TASK);
			pr_err("[%02d] %08x %08x\n", j, enable, irq_flags);
			if (thread->pkt) {
				pr_err("====== PKT =====\n");
				cmdq_buf_dump(thread, (curr_pa != end_pa), readl(thread->base + CMDQ_THR_CURR_ADDR));
			}
		}
	}
	return 0;
}

static long mtk_wrapper_gce_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppops)
{
	char buf[16];
	size_t ret;

	memset(buf, 0, sizeof(buf));

	if (count > (sizeof(buf) - 1)) {
		count = sizeof(buf) - 1;
	}

	if (copy_from_user(buf, ubuf, count)) {
		return -EFAULT;
	}
	buf[count] = '\0';

	ret = strncmp(buf, CMD_DUMP_GCE, strlen(CMD_DUMP_GCE));
	if (ret == 0) {
		ret = mtk_wrapper_gce_dump();
		return ret == 0 ? count : ret;
	}
	return count;
}

static const struct file_operations mtk_wrapper_gce_fops = {
	.write = mtk_wrapper_gce_write
};

int init_module_mtk_wrapper_gce(void)
{
	struct platform_device *cmdq;
	struct platform_device *pdev;
	struct device_node *node;

	int ret;
	int i;

	pr_info("%s start\n", __func__);
	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_GCE, SCE_KM_GCE_DEVICE_NAME, &mtk_wrapper_gce_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_GCE_DEVICE_NAME, ret);
		return ret;
	}

	cmdq = pdev_find_dt("sie,cmdq");
	if (!cmdq) {
		return 0;
	}

	for (i = 0; i < GCE_NUM; i++) {
		node = of_parse_phandle(cmdq->dev.of_node, "mediatek,gce", i);
		if (!node) {
			return -EINVAL;
		}
		pdev = of_find_device_by_node(node);
		if (!pdev || !pdev->dev.driver) {
			return -EINVAL;
		}
		s_cmdq[i] = platform_get_drvdata(pdev);
	}
	return 0;
}

void cleanup_module_mtk_wrapper_gce(void)
{
}
