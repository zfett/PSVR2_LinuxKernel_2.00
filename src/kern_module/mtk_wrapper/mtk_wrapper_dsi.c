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
#include <linux/delay.h>
#include <linux/slab.h>

#include <soc/mediatek/mtk_dsi.h>
#include <soc/mediatek/mtk_ddds.h>
#include <soc/mediatek/mtk_frmtrk.h>

#include <sce_km_defs.h>
#include "mtk_wrapper_dsi.h"
#include "mtk_wrapper_util.h"

#include <drm/drm_mipi_dsi.h>
#include <video/videomode.h>
#include <video/mipi_display.h>
#include <dt-bindings/gce/mt3612-gce.h>

static struct device *s_ddds;
static struct device *s_dsi;
static struct device *s_frmtrk;
static struct cmdq_client *s_cmdq_client;

enum mtk_dsi_pixel_format mtk_wrapper_dsi_convert_format(MTK_WRAPPER_DSI_FORMAT format)
{
	enum mtk_dsi_pixel_format ret = MTK_DSI_FMT_RGB888;

	switch (format) {
	case MTK_WRAPPER_DSI_FORMAT_RGB888:
		ret = MTK_DSI_FMT_RGB888;
		break;
	case MTK_WRAPPER_DSI_FORMAT_COMPRESSION_24_8:
		ret = MTK_DSI_FMT_COMPRESSION_24_8;
		break;
	case MTK_WRAPPER_DSI_FORMAT_COMPRESSION_30_15:
		ret = MTK_DSI_FMT_COMPRESSION_30_15;
		break;
	case MTK_WRAPPER_DSI_FORMAT_COMPRESSION_30_10:
		ret = MTK_DSI_FMT_COMPRESSION_30_10;
		break;
	case MTK_WRAPPER_DSI_FORMAT_COMPRESSION_24_12:
		ret = MTK_DSI_FMT_COMPRESSION_24_12;
		break;
	default:
		ret = MTK_DSI_FMT_NR;
		break;
	}
	return ret;
}

static int mtk_wrapper_dsi_lane_swap(uint8_t mipi_swap_index)
{
	int ret;

	ret = mtk_dsi_lane_swap(s_dsi, mipi_swap_index);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int mtk_wrapper_dsi_init(void)
{
	int ret;

	ret = mtk_dsi_hw_init(s_dsi);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int mtk_wrapper_dsi_set_config_ddds(void *user)
{
	int ret;
	args_dsi_set_config_ddds args;

	ret = copy_from_user(&args, user, sizeof(args_dsi_set_config_ddds));
	if (ret != 0) {
		return -EFAULT;
	}
	if (!args.xtal) {
		ret = mtk_ddds_enable(s_ddds, true);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_ddds_set_target_freq(s_ddds, args.target_freq);
		if (ret < 0) {
			return ret;
		}
		ret = mtk_ddds_set_err_limit(s_ddds, args.max_freq);
		if (ret < 0) {
			return ret;
		}
	}
	ret = mtk_ddds_refclk_out(s_ddds, args.xtal, true);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int mtk_wrapper_dsi_enable_ddds(void *user)
{
	int ret;
	args_dsi_enable_ddds args;

	ret = copy_from_user(&args, user, sizeof(args_dsi_enable_ddds));
	if (ret != 0) {
		return -EFAULT;
	}

	mtk_ddds_set_hlen(s_ddds, DDDS_FRAMETRACK_LOCK, args.hsync_len);
	mtk_ddds_set_hlen(s_ddds, DDDS_FRAMETRACK_FAST1, (args.hsync_len + args.step1));
	mtk_ddds_set_hlen(s_ddds, DDDS_FRAMETRACK_FAST2, (args.hsync_len + args.step2));
	mtk_ddds_set_hlen(s_ddds, DDDS_FRAMETRACK_SLOW1, (args.hsync_len - args.step1));
	mtk_ddds_set_hlen(s_ddds, DDDS_FRAMETRACK_SLOW2, (args.hsync_len - args.step2));
	mtk_ddds_close_loop(s_ddds, true);
	mtk_ddds_frmtrk_mode(s_ddds, true);
	return 0;
}

static int mtk_wrapper_dsi_wait_ddds_lock(void *user)
{
	int ret;
	args_dsi_polling args;
	bool lock_result;
	int i = 0;

	ret = copy_from_user(&args, user, sizeof(args_dsi_polling));
	if (ret != 0) {
		return -EFAULT;
	}

	while (1) {
		lock_result = mtk_ddds_locked(s_ddds);
		pr_debug("ddds Enable Try\n");
		if (!lock_result) {
			if (i < args.count) {
				i++;
				usleep_range(args.interval * 1000, (args.interval * 1000) + 100);
				continue;
			} else {
				pr_debug("%s ddds can't lock within %d ms\n",
					 __func__, args.count * args.interval);
				mtk_ddds_dump_registers(s_ddds);
				return -ETIMEDOUT; /* -1; */
			}
		} else {
			pr_debug("%s ddds lock within count = %d\n",
				 __func__, i);
			mtk_ddds_dump_registers(s_ddds);
			return i;
		}
	}
}

static int mtk_wrapper_dsi_config_frmtrk(void *user)
{
	int ret;
	args_dsi_config_frmtrk args;

	ret = copy_from_user(&args, user, sizeof(args_dsi_config_frmtrk));
	if (ret != 0) {
		return -EFAULT;
	}

	mtk_frmtrk_config_vtotal(s_frmtrk, args.lcm_vttl, NULL);
	mtk_frmtrk_target_line(s_frmtrk, args.target_line, NULL);
	mtk_frmtrk_lock_window(s_frmtrk, args.lock_win, NULL);
	mtk_frmtrk_turbo_window(s_frmtrk, args.turbo_win, NULL);
	return 0;
}

static int mtk_wrapper_dsi_disable_ddds(void)
{
	mtk_ddds_close_loop(s_ddds, false);
	mtk_ddds_frmtrk_mode(s_ddds, false);
	mtk_ddds_refclk_out(s_ddds, true, true);
	mtk_ddds_refclk_out(s_ddds, false, false);
	mtk_ddds_enable(s_ddds, false);
	return 0;
}

static int mtk_wrapper_dsi_disable_frmtrk(void)
{
	mtk_frmtrk_stop(s_frmtrk);
	mtk_frmtrk_set_mask(s_frmtrk, 1, 0, NULL);
	return 0;
}

static int mtk_wrapper_dsi_set_datarate(uint32_t rate)
{
	return mtk_dsi_set_datarate(s_dsi, rate);
}

static int mtk_wrapper_dsi_get_fps(void)
{
	u32 fps;
	int ret = mtk_dsi_get_fps(s_dsi, &fps);

	if (ret < 0)
		return ret;
	return (int)fps;
}

static int mtk_wrapper_dsi_set_sw_mute(void *user)
{
	int ret;

	args_dsi_set_sw_mute args;

	ret = copy_from_user(&args, user, sizeof(args_dsi_set_sw_mute));

	if (ret != 0) {
		return -EFAULT;
	}

	mtk_dsi_sw_mute_config(s_dsi, args.mute, NULL);
	return ret;
}

static int mtk_wrapper_dsi_wait_frmtrk_lock(void *user)
{
	bool lock_result;
	args_dsi_polling args;
	int i = 0;
	int ret;
	u32 dist;

	ret = copy_from_user(&args, user, sizeof(args_dsi_polling));
	if (ret != 0) {
		return -EFAULT;
	}

	while (1) {
		lock_result = mtk_frmtrk_locked(s_frmtrk);
		if (!lock_result) {
			if (i < args.count) {
				if (i % 10 == 0) {
					mtk_frmtrk_get_trk_dist(s_frmtrk, &dist);
					pr_info("frame tracker is tuning %d [%d]!\n",
						i, dist);
				}
				i++;
				usleep_range(args.interval * 1000, (args.interval * 1000) + 100);
				continue;
			} else {
				mtk_frmtrk_get_trk_dist(s_frmtrk, &dist);
				pr_info("%s frame tracker can't lock i = %d [%d]\n",
					__func__, i, dist);
				return -ETIMEDOUT;
			}
		} else {
			mtk_frmtrk_get_trk_dist(s_frmtrk, &dist);
			pr_info("%s frame tracker lock within count = %d [%d]\n",
				__func__, i, dist);
			return 0;
		}
	}
}

static int mtk_wrapper_dsi_start_output(void *user)
{
	struct cmdq_pkt *pkt;
	u32 event;
	args_dsi_start_param args;
	int ret;

	ret = copy_from_user(&args, user, sizeof(args_dsi_start_param));
	if (ret != 0) {
		return -EFAULT;
	}

	if (args.wait == MTK_WRAPPER_DSI_NO_WAIT) {
		return mtk_dsi_output_start(s_dsi, NULL);
	}

	if (args.wait == MTK_WRAPPER_DSI_WAIT_SLICER_0) {
		event = CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_0;
	} else if (args.wait == MTK_WRAPPER_DSI_WAIT_SLICER_1) {
		event = CMDQ_EVENT_MMSYS_CORE_SLICER_EVENT_1;
	} else {
		return -EINVAL;
	}

	if (args.mask < 1) {
		return -EINVAL;
	}

	cmdq_pkt_create(&pkt);
	cmdq_pkt_clear_event(pkt, event);
	cmdq_pkt_wfe(pkt, event);
	mtk_dsi_output_start(s_dsi, pkt);
	mtk_frmtrk_set_mask(s_frmtrk, args.mask, args.mask-1, pkt);
	mtk_frmtrk_start(s_frmtrk, pkt);
	cmdq_pkt_flush(s_cmdq_client, pkt);
	cmdq_pkt_destroy(pkt);

	return 0;
}

static int mtk_wrapper_dsi_stop_output(void)
{
	int ret;

	ret = mtk_dsi_output_stop(s_dsi);
	if (ret != 0) {
		pr_err("dsi_output_stop failed(%d)\n", ret);
		return ret;
	}
	ret = mtk_dsi_send_dcs_cmd(s_dsi, MIPI_DCS_SET_DISPLAY_OFF, 0, NULL);
	if (ret != 0) {
		pr_err("dsi_send_dcs_cmd failed(%d)\n", ret);
	}
	ret = mtk_dsi_send_dcs_cmd(s_dsi, MIPI_DCS_ENTER_SLEEP_MODE, 0, NULL);
	if (ret != 0) {
		pr_err("dsi_send_dcs_cmd failed(%d)\n", ret);
	}
	return ret;

}

static int mtk_wrapper_dsi_reset(void)
{
	return mtk_dsi_lcm_reset(s_dsi);
}

static int mtk_wrapper_dsi_set_panel_param(void *user)
{
	int ret;
	args_dsi_set_panel_param args;
	unsigned long mode_flags;
	struct videomode vm;
	enum mtk_dsi_pixel_format format;

	ret = copy_from_user(&args, user, sizeof(args_dsi_set_panel_param));
	if (ret != 0) {
		return -EFAULT;
	}

	mode_flags = MIPI_DSI_MODE_VIDEO;

	vm.hactive = args.hactive;
	vm.vactive = args.vactive;
	vm.vsync_len = args.vsync_len;
	vm.vback_porch = args.vback_porch;
	vm.vfront_porch = args.vfront_porch;
	vm.hsync_len = args.hsync_len;
	vm.hback_porch = args.hback_porch;
	vm.hfront_porch = args.hfront_porch;

	format = mtk_wrapper_dsi_convert_format(args.format);
	if (format == MTK_DSI_FMT_NR) {
		return -EINVAL;
	}

	ret = mtk_dsi_set_panel_params(s_dsi, mode_flags, format, &vm);
	return ret;
}

static int mtk_wrapper_dsi_set_lcm_param(void *user)
{
	int ret, i;
	args_dsi_set_lcm_param args;
	struct lcm_setting_table *table;

	if (sizeof(struct mtk_wrapper_lcm_setting_table) != sizeof(struct lcm_setting_table)) {
		return -EINVAL;
	}

	ret = copy_from_user(&args, user, sizeof(args_dsi_set_lcm_param));
	if (ret != 0) {
		return -EFAULT;
	}

	if (args.table_num <= 0) {
		return -EINVAL;
	}

	table = kmalloc_array(args.table_num, sizeof(struct lcm_setting_table), GFP_KERNEL);
	if (!table) {
		return -ENOMEM;
	}

	ret = copy_from_user(table, args.table, args.table_num * sizeof(struct lcm_setting_table));
	if (ret != 0) {
		kfree(table);
		return -EFAULT;
	}
	for (i = 0; i < args.table_num; i++) {
		if (table[i].cmd != REGFLAG_DELAY &&
		    table[i].cmd != REGFLAG_UDELAY &&
		    table[i].cmd != REGFLAG_END_OF_TABLE &&
		    table[i].count > sizeof(table[0].para_list)) {
			kfree(table);
			return -EINVAL;
		}
		if (table[i].cmd == REGFLAG_END_OF_TABLE) {
			break;
		}
	}
	if (args.is_vm_mode) {
		ret = mtk_dsi_lcm_vm_setting(s_dsi, table, args.table_num);
	} else {
		ret = mtk_dsi_lcm_setting(s_dsi, table, args.table_num);
	}
	kfree(table);
	return ret;
}

static int mtk_wrapper_dsi_send_vm_command(void *user)
{
	int ret;
	args_dsi_send_vm_command args;

	ret = copy_from_user(&args, user, sizeof(args_dsi_send_vm_command));
	if (ret != 0) {
		return -EFAULT;
	}

	if (args.count == 0) {
		ret = mtk_dsi_send_vm_cmd(s_dsi, MIPI_DSI_DCS_SHORT_WRITE,
					  args.cmd, 0, NULL);
	} else if (args.count == 1) {
		ret = mtk_dsi_send_vm_cmd(s_dsi, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
					  args.cmd, args.para_list[0], NULL);
	} else {
		if (args.cmd != 0) {
			pr_warn("DSI_DCS_LONG_WRITE cmd ignore, please input in para_list[0].");
		}
		ret = mtk_dsi_send_vm_cmd(s_dsi, MIPI_DSI_DCS_LONG_WRITE,
					  args.count, 0,  args.para_list);
	}
	return ret;
}

#define PANEL_ID_TOP_HALF (5)
#define PANEL_ID_BOTTOM_HALF (6)

static int mtk_wrapper_dsi_read_panel_id(void *user)
{
	int ret;

	struct dsi_panel_id panel_id;
	struct dsi_read_data read_data;

	ret = mtk_dsi_read_dcs_cmd(s_dsi, MIPI_DCS_READ_DDB_START, PANEL_ID_TOP_HALF, &read_data);
	if (ret != 0) {
		return ret;
	}
	/* dcs_cmd use 0,3 line */
	memcpy(panel_id.l, read_data.byte[0], PANEL_ID_TOP_HALF);
	memcpy(panel_id.r, read_data.byte[3], PANEL_ID_TOP_HALF);

	ret = mtk_dsi_read_dcs_cmd(s_dsi, MIPI_DCS_READ_DDB_CONTINUE, PANEL_ID_BOTTOM_HALF, &read_data);
	if (ret != 0) {
		return ret;
	}
	memcpy(&panel_id.l[PANEL_ID_TOP_HALF], read_data.byte[0], PANEL_ID_BOTTOM_HALF);
	memcpy(&panel_id.r[PANEL_ID_TOP_HALF], read_data.byte[3], PANEL_ID_BOTTOM_HALF);

	ret = copy_to_user(user, &panel_id, sizeof(panel_id));
	if (ret != 0) {
		return -EFAULT;
	}

	return ret;
}

static int mtk_wrapper_dsi_read_panel_err(void *user)
{
	int ret;
	struct dsi_read_data read_data;
	struct dsi_panel_err err;

	ret = mtk_dsi_read_dcs_cmd(s_dsi, 0xEA, 2, &read_data);
	if (ret != 0) {
		return ret;
	}

	memcpy(&err.l, read_data.byte[0], 2);
	memcpy(&err.r, read_data.byte[3], 2);

	ret = copy_to_user(user, &err, sizeof(err));
	if (ret != 0) {
		return -EFAULT;
	}

	return 0;
}

static int mtk_wrapper_dsi_update_hbrank_reg(void *user)
{
	int ret;
	args_dsi_update_hblank_reg args;

	ret = copy_from_user(&args, user, sizeof(args_dsi_update_hblank_reg));
	if (ret != 0) {
		return -EFAULT;
	}

	ret = mtk_dsi_set_lcm_hblank(s_dsi, args.hsync_len,
				     args.hback_porch,
				     args.hfront_porch);
	return ret;
}

static int mtk_wrapper_dsi_init_state_check(void *user)
{
	int ret;
	int i = 0;
	u32 target;

	args_dsi_init_state_check args;

	ret = copy_from_user(&args, user, sizeof(args_dsi_init_state_check));
	if (ret != 0) {
		return -EFAULT;
	}

	while (1) {
		mtk_frmtrk_get_trk_dist(s_frmtrk, &target);
		if (i < 50) {
			if (target == args.target_line) {
				return 0;
			}
			usleep_range(20*1000, 21*1000);
			i++;
		}
	}
	return -ETIMEDOUT;
}


static int mtk_wrapper_dsi_finit(void)
{
	return mtk_dsi_hw_fini(s_dsi);
}

static long mtk_wrapper_dsi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case DSI_INIT:
		ret = mtk_wrapper_dsi_init();
		break;
	case DSI_LANE_SWAP:
		ret = mtk_wrapper_dsi_lane_swap((uint8_t)arg);
		break;
	case DSI_SET_CONFIG_DDDS:
		ret = mtk_wrapper_dsi_set_config_ddds((void *)arg);
		break;
	case DSI_ENABLE_DDDS:
		ret = mtk_wrapper_dsi_enable_ddds((void *)arg);
		break;
	case DSI_DISABLE_DDDS:
		ret = mtk_wrapper_dsi_disable_ddds();
		break;
	case DSI_CONFIG_FRMTRK:
		ret = mtk_wrapper_dsi_config_frmtrk((void *)arg);
		break;
	case DSI_DISABLE_FRMTRK:
		ret = mtk_wrapper_dsi_disable_frmtrk();
		break;
	case DSI_SET_SW_MUTE:
		ret = mtk_wrapper_dsi_set_sw_mute((void *)arg);
		break;
	case DSI_START_OUTPUT:
		ret = mtk_wrapper_dsi_start_output((void *)arg);
		break;
	case DSI_STOP_OUTPUT:
		ret = mtk_wrapper_dsi_stop_output();
		break;
	case DSI_RESET:
		ret = mtk_wrapper_dsi_reset();
		break;
	case DSI_GET_FPS:
		ret = mtk_wrapper_dsi_get_fps();
		break;
	case DSI_SET_PANEL_PARAM:
		ret = mtk_wrapper_dsi_set_panel_param((void *)arg);
		break;
	case DSI_SET_LCM_PARAM:
		ret = mtk_wrapper_dsi_set_lcm_param((void *)arg);
		break;
	case DSI_SET_DATARATE:
		ret = mtk_wrapper_dsi_set_datarate((uint32_t)arg);
		break;
	case DSI_SEND_VM_COMMAND:
		ret = mtk_wrapper_dsi_send_vm_command((void *)arg);
		break;
	case DSI_READ_PANEL_ID:
		ret = mtk_wrapper_dsi_read_panel_id((void *)arg);
		break;
	case DSI_READ_PANEL_ERR:
		ret = mtk_wrapper_dsi_read_panel_err((void *)arg);
		break;
	case DSI_UPDATE_HBLANKING:
		ret = mtk_wrapper_dsi_update_hbrank_reg((void *)arg);
		break;
	case DSI_WAIT_DDDS_LOCK:
		ret = mtk_wrapper_dsi_wait_ddds_lock((void *)arg);
		break;
	case DSI_WAIT_FRMTRK_LOCK:
		ret = mtk_wrapper_dsi_wait_frmtrk_lock((void *)arg);
		break;
	case DSI_INIT_STATE_CHECK:
		ret = mtk_wrapper_dsi_init_state_check((void *)arg);
		break;
	case DSI_FINIT:
		ret = mtk_wrapper_dsi_finit();
		break;
	default:
		pr_err("dsi_ioctl: unknown command(%d)\n", cmd);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const struct file_operations mtk_wrapper_dsi_fops = {
	.unlocked_ioctl = mtk_wrapper_dsi_ioctl
};

int init_module_mtk_wrapper_dsi(void)
{
	int ret;
	struct platform_device *pdev;
	struct device *disp_pipe;

	pr_info("%s start\n", __func__);

	ret = register_chrdev(SCE_KM_CDEV_MAJOR_MTK_WRAPPER_DSI, SCE_KM_DSI_DEVICE_NAME, &mtk_wrapper_dsi_fops);
	if (ret < 0) {
		pr_err("log: alloc_chrdev(%s) failed(%d).\n", SCE_KM_DSI_DEVICE_NAME, ret);
		return ret;
	}
	ret = mtk_wrapper_display_pipe_probe("mediatek,dsi", 0, &s_dsi);
	if (ret < 0) {
		pr_err("can't find display_pipe dsi\n");
		return ret;
	}
	ret = mtk_wrapper_display_pipe_probe("mediatek,ddds", 0, &s_ddds);
	if (ret < 0) {
		pr_err("can't find display_pipe ddds\n");
		return ret;
	}
	ret = mtk_wrapper_display_pipe_probe("mediatek,frmtrk", 0, &s_frmtrk);
	if (ret < 0) {
		pr_err("can't find display_pipe frmtrk\n");
		return ret;
	}
	pdev = pdev_find_dt("sie,disp-pipe");
	if (pdev) {
		disp_pipe = &pdev->dev;
	}
	s_cmdq_client = cmdq_mbox_create(disp_pipe, 5);
	if (!s_cmdq_client ||
	    s_cmdq_client == (struct cmdq_client *)-ENOMEM ||
	    s_cmdq_client == (struct cmdq_client *)-EINVAL) {
		return -ENOMEM;
	}

	return 0;
}

void cleanup_module_mtk_wrapper_dsi(void)
{
}
