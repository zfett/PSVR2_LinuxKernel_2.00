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


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt3615/core.h>
#include <linux/mfd/mt3615/registers.h>
#include <linux/mfd/mt3615/mt3615_api.h>
#include <linux/regulator/consumer.h>

/* Current wtih HWID, 0x151A 0x152A*/
#define MT3615_CID_CODE		0x1500

static int _pmic_config_reg(unsigned int reg_num, unsigned int val,
					unsigned int mask, unsigned int shift);
static int _pmic_read_reg(unsigned int reg_num, unsigned int *val,
					unsigned int mask, unsigned int shift);
static struct mt3615_chip *mt_g;


/* pmic mt3615 sub cell regulator/pmw */
static const struct mfd_cell mt3615_devs[] = {
	{
		.name = "mt3615-regulator",
		.of_compatible = "mediatek,mt3615-regulator"
	}, {
		.name = "mtk_pwm_intf",
		.of_compatible = "mediatek,mt3615-pwm"
	},
};

int mt3615_set_soc_ready(unsigned int en)
{
	unsigned int val = 0;
	int ret;

	if (en != 0 && en != 1)
		return en;
	ret = _pmic_config_reg(MT3615_SOC_READY_STATUS_ADDR, en,
		MT3615_SOC_READY_STATUS_MASK, MT3615_SOC_READY_STATUS_SHIFT);
	if (ret < 0)
		return ret;
	ret = _pmic_read_reg(MT3615_SOC_READY_STATUS_ADDR, &val,
				MT3615_SOC_READY_STATUS_MASK,
				MT3615_SOC_READY_STATUS_SHIFT);

	return ret;
}

static int _pmic_config_reg(unsigned int reg_num, unsigned int val,
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

static int _pmic_read_reg(unsigned int reg_num, unsigned int *val,
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

#define PMIC_STATUS_SIZE	10
static ssize_t show_pmic_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;

	regmap_read(mt_g->regmap, MT3615_PSEQ2_W1C_STATUS0, &val);
	buf[0] = (val & 0xFF00) >> 8;
	buf[1] = (val & 0x00FF);
	regmap_read(mt_g->regmap, MT3615_PSEQ2_W1C_STATUS1, &val);
	buf[2] = (val & 0xFF00) >> 8;
	buf[3] = (val & 0x00FF);
	regmap_read(mt_g->regmap, MT3615_PSEQ2_W1C_STATUS2, &val);
	buf[4] = (val & 0xFF00) >> 8;
	buf[5] = (val & 0x00FF);
	regmap_read(mt_g->regmap, MT3615_BUCK_TOP_INT_STATUS0, &val);
	buf[6] = (val & 0xFF00) >> 8;
	buf[7] = (val & 0x00FF);
	regmap_read(mt_g->regmap, MT3615_LDO_TOP_INT_STATUS0, &val);
	buf[8] = (val & 0xFF00) >> 8;
	buf[9] = (val & 0x00FF);
	return PMIC_STATUS_SIZE;
}

static ssize_t store_pmic_status(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	if (size > PMIC_STATUS_SIZE)
		return -EINVAL;

	regmap_write(mt_g->regmap, MT3615_PSEQ2_W1C_STATUS0,
		buf[0] << 8 | buf[1]);
	regmap_write(mt_g->regmap, MT3615_PSEQ2_W1C_STATUS1,
		buf[2] << 8 | buf[3]);
	regmap_write(mt_g->regmap, MT3615_PSEQ2_W1C_STATUS2,
		buf[4] << 8 | buf[5]);
	regmap_write(mt_g->regmap, MT3615_BUCK_TOP_INT_STATUS0,
		buf[6] << 8 | buf[7]);
	regmap_write(mt_g->regmap, MT3615_LDO_TOP_INT_STATUS0,
		buf[8] << 8 | buf[9]);
	return size;
}

#define PMIC_MEM_SIZE	32
static ssize_t show_pmic_mem(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	int val;

	for (i = 0; i < PMIC_MEM_SIZE; i = i + 2) {
		regmap_read(mt_g->regmap, MT3615_PMIC_MEM_0 + i, &val);
		buf[i]     = (val & 0xFF00) >> 8;
		buf[i + 1] = val & 0x00FF;
	}
	return PMIC_MEM_SIZE;
}
static ssize_t store_pmic_mem(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int i;

	if (size > PMIC_MEM_SIZE)
		return -EINVAL;
	for (i = 0; i < size; i = i + 2) {
		regmap_write(mt_g->regmap, MT3615_PMIC_MEM_0 + i,
						buf[i] << 8 | buf[i + 1]);
	}
	return size;
}

static ssize_t store_pmic_vefuse(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	struct regulator *pmic_reg;

	pmic_reg = regulator_get(dev, "vefuse");
	if (IS_ERR(pmic_reg)) {
		dev_err(dev, "regulator_get failed\n");
		return PTR_ERR(pmic_reg);
	}
	if (!strncmp(buf, "on", 2)) {
		ret = regulator_enable(pmic_reg);
		if (ret) {
			dev_err(dev, "failed to enable regulator\n");
			return ret;
		}
	} else if (!strncmp(buf, "off", 3)) {
		ret = regulator_disable(pmic_reg);
		if (ret) {
			dev_err(dev, "failed to disable regulator\n");
			return ret;
		}
	}
	return size;
}

static ssize_t show_pmic_efuse_version(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret,val;

	ret = regmap_read(mt_g->regmap, MT3615_TOP2_ELR1, &val);
	if (ret != 0) {
		return ret;
	}
	*buf = (val & 0xFF00) >> 8;
	return 1;
}

static ssize_t show_pmic_vdram(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret,val;

	ret = regmap_read(mt_g->regmap, MT3615_BUCK_VDRAM2_ELR0, &val);
	if (ret != 0) {
		return ret;
	}
	*buf = (val & 0x00FF);
	return 1;
}

static DEVICE_ATTR(status, 0664, show_pmic_status, store_pmic_status);
static DEVICE_ATTR(mem, 0664, show_pmic_mem, store_pmic_mem);
static DEVICE_ATTR(vefuse, 0200, NULL, store_pmic_vefuse);
static DEVICE_ATTR(efuse_version, 0400, show_pmic_efuse_version, NULL);
static DEVICE_ATTR(vdram, 0400, show_pmic_vdram, NULL);

#if defined(CONFIG_MFD_MT3615_REG)
static ssize_t show_pmic_reg(struct device *dev
	, struct device_attribute *attr, char *buf)
{
	int val = 0;
	int ret = 0;

	ret = _pmic_read_reg(0x0, &val, 0xffff, 0);
	dev_err(mt_g->dev, "pmic_reg read add= 0x%x,val= 0x%x\n", 0, val);
	ret = _pmic_read_reg(0x8, &val, 0xffff, 0);
	dev_err(mt_g->dev, "pmic_reg read add= 0x%x,val= 0x%x\n", 8, val);

	return sprintf(buf, "%04X\n", val);
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
			ret = _pmic_read_reg(address, &value, mask, shift);
			dev_err(mt_g->dev, "pmic_reg read = 0x%x\n", value);
		}
	} else if (!strncmp(buf, "w", 1)) {
		if (sscanf(buf + 2, "%x %x %x %x", &address, &value, &mask,
			&shift) < 5) {
			dev_err(mt_g->dev, "input %x,%x,%x,%x\n",
				address, value, mask, shift);
			ret = _pmic_read_reg(address, &val, mask, shift);
			dev_err(mt_g->dev, "pmic_reg org read = 0x%x\n", val);
			ret = _pmic_config_reg(address, value, mask, shift);
			ret = _pmic_read_reg(address, &val, mask, shift);
			dev_err(mt_g->dev, "pmic_reg w af read = 0x%x\n", val);
		}
	} else {
		dev_err(mt_g->dev, "r/w command error");
	}
	return size;
}
static DEVICE_ATTR(pmic_reg, 0664, show_pmic_reg, store_pmic_reg);
#endif

static int mt3615_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int id;
	struct mt3615_chip *pmic;

	pmic = devm_kzalloc(&pdev->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;
	pmic->dev = &pdev->dev;

	/*
	 * mt3615 MFD is child device of soc pmic wrapper.
	 * Regmap is set from its parent.
	 */
	pmic->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!pmic->regmap) {
		ret = -ENODEV;
		goto err;
	}

	platform_set_drvdata(pdev, pmic);

	/*check pmic HWID.*/
	ret = regmap_read(pmic->regmap, MT3615_HWCID, &id);
	if (ret) {
		dev_err(pmic->dev, "Failed to read chip id: %d\n", ret);
		ret = -ENODEV;
		goto err;
	}
	mt_g = pmic;

	switch (id & 0xff00) {
	case MT3615_CID_CODE:
		ret = mfd_add_devices(&pdev->dev, -1, mt3615_devs,
					ARRAY_SIZE(mt3615_devs), NULL, 0, NULL);
		if (ret) {
			dev_err(&pdev->dev, "failed to add devices: %d\n", ret);
			ret = -ENODEV;
			goto err;
		}
		break;
	default:
		dev_err(&pdev->dev, "unsupported chip: %d\n", id);
		ret = -ENODEV;
		goto err;
	}

	/*set soc ready status to pmic.*/
	mt3615_set_soc_ready(1);

	if (device_create_file(&(pdev->dev), &dev_attr_status))
		device_remove_file(&(pdev->dev), &dev_attr_status);
	if (device_create_file(&(pdev->dev), &dev_attr_mem))
		device_remove_file(&(pdev->dev), &dev_attr_mem);
	if (device_create_file(&(pdev->dev), &dev_attr_vefuse))
		device_remove_file(&(pdev->dev), &dev_attr_vefuse);
	if (device_create_file(&(pdev->dev), &dev_attr_efuse_version))
		device_remove_file(&(pdev->dev), &dev_attr_efuse_version);
	if (device_create_file(&(pdev->dev), &dev_attr_vdram))
		device_remove_file(&(pdev->dev), &dev_attr_vdram);
#if defined(CONFIG_MFD_MT3615_REG)
	if (device_create_file(&(pdev->dev), &dev_attr_pmic_reg))
		device_remove_file(&(pdev->dev), &dev_attr_pmic_reg);
#endif

err:
	if (ret) {
		dev_err(&pdev->dev, "failed to add child devices: %d\n", ret);
		devm_kfree(pmic->dev, pmic);
	}
	dev_dbg(pmic->dev, "mt3615_probe done\n");

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
