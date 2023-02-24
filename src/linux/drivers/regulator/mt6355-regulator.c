/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Andrew-sh.Cheng, Mediatek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/mt6355/core.h>
#include <linux/mfd/mt6355/registers.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/mt6355-regulator.h>
/*
 * MT6392 regulators' information
 *
 * @desc: standard fields of regulator description.
 * @qi: Mask for query enable signal status of regulators
 * @vselon_reg: Register sections for hardware control mode of bucks
 * @vselctrl_reg: Register for controlling the buck control mode.
 * @vselctrl_mask: Mask for query buck's voltage control mode.
 */
struct mt6355_regulator_info {
	struct regulator_desc desc;
	unsigned int qi_reg;
	u32 qi;
	u32 vselon_reg;
	u32 vselctrl_reg;
	u32 vselctrl_mask;
	u32 modeset_reg;
	u32 modeset_mask;
};

/*
 * match:	corresponding regulator child node in mt6355.dtsi
 * vreg:	MT6355_ID_##vreg is the index in mt6355-regulator.h
 * min, max, step:	output voltage variables
 * volt_ranges:		voltage linear range
 * enreg:			enable register
 * vosel:			voltage register
 * vosel_mask:		voltage register mask
 * voselon/vosel_ctrl:		mt6355 with no HW control
 * _modeset_reg/_modeset_mask:	Default not support set_mode, add if needed
 */
#define MT6355_BUCK(match, vreg, min, max, step, volt_ranges, enreg,	\
		vosel, vosel_mask, voselon, vosel_ctrl,			\
		_modeset_reg, _modeset_mask)				\
[MT6355_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6355_volt_range_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6355_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = (max - min)/step + 1,			\
		.linear_ranges = volt_ranges,				\
		.n_linear_ranges = ARRAY_SIZE(volt_ranges),		\
		.vsel_reg = vosel,					\
		.vsel_mask = vosel_mask,				\
		.enable_reg = enreg,					\
		.enable_mask = BIT(0),					\
	},								\
	.qi = BIT(13),							\
	.vselon_reg = voselon,						\
	.vselctrl_reg = vosel_ctrl,					\
	.vselctrl_mask = BIT(1),					\
	.modeset_reg = _modeset_reg,					\
	.modeset_mask = _modeset_mask,					\
}

#define MT6355_LINEAR_LDO(match, vreg, min, max, step, volt_ranges, \
		enreg, vosel, vosel_mask, qireg)	\
[MT6355_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6355_volt_range_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6355_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = (max - min)/step + 1,			\
		.linear_ranges = volt_ranges,				\
		.n_linear_ranges = ARRAY_SIZE(volt_ranges),		\
		.vsel_reg = vosel,					\
		.vsel_mask = vosel_mask,				\
		.enable_reg = enreg,					\
		.enable_mask = BIT(0),					\
	},								\
	.qi_reg = qireg,				\
	.qi = BIT(0),							\
}

/*
 * match:	corresponding regulator child node in mt6355.dtsi
 * vreg:	MT6355_ID_##vreg is the index in mt6355-regulator.h
 */
#define MT6355_LDO(match, vreg, ldo_volt_table, enreg, enbit, vosel,	\
		vosel_mask, qireg, _modeset_reg, _modeset_mask)		\
[MT6355_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6355_volt_table_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6355_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = ARRAY_SIZE(ldo_volt_table),		\
		.volt_table = ldo_volt_table,				\
		.vsel_reg = vosel,					\
		.vsel_mask = vosel_mask,				\
		.enable_reg = enreg,					\
		.enable_mask = BIT(enbit),				\
	},								\
	.qi_reg = qireg,				\
	.qi = BIT(15),							\
	.modeset_reg = _modeset_reg,					\
	.modeset_mask = _modeset_mask,					\
}

#define MT6355_REG_FIXED(match, vreg, enreg, enbit, volt,		\
		qireg, _modeset_reg, _modeset_mask)		\
[MT6355_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt6355_volt_fixed_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT6355_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = 1,					\
		.enable_reg = enreg,					\
		.enable_mask = BIT(enbit),				\
		.min_uV = volt,						\
	},								\
	.qi_reg = qireg,				\
	.qi = BIT(15),							\
	.modeset_reg = _modeset_reg,					\
	.modeset_mask = _modeset_mask,					\
}

static const struct regulator_linear_range buck_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(518750, 0, 0x7f, 6250),
};

static const struct regulator_linear_range buck_volt_range2[] = {
	REGULATOR_LINEAR_RANGE(400000, 0, 0x7f, 6250),
};

static const struct regulator_linear_range buck_volt_range3[] = {
	REGULATOR_LINEAR_RANGE(406250, 0, 0x7f, 6250),
};

static const struct regulator_linear_range buck_volt_range4[] = {
	REGULATOR_LINEAR_RANGE(500000, 0, 0x3f, 50000),
};

static const struct regulator_linear_range buck_volt_range5[] = {
	REGULATOR_LINEAR_RANGE(1200000, 0, 0x7f, 12500),
};

static const struct regulator_linear_range linear_ldo_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(518750, 0, 0x7f, 6250),
};

static const u32 ldo_volt_table1[] = {
	1800000, 0, 0, 0,
	0, 0, 0, 2500000,
	0, 2700000, 2800000, 2900000,
	3000000, 0, 0, 0,
};

static const u32 ldo_volt_table2[] = {
	0, 0, 0, 900000,
	1000000, 1100000, 1200000, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

static const u32 ldo_volt_table3[] = {
	0, 0, 0, 900000,
	1000000, 1100000, 1200000, 1300000,
	0, 1500000, 0, 0,
	1800000, 0, 0, 0,
};

static const u32 ldo_volt_table4[] = {
	600000, 700000, 800000, 900000, 1000000, 1100000, 1200000, 1300000,
	1400000, 1500000, 1600000, 1700000, 1800000, 1900000, 2000000, 2100000,
};

static const u32 ldo_volt_table5[] = {
	0, 0, 0, 1700000,
	1800000, 0, 0, 0,
	2700000, 0, 0, 3000000,
	3100000, 0, 0, 0,
};

static const u32 ldo_volt_table6[] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 2800000, 0, 3000000,
	0, 0, 0, 0,
};

static const u32 ldo_volt_table7[] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 1700000,
	1800000, 0, 2000000, 2100000,
};

static const u32 ldo_volt_table8[] = {
	0, 0, 0, 0,
	1800000, 0, 0, 0,
	0, 0, 1900000, 3000000,
	0, 3300000, 0, 0,
};

static const u32 ldo_volt_table9[] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 2900000, 3000000,
	0, 3300000, 0, 0,
};

static const u32 ldo_volt_table10[] = {
	1500000, 1700000, 1800000, 2000000,
	2100000, 2200000, 2800000, 3300000,
};

static const u32 ldo_volt_table11[] = {
	1200000, 1300000, 1500000, 1800000,
	2000000, 2800000, 3000000, 3300000,
};

static int mt6392_get_status(struct regulator_dev *rdev)
{
	int ret;
	u32 regval;
	struct mt6355_regulator_info *info = rdev_get_drvdata(rdev);

	ret = regmap_read(rdev->regmap, info->qi_reg, &regval);
	if (ret != 0) {
		dev_err(&rdev->dev, "Failed to get enable reg: %d\n", ret);
		return ret;
	}

	return (regval & info->qi) ? REGULATOR_STATUS_ON : REGULATOR_STATUS_OFF;
}

static struct regulator_ops mt6355_volt_range_ops = {
	.list_voltage = regulator_list_voltage_linear_range,
	.map_voltage = regulator_map_voltage_linear_range,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt6392_get_status,
};

static struct regulator_ops mt6355_volt_table_ops = {
	.list_voltage = regulator_list_voltage_table,
	.map_voltage = regulator_map_voltage_iterate,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt6392_get_status,
};

static struct regulator_ops mt6355_volt_fixed_ops = {
	.list_voltage = regulator_list_voltage_linear,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt6392_get_status,
};

/* The array is indexed by id(MT6392_ID_XXX) */
static struct mt6355_regulator_info mt6355_regulators[] = {
	MT6355_BUCK("buck_vdram1", VDRAM1, 518750, 1312500, 6250
		, buck_volt_range1, MT6355_BUCK_VDRAM1_CON0
		, MT6355_BUCK_VDRAM1_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vdram2", VDRAM2, 400000, 1193750, 6250
		, buck_volt_range2, MT6355_BUCK_VDRAM2_CON0
		, MT6355_BUCK_VDRAM2_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vcore", VCORE, 406250, 1200000, 6250
		, buck_volt_range3, MT6355_BUCK_VCORE_CON0
		, MT6355_BUCK_VCORE_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vproc11", VPROC11, 406250, 1200000, 6250
		, buck_volt_range3, MT6355_BUCK_VPROC11_CON0
		, MT6355_BUCK_VPROC11_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vproc12", VPROC12, 406250, 1200000, 6250
		, buck_volt_range3, MT6355_BUCK_VPROC12_CON0
		, MT6355_BUCK_VPROC12_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vgpu", VGPU, 406250, 1200000, 6250
		, buck_volt_range3, MT6355_BUCK_VGPU_CON0
		, MT6355_BUCK_VGPU_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vmodem", VMODEM, 400000, 1193750, 6250
		, buck_volt_range2, MT6355_BUCK_VMODEM_CON0
		, MT6355_BUCK_VMODEM_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vpa", VPA, 500000, 3650000, 50000
		, buck_volt_range4, MT6355_BUCK_VPA_CON0
		, MT6355_BUCK_VPA_CON1, 0x3F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vs1", VS1, 1200000, 2787500, 12500
		, buck_volt_range5, MT6355_BUCK_VS1_CON0
		, MT6355_BUCK_VS1_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_BUCK("buck_vs2", VS2, 1200000, 2787500, 12500
		, buck_volt_range5, MT6355_BUCK_VS2_CON0
		, MT6355_BUCK_VS2_CON1, 0x7F, 0, 0, 0, 0),

	MT6355_REG_FIXED("ldo_vfe28", VFE28, MT6355_LDO_VFE28_CON0
		, 0, 2800000, MT6355_LDO_VFE28_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vtcxo24", VTCXO24, MT6355_LDO_VTCXO24_CON0
		, 0, 2300000, MT6355_LDO_VTCXO24_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vxo22", VXO22, MT6355_LDO_VXO22_CON0
		, 0, 2200000, MT6355_LDO_VXO22_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vxo18", VXO18, MT6355_LDO_VXO18_CON0
		, 0, 1810000, MT6355_LDO_VXO18_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vrf18_1", VRF18_1, MT6355_LDO_VRF18_1_CON0
		, 0, 1810000, MT6355_LDO_VRF18_1_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vrf18_2", VRF18_2, MT6355_LDO_VRF18_2_CON0
		, 0, 1810000, MT6355_LDO_VRF18_2_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vrf12", VRF12, MT6355_LDO_VRF12_CON0
		, 0, 1200000, MT6355_LDO_VRF12_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vcn33_bt", VCN33_BT, MT6355_LDO_VCN33_CON0_BT
		, 0, 3300000, MT6355_LDO_VCN33_CON1, 0, 0),
	MT6355_REG_FIXED("ldo_vcn33_wifi", VCN33_WIFI
		, MT6355_LDO_VCN33_CON0_WIFI
		, 0, 3300000, MT6355_LDO_VCN33_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vcn28", VCN28
		, MT6355_LDO_VCN28_CON0
		, 0, 2800000, MT6355_LDO_VCN28_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vcn18", VCN18, MT6355_LDO_VCN18_CON0
		, 0, 1800000, MT6355_LDO_VCN18_CON1, 0, 0),

	MT6355_LDO("ldo_vcama1", VCAMA1, ldo_volt_table1, MT6355_LDO_VCAMA1_CON0
		, 0, MT6355_ALDO_ANA_CON8, 0xF00, MT6355_LDO_VCAMA1_CON1, 0, 0),

	MT6355_LDO("ldo_vcama2", VCAMA2, ldo_volt_table1, MT6355_LDO_VCAMA2_CON0
		, 0, MT6355_ALDO_ANA_CON10, 0xF00
		, MT6355_LDO_VCAMA2_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vcamio", VCAMIO, MT6355_LDO_VCAMIO_CON0
		, 0, 1800000, MT6355_LDO_VCAMIO_CON1, 0, 0),

	MT6355_LDO("ldo_vcamd1", VCAMD1, ldo_volt_table2, MT6355_LDO_VCAMD1_CON0
		, 0, MT6355_SLDO14_ANA_CON2, 0xF00
		, MT6355_LDO_VCAMD1_CON1, 0, 0),

	MT6355_LDO("ldo_vcamd2", VCAMD2, ldo_volt_table3, MT6355_LDO_VCAMD2_CON0
		, 0, MT6355_SLDO14_ANA_CON4, 0xF00
		, MT6355_LDO_VCAMD2_CON1, 0, 0),

	MT6355_LDO("ldo_va10", VA10, ldo_volt_table4, MT6355_LDO_VA10_CON0
		, 0, MT6355_LDO_VA10_CON1, 0xF00, MT6355_LDO_VA10_CON2, 0, 0),

	MT6355_REG_FIXED("ldo_va12", VA12, MT6355_LDO_VA12_CON0
		, 0, 1200000, MT6355_LDO_VA12_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_va18", VA18, MT6355_LDO_VA18_CON0
		, 0, 1800000, MT6355_LDO_VA18_CON1, 0, 0),

	MT6355_LINEAR_LDO("ldo_vsram_proc", VSRAM_PROC, 518750, 1118750, 6250
		, linear_ldo_volt_range1, MT6355_LDO_VSRAM_PROC_CON0
		, MT6355_LDO_VSRAM_PROC_CON1, 0X7F, MT6355_LDO_VSRAM_PROC_DBG1),

	MT6355_LINEAR_LDO("ldo_vsram_gpu", VSRAM_GPU, 518750, 1118750, 6250
		, linear_ldo_volt_range1, MT6355_LDO_VSRAM_GPU_CON0
		, MT6355_LDO_VSRAM_GPU_CON1, 0X7F, MT6355_LDO_VSRAM_GPU_DBG1),

	MT6355_LINEAR_LDO("ldo_vsram_md", VSRAM_MD, 518750, 1118750, 6250
		, linear_ldo_volt_range1, MT6355_LDO_VSRAM_MD_CON0
		, MT6355_LDO_VSRAM_MD_CON1, 0X7F, MT6355_LDO_VSRAM_MD_DBG1),

	MT6355_LINEAR_LDO("ldo_vsram_core", VSRAM_CORE, 518750, 1118750, 6250
		, linear_ldo_volt_range1, MT6355_LDO_VSRAM_CORE_CON0
		, MT6355_LDO_VSRAM_CORE_CON1, 0X7F, MT6355_LDO_VSRAM_CORE_DBG1),

	MT6355_LDO("ldo_vsim1", VSIM1, ldo_volt_table5, MT6355_LDO_VSIM1_CON0
		, 0, MT6355_DLDO_ANA_CON2, 0xF00, MT6355_LDO_VSIM1_CON1, 0, 0),

	MT6355_LDO("ldo_vsim2", VSIM2, ldo_volt_table5, MT6355_LDO_VSIM2_CON0
		, 0, MT6355_DLDO_ANA_CON4, 0xF00, MT6355_LDO_VSIM2_CON1, 0, 0),

	MT6355_LDO("ldo_vldo28", VLDO28, ldo_volt_table6
		, MT6355_LDO_VLDO28_CON0_AF, 0, MT6355_DLDO_ANA_CON6, 0xF00
		, MT6355_LDO_VLDO28_CON1, 0, 0),

	MT6355_LDO("ldo_vmipi", VMIPI, ldo_volt_table7
		, MT6355_LDO_VMIPI_CON0, 0, MT6355_SLDO20_ANA_CON8, 0xF00
		, MT6355_LDO_VMIPI_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vio28", VIO28, MT6355_LDO_VIO28_CON0
		, 0, 2800000, MT6355_LDO_VIO28_CON1, 0, 0),

	MT6355_LDO("ldo_vmc", VMC, ldo_volt_table8, MT6355_LDO_VMC_CON0
		, 0, MT6355_DLDO_ANA_CON10, 0xF00, MT6355_LDO_VMC_CON1, 0, 0),

	MT6355_LDO("ldo_vmch", VMCH, ldo_volt_table9, MT6355_LDO_VMCH_CON0
		, 0, MT6355_DLDO_ANA_CON12, 0xF00, MT6355_LDO_VMCH_CON1, 0, 0),

	MT6355_LDO("ldo_vemc", VEMC, ldo_volt_table9, MT6355_LDO_VEMC_CON0
		, 0, MT6355_DLDO_ANA_CON14, 0xF00, MT6355_LDO_VEMC_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vufs18", VUFS18, MT6355_LDO_VUFS18_CON0
		, 0, 1800000, MT6355_LDO_VUFS18_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vusb33", VUSB33, MT6355_LDO_VUSB33_CON0_0
		, 0, 3000000, MT6355_LDO_VUSB33_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vbif28", VBIF28, MT6355_LDO_VBIF28_CON0
		, 0, 2800000, MT6355_LDO_VBIF28_CON1, 0, 0),

	MT6355_REG_FIXED("ldo_vio18", VIO18, MT6355_LDO_VIO18_CON0
		, 0, 1800000, MT6355_LDO_VIO18_CON1, 0, 0),

	MT6355_LDO("ldo_vgp", VGP, ldo_volt_table10, MT6355_LDO_VGP_CON0
		, 0, MT6355_DLDO_ANA_CON16, 0xF00, MT6355_LDO_VGP_CON1, 0, 0),

	MT6355_LDO("ldo_vgp2", VGP2, ldo_volt_table11, MT6355_LDO_VGP2_CON0
		, 0, MT6355_ALDO_ANA_CON18, 0xF00, MT6355_LDO_VGP2_CON1, 0, 0),
};

static int mt6355_set_buck_vosel_reg(struct platform_device *pdev)
{
	struct mt6355_chip *mt6355 = dev_get_drvdata(pdev->dev.parent);
	int i;
	u32 regval;

	for (i = 0; i < MT6355_MAX_REGULATOR; i++) {
		if (!mt6355_regulators[i].vselctrl_reg)
			continue;

		if (regmap_read(mt6355->regmap,
				mt6355_regulators[i].vselctrl_reg,
				&regval) < 0) {
			dev_err(&pdev->dev, "Failed to read buck ctrl\n");
			return -EIO;
		}

		if (regval & mt6355_regulators[i].vselctrl_mask) {
			mt6355_regulators[i].desc.vsel_reg =
			mt6355_regulators[i].vselon_reg;
		}
	}

	return 0;
}

struct regmap *temp_map;

unsigned int pmic_config_interface_mssv(unsigned int reg_num, unsigned int val,
				   unsigned int mask, unsigned int shift)
{
	unsigned int return_value = 0;
	unsigned int pmic_reg = 0;

	if (val > mask) {
		pr_err("[Power/PMIC][pmic_config_interface_mssv] ");
		pr_err("Invalid data, Reg[%x]: mask = 0x%x, val = 0x%x\n",
			reg_num, mask, val);
		return -1;
	}

	return_value = regmap_read(temp_map, reg_num, &pmic_reg);
	if (return_value != 0) {
		pr_err("[Power/PMIC][pmic_config_interface_mssv] ");
		pr_err("Reg[%x]= pmic_wrap read data fail\n", reg_num);
		return return_value;
	}

	pmic_reg &= ~(mask << shift);
	pmic_reg |= (val << shift);

	return_value = regmap_write(temp_map, reg_num, pmic_reg);
	if (return_value != 0) {
		pr_err("[Power/PMIC][pmic_config_interface_mssv] ");
		pr_err("Reg[%x]= pmic_wrap read data fail\n", reg_num);
		return return_value;
	}

	return return_value;
}

unsigned int pmic_read_interface_mssv(unsigned int reg_num, unsigned int *val,
				 unsigned int mask, unsigned int shift)
{
	unsigned int return_value = 0;
	unsigned int pmic_reg = 0;

	return_value = regmap_read(temp_map, reg_num, &pmic_reg);
	if (return_value != 0) {
		pr_err("[Power/PMIC][pmic_read_interface_mssv] ");
		pr_err("Reg[%x]= pmic_wrap read data fail\n", reg_num);
		return return_value;
	}

	pmic_reg &= (mask << shift);
	*val = (pmic_reg >> shift);

	return return_value;
}

u32 g_reg_value;
static ssize_t show_pmic_access(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	pr_notice("[show_pmic_access] 0x%x\n", g_reg_value);

	return sprintf(buf, "%04X\n", g_reg_value);
}

static ssize_t store_pmic_access(struct device *dev
	, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	char temp_buf[32];
	char *pvalue;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	strncpy(temp_buf, buf, sizeof(temp_buf));
	temp_buf[sizeof(temp_buf) - 1] = 0;
	pvalue = temp_buf;

	if (size != 0) {
		if (size > 5) {
			ret = kstrtouint(strsep(&pvalue, " ")
					 , 16, &reg_address);
			if (ret)
				return ret;
			ret = kstrtouint(pvalue, 16, &reg_value);
			if (ret)
				return ret;
			pr_notice("[store_pmic_access] ");
			pr_notice("write PMU reg 0x%x with value 0x%x !\n",
				  reg_address, reg_value);
			ret = pmic_config_interface_mssv(reg_address
				, reg_value, 0xFFFF, 0x0);
		} else {
			ret = kstrtouint(pvalue, 16, &reg_address);
			if (ret)
				return ret;
			ret = pmic_read_interface_mssv(reg_address
				, &g_reg_value, 0xFFFF, 0x0);
			pr_notice("[store_pmic_access] ");
			pr_notice("read PMU reg 0x%x with value 0x%x !\n",
				  reg_address, g_reg_value);
			pr_notice("[store_pmic_access] ");
			pr_notice(
			"Please use \"cat pmic_access\" to get value\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(pmic_access, 0664, show_pmic_access, store_pmic_access);

static int mt6355_regulator_probe(struct platform_device *pdev)
{
	struct mt6355_chip *mt6355 = dev_get_drvdata(pdev->dev.parent);
	struct regulator_config config = {};
	struct regulator_dev *rdev;
	struct regulation_constraints *c;
	int i;
	u32 reg_value;

	/* Query buck controller to select activated voltage register part */
	if (mt6355_set_buck_vosel_reg(pdev))
		return -EIO;

	/* Read PMIC chip revision to update constraints and voltage table */
	if (regmap_read(mt6355->regmap, MT6355_HWCID, &reg_value) < 0) {
		dev_err(&pdev->dev, "Failed to read Chip ID\n");
		return -EIO;
	}
	dev_info(&pdev->dev, "Chip ID = 0x%x\n", reg_value);

	for (i = 0; i < MT6355_MAX_REGULATOR; i++) {
		config.dev = &pdev->dev;
		config.driver_data = &mt6355_regulators[i];
		config.regmap = mt6355->regmap;
		rdev = devm_regulator_register(&pdev->dev,
				&mt6355_regulators[i].desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register %s\n",
				mt6355_regulators[i].desc.name);
			return PTR_ERR(rdev);
		}

		/* Constrain board-specific capabilities according to what
		 * this driver and the chip itself can actually do.
		 */
		c = rdev->constraints;
		c->valid_modes_mask |= REGULATOR_MODE_NORMAL|
			REGULATOR_MODE_STANDBY | REGULATOR_MODE_FAST;
		c->valid_ops_mask |= REGULATOR_CHANGE_MODE;
	}


	temp_map = mt6355->regmap;

	if (device_create_file(&(pdev->dev), &dev_attr_pmic_access) < 0)
		dev_err(&(pdev->dev), "Create file pmic_access att failed\n");
	return 0;
}

static const struct platform_device_id mt6355_platform_ids[] = {
	{"mt6355-regulator", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, mt6355_platform_ids);

static const struct of_device_id mt6355_of_match[] = {
	{ .compatible = "mediatek,mt6355-regulator", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mt6355_of_match);

static struct platform_driver mt6355_regulator_driver = {
	.driver = {
		.name = "mt6355-regulator",
		.of_match_table = of_match_ptr(mt6355_of_match),
	},
	.probe = mt6355_regulator_probe,
	.id_table = mt6355_platform_ids,
};

module_platform_driver(mt6355_regulator_driver);

MODULE_AUTHOR("Andrew-sh.Cheng, Mediatek");
MODULE_DESCRIPTION("Regulator Driver for MediaTek MT6355 PMIC");
MODULE_LICENSE("GPL v2");

