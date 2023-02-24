/* SIE CONFIDENTIAL
 *
 *  Copyright (C) 2019 Sony Interactive Entertainment Inc.
 *                     All Rights Reserved.
 *
 **/

#include <linux/debugfs.h>
#include <linux/of_platform.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <mtk_wrapper_wdma.h>
#include "../mtk_wrapper_util.h"
#include "mtk_wrapper_debugfs.h"

#include <../drivers/misc/mediatek/imgproc/mtk_wdma_reg.h>
#include <../drivers/misc/mediatek/imgproc/mtk_resizer_reg.h>

#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))

static int mtk_wrapper_debugfs_wdma_regs(struct seq_file *s, void *unused)
{
	struct devfsinfo *info = (struct devfsinfo *)s->private;

	u32 reg[MAX_HW_NUM];
	u32 reg2[MAX_HW_NUM];
	char *fmt_label = "";
	int i;

	seq_printf(s, "%08x(%p)\n", info->phys[0], info->regs[0]);
	seq_printf(s, "STATE1 :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_FLOW_CTRL_DBG);
		seq_printf(s, "%08x ", reg[i] & 0x3FF);
	}
	seq_printf(s, "\nSTATE2 :");

	for (i = 0; i < info->hw_num ; i++) {
		seq_printf(s, "  %c%c%c%c%c%c ",
			   CHECK_BIT(reg[i], 12) ? 'F' : ' ',
			   CHECK_BIT(reg[i], 13) ? 'G' : ' ',
			   CHECK_BIT(reg[i], 14) ? 'R' : ' ',
			   CHECK_BIT(reg[i], 15) ? 'R' : ' ',
			   CHECK_BIT(reg[i], 16) ? 'D' : ' ',
			   CHECK_BIT(reg[i], 17) ? 'I' : ' ');
	}

	seq_printf(s, "\nENABLE :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_EN);
		reg2[i] = readl(info->regs[i] + WDMA_CFG);
		seq_printf(s, "  %c %c %c  ",
			   CHECK_BIT(reg[i], 0) ?  'E' : ' ',
			   CHECK_BIT(reg[i], 16) ?  'P' : ' ',
			   CHECK_BIT(reg2[i], 20) ? 'T' : ' ');
	}
	seq_printf(s, "\nFORMAT :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = (reg2[i] >> 4) & (0xf);
		switch (reg[i]) {
		case 0:
			fmt_label = "RGB565";
			break;
		case 1:
			fmt_label = "RGB888";
			break;
		case 2:
			fmt_label = "RGBA888";
			break;
		case 3:
			fmt_label = "ARGB888";
			break;
		case 7:
			fmt_label = "Y-8bits";
			break;
		case 11:
			fmt_label = "BGRA102";
			break;
		case 15:
			fmt_label = "ARGB210";
			break;
		default:
			fmt_label = "UNKNOWN";
			break;
		}
		seq_printf(s, "%-8s ", fmt_label);
	}

	seq_printf(s, "\nPITCH  :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_DST_W_IN_BYTE);
		seq_printf(s, "%8d ", reg[i] & 0xFFFF);
	}
	seq_printf(s, "\nDST0   :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_DST_ADDR0);
		seq_printf(s, "%08x ", reg[i]);
	}
	seq_printf(s, "\nDST1   :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_DST_ADDR1);
		seq_printf(s, "%08x ", reg[i]);
	}
	seq_printf(s, "\nDST2   :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_DST_ADDR2);
		seq_printf(s, "%08x ", reg[i]);
	}
	seq_printf(s, "\nDSTTL  :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_DST_ADDR0_TDLR);
		seq_printf(s, "%08x ", reg[i]);
	}
	seq_printf(s, "\nINP_CNT:");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_CT_DBG);
		seq_printf(s, "%08x ", reg[i]);
	}

	seq_printf(s, "\nINP_W  :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + WDMA_SRC_SIZE);
		seq_printf(s, "%8d ", reg[i] & 0x3FFF);
	}
	seq_printf(s, "\nINP_H  :");
	for (i = 0; i < info->hw_num; i++) {
		seq_printf(s, "%8d ", (reg[i] >> 16) & 0x3FFF);
	}
	seq_printf(s, "\n");

	return 0;
}

static int mtk_wrapper_debugfs_wdma_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_wrapper_debugfs_wdma_regs, inode->i_private);
}

static struct file_operations mtk_wrapper_debugfs_wdma_fops = {
	.owner = THIS_MODULE,
	.open  = mtk_wrapper_debugfs_wdma_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

extern int mtk_wrapper_debugfs_slicer_regs(struct seq_file *s, void *unused);
static int mtk_wrapper_debugfs_slicer_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_wrapper_debugfs_slicer_regs, inode->i_private);
}

static struct file_operations mtk_wrapper_debugfs_slicer_fops = {
	.owner = THIS_MODULE,
	.open  = mtk_wrapper_debugfs_slicer_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mtk_wrapper_debugfs_resizer_regs(struct seq_file *s, void *unused)
{
	u32 reg[MAX_HW_NUM];
	u32 intr[MAX_HW_NUM];
	struct devfsinfo *info = (struct devfsinfo *)s->private;
	int i;

	seq_printf(s, "%08x(%p)\n", info->phys[0], info->regs[0]);
	seq_printf(s, "STATUS :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + RSZ_ENABLE);
		intr[i] = readl(info->regs[i] + RSZ_INT_FLAG);
		seq_printf(s, "    %c%c%c%c%c",
			   CHECK_BIT(reg[i], 0) ? 'E' : ' ',  /* Rsizer Enable */
			   CHECK_BIT(intr[i], 5) ? 'R' : ' ', /* SOF reset interrupt */
			   CHECK_BIT(intr[i], 4) ? 'Z' : ' ', /* Size error interrupt */
			   CHECK_BIT(intr[i], 1) ? 'e' : ' ', /* frame end interrupt */
			   CHECK_BIT(intr[i], 0) ? 's' : ' '  /* frame start interrupt */
			);
	}

	seq_printf(s, "\nIN_H   :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + RSZ_INPUT_IMAGE);
		seq_printf(s, "%8d ", reg[i] & 0xFFFF);
	}
	seq_printf(s, "\nIN_V   :");
	for (i = 0; i < info->hw_num; i++) {
		seq_printf(s, "%8d ", (reg[i] >> 16) & 0xFFFF);
	}

	seq_printf(s, "\nOUT_H  :");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + RSZ_OUTPUT_IMAGE);
		seq_printf(s, "%8d ", reg[i] & 0xFFFF);
	}
	seq_printf(s, "\nOUT_V  :");
	for (i = 0; i < info->hw_num; i++) {
		seq_printf(s, "%8d ", (reg[i] >> 16) & 0xFFFF);
	}

	seq_printf(s, "\nOUT_BIT:");
	for (i = 0; i < info->hw_num; i++) {
		reg[i] = readl(info->regs[i] + RSZ_OUT_BIT_CONTROL);
		seq_printf(s, "%8s ", CHECK_BIT(reg[i], 16) ? "8-bit" : "10bit");
	}

	seq_printf(s, "\n");

	return 0;
}

static int mtk_wrapper_debugfs_resizer_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_wrapper_debugfs_resizer_regs, inode->i_private);
}

static struct file_operations mtk_wrapper_debugfs_resizer_fops = {
	.owner = THIS_MODULE,
	.open  = mtk_wrapper_debugfs_resizer_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct devfsinfo s_devfs_rule[] = {
	{"mediatek,wdma", 0, "wdma0", &mtk_wrapper_debugfs_wdma_fops, 1, {NULL}, {0} },
	{"mediatek,wdma", 1, "wdma1", &mtk_wrapper_debugfs_wdma_fops, 1, {NULL}, {0} },
	{"mediatek,wdma", 2, "wdma2", &mtk_wrapper_debugfs_wdma_fops, 1, {NULL}, {0} },
	{"mediatek,wdma", 3, "wdma3", &mtk_wrapper_debugfs_wdma_fops, 1, {NULL}, {0} },
	{"mediatek,slicer",  0, "slicer", &mtk_wrapper_debugfs_slicer_fops, 1, {NULL}, {0} },
	{"mediatek,resizer", 0, "resizer0", &mtk_wrapper_debugfs_resizer_fops, 1, {NULL}, {0} },
	{"mediatek,resizer", 1, "resizer1", &mtk_wrapper_debugfs_resizer_fops, 1, {NULL}, {0} },
};

#define VMALLOCINFO_BUF_SIZE (64 * 1024)
int init_module_mtk_wrapper_debugfs(void)
{
	struct dentry *mtk_wrapper_root;
	struct file *fp;
	char *buf;
	char *cur;
	int ret;
	mm_segment_t oldfs;
	char target[32];
	loff_t pos = 0;
	int i, j;

	mtk_wrapper_root = debugfs_create_dir("mtk_wrapper", NULL);
	if (!mtk_wrapper_root) {
		return -ENOMEM;
	}

	buf = kmalloc(VMALLOCINFO_BUF_SIZE, GFP_KERNEL);
	cur = buf;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open("/proc/vmallocinfo", O_RDONLY, 0);
	if (IS_ERR(fp)) {
		pr_err("Can't open /proc/vmallocinfo\n");
		return 0;
	}

	ret = 1;
	while (ret > 0 && cur < buf + VMALLOCINFO_BUF_SIZE) {
		ret = vfs_read(fp, cur, PAGE_SIZE, &pos);
		cur += (ret);
	}
	filp_close(fp, 0);
	set_fs(oldfs);

	for (i = 0; i < ARRAY_SIZE(s_devfs_rule); i++) {
		struct platform_device *pdev;

		ret = mtk_wrapper_display_pipe_probe_pdev(s_devfs_rule[i].compatible, s_devfs_rule[i].idx, &pdev);
		if (ret < 0) {
			return -ENODEV;
		}

		for (j = 0; j < s_devfs_rule[i].hw_num; j++) {
			struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, j);

			if (!res) {
				continue;
			}

			s_devfs_rule[i].phys[j] = (u32)res->start;
			snprintf(target, sizeof(target), "phys=%8x", (unsigned int)res->start);
			cur = strstr(buf, target);
			if (!cur) {
				pr_err("Device(%s:%s) can't find ioremap info\n", s_devfs_rule[i].dev_name, target);
				continue;
			}
			while (*cur != '\n' && cur != buf) {
				cur--;
			}
			cur++;
			*(cur + 18) = '\0';
			ret = kstrtoull(cur, 16, (unsigned long long *)&s_devfs_rule[i].regs[j]);
			if (ret != 0) {
				pr_err("Device(%s:%s) ioremap info parse failed\n", s_devfs_rule[i].dev_name, target);
				*(cur + 18) = ' ';
				continue;
			}
			*(cur + 18) = ' ';
		}
		if (!debugfs_create_file(s_devfs_rule[i].dev_name, S_IRUSR, mtk_wrapper_root,
					 (void *)&s_devfs_rule[i], s_devfs_rule[i].fops)) {
			return -ENOMEM;
		}
	}

	kfree(buf);
	return 0;
}

void cleanup_module_mtk_wrapper_debugfs(void)
{
}
