/*
 * Copyright (c) 2015 MediaTek Inc.
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

#ifndef _MTK_DPRX_DRV_H_
#define _MTK_DPRX_DRV_H_

/**
 * @file mtk_dprx_drv_int.h
 * Header of internal DPRX function
 */

/* ====================== Global Definition ======================== */
extern struct mtk_dprx *pdprx;
extern struct platform_device *my_pdev;
extern bool f_pwr_on_initial;

/** @ingroup IP_group_dprx_internal_def
 * @{
 */
#define SUB_HW_NUMBER 3
#define SYS_REF_CLK 26

#define MAC_BASE 0x00
#define PHYD_BASE 0x01
#define SRAM_BASE 0x02

#define BIT_0_POS  0
#define BIT_1_POS  1
#define BIT_2_POS  2
#define BIT_3_POS  3
#define BIT_4_POS  4
#define BIT_5_POS  5
#define BIT_6_POS  6
#define BIT_7_POS  7
#define BIT_8_POS  8
#define BIT_9_POS  9
#define BIT_10_POS 10
#define BIT_11_POS 11
#define BIT_12_POS 12
#define BIT_13_POS 13
#define BIT_14_POS 14
#define BIT_15_POS 15
#define BIT_16_POS 16
#define BIT_17_POS 17
#define BIT_18_POS 18
#define BIT_19_POS 19
#define BIT_20_POS 20
#define BIT_21_POS 21
#define BIT_22_POS 22
#define BIT_23_POS 23
#define BIT_24_POS 24
#define BIT_25_POS 25
#define BIT_26_POS 26
#define BIT_27_POS 27
#define BIT_28_POS 28
#define BIT_29_POS 29
#define BIT_30_POS 30
#define BIT_31_POS 31

#define BIT_0  0x00000001
#define BIT_1  0x00000002
#define BIT_2  0x00000004
#define BIT_3  0x00000008
#define BIT_4  0x00000010
#define BIT_5  0x00000020
#define BIT_6  0x00000040
#define BIT_7  0x00000080
#define BIT_8  0x00000100
#define BIT_9  0x00000200
#define BIT_10 0x00000400
#define BIT_11 0x00000800
#define BIT_12 0x00001000
#define BIT_13 0x00002000
#define BIT_14 0x00004000
#define BIT_15 0x00008000
#define BIT_16 0x00010000
#define BIT_17 0x00020000
#define BIT_18 0x00040000
#define BIT_19 0x00080000
#define BIT_20 0x00100000
#define BIT_21 0x00200000
#define BIT_22 0x00400000
#define BIT_23 0x00800000
#define BIT_24 0x01000000
#define BIT_25 0x02000000
#define BIT_26 0x04000000
#define BIT_27 0x08000000
#define BIT_28 0x10000000
#define BIT_29 0x20000000
#define BIT_30 0x40000000
#define BIT_31 0x80000000

#define BYTE3_POS  24
#define BYTE2_POS  16
#define BYTE1_POS  8
#define BYTE0_POS  0
#define BYTE_3 (0xff<<BYTE3_POS)
#define BYTE_2 (0xff<<BYTE2_POS)
#define BYTE_1 (0xff<<BYTE1_POS)
#define BYTE_0 (0xff<<BYTE0_POS)

#define EDID_BLOCK_0 127
#define EDID_BLOCK_1 255
#define EDID_BLOCK_2 383
#define EDID_BLOCK_3 511

#define TSP_HDCP_DP_KEY   0xf2002023
/** @}
 */

/* ====================== Global Structure ========================= */
/** @ingroup IP_group_dprx_internal_enum
* @brief DPRX link rate\n
* value is 0x06, 0x0A, 0x14, 0x1E
*/
enum mtk_dprx_link_rate {
	/** 0x06: 1.62 Gbps/lane */
	MTK_DPRX_LINK_RATE_RBR = 0x6,
	/** 0x0A: 2.7 Gbps/lane */
	MTK_DPRX_LINK_RATE_HBR = 0xa,
	/** 0x14: 5.4 Gbps/lane */
	MTK_DPRX_LINK_RATE_HBR2 = 0x14,
	/** 0x1E: 8.1 Gbps/lane */
	MTK_DPRX_LINK_RATE_HBR3 = 0x1e
};

/** @ingroup IP_group_dprx_internal_struct
 * @brief DPRX device structure
 */
struct mtk_dprx {
	/** Linux device structure */
	struct device *dev;
	struct device *mutex_dev, *mmsys_dev;
	struct mtk_mutex_res *phy_mutex;
	struct cmdq_client *phy_client;
	struct cmdq_pkt *gce_pkt;

	/** Register IO Memory Remapping */
	void __iomem *regs[SUB_HW_NUMBER];
	/** Ref. clock */
	struct clk *ref_clk;
	/** reset control */
	struct reset_control *rstc;
	struct reset_control *apb_rstc;

	/** DPRX Max. Link Rate */
	unsigned int max_link_rate;
	/** DPRX Max. Frame Rate */
	unsigned int max_fps;
	/** HW number for reg in device tree */
	u32 sub_hw_nr;
	u32 gce_subsys;
	u32 gce_subsys_offset[SUB_HW_NUMBER];
	/** FPGA Mode Information */
	u32 fpga_mode;
	/** 3612 early porting Information */
	u32 early_porting;
	/** DP1.3 Information */
	u32 dp1p3;
	/** HDCP2.2 Information */
	u32 hdcp2p2;
	/** DSC Mode Information */
	u32 dsc;
	/** FEC Information */
	u32 fec;
	/** YUV420 Information */
	u32 yuv420;
	/** Audio Information */
	u32 audio;
	/** HBR3 Information */
	u32 hbr3;

	/** DPRX Link Rate */
	enum mtk_dprx_link_rate link_rate;

	/** DPRX Lane Count */
	unsigned int lanes;

	/** DPRX IRQ Number */
	int irq_num;
	/** DPRX IRQ Data */
	int irq_data;
	/** Linux PHY structure */
	struct phy **phys;
	/** PHY number */
	int num_phys;

	/* event thread setting */
	struct task_struct *main_task;
	wait_queue_head_t main_wq;
	atomic_t main_event;
	bool is_rx_init;

	/* callback function */
	struct device *ap_dev;
	int (*callback)(struct device *dev, enum DPRX_NOTIFY_T event);

};

/* ====================== Global Macro ============================= */
/* MAC ACCESS*/
static inline u32 mtk_dprx_read32(u32 offset)
{
	return ioread32(pdprx->regs[MAC_BASE] + offset);
}

static inline void mtk_dprx_write32(u32 offset, u32 data)
{
	iowrite32(data, pdprx->regs[MAC_BASE] + offset);
}

static inline u32 mtk_dprx_read_mask(u32 offset, u32 mask, u32 shift)
{
	u32 data = mtk_dprx_read32(offset);

	data = ((data & mask) >> shift);
	return data;
}

static inline void mtk_dprx_write_mask(
	u32 offset, u32 value, u32 mask, u32 shift)
{
	u32 data = mtk_dprx_read32(offset);

	value = value << shift;
	data = ((data & ~(mask)) | (value));
	mtk_dprx_write32(offset, data);
}

static inline bool mtk_dprx_is_set_bit(u32 offset, u32 bit)
{
	u32 data = mtk_dprx_read32(offset);

	data &= bit;
	if (data)
		return 1;
	else
		return 0;
}

#define MTK_DPRX_REG_READ_MASK(offset, mask)                                   \
	mtk_dprx_read_mask(offset, mask, mask##_POS)
#define MTK_DPRX_REG_WRITE_MASK(offset, value, mask)                           \
	mtk_dprx_write_mask(offset, value, mask, mask##_POS)

#define MTK_DPRX_READ32(offset) mtk_dprx_read32(offset)
#define MTK_DPRX_WRITE32(offset, value) mtk_dprx_write32(offset, value)

#define MTK_DPRX_REG_IS_BIT_SET(offset, bit_mask)                              \
	mtk_dprx_is_set_bit(offset, bit_mask)

/* PHYD ACCESS */
static inline u32 mtk_dprx_phyd_read32(u32 offset)
{
	return ioread32(pdprx->regs[PHYD_BASE] + offset);
}

static inline void mtk_dprx_phyd_write32(u32 offset, u32 data)
{
	iowrite32(data, pdprx->regs[PHYD_BASE] + offset);
}

static inline u32 mtk_dprx_phyd_read_mask(u32 offset, u32 mask, u32 shift)
{
	u32 data = mtk_dprx_phyd_read32(offset);

	data = ((data & mask) >> shift);
	return data;
}

static inline void mtk_dprx_phyd_write_mask(
	u32 offset, u32 value, u32 mask, u32 shift)
{
	u32 data = mtk_dprx_phyd_read32(offset);

	value = value << shift;
	data = ((data & ~(mask)) | (value));
	mtk_dprx_phyd_write32(offset, data);
}

#define MTK_DPRX_REG_PHYD_READ_MASK(offset, mask)                              \
	mtk_dprx_phyd_read_mask(offset, mask, mask##_POS)
#define MTK_DPRX_REG_PHYD_WRITE_MASK(offset, value, mask)                      \
	mtk_dprx_phyd_write_mask(offset, value, mask, mask##_POS)

#define MTK_DPRX_PHYD_READ32(offset) mtk_dprx_phyd_read32(offset)
#define MTK_DPRX_PHYD_WRITE32(offset, value)                                   \
	mtk_dprx_phyd_write32(offset, value)

/* SRAM ACCESS */
static inline u32 mtk_dprx_sram_read32(u32 offset)
{
	return ioread32(pdprx->regs[SRAM_BASE] + offset);
}

static inline void mtk_dprx_sram_write32(u32 offset, u32 data)
{
	iowrite32(data, pdprx->regs[SRAM_BASE] + offset);
}

#define MTK_DPRX_SRAM_READ32(offset) mtk_dprx_sram_read32(offset)
#define MTK_DPRX_SRAM_WRITE32(offset, value)                                   \
	mtk_dprx_sram_write32(offset, value)

#define RST_LOGIC_RESET_FLDMASK_LEN 1
#define RST_LOGIC_RESET_FLDMASK_POS 0

/* ====================== Function Prototype ======================= */
int mtk_dprx_hw_init(struct mtk_dprx *dprx, bool hdcp2x);
void mtk_dprx_hw_deinit(struct mtk_dprx *dprx);
void dprx_drv_dpcd_init(void);
void dprx_drv_hdcp_key_init(void);
void dprx_drv_hdcp1x_init(void);
void dprx_drv_hdcp2x_init(bool hdcp2x);
void dprx_drv_hdcp2x_enable(void);
void dprx_drv_hdcp2x_disable(void);
void dprx_drv_hdcp2x_auth_status_clear(void);
void dprx_drv_sw_hpd_ready_ctrl(bool val);
void dprx_drv_dsc_init(struct mtk_dprx *dprx);
void dprx_lt_table_setting(void);
int dprx_phy_init(struct mtk_dprx *dprx);
void dprx_align_status_unlock_intr_enable(bool en);
void dprx_dpcd_int_config(void);
void dprx_phy_exit(struct mtk_dprx *dprx);
int dprx_phy_power_on(struct mtk_dprx *dprx);
void dprx_phy_power_off(struct mtk_dprx *dprx);
int dprx_26m_clk_enable(struct mtk_dprx *dprx);
void dprx_26m_clk_disable(struct mtk_dprx *dprx);
int dprx_rst_assert(struct mtk_dprx *dprx);
int dprx_rst_deassert(struct mtk_dprx *dprx);
int dprx_power_on_before_work(void);
int dprx_power_off_after_work(void);
int dprx_power_on_seq(struct mtk_dprx *dprx);
int dprx_power_off_seq(struct mtk_dprx *dprx);

#endif
