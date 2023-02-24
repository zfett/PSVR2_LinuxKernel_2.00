/*
 * Copyright (c) 2015 MediaTek Inc.
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
 * @file mtk_dsi_debugfs.c
 * MTK DSI Debug Linux Driver.\n
 * This driver provide some test functions for debug use.\n
 * User can use these function by debugfs - sys/kernel/debug/mtk_dsi.\n
 * ex. echo init > sys/kernel/debug/mtk_dsi --> initial dsi subsystem.
 */

#include <linux/debugfs.h>
#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <linux/uaccess.h>
#include <video/videomode.h>
#include <video/mipi_display.h>
#include <linux/of_platform.h>

#include "mtk_dsi_int.h"
#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_ddds.h>

/** @ingroup IP_group_dsi_internal_def_debug
 * @{
 */
#define HELP_INFO_DSI \
	"\n" \
	"USAGE\n" \
	"	echo [ACTION]... > /sys/kernel/debug/mtk_dsi\n" \
	"\n" \
	"ACTION\n" \
	"\n" \
	"	regr:addr\n" \
	"	regw:addr=value\n" \
	"	dump\n" \
	"	init\n" \
	"	fini\n" \
	"	start\n" \
	"	stop\n" \
	"	lane:value\n" \
	"	self-on\n" \
	"	self-off\n" \
	"	fps\n" \
	"	disp-on\n" \
	"	disp-off\n" \
	"	pixel:value\n" \

#define MTK_DSI_BASE		0x14022000

#define COLOR_RED		0x3ff00000
#define COLOR_GREEN		0x000ffc00
#define COLOR_BLUE		0x000003ff
/**
 * @}
 */

/** @ingroup IP_group_dsi_internal_function_debug
 * @par Description
 *     This is dsi debugfs file open function.
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
static int mtk_dsi_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

/** @ingroup IP_group_dsi_internal_function_debug
 * @par Description
 *     This is dsi debugfs file read function.
 * @param[in] file: file pointer
 * @param[out] ubuf: user space buffer pointer
 * @param[in] count: number of data to read
 * @param[in,out] ppos: offset
 * @return
 *     Negative, failed. \n
 *     Others, number of data readed.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static ssize_t mtk_dsi_debug_read(struct file *file, char __user *ubuf,
				  size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(ubuf, count, ppos, HELP_INFO_DSI,
				       strlen(HELP_INFO_DSI));
}

/** @ingroup IP_group_dsi_internal_function_debug
 * @par Description
 *     This is dsi test handler function.
 * @param[in] cmd: test command
 * @param[in] dsi: dsi driver data struct
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
static int mtk_dsi_process_cmd(char *cmd, struct mtk_dsi *dsi)
{
	char *np, *opt;
	unsigned long addr;
	int data;

	dev_info(dsi->dev, "dbg cmd: %s\n", cmd);
	opt = strsep(&cmd, " ");

	if (strncmp(opt, "regr:", 5) == 0) {
		char *p = (char *)opt + 5;

		if (kstrtoul(p, 16, &addr))
			goto errcode;

		if ((addr & MTK_DSI_BASE) == MTK_DSI_BASE)
			DRM_INFO("dsi read register 0x%08lX: 0x%08X\n", addr,
				 readl(dsi->regs + addr - MTK_DSI_BASE));
		else
			goto errcode;
	} else if (strncmp(opt, "regw:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned long val;

		np = strsep(&p, "=");
		if (kstrtoul(np, 16, &addr))
			goto errcode;

		if (!p)
			goto errcode;

		np = strsep(&p, "=");
		if (kstrtoul(np, 16, &val))
			goto errcode;

		if ((addr & MTK_DSI_BASE) == MTK_DSI_BASE)
			writel(val, dsi->regs + addr - MTK_DSI_BASE);
		else
			goto errcode;
	} else if (strncmp(opt, "dump", 4) == 0) {
		mtk_dsi_dump_registers(dsi);
	} else if (strncmp(opt, "init", 4) == 0) {
		mtk_dsi_hw_init(dsi->dev);
		mtk_dsi_lcm_setting(dsi->dev, NULL, 0);
	} else if (strncmp(opt, "fini", 4) == 0) {
		mtk_dsi_hw_fini(dsi->dev);
	} else if (strncmp(opt, "start", 5) == 0) {
		mtk_dsi_output_start(dsi->dev, NULL);
	} else if (strncmp(opt, "stop", 4) == 0) {
		mtk_dsi_output_stop(dsi->dev);
	} else if (strncmp(opt, "lane:", 5) == 0) {
		char *p = (char *)opt + 5;

		if (kstrtoint(p, 16, &data))
			goto errcode;

		DRM_INFO("dsi lane: %d\n", data);

		mtk_dsi_lane_config(dsi->dev, data);
	} else if (strncmp(opt, "self-on", 7) == 0) {
		mtk_dsi_output_self_pattern(dsi->dev, true, COLOR_RED);
	} else if (strncmp(opt, "self-off", 8) == 0) {
		mtk_dsi_output_self_pattern(dsi->dev, false, COLOR_RED);
	} else if (strncmp(opt, "fps:", 4) == 0) {
		char *p = (char *)opt + 4;

		if (kstrtoint(p, 16, &data))
			goto errcode;

		if (data == 0) {
			DRM_INFO("dsi fps: 30\n");
			mtk_dsi_set_fps(dsi->dev, MTK_DSI_FPS_30);
		} else if (data == 1) {
			DRM_INFO("dsi fps: 60\n");
			mtk_dsi_set_fps(dsi->dev, MTK_DSI_FPS_60);
		} else if (data == 2) {
			DRM_INFO("dsi fps: 90\n");
			mtk_dsi_set_fps(dsi->dev, MTK_DSI_FPS_90);
		} else {
			DRM_INFO("dsi fps: 120\n");
			mtk_dsi_set_fps(dsi->dev, MTK_DSI_FPS_120);
		}
		DRM_INFO("dsi data rate: %d\n", dsi->data_rate);
	} else if (strncmp(opt, "fps", 3) == 0) {
		u32 fps;

		mtk_dsi_get_fps(dsi->dev, &fps);
		DRM_INFO("dsi fps: %d\n", fps);
	} else if (strncmp(opt, "disp-on", 7) == 0) {
		mtk_dsi_send_dcs_cmd(dsi->dev, MIPI_DCS_SET_DISPLAY_ON, 0,
				     NULL);
	} else if (strncmp(opt, "disp-off", 8) == 0) {
		mtk_dsi_send_dcs_cmd(dsi->dev, MIPI_DCS_SET_DISPLAY_OFF, 0,
				     NULL);
	} else if (strncmp(opt, "deskew", 6) == 0) {
		data = mtk_dsi_deskew(dsi->dev);
		DRM_INFO("deskew result: %d\n", data);
	} else if (strncmp(opt, "cksm:", 5) == 0) {
		char *p = (char *)opt + 5;
		u16 tmp16;

		if (kstrtoint(p, 16, &data))
			goto errcode;

		mtk_dsi_set_frame_cksm(dsi->dev, data, true);

		usleep_range(4000, 4050);

		mtk_dsi_get_frame_cksm(dsi->dev, data, &tmp16);
		mtk_dsi_set_frame_cksm(dsi->dev, data, false);

		DRM_INFO("dsi[%d], checksum:0x%04X\n", data, tmp16);
	} else if (strncmp(opt, "disp_on", 7) == 0) {
		mtk_dsi_send_vm_cmd(dsi->dev, MIPI_DSI_DCS_SHORT_WRITE,
				    MIPI_DCS_SET_DISPLAY_ON, 0, NULL);
	} else if (strncmp(opt, "disp_off", 8) == 0) {
		mtk_dsi_send_vm_cmd(dsi->dev, MIPI_DSI_DCS_SHORT_WRITE,
				    MIPI_DCS_SET_DISPLAY_OFF, 0, NULL);
	} else if (strncmp(opt, "pixel:", 6) == 0) {
		char *p = (char *)opt + 6;

		if (kstrtoint(p, 16, &data))
			goto errcode;

		/* 0:RGB10; 1:RGB8; >=5:Compression*/
		mtk_dsi_ps_config(dsi->dev, data);
	} else if (strncmp(opt, "rstlcm", 6) == 0) {
		DRM_INFO("Reset LCM!\n");
		mtk_dsi_lcm_reset(dsi->dev);
	} else if (strncmp(opt, "cksmcmp:", 8) == 0) {
		char *p = (char *)opt + 8;
		u16 cksm0, i;
		unsigned long cksm1;

		for (i = 0; i < dsi->hw_nr; i++)
			mtk_dsi_set_frame_cksm(dsi->dev, i, true);

		/* delay=1000/20(fps)=50(ms) */
		mdelay(50);
		for (i = 0; i < dsi->hw_nr; i++) {
			if (!p) {
				DRM_INFO("dsi[%d] script parsing fail!\n", i);
				goto slterr;
			}

			cksm1 = 0;
			np = strsep(&p, ":");
			if (kstrtoul(np, 16, &cksm1)) {
				DRM_INFO("dsi[%d] fail, cksm = 0x%lx!\n",
					 i, cksm1);
				goto slterr;
			}

			mtk_dsi_get_phy_cksm(dsi->dev, i, &cksm0);
			DRM_INFO("dsi[%d], cksm(phy) = 0x%x\n", i, cksm0);

			if ((u16)cksm1 != cksm0)
				goto slterr;

		}
	} else if (strncmp(opt, "selfpanel:", 10) == 0) {
		char *p = (char *)opt + 10;

		if (kstrtoint(p, 10, &data))
			goto errcode;
		if (data < MTK_DSI_FPS_30 || data > MTK_DSI_FPS_120)
			goto errcode;

		DRM_INFO("fps: %d\n", data);

		mtk_dsi_set_lcm_index(dsi->dev, MTK_LCM_RGB888);
		mtk_dsi_lane_swap(dsi->dev, 3); /* for EPN-DTB-DISP */
		mtk_dsi_set_fps(dsi->dev, data);
		mtk_dsi_lcm_reset(dsi->dev);
		mtk_dsi_hw_init(dsi->dev);
		mtk_dsi_lcm_setting(dsi->dev, NULL, 0);
		mtk_dsi_output_self_pattern(dsi->dev, true, 0x000fffff);
		mtk_dsi_output_start(dsi->dev, NULL);
		mtk_dsi_lcm_vm_setting(dsi->dev, NULL, 0);
		mtk_dsi_send_vm_cmd(dsi->dev, MIPI_DSI_DCS_SHORT_WRITE,
				    MIPI_DCS_SET_DISPLAY_ON, 0, NULL);
	} else if (strncmp(opt, "read-panel:", 11) == 0) {
		struct dsi_read_data read_data;
		int i, j;
		char *p = (char *)opt + 11;

		if (kstrtoint(p, 10, &data))
			goto errcode;

		data = data > 10 ? 10 : data;
		DRM_INFO("data: %d\n", data);

		mtk_dsi_read_dcs_cmd(dsi->dev, MIPI_DCS_READ_DDB_START, data,
				     &read_data);
		for (i = 0; i < dsi->hw_nr; i++)
			for (j = 0; j < data; j++)
				DRM_INFO("data[%d][%d]:0x%02x\n",
					 i, j, read_data.byte[i][j]);
	} else if (strncmp(opt, "frm-rate", 8) == 0) {
		int i;
		u32 counter, frm_rate = 0;

		for (i = 0; i < dsi->hw_nr; i++) {
			mtk_dsi_enable_frame_counter(dsi->dev, i, true);
			mdelay(3000);
			mtk_dsi_get_frame_counter(dsi->dev, i, &counter);

			mtk_dsi_enable_frame_counter(dsi->dev, i, false);

			/* frame rate=data rate*1000(KHz->Hz)/8/counter */
			if (counter > 0)
				frm_rate = dsi->data_rate * 1000 / 8 / counter;
			DRM_INFO(
				"dsi[%d], counter=%d, data rate=%d(KHz), frame rate=%d(fps)\n",
				i, counter, dsi->data_rate, frm_rate);
		}
	} else if (strncmp(opt, "format:", 7) == 0) {
		char *p = (char *)opt + 7;

		if (kstrtoint(p, 10, &data))
			goto errcode;

		mtk_dsi_set_format(dsi->dev, data);
	} else if (strncmp(opt, "mode:", 5) == 0) {
		char *p = (char *)opt + 5;

		if (kstrtoint(p, 10, &data))
			goto errcode;

		mtk_dsi_set_display_mode(dsi->dev, data);
	} else if (strncmp(opt, "clk:", 4) == 0) {
		struct device_node *node;
		struct platform_device *ctrl_pdev;
		char *p = (char *)opt + 4;

		if (kstrtoint(p, 10, &data))
			goto errcode;

		node = of_find_compatible_node(NULL, NULL,
					       "mediatek,mt3612-ddds");
		if (!node) {
			dev_err(dsi->dev, "Failed to get ddds node\n");
			of_node_put(node);
			goto errcode;
		}
		ctrl_pdev = of_find_device_by_node(node);
		if (!ctrl_pdev) {
			dev_err(dsi->dev, "Device does not enable %s\n",
				node->full_name);
			of_node_put(node);
			goto errcode;
		}
		if (!dev_get_drvdata(&ctrl_pdev->dev)) {
			dev_err(dsi->dev, "Waiting for ddds device %s\n",
				node->full_name);
			of_node_put(node);
			goto errcode;
		}
		of_node_put(node);

		mtk_ddds_refclk_out(&ctrl_pdev->dev, true, data);
	} else {
		goto errcode;
	}

	return 0;

errcode:
	dev_err(dsi->dev, "invalid dbg command\n");
slterr:

	return -1;
}

/** @ingroup IP_group_dsi_internal_function_debug
 * @par Description
 *     This is dsi debugfs file write function.
 * @param[in] file: file pointer
 * @param[in] ubuf: user space buffer pointer
 * @param[in] count: number of data to write
 * @param[in,out] ppos: offset
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
static ssize_t mtk_dsi_debug_write(struct file *file, const char __user *ubuf,
				   size_t count, loff_t *ppos)
{
	char str_buf[64];
	size_t ret;
	struct mtk_dsi *dsi;

	dsi = file->private_data;
	memset(str_buf, 0, sizeof(str_buf));

	if (count > (sizeof(str_buf) - 1))
		count = sizeof(str_buf) - 1;

	if (copy_from_user(str_buf, ubuf, count))
		return -EFAULT;

	str_buf[count] = 0;

	ret = (size_t)mtk_dsi_process_cmd(str_buf, dsi);

	ret = ret == 0 ? count : ret;

	return ret;
}

/**
 * @brief a file operation structure for dsi_debug driver
 */
static const struct file_operations mtk_dsi_debug_fops = {
	.read = mtk_dsi_debug_read,
	.write = mtk_dsi_debug_write,
	.open = mtk_dsi_debug_open,
};

#if defined(CONFIG_DEBUG_FS)
/** @ingroup IP_group_dsi_internal_function_debug
 * @par Description
 *     Register this test function and create file entry in debugfs.
 * @param[in] dsi: dsi driver data struct
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
int mtk_drm_dsi_debugfs_init(struct mtk_dsi *dsi)
{
	dsi->debugfs =
	    debugfs_create_file("mtk_dsi", S_IFREG | S_IRUGO |
				S_IWUSR | S_IWGRP, NULL, (void *)dsi,
				&mtk_dsi_debug_fops);

	if (IS_ERR(dsi->debugfs))
		return PTR_ERR(dsi->debugfs);

	return 0;
}

/** @ingroup IP_group_dsi_internal_function_debug
 * @par Description
 *     Unregister this test function and remove file entry from debugfs.
 * @param[in] dsi: dsi driver data struct
 * @return none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mtk_drm_dsi_debugfs_exit(struct mtk_dsi *dsi)
{
	debugfs_remove(dsi->debugfs);
}
#endif
