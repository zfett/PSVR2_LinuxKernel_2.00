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
#include <linux/mfd/mt3615/core.h>
#include <linux/mfd/mt3615/registers.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/mt3615-regulator.h>
/*
 * MT3615 regulators' information
 *
 * @desc: standard fields of regulator description.
 * @qi: Mask for query enable signal status of regulators
 * @qi_reg: Register for query enable signal status of regulators
 */
struct mt3615_regulator_info {
	struct regulator_desc desc;
	unsigned int qi_reg;
	u32 qi;
};

/*
 * match:	corresponding regulator child node in mt3615.dtsi
 * vreg:	MT3615_ID_##vreg is the index in mt3615-regulator.h
 * min, max, step:	output voltage variables
 * volt_ranges:		voltage linear range
 * enreg:		enable register
 * vosel:		voltage register
 * vosel_mask:		voltage register mask
 * qi_reg:		Register for query enable signal status
 */
#define MT3615_BUCK(match, vreg, min, max, step, volt_ranges, enreg,	\
		vosel, vosel_mask, qireg)				\
[MT3615_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt3615_volt_range_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT3615_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = (max - min)/step + 1,			\
		.linear_ranges = volt_ranges,				\
		.n_linear_ranges = ARRAY_SIZE(volt_ranges),		\
		.vsel_reg = vosel,					\
		.vsel_mask = vosel_mask,				\
		.enable_reg = enreg,					\
		.enable_mask = BIT(0),					\
		.enable_val = 1,					\
		.disable_val = 0,					\
	},								\
	.qi_reg = qireg,						\
	.qi = BIT(0),							\
}
/*
 * match:	corresponding regulator child node in mt3615.dtsi
 * vreg:	MT3615_ID_##vreg is the index in mt3615-regulator.h
 * min, max, step:	output voltage variables
 * volt_ranges:		voltage linear range
 * enreg:		enable register
 * vosel:		voltage register
 * vosel_mask:		voltage register mask
 * qi_reg:		Register for query enable signal status
 */
#define MT3615_LINEAR_LDO(match, vreg, min, max, step, volt_ranges,	\
		enreg, vosel, vosel_mask, qireg)			\
[MT3615_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt3615_volt_range_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT3615_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = (max - min)/step + 1,			\
		.linear_ranges = volt_ranges,				\
		.n_linear_ranges = ARRAY_SIZE(volt_ranges),		\
		.vsel_reg = vosel,					\
		.vsel_mask = vosel_mask,				\
		.enable_reg = enreg,					\
		.enable_mask = BIT(0),					\
	},								\
	.qi_reg = qireg,						\
	.qi = BIT(0),							\
}
/*
 * match:	corresponding regulator child node in mt3615.dtsi
 * vreg:	MT3615_ID_##vreg is the index in mt3615-regulator.h
 * ldo_volt_table:	ldo voltage table
 * enreg:		enable register
 * enbit:		enable register mask
 * vosel:		voltage register
 * vosel_mask:		voltage register mask
 * qi_reg:		Register for query enable signal status
 */
#define MT3615_LDO(match, vreg, ldo_volt_table, enreg, enbit, vosel,	\
		vosel_mask, qireg)					\
[MT3615_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt3615_volt_table_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT3615_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = ARRAY_SIZE(ldo_volt_table),		\
		.volt_table = ldo_volt_table,				\
		.vsel_reg = vosel,					\
		.vsel_mask = vosel_mask,				\
		.enable_reg = enreg,					\
		.enable_mask = BIT(enbit),				\
	},								\
	.qi_reg = qireg,						\
	.qi = BIT(0),							\
}
/*
 * match:	corresponding regulator child node in mt3615.dtsi
 * vreg:	MT3615_ID_##vreg is the index in mt3615-regulator.h
 * enreg:		enable register
 * enbit:		enable register mask
 * volt:		voltage
 * qi_reg:		Register for query enable signal status
 */
#define MT3615_REG_FIXED(match, vreg, enreg, enbit, volt, qireg)	\
[MT3615_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt3615_volt_fixed_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT3615_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = 1,					\
		.enable_reg = enreg,					\
		.enable_mask = BIT(enbit),				\
		.min_uV = volt,						\
	},								\
	.qi_reg = qireg,						\
	.qi = BIT(0),							\
}
static const struct regulator_linear_range buck_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xB4, 6250),
};

static const struct regulator_linear_range buck_volt_range2[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x60, 0x9F, 6250),
};

static const struct regulator_linear_range buck_volt_range3[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0x96, 6250),
};

static const struct regulator_linear_range buck_volt_range4[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0x7C, 6250),
};

static const struct regulator_linear_range buck_volt_range5[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0x8D, 6250),
};

static const struct regulator_linear_range buck_volt_range6[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0x8D, 6250),
};

static const struct regulator_linear_range buck_volt_range7[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xB0, 12500),
};

static const struct regulator_linear_range buck_volt_range8[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0x78, 12500),
};

static const struct regulator_linear_range buck_volt_range9[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xB0, 6250),
};

static const struct regulator_linear_range buck_volt_range10[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0x84, 25000),
};

static const struct regulator_linear_range buck_volt_range11[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xA8, 12500),
};

static const struct regulator_linear_range buck_volt_range12[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xBD, 6250),
};

static const struct regulator_linear_range buck_volt_range13[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 0xBD, 6250),
};


static const struct regulator_linear_range linear_ldo_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(500000, 0, 0x60, 6250),
};

static const u32 ldo_volt_table1[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1600000, 1700000, 1800000, 1900000, 0, 0,
};

static const u32 ldo_volt_table2[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2900000, 3000000, 3100000, 3200000, 0, 0,
};

static const u32 ldo_volt_table3[] = {
	0, 0, 0, 0, 0, 1100000, 1200000, 1300000,
	0, 0, 0, 0, 0, 0, 0, 0,
};

static const u32 ldo_volt_table4[] = {
	0, 0, 800000, 900000, 1000000, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};


static int mt3615_get_status(struct regulator_dev *rdev)
{
	int ret;
	u32 regval;
	struct mt3615_regulator_info *info = rdev_get_drvdata(rdev);

	ret = regmap_read(rdev->regmap, info->qi_reg, &regval);
	if (ret != 0) {
		dev_err(&rdev->dev, "Failed to get enable reg: %d\n", ret);
		return ret;
	}

	return (regval & info->qi) ? REGULATOR_STATUS_ON : REGULATOR_STATUS_OFF;
}

static struct regulator_ops mt3615_volt_range_ops = {
	.list_voltage = regulator_list_voltage_linear_range,
	.map_voltage = regulator_map_voltage_linear_range,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt3615_get_status,
};

static struct regulator_ops mt3615_volt_table_ops = {
	.list_voltage = regulator_list_voltage_table,
	.map_voltage = regulator_map_voltage_iterate,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt3615_get_status,
};

static struct regulator_ops mt3615_volt_fixed_ops = {
	.list_voltage = regulator_list_voltage_linear,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = mt3615_get_status,
};

/* The array is indexed by id(MT3615_ID_XXX) */
static struct mt3615_regulator_info mt3615_regulators[] = {
	MT3615_BUCK("buck_vproc", VPROC, 0, 1125000, 6250
		, buck_volt_range1, MT3615_BUCK_VPROC_CON0
		, MT3615_BUCK_VPROC_ELR0, 0xFF, MT3615_BUCK_VPROC_DBG1),
	MT3615_BUCK("buck_vgpu11", VGPU11, 0, 993750, 6250
		, buck_volt_range2, MT3615_BUCK_VGPU11_CON0
		, MT3615_BUCK_VGPU11_ELR0, 0xFF, MT3615_BUCK_VGPU11_DBG1),
	MT3615_BUCK("buck_vgpu12", VGPU12, 0, 993750, 6250
		, buck_volt_range2, MT3615_BUCK_VGPU12_CON0
		, MT3615_BUCK_VGPU12_ELR0, 0xFF, MT3615_BUCK_VGPU12_DBG1),
	MT3615_BUCK("buck_vpu", VPU, 0, 937500, 6250
		, buck_volt_range3, MT3615_BUCK_VPU_CON0
		, MT3615_BUCK_VPU_ELR0, 0xFF, MT3615_BUCK_VPU_DBG1),
	MT3615_BUCK("buck_vcore1", VCORE1, 0, 775000, 6250
		, buck_volt_range4, MT3615_BUCK_VCORE1_CON0
		, MT3615_BUCK_VCORE1_ELR0, 0xFF, MT3615_BUCK_VCORE1_DBG1),
	MT3615_BUCK("buck_vcore2", VCORE2, 0, 881250, 6250
		, buck_volt_range5, MT3615_BUCK_VCORE2_CON0
		, MT3615_BUCK_VCORE2_ELR0, 0xFF, MT3615_BUCK_VCORE2_DBG1),
	MT3615_BUCK("buck_vcore4", VCORE4, 0, 881250, 6250
		, buck_volt_range6, MT3615_BUCK_VCORE4_CON0
		, MT3615_BUCK_VCORE4_ELR0, 0xFF, MT3615_BUCK_VCORE4_DBG1),
	MT3615_BUCK("buck_vs1", VS1, 0, 2200000, 12500
		, buck_volt_range7, MT3615_BUCK_VS1_CON0
		, MT3615_BUCK_VS1_ELR0, 0xFF, MT3615_BUCK_VS1_DBG1),
	MT3615_BUCK("buck_vs2", VS2, 0, 1500000, 12500
		, buck_volt_range8, MT3615_BUCK_VS2_CON0
		, MT3615_BUCK_VS2_ELR0, 0xFF, MT3615_BUCK_VS2_DBG1),
	MT3615_BUCK("buck_vs3", VS3, 0, 1100000, 6250
		, buck_volt_range9, MT3615_BUCK_VS3_CON0
		, MT3615_BUCK_VS3_ELR0, 0xFF, MT3615_BUCK_VS3_DBG1),
	MT3615_BUCK("buck_vdram1", VDRAM1, 0, 1181250, 6250
		, buck_volt_range12, MT3615_BUCK_VDRAM1_CON0
		, MT3615_BUCK_VDRAM1_ELR0, 0xFF, MT3615_BUCK_VDRAM1_DBG1),
	MT3615_BUCK("buck_vdram2", VDRAM2, 0, 1181250, 6250
		, buck_volt_range13, MT3615_BUCK_VDRAM2_CON0
		, MT3615_BUCK_VDRAM2_ELR0, 0xFF, MT3615_BUCK_VDRAM2_DBG1),
	MT3615_BUCK("buck_vio18", VIO18, 0, 2100000, 12500
		, buck_volt_range11, MT3615_BUCK_VIO18_CON0
		, MT3615_BUCK_VIO18_ELR0, 0xFF, MT3615_BUCK_VIO18_DBG1),
	MT3615_BUCK("buck_vio31", VIO31, 0, 3300000, 25000
		, buck_volt_range10, MT3615_BUCK_VIO31_CON0
		, MT3615_BUCK_VIO31_ELR0, 0x7F, MT3615_BUCK_VIO31_DBG1),
	MT3615_REG_FIXED("ldo_vaux18", VAUX18, MT3615_LDO_VAUX18_CON0
		, 0, 1840000, MT3615_LDO_VAUX18_MON),
	MT3615_REG_FIXED("ldo_vxo", VXO, MT3615_LDO_VXO_CON0
		, 0, 2240000, MT3615_LDO_VXO_MON),
	/* MT3615_REG_FIXED("ldo_vefuse", VEFUSE, MT3615_LDO_VEFUSE_CON0
	 *	, 0, 1890000, MT3615_LDO_VEFUSE_MON),
	 */
	MT3615_LDO("ldo_vefuse", VEFUSE, ldo_volt_table1, MT3615_LDO_VEFUSE_CON0
		, 0, MT3615_VEFUSE_ANA_CON0, 0xF00, MT3615_LDO_VEFUSE_MON),
	MT3615_LDO("ldo_vm18", VM18, ldo_volt_table1, MT3615_LDO_VM18_CON0
		, 0, MT3615_VM18_ANA_CON0, 0xF00, MT3615_LDO_VM18_MON),
	/*
	 * MT3615_REG_FIXED("ldo_vusb", VUSB, MT3615_LDO_VUSB_CON0
	 *	, 0, 3100000, MT3615_LDO_VUSB_MON),
	 * MT3615_REG_FIXED("ldo_va18", VA18, MT3615_LDO_VA18_CON0
	 *	, 0, 1820000, MT3615_LDO_VA18_MON),
	 * MT3615_REG_FIXED("ldo_va12", VA12, MT3615_LDO_VA12_CON0
	 *	, 0, 1210000, MT3615_LDO_VA12_MON),
	 * MT3615_REG_FIXED("ldo_va09", VA09, MT3615_LDO_VA09_CON0
	 *	, 0, 910000, MT3615_LDO_VA09_MON),
	 */
	MT3615_LDO("ldo_vusb", VUSB, ldo_volt_table2, MT3615_LDO_VUSB_CON0
		, 0, MT3615_VUSB_ANA_CON0, 0xF00, MT3615_LDO_VUSB_MON),
	MT3615_LDO("ldo_va18", VA18, ldo_volt_table1, MT3615_LDO_VA18_CON0
		, 0, MT3615_VA18_ANA_CON0, 0xF00, MT3615_LDO_VA18_MON),
	MT3615_LDO("ldo_va12", VA12, ldo_volt_table3, MT3615_LDO_VA12_CON0
		, 0, MT3615_VA12_ANA_CON0, 0xF00, MT3615_LDO_VA12_MON),
	MT3615_LDO("ldo_va09", VA09, ldo_volt_table4, MT3615_LDO_VA09_CON0
		, 0, MT3615_VA09_ANA_CON0, 0xF00, MT3615_LDO_VA09_MON),
	MT3615_LINEAR_LDO("ldo_vsram_proc", VSRAM_PROC, 500000, 1100000, 6250
		, linear_ldo_volt_range1, MT3615_LDO_VSRAM_PROC_CON0
		, MT3615_LDO_VSRAM_PROC_ELR, 0X7F, MT3615_LDO_VSRAM_PROC_MON),
	MT3615_LINEAR_LDO("ldo_vsram_corex", VSRAM_COREX, 500000, 1100000, 6250
		, linear_ldo_volt_range1, MT3615_LDO_VSRAM_COREX_CON0
		, MT3615_LDO_VSRAM_COREX_ELR, 0X7F, MT3615_LDO_VSRAM_COREX_MON),
	MT3615_LINEAR_LDO("ldo_vsram_gpu", VSRAM_GPU, 500000, 1100000, 6250
		, linear_ldo_volt_range1, MT3615_LDO_VSRAM_GPU_CON0
		, MT3615_LDO_VSRAM_GPU_ELR, 0X7F, MT3615_LDO_VSRAM_GPU_MON),
	MT3615_LINEAR_LDO("ldo_vsram_vpu", VSRAM_VPU, 500000, 1100000, 6250
		, linear_ldo_volt_range1, MT3615_LDO_VSRAM_VPU_CON0
		, MT3615_LDO_VSRAM_VPU_ELR, 0X7F, MT3615_LDO_VSRAM_VPU_MON),
	MT3615_REG_FIXED("ldo_vpio31", VPIO31, MT3615_LDO_VPIO31_CON0
		, 0, 3100000, MT3615_LDO_VPIO31_MON),
	MT3615_REG_FIXED("ldo_vbbck", VBBCK, MT3615_LDO_VBBCK_CON0
		, 0, 1200000, MT3615_LDO_VBBCK_MON),
	MT3615_REG_FIXED("ldo_vpio18", VPIO18,	MT3615_LDO_VPIO18_CON0
		, 0, 1800000, MT3615_LDO_VPIO18_MON),
};

static int mt3615_regulator_probe(struct platform_device *pdev)
{
	struct mt3615_chip *mt3615 = dev_get_drvdata(pdev->dev.parent);
	struct regulator_config config = {};
	struct regulator_dev *rdev;
	struct regulation_constraints *c;
	int i;
	u32 reg_value;

	/* Read PMIC chip revision to update constraints and voltage table */
	if (regmap_read(mt3615->regmap, MT3615_HWCID, &reg_value) < 0) {
		dev_err(&pdev->dev, "Failed to read Chip ID\n");
		return -EIO;
	}
	dev_info(&pdev->dev, "PMIC Chip ID = 0x%x\n", reg_value);

	for (i = 0; i < MT3615_MAX_REGULATOR; i++) {
		config.dev = &pdev->dev;
		config.driver_data = &mt3615_regulators[i];
		config.regmap = mt3615->regmap;
		rdev = devm_regulator_register(&pdev->dev,
				&mt3615_regulators[i].desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register %s\n",
				mt3615_regulators[i].desc.name);
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

	return 0;
}

static const struct platform_device_id mt3615_platform_ids[] = {
	{"mt3615-regulator", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, mt3615_platform_ids);

static const struct of_device_id mt3615_of_match[] = {
	{ .compatible = "mediatek,mt3615-regulator", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mt3615_of_match);

static struct platform_driver mt3615_regulator_driver = {
	.driver = {
		.name = "mt3615-regulator",
		.of_match_table = of_match_ptr(mt3615_of_match),
	},
	.probe = mt3615_regulator_probe,
	.id_table = mt3615_platform_ids,
};

module_platform_driver(mt3615_regulator_driver);

MODULE_AUTHOR("zm chen, Mediatek");
MODULE_DESCRIPTION("Regulator Driver for MediaTek MT3615 PMIC");
MODULE_LICENSE("GPL v2");

