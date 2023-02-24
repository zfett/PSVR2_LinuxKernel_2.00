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
#include <asm-generic/io.h>

#include "../drivers/misc/mediatek/slicer/mtk_slicer_reg.h"
#include "mtk_wrapper_debugfs.h"

int mtk_wrapper_debugfs_slicer_regs(struct seq_file *s, void *unused)
{
	u32 reg;
	struct devfsinfo *info = (struct devfsinfo *)s->private;

	seq_printf(s, "%08x(%p)\n", info->phys[0], info->regs[0]);
	reg = readl(info->regs[0] + SLCR_CTRL_0);
	if (readl(info->regs[0] + SLCR_VID_CTRL_0) & 0x01) {
		seq_printf(s, "VID:\n");
		reg = readl(info->regs[0] + SLCR_VID_CTRL_1);
		seq_printf(s, "WIDTH  :%4d\n", reg & 0x1FFF);
		seq_printf(s, "HEIGHT :%4d\n", (reg >> 16) & 0x1FFF);
		reg = readl(info->regs[0] + SLCR_VID_CTRL_4);
		seq_printf(s, "VID0   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_VID_CTRL_5);
		seq_printf(s, "VID1   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_VID_CTRL_6);
		seq_printf(s, "VID2   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_VID_CTRL_7);
		seq_printf(s, "VID3   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_VID_CTRL_8);
		seq_printf(s, "VID_EN :%04x\n", reg & 0xffff);
	}
	if (readl(info->regs[0] + SLCR_DSC_CTRL_0) & 0x01) {
		seq_printf(s, "DSC:\n");
		reg = readl(info->regs[0] + SLCR_DSC_CTRL_1);
		seq_printf(s, "WIDTH  :%4d\n", reg & 0x1FFF);
		seq_printf(s, "HEIGHT :%4d\n", (reg >> 16) & 0x1FFF);
		reg = readl(info->regs[0] + SLCR_DSC_CTRL_2);
		seq_printf(s, "BITRATE :%2d\n", (reg >> 24) & 0x3F);
		reg = readl(info->regs[0] + SLCR_DSC_CTRL_4);
		seq_printf(s, "DSC0   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_DSC_CTRL_5);
		seq_printf(s, "DSC1   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_DSC_CTRL_6);
		seq_printf(s, "DSC2   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_DSC_CTRL_7);
		seq_printf(s, "DSC3   :%4d:%4d\n", reg & 0x1fff, (reg >> 16) & 0x1fff);
		reg = readl(info->regs[0] + SLCR_DSC_CTRL_8);
		seq_printf(s, "DSC_EN :%04x\n", reg & 0xffff);
	}

	return 0;
}
