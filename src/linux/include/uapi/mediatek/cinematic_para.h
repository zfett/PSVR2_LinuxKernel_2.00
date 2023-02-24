/*
 * Copyright (c) 2017 MediaTek Inc.
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
 * @file cinematic_para.h
 */

#ifndef __CINEMATIC_PARA_H__
#define __CINEMATIC_PARA_H__

#define IMG_PATH_LEN 256
#define CB_BUF_SIZE 128
#define CINEMATIC_MODULE_CNT 5
#define COMPRESS_OUTPUT_ENABLE	(1 << 1)
#define COMPRESS_INPUT_ENABLE	(1 << 0)

/* IOCTL with cinematic kernel */
#define CLI_MAGIC 'I'
#define IOCTL_READ		_IOR(CLI_MAGIC, 0, int)
#define IOCTL_WRITE		_IOW(CLI_MAGIC, 1, int)
#define IOCTL_IMPORT_FD_TO_HANDLE \
		_IOWR(CLI_MAGIC, 2, struct mtk_common_buf_handle)
#define IOCTL_SET_BUF		_IOW(CLI_MAGIC, 3, struct mtk_common_set_buf)
#define IOCTL_PUT_HANDLE	_IOW(CLI_MAGIC, 4, struct mtk_common_put_handle)
#define IOCTL_SET_PARAMETER	_IOW(CLI_MAGIC, 5, struct cinematic_parameter)
#define IOCTL_SET_PIPE		_IOW(CLI_MAGIC, 6, enum pipe_cmd_list)
#define IOCTL_SET_DISP_ID	_IOW(CLI_MAGIC, 7, int)
#define IOCTL_QUEUE_JOBS	_IOW(CLI_MAGIC, 8, struct cc_job)
#define IOCTL_GET_WDMA_FRAME_CNT _IOR(CLI_MAGIC, 9, unsigned long long)
#define IOCTL_GET_WDMA_FRAME_ID  _IOR(CLI_MAGIC, 10, int)
#define IOCTL_MUTE_DSI		 _IOW(CLI_MAGIC, 11, int)
#define IOCTL_GET_RBFC_WR_AVG_DIFF  _IOR(CLI_MAGIC, 12, int)
#define IOCTL_DUMP_WDMA		_IOW(CLI_MAGIC, 13, int)
#define IOCTL_DUMP_RDMA		_IOW(CLI_MAGIC, 14, int)
#define IOCTL_SET_STRIP_BUF_ID  _IOW(CLI_MAGIC, 15, int)


/* IOCTL with disp sys kernel */
#define DISP_CLI_MAGIC 'd'
#define IOCTL_DISP_LHC _IOR(DISP_CLI_MAGIC, 7, struct mtk_disp_lhc_hist)

/** @ingroup type_group_twin_enum
 * @brief Module types supported by Twin Mode Library. \n
 * value is from TAMT_RDMA to TAMT_MAX
*/
enum twin_algo_module_type {
	/** RDMA module */
	TAMT_RDMA,
	/** WARP module */
	TAMT_WARP,
	/** ISP module */
	TAMT_ISP,
	/** WDMA module */
	TAMT_WDMA,
	/** MERGE module */
	TAMT_MERGE,
	/** RESIZER module */
	TAMT_RESIZER,
	/** HDR module */
	TAMT_HDR,
	/** TDSHP module */
	TAMT_TDSHP,
	/** CROP module */
	TAMT_CROP,
	/** SLICER module */
	TAMT_SLICER,
	/** P2S module */
	TAMT_P2S,
	/** Undefined module */
	TAMT_MAX,
};

/** @ingroup type_group_twin_struct
 * @brief Output structure of Twin Mode Lib. \n
*/
struct twin_algo_module_para {
	/** Module types supported by Twin Mode Library */
	enum twin_algo_module_type type;
	/** Enable pq or not, have effects on HDR and TDSHP */
	unsigned char pq_enable;
	/** Input start offset of each partition */
	int in_start[4];
	/** Input end offset of each partition */
	int in_end[4];
	/** Output start offset of each partition */
	int out_start[4];
	/** Output start offset of each partition */
	int out_end[4];
	/** Input offset for resizer */
	int in_int_offset[4];
	/** Input sub offset for resizer */
	int in_sub_offset[4];
	/** Input height */
	int in_height;
	/** Output height */
	int out_height;
};

/* which path to import fd */
enum mtk_cu_path_index {
	PIPE_0,
	PIPE_1,
	PIPE_2,
	PIPE_3,
	CU_PATH_MAX = 4
};

/* which device to import fd */
enum mtk_cu_buf_dev {
	CINEMATIC_WDMA0,
	CINEMATIC_WDMA1,
	CINEMATIC_WDMA2,
	CINEMATIC_WDMA3,
	CINEMATIC_RDMA0,
	CU_DEV_MAX = 5
};

struct crop_para {
	int x;
	int y;
	int out_w;
	int out_h;
};

struct rsz_para {
	int out_w;
	int out_h;
};

enum pipe_cmd_list {
	/** Initialize path */
	INIT_PATH,
	/** Trigger Path to work */
	TRIG_PATH,
	/** Reset Path hardware */
	RESET_PATH,
	/** Display */
	DISP_PATH,
	/** Close Disp */
	DEINIT_PATH,
	/** Pause Disp */
	PAUSE_PATH,
	/** Resume cmd */
	RESUME_PATH,
	/** Unknown cmd */
	UNKNOWN_CMD,
};

enum CINEMATIC_CB_TYPE {
	CB_UNKNOWN,
	CB_DSI_UNDERFLOW,
	CB_GPU_SKIP_DONE,
	CB_SEQ_CHG,
	CB_DISP_PAUSE,
	CB_DISP_DISPLAY,
	CB_WAIT_DP,
	CB_DP_STABLE,
	CB_WDMA_FIRST_FRAME,
	CB_EXIT
};

struct rsz_out {
	int w;
	int h;
};

struct crop_out {
	int w;
	int h;
};

struct cc_job {
	struct rsz_out ct_rsz;
	struct crop_out ct_crop;
};

struct gpu_para {
	int bypass;
	int out_w;
	int out_h;
	int in_fmt;
	int out_fmt;
	int strip_mode;
	int strip_h;
	int strip_buf_num;
};

struct exit_cb_data {
	unsigned long long wdma_frame_cnt;
	unsigned long long wdma_sof_cnt;
	unsigned int skip_frame_cnt;
};

struct cinematic_parameter {
	int pipe_idx;
	int input_width;
	int input_height;
	int pat_sel;
	int lhc_en;
	int lhc_sync_path;
	int pvric_en;
	int input_fps;
	int disp_fps;
	int frmtrk_en;
	int sof_sel;
	int input_from_dp;
	int dsc_enable;
	int encode_rate;
	int err_detect;
	int sbrc_recover;
	int quadruple_pipe;
	int wdma_csc_en;
	int dsi_out_fmt;
	int check_byp;
	int edid_sel;
	struct gpu_para gp;
	struct crop_para cp;
	struct rsz_para rp;
	struct twin_algo_module_para cinematic_cfg[CINEMATIC_MODULE_CNT];
};

#endif


