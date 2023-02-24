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
#include "../include/pmic.h"
#include <linux/mfd/mt3615/core.h>
#include <soc/mediatek/mtk_pwm_intf.h>
#include "../include/mt3615_pwm.h"

/* channel_rdy : PMIC PWM ready bit
 * channel_out : PMIC PWM out data
 * resolution : PMIC PWM resolution
 * r_val : PMIC PWM channel R value
 */

struct mtk_pwm_ops {
	void (*lock)(void);
	void (*unlock)(void);
	void (*dump_regs)(char *buf);
	int (*get_channel_value)(u8 channel);
};

static struct mtk_pwm_intf *pwm_intf;

const char *pmic_pwm_channel_name[] = {
	/* mt3615 */
	"LED1",
	"LED2",
	"LED3",
	"PWM1",
	"PWM2",
};

#define PMIC_PWM_CHANNEL_MAX	ARRAY_SIZE(pmic_pwm_channel_name)
struct regmap *mt3615_pwrap_regmap;

const char *pmic_get_pwm_name(u8 list)
{
	return pmic_pwm_channel_name[list];
}
EXPORT_SYMBOL(pmic_get_pwm_name);

static struct mtk_pwm_ops pmic_pwm_ops = {
	.get_channel_value = NULL,
	.dump_regs = NULL,
};

static struct mtk_pwm_intf pmic_pwm_intf = {
	.ops = &pmic_pwm_ops,
	.name = "mtk_pwm_intf",
	.channel_num = PMIC_PWM_CHANNEL_MAX,
	.channel_name = pmic_pwm_channel_name,
};

static int register_mtk_pwm_intf(struct mtk_pwm_intf *intf)
{
	pwm_intf = intf;

	return RET_SUCCESS;
}



static void mtk_pwm_init(struct regmap *map)
{
	mt3615_pwm_init(map);
	if (register_mtk_pwm_intf(&pmic_pwm_intf) < 0)
		pr_err("[%s] register MTK pwm Intf Fail\n", __func__);
}

/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_db_reg(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	char *operation = param[1];
	int32_t ret = 0;
	int32_t reg_val = 0;
	long addr = 0;
	long val  = 0;
	long mask = 0;
	long shift = 0;

	if (kstrtol(param[2], 16, &addr) != 0)
		return RET_FAIL;
	if (kstrtol(param[3], 16, &val) != 0)
		return RET_FAIL;
	if (kstrtol(param[4], 16, &mask) != 0)
		return RET_FAIL;
	if (kstrtol(param[5], 16, &shift) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) || (param[2] == NULL) ||
		(param[3] == NULL) || (param[4] == NULL) || (len < 6)) {
		pr_info("Error: parameter not enough\n");
	} else {
		if (strcmp(operation, "w") == 0) {
			ret = pwrap_pwm_write_reg(addr, val,
				mask, (uint32_t)shift);
			pr_info("write reg\n");
		} else if (strcmp(operation, "r") == 0) {
			ret = pwrap_pwm_read_reg(addr,
				&reg_val, mask,
				shift);
			pr_info("read reg=0x%lx,reg_val=0x%x\n", addr, reg_val);
		}
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return RET_SUCCESS;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_get_soc_ready(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;

	if ((param[0] == NULL) || (len == 0)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_get_soc_ready();
		pr_info("pwm get soc ready bit == %d\n", ret);
	}

	if (ret < 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return RET_SUCCESS;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_cfg_soc_ready(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;
	long flag = 0;

	if (kstrtol(param[1], 10, &flag) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (len == 0)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_cfg_soc_ready(flag);
		pr_info("pwm cfg soc ready bit ,ret %d\n", ret);
	}

	if (ret != 0)
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);

	return ret;
}

/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_ledx_cfg_autoload(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret;
	long start_pwm_no  = 0;
	long end_pwm_no  = 0;
	long flag = 0;
	long scenarios = 0;

	if (kstrtol(param[1], 10, &start_pwm_no) != 0)
		return RET_FAIL;
	if (kstrtol(param[2], 10, &end_pwm_no) != 0)
		return RET_FAIL;
	if (kstrtol(param[3], 10, &flag) != 0)
		return RET_FAIL;
	if (kstrtol(param[4], 10, &scenarios) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) || (param[2] == NULL) ||
		(param[3] == NULL) || (len < 4)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_ledx_cfg_autoload(start_pwm_no, end_pwm_no,
				flag, scenarios);
		pr_info("pwm_Idx1_cfg_autoload ,ret %d\n", ret);
	}

	if (ret != 0)
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);

	return  ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_Idx1_cfg_autoload(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;
	long flag = 0;
	long scenarios = 0;

	if (kstrtol(param[1], 10, &flag) != 0)
		return RET_FAIL;
	if (kstrtol(param[2], 10, &scenarios) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) || (len < 2)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_Idx1_cfg_autoload(flag, scenarios);
		pr_info("pwm_Idx1_cfg_autoload ,ret %d\n", ret);
	}

	if (ret != 0)
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);

	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_Idx2_cfg_autoload(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;
	long flag = 0;
	long scenarios = 0;

	if (kstrtol(param[1], 10, &flag) != 0)
		return RET_FAIL;
	if (kstrtol(param[2], 10, &scenarios) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) || (len < 2)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_Idx2_cfg_autoload(flag, scenarios);
		pr_info("pwm_Idx2_cfg_autoload ,ret %d\n", ret);
	}

	if (ret != 0)
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);

	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_ledx_apply_en(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;
	long start_pwm_no = 0;
	long end_pwm_no = 0;

	if (kstrtol(param[1], 10, &start_pwm_no) != 0)
		return RET_FAIL;
	if (kstrtol(param[2], 10, &end_pwm_no) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) || (len < 2)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_ledx_apply_en(start_pwm_no, end_pwm_no);
		pr_info("pwm_Idx1_apply_en ,ret %d\n", ret);
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}
	return ret;
}


/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_Idx1_apply_en(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	uint32_t ret = 0;

	if ((param[0] == NULL) || (len == 0)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_Idx1_apply_en();
		pr_info("pwm_Idx1_apply_en ,ret %d\n", ret);
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}
	return ret;
}
/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_Idx2_apply_en(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	uint32_t ret = 0;
	long scenarios = 0;

	if (kstrtol(param[1], 10, &scenarios) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (len < 1)) {
		pr_info("Error: parameter not enough\n");
	} else {
		ret = mt3615_pwm_Idx2_apply_en(scenarios);
		pr_info("pwm_Idx2_apply_en ,ret %d\n", ret);
	}

	if (ret != 0) {
		pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
		return ret;
	}

	return ret;
}


/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_ledx_test(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;
	long start_pwm_no  = 0;
	long end_pwm_no  = 0;
	long duty_cycle = 0;
	long mode = 0;
	long scenarios = 0;
	long pstart = 0;
	long pend = 0;
	long TH0 = 0;
	long TL0 = 0;
	long TR0 = 0;
	long TF0 = 0;
	long DH = 0;
	long TH = 0;
	long DL = 0;
	long TL = 0;
	long TR = 0;
	long TF = 0;

	if (kstrtol(param[1], 10, &start_pwm_no) != 0)
		return RET_FAIL;
	if (kstrtol(param[2], 10, &end_pwm_no) != 0)
		return RET_FAIL;
	if (kstrtol(param[3], 10, &duty_cycle) != 0)
		return RET_FAIL;
	if (kstrtol(param[4], 10, &mode) != 0)
		return RET_FAIL;
	if (kstrtol(param[5], 10, &scenarios) != 0)
		return RET_FAIL;
	if (kstrtol(param[6], 10, &pstart) != 0)
		return RET_FAIL;
	if (kstrtol(param[7], 10, &pend) != 0)
		return RET_FAIL;
	if (kstrtol(param[8], 10, &TH0) != 0)
		return RET_FAIL;
	if (kstrtol(param[9], 10, &TL0) != 0)
		return RET_FAIL;
	if (kstrtol(param[10], 10, &TR0) != 0)
		return RET_FAIL;
	if (kstrtol(param[11], 10, &TF0) != 0)
		return RET_FAIL;
	if (kstrtol(param[12], 10, &DH) != 0)
		return RET_FAIL;
	if (kstrtol(param[13], 10, &TH) != 0)
		return RET_FAIL;
	if (kstrtol(param[14], 10, &DL) != 0)
		return RET_FAIL;
	if (kstrtol(param[15], 10, &TL) != 0)
		return RET_FAIL;
	if (kstrtol(param[16], 10, &TR) != 0)
		return RET_FAIL;
	if (kstrtol(param[17], 10, &TF) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) || (param[2] == NULL)  ||
		(param[3] == NULL)  || (param[4] == NULL) || (len < 17) ||
		(param[5] == NULL) || (param[6] == NULL) ||
		(param[7] == NULL) || (param[8] == NULL) ||
		(param[9] == NULL) || (param[10] == NULL) ||
		(param[11] == NULL) || (param[12] == NULL) ||
		(param[13] == NULL) || (param[14] == NULL) ||
		(param[15] == NULL) || (param[16] == NULL) ||
		(start_pwm_no < 1) || (start_pwm_no > 3) ||
		(end_pwm_no > 3) || (start_pwm_no > end_pwm_no) ||
		(scenarios > 6) || (mode > 3)) {
		pr_info("Error: parameter invalid\n");
	} else {
		switch (mode) {
		case PWM_DISABLE:
			pr_info("pwm_ledx_test disable  ,ret %d\n", ret);
			break;
		case PWM_FIXED:
			ret = mt3615_pwm_ledx_fixed_mode_config(start_pwm_no,
			end_pwm_no, duty_cycle, scenarios);
			pr_info("pwm_ledx_test fixed_mode  ,ret %d\n", ret);
			break;
		case PWM_FLASH:
			ret = mt3615_pwm_ledx_flash_mode_config(start_pwm_no,
			end_pwm_no, scenarios, pstart, pend, TH0, TL0, TR0,
			TF0, DH, TH, DL, TL, TR, TF);
			pr_info("flash_mode enable ,ret %d\n", ret);
		break;
		}

		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}

		ret = mt3615_pwm_ledx_mode_sel(start_pwm_no, end_pwm_no, mode,
			scenarios);
		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}
		pr_info("ledx_mode_sel mode_sel  ,ret %d\n", ret);

		ret = mt3615_pwm_ledx_apply_en(start_pwm_no, end_pwm_no);
		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}
		pr_info("ledx_apply_en para set apply_en  ,ret %d\n", ret);
	}

	return ret;
}

/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_Idx1_test(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;
	uint32_t duty_step = 0;
	long pwm_freq_idex = 0;
	long duty_cycle = 0;
	long enable = 0;
	long scenarios = 0;

	if (kstrtol(param[1], 10, &pwm_freq_idex) != 0)
		return RET_FAIL;
	if (kstrtol(param[2],  10, &duty_cycle) != 0)
		return RET_FAIL;
	if (kstrtol(param[3],  10, &enable) != 0)
		return RET_FAIL;
	if (kstrtol(param[4], 10, &scenarios) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) || (param[2] == NULL) ||
		(param[3] == NULL) || (len < 4) || (duty_cycle > 100) ||
		(pwm_freq_idex > 3) || (scenarios > 7) || (enable > 2)) {
		pr_info("Error: parameter not enough\n");
	} else {
		switch (enable) {
		case PWM1_DISABLE:
			ret = mt3615_pwm_Idx1_disable(scenarios);
			pr_info("pwm_Idx1_test disable  ,ret %d\n", ret);
		break;
		case PWM1_ENABLE:
			ret = mt3615_pwm_Idx1_enable(scenarios);
			pr_info("pwm_Idx1_test enable  ,ret %d\n", ret);
			break;
		case PWM1_CONFIG:
			duty_step = (duty_cycle/2);
			ret = mt3615_pwm_Idx1_config(pwm_freq_idex, duty_step,
			scenarios);
			if (ret != 0) {
				pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
				return ret;
			}
			ret = mt3615_pwm_Idx1_enable(scenarios);
			if (ret != 0) {
				pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
				return ret;
			}
			pr_info("para set enable  ,ret %d\n", ret);
			break;
		}

		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}

		ret = mt3615_pwm_Idx1_apply_en();
		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}
		pr_info("pwm_Idx1_test apply_en =%d\n", ret);
	}

	return ret;
}

/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/
uint32_t pwm_Idx2_test(struct mtk_pwm_intf *pwm_intf_dev,
				uint32_t len, char **param)
{
	int32_t ret = 0;
	long pwm_freq_idex = 0;
	long enable = 0;
	long scenarios = 0;
	long TOFF = 0;
	long TON = 0;
	long TREPEAT = 0;

	if (kstrtol(param[1], 10, &pwm_freq_idex) != 0)
		return RET_FAIL;
	if (kstrtol(param[2], 10, &enable) != 0)
		return RET_FAIL;
	if (kstrtol(param[3], 10, &scenarios) != 0)
		return RET_FAIL;
	if (kstrtol(param[4], 10, &TOFF) != 0)
		return RET_FAIL;
	if (kstrtol(param[5], 10, &TON) != 0)
		return RET_FAIL;
	if (kstrtol(param[6], 10, &TREPEAT) != 0)
		return RET_FAIL;

	if ((param[0] == NULL) || (param[1] == NULL) | (len < 6) ||
		(pwm_freq_idex > 15) || (enable > 2)  || (scenarios > 6)) {
		pr_info("Error: parameter not enough\n");
	} else {
		switch (enable) {
		case PWM2_ENABLE:
			ret = mt3615_pwm_Idx2_enable(scenarios);
			pr_info("pwm_Idx2_test enable ,ret %d\n", ret);
			break;
		case PWM2_CONFIG:
			mt3615_pwm_Idx2_config(pwm_freq_idex, scenarios,
				TOFF, TON, TREPEAT);
			ret = mt3615_pwm_Idx2_enable(scenarios);
			pr_info("pwm_Idx1_test para set enable ,ret %d\n", ret);
			break;
		}

		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}

		ret = mt3615_pwm_Idx2_apply_en(scenarios);

		if (ret != 0) {
			pr_err("[PMIC/PWM]Func:%s fail\n", __func__);
			return ret;
		}

		pr_info("pwm_Idx2_test apply_en ,ret %d\n", ret);
	}

	return RET_SUCCESS;
}
struct cmd_tbl_t cmd_table_pwm[] = {
	{"db", &pwm_db_reg},
	{"gsr", &pwm_get_soc_ready},
	{"cfgsocr", &pwm_cfg_soc_ready},
	{"LEDxload", &pwm_ledx_cfg_autoload},
	{"pwm1load", &pwm_Idx1_cfg_autoload},
	{"pwm2load", &pwm_Idx2_cfg_autoload},
	{"LEDxapply", pwm_ledx_apply_en},
	{"pwm1apply", pwm_Idx1_apply_en},
	{"pwm2apply", pwm_Idx2_apply_en},
	{"pwmLEDx", &pwm_ledx_test},
	{"pwm1", &pwm_Idx1_test},
	{"pwm2", &pwm_Idx2_test},
};
uint32_t call_function_pwm(struct mtk_pwm_intf *pwm_intf_dev, char *buf)
{
	int i;
	int argc;
	char *argv[MAX_PARA_NUM];

	argc = 0;
	do {
		argv[argc] = strsep(&buf, " ");
		pr_debug("[%d] %s \r\n", argc, argv[argc]);
		argc++;
	} while (buf && argc < MAX_PARA_NUM);
	for (i = 0; i < ARRAY_SIZE(cmd_table_pwm); i++) {
		if ((!strcmp(cmd_table_pwm[i].name, argv[0])) &&
		(cmd_table_pwm[i].cb_func != NULL))
			return cmd_table_pwm[i].cb_func(pwm_intf_dev,
					argc, argv);
	}
	return RET_SUCCESS;
}
static ssize_t mtk_pwm_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mtk_pwm_intf *pwm_intf_dev;
	char *command;
	int ret;

	pwm_intf_dev = dev_get_drvdata(dev);
	command = kmalloc(count, GFP_KERNEL);
	memcpy(command, buf, count);
	if (command[count-1] == '\n')
		command[count-1] = '\0';

	ret = call_function_pwm(pwm_intf_dev, command);
	if (ret == RET_FAIL)
		pr_debug("TEST FAIL\n");
	else if (ret == RET_SUCCESS)
		pr_debug("TEST PASS\n");
	else
		pr_debug("TEST FAIL: INVALID RET %d\n", ret);
	kfree(command);
	return count;
}

static ssize_t mtk_pwm_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	pr_debug("Func:%s,line%d\n", __func__, __LINE__);
	return RET_FAIL;
}
static DEVICE_ATTR(pwm_manus, 0644, mtk_pwm_show, mtk_pwm_store);
static int create_sysfs_interface(struct device *dev)
{
	if (device_create_file(dev, &dev_attr_pwm_manus))
	return -EINVAL;

	return RET_SUCCESS;
}

static int remove_sysfs_interface(struct device *dev)
{
	device_remove_file(dev, &dev_attr_pwm_manus);
	return RET_SUCCESS;
}

static int mtk_pwm_intf_probe(struct platform_device *pdev)
{
	int ret;
	struct mt3615_chip *mt3615 = dev_get_drvdata(pdev->dev.parent);

	pr_info("%s\n", __func__);
	pr_debug("3615-pwm: filepath: %s\n", __FILE__);
	pr_debug("3615-pwm:Func:%s,line:%d\n", __func__, __LINE__);
	platform_set_drvdata(pdev, pwm_intf);
	ret = create_sysfs_interface(&pdev->dev);
	if (ret < 0) {
		pr_err("%s create sysfs fail\n", __func__);
		return -EINVAL;
	}
	pr_info("3615-pwm:Func:%s,line:%d, mt3615->regmap =%p\n",
		__func__, __LINE__, mt3615->regmap);
	mtk_pwm_init(mt3615->regmap);

	return RET_SUCCESS;
}

static int mtk_pwm_intf_remove(struct platform_device *pdev)
{
	int ret;

	ret = remove_sysfs_interface(&pdev->dev);
	return RET_SUCCESS;
}

static const struct of_device_id mt3615_of_match[] = {
	{ .compatible = "mediatek,mt3615-pwm", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mt3615_of_match);

static struct platform_driver mtk_pwm_intf_driver = {
	.driver = {
		.name = "mtk_pwm_intf",
		.of_match_table = of_match_ptr(mt3615_of_match),
	},
	.probe = mtk_pwm_intf_probe,
	.remove = mtk_pwm_intf_remove,
};
module_platform_driver(mtk_pwm_intf_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTK PMIC PWM Interface Driver");
MODULE_AUTHOR("Guodong Liu <guodong.liu@mediatek.com>");
MODULE_VERSION("1.0.0_M");
