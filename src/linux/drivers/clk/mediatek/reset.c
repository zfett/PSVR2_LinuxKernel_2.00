/*
 * Copyright (c) 2014 MediaTek Inc.
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

#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/infracfg.h>
#include <linux/soc/mediatek/infracfg_ao_reg.h>
#include <dt-bindings/reset-controller/mt3612-resets.h>
#include "clk-mtk.h"

#define UNLOCK_KEY 0x88
#define KEY_SHIFT 24
#define RESET_BUS_PROTECT

struct mtk_reset {
	struct regmap *regmap;
#ifdef RESET_BUS_PROTECT
	struct regmap *infra_regmap;
#endif
	int regofs;
	struct reset_controller_dev rcdev;
};

#ifdef RESET_BUS_PROTECT
static int set_bus_protect(int regofs, struct regmap *infrareg, int id)
{
	int ret = 0;

	if (regofs == MT3612_TOPRGU_OFS) {
		switch (id) {
		case MT3612_TOPRGU_MMSYS_CORE_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			GENMASK(1, 0) | GENMASK(22, 21));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	GENMASK(1, 0));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, GENMASK(22, 21));
			break;
		case MT3612_TOPRGU_MM_RST: /* MMSYS_COM */
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(2) | GENMASK(24, 23));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(2));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, GENMASK(24, 23));
			break;
		case MT3612_TOPRGU_MMSYS_GAZE_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(3) | BIT(16));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(3));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(16));
			break;
		case MT3612_TOPRGU_CAM_SIDE0_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(6) | BIT(12));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(6));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(12));
			break;
		case MT3612_TOPRGU_CAM_SIDE1_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(8) | BIT(13));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(8));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(13));
			break;
		case MT3612_TOPRGU_IMG_SIDE0_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(7) | BIT(14) | BIT(19));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(7));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(14));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(19));
			break;
		case MT3612_TOPRGU_IMG_SIDE1_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(9) | BIT(15) | BIT(20));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(9));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(15));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(20));
			break;
		case MT3612_TOPRGU_ISP_GAZE0_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(10) | BIT(17));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(10));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(17));
			break;
		case MT3612_TOPRGU_ISP_GAZE1_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(11) | BIT(18));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(11));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(18));
			break;
		case MT3612_TOPRGU_VPU_CONN_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(16));
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			GENMASK(5, 4) | GENMASK(26, 25));
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(25));
			mtk_infracfg_set_bus_protection(infrareg, SMI_PROTECTEN,
			SMI_PROTECTSTA1, GENMASK(5, 4) | GENMASK(26, 25));
			break;
		case MT3612_TOPRGU_VPU0_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(29));
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(12) | BIT(15) | BIT(19) | BIT(26));
			break;
		case MT3612_TOPRGU_VPU1_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(30));
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(13) | BIT(17) | BIT(20) | BIT(27));
			break;
		case MT3612_TOPRGU_VPU2_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(31));
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(14) | BIT(18) | BIT(21) | BIT(28));
			break;
		case MT3612_TOPRGU_MFG_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			GENMASK(25, 22));
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(24));
			mtk_infracfg_set_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	GENMASK(28, 27));
			break;
		default:
			break;
		}
	} else if (regofs == MT3612_INFRACFG_OFS) {
		switch (id) {
		case MT3612_INFRA_SCP_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(15));
			break;
		case MT3612_INFRA_GCE4_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(7));
			break;
		case MT3612_INFRA_GCE5_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(8));
			break;
		case MT3612_INFRA_THERMAL_CTRL_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_1, INFRA_TOPAXI_PROTECTSTA1_1,
			BIT(30));
			break;
		default:
			break;
		}
	} else if (regofs == MT3612_PERICFG_OFS) {
		switch (id) {
		case MT3612_PERI_IOMMU_SW_RST:
			mtk_infracfg_set_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(19));
			break;
		default:
			break;
		}
	} else {
		ret = 0;
	}
	return ret;
}

static int clear_bus_protect(int regofs, struct regmap *infrareg, int id)
{
	int ret = 0;

	if (regofs == MT3612_TOPRGU_OFS) {
		switch (id) {
		case MT3612_TOPRGU_MMSYS_CORE_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			GENMASK(1, 0) | GENMASK(22, 21));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	GENMASK(1, 0));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, GENMASK(22, 21));
			break;
		case MT3612_TOPRGU_MM_RST: /* MMSYS_COM */
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(2) | GENMASK(24, 23));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(2));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, GENMASK(24, 23));
			break;
		case MT3612_TOPRGU_MMSYS_GAZE_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(3) | BIT(16));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(3));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(16));
			break;
		case MT3612_TOPRGU_CAM_SIDE0_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(6) | BIT(12));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(6));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(12));
			break;
		case MT3612_TOPRGU_CAM_SIDE1_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(8) | BIT(13));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(8));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(13));
			break;
		case MT3612_TOPRGU_IMG_SIDE0_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(7) | BIT(14) | BIT(19));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(7));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(14));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(19));
			break;
		case MT3612_TOPRGU_IMG_SIDE1_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(9) | BIT(15) | BIT(20));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(9));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(15));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(20));
			break;
		case MT3612_TOPRGU_ISP_GAZE0_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(10) | BIT(17));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(10));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(17));
			break;
		case MT3612_TOPRGU_ISP_GAZE1_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			BIT(11) | BIT(18));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	BIT(11));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1, BIT(18));
			break;
		case MT3612_TOPRGU_VPU_CONN_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(16));
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_3, INFRA_TOPAXI_PROTECTSTA1_3,
			GENMASK(5, 4) | GENMASK(26, 25));
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(25));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,
			GENMASK(5, 4) | GENMASK(26, 25));
			break;
		case MT3612_TOPRGU_VPU0_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(29));
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(12) | BIT(15) | BIT(19) | BIT(26));
			break;
		case MT3612_TOPRGU_VPU1_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(30));
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(13) | BIT(17) | BIT(20) | BIT(27));
			break;
		case MT3612_TOPRGU_VPU2_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(31));
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			BIT(14) | BIT(18) | BIT(21) | BIT(28));
			break;
		case MT3612_TOPRGU_MFG_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_2, INFRA_TOPAXI_PROTECTSTA1_2,
			GENMASK(25, 22));
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(24));
			mtk_infracfg_clear_bus_protection(infrareg,
			SMI_PROTECTEN, SMI_PROTECTSTA1,	GENMASK(28, 27));
			break;
		default:
			break;
		}
	} else if (regofs == MT3612_INFRACFG_OFS) {
		switch (id) {
		case MT3612_INFRA_SCP_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(15));
			break;
		case MT3612_INFRA_GCE4_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(7));
			break;
		case MT3612_INFRA_GCE5_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(8));
			break;
		case MT3612_INFRA_THERMAL_CTRL_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN_1, INFRA_TOPAXI_PROTECTSTA1_1,
			BIT(30));
			break;
		default:
			break;
		}
	} else if (regofs == MT3612_PERICFG_OFS) {
		switch (id) {
		case MT3612_PERI_IOMMU_SW_RST:
			mtk_infracfg_clear_bus_protection(infrareg,
			INFRA_TOPAXI_PROTECTEN, INFRA_TOPAXI_PROTECTSTA1,
			BIT(19));
			break;
		default:
			break;
		}
	} else {
		ret = 0;
	}
	return ret;
}
#endif

static int mtk_reset_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct mtk_reset *data = container_of(rcdev, struct mtk_reset, rcdev);

#ifdef RESET_BUS_PROTECT
	set_bus_protect(data->regofs, data->infra_regmap, id);
	if (data->regofs == MT3612_TOPRGU_OFS)
#else
	if (data->regofs == 0x18)
#endif
		return regmap_update_bits(data->regmap, data->regofs +
					 ((id / 32) << 2),
					 BIT(31) | BIT(27) | BIT(id % 32),
					 (~0) | (UNLOCK_KEY << KEY_SHIFT));
	else
		return regmap_update_bits(data->regmap, data->regofs +
					 ((id / 32) << 4), BIT(id % 32), ~0);
}

static int mtk_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	struct mtk_reset *data = container_of(rcdev, struct mtk_reset, rcdev);

#ifdef RESET_BUS_PROTECT
	clear_bus_protect(data->regofs, data->infra_regmap, id);
	if (data->regofs == MT3612_TOPRGU_OFS)
#else
	if (data->regofs == 0x18)
#endif
		return regmap_update_bits(data->regmap, data->regofs +
					 ((id / 32) << 2),
					 BIT(31) | BIT(27) | BIT(id % 32),
					 0 | (UNLOCK_KEY << KEY_SHIFT));
	else
		return regmap_write(data->regmap, data->regofs +
					 ((id / 32) << 4) + 0x04, BIT(id % 32));
}

static int mtk_reset(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	int ret;

	ret = mtk_reset_assert(rcdev, id);
	if (ret)
		return ret;

	return mtk_reset_deassert(rcdev, id);
}

static struct reset_control_ops mtk_reset_ops = {
	.assert = mtk_reset_assert,
	.deassert = mtk_reset_deassert,
	.reset = mtk_reset,
};

void mtk_register_reset_controller(struct device_node *np,
			unsigned int num_regs, int regofs)
{
	struct mtk_reset *data;
	int ret;
	struct regmap *regmap;
#ifdef RESET_BUS_PROTECT
	struct device_node *node = NULL;
	struct regmap *infra_regmap;
#endif

	regmap = syscon_node_to_regmap(np);
	if (IS_ERR(regmap)) {
		pr_err("Cannot find regmap for %s: %ld\n", np->full_name,
				PTR_ERR(regmap));
		return;
	}

#ifdef RESET_BUS_PROTECT
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt3612-infracfg");
	if (!node) {
		pr_err("find mediatek,mt3612-infracfg node failed!!!");
		return;
	}
	infra_regmap = syscon_node_to_regmap(node);
	if (IS_ERR(regmap)) {
		pr_err("Cannot find regmap for %s: %ld\n", np->full_name,
				PTR_ERR(regmap));
		return;
	}
#endif

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return;

	data->regmap = regmap;
#ifdef RESET_BUS_PROTECT
	data->infra_regmap = infra_regmap;
#endif
	data->regofs = regofs;
	data->rcdev.owner = THIS_MODULE;
	data->rcdev.nr_resets = num_regs * 32;
	data->rcdev.ops = &mtk_reset_ops;
	data->rcdev.of_node = np;

	ret = reset_controller_register(&data->rcdev);
	if (ret) {
		pr_err("could not register reset controller: %d\n", ret);
		kfree(data);
		return;
	}
}
