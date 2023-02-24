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

#ifndef _MTK_WRAPPER_DSI_
#define _MTK_WRAPPER_DSI_

#include <linux/ioctl.h>
#include <linux/types.h>

#define DSI_DEVICE_IOC_TYPE 'D'

#define REGFLAG_DELAY    0xfffc
#define REGFLAG_UDELAY   0xfffb
#define REGFLAG_END_OF_TABLE 0xfffd
#define REGFLAG_RESET_LOW 0xfffe
#define REGFLAG_RESET_HIGH 0xffff

#define MTK_WRAPPER_DSI_MIPI_SWAP_DEFAULT (0)
#define MTK_WRAPPER_DSI_MIPI_PN_SWAP_02   (2)
#define MTK_WRAPPER_DSI_MIPI_01_SWAP_03   (3)

typedef enum {
	MTK_WRAPPER_DSI_FORMAT_RGB888,
	MTK_WRAPPER_DSI_FORMAT_COMPRESSION_30_15,
	MTK_WRAPPER_DSI_FORMAT_COMPRESSION_30_10,
	MTK_WRAPPER_DSI_FORMAT_COMPRESSION_24_12,
	MTK_WRAPPER_DSI_FORMAT_COMPRESSION_24_8,
} MTK_WRAPPER_DSI_FORMAT;

typedef enum {
	MTK_WRAPPER_DSI_NO_WAIT,
	MTK_WRAPPER_DSI_WAIT_SLICER_0,
	MTK_WRAPPER_DSI_WAIT_SLICER_1,
} MTK_WRAPPER_DSI_WAIT;

typedef struct _args_dsi_set_config_ddds {
	__u32 target_freq;
	__u32 max_freq;
	bool     xtal;
} args_dsi_set_config_ddds;

typedef struct _args_dsi_enable_ddds {
	__u32 hsync_len;
	__u32 step1;
	__u32 step2;
} args_dsi_enable_ddds;

typedef struct _args_dsi_config_frmtrk {
	__u16 target_line;
	__u8  lock_win;
	__u16 turbo_win;
	__u16 lcm_vttl;
} args_dsi_config_frmtrk;

typedef struct _args_dsi_set_sw_mute {
	bool mute;
} args_dsi_set_sw_mute;

typedef struct _args_dsi_start_param {
	MTK_WRAPPER_DSI_WAIT wait;
	__u32 mask;
} args_dsi_start_param;

typedef struct _args_dsi_set_panel_param {
	__u32 hactive;
	__u32 vactive;
	__u32 vsync_len;
	__u32 vback_porch;
	__u32 vfront_porch;
	__u32 hsync_len;
	__u32 hback_porch;
	__u32 hfront_porch;
	MTK_WRAPPER_DSI_FORMAT format;
} args_dsi_set_panel_param;

typedef struct _args_dsi_update_hblank_reg {
	__u32 hsync_len;
	__u32 hback_porch;
	__u32 hfront_porch;
} args_dsi_update_hblank_reg;

struct mtk_wrapper_lcm_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[128];
};

typedef struct _args_dsi_set_lcm_param {
	const struct mtk_wrapper_lcm_setting_table *table;
	int table_num;
	int is_vm_mode;
} args_dsi_set_lcm_param;

typedef struct _args_dsi_send_vm_command {
	__u8 cmd;
	__u8 count;
	__u8  para_list[16];
} args_dsi_send_vm_command;

typedef struct _args_dsi_polling {
	__u16 interval; /* ms */
	__u16 count;
} args_dsi_polling;

typedef struct _args_dsi_init_state_check {
	__u32 target_line;
} args_dsi_init_state_check;

#define PANEL_ID_SIZE (11)

struct dsi_panel_id {
	__u8 l[PANEL_ID_SIZE];
	__u8 r[PANEL_ID_SIZE];
};

struct dsi_panel_err {
	__u16 l;
	__u16 r;
};

#define DSI_INIT              _IO(DSI_DEVICE_IOC_TYPE, 1)
#define DSI_SET_CONFIG_DDDS   _IOW(DSI_DEVICE_IOC_TYPE, 2, args_dsi_set_config_ddds)
#define DSI_ENABLE_DDDS       _IOW(DSI_DEVICE_IOC_TYPE, 3, args_dsi_enable_ddds)
#define DSI_DISABLE_DDDS      _IO(DSI_DEVICE_IOC_TYPE, 4)
#define DSI_SET_SW_MUTE       _IOW(DSI_DEVICE_IOC_TYPE, 6, bool)
#define DSI_START_OUTPUT      _IOW(DSI_DEVICE_IOC_TYPE, 7, args_dsi_start_param)
#define DSI_STOP_OUTPUT       _IO(DSI_DEVICE_IOC_TYPE,  8)
#define DSI_RESET             _IO(DSI_DEVICE_IOC_TYPE,  10)
#define DSI_GET_FPS           _IO(DSI_DEVICE_IOC_TYPE,  11)
#define DSI_SET_PANEL_PARAM   _IOW(DSI_DEVICE_IOC_TYPE, 12, args_dsi_set_panel_param)
#define DSI_SET_LCM_PARAM     _IOW(DSI_DEVICE_IOC_TYPE, 13, args_dsi_set_lcm_param)
#define DSI_CONFIG_FRMTRK     _IOW(DSI_DEVICE_IOC_TYPE, 14, args_dsi_config_frmtrk)
#define DSI_DISABLE_FRMTRK    _IO(DSI_DEVICE_IOC_TYPE,  15)
#define DSI_SET_DATARATE      _IOW(DSI_DEVICE_IOC_TYPE, 16, __u32)
#define DSI_SEND_VM_COMMAND   _IOW(DSI_DEVICE_IOC_TYPE, 17, args_dsi_send_vm_command)
#define DSI_READ_PANEL_ID     _IOR(DSI_DEVICE_IOC_TYPE, 18, struct dsi_panel_id)
#define DSI_READ_PANEL_ERR    _IOR(DSI_DEVICE_IOC_TYPE, 19, struct dsi_panel_err)
#define DSI_UPDATE_HBLANKING  _IOW(DSI_DEVICE_IOC_TYPE, 20, args_dsi_update_hblank_reg)
#define DSI_WAIT_DDDS_LOCK    _IOW(DSI_DEVICE_IOC_TYPE, 21, args_dsi_polling)
#define DSI_WAIT_FRMTRK_LOCK  _IOW(DSI_DEVICE_IOC_TYPE, 22, args_dsi_polling)
#define DSI_FINIT             _IO(DSI_DEVICE_IOC_TYPE, 23)
#define DSI_LANE_SWAP         _IOW(DSI_DEVICE_IOC_TYPE, 24, __u8)
#define DSI_INIT_STATE_CHECK  _IOW(DSI_DEVICE_IOC_TYPE, 25, args_dsi_init_state_check)

#endif /* _MTK_WRAPPER_DSI_ */
