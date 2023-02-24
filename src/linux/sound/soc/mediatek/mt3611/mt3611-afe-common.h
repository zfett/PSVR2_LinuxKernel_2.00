/**
 * @file mt3611-afe-common.h
 * Mediatek audio driver common definitions
 *
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MT3611_AFE_COMMON_H_
#define _MT3611_AFE_COMMON_H_

#include <linux/clk.h>
#include <linux/regmap.h>
#include <sound/asound.h>

#if defined(CONFIG_COMMON_CLK_MT3612) || defined(CONFIG_COMMON_CLK_MT3611)
#define COMMON_CLOCK_FRAMEWORK_API
#endif
#ifdef CONFIG_MACH_FPGA
#define FPGA_ONLY
#else
#define DPTOI2S_SYSFS
#endif
#ifdef CONFIG_MACH_MT3612EP
#define HDMI_ENABLE
#endif
/** @ingroup type_group_afe_enum
 * @brief Memory and I/O Interface
 */
enum mt3611_afe_memif_io {
	/** Downlink Memory Interface */
	MT3611_AFE_MEMIF_DL12,
	/** Uplink Memory Interface */
	MT3611_AFE_MEMIF_VUL12,
	/** Audio WriteBack Memory Interface */
	MT3611_AFE_MEMIF_AWB,
	/** TDM Out Memory Interface */
	MT3611_AFE_MEMIF_TDM_OUT,
	/** TDM In Memory Interface */
	MT3611_AFE_MEMIF_TDM_IN,
	/** Number of Memory Interfaces */
	MT3611_AFE_MEMIF_NUM,
	/** Base of Back End */
	MT3611_AFE_BACKEND_BASE = MT3611_AFE_MEMIF_NUM,
	/** I2S Interface */
	MT3611_AFE_IO_I2S = MT3611_AFE_BACKEND_BASE,
	/** TDM Interface */
	MT3611_AFE_IO_TDM,
	/** Mphone Multi Interface */
	MT3611_AFE_IO_MPHONE_MULTI,
	/** End of Back End */
	MT3611_AFE_BACKEND_END,
	/** Number of Back Ends */
	MT3611_AFE_BACKEND_NUM =
		(MT3611_AFE_BACKEND_END - MT3611_AFE_BACKEND_BASE),
};

/** @ingroup type_group_afe_enum
 * @brief Clock Type
 */
enum mt3611_afe_clk_type {
	MT3611_CLK_AUD_INTBUS_CG,
	MT3611_CLK_APLL1_CK,
	MT3611_CLK_APLL2_CK,
#ifdef HDMI_ENABLE
	MT3611_CLK_HDMIPLL_CK,
	MT3611_CLK_HDMIPLL_D6,
#endif
	MT3611_CLK_DPAPLL_CK,
	MT3611_CLK_DPAPLL_D3,
	MT3611_CLK_A1SYS_HP_CG,
	MT3611_CLK_A2SYS_HP_CG,
	MT3611_CLK_A3SYS_HP_CG,
	MT3611_CLK_A3SYS_HP_SEL,
	MT3611_CLK_TDMOUT_SEL,
	MT3611_CLK_TDMIN_SEL,
	MT3611_CLK_APLL12_DIV0,
	MT3611_CLK_APLL12_DIV1,
	MT3611_CLK_APLL12_DIV2,
	MT3611_CLK_APLL12_DIV3,
	MT3611_CLK_AUD_MAS_SLV_BCLK,
	MT3611_CLK_NUM
};

/** @ingroup type_group_afe_enum
 * @brief TDM Channels to Start at
 */
enum mt3611_afe_tdm_ch_start {
	/** Start at 1st sdata */
	AFE_TDM_CH_START_CH01 = 0,
	/** Start at 2nd sdata */
	AFE_TDM_CH_START_CH23,
	/** Start at 3rd sdata */
	AFE_TDM_CH_START_CH45,
	/** Start at 4th sdata */
	AFE_TDM_CH_START_CH67,
	/** Zero Output */
	AFE_TDM_CH_ZERO,
};

/** @ingroup type_group_afe_enum
 * @brief TDM Output Mode
 */
enum mt3611_afe_tdm_out_mode {
	/** To External Component */
	AFE_TDM_OUT_NORMAL = 0,
	/** To Mphone Multi */
	AFE_TDM_OUT_TO_MPHONE,
};

/** @ingroup type_group_afe_enum
 * @brief IRQ Mode
 */
enum mt3611_afe_irq_mode {
	/** Common IRQ */
	MT3611_AFE_IRQ_0 = 0,
	/** Common IRQ */
	MT3611_AFE_IRQ_1,
	/** Common IRQ */
	MT3611_AFE_IRQ_2,
	/** Common IRQ */
	MT3611_AFE_IRQ_3,
	/** Number of Common IRQs */
	MT3611_AFE_COMMON_IRQ_NUM,
	/** Dedicated for Mphone Multi */
	MT3611_AFE_IRQ_4 = MT3611_AFE_COMMON_IRQ_NUM,
	/** Dedicated for TDM IN */
	MT3611_AFE_IRQ_5,
	/** Dedicated for TDM OUT */
	MT3611_AFE_IRQ_6,
	/** Number of IRQs */
	MT3611_AFE_IRQ_NUM
};

/** @ingroup type_group_afe_enum
 * @brief Top Clock Gate Type
 */
enum mt3611_afe_top_clock_gate {
	MT3611_AFE_CG_AFE = 0,
	MT3611_AFE_CG_TDM_OUT,
	MT3611_AFE_CG_DP_RX,
#ifdef HDMI_ENABLE
	MT3611_AFE_CG_HDMI_RX,
#endif
	MT3611_AFE_CG_A1SYS,
	MT3611_AFE_CG_A2SYS,
	MT3611_AFE_CG_A3SYS,
	MT3611_AFE_CG_TDM_IN,
	MT3611_AFE_CG_I2S_OUT,
	MT3611_AFE_CG_I2S_IN,
	MT3611_AFE_CG_NUM
};

/** @ingroup type_group_afe_enum
 * @brief Engen Type
 */
enum mt3611_afe_engen {
	MT3611_AFE_ENGEN_A1SYS = 0,
	MT3611_AFE_ENGEN_A2SYS,
	MT3611_AFE_ENGEN_A3SYS,
	MT3611_AFE_ENGEN_NUM
};

/** @ingroup type_group_afe_enum
 * @brief Debug FS Type
 */
enum mt3611_afe_debugfs {
	MT3611_AFE_DEBUGFS_I2S,
	MT3611_AFE_DEBUGFS_TDM,
	MT3611_AFE_DEBUGFS_MPHONE,
	MT3611_AFE_DEBUGFS_NUM,
};

/** @ingroup type_group_afe_enum
 * @brief Source PLL Type
 */
enum mt3611_afe_source_pll {
	FROM_AUDIO_PLL = 0,
#ifdef HDMI_ENABLE
	FROM_HDMIRX_PLL,
#endif
	FROM_DPRX_PLL,
};

/** @ingroup type_group_afe_enum
 * @brief Mphone Source Type
 */
enum mt3611_afe_mphone_source {
#ifdef HDMI_ENABLE
	MPHONE_FROM_HDMIRX = 0,
	MPHONE_FROM_DPRX,
#else
	MPHONE_FROM_DPRX = 0,
#endif
	MPHONE_FROM_TDM_OUT,
};

struct snd_pcm_substream;

/** @ingroup type_group_afe_struct
 * @brief Memery Interface Data
 */
struct mt3611_afe_memif_data {
	/** memory interface id */
	int id;
	/** memory interface name */
	const char *name;
	/** register to set DMA base address */
	int reg_ofs_base;
	/** register to set DMA end address */
	int reg_ofs_end;
	/** register to get DMA current address */
	int reg_ofs_cur;
	/** register to set memory interface frequency */
	int data_fs_reg;
	/** shift bits to set memory interface frequency */
	int fs_shift;
	/** register to set mono channel */
	int data_mono_reg;
	/** shift bits to set mono channel */
	int mono_shift;
	/** shift bits to set quad channel */
	int four_ch_shift;
	/** register to enable memory interface */
	int enable_reg;
	/** shift bits to enable memory interface */
	int enable_shift;
	/** register to set irq counter */
	int irq_reg_cnt;
	/** shift bits to set irq counter */
	int irq_cnt_shift;
	/** irq mode */
	int irq_mode;
	/** register to set irq frequency */
	int irq_fs_reg;
	/** shift bits to set irq frequency */
	int irq_fs_shift;
	/** shift bits to clear irq */
	int irq_clr_shift;
	/** irq delay counter */
	int irq_delay_cnt;
	/** shift bits to enable irq delay */
	int irq_delay_en_shift;
	/** trigger source for irq delay */
	int irq_delay_src;
	/** shift bits to set trigger source */
	int irq_delay_src_shift;
	/** audio sram size to use */
	int max_sram_size;
	/** audio sram offset to use */
	int sram_offset;
	/** register to set data format */
	int format_reg;
	/** shift bits to set data format */
	int format_shift;
	/** mask to set data format */
	int format_mask;
	/** shift bits to set data format alignment (MSB or LSB) */
	int format_align_shift;
	/** pre allocation size for DMA buffer (dram) in bytes */
	int prealloc_size;
};

/** @ingroup type_group_afe_struct
 * @brief Back End DAI Data
 */
struct mt3611_afe_be_dai_data {
	/** back end dai has prepared or not */
	bool prepared;
	/** cached rate in prepare callback */
	unsigned int cached_rate;
	/** cached channel count in prepare callback */
	unsigned int cached_channels;
	/** cached format in prepare callback */
	unsigned int cached_format;
	/** format setting from set_fmt */
	unsigned int fmt_mode;
	/** mclk frequency setting from set_sysclk */
	unsigned int mclk_freq;
};

/** @ingroup type_group_afe_struct
 * @brief Memory Interface
 */
struct mt3611_afe_memif {
	/** DMA buffer physical address */
	unsigned int phys_buf_addr;
	/** DMA buffer size in bytes */
	int buffer_size;
	/** use sram or dram for DMA buffer */
	bool use_sram;
	/** front end dai has prepared or not */
	bool prepared;
	struct snd_pcm_substream *substream;
	const struct mt3611_afe_memif_data *data;
};

/** @ingroup type_group_afe_struct
 * @brief Control Data for Mixer Settings
 */
struct mt3611_afe_control_data {
	/** sine gen type, refer to enum afe_sgen_port */
	unsigned int sinegen_type;
	/** sine gen frequency, refer to enum afe_sgen_freq */
	unsigned int sinegen_fs;
	/** clock source for memory interface downlink */
	unsigned int dl12_clk_src;
	/** clock source for memory interface uplink */
	unsigned int vul12_clk_src;
	/** tdm output mode */
	unsigned int tdm_out_mode;
	/** clock source for common irq */
	unsigned int common_irq_clk_src[MT3611_AFE_COMMON_IRQ_NUM];
	/** clock source for back end hardware */
	unsigned int backend_clk_src[MT3611_AFE_BACKEND_NUM];
};

/** @ingroup type_group_afe_struct
 * @brief AFE Data
 */
struct mtk_afe {
	/** address for ioremap audio hardware register */
	void __iomem *base_addr;
	/** address for ioremap audio sram */
	void __iomem *sram_address;
	/** physical address of audio sram */
	u32 sram_phy_address;
	/** audio sram size in bytes */
	u32 sram_size;
	struct device *dev;
	struct regmap *regmap;
	struct mt3611_afe_memif memif[MT3611_AFE_MEMIF_NUM];
	struct mt3611_afe_be_dai_data be_data[MT3611_AFE_BACKEND_NUM];
	struct mt3611_afe_control_data ctrl_data;
	/** audio clock handle */
	struct clk *clocks[MT3611_CLK_NUM];
	/** registers to backup while suspend */
	unsigned int *backup_regs;
	/** suspend status */
	bool suspended;
	/** reference count to record afe on/off status */
	int afe_on_ref_cnt;
	/** reference count to record irq on/off status */
	int irq_mode_ref_cnt[MT3611_AFE_IRQ_NUM];
	/** reference count to record top cg on/off status */
	int top_cg_ref_cnt[MT3611_AFE_CG_NUM];
	/** reference count to record engen on/off status */
	int engen_ref_cnt[MT3611_AFE_ENGEN_NUM];
	int tdm_slot_width;
	/** control lock */
	spinlock_t afe_ctrl_lock;
	/** clock mutex */
	struct mutex afe_clk_mutex;
#ifdef FPGA_ONLY
	void __iomem *topckgen_base_addr;
#endif
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dentry[MT3611_AFE_DEBUGFS_NUM];
#endif
};

#endif
