/*
 * Copyright (c) 2020 MediaTek Inc.
 * Authors:
 *	CK Hu <ck.hu@mediatek.com>
 *
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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

#pragma once

#include <linux/types.h>
#include <linux/ioctl.h>
#include <mtk_cv_common.h>
#ifndef __KERNEL__
#define BIT(n) (1UL << (n))
#endif

/*---------------------sub device control bit--------------------------*/

#define DEV_WARPA_FE_ENABLE_MASK			BIT(0)

/*warpa_fe control bit*/
/* enable warpa -> fe */
#define WARPA_FE_CONNECT_TO_FE				BIT(0)
/* enable warpa -> wdma */
#define WARPA_FE_CONNECT_TO_WDMA			BIT(1)
/* enable warpa -> p_scaler */
#define WARPA_FE_CONNECT_TO_P_SCALE			BIT(2)
/* enable read rbfc of warpa */
#define WARPA_FE_RBFC_ENABLE_MASK			BIT(3)
/* bypass rsz0 */
#define WARPA_FE_RSZ_0_BYPASS_MASK			BIT(4)
/* bypass rsz1 */
#define WARPA_FE_RSZ_1_BYPASS_MASK			BIT(5)
/* bypass padding0 */
#define WARPA_FE_PADDING_0_BYPASS_MASK		BIT(6)
/* bypass padding1 */
#define WARPA_FE_PADDING_1_BYPASS_MASK		BIT(7)
/* bypass padding2 */
#define WARPA_FE_PADDING_2_BYPASS_MASK		BIT(8)
/* warpa_fe will trigger by p2 gce done event instead of mutex sof */
#define WARPA_FE_KICK_AFTER_P2_DONE			BIT(15)
/* if wait_triger will delay, next frame will be triggered automatically */
#define WARPA_FE_AUTO_NEXT_TRIGGER			BIT(17)

#define MTK_WARPA_FE_CAMERA_NUM 4
#define MTK_WARPA_FE_WARPMAP_NUM 2

#define MTK_WARPA_FE_BUF_MAX_OUT_BUF_CNT 2

#define MTK_WARPA_FE_BUF_MAX_OUT_BUF_NUM (MTK_WARPA_FE_BUF_MAX_OUT_BUF_CNT * MTK_WARPA_FE_CAMERA_NUM)
#define MTK_WARPA_FE_GET_BUF_NUM(base, buf_cnt, camera_id) (base + MTK_WARPA_FE_CAMERA_NUM * (buf_cnt) + (camera_id))

/*warpa_fe*/
enum mtk_warpa_fe_buf_type {
	MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MIN = 0,
	MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MAX = MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MIN + MTK_WARPA_FE_CAMERA_NUM - 1,

	MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_MIN,
	MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_MAX = MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_MIN + MTK_WARPA_FE_CAMERA_NUM * MTK_WARPA_FE_WARPMAP_NUM - 1,

	MTK_WARPA_FE_BUF_TYPE_FE_POINT_MIN,
	MTK_WARPA_FE_BUF_TYPE_FE_POINT_MAX = MTK_WARPA_FE_BUF_TYPE_FE_POINT_MIN + MTK_WARPA_FE_BUF_MAX_OUT_BUF_NUM - 1,

	MTK_WARPA_FE_BUF_TYPE_FE_DESC_MIN,
	MTK_WARPA_FE_BUF_TYPE_FE_DESC_MAX = MTK_WARPA_FE_BUF_TYPE_FE_DESC_MIN + MTK_WARPA_FE_BUF_MAX_OUT_BUF_NUM - 1,

	MTK_WARPA_FE_BUF_TYPE_WDMA_MIN,
	MTK_WARPA_FE_BUF_TYPE_WDMA_MAX = MTK_WARPA_FE_BUF_TYPE_WDMA_MIN + MTK_WARPA_FE_BUF_MAX_OUT_BUF_CNT - 1,

	MTK_WARPA_FE_BUF_TYPE_WDMA_1_MIN,
	MTK_WARPA_FE_BUF_TYPE_WDMA_1_MAX = MTK_WARPA_FE_BUF_TYPE_WDMA_1_MIN + MTK_WARPA_FE_BUF_MAX_OUT_BUF_CNT - 1,

	MTK_WARPA_FE_BUF_TYPE_WDMA_2_MIN,
	MTK_WARPA_FE_BUF_TYPE_WDMA_2_MAX = MTK_WARPA_FE_BUF_TYPE_WDMA_2_MIN + MTK_WARPA_FE_BUF_MAX_OUT_BUF_CNT - 1,

	MTK_WARPA_FE_BUF_TYPE_TEMPORARY_USE_FOR_SYNC_BUFFER,

	MTK_WARPA_FE_BUF_TYPE_MAX,
};

struct mtk_warpa_fe_set_fe_param {
	__u32 th_cr;
	__u32 th_grad;
	__u32 flat_enable;
	__u32 harris_kappa;
	__u32 cr_val_sel;
};

struct mtk_warpa_fe_set_mask {
	__u32 warpafemask;
};

enum mtk_warpa_fe_proc_mode {
	MTK_WARPA_FE_PROC_MODE_L_ONLY = 0,
	MTK_WARPA_FE_PROC_MODE_LR = 1,
	MTK_WARPA_FE_PROC_MODE_QUAD = 3
};

enum mtk_warpa_fe_out_mode {
	MTK_WARPA_FE_OUT_MODE_ALL = 0,
	MTK_WARPA_FE_OUT_MODE_DRAM = 1,
	MTK_WARPA_FE_OUT_MODE_DIRECT_LINK = 2,
};

enum mtk_warpa_fe_block_size {
	/** 0: fe 32x32 block size */
	MTK_WARPA_FE_BLK_SZ_32x32 = 0,
	/** 1: fe 16x16 block size */
	MTK_WARPA_FE_BLK_SZ_16x16 = 1,
	/** 2: fe 8x8 block size */
	MTK_WARPA_FE_BLK_SZ_8x8 = 2,
	MTK_WARPA_FE_BLK_SZ_MAX = 3,
};

enum mtk_warpa_fe_frame_merge_mode {
	/** 0: no merge, 1 frame input */
	MTK_WARPA_FE_MERGE_SINGLE_FRAME = 0,
	/** 1: frame merge, 2 frame input */
	MTK_WARPA_FE_MERGE_DOUBLE_FRAME = 1,
	/** 2: frame merge, 4 frame input */
	MTK_WARPA_FE_MERGE_QUAD_FRAME = 2,
	/** 3: dummy index for specify max index */
	MTK_WARPA_FE_MERGE_MAX = 3,
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

enum mtk_warpa_fe_wait_flag {
	MTK_WARPA_FE_TRIGGER_AND_WAIT = 0,
	MTK_WARPA_FE_TRIGGER_ONLY,
	MTK_WARPA_FE_TRIGGER_WAIT,
	MTK_WARPA_FE_TRIGGER_NO_WAIT,
};

enum mtk_warpa_fe_err_status {
	MTK_WARPA_FE_NO_ERR = 0,
	MTK_WARPA_FE_NEED_TO_RESTART,
};

struct mtk_warpa_fe_trigger {
	__u32 wait_flag;
	__u32 buf_index;
	__u32 vts;
	__u32 err_status;
};

enum mtk_warpa_fe_cmdq_event {
	MTK_WARPA_FE_CMDQ_EVENT_FRAME_DONE_FE,
	MTK_WARPA_FE_CMDQ_EVENT_FRAME_DONE_WDMA,
	MTK_WARPA_FE_CMDQ_EVENT_FRAME_DONE_WDMA_1,
	MTK_WARPA_FE_CMDQ_EVENT_FRAME_DONE_WDMA_2,
	MTK_WARPA_FE_CMDQ_EVENT_ISP_P2_DONE,
	MTK_WARPA_FE_CMDQ_EVENT_MAX,
};

/** warpa_fe ioctl numbers **/
#define MTK_WARPA_FE_MAGIC		0xae
#define MTK_WARPA_FE_IOCTL_IMPORT_FD_TO_HANDLE	_IOW(MTK_WARPA_FE_MAGIC, 0x00,\
		struct mtk_cv_common_buf_handle)
#define MTK_WARPA_FE_IOCTL_SET_CTRL_MASK		_IOW(MTK_WARPA_FE_MAGIC, 0x01,\
		struct mtk_warpa_fe_set_mask)
#define MTK_WARPA_FE_IOCTL_CONFIG_WARPA			_IOW(MTK_WARPA_FE_MAGIC, 0x02,\
		struct mtk_warpa_fe_config_warpa)
#define MTK_WARPA_FE_IOCTL_CONFIG_FE			_IOW(MTK_WARPA_FE_MAGIC, 0x03,\
		struct mtk_warpa_fe_config_fe)
#define MTK_WARPA_FE_IOCTL_CONFIG_WDMA			_IOW(MTK_WARPA_FE_MAGIC, 0x04,\
		struct mtk_warpa_fe_config_wdma)
#define MTK_WARPA_FE_IOCTL_CONFIG_RSZ			_IOW(MTK_WARPA_FE_MAGIC, 0x05,\
		struct mtk_warpa_fe_config_rsz)
#define MTK_WARPA_FE_IOCTL_CONFIG_PADDING		_IOW(MTK_WARPA_FE_MAGIC, 0x06,\
		struct mtk_warpa_fe_config_padding)
#define MTK_WARPA_FE_IOCTL_SET_BUF				_IOW(MTK_WARPA_FE_MAGIC, 0x07,\
		struct mtk_cv_common_set_buf)
#define MTK_WARPA_FE_IOCTL_STREAMON				_IOW(MTK_WARPA_FE_MAGIC, 0x08,\
		__u32)
#define MTK_WARPA_FE_IOCTL_STREAMOFF			_IOW(MTK_WARPA_FE_MAGIC, 0x09,\
		__u32)
#define MTK_WARPA_FE_IOCTL_TRIGGER				_IOWR(MTK_WARPA_FE_MAGIC, 0x0A,\
		struct mtk_warpa_fe_trigger)
#define MTK_WARPA_FE_IOCTL_RESET				_IOW(MTK_WARPA_FE_MAGIC, 0x0B,\
		__u32)
#define MTK_WARPA_FE_IOCTL_PUT_HANDLE			_IOW(MTK_WARPA_FE_MAGIC, 0x0D,\
		struct mtk_cv_common_put_handle)
#define MTK_WARPA_FE_IOCTL_SET_FE_PARAM			_IOW(MTK_WARPA_FE_MAGIC, 0x0E,\
		struct mtk_warpa_fe_set_fe_param)
#define MTK_WARPA_FE_IOCTL_SYNC_HANDLE			_IOW(MTK_WARPA_FE_MAGIC, 0x0F,\
		struct mtk_cv_common_sync_handle)
#define MTK_WARPA_FE_IOCTL_SET_WARPMAP			_IOW(MTK_WARPA_FE_MAGIC, 0x10,\
		__u32)
