/*
 * Copyright (c) 2019 MediaTek Inc.
 * Authors:
 *	Shan Lin <Shan.Lin@mediatek.com>
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

#ifndef MTK_VR_TRACKING_COMMON_H
#define MTK_VR_TRACKING_COMMON_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define CONFIG_GAZE_CAMERA_PATH 1
#define CONFIG_CAM 1
#define CONFIG_VPU 1
#define CV_LOG_DBG 2
#define CV_LOG_INFO 1
#define CV_LOG_ERR 0

#define POWER_READY 1

#define GCE_SW_TOKEN_SIDE_P1_SOF 940
#define GCE_SW_TOKEN_SIDE_P2_SOF 941
#define GCE_SW_TOKEN_SIDE_P2_DONE 942
#define GCE_SW_TOKEN_SIDE_WPE_SOF 943
#define GCE_SW_TOKEN_SIDE_WPE_DONE 944
#define GCE_SW_TOKEN_FE_SOF 945
#define GCE_SW_TOKEN_FE_DONE 946
#define GCE_SW_TOKEN_WDMA0_DONE 947
#define GCE_SW_TOKEN_WDMA1_DONE 948
#define GCE_SW_TOKEN_WDMA2_DONE 949
#define GCE_SW_TOKEN_GZ_P2_SOF 950
#define GCE_SW_TOKEN_GZ_WDMA_DONE 951
#define GCE_SW_TOKEN_KICK_FM_VPU 952
#define GCE_SW_TOKEN_KICK_BE_VPU 953
#define GCE_SW_TOKEN_KICK_WPE_AFTER_P2_DONE 954


/*---------------------sub device control bit--------------------------*/

#define DEV_WARPA_FE_ENABLE_MASK		(1 << 0)
#define DEV_FM_ENABLE_MASK			(1 << 1)


/*warpa_fe control bit*/
/* enable warpa -> fe */
#define WARPA_FE_CONNECT_TO_FE			(1 << 0)
/* enable warpa -> wdma */
#define WARPA_FE_CONNECT_TO_WDMA		(1 << 1)
/* enable warpa -> p_scaler */
#define WARPA_FE_CONNECT_TO_P_SCALE		(1 << 2)
/* enable read rbfc of warpa */
#define WARPA_FE_RBFC_ENABLE_MASK		(1 << 3)
/* bypass rsz0 */
#define WARPA_FE_RSZ_0_BYPASS_MASK		(1 << 4)
/* bypass rsz1 */
#define WARPA_FE_RSZ_1_BYPASS_MASK		(1 << 5)
/* bypass padding0 */
#define WARPA_FE_PADDING_0_BYPASS_MASK		(1 << 6)
/* bypass padding1 */
#define WARPA_FE_PADDING_1_BYPASS_MASK		(1 << 7)
/* bypass padding2 */
#define WARPA_FE_PADDING_2_BYPASS_MASK		(1 << 8)
/* warpa read from 1:sram, 0:dram */
#define WARPA_FE_WPE_INPUT_FROM_SYSRAM		(1 << 9)
/* warpa output(wdma0) to 1:sram, 0:dram */
#define WARPA_FE_WPE_OUTPUT_TO_SYSRAM		(1 << 10)
/* fe output(feo) to 1:sram, 0:dram */
#define WARPA_FE_FE_OUTPUT_TO_SYSRAM		(1 << 11)
/* padding output(wdma1, wdma2) to 1:sram, 0:dram */
#define WARPA_FE_PADDING_OUTPUT_TO_SYSRAM	(1 << 12)
/* set warpa in buffer to 1: p2 dump buffer, 0: original buffer */
#define WARPA_FE_FROM_SENSOR_MASK		(1 << 13)
/* set half buffer mode to warpa1_rbfc(only work when rbfc enabled) */
#define WARPA_FE_HALF_BUFFER_MASK		(1 << 14)
/* warpa_fe will trigger by p2 gce done event instead of mutex sof */
#define WARPA_FE_KICK_AFTER_P2_DONE		(1 << 15)
/* set 256-line buffer mode to warpa1_rbfc(only work when rbfc enabled) */
#define WARPA_FE_LINE_BUFFER_MASK		(1 << 16)
/* enalbe warpa_fe to dump ir & vpu(edge detection) file or not */
#define WARPA_FE_DUMP_BE_FILE_MASK		(1 << 17)


/*fm control bit*/
/* fm input(feo/feT) from 1:sram, 0:dram */
#define FM_DEV_FEO_SYSRAM_MODE			(1 << 0)
/* fm input(wdma) from 1:sram, 0:dram */
#define FM_DEV_IMG_SYSRAM_MODE			(1 << 1)
/* fm output(fmo) to 1:sram, 0:dram */
#define FM_DEV_FMO_SYSRAM_MODE			(1 << 2)
/* enable fm_dev dump file or not */
#define FM_DEV_DUMP_FILE_ENABLE			(1 << 3)
/* enable fm_dev to run scaled(1/4, 1/16) fmt */
#define FM_DEV_SCALED_FMT_ENABLE		(1 << 4)

/*gaze control bit*/
#define DEV_GAZE_CAMERA_ENABLE_RBFC_MASK	(1 << 0)
#define DEV_GAZE_CAMERA_FROM_SENSOR_MASK	(1 << 1)
#define DEV_GAZE_CAMERA_MUTEX_EOF_TIMING	(1 << 2)
#define DEV_GAZE_CAMERA_RBFC_FROM_SYSRAM_MASK	(1 << 3)
#define DEV_GAZE_CAMERA_DBG_TEST_MASK		(1 << 4)
#define DEV_GAZE_CAMERA_WARPA_OUT_MODE_ALL	(1 << 5)
#define DEV_GAZE_CAMERA_P2_SYSRAM_ENABLE	(1 << 6)
#define DEV_GAZE_CAMERA_WARPA_INPUT_DUMP	(1 << 7)
#define DEV_GAZE_CAMERA_WARPA_MAP_BUF_DUMP	(1 << 8)
#define DEV_GAZE_CAMERA_FOR_DEBUG_1		(1 << 9)
#define DEV_GAZE_CAMERA_WARPA_KICK_P2_DONE_GCE	(1 << 10)
#define DEV_GAZE_CAMERA_DUMP_FILE_ENABLE	(1 << 11)
#define DEV_GAZE_CAMERA_WAIT_VPU_DONE		(1 << 12)
#define DEV_GAZE_CAMERA_2EYES_TEST_MASK		(1 << 13)


#ifdef CONFIG_MACH_MT3612_A0
/*
 * Emulator hard coded sram address (0x0 - 0x5FFFFF)
 *** WPE
 * for P1,P2,         : 0x000000 - 0x190000 , size 0x190000 (2560x640=0x190000)
 * for P3~P6,         : 0x500000 - 0x550000 , size 0x050000 (1280x256=0x50000)
 * (P3~P6 start from 0x500000 for sharing with GPU firmware use)
 *** WDMA
 * for P1,P2
 * wdma0_R0           : 0x190000 - 0x320000, size 0x190000 (2560x640=0x190000)
 * wdma0_R1           : 0x320000 - 0x4B0000, size 0x190000 (2560x640=0x190000)
 * wdma1_R0           : 0x4B0000 - 0x528000, size 0x078000 (1536x320=0x78000)
 * wdma1_R1           : 0x528000 - 0x5A0000, size 0x078000 (1536x320=0x78000)
 * wdma2_R0           : 0x5A0000 - 0x5C8000, size 0x028000 (1024x160=0x28000)
 * wdma2_R1           : 0x5C8000 - 0x5F0000, size 0x028000 (1024x160=0x28000)
 * for P3
 * wdma0_R0           : 0x074000 - 0x13C000, size 0xC8000 (1280x640=0xC8000)
 * wdma0_R1           : 0x13C000 - 0x204000, size 0xC8000 (1280x640=0xC8000)
 *** FE
 * feo0_P             : 0x240000 - 0x240C80, size 0x001000 (real size=0xC80)
 * feo0_D             : 0x241000 - 0x217400, size 0x007000 (real size=0x6400)
 * feo1_P             : 0x248000 - 0x248C80, size 0x001000 (real size=0xC80)
 * feo1_D             : 0x249000 - 0x24F400, size 0x007000 (real size=0x6400)
 * feo2_P             : 0x250000 - 0x250C80, size 0x001000 (real size=0xC80)
 * feo2_D             : 0x251000 - 0x257400, size 0x007000 (real size=0x6400)
 * feo3_P             : 0x258000 - 0x258C80, size 0x001000 (real size=0xC80)
 * feo3_D             : 0x259000 - 0x257400, size 0x007000 (real size=0x6400)
 * fet0_P             : 0x260000 - 0x260C80, size 0x001000 (real size=0xC80)
 * fet0_D             : 0x261000 - 0x267400, size 0x007000 (real size=0x6400)
 * fet1_P             : 0x268000 - 0x268C80, size 0x001000 (real size=0xC80)
 * fet1_D             : 0x269000 - 0x26F400, size 0x007000 (real size=0x6400)
 * fet2_P             : 0x270000 - 0x270C80, size 0x001000 (real size=0xC80)
 * fet2_D             : 0x271000 - 0x277400, size 0x007000 (real size=0x6400)
 *** FM
 * fmo_s_R0           : 0x280000 - 0x281100, size 0x001100 (real size=0x1080)
 * fm_zncc_s_R0       : 0x281100 - 0x287500, size 0x006F00 (real size=0x6400)
 * fmo_s_R1           : 0x288000 - 0x289100, size 0x001100 (real size=0x1080)
 * fm_zncc_s_R1       : 0x289100 - 0x28F500, size 0x006F00 (real size=0x6400)
 * fmo_t_R0           : 0x290000 - 0x291100, size 0x001100 (real size=0x1080)
 * fm_zncc_t_R0       : 0x291100 - 0x297500, size 0x006F00 (real size=0x6400)
 * fmo_t_R1           : 0x298000 - 0x299100, size 0x001100 (real size=0x1080)
 * fm_zncc_t_R1       : 0x299100 - 0x29F500, size 0x006F00 (real size=0x6400)
 * 1_4_fmo_t_R0       : 0x2A0000 - 0x2A1100, size 0x001100 (real size=0x1080)
 * 1_4_fm_zncc_t_R0   : 0x2A1100 - 0x2A7500, size 0x006F00 (real size=0x6400)
 * 1_4_fmo_t_R1       : 0x2A8000 - 0x2A9100, size 0x001100 (real size=0x1080)
 * 1_4_fm_zncc_t_R1   : 0x2A9100 - 0x2AF500, size 0x006F00 (real size=0x6400)
 * 1_16_fmo_t_R0      : 0x2B0000 - 0x2B1100, size 0x001100 (real size=0x1080)
 * 1_16_fm_zncc_t_R0  : 0x2B1100 - 0x2B7500, size 0x006F00 (real size=0x6400)
 * 1_16_fmo_t_R1      : 0x2B8000 - 0x2B9100, size 0x001100 (real size=0x1080)
 * 1_16_fm_zncc_t_R1  : 0x2B9100 - 0x2BF500, size 0x006F00 (real size=0x6400)
 */

/* P1,P2 WPE */
#define WARPA_IN_SRAM_ADDR 0x0
/* P3~P6 WPE */
#define WARPA_IN_SRAM_ADDR_SHARE 0x500000
#define WARPA_IN_SRAM_ADDR_END 0x550000

/* 3c1 WDMA addr */
#define WDMA_0_R0_OUT_SRAM_ADDR_FULL 0x0
#define WDMA_1_R0_OUT_SRAM_ADDR_FULL 0x190000
#define WDMA_2_R0_OUT_SRAM_ADDR_FULL 0x210000

/* P1,P2 WDMA */
#define WDMA_0_R0_OUT_SRAM_ADDR 0x190000
#define WDMA_0_R1_OUT_SRAM_ADDR 0x320000
#define WDMA_1_R0_OUT_SRAM_ADDR 0x4B0000
#define WDMA_1_R1_OUT_SRAM_ADDR 0x528000
#define WDMA_2_R0_OUT_SRAM_ADDR 0x5A0000
#define WDMA_2_R1_OUT_SRAM_ADDR 0x5C8000
#define VR_END_ADDR 0x5F0000

/* P3 WDMA */
#define WDMA_0_R0_OUT_SRAM_ADDR_SHARE 0x74000
#define WDMA_0_R1_OUT_SRAM_ADDR_SHARE 0x13C000

#define FEO_0_P_OUT_SRAM_ADDR 0x240000
#define FEO_0_D_OUT_SRAM_ADDR 0x241000
#define FEO_1_P_OUT_SRAM_ADDR 0x248000
#define FEO_1_D_OUT_SRAM_ADDR 0x249000
#define FEO_2_P_OUT_SRAM_ADDR 0x250000
#define FEO_2_D_OUT_SRAM_ADDR 0x251000
#define FEO_3_P_OUT_SRAM_ADDR 0x258000
#define FEO_3_D_OUT_SRAM_ADDR 0x259000

#define FET_0_P_OUT_SRAM_ADDR 0x260000
#define FET_0_D_OUT_SRAM_ADDR 0x261000
#define FET_1_P_OUT_SRAM_ADDR 0x268000
#define FET_1_D_OUT_SRAM_ADDR 0x269000
#define FET_2_P_OUT_SRAM_ADDR 0x270000
#define FET_2_D_OUT_SRAM_ADDR 0x271000

#define FMO_S_R0_OUT_SRAM_ADDR 0x280000
#define FMO_S_R0_ZNCC_SRAM_ADDR 0x281100
#define FMO_S_R1_OUT_SRAM_ADDR 0x288000
#define FMO_S_R1_ZNCC_SRAM_ADDR 0x289100

#define FMO_T_R0_OUT_SRAM_ADDR 0x290000
#define FMO_T_R0_ZNCC_SRAM_ADDR 0x291100
#define FMO_T_R1_OUT_SRAM_ADDR 0x298000
#define FMO_T_R1_ZNCC_SRAM_ADDR 0x299100

#define FMO_1_4_T_R0_OUT_SRAM_ADDR 0x2A0000
#define FMO_1_4_T_R0_ZNCC_SRAM_ADDR 0x2A1100
#define FMO_1_4_T_R1_OUT_SRAM_ADDR 0x2A8000
#define FMO_1_4_T_R1_ZNCC_SRAM_ADDR 0x2A9100

#define FMO_1_16_T_R0_OUT_SRAM_ADDR 0x2B0000
#define FMO_1_16_T_R0_ZNCC_SRAM_ADDR 0x2B1100
#define FMO_1_16_T_R1_OUT_SRAM_ADDR 0x2B8000
#define FMO_1_16_T_R1_ZNCC_SRAM_ADDR 0x2B9100
#define CV_END_ADDR 0x2BF500

#else
/*
 * FPGA hard coded sram address (0x0 - 0x3FFFFF)
 *** WPE
 * for test case 3c1, need 2560x640 for wdma0
 * wpe input : 0x0 - 0x0, size 0x0, only for 3b, align p2 output addr
 *** WDMA
 * wdma0              : 0x000000 - 0x190000, size 0x190000 (2560x640=0x190000)
 * wdma1              : 0x190000 - 0x208000, size 0x080000 (1536x320=0x78000)
 * wdma2              : 0x210000 - 0x238000, size 0x030000 (1024x160=0x28000)
 *** FE
 * feo0_P             : 0x240000 - 0x240C80, size 0x001000 (real size=0xC80)
 * feo0_D             : 0x241000 - 0x217400, size 0x007000 (real size=0x6400)
 * feo1_P             : 0x248000 - 0x248C80, size 0x001000 (real size=0xC80)
 * feo1_D             : 0x249000 - 0x24F400, size 0x007000 (real size=0x6400)
 * feo2_P             : 0x250000 - 0x250C80, size 0x001000 (real size=0xC80)
 * feo2_D             : 0x251000 - 0x257400, size 0x007000 (real size=0x6400)
 * feo3_P             : 0x258000 - 0x258C80, size 0x001000 (real size=0xC80)
 * feo3_D             : 0x259000 - 0x257400, size 0x007000 (real size=0x6400)
 * fet0_P             : 0x260000 - 0x260C80, size 0x001000 (real size=0xC80)
 * fet0_D             : 0x261000 - 0x267400, size 0x007000 (real size=0x6400)
 * fet1_P             : 0x268000 - 0x268C80, size 0x001000 (real size=0xC80)
 * fet1_D             : 0x269000 - 0x26F400, size 0x007000 (real size=0x6400)
 * fet2_P             : 0x270000 - 0x270C80, size 0x001000 (real size=0xC80)
 * fet2_D             : 0x271000 - 0x277400, size 0x007000 (real size=0x6400)
 *** FM
 * fmo_s_R0           : 0x280000 - 0x281100, size 0x001100 (real size=0x1080)
 * fm_zncc_s_R0       : 0x281100 - 0x287500, size 0x006F00 (real size=0x6400)
 * fmo_s_R1           : 0x288000 - 0x289100, size 0x001100 (real size=0x1080)
 * fm_zncc_s_R1       : 0x289100 - 0x28F500, size 0x006F00 (real size=0x6400)
 * fmo_t_R0           : 0x290000 - 0x291100, size 0x001100 (real size=0x1080)
 * fm_zncc_t_R0       : 0x291100 - 0x297500, size 0x006F00 (real size=0x6400)
 * fmo_t_R1           : 0x298000 - 0x299100, size 0x001100 (real size=0x1080)
 * fm_zncc_t_R1       : 0x299100 - 0x29F500, size 0x006F00 (real size=0x6400)
 * 1_4_fmo_t_R0       : 0x2A0000 - 0x2A1100, size 0x001100 (real size=0x1080)
 * 1_4_fm_zncc_t_R0   : 0x2A1100 - 0x2A7500, size 0x006F00 (real size=0x6400)
 * 1_4_fmo_t_R1       : 0x2A8000 - 0x2A9100, size 0x001100 (real size=0x1080)
 * 1_4_fm_zncc_t_R1   : 0x2A9100 - 0x2AF500, size 0x006F00 (real size=0x6400)
 * 1_16_fmo_t_R0      : 0x2B0000 - 0x2B1100, size 0x001100 (real size=0x1080)
 * 1_16_fm_zncc_t_R0  : 0x2B1100 - 0x2B7500, size 0x006F00 (real size=0x6400)
 * 1_16_fmo_t_R1      : 0x2B8000 - 0x2B9100, size 0x001100 (real size=0x1080)
 * 1_16_fm_zncc_t_R1  : 0x2B9100 - 0x2BF500, size 0x006F00 (real size=0x6400)
 * end_addr           : 0x2BF500
 */

/* only 3b use warpa in sram addr for fpga*/
#define WARPA_IN_SRAM_ADDR 0x0
#define WARPA_IN_SRAM_ADDR_SHARE 0x24000
#define WARPA_IN_SRAM_ADDR_END 0x74000

/* 3c1 WDMA addr */
#define WDMA_0_R0_OUT_SRAM_ADDR_FULL 0x0
#define WDMA_1_R0_OUT_SRAM_ADDR_FULL 0x190000
#define WDMA_2_R0_OUT_SRAM_ADDR_FULL 0x210000

/* only 3c1 use following sram addr for fpga*/
#define WDMA_0_R0_OUT_SRAM_ADDR 0x0
#define WDMA_1_R0_OUT_SRAM_ADDR 0x190000
#define WDMA_2_R0_OUT_SRAM_ADDR 0x210000
#define VR_END_ADDR 0x5F0000

#define FEO_0_P_OUT_SRAM_ADDR 0x240000
#define FEO_0_D_OUT_SRAM_ADDR 0x241000
#define FEO_1_P_OUT_SRAM_ADDR 0x248000
#define FEO_1_D_OUT_SRAM_ADDR 0x249000
#define FEO_2_P_OUT_SRAM_ADDR 0x250000
#define FEO_2_D_OUT_SRAM_ADDR 0x251000
#define FEO_3_P_OUT_SRAM_ADDR 0x258000
#define FEO_3_D_OUT_SRAM_ADDR 0x259000

#define FET_0_P_OUT_SRAM_ADDR 0x260000
#define FET_0_D_OUT_SRAM_ADDR 0x261000
#define FET_1_P_OUT_SRAM_ADDR 0x268000
#define FET_1_D_OUT_SRAM_ADDR 0x269000
#define FET_2_P_OUT_SRAM_ADDR 0x270000
#define FET_2_D_OUT_SRAM_ADDR 0x271000

#define FMO_S_R0_OUT_SRAM_ADDR 0x280000
#define FMO_S_R0_ZNCC_SRAM_ADDR 0x281100
#define FMO_S_R1_OUT_SRAM_ADDR 0x288000
#define FMO_S_R1_ZNCC_SRAM_ADDR 0x289100

#define FMO_T_R0_OUT_SRAM_ADDR 0x290000
#define FMO_T_R0_ZNCC_SRAM_ADDR 0x291100
#define FMO_T_R1_OUT_SRAM_ADDR 0x298000
#define FMO_T_R1_ZNCC_SRAM_ADDR 0x299100

#define FMO_1_4_T_R0_OUT_SRAM_ADDR 0x2A0000
#define FMO_1_4_T_R0_ZNCC_SRAM_ADDR 0x2A1100
#define FMO_1_4_T_R1_OUT_SRAM_ADDR 0x2A8000
#define FMO_1_4_T_R1_ZNCC_SRAM_ADDR 0x2A9100

#define FMO_1_16_T_R0_OUT_SRAM_ADDR 0x2B0000
#define FMO_1_16_T_R0_ZNCC_SRAM_ADDR 0x2B1100
#define FMO_1_16_T_R1_OUT_SRAM_ADDR 0x2B8000
#define FMO_1_16_T_R1_ZNCC_SRAM_ADDR 0x2B9100
#define CV_END_ADDR 0x2BF500

/* P3 WDMA */
#define WDMA_0_R0_OUT_SRAM_ADDR_SHARE 0x74000
#define WDMA_0_R1_OUT_SRAM_ADDR_SHARE 0x13C000

#endif

/* gaze warpa in sram addr for fpga*/
#define GAZE_WARPA_IN_SRAM_ADDR	0x0
#define GAZE_SYSRAM1_END_ADDR	0x60000
#define GAZE_VPU_BUF_SIZE	51200

/** common buffer related structure **/
struct mtk_common_set_buf {
	__u32 dev_index;
	__u32 buf_type;
	__u64 handle;
	__u32 pitch;
	__u32 offset;
	__u32 format;
};

struct mtk_common_buf_handle {
	__u32 index;
	__u32 buf_type;
	__s32 fd;
	__u32 offset;
	__u64 handle;
};

struct mtk_common_put_handle {
	__u32 index;
	__u32 buf_type;
	__u32 offset;
	__u64 handle;
};

/* VR tracking GCE used CPR thread define */
#define GCE_P2_sof_cnt			GCE_CPR(50)
#define GCE_P2_done_cnt			GCE_CPR(51)
#define GCE_wpe_done_cnt		GCE_CPR(52)
#define GCE_fe_done_cnt			GCE_CPR(53)
#define GCE_fm_vpu_done_cnt		GCE_CPR(54)
#define GCE_P1_IR_sof_cnt		GCE_CPR(55)
#define GCE_P1_IR_done_cnt		GCE_CPR(56)
#define GCE_wpe_max_exec		GCE_CPR(57)
#define GCE_fe_sof_ts			GCE_CPR(58)
#define GCE_fe_done_ts			GCE_CPR(59)
#define GCE_fe_max_exec			GCE_CPR(60)
#define GCE_fms_max_delay		GCE_CPR(61)
#define GCE_fms_max_exec		GCE_CPR(62)
#define GCE_feT_max_delay		GCE_CPR(63)
#define GCE_feT_max_exec		GCE_CPR(64)
#define GCE_feT_1_4_max_exec		GCE_CPR(65)
#define GCE_feT_1_16_max_exec		GCE_CPR(66)
#define GCE_sc_1_16_max_delay		GCE_CPR(67)
#define GCE_sc_1_16_max_exec		GCE_CPR(68)
#define GCE_fmt_1_16_max_delay		GCE_CPR(69)
#define GCE_fmt_1_16_max_exec		GCE_CPR(70)
#define GCE_sc_1_4_max_delay		GCE_CPR(71)
#define GCE_sc_1_4_max_exec		GCE_CPR(72)
#define GCE_fmt_1_4_max_delay		GCE_CPR(73)
#define GCE_fmt_1_4_max_exec		GCE_CPR(74)
#define GCE_sc_max_delay		GCE_CPR(75)
#define GCE_sc_max_exec			GCE_CPR(76)
#define GCE_fmt_max_delay		GCE_CPR(77)
#define GCE_fmt_max_exec		GCE_CPR(78)
#define GCE_headpose_max_delay		GCE_CPR(79)
#define GCE_headpose_max_exec		GCE_CPR(80)
#define GCE_fm_vpu_done_ts		GCE_CPR(81)
#define GCE_fm_vpu_max_exec		GCE_CPR(82)
#define GCE_fms_sof_ts			GCE_CPR(83)
#define GCE_fms_done_ts			GCE_CPR(84)
#define GCE_feT_sof_ts			GCE_CPR(85)
#define GCE_feT_done_ts			GCE_CPR(86)
#define GCE_feT_1_4_done_ts		GCE_CPR(87)
#define GCE_feT_1_16_done_ts		GCE_CPR(88)
#define GCE_sc_1_16_sof_ts		GCE_CPR(89)
#define GCE_sc_1_16_done_ts		GCE_CPR(90)
#define GCE_fmt_1_16_sof_ts		GCE_CPR(91)
#define GCE_fmt_1_16_done_ts		GCE_CPR(92)
#define GCE_sc_1_4_sof_ts		GCE_CPR(93)
#define GCE_sc_1_4_done_ts		GCE_CPR(94)
#define GCE_fmt_1_4_sof_ts		GCE_CPR(95)
#define GCE_fmt_1_4_done_ts		GCE_CPR(96)
#define GCE_sc_sof_ts			GCE_CPR(97)
#define GCE_sc_done_ts			GCE_CPR(98)
#define GCE_fmt_sof_ts			GCE_CPR(99)
#define GCE_fmt_done_ts			GCE_CPR(100)
#define GCE_headpose_sof_ts		GCE_CPR(101)
#define GCE_P1_sof_ts			GCE_CPR(102)
#define GCE_P1_done_ts			GCE_CPR(103)
#define GCE_P1_max_exec			GCE_CPR(104)
#define GCE_BE_VPU_sof_ts		GCE_CPR(105)
#define GCE_BE_VPU_done_ts		GCE_CPR(106)
#define GCE_BE_VPU_max_delay		GCE_CPR(107)
#define GCE_BE_VPU_max_exec		GCE_CPR(108)
#define GCE_P1_VPU_max_exec		GCE_CPR(109)
#define GCE_P1_VPU_done_cnt		GCE_CPR(110)
#define GCE_FM_VPU_KICK_ts		GCE_CPR(111)
#define GCE_BE_VPU_KICK_ts		GCE_CPR(112)




struct mtk_fm_dev_statistics {
	__u32 warpa_frame_cnt;
	__u32 warpa_done_int_cur;
	__u32 warpa_done_int_max;
	__u32 fm_vpu_frame_cnt;
	__u32 fm_vpu_done_int_cur;
	__u32 fm_vpu_done_int_max;
	__u32 fe_frame_cnt;
	__u32 fe_done_int_cur;
	__u32 fe_done_int_max;
	__u32 fms_delay_cur;
	__u32 fms_delay_max;
	__u32 fms_done_int_cur;
	__u32 fms_done_int_max;
	__u32 feT_delay_cur;
	__u32 feT_delay_max;
	__u32 feT_done_int_cur;
	__u32 feT_done_int_max;
	__u32 feT_1_4_done_int_cur;
	__u32 feT_1_4_done_int_max;
	__u32 feT_1_16_done_int_cur;
	__u32 feT_1_16_done_int_max;
	__u32 sc_1_16_delay_cur;
	__u32 sc_1_16_delay_max;
	__u32 sc_1_16_exec_cur;
	__u32 sc_1_16_exec_max;
	__u32 fmt_1_16_delay_cur;
	__u32 fmt_1_16_delay_max;
	__u32 fmt_1_16_done_int_cur;
	__u32 fmt_1_16_done_int_max;
	__u32 sc_1_4_delay_cur;
	__u32 sc_1_4_delay_max;
	__u32 sc_1_4_exec_cur;
	__u32 sc_1_4_exec_max;
	__u32 fmt_1_4_delay_cur;
	__u32 fmt_1_4_delay_max;
	__u32 fmt_1_4_done_int_cur;
	__u32 fmt_1_4_done_int_max;
	__u32 sc_delay_cur;
	__u32 sc_delay_max;
	__u32 sc_exec_cur;
	__u32 sc_exec_max;
	__u32 fmt_delay_cur;
	__u32 fmt_delay_max;
	__u32 fmt_done_int_cur;
	__u32 fmt_done_int_max;
	__u32 head_pose_delay_cur;
	__u32 head_pose_delay_max;
	__u32 head_pose_done_int_cur;
	__u32 head_pose_done_int_max;
	__u32 be0_lim_cnt;
	__u32 be1_lim_cnt;
	__u32 be2_lim_cnt;
	__u32 be3_lim_cnt;
};

struct mtk_warpa_fe_statistics {
	__u32 p2_sof_cnt;
	__u32 p2_done_cnt;
	__u32 p1_sof_cnt;
	__u32 p1_done_cnt;
	__u32 be_vpu_cnt;
	__u32 wdma0_cnt;
	__u32 wdma1_cnt;
	__u32 wdma2_cnt;
	__u32 P1_exec_cur;
	__u32 P1_exec_max;
	__u32 BE_VPU_delay_cur;
	__u32 BE_VPU_delay_max;
	__u32 BE_VPU_exec_cur;
	__u32 BE_VPU_exec_max;
	__u32 P1_VPU_exec_cur;
	__u32 P1_VPU_exec_max;
};

enum vt_sysram_layout {
	/* dram mode */
	VT_SYSRAM_LAYOUT_NONE = 0,
	/* for 3c1, except wpe in, all sysram */
	VT_SYSRAM_LAYOUT_FULL = 1,
	/* for P1, P2, only wpe in, wdma0,1,2 go sysram */
	VT_SYSRAM_LAYOUT_VR = 2,
	/* for P3, P4, P5, P6, P7, P8, share with GPU */
	VT_SYSRAM_LAYOUT_CINE = 3,
};

/** warpa_fe header defines **/
enum mtk_warpa_fe_cmdq_event {
	/*MTK_WARPA_FE_CMDQ_EVENT_WR_OV_TH,*/
	MTK_WARPA_FE_CMDQ_EVENT_ISP_P1_SOF,
	MTK_WARPA_FE_CMDQ_EVENT_ISP_P1_DONE,
	MTK_WARPA_FE_CMDQ_EVENT_ISP_P2_SOF,
	MTK_WARPA_FE_CMDQ_EVENT_ISP_P2_DONE,
	MTK_WARPA_FE_CMDQ_EVENT_FE_DONE,
	MTK_WARPA_FE_CMDQ_EVENT_VPU_CORE0_DONE,
	MTK_WARPA_FE_CMDQ_EVENT_VPU_CORE1_DONE,
	MTK_WARPA_FE_CMDQ_EVENT_VPU_CORE2_DONE,
	MTK_WARPA_FE_CMDQ_EVENT_MUTEX_TD_EVENT0,
	MTK_WARPA_FE_CMDQ_EVENT_MAX,
};


struct mtk_warpa_fe_set_mask {
	__u32 warpafemask;
	__u32 sysram_layout;
};

struct mtk_warpa_fe_vpu_req {
	struct mtkivp_request *process_p1_img[16];
	__u32 be_ring_buf_cnt;
	__u32 be_en;
	__u32 be_label_img_en;
	__u32 p1vpu_pair;
	__u32 path_mask;
};


struct mtk_warpa_fe_config_warpa {
	__u32 coef_tbl_idx;
	__u16 user_coef_tbl[65];
	bool border_color_enable;
	__u8 border_color;
	__u8 unmapped_color;
	__u32 proc_mode;
	__u32 out_mode;
	__u32 reset;
	__u32 warpa_in_w;
	__u32 warpa_in_h;
	__u32 warpa_out_w;
	__u32 warpa_out_h;
	__u32 warpa_map_w;
	__u32 warpa_map_h;
	__u32 warpa_align;
};

struct mtk_warpa_fe_config_fe {
	__u32 fe_w;
	__u32 fe_h;
	__u32 blk_sz;
	__u32 fe_merge_mode;
	__u32 fe_flat_enable;
	__u32 fe_harris_kappa;
	__u32 fe_th_g;
	__u32 fe_th_c;
	__u32 fe_cr_val_sel;
	__u32 fe_align;
};

struct mtk_warpa_fe_config_wdma {
	__u32 wdma_align;
};

struct mtk_warpa_fe_config_rsz {
	__u32 rsz_0_out_w;
	__u32 rsz_0_out_h;
	__u32 rsz_1_out_w;
	__u32 rsz_1_out_h;
};

struct mtk_warpa_fe_config_padding {
	__u32 padding_val_0;
	__u32 padding_val_1;
	__u32 padding_val_2;
};


struct mtk_warpa_fe_trigger {
	__u32 no_wait;
};

struct mtk_warpa_fe_golden_path {
	char feo0_P_golden_path[200];
	char feo0_D_golden_path[200];
	char feo1_P_golden_path[200];
	char feo1_D_golden_path[200];
	char feo2_P_golden_path[200];
	char feo2_D_golden_path[200];
	char feo3_P_golden_path[200];
	char feo3_D_golden_path[200];
	__u32 feoP_size;
	__u32 feoD_size;
};

struct mtk_warpa_fe_camera_sync {
	__u32 sensor_fps;
	__u32 power_flag;
	__u32 delay;
};

/** warpa_fe ioctl numbers **/
#define MTK_WARPA_FE_MAGIC		0xae
#define MTK_WARPA_FE_IOCTL_IMPORT_FD_TO_HANDLE	_IOW(MTK_WARPA_FE_MAGIC, 0x00,\
		struct mtk_common_buf_handle)
#define MTK_WARPA_FE_IOCTL_SET_CTRL_MASK	_IOW(MTK_WARPA_FE_MAGIC, 0x01,\
		struct mtk_warpa_fe_set_mask)
#define MTK_WARPA_FE_IOCTL_CONFIG_WARPA		_IOW(MTK_WARPA_FE_MAGIC, 0x02,\
		struct mtk_warpa_fe_config_warpa)
#define MTK_WARPA_FE_IOCTL_CONFIG_FE	_IOW(MTK_WARPA_FE_MAGIC, 0x03,\
		struct mtk_warpa_fe_config_fe)
#define MTK_WARPA_FE_IOCTL_CONFIG_WDMA	_IOW(MTK_WARPA_FE_MAGIC, 0x04,\
		struct mtk_warpa_fe_config_wdma)
#define MTK_WARPA_FE_IOCTL_CONFIG_RSZ	_IOW(MTK_WARPA_FE_MAGIC, 0x05,\
		struct mtk_warpa_fe_config_rsz)
#define MTK_WARPA_FE_IOCTL_CONFIG_PADDING	_IOW(MTK_WARPA_FE_MAGIC, 0x6,\
		struct mtk_warpa_fe_config_padding)
#define MTK_WARPA_FE_IOCTL_SET_BUF		_IOW(MTK_WARPA_FE_MAGIC, 0x07,\
		struct mtk_common_set_buf)
#define MTK_WARPA_FE_IOCTL_STREAMON		_IOW(MTK_WARPA_FE_MAGIC, 0x08,\
		__u32)
#define MTK_WARPA_FE_IOCTL_STREAMOFF	_IOW(MTK_WARPA_FE_MAGIC, 0x09,\
		__u32)
#define MTK_WARPA_FE_IOCTL_TRIGGER	_IOW(MTK_WARPA_FE_MAGIC, 0x0A,\
		__u32)
#define MTK_WARPA_FE_IOCTL_RESET	_IOW(MTK_WARPA_FE_MAGIC, 0x0B,\
		__u32)
#define MTK_WARPA_FE_IOCTL_COMP	_IOW(MTK_WARPA_FE_MAGIC, 0x0C,\
		struct mtk_warpa_fe_golden_path)
#define MTK_WARPA_FE_IOCTL_PUT_HANDLE	_IOW(MTK_WARPA_FE_MAGIC, 0x0D,\
		struct mtk_common_put_handle)
#define MTK_WARPA_FE_IOCTL_SET_VPU_REQ	_IOW(MTK_WARPA_FE_MAGIC, 0x0E,\
		struct mtk_warpa_fe_vpu_req)
#define MTK_WARPA_FE_IOCTL_CHECK_FLOW_DONE	_IOW(MTK_WARPA_FE_MAGIC, 0x0F,\
		__u32)
#define MTK_WARPA_FE_IOCTL_GET_STATISTICS	_IOW(MTK_WARPA_FE_MAGIC, 0x10,\
		struct mtk_warpa_fe_statistics)
#define MTK_WARPA_FE_IOCTL_SET_LOG_LEVEL _IOW(MTK_WARPA_FE_MAGIC, 0x11,\
		__u32)
#define MTK_WARPA_FE_ENABLE_CAMERA_SYNC	_IOW(MTK_WARPA_FE_MAGIC, 0x12,\
		struct mtk_warpa_fe_camera_sync)
#define MTK_WARPA_FE_DISABLE_CAMERA_SYNC	_IOW(MTK_WARPA_FE_MAGIC, 0x13,\
		__u32)
#define MTK_WARPA_FE_IOCTL_FLUSH_BUF_CACHE	_IOW(MTK_WARPA_FE_MAGIC, 0x14,\
		__u64)
#define MTK_WARPA_FE_IOCTL_ONE_SHOT_DUMP	_IOW(MTK_WARPA_FE_MAGIC, 0x15,\
		__u32)
#define MTK_WARPA_FE_IOCTL_DEBUG_TEST_CAM	_IOW(MTK_WARPA_FE_MAGIC, 0x16,\
		struct mtk_warpa_fe_debug_test_cam)



/** fm_dev header defines **/

enum mtk_fm_dev_sr_mode {
	MTK_FM_SR_MODE_TEMPORAL,
	MTK_FM_SR_MODE_SPAIAL_AND_TEMPORAL,
	MTK_FM_SR_MODE_SPATIAL,
};

struct mtk_fm_dev_vpu_req {
	struct mtkivp_request *process_feT1;
	struct mtkivp_request *process_feT2;
	struct mtkivp_request *process_1_4_feT1;
	struct mtkivp_request *process_1_4_feT2;
	struct mtkivp_request *process_1_16_feT1;
	struct mtkivp_request *process_1_16_feT2;
	struct mtkivp_request *process_headpose;
	struct mtkivp_request *process_1_4_WDMA;
	struct mtkivp_request *process_1_16_WDMA;
	struct mtkivp_request *process_DRAM_TO_SRAM;
	struct mtkivp_request *process_SRAM_TO_DRAM;
};

enum mtk_fm_dev_flow {
	MTK_FM_FLOW_SPATIAL = 0,
	MTK_FM_FLOW_TEMPORAL = 1,
	MTK_FM_FLOW_1_4_TEMPORAL = 2,
	MTK_FM_FLOW_1_16_TEMPORAL = 3,
	MTK_FM_FLOW_MAX = 4,
};

enum mtk_fm_dev_wdma_idx {
	MTK_FM_WDMA0 = 0,
	MTK_FM_WDMA1 = 1,
	MTK_FM_WDMA2 = 2,
	MTK_FM_WDMA_IDX_MAX = 3,
};

struct mtk_fm_dev_debug_test {
	__u32 fm[MTK_FM_FLOW_MAX][2];
	__u32 wdma_dbg[MTK_FM_WDMA_IDX_MAX];
};

enum mtk_fm_dev_side_idx {
	MTK_SIDE_CAM_0 = 0,
	MTK_SIDE_CAM_1 = 1,
	MTK_SIDE_IDX_MAX = 2,
};

enum mtk_fm_dev_gaze_idx {
	MTK_GAZE_CAM_0 = 0,
	MTK_GAZE_CAM_1 = 1,
	MTK_GAZE_IDX_MAX = 2,
};

struct be_dbg {
	__u32 label_dbg;
	__u32 data_dbg;
	__u32 merge_table_dbg;
};

struct side_cam_dbg {
	__u32 camsv_dbg[2];
	struct be_dbg be[2];
};

struct gaze_cam_dbg {
	__u32 camsv_dbg;
};

struct mtk_warpa_fe_debug_test_cam {
	struct side_cam_dbg side[MTK_SIDE_IDX_MAX];
	struct gaze_cam_dbg gaze[MTK_GAZE_IDX_MAX];
};


struct mtk_fm_dev_trigger {
	__u32 no_wait;
};

enum mtk_fm_dev_cmdq_event {
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_FE,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA0,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA1,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA2,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_FM,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_VPU_CORE0,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_VPU_CORE1,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_VPU_CORE2,
	MTK_FM_DEV_CMDQ_EVENT_SOF_WARPA,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WARPA,
	MTK_FM_DEV_CMDQ_EVENT_SOF_FE,
	MTK_FM_DEV_CMDQ_EVENT_P2_WEN0_INCOMP,
	MTK_FM_DEV_CMDQ_EVENT_P2_WEN1_INCOMP,
	MTK_FM_DEV_CMDQ_EVENT_P2_WEN2_INCOMP,
	MTK_FM_DEV_CMDQ_EVENT_P2_WEN3_INCOMP,
	MTK_FM_DEV_CMDQ_EVENT_WPE_REN_INCOMP,
	MTK_FM_DEV_CMDQ_EVENT_BE0_LIM,
	MTK_FM_DEV_CMDQ_EVENT_BE1_LIM,
	MTK_FM_DEV_CMDQ_EVENT_BE2_LIM,
	MTK_FM_DEV_CMDQ_EVENT_BE3_LIM,
	MTK_FM_DEV_CMDQ_EVENT_MAX,
};

enum mtk_fm_dev_img_type {
	MTK_FM_DEV_IMG_TYPE_NORMAL = 0,
	MTK_FM_DEV_IMG_TYPE_1_4_WDMA = 1,
	MTK_FM_DEV_IMG_TYPE_1_16_WDMA = 2,
};

struct mtk_fm_reqinterval {
	/** rdma(img) request interval, range 1 to 65535(cycle) */
	__u32 img_req_interval;
	/** rdma(descriptor) request interval, range 1 to 65535(cycle) */
	__u32 desc_req_interval;
	/** rdma(point) request interval, range 1 to 65535(cycle) */
	__u32 point_req_interval;
	/** wdma(fmo) request interval, range 1 to 65535(cycle) */
	__u32 fmo_req_interval;
	/** wdma(zncc_subpix) request interval, range 1 to 65535(cycle) */
	__u32 zncc_req_interval;
};

struct mtk_fm_dev_fm_parm {
	__u32 fmdevmask;
	__u32 sysram_layout;
	__u32 img_num;
	__u32 fe_w;
	__u32 fe_h;
	__u32 fm_msk_w_s;
	__u32 fm_msk_h_s;
	__u32 fm_msk_w_tp;
	__u32 fm_msk_h_tp;
	__u32 blk_type;
	__u32 sr_type;
	__u32 start_hdr_idx_cur;
	__u32 start_hdr_idx_pre;
	__u32 zncc_th;
	char mask_path_s[200];
	char mask_path_tp[200];
	char sc_path[200];
	char sc_path_1_4[200];
	char sc_path_1_16[200];
	char dump_path[200];
	struct mtk_fm_reqinterval req_interval;
};

struct mtk_fm_dev_fm_golden_path {
	char fmo1_golden_path[200];
	__u32 fmo1_size;
	char fmo2_golden_path[200];
	__u32 fmo2_size;
};

/** fm_dev ioctl numbers **/
#define MTK_FM_DEV_MAGIC		0xfa
#define MTK_FM_DEV_IOCTL_IMPORT_FD_TO_HANDLE _IOW(MTK_FM_DEV_MAGIC, 0x00,\
	struct mtk_common_buf_handle)
#define MTK_FM_DEV_IOCTL_SET_BUF _IOW(MTK_FM_DEV_MAGIC, 0x01,\
	struct mtk_common_set_buf)
#define MTK_FM_DEV_IOCTL_SET_FM_PROP _IOW(MTK_FM_DEV_MAGIC, 0x02, \
	struct mtk_fm_dev_fm_parm)
#define MTK_FM_DEV_IOCTL_STREAMON _IOW(MTK_FM_DEV_MAGIC, 0x03,\
	__u32)
#define MTK_FM_DEV_IOCTL_STREAMOFF _IOW(MTK_FM_DEV_MAGIC, 0x04,\
	__u32)
#define MTK_FM_DEV_IOCTL_TRIGGER _IOW(MTK_FM_DEV_MAGIC, 0x05,\
	__u32)
#define MTK_FM_DEV_IOCTL_COMP _IOW(MTK_FM_DEV_MAGIC, 0x06,\
	struct mtk_fm_dev_fm_golden_path)
#define MTK_FM_DEV_IOCTL_PUT_HANDLE	_IOW(MTK_FM_DEV_MAGIC, 0x07,\
	struct mtk_common_put_handle)
#define MTK_FM_DEV_IOCTL_SET_VPU_REQ	_IOW(MTK_FM_DEV_MAGIC, 0x08,\
	struct mtk_fm_dev_vpu_req)
#define MTK_FM_DEV_IOCTL_CHECK_FLOW_DONE _IOW(MTK_FM_DEV_MAGIC, 0x09,\
	__u32)
#define MTK_FM_DEV_IOCTL_GET_STATISTICS _IOW(MTK_FM_DEV_MAGIC, 0x0a,\
	struct mtk_fm_dev_statistics)
#define MTK_FM_DEV_IOCTL_SET_LOG_LEVEL _IOW(MTK_FM_DEV_MAGIC, 0x0b,\
	__u32)
#define MTK_FM_DEV_IOCTL_DEBUG_TEST	_IOW(MTK_FM_DEV_MAGIC, 0x0c,\
	struct mtk_fm_dev_debug_test)
#define MTK_FM_DEV_IOCTL_FLUSH_BUF_CACHE _IOW(MTK_FM_DEV_MAGIC, 0x0d,\
	__u32)
#define MTK_FM_DEV_IOCTL_ONE_SHOT_DUMP _IOW(MTK_FM_DEV_MAGIC, 0x0e,\
	__u32)



/** gaze header defines **/
/** mm_gaze ioctl data structures **/
struct mtk_gaze_camera_set_prop {
	__u32 mmgazemask;
	char dump_path[200];
};

struct mtk_mm_gaze_vpu_req {
	struct mtkivp_request *process_GAZE_WDMA;
};

struct mtk_gaze_camera_config_warpa {
	__u32 coef_tbl_idx;
	__u16 user_coef_tbl[65];
	bool border_color_enable;
	__u8 border_color;
	__u8 unmapped_color;
	__u32 proc_mode;
	__u32 out_mode;
	__u32 reset;
	__u32 warpa_in_w;
	__u32 warpa_in_h;
	__u32 warpa_out_w;
	__u32 warpa_out_h;
	__u32 warpa_map_w;
	__u32 warpa_map_h;
	__u32 warpa_align;
};

struct mtk_gaze_camera_config_wdma {
	__u32 wdma_align;
};

struct mtk_gaze_camera_trigger {
	__u32 no_wait;
};

struct mtk_gaze_camera_dbg_test {
	__u32 gaze_wdma_dbg;
};

struct mtk_gaze_statistics {
	__u32 gaze_frame_cnt;
	__u32 gaze_p2_sof_cnt;
};

struct mtk_gaze_camera_sync {
	__u32 sensor_fps;
	__u32 power_flag;
	__u32 delay_cycle;
};

/** GAZE ioctl numbers **/
/* IOCTL with gaze_camera kernel */
#define MTK_GAZE_CAMERA_MAGIC		'I'
#define MTK_GAZE_CAMERA_IOCTL_IMPORT_FD_TO_HANDLE	\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x00, struct mtk_common_buf_handle)
#define MTK_GAZE_CAMERA_IOCTL_SET_GAZE_PROP		\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x01,\
			struct mtk_gaze_camera_set_prop)
#define MTK_GAZE_CAMERA_IOCTL_CONFIG_WARPA		\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x02,\
			struct mtk_gaze_camera_config_warpa)
#define MTK_GAZE_CAMERA_IOCTL_CONFIG_WDMA	\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x03,\
			struct mtk_gaze_camera_config_wdma)
#define MTK_GAZE_CAMERA_IOCTL_SET_BUF		\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x04, struct mtk_common_set_buf)
#define MTK_GAZE_CAMERA_IOCTL_STREAMON		\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x05, __u32)
#define MTK_GAZE_CAMERA_IOCTL_STREAMOFF	_IOW(MTK_GAZE_CAMERA_MAGIC, 0x06,\
		__u32)
#define MTK_GAZE_CAMERA_IOCTL_TRIGGER	_IOW(MTK_GAZE_CAMERA_MAGIC, 0x07,\
		__u32)
#define MTK_GAZE_CAMERA_IOCTL_RESET	_IOW(MTK_GAZE_CAMERA_MAGIC, 0x08,\
		__u32)
#define MTK_GAZE_CAMERA_IOCTL_PUT_HANDLE	\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x09, struct mtk_common_put_handle)
#define MTK_GAZE_GAMERA_IOCTL_SET_VPU_REQ	\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x0A,\
			struct mtk_mm_gaze_vpu_req)
#define MTK_GAZE_GAMERA_IOCTL_CHECK_FLOW_DONE	\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x0B, __u32)
#define MTK_GAZE_GAMERA_IOCTL_SET_LOG_LEVEL	\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x0C, __u32)
#define MTK_GAZE_GAMERA_IOCTL_DBG_TEST	\
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x0D, __u32)
#define MTK_GAZE_GAMERA_IOCTL_GET_STATISTICS \
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x0E, __u32)
#define MTK_GAZE_GAMERA_ENABLE_CAMERA_SYNC \
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x0F, struct mtk_gaze_camera_sync)
#define MTK_GAZE_GAMERA_DISABLE_CAMERA_SYNC \
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x10, __u32)
#define MTK_GAZE_GAMERA_IOCTL_FLUSH_BUF_CACHE \
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x11, __u32)
#define MTK_GAZE_GAMERA_IOCTL_ONE_SHOT_DUMP \
		_IOW(MTK_GAZE_CAMERA_MAGIC, 0x12, __u32)


/** kernel middle-ware buffer control enum **/
/*device index group*/
enum mtk_vr_tracking_buf_index {
	WARPA_FE_BUF_INDEX = 0,
	FM_DEV_BUF_INDEX = 1,
	SIDE_BUF_INDEX = 2,
	GAZE_BUF_INDEX = 3,
	SIDE_SHARE_BUF_INDEX = 4,
	GAZE_SHARE_BUF_INDEX = 5,
	CV_BUF_INDEX_MAX = 6,
};

/*warpa_fe*/
enum mtk_warpa_fe_buf_type {
	MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_0 = 0,

	MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_0 = 1,
	MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_1 = 2,
	MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_2 = 3,
	MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_3 = 4,

	MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_IMG = 5,
	MTK_WARPA_FE_BUF_TYPE_WARPA_DBG_OUT_VLD = 6,

	/* need to do ping pong buffer for temporal fm */
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_0_R0 = 7,
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_1_R0 = 8,
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_2_R0 = 9,
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_3_R0 = 10,

	MTK_WARPA_FE_BUF_TYPE_FE_DESC_0_R0 = 11,
	MTK_WARPA_FE_BUF_TYPE_FE_DESC_1_R0 = 12,
	MTK_WARPA_FE_BUF_TYPE_FE_DESC_2_R0 = 13,
	MTK_WARPA_FE_BUF_TYPE_FE_DESC_3_R0 = 14,

	MTK_WARPA_FE_BUF_TYPE_FE_POINT_0_R1 = 15,
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_1_R1 = 16,
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_2_R1 = 17,
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_3_R1 = 18,

	MTK_WARPA_FE_BUF_TYPE_FE_DESC_0_R1 = 19,
	MTK_WARPA_FE_BUF_TYPE_FE_DESC_1_R1 = 20,
	MTK_WARPA_FE_BUF_TYPE_FE_DESC_2_R1 = 21,
	MTK_WARPA_FE_BUF_TYPE_FE_DESC_3_R1 = 22,

	MTK_WARPA_FE_BUF_TYPE_WDMA_0_R0 = 23,
	MTK_WARPA_FE_BUF_TYPE_WDMA_1_R0 = 24,
	MTK_WARPA_FE_BUF_TYPE_WDMA_2_R0 = 25,

	MTK_WARPA_FE_BUF_TYPE_WDMA_0_R1 = 26,
	MTK_WARPA_FE_BUF_TYPE_WDMA_1_R1 = 27,
	MTK_WARPA_FE_BUF_TYPE_WDMA_2_R1 = 28,

	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R0 = 29,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R1 = 30,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R2 = 31,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R3 = 32,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R4 = 33,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R5 = 34,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R6 = 35,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R7 = 36,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R8 = 37,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R9 = 38,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R10 = 39,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R11 = 40,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R12 = 41,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R13 = 42,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R14 = 43,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELL_R15 = 44,

	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R0 = 45,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R1 = 46,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R2 = 47,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R3 = 48,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R4 = 49,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R5 = 50,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R6 = 51,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R7 = 52,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R8 = 53,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R9 = 54,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R10 = 55,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R11 = 56,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R12 = 57,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R13 = 58,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R14 = 59,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDELR_R15 = 60,

	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R0 = 61,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R1 = 62,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R2 = 63,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R3 = 64,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R4 = 65,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R5 = 66,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R6 = 67,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R7 = 68,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R8 = 69,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R9 = 70,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R10 = 71,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R11 = 72,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R12 = 73,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R13 = 74,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R14 = 75,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERL_R15 = 76,

	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R0 = 77,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R1 = 78,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R2 = 79,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R3 = 80,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R4 = 81,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R5 = 82,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R6 = 83,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R7 = 84,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R8 = 85,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R9 = 86,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R10 = 87,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R11 = 88,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R12 = 89,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R13 = 90,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R14 = 91,
	MTK_WARPA_FE_BUF_TYPE_P1_IMG_SIDERR_R15 = 92,

	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R0 = 93,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R1 = 94,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R2 = 95,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R3 = 96,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R4 = 97,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R5 = 98,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R6 = 99,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R7 = 100,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R8 = 101,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R9 = 102,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R10 = 103,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R11 = 104,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R12 = 105,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R13 = 106,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R14 = 107,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELL_R15 = 108,

	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R0 = 109,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R1 = 110,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R2 = 111,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R3 = 112,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R4 = 113,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R5 = 114,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R6 = 115,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R7 = 116,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R8 = 117,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R9 = 118,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R10 = 119,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R11 = 120,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R12 = 121,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R13 = 122,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R14 = 123,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDELR_R15 = 124,

	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R0 = 125,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R1 = 126,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R2 = 127,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R3 = 128,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R4 = 129,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R5 = 130,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R6 = 131,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R7 = 132,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R8 = 133,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R9 = 134,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R10 = 135,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R11 = 136,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R12 = 137,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R13 = 138,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R14 = 139,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERL_R15 = 140,

	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R0 = 141,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R1 = 142,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R2 = 143,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R3 = 144,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R4 = 145,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R5 = 146,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R6 = 147,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R7 = 148,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R8 = 149,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R9 = 150,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R10 = 151,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R11 = 152,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R12 = 153,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R13 = 154,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R14 = 155,
	MTK_WARPA_FE_BUF_TYPE_BE_DATINFO_SIDERR_R15 = 156,

	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R0 = 157,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R1 = 158,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R2 = 159,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R3 = 160,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R4 = 161,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R5 = 162,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R6 = 163,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R7 = 164,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R8 = 165,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R9 = 166,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R10 = 167,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R11 = 168,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R12 = 169,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R13 = 170,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R14 = 171,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELL_R15 = 172,

	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R0 = 173,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R1 = 174,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R2 = 175,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R3 = 176,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R4 = 177,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R5 = 178,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R6 = 179,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R7 = 180,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R8 = 181,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R9 = 182,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R10 = 183,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R11 = 184,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R12 = 185,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R13 = 186,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R14 = 187,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDELR_R15 = 188,

	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R0 = 189,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R1 = 190,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R2 = 191,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R3 = 192,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R4 = 193,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R5 = 194,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R6 = 195,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R7 = 196,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R8 = 197,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R9 = 198,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R10 = 199,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R11 = 200,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R12 = 201,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R13 = 202,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R14 = 203,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERL_R15 = 204,

	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R0 = 205,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R1 = 206,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R2 = 207,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R3 = 208,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R4 = 209,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R5 = 210,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R6 = 211,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R7 = 212,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R8 = 213,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R9 = 214,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R10 = 215,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R11 = 216,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R12 = 217,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R13 = 218,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R14 = 219,
	MTK_WARPA_FE_BUF_TYPE_BE_LABELIMG_SIDERR_R15 = 220,

	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R0 = 221,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R1 = 222,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R2 = 223,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R3 = 224,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R4 = 225,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R5 = 226,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R6 = 227,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R7 = 228,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R8 = 229,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R9 = 230,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R10 = 231,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R11 = 232,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R12 = 233,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R13 = 234,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R14 = 235,
	MTK_WARPA_FE_BUF_TYPE_VPU_EDGE_R15 = 236,
	MTK_WARPA_FE_BUF_TYPE_MAX,
};

/*mm gaze*/
enum mtk_gaze_camera_buf_type {
	MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_0 = 0,
	MTK_GAZE_CAMERA_BUF_TYPE_WARPA_IMG_BUF_1 = 1,
	MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_0 = 2,
	MTK_GAZE_CAMERA_BUF_TYPE_WARPA_MAP_1 = 3,
	MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_IMG = 4,
	MTK_GAZE_CAMERA_BUF_TYPE_WARPA_DBG_OUT_VLD = 5,
	MTK_GAZE_CAMERA_BUF_TYPE_WDMA = 6,
	MTK_GAZE_CAMERA_BUF_TYPE_WDMA_DUMMY = 7,
	MTK_GAZE_CAMERA_BUF_TYPE_MAX,
};

/*mtk_fm_dev*/
enum mtk_fm_dev_buf_type {
	MTK_FM_DEV_BUF_TYPE_FE_L_POINT_R0 = 0,
	MTK_FM_DEV_BUF_TYPE_FE_L_DESC_R0 = 1,
	MTK_FM_DEV_BUF_TYPE_FE_R_POINT_R0 = 2,
	MTK_FM_DEV_BUF_TYPE_FE_R_DESC_R0 = 3,
	MTK_FM_DEV_BUF_TYPE_WDMA_R0 = 4,
	MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R0 = 5,
	MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R0 = 6,

	MTK_FM_DEV_BUF_TYPE_FE_L_POINT_R1 = 7,
	MTK_FM_DEV_BUF_TYPE_FE_L_DESC_R1 = 8,
	MTK_FM_DEV_BUF_TYPE_FE_R_POINT_R1 = 9,
	MTK_FM_DEV_BUF_TYPE_FE_R_DESC_R1 = 10,
	MTK_FM_DEV_BUF_TYPE_WDMA_R1 = 11,
	MTK_FM_DEV_BUF_TYPE_1_4_WDMA_R1 = 12,
	MTK_FM_DEV_BUF_TYPE_1_16_WDMA_R1 = 13,

	MTK_FM_DEV_BUF_TYPE_FET_P = 14,
	MTK_FM_DEV_BUF_TYPE_FET_P_DUMMY = 15,
	MTK_FM_DEV_BUF_TYPE_FET_D = 16,
	MTK_FM_DEV_BUF_TYPE_FET_D_DUMMY = 17,
	MTK_FM_DEV_BUF_TYPE_1_4_FET_P = 18,
	MTK_FM_DEV_BUF_TYPE_1_4_FET_P_DUMMY = 19,
	MTK_FM_DEV_BUF_TYPE_1_4_FET_D = 20,
	MTK_FM_DEV_BUF_TYPE_1_4_FET_D_DUMMY = 21,
	MTK_FM_DEV_BUF_TYPE_1_16_FET_P = 22,
	MTK_FM_DEV_BUF_TYPE_1_16_FET_P_DUMMY = 23,
	MTK_FM_DEV_BUF_TYPE_1_16_FET_D = 24,
	MTK_FM_DEV_BUF_TYPE_1_16_FET_D_DUMMY = 25,

	MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R0 = 26,
	MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R0 = 27,
	MTK_FM_DEV_BUF_TYPE_FM_OUT_S_R1 = 28,
	MTK_FM_DEV_BUF_TYPE_FM_ZNCC_S_R1 = 29,
	MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R0 = 30,
	MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R0 = 31,
	MTK_FM_DEV_BUF_TYPE_FM_OUT_T_R1 = 32,
	MTK_FM_DEV_BUF_TYPE_FM_ZNCC_T_R1 = 33,
	MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R0 = 34,
	MTK_FM_DEV_BUF_TYPE_1_4_FM_ZNCC_T_R0 = 35,
	MTK_FM_DEV_BUF_TYPE_1_4_FM_OUT_T_R1 = 36,
	MTK_FM_DEV_BUF_TYPE_1_4_FM_ZNCC_T_R1 = 37,
	MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R0 = 38,
	MTK_FM_DEV_BUF_TYPE_1_16_FM_ZNCC_T_R0 = 39,
	MTK_FM_DEV_BUF_TYPE_1_16_FM_OUT_T_R1 = 40,
	MTK_FM_DEV_BUF_TYPE_1_16_FM_ZNCC_T_R1 = 41,
	MTK_FM_DEV_BUF_TYPE_HEAD_POSE = 42,
	MTK_FM_DEV_BUF_TYPE_HEAD_POSE_DUMMY = 43,
	MTK_FM_DEV_BUF_TYPE_MAX,
};

#endif /* MTK_VR_TRACKING_COMMON_H */
