/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Peggy Jao <peggy.jao@mediatek.com>
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
 * @file mtk_sbrc_debugfs.c
 * sbrc debugfs driver.
 * dump sbrc register status
 * ex: cat /sys/kernel/debug/mediatek/sbrc/dump_all
 */

#ifdef CONFIG_MTK_DEBUGFS
#include <linux/debugfs.h>
#include <linux/dma-attrs.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <soc/mediatek/mtk_sbrc.h>
#include "mtk_sbrc_int.h"
#include "sbrc_reg.h"


/** @ingroup IP_group_sbrc_internal_def_debug
 * @{
 */
#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))
#define HELP_INFO_SBRC \
	"\n" \
	"[USAGE]\n" \
	"   echo [ACTION]... > /sys/kernel/debug/mediatek/sbrc/debug\n" \
	"\n" \
	"[ACTION]\n" \
	"   bufnum  [num]\n" \
	"   depth   [depth height]\n" \
	"   bsize   [size]\n" \
	"   enable  [1:on/0:off]\n" \
	"   swbyp   [1:on/0:off]\n" \
	"   exitbyp\n" \
	"   clrirq  [bitmask]\n" \
	"\n" \

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC GCE cmd test.
 * @param[in]
 *     sbrc: sbrc structure.
 * @param[in]
 *     event_id: GCE event id.
 * @return
 *     0, dump successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
#if 0
static int mtk_sbrc_gce_test(struct mtk_sbrc *sbrc, u32 event_id)
{
	/* 132: gce_w_over_th */
	/* 133: gce_gpu_frame_skip_done */
	/* 134: gce_gpio_timeout */
	/* 135: gpu_n_sbrc_rw_clash */
	/* 136: gce_w_incomplete */
	/* 137: gce_gpu_render_fault */

	u32 *va, mask = 0xffffffff;
	dma_addr_t pa;
	struct dma_attrs dma_attrs;
	struct device *dev = sbrc->dev;

	init_dma_attrs(&dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE |
			DMA_ATTR_FORCE_CONTIGUOUS, &dma_attrs);

	va = (u32 *)dma_alloc_attrs(dev, sizeof(u32),
				&pa, GFP_KERNEL, &dma_attrs);
	*va = 99;
	dev_dbg(dev, "Init value: %d for GCE event: %d\n", *va, event_id);
	dev_dbg(dev, "pa: %llx\n", pa);

	sbrc->cmdq_client = cmdq_mbox_create(dev, 0);
	if (IS_ERR(sbrc->cmdq_client)) {
		dev_err(dev, "failed to create mailbox cmdq_client\n");
		return PTR_ERR(sbrc->cmdq_client);
	}

	cmdq_pkt_create(&sbrc->cmdq_pkt);
	cmdq_pkt_wfe(sbrc->cmdq_pkt, event_id);

	/* ts subsys = 10 (0x102d) */
	/* include/dt-bindings/gce/mt3612-gce.h */
	/* GCE_REG[GCE_SPR1] = 0x102d3020(us) */
	cmdq_pkt_read(sbrc->cmdq_pkt, 10, 0x3020, GCE_SPR1);
	/* GCE_REG[GCE_SPR0] = pa[i] */
	cmdq_pkt_assign_command(sbrc->cmdq_pkt, GCE_SPR0, pa);
	/* *GCE_REG[GCE_SPR0] = GCE_REG[GCE_SPR1] */
	cmdq_pkt_store_reg(sbrc->cmdq_pkt, GCE_SPR0, GCE_SPR1, mask);
	cmdq_pkt_flush(sbrc->cmdq_client, sbrc->cmdq_pkt);
	cmdq_pkt_destroy(sbrc->cmdq_pkt);
	cmdq_mbox_destroy(sbrc->cmdq_client);

	dev_dbg(dev, "Updated value: %d for GCE event: %d\n", *va, event_id);
	dev_dbg(dev, "pa: %llx\n", pa);

	return 0;
}
#endif

/** @ingroup IP_group_sbrc_internal_function
 * @par Description
 *     SBRC dump irq status.
 * @param[in]
 *     sbrc: sbrc structure.
 * @return
 *     0, dump successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_sbrc *sbrc = s->private;
	u32 reg;

	seq_puts(s, "-------------------- SBRC DEBUG --------------------\n");

	reg = readl(sbrc->regs + SBRC_IRQ_STATUS);
	seq_puts(s, "[IRQ Status Dump]\n");
	seq_printf(s, "W_INCOMPLETE           :%8x\n", CHECK_BIT(reg, 0) ? 1:0);
	seq_printf(s, "GPU_RENDER_FAULT       :%8x\n", CHECK_BIT(reg, 1) ? 1:0);
	seq_printf(s, "GPU_N_SBRC_RW_CLASH    :%8x\n", CHECK_BIT(reg, 2) ? 1:0);
	seq_printf(s, "GPU_FRAME_SKIP_DONE    :%8x\n", CHECK_BIT(reg, 4) ? 1:0);
	seq_printf(s, "GPIO_TIMEOUT           :%8x\n\n",
		   CHECK_BIT(reg, 5) ? 1:0);

	seq_puts(s, "[Basic SBRC Settings]\n");
	reg = readl(sbrc->regs + SBRC_ENABLE);
	seq_printf(s, "SBRC_EN                :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_BUFFER_NUM_CFG, SBRC_BUFFER_NUM);
	seq_printf(s, "BUFFER_NUM             :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_BUFFER_DEPTH_CFG,
				SBRC_BUFFER_DEPTH);
	seq_printf(s, "BUFFER_DEPTH           :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_BUFFER_SIZE_CFG, SBRC_BUFFER_SIZE);
	seq_printf(s, "BUFFER_SIZE            :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_TOTAL_STRIP_NUM_CFG,
				SBRC_TOTAL_STRIP_NUM);
	seq_printf(s, "STRIP_NUM              :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_LINE_REMAINDER_CFG,
				SBRC_LINE_REMAINDER);
	seq_printf(s, "LINE_REMAINDER         :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_URGENT_ENABLE, SBRC_URGENT_EN);
	seq_printf(s, "URGENT_EN              :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_URGENT_TH_VAL,
					SBRC_WRITE_OVER_READ_VAL);
	seq_printf(s, "URGENT_TH_VAL          :%8x\n", reg);

	reg = reg_read_mask(sbrc->regs, SBRC_BYPASS_MODE, SBRC_BYPASS_MODE_SET);
	seq_printf(s, "BYPASS_MODE_SET        :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_BYPASS_MODE, SBRC_BYPASS_MODE_CLR);
	seq_printf(s, "BYPASS_MODE_CLR        :%8x\n\n", (reg >> 1));

	seq_puts(s, "[SBRC Reg Status]\n");
	reg = reg_read_mask(sbrc->regs, SBRC_MON, ALL_BYPASS_MODE_STA);
	seq_printf(s, "ALL_BYPASS_MODE_STA    :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_MON, CFG_BYPASS_MODE_STA);
	seq_printf(s, "CFG_BYPASS_MODE_STA    :%8x\n", (reg >> 1));
	reg = reg_read_mask(sbrc->regs, SBRC_MON, EXT_BYPASS_MODE_STA);
	seq_printf(s, "EXT_BYPASS_MODE_STA    :%8x\n", (reg >> 2));
	reg = reg_read_mask(sbrc->regs, SBRC_BUFFER_STA, SBRC_WRITE_BUFFER_STA);
	seq_printf(s, "WRITE_BUFFER_STA       :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, SBRC_BUFFER_STA, SBRC_READ_BUFFER_STA);
	seq_printf(s, "READ_BUFFER_STA        :%8x\n", (reg >> 8));
	reg = reg_read_mask(sbrc->regs, SBRC_MON, SBRC_URGENT_STA);
	seq_printf(s, "URGENT_STA             :%8x\n\n", (reg >> 3));

	seq_puts(s, "[GPIO Register Status]\n");
	reg = reg_read_mask(sbrc->regs, GPIO_SKIP_FRAME_CONTROL_APB,
					GPIO_SKIP_FRAME_CONTROL);
	seq_printf(s, "SKIP_FRAME_CONTROL     :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, GPIO_PENDING_BUF_IDX_VALID_APB,
					PENDING_BUF_IDX_VALID);
	seq_printf(s, "PENDING_BUF_IDX_VALID  :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, GPIO_BUF_AVA_BITMAP_0_31,
					GPIO_BUF_AVA_BITMAP_0_7);
	seq_printf(s, "AVA_BITMAP_0_7         :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, GPIO_PENDING_BUF_IDX_APB,
					PENDING_BUF_IDX);
	seq_printf(s, "PENDING_BUF_IDX        :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, GPIO_RENDER_STRIP_INDEX_APB,
					GPIO_RENDER_STRIP_INDEX);
	seq_printf(s, "RENDER_STRIP_INDEX     :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, GPIO_BUFFER_RENDER_COMPLETE_APB,
					GPIO_BUFFER_RENDER_COMPLETE);
	seq_printf(s, "BUFFER_RENDER_COMPLETE :%8x\n", reg);
	reg = reg_read_mask(sbrc->regs, GPIO_BUFFER_RENDER_FAULT_APB,
					GPIO_BUFFER_RENDER_FAULT);
	seq_printf(s, "BUFFER_RENDER_FAULT    :%8x\n", reg);
	seq_puts(s, "----------------------------------------------------\n");

	return 0;
}

/** @ingroup IP_group_sbrc_internal_function_debug
 * @par Description
 *     This is sbrc debugfs file open function.
 * @param[in] inode: device inode
 * @param[in] file: file pointer
 * @return
 *     0, file open success. \n
 *     Negative, file open failed.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_sbrc_debug_show, inode->i_private);
}

/** @ingroup IP_group_sbrc_internal_function_debug
 * @par Description
 *     This is sbrc test handler function.
 * @param[in]
 *     cmd: test command
 * @param[in]
 *     sbrc: sbrc driver data struct
 * @return
 *     0, success. \n
 *     Negative, failed.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_sbrc_process_cmd(char *cmd, struct mtk_sbrc *sbrc)
{
	char *opt, *np;
	u32 val, val2;

	dev_info(sbrc->dev, "debug_cmd: %s\n", cmd);
	opt = strsep(&cmd, " ");

	if (strncmp(opt, "bufnum", 7) == 0) {
		/* set sbrc buffer number */
		/* echo bufnum 2 > sys/kernel/debug/mediatek/sbrc/debug */
		char *p = (char *)opt + 7;

		if (kstrtouint(p, 10, &val))
			goto errcode;
		mtk_sbrc_set_buffer_num(sbrc->dev, val);
		dev_dbg(sbrc->dev, "SBRC buffer num: %d\n", val);

	} else if (strncmp(opt, "depth", 6) == 0) {
		/* set sbrc buffer depth */
		/* echo depth 128 1080 > sys/kernel/debug/mediatek/sbrc/debug */
		char *p = (char *)opt + 6;

		np = strsep(&p, " ");
		if (kstrtouint(np, 10, &val))
			goto errcode;

		np = strsep(&p, " ");
		if (kstrtouint(np, 10, &val2))
			goto errcode;

		mtk_sbrc_set_buffer_depth(sbrc->dev, val2, val);
		dev_dbg(sbrc->dev, "SBRC buffer depth/height: %d\t%d\n",
			val, val2);

	} else if (strncmp(opt, "bsize", 6) == 0) {
		/* set sbrc buffer size */
		/* echo bsize 4000 > sys/kernel/debug/mediatek/sbrc/debug */
		char *p = (char *)opt + 6;

		if (kstrtouint(p, 10, &val))
			goto errcode;
		mtk_sbrc_set_buffer_size(sbrc->dev, val);
		dev_dbg(sbrc->dev, "SBRC buffer width: %d\n", val);

	} else if (strncmp(opt, "enable", 7) == 0) {
		/* set sbrc enable */
		/* echo enable 1 > sys/kernel/debug/mediatek/sbrc/debug */
		char *p = (char *)opt + 7;

		if (kstrtouint(p, 10, &val))
			goto errcode;
		if (val)
			mtk_sbrc_enable(sbrc->dev);
		else
			mtk_sbrc_disable(sbrc->dev);
		dev_dbg(sbrc->dev, "SBRC enable: %s\n",
				((val == 1) ? "TRUE" : "FALSE"));

	} else if ((strncmp(opt, "swbyp", 6) == 0)) {
		/* enter sbrc bypass mode by SW */
		/* echo swbyp 1 > sys/kernel/debug/mediatek/sbrc/debug */
		char *p = (char *)opt + 6;

		if (kstrtouint(p, 10, &val))
			goto errcode;
		mtk_sbrc_set_bypass(sbrc->regs, val);
		dev_dbg(sbrc->dev, "SBRC enter bypass mode by SW: %s\n",
				((val == 1) ? "TRUE" : "FALSE"));

	} else if ((strncmp(opt, "exitbyp", 7) == 0)) {
		/* exit sbrc bypass mode*/
		/* echo exitbyp > sys/kernel/debug/mediatek/sbrc/debug */
		mtk_sbrc_clr_bypass(sbrc->dev, NULL);
		dev_dbg(sbrc->dev, "SBRC exit bypass mode\n");

	} else if ((strncmp(opt, "clrirq", 7) == 0)) {
		/* clear irq status */
		/* echo clrirq BITMASK > sys/kernel/debug/mediatek/sbrc/debug */
		char *p = (char *)opt + 7;

		dev_dbg(sbrc->dev, "W_INCOMPLETE_CLR: BIT(16)\n");
		dev_dbg(sbrc->dev, "RENDER_FAULT_CLR: BIT(17)\n");
		dev_dbg(sbrc->dev, "N_SBRC_RW_CLASH_CLR: BIT(18)\n");
		dev_dbg(sbrc->dev, "GPIO_TIMEOUT_CLR: BIT(21)\n");
		dev_dbg(sbrc->dev, "FRAME_SKIP_DONE_CLR: BIT(20)\n\n");

		if (kstrtouint(p, 10, &val))
			goto errcode;

		dev_dbg(sbrc->dev, "clr BITMASK(%d) 0x%x\n", val, (1 << val));
		mtk_sbrc_irq_clear(sbrc->regs, 1 << val);
		dev_dbg(sbrc->dev, "SBRC clear IRQ status done!\n");

#if 0
	} else if ((strncmp(opt, "gcetest", 8) == 0)) {
		/* gce hw read ts reg test */
		/* echo gce_test event > sys/kernel/debug/mediatek/sbrc/debug */

		char *p = (char *)opt + 8;

		if (kstrtouint(p, 10, &val))
			goto errcode;
		mtk_sbrc_gce_test(sbrc, val);

#endif
	} else
		goto errcode;

	return 0;

errcode:
	dev_err(sbrc->dev, "invalid dbg command\n");
	return -1;
}

/** @ingroup IP_group_sbrc_internal_function_debug
 * @par Description
 *     This is sbrc debugfs file write function.
 * @param[in]
 *     file: file pointer
 * @param[in]
 *     ubuf: user space buffer pointer
 * @param[in]
 *     count: number of data to write
 * @param[in,out]
 *     ppos: offset
 * @return
 *     Negative, file write failed. \n
 *     Others, number of data wrote.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mtk_sbrc_debug_write(struct file *file, const char __user *ubuf,
				   size_t count, loff_t *ppos)
{
	char str_buf[64];
	size_t ret;
	struct mtk_sbrc *sbrc;
	struct seq_file *seq_ptr;

	ret = count;
	memset(str_buf, 0, sizeof(str_buf));

	if (count > (sizeof(str_buf) - 1))
		count = sizeof(str_buf) - 1;

	if (copy_from_user(str_buf, ubuf, count))
		return -EFAULT;

	seq_ptr = (struct seq_file *)file->private_data;
	str_buf[count] = 0;

	sbrc = (struct mtk_sbrc *)seq_ptr->private;
	mtk_sbrc_process_cmd(str_buf, sbrc);

	return ret;
}

/** @ingroup IP_group_sbrc_internal_function_debug
 * @par Description
 *     This is sbrc debugfs file read function.
 * @param[in]
 *     file: file pointer
 * @param[in]
 *     ubuf: user space buffer pointer
 * @param[in]
 *     count: number of data to read
 * @param[in,out]
 *     ppos: offset
 * @return
 *     Negative, file read failed. \n
 *     Others, number of data read.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
#if 0
static ssize_t mtk_sbrc_debug_read(struct file *file, char __user *ubuf,
				size_t count, loff_t *ppos)
{
	/* Enter following cmd will show the usage of SBRC debugfs */
	/* cat /sys/kernel/debug/mediatek/sbrc/debug */
	return simple_read_from_buffer(ubuf, count, ppos, HELP_INFO_SBRC,
					strlen(HELP_INFO_SBRC));
}
#endif
/**
 * @brief a file operation structure for sbrc_debug driver
 */
static const struct file_operations mtk_sbrc_debug_fops = {
	.open = mtk_sbrc_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = mtk_sbrc_debug_write,
};

/** @ingroup IP_group_sbrc_internal_function_debug
 * @par Description
 *     Register this test function and create file entry in debugfs.
 * @param[in]
 *     sbrc: sbrc driver data struct
 * @return
 *     0, debugfs init success \n
 *     Others, debugfs init failed
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_sbrc_debugfs_init(struct mtk_sbrc *sbrc)
{
	struct dentry *debug_dentry;

	/* debug info create */
	if (!sbrc->debugfs)
		sbrc->debugfs = debugfs_create_dir("sbrc", mtk_debugfs_root);

	if (IS_ERR(sbrc->debugfs)) {
		dev_dbg(sbrc->dev, "failed to create sbrc debugfs root dir.\n");
		return PTR_ERR(sbrc->debugfs);
	}

	debug_dentry = debugfs_create_file("debug", 0664, sbrc->debugfs,
					   (void *)sbrc, &mtk_sbrc_debug_fops);

	if (IS_ERR(debug_dentry))
		return PTR_ERR(debug_dentry);

	return 0;
}

/** @ingroup IP_group_sbrc_internal_function_debug
 * @par Description
 *     Unregister this test function and remove file entry from debugfs.
 * @param[in]
 *     sbrc: sbrc driver data struct
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_sbrc_debugfs_exit(struct mtk_sbrc *sbrc)
{
	debugfs_remove_recursive(sbrc->debugfs);
	sbrc->debugfs = NULL;
}
#endif
