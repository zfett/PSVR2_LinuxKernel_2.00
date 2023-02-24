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

#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/dma-buf.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/gcd.h>
#include <linux/kthread.h>
#include <mtk_warpa_fe.h>
#include <mtk_cv_common.h>
#include <soc/mediatek/mtk_fe.h>
#include <soc/mediatek/mtk_mmsys_cmmn_top.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include <soc/mediatek/mtk_mutex.h>
#include <soc/mediatek/mtk_rbfc.h>
#include <soc/mediatek/mtk_warpa.h>
#include <soc/mediatek/mtk_wdma.h>
#include <soc/mediatek/mtk_resizer.h>
#include <soc/mediatek/mtk_padding.h>
#include <soc/mediatek/mtk_ts.h>

#define WARPA_FE_TRIGGER_THREAD_PRIORITY (96)
#define WARPA_FE_TIMEOUT (HZ*4) /* 4 sec */
#define TRIGGER_THREAD_STOP 2
#define TRIGGER_THREAD_WAIT 3

#define CHK_ERR(func) \
	do { \
		int ret = func; \
		if (ret) { \
			dev_err(warpa_fe->dev, "%s(line:%d) fail. ret:%d\n", __func__, __LINE__, ret); \
			return ret; \
		} \
	} while (0)
#define CHK_RANGE(x, a) ((x) >= a##_MIN && (x) <= a##_MAX)
#define GET_CAMERA_ID(buf_type) (buf_type % MTK_WARPA_FE_CAMERA_NUM)
#define GET_WARPA_ID(buf_type) (GET_CAMERA_ID(buf_type) / MTK_WARPA_FE_CAMERA_PER_WARPA_NUM)

#define MAX_WPE_DISTORT 20

struct mtk_warpa_fe {
	struct device *dev;
	struct device *dev_mmsys_cmmn_top;
	struct device *dev_mmsys_core_top;
	struct device *dev_sysram;
	struct device *dev_mutex;
	struct device *dev_rbfc;
	struct device *dev_warpa;
	struct device *dev_fe;
	struct cmdq_client *cmdq_clients;
	struct cmdq_pkt *cmdq_handles;
	struct device *dev_wdma0, *dev_wdma1, *dev_wdma2;
	struct device *dev_padding0, *dev_padding1, *dev_padding2;
	struct device *dev_p_rsz0, *dev_p_rsz1;
	struct device *dev_ts;

	dev_t devt;
	struct cdev chrdev;
	struct class *dev_class;
	struct mtk_mutex_res *mutex;
	struct mtk_mutex_res *mutex_warpa_delay;
	struct mtk_cv_common_buf *buf[MTK_WARPA_FE_BUF_TYPE_MAX];
	u32 warpa_in_w;
	u32 warpa_in_h;
	u32 warpa_out_w;
	u32 warpa_out_h;
	u32 warpa_map_w;
	u32 warpa_map_h;
	struct mtk_warpa_coef_tbl coef_tbl;
	struct mtk_warpa_border_color border_color;
	u32 proc_mode;
	u32 warpa_out_mode;
	u32 rbfc_chassing_mode;
	u32 reset;
	u32 fe_w;
	u32 fe_h;
	u32 blk_sz;
	u32 fe_merge_mode;
	u32 fe_flat_enable;
	u32 fe_harris_kappa;
	u32 fe_th_g;
	u32 fe_th_c;
	u32 fe_cr_val_sel;
	u32 warpa_align;
	u32 fe_align;
	u32 wdma_align;
	u32 padding_val_0;
	u32 padding_val_1;
	u32 padding_val_2;
	u32 rsz_0_out_w;
	u32 rsz_0_out_h;
	u32 rsz_1_out_w;
	u32 rsz_1_out_h;
	u32 img_num;
	u32 sensor_fps;

	wait_queue_head_t trigger_wait_queue;
	struct cmdq_client *cmdq_client;
	u32 cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_MAX];

	struct task_struct *trigger_task;

	struct cmdq_pkt *cmdq_pkt;
	int cmdq_err;
	u32 latest_vts;
	u32 current_buf_index;
	u32 trigger_buf_index;

	/* conctrl bit for path config */
	u32 warpafemask;
	u32 warpmap_cnt;
};

static int (*s_mtk_warpa_set_buf_in[MTK_WARPA_FE_CAMERA_NUM])(const struct device *, struct cmdq_pkt *, dma_addr_t, u32) = {
	mtk_warpa_set_buf_in_no0,
	mtk_warpa_set_buf_in_no1,
	mtk_warpa_set_buf_in_no2,
	mtk_warpa_set_buf_in_no3,
};

static int (*s_mtk_warpa_set_buf_map[MTK_WARPA_FE_CAMERA_NUM])(const struct device*, struct cmdq_pkt*, dma_addr_t, u32) = {
	mtk_warpa_set_buf_map_no0,
	mtk_warpa_set_buf_map_no1,
	mtk_warpa_set_buf_map_no2,
	mtk_warpa_set_buf_map_no3,
};

static inline void sync_for_device(struct device *dev, struct mtk_cv_common_buf *buf)
{
	if (!buf->is_sysram) {
		dma_sync_sg_for_device(dev, buf->sg->sgl, buf->sg->nents, DMA_TO_DEVICE);
	}
}

static int mtk_warpa_fe_config_rbfc(struct mtk_warpa_fe *warpa_fe)
{
	u32 r_th[1] = {0};
	u32 rbfc_act_nr[2] = { 1, 1};
	u32 rbfc_in_pitch;
	u32 rbfc_w, rbfc_h;
	int warpa_idx;

	CHK_ERR(mtk_rbfc_set_plane_num(warpa_fe->dev_rbfc, 1));

	switch (warpa_fe->img_num) {
	case 1:
		rbfc_act_nr[0] = 0x1;
		rbfc_w = warpa_fe->warpa_in_w;
		rbfc_h = warpa_fe->warpa_in_h;
		break;
	case 2:
		rbfc_act_nr[0] = 0x3;
		rbfc_w = warpa_fe->warpa_in_w * 2;
		rbfc_h = warpa_fe->warpa_in_h;
		break;
	case 4:
		rbfc_act_nr[0] = 0xf;
		rbfc_w = warpa_fe->warpa_in_w * 4;
		rbfc_h = warpa_fe->warpa_in_h;
		break;
	default:
		pr_err("err in img_num:%d\n", warpa_fe->img_num);
		return -EINVAL;
	}

	r_th[0] = (roundup(rbfc_h * MAX_WPE_DISTORT, 100) / 100) + 8;

	rbfc_in_pitch = roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) *
		warpa_fe->img_num;
	CHK_ERR(mtk_rbfc_set_active_num(warpa_fe->dev_rbfc, NULL, rbfc_act_nr));
	CHK_ERR(mtk_rbfc_set_region_multi(warpa_fe->dev_rbfc, NULL, 0, rbfc_w, rbfc_h));

	warpa_idx = MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MIN;
	if (warpa_fe->buf[warpa_idx]->is_sysram) {
		CHK_ERR(mtk_rbfc_set_target(warpa_fe->dev_rbfc, NULL, MTK_RBFC_TO_SYSRAM));
	} else {
		CHK_ERR(mtk_rbfc_set_target(warpa_fe->dev_rbfc, NULL, MTK_RBFC_TO_DRAM));
	}

	if (warpa_fe->buf[warpa_idx]->pitch * warpa_fe->warpa_in_h > warpa_fe->buf[warpa_idx]->dma_buf->size) {
		/* RBFC uses Chassing Mode when buffer size is smaller than Full Frame. */
		warpa_fe->rbfc_chassing_mode = 1;
	} else {
		warpa_fe->rbfc_chassing_mode = 0;
	}

	if (warpa_fe->rbfc_chassing_mode == 1) {
		CHK_ERR(mtk_rbfc_set_ring_buf_multi(warpa_fe->dev_rbfc, NULL, 0,
						    warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MIN]->dma_addr,
						    rbfc_in_pitch,
						    warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MIN]->dma_buf->size /
						    warpa_fe->buf[MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MIN]->pitch));
	} else {
			CHK_ERR(mtk_rbfc_set_ring_buf_multi(warpa_fe->dev_rbfc, NULL, 0,
							    0x0, rbfc_in_pitch, warpa_fe->warpa_in_h));
	}

	if (warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) {
		CHK_ERR(mtk_rbfc_set_read_thd(warpa_fe->dev_rbfc, NULL, r_th));
		CHK_ERR(mtk_rbfc_start_mode(warpa_fe->dev_rbfc, NULL, MTK_RBFC_NORMAL_MODE));
		dev_dbg(warpa_fe->dev, "enable rbfc\n");
	} else {
		CHK_ERR(mtk_rbfc_start_mode(warpa_fe->dev_rbfc, NULL, MTK_RBFC_DISABLE_MODE));
		dev_dbg(warpa_fe->dev, "disable rbfc\n");
	}
	return 0;
}

static int mtk_warpa_fe_config_warpa(struct mtk_warpa_fe *warpa_fe)
{
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_in_w %d\n", warpa_fe->warpa_in_w);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_in_h %d\n", warpa_fe->warpa_in_h);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_out_w %d\n", warpa_fe->warpa_out_w);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_out_h %d\n", warpa_fe->warpa_out_h);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_map_w %d\n", warpa_fe->warpa_map_w);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_map_h %d\n", warpa_fe->warpa_map_h);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_align %d\n", warpa_fe->warpa_align);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_proc_mode %d\n", warpa_fe->proc_mode);
	dev_dbg(warpa_fe->dev, "warpa_fe->warpa_out_mode %d\n", warpa_fe->warpa_out_mode);
	dev_dbg(warpa_fe->dev, "warpa_fe->blk_sz %d\n", warpa_fe->blk_sz);

	CHK_ERR(mtk_warpa_set_region_in(warpa_fe->dev_warpa, warpa_fe->warpa_in_w, warpa_fe->warpa_in_h));
	CHK_ERR(mtk_warpa_set_region_out(warpa_fe->dev_warpa, warpa_fe->warpa_out_w, warpa_fe->warpa_out_h));
	CHK_ERR(mtk_warpa_set_region_map(warpa_fe->dev_warpa, warpa_fe->warpa_map_w, warpa_fe->warpa_map_h));
	CHK_ERR(mtk_warpa_set_coef_tbl(warpa_fe->dev_warpa, NULL, &warpa_fe->coef_tbl));
	CHK_ERR(mtk_warpa_set_border_color(warpa_fe->dev_warpa, NULL, &warpa_fe->border_color));
	CHK_ERR(mtk_warpa_set_proc_mode(warpa_fe->dev_warpa, NULL, warpa_fe->proc_mode));
	CHK_ERR(mtk_warpa_set_out_mode(warpa_fe->dev_warpa, warpa_fe->warpa_out_mode));

	return 0;
}

static int mtk_warpa_fe_config_wdma(struct mtk_warpa_fe *warpa_fe)
{
	int w, h;

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		w = warpa_fe->warpa_out_w;
		h = warpa_fe->warpa_out_h;
		dev_dbg(warpa_fe->dev, "wdma_0 region w:%d h:%d\n", w, h);
		CHK_ERR(mtk_wdma_set_region(warpa_fe->dev_wdma0, NULL, w, h, w, h, 0, 0));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		w = warpa_fe->rsz_0_out_w + warpa_fe->padding_val_1 * (warpa_fe->img_num - 1);
		h = warpa_fe->rsz_0_out_h;

		dev_dbg(warpa_fe->dev, "wdma_1 region w:%d h:%d\n", w, h);
		CHK_ERR(mtk_wdma_set_region(warpa_fe->dev_wdma1, NULL, w, h, w, h, 0, 0));

		w = warpa_fe->rsz_1_out_w + warpa_fe->padding_val_2 * (warpa_fe->img_num - 1);
		h = warpa_fe->rsz_1_out_h;

		dev_dbg(warpa_fe->dev, "wdma_2 region w:%d h:%d\n", w, h);
		CHK_ERR(mtk_wdma_set_region(warpa_fe->dev_wdma2, NULL, w, h, w, h, 0, 0));
	}

	return 0;
}

static char *blk_sz_name[MTK_WARPA_FE_BLK_SZ_MAX + 1] = {
	[MTK_WARPA_FE_BLK_SZ_32x32] = "32x32",
	[MTK_WARPA_FE_BLK_SZ_16x16] = "16x16",
	[MTK_WARPA_FE_BLK_SZ_8x8]   = "8x8",
	[MTK_WARPA_FE_BLK_SZ_MAX]   = "Invalid",
};

static int mtk_warpa_fe_config_fe(struct mtk_warpa_fe *warpa_fe)
{
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		warpa_fe->fe_w = warpa_fe->warpa_out_w;
		warpa_fe->fe_h = warpa_fe->warpa_out_h;

		dev_dbg(warpa_fe->dev, "fe blk_size:%d[%s]\n",
			warpa_fe->blk_sz, blk_sz_name[warpa_fe->blk_sz < MTK_WARPA_FE_BLK_SZ_MAX ? warpa_fe->blk_sz : MTK_WARPA_FE_BLK_SZ_MAX]);

		CHK_ERR(mtk_fe_set_block_size(warpa_fe->dev_fe, NULL, warpa_fe->blk_sz));

		dev_dbg(warpa_fe->dev, "fe region w:%d h:%d merge_mode:%d\n", warpa_fe->fe_w, warpa_fe->fe_h,
			warpa_fe->fe_merge_mode);
		CHK_ERR(mtk_fe_set_region(warpa_fe->dev_fe, NULL, warpa_fe->fe_w, warpa_fe->fe_h,
					  warpa_fe->fe_merge_mode));

		dev_dbg(warpa_fe->dev,
			"fe flat_en:%d, harris:%d, th_g:%d, th_c:%d, cr_val:%d\n",
			warpa_fe->fe_flat_enable, warpa_fe->fe_harris_kappa,
			warpa_fe->fe_th_g, warpa_fe->fe_th_c, warpa_fe->fe_cr_val_sel);
		CHK_ERR(mtk_fe_set_params(warpa_fe->dev_fe, NULL, warpa_fe->fe_flat_enable, warpa_fe->fe_harris_kappa,
					  warpa_fe->fe_th_g, warpa_fe->fe_th_c, warpa_fe->fe_cr_val_sel));
	}

	return 0;
}

static int mtk_warpa_fe_config_rsz(struct mtk_warpa_fe *warpa_fe)
{
	struct mtk_resizer_config config_p_scalar0 = { 0 };
	struct mtk_resizer_config config_p_scalar1 = { 0 };

	/*resizer cofing */
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		dev_dbg(warpa_fe->dev, "rsz0 out region w:%d h:%d\n", warpa_fe->rsz_0_out_w, warpa_fe->rsz_0_out_h);
		dev_dbg(warpa_fe->dev, "rsz1 out region w:%d h:%d\n", warpa_fe->rsz_1_out_w, warpa_fe->rsz_1_out_h);

		config_p_scalar0.in_width = warpa_fe->warpa_out_w + warpa_fe->padding_val_0 * (warpa_fe->img_num - 1);
		config_p_scalar0.in_height = warpa_fe->warpa_out_h;
		if (warpa_fe->warpafemask & WARPA_FE_RSZ_0_BYPASS_MASK) {
			config_p_scalar0.out_width = config_p_scalar0.in_width;
			config_p_scalar0.out_height = config_p_scalar0.in_height;
			dev_dbg(warpa_fe->dev, "bypass rsz0 with in/out w:%d h:%d\n",
				config_p_scalar0.in_width, config_p_scalar0.in_height);
		} else {
			config_p_scalar0.out_width = warpa_fe->rsz_0_out_w;
			config_p_scalar0.out_height = warpa_fe->rsz_0_out_h;
		}
		CHK_ERR(mtk_resizer_config(warpa_fe->dev_p_rsz0, &warpa_fe->cmdq_handles, &config_p_scalar0));

		config_p_scalar1.in_width = warpa_fe->rsz_0_out_w;
		config_p_scalar1.in_height = warpa_fe->rsz_0_out_h;
		if (warpa_fe->warpafemask & WARPA_FE_RSZ_1_BYPASS_MASK) {
			config_p_scalar1.out_width = config_p_scalar1.in_width;
			config_p_scalar1.out_height = config_p_scalar1.in_height;
			dev_dbg(warpa_fe->dev, "bypass rsz1 with in/out w:%d h:%d\n",
				config_p_scalar1.in_width, config_p_scalar1.in_height);
		} else {
			config_p_scalar1.out_width = warpa_fe->rsz_1_out_w;
			config_p_scalar1.out_height = warpa_fe->rsz_1_out_h;
		}
		CHK_ERR(mtk_resizer_config(warpa_fe->dev_p_rsz1, &warpa_fe->cmdq_handles, &config_p_scalar1));
	}

	return 0;
}

static int mtk_warpa_fe_config_padding(struct mtk_warpa_fe *warpa_fe)
{
	int padding_mode = warpa_fe->img_num == 2 ? 1 : 0;

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		dev_dbg(warpa_fe->dev, "padding val 0:%d 1:%d 2:%d\n",
			warpa_fe->padding_val_0, warpa_fe->padding_val_1, warpa_fe->padding_val_2);

		if (warpa_fe->warpafemask & WARPA_FE_PADDING_0_BYPASS_MASK) {
			warpa_fe->padding_val_0 = 0;
			dev_dbg(warpa_fe->dev, "bypass padding 0\n");
		}
		if (warpa_fe->warpafemask & WARPA_FE_PADDING_1_BYPASS_MASK) {
			warpa_fe->padding_val_1 = 0;
			dev_dbg(warpa_fe->dev, "bypass padding 1\n");
		}
		if (warpa_fe->warpafemask & WARPA_FE_PADDING_2_BYPASS_MASK) {
			warpa_fe->padding_val_2 = 0;
			dev_dbg(warpa_fe->dev, "bypass padding 2\n");
		}

		CHK_ERR(mtk_padding_config(warpa_fe->dev_padding0, warpa_fe->cmdq_handles, padding_mode,
					   warpa_fe->padding_val_0, warpa_fe->warpa_out_w, warpa_fe->warpa_out_h));
		CHK_ERR(mtk_padding_config(warpa_fe->dev_padding1, warpa_fe->cmdq_handles, padding_mode,
					   warpa_fe->padding_val_1, warpa_fe->rsz_0_out_w, warpa_fe->rsz_0_out_h));
		CHK_ERR(mtk_padding_config(warpa_fe->dev_padding2, warpa_fe->cmdq_handles, padding_mode,
					   warpa_fe->padding_val_2, warpa_fe->rsz_1_out_w, warpa_fe->rsz_1_out_h));

		CHK_ERR(mtk_padding_set_bypass(warpa_fe->dev_padding0, warpa_fe->cmdq_handles,
					       warpa_fe->padding_val_0 > 0 ? 0 : 1));
		CHK_ERR(mtk_padding_set_bypass(warpa_fe->dev_padding1, warpa_fe->cmdq_handles,
					       warpa_fe->padding_val_1 > 0 ? 0 : 1));
		CHK_ERR(mtk_padding_set_bypass(warpa_fe->dev_padding2, warpa_fe->cmdq_handles,
					       warpa_fe->padding_val_2 > 0 ? 0 : 1));
	}

	return 0;
}

static int mtk_warpa_fe_module_config(struct mtk_warpa_fe *warpa_fe)
{
	CHK_ERR(mtk_warpa_fe_config_warpa(warpa_fe));
	CHK_ERR(mtk_warpa_fe_config_rbfc(warpa_fe));
	CHK_ERR(mtk_warpa_fe_config_wdma(warpa_fe));
	CHK_ERR(mtk_warpa_fe_config_fe(warpa_fe));
	CHK_ERR(mtk_warpa_fe_config_rsz(warpa_fe));
	CHK_ERR(mtk_warpa_fe_config_padding(warpa_fe));

	return 0;
}

static int mtk_warpa_fe_module_set_buf(struct mtk_warpa_fe *warpa_fe, u32 out_buf_cnt)
{
	int i = 0;
	int warpa_idx, warp_map_idx, point_idx, desc_idx, wdma0_idx, wdma1_idx, wdma2_idx;
	dma_addr_t addr;
	int sysram_cnt;

	dev_dbg(warpa_fe->dev, "warpa_fe set_buf, mask = %d\n", warpa_fe->warpafemask);

	warpa_idx = MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_MIN;
	sync_for_device(warpa_fe->dev_warpa, warpa_fe->buf[warpa_idx]);

	sysram_cnt = 0;
	for (i = 0; i < warpa_fe->img_num; i++) {
		if (warpa_fe->rbfc_chassing_mode == 1) {
			addr = roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) * i;
		} else {
			addr = warpa_fe->buf[warpa_idx]->dma_addr + roundup(warpa_fe->warpa_in_w, warpa_fe->warpa_align) * i;
		}
		dev_dbg(warpa_fe->dev, "set buf MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF_%d @0x%llx\n",
			i, addr);
		CHK_ERR((*s_mtk_warpa_set_buf_in[i])(warpa_fe->dev_warpa, NULL, addr,
						     warpa_fe->buf[warpa_idx]->pitch));

		warp_map_idx = MTK_WARPA_FE_GET_BUF_NUM(MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_MIN, warpa_fe->warpmap_cnt, i);
		dev_dbg(warpa_fe->dev, "set buf MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_%d @0x%llx\n",
			i, warpa_fe->buf[warp_map_idx]->dma_addr);
		CHK_ERR((*s_mtk_warpa_set_buf_map[i])(warpa_fe->dev_warpa, NULL,
						      warpa_fe->buf[warp_map_idx]->dma_addr, warpa_fe->buf[warp_map_idx]->pitch));

		point_idx =  MTK_WARPA_FE_GET_BUF_NUM(MTK_WARPA_FE_BUF_TYPE_FE_POINT_MIN, out_buf_cnt, i);
		addr = warpa_fe->buf[point_idx]->dma_addr;
		dev_dbg(warpa_fe->dev, "set buf MTK_WARPA_FE_BUF_TYPE_FE_L_POINT_%d_%d @0x%llx\n",
			out_buf_cnt, i, addr);
		CHK_ERR(mtk_fe_set_multi_out_buf(warpa_fe->dev_fe, NULL, i, MTK_FE_OUT_BUF_POINT, addr,
						 warpa_fe->buf[point_idx]->pitch));
		if (warpa_fe->buf[point_idx]->is_sysram) {
			sysram_cnt++;
		}

		desc_idx =  MTK_WARPA_FE_GET_BUF_NUM(MTK_WARPA_FE_BUF_TYPE_FE_DESC_MIN, out_buf_cnt, i);
		addr = warpa_fe->buf[desc_idx]->dma_addr;
		dev_dbg(warpa_fe->dev, "set buf MTK_WARPA_FE_BUF_TYPE_FE_L_DESC_%d_%d @0x%llx\n",
			out_buf_cnt, i, addr);
		CHK_ERR(mtk_fe_set_multi_out_buf(warpa_fe->dev_fe, NULL, i, MTK_FE_OUT_BUF_DESCRIPTOR, addr,
						 warpa_fe->buf[desc_idx]->pitch));
		if (warpa_fe->buf[desc_idx]->is_sysram) {
			sysram_cnt++;
		}
	}
	if (sysram_cnt == warpa_fe->img_num * 2) {
		dev_dbg(warpa_fe->dev, "set buf: FE_POINT/DESC buffer is SYSRAM.\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
						   MTK_MMSYS_CMMN_FE, MTK_MMSYS_CMMN_SYSRAM));
	} else if (sysram_cnt == 0) {
		dev_dbg(warpa_fe->dev, "set buf: FE_POINT/DESC buffer is DRAM.\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
						   MTK_MMSYS_CMMN_FE, MTK_MMSYS_CMMN_DRAM));
	} else {
		dev_err(warpa_fe->dev, "set FE_POINT/DESC buf error: RAM and SYSRAM are mixed.(%d)\n", sysram_cnt);
		return -EINVAL;
	}

	wdma0_idx = MTK_WARPA_FE_BUF_TYPE_WDMA_MIN + out_buf_cnt;
	addr = warpa_fe->buf[wdma0_idx]->dma_addr;
	dev_dbg(warpa_fe->dev, "set buf MTK_WARPA_FE_BUF_TYPE_WDMA_%d_0 @0x%llx\n",
		out_buf_cnt, addr);
	CHK_ERR(mtk_wdma_set_out_buf(warpa_fe->dev_wdma0, NULL, addr, warpa_fe->buf[wdma0_idx]->pitch,
				     warpa_fe->buf[wdma0_idx]->format));
	if (warpa_fe->buf[wdma0_idx]->is_sysram) {
		dev_dbg(warpa_fe->dev, "set buf: WDMA buffer is SYSRAM.\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
						   MTK_MMSYS_CMMN_WDMA_0, MTK_MMSYS_CMMN_SYSRAM));
	} else {
		dev_dbg(warpa_fe->dev, "set buf: WDMA buffer is DRAM.\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
						   MTK_MMSYS_CMMN_WDMA_0, MTK_MMSYS_CMMN_DRAM));
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		wdma1_idx = MTK_WARPA_FE_BUF_TYPE_WDMA_1_MIN + out_buf_cnt;
		addr = warpa_fe->buf[wdma1_idx]->dma_addr;
		dev_dbg(warpa_fe->dev, "set buf MTK_WARPA_FE_BUF_TYPE_WDMA_%d_1 @0x%llx\n",
			out_buf_cnt, addr);
		CHK_ERR(mtk_wdma_set_out_buf(warpa_fe->dev_wdma1, NULL, addr, warpa_fe->buf[wdma1_idx]->pitch,
					     warpa_fe->buf[wdma1_idx]->format));
		if (warpa_fe->buf[wdma1_idx]->is_sysram) {
			dev_dbg(warpa_fe->dev, "set buf: WDMA_1 buffer is SYSRAM.\n");
			CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
							   MTK_MMSYS_CMMN_WDMA_1, MTK_MMSYS_CMMN_SYSRAM));
		} else {
			dev_dbg(warpa_fe->dev, "set buf: WDMA_1 buffer is DRAM.\n");
			CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
							   MTK_MMSYS_CMMN_WDMA_1, MTK_MMSYS_CMMN_DRAM));
		}

		wdma2_idx = MTK_WARPA_FE_BUF_TYPE_WDMA_2_MIN + out_buf_cnt;
		addr = warpa_fe->buf[wdma2_idx]->dma_addr;
		dev_dbg(warpa_fe->dev, "set buf MTK_WARPA_FE_BUF_TYPE_WDMA_%d_2 @0x%llx\n",
			out_buf_cnt, addr);
		CHK_ERR(mtk_wdma_set_out_buf(warpa_fe->dev_wdma2, NULL, addr, warpa_fe->buf[wdma2_idx]->pitch,
					     warpa_fe->buf[wdma2_idx]->format));
		if (warpa_fe->buf[wdma2_idx]->is_sysram) {
			dev_dbg(warpa_fe->dev, "set buf: WDMA_2 buffer is SYSRAM.\n");
			CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
							   MTK_MMSYS_CMMN_WDMA_2, MTK_MMSYS_CMMN_SYSRAM));
		} else {
			dev_dbg(warpa_fe->dev, "set buf: WDMA_2 buffer is DRAM.\n");
			CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(warpa_fe->dev_mmsys_cmmn_top,
							   MTK_MMSYS_CMMN_WDMA_2, MTK_MMSYS_CMMN_DRAM));
		}
	}

	return 0;
}

static int mtk_warpa_fe_trigger_thread(void *arg);

static int mtk_warpa_fe_module_start(struct mtk_warpa_fe *warpa_fe)
{
	dev_dbg(warpa_fe->dev, "warpa_fe_module_start\n");

	CHK_ERR(mtk_warpa_start(warpa_fe->dev_warpa, NULL));
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		CHK_ERR(mtk_fe_start(warpa_fe->dev_fe, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		CHK_ERR(mtk_wdma_start(warpa_fe->dev_wdma0, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_padding_start(warpa_fe->dev_padding0, warpa_fe->cmdq_handles));
		CHK_ERR(mtk_resizer_start(warpa_fe->dev_p_rsz0, &warpa_fe->cmdq_handles));
		CHK_ERR(mtk_padding_start(warpa_fe->dev_padding1, warpa_fe->cmdq_handles));
		CHK_ERR(mtk_wdma_start(warpa_fe->dev_wdma1, NULL));
		CHK_ERR(mtk_resizer_start(warpa_fe->dev_p_rsz1, &warpa_fe->cmdq_handles));
		CHK_ERR(mtk_padding_start(warpa_fe->dev_padding2, warpa_fe->cmdq_handles));
		CHK_ERR(mtk_wdma_start(warpa_fe->dev_wdma2, NULL));
	}

	warpa_fe->cmdq_client = cmdq_mbox_create(warpa_fe->dev, 0);
	if (IS_ERR(warpa_fe->cmdq_client)) {
		dev_err(warpa_fe->dev, "fail to create mailbox cmdq_client\n");
		return PTR_ERR(warpa_fe->cmdq_client);
	}

	if (warpa_fe->trigger_task) {
		dev_err(warpa_fe->dev, "%s: trigger_task is not NULL.\n", __func__);
		return -EINVAL;
	}

	warpa_fe->cmdq_err = 0;
	warpa_fe->trigger_task = kthread_run(mtk_warpa_fe_trigger_thread, warpa_fe, "WarpaFeTriggerThread");
	if (IS_ERR(warpa_fe->trigger_task)) {
		dev_err(warpa_fe->dev, "Failed to kthread_run. %ld\n",
			PTR_ERR(warpa_fe->trigger_task));
		return -EPERM;
	}
#ifdef WARPA_FE_TRIGGER_THREAD_PRIORITY
	{
		struct sched_param param;

		param.sched_priority = WARPA_FE_TRIGGER_THREAD_PRIORITY;
		sched_setscheduler(warpa_fe->trigger_task, SCHED_RR, &param);
	}
#endif


	return 0;
}

static int mtk_warpa_fe_module_stop(struct mtk_warpa_fe *warpa_fe)
{
	if (!warpa_fe->trigger_task) {
		dev_err(warpa_fe->dev, "%s: trigger_task is NULL.\n", __func__);
		return -EINVAL;
	}
	if (!(warpa_fe->warpafemask & WARPA_FE_AUTO_NEXT_TRIGGER)) {
		warpa_fe->trigger_buf_index = TRIGGER_THREAD_STOP;
		wake_up_interruptible(&warpa_fe->trigger_wait_queue);
	}
	kthread_stop(warpa_fe->trigger_task);
	warpa_fe->trigger_task = NULL;

	CHK_ERR(cmdq_mbox_destroy(warpa_fe->cmdq_client));

	/* when warpa_fe triggered by sof */
	/* need to disable mutex before stop */
	if (warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) {
		CHK_ERR(mtk_mutex_disable(warpa_fe->mutex, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		CHK_ERR(mtk_fe_stop(warpa_fe->dev_fe, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		CHK_ERR(mtk_wdma_stop(warpa_fe->dev_wdma0, NULL));
	}
	CHK_ERR(mtk_warpa_stop(warpa_fe->dev_warpa, NULL));
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_padding_stop(warpa_fe->dev_padding0, NULL));
		CHK_ERR(mtk_resizer_stop(warpa_fe->dev_p_rsz0, NULL));
		CHK_ERR(mtk_padding_stop(warpa_fe->dev_padding1, NULL));
		CHK_ERR(mtk_wdma_stop(warpa_fe->dev_wdma1, NULL));
		CHK_ERR(mtk_resizer_stop(warpa_fe->dev_p_rsz1, NULL));
		CHK_ERR(mtk_padding_stop(warpa_fe->dev_padding2, NULL));
		CHK_ERR(mtk_wdma_stop(warpa_fe->dev_wdma2, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) {
		CHK_ERR(mtk_rbfc_finish(warpa_fe->dev_rbfc));
	}

	return 0;
}

static int mtk_warpa_fe_module_power_on(struct mtk_warpa_fe *warpa_fe)
{
	CHK_ERR(mtk_mmsys_cmmn_top_power_on(warpa_fe->dev_mmsys_cmmn_top));
	CHK_ERR(mtk_mmsys_cfg_power_on(warpa_fe->dev_mmsys_core_top));
	CHK_ERR(mtk_warpa_power_on(warpa_fe->dev_warpa));
	CHK_ERR(mtk_rbfc_power_on(warpa_fe->dev_rbfc));

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		CHK_ERR(mtk_fe_power_on(warpa_fe->dev_fe));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		CHK_ERR(mtk_wdma_power_on(warpa_fe->dev_wdma0));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_padding_power_on(warpa_fe->dev_padding0));
		CHK_ERR(mtk_resizer_power_on(warpa_fe->dev_p_rsz0));
		CHK_ERR(mtk_padding_power_on(warpa_fe->dev_padding1));
		CHK_ERR(mtk_wdma_power_on(warpa_fe->dev_wdma1));
		CHK_ERR(mtk_resizer_power_on(warpa_fe->dev_p_rsz1));
		CHK_ERR(mtk_padding_power_on(warpa_fe->dev_padding2));
		CHK_ERR(mtk_wdma_power_on(warpa_fe->dev_wdma2));
	}
	warpa_fe->mutex = mtk_mutex_get(warpa_fe->dev_mutex, 0);
	if (IS_ERR(warpa_fe->mutex)) {
		int ret = PTR_ERR(warpa_fe->mutex);

		dev_err(warpa_fe->dev, "mutex_get fail!! %d\n", ret);
		return ret;
	}
	CHK_ERR(mtk_mutex_power_on(warpa_fe->mutex));

	return 0;
}

static int mtk_warpa_fe_module_power_off(struct mtk_warpa_fe *warpa_fe)
{
	CHK_ERR(mtk_mutex_power_off(warpa_fe->mutex));
	CHK_ERR(mtk_mutex_put(warpa_fe->mutex));
	CHK_ERR(mtk_mmsys_cmmn_top_power_off(warpa_fe->dev_mmsys_cmmn_top));
	CHK_ERR(mtk_mmsys_cfg_power_off(warpa_fe->dev_mmsys_core_top));
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		CHK_ERR(mtk_fe_power_off(warpa_fe->dev_fe));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		CHK_ERR(mtk_wdma_power_off(warpa_fe->dev_wdma0));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_padding_power_off(warpa_fe->dev_padding0));
		CHK_ERR(mtk_resizer_power_off(warpa_fe->dev_p_rsz0));
		CHK_ERR(mtk_padding_power_off(warpa_fe->dev_padding1));
		CHK_ERR(mtk_wdma_power_off(warpa_fe->dev_wdma1));
		CHK_ERR(mtk_resizer_power_off(warpa_fe->dev_p_rsz1));
		CHK_ERR(mtk_padding_power_off(warpa_fe->dev_padding2));
		CHK_ERR(mtk_wdma_power_off(warpa_fe->dev_wdma2));
	}
	CHK_ERR(mtk_warpa_power_off(warpa_fe->dev_warpa));
	CHK_ERR(mtk_rbfc_power_off(warpa_fe->dev_rbfc));

	return 0;
}

static int mtk_warpa_fe_path_connect(struct mtk_warpa_fe *warpa_fe)
{
	if ((warpa_fe->warpafemask & WARPA_FE_RBFC_ENABLE_MASK) &&
	    !(warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE)) {
		dev_dbg(warpa_fe->dev, "warpa_fe select rbfc sof!\n");
		CHK_ERR(mtk_mutex_select_sof(warpa_fe->mutex, MUTEX_COMMON_SOF_RBFC_SIDE0_0_WPEA, NULL, false));
	} else {
		dev_dbg(warpa_fe->dev, "warpa_fe select single sof!\n");
		CHK_ERR(mtk_mutex_select_sof(warpa_fe->mutex, MUTEX_COMMON_SOF_SINGLE, NULL, false));
	}

	dev_dbg(warpa_fe->dev, "warpa fe path conn\n");
	dev_dbg(warpa_fe->dev, "warpafemask:%d\n", warpa_fe->warpafemask);

	CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_RBFC_WPEA1, NULL));
	CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_WPEA1, NULL));
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_FE, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_WDMA1, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_PADDING_0, NULL));
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_MMCOMMON_RSZ0, NULL));
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_PADDING_1, NULL));
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_WDMA2, NULL));
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_MMCOMMON_RSZ1, NULL));
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_PADDING_2, NULL));
		CHK_ERR(mtk_mutex_add_comp(warpa_fe->mutex, MUTEX_COMPONENT_WDMA3, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		CHK_ERR(mtk_mmsys_cmmn_top_conn(warpa_fe->dev_mmsys_cmmn_top,
						MTK_MMSYS_CMMN_MOD_WPEA, MTK_MMSYS_CMMN_MOD_FE));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		CHK_ERR(mtk_mmsys_cmmn_top_conn(warpa_fe->dev_mmsys_cmmn_top,
						MTK_MMSYS_CMMN_MOD_WPEA, MTK_MMSYS_CMMN_MOD_WDMA));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_mmsys_cmmn_top_conn(warpa_fe->dev_mmsys_cmmn_top,
						MTK_MMSYS_CMMN_MOD_WPEA, MTK_MMSYS_CMMN_MOD_PADDING_0));
		CHK_ERR(mtk_mmsys_cmmn_top_conn(warpa_fe->dev_mmsys_cmmn_top,
						MTK_MMSYS_CMMN_MOD_RSZ_0, MTK_MMSYS_CMMN_MOD_PADDING_1));
		CHK_ERR(mtk_mmsys_cmmn_top_conn(warpa_fe->dev_mmsys_cmmn_top,
						MTK_MMSYS_CMMN_MOD_RSZ_0, MTK_MMSYS_CMMN_MOD_PADDING_2));
	}

	return 0;
}

static int mtk_warpa_fe_path_disconnect(struct mtk_warpa_fe *warpa_fe)
{
	dev_dbg(warpa_fe->dev, "warpa fe path disconn\n");

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_mmsys_cmmn_top_disconn(warpa_fe->dev_mmsys_cmmn_top,
						   MTK_MMSYS_CMMN_MOD_WPEA, MTK_MMSYS_CMMN_MOD_PADDING_0));
		CHK_ERR(mtk_mmsys_cmmn_top_disconn(warpa_fe->dev_mmsys_cmmn_top,
						   MTK_MMSYS_CMMN_MOD_RSZ_0, MTK_MMSYS_CMMN_MOD_PADDING_1));
		CHK_ERR(mtk_mmsys_cmmn_top_disconn(warpa_fe->dev_mmsys_cmmn_top,
						   MTK_MMSYS_CMMN_MOD_RSZ_0, MTK_MMSYS_CMMN_MOD_PADDING_2));
	}
	CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_WPEA1, NULL));
	CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_RBFC_WPEA1, NULL));
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_FE) {
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_FE, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_WDMA1, NULL));
	}
	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_PADDING_0, NULL));
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_MMCOMMON_RSZ0, NULL));
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_PADDING_1, NULL));
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_WDMA2, NULL));
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_MMCOMMON_RSZ1, NULL));
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_PADDING_2, NULL));
		CHK_ERR(mtk_mutex_remove_comp(warpa_fe->mutex, MUTEX_COMPONENT_WDMA3, NULL));
	}

	return 0;
}

static int mtk_warpa_fe_buffer_sync_for_cpu(struct mtk_warpa_fe *warpa_fe, u32 out_buf_cnt)
{
	int i;
	int buf_type;

	for (i = 0; i < warpa_fe->img_num; i++) {
		buf_type = MTK_WARPA_FE_GET_BUF_NUM(MTK_WARPA_FE_BUF_TYPE_FE_POINT_MIN, out_buf_cnt, i);
		if (!warpa_fe->buf[buf_type]->is_sysram) {
			dma_sync_sg_for_cpu(warpa_fe->dev_fe,
					    warpa_fe->buf[buf_type]->sg->sgl, warpa_fe->buf[buf_type]->sg->nents, DMA_BIDIRECTIONAL);
		}
		buf_type = MTK_WARPA_FE_GET_BUF_NUM(MTK_WARPA_FE_BUF_TYPE_FE_DESC_MIN, out_buf_cnt, i);
		if (!warpa_fe->buf[buf_type]->is_sysram) {
			dma_sync_sg_for_cpu(warpa_fe->dev_fe,
					    warpa_fe->buf[buf_type]->sg->sgl, warpa_fe->buf[buf_type]->sg->nents, DMA_BIDIRECTIONAL);
		}
	}

	buf_type = MTK_WARPA_FE_BUF_TYPE_WDMA_MIN + out_buf_cnt;
	if (!warpa_fe->buf[buf_type]->is_sysram) {
		dma_sync_sg_for_cpu(warpa_fe->dev_wdma0,
				    warpa_fe->buf[buf_type]->sg->sgl, warpa_fe->buf[buf_type]->sg->nents, DMA_BIDIRECTIONAL);
	}

	if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
		buf_type = MTK_WARPA_FE_BUF_TYPE_WDMA_1_MIN + out_buf_cnt;
		if (!warpa_fe->buf[buf_type]->is_sysram) {
			dma_sync_sg_for_cpu(warpa_fe->dev_wdma1,
					    warpa_fe->buf[buf_type]->sg->sgl, warpa_fe->buf[buf_type]->sg->nents, DMA_BIDIRECTIONAL);
		}
		buf_type = MTK_WARPA_FE_BUF_TYPE_WDMA_2_MIN + out_buf_cnt;
		if (!warpa_fe->buf[buf_type]->is_sysram) {
			dma_sync_sg_for_cpu(warpa_fe->dev_wdma2,
					    warpa_fe->buf[buf_type]->sg->sgl, warpa_fe->buf[buf_type]->sg->nents, DMA_BIDIRECTIONAL);
		}
	}
	return 0;
}

static int mtk_warpa_fe_trigger_thread(void *arg)
{
	struct mtk_warpa_fe *warpa_fe = (struct mtk_warpa_fe *)arg;
	u32 fe_done_event = warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_FRAME_DONE_FE];
	u32 fe_wdma_done_event = warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_FRAME_DONE_WDMA];
	u32 fe_wdma_2_done_event = warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_FRAME_DONE_WDMA_2];
	u32 wait_before_start_event = warpa_fe->cmdq_events[MTK_WARPA_FE_CMDQ_EVENT_ISP_P2_DONE];
	u32 vts;
	u32 dp_counter;
	int retry;
	u64 start_time;
	int ret = 0;

	pr_info("%s start.\n", __func__);
	warpa_fe->current_buf_index = 1;
	if (warpa_fe->warpafemask & WARPA_FE_AUTO_NEXT_TRIGGER) {
		warpa_fe->trigger_buf_index = warpa_fe->current_buf_index ^ 1;
	} else {
		warpa_fe->trigger_buf_index = TRIGGER_THREAD_WAIT;
	}

	CHK_ERR(mtk_warpa_fe_module_set_buf(warpa_fe, warpa_fe->current_buf_index));

#ifdef CONFIG_SIE_DEVELOP_BUILD
	if (warpa_fe->cmdq_pkt) {
		BUG_ON(1);
	}
#endif
	cmdq_pkt_create(&warpa_fe->cmdq_pkt);

	while (!kthread_should_stop()) {
		if (!(warpa_fe->warpafemask & WARPA_FE_AUTO_NEXT_TRIGGER)) {
			ret = wait_event_interruptible_timeout(warpa_fe->trigger_wait_queue, warpa_fe->trigger_buf_index < TRIGGER_THREAD_WAIT, WARPA_FE_TIMEOUT);
			if (ret == 0) {
				// dev_err(warpa_fe->dev, "%s wait evnet TIMEOUT!!!\n", __func__);
				continue;
			}
			if (warpa_fe->trigger_buf_index == TRIGGER_THREAD_STOP) {
				schedule();
				continue;
			}
			warpa_fe->current_buf_index = warpa_fe->trigger_buf_index;
			CHK_ERR(mtk_warpa_fe_module_set_buf(warpa_fe, warpa_fe->current_buf_index));
		}

		if (!(warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE)) {
			retry = 10;
		} else {
			retry = 0;
		}

		start_time = ktime_get_ns();
		do {
			cmdq_pkt_multiple_reset(&warpa_fe->cmdq_pkt, 1);
			if (warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE) {
				cmdq_pkt_wfe(warpa_fe->cmdq_pkt, wait_before_start_event);
			}
			/* Clear Event */
			cmdq_pkt_clear_event(warpa_fe->cmdq_pkt, fe_done_event);
			cmdq_pkt_clear_event(warpa_fe->cmdq_pkt, fe_wdma_done_event);
			cmdq_pkt_clear_event(warpa_fe->cmdq_pkt, fe_wdma_2_done_event);

			ret = mtk_mutex_enable(warpa_fe->mutex, &warpa_fe->cmdq_pkt);
			if (ret) {
				dev_err(warpa_fe->dev, "%s: mtk_mutex_enable fail. ret:%d\n", __func__, ret);
			}

			/* Wait Event */
			cmdq_pkt_wfe(warpa_fe->cmdq_pkt, fe_done_event);
			if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_WDMA) {
				cmdq_pkt_wfe(warpa_fe->cmdq_pkt, fe_wdma_done_event);
			}
			if (warpa_fe->warpafemask & WARPA_FE_CONNECT_TO_P_SCALE) {
				cmdq_pkt_wfe(warpa_fe->cmdq_pkt, fe_wdma_2_done_event);
			}

			ret = mtk_mutex_disable(warpa_fe->mutex, &warpa_fe->cmdq_pkt);
			if (ret) {
				dev_err(warpa_fe->dev, "%s: mtk_mutex_disable fail. ret:%d\n", __func__, ret);
			}

			if (warpa_fe->warpafemask & WARPA_FE_KICK_AFTER_P2_DONE) {
				cmdq_pkt_clear_event(warpa_fe->cmdq_pkt, wait_before_start_event);
			}

			ret = cmdq_pkt_flush(warpa_fe->cmdq_client, warpa_fe->cmdq_pkt);
			if (ret != 0) {
				dev_err(warpa_fe->dev, "%s: cmdq_pkt_flush(Wait Event) fail. ret:%d\n", __func__, ret);
				warpa_fe->cmdq_err++;
			}

			if (mtk_warpa_get_wpe_done(warpa_fe->dev_warpa)) {
				break;
			}
			retry--;
			pr_warn("WARPA_FE Retry %d.(%lld ms) WARP_DONE:%d\n",
				retry, (ktime_get_ns() - start_time) / (1000 * 1000),
				mtk_warpa_get_wpe_done(warpa_fe->dev_warpa));
		} while (retry > 0);

		mtk_ts_get_side_0_a_p2_vision_time(warpa_fe->dev_ts, &vts, &dp_counter);
		warpa_fe->latest_vts = vts;

		smp_mb(); /* wmb for latest_vts */ /* rmb for trigger_wait_flag */

		if (warpa_fe->warpafemask & WARPA_FE_AUTO_NEXT_TRIGGER) {
			if (warpa_fe->trigger_buf_index == warpa_fe->current_buf_index) {
				warpa_fe->current_buf_index ^= 1;
				CHK_ERR(mtk_warpa_fe_module_set_buf(warpa_fe, warpa_fe->current_buf_index));
			}
		} else {
			warpa_fe->trigger_buf_index = TRIGGER_THREAD_WAIT;
		}
		wake_up_interruptible(&warpa_fe->trigger_wait_queue);
	}

	cmdq_pkt_destroy(warpa_fe->cmdq_pkt);
	warpa_fe->cmdq_pkt = NULL;

	pr_info("%s exit.\n", __func__);
	return 0;
}

static int mtk_warpa_fe_path_trigger(struct mtk_warpa_fe *warpa_fe, u32 buf_index)
{
	int ret;

	dev_dbg(warpa_fe->dev, "%s is called.\n", __func__);

	if (!(warpa_fe->warpafemask & WARPA_FE_AUTO_NEXT_TRIGGER)) {
		warpa_fe->trigger_buf_index = buf_index;;
		wake_up_interruptible(&warpa_fe->trigger_wait_queue);
		return 0;
	}

	if (warpa_fe->current_buf_index != buf_index) {
		warpa_fe->trigger_buf_index = buf_index ^ 1;
		smp_wmb(); // for wait_buf_index;
		ret = wait_event_interruptible_timeout(warpa_fe->trigger_wait_queue, warpa_fe->current_buf_index == buf_index, WARPA_FE_TIMEOUT);
		if (ret == 0) {
			dev_err(warpa_fe->dev, "%s wait evnet TIMEOUT!!!\n", __func__);
			return -ETIMEDOUT;
		}
	}

	if (warpa_fe->warpafemask & WARPA_FE_AUTO_NEXT_TRIGGER) {
		warpa_fe->trigger_buf_index = buf_index;
		smp_wmb(); // for wait_buf_index;
	}

	return 0;
}

static int mtk_warpa_fe_path_wait_trigger(struct mtk_warpa_fe *warpa_fe, u32 buf_index)
{
	int ret;

	dev_dbg(warpa_fe->dev, "%s start.\n", __func__);

	if (warpa_fe->warpafemask & WARPA_FE_AUTO_NEXT_TRIGGER) {
		if (warpa_fe->trigger_buf_index != buf_index) {
			dev_err(warpa_fe->dev, "%s Not Triggered index:%d\n", __func__, buf_index);
			return -EPERM;
		}
		ret = wait_event_interruptible_timeout(warpa_fe->trigger_wait_queue, warpa_fe->current_buf_index != buf_index, WARPA_FE_TIMEOUT);
	} else {
		ret = wait_event_interruptible_timeout(warpa_fe->trigger_wait_queue, warpa_fe->trigger_buf_index == 3, WARPA_FE_TIMEOUT);
	}

	if (ret == 0) {
		dev_err(warpa_fe->dev, "%s wait evnet TIMEOUT!!!\n", __func__);
		return -ETIMEDOUT;
	}

	if (warpa_fe->cmdq_err > 0) {
		dev_err(warpa_fe->dev, "%s cmdq_err! %d\n", __func__, warpa_fe->cmdq_err);
		return -EIO;
	}

	dev_dbg(warpa_fe->dev, "%s end.\n", __func__);

	return 0;
}

static int mtk_warpa_fe_reset(struct mtk_warpa_fe *warpa_fe)
{
	dev_dbg(warpa_fe->dev, "warpa_fe_reset\n");

#ifdef ENABLE_SW_RESET
	/* sw reset */
	CHK_ERR(mtk_warpa_reset(warpa_fe->dev_warpa));
	CHK_ERR(mtk_wdma_reset(warpa_fe->dev_wdma0));
	CHK_ERR(mtk_wdma_reset(warpa_fe->dev_wdma1));
	CHK_ERR(mtk_wdma_reset(warpa_fe->dev_wdma2));

	mtk_padding_reset(warpa_fe->dev_padding0, NULL);
	mtk_padding_reset(warpa_fe->dev_padding1, NULL);
	mtk_padding_reset(warpa_fe->dev_padding2, NULL);

	CHK_ERR(mtk_fe_reset(warpa_fe->dev_fe));
#endif

	/* hw reset */
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, MDP_WPEA_1));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, MDP_FE));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, DISP_WDMA_1));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, DISP_WDMA_2));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, DISP_WDMA_3));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, padding_0));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, padding_1));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, padding_2));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, RESIZER_0));
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(warpa_fe->dev_mmsys_cmmn_top, RESIZER_1));

	return 0;
}

static int mtk_warpa_fe_get_dev(struct device **dev, struct mtk_warpa_fe *warpa_fe, int idx)
{
	if (CHK_RANGE(idx, MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF)) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_WARPA_IMG_BUF: %d\n", idx);
		*dev = warpa_fe->dev_warpa;
	} else if (CHK_RANGE(idx, MTK_WARPA_FE_BUF_TYPE_WARPA_MAP)) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_WARPA_MAP: %d\n", idx);
		*dev = warpa_fe->dev_warpa;
	} else if (CHK_RANGE(idx, MTK_WARPA_FE_BUF_TYPE_FE_POINT)) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_FE_POINT: %d\n", idx);
		*dev = warpa_fe->dev_fe;
	} else if (CHK_RANGE(idx, MTK_WARPA_FE_BUF_TYPE_FE_DESC)) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_FE_DESC: %d\n", idx);
		*dev = warpa_fe->dev_fe;
	} else if (CHK_RANGE(idx, MTK_WARPA_FE_BUF_TYPE_WDMA)) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_WDMA: %d\n", idx);
		*dev = warpa_fe->dev_wdma0;
	} else if (CHK_RANGE(idx, MTK_WARPA_FE_BUF_TYPE_WDMA_1)) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_WDMA_1: %d\n", idx);
		*dev = warpa_fe->dev_wdma1;
	} else if (CHK_RANGE(idx, MTK_WARPA_FE_BUF_TYPE_WDMA_2)) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_WDMA_2: %d\n", idx);
		*dev = warpa_fe->dev_wdma2;
	} else if (idx == MTK_WARPA_FE_BUF_TYPE_TEMPORARY_USE_FOR_SYNC_BUFFER) {
		dev_dbg(warpa_fe->dev, "MTK_WARPA_FE_BUF_TYPE_TEMPORARY_USE_FOR_BUFFER_SYNC: %d\n", idx);
		*dev = warpa_fe->dev_warpa;
	} else {
		dev_err(warpa_fe->dev, "%s:SET_BUF: buf type %d. Unknown buf type error\n", __func__, idx);
		return -EINVAL;
	}

	return 0;
}

static int mtk_warpa_fe_ioctl_import_fd_to_handle(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_cv_common_buf_handle handle;
	struct device *dev;

	dev_dbg(warpa_fe->dev, "FD_TO_HANDLE is called.\n");

	CHK_ERR(copy_from_user((void *)&handle, (void *)arg, sizeof(handle)));
	CHK_ERR(mtk_warpa_fe_get_dev(&dev, warpa_fe, handle.buf_type));
	CHK_ERR(mtk_cv_common_fd_to_handle_offset(dev, handle.fd, &handle.handle, handle.offset));
	CHK_ERR(copy_to_user((void *)arg, &handle, sizeof(handle)));

	return 0;
}

static int mtk_warpa_fe_ioctl_set_ctrl_mask(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_set_mask config_mask;

	dev_dbg(warpa_fe->dev, "SET_MASK is called.\n");

	CHK_ERR(copy_from_user((void *)&config_mask, (void *)arg, sizeof(config_mask)));
	warpa_fe->warpafemask = config_mask.warpafemask;

	return 0;
}

static int mtk_warpa_fe_ioctl_config_warpa(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_config_warpa config_warpa;

	dev_dbg(warpa_fe->dev, "CONFIG_WARPA is called.\n");

	CHK_ERR(copy_from_user(&config_warpa, (void *)arg, sizeof(config_warpa)));
	warpa_fe->coef_tbl.idx = config_warpa.coef_tbl_idx;
	memcpy(warpa_fe->coef_tbl.user_coef_tbl, config_warpa.user_coef_tbl, sizeof(warpa_fe->coef_tbl.user_coef_tbl));
	warpa_fe->border_color.enable = config_warpa.border_color_enable;
	warpa_fe->border_color.border_color = config_warpa.border_color;
	warpa_fe->border_color.unmapped_color = config_warpa.unmapped_color;
	warpa_fe->proc_mode = config_warpa.proc_mode;
	warpa_fe->warpa_out_mode = config_warpa.out_mode;
	warpa_fe->reset = config_warpa.reset;
	warpa_fe->warpa_in_w = config_warpa.warpa_in_w;
	warpa_fe->warpa_in_h = config_warpa.warpa_in_h;
	warpa_fe->warpa_out_w = config_warpa.warpa_out_w;
	warpa_fe->warpa_out_h = config_warpa.warpa_out_h;
	warpa_fe->warpa_map_w = config_warpa.warpa_map_w;
	warpa_fe->warpa_map_h = config_warpa.warpa_map_h;
	warpa_fe->warpa_align = config_warpa.warpa_align;

	switch (warpa_fe->proc_mode) {
	case MTK_WARPA_PROC_MODE_LR:
		warpa_fe->img_num = 2;
		break;
	case MTK_WARPA_PROC_MODE_L_ONLY:
		warpa_fe->img_num = 1;
		break;
	case MTK_WARPA_PROC_MODE_QUAD:
		warpa_fe->img_num = 4;
		break;
	default:
		dev_err(warpa_fe->dev, "invalid proc_mode %d\n", warpa_fe->proc_mode);
		return -EINVAL;
	}

	return 0;
}

static int mtk_warpa_fe_ioctl_config_fe(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_config_fe config_fe;

	dev_dbg(warpa_fe->dev, "CONFIG_FE is called.\n");

	CHK_ERR(copy_from_user(&config_fe, (void *)arg, sizeof(config_fe)));
	warpa_fe->blk_sz = config_fe.blk_sz;
	warpa_fe->fe_w = config_fe.fe_w;
	warpa_fe->fe_h = config_fe.fe_h;
	warpa_fe->fe_merge_mode = config_fe.fe_merge_mode;
	warpa_fe->fe_flat_enable = config_fe.fe_flat_enable;
	warpa_fe->fe_harris_kappa = config_fe.fe_harris_kappa;
	warpa_fe->fe_th_g = config_fe.fe_th_g;
	warpa_fe->fe_th_c = config_fe.fe_th_c;
	warpa_fe->fe_cr_val_sel = config_fe.fe_cr_val_sel;
	warpa_fe->fe_align = config_fe.fe_align;

	return 0;
}

static int mtk_warpa_fe_ioctl_config_wdma(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_config_wdma config_wdma;

	dev_dbg(warpa_fe->dev, "CONFIG_WDMA is called.\n");

	CHK_ERR(copy_from_user(&config_wdma, (void *)arg, sizeof(config_wdma)));
	warpa_fe->wdma_align = config_wdma.wdma_align;

	return 0;
}

static int mtk_warpa_fe_ioctl_config_rsz(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_config_rsz config_rsz;

	dev_dbg(warpa_fe->dev, "CONFIG_RSZ is called.\n");

	CHK_ERR(copy_from_user(&config_rsz, (void *)arg, sizeof(config_rsz)));
	warpa_fe->rsz_0_out_w = config_rsz.rsz_0_out_w;
	warpa_fe->rsz_0_out_h = config_rsz.rsz_0_out_h;
	warpa_fe->rsz_1_out_w = config_rsz.rsz_1_out_w;
	warpa_fe->rsz_1_out_h = config_rsz.rsz_1_out_h;

	return 0;
}

static int mtk_warpa_fe_ioctl_config_padding(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_config_padding config_padding;

	dev_dbg(warpa_fe->dev, "CONFIG_PADDING is called.\n");

	CHK_ERR(copy_from_user(&config_padding, (void *)arg, sizeof(config_padding)));
	warpa_fe->padding_val_0 = config_padding.padding_val_0;
	warpa_fe->padding_val_1 = config_padding.padding_val_1;
	warpa_fe->padding_val_2 = config_padding.padding_val_2;

	return 0;
}

static int mtk_warpa_fe_ioctl_set_buf(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_cv_common_set_buf buf;
	struct mtk_cv_common_buf *buf_handle;

	CHK_ERR(copy_from_user((void *)&buf, (void *)arg, sizeof(buf)));
	if (buf.buf_type >= MTK_WARPA_FE_BUF_TYPE_MAX) {
		dev_err(warpa_fe->dev, "MTK_WARPA_FE:SET_BUF: buf type %d error\n", buf.buf_type);
		return -EINVAL;
	}

	buf_handle = mtk_cv_common_get_buf_from_handle(buf.handle);
	buf_handle->pitch = buf.pitch;
	buf_handle->offset = buf.offset;
	buf_handle->format = buf.format;
	dev_dbg(warpa_fe->dev, "%s:SET_BUF: buf_handle:%d pitch:%d format:%d buf_type:%d dma_addr:%llx offset:%d\n", __func__,
		buf.handle, buf.pitch, buf.format, buf.buf_type, buf_handle->dma_addr,  buf_handle->offset);
	warpa_fe->buf[buf.buf_type] = buf_handle;

	return 0;
}

static int mtk_warpa_fe_ioctl_streamon(struct mtk_warpa_fe *warpa_fe)
{
	dev_dbg(warpa_fe->dev, "STREAMON is called.\n");

	CHK_ERR(mtk_warpa_fe_module_power_on(warpa_fe));
	if (warpa_fe->reset) {
		dev_dbg(warpa_fe->dev, "%s: Do RESET FE.\n", __func__);
		CHK_ERR(mtk_warpa_fe_reset(warpa_fe));
	} else {
		dev_dbg(warpa_fe->dev, "%s: Do NOT RESET FE.\n", __func__);
	}

	CHK_ERR(mtk_warpa_fe_module_config(warpa_fe));
	CHK_ERR(mtk_warpa_fe_path_connect(warpa_fe));
	CHK_ERR(mtk_warpa_fe_module_start(warpa_fe));
	dev_dbg(warpa_fe->dev, "stream on done!\n");

	return 0;
}

static int mtk_warpa_fe_ioctl_streamoff(struct mtk_warpa_fe *warpa_fe)
{
	dev_dbg(warpa_fe->dev, "STREAMOFF is called.");

	CHK_ERR(mtk_warpa_fe_module_stop(warpa_fe));
	CHK_ERR(mtk_warpa_fe_path_disconnect(warpa_fe));
	CHK_ERR(mtk_warpa_fe_module_power_off(warpa_fe));
	dev_dbg(warpa_fe->dev, "stream off done!\n");

	return 0;
}

static int mtk_warpa_fe_ioctl_trigger(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_trigger trigger;
	u32 buf_index;
	int ret = 0;

	CHK_ERR(copy_from_user(&trigger, (void *)arg, sizeof(trigger)));

	buf_index = trigger.buf_index;
	dev_dbg(warpa_fe->dev, "%s: buf_index:%d\n", __func__, buf_index);
	if (buf_index >= MTK_WARPA_FE_BUF_MAX_OUT_BUF_CNT) {
		dev_err(warpa_fe->dev, "%s:TRIGGER: buf_index is too big. %d\n", __func__, buf_index);
		return -EINVAL;
	}

	if (trigger.wait_flag == MTK_WARPA_FE_TRIGGER_AND_WAIT ||
	    trigger.wait_flag == MTK_WARPA_FE_TRIGGER_ONLY) {
		CHK_ERR(mtk_warpa_fe_path_trigger(warpa_fe, buf_index));
	}

	if (trigger.wait_flag == MTK_WARPA_FE_TRIGGER_AND_WAIT ||
	    trigger.wait_flag == MTK_WARPA_FE_TRIGGER_WAIT) {
		ret = mtk_warpa_fe_path_wait_trigger(warpa_fe, buf_index);
		if (ret == -EIO || ret == ETIMEDOUT) {
			/* cmdq error happen */
			ret = 0; /* ioctl itself return OK to notice Error to upper layer */
			trigger.err_status = MTK_WARPA_FE_NEED_TO_RESTART;
		} else {
			trigger.err_status = MTK_WARPA_FE_NO_ERR;
		}

		mtk_warpa_fe_buffer_sync_for_cpu(warpa_fe, buf_index);

		smp_rmb(); /* for latest_vts */
		trigger.vts = warpa_fe->latest_vts;
		CHK_ERR(copy_to_user((void *)arg, &trigger, sizeof(trigger)));
	}

	return ret;
}

static int mtk_warpa_fe_ioctl_reset(struct mtk_warpa_fe *warpa_fe)
{
	dev_dbg(warpa_fe->dev, "RESET is called.");
	CHK_ERR(mtk_warpa_fe_reset(warpa_fe));

	return 0;
}

static int mtk_warpa_fe_ioctl_put_handle(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_cv_common_put_handle put_handle;

	dev_dbg(warpa_fe->dev, "PUT_HANDLE is called.");
	CHK_ERR(copy_from_user(&put_handle, (void *)arg, sizeof(put_handle)));
	CHK_ERR(mtk_cv_common_put_handle(put_handle.handle));
	if (put_handle.buf_type < MTK_WARPA_FE_BUF_TYPE_MAX) {
		warpa_fe->buf[put_handle.buf_type] = NULL;
	}

	return 0;
}

static int mtk_warpa_fe_ioctl_set_fe_param(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_warpa_fe_set_fe_param fe_param;

	dev_dbg(warpa_fe->dev, "SET_FE_PARAM is called.");

	CHK_ERR(copy_from_user(&fe_param, (void *)arg, sizeof(fe_param)));
	warpa_fe->fe_flat_enable =  fe_param.flat_enable;
	warpa_fe->fe_harris_kappa = fe_param.harris_kappa;
	warpa_fe->fe_th_g = fe_param.th_grad;
	warpa_fe->fe_th_c = fe_param.th_cr;
	warpa_fe->fe_cr_val_sel = fe_param.cr_val_sel;
	dev_dbg(warpa_fe->dev,
		"fe flat_en:%d, harris:%d, th_g:%d, th_c:%d, cr_val:%d\n",
		warpa_fe->fe_flat_enable, warpa_fe->fe_harris_kappa,
		warpa_fe->fe_th_g, warpa_fe->fe_th_c, warpa_fe->fe_cr_val_sel);
	CHK_ERR(mtk_fe_set_params(warpa_fe->dev_fe, NULL,
				  warpa_fe->fe_flat_enable, warpa_fe->fe_harris_kappa, warpa_fe->fe_th_g,
				  warpa_fe->fe_th_c, warpa_fe->fe_cr_val_sel));

	return 0;
}

static int mtk_warpa_fe_ioctl_sync_handle(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	struct mtk_cv_common_sync_handle sync_handle;

	dev_dbg(warpa_fe->dev, "SYNC_HANDLE is called.");

	CHK_ERR(copy_from_user(&sync_handle, (void *)arg, sizeof(sync_handle)));
	dev_dbg(warpa_fe->dev, "%s:SYNC_HANDLE: handle:%d dir:%d\n", __func__, sync_handle.handle, sync_handle.dir);
	CHK_ERR(mtk_cv_common_sync_handle(sync_handle.handle, sync_handle.dir));

	return 0;
}

static int mtk_warpa_fe_ioctl_set_warpmap(struct mtk_warpa_fe *warpa_fe, unsigned long arg)
{
	__u32 warpmap_cnt;
	int warp_map_idx;
	int i;

	dev_dbg(warpa_fe->dev, "SET_WARPMAP is called.");

	CHK_ERR(copy_from_user(&warpmap_cnt, (void *)arg, sizeof(__u32)));
	dev_dbg(warpa_fe->dev, "%s: warpmap_cnt:%d\n", __func__, warpmap_cnt);
	if (warpmap_cnt >= MTK_WARPA_FE_WARPMAP_NUM) {
		dev_err(warpa_fe->dev, "%s: Invalid warpmap_cnt:%u\n", __func__, warpmap_cnt);
		return -EINVAL;
	}
	warpa_fe->warpmap_cnt = warpmap_cnt;
	for (i = 0; i < warpa_fe->img_num; i++) {
		warp_map_idx = MTK_WARPA_FE_GET_BUF_NUM(MTK_WARPA_FE_BUF_TYPE_WARPA_MAP_MIN, warpa_fe->warpmap_cnt, i);
		sync_for_device(warpa_fe->dev_warpa, warpa_fe->buf[warp_map_idx]);
	}

	return 0;
}

static long mtk_warpa_fe_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	struct mtk_warpa_fe *warpa_fe;
	int ret = 0;

	if (!flip) {
		pr_err("mtk_warpa_fe_ioctl flip is NULL!\n");
		return -EFAULT;
	}
	warpa_fe = flip->private_data;

	switch (cmd) {
	case MTK_WARPA_FE_IOCTL_IMPORT_FD_TO_HANDLE:
		ret = mtk_warpa_fe_ioctl_import_fd_to_handle(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_CTRL_MASK:
		ret = mtk_warpa_fe_ioctl_set_ctrl_mask(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_WARPA:
		ret = mtk_warpa_fe_ioctl_config_warpa(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_FE:
		ret = mtk_warpa_fe_ioctl_config_fe(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_WDMA:
		ret = mtk_warpa_fe_ioctl_config_wdma(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_RSZ:
		ret = mtk_warpa_fe_ioctl_config_rsz(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_CONFIG_PADDING:
		ret = mtk_warpa_fe_ioctl_config_padding(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_BUF:
		ret = mtk_warpa_fe_ioctl_set_buf(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_STREAMON:
		ret = mtk_warpa_fe_ioctl_streamon(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_STREAMOFF:
		ret = mtk_warpa_fe_ioctl_streamoff(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_TRIGGER:
		ret = mtk_warpa_fe_ioctl_trigger(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_RESET:
		ret = mtk_warpa_fe_ioctl_reset(warpa_fe);
		break;
	case MTK_WARPA_FE_IOCTL_PUT_HANDLE:
		ret = mtk_warpa_fe_ioctl_put_handle(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_FE_PARAM:
		ret = mtk_warpa_fe_ioctl_set_fe_param(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SYNC_HANDLE:
		ret = mtk_warpa_fe_ioctl_sync_handle(warpa_fe, arg);
		break;
	case MTK_WARPA_FE_IOCTL_SET_WARPMAP:
		ret = mtk_warpa_fe_ioctl_set_warpmap(warpa_fe, arg);
		break;
	default:
		dev_err(warpa_fe->dev, "warpa_fe_ioctl: no such command!\n");
		return -EINVAL;
	}

	return ret;
}

static int mtk_warpa_fe_open(struct inode *inode, struct file *flip)
{
	struct mtk_warpa_fe *warpa_fe = container_of(inode->i_cdev, struct mtk_warpa_fe, chrdev);

	flip->private_data = warpa_fe;
	return 0;
}

static int mtk_warpa_fe_release(struct inode *inode, struct file *flip)
{
	flip->private_data = NULL;

	return 0;
}

static const struct file_operations mtk_warpa_fe_fops = {
	.owner = THIS_MODULE,
	.open = mtk_warpa_fe_open,
	.release = mtk_warpa_fe_release,
	.unlocked_ioctl = mtk_warpa_fe_ioctl,
};

static int mtk_warpa_fe_reg_chardev(struct mtk_warpa_fe *warpa_fe)
{
	struct device *dev;
	char warpa_fe_dev_name[] = "mtk_warpa_fe";
	int ret;

	pr_debug("warpa_fe_reg_chardev\n");

	ret = alloc_chrdev_region(&warpa_fe->devt, 0, 1, warpa_fe_dev_name);
	if (ret < 0) {
		pr_err("warpa_fe_reg_chardev: alloc_chrdev_region fail, %d\n", ret);
		return ret;
	}

	pr_debug("warpa_fe_reg_chardev: MAJOR/MINOR = 0x%08x\n", warpa_fe->devt);

	/* Attatch file operation. */
	cdev_init(&warpa_fe->chrdev, &mtk_warpa_fe_fops);
	warpa_fe->chrdev.owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(&warpa_fe->chrdev, warpa_fe->devt, 1);
	if (ret < 0) {
		pr_err("warpa_fe_reg_chardev: attatch file operation fail, %d\n", ret);
		unregister_chrdev_region(warpa_fe->devt, 1);
		return -EIO;
	}

	/* Create class register */
	warpa_fe->dev_class = class_create(THIS_MODULE, warpa_fe_dev_name);
	if (IS_ERR(warpa_fe->dev_class)) {
		ret = PTR_ERR(warpa_fe->dev_class);
		pr_err("Unable to create class, err = %d\n", ret);
		return -EIO;
	}

	dev = device_create(warpa_fe->dev_class, NULL, warpa_fe->devt, NULL, warpa_fe_dev_name);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("Failed to create device: /dev/%s, err = %d", warpa_fe_dev_name, ret);
		return -EIO;
	}

	pr_debug("warpa_fe_reg_chardev done\n");

	return 0;
}

static void mtk_warpa_fe_unreg_chardev(struct mtk_warpa_fe *warpa_fe)
{
	device_destroy(warpa_fe->dev_class, warpa_fe->devt);
	class_destroy(warpa_fe->dev_class);
	cdev_del(&warpa_fe->chrdev);
	unregister_chrdev_region(warpa_fe->devt, 1);
}

static int mtk_warpa_fe_get_device(struct device *dev, char *compatible, int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev, "warpa_fe_get_device: could not find %s %d\n", compatible, idx);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev, "warpa_fe_get_device: waiting for device %s\n", node->full_name);
		return -EPROBE_DEFER;
	}

	*child_dev = &pdev->dev;

	return 0;
}

int mtk_warpa_fe_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_warpa_fe *warpa_fe;

	pr_debug("warpa_fe probe start\n");

	warpa_fe = devm_kzalloc(dev, sizeof(*warpa_fe), GFP_KERNEL);
	if (!warpa_fe)
		return -ENOMEM;

	warpa_fe->dev = dev;

	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,mmsys_common_top", 0, &warpa_fe->dev_mmsys_cmmn_top));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,mmsys_core_top", 0, &warpa_fe->dev_mmsys_core_top));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,mutex", 0, &warpa_fe->dev_mutex));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,sysram", 0, &warpa_fe->dev_sysram));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,rbfc", 0, &warpa_fe->dev_rbfc));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,warpa", 0, &warpa_fe->dev_warpa));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,fe", 0, &warpa_fe->dev_fe));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,wdma", 0, &warpa_fe->dev_wdma0));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,wdma", 1, &warpa_fe->dev_wdma1));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,wdma", 2, &warpa_fe->dev_wdma2));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,padding", 0, &warpa_fe->dev_padding0));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,padding", 1, &warpa_fe->dev_padding1));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,padding", 2, &warpa_fe->dev_padding2));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,resizer", 0, &warpa_fe->dev_p_rsz0));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,resizer", 1, &warpa_fe->dev_p_rsz1));
	CHK_ERR(mtk_warpa_fe_get_device(dev, "mediatek,timestamp", 0, &warpa_fe->dev_ts));
	CHK_ERR(mtk_warpa_fe_reg_chardev(warpa_fe));

	of_property_read_u32_array(dev->of_node, "gce-events", warpa_fe->cmdq_events, MTK_WARPA_FE_CMDQ_EVENT_MAX);

	init_waitqueue_head(&warpa_fe->trigger_wait_queue);

	warpa_fe->coef_tbl.idx = 4;
	warpa_fe->border_color.enable = false;
	warpa_fe->proc_mode = MTK_WARPA_PROC_MODE_LR;
	warpa_fe->warpa_out_mode = MTK_WARPA_OUT_MODE_DIRECT_LINK;
	warpa_fe->reset = 0;
	warpa_fe->blk_sz = 32;

	warpa_fe->fe_flat_enable = 1;
	warpa_fe->fe_harris_kappa = 4;
	warpa_fe->fe_th_g = 2;
	warpa_fe->fe_th_c = 0;
	warpa_fe->fe_cr_val_sel = 0;

	platform_set_drvdata(pdev, warpa_fe);
	pr_debug("warpa_fe probe success!\n");

	return 0;
}

int mtk_warpa_fe_remove(struct platform_device *pdev)
{
	struct mtk_warpa_fe *warpa_fe = platform_get_drvdata(pdev);

	mtk_warpa_fe_unreg_chardev(warpa_fe);
	CHK_ERR(mtk_mutex_put(warpa_fe->mutex));

	return 0;
}

MODULE_LICENSE("GPL");
