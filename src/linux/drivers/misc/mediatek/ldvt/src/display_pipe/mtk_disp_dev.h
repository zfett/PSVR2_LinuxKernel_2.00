/*
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_DISP_DEV_H
#define _MTK_DISP_DEV_H

#include <linux/ioctl.h>
#include <soc/mediatek/mtk_lhc.h>

#define IOCTL_BUF_SIZE 128
#define MAX_FILE_NUM 2
#define MAX_ARG_SIZE 20

struct disp_parameter {
	uint32_t scenario;

	uint32_t input_format;
	uint32_t output_format;
	uint32_t input_height;
	uint32_t input_width;
	uint32_t input_fps;

	uint32_t disp_fps;

	uint32_t dp_enable;
	uint32_t frmtrk_enable;

	uint32_t dsc_enable;
	uint32_t encode_rate;
	uint32_t dsc_passthru;
	uint32_t lhc_enable;
	uint32_t dump_enable;
	uint32_t fbdc_enable;

	uint32_t edid_sel;
};

struct disp_img_info {
	char file[MAX_FILE_NUM][IOCTL_BUF_SIZE];
	char out_file[IOCTL_BUF_SIZE];
	uint32_t index;
	uint32_t format;
	uint32_t height;
	uint32_t width;
};

struct mtk_disp_lhc_hist {
	u8 module_num; /* FPGA: 1, SoC: 4 */
	/** LHC HIST RESULT */
	u8 r[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
	u8 g[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
	u8 b[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
};

#define CLI_MAGIC 'd'

#define IOCTL_DISP_SET_PARAMETER	\
		_IOW(CLI_MAGIC, 0, struct disp_parameter)
#define IOCTL_DISP_START		_IO(CLI_MAGIC, 1)
#define IOCTL_DISP_STOP			_IO(CLI_MAGIC, 2)
#define IOCTL_DISP_LOAD_IMG		_IOW(CLI_MAGIC, 3, struct disp_img_info)
#define IOCTL_DISP_SAVE_IMG		_IOW(CLI_MAGIC, 4, struct disp_img_info)
#define IOCTL_DISP_UNLOAD_IMG		_IO(CLI_MAGIC, 5)
#define IOCTL_DISP_IMG_IDX		_IOW(CLI_MAGIC, 6, int)
#define IOCTL_DISP_LHC			\
		_IOR(CLI_MAGIC, 7, struct mtk_disp_lhc_hist)

#define IOCTL_DISP_ION_GET_HANDLE	\
		_IOWR(CLI_MAGIC, 10, struct mtk_common_buf_handle)
#define IOCTL_DISP_ION_SET_BUFFER	\
		_IOW(CLI_MAGIC, 11, struct mtk_common_buf_handle)
#define IOCTL_DISP_ION_PUT_HANDLE	\
		_IOW(CLI_MAGIC, 12, struct mtk_common_buf_handle)

#endif
