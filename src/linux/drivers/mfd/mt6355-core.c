/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Andrew-sh.Cheng, MediaTek
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
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt6355/core.h>
#include <linux/mfd/mt6355/registers.h>

/* Current wtih E1~E3, 0x5510, 0x5520, 0x5530 */
#define MT6355_CID_CODE		0x5500

static const struct resource mt6355_keys_resources[] = {
	{
		/* KEYPAD owner need to modify */
		.start = MT6355_INT_STATUS_PWRKEY,
		.end   = MT6355_INT_STATUS_HOMEKEY_R,
		.flags = IORESOURCE_IRQ,
	},
};

static const struct mfd_cell mt6355_devs[] = {
	{
		.name = "mt6355-regulator",
		.of_compatible = "mediatek,mt6355-regulator"
	}, {
		.name = "mt6355-auxadc",
		.of_compatible = "mediatek,mt6355-auxadc"
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6355_keys_resources),
		.resources = mt6355_keys_resources,
		.of_compatible = "mediatek,mt6392-keys"
	},
};


static void mt6355_irq_lock(struct irq_data *data)
{
	struct mt6355_chip *mt6355 = irq_data_get_irq_chip_data(data);

	mutex_lock(&mt6355->irqlock);
}

static void mt6355_irq_sync_unlock(struct irq_data *data)
{
	struct mt6355_chip *mt6355 = irq_data_get_irq_chip_data(data);

	regmap_write(mt6355->regmap, mt6355->int_con[0],
		     mt6355->irq_masks_cur[0]);
	regmap_write(mt6355->regmap, mt6355->int_con[1],
		     mt6355->irq_masks_cur[1]);
	regmap_write(mt6355->regmap, mt6355->int_con[2],
		     mt6355->irq_masks_cur[2]);
	regmap_write(mt6355->regmap, mt6355->int_con[3],
		     mt6355->irq_masks_cur[3]);
	regmap_write(mt6355->regmap, mt6355->int_con[4],
		     mt6355->irq_masks_cur[4]);
	regmap_write(mt6355->regmap, mt6355->int_con[5],
		     mt6355->irq_masks_cur[5]);
	regmap_write(mt6355->regmap, mt6355->int_con[6],
		     mt6355->irq_masks_cur[6]);

	mutex_unlock(&mt6355->irqlock);
}

static void mt6355_irq_disable(struct irq_data *data)
{
	struct mt6355_chip *mt6355 = irq_data_get_irq_chip_data(data);
	int shift = data->hwirq & 0xf;
	int reg = data->hwirq >> 4;

	mt6355->irq_masks_cur[reg] &= ~BIT(shift);
}

static void mt6355_irq_enable(struct irq_data *data)
{
	struct mt6355_chip *mt6355 = irq_data_get_irq_chip_data(data);
	int shift = data->hwirq & 0xf;
	int reg = data->hwirq >> 4;

	mt6355->irq_masks_cur[reg] |= BIT(shift);
}

#ifdef CONFIG_PM_SLEEP
static int mt6355_irq_set_wake(struct irq_data *irq_data, unsigned int on)
{
	struct mt6355_chip *mt6355 = irq_data_get_irq_chip_data(irq_data);
	int shift = irq_data->hwirq & 0xf;
	int reg = irq_data->hwirq >> 4;

	if (on)
		mt6355->wake_mask[reg] |= BIT(shift);
	else
		mt6355->wake_mask[reg] &= ~BIT(shift);

	return 0;
}
#else
#define mt6355_irq_set_wake NULL
#endif

static struct irq_chip mt6355_irq_chip = {
	.name = "mt6355-irq",
	.irq_bus_lock = mt6355_irq_lock,
	.irq_bus_sync_unlock = mt6355_irq_sync_unlock,
	.irq_enable = mt6355_irq_enable,
	.irq_disable = mt6355_irq_disable,
	.irq_set_wake = mt6355_irq_set_wake,
};

static void mt6355_irq_handle_reg(struct mt6355_chip *mt6355, int reg,
				  const int irqbase)
{
	unsigned int status;
	int i, irq, ret;

	ret = regmap_read(mt6355->regmap, reg, &status);
	if (ret) {
		dev_err(mt6355->dev, "Failed to read irq status: %d\n", ret);
		return;
	}

	for (i = 0; i < 16; i++) {
		if (status & BIT(i)) {
			irq = irq_find_mapping(mt6355->irq_domain, irqbase + i);
			if (irq)
				handle_nested_irq(irq);
		}
	}

	regmap_write(mt6355->regmap, reg, status);
}

static irqreturn_t mt6355_irq_thread(int irq, void *data)
{
	struct mt6355_chip *mt6355 = data;

	mt6355_irq_handle_reg(mt6355, mt6355->int_status[0], 0);
	mt6355_irq_handle_reg(mt6355, mt6355->int_status[1], 16);
	mt6355_irq_handle_reg(mt6355, mt6355->int_status[2], 32);
	mt6355_irq_handle_reg(mt6355, mt6355->int_status[3], 48);
	mt6355_irq_handle_reg(mt6355, mt6355->int_status[4], 64);
	mt6355_irq_handle_reg(mt6355, mt6355->int_status[5], 80);
	mt6355_irq_handle_reg(mt6355, mt6355->int_status[6], 96);

	return IRQ_HANDLED;
}

static int mt6355_irq_domain_map(struct irq_domain *d, unsigned int irq,
					irq_hw_number_t hw)
{
	struct mt6355_chip *mt6355 = d->host_data;

	irq_set_chip_data(irq, mt6355);
	irq_set_chip_and_handler(irq, &mt6355_irq_chip, handle_level_irq);
	irq_set_nested_thread(irq, 1);
	irq_set_noprobe(irq);

	return 0;
}

static const struct irq_domain_ops mt6355_irq_domain_ops = {
	.map = mt6355_irq_domain_map,
};
static int mt6355_irq_init(struct mt6355_chip *mt6355)
{
	int ret;

	mutex_init(&mt6355->irqlock);

	/* Mask all interrupt sources */
	regmap_write(mt6355->regmap, mt6355->int_con[0], 0x0);
	regmap_write(mt6355->regmap, mt6355->int_con[1], 0x0);
	regmap_write(mt6355->regmap, mt6355->int_con[2], 0x0);
	regmap_write(mt6355->regmap, mt6355->int_con[3], 0x0);
	regmap_write(mt6355->regmap, mt6355->int_con[4], 0x0);
	regmap_write(mt6355->regmap, mt6355->int_con[5], 0x0);
	regmap_write(mt6355->regmap, mt6355->int_con[6], 0x0);

	/* For all interrupt events, turn on interrupt module clock */
	regmap_write_bits(mt6355->regmap, PMIC_RG_INTRP_CK_PDN_ADDR,
			  PMIC_RG_INTRP_CK_PDN_MASK <<
			  PMIC_RG_INTRP_CK_PDN_SHIFT, 0);

	mt6355->irq_domain = irq_domain_add_linear(mt6355->dev->of_node,
		MT6355_IRQ_NR, &mt6355_irq_domain_ops, mt6355);
	if (!mt6355->irq_domain) {
		dev_err(mt6355->dev, "could not create irq domain\n");
		return -ENOMEM;
	}

	ret = devm_request_threaded_irq(mt6355->dev, mt6355->irq, NULL,
		mt6355_irq_thread, IRQF_ONESHOT, "mt6355-pmic", mt6355);
	if (ret) {
		dev_err(mt6355->dev, "failed to register irq=%d; err: %d\n",
			mt6355->irq, ret);
		return ret;
	}

	return 0;
}


#ifdef CONFIG_PM_SLEEP
static int mt6355_irq_suspend(struct device *dev)
{
	struct mt6355_chip *chip = dev_get_drvdata(dev);

	regmap_write(chip->regmap, chip->int_con[0], chip->wake_mask[0]);
	regmap_write(chip->regmap, chip->int_con[1], chip->wake_mask[1]);
	regmap_write(chip->regmap, chip->int_con[2], chip->wake_mask[2]);
	regmap_write(chip->regmap, chip->int_con[3], chip->wake_mask[3]);
	regmap_write(chip->regmap, chip->int_con[4], chip->wake_mask[4]);
	regmap_write(chip->regmap, chip->int_con[5], chip->wake_mask[5]);
	regmap_write(chip->regmap, chip->int_con[6], chip->wake_mask[6]);

	enable_irq_wake(chip->irq);

	return 0;
}

static int mt6355_irq_resume(struct device *dev)
{
	struct mt6355_chip *chip = dev_get_drvdata(dev);

	regmap_write(chip->regmap, chip->int_con[0], chip->irq_masks_cur[0]);
	regmap_write(chip->regmap, chip->int_con[1], chip->irq_masks_cur[1]);
	regmap_write(chip->regmap, chip->int_con[2], chip->irq_masks_cur[2]);
	regmap_write(chip->regmap, chip->int_con[3], chip->irq_masks_cur[3]);
	regmap_write(chip->regmap, chip->int_con[4], chip->irq_masks_cur[4]);
	regmap_write(chip->regmap, chip->int_con[5], chip->irq_masks_cur[5]);
	regmap_write(chip->regmap, chip->int_con[6], chip->irq_masks_cur[6]);

	disable_irq_wake(chip->irq);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mt6355_pm_ops, mt6355_irq_suspend,
			mt6355_irq_resume);

static int mt6355_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int id;
	struct mt6355_chip *pmic;

	pmic = devm_kzalloc(&pdev->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	pmic->dev = &pdev->dev;

	/*
	 * mt6355 MFD is child device of soc pmic wrapper.
	 * Regmap is set from its parent.
	 */
	pmic->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!pmic->regmap)
		return -ENODEV;

	platform_set_drvdata(pdev, pmic);

	ret = regmap_read(pmic->regmap, MT6355_HWCID, &id);
	if (ret) {
		dev_err(pmic->dev, "Failed to read chip id: %d\n", ret);
		return ret;
	}

	pmic->irq = platform_get_irq(pdev, 0);
	if (pmic->irq <= 0)
		return pmic->irq;

	switch (id & 0xff00) {
	case MT6355_CID_CODE:
		pmic->int_con[0] = MT6355_INT_CON0;
		pmic->int_con[1] = MT6355_INT_CON1;
		pmic->int_con[2] = MT6355_INT_CON2;
		pmic->int_con[3] = MT6355_INT_CON3;
		pmic->int_con[4] = MT6355_INT_CON4;
		pmic->int_con[5] = MT6355_INT_CON5;
		pmic->int_con[6] = MT6355_INT_CON6;
		pmic->int_status[0] = MT6355_INT_STATUS0;
		pmic->int_status[1] = MT6355_INT_STATUS1;
		pmic->int_status[2] = MT6355_INT_STATUS2;
		pmic->int_status[3] = MT6355_INT_STATUS3;
		pmic->int_status[4] = MT6355_INT_STATUS4;
		pmic->int_status[5] = MT6355_INT_STATUS5;
		pmic->int_status[6] = MT6355_INT_STATUS6;
		ret = mt6355_irq_init(pmic);
		if (ret)
			return ret;

		ret = mfd_add_devices(&pdev->dev, -1, mt6355_devs,
					   ARRAY_SIZE(mt6355_devs), NULL,
					   0, NULL);
		break;

	default:
		dev_err(&pdev->dev, "unsupported chip: %d\n", id);
		ret = -ENODEV;
		break;
	}


	if (ret) {
		irq_domain_remove(pmic->irq_domain);
		dev_err(&pdev->dev, "failed to add child devices: %d\n", ret);
	}

	return ret;
}

static const struct of_device_id mt6355_of_match[] = {
	{ .compatible = "mediatek,mt6355" },
	{ }
};
MODULE_DEVICE_TABLE(of, mt6355_of_match);

static const struct platform_device_id mt6355_id[] = {
	{ "mt6355", 0 },
	{ },
};
MODULE_DEVICE_TABLE(platform, mt6355_id);

static struct platform_driver mt6355_driver = {
	.probe = mt6355_probe,
	.driver = {
		.name = "mt6355",
		.of_match_table = of_match_ptr(mt6355_of_match),
		.pm = &mt6355_pm_ops,
	},
	.id_table = mt6355_id,
};

module_platform_driver(mt6355_driver);

MODULE_AUTHOR("Andrew-sh.Cheng, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT6355 PMIC");
MODULE_LICENSE("GPL");
