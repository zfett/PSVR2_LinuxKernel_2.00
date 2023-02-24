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
 * @file mtk_lhc.h
 * Header of mtk_lhc.c
 */

#ifndef MTK_LHC_H
#define MTK_LHC_H

#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/mtk_drv_def.h>

#define LHC_BLK_X_NUM		4
#define LHC_BLK_Y_NUM		16
#define LHC_BLK_BIN_NUM		17
/**
 * LHC_X_BIN_BYTES is the amount of real data in LHC_BLK_X_NUM blocks along the
 * X direction. It's in unit of 8-bit.
 */
#define LHC_X_BIN_BYTES		(LHC_BLK_X_NUM * LHC_BLK_BIN_NUM)
/**
 * LHC_X_BIN_NUM is the amount of data reading from hardware in LHC_BLK_X_NUM
 * blocks along the X direction. It's in unit of 32-bit.
 */
#define LHC_X_BIN_NUM	(((LHC_BLK_X_NUM * LHC_BLK_BIN_NUM + 7) >> 3) << 1)
#define LHC_BLK_X_NUMS		(LHC_BLK_X_NUM * MTK_MM_MODULE_MAX_NUM)

/** @ingroup IP_group_lhc_external_def
 * @{
 */
#define LHC_CB_STA_FRAME_DONE			BIT(0)
#define LHC_CB_STA_READ_FINISH			BIT(1)
#define LHC_CB_STA_IF_UNFINISH			BIT(2)
#define LHC_CB_STA_OF_UNFINISH			BIT(3)
#define LHC_CB_STA_SRAM_RW_ERR			BIT(4)
#define LHC_CB_STA_APB_RW_ERR			BIT(5)
/*
 * @}
 */

/** @ingroup IP_group_lhc_external_struct
 * @brief LHC slice configuration parameters
 */
struct mtk_lhc_slice {
	u32 w;		/* frame width */
	u32 h;		/* frame height */
	u32 start;	/* active window x start */
	u32 end;	/* active window x end */
};

/** @ingroup IP_group_lhc_external_enum
 * @brief LHC color transform internal table
 */
enum mtk_lhc_int_table {
	LHC_INT_TBL_BT2020L = 0,
	LHC_INT_TBL_BT2020F,
	LHC_INT_TBL_BT709L,
	LHC_INT_TBL_B709F,
	LHC_INT_TBL_BT601L,
	LHC_INT_TBL_BT601F,
	LHC_INT_TBL_MAX
};

/** @ingroup IP_group_lhc_external_struct
 * @brief LHC color transform parameters
 */
struct mtk_lhc_color_trans {
	/**  Set true for YUV input, false for RGB input */
	bool enable;
	/**  Set true to use programmable table, false to use internal table */
	bool external_matrix;
	enum mtk_lhc_int_table int_matrix;
	/** External color transform matirx */
	u32 coff[3][3];
	/** pre-adding parameters */
	u32 pre_add[3];
	/** post-adding parameters */
	u32 post_add[3];
};

/** @ingroup IP_group_lhc_external_struct
 * @brief LHC hist count result
 */
struct mtk_lhc_hist {
	u8 module_num; /* FPGA: 1, SoC: 4 */
	/** LHC HIST RESULT */
	u8 r[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
	u8 g[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
	u8 b[LHC_BLK_Y_NUM][LHC_BLK_X_NUMS][LHC_BLK_BIN_NUM];
};

int mtk_lhc_register_cb(const struct device *dev,
			mtk_mmsys_cb cb, u32 status, void *priv_data);
int mtk_lhc_config_slices(const struct device *dev,
			  struct mtk_lhc_slice (*config)[4],
			  struct cmdq_pkt **handle);
int mtk_lhc_config(const struct device *dev, unsigned int w, unsigned int h,
		   struct cmdq_pkt **handle);
int mtk_lhc_start(const struct device *dev, struct cmdq_pkt **handle);
int mtk_lhc_stop(const struct device *dev, struct cmdq_pkt **handle);
int mtk_lhc_reset(const struct device *dev, struct cmdq_pkt **handle);
int mtk_lhc_power_on(struct device *dev);
int mtk_lhc_power_off(struct device *dev);
int mtk_lhc_set_color_transform(const struct device *dev,
				struct mtk_lhc_color_trans *color_trans,
				struct cmdq_pkt **handle);
int mtk_lhc_read_frame_done_status(const struct device *dev, bool *frame_done);
int mtk_lhc_read_histogram(const struct device *dev, struct mtk_lhc_hist *data);
/* test only */
int mtk_lhc_write_histogram(const struct device *dev,
			    struct mtk_lhc_hist *data);

#endif
