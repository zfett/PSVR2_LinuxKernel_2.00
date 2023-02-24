/*
 * Copyright (c) 2015 Pengutronix, Sascha Hauer <kernel@pengutronix.de>
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

#include <linux/export.h>
#include <linux/jiffies.h>
#include <linux/regmap.h>
#include <linux/soc/mediatek/infracfg.h>
#include <asm/processor.h>

/**
 * mtk_infracfg_set_bus_protection - enable bus protection
 * @regmap: The infracfg regmap
 * @mask: The mask containing the protection bits to be enabled.
 *
 * This function enables the bus protection bits for disabled power
 * domains so that the system does not hang when some unit accesses the
 * bus while in power down.
 */
int mtk_infracfg_set_bus_protection(struct regmap *infracfg, u32 prot,
					u32 protsta, u32 mask)
{
	unsigned long expired;
	u32 val;
	int ret;

	regmap_update_bits(infracfg, prot, mask, mask);

	expired = jiffies + HZ/10; /* timeout:100ms */

	while (1) {
		ret = regmap_read(infracfg, protsta, &val);
		if (ret)
			return ret;

		if ((val & mask) == mask)
			break;

		cpu_relax();
		if (time_after(jiffies, expired))
			break;
	}

	return 0;
}

/**
 * mtk_infracfg_clear_bus_protection - disable bus protection
 * @regmap: The infracfg regmap
 * @mask: The mask containing the protection bits to be disabled.
 *
 * This function disables the bus protection bits previously enabled with
 * mtk_infracfg_set_bus_protection.
 */
int mtk_infracfg_clear_bus_protection(struct regmap *infracfg, u32 prot,
					u32 protsta, u32 mask)
{
	unsigned long expired;
	int ret;

	regmap_update_bits(infracfg, prot, mask, 0);

	expired = jiffies + HZ;

	while (1) {
		u32 val;

		ret = regmap_read(infracfg, protsta, &val);
		if (ret)
			return ret;

		if (!(val & mask))
			break;

		cpu_relax();
		if (time_after(jiffies, expired))
			return -EIO;
	}

	return 0;
}

/**
 * mtk_infracfg_set_topaxi_si0
 * @regmap: The infracfg regmap
 * @mask: The mask containing the path to be disable/enable
 */
int mtk_infracfg_set_topaxi_si0(struct regmap *infracfg, u32 ctlreg,
					u32 mask, u32 val)
{
	regmap_update_bits(infracfg, ctlreg, mask, val);

	return 0;
}


