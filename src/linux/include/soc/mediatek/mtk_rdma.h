/*
 * Copyright (c) 2017 MediaTek Inc.
 * Authors:
 *	Monica Wang <monica.wang@mediatek.com>
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

#ifndef MTK_RDMA_H
#define MTK_RDMA_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

/** @ingroup IP_group_rdma_external_def
 * @{
 */
#define MTK_RDMA_FRAME_COMPLETE			BIT(0)
#define MTK_RDMA_REG_UPDATE			BIT(1)
#define MTK_RDMA_UNDERRUN			BIT(2)

#define RDMA_TYPE_MDP                   (0)
#define RDMA_TYPE_PVRIC                 (1)
#define RDMA_TYPE_DISP                  (2)
/*
 * @}
 */

struct mtk_rdma_src {
	u32 mem_addr[2];
	u32 mem_size[2];
	u32 mem_ufo_len_addr[2];
};

/** @ingroup IP_group_rdma_external_struct
 * @brief basic information of module in/out Data Structure.
 */
struct mtk_rdma_config {
	/** input color format */
	u32 format;
	/** input start buffer address */
	u32 mem_addr[2];
	/** input buffer size */
	u32 mem_size[2];
	/** input ufo length information buffer address, unused in MT3612 */
	u32 mem_ufo_len_addr[2];
	/** input Y buffer pitch */
	u32 y_pitch;
	/** input UV buffer pitch */
	u32 uv_pitch;
	/** input width */
	u32 width;
	/** input height */
	u32 height;
	/** unused in MT3612 */
	bool sync_smi_channel;
};

/** @ingroup IP_group_rdma_external_struct
 * @brief Positions and height of module in/out Data Structure.
 */
struct mtk_rdma_frame_adv_config {
	/** input start positions */
	int in_x_left[4];
	/** input end positions */
	int in_x_right[4];
	/** input width */
	int in_width;
	/** input height */
	int in_height;

	/** output start positions */
	int out_x_left[4];
	/** output end positions */
	int out_x_right[4];
	/** output height */
	int out_height;
};

int mtk_rdma_larb_get(const struct device *dev, bool is_sram);
int mtk_rdma_larb_put(const struct device *dev, bool is_sram);
int mtk_rdma_power_on(struct device *dev);
int mtk_rdma_power_off(struct device *dev);
int mtk_rdma_register_cb(struct device *dev,
			mtk_mmsys_cb cb, u32 status, void *priv_data);
int mtk_rdma_reset_split_start(
	const struct device *dev, struct cmdq_pkt *cmdq_handle);
int mtk_rdma_reset_split_end(const struct device *dev);
int mtk_rdma_reset_split_polling(
	const struct device *dev, struct cmdq_pkt *cmdq_handle);
int mtk_rdma_reset(const struct device *dev);
int mtk_rdma_start(const struct device *dev,
				struct cmdq_pkt **cmdq_handle);
int mtk_rdma_stop(const struct device *dev,
				struct cmdq_pkt **cmdq_handle);
int mtk_rdma_config_srcaddr(const struct device *dev,
				      struct cmdq_pkt **cmdq_handle,
				      struct mtk_rdma_src *param);
int mtk_rdma_set_target_line(
		const struct device *dev,
		struct cmdq_pkt **cmdq_handle,
		unsigned int target_line_count);
int mtk_rdma_sel_sram(const struct device *dev,
		struct cmdq_pkt **cmdq_handle,
		u8 sel_rdma_type);
int mtk_rdma_gmc_rt(const struct device *dev, bool set_rt,
					struct cmdq_pkt **cmdq_handle);
int mtk_rdma_config_frame(const struct device *dev, u32 eye_nr,
					struct cmdq_pkt **cmdq_handle,
					struct mtk_rdma_config **configs);
int mtk_rdma_config_quarter(const struct device *dev,
			       struct cmdq_pkt **cmdq_handle,
			       struct mtk_rdma_frame_adv_config *config);
#endif
