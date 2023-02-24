/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Houlong Wei <houlong.wei@mediatek.com>
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
 * @file mtk_mutex.h
 * Header of mtk_mutex.c
 */

#ifndef MTK_MUTEX_H
#define MTK_MUTEX_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

struct mtk_mutex_res;

/** @ingroup IP_group_mutex_external_enum
 * @brief Mutex Component ID, each of component ID means one HW block.\n
 * Component ID from 0 to 78 presents modules in MMCore partitions.\n
 * Component ID from 64 to 66 presents modules in GAZE partitions.\n
 * Component ID from 128 to 139 presents modules in MMCommon partitions.\n
 */
enum mtk_mutex_comp_id {
	MUTEX_COMPONENT_MMSYS = 0,
	MUTEX_COMPONENT_SLICER,
	MUTEX_COMPONENT_P2D0,
	MUTEX_COMPONENT_MDP_RDMA0,
	MUTEX_COMPONENT_MDP_RDMA1,
	MUTEX_COMPONENT_MDP_RDMA2,
	MUTEX_COMPONENT_MDP_RDMA3,
	MUTEX_COMPONENT_MDP_RDMA_PVRIC0,
	MUTEX_COMPONENT_MDP_RDMA_PVRIC1,
	MUTEX_COMPONENT_MDP_RDMA_PVRIC2,
	MUTEX_COMPONENT_MDP_RDMA_PVRIC3,
	MUTEX_COMPONENT_DISP_RDMA0,
	MUTEX_COMPONENT_DISP_RDMA1,
	MUTEX_COMPONENT_DISP_RDMA2,
	MUTEX_COMPONENT_DISP_RDMA3,
	MUTEX_COMPONENT_LHC0,
	MUTEX_COMPONENT_LHC1,
	MUTEX_COMPONENT_LHC2,
	MUTEX_COMPONENT_LHC3,
	MUTEX_COMPONENT_CROP0,
	MUTEX_COMPONENT_CROP1,
	MUTEX_COMPONENT_CROP2,
	MUTEX_COMPONENT_CROP3,
	MUTEX_COMPONENT_DISP_WDMA0,
	MUTEX_COMPONENT_DISP_WDMA1,
	MUTEX_COMPONENT_DISP_WDMA2,
	MUTEX_COMPONENT_DISP_WDMA3,
	MUTEX_COMPONENT_RSZ0,
	MUTEX_COMPONENT_RSZ1,
	MUTEX_COMPONENT_DSC0,
	MUTEX_COMPONENT_DSC1,
	MUTEX_COMPONENT_DSC2,
	MUTEX_COMPONENT_DSC3 = 32,
	MUTEX_COMPONENT_DSI0,
	MUTEX_COMPONENT_DSI1,
	MUTEX_COMPONENT_DSI2,
	MUTEX_COMPONENT_DSI3,
	MUTEX_COMPONENT_RBFC0,
	MUTEX_COMPONENT_RBFC1,
	MUTEX_COMPONENT_RBFC2,
	MUTEX_COMPONENT_RBFC3,
	MUTEX_COMPONENT_SBRC,
	MUTEX_COMPONENT_PAT_GEN_0,
	MUTEX_COMPONENT_PAT_GEN_1,
	MUTEX_COMPONENT_PAT_GEN_2,
	MUTEX_COMPONENT_PAT_GEN_3,
	MUTEX_COMPONENT_DISP_PWM0,
	MUTEX_COMPONENT_RESERVED_BIT15,
	MUTEX_COMPONENT_EMI,
	MUTEX_COMPONENT_WPEA0 = 64,
	MUTEX_COMPONENT_WDMA_GAZE,
	MUTEX_COMPONENT_RBFC_WPEA0,
	MUTEX_COMPONENT_PADDING_0 = 128,
	MUTEX_COMPONENT_PADDING_1,
	MUTEX_COMPONENT_PADDING_2,
	MUTEX_COMPONENT_WPEA1,
	MUTEX_COMPONENT_FE,
	MUTEX_COMPONENT_WDMA1,
	MUTEX_COMPONENT_WDMA2,
	MUTEX_COMPONENT_WDMA3,
	MUTEX_COMPONENT_FM,
	MUTEX_COMPONENT_RBFC_WPEA1,
	MUTEX_COMPONENT_MMCOMMON_RSZ0,
	MUTEX_COMPONENT_MMCOMMON_RSZ1,
	MUTEX_COMPONENT_ID_MAX
};

/** @ingroup IP_group_mutex_external_enum
 * @brief Mutex Start of Frame(SOF) ID which presents the source of SOF.\n
 * SOF ID from 0 to 40 can be the source of SOF for MMCore partitions.\n
 * SOF ID from 64 to 104 can be the source of SOF for GAZE partitions.\n
 * SOF ID from 128 to 168 can be the source of SOF for MMCommon partitions.\n
 * MUTEX_XXX_SOF_SINGLE means single triggerred by SW writing register.
 */
enum mtk_mutex_sof_id {
	MUTEX_MMSYS_SOF_SINGLE = 0,
	MUTEX_MMSYS_SOF_DSI0_MUTEX, /* vsync from dsi0 */
	MUTEX_MMSYS_SOF_DSI1_MUTEX,
	MUTEX_MMSYS_SOF_DSI2_MUTEX,
	MUTEX_MMSYS_SOF_DSI3_MUTEX,
	MUTEX_MMSYS_SOF_DP, /* from slicer */
	MUTEX_MMSYS_SOF_DP_DSC, /* from slicer */
	MUTEX_MMSYS_SOF_RBFC_SIDE0_0_WPEA, /* from img-side0 */
	MUTEX_MMSYS_SOF_RBFC_SIDE0_1_WPEA, /* from img-side0 */
	MUTEX_MMSYS_SOF_RBFC_SIDE1_0_WPEA, /* from img-side1 */
	MUTEX_MMSYS_SOF_RBFC_SIDE1_1_WPEA, /* from img-side1 */
	MUTEX_MMSYS_SOF_RBFC_GAZE0_WPEA, /* from img-gaze0 */
	MUTEX_MMSYS_SOF_RBFC_GAZE1_WPEA, /* from img-gaze1 */
	MUTEX_MMSYS_SOF_RBFC_SIDE0_CAM_0, /* from cam-side0 */
	MUTEX_MMSYS_SOF_RBFC_SIDE0_CAM_1, /* from cam-side0 */
	MUTEX_MMSYS_SOF_RBFC_SIDE1_CAM_0, /* from cam-side1 */
	MUTEX_MMSYS_SOF_RBFC_SIDE1_CAM_1, /* from cam-side1 */
	MUTEX_MMSYS_SOF_RBFC_GAZE0_CAM, /* from cam-gaze0 */
	MUTEX_MMSYS_SOF_RBFC_GAZE1_CAM, /* from cam-gaze1 */
	MUTEX_MMSYS_SOF_STRIP_BUFFER_CONTROL,
	MUTEX_MMSYS_SOF_IMG_SIDE0_0, /* from img-side0 */
	MUTEX_MMSYS_SOF_IMG_SIDE0_1, /* from img-side0 */
	MUTEX_MMSYS_SOF_IMG_SIDE1_0, /* from img-side1 */
	MUTEX_MMSYS_SOF_IMG_SIDE1_1, /* from img-side1 */
	MUTEX_MMSYS_SOF_IMG_GAZE0, /* from img-gaze0 */
	MUTEX_MMSYS_SOF_IMG_GAZE1, /* from img-gaze1 */
	MUTEX_MMSYS_SOF_DSI0, /* vsync from dsi0 */
	MUTEX_MMSYS_SOF_DSI1,
	MUTEX_MMSYS_SOF_DSI2,
	MUTEX_MMSYS_SOF_DSI3,
	MUTEX_MMSYS_SOF_CAMERA_VSYNC,
	MUTEX_MMSYS_SOF_RESERVED1,
	MUTEX_MMSYS_SOF_RESERVED2,
	MUTEX_MMSYS_SOF_SYNC_DELAY0,
	MUTEX_MMSYS_SOF_SYNC_DELAY1,
	MUTEX_MMSYS_SOF_SYNC_DELAY2,
	MUTEX_MMSYS_SOF_SYNC_DELAY3,
	MUTEX_MMSYS_SOF_SYNC_DELAY4,
	MUTEX_MMSYS_SOF_SYNC_DELAY5,
	MUTEX_MMSYS_SOF_SYNC_DELAY6,
	MUTEX_MMSYS_SOF_SYNC_DELAY7,
	MUTEX_GAZE_SOF_SINGLE = 64,
	MUTEX_GAZE_SOF_DSI0_MUTEX,
	MUTEX_GAZE_SOF_DSI1_MUTEX,
	MUTEX_GAZE_SOF_DSI2_MUTEX,
	MUTEX_GAZE_SOF_DSI3_MUTEX,
	MUTEX_GAZE_SOF_DP,
	MUTEX_GAZE_SOF_DP_DSC,
	MUTEX_GAZE_SOF_RBFC_SIDE0_0_WPEA,
	MUTEX_GAZE_SOF_RBFC_SIDE0_1_WPEA,
	MUTEX_GAZE_SOF_RBFC_SIDE1_0_WPEA,
	MUTEX_GAZE_SOF_RBFC_SIDE1_1_WPEA,
	MUTEX_GAZE_SOF_RBFC_GAZE0_WPEA,
	MUTEX_GAZE_SOF_RBFC_GAZE1_WPEA,
	MUTEX_GAZE_SOF_RBFC_SIDE0_CAM_0,
	MUTEX_GAZE_SOF_RBFC_SIDE0_CAM_1,
	MUTEX_GAZE_SOF_RBFC_SIDE1_CAM_0,
	MUTEX_GAZE_SOF_RBFC_SIDE1_CAM_1,
	MUTEX_GAZE_SOF_RBFC_GAZE0_CAM,
	MUTEX_GAZE_SOF_RBFC_GAZE1_CAM,
	MUTEX_GAZE_SOF_STRIP_BUFFER_CONTROL,
	MUTEX_GAZE_SOF_IMG_SIDE0_0,
	MUTEX_GAZE_SOF_IMG_SIDE0_1,
	MUTEX_GAZE_SOF_IMG_SIDE1_0,
	MUTEX_GAZE_SOF_IMG_SIDE1_1,
	MUTEX_GAZE_SOF_IMG_GAZE0,
	MUTEX_GAZE_SOF_IMG_GAZE1,
	MUTEX_GAZE_SOF_DSI0,
	MUTEX_GAZE_SOF_DSI1,
	MUTEX_GAZE_SOF_DSI2,
	MUTEX_GAZE_SOF_DSI3,
	MUTEX_GAZE_SOF_CAMERA_VSYNC,
	MUTEX_GAZE_SOF_RESERVED1,
	MUTEX_GAZE_SOF_RESERVED2,
	MUTEX_GAZE_SOF_SYNC_DELAY0,
	MUTEX_GAZE_SOF_SYNC_DELAY1,
	MUTEX_GAZE_SOF_SYNC_DELAY2,
	MUTEX_GAZE_SOF_SYNC_DELAY3,
	MUTEX_GAZE_SOF_SYNC_DELAY4,
	MUTEX_GAZE_SOF_SYNC_DELAY5,
	MUTEX_GAZE_SOF_SYNC_DELAY6,
	MUTEX_GAZE_SOF_SYNC_DELAY7,
	MUTEX_COMMON_SOF_SINGLE = 128,
	MUTEX_COMMON_SOF_DSI0_MUTEX,
	MUTEX_COMMON_SOF_DSI1_MUTEX,
	MUTEX_COMMON_SOF_DSI2_MUTEX,
	MUTEX_COMMON_SOF_DSI3_MUTEX,
	MUTEX_COMMON_SOF_DP,
	MUTEX_COMMON_SOF_DP_DSC,
	MUTEX_COMMON_SOF_RBFC_SIDE0_0_WPEA,
	MUTEX_COMMON_SOF_RBFC_SIDE0_1_WPEA,
	MUTEX_COMMON_SOF_RBFC_SIDE1_0_WPEA,
	MUTEX_COMMON_SOF_RBFC_SIDE1_1_WPEA,
	MUTEX_COMMON_SOF_RBFC_GAZE0_WPEA,
	MUTEX_COMMON_SOF_RBFC_GAZE1_WPEA,
	MUTEX_COMMON_SOF_RBFC_SIDE0_CAM_0,
	MUTEX_COMMON_SOF_RBFC_SIDE0_CAM_1,
	MUTEX_COMMON_SOF_RBFC_SIDE1_CAM_0,
	MUTEX_COMMON_SOF_RBFC_SIDE1_CAM_1,
	MUTEX_COMMON_SOF_RBFC_GAZE0_CAM,
	MUTEX_COMMON_SOF_RBFC_GAZE1_CAM,
	MUTEX_COMMON_SOF_STRIP_BUFFER_CONTROL,
	MUTEX_COMMON_SOF_IMG_SIDE0_0,
	MUTEX_COMMON_SOF_IMG_SIDE0_1,
	MUTEX_COMMON_SOF_IMG_SIDE1_0,
	MUTEX_COMMON_SOF_IMG_SIDE1_1,
	MUTEX_COMMON_SOF_IMG_GAZE0,
	MUTEX_COMMON_SOF_IMG_GAZE1,
	MUTEX_COMMON_SOF_CAM_SIDE0_0,
	MUTEX_COMMON_SOF_CAM_SIDE0_1,
	MUTEX_COMMON_SOF_CAM_SIDE1_0,
	MUTEX_COMMON_SOF_CAM_SIDE1_1,
	MUTEX_COMMON_SOF_DSI0,
	MUTEX_COMMON_SOF_RESERVED1,
	MUTEX_COMMON_SOF_RESERVED2,
	MUTEX_COMMON_SOF_SYNC_DELAY0,
	MUTEX_COMMON_SOF_SYNC_DELAY1,
	MUTEX_COMMON_SOF_SYNC_DELAY2,
	MUTEX_COMMON_SOF_SYNC_DELAY3,
	MUTEX_COMMON_SOF_SYNC_DELAY4,
	MUTEX_COMMON_SOF_SYNC_DELAY5,
	MUTEX_COMMON_SOF_SYNC_DELAY6,
	MUTEX_COMMON_SOF_SYNC_DELAY7,
	MUTEX_SOF_ID_MAX
};

/** @ingroup IP_group_mutex_external_enum
 * @brief Mutex Error ID which means different error type\n
 * that can be detected by Mutex HW.\n
 * Error ID from 0 to 26 means the error type for MMCore partitions.\n
 * Error ID from 32 to 58 means the error type for GAZE partitions.\n
 * Error ID from 64 to 90 means the error type for MMCommon partitions.
 */
enum mtk_mutex_error_id {
	MUTEX_MMSYS_DSI0_UNDERFLOW = 0,
	MUTEX_MMSYS_DSI1_UNDERFLOW,
	MUTEX_MMSYS_DSI2_UNDERFLOW,
	MUTEX_MMSYS_DSI3_UNDERFLOW,
	MUTEX_MMSYS_DP_MUTE,
	MUTEX_MMSYS_DP_DSC_MUTE,
	MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_0, /* from mmsys core */
	MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_1, /* from mmsys core */
	MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_2, /* from mmsys core */
	MUTEX_MMSYS_STRIP_BUFFER_R_UNFINISH_3, /* from mmsys core */
	MUTEX_MMSYS_STRIP_BUFFER_W_UNFINISH,
	MUTEX_MMSYS_RBFC_SIDE0_CAM_0_RW_UNFINISH, /* from img/cam-side0 */
	MUTEX_MMSYS_RBFC_SIDE0_CAM_1_RW_UNFINISH, /* from img/cam-side0 */
	MUTEX_MMSYS_RBFC_SIDE1_CAM_0_RW_UNFINISH, /* from img/cam-side1 */
	MUTEX_MMSYS_RBFC_SIDE1_CAM_1_RW_UNFINISH, /* from img/cam-side1 */
	MUTEX_MMSYS_RBFC_GAZE0_CAM_RW_UNFINISH, /* from img/cam-gaze0 */
	MUTEX_MMSYS_RBFC_GAZE1_CAM_RW_UNFINISH, /* from img/cam-gaze1 */
	MUTEX_MMSYS_RBFC_SIDE0_IMG_YONLY_W_UNFINISH,
	MUTEX_MMSYS_RBFC_SIDE1_IMG_YONLY_W_UNFINISH,
	MUTEX_MMSYS_RBFC_SIDE_IMG_YONLY_R_UNFINISH,  /* from mmsys_common */
	MUTEX_MMSYS_RBFC_GAZE0_IMG_YONLY_W_UNFINISH,
	MUTEX_MMSYS_RBFC_GAZE1_IMG_YONLY_W_UNFINISH,
	MUTEX_MMSYS_RBFC_GAZE_IMG_YONLY_R_UNFINISH,  /* from mmsys gaze */
	MUTEX_MMSYS_RBFC_SIDE0_IMG_YONLY_W_FULL,
	MUTEX_MMSYS_RBFC_SIDE1_IMG_YONLY_W_FULL,
	MUTEX_MMSYS_RBFC_GAZE0_IMG_YONLY_W_FULL,
	MUTEX_MMSYS_RBFC_GAZE1_IMG_YONLY_W_FULL,
	MUTEX_GAZE_DSI0_UNDERFLOW = 32,
	MUTEX_GAZE_DSI1_UNDERFLOW,
	MUTEX_GAZE_DSI2_UNDERFLOW,
	MUTEX_GAZE_DSI3_UNDERFLOW,
	MUTEX_GAZE_DP_MUTE,
	MUTEX_GAZE_DP_DSC_MUTE,
	MUTEX_GAZE_STRIP_BUFFER_R_UNFINISH_0, /* from mmsys core */
	MUTEX_GAZE_STRIP_BUFFER_R_UNFINISH_1, /* from mmsys core */
	MUTEX_GAZE_STRIP_BUFFER_R_UNFINISH_2, /* from mmsys core */
	MUTEX_GAZE_STRIP_BUFFER_R_UNFINISH_3, /* from mmsys core */
	MUTEX_GAZE_STRIP_BUFFER_W_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE0_CAM_0_RW_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE0_CAM_1_RW_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE1_CAM_0_RW_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE1_CAM_1_RW_UNFINISH,
	MUTEX_GAZE_RBFC_GAZE0_CAM_RW_UNFINISH,
	MUTEX_GAZE_RBFC_GAZE1_CAM_RW_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE0_IMG_YONLY_W_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE1_IMG_YONLY_W_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE_IMG_YONLY_R_UNFINISH,
	MUTEX_GAZE_RBFC_GAZE0_IMG_YONLY_W_UNFINISH,
	MUTEX_GAZE_RBFC_GAZE1_IMG_YONLY_W_UNFINISH,
	MUTEX_GAZE_RBFC_GAZE_IMG_YONLY_R_UNFINISH,
	MUTEX_GAZE_RBFC_SIDE0_IMG_YONLY_W_FULL,
	MUTEX_GAZE_RBFC_SIDE1_IMG_YONLY_W_FULL,
	MUTEX_GAZE_RBFC_GAZE0_IMG_YONLY_W_FULL,
	MUTEX_GAZE_RBFC_GAZE1_IMG_YONLY_W_FULL,
	MUTEX_COMMON_DSI0_UNDERFLOW = 64,
	MUTEX_COMMON_DSI1_UNDERFLOW,
	MUTEX_COMMON_DSI2_UNDERFLOW,
	MUTEX_COMMON_DSI3_UNDERFLOW,
	MUTEX_COMMON_DP_MUTE,
	MUTEX_COMMON_DP_DSC_MUTE,
	MUTEX_COMMON_STRIP_BUFFER_R_UNFINISH_0, /* from mmsys core */
	MUTEX_COMMON_STRIP_BUFFER_R_UNFINISH_1, /* from mmsys core */
	MUTEX_COMMON_STRIP_BUFFER_R_UNFINISH_2, /* from mmsys core */
	MUTEX_COMMON_STRIP_BUFFER_R_UNFINISH_3, /* from mmsys core */
	MUTEX_COMMON_STRIP_BUFFER_W_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE0_CAM_0_RW_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE0_CAM_1_RW_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE1_CAM_0_RW_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE1_CAM_1_RW_UNFINISH,
	MUTEX_COMMON_RBFC_GAZE0_CAM_RW_UNFINISH,
	MUTEX_COMMON_RBFC_GAZE1_CAM_RW_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE0_IMG_YONLY_W_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE1_IMG_YONLY_W_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE_IMG_YONLY_R_UNFINISH,
	MUTEX_COMMON_RBFC_GAZE0_IMG_YONLY_W_UNFINISH,
	MUTEX_COMMON_RBFC_GAZE1_IMG_YONLY_W_UNFINISH,
	MUTEX_COMMON_RBFC_GAZE_IMG_YONLY_R_UNFINISH,
	MUTEX_COMMON_RBFC_SIDE0_IMG_YONLY_W_FULL,
	MUTEX_COMMON_RBFC_SIDE1_IMG_YONLY_W_FULL,
	MUTEX_COMMON_RBFC_GAZE0_IMG_YONLY_W_FULL,
	MUTEX_COMMON_RBFC_GAZE1_IMG_YONLY_W_FULL,
	MUTEX_ERROR_ID_MAX
};

/** @ingroup IP_group_mutex_external_enum
 * @brief Mutex External Signal Index.\n
 * Mutex can send external signal while it detects error.\n
 * External Signal ID from 0 to 8 can be used for MMCore partitions.\n
 * External Signal ID 32 can be used for GAZE partitions.
 * External Signal ID 64 can be used for MMCommon partitions.\n
 */
enum mtk_mutex_ext_signal_index {
	MMCORE_DSI_0_MUTE_EXT_SIGNAL = 0,
	MMCORE_DSI_1_MUTE_EXT_SIGNAL,
	MMCORE_DSI_2_MUTE_EXT_SIGNAL,
	MMCORE_DSI_3_MUTE_EXT_SIGNAL,
	MMCORE_DSC0_MUTE_0_EXT_SIGNAL,
	MMCORE_DSC0_MUTE_1_EXT_SIGNAL,
	MMCORE_DSC1_MUTE_0_EXT_SIGNAL,
	MMCORE_DSC1_MUTE_1_EXT_SIGNAL,
	MMCORE_SBRC_BYPASS_EXT_SIGNAL,
	GAZE_RBFC_GAZE_BYPASS_EXT_SIGNAL = 32,
	MMCOMMON_IMG_SIDE_BYPASS_EXT_SIGNAL = 64,
	MUTEX_EXT_SIGNAL_MAX,
};

/** @ingroup IP_group_mutex_external_enum
 * @brief predefined MM Core Mutex Index.\n
 * Each subpath in MMCore can get mutex SW resource with predefined index\n
 * by calling mtk_mutex_get API.
 */
enum mtk_mmcore_mutex_index {
	/* to do: assign mutex by scenario */
	MMCORE_MUTEX_MAX_IDX = 16,
};

/** @ingroup IP_group_mutex_external_enum
 * @brief MM Core Delay Generator Index.\n
 * Each subpath in MMCore can get mutex SW delay generator resource\n
 * with predefined index by calling mtk_mutex_delay_get API.
 */
enum mtk_mmcore_delay_gen_index {
	/* to do: assign mutex by scenario */
	MMCORE_MUTEX_DELAY_MAX_IDX = 8,
};

/** @ingroup IP_group_mutex_external_enum
 * @brief predefined MM Common Mutex Index.\n
 * Each subpath in MMCommon can get mutex SW resource with predefined index\n
 * by calling mtk_mutex_get API.
 */
enum mtk_mmcommon_mutex_index {
	/* to do: assign mutex by scenario */
	CV_MUTEX_MAX_IDX = 16,
};

/** @ingroup IP_group_mutex_external_enum
 * @brief predefined GAZE Mutex Index.\n
 * Each subpath in GAZE can get mutex SW resource with predefined index\n
 * by calling mtk_mutex_get API.
 */
enum mtk_gaze_mutex_index {
	/* to do: assign mutex by scenario */
	GAZE_MUTEX_MAX_IDX = 16,
};

/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex macro definition to check if sof_id is valid or not.
 */
#define is_invalid_sof(sof_id) \
		      ((sof_id >= MUTEX_SOF_ID_MAX) || \
		      ((sof_id < MUTEX_GAZE_SOF_SINGLE) && \
		       (sof_id > MUTEX_MMSYS_SOF_SYNC_DELAY7)) || \
		      ((sof_id < MUTEX_COMMON_SOF_SINGLE) && \
		       (sof_id > MUTEX_GAZE_SOF_SYNC_DELAY7)))

/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex macro definition to check if external signal id is valid or not.
 */
#define is_invalid_ext_signal(id) \
			     ((id >= MUTEX_EXT_SIGNAL_MAX) || \
			     ((id < GAZE_RBFC_GAZE_BYPASS_EXT_SIGNAL) && \
			      (id > MMCORE_SBRC_BYPASS_EXT_SIGNAL)) || \
			     ((id < MMCOMMON_IMG_SIDE_BYPASS_EXT_SIGNAL) && \
			      (id > GAZE_RBFC_GAZE_BYPASS_EXT_SIGNAL)))

/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex macro definition to check if error id is valid or not.
 */
#define is_invalid_err(id) \
		      ((id >= MUTEX_ERROR_ID_MAX) || \
		      ((id < MUTEX_GAZE_DSI0_UNDERFLOW) && \
		       (id > MUTEX_MMSYS_RBFC_GAZE1_IMG_YONLY_W_FULL)) || \
		      ((id < MUTEX_COMMON_DSI0_UNDERFLOW) && \
		       (id > MUTEX_GAZE_RBFC_GAZE1_IMG_YONLY_W_FULL)))

/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex macro definition to check if component id is valid or not.
 */
#define is_invalid_comp(id) \
		       ((id >= MUTEX_COMPONENT_ID_MAX) || \
		       ((id < MUTEX_COMPONENT_DSC3) && \
			(id > MUTEX_COMPONENT_DSC2)) || \
		       ((id < MUTEX_COMPONENT_WPEA0) && \
			(id > MUTEX_COMPONENT_EMI)) || \
		       ((id < MUTEX_COMPONENT_PADDING_0) && \
			(id > MUTEX_COMPONENT_RBFC_WPEA0)))

/** @ingroup IP_group_mutex_external_enum
 * @brief Mutex SOF timing.\n
 * Mutex will send SOF signal to its components when it detects the specified\n
 * timing from source.
 */
enum mtk_mutex_sof_timing {
	MUTEX_SOF_VSYNC_FALLING,
	MUTEX_SOF_VSYNC_RISING,
	MUTEX_SOF_VDE_RISING,
	MUTEX_SOF_VDE_FALLING
};

/** @ingroup IP_group_mutex_external_enum
 * @brief Mutex EOF timing.\n
 * Mutex will send EOF signal to its components when it detects the specified\n
 * timing from source.\n
 * Only the corresponding mask bit of register DISP_MUTEX_EOF_MASK_LSB/MSB is\n
 * set to 1, the component can receive EOF signal from Mutex.
 */
enum mtk_mutex_eof_timing {
	MUTEX_EOF_VDE_FALLING,
	MUTEX_EOF_VSYNC_FALLING,
	MUTEX_EOF_VSYNC_RISING,
	MUTEX_EOF_VDE_RISING
};

/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex macro definition to check if sof timing is valid or not.
 */
#define is_invalid_sof_timing(t) \
		(t < MUTEX_SOF_VSYNC_FALLING || t > MUTEX_SOF_VDE_FALLING)

/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex macro definition to check if eof timing is valid or not.
 */
#define is_invalid_eof_timing(t) \
		(t < MUTEX_EOF_VDE_FALLING || t > MUTEX_EOF_VDE_RISING)

/** @ingroup IP_group_mutex_external_enum
 * @brief Mutex timer timeout event selection.\n
 */
enum mtk_mutex_timeout_event {
	MUTEX_TO_EVENT_SRC_EDGE = 0,
	MUTEX_TO_EVENT_REF_EDGE,
	MUTEX_TO_EVENT_SRC_OVERFLOW,
	MUTEX_TO_EVENT_REF_OVERFLOW
};

/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex macro definition to check if timer timeout event selection
 * is valid or not.
 */
#define is_invalid_timeout_event(e) \
		(e < MUTEX_TO_EVENT_SRC_EDGE || e > MUTEX_TO_EVENT_REF_OVERFLOW)

/** @ingroup IP_group_mutex_internal_enum
 * @brief predefined mutex IRQ status index.\n
 */
enum mtk_mutex_irq_sta_index {
	MUTEX_IRQ_STA_0 = 0,
	MUTEX_IRQ_STA_1,
	MUTEX_IRQ_STA_2,
	MUTEX_IRQ_STA_3,
	MUTEX_IRQ_STA_4,
	MUTEX_IRQ_STA_5,
	MUTEX_IRQ_STA_6,
	MUTEX_IRQ_STA_7,
	MUTEX_IRQ_STA_8,
	MUTEX_IRQ_STA_9,
	MUTEX_IRQ_STA_10,
	MUTEX_IRQ_STA_NR,
};

/** @ingroup IP_group_mutex_internal_struct
 * @brief Mutex Timer Debugger Data Structure.
 */
struct mtk_mutex_timer_status {
	u32 sw_reset:1;
	u32 enable:1;
	u32 loop:1;
	u32 src_edge:1;
	u32 ref_edge:1;
	u32 event_sel:2;
	u32 reserved:1;
	u32 src_sel:8;
	u32 ref_sel:8;
	u32 state:8;
	u32 src_timeout;
	u32 ref_timeout;
	u32 src_time;
	u32 ref_time;
	u32 mm_clock;
};

struct mtk_mutex_res *mtk_mutex_get(const struct device *dev, unsigned int id);
int mtk_mutex_res2id(struct mtk_mutex_res *mutex_res);
int mtk_mutex_put(struct mtk_mutex_res *mutex_res);
struct mtk_mutex_res *mtk_mutex_delay_get(const struct device *dev,
					  unsigned int id);
int mtk_mutex_delay_put(struct mtk_mutex_res *mutex_delay_res);
int mtk_mutex_power_on(const struct mtk_mutex_res *mutex_res);
int mtk_mutex_power_off(const struct mtk_mutex_res *mutex_res);
int mtk_mutex_select_sof_eof(const struct mtk_mutex_res *mutex_res,
			enum mtk_mutex_sof_id sofid,
			enum mtk_mutex_sof_id eofid,
			enum mtk_mutex_sof_timing sof_timing,
			enum mtk_mutex_eof_timing eof_timing,
			struct cmdq_pkt **handle);
int mtk_mutex_select_sof(const struct mtk_mutex_res *mutex_res,
			 enum mtk_mutex_sof_id sofid,
			 struct cmdq_pkt **handle, bool only_vsync);
int mtk_mutex_add_comp(const struct mtk_mutex_res *mutex_res,
		       enum mtk_mutex_comp_id compid,
		       struct cmdq_pkt **handle);
int mtk_mutex_remove_comp(const struct mtk_mutex_res *mutex_res,
			  enum mtk_mutex_comp_id compid,
			  struct cmdq_pkt **handle);
int mtk_mutex_set_comp_eof_mask(const struct device *dev,
		       enum mtk_mutex_comp_id compid, bool receive_eof,
		       struct cmdq_pkt **handle);
int mtk_mutex_enable(const struct mtk_mutex_res *mutex_res,
		     struct cmdq_pkt **handle);
int mtk_mutex_disable_ex(const struct mtk_mutex_res *mutex_res,
		      struct cmdq_pkt **handle, bool reset);
int mtk_mutex_disable(const struct mtk_mutex_res *mutex_res,
		      struct cmdq_pkt **handle);
int mtk_mutex_select_timer_sof(const struct mtk_mutex_res *mutex_res,
			       enum mtk_mutex_sof_id src_sofid,
			       enum mtk_mutex_sof_id ref_sofid,
			       struct cmdq_pkt **handle);
int mtk_mutex_select_timer_sof_timing(const struct mtk_mutex_res *mutex_res,
			       bool src_neg, bool ref_neg,
			       struct cmdq_pkt **handle);
int mtk_mutex_set_timer_us(const struct mtk_mutex_res *mutex_res, u32 src_time,
			   u32 ref_time, struct cmdq_pkt **handle);
int mtk_mutex_timer_enable_ex(const struct mtk_mutex_res *mutex_res, bool loop,
			   enum mtk_mutex_timeout_event event,
			   struct cmdq_pkt **handle);
int mtk_mutex_timer_enable(const struct mtk_mutex_res *mutex_res, bool loop,
			   struct cmdq_pkt **handle);
int mtk_mutex_timer_get_status(const struct mtk_mutex_res *mutex_res,
				struct mtk_mutex_timer_status *status);
int mtk_mutex_timer_disable(const struct mtk_mutex_res *mutex_res,
			    struct cmdq_pkt **handle);
int mtk_mutex_select_timer_sof_w_gce5(const struct mtk_mutex_res *mutex_res,
					enum mtk_mutex_sof_id src_sofid,
					enum mtk_mutex_sof_id ref_sofid,
					u16 work_reg_idx_0,
					u16 work_reg_idx_1,
					struct cmdq_pkt **handle);
int mtk_mutex_set_timer_us_w_gce5(const struct mtk_mutex_res *mutex_res,
				u32 src_time, u32 ref_time,
				u16 work_reg_idx_0, u16 work_reg_idx_1,
				struct cmdq_pkt **handle);
int mtk_mutex_timer_enable_ex_w_gce5(const struct mtk_mutex_res *mutex_res,
				bool loop, enum mtk_mutex_timeout_event event,
				u16 work_reg_idx_0, u16 work_reg_idx_1,
				struct cmdq_pkt **handle);
int mtk_mutex_timer_disable_w_gce5(const struct mtk_mutex_res *mutex_res,
				u16 work_reg_idx_0, u16 work_reg_idx_1,
				struct cmdq_pkt **handle);
int mtk_mutex_select_delay_sof(const struct mtk_mutex_res *mutex_delay_res,
			       enum mtk_mutex_sof_id sof_id,
			       struct cmdq_pkt **handle);
int mtk_mutex_set_delay_us(const struct mtk_mutex_res *mutex_delay_res,
			   u32 delay_time, struct cmdq_pkt **handle);
int mtk_mutex_set_delay_cycle(const struct mtk_mutex_res *mutex_delay_res,
			   u32 clock, u32 cycles, struct cmdq_pkt **handle);
int mtk_mutex_get_delay_sof(const struct mtk_mutex_res *mutex_delay_res);
int mtk_mutex_delay_enable(const struct mtk_mutex_res *mutex_delay_res,
			   struct cmdq_pkt **handle);
int mtk_mutex_delay_disable(const struct mtk_mutex_res *mutex_delay_res,
			    struct cmdq_pkt **handle);
int mtk_mutex_error_monitor_enable(const struct mtk_mutex_res *mutex_res,
				   struct cmdq_pkt **handle,
				   bool auto_off);
int mtk_mutex_error_monitor_disable(const struct mtk_mutex_res *mutex_res,
				    struct cmdq_pkt **handle);
int mtk_mutex_add_monitor(const struct mtk_mutex_res *mutex_res,
			  enum mtk_mutex_error_id error_id,
			  struct cmdq_pkt **handle);
int mtk_mutex_remove_monitor(const struct mtk_mutex_res *mutex_res,
			     enum mtk_mutex_error_id error_id,
			     struct cmdq_pkt **handle);
int mtk_mutex_error_clear(const struct mtk_mutex_res *mutex_res,
			  struct cmdq_pkt **handle);
int mtk_mutex_add_to_ext_signal(const struct mtk_mutex_res *mutex_res,
				enum mtk_mutex_ext_signal_index ext_signal_id,
				struct cmdq_pkt **handle);
int mtk_mutex_remove_from_ext_signal(const struct mtk_mutex_res *mutex_res,
				     enum mtk_mutex_ext_signal_index
				     ext_signal_id, struct cmdq_pkt **handle);
int mtk_mutex_debug_enable(const struct device *dev, struct cmdq_pkt **handle);
int mtk_mutex_debug_disable(const struct device *dev, struct cmdq_pkt **handle);
int mtk_mutex_debug_get_sent_sof_count(const struct device *dev,
				       enum mtk_mutex_comp_id compid);
int mtk_mutex_register_cb(struct device *dev, enum mtk_mutex_irq_sta_index idx,
			  mtk_mmsys_cb cb, u32 status, void *priv_data);

#endif
