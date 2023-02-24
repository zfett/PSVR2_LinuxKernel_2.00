/**
 * @file mt3611-afe-debug.c
 * Mediatek audio debug function
 *
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mt3611-afe-common.h"
#include "mt3611-afe-debug.h"
#include "mt3611-afe-regs.h"
#include "mt3611-afe-util.h"
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#ifdef CONFIG_DEBUG_FS

struct mt3611_afe_debug_fs {
	char *fs_name;
	const struct file_operations *fops;
};

struct afe_dump_reg_attr {
	uint32_t offset;
	char *name;
};

#define DUMP_REG_ENTRY(reg) {reg, #reg}

static const struct afe_dump_reg_attr common_dump_regs[] = {
	DUMP_REG_ENTRY(AUDIO_TOP_CON0),
	DUMP_REG_ENTRY(AUDIO_TOP_CON1),
	DUMP_REG_ENTRY(AUDIO_TOP_CON3),
	DUMP_REG_ENTRY(AFE_FS_TIMING_CON),
	DUMP_REG_ENTRY(AFE_MEMIF_MSB),
	DUMP_REG_ENTRY(AFE_MEMIF_PBUF_SIZE),
	DUMP_REG_ENTRY(AFE_MEMIF_HD_MODE),
	DUMP_REG_ENTRY(AFE_MEMIF_HDALIGN),
	DUMP_REG_ENTRY(AFE_DAC_CON0),
	DUMP_REG_ENTRY(AFE_DAC_CON1),
	DUMP_REG_ENTRY(AFE_DAC_MON),
	DUMP_REG_ENTRY(AFE_IRQ_MCU_CON),
	DUMP_REG_ENTRY(AFE_IRQ_MCU_EN),
	DUMP_REG_ENTRY(AFE_IRQ_DELAY_CON),
};

static const struct afe_dump_reg_attr i2s_dump_regs[] = {
	DUMP_REG_ENTRY(AFE_I2S_CON1),
	DUMP_REG_ENTRY(AFE_I2S_CON2),
	DUMP_REG_ENTRY(AFE_I2S_SHIFT_CON0),
	DUMP_REG_ENTRY(AFE_CONN_MUX_CFG),
	DUMP_REG_ENTRY(AFE_DL12_BASE),
	DUMP_REG_ENTRY(AFE_DL12_CUR),
	DUMP_REG_ENTRY(AFE_DL12_END),
	DUMP_REG_ENTRY(AFE_VUL12_BASE),
	DUMP_REG_ENTRY(AFE_VUL12_CUR),
	DUMP_REG_ENTRY(AFE_VUL12_END),
	DUMP_REG_ENTRY(AFE_IRQ0_MCU_CNT),
	DUMP_REG_ENTRY(AFE_IRQ1_MCU_CNT),
	DUMP_REG_ENTRY(AFE_DL12_RD_COUNTER),
	DUMP_REG_ENTRY(AFE_UL12_WR_COUNTER),
	DUMP_REG_ENTRY(AFE_DL12_RD_BUF_MON),
	DUMP_REG_ENTRY(AFE_VUL12_WR_BUF_MON),
	DUMP_REG_ENTRY(AFE_MEMIF_MON4),
	DUMP_REG_ENTRY(AFE_MEMIF_MON5),
	DUMP_REG_ENTRY(AFE_MEMIF_MON6),
	DUMP_REG_ENTRY(AFE_MEMIF_MON7),
	DUMP_REG_ENTRY(AFE_MEMIF_MON14),
	DUMP_REG_ENTRY(AFE_MEMIF_MON15),
	DUMP_REG_ENTRY(AFE_MEMIF_MON16),
	DUMP_REG_ENTRY(AFE_MEMIF_MON17),
	DUMP_REG_ENTRY(AFE_SGEN_CON0),
	DUMP_REG_ENTRY(AFE_SGEN_CON2),
	DUMP_REG_ENTRY(AFE_IRQ_DELAY_CNT0),
#ifdef FPGA_ONLY
	DUMP_REG_ENTRY(FPGA_CFG1),
#endif
};

static const struct afe_dump_reg_attr tdm_dump_regs[] = {
	DUMP_REG_ENTRY(AFE_TDM_CON1),
	DUMP_REG_ENTRY(AFE_TDM_CON2),
	DUMP_REG_ENTRY(AFE_TDM_OUT_CON0),
	DUMP_REG_ENTRY(AFE_TDM_IN_CON1),
	DUMP_REG_ENTRY(AFE_TDM_IN_CON2),
	DUMP_REG_ENTRY(AFE_TDM_SHIFT_CON0),
	DUMP_REG_ENTRY(AFE_CONN_TDMOUT),
	DUMP_REG_ENTRY(AFE_TDM_OUT_BASE),
	DUMP_REG_ENTRY(AFE_TDM_OUT_CUR),
	DUMP_REG_ENTRY(AFE_TDM_OUT_END),
	DUMP_REG_ENTRY(AFE_TDM_IN_BASE),
	DUMP_REG_ENTRY(AFE_TDM_IN_WR_CUR),
	DUMP_REG_ENTRY(AFE_TDM_IN_END),
	DUMP_REG_ENTRY(AFE_IRQ5_MCU_CNT),
	DUMP_REG_ENTRY(AFE_IRQ6_MCU_CNT),
	DUMP_REG_ENTRY(AFE_IRQ5_MCU_CNT_MON),
	DUMP_REG_ENTRY(AFE_IRQ6_MCU_CNT_MON),
	DUMP_REG_ENTRY(AFE_TDM_OUT_COUNTER),
	DUMP_REG_ENTRY(AFE_TDM_IN_COUNTER),
	DUMP_REG_ENTRY(AFE_TDM_OUT_BUF_MON),
	DUMP_REG_ENTRY(AFE_TDM_IN_BUF_MON),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON_TDM),
	DUMP_REG_ENTRY(AFE_SINEGEN_CON_TDM_IN),
	DUMP_REG_ENTRY(AFE_IRQ_DELAY_CNT2),
	DUMP_REG_ENTRY(AFE_IRQ_DELAY_CNT3),
#ifdef FPGA_ONLY
	DUMP_REG_ENTRY(FPGA_CFG1),
#endif
};

static const struct afe_dump_reg_attr mphone_dump_regs[] = {
	DUMP_REG_ENTRY(AFE_AWB_BASE),
	DUMP_REG_ENTRY(AFE_AWB_CUR),
	DUMP_REG_ENTRY(AFE_AWB_END),
	DUMP_REG_ENTRY(AFE_MPHONE_MULTI_CON0),
	DUMP_REG_ENTRY(AFE_MPHONE_MULTI_CON1),
	DUMP_REG_ENTRY(AFE_IRQ_MCU_CNT4),
	DUMP_REG_ENTRY(AFE_IRQ4_MCU_CNT_MON),
	DUMP_REG_ENTRY(AFE_AWB_WR_COUNTER),
	DUMP_REG_ENTRY(AFE_AWB_WR_BUF_MON),
	DUMP_REG_ENTRY(AFE_MEMIF_MON12),
	DUMP_REG_ENTRY(AFE_MEMIF_MON13),
	DUMP_REG_ENTRY(AFE_IRQ_DELAY_CNT2),
};

/** @ingroup type_group_afe_InFn
 * @par Description
 *     dump register settings into string buffer.
 * @param[out]
 *     user_buf: buffer to write to
 * @param[in]
 *     count: buffer length
 * @param[in,out]
 *     pos: file offset
 * @param[in]
 *     afe: AFE data
 * @param[in]
 *     regs: register attribute array
 * @param[in]
 *     regs_len: length of register attribute array
 * @return
 *     total data bytes that has written
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mt3611_afe_dump_registers(char __user *user_buf,
					 size_t count,
					 loff_t *pos,
					 struct mtk_afe *afe,
					 const struct afe_dump_reg_attr *regs,
					 size_t regs_len)
{
	ssize_t ret, i;
	char *buf;
	unsigned int reg_value;
	int n = 0;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mt3611_afe_enable_main_clk(afe);

	for (i = 0; i < ARRAY_SIZE(common_dump_regs); i++) {
		if (regmap_read(afe->regmap, common_dump_regs[i].offset,
				&reg_value))
			n += scnprintf(buf + n, count - n, "%s = N/A\n",
				       common_dump_regs[i].name);
		else
			n += scnprintf(buf + n, count - n, "%s = 0x%x\n",
				       common_dump_regs[i].name, reg_value);
	}

	for (i = 0; i < regs_len; i++) {
		if (regmap_read(afe->regmap, regs[i].offset, &reg_value))
			n += scnprintf(buf + n, count - n, "%s = N/A\n",
				       regs[i].name);
		else
			n += scnprintf(buf + n, count - n, "%s = 0x%x\n",
				       regs[i].name, reg_value);
	}

	mt3611_afe_disable_main_clk(afe);

	ret = simple_read_from_buffer(user_buf, count, pos, buf, n);

	kfree(buf);

	return ret;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s read function for struct file_operations.
 * @param[in]
 *     file: file object
 * @param[out]
 *     user_buf: buffer to write to
 * @param[in]
 *     count: buffer length
 * @param[in,out]
 *     pos: file offset
 * @return
 *     total data bytes that has written
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mt3611_afe_i2s_read_file(struct file *file,
					char __user *user_buf,
					size_t count,
					loff_t *pos)
{
	struct mtk_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt3611_afe_dump_registers(user_buf, count, pos, afe,
					i2s_dump_regs,
					ARRAY_SIZE(i2s_dump_regs));

	return ret;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     i2s write function for struct file_operations.
 * @param[in]
 *     file: file object
 * @param[in]
 *     user_buf: buffer to read from
 * @param[in]
 *     count: buffer length
 * @param[in,out]
 *     pos: file offset
 * @return
 *     total data bytes that has read
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mt3611_afe_i2s_write_file(struct file *file,
					 const char __user *user_buf,
					 size_t count,
					 loff_t *pos)
{
	char buf[64];
	size_t buf_size;
	char *start = buf;
	char *reg_str;
	char *value_str;
	const char delim[] = " ,";
	unsigned long reg, value;
	struct mtk_afe *afe = file->private_data;

	buf_size = min(count, (sizeof(buf) - 1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	buf[buf_size] = 0;

	reg_str = strsep(&start, delim);
	if (!reg_str || !strlen(reg_str))
		return -EINVAL;

	value_str = strsep(&start, delim);
	if (!value_str || !strlen(value_str))
		return -EINVAL;

	if (kstrtoul(reg_str, 16, &reg))
		return -EINVAL;

	if (kstrtoul(value_str, 16, &value))
		return -EINVAL;

	mt3611_afe_enable_main_clk(afe);

	regmap_write(afe->regmap, reg, value);

	mt3611_afe_disable_main_clk(afe);

	return buf_size;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     tdm read function for struct file_operations.
 * @param[in]
 *     file: file object
 * @param[out]
 *     user_buf: buffer to write to
 * @param[in]
 *     count: buffer length
 * @param[in,out]
 *     pos: file offset
 * @return
 *     total data bytes that has written
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mt3611_afe_tdm_read_file(struct file *file,
					char __user *user_buf,
					size_t count,
					loff_t *pos)
{
	struct mtk_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt3611_afe_dump_registers(user_buf, count, pos, afe,
					tdm_dump_regs,
					ARRAY_SIZE(tdm_dump_regs));

	return ret;
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     mphone multi read function for struct file_operations.
 * @param[in]
 *     file: file object
 * @param[out]
 *     user_buf: buffer to write to
 * @param[in]
 *     count: buffer length
 * @param[in,out]
 *     pos: file offset
 * @return
 *     total data bytes that has written
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mt3611_afe_mphone_read_file(struct file *file,
					   char __user *user_buf,
					   size_t count,
					   loff_t *pos)
{
	struct mtk_afe *afe = file->private_data;
	ssize_t ret;

	if (*pos < 0 || !count)
		return -EINVAL;

	ret = mt3611_afe_dump_registers(user_buf, count, pos, afe,
					mphone_dump_regs,
					ARRAY_SIZE(mphone_dump_regs));

	return ret;
}

static const struct file_operations mt3611_afe_i2s_fops = {
	.open = simple_open,
	.read = mt3611_afe_i2s_read_file,
	.write = mt3611_afe_i2s_write_file,
	.llseek = default_llseek,
};

static const struct file_operations mt3611_afe_tdm_fops = {
	.open = simple_open,
	.read = mt3611_afe_tdm_read_file,
	.llseek = default_llseek,
};

static const struct file_operations mt3611_afe_mphone_fops = {
	.open = simple_open,
	.read = mt3611_afe_mphone_read_file,
	.llseek = default_llseek,
};

static const struct mt3611_afe_debug_fs afe_dbg_fs[MT3611_AFE_DEBUGFS_NUM] = {
	{"mtkafei2s", &mt3611_afe_i2s_fops},
	{"mtkafetdm", &mt3611_afe_tdm_fops},
	{"mtkafemphone", &mt3611_afe_mphone_fops},
};

#endif

/** @ingroup type_group_afe_InFn
 * @par Description
 *     create debugfs file.
 * @param[in]
 *     afe: AFE data
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt3611_afe_init_debugfs(struct mtk_afe *afe)
{
#ifdef CONFIG_DEBUG_FS
	int i;

	for (i = 0; i < ARRAY_SIZE(afe_dbg_fs); i++) {
		afe->debugfs_dentry[i] =
			debugfs_create_file(afe_dbg_fs[i].fs_name, 0644,
					    NULL, afe, afe_dbg_fs[i].fops);
		if (!afe->debugfs_dentry[i])
			dev_warn(afe->dev,
				 "%s failed to create %s debugfs file\n",
				 __func__, afe_dbg_fs[i].fs_name);
	}
#endif
}

/** @ingroup type_group_afe_InFn
 * @par Description
 *     remove debugfs entry.
 * @param[in]
 *     afe: AFE data
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt3611_afe_cleanup_debugfs(struct mtk_afe *afe)
{
#ifdef CONFIG_DEBUG_FS
	int i;

	if (!afe)
		return;

	for (i = 0; i < MT3611_AFE_DEBUGFS_NUM; i++)
		debugfs_remove(afe->debugfs_dentry[i]);
#endif
}
