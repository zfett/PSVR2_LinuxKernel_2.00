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

#pragma once
#include <linux/mutex.h>

#define SPI_BUF_SIZE (1024 * 32)

struct sieimu_spi {
	int (*read)(struct sieimu_spi *handle,  u8 reg, u8 *buf, u32 len);
	int (*write)(struct sieimu_spi *handle, u8 reg, const u8 *buf, u32 len);
	int (*read_fifo)(struct sieimu_spi *handle, u8 fifo_reg, u32 len,
					 void (*completion)(u8 *buf, u32 len, void *arg),
					 void *arg);
	void* spi_info;
	struct device *dev;
	u8 *tx_buf;
	u8 *rx_buf;
	struct mutex buf_lock;  /* lock for tx/rx buf */
};

int sieimu_probe(struct sieimu_spi *handle, int id);
void sieimu_remove(struct sieimu_spi *handle);
