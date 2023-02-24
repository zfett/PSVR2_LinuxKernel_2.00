/*
 * Copyright (c) 2019 MediaTek Inc.
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

#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <soc/mediatek/mtk_fm.h>
#include <soc/mediatek/mtk_mmsys_cmmn_top.h>

#include <mtk_fm_dev.h>
#include <mtk_warpa_fe.h>

#define ENABLE_AUTO_CACHE_SYNC_FOR_DEVICE
#define CHK_ERR(func) \
	do { \
		int ret = func; \
		if (ret) { \
			dev_err(fm_dev->dev, "%s(line:%d) fail. ret:%d\n", __func__, __LINE__, ret); \
			return ret; \
		} \
	} while (0)

#ifndef CONFIG_MTK_DEBUGFS
#define dump_fm_reg(x)
#endif

enum mtk_fm_dev_fm_mask_hw_buf_index {
	MTK_FM_DEV_FM_MASK_HW_BUF_INDEX_TEMPORAL = 0,
	MTK_FM_DEV_FM_MASK_HW_BUF_INDEX_SPATIAL,
	MTK_FM_DEV_FM_MASK_HW_BUF_INDEX_MAX,
};

enum mtk_fm_dev_cmdq_event {
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_FE,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA0,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA1,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_WDMA2,
	MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_FM,
	MTK_FM_DEV_CMDQ_EVENT_MAX,
};

struct mtk_fm_dev {
	struct device *dev;
	struct device *dev_fm;
	struct device *dev_mmsys_cmmn_top;
	struct device *dev_sysram;
	dev_t devt;
	struct cdev chrdev;
	struct class *dev_class;
	struct mtk_cv_common_buf *buf[MTK_FM_DEV_BUF_TYPE_MAX];
	int param_id; /* enum FM_DEV_FM_PARAM_ID */
	void *fm_mask_tbl[MTK_FM_DEV_FM_PARAM_ID_MAX];
	u32 fm_mask_tbl_size[MTK_FM_DEV_FM_PARAM_ID_MAX];
	int prev_mask_tbl_id[MTK_FM_DEV_FM_MASK_HW_BUF_INDEX_MAX]; /* enum FM_DEV_FM_PARAM_ID */
	u32 fmdevmask;
	int img_num;
	int fe_w;
	int fe_h;
	int blk_sz;
	int blk_nr;
	int sr_type;
	int blk_type;
	int img_type;
	u32 zncc_th;
	struct mtk_fm_reqinterval req_interval;
	struct cmdq_client *cmdq_client;
	u32 cmdq_events[MTK_FM_DEV_CMDQ_EVENT_MAX];
	int wait_trigger;
};

#ifdef ENABLE_AUTO_CACHE_SYNC_FOR_DEVICE
static inline void sync_for_device(struct device *dev, struct mtk_cv_common_buf *buf)
{
	if (!buf->is_sysram) {
		dma_sync_sg_for_device(dev, buf->sg->sgl, buf->sg->nents, DMA_TO_DEVICE);
	}
}
#else
#define sync_for_device(dev, buf)
#endif

static int mtk_fm_dev_set_mask(struct mtk_fm_dev *fm_dev, struct fm_dev_fm_mask_param *fm_mask_param)
{
	u32 mask_blk_w;
	u32 mask_blk_h;
	u32 mask_tbl_size;
	int param_id = fm_mask_param->id;
	int i;

	mask_blk_w = (fm_mask_param->fm_mask_w + 3) / 4;
	mask_blk_w = ((mask_blk_w + 1) / 2) * 2;
	mask_blk_h = (fm_mask_param->fm_mask_h + 7) / 8;
	mask_tbl_size = mask_blk_w * mask_blk_h;

	dev_dbg(fm_dev->dev, "%s: param_id = %d\n", __func__, fm_mask_param->id);
	dev_dbg(fm_dev->dev, "%s: mask_tbl_size = %d / fm_mask_param->fm_mask_size = %d\n", __func__,
		mask_tbl_size, fm_mask_param->fm_mask_size);

	if (param_id < 0 || param_id >= MTK_FM_DEV_FM_PARAM_ID_MAX) {
		dev_err(fm_dev->dev, "%s: wrong id:%d\n", __func__, param_id);
		return -EINVAL;
	}

	if (fm_mask_param->fm_mask_size > mask_tbl_size) {
		dev_err(fm_dev->dev, "%s: fm_mask_size is too big.\n", __func__);
		return -EINVAL;
	}

	if (fm_dev->fm_mask_tbl_size[param_id] != mask_tbl_size) {
		kfree(fm_dev->fm_mask_tbl[param_id]);
		fm_dev->fm_mask_tbl[param_id] = NULL;
		fm_dev->fm_mask_tbl_size[param_id] = 0;
	}

	if (!fm_dev->fm_mask_tbl[param_id]) {
		fm_dev->fm_mask_tbl_size[param_id] = mask_tbl_size;
		fm_dev->fm_mask_tbl[param_id] = kzalloc(roundup(mask_tbl_size * sizeof(u32), PAGE_SIZE), GFP_KERNEL);
	}

	dev_dbg(fm_dev->dev, "%s: fm_dev->fm_msk_tbl[%d]: %p\n", __func__, param_id, fm_dev->fm_mask_tbl[param_id]);
	CHK_ERR(copy_from_user(fm_dev->fm_mask_tbl[param_id], fm_mask_param->fm_mask, fm_mask_param->fm_mask_size * sizeof(u32)));

	for (i = 0; i < MTK_FM_DEV_FM_MASK_HW_BUF_INDEX_MAX; i++) {
		if (fm_dev->prev_mask_tbl_id[i] == param_id) {
			fm_dev->prev_mask_tbl_id[i] = -1;
		}
	}

	return 0;
}

#define MAX_SEARCH_CENTER_NUM 400
static int mtk_fm_dev_set_search_center(struct mtk_fm_dev *fm_dev)
{
	struct mtk_fm_search_center sc;
	struct mtk_cv_common_buf *buf;
	int param_id = fm_dev->param_id;
	int sc_idx;
	int size;
	u32 *ptr;

	dev_dbg(fm_dev->dev, "%s: param_id = %d\n", __func__, param_id);

	sc_idx = MTK_FM_DEV_BUF_TYPE_FM_IN_SEARCH_CENTER_MIN + param_id;
	buf = fm_dev->buf[sc_idx];

	if (!buf) {
		dev_err(fm_dev->dev, "%s: buf_id:%d is NULL\n", __func__, sc_idx);
		return -ENOMEM;
	}

	dev_dbg(fm_dev->dev, "%s: buf[sc_idx]->dma_buf:%p\n", __func__, fm_dev->buf[sc_idx]->dma_buf);
	dev_dbg(fm_dev->dev, "%s: buf[sc_idx]->dma_buf->size: 0x%lx\n", __func__, fm_dev->buf[sc_idx]->dma_buf->size);
	size = fm_dev->buf[sc_idx]->dma_buf->size;
	size = (size <= sizeof(u32) * MAX_SEARCH_CENTER_NUM) ? size : sizeof(u32) * MAX_SEARCH_CENTER_NUM;

	if (!buf->kvaddr) {
		ptr = (u32 *)pa_to_vaddr(buf, size, true);
		if (!ptr) {
			return -ENOMEM;
		}
	} else {
		ptr = buf->kvaddr;
		sync_for_device(fm_dev->dev_fm, buf);
	}

	sc.sc = ptr;
	sc.size = size / sizeof(u32);
	CHK_ERR(mtk_fm_set_search_center(fm_dev->dev_fm, NULL, &sc));

	return 0;
}

static int mtk_fm_dev_module_config(struct mtk_fm_dev *fm_dev)
{
	struct mtk_fm_mask_tbl mask_tbl;
	u32 hw_buf_index;

	/* set blk_sz */
	dev_dbg(fm_dev->dev, "set blk:%d\n", fm_dev->blk_type);
	CHK_ERR(mtk_fm_set_blk_type(fm_dev->dev_fm, NULL, fm_dev->blk_type));

	/* set region */
	dev_dbg(fm_dev->dev, "set region w:%d h:%d, blk_nr:%d\n", fm_dev->fe_w, fm_dev->fe_h, fm_dev->blk_nr);
	CHK_ERR(mtk_fm_set_region(fm_dev->dev_fm, NULL, fm_dev->fe_w, fm_dev->fe_h, fm_dev->blk_nr));

	/* set sr type */
	dev_dbg(fm_dev->dev, "set sr:%d\n", fm_dev->sr_type);
	CHK_ERR(mtk_fm_set_sr_type(fm_dev->dev_fm, NULL, fm_dev->sr_type));

	if (!fm_dev->fm_mask_tbl[fm_dev->param_id] ||
	    !fm_dev->fm_mask_tbl_size[fm_dev->param_id]) {
		dev_dbg(fm_dev->dev, "fm config set mask error!\n");
	}

	if (fm_dev->sr_type == MTK_FM_SR_TYPE_SPATIAL) {
		hw_buf_index = MTK_FM_DEV_FM_MASK_HW_BUF_INDEX_SPATIAL;
	} else {
		hw_buf_index = MTK_FM_DEV_FM_MASK_HW_BUF_INDEX_TEMPORAL;
	}

	CHK_ERR(mtk_fm_set_mask_tbl_sw_idx(fm_dev->dev_fm, NULL, hw_buf_index));

	if (fm_dev->prev_mask_tbl_id[hw_buf_index] != fm_dev->param_id) {
		dev_dbg(fm_dev->dev, "%s: Update active mask(%d) in hw_buf_index(%d)\n",
			__func__, fm_dev->param_id, hw_buf_index);
		mask_tbl.mask_tbl = (u32 *)fm_dev->fm_mask_tbl[fm_dev->param_id];
		mask_tbl.size = fm_dev->fm_mask_tbl_size[fm_dev->param_id];
		CHK_ERR(mtk_fm_set_mask_tbl(fm_dev->dev_fm, NULL, &mask_tbl));
		fm_dev->prev_mask_tbl_id[hw_buf_index] = fm_dev->param_id;
	} else {
		dev_dbg(fm_dev->dev, "%s: Reuse active mask(%d) in hw_buf_index(%d)\n",
			__func__, fm_dev->param_id, hw_buf_index);
	}

	CHK_ERR(mtk_fm_set_mask_tbl_hw_idx(fm_dev->dev_fm, NULL, hw_buf_index));

	if (fm_dev->sr_type == MTK_FM_SR_TYPE_TEMPORAL_PREDICTION) {
		CHK_ERR(mtk_fm_dev_set_search_center(fm_dev));
		dev_dbg(fm_dev->dev, "fm config search center!\n");
	}

	/* Set req interval*/
	dev_dbg(fm_dev->dev, "set req interval img:0x%x desc:0x%x point:0x%x fmo:0x%x zncc:0x%x\n",
		fm_dev->req_interval.img_req_interval,
		fm_dev->req_interval.desc_req_interval, fm_dev->req_interval.point_req_interval,
		fm_dev->req_interval.point_req_interval, fm_dev->req_interval.zncc_req_interval);
	CHK_ERR(mtk_fm_set_req_interval(fm_dev->dev_fm, NULL, fm_dev->req_interval.img_req_interval,
					fm_dev->req_interval.desc_req_interval, fm_dev->req_interval.point_req_interval,
					fm_dev->req_interval.fmo_req_interval, fm_dev->req_interval.zncc_req_interval));

	dev_dbg(fm_dev->dev, "fm_set fe_w:%d fe_h:%d sr_type:%d\n", fm_dev->fe_w, fm_dev->fe_h, fm_dev->sr_type);

	return 0;
}

static int mtk_fm_dev_path_connect(struct mtk_fm_dev *fm_dev)
{
	dev_dbg(fm_dev->dev, "fm0_path_connect\n");

	fm_dev->cmdq_client = cmdq_mbox_create(fm_dev->dev, 0);
	if (IS_ERR(fm_dev->cmdq_client)) {
		dev_err(fm_dev->dev, "failed to create mailbox cmdq_client\n");
		return PTR_ERR(fm_dev->cmdq_client);
	}

	return 0;
}

static int mtk_fm_dev_path_disconnect(struct mtk_fm_dev *fm_dev)
{
	dev_dbg(fm_dev->dev, "fm0_path_disconnect\n");

	CHK_ERR(cmdq_mbox_destroy(fm_dev->cmdq_client));

	return 0;
}

static int mtk_fm_dev_module_set_buf(struct mtk_fm_dev *fm_dev, struct cmdq_pkt *handle, u32 img_type, u32 sr_type)
{
	int img_idx, point_idx, desc_idx, fmo_point_idx, fmo_desc_idx;
	dma_addr_t addr, addr_img, addr_point, addr_desc;
	int param_id = fm_dev->param_id;

	/* set input buffer */
	point_idx = MTK_FM_DEV_BUF_TYPE_FM_IN_POINT_MIN + param_id;
	if (!fm_dev->buf[point_idx]) {
		dev_dbg(fm_dev->dev, "fm_dev->buf[point_idx: %d] is null\n", point_idx);
		return -EINVAL;
	}
	addr_point = fm_dev->buf[point_idx]->dma_addr;
	sync_for_device(fm_dev->dev_fm, fm_dev->buf[point_idx]);
	if (!fm_dev->buf[point_idx]->is_sysram) {
		dev_dbg(fm_dev->dev, "FMIn: FE Point is DRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_FP, MTK_MMSYS_CMMN_DRAM));
	} else {
		dev_dbg(fm_dev->dev, "FMIn: FE Point is SYSRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_FP, MTK_MMSYS_CMMN_SYSRAM));
	}

	desc_idx = MTK_FM_DEV_BUF_TYPE_FM_IN_DESC_MIN + param_id;
	if (!fm_dev->buf[desc_idx]) {
		dev_dbg(fm_dev->dev, "fm_dev->buf[desc_idx: %d] is null\n", desc_idx);
		return -EINVAL;
	}
	addr_desc = fm_dev->buf[desc_idx]->dma_addr;
	sync_for_device(fm_dev->dev_fm, fm_dev->buf[desc_idx]);
	if (!fm_dev->buf[desc_idx]->is_sysram) {
		dev_dbg(fm_dev->dev, "FMIn: FE Desc is DRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_FD, MTK_MMSYS_CMMN_DRAM));
	} else {
		dev_dbg(fm_dev->dev, "FMIn: FE Desc is SYSRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_FD, MTK_MMSYS_CMMN_SYSRAM));
	}

	img_idx = MTK_FM_DEV_BUF_TYPE_FM_IN_IMG_MIN + param_id;
	if (!fm_dev->buf[img_idx]) {
		dev_dbg(fm_dev->dev, "fm_dev->buf[img_idx: %d] is null\n", img_idx);
		return -EINVAL;
	}
	addr_img = fm_dev->buf[img_idx]->dma_addr + fm_dev->buf[img_idx]->offset;
	sync_for_device(fm_dev->dev_fm, fm_dev->buf[img_idx]);
	if (!fm_dev->buf[img_idx]->is_sysram) {
		dev_dbg(fm_dev->dev, "FMIn: Img is DRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_IMG, MTK_MMSYS_CMMN_DRAM));
	} else {
		dev_dbg(fm_dev->dev, "FMIn: Img is SYSRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_IMG, MTK_MMSYS_CMMN_SYSRAM));
	}

	dev_dbg(fm_dev->dev, "set buf MTK_FM_IN_BUF_IMG @0x%llx, pitch:%d\n",
		addr_img, fm_dev->buf[img_idx]->pitch);
	CHK_ERR(mtk_fm_set_multi_in_buf(fm_dev->dev_fm, handle, MTK_FM_IN_BUF_IMG,
					addr_img, fm_dev->buf[img_idx]->pitch));
	dev_dbg(fm_dev->dev, "set buf MTK_FM_IN_BUF_DESCRIPTOR @0x%llx\n", addr_desc);
	CHK_ERR(mtk_fm_set_multi_in_buf(fm_dev->dev_fm, handle, MTK_FM_IN_BUF_DESCRIPTOR,
					addr_desc, fm_dev->buf[desc_idx]->pitch));
	dev_dbg(fm_dev->dev, "set buf MTK_FM_IN_BUF_POINT @0x%llx\n", addr_point);
	CHK_ERR(mtk_fm_set_multi_in_buf(fm_dev->dev_fm, handle, MTK_FM_IN_BUF_POINT,
					addr_point, fm_dev->buf[point_idx]->pitch));

	/* set output buffer */
	fmo_point_idx = MTK_FM_DEV_BUF_TYPE_FM_OUT_POINT_MIN + param_id;
	if (!fm_dev->buf[fmo_point_idx]) {
		dev_dbg(fm_dev->dev, "fm_dev->buf[fmo_point_idx: %d] is null\n", fmo_point_idx);
		return -EINVAL;
	}
	addr = fm_dev->buf[fmo_point_idx]->dma_addr;
	dev_dbg(fm_dev->dev, "set buf MTK_FM_DEV_BUF_TYPE_FM_OUT @0x%llx\n", addr);
	CHK_ERR(mtk_fm_set_multi_out_buf(fm_dev->dev_fm, handle, MTK_FM_OUT_BUF_FMO, addr));
	if (!fm_dev->buf[fmo_point_idx]->is_sysram) {
		dev_dbg(fm_dev->dev, "FMO is DRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_FMO, MTK_MMSYS_CMMN_DRAM));
	} else {
		dev_dbg(fm_dev->dev, "FMO is SYSRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_FMO, MTK_MMSYS_CMMN_SYSRAM));
	}

	fmo_desc_idx = MTK_FM_DEV_BUF_TYPE_FM_OUT_DESC_MIN + param_id;
	if (!fm_dev->buf[fmo_desc_idx]) {
		dev_dbg(fm_dev->dev, "fm_dev->buf[fmo_desc_idx: %d] is null\n", fmo_desc_idx);
		return -EINVAL;
	}
	addr = fm_dev->buf[fmo_desc_idx]->dma_addr;
	dev_dbg(fm_dev->dev, "set buf MTK_FM_DEV_BUF_TYPE_FM_ZNCC @0x%llx\n", addr);
	CHK_ERR(mtk_fm_set_multi_out_buf(fm_dev->dev_fm, handle, MTK_FM_OUT_BUF_ZNCC_SUBPIXEL, addr));
	if (!fm_dev->buf[fmo_desc_idx]->is_sysram) {
		dev_dbg(fm_dev->dev, "FM ZNCC is DRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_ZNCC, MTK_MMSYS_CMMN_DRAM));
	} else {
		dev_dbg(fm_dev->dev, "FM ZNCC is SYSRAM\n");
		CHK_ERR(mtk_mmsys_cmmn_top_sel_mem(fm_dev->dev_mmsys_cmmn_top, MTK_MMSYS_CMMN_FM_ZNCC, MTK_MMSYS_CMMN_SYSRAM));
	}

	return 0;
}

static int mtk_fm_dev_module_power_on(struct mtk_fm_dev *fm_dev)
{
	return mtk_fm_power_on(fm_dev->dev_fm);
}

static int mtk_fm_dev_module_power_off(struct mtk_fm_dev *fm_dev)
{
	return mtk_fm_power_off(fm_dev->dev_fm);
}

static int mtk_fm_dev_module_start(struct mtk_fm_dev *fm_dev)
{
	return mtk_fm_start(fm_dev->dev_fm, NULL);
}

static int mtk_fm_dev_path_trigger_wait(struct mtk_fm_dev *fm_dev);

static int mtk_fm_dev_module_stop(struct mtk_fm_dev *fm_dev)
{
	if (fm_dev->wait_trigger == 1) {
		mtk_fm_dev_path_trigger_wait(fm_dev);
	}

	CHK_ERR(mtk_fm_stop(fm_dev->dev_fm, NULL));
	CHK_ERR(mtk_fm_reset(fm_dev->dev_fm));

#ifdef SUPPORT_BLK_NUM_CHANGE
	/* when change fm blk_num, need to do this */
	CHK_ERR(mtk_mmsys_cmmn_top_mod_reset(fm_dev->dev_mmsys_cmmn_top, MDP_FM));
#endif

	CHK_ERR(mtk_fm_get_dma_idle(fm_dev->dev_fm, NULL));

	return 0;
}

static int mtk_fm_dev_open(struct inode *inode, struct file *flip)
{
	struct mtk_fm_dev *fm = container_of(inode->i_cdev, struct mtk_fm_dev, chrdev);

	flip->private_data = fm;

	return 0;
}

static int mtk_fm_dev_release(struct inode *inode, struct file *flip)
{
	int i;
	struct mtk_fm_dev *fm_dev = flip->private_data;

	for (i = 0; i < MTK_FM_DEV_FM_PARAM_ID_MAX; i++) {
		kfree(fm_dev->fm_mask_tbl[i]);
		fm_dev->fm_mask_tbl[i] = NULL;
		fm_dev->fm_mask_tbl_size[i] = 0;
	}

	return 0;
}

static int mtk_fm_dev_path_wait_done(struct mtk_fm_dev *fm_dev)
{
#ifdef CV_CMDQ_ENABLE
	u32 fm_framem_done_event = fm_dev->cmdq_events[MTK_FM_DEV_CMDQ_EVENT_FRAME_DONE_FM];
	struct cmdq_pkt *cmdq_pkt;

	dev_dbg(fm_dev->dev, "%s is called.\n", __func__);
	cmdq_pkt_create(&cmdq_pkt);
	cmdq_pkt_wfe(cmdq_pkt, fm_framem_done_event);
	if (cmdq_pkt_flush(fm_dev->cmdq_client, cmdq_pkt) != 0) {
		dev_err(fm_dev->dev, "%s: cmdq_pkt_flush failed.\n", __func__);
		dump_fm_reg(fm_dev->dev_fm);
		return -1;
	}
	dev_dbg(fm_dev->dev, "%s: fm done.\n", __func__);
	cmdq_pkt_destroy(cmdq_pkt);
#else
	u32 fm_done;
	int i;

	dev_dbg(fm_dev->dev, "start wait fm done!\n");

	for (i = 0; i < 300; i++) {
		mtk_fm_get_fm_done(fm_dev->dev_fm, &fm_done);
		if (fm_done)
			break;

		/*usleep_range(500, 1000);*/
		msleep(100);
	}

	if (i == 300) {
		dev_err(fm_dev->dev, "fm_trigger: MTK_FM_PROP_FM_DONE timeout\n");
	} else {
		dev_dbg(fm_dev->dev, "fm done %d!\n", i);
	}
#endif

	return 0;
}

static int mtk_fm_buffer_sync_for_cpu(struct mtk_fm_dev *fm_dev)
{
	int param_id = fm_dev->param_id;
	int buf_type;

	buf_type = MTK_FM_DEV_BUF_TYPE_FM_OUT_POINT_MIN + param_id;
	if (!fm_dev->buf[buf_type]->is_sysram) {
		dma_sync_sg_for_cpu(fm_dev->dev_fm, fm_dev->buf[buf_type]->sg->sgl,
				    fm_dev->buf[buf_type]->sg->nents, DMA_FROM_DEVICE);
	}

	buf_type = MTK_FM_DEV_BUF_TYPE_FM_OUT_DESC_MIN + param_id;
	if (!fm_dev->buf[buf_type]->is_sysram) {
		dma_sync_sg_for_cpu(fm_dev->dev_fm, fm_dev->buf[buf_type]->sg->sgl,
				    fm_dev->buf[buf_type]->sg->nents, DMA_FROM_DEVICE);
	}

	return 0;
}

static int mtk_fm_dev_path_trigger_start(struct mtk_fm_dev *fm_dev, bool onoff)
{
	dev_dbg(fm_dev->dev, "fm_dev_trigger_start: start %d\n", onoff);
	if (onoff) {
		CHK_ERR(mtk_fm_set_zncc_threshold(fm_dev->dev_fm, NULL, fm_dev->zncc_th));
		CHK_ERR(mtk_fm_dev_module_config(fm_dev));
		CHK_ERR(mtk_fm_dev_module_set_buf(fm_dev, NULL, fm_dev->img_type, fm_dev->sr_type));
		CHK_ERR(mtk_fm_dev_module_start(fm_dev));
		fm_dev->wait_trigger = 1;
	}

	return 0;
}

static int mtk_fm_dev_path_trigger_wait(struct mtk_fm_dev *fm_dev)
{
	dev_dbg(fm_dev->dev, "fm_dev_trigger_wait: start\n");

	fm_dev->wait_trigger = 0;

	CHK_ERR(mtk_fm_dev_path_wait_done(fm_dev));
	CHK_ERR(mtk_fm_dev_module_stop(fm_dev));
	CHK_ERR(mtk_fm_buffer_sync_for_cpu(fm_dev));

	dev_dbg(fm_dev->dev, "fm_dev_trigger_wait: end\n");

	return 0;
}

static int mtk_fm_dev_get_device(struct device *dev, char *compatible, int idx, struct device **child_dev)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_parse_phandle(dev->of_node, compatible, idx);
	if (!node) {
		dev_err(dev, "fm_dev_get_device: could not find %s %d\n", compatible, idx);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(node);
	if (!pdev || !pdev->dev.driver) {
		dev_warn(dev, "fm_dev_get_device: waiting for device %s\n", node->full_name);
		return -EPROBE_DEFER;
	}

	*child_dev = &pdev->dev;

	return 0;
}

static int mtk_fm_dev_ioctl_import_fd_to_handle(struct mtk_fm_dev *fm_dev, unsigned long arg)
{
	struct device *dev = fm_dev->dev_fm;
	struct mtk_cv_common_buf_handle handle;

	dev_dbg(dev, "%s:IMPORT_FD_TO_HANDLE\n", __func__);

	CHK_ERR(copy_from_user((void *)&handle, (void *)arg, sizeof(handle)));
	CHK_ERR(mtk_cv_common_fd_to_handle(dev, handle.fd, &handle.handle));
	CHK_ERR(copy_to_user((void *)arg, &handle, sizeof(handle)));

	return 0;
}

static int mtk_fm_dev_ioctl_set_buf(struct mtk_fm_dev *fm_dev, unsigned long arg)
{
	struct mtk_cv_common_set_buf buf;
	struct mtk_cv_common_buf *buf_handle;
	struct device *dev = fm_dev->dev_fm;

	dev_dbg(dev, "%s:SET_BUF\n", __func__);

	CHK_ERR(copy_from_user((void *)&buf, (void *)arg, sizeof(buf)));
	if (buf.buf_type >= MTK_FM_DEV_BUF_TYPE_MAX) {
		dev_err(dev, "MTK_FM_DEV_IOCTL_SET_BUF: buf type %d error\n", buf.buf_type);
		return -EINVAL;
	}
	buf_handle = mtk_cv_common_get_buf_from_handle(buf.handle);
	buf_handle->pitch = buf.pitch;
	buf_handle->offset = buf.offset;
	buf_handle->format = buf.format;

	dev_dbg(dev, "set FM_DEV buf %d : kvaddr = %p, dma_addr = %pad, pitch:%d, offset:%d, format:%d\n",
		buf.buf_type, (void *)buf_handle->kvaddr, &buf_handle->dma_addr,
		buf_handle->pitch, buf_handle->offset, buf_handle->format);

	fm_dev->buf[buf.buf_type] = buf_handle;

	return 0;
}

static int mtk_fm_dev_ioctl_set_fm_prop(struct mtk_fm_dev *fm_dev, unsigned long arg)
{
	struct mtk_fm_dev_fm_parm parm;

	dev_dbg(fm_dev->dev, "%s:SET_FM_PROP\n", __func__);

	CHK_ERR(copy_from_user((void *)&parm, (void *)arg, sizeof(parm)));
	fm_dev->fmdevmask = parm.fmdevmask;
	fm_dev->img_num = parm.img_num;
	fm_dev->sr_type = parm.sr_type;
	fm_dev->blk_type = parm.blk_type;
	fm_dev->fe_w = parm.fe_w;
	fm_dev->fe_h = parm.fe_h;
	fm_dev->param_id = parm.param_id;
	fm_dev->zncc_th = parm.zncc_th;

	/** Setting req interval */
	fm_dev->req_interval.img_req_interval = parm.req_interval.img_req_interval;
	fm_dev->req_interval.desc_req_interval = parm.req_interval.desc_req_interval;
	fm_dev->req_interval.point_req_interval = parm.req_interval.point_req_interval;
	fm_dev->req_interval.fmo_req_interval = parm.req_interval.fmo_req_interval;
	fm_dev->req_interval.zncc_req_interval = parm.req_interval.zncc_req_interval;

	switch (fm_dev->blk_type) {
	case MTK_FM_BLK_SIZE_TYPE_32x32:
		fm_dev->blk_sz = 32;
		break;
	case MTK_FM_BLK_SIZE_TYPE_16x16:
		fm_dev->blk_sz = 16;
		break;
	case MTK_FM_BLK_SIZE_TYPE_8x8:
		fm_dev->blk_sz = 8;
		break;
	default:
		dev_err(fm_dev->dev, "fm_dev_set_prop: error invalid blk_type %d\n", fm_dev->blk_type);
		return -EINVAL;
	}
	fm_dev->blk_nr = ((fm_dev->fe_w + fm_dev->blk_sz - 1) / fm_dev->blk_sz)
		* ((fm_dev->fe_h + fm_dev->blk_sz - 1) / fm_dev->blk_sz);

	return 0;
}

static int mtk_fm_dev_ioctl_streamon(struct mtk_fm_dev *fm_dev)
{
	dev_dbg(fm_dev->dev, "%s:STREAMON\n", __func__);

	CHK_ERR(mtk_fm_dev_module_power_on(fm_dev));
	CHK_ERR(mtk_fm_dev_module_config(fm_dev));
	CHK_ERR(mtk_fm_dev_path_connect(fm_dev));
	dev_dbg(fm_dev->dev, "stream on done!\n");

	return 0;
}

static int mtk_fm_dev_ioctl_streamoff(struct mtk_fm_dev *fm_dev)
{
	dev_dbg(fm_dev->dev, "%s:STREAMOFF\n", __func__);

	CHK_ERR(mtk_fm_dev_module_stop(fm_dev));
	CHK_ERR(mtk_fm_dev_path_disconnect(fm_dev));
	CHK_ERR(mtk_fm_dev_module_power_off(fm_dev));

	return 0;
}

static int mtk_fm_dev_ioctl_trigger(struct mtk_fm_dev *fm_dev, unsigned long arg)
{
	struct mtk_fm_dev_trigger trigger;
	struct device *dev = fm_dev->dev_fm;

	CHK_ERR(copy_from_user(&trigger, (void *)arg, sizeof(trigger)));
	if (trigger.wait_flag == MTK_FM_DEV_TRIGGER_AND_WAIT ||
	    trigger.wait_flag == MTK_FM_DEV_TRIGGER_ONLY) {
		dev_dbg(dev, "%s:TRIGGER START\n", __func__);
		CHK_ERR(mtk_fm_dev_path_trigger_start(fm_dev, true));
	}

	if (trigger.wait_flag == MTK_FM_DEV_TRIGGER_AND_WAIT ||
	    trigger.wait_flag == MTK_FM_DEV_TRIGGER_WAIT) {
		dev_dbg(dev, "%s:TRIGGER WAIT\n", __func__);
		CHK_ERR(mtk_fm_dev_path_trigger_wait(fm_dev));
	}

	return 0;
}

static int mtk_fm_dev_ioctl_put_handle(struct mtk_fm_dev *fm_dev, unsigned long arg)
{
	struct device *dev = fm_dev->dev_fm;
	struct mtk_cv_common_put_handle put_handle;

	dev_dbg(dev, "%s: PUT_HANDLEF\n", __func__);

	CHK_ERR(copy_from_user(&put_handle, (void *)arg, sizeof(put_handle)));
	CHK_ERR(mtk_cv_common_put_handle(put_handle.handle));

	return 0;
}

static int mtk_fm_dev_ioctl_set_fm_mask(struct mtk_fm_dev *fm_dev, unsigned long arg)
{
	struct fm_dev_fm_mask_param fm_mask_param;
	struct device *dev = fm_dev->dev_fm;

	dev_dbg(dev, "%s:SET_FM_MASK\n", __func__);

	CHK_ERR(copy_from_user((void *)&fm_mask_param, (void *)arg, sizeof(fm_mask_param)));
	CHK_ERR(mtk_fm_dev_set_mask(fm_dev, &fm_mask_param));

	return 0;
}

static long mtk_fm_dev_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	struct mtk_fm_dev *fm_dev;
	struct device *dev;
	int ret = 0;

	if (!flip) {
		pr_err("mtk_fm_dev_ioctl flip is NULL!\n");
		return -EFAULT;
	}

	fm_dev = flip->private_data;
	dev = fm_dev->dev_fm;

	switch (cmd) {
	case MTK_FM_DEV_IOCTL_IMPORT_FD_TO_HANDLE:
		ret = mtk_fm_dev_ioctl_import_fd_to_handle(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_SET_BUF:
		ret = mtk_fm_dev_ioctl_set_buf(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_SET_FM_PROP:
		ret = mtk_fm_dev_ioctl_set_fm_prop(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_STREAMON:
		ret = mtk_fm_dev_ioctl_streamon(fm_dev);
		break;
	case MTK_FM_DEV_IOCTL_STREAMOFF:
		ret = mtk_fm_dev_ioctl_streamoff(fm_dev);
		break;
	case MTK_FM_DEV_IOCTL_TRIGGER:
		ret = mtk_fm_dev_ioctl_trigger(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_PUT_HANDLE:
		ret = mtk_fm_dev_ioctl_put_handle(fm_dev, arg);
		break;
	case MTK_FM_DEV_IOCTL_SET_FM_MASK:
		ret = mtk_fm_dev_ioctl_set_fm_mask(fm_dev, arg);
		break;
	default:
		dev_err(dev, "fm_dev_ioctl: no such command!\n");
		return -EINVAL;
	}

	return ret;
}

static const struct file_operations mtk_fm_dev_fops = {
	.owner = THIS_MODULE,
	.open = mtk_fm_dev_open,
	.release = mtk_fm_dev_release,
	.unlocked_ioctl = mtk_fm_dev_ioctl,
};

static int mtk_fm_dev_reg_chardev(struct mtk_fm_dev *fm)
{
	struct device *dev;
	char fm_dev_name[] = "mtk_fm_dev";
	int ret;

	dev_dbg(fm->dev, "fm_dev_reg_chardev\n");

	ret = alloc_chrdev_region(&fm->devt, 0, 1, fm_dev_name);
	if (ret < 0) {
		pr_err("fm_dev_reg_chardev: alloc_chrdev_region failed, %d\n", ret);
		return ret;
	}

	dev_dbg(fm->dev, "fm_dev_reg_chardev: MAJOR/MINOR = 0x%08x\n", fm->devt);

	/* Attatch file operation. */
	cdev_init(&fm->chrdev, &mtk_fm_dev_fops);
	fm->chrdev.owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(&fm->chrdev, fm->devt, 1);
	if (ret < 0) {
		pr_err("fm_dev_reg_chardev: attatch file operation failed, %d\n", ret);
		unregister_chrdev_region(fm->devt, 1);
		return -EIO;
	}

	/* Create class register */
	fm->dev_class = class_create(THIS_MODULE, fm_dev_name);
	if (IS_ERR(fm->dev_class)) {
		ret = PTR_ERR(fm->dev_class);
		pr_err("Unable to create class, err = %d\n", ret);
		return -EIO;
	}

	dev = device_create(fm->dev_class, NULL, fm->devt, NULL, fm_dev_name);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("Failed to create device: /dev/%s, err = %d", fm_dev_name, ret);
		return -EIO;
	}

	dev_dbg(fm->dev, "fm_dev_reg_chardev done\n");

	return 0;
}

int mtk_fm_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_fm_dev *fm_dev;

	fm_dev = devm_kzalloc(dev, sizeof(struct mtk_fm_dev), GFP_KERNEL);
	if (!fm_dev) {
		return -ENOMEM;
	}
	fm_dev->dev = dev;

	CHK_ERR(mtk_fm_dev_get_device(dev, "mediatek,mmsys_common_top", 0, &fm_dev->dev_mmsys_cmmn_top));
	CHK_ERR(mtk_fm_dev_get_device(dev, "mediatek,sysram", 0, &fm_dev->dev_sysram));
	CHK_ERR(mtk_fm_dev_get_device(dev, "mediatek,fm", 0, &fm_dev->dev_fm));
	CHK_ERR(mtk_fm_dev_reg_chardev(fm_dev));

	platform_set_drvdata(pdev, fm_dev);
	of_property_read_u32_array(dev->of_node, "gce-events", fm_dev->cmdq_events, MTK_FM_DEV_CMDQ_EVENT_MAX);

	return 0;
}

int mtk_fm_dev_remove(struct platform_device *pdev)
{
	struct mtk_fm_dev *fm = platform_get_drvdata(pdev);

	device_destroy(fm->dev_class, fm->devt);
	class_destroy(fm->dev_class);
	cdev_del(&fm->chrdev);
	unregister_chrdev_region(fm->devt, 1);
	return 0;
}

MODULE_LICENSE("GPL");
