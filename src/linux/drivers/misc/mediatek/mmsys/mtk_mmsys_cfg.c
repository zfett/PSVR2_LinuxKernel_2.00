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
 * @file mtk_mmsys_cfg.c
 * This mmsys cfg driver is used to configure MTK mmsys core top hardware
 * module.\n
 * It includes controlling mux, mout and provides reset function for sub\n
 * modules which are belong to mmcore partition.
 */

/**
 * @defgroup IP_group_mmsys_cfg MMSYS_CORE engine
 *     There are several mux and mout modules in mmsys top in MT3612.\n
 *     By setting the mux and mout modules, we can control the data flow
 *     in mmcore partition.\n
 *
 *   @{
 *       @defgroup IP_group_mmsys_cfg_external EXTERNAL
 *         The external API document for mmsys_cfg. \n
 *
 *         @{
 *            @defgroup IP_group_mmsys_cfg_external_function 1.function
 *              This is mmsys_cfg external API.
 *            @defgroup IP_group_mmsys_cfg_external_struct 2.structure
 *              none.
 *            @defgroup IP_group_mmsys_cfg_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_mmsys_cfg_external_enum 4.enumeration
 *              This is mmsys_cfg external enumeration.
 *            @defgroup IP_group_mmsys_cfg_external_def 5.define
 *              This is mmsys_cfg external definition.
 *         @}
 *
 *       @defgroup IP_group_mmsys_cfg_internal INTERNAL
 *         The internal API document for mmsys_cfg. \n
 *
 *         @{
 *            @defgroup IP_group_mmsys_cfg_internal_function 1.function
 *              Mmsys_cfg mux/mout setting and module init.
 *            @defgroup IP_group_mmsys_cfg_internal_struct 2.structure
 *              Internal structure used for mmsys_cfg.
 *            @defgroup IP_group_mmsys_cfg_internal_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_mmsys_cfg_internal_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_mmsys_cfg_internal_def 5.define
 *              Mmsys_cfg register definition and constant definition.
 *         @}
 *     @}
 */

#include <linux/uaccess.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#ifdef CONFIG_MTK_DEBUGFS
#include <linux/slab.h>
#endif
#include <soc/mediatek/mtk_drv_def.h>
#include <soc/mediatek/mtk_mmsys_cfg.h>
#include "mtk_mmsys_cfg_reg.h"
#include <linux/pm_runtime.h>

#define MODULE_MAX_NUM		1

/** @ingroup IP_group_mmsys_cfg_internal_def
 * @brief Mmsys configuration register
 * @{
 */
#define VSGEN_DLY_CTRL0(n)		(VSGEN_DSI_DLY0_CTRL0 + 0xc * (n))
#define VSGEN_DLY_CTRL1(n)		(VSGEN_DSI_DLY0_CTRL1 + 0xc * (n))
#define VSGEN_DLY_CTRL2(n)		(VSGEN_DSI_DLY0_CTRL2 + 0xc * (n))
#define VSGEN_FRC_PARAM			(32)
#define PAT_CTRL0(n)			(MMSYS_PAT0_CTRL0 + 0x28 * (n))
#define PAT_CTRL1(n)			(MMSYS_PAT0_CTRL1 + 0x28 * (n))
#define PAT_CTRL2(n)			(MMSYS_PAT0_CTRL2 + 0x28 * (n))
#define PAT_CTRL3(n)			(MMSYS_PAT0_CTRL3 + 0x28 * (n))
#define PAT_CTRL4(n)			(MMSYS_PAT0_CTRL4 + 0x28 * (n))
#define PAT_CTRL5(n)			(MMSYS_PAT0_CTRL5 + 0x28 * (n))
#define PAT_CTRL6(n)			(MMSYS_PAT0_CTRL6 + 0x28 * (n))
#define PAT_CTRL7(n)			(MMSYS_PAT0_CTRL7 + 0x28 * (n))
#define CAMERA_VSYNC_IRQ_CNT		((MMSYSCFG_CAMERA_SYNC_MAX) * 2)
#define CRC_OUT(n)			(MMSYS_CRC_OUT_0 + 0x4 * (n))
#define CRC_NUM(n)			(MMSYS_CRC_NUM_0 + 0x4 * (n))
/**
 * @}
 */

/** @ingroup IP_group_mmsys_cfg_internal_def
 * @brief Mmsys_cfg Macro Definitions
 * @{
 */
/** No component */
#define NOC		MMSYSCFG_COMPONENT_ID_MAX
/** Possible current components number */
#define CNR		4
/** Possible next components number */
#define NNR		4
/**
 * @}
 */

/** @ingroup IP_group_mmsys_cfg_internal_def
 * @brief Csc reg group information Definitions
 * @{
 */
#define CSC_REG_GROUP_COUNT 4
#define CSC_REG_GROUP_GAP 0x28
/**
 * @}
 */

/** @ingroup IP_group_mmsys_cfg_internal_struct
 * @brief Mmsys_cfg components connection map
 */
struct mmsys_comp_map_t {
	enum mtk_mmsys_config_comp_id	curr[CNR];
	enum mtk_mmsys_config_comp_id	next[NNR];
	u32				reg_offset;
};

static struct mmsys_comp_map_t const s_mout_map[] = {
	{
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_CROP0,
			MMSYSCFG_COMPONENT_LHC0,
			MMSYSCFG_COMPONENT_DISP_RDMA0,
			NOC
		},
		MMSYS_SLCR_MOUT_EN0
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_CROP1,
			MMSYSCFG_COMPONENT_LHC1,
			MMSYSCFG_COMPONENT_DISP_RDMA1,
			NOC
		},
		MMSYS_SLCR_MOUT_EN1
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_CROP2,
			MMSYSCFG_COMPONENT_LHC2,
			MMSYSCFG_COMPONENT_DISP_RDMA2,
			NOC
		},
		MMSYS_SLCR_MOUT_EN2
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_CROP3,
			MMSYSCFG_COMPONENT_LHC3,
			MMSYSCFG_COMPONENT_DISP_RDMA3,
			NOC
		},
		MMSYS_SLCR_MOUT_EN3
	}, {
		{
			MMSYSCFG_COMPONENT_DISP_RDMA0, /* option 0 */
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0, /* option 1 */
			MMSYSCFG_COMPONENT_MDP_RDMA0, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC0,
			MMSYSCFG_COMPONENT_DP_PAT_GEN0,
			NOC
		},
		MMSYS_RDMA_MOUT_EN0
	}, {
		{
			MMSYSCFG_COMPONENT_DISP_RDMA1, /* option 0 */
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC1, /* option 1 */
			MMSYSCFG_COMPONENT_MDP_RDMA1, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC1,
			MMSYSCFG_COMPONENT_DP_PAT_GEN1,
			NOC
		},
		MMSYS_RDMA_MOUT_EN1
	}, {
		{
			MMSYSCFG_COMPONENT_DISP_RDMA2, /* option 0 */
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC2, /* option 1 */
			MMSYSCFG_COMPONENT_MDP_RDMA2, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC2,
			MMSYSCFG_COMPONENT_DP_PAT_GEN2,
			NOC
		},
		MMSYS_RDMA_MOUT_EN2
	}, {
		{
			MMSYSCFG_COMPONENT_DISP_RDMA3, /* option 0 */
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC3, /* option 1 */
			MMSYSCFG_COMPONENT_MDP_RDMA3, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC3,
			MMSYSCFG_COMPONENT_DP_PAT_GEN3,
			NOC
		},
		MMSYS_RDMA_MOUT_EN3
	}, {
		{
			MMSYSCFG_COMPONENT_RSZ1,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_WDMA1,
			MMSYSCFG_COMPONENT_P2S0,
			NOC
		},
		MMSYS_RSZ_MOUT_EN
	}, {
		{
			MMSYSCFG_COMPONENT_DSC0,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
			MMSYSCFG_COMPONENT_DISP_WDMA0,
			NOC
		},
		MMSYS_DSC_MOUT_EN0
	}, {
		{
			MMSYSCFG_COMPONENT_DSC0,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
			MMSYSCFG_COMPONENT_DISP_WDMA1,
			NOC
		},
		MMSYS_DSC_MOUT_EN1
	}, {
		{
			MMSYSCFG_COMPONENT_DSC1,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
			MMSYSCFG_COMPONENT_DISP_WDMA2,
			NOC
		},
		MMSYS_DSC_MOUT_EN2
	}, {
		{
			MMSYSCFG_COMPONENT_DSC1,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DSI_LANE_SWAP,
			MMSYSCFG_COMPONENT_DISP_WDMA3,
			NOC
		},
		MMSYS_DSC_MOUT_EN3
	},
	{ { NOC }, { NOC }, 0}
};

static struct mmsys_comp_map_t const s_out_sel_map[] = {
	{
		{
			MMSYSCFG_COMPONENT_DISP_RDMA0,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0,
			MMSYSCFG_COMPONENT_MDP_RDMA0,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC0,
			MMSYSCFG_COMPONENT_DP_PAT_GEN0,
			NOC
		},
		MMSYS_RDMA_OUT_SEL0
	}, {
		{
			MMSYSCFG_COMPONENT_DISP_RDMA1,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC1,
			MMSYSCFG_COMPONENT_MDP_RDMA1,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC1,
			MMSYSCFG_COMPONENT_DP_PAT_GEN1,
			NOC
		},
		MMSYS_RDMA_OUT_SEL1
	}, {
		{
			MMSYSCFG_COMPONENT_DISP_RDMA2,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC2,
			MMSYSCFG_COMPONENT_MDP_RDMA2,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC2,
			MMSYSCFG_COMPONENT_DP_PAT_GEN2,
			NOC
		},
		MMSYS_RDMA_OUT_SEL2
	}, {
		{
			MMSYSCFG_COMPONENT_DISP_RDMA3,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC3,
			MMSYSCFG_COMPONENT_MDP_RDMA3,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC3,
			MMSYSCFG_COMPONENT_DP_PAT_GEN3,
			NOC
		},
		MMSYS_RDMA_OUT_SEL3
	},
	{ { NOC }, { NOC }, 0}
};

static struct mmsys_comp_map_t const s_in_map[] = {
	{
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_SLICER_DSC,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_RDMA0,
			NOC
		},
		MMSYS_RDMA_IN_SEL0
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_SLICER_DSC,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_RDMA1,
			NOC
		},
		MMSYS_RDMA_IN_SEL1
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_SLICER_DSC,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_RDMA2,
			NOC
		},
		MMSYS_RDMA_IN_SEL2
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_SLICER_DSC,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_RDMA3,
			NOC
		},
		MMSYS_RDMA_IN_SEL3
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_DISP_RDMA0, /* option 0 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC0,
			NOC
		},
		MMSYS_LHC_SEL0
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0, /* option 1 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC0,
			NOC
		},
		MMSYS_LHC_SEL0
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA0, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC0,
			NOC
		},
		MMSYS_LHC_SEL0
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_DISP_RDMA1, /* option 0 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC1,
			NOC
		},
		MMSYS_LHC_SEL1
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC1, /* option 1 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC1,
			NOC
		},
		MMSYS_LHC_SEL1
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA1, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC1,
			NOC
		},
		MMSYS_LHC_SEL1
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_DISP_RDMA2, /* option 0 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC2,
			NOC
		},
		MMSYS_LHC_SEL2
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC2, /* option 1 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC2,
			NOC
		},
		MMSYS_LHC_SEL2
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA2, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC2,
			NOC
		},
		MMSYS_LHC_SEL2
	},  {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_DISP_RDMA3, /* option 0 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC3,
			NOC
		},
		MMSYS_LHC_SEL3
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC3, /* option 1 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC3,
			NOC
		},
		MMSYS_LHC_SEL3
	}, {
		{
			MMSYSCFG_COMPONENT_SLICER_VID,
			MMSYSCFG_COMPONENT_MDP_RDMA3, /* option 2 */
			NOC
		},
		{
			MMSYSCFG_COMPONENT_LHC3,
			NOC
		},
		MMSYS_LHC_SEL3
	}, {
		{
			MMSYSCFG_COMPONENT_P2S0,
			MMSYSCFG_COMPONENT_DSC0,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_WDMA0,
			NOC
		},
		MMSYS_WDMA_SEL0
	}, {
		{
			MMSYSCFG_COMPONENT_RSZ1,
			MMSYSCFG_COMPONENT_DSC0,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_WDMA1,
			NOC
		},
		MMSYS_WDMA_SEL1
	}, {
		{
			MMSYSCFG_COMPONENT_CROP2,
			MMSYSCFG_COMPONENT_DSC1,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_WDMA2,
			NOC
		},
		MMSYS_WDMA_SEL2
	}, {
		{
			MMSYSCFG_COMPONENT_CROP3,
			MMSYSCFG_COMPONENT_DSC1,
			NOC
		},
		{
			MMSYSCFG_COMPONENT_DISP_WDMA3,
			NOC
		},
		MMSYS_WDMA_SEL3
	},
	{ { NOC }, { NOC }, 0}
};

/** @ingroup IP_group_mmsys_cfg_internal_struct
 * @brief Mmsys_cfg Driver Data Structure.
 */
struct mtk_mmsys_config {
	/** mmsys_cfg device node */
	struct device *dev;
	/** mmsys_cfg pattern generator clock nodes */
	struct clk *clk_pat_gen[MTK_MM_MODULE_MAX_NUM];
	/** mmsys_cfg DSC multiple output clock nodes */
	struct clk *clk_dsc_mout[MTK_MM_MODULE_MAX_NUM];
	/** mmsys_cfg RDMA multiple output clock nodes */
	struct clk *clk_rdma_mout[MTK_MM_MODULE_MAX_NUM];
	/** mmsys_cfg RSZ0 multiple output clock node */
	struct clk *clk_rsz0_mout;
	/** mmsys_cfg DSI lane swap clock node */
	struct clk *clk_dsi_swap;
	/** mmsys_cfg LHC swap clock node */
	struct clk *clk_lhc_swap;
	/** mmsys_cfg event tx clock node */
	struct clk *clk_event_tx;
	/** mmsys_cfg camera sync clock node */
	struct clk *clk_camera_sync;
	/** mmsys_cfg CRC clock node */
	struct clk *clk_crc;
	/** mmsys_cfg camera sync clock select */
	struct clk *clk_sync_sel;
	/** mmsys_cfg camera sync 26M clock */
	struct clk *pll_26M;
	/** mmsys_cfg camera sync 2.6M clock */
	struct clk *pll_26M_D10;
	/** mmsys_cfg register base */
	void __iomem *regs[MODULE_MAX_NUM];
	/** mmsys_cfg hardware number */
	u32 hw_nr;
	/** gce subsys */
	u32 gce_subsys[MODULE_MAX_NUM];
	/** gce subsys offset */
	u32 gce_subsys_offset[MODULE_MAX_NUM];
	/** camera vsync irq */
	int cam_vsync_irq[CAMERA_VSYNC_IRQ_CNT];
	/** spinlock for irq handler */
	spinlock_t			lock_irq;
	mtk_mmsys_cb			cb_func;
	struct mtk_mmsys_cb_data	cb_data;
	u32				cb_status;
	/** pattern info */
	u32				pat_cursor[MMSYSCFG_PAT_GEN_ID_MAX];
	u32				pat_act_w[MMSYSCFG_PAT_GEN_ID_MAX];
	u32				pat_act_h[MMSYSCFG_PAT_GEN_ID_MAX];
#ifdef CONFIG_MTK_DEBUGFS
	u32 reg_base[MODULE_MAX_NUM];
	struct dentry			**debugfs;
	const char			*file_name;
#endif
};

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     csc reset.\n
 *     When user calls this API, this API will reset csc hardware.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, reset success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mmsys_cfg_dev is NULL, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_csc_reset(const struct device *mmsys_cfg_dev,
		struct cmdq_pkt **handle)
{
	int i;
	unsigned int offset;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	for (i = 0; i < CSC_REG_GROUP_COUNT; i++) {
		offset = RDMA0_CSC_TRANSFORM_0 + (i * CSC_REG_GROUP_GAP);

		if (!handle) {
			writel(readl(mmsyscfg->regs[0] + offset) |
			       RDMA0_CONV_CLR,
			       mmsyscfg->regs[0] + offset);
			writel(readl(mmsyscfg->regs[0] + offset) &
			       (~RDMA0_CONV_CLR),
			       mmsyscfg->regs[0] + offset);
		} else {
			cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] + offset,
					RDMA0_CONV_CLR,
					RDMA0_CONV_CLR);
			cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] + offset,
					0,
					RDMA0_CONV_CLR);
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_csc_reset);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Transform color format.\n
 *     When user calls this API to transform color format, this API will
 *     configure transform parameters including enable, internal or external
 *     matrix table.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     trans_en: transform enable or not.
 * @param[in]
 *     trans_mode: transform mode select.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mmsys_cfg_dev is NULL, return -EINVAL.\n
 *     2. If trans_mode is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_csc_config(const struct device *mmsys_cfg_dev,
		struct cmdq_pkt **handle,
		bool trans_en,
		enum mtk_csc_trans_mode trans_mode)
{
	struct mtk_mmsys_config *mmsyscfg;
	int i, ret = 0;
	void __iomem *regs;
	u32 subsys;
	u32 subsys_offset;
	struct cmdq_pkt *pkt;
	u8 ext_matrix_en = 0;
	u8 int_matrix_sel = 0;
	u16 ext_c_00 = 0;
	u16 ext_c_01 = 0;
	u16 ext_c_02 = 0;
	u16 ext_c_10 = 0;
	u16 ext_c_11 = 0;
	u16 ext_c_12 = 0;
	u16 ext_c_20 = 0;
	u16 ext_c_21 = 0;
	u16 ext_c_22 = 0;
	u16 ext_in_offset_0 = 0;
	u16 ext_in_offset_1 = 0;
	u16 ext_in_offset_2 = 0;
	u16 ext_out_offset_0 = 0;
	u16 ext_out_offset_1 = 0;
	u16 ext_out_offset_2 = 0;
	u32 value_0 = 0;
	u32 value_1 = 0;
	u32 value_2 = 0;
	u32 value_3 = 0;
	u32 value_4 = 0;
	u32 value_5 = 0;
	u32 value_6 = 0;
	u32 value_7 = 0;
	u32 value_8 = 0;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	if (trans_en) {
		if (trans_mode == CSC_TRANS_MODE_BT601_TO_RGB) {
			int_matrix_sel = 6;
		} else if (trans_mode == CSC_TRANS_MODE_BT709_TO_RGB) {
			int_matrix_sel = 7;
		} else if (trans_mode == CSC_TRANS_MODE_SWAP_RB) {
			ext_matrix_en = 1;
			ext_c_00 = 0;
			ext_c_01 = 0;
			ext_c_02 = 0x1000;
			ext_c_10 = 0;
			ext_c_11 = 0x1000;
			ext_c_12 = 0;
			ext_c_20 = 0x1000;
			ext_c_21 = 0;
			ext_c_22 = 0;
			ext_in_offset_0 = 0;
			ext_in_offset_1 = 0;
			ext_in_offset_2 = 0;
			ext_out_offset_0 = 0;
			ext_out_offset_1 = 0;
			ext_out_offset_2 = 0;
		} else {
			dev_err(mmsys_cfg_dev,
				"invalid csc transform mode = %d\n",
				trans_mode);
			return -EINVAL;
		}
	}

	value_0 = (((u32)int_matrix_sel << 24) & RDMA0_CONV_INT_MATRIX_SEL) |
		(((u32)ext_matrix_en << 20) & RDMA0_CONV_EXT_MATRIX_EN) |
		(((u32)trans_en << 16) & RDMA0_CONV_TRANS_EN) |
		RDMA0_CONV_ENG_ACTIVE;
	value_1 = (((u32)ext_c_01 << 16) & RDMA0_CONV_EXT_C_01) |
		((u32)ext_c_00 & RDMA0_CONV_EXT_C_00);
	value_2 = (((u32)ext_c_10 << 16) & RDMA0_CONV_EXT_C_10) |
		((u32)ext_c_02 & RDMA0_CONV_EXT_C_02);
	value_3 = (((u32)ext_c_12 << 16) & RDMA0_CONV_EXT_C_12) |
		((u32)ext_c_11 & RDMA0_CONV_EXT_C_11);
	value_4 = (((u32)ext_c_21 << 16) & RDMA0_CONV_EXT_C_21) |
		((u32)ext_c_20 & RDMA0_CONV_EXT_C_20);
	value_5 = (u32)ext_c_22 & RDMA0_CONV_EXT_C_22;
	value_6 =
		(((u32)ext_in_offset_1 << 16) & RDMA0_CONV_EXT_IN_OFFSET_1) |
		((u32)ext_in_offset_0 & RDMA0_CONV_EXT_IN_OFFSET_0);
	value_7 =
		(((u32)ext_out_offset_1 << 16) & RDMA0_CONV_EXT_OUT_OFFSET_1) |
		((u32)ext_out_offset_0 & RDMA0_CONV_EXT_OUT_OFFSET_0);
	value_8 =
		(((u32)ext_out_offset_2 << 16) & RDMA0_CONV_EXT_OUT_OFFSET_2) |
		((u32)ext_in_offset_2 & RDMA0_CONV_EXT_IN_OFFSET_2);

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	if (!handle)
		pkt = NULL;
	else
		pkt = handle[0];

	subsys = mmsyscfg->gce_subsys[0];

	for (i = 0; i < CSC_REG_GROUP_COUNT; i++) {
		regs = mmsyscfg->regs[0] + (i * CSC_REG_GROUP_GAP);
		subsys_offset = mmsyscfg->gce_subsys_offset[0] +
				(i * CSC_REG_GROUP_GAP);

		if (!pkt)
			writel(value_0, regs + RDMA0_CSC_TRANSFORM_0);
		else
			ret |= cmdq_pkt_write_value(pkt, subsys,
				subsys_offset + RDMA0_CSC_TRANSFORM_0,
				value_0, 0xffffffff);

		if (ext_matrix_en != 0) {
			if (!pkt) {
				writel(value_1, regs + RDMA0_CSC_TRANSFORM_1);
				writel(value_2, regs + RDMA0_CSC_TRANSFORM_2);
				writel(value_3, regs + RDMA0_CSC_TRANSFORM_3);
				writel(value_4, regs + RDMA0_CSC_TRANSFORM_4);
				writel(value_5, regs + RDMA0_CSC_TRANSFORM_5);
				writel(value_6, regs + RDMA0_CSC_TRANSFORM_6);
				writel(value_7, regs + RDMA0_CSC_TRANSFORM_7);
				writel(value_8, regs + RDMA0_CSC_TRANSFORM_8);
			} else {
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_1,
					value_1, 0xffffffff);
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_2,
					value_2, 0xffffffff);
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_3,
					value_3, 0xffffffff);
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_4,
					value_4, 0xffffffff);
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_5,
					value_5, 0xffffffff);
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_6,
					value_6, 0xffffffff);
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_7,
					value_7, 0xffffffff);
				ret |= cmdq_pkt_write_value(pkt, subsys,
					subsys_offset + RDMA0_CSC_TRANSFORM_8,
					value_8, 0xffffffff);
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(mtk_csc_config);

/** @ingroup IP_group_mmsys_cfg_internal_function
 * @par Description
 *     Configure Register of MOUT(Multiple Output) Muxer.\n
 *     Some modules can output data to more than one modules at the same
 *     time. In such case, we need to set the multiple output bits for
 *     connection and clear for disconnection.
 * @param[in]
 *     cur: source module id.
 * @param[in]
 *     next: target module id.
 * @param[in]
 *     regs: mmsys core register base.
 * @param[in]
 *     connect: true for connecting, false for disconnecting.
 * @param[in]
 *     subsys: gce subsys.
 * @param[in]
 *     subsys_offset: gce subsys offset.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @param[out]
 *     link_status: the two modules are connected by MOUT muxer or not.
 * @return
 *     0, configuration success.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. cur must be smaller than MMSYSCFG_COMPONENT_ID_MAX.\n
 *     2. next must be smaller than MMSYSCFG_COMPONENT_ID_MAX.\n
 *     3. cur and next modules can be connected.
 * @par Error case and Error handling
 *     1. If cur module or next module is illegal, do nothing.\n
 *     2. If cur module can not output to next module, do nothing.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_cfg_mout(enum mtk_mmsys_config_comp_id cur,
			      enum mtk_mmsys_config_comp_id next,
			      void __iomem *regs, bool connect,
			      u32 subsys, u32 subsys_offset,
			      struct cmdq_pkt *handle, bool *link_status)
{
	const struct mmsys_comp_map_t *map = s_mout_map;
	int ret = 0, i = 0, j = 0;
	u32 offset[2] = {0, 0}, value[2] = {0, 0};
	bool matched[2] = {false, false};
	u32 max_mout_cnt = 1;

	if (next == MMSYSCFG_COMPONENT_DSI_LANE_SWAP)
		max_mout_cnt = 2;

	while (map->curr[0] != NOC) {
		for (i = 0; i < CNR; i++) {
			if (map->curr[i] == NOC)
				break;
			/* current component is matched */
			if (map->curr[i] == cur) {
				for (j = 0; j < NNR; j++) {
					if (map->next[j] == NOC)
						break;
					/* next component is matched */
					if (map->next[j] == next &&
					    max_mout_cnt > 0) {
						max_mout_cnt--;
						matched[max_mout_cnt] = true;
						value[max_mout_cnt] = BIT(j);
						offset[max_mout_cnt] =
							map->reg_offset;
					}
					if (max_mout_cnt == 0)
						break;
				}
			}
			if (matched[0])
				break;
		}
		if (matched[0])
			break;
		map++;
	}

	if (link_status)
		*link_status = matched[0];
	if (!matched[0])
		return ret;

	if (!handle) {
		if (connect) {
			if (matched[0])
				writel((readl(regs + offset[0]) | value[0]),
					regs + offset[0]);
			if (matched[1])
				writel((readl(regs + offset[1]) | value[1]),
					regs + offset[1]);
		} else {
			if (matched[0])
				writel((readl(regs + offset[0]) & ~value[0]),
					regs + offset[0]);
			if (matched[1])
				writel((readl(regs + offset[1]) & ~value[1]),
					regs + offset[1]);
		}
	} else {
		if (connect) {
			if (matched[0])
				ret |= cmdq_pkt_write_value(handle, subsys,
						    subsys_offset + offset[0],
						    value[0], value[0]);
			if (matched[1])
				ret |= cmdq_pkt_write_value(handle, subsys,
						    subsys_offset + offset[1],
						    value[1], value[1]);
		} else {
			if (matched[0])
				ret |= cmdq_pkt_write_value(handle, subsys,
						    subsys_offset + offset[0],
						    0, value[0]);
			if (matched[1])
				ret |= cmdq_pkt_write_value(handle, subsys,
						    subsys_offset + offset[1],
						    0, value[1]);
		}
	}

	return ret;
}

/** @ingroup IP_group_mmsys_cfg_internal_function
 * @par Description
 *     Configure Register of Output Muxer.\n
 *     Some modules have the different output selection but only can choose
 *     one at the same time. This API will set the output muxer according to
 *     the two components which need to be connected.
 * @param[in]
 *     cur: source module id.
 * @param[in]
 *     next: target module id.
 * @param[in]
 *     regs: mmsys core register base.
 * @param[in]
 *     subsys: gce subsys.
 * @param[in]
 *     subsys_offset: gce subsys offset.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @param[out]
 *     link_status: the two modules are connected by output muxer or not.
 * @return
 *     0, configuration success.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. cur must be smaller than MMSYSCFG_COMPONENT_ID_MAX.\n
 *     2. next must be smaller than MMSYSCFG_COMPONENT_ID_MAX.\n
 *     3. cur and next modules can be connected.
 * @par Error case and Error handling
 *     1. If cur module or next module is illegal, do nothing.\n
 *     2. If cur module can not output to next module, do nothing.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_cfg_out_sel(enum mtk_mmsys_config_comp_id cur,
				 enum mtk_mmsys_config_comp_id next,
				 void __iomem *regs, u32 subsys,
				 u32 subsys_offset, struct cmdq_pkt *handle,
				 bool *link_status)
{
	const struct mmsys_comp_map_t *map = s_out_sel_map;
	int ret = 0, i = 0, j = 0;
	u32 offset, value;
	bool matched = false;

	while (map->curr[0] != NOC) {
		for (i = 0; i < CNR; i++) {
			if (map->curr[i] == NOC)
				break;
			/* current component is matched */
			if (map->curr[i] == cur) {
				for (j = 0; j < NNR; j++) {
					if (map->next[j] == NOC)
						break;
					/* next component is matched */
					if (map->next[j] == next) {
						matched = true;
						value = i;
						offset = map->reg_offset;
						break;
					}
				}
			}
			if (matched)
				break;
		}
		if (matched)
			break;
		map++;
	}

	if (link_status)
		*link_status = matched;
	if (!matched)
		return ret;

	if (!handle)
		writel(value, regs + offset);
	else
		ret |= cmdq_pkt_write_value(handle, subsys,
					    subsys_offset + offset,
					    value, 0xffffffff);
	return ret;
}

/** @ingroup IP_group_mmsys_cfg_internal_function
 * @par Description
 *     Configure Register of Input Muxer.\n
 *     Some modules have the different input selection but only can choose
 *     one at the same time. This API will set the input muxer according to
 *     the two components which need to be connected.
 * @param[in]
 *     cur: source module id.
 * @param[in]
 *     next: target module id.
 * @param[in]
 *     regs: mmsys core register base.
 * @param[in]
 *     subsys: gce subsys.
 * @param[in]
 *     subsys_offset: gce subsys offset.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @param[out]
 *     link_status: the two modules are connected by input muxer or not.
 * @return
 *     0, configuration success.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. cur must be smaller than MMSYSCFG_COMPONENT_ID_MAX.\n
 *     2. next must be smaller than MMSYSCFG_COMPONENT_ID_MAX.\n
 *     3. cur and next modules can be connected.
 * @par Error case and Error handling
 *     1. If cur module or next module is illegal, do nothing.\n
 *     2. If cur module can not output to next module, do nothing.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_cfg_in_sel(enum mtk_mmsys_config_comp_id cur,
				enum mtk_mmsys_config_comp_id next,
				void __iomem *regs, u32 subsys,
				u32 subsys_offset, struct cmdq_pkt *handle,
				bool *link_status)
{
	const struct mmsys_comp_map_t *map = s_in_map;
	int ret = 0, i = 0;
	u32 offset, value;
	bool matched = false;

	while (map->curr[0] != NOC) {
		/* next component is matched */
		if (map->next[0] == next) {
			for (i = 0; i < CNR; i++) {
				if (map->curr[i] == NOC)
					break;
				/* current component is matched */
				if (map->curr[i] == cur) {
					matched = true;
					value = i;
					offset = map->reg_offset;
					break;
				}
			}
		}

		if (matched)
			break;
		map++;
	}

	if (link_status)
		*link_status = matched;
	if (!matched)
		return ret;

	if (!handle)
		writel(value, regs + offset);
	else
		ret |= cmdq_pkt_write_value(handle, subsys,
					    subsys_offset + offset, value,
					    0xffffffff);
	return ret;
}

/** @ingroup IP_group_mmsys_cfg_internal_function
 * @par Description
 *     Check the two given modules are direct connection or not.
 * @param[in]
 *     cur: source module id.
 * @param[in]
 *     next: target module id.
 * @return
 *     true, direct connection.\n
 *     false, not direct connection.
 * @par Boundary case and Limitation
 *     1. cur must be smaller than MMSYSCFG_COMPONENT_ID_MAX.\n
 *     2. next must be smaller than MMSYSCFG_COMPONENT_ID_MAX.
 * @par Error case and Error handling
 *     None
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool mtk_mmsys_cfg_is_direct_link(enum mtk_mmsys_config_comp_id cur,
					 enum mtk_mmsys_config_comp_id next)
{
	bool connect = false;

	if (cur == MMSYSCFG_COMPONENT_RBFC0 &&
	    (next == MMSYSCFG_COMPONENT_MDP_RDMA0 ||
	     next == MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC0 ||
	     next == MMSYSCFG_COMPONENT_DISP_RDMA0))
		connect = true;
	else if (cur == MMSYSCFG_COMPONENT_RBFC1 &&
		 (next == MMSYSCFG_COMPONENT_MDP_RDMA1 ||
		  next == MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC1 ||
		  next == MMSYSCFG_COMPONENT_DISP_RDMA1))
		connect = true;
	else if (cur == MMSYSCFG_COMPONENT_RBFC2 &&
		 (next == MMSYSCFG_COMPONENT_MDP_RDMA2 ||
		  next == MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC2 ||
		  next == MMSYSCFG_COMPONENT_DISP_RDMA2))
		connect = true;
	else if (cur == MMSYSCFG_COMPONENT_RBFC3 &&
		 (next == MMSYSCFG_COMPONENT_MDP_RDMA3 ||
		  next == MMSYSCFG_COMPONENT_MDP_RDMA_PVRIC3 ||
		  next == MMSYSCFG_COMPONENT_DISP_RDMA3))
		connect = true;
	else if (cur == MMSYSCFG_COMPONENT_CROP0 &&
		 next == MMSYSCFG_COMPONENT_RSZ0)
		connect = true;
	else if (cur == MMSYSCFG_COMPONENT_CROP1 &&
		 next == MMSYSCFG_COMPONENT_RSZ1)
		connect = true;
	else if (cur == MMSYSCFG_COMPONENT_RSZ0 &&
		 next == MMSYSCFG_COMPONENT_P2S0)
		connect = true;
	else if ((cur == MMSYSCFG_COMPONENT_DP_PAT_GEN0 ||
		  cur == MMSYSCFG_COMPONENT_DP_PAT_GEN1) &&
		 next == MMSYSCFG_COMPONENT_DSC0)
		connect = true;
	else if ((cur == MMSYSCFG_COMPONENT_DP_PAT_GEN2 ||
		  cur == MMSYSCFG_COMPONENT_DP_PAT_GEN3) &&
		 next == MMSYSCFG_COMPONENT_DSC1)
		connect = true;

	return connect;
}

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Turn On MMSYS Power and Clock.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @return
 *     0, successful to turn on power and clock.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from pm_runtime_get_sync().\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mmsys_cfg_dev is NULL, return -EINVAL.\n
 *     2. If pm_runtime_get_sync() fails, return its error code.\n
 *     3. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_power_on(const struct device *mmsys_cfg_dev)
{
	struct mtk_mmsys_config *mmsyscfg;
	int ret = 0;
#ifdef CONFIG_COMMON_CLK_MT3612
	int i;
#endif

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	ret = pm_runtime_get_sync(mmsyscfg->dev);
	if (ret < 0) {
		dev_err(mmsyscfg->dev, "Failed to enable power domain: %d\n",
			ret);
		return ret;
	}
	for (i = 0; i < ARRAY_SIZE(mmsyscfg->clk_pat_gen); i++) {
		ret = clk_prepare_enable(mmsyscfg->clk_pat_gen[i]);
		if (ret) {
			dev_err(mmsys_cfg_dev,
				"Failed to enable pat gen clock: %d\n", ret);
			goto err_pat_gen;
		}
	}
	for (i = 0; i < ARRAY_SIZE(mmsyscfg->clk_dsc_mout); i++) {
		ret = clk_prepare_enable(mmsyscfg->clk_dsc_mout[i]);
		if (ret) {
			dev_err(mmsys_cfg_dev,
				"Failed to enable dsc mout[%d] clock: %d\n",
				i, ret);
			goto err_dsc_mout;
		}
	}
	for (i = 0; i < ARRAY_SIZE(mmsyscfg->clk_rdma_mout); i++) {
		ret = clk_prepare_enable(mmsyscfg->clk_rdma_mout[i]);
		if (ret) {
			dev_err(mmsys_cfg_dev,
				"Failed to enable rdma mout[%d] clock :%d\n",
				i, ret);
			goto err_rdma_mout;
		}
	}
	ret = clk_prepare_enable(mmsyscfg->clk_rsz0_mout);
	if (ret) {
		dev_err(mmsys_cfg_dev, "Failed to enable rsz0 mout clock :%d\n",
			ret);
		goto err_rsz0_mout;
	}
	ret = clk_prepare_enable(mmsyscfg->clk_dsi_swap);
	if (ret) {
		dev_err(mmsys_cfg_dev, "Failed to enable dsi swap clock :%d\n",
			ret);
		goto err_dsi_swap;
	}
	ret = clk_prepare_enable(mmsyscfg->clk_lhc_swap);
	if (ret) {
		dev_err(mmsys_cfg_dev, "Failed to enable lhc swap clock :%d\n",
			ret);
		goto err_lhc_swap;
	}
	ret = clk_prepare_enable(mmsyscfg->clk_event_tx);
	if (ret) {
		dev_err(mmsys_cfg_dev, "Failed to enable event tx clock :%d\n",
			ret);
		goto err_event_tx;
	}
	ret = clk_prepare_enable(mmsyscfg->clk_camera_sync);
	if (ret) {
		dev_err(mmsys_cfg_dev, "Failed to enable cam sync clock :%d\n",
			ret);
		goto err_camera_sync;
	}
	ret = clk_prepare_enable(mmsyscfg->clk_crc);
	if (ret) {
		dev_err(mmsys_cfg_dev, "Failed to enable crc clock :%d\n",
			ret);
		goto err_crc;
	}
#endif

	return ret;

#ifdef CONFIG_COMMON_CLK_MT3612
err_crc:
	clk_disable_unprepare(mmsyscfg->clk_camera_sync);
err_camera_sync:
	clk_disable_unprepare(mmsyscfg->clk_event_tx);
err_event_tx:
	clk_disable_unprepare(mmsyscfg->clk_lhc_swap);
err_lhc_swap:
	clk_disable_unprepare(mmsyscfg->clk_dsi_swap);
err_dsi_swap:
	clk_disable_unprepare(mmsyscfg->clk_rsz0_mout);
err_rsz0_mout:
	i = ARRAY_SIZE(mmsyscfg->clk_rdma_mout);
err_rdma_mout:
	while (--i >= 0)
		clk_disable_unprepare(mmsyscfg->clk_rdma_mout[i]);
	i = ARRAY_SIZE(mmsyscfg->clk_dsc_mout);
err_dsc_mout:
	while (--i >= 0)
		clk_disable_unprepare(mmsyscfg->clk_dsc_mout[i]);
	i = ARRAY_SIZE(mmsyscfg->clk_pat_gen);
err_pat_gen:
	while (--i >= 0)
		clk_disable_unprepare(mmsyscfg->clk_pat_gen[i]);

	pm_runtime_put(mmsyscfg->dev);
	return ret;
#endif
}
EXPORT_SYMBOL(mtk_mmsys_cfg_power_on);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Turn Off MMSYS Power and Clock.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @return
 *     0, successful to turn off power and clock.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     If mmsys_cfg_dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_power_off(const struct device *mmsys_cfg_dev)
{
	struct mtk_mmsys_config *mmsyscfg;
#ifdef CONFIG_COMMON_CLK_MT3612
	int i;
#endif

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

#ifdef CONFIG_COMMON_CLK_MT3612
	clk_disable_unprepare(mmsyscfg->clk_crc);
	clk_disable_unprepare(mmsyscfg->clk_camera_sync);
	clk_disable_unprepare(mmsyscfg->clk_event_tx);
	clk_disable_unprepare(mmsyscfg->clk_lhc_swap);
	clk_disable_unprepare(mmsyscfg->clk_dsi_swap);
	clk_disable_unprepare(mmsyscfg->clk_rsz0_mout);
	i = ARRAY_SIZE(mmsyscfg->clk_rdma_mout);
	while (--i >= 0)
		clk_disable_unprepare(mmsyscfg->clk_rdma_mout[i]);
	i = ARRAY_SIZE(mmsyscfg->clk_dsc_mout);
	while (--i >= 0)
		clk_disable_unprepare(mmsyscfg->clk_dsc_mout[i]);
	i = ARRAY_SIZE(mmsyscfg->clk_pat_gen);
	while (--i >= 0)
		clk_disable_unprepare(mmsyscfg->clk_pat_gen[i]);

	pm_runtime_put(mmsyscfg->dev);
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_power_off);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Connect two Modules.\n
 *     When user calls this API to connect two components, this API will call
 *     three internal functions which are mtk_mmsys_cfg_mout,
 *     mtk_mmsys_cfg_out_sel and mtk_mmsys_cfg_in_sel to configure multiple
 *     output, output selection and input selection.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     cur: source module id.
 * @param[in]
 *     next: target module id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from mtk_mmsys_cfg_mout().\n
 *     error code from mtk_mmsys_cfg_in_sel().\n
 *     error code from mtk_mmsys_cfg_out_sel().\n
 *     -EPIPE, the two given modules are not connected.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mmsys_cfg_dev is NULL, return -EINVAL.\n
 *     2. If mtk_mmsys_cfg_mout fails, return its error code.\n
 *     3. If mtk_mmsys_cfg_in_sel fails, return its error code.\n
 *     4. If mtk_mmsys_cfg_out_sel fails, return its error code.\n
 *     5. If the two given modules are not connected, return -EPIPE.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_connect_comp(const struct device *mmsys_cfg_dev,
			       enum mtk_mmsys_config_comp_id cur,
			       enum mtk_mmsys_config_comp_id next,
			       struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 i, cnt;
	int ret = 0;
	struct cmdq_pkt *pkt;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	cnt = min_t(u32, mmsyscfg->hw_nr, (u32)ARRAY_SIZE(mmsyscfg->regs));
	for (i = 0; i < cnt; i++) {
		bool connect[3] = {false, false, false};

		if (!handle)
			pkt = NULL;
		else
			pkt = handle[i];

		ret |= mtk_mmsys_cfg_mout(cur, next, mmsyscfg->regs[i], true,
					  mmsyscfg->gce_subsys[i],
					  mmsyscfg->gce_subsys_offset[i],
					  pkt, &connect[0]);
		ret |= mtk_mmsys_cfg_in_sel(cur, next, mmsyscfg->regs[i],
					    mmsyscfg->gce_subsys[i],
					    mmsyscfg->gce_subsys_offset[i],
					    pkt, &connect[1]);
		ret |= mtk_mmsys_cfg_out_sel(cur, next, mmsyscfg->regs[i],
					     mmsyscfg->gce_subsys[i],
					     mmsyscfg->gce_subsys_offset[i],
					     pkt, &connect[2]);
		if (!connect[0] && !connect[1] && !connect[2]) {
			if (!mtk_mmsys_cfg_is_direct_link(cur, next)) {
				ret = -EPIPE;
				break;
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_connect_comp);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Disconnect two Modules.\n
 *     When user calls this API to disconnect two components, this API will
 *     call mtk_mmsys_cfg_mout to clear multiple output. It does not have to
 *     call mtk_mmsys_cfg_out_sel and mtk_mmsys_cfg_in_sel because the
 *     input/output selection will be configured in next connection.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     cur: source module id.
 * @param[in]
 *     next: target module id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from mtk_mmsys_cfg_mout().
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mmsys_cfg_dev is NULL, return -EINVAL.\n
 *     2. If mtk_mmsys_cfg_mout fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_disconnect_comp(const struct device *mmsys_cfg_dev,
				  enum mtk_mmsys_config_comp_id cur,
				  enum mtk_mmsys_config_comp_id next,
				  struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 i, cnt;
	int ret = 0;
	struct cmdq_pkt *pkt;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	cnt = min_t(u32, mmsyscfg->hw_nr, (u32)ARRAY_SIZE(mmsyscfg->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle)
			pkt = NULL;
		else
			pkt = handle[i];

		ret |= mtk_mmsys_cfg_mout(cur, next, mmsyscfg->regs[i], false,
					  mmsyscfg->gce_subsys[i],
					  mmsyscfg->gce_subsys_offset[i], pkt,
					  NULL);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_disconnect_comp);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Module Hardware Reset.\n
 *     After doing hardwae reset, the module will return to initial state.
 *     User only can call this API when the module is idle or wants to recover
 *     module from error state.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     module: module ID.
 * @return
 *     0, reset success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. module has to support reset capability in mmcore.\n
 *     3. only can call this API if the module is in idle state or user wants
 *     recover module from error state.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if the module does not support reset function.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_reset_module(const struct device *mmsys_cfg_dev,
			       struct cmdq_pkt **handle,
			       enum mtk_mmsys_config_comp_id module)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 i, cnt;
	unsigned int offset, value;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if ((int)module < 0 || (int)module >= MMSYSCFG_COMPONENT_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong reset module id\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	if (module < MMSYSCFG_COMPONENT_DSI0)
		offset = MMSYS_SW_RSTB0;
	else if (module <= MMSYSCFG_COMPONENT_LHC_SWAP)
		offset = MMSYS_SW_RSTB1;
	else if (module < MMSYSCFG_COMPONENT_RBFC0) {
		dev_err(mmsys_cfg_dev, "wrong reset module id %d\n", module);
		return -EINVAL;
	} else if (module < MMSYSCFG_COMPONENT_ID_MAX)
		offset = MMSYS_SW_RSTB2;

	value = 0x1;

	cnt = min_t(u32, mmsyscfg->hw_nr, (u32)ARRAY_SIZE(mmsyscfg->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			writel(readl(mmsyscfg->regs[i] + offset) &
			       (~(value << (module & 0x1f))),
			       mmsyscfg->regs[i] + offset);
			writel(0xffffffff, mmsyscfg->regs[i] + offset);
		} else {
			cmdq_pkt_write_value(handle[i],
					mmsyscfg->gce_subsys[i],
					mmsyscfg->gce_subsys_offset[i] +
					offset, 0,
					(value << (module & 0x1f)));
			cmdq_pkt_write_value(handle[i],
					mmsyscfg->gce_subsys[i],
					mmsyscfg->gce_subsys_offset[i] +
					offset,
					(value << (module & 0x1f)),
					(value << (module & 0x1f)));
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_reset_module);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Select Camera Sync Delay Counter Clock.\n
 *     There are two clock source which can be used for Camera Sync delay
 *     counter. One is 2.6MHz and the other is 26MHz. User can choose which
 *     clock source they want to use.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     use_26m: true for using 26M, false for using 2.6M.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from clk_set_parent().
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     1. If mmsys_cfg_dev is NULL, return -EINVAL.\n
 *     2. If clk_set_parent() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_camera_sync_clock_sel(const struct device *mmsys_cfg_dev,
					bool use_26m)
{
	struct mtk_mmsys_config *mmsyscfg;
	int ret = 0;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;


#ifdef CONFIG_COMMON_CLK_MT3612
	ret = clk_set_parent(mmsyscfg->clk_sync_sel, use_26m ?
			     mmsyscfg->pll_26M : mmsyscfg->pll_26M_D10);

	if (ret)
		dev_err(mmsys_cfg_dev, "Failed to set camera sync clock: %d\n",
			ret);
#endif

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_camera_sync_clock_sel);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     DSI vsync input polarity control. Camera sync will generate timing base\n
 *     the polarity.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     dsi_vsync_high_acive: select DSI polarity.
 * @return
 *     0, configure camera sync done.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_camera_sync_dsi_control(const struct device *mmsys_cfg_dev,
					  bool dsi_vsync_high_acive)
{
	struct mtk_mmsys_config *mmsyscfg;
	void *addr;
	u32 reg;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	addr = mmsyscfg->regs[0] + VSGEN_DSI_CTRL;
	reg = readl(addr) & ~VSGEN_DSI_VSYNC_POL;
	if (dsi_vsync_high_acive)
		reg |= VSGEN_DSI_VSYNC_POL;
	writel(reg, addr);

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_camera_sync_dsi_control);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Set Vsync Gen Delay Value.\n
 *     There are 5 valid Vsync Delay Generator. For each of them, user can
 *     configure delay time, output vsync width, and output active polarity.\n
 *     The Vsync source of all Vsync Delay Generator is DSI Vsync.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     vsgen_id: camera sync index.
 * @param[in]
 *     vsync_cycle: vsync pulse width in clock cycle count, should be > 1.
 * @param[in]
 *     delay_cycle: delay value in clock cycle count.
 * @param[in]
 *     vsync_low_acive: output vsync is low active or high active.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure camera sync done.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. vsgen_id should be smaller than MMSYSCFG_CAMERA_SYNC_MAX.\n
 *     3. vsync_cycle should be smaller than CAMERA_SYNC_MAX_VSYNC_WIDTH.\n
 *     4. delay_cycle should be smaller than CAMERA_SYNC_MAX_DELAY_CYCLE.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if vsgen_id is larger than MMSYSCFG_CAMERA_SYNC_MAX.\n
 *     3. return -EINVAL if vsync_cycle is 0 or larger than
 *        CAMERA_SYNC_MAX_VSYNC_WIDTH.\n
 *     4. return -EINVAL if delay_cycle is large than
 *        CAMERA_SYNC_MAX_DELAY_CYCLE.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_camera_sync_config(const struct device *mmsys_cfg_dev,
				     enum mtk_mmsys_camera_sync_id vsgen_id,
				     u32 vsync_cycle, u32 delay_cycle,
				     bool vsync_low_acive,
				     struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	if (vsgen_id >= MMSYSCFG_CAMERA_SYNC_MAX) {
		dev_err(mmsys_cfg_dev, "wrong camera sync index\n");
		return -EINVAL;
	}

	/*
	if (delay_cycle > VSGEN_DLY0_ON_DELAY) {
		dev_err(mmsys_cfg_dev, "delay_cycle exceeds the limitation\n");
		return -EINVAL;
	}
	*/

	if (vsync_cycle == 0 || vsync_cycle > VSGEN_DLY0_ON_TIME) {
		dev_err(mmsys_cfg_dev, "vsync_cycle is invalid\n");
		return -EINVAL;
	}

	if (handle == NULL) {
		void *addr;
		unsigned int reg = 0;

		addr = mmsyscfg->regs[0] + VSGEN_DLY_CTRL0(vsgen_id);
		/* reg = readl(addr) & ~VSGEN_DLY0_ON_DELAY; */
		reg |= delay_cycle;
		writel(reg, addr);

		addr = mmsyscfg->regs[0] + VSGEN_DLY_CTRL1(vsgen_id);
		reg = readl(addr) & ~VSGEN_DLY0_ON_TIME;
		reg |= vsync_cycle;
		if (vsync_low_acive)
			writel(reg & ~VSGEN_DLY0_OUT_POL, addr);
		else
			writel(reg | VSGEN_DLY0_OUT_POL, addr);
	} else {
		cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     VSGEN_DLY_CTRL0(vsgen_id),
				     delay_cycle, VSGEN_DLY0_ON_DELAY);
		if (vsync_low_acive) {
			cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     VSGEN_DLY_CTRL1(vsgen_id), vsync_cycle,
				     VSGEN_DLY0_OUT_POL | VSGEN_DLY0_ON_TIME);
		} else {
			cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     VSGEN_DLY_CTRL1(vsgen_id),
				     VSGEN_DLY0_OUT_POL | vsync_cycle,
				     VSGEN_DLY0_OUT_POL | VSGEN_DLY0_ON_TIME);
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_camera_sync_config);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Set Vsync Gen FRC(Frame Rate Converter) parameters.\n
 *     There are 5 valid Vsync Delay Generator. For each of them, user can
 *     configure FRC ratio n and m. Camera Vsync is n/m of DSI Vsync.
 *     The Vsync source of all Vsync Delay Generator is DSI Vsync.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     vsgen_id: camera sync index.
 * @param[in]
 *     n: numerator of FRC ratio, range from 1 to 32.
 * @param[in]
 *     m: denominator of FRC ratio, range from 1 to 32.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure camera sync done.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. n should be range from 1 to 32.\n
 *     3. m should be range from 1 to 32.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if vsgen_id is large than MMSYSCFG_CAMERA_SYNC_MAX.\n
 *     3. return -EINVAL if n or m does not range from 1 to 32.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_camera_sync_frc(const struct device *mmsys_cfg_dev,
				  enum mtk_mmsys_camera_sync_id vsgen_id,
				  unsigned char n, unsigned char m,
				  struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 period;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	if (vsgen_id >= MMSYSCFG_CAMERA_SYNC_MAX) {
		dev_err(mmsys_cfg_dev, "wrong camera sync index\n");
		return -EINVAL;
	}

	if (n < 1 || n > VSGEN_FRC_PARAM || m < 1 || m > VSGEN_FRC_PARAM) {
		dev_err(mmsys_cfg_dev, "FRC parameter exceeds the limitation\n");
		return -EINVAL;
	}

	period = readl(mmsyscfg->regs[0] + VSGEN_DSI_PERIOD_CNT_MON);
	period = period * m / n;

	/* 0 means max value for hardware */
	if (n == VSGEN_FRC_PARAM)
		n = 0;
	if (m == VSGEN_FRC_PARAM)
		m = 0;

	if (handle == NULL) {
		void *addr;
		unsigned int reg;

		addr = mmsyscfg->regs[0] + VSGEN_DLY_CTRL1(vsgen_id);
		reg = readl(addr);
		reg &= ~(VSGEN_DLY0_FRM_CONV_N | VSGEN_DLY0_FRM_CONV_M);
		writel(reg | (n << 24) | (m << 16), addr);
		addr = mmsyscfg->regs[0] + VSGEN_DLY_CTRL2(vsgen_id);
		writel(period, addr);
	} else {
		cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
			     mmsyscfg->gce_subsys_offset[0] +
			     VSGEN_DLY_CTRL1(vsgen_id),
			     (n << 24) | (m << 16),
			     VSGEN_DLY0_FRM_CONV_N | VSGEN_DLY0_FRM_CONV_M);
		cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
			     mmsyscfg->gce_subsys_offset[0] +
			     VSGEN_DLY_CTRL2(vsgen_id),
			     period,
			     VSGEN_DLY0_FRM_COV_PERIOD);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_camera_sync_frc);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Register callback function for camera vsync interrupt.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     cb: Camera vsync irq callback function pointer assigned by user.
 * @param[in]
 *     status: Specify which irq status should be notified via callback.
 *         It's combination of MMSYSCFG_CAMERA_SYNC_IRQ_xxx, e.g.
 *         MMSYSCFG_CAMERA_SYNC_IRQ_SIDE01 | MMSYSCFG_CAMERA_SYNC_IRQ_SIDE23.
 * @param[in]
 *     priv_data: Camera sync irq callback function's priv_data.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_camera_sync_register_cb(struct device *mmsys_cfg_dev,
					  mtk_mmsys_cb cb, u32 status,
					  void *priv_data)
{
	struct mtk_mmsys_config *mmsyscfg;
	unsigned long flags;
	void *addr;
	u32 reg;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	spin_lock_irqsave(&mmsyscfg->lock_irq, flags);
	mmsyscfg->cb_func = cb;
	mmsyscfg->cb_data.priv_data = priv_data;
	mmsyscfg->cb_status = status;

	addr = mmsyscfg->regs[0] + VSGEN_IRQ_CTRL;
	reg = readl(addr) & ~VSGEN_IRQ_EN;
	if (cb)
		reg |= status & VSGEN_IRQ_EN;
	writel(reg, addr);
	spin_unlock_irqrestore(&mmsyscfg->lock_irq, flags);

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_camera_sync_register_cb);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Enable Camera Sync Function.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     vsgen_id: Camera sync index.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, enable camera sync successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. vsgen_id should be smaller than MMSYSCFG_CAMERA_SYNC_MAX.\n
 *     3. please call mtk_mmsys_cfg_camera_sync_clock_sel and
 *        mtk_mmsys_cfg_camera_sync_config to finish configuration
 *        before enabling camera sync function.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if vsgen_id is large than MMSYSCFG_CAMERA_SYNC_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_camera_sync_enable(const struct device *mmsys_cfg_dev,
				     enum mtk_mmsys_camera_sync_id vsgen_id,
				     struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 flag;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	if (vsgen_id >= MMSYSCFG_CAMERA_SYNC_MAX) {
		dev_err(mmsys_cfg_dev, "wrong camera sync index\n");
		return -EINVAL;
	}

	flag = BIT(vsgen_id) | BIT(vsgen_id + MMSYSCFG_CAMERA_SYNC_MAX);

	if (handle == NULL) {
		void *addr = mmsyscfg->regs[0] + VSGEN_DLY_CTRL1(vsgen_id);
		u32 reg;

		reg = readl(addr) | VSGEN_DLY0_OUT_EN;
		writel(reg, addr);

		addr = mmsyscfg->regs[0] + VSGEN_EVENT_CTRL;
		reg = readl(addr) | flag;
		writel(reg, addr);
	} else {
		cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     VSGEN_DLY_CTRL1(vsgen_id),
				     VSGEN_DLY0_OUT_EN,
				     VSGEN_DLY0_OUT_EN);
		cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     VSGEN_EVENT_CTRL,
				     flag, flag);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_camera_sync_enable);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Disable Camera Sync Function.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     vsgen_id: Camera sync index.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, disable camera sync successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. vsgen_id should be smaller than MMSYSCFG_CAMERA_SYNC_MAX.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if vsgen_id is large than MMSYSCFG_CAMERA_SYNC_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_camera_sync_disable(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_camera_sync_id vsgen_id,
				      struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 flag;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	if (vsgen_id >= MMSYSCFG_CAMERA_SYNC_MAX) {
		dev_err(mmsys_cfg_dev, "wrong camera sync index\n");
		return -EINVAL;
	}

	flag = BIT(vsgen_id) | BIT(vsgen_id + MMSYSCFG_CAMERA_SYNC_MAX);

	if (handle == NULL) {
		void *addr = mmsyscfg->regs[0] + VSGEN_DLY_CTRL1(vsgen_id);
		u32 reg;

		reg = readl(addr) & ~VSGEN_DLY0_OUT_EN;
		writel(reg, addr);

		addr = mmsyscfg->regs[0] + VSGEN_EVENT_CTRL;
		reg = readl(addr) & ~flag;
		writel(reg, addr);
	} else {
		cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     VSGEN_DLY_CTRL1(vsgen_id),
				     0, VSGEN_DLY0_OUT_EN);
		cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     VSGEN_EVENT_CTRL,
				     0, flag);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_camera_sync_disable);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Configure LCM(LCD Module) Reset signal.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     reset: assert reset or not.
 * @param[in]
 *     flags: specify which LCM will be reset. It's a combination value of
 *     MMSYS_LCMx_RST, e.g. MMSYS_LCM0_RST | MMSYS_LCM3_RST.
 * @return
 *     0, reset success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.
 * @par Error case and Error handling
 *     return -EINVAL if mmsys_cfg_dev is NULL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_reset_lcm(const struct device *mmsys_cfg_dev, bool reset,
			    u32 flags)
{
	struct mtk_mmsys_config *mmsyscfg;
	unsigned int i, reg, cnt;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	flags &= LCM_RST_B | LCM1_RST_B | LCM2_RST_B | LCM3_RST_B;

	cnt = min_t(u32, mmsyscfg->hw_nr, (u32)ARRAY_SIZE(mmsyscfg->regs));
	for (i = 0; i < cnt; i++) {
		if (reset) {
			reg = readl(mmsyscfg->regs[i] + MMSYS_LCM_RST_B) &
			      (~flags);
			writel(reg, mmsyscfg->regs[i] + MMSYS_LCM_RST_B);
		} else {
			reg = readl(mmsyscfg->regs[i] + MMSYS_LCM_RST_B) |
				flags;
			writel(reg, mmsyscfg->regs[i] + MMSYS_LCM_RST_B);
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_reset_lcm);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     DSI link swap configuration function.\n
 *     Configure which link connect to which dsi.
 *     User can use this function to configure link sequence.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     to_dsi0: select which link(0~3) connect to dsi0
 * @param[in]
 *     to_dsi1: select which link(0~3) connect to dsi1
 * @param[in]
 *     to_dsi2: select which link(0~3) connect to dsi2
 * @param[in]
 *     to_dsi3: select which link(0~3) connect to dsi3
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     Parameter to_dsix should be 0 ~ 3.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EINVAL. \n
 *     2. The parameter, to_dsix, is incorrect, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_dsi_lane_swap_config(const struct device *mmsys_cfg_dev,
				       u32 to_dsi0, u32 to_dsi1, u32 to_dsi2,
				       u32 to_dsi3, struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	int ret = 0;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	if (WARN_ON((to_dsi0 >= 4) || (to_dsi1 >= 4) || (to_dsi2 >= 4) ||
		    (to_dsi3 >= 4)))
		return -EINVAL;

	if (handle == NULL) {
		void *addr;
		u32 reg;

		addr = mmsyscfg->regs[0] + MMSYS_DSI_LANE_SWAP_SEL0;
		reg = readl(addr) & ~DSI0_LANE_SWAP_SEL;
		writel(reg | to_dsi0, addr);
		addr = mmsyscfg->regs[0] + MMSYS_DSI_LANE_SWAP_SEL1;
		reg = readl(addr) & ~DSI1_LANE_SWAP_SEL;
		writel(reg | to_dsi1, addr);
		addr = mmsyscfg->regs[0] + MMSYS_DSI_LANE_SWAP_SEL2;
		reg = readl(addr) & ~DSI2_LANE_SWAP_SEL;
		writel(reg | to_dsi2, addr);
		addr = mmsyscfg->regs[0] + MMSYS_DSI_LANE_SWAP_SEL3;
		reg = readl(addr) & ~DSI3_LANE_SWAP_SEL;
		writel(reg | to_dsi3, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_DSI_LANE_SWAP_SEL0,
					to_dsi0, DSI0_LANE_SWAP_SEL);
		ret |= cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_DSI_LANE_SWAP_SEL1,
					to_dsi1, DSI1_LANE_SWAP_SEL);
		ret |= cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_DSI_LANE_SWAP_SEL2,
					to_dsi2, DSI2_LANE_SWAP_SEL);
		ret |= cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_DSI_LANE_SWAP_SEL3,
					to_dsi3, DSI3_LANE_SWAP_SEL);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_dsi_lane_swap_config);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     LHC link swap configuration function.\n
 *     Configure which link connect to which lhc.
 *     User can use this function to configure link sequence.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     to_lhc0: select which link(0~3) connect to lhc0
 * @param[in]
 *     to_lhc1: select which link(0~3) connect to lhc1
 * @param[in]
 *     to_lhc2: select which link(0~3) connect to lhc2
 * @param[in]
 *     to_lhc3: select which link(0~3) connect to lhc3
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success \n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     Parameter to_lhcx should be 0 ~ 3.
 * @par Error case and Error handling
 *     1. The device struct is not assigned, return -EINVAL. \n
 *     2. The parameter, to_lhcx, is incorrect, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_lhc_swap_config(const struct device *mmsys_cfg_dev,
				       u32 to_lhc0, u32 to_lhc1, u32 to_lhc2,
				       u32 to_lhc3, struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	int ret = 0;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	if (WARN_ON((to_lhc0 >= 4) || (to_lhc1 >= 4) || (to_lhc2 >= 4) ||
		    (to_lhc3 >= 4)))
		return -EINVAL;

	if (handle == NULL) {
		void *addr;
		u32 reg;

		addr = mmsyscfg->regs[0] + MMSYS_LHC_SWAP_SEL0;
		reg = readl(addr) & ~LHC0_SWAP_SEL;
		writel(reg | to_lhc0, addr);
		addr = mmsyscfg->regs[0] + MMSYS_LHC_SWAP_SEL1;
		reg = readl(addr) & ~LHC1_SWAP_SEL;
		writel(reg | to_lhc1, addr);
		addr = mmsyscfg->regs[0] + MMSYS_LHC_SWAP_SEL2;
		reg = readl(addr) & ~LHC2_SWAP_SEL;
		writel(reg | to_lhc2, addr);
		addr = mmsyscfg->regs[0] + MMSYS_LHC_SWAP_SEL3;
		reg = readl(addr) & ~LHC3_SWAP_SEL;
		writel(reg | to_lhc3, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_LHC_SWAP_SEL0,
					to_lhc0, LHC0_SWAP_SEL);
		ret |= cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_LHC_SWAP_SEL1,
					to_lhc1, LHC1_SWAP_SEL);
		ret |= cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_LHC_SWAP_SEL2,
					to_lhc2, LHC2_SWAP_SEL);
		ret |= cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_LHC_SWAP_SEL3,
					to_lhc3, LHC3_SWAP_SEL);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_lhc_swap_config);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Slicer to DDDS selection.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     sel: slicer selection.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, success\n
 *     -EINVAL, invalid parameter
 * @par Boundary case and Limitation
 *     sel should be 0 ~ 1.
 * @par Error case and Error handling
 *     1. If mmsys_cfg_dev is NULL, return -EINVAL.\n
 *     2. If sel is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_slicer_to_ddds_select(const struct device *mmsys_cfg_dev,
					enum mtk_mmsys_slicer_to_ddds_sel sel,
					struct cmdq_pkt **handle)
{
	struct mtk_mmsys_config *mmsyscfg;
	int ret = 0;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	if (sel < MMSYSCFG_SLCR_VID_TO_DDDS || sel >= MMSYSCFG_SLCR_TO_DDDS_MAX)
		return -EINVAL;

	if (handle == NULL) {
		void *addr = mmsyscfg->regs[0] + MMSYS_SLCR_TO_DDDS_SELECT;
		u32 reg = readl(addr) & ~MMSYS_SLCR_TO_DDDS_SEL;

		writel(reg | sel, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0],
					mmsyscfg->gce_subsys[0],
					mmsyscfg->gce_subsys_offset[0] +
					MMSYS_SLCR_TO_DDDS_SELECT,
					sel, MMSYS_SLCR_TO_DDDS_SEL);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_slicer_to_ddds_select);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Configure frame size of pattern.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     width: horizontal active size of pattern.
 * @param[in]
 *     height: vertical active size of pattern.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. width/height ranges from 0 to 0x1fff, the excess will be truncated
 *        by bitwise and 0x1fff.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_config_frame_size(const struct device *mmsys_cfg_dev,
					enum mtk_mmsys_pat_gen_id id,
					u32 width, u32 height,
					struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	mmsyscfg->pat_act_w[id] = width & ((PAT_0_H_ACT) >> 16);
	mmsyscfg->pat_act_h[id] = height & PAT_0_V_ACT;
	reg = ((width << 16) & PAT_0_H_ACT) | (height & PAT_0_V_ACT);

	if (handle == NULL) {
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL1(id);
		reg |= readl(addr) & ~(PAT_0_H_ACT | PAT_0_V_ACT);
		writel(reg, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL1(id),
				     reg, PAT_0_H_ACT | PAT_0_V_ACT);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_config_frame_size);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Configure border size of pattern.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     width: horizontal border size of pattern.
 * @param[in]
 *     height: vertical border size of pattern.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. width/height ranges from 0 to 0x1fff, the excess will be truncated
 *        by bitwise and 0x1fff.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_config_border_size(const struct device *mmsys_cfg_dev,
					enum mtk_mmsys_pat_gen_id id,
					u32 width, u32 height,
					struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	reg = ((width << 16) & PAT_0_FRM_SIZE_H) | (height & PAT_0_FRM_SIZE_V);

	if (handle == NULL) {
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL2(id);
		reg |= readl(addr) & ~(PAT_0_FRM_SIZE_H | PAT_0_FRM_SIZE_V);
		writel(reg, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL2(id),
				     reg, PAT_0_FRM_SIZE_H | PAT_0_FRM_SIZE_V);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_config_border_size);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Configure foreground color of pattern.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     color_y: Y component of YUV color.
 * @param[in]
 *     color_u: U component of YUV color.
 * @param[in]
 *     color_v: V component of YUV color.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. color_y/color_u/color_v ranges from 0 to 0x1ff, the excess will be
 *        truncated by bitwise and 0x1ff.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_config_fg_color(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_pat_gen_id id,
				      u32 color_y, u32 color_u, u32 color_v,
				      struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	reg = ((color_u << 16) & PAT_0_COLOR_U) | (color_y & PAT_0_COLOR_Y);

	if (handle == NULL) {
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL3(id);
		reg |= readl(addr) & ~(PAT_0_COLOR_U | PAT_0_COLOR_Y);
		writel(reg, addr);
		addr = mmsyscfg->regs[0] + PAT_CTRL4(id);
		reg = color_v & PAT_0_COLOR_V;
		reg |= readl(addr) & ~PAT_0_COLOR_V;
		writel(reg, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL3(id),
				     reg, PAT_0_COLOR_U | PAT_0_COLOR_Y);
		reg = color_v & PAT_0_COLOR_V;
		ret |= cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL4(id),
				     reg, PAT_0_COLOR_V);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_config_fg_color);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Configure background color of pattern.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     color_y: Y component of YUV color.
 * @param[in]
 *     color_u: U component of YUV color.
 * @param[in]
 *     color_v: V component of YUV color.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. color_y/color_u/color_v ranges from 0 to 0x1ff, the excess will be
 *        truncated by bitwise and 0x1ff.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_config_bg_color(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_pat_gen_id id,
				      u32 color_y, u32 color_u, u32 color_v,
				      struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	reg = color_u & PAT_0_BG_COLOR_U;
	reg |= (color_v << 16) & PAT_0_BG_COLOR_V;

	if (handle == NULL) {
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL5(id);
		reg |= readl(addr) & ~(PAT_0_BG_COLOR_V | PAT_0_BG_COLOR_U);
		writel(reg, addr);
		addr = mmsyscfg->regs[0] + PAT_CTRL4(id);
		reg = (color_y << 16) & PAT_0_BG_COLOR_Y;
		reg |= readl(addr) & ~PAT_0_BG_COLOR_Y;
		writel(reg, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL5(id),
				     reg, PAT_0_BG_COLOR_V | PAT_0_BG_COLOR_U);
		reg = (color_y << 16) & PAT_0_BG_COLOR_Y;
		ret |= cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL4(id),
				     reg, PAT_0_BG_COLOR_Y);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_config_bg_color);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Select pure color as pattern type.\n
 *     It shows foreground color.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. color_y/color_u/color_v ranges from 0 to 0x1ff, the excess will be
 *        truncated by bitwise and 0x1ff.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_type_sel_pure_color(const struct device *mmsys_cfg_dev,
					  enum mtk_mmsys_pat_gen_id id,
					  struct cmdq_pkt **handle)
{
	int ret = 0;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	mmsyscfg->pat_cursor[id] = 0;

	if (handle == NULL) {
		u32 reg;
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL0(id);
		reg = readl(addr) & ~PAT_0_PAT_TYPE;
		writel(reg, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL0(id),
				     0, PAT_0_PAT_TYPE);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_type_sel_pure_color);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Select cursor as pattern type.\n
 *     Cursor line is foreground color, others display input data.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     show_cursor: Show cursor or not.
 * @param[in]
 *     pos_x: Position X of cursor.
 * @param[in]
 *     pos_y: Position Y of cursor.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. pos_x/pos_y ranges from 0 to 0x1ff, the excess will be truncated
 *        by bitwise and 0x1fff.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_type_sel_cursor(const struct device *mmsys_cfg_dev,
				      enum mtk_mmsys_pat_gen_id id,
				      bool show_cursor, u32 pos_x, u32 pos_y,
				      struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg = 1 << 16;
	u32 reg_pos;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	mmsyscfg->pat_cursor[id] = 1;
	if (show_cursor)
		reg |= PAT_0_CURSOR_SHOW;
	reg_pos = ((pos_y << 16) & REG_PAT_0_POS_Y) | (pos_x & REG_PAT_0_POS_X);

	if (handle == NULL) {
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL0(id);
		reg |= readl(addr) & ~(PAT_0_PAT_TYPE | PAT_0_CURSOR_SHOW);
		writel(reg, addr);
		addr = mmsyscfg->regs[0] + PAT_CTRL6(id);
		reg_pos |= readl(addr) & ~(REG_PAT_0_POS_Y | REG_PAT_0_POS_X);
		writel(reg_pos, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL0(id),
				     reg, PAT_0_PAT_TYPE | PAT_0_CURSOR_SHOW);
		ret |= cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL6(id),
				     reg_pos,
				     REG_PAT_0_POS_Y | REG_PAT_0_POS_X);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_type_sel_cursor);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Select ramp as pattern type.\n
 *     Ramp value starts from foreground color.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     y_en: Enable Y component of foreground color for ramp pattern gen.
 * @param[in]
 *     u_en: Enable U component of foreground color for ramp pattern gen.
 * @param[in]
 *     v_en: Enable V component of foreground color for ramp pattern gen.
 * @param[in]
 *     width: Pixel width for keeping ramp value.
 * @param[in]
 *     step: The value of ramp step.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. width ranges from 0 to 0x1fff, the excess will be truncated
 *        by bitwise and 0x1fff.\n
 *     3. step ranges from 0 to 0x3ff, the excess will be truncated
 *        by bitwise and 0x3ff.\n
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_type_sel_ramp(const struct device *mmsys_cfg_dev,
				    enum mtk_mmsys_pat_gen_id id,
				    bool y_en, bool u_en, bool v_en,
				    u32 width, u32 step,
				    struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg = 2 << 16;
	u32 reg_ramp;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	mmsyscfg->pat_cursor[id] = 0;
	if (y_en)
		reg |= BIT(10);
	if (u_en)
		reg |= BIT(9);
	if (v_en)
		reg |= BIT(8);
	reg_ramp = step & PAT_0_RAMP_H_STEP;
	reg_ramp |= (width << 16) & PAT_0_RAMP_H_WIDTH;

	if (handle == NULL) {
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL0(id);
		reg |= readl(addr) & ~(PAT_0_PAT_TYPE | PAT_0_RAMP_EN);
		writel(reg, addr);
		addr = mmsyscfg->regs[0] + PAT_CTRL7(id);
		reg_ramp |= readl(addr) &
			~(PAT_0_RAMP_H_WIDTH | PAT_0_RAMP_H_STEP);
		writel(reg_ramp, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL0(id),
				     reg, PAT_0_PAT_TYPE | PAT_0_RAMP_EN);
		ret |= cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL7(id),
				     reg_ramp,
				     PAT_0_RAMP_H_WIDTH | PAT_0_RAMP_H_STEP);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_type_sel_ramp);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Select grid as pattern type.\n
 *     Grid is foreground color, others are background color.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     size: The value for caculating display grid size.
 *           The formula is 2^(size+1), size should be <= 11.
 * @param[in]
 *     show_boundary: Show boundary or not.
 * @param[in]
 *     show_grid: Show grid or not.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configure pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. size ranges from 0 to 11, it will be set as 11 if it is excess.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_type_sel_grid(const struct device *mmsys_cfg_dev,
				    enum mtk_mmsys_pat_gen_id id,
				    u32 size, bool show_boundary,
				    bool show_grid,
				    struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg = 3 << 16;
	u32 mask;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	mmsyscfg->pat_cursor[id] = 0;
	if (size > 11)
		size = 11;
	reg |= size << 4;
	if (show_boundary)
		reg |= BIT(3);
	if (show_grid)
		reg |= BIT(2);
	mask = PAT_0_PAT_TYPE | PAT_0_GRID_SIZE | PAT_0_GRID_SHOW_EN;

	if (handle == NULL) {
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL0(id);
		reg |= readl(addr) & ~mask;
		writel(reg, addr);
		addr = mmsyscfg->regs[0] + PAT_CTRL6(id);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL0(id),
				     reg, mask);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_type_sel_grid);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Enable Pattern Generator function.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     force_input_valid: force input of DSC valid or not. If RDMA is enabled
 *         and DSC can has valid input, set it false. Otherwise, set it true.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, enable pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.\n
 *     3. please call mtk_mmsys_cfg_pat_config_xxx for common configurations
 *        and mtk_mmsys_cfg_pat_type_sel_xxx to select a pattern type
 *        before enabling pattern generator function.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_gen_eanble(const struct device *mmsys_cfg_dev,
				 enum mtk_mmsys_pat_gen_id id,
				 bool force_input_valid,
				 struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 reg = PAT_0_PAT_GEN_EN;
	u32 reg_pos;
	u32 mask = PAT_0_PAT_FORCE_VALID | PAT_0_PAT_GEN_EN;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	reg_pos = (mmsyscfg->pat_act_h[id] << 16) | mmsyscfg->pat_act_w[id];

	if (force_input_valid)
		reg |= PAT_0_PAT_FORCE_VALID;

	if (handle == NULL) {
		void *addr;

		if (mmsyscfg->pat_cursor[id] == 0) {
			addr = mmsyscfg->regs[0] + PAT_CTRL6(id);
			reg_pos |= readl(addr) &
					~(REG_PAT_0_POS_Y | REG_PAT_0_POS_X);
			writel(reg_pos, addr);
		}
		addr = mmsyscfg->regs[0] + PAT_CTRL0(id);
		reg |= readl(addr) & ~mask;
		writel(reg, addr);
	} else {
		if (mmsyscfg->pat_cursor[id] == 0) {
			ret |= cmdq_pkt_write_value(handle[0],
				     mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL6(id),
				     reg_pos,
				     REG_PAT_0_POS_Y | REG_PAT_0_POS_X);
		}
		ret |= cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL0(id),
				     reg,
				     PAT_0_PAT_FORCE_VALID | PAT_0_PAT_GEN_EN);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_gen_eanble);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Disable Pattern Generator Function.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     id: Pattern Generator index.
 * @param[in]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, disable pattern generator successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mmsys_cfg_dev can not be NULL.\n
 *     2. id should be smaller than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Error case and Error handling
 *     1. return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2. return -EINVAL if id is large than MMSYSCFG_PAT_GEN_ID_MAX.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_pat_gen_disable(const struct device *mmsys_cfg_dev,
				 enum mtk_mmsys_pat_gen_id id,
				 struct cmdq_pkt **handle)
{
	int ret = 0;
	u32 mask = PAT_0_PAT_FORCE_VALID | PAT_0_PAT_GEN_EN;
	struct mtk_mmsys_config *mmsyscfg;

	if (!mmsys_cfg_dev)
		return -EINVAL;
	if (id < 0 || id >= MMSYSCFG_PAT_GEN_ID_MAX) {
		dev_err(mmsys_cfg_dev, "wrong pattern gen index\n");
		return -EINVAL;
	}

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	if (handle == NULL) {
		u32 reg;
		void *addr;

		addr = mmsyscfg->regs[0] + PAT_CTRL0(id);
		reg = readl(addr) & ~mask;
		writel(reg, addr);
	} else {
		ret = cmdq_pkt_write_value(handle[0], mmsyscfg->gce_subsys[0],
				     mmsyscfg->gce_subsys_offset[0] +
				     PAT_CTRL0(id),
				     0, mask);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_pat_gen_disable);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Enable CRC of display path.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     flags: Specify which module's CRC will be enabled. It's combination of
 *            MMSYS_CRC_xxx, e.g. MMSYS_CRC_RDMA_0_TO_PAT | MMSYS_CRC_LHC_IN_0.
 * @return
 *     0, successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.\n
 * @par Error case and Error handling
 *     return -EINVAL if mmsys_cfg_dev is NULL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_crc_enable(const struct device *mmsys_cfg_dev, u32 flags)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 reg;
	void *addr;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	addr = mmsyscfg->regs[0] + MMSYS_CRC_CTRL_EN;
	reg = readl(addr) | flags;
	writel(reg, addr);

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_crc_enable);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Disable CRC of display path.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     flags: Specify which module's CRC will be disabled. It's combination of
 *            MMSYS_CRC_xxx, e.g. MMSYS_CRC_RDMA_0_TO_PAT | MMSYS_CRC_LHC_IN_0.
 * @return
 *     0, successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.\n
 * @par Error case and Error handling
 *     return -EINVAL if mmsys_cfg_dev is NULL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_crc_disable(const struct device *mmsys_cfg_dev, u32 flags)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 reg;
	void *addr;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	addr = mmsyscfg->regs[0] + MMSYS_CRC_CTRL_EN;
	reg = readl(addr) & ~flags;
	writel(reg, addr);

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_crc_disable);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Clear CRC of display path.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     flags: Specify which module's CRC will be cleared. It's combination of
 *            MMSYS_CRC_xxx, e.g. MMSYS_CRC_RDMA_0_TO_PAT | MMSYS_CRC_LHC_IN_0.
 * @return
 *     0, successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     mmsys_cfg_dev can not be NULL.\n
 * @par Error case and Error handling
 *     return -EINVAL if mmsys_cfg_dev is NULL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_crc_clear(const struct device *mmsys_cfg_dev, u32 flags)
{
	struct mtk_mmsys_config *mmsyscfg;
	u32 reg;
	void *addr;

	if (!mmsys_cfg_dev)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;
	addr = mmsyscfg->regs[0] + MMSYS_CRC_CTRL_CLR;
	reg = readl(addr);
	writel(reg | flags, addr);
	writel(reg & ~flags, addr);

	return 0;

}
EXPORT_SYMBOL(mtk_mmsys_cfg_crc_clear);

/** @ingroup IP_group_mmsys_cfg_external_function
 * @par Description
 *     Read CRC of display path.
 * @param[in]
 *     mmsys_cfg_dev: mmsys_cfg device node.
 * @param[in]
 *     flags: Specify which module's CRC will be read. It's combination of
 *            MMSYS_CRC_xxx, e.g. MMSYS_CRC_RDMA_0_TO_PAT | MMSYS_CRC_LHC_IN_0.
 * @param[out]
 *     crc_out: Pointer to CRC values which will be read.
 * @param[out]
 *     crc_num: Pointer to pixels count which will be read and used to
 *              caculate CRC. Can be NULL.
 * @param[in]
 *     mod_cnt: Specify how many modules' CRC will be read.
 * @return
 *     0, successfully.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1.mmsys_cfg_dev can not be NULL.\n
 *     2.flags can not be zero.\n
 *     3.crc_out can not be NULL.\n
 *     4.mod_cnt should equal or be larger than the number of bits which are
 *       set to 1 in flags.
 * @par Error case and Error handling
 *     1.return -EINVAL if mmsys_cfg_dev is NULL.\n
 *     2.return -EINVAL if flags equals 0 or the number of bits which are set
 *       to 1 in flags is larger than mod_cnt.\n
 *     3.return -EINVAL if crc_out is NULL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mmsys_cfg_crc_read(const struct device *mmsys_cfg_dev, u32 flags,
			  u32 *crc_out, u32 *crc_num, u32 mod_cnt)
{
	struct mtk_mmsys_config *mmsyscfg;
	unsigned long i;
	unsigned long size = fls(flags);
	unsigned long tmp = flags;

	if (!mmsys_cfg_dev || !crc_out)
		return -EINVAL;
	if (flags == 0u || (u32)size > mod_cnt)
		return -EINVAL;

	mmsyscfg = dev_get_drvdata(mmsys_cfg_dev);
	if (IS_ERR_OR_NULL(mmsyscfg))
		return -EINVAL;

	for_each_set_bit(i, &tmp, size) {
		*crc_out++ = readl(mmsyscfg->regs[0] + CRC_OUT(i));
		if (crc_num)
			*crc_num++ = readl(mmsyscfg->regs[0] + CRC_NUM(i));
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mmsys_cfg_crc_read);

/** @ingroup IP_group_mmsys_cfg_internal_function
 * @par Description
 *     camera vsync interrupt handler.
 * @param[in]
 *     irq: irq number.
 * @param[out]
 *     dev_id: mmsys_cfg device node.
 * @return
 *     IRQ_HANDLED, irq handler execute done.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_camera_vsync_irq_handler(int irq, void *dev_id)
{
	struct mtk_mmsys_config *mmsyscfg = dev_id;
	u32 i, int_sta = 0, cb_status;
	unsigned long flags;
	mtk_mmsys_cb cb;
	struct mtk_mmsys_cb_data cb_data;

	for (i = 0; i < CAMERA_VSYNC_IRQ_CNT; i++) {
		if  (irq == mmsyscfg->cam_vsync_irq[i]) {
			void *addr = mmsyscfg->regs[0] + VSGEN_IRQ_STATUS;
			u32 reg;

			int_sta = readl(addr);
			int_sta = ~int_sta; /* low active */
			int_sta &= VSGEN_IRQ_STATUS_MON;

			addr = mmsyscfg->regs[0] + VSGEN_IRQ_CTRL;
			reg = readl(addr) & ~VSGEN_IRQ_CLR;
			writel(reg | (int_sta << 16), addr);
			writel(reg, addr);
			break;
		}
	}

	if (i == MMSYSCFG_CAMERA_SYNC_MAX)
		return IRQ_NONE;

	spin_lock_irqsave(&mmsyscfg->lock_irq, flags);
	cb_data = mmsyscfg->cb_data;
	cb = mmsyscfg->cb_func;
	cb_status = mmsyscfg->cb_status;
	spin_unlock_irqrestore(&mmsyscfg->lock_irq, flags);

	if ((cb_status & int_sta) && cb) {
		cb_data.part_id = 0;
		cb_data.status = int_sta;
		cb(&cb_data);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_MTK_DEBUGFS
static ssize_t mtk_mmsys_cfg_debug_read(struct file *file, char __user *ubuf,
				size_t count, loff_t *ppos)
{
	struct mtk_mmsys_config *mmsyscfg = file->private_data;
	char *dir = "/sys/kernel/debug/mediatek/mmsyscfg";
	int len = 0, size = 200;
	ssize_t ret = 0;
	char *usage;

	usage = kzalloc(size, GFP_KERNEL);
	if (usage == NULL)
		return -ENOMEM;

	len += scnprintf(usage + len, size - len, "[USAGE]\n");
	len += scnprintf(usage + len, size - len,
		"   echo [ACTION]... > %s/%s\n", dir, mmsyscfg->file_name);
	len += scnprintf(usage + len, size - len, "[ACTION]\n");
	len += scnprintf(usage + len, size - len, "   reg_r <offset> <size>\n");
	len += scnprintf(usage + len, size - len, "   path_info\n");

	if (len > size)
		len = size;
	ret = simple_read_from_buffer(ubuf, count, ppos, usage, len);
	kfree(usage);

	return ret;
}

static void mtk_mmsys_cfg_debug_reg_read(struct mtk_mmsys_config *mmsyscfg,
					u32 offset, u32 size)
{
	void *r;
	u32 i, j, t1, t2, cnt;

	if (offset >= 0x1000 || (offset + size) > 0x1000) {
		dev_err(mmsyscfg->dev, "Specified register range overflow!\n");
		return;
	}

	t1 = rounddown(offset, 16);
	t2 = roundup(offset + size, 16);

	pr_err("---------------- %s reg_r 0x%x 0x%x ----------------\n",
		mmsyscfg->file_name, offset, size);
	cnt = min_t(u32, mmsyscfg->hw_nr, (u32)ARRAY_SIZE(mmsyscfg->regs));
	for (i = 0; i < cnt; i++) {
		if (mmsyscfg->hw_nr > 1)
			pr_err("-------- %s[%d]\n", mmsyscfg->file_name, i);
		for (j = t1; j < t2; j += 16) {
			r = mmsyscfg->regs[i] + j;
			pr_err("%X | %08X %08X %08X %08X\n",
				mmsyscfg->reg_base[i] + j,
				readl(r), readl(r + 4),
				readl(r + 8), readl(r + 12));
		}
	}
	pr_err("---------------------------------------------------\n");
}


struct mmsys_comp_name_map_t {
	u32		   prev_reg_offset;
	const char * const curr[CNR];
	const char * const next[NNR];
	const char * const mux;
	u32		   reg_offset;
};

struct mmsys_comp_in_mux_t {
	const char * const curr[CNR];
	u32 reg_offset;
};

struct mmsys_comp_in_sel_name_map_t {
	struct mmsys_comp_in_mux_t in[CNR];
	const char * const next[NNR];
	const char * const mux;
	u32		   reg_offset;
};

static struct mmsys_comp_name_map_t const s_mout_name_map[] = {
	{
		0,
		{
			"slicer_vid",
			NULL
		},
		{
			"crop0 -> rsz0",
			"lhc0",
			"disp_rdma0",
			NULL
		},
		"MMSYS_SLCR_MOUT_EN0",
		MMSYS_SLCR_MOUT_EN0
	}, {
		0,
		{
			"slicer_vid",
			NULL
		},
		{
			"crop1 - > rsz1",
			"lhc1",
			"disp_rdma1",
			NULL
		},
		"MMSYS_SLCR_MOUT_EN1",
		MMSYS_SLCR_MOUT_EN1
	}, {
		0,
		{
			"slicer_vid",
			NULL
		},
		{
			"crop2",
			"lhc2",
			"disp_rdma2",
			NULL
		},
		"MMSYS_SLCR_MOUT_EN2",
		MMSYS_SLCR_MOUT_EN2
	}, {
		0,
		{
			"slicer_vid",
			NULL
		},
		{
			"crop3",
			"lhc3",
			"disp_rdma3",
			NULL
		},
		"MMSYS_SLCR_MOUT_EN3",
		MMSYS_SLCR_MOUT_EN3
	}, {
		MMSYS_RDMA_OUT_SEL0,
		{
			"disp_rdma0", /* option 0 */
			"mdp_rdma_pvric0", /* option 1 */
			"mdp_rdma0", /* option 2 */
			NULL
		},
		{
			"lhc0",
			"dp_pat_gen0 -> dsc0",
			NULL
		},
		"MMSYS_RDMA_MOUT_EN0",
		MMSYS_RDMA_MOUT_EN0
	}, {
		MMSYS_RDMA_OUT_SEL1,
		{
			"disp_rdma1", /* option 0 */
			"mdp_rdma_pvric1", /* option 1 */
			"mdp_rdma1", /* option 2 */
			NULL
		},
		{
			"lhc1",
			"dp_pat_gen1 -> dsc0",
			NULL
		},
		"MMSYS_RDMA_MOUT_EN1",
		MMSYS_RDMA_MOUT_EN1
	}, {
		MMSYS_RDMA_OUT_SEL2,
		{
			"disp_rdma2", /* option 0 */
			"mdp_rdma_pvric2", /* option 1 */
			"mdp_rdma2", /* option 2 */
			NULL
		},
		{
			"lhc2",
			"dp_pat_gen2 -> dsc1",
			NULL
		},
		"MMSYS_RDMA_MOUT_EN2",
		MMSYS_RDMA_MOUT_EN2
	}, {
		MMSYS_RDMA_OUT_SEL3,
		{
			"disp_rdma3", /* option 0 */
			"mdp_rdma_pvric3", /* option 1 */
			"mdp_rdma3", /* option 2 */
			NULL
		},
		{
			"lhc3",
			"dp_pat_gen3 -> dsc1",
			NULL
		},
		"MMSYS_RDMA_MOUT_EN3",
		MMSYS_RDMA_MOUT_EN3
	}, {
		0,
		{
			"rsz1",
			NULL
		},
		{
			"disp_wdma1",
			"p2s0",
			NULL
		},
		"MMSYS_RSZ_MOUT_EN",
		MMSYS_RSZ_MOUT_EN
	}, {
		0,
		{
			"dsc0",
			NULL
		},
		{
			"dsi_lane_swap",
			"disp_wdma0",
			NULL
		},
		"MMSYS_DSC_MOUT_EN0",
		MMSYS_DSC_MOUT_EN0
	}, {
		0,
		{
			"dsc0",
			NULL
		},
		{
			"dsi_lane_swap",
			"disp_wdma1",
			NULL
		},
		"MMSYS_DSC_MOUT_EN1",
		MMSYS_DSC_MOUT_EN1
	}, {
		0,
		{
			"dsc1",
			NULL
		},
		{
			"dsi_lane_swap",
			"disp_wdma2",
			NULL
		},
		"MMSYS_DSC_MOUT_EN2",
		MMSYS_DSC_MOUT_EN2
	}, {
		0,
		{
			"dsc1",
			NULL
		},
		{
			"dsi_lane_swap",
			"disp_wdma3",
			NULL
		},
		"MMSYS_DSC_MOUT_EN3",
		MMSYS_DSC_MOUT_EN3
	}
};

static struct mmsys_comp_in_sel_name_map_t const s_in_sel_name_map[] = {
	{
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "slicer_dsc", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_rdma0",
			NULL
		},
		"MMSYS_RDMA_IN_SEL0",
		MMSYS_RDMA_IN_SEL0
	}, {
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "slicer_dsc", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_rdma1",
			NULL
		},
		"MMSYS_RDMA_IN_SEL1",
		MMSYS_RDMA_IN_SEL1
	}, {
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "slicer_dsc", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_rdma2", NULL
		},
		"MMSYS_RDMA_IN_SEL2", MMSYS_RDMA_IN_SEL2
	}, {
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "slicer_dsc", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_rdma3",
			NULL
		},
		"MMSYS_RDMA_IN_SEL3",
		MMSYS_RDMA_IN_SEL3
	}, {
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "disp_rdma0", "mdp_rdma_pvric0",
				"mdp_rdma0", NULL },
				MMSYS_RDMA_OUT_SEL0 },
			{ {NULL}, 0 }
		},
		{
			"lhc0",
			NULL
		},
		"MMSYS_LHC_SEL0",
		MMSYS_LHC_SEL0
	}, {
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "disp_rdma1", "mdp_rdma_pvric1",
				"mdp_rdma1", NULL },
				MMSYS_RDMA_OUT_SEL1 },
			{ {NULL}, 0 }
		},
		{
			"lhc1",
			NULL
		},
		"MMSYS_LHC_SEL1",
		MMSYS_LHC_SEL1
	}, {
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "disp_rdma2", "mdp_rdma_pvric2",
				"mdp_rdma2", NULL },
				MMSYS_RDMA_OUT_SEL2 },
			{ {NULL}, 0 }
		},
		{
			"lhc2",
			NULL
		},
		"MMSYS_LHC_SEL2",
		MMSYS_LHC_SEL2
	},  {
		{
			{ { "slicer_vid", NULL }, 0 },
			{ { "disp_rdma3", "mdp_rdma_pvric3",
				"mdp_rdma3", NULL },
				MMSYS_RDMA_OUT_SEL3 },
			{ {NULL}, 0 }
		},
		{
			"lhc3",
			NULL
		},
		"MMSYS_LHC_SEL3",
		MMSYS_LHC_SEL3
	}, {
		{
			{ { "p2s0", NULL }, 0 },
			{ { "dsc0", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_wdma0",
			NULL
		},
		"MMSYS_WDMA_SEL0",
		MMSYS_WDMA_SEL0
	}, {
		{
			{ { "rsz1", NULL }, 0 },
			{ { "dsc0", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_wdma1",
			NULL
		},
		"MMSYS_WDMA_SEL1",
		MMSYS_WDMA_SEL1
	}, {
		{
			{ { "crop2", NULL }, 0 },
			{ { "dsc1", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_wdma2",
			NULL
		},
		"MMSYS_WDMA_SEL2",
		MMSYS_WDMA_SEL2
	}, {
		{
			{ { "crop3", NULL }, 0 },
			{ { "dsc1", NULL }, 0 },
			{ { NULL }, 0 }
		},
		{
			"disp_wdma3",
			NULL
		},
		"MMSYS_WDMA_SEL3",
		MMSYS_WDMA_SEL3
	}
};

static void mtk_mmsys_cfg_debug_mout_info(struct mtk_mmsys_config *mmsyscfg)
{
	const struct mmsys_comp_name_map_t *map = s_mout_name_map;
	u32 i, j, cnt;
	unsigned long val;
	const char *curr;
	void *addr;

	addr = mmsyscfg->regs[0];
	cnt = ARRAY_SIZE(s_mout_name_map);

	for (i = 0; i < cnt; i++, map++) {
		val = readl(addr + map->reg_offset);
		if (val == 0)
			continue;
		if (map->prev_reg_offset == 0) {
			curr = map->curr[0];
		} else {
			u32 tmp;

			tmp = readl(addr + map->prev_reg_offset);
			if (tmp < CNR && map->curr[tmp])
				curr = map->curr[tmp];
			else
				curr = "unkonwn";
		}
		pr_err("%s: %x\n", map->mux, (u32)val);
		for  (j = 0; j < NNR; j++) {
			if (test_bit(j, &val) && map->next[j])
				pr_err("    %s -> %s\n", curr, map->next[j]);
		}
	}
}

static void mtk_mmsys_cfg_debug_in_sel_info(struct mtk_mmsys_config *mmsyscfg)
{
	const struct mmsys_comp_in_sel_name_map_t *map = s_in_sel_name_map;
	const struct mmsys_comp_in_mux_t *in;
	u32 i, cnt, val;
	const char *curr;
	void *addr;

	addr = mmsyscfg->regs[0];
	cnt = ARRAY_SIZE(s_in_sel_name_map);

	for (i = 0; i < cnt; i++, map++) {
		val = readl(addr + map->reg_offset);
		pr_err("%s: %x\n", map->mux, val);
		if (val >= CNR)
			continue;
		in = &map->in[val];
		if (in->reg_offset == 0) {
			curr = in->curr[0];
		} else {
			u32 tmp;

			tmp = readl(addr + in->reg_offset);
			if (tmp < CNR && in->curr[tmp])
				curr = in->curr[tmp];
			else
				curr = "unkonwn";
		}

		pr_err("    %s -> %s\n", curr, map->next[0]);
	}
}

static void mtk_mmsys_cfg_debug_path_info(struct mtk_mmsys_config *mmsyscfg)
{
	pr_err("---------------- %s path_info ----------------\n",
		mmsyscfg->file_name);
	pr_err("----------------mout info:\n");
	mtk_mmsys_cfg_debug_mout_info(mmsyscfg);
	pr_err("----------------in sel info:\n");
	mtk_mmsys_cfg_debug_in_sel_info(mmsyscfg);
	pr_err("----------------------------------------------\n");
}

static ssize_t mtk_mmsys_cfg_debug_write(struct file *file,
			const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct mtk_mmsys_config *mmsyscfg = file->private_data;
	char buf[64];
	size_t buf_size = min(count, sizeof(buf) - 1);

	if (copy_from_user(buf, ubuf, buf_size))
		return -EFAULT;
	buf[buf_size] = '\0';

	dev_err(mmsyscfg->dev, "debug cmd: %s\n", buf);
	if (strncmp(buf, "reg_r", 5) == 0) {
		int ret;
		u32 offset = 0, size = 16;

		ret = sscanf(buf, "reg_r %x %x", &offset, &size);
		if (ret == 2)
			mtk_mmsys_cfg_debug_reg_read(mmsyscfg, offset, size);
		else
			pr_err("Correct your input! (reg_r <offset> <size>)\n");

	} else if (strncmp(buf, "path_info", 9) == 0) {
		mtk_mmsys_cfg_debug_path_info(mmsyscfg);
	} else {
		dev_err(mmsyscfg->dev, "unknown command %s\n", buf);
	}

	return count;
}

static const struct file_operations mtk_mmsys_cfg_debug_fops = {
	.owner = THIS_MODULE,
	.open		= simple_open,
	.read		= mtk_mmsys_cfg_debug_read,
	.write		= mtk_mmsys_cfg_debug_write,
	.llseek		= default_llseek,
};

static void mtk_mmsys_cfg_debugfs_init(struct mtk_mmsys_config *mmsyscfg,
					const char *name)
{
	static struct dentry *debugfs_dir;
	struct dentry *file;

	if (!mtk_debugfs_root) {
		dev_err(mmsyscfg->dev, "No mtk debugfs root!\n");
		return;
	}

	mmsyscfg->debugfs = &debugfs_dir;
	if (debugfs_dir == NULL) {
		debugfs_dir = debugfs_create_dir("mmsyscfg", mtk_debugfs_root);
		if (IS_ERR(debugfs_dir)) {
			dev_err(mmsyscfg->dev,
				"failed to create debug dir (%p)\n",
				debugfs_dir);
			debugfs_dir = NULL;
			return;
		}
	}
	file = debugfs_create_file(name, 0664, debugfs_dir,
				   (void *)mmsyscfg, &mtk_mmsys_cfg_debug_fops);
	if (IS_ERR(file)) {
		dev_err(mmsyscfg->dev, "failed to create debug file (%p)\n",
			file);
		debugfs_remove_recursive(debugfs_dir);
		debugfs_dir = NULL;
		return;
	}
	mmsyscfg->file_name = name;
}
#endif

/** @ingroup IP_group_mmsys_cfg_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.\n
 *     There are HW number, clock node, and gce related information
 *     which will be parsed from device tree.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, function success.\n
 *     -ENOMEM, no memory.\n
 *     PTR_ERR, devm_ioremap_resource fails.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. return -ENOMEM if memory allocation fails.\n
 *     2. return PTR_ERR() if devm_ioremap_resource returns error.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_cfg_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mmsys_config *mmsyscfg;
	struct resource *regs;
	int i;

	dev_dbg(dev, "mtk_mmsys_cfg_probe\n");

	mmsyscfg = devm_kzalloc(dev, sizeof(*mmsyscfg), GFP_KERNEL);
	if (!mmsyscfg)
		return -ENOMEM;

	for (i = 0; i < CAMERA_VSYNC_IRQ_CNT; i++) {
		int irq, ret;

		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(dev, "Failed to get irq %x\n", irq);
			return -EINVAL;
		}
		mmsyscfg->cam_vsync_irq[i] = irq;
		ret = devm_request_irq(dev, irq, mtk_camera_vsync_irq_handler,
				       IRQF_TRIGGER_LOW, dev_name(dev),
				       mmsyscfg);
		if (ret < 0) {
			dev_err(dev, "Failed to request irq %d:%d\n", irq, ret);
			return ret;
		}
	}

	spin_lock_init(&mmsyscfg->lock_irq);

	of_property_read_u32(dev->of_node, "hw-number", &mmsyscfg->hw_nr);
	if (mmsyscfg->hw_nr > MODULE_MAX_NUM) {
		dev_err(dev, "Wrong \"hw-number\" value %u, max %u\n",
			mmsyscfg->hw_nr, MODULE_MAX_NUM);
		return -EINVAL;
	}

	for (i = 0; i < mmsyscfg->hw_nr; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		mmsyscfg->regs[i] = devm_ioremap_resource(dev, regs);
		mmsyscfg->gce_subsys_offset[i] = regs->start & 0xffff;
		if (IS_ERR(mmsyscfg->regs[i])) {
			dev_err(dev, "Failed to map mmsyscfg%d registers\n", i);
			return PTR_ERR(mmsyscfg->regs[i]);
		}
		#ifdef CONFIG_MTK_DEBUGFS
		mmsyscfg->reg_base[i] = regs->start;
		#endif
	}
	of_property_read_u32_array(dev->of_node, "gce-subsys",
				   mmsyscfg->gce_subsys, mmsyscfg->hw_nr);

#ifdef CONFIG_COMMON_CLK_MT3612
	for (i = 0; i < ARRAY_SIZE(mmsyscfg->clk_pat_gen); i++) {
		mmsyscfg->clk_pat_gen[i] = of_clk_get(dev->of_node, i);
		if (IS_ERR(mmsyscfg->clk_pat_gen[i])) {
			dev_err(dev, "Failed to get pat gen[%d] clock\n", i);
			return PTR_ERR(mmsyscfg->clk_pat_gen[i]);
		}
	}
	for (i = 0; i < ARRAY_SIZE(mmsyscfg->clk_dsc_mout); i++) {
		mmsyscfg->clk_dsc_mout[i] = of_clk_get(dev->of_node, 4 + i);
		if (IS_ERR(mmsyscfg->clk_dsc_mout[i])) {
			dev_err(dev, "Failed to get dsc mout[%d] clock\n", i);
			return PTR_ERR(mmsyscfg->clk_dsc_mout[i]);
		}
	}
	for (i = 0; i < ARRAY_SIZE(mmsyscfg->clk_rdma_mout); i++) {
		mmsyscfg->clk_rdma_mout[i] = of_clk_get(dev->of_node, 8 + i);
		if (IS_ERR(mmsyscfg->clk_rdma_mout[i])) {
			dev_err(dev, "Failed to get rdma mout[%d] clock\n", i);
			return PTR_ERR(mmsyscfg->clk_rdma_mout[i]);
		}
	}
	mmsyscfg->clk_rsz0_mout = of_clk_get(dev->of_node, 12);
	if (IS_ERR(mmsyscfg->clk_rsz0_mout)) {
		dev_err(dev, "Failed to get rsz0 mout clock\n");
		return PTR_ERR(mmsyscfg->clk_rsz0_mout);
	}
	mmsyscfg->clk_dsi_swap = of_clk_get(dev->of_node, 13);
	if (IS_ERR(mmsyscfg->clk_dsi_swap)) {
		dev_err(dev, "Failed to get dsi swap clock\n");
		return PTR_ERR(mmsyscfg->clk_dsi_swap);
	}
	mmsyscfg->clk_lhc_swap = of_clk_get(dev->of_node, 14);
	if (IS_ERR(mmsyscfg->clk_lhc_swap)) {
		dev_err(dev, "Failed to get dsi swap clock\n");
		return PTR_ERR(mmsyscfg->clk_lhc_swap);
	}
	mmsyscfg->clk_event_tx = of_clk_get(dev->of_node, 15);
	if (IS_ERR(mmsyscfg->clk_event_tx)) {
		dev_err(dev, "Failed to get event tx clock\n");
		return PTR_ERR(mmsyscfg->clk_event_tx);
	}
	mmsyscfg->clk_camera_sync = of_clk_get(dev->of_node, 16);
	if (IS_ERR(mmsyscfg->clk_camera_sync)) {
		dev_err(dev, "Failed to get event tx clock\n");
		return PTR_ERR(mmsyscfg->clk_camera_sync);
	}
	mmsyscfg->clk_crc = of_clk_get(dev->of_node, 17);
	if (IS_ERR(mmsyscfg->clk_crc)) {
		dev_err(dev, "Failed to get event tx clock\n");
		return PTR_ERR(mmsyscfg->clk_crc);
	}
	mmsyscfg->clk_sync_sel = of_clk_get(dev->of_node, 18);
	if (IS_ERR(mmsyscfg->clk_sync_sel)) {
		dev_err(dev, "Failed to get camera sync sel\n");
		return PTR_ERR(mmsyscfg->clk_sync_sel);
	}
	mmsyscfg->pll_26M = of_clk_get(dev->of_node, 19);
	if (IS_ERR(mmsyscfg->pll_26M)) {
		dev_err(dev, "Failed to get PLL 26M\n");
		return PTR_ERR(mmsyscfg->pll_26M);
	}
	mmsyscfg->pll_26M_D10 = of_clk_get(dev->of_node, 20);
	if (IS_ERR(mmsyscfg->pll_26M_D10)) {
		dev_err(dev, "Failed to get PLL 2.6M\n");
		return PTR_ERR(mmsyscfg->pll_26M_D10);
	}

	pm_runtime_enable(dev);
#endif

	mmsyscfg->dev = dev;
	platform_set_drvdata(pdev, mmsyscfg);
#ifdef CONFIG_MTK_DEBUGFS
	mtk_mmsys_cfg_debugfs_init(mmsyscfg, pdev->name);
#endif

	return 0;
}

/** @ingroup IP_group_mmsys_cfg_internal_function
 * @par Description
 *     Do Nothing.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mmsys_cfg_remove(struct platform_device *pdev)
{
	struct mtk_mmsys_config *mmsyscfg;

	mmsyscfg = platform_get_drvdata(pdev);
#ifdef CONFIG_MTK_DEBUGFS
	if (mmsyscfg->debugfs) {
		debugfs_remove_recursive(*mmsyscfg->debugfs);
		*mmsyscfg->debugfs = NULL;
	}
#endif
#ifdef COMMON_CLK_MT3612
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

/** @ingroup IP_group_mmsys_cfg_internal_struct
 * @brief Mmsys_cfg Open Framework Device ID.\n
 * This structure is used to attach specific names to
 * platform device for use with device tree.
 */
static const struct of_device_id mmsyscfg_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-mmsyscfg"},
	{},
};

MODULE_DEVICE_TABLE(of, mmsyscfg_driver_dt_match);

/** @ingroup IP_group_mmsys_cfg_internal_struct
 * @brief Mmsys_cfg platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_mmsyscfg_driver = {
	.probe = mtk_mmsys_cfg_probe,
	.remove = mtk_mmsys_cfg_remove,
	.driver = {
		   .name = "mediatek-mmsyscfg",
		   .owner = THIS_MODULE,
		   .of_match_table = mmsyscfg_driver_dt_match,
	},
};

module_platform_driver(mtk_mmsyscfg_driver);
MODULE_LICENSE("GPL");
