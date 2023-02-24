/*
 * Copyright (c) 2020 MediaTek Inc.
 * Authors:
 *	Shan Lin <Shan.Lin@mediatek.com>
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

#define MTK_FM_DEV_FM_PARAM_ID_MAX 6

#define MTK_FM_DEV_BUF_IN_BUF_CNT MTK_FM_DEV_FM_PARAM_ID_MAX
#define MTK_FM_DEV_BUF_IN_BUF_NUM (MTK_FM_DEV_BUF_IN_BUF_CNT * 1)

enum mtk_fm_dev_buf_type {
	MTK_FM_DEV_BUF_TYPE_FM_IN_POINT_MIN = 0,
	MTK_FM_DEV_BUF_TYPE_FM_IN_POINT_MAX = MTK_FM_DEV_BUF_TYPE_FM_IN_POINT_MIN + MTK_FM_DEV_BUF_IN_BUF_NUM - 1,

	MTK_FM_DEV_BUF_TYPE_FM_IN_DESC_MIN,
	MTK_FM_DEV_BUF_TYPE_FM_IN_DESC_MAX = MTK_FM_DEV_BUF_TYPE_FM_IN_DESC_MIN + MTK_FM_DEV_BUF_IN_BUF_NUM - 1,

	MTK_FM_DEV_BUF_TYPE_FM_IN_IMG_MIN,
	MTK_FM_DEV_BUF_TYPE_FM_IN_IMG_MAX = MTK_FM_DEV_BUF_TYPE_FM_IN_IMG_MIN + MTK_FM_DEV_BUF_IN_BUF_NUM - 1,

	MTK_FM_DEV_BUF_TYPE_FM_OUT_POINT_MIN,
	MTK_FM_DEV_BUF_TYPE_FM_OUT_POINT_MAX = MTK_FM_DEV_BUF_TYPE_FM_OUT_POINT_MIN + MTK_FM_DEV_BUF_IN_BUF_NUM - 1,

	MTK_FM_DEV_BUF_TYPE_FM_OUT_DESC_MIN,
	MTK_FM_DEV_BUF_TYPE_FM_OUT_DEDC_MAX = MTK_FM_DEV_BUF_TYPE_FM_OUT_DESC_MIN + MTK_FM_DEV_BUF_IN_BUF_NUM - 1,

	MTK_FM_DEV_BUF_TYPE_FM_IN_SEARCH_CENTER_MIN,
	MTK_FM_DEV_BUF_TYPE_FM_IN_SEARCH_CENTER_MAX = MTK_FM_DEV_BUF_TYPE_FM_IN_SEARCH_CENTER_MIN + MTK_FM_DEV_BUF_IN_BUF_NUM - 1,

	MTK_FM_DEV_BUF_TYPE_MAX,
};

enum mtk_fm_dev_sr_type {
	/** 1: search range for temporal tracking */
	MTK_FM_DEV_SR_TYPE_TEMPORAL_PREDICTION = 1,
	/* 2: search range for spatial */
	MTK_FM_DEV_SR_TYPE_SPATIAL = 2,
};

enum mtk_fm_dev_blk_size_type {
	/** 0: block size 32x32 */
	MTK_FM_DEV_BLK_SIZE_TYPE_32x32 = 0,
	/** 1: block size 16x16 */
	MTK_FM_DEV_BLK_SIZE_TYPE_16x16 = 1,
	/** 2: block size 8x8 */
	MTK_FM_DEV_BLK_SIZE_TYPE_8x8 = 2,
};

enum mtk_fm_dev_wait_flag {
	MTK_FM_DEV_TRIGGER_AND_WAIT = 0,
	MTK_FM_DEV_TRIGGER_ONLY,
	MTK_FM_DEV_TRIGGER_WAIT,
};

struct mtk_fm_dev_trigger {
	__u32 wait_flag;
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
	__u32 img_num;
	__u32 fe_w;
	__u32 fe_h;
	__u32 blk_type;
	__u32 sr_type;
	__u32 img_type;
	__u32 zncc_th;
	__u32 param_id;
	struct mtk_fm_reqinterval req_interval;
};

struct fm_dev_fm_mask_param {
	int id;
	__u32 fm_mask_w;
	__u32 fm_mask_h;
	const __u32 *fm_mask;
	__u32 fm_mask_size;
};

struct fm_dev_search_center_param {
	int id;
	__u64 handle;
};

/* ioctl numbers */
#define MTK_FM_DEV_MAGIC		0xfa
#define MTK_FM_DEV_IOCTL_IMPORT_FD_TO_HANDLE _IOW(MTK_FM_DEV_MAGIC, 0x00,\
	struct mtk_cv_common_buf_handle)
#define MTK_FM_DEV_IOCTL_SET_BUF _IOW(MTK_FM_DEV_MAGIC, 0x02,\
	struct mtk_cv_common_set_buf)
#define MTK_FM_DEV_IOCTL_SET_FM_PROP _IOW(MTK_FM_DEV_MAGIC, 0x03, \
	struct mtk_fm_dev_fm_parm)
#define MTK_FM_DEV_IOCTL_STREAMON _IOW(MTK_FM_DEV_MAGIC, 0x04,\
	u32)
#define MTK_FM_DEV_IOCTL_STREAMOFF _IOW(MTK_FM_DEV_MAGIC, 0x05,\
	u32)
#define MTK_FM_DEV_IOCTL_TRIGGER _IOW(MTK_FM_DEV_MAGIC, 0x06,\
	u32)
#define MTK_FM_DEV_IOCTL_SET_FM_MASK _IOW(MTK_FM_DEV_MAGIC, 0x07,\
	struct fm_dev_fm_mask_param)
#define MTK_FM_DEV_IOCTL_PUT_HANDLE _IOW(MTK_FM_DEV_MAGIC, 0x09,\
	struct mtk_cv_common_put_handle)
