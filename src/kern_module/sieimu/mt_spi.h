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
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

struct mt_spi_transfer {
	const u8	*tx_buf;
	u8		*rx_buf;
	unsigned	len;

	dma_addr_t	tx_dma;
	dma_addr_t	rx_dma;
	struct sg_table tx_sg;
	struct sg_table rx_sg;
	u32			speed_hz;
};

/** @ingroup IP_group_spi_internal_struct
 * @brief Struct keeping spi driver data
 */
struct mt_spi {
	struct device *dev;
	struct completion xfer_completion;
	u32 max_speed_hz;

	/** to get spi bus base address  */
	void __iomem *base;
	/** used for get or put spi status  */
	u32 state;
	/** used for get spi padsel number */
	int pad_num;
	/** used for get clk pointer */
	struct clk *parent_clk, *sel_clk, *spi_clk;
	/** used for save current spi_transfer pointer */
	struct mt_spi_transfer *cur_transfer;
	/** used for save current spi_transfer length */
	u32 xfer_len;
	/** used for get tx/rx scatterlist pointer */
	struct scatterlist *tx_sgl, *rx_sgl;
	/** used for get tx/rx scatterlist length */
	u32 tx_sgl_len, rx_sgl_len;
	/** used for IC compatible */
	const struct mt_spi_compatible *dev_comp;
	u16			mode;
};

struct mt_spi *mt_spi_probe(struct platform_device *pdev);
int mt_spi_remove(struct platform_device *pdev);
int mt_spi_sync_send(struct mt_spi *mdata, struct mt_spi_transfer *xfer);
