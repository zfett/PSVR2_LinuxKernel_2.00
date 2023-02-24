/*
 * Copyright (C) 2019 MediaTek Inc.
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

#include "power_controller.h"

/**/
#define SRAM2_GRP_NUM 12
#define SRAM2_PWR_GRP_VALID_BIT 19
#define SRAM2_ADDR2GRP(addr) ((addr) >> SRAM2_PWR_GRP_VALID_BIT)

/* Data Types */
struct power_group {
	int ref;
	u32 mask1;
	u32 mask2;
};

/* Internal Variables */
static struct power_group sram2_pwr_tab[SRAM2_GRP_NUM] = {
	/** Group 0: 0x0000_0000 ~ 0x0007_FFFF */
	{ .mask1 = 0x01001001, .mask2 = 0x00000010 },
	/** Group 1: 0x0008_0000 ~ 0x000F_FFFF */
	{ .mask1 = 0x02002002, .mask2 = 0x00000020 },
	/** Group 2: 0x0010_0000 ~ 0x0017_FFFF */
	{ .mask1 = 0x04004004, .mask2 = 0x00000040 },
	/** Group 3: 0x0018_0000 ~ 0x001F_FFFF */
	{ .mask1 = 0x08008008, .mask2 = 0x00000080 },
	/** Group 4: 0x0020_0000 ~ 0x0027_FFFF */
	{ .mask1 = 0x10010010, .mask2 = 0x00000100 },
	/** Group 5: 0x0028_0000 ~ 0x002F_FFFF */
	{ .mask1 = 0x20020020, .mask2 = 0x00000200 },
	/** Group 6: 0x0030_0000 ~ 0x0037_FFFF */
	{ .mask1 = 0x40040040, .mask2 = 0x00000400 },
	/** Group 7: 0x0038_0000 ~ 0x003F_FFFF */
	{ .mask1 = 0x80080080, .mask2 = 0x00000800 },
	/** Group 8: 0x0040_0000 ~ 0x0047_FFFF */
	{ .mask1 = 0x00100100, .mask2 = 0x00001001 },
	/** Group 9: 0x0048_0000 ~ 0x004F_FFFF */
	{ .mask1 = 0x00200200, .mask2 = 0x00002002 },
	/** Group 10: 0x0050_0000 ~ 0x0057_FFFF */
	{ .mask1 = 0x00400400, .mask2 = 0x00004004 },
	/** Group 11: 0x0058_0000 ~ 0x0058_FFFF */
	{ .mask1 = 0x00800800, .mask2 = 0x00008008 },
};

/* Internal Functions */
static int init_individual(struct power_controller *this)
{
	int i;

	switch (this->type) {
	case PWR_CTRL_TYPE_NORMAL:
		break;

	case PWR_CTRL_TYPE_3612_SRAM2:
		/* Initialize reference counter */
		for (i = 0; i < SRAM2_GRP_NUM; i++)
			sram2_pwr_tab[i].ref = 0;

		break;

	default:
		WARN_ON(1);
	}

	return 0;
}

static int exit_individual(struct power_controller *this)
{
	int i;

	switch (this->type) {
	case PWR_CTRL_TYPE_NORMAL:
		break;

	case PWR_CTRL_TYPE_3612_SRAM2:
		/* Check reference counter */
		for (i = 0; i < SRAM2_GRP_NUM; i++) {
			if (sram2_pwr_tab[i].ref != 0) {
				WARN_ON(1);
				return -EBUSY;
			}
		}

		break;

	default:
		WARN_ON(1);
	}

	return 0;
}

static int power_on_individual(struct power_controller *this, int addr,
								int size)
{
	int end;
	int start_grp, end_grp;
	u32 reg_pd1, reg_pd2;
	int i;

	/* Sanity check */
	if (!this || !this->dev || !this->base) {
		WARN_ON(1);
		return -EINVAL;
	}

	/* Initialize */
	end = addr + size - 1;

	/**/
	switch (this->type) {
	case PWR_CTRL_TYPE_NORMAL:
		break;

	case PWR_CTRL_TYPE_3612_SRAM2:
		/* Read PD registers */
		reg_pd1 = readl(this->base + SYS2_LOCAL_SRAM_PD_1);
		reg_pd2 = readl(this->base + SYS2_LOCAL_SRAM_PD_2);

		/* Check power group */
		start_grp = SRAM2_ADDR2GRP(addr);
		end_grp = SRAM2_ADDR2GRP(end);

		/**/
		if (start_grp > end_grp) {
			WARN_ON(1);
			return -EINVAL;
		}

		if ((unsigned)start_grp >= SRAM2_GRP_NUM) {
			WARN_ON(1);
			return -EINVAL;
		}

		if ((unsigned)end_grp >= SRAM2_GRP_NUM) {
			WARN_ON(1);
			return -EINVAL;
		}

		/**/
		for (i = start_grp; i <= end_grp; i++) {
			sram2_pwr_tab[i].ref++;
			reg_pd1 &= ~sram2_pwr_tab[i].mask1;
			reg_pd2 &= ~sram2_pwr_tab[i].mask2;
		}

		/* Write PD registers */
		writel(reg_pd1, this->base + SYS2_LOCAL_SRAM_PD_1);
		writel(reg_pd2, this->base + SYS2_LOCAL_SRAM_PD_2);

		break;

	default:
		WARN_ON(1);
	}

	return 0;
}

static int power_off_individual(struct power_controller *this, int addr,
								int size)
{
	int end;
	int start_grp, end_grp;
	u32 reg_pd1, reg_pd2;
	int i;

	/* Initialize */
	end = addr + size - 1;

	/**/
	switch (this->type) {
	case PWR_CTRL_TYPE_NORMAL:
		break;

	case PWR_CTRL_TYPE_3612_SRAM2:
		/* Read PD registers */
		reg_pd1 = readl(this->base + SYS2_LOCAL_SRAM_PD_1);
		reg_pd2 = readl(this->base + SYS2_LOCAL_SRAM_PD_2);

		/* Check power group */
		start_grp = SRAM2_ADDR2GRP(addr);
		end_grp = SRAM2_ADDR2GRP(end);

		/**/
		if (start_grp > end_grp) {
			WARN_ON(1);
			return -EINVAL;
		}

		if ((unsigned)start_grp >= SRAM2_GRP_NUM) {
			WARN_ON(1);
			return -EINVAL;
		}

		if ((unsigned)end_grp >= SRAM2_GRP_NUM) {
			WARN_ON(1);
			return -EINVAL;
		}

		/**/
		for (i = start_grp; i <= end_grp; i++) {
			WARN_ON(sram2_pwr_tab[i].ref == 0);

			if (--sram2_pwr_tab[i].ref == 0) {
				reg_pd1 |= sram2_pwr_tab[i].mask1;
				reg_pd2 |= sram2_pwr_tab[i].mask2;
			}
		}

		/* Write PD registers */
		writel(reg_pd1, this->base + SYS2_LOCAL_SRAM_PD_1);
		writel(reg_pd2, this->base + SYS2_LOCAL_SRAM_PD_2);

		break;

	default:
		WARN_ON(1);
	}

	return 0;
}

/* External Functions */
int pwr_ctrl_init(struct power_controller *this, struct device *dev, int size,
							void *base, int type)
{
	/* Sanity check */
	if (!this) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (size <= 0) {
		WARN_ON(1);
		return -EINVAL;
	}

	if ((unsigned)type >= _PWR_CTRL_TYPE_NUM) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (base == NULL) {
		WARN_ON(1);
		return -EINVAL;
	}

	/* Initialize common part */
	this->type = type;
	this->dev = dev;
	this->size = size;
	INIT_LIST_HEAD(&this->addr_list);
	this->base = base;

	/* Initialize individual part */
	if (init_individual(this)) {
		WARN_ON(1);
		return -EINVAL;
	}

	return 0;
}

int pwr_ctrl_exit(struct power_controller *this)
{
	int ret;

	/* Sanity check */
	if (!this) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (!this->dev) {
		WARN_ON(1);
		return -EINVAL;
	}

	/* Check addr list */
	if (!list_empty(&this->addr_list))
		return -EBUSY;

	/* Cleanup individual part */
	ret = exit_individual(this);
	if (ret)
		return ret;

	return 0;
}

int pwr_ctrl_require(struct power_controller *this, int addr, int size)
{
	int end;
	struct addr_node *node;

	/* Sanity check */
	if (!this) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (!this->dev) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (addr < 0)
		return -EINVAL;

	if (size <= 0)
		return -EINVAL;

	if (addr + size > this->size)
		return -EINVAL;

	/* Initialize */
	end = addr + size - 1;

	/* Check overlapping */
	list_for_each_entry(node, &this->addr_list, list) {
		if (!(end < node->begin) && !(node->end < addr)) {
			/* Overlapping */
			dev_warn(this->dev, "Region overlapping: 0x%x-0x%x\n",
						node->begin, node->end);
			return -EINVAL;
		}
	}

	/* Power on */
	if (power_on_individual(this, addr, size))
		return -EINVAL;

	/* Allocate new node */
	node = devm_kzalloc(this->dev, sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	node->begin = addr;
	node->end = end;

	/* Insert */
	list_add(&node->list, &this->addr_list);

	return 0;
}

int pwr_ctrl_release(struct power_controller *this, int addr, int size)
{
	int end;
	struct addr_node *node, *node_tmp;

	/* Sanity check */
	if (!this) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (!this->dev) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (addr < 0)
		return -EINVAL;

	if (size <= 0)
		return -EINVAL;

	if (addr + size > this->size)
		return -EINVAL;

	/* Initialize */
	end = addr + size - 1;

	/* Check overlapping */
	list_for_each_entry_safe(node, node_tmp, &this->addr_list, list) {
		if ((addr == node->begin) && (end == node->end)) {
			/* Power off */
			power_off_individual(this, addr, size);

			/* Remove node */
			list_del(&node->list);
			devm_kfree(this->dev, node);
			return 0;
		}
	}

	dev_warn(this->dev, "No matched region: 0x%x-0x%x\n", addr, end);

	return -EINVAL;
}
