/*
 * Copyright (C) 2017 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/**
 * @file upmu_pwm.c
 *  pmic pwm linux driver test code, this just a test code ,
 *  so no do doxygen completely.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt3615/core.h>
#include <linux/mfd/mt3615/registers.h>
#include <linux/mfd/mt3615/mt3615_api.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/mt3615-regulator.h>

#include "../regulator/internal.h"

#define MAX_CMD_NUM 20

/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
int pmic_reboot(struct mt3615_chip *ut_dev, int len, char **param)
{
	if (len < 1) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 1) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	mt3615_pmic_reboot();
	dev_dbg(ut_dev->dev, "[PMIC]Func:%s\n", __func__);
	return 0;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
int pmic_poweroff(struct mt3615_chip *ut_dev, int len, char **param)
{
	if (len < 1) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 1) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	mt3615_pmic_poweroff();
	dev_dbg(ut_dev->dev, "[PMIC]Func:%s\n", __func__);
	return 0;
}

/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
int pmic_get_soc_ready(struct mt3615_chip *ut_dev,
				int len, char **param)
{
	unsigned int val = 0;
	int ret = 0;

	if (len > 1) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	val = mt3615_get_soc_ready();
	dev_dbg(ut_dev->dev, "pmic get soc ready bit =%d\n", val);
	if (val != 0 && val != 1)
		ret = -1;

	dev_dbg(ut_dev->dev, "[PMIC]Func:%s\n", __func__);
	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
int pmic_set_soc_ready(struct mt3615_chip *ut_dev,
				int len, char **param)
{
	int ret = 0;
	long int en = 0;

	if (len < 2) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 2) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	if (kstrtol(param[1], 10, &en) != 0)
		return -1;
	ret = mt3615_set_soc_ready(en);
	dev_dbg(ut_dev->dev, "pmic set soc ready bit =%ld\n", en);

	if (ret != 0)
		dev_err(ut_dev->dev, "[PMIC]Func:%s fail\n", __func__);
	dev_dbg(ut_dev->dev, "[PMIC]Func:%s PASS\n", __func__);
	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
int pmic_shutdown_pin_enable(struct mt3615_chip *ut_dev,
					int len, char **param)
{
	int ret = 0;
	long int idx = 1;
	long int en = 0;

	if (len < 3) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 3) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	if (kstrtol(param[1], 10, &idx) != 0)
		return -1;
	if (kstrtol(param[2], 10, &en) != 0)
		return -1;

	ret = mt3615_enable_shutdown_pad(idx - 1, en);
	dev_dbg(ut_dev->dev, "pmic set shutdown%ld, en=%ld\n", idx, en);

	if (ret != 0)
		dev_err(ut_dev->dev, "[PMIC]Func:%s fail\n", __func__);
	dev_dbg(ut_dev->dev, "[PMIC]Func:%s PASS\n", __func__);
	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
int pmic_timerout_pin_set(struct mt3615_chip *ut_dev,
					int len, char **param)
{
	int ret = 0;
	long int idx = 1;
	long int en = 0;
	long int t1s = 0;

	if (len < 4) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 4) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	if (kstrtol(param[1], 10, &idx) != 0)
		return -1;
	if (kstrtol(param[2], 10, &en) != 0)
		return -1;
	if (kstrtol(param[3], 10, &t1s) != 0)
		return -1;

	ret = mt3615_enable_timerout(idx - 1, en, t1s);
	dev_dbg(ut_dev->dev, "pmic set timer%ld, en =%ld, t1s=%ld\n",
					idx, en, t1s);

	if (ret != 0)
		dev_err(ut_dev->dev, "[PMIC]Func:%s fail\n", __func__);
	dev_dbg(ut_dev->dev, "[PMIC]Func:%s PASS\n", __func__);
	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
int pmic_set_regulator_enable(struct mt3615_chip *ut_dev,
					int len, char **param)
{
	int ret = 0;
	char *idx = param[1];
	long int en = 0;
	struct regulator *pmic_regu;

	if (len < 3) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 3) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	if (strcmp(idx, "vefuse")) {
		dev_err(ut_dev->dev, "Only ldo vefuse not always on\n");
		dev_err(ut_dev->dev, "[PMIC]Func:%s fail\n", __func__);
		return -1;
	}
	if (kstrtol(param[2], 10, &en) != 0)
		return -1;

	pmic_regu = regulator_get(ut_dev->dev, idx);
	if (IS_ERR(pmic_regu)) {
		dev_err(ut_dev->dev, "regulator_get %s failed\n", idx);
		return PTR_ERR(pmic_regu);
	}
	if (en > 0)
		ret = regulator_enable(pmic_regu);
	else
		ret = regulator_disable(pmic_regu);
	if (ret < 0) {
		dev_err(ut_dev->dev,
			"regulator enable/disable %s en=%ld fail\n", idx, en);
	}
	regulator_put(pmic_regu);
	return ret;
}

/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
static int mt3615_set_voltage_ldo(struct regulator *regulator,
					int min_uV, int max_uV)
{
	unsigned int selector;
	int ret = 0;
	int best_val = 0;
	int vocal_val = 0;
	struct regulator_dev *rdev = regulator->rdev;

	if (min_uV < rdev->constraints->min_uV)
		min_uV = rdev->constraints->min_uV;
	if (min_uV > rdev->constraints->max_uV)
		min_uV = rdev->constraints->max_uV;

	if (rdev->desc->volt_table && rdev->desc->ops->set_voltage_sel) {
		max_uV = rdev->constraints->max_uV;
		ret = regulator_map_voltage_iterate(rdev, min_uV, max_uV);
		best_val = rdev->desc->ops->list_voltage(rdev, ret);
		if (best_val > min_uV && ret >= 1)
			ret -= 1;
		best_val = rdev->desc->ops->list_voltage(rdev, ret);

		if (best_val <= max_uV) {
			selector = ret;
			ret = rdev->desc->ops->set_voltage_sel(rdev, selector);
			vocal_val = (min_uV - best_val) / 10000;
			if (vocal_val >= 0 && vocal_val < 10) {
				ret = regmap_update_bits(rdev->regmap,
						rdev->desc->vsel_reg,
						0xF, vocal_val);
			} else {
				ret = regmap_update_bits(rdev->regmap,
						rdev->desc->vsel_reg, 0xF, 0);
			}
		} else {
			ret = -EINVAL;
		}
	}
	return ret;
}

int pmic_set_regulator_voltage(struct mt3615_chip *ut_dev,
					int len, char **param)
{
	int ret = 0;
	char *idx = param[1];
	long int vmin = 0;
	long int vmax = 0;
	unsigned int cur_vol;
	struct regulator *pmic_regu;
	struct regulator_dev *rdev;

	if (len < 4) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 4) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}
	if (kstrtol(param[2], 10, &vmin) != 0)
		return -1;
	if (kstrtol(param[3], 10, &vmax) != 0)
		return -1;

	pmic_regu = regulator_get(ut_dev->dev, idx);
	if (IS_ERR(pmic_regu)) {
		dev_err(ut_dev->dev, "regulator_get %s failed\n", idx);
		return PTR_ERR(pmic_regu);
	}
	rdev = pmic_regu->rdev;

	cur_vol = regulator_get_voltage(pmic_regu);
	dev_err(ut_dev->dev, "regulator get vol %s cur_vol=%d\n",
		idx, cur_vol);

	if (rdev->desc->id < MT3615_ID_VEFUSE
		|| rdev->desc->id > MT3615_ID_VA09) {
		ret = regulator_set_voltage(pmic_regu, vmin, vmax);
	} else {
		ret = mt3615_set_voltage_ldo(pmic_regu, vmin, vmax);
	}
	if (ret < 0) {
		dev_err(ut_dev->dev,
			"regulator setvol %s,vmin=%ld,vmax=%ld fail\n",
			idx, vmin, vmax);
	}
	regulator_put(pmic_regu);
	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
static int mt3615_get_voltage_ldo(struct regulator *regulator)
{
	int ret, sel, vol, vocal;
	struct regulator_dev *rdev = regulator->rdev;

	if (rdev->desc->ops->get_voltage_sel) {
		sel = rdev->desc->ops->get_voltage_sel(rdev);
		if (sel < 0)
			return sel;
		vol = rdev->desc->ops->list_voltage(rdev, sel);
	} else {
		return -EINVAL;
	}

	ret = regmap_read(rdev->regmap, rdev->desc->vsel_reg, &vocal);
	if (ret != 0)
		return ret;
	vocal = (vocal & 0xF) * 10000;
	vol += vocal;

	return vol;
}

int pmic_get_regulator_voltage(struct mt3615_chip *ut_dev,
					int len, char **param)
{
	char *idx = param[1];
	unsigned int cur_vol;
	struct regulator *pmic_regu;
	struct regulator_dev *rdev;

	if (len < 2) {
		dev_err(ut_dev->dev, "Error: parameter not enough\n");
		return -1;
	}
	if (len > 2) {
		dev_err(ut_dev->dev, "Error: too many parameters\n");
		return -1;
	}

	pmic_regu = regulator_get(ut_dev->dev, idx);
	rdev = pmic_regu->rdev;
	if (IS_ERR(pmic_regu)) {
		dev_err(ut_dev->dev, "regulator_get %s failed\n", idx);
		return PTR_ERR(pmic_regu);
	}

	if (rdev->desc->id < MT3615_ID_VEFUSE
		|| rdev->desc->id > MT3615_ID_VA09)
		cur_vol = regulator_get_voltage(pmic_regu);
	else
		cur_vol = mt3615_get_voltage_ldo(pmic_regu);

	dev_err(ut_dev->dev, "%s cur_vol=%dmV\n",
			idx, cur_vol / 1000);

	return 0;
}

struct cmd_t cmd_pmic[] = {
	{"rb", &pmic_reboot},
	{"off", &pmic_poweroff},
	{"ssr", &pmic_set_soc_ready},
	{"gsr", &pmic_get_soc_ready},
	{"ssb", &pmic_shutdown_pin_enable},
	{"stb", &pmic_timerout_pin_set},
	{"sre", &pmic_set_regulator_enable},
	{"srv", &pmic_set_regulator_voltage},
	{"grv", &pmic_get_regulator_voltage},
};
int call_pmic_ut(struct mt3615_chip *ut_dev, char *buf)
{
	int i;
	int argc;
	char *argv[MAX_CMD_NUM];

	argc = 0;
	do {
		argv[argc] = strsep(&buf, " ");
		pr_err("[%d] %s \r\n", argc, argv[argc]);
		argc++;
	} while (buf && argc < MAX_CMD_NUM);
	for (i = 0; i < ARRAY_SIZE(cmd_pmic); i++) {
		if ((!strcmp(cmd_pmic[i].name, argv[0])) &&
		(cmd_pmic[i].cb_func != NULL))
			return cmd_pmic[i].cb_func(ut_dev, argc, argv);
	}
	return 1;
}
EXPORT_SYMBOL(call_pmic_ut);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTK PMIC UT Interface Driver");
MODULE_AUTHOR("zm.chen@mediatek.com");
MODULE_VERSION("1.0.0_M");
