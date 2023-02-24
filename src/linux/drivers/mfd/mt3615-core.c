/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: zm.chen, MediaTek
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


#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt3615/core.h>
#include <linux/mfd/mt3615/registers.h>
#include <linux/mfd/mt3615/mt3615_api.h>
#include <linux/regulator/consumer.h>


/* Current wtih HWID, 0x151A 0x152A*/
#define MT3615_CID_CODE		0x1500
/* pmic thread polling time 2000 = 2s */
#define pmic_polling_time	2000

/* reserve pmic thread polling mode,1: use polling mode, default off*/
#define pmic_timer	0

#define MAX_CMD_NUM 20

struct mt3615_chip *mt_g;

/* pmic mt3615 sub cell regulator/pmw/pinctrl */
static const struct mfd_cell mt3615_devs[] = {
	{
		.name = "mt3615-regulator",
		.of_compatible = "mediatek,mt3615-regulator"
	}, {
		.name = "mtk_pwm_intf",
		.of_compatible = "mediatek,mt3615-pwm"

	}, {
		.name = "mediatek-mt3615-pinctrl",
		.of_compatible = "mediatek,mt3615-pinctrl"
	},
};

static irqreturn_t mt3615_key_irq_handler(int irq, void *data)
{
	struct mt3615_chip *mt3615 = data;

	/*mt3615_thread_wakeup();*/
	dev_info(mt3615->dev, "mt3615_key_irq_handler\n");

	return IRQ_HANDLED;
}
static irqreturn_t mt3615_cable_irq_handler(int irq, void *data)
{
	struct mt3615_chip *mt3615 = data;

	/*mt3615_thread_wakeup();*/
	dev_info(mt3615->dev, "mt3615_cable_irq_handler\n");

	return IRQ_HANDLED;
}
/* mt3615.dtsi define interrupt irq num and name mt3615-key mt3615-cable */
static int mt3615_eint_init(struct mt3615_chip *mt3615)
{
	int ret;

	mutex_init(&mt3615->irqlock);

	mt3615->key_irq = irq_of_parse_and_map(mt3615->dev->of_node, 0);
	if (mt3615->key_irq < 0) {
		dev_err(mt3615->dev, "failed to of mt3615 key_irq\n");
		return -1;
	}
	mt3615->cable_irq = irq_of_parse_and_map(mt3615->dev->of_node, 1);
	if (mt3615->cable_irq < 0) {
		dev_err(mt3615->dev, "failed to of mt3615 cable_irq\n");
		return -1;
	}
	ret = devm_request_threaded_irq(mt3615->dev, mt3615->key_irq, NULL,
		mt3615_key_irq_handler, IRQF_ONESHOT, "mt3615-key", mt3615);
	ret = devm_request_threaded_irq(mt3615->dev, mt3615->cable_irq, NULL,
		mt3615_cable_irq_handler, IRQF_ONESHOT, "mt3615-cable", mt3615);

	if (ret) {
		dev_err(mt3615->dev, "failed to register irq=%d,%d; err: %d\n",
			mt3615->key_irq, mt3615->cable_irq, ret);
		return ret;
	}

	return 0;
}

void mt3615_pmic_poweroff(void)
{
	pmic_config_reg(MT3615_PMIC_STANDBY_IN_ADDR, 0x1,
		MT3615_PMIC_STANDBY_IN_MASK, MT3615_PMIC_STANDBY_IN_SHIFT);

}
EXPORT_SYMBOL(mt3615_pmic_poweroff);

int mt3615_set_soc_ready(unsigned int en)
{
	unsigned int val = 0;
	int ret;

	if (en != 0 && en != 1)
		return en;
	ret = pmic_config_reg(MT3615_SOC_READY_STATUS_ADDR, en,
		MT3615_SOC_READY_STATUS_MASK, MT3615_SOC_READY_STATUS_SHIFT);
	if (ret < 0)
		return ret;
	ret = pmic_read_reg(MT3615_SOC_READY_STATUS_ADDR, &val,
				MT3615_SOC_READY_STATUS_MASK,
				MT3615_SOC_READY_STATUS_SHIFT);

	pr_notice("mt3615_set_soc_ready bit, val = %u\n", val);
	return ret;
}
EXPORT_SYMBOL(mt3615_set_soc_ready);

#if defined(CONFIG_MFD_MT3615_UT)
void mt3615_pmic_reboot(void)
{
	pmic_config_reg(MT3615_PMIC_REBOOT_ADDR, 0x1,
		MT3615_PMIC_REBOOT_MASK, MT3615_PMIC_REBOOT_SHIFT);
}
EXPORT_SYMBOL(mt3615_pmic_reboot);

unsigned int mt3615_get_soc_ready(void)
{
	unsigned int val = 0;

	pmic_read_reg(MT3615_SOC_READY_STATUS_ADDR, &val,
				MT3615_SOC_READY_STATUS_MASK,
				MT3615_SOC_READY_STATUS_SHIFT);
	pr_notice("mt3615_get_soc_ready bit, val = %d\n", val);
	return val;
}
EXPORT_SYMBOL(mt3615_get_soc_ready);

/*
 * MT3615 shutdown pin disable/enable
 *
 * @idx: 0: shutdown1; 1:shutdown2; 2:shutdown3.
 * @en: 0:disable; 1:enable
 */
int mt3615_enable_shutdown_pad(enum SHUTDOWN_IDX idx
					, unsigned int en)
{
	int ret;

	if (en != 0 && en != 1)
		return en;
	switch (idx) {
	case SHUTDOWN1:
		ret = pmic_config_reg(MT3615_PMIC_SHUTDOWN1_EN_ADDR, en,
					MT3615_PMIC_SHUTDOWN1_EN_MASK,
					MT3615_PMIC_SHUTDOWN1_EN_SHIFT);
		break;
	case SHUTDOWN2:
		ret = pmic_config_reg(MT3615_PMIC_SHUTDOWN2_EN_ADDR, en,
					MT3615_PMIC_SHUTDOWN2_EN_MASK,
					MT3615_PMIC_SHUTDOWN2_EN_SHIFT);
		break;
	case SHUTDOWN3:
		ret = pmic_config_reg(MT3615_PMIC_SHUTDOWN3_EN_ADDR, en,
					MT3615_PMIC_SHUTDOWN3_EN_MASK,
					MT3615_PMIC_SHUTDOWN3_EN_SHIFT);
		break;
	default:
		ret = -1;
		break;
		}
	return ret;
}
EXPORT_SYMBOL(mt3615_enable_shutdown_pad);
/*
 * MT3615 timwerout pin disable/enable
 *
 * @idx: 0: timerout1; 1: timerout2.
 * @en: 0:disable; 1:enable; timerout2 en need to check timerout1
 * @set setting timerout1 timers 10~320s step: 10s
 */
int mt3615_enable_timerout(unsigned int idx, unsigned int en,
					unsigned int set)
{
	unsigned int val = 0;
	int ret;

	if (en != 0 && en != 1)
		return en;
	switch (idx) {
	case 0:
		if ((set < 10) || (set > 320))
			return -1;
		set = (set - 10) / 10;
		pmic_config_reg(MT3615_TIMER_OUT1_SET_ADDR, set,
					MT3615_TIMER_OUT1_SET_MASK,
					MT3615_TIMER_OUT1_SET_SHIFT);
		ret = pmic_config_reg(MT3615_TIMER_OUT1_EN_ADDR, en,
					MT3615_TIMER_OUT1_EN_MASK,
					MT3615_TIMER_OUT1_EN_SHIFT);

		break;
	case 1:
		ret = pmic_read_reg(MT3615_TIMER_OUT1_EN_ADDR, &val,
					MT3615_TIMER_OUT1_EN_MASK,
					MT3615_TIMER_OUT1_EN_SHIFT);
		if ((en == 1 && val != 1) || (ret < 0))
			return -1;
		ret = pmic_config_reg(MT3615_TIMER_OUT2_EN_ADDR, en,
					MT3615_TIMER_OUT2_EN_MASK,
					MT3615_TIMER_OUT2_EN_SHIFT);
		break;
	default:
		ret = -1;
		break;
		}
	return ret;
}
EXPORT_SYMBOL(mt3615_enable_timerout);
#endif

int pmic_config_reg(unsigned int reg_num, unsigned int val,
				   unsigned int mask, unsigned int shift)
{
	int ret = 0;
	unsigned int pmic_reg = 0;

	if (val > mask) {
		pr_err("[Power/PMIC][pmic_config_reg] ");
		pr_err("Invalid data, Reg[%x]: mask = 0x%x, val = 0x%x\n",
			reg_num, mask, val);
		return -1;
	}

	ret = regmap_read(mt_g->regmap, reg_num, &pmic_reg);
	if (ret != 0) {
		pr_err("[Power/PMIC][pmic_config_reg] ");
		pr_err("Reg[%x]= pmic_wrap read data fail\n", reg_num);
		return ret;
	}

	pmic_reg &= ~(mask << shift);
	pmic_reg |= (val << shift);

	ret = regmap_write(mt_g->regmap, reg_num, pmic_reg);
	if (ret != 0) {
		pr_err("[Power/PMIC][pmic_config_reg] ");
		pr_err("Reg[%x]= pmic_wrap write data fail\n", reg_num);
		return ret;
	}

	return ret;
}

int pmic_read_reg(unsigned int reg_num, unsigned int *val,
				 unsigned int mask, unsigned int shift)
{
	int ret = 0;
	unsigned int pmic_reg = 0;

	ret = regmap_read(mt_g->regmap, reg_num, &pmic_reg);
	if (ret != 0) {
		pr_err("[Power/PMIC][pmic_read_reg] ");
		pr_err("Reg[%x]= pmic_wrap read data fail\n", reg_num);
		return ret;
	}

	pmic_reg &= (mask << shift);
	*val = (pmic_reg >> shift);

	return ret;
}

#if defined(CONFIG_MFD_MT3615_REG)
static ssize_t show_pmic_reg(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	int val = 0;
	int ret = 0;

	ret = pmic_read_reg(0x0, &val, 0xffff, 0);
	dev_err(mt_g->dev, "pmic_reg read add= 0x%x,val= 0x%x\n", 0, val);
	ret = pmic_read_reg(0x8, &val, 0xffff, 0);
	dev_err(mt_g->dev, "pmic_reg read add= 0x%x,val= 0x%x\n", 8, val);

	ret = sprintf(buf, "%04X\n", val);
	if (ret >= 0)
		return ret;
	else
		return -1;
}

static ssize_t store_pmic_reg(struct device *dev
	, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	int val = 0;
	int address, value, mask, shift;

	if (!strncmp(buf, "r", 1)) {
		if (sscanf(buf + 2, "%x %x %x", &address, &mask, &shift) < 4) {
			dev_err(mt_g->dev, "input %x,%x,%x\n",
				address, mask, shift);
			ret = pmic_read_reg(address, &value, mask, shift);
			dev_err(mt_g->dev, "pmic_reg read = 0x%x\n", value);
		}
	} else if (!strncmp(buf, "w", 1)) {
		if (sscanf(buf + 2, "%x %x %x %x", &address, &value, &mask,
			&shift) < 5) {
			dev_err(mt_g->dev, "input %x,%x,%x,%x\n",
				address, value, mask, shift);
			ret = pmic_read_reg(address, &val, mask, shift);
			dev_err(mt_g->dev, "pmic_reg org read = 0x%x\n", val);
			ret = pmic_config_reg(address, value, mask, shift);
			ret = pmic_read_reg(address, &val, mask, shift);
			dev_err(mt_g->dev, "pmic_reg w af read = 0x%x\n", val);
		}
	} else {
		dev_err(mt_g->dev, "r/w command error");
	}
	return size;
}
#endif

static ssize_t show_regu_en(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	dev_err(dev, "pmic set regulator enable/disable:\n");
	dev_err(dev, "echo vxxx 1/0 > regu_en\n");

	return 0;
}

static ssize_t store_regu_en(struct device *dev
	, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	struct regulator *pmic_regu;
	struct mt3615_chip *re_dev;
	int argc;
	char *argv[MAX_CMD_NUM], *cmd, *idx;
	long int en;

	cmd = kmalloc(size, GFP_KERNEL);
	memcpy(cmd, buf, size);
	if (cmd[size-1] == '\n')
		cmd[size-1] = '\0';
	argc = 0;
	do {
		argv[argc] = strsep(&cmd, " ");
		pr_err("[%d] %s \r\n", argc, argv[argc]);
		argc++;
	} while (cmd && argc < MAX_CMD_NUM);

	re_dev = dev_get_drvdata(dev);
	idx = argv[0];
	if (strncmp(idx, "vefuse", 6)) {
		kfree(cmd);
		dev_err(dev, "Only ldo vefuse not always on\n");
		return -1;
	}
	if (kstrtol(argv[1], 10, &en) != 0) {
		kfree(cmd);
		return -1;
	}

	dev_err(dev, "input %s,%ld\n", idx, en);
	pmic_regu = regulator_get(re_dev->dev, idx);
	if (IS_ERR(pmic_regu)) {
		kfree(cmd);
		dev_err(re_dev->dev, "regulator_get %s failed\n", idx);
		return PTR_ERR(pmic_regu);
	}
	if (en > 0)
		ret = regulator_enable(pmic_regu);
	else
		ret = regulator_disable(pmic_regu);
	if (ret < 0) {
		dev_err(re_dev->dev, "regulator %s enable/disable =%ld fail\n",
			idx, en);
	}
	regulator_put(pmic_regu);
	kfree(cmd);
	return size;
}

static ssize_t show_regu_volt(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	dev_err(dev, "pmic set regulator volt:\n");
	dev_err(dev, "echo vxxx 900000 1000000 > regu_volt\n");

	return 0;
}

static ssize_t store_regu_volt(struct device *dev
	, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	struct regulator *pmic_regu;
	struct mt3615_chip *re_dev;
	char *argv[MAX_CMD_NUM];
	char *cmd, *idx;
	long int vmin, vmax;
	int cur_vol;
	int argc;

	cmd = kmalloc(size, GFP_KERNEL);
	memcpy(cmd, buf, size);
	if (cmd[size-1] == '\n')
		cmd[size-1] = '\0';
	argc = 0;
	do {
		argv[argc] = strsep(&cmd, " ");
		pr_err("[%d] %s \r\n", argc, argv[argc]);
		argc++;
	} while (cmd && argc < MAX_CMD_NUM);

	re_dev = dev_get_drvdata(dev);
	idx = argv[0];
	if (kstrtol(argv[1], 10, &vmin) != 0) {
		kfree(cmd);
		return -1;
	}
	if (kstrtol(argv[2], 10, &vmax) != 0) {
		kfree(cmd);
		return -1;
	}

	dev_err(re_dev->dev, "input %s,%ld,%ld\n", idx, vmin, vmax);
	pmic_regu = regulator_get(dev, idx);
	if (IS_ERR(pmic_regu)) {
		kfree(cmd);
		dev_err(re_dev->dev, "regulator_get %s failed\n", idx);
		return PTR_ERR(pmic_regu);
	}
	cur_vol = regulator_get_voltage(pmic_regu);
	dev_err(re_dev->dev, "regulator get vol %s cur_vol=%d\n", idx, cur_vol);
	ret = regulator_set_voltage(pmic_regu, vmin, vmax);
	if (ret < 0)
		dev_err(re_dev->dev,
			"regulator set vol %s vmin=%ld vmax=%ld failed\n",
			idx, vmin, vmax);
	cur_vol = regulator_get_voltage(pmic_regu);
	dev_err(re_dev->dev, "regulator %s set after vol=%d\n", idx, cur_vol);
	regulator_put(pmic_regu);
	kfree(cmd);
	return size;
}

#if defined(CONFIG_MFD_MT3615_UT)
static ssize_t show_pmic_ut(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	pr_err("Func:%s,line%d\n", __func__, __LINE__);

	return 0;
}

static ssize_t store_pmic_ut(struct device *dev
	, struct device_attribute *attr, const char *buf, size_t size)
{
	struct mt3615_chip *ut_dev;
	char *cmd;
	int ret;

	ut_dev = dev_get_drvdata(dev);
	cmd = kmalloc(size, GFP_KERNEL);
	memcpy(cmd, buf, size);
	if (cmd[size-1] == '\n')
		cmd[size-1] = '\0';

	ret = call_pmic_ut(ut_dev, cmd);
	if (ret == 0)
		pr_err("TEST PASS\n");
	else if (ret == 1)
		pr_err("The first param of input command is not right !!!\n");
	else
		pr_err("TEST FAIL: RET = %d\n", ret);
	kfree(cmd);

	return size;
}
#endif

#if defined(CONFIG_MFD_MT3615_REG)
static DEVICE_ATTR(pmic_reg, 0664, show_pmic_reg, store_pmic_reg);
#endif

static DEVICE_ATTR(regu_en, 0664, show_regu_en, store_regu_en);
static DEVICE_ATTR(regu_volt, 0664, show_regu_volt, store_regu_volt);

#if defined(CONFIG_MFD_MT3615_UT)
static DEVICE_ATTR(pmic_ut, 0664, show_pmic_ut, store_pmic_ut);
#endif

static int mt3615_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int id;
	struct mt3615_chip *pmic;
	/*struct task_struct *th;*/

	pmic = devm_kzalloc(&pdev->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	pmic->dev = &pdev->dev;

	/*
	 * mt3615 MFD is child device of soc pmic wrapper.
	 * Regmap is set from its parent.
	 */
	pmic->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!pmic->regmap)
		return -ENODEV;

	platform_set_drvdata(pdev, pmic);

	/*check pmic HWID.*/
	ret = regmap_read(pmic->regmap, MT3615_HWCID, &id);
	if (ret) {
		dev_err(pmic->dev, "Failed to read chip id: %d\n", ret);
		return ret;
	}
	mt_g = pmic;
	mt_g->pmic_thread_flag = 0;
	init_waitqueue_head(&mt_g->wait_queue);

	/*set soc ready status to pmic.*/
	mt3615_set_soc_ready(1);

	switch (id & 0xff00) {
	case MT3615_CID_CODE:
		ret = mfd_add_devices(&pdev->dev, -1, mt3615_devs,
					   ARRAY_SIZE(mt3615_devs), NULL,
					   0, NULL);
		/*PMIC status polling*/
		/*mt3615_pmic_timer_init(pmic);
		*th = kthread_run(mt3615_kthread, (void *)pmic,"mt3615_thread");
		*if (IS_ERR(th)) {
		*	dev_warn(pmic->dev,
		*		"Unable to start pmic thread\n");
		*	return PTR_ERR(th);
		*}
		*/
		pmic->key_irq = platform_get_irq(pdev, 0);
		if (pmic->key_irq <= 0)
			return pmic->key_irq;
		pmic->cable_irq = platform_get_irq(pdev, 0);
		if (pmic->cable_irq <= 0)
			return pmic->cable_irq;

		ret = mt3615_eint_init(pmic);
		if (ret) {
			devm_free_irq(pmic->dev, pmic->key_irq, pmic);
			devm_free_irq(pmic->dev, pmic->cable_irq, pmic);
			dev_err(pmic->dev, "failed to eint init: %d\n", ret);
			return ret;
		}
		break;

	default:
		dev_err(&pdev->dev, "unsupported chip: %d\n", id);
		ret = -ENODEV;
		break;
	}

#if defined(CONFIG_MFD_MT3615_REG)
	if (device_create_file(&(pdev->dev), &dev_attr_pmic_reg))
		device_remove_file(&(pdev->dev), &dev_attr_pmic_reg);
#endif

	if (device_create_file(&(pdev->dev), &dev_attr_regu_en))
		device_remove_file(&(pdev->dev), &dev_attr_regu_en);
	if (device_create_file(&(pdev->dev), &dev_attr_regu_volt))
		device_remove_file(&(pdev->dev), &dev_attr_regu_volt);

#if defined(CONFIG_MFD_MT3615_UT)
	if (device_create_file(&(pdev->dev), &dev_attr_pmic_ut))
		device_remove_file(&(pdev->dev), &dev_attr_pmic_ut);
#endif

	if (ret) {
		dev_err(&pdev->dev, "failed to add child devices: %d\n", ret);
		devm_kfree(pmic->dev, pmic);
	}
	dev_info(pmic->dev, "mt3615_probe done\n");

	return ret;
}

static const struct of_device_id mt3615_of_match[] = {
	{ .compatible = "mediatek,mt3615" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mt3615_of_match);

static const struct platform_device_id mt3615_id[] = {
	{ "mt3615", 0 },
	{ },
};
MODULE_DEVICE_TABLE(platform, mt3615_id);

static struct platform_driver mt3615_driver = {
	.probe = mt3615_probe,
	.driver = {
		.name = "mt3615",
		.of_match_table = of_match_ptr(mt3615_of_match),
	},
	.id_table = mt3615_id,
};

module_platform_driver(mt3615_driver);

MODULE_AUTHOR("zm.chen@mediatek.com, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT3615 PMIC");
MODULE_LICENSE("GPL");
