/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>

#include <sce_km_defs.h>
#include <sieimu.h>
#include "sie_icm426/sie_icm426.h"

struct sieimu_pdata {
	int id;
};

static struct sieimu_pdata s_pdata = { .id = -1 };

struct sieimu_data *sieimuGetFifoAddress(void *handle)
{
#ifdef CONFIG_SIE_DEVELOP_BUILD
	/* Check if ICM426 */
	if (s_pdata.id != 0) {
		pr_err("%s: Unknown ID", __func__);
		return NULL;
	}
#endif

	return sie_icm426_get_fifo_address();
}
EXPORT_SYMBOL(sieimuGetFifoAddress);

int sieimuGetFifoSize(void *handle)
{
#ifdef CONFIG_SIE_DEVELOP_BUILD
	/* Check if ICM426 */
	if (s_pdata.id != 0) {
		pr_err("%s: Unknown ID", __func__);
		return -1;
	}
#endif

	return sie_icm426_get_fifo_size();
}
EXPORT_SYMBOL(sieimuGetFifoSize);

int sieimuRequestCallBack(void *handle,
			  void (*callback)(struct sieimu_data *ring_buffer))
{
#ifdef CONFIG_SIE_DEVELOP_BUILD
	/* Check if ICM426 */
	if (s_pdata.id != 0) {
		pr_err("%s: Unknown ID", __func__);
		return -1;
	}
#endif

	return sie_icm426_request_callback(callback);
}
EXPORT_SYMBOL(sieimuRequestCallBack);

int sieimu_probe(struct sieimu_spi *handle, int id)
{
	int ret;

	if (s_pdata.id != -1) {
		pr_err("%s: sieimu dirver has already probed\n", __func__);
		return -EINVAL;
	}

	switch (id) {
	case 0: /* ICM426 */
		ret = sie_icm426_probe(handle);
		break;
	default:
		dev_err(handle->dev, "%s: Unknown device id: %d\n", __func__, id);
		ret = -EINVAL;
		break;
	}

	if (!ret) {
		s_pdata.id = id;
	}

	return ret;
}

void sieimu_remove(struct sieimu_spi *handle)
{
	int id = s_pdata.id;

	switch (id) {
	case 0: /* ICM426 */
		sie_icm426_remove(handle);
		break;
	default:
		dev_err(handle->dev, "%s: Unknown device id: %d\n", __func__, id);
		break;
	}
	s_pdata.id = -1;
}

