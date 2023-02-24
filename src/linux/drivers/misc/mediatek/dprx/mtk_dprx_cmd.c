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
* @file mtk_dprx_os.c
*
*  OS(Linux) related function
 *
 */

/* include file  */
#include "mtk_dprx_os_int.h"
#include <soc/mediatek/mtk_dprx_info.h>
#include "mtk_dprx_drv_int.h"
#include "mtk_dprx_avc_int.h"
#include "mtk_dprx_isr_int.h"
#include "mtk_dprx_reg_int.h"
#include <soc/mediatek/mtk_dprx_if.h>
#include "mtk_dprx_cmd_int.h"

/* ====================== Global Structure ========================= */
#define DBUF_LEN	1024
static char debug_buffer[DBUF_LEN];
#ifdef CONFIG_DEBUG_FS
struct dentry *dprx_debugfs;
#endif

/* ====================== Function ================================= */

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command init function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_init(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx init\n");
	ret = mtk_dprx_drv_init(dev, true);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command init function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_deinit(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx deinit\n");
	mtk_dprx_drv_deinit(dev);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command phy gce init function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_phy_gce_init(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx phy gce init\n");
	ret = mtk_dprx_phy_gce_init(dev);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command phy gce init function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_phy_gce_deinit(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx phy gce deinit\n");
	ret = mtk_dprx_phy_gce_deinit(dev);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command start function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_start(struct mtk_dprx *dprx)
{
	int ret = 0;

	pr_debug("[DPRX]dprx start\n");
	ret = mtk_dprx_drv_start();

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command stop function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_stop(struct mtk_dprx *dprx)
{
	int ret = 0;

	pr_debug("[DPRX]dprx stop\n");
	ret = mtk_dprx_drv_stop();

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command power on function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_power_on(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx power on\n");
	ret = mtk_dprx_power_on(dev);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command power off function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_power_off(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx power off\n");
	ret = mtk_dprx_power_off(dev);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command phy power on function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_power_on_phy(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx power on phy\n");
	ret = mtk_dprx_power_on_phy(dev);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command phy power off function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_power_off_phy(struct mtk_dprx *dprx)
{
	struct device *dev = dprx->dev;
	int ret = 0;

	pr_debug("[DPRX]dprx power off phy\n");
	mtk_dprx_power_off_phy(dev);

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command get crc function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function pass.\n
 *     1, function fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_get_crc(struct mtk_dprx *dprx)
{
	int ret = 0;

	pr_debug("[DPRX]dprx get crc\n");
	ret = dprx_get_video_crc();

	return ret;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs command slt function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, function fail.\n
 *     1, function pass.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static unsigned int dprxcmd_slt(struct mtk_dprx *dprx)
{
	struct dprx_video_crc_s crc_bk0, crc_bk1;
	int i;
	int ret;
	int timeout = 1000;

	crc_bk0.lb_crc_r_cr = 0;
	crc_bk0.hb_crc_r_cr = 0;
	crc_bk0.lb_crc_g_y = 0;
	crc_bk0.hb_crc_g_y = 0;
	crc_bk0.lb_crc_b_cb = 0;
	crc_bk0.hb_crc_b_cb = 0;
	crc_bk1.lb_crc_r_cr = 1;
	crc_bk1.hb_crc_r_cr = 1;
	crc_bk1.lb_crc_g_y = 1;
	crc_bk1.hb_crc_g_y = 1;
	crc_bk1.lb_crc_b_cb = 1;
	crc_bk1.hb_crc_b_cb = 1;

	do {
		mdelay(1);
		timeout--;
	} while ((!dprx_get_video_stable_status()) && (timeout > 0));

	if (timeout == 0) {
		pr_err("[DPRX] get video stable timeout!\n");
		return 0;
	}

	for (i = 0; i < 3; i++) {
		ret = dprx_get_video_crc();
		if (ret == 0)
			memcpy(&crc_bk0, &dp_vid_crc, sizeof(dp_vid_crc));
		pr_debug("[DPRX]Video CRC0 R Cr Out = 0x%x : 0x%x\n",
			 crc_bk0.lb_crc_r_cr, crc_bk0.hb_crc_r_cr);
		pr_debug("[DPRX]Video CRC0 G Y  Out  = 0x%x : 0x%x\n",
			 crc_bk0.lb_crc_g_y, crc_bk0.hb_crc_g_y);
		pr_debug("[DPRX]Video CRC0 B Cb Out = 0x%x: 0x%x\n",
			 crc_bk0.lb_crc_b_cb, crc_bk0.hb_crc_b_cb);

		ret = dprx_get_video_crc();
		if (ret == 0)
			memcpy(&crc_bk1, &dp_vid_crc, sizeof(dp_vid_crc));
		pr_debug("[DPRX]Video CRC1 R Cr Out = 0x%x : 0x%x\n",
			 crc_bk1.lb_crc_r_cr, crc_bk1.hb_crc_r_cr);
		pr_debug("[DPRX]Video CRC1 G Y  Out  = 0x%x : 0x%x\n",
			 crc_bk1.lb_crc_g_y, crc_bk1.hb_crc_g_y);
		pr_debug("[DPRX]Video CRC1 B Cb Out = 0x%x: 0x%x\n",
			 crc_bk1.lb_crc_b_cb, crc_bk1.hb_crc_b_cb);

		if ((crc_bk0.lb_crc_r_cr == crc_bk1.lb_crc_r_cr) &&
		    (crc_bk0.hb_crc_r_cr == crc_bk1.hb_crc_r_cr) &&
		    (crc_bk0.lb_crc_g_y == crc_bk1.lb_crc_g_y) &&
		    (crc_bk0.hb_crc_g_y == crc_bk1.hb_crc_g_y) &&
		    (crc_bk0.lb_crc_b_cb == crc_bk1.lb_crc_b_cb) &&
		    (crc_bk0.hb_crc_b_cb == crc_bk1.hb_crc_b_cb))
			return 1;
	}
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX process debug command function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @param[in]
 *     opt: command string.
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void process_dbg_cmd(struct mtk_dprx *dprx, char *opt)
{
	if (strncmp(opt, "init", 4) == 0) {
		dprxcmd_init(dprx);
	} else if (strncmp(opt, "deinit", 6) == 0) {
		dprxcmd_deinit(dprx);
	} else if (strncmp(opt, "phygceinit", 10) == 0) {
		dprxcmd_phy_gce_init(dprx);
	} else if (strncmp(opt, "phygcedeinit", 12) == 0) {
		dprxcmd_phy_gce_deinit(dprx);
	} else if (strncmp(opt, "start", 5) == 0) {
		dprxcmd_start(dprx);
	} else if (strncmp(opt, "stop", 4) == 0) {
		dprxcmd_stop(dprx);
	} else if (strncmp(opt, "poweron", 7) == 0) {
		dprxcmd_power_on(dprx);
	} else if (strncmp(opt, "poweroff", 8) == 0) {
		dprxcmd_power_off(dprx);
	} else if (strncmp(opt, "pwronphy", 8) == 0) {
		dprxcmd_power_on_phy(dprx);
	} else if (strncmp(opt, "pwroffphy", 9) == 0) {
		dprxcmd_power_off_phy(dprx);
	} else if (strncmp(opt, "crc", 3) == 0) {
		dprxcmd_get_crc(dprx);
	} else if (strncmp(opt, "slt", 3) == 0) {
		if (dprxcmd_slt(dprx) == 1)
			sprintf(debug_buffer, "%d", 0);
		else
			sprintf(debug_buffer, "%d", 1);
	} else {
		pr_info("[DPRX]parse command error\n");
	}
}

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t debug_read(struct file *file, char __user *ubuf,
			  size_t count, loff_t *ppos)
{
	size_t n;

	n = strnlen(debug_buffer, DBUF_LEN - 1);
	debug_buffer[n++] = 0;

	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t debug_write(struct file *file, const char __user *ubuf,
			   size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct mtk_dprx *dprx;

	dprx = file->private_data;
	ret = count;

	if (count > DBUF_LEN - 1)
		count = DBUF_LEN - 1;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;

	process_dbg_cmd(dprx, debug_buffer);

	return ret;
}

static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs init function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     0, debugfs init is done.\n
 *     non zero, debugfs init is fail.
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int dprxcmd_debug_init(struct mtk_dprx *dprx)
{
	pr_debug("[DPRX]%s\n", __func__);
#ifdef CONFIG_DEBUG_FS
	dprx_debugfs = debugfs_create_file(
		"dprx", S_IFREG | S_IRUGO, NULL, (void *)dprx, &debug_fops);

	if (IS_ERR(dprx_debugfs))
		return PTR_ERR(dprx_debugfs);
#endif
	return 0;
}

/** @ingroup IP_group_dprx_internal_function
 * @par Description
 *     DPRX debugfs uninit function
 * @param[in]
 *     dprx: struct mtk_dprx *dprx
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void dprxcmd_debug_uninit(struct mtk_dprx *dprx)
{
#ifdef CONFIG_DEBUG_FS
	debugfs_remove(dprx_debugfs);
#endif
}
