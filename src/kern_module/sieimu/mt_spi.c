/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Leilk Liu <leilk.liu@mediatek.com>
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

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spi.h>
#include "mt_spi.h"

/** @ingroup IP_group_spi_internal_def
 * @{
 */
#define SPI_CFG0_REG                      0x0000
#define SPI_CFG1_REG                      0x0004
#define SPI_TX_SRC_REG                    0x0008
#define SPI_RX_DST_REG                    0x000C
#define SPI_TX_DATA_REG                   0x0010
#define SPI_RX_DATA_REG                   0x0014
#define SPI_CMD_REG                       0x0018
#define SPI_STATUS0_REG                   0x001C
#define SPI_STATUS1_REG                   0x0020
#define SPI_CFG2_REG                      0x0028

#define SPI_CFG0_SCK_HIGH_OFFSET          0
#define SPI_CFG0_SCK_LOW_OFFSET           16
#define SPI_CFG0_CS_HOLD_OFFSET           0
#define SPI_CFG0_CS_SETUP_OFFSET          16

#define SPI_CFG1_CS_IDLE_OFFSET           0
#define SPI_CFG1_PACKET_LOOP_OFFSET       8
#define SPI_CFG1_PACKET_LENGTH_OFFSET     16
#define SPI_CFG1_GET_TICK_DLY_OFFSET      29

#define SPI_CFG1_CS_IDLE_MASK             0xff
#define SPI_CFG1_PACKET_LOOP_MASK         0xff00
#define SPI_CFG1_PACKET_LENGTH_MASK       0x3ff0000
#define SPI_CFG1_GET_TICK_DLY_MASK        0xe0000000

#define SPI_CMD_ACT                  BIT(0)
#define SPI_CMD_RESUME               BIT(1)
#define SPI_CMD_RST                  BIT(2)
#define SPI_CMD_PAUSE_EN             BIT(4)
#define SPI_CMD_DEASSERT             BIT(5)
#define SPI_CMD_CPHA                 BIT(8)
#define SPI_CMD_CPOL                 BIT(9)
#define SPI_CMD_RX_DMA               BIT(10)
#define SPI_CMD_TX_DMA               BIT(11)
#define SPI_CMD_TXMSBF               BIT(12)
#define SPI_CMD_RXMSBF               BIT(13)
#define SPI_CMD_RX_ENDIAN            BIT(14)
#define SPI_CMD_TX_ENDIAN            BIT(15)
#define SPI_CMD_FINISH_IE            BIT(16)
#define SPI_CMD_PAUSE_IE             BIT(17)
#define SPI_CMD_CS_GPIO             BIT(18)

#define MT_SPI_PAUSE_INT_STATUS 0x2

#define MT_SPI_IDLE 0
#define MT_SPI_PAUSED 1

#define MT_SPI_MAX_FIFO_SIZE 32
#define MT_SPI_PACKET_SIZE 1024

/**
  * @}
  */

#define RX_MLSB 1
#define TX_MLSB 1
#define GET_TICK_DLY 0

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *     soft-reset spi HW.
 * @param[in]
 *     mdata: mt spi driver data pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_reset(struct mt_spi *mdata)
{
	u32 reg_val;

	/* set the software reset bit in SPI_CMD_REG. */
	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val |= SPI_CMD_RST;
	writel(reg_val, mdata->base + SPI_CMD_REG);

	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RST;
	writel(reg_val, mdata->base + SPI_CMD_REG);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   prepare hw configure setting for spi_message.
 * @param[in]
 *     mdata: mt spi driver data pointer.
 * @return
 *	   0, always successed.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_spi_prepare_message(struct mt_spi *mdata)
{
	u16 cpha, cpol;
	u32 reg_val;

	cpha = (mdata->mode & SPI_CPHA) ? 1 : 0;
	cpol = (mdata->mode & SPI_CPOL) ? 1 : 0;

	reg_val = readl(mdata->base + SPI_CMD_REG);
	if (cpha)
		reg_val |= SPI_CMD_CPHA;
	else
		reg_val &= ~SPI_CMD_CPHA;
	if (cpol)
		reg_val |= SPI_CMD_CPOL;
	else
		reg_val &= ~SPI_CMD_CPOL;

	/* set the mlsbx and mlsbtx */
#if TX_MLSB == 1
	reg_val |= SPI_CMD_TXMSBF;
#else
	reg_val &= ~SPI_CMD_TXMSBF;
#endif

#if RX_MLSB == 1
	reg_val |= SPI_CMD_RXMSBF;
#else
	reg_val &= ~SPI_CMD_RXMSBF;
#endif

	/* set the tx/rx endian */
#ifdef __LITTLE_ENDIAN
	reg_val &= ~SPI_CMD_TX_ENDIAN;
	reg_val &= ~SPI_CMD_RX_ENDIAN;
#else
	reg_val |= SPI_CMD_TX_ENDIAN;
	reg_val |= SPI_CMD_RX_ENDIAN;
#endif

	/* set finish and pause interrupt always enable */
	reg_val |= SPI_CMD_FINISH_IE | SPI_CMD_PAUSE_IE;

	/* disable dma mode */
	reg_val &= ~(SPI_CMD_TX_DMA | SPI_CMD_RX_DMA);

	/* disable deassert mode */
	reg_val &= ~SPI_CMD_DEASSERT;

	writel(reg_val, mdata->base + SPI_CMD_REG);

	reg_val = readl(mdata->base + SPI_CFG1_REG);
	reg_val &= ~SPI_CFG1_GET_TICK_DLY_MASK;
	reg_val |= GET_TICK_DLY << SPI_CFG1_GET_TICK_DLY_OFFSET;
	writel(reg_val, mdata->base + SPI_CFG1_REG);

	return 0;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	triggle spi hw to pull up/down cs.
 * @param[in]
 *	mdata: mt spi driver data pointer.
  * @param[in]
 *	enable: the flag to set spi hw cs high/low.
 * @return
 *	none.
 * @par Boundary case and Limitation
 *	none.
 * @par Error case and Error handling
 *	none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_set_cs(struct mt_spi *mdata, bool enable)
{
	u32 reg_val;

	reg_val = readl(mdata->base + SPI_CMD_REG);
	if (!enable) {
		reg_val |= SPI_CMD_PAUSE_EN;
		writel(reg_val, mdata->base + SPI_CMD_REG);
	} else {
		reg_val &= ~SPI_CMD_PAUSE_EN;
		writel(reg_val, mdata->base + SPI_CMD_REG);
		mdata->state = MT_SPI_IDLE;
		mt_spi_reset(mdata);
	}
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   prepare hw configure setting for spi_transfer.
 *	   for example, set clk,cs time.
 * @param[in]
 *	   mdata: mt spi driver data pointer.
  * @param[in]
 *	   xfer: the read/write buffer pair pointer.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_prepare_transfer(struct mt_spi *mdata,
				    struct mt_spi_transfer *xfer)
{
	u32 spi_clk_hz, div, sck_time, cs_time, reg_val = 0;

#if defined(CONFIG_COMMON_CLK_MT3612)
	spi_clk_hz = clk_get_rate(mdata->spi_clk);
#else
	spi_clk_hz = 6000000;
#endif

	if (!xfer->speed_hz) {
		xfer->speed_hz = mdata->max_speed_hz;
	}

	pr_debug("spi_clk_hz=%d,speed_hz=%d\n", spi_clk_hz, xfer->speed_hz);

	if (xfer->speed_hz < spi_clk_hz / 2)
		div = DIV_ROUND_UP(spi_clk_hz, xfer->speed_hz);
	else
		div = 1;

	sck_time = (div + 1) / 2;
	cs_time = 10 + sck_time;

	reg_val |= (((sck_time - 1) & 0xff) << SPI_CFG0_SCK_HIGH_OFFSET);
	reg_val |= (((sck_time - 1) & 0xff) << SPI_CFG0_SCK_LOW_OFFSET);
	writel(reg_val, mdata->base + SPI_CFG2_REG);

	reg_val |= (((cs_time - 1) & 0xff) << SPI_CFG0_CS_HOLD_OFFSET);
	reg_val |= (((cs_time - 1) & 0xff) << SPI_CFG0_CS_SETUP_OFFSET);
	writel(reg_val, mdata->base + SPI_CFG0_REG);

	reg_val = readl(mdata->base + SPI_CFG1_REG);
	reg_val &= ~SPI_CFG1_CS_IDLE_MASK;
	reg_val |= (((cs_time - 1) & 0xff) << SPI_CFG1_CS_IDLE_OFFSET);
	writel(reg_val, mdata->base + SPI_CFG1_REG);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   config spi transfer length
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_setup_packet(struct mt_spi *mdata)
{
	u32 packet_size, packet_loop, reg_val;

	packet_size = min_t(u32, mdata->xfer_len, MT_SPI_PACKET_SIZE);
	packet_loop = mdata->xfer_len / packet_size;

	reg_val = readl(mdata->base + SPI_CFG1_REG);
	reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
	reg_val |= (packet_size - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET;
	reg_val |= (packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET;
	writel(reg_val, mdata->base + SPI_CFG1_REG);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   enable spi hw to triggle transfer
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_enable_transfer(struct mt_spi *mdata)
{
	u32 cmd;

	cmd = readl(mdata->base + SPI_CMD_REG);
	if (mdata->state == MT_SPI_IDLE)
		cmd |= SPI_CMD_ACT;
	else
		cmd |= SPI_CMD_RESUME;
	writel(cmd, mdata->base + SPI_CMD_REG);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   get remainder of transfer length.
 * @param[in]
 *	   xfer_len: spi transfer length.
 * @return
 *	   the remainder of transfer length.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_spi_get_mult_delta(u32 xfer_len)
{
	u32 mult_delta;

	if (xfer_len > MT_SPI_PACKET_SIZE)
		mult_delta = xfer_len % MT_SPI_PACKET_SIZE;
	else
		mult_delta = 0;

	return mult_delta;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   update transfer length.
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_update_mdata_len(struct mt_spi *mdata)
{
	int mult_delta;

	if (mdata->tx_sgl_len && mdata->rx_sgl_len) {
		if (mdata->tx_sgl_len > mdata->rx_sgl_len) {
			mult_delta = mt_spi_get_mult_delta(mdata->rx_sgl_len);
			mdata->xfer_len = mdata->rx_sgl_len - mult_delta;
			mdata->rx_sgl_len = mult_delta;
			mdata->tx_sgl_len -= mdata->xfer_len;
		} else {
			mult_delta = mt_spi_get_mult_delta(mdata->tx_sgl_len);
			mdata->xfer_len = mdata->tx_sgl_len - mult_delta;
			mdata->tx_sgl_len = mult_delta;
			mdata->rx_sgl_len -= mdata->xfer_len;
		}
	} else if (mdata->tx_sgl_len) {
		mult_delta = mt_spi_get_mult_delta(mdata->tx_sgl_len);
		mdata->xfer_len = mdata->tx_sgl_len - mult_delta;
		mdata->tx_sgl_len = mult_delta;
	} else if (mdata->rx_sgl_len) {
		mult_delta = mt_spi_get_mult_delta(mdata->rx_sgl_len);
		mdata->xfer_len = mdata->rx_sgl_len - mult_delta;
		mdata->rx_sgl_len = mult_delta;
	}
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   write tx/rx dma address to tx/rx dma pointer.
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @param[in]
 *	   xfer: the read/write buffer pair pointer.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_setup_dma_addr(struct mt_spi *mdata,
				  struct mt_spi_transfer *xfer)
{
	if (mdata->tx_sgl)
		writel(xfer->tx_dma, mdata->base + SPI_TX_SRC_REG);
	if (mdata->rx_sgl)
		writel(xfer->rx_dma, mdata->base + SPI_RX_DST_REG);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   config spi fifo mode transfer and triggle transfer.
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @param[in]
 *	   xfer: the read/write buffer pair pointer.
 * @return
 *	   1, always success.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_spi_fifo_transfer(struct mt_spi *mdata,
				struct mt_spi_transfer *xfer)
{
	int cnt, remainder;
	u32 reg_val;

	mdata->cur_transfer = xfer;
	mdata->xfer_len = xfer->len;
	mt_spi_prepare_transfer(mdata, xfer);
	mt_spi_setup_packet(mdata);

	cnt = xfer->len / 4;
	iowrite32_rep(mdata->base + SPI_TX_DATA_REG, xfer->tx_buf, cnt);

	remainder = xfer->len % 4;
	if (remainder > 0) {
		reg_val = 0;
		memcpy(&reg_val, xfer->tx_buf + (cnt * 4), remainder);
		writel(reg_val, mdata->base + SPI_TX_DATA_REG);
	}

	mt_spi_enable_transfer(mdata);

	return 1;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   config spi dma mode transfer and triggle transfer.
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @param[in]
 *	   xfer: the read/write buffer pair pointer.
 * @return
 *	   1, always success.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_spi_dma_transfer(struct mt_spi *mdata,
			       struct mt_spi_transfer *xfer)
{
	int cmd;

	mdata->tx_sgl = NULL;
	mdata->rx_sgl = NULL;
	mdata->tx_sgl_len = 0;
	mdata->rx_sgl_len = 0;
	mdata->cur_transfer = xfer;

	mt_spi_prepare_transfer(mdata, xfer);

	cmd = readl(mdata->base + SPI_CMD_REG);
	if (xfer->tx_buf)
		cmd |= SPI_CMD_TX_DMA;
	if (xfer->rx_buf)
		cmd |= SPI_CMD_RX_DMA;
	writel(cmd, mdata->base + SPI_CMD_REG);

	if (xfer->tx_buf)
		mdata->tx_sgl = xfer->tx_sg.sgl;
	if (xfer->rx_buf)
		mdata->rx_sgl = xfer->rx_sg.sgl;

	if (mdata->tx_sgl) {
		xfer->tx_dma = sg_dma_address(mdata->tx_sgl);
		mdata->tx_sgl_len = sg_dma_len(mdata->tx_sgl);
	}
	if (mdata->rx_sgl) {
		xfer->rx_dma = sg_dma_address(mdata->rx_sgl);
		mdata->rx_sgl_len = sg_dma_len(mdata->rx_sgl);
	}

	mt_spi_update_mdata_len(mdata);
	mt_spi_setup_packet(mdata);
	mt_spi_setup_dma_addr(mdata, xfer);
	mt_spi_enable_transfer(mdata);

	return 1;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   according to one transfer length to use dma or fifo mode.
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @param[in]
 *	   xfer: the read/write buffer pair pointer.
 * @return
 *	   0, use fifo mode.
 *	   1, use dma mode.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool mt_spi_can_dma(struct mt_spi *mdata,
			   struct mt_spi_transfer *xfer)
{
	return xfer->len > MT_SPI_MAX_FIFO_SIZE;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   set spi HW to do one transfer,
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @param[in]
 *	   xfer: the read/write buffer pair pointer.
 * @return
 *	   1, always success.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_spi_transfer_one(struct mt_spi *mdata,
			       struct mt_spi_transfer *xfer)
{
	if (mt_spi_can_dma(mdata, xfer))
		return mt_spi_dma_transfer(mdata, xfer);
	else
		return mt_spi_fifo_transfer(mdata, xfer);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   wait spi completion,\n
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @param[in]
 *	   ms: timeout value in ms.
 * @return
 *	   0 if timed out,
 *	   positive (at least 1, or number of jiffies left till timeout)
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_spi_wait_completion(struct mt_spi *mdata, unsigned long ms)
{
	ms = wait_for_completion_timeout(&mdata->xfer_completion,
					 msecs_to_jiffies(ms));
	return ms;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   Notify spi finallize,\n
 * @param[in]
 *	   mdata: mt spi driver data pointer.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_finalize_current_transfer(struct mt_spi *mdata)
{
	complete(&mdata->xfer_completion);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   map buffer for dma transfer.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_spi_map_buf(struct mt_spi *mdata, struct device *dev,
			  struct sg_table *sgt, void *buf, size_t len,
			  enum dma_data_direction dir)
{
	const bool vmalloced_buf = is_vmalloc_addr(buf);
	int desc_len;
	int sgs;
	struct page *vm_page;
	void *sg_buf;
	size_t min;
	int i, ret;

	if (vmalloced_buf) {
		desc_len = PAGE_SIZE;
		sgs = DIV_ROUND_UP(len + offset_in_page(buf), desc_len);
	} else {
		/* desc_len = mdata->max_dma_len; */
		desc_len = INT_MAX;
		sgs = DIV_ROUND_UP(len, desc_len);
	}

	ret = sg_alloc_table(sgt, sgs, GFP_KERNEL);
	if (ret != 0)
		return ret;

	for (i = 0; i < sgs; i++) {
		if (vmalloced_buf) {
			/*
			 * Next scatterlist entry size is the minimum between
			 * the desc_len and the remaining buffer length that
			 * fits in a page.
			 */
			min = min_t(size_t, desc_len,
				    min_t(size_t, len,
					  PAGE_SIZE - offset_in_page(buf)));
			vm_page = vmalloc_to_page(buf);
			if (!vm_page) {
				sg_free_table(sgt);
				return -ENOMEM;
			}
			sg_set_page(&sgt->sgl[i], vm_page,
				    min, offset_in_page(buf));
		} else {
			min = min_t(size_t, len, desc_len);
			sg_buf = buf;
			sg_set_buf(&sgt->sgl[i], sg_buf, min);
		}

		buf = (u8 *)buf + min;
		len -= min;
	}

	ret = dma_map_sg(dev, sgt->sgl, sgt->nents, dir);
	if (!ret)
		ret = -ENOMEM;
	if (ret < 0) {
		sg_free_table(sgt);
		return ret;
	}

	sgt->nents = ret;

	return 0;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   unmap buffer for dma transfer.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_spi_unmap_buf(struct mt_spi *mdata, struct device *dev,
			     struct sg_table *sgt, enum dma_data_direction dir)
{
	if (sgt->orig_nents) {
		dma_unmap_sg(dev, sgt->sgl, sgt->orig_nents, dir);
		sg_free_table(sgt);
	}
}

/**
 * @ingroup IP_group_spi_external_function
 * @par Description
 *	   send spi message with sync.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt_spi_sync_send(struct mt_spi *mdata, struct mt_spi_transfer *xfer)
{
	int ret = 0;
	struct device *tx_dev, *rx_dev;
	bool dma = mt_spi_can_dma(mdata, xfer);

	if (dma) {
		tx_dev = mdata->dev->parent;
		rx_dev = mdata->dev->parent;
		ret = mt_spi_map_buf(mdata, tx_dev, &xfer->tx_sg,
				     (void *)xfer->tx_buf, xfer->len,
				     DMA_TO_DEVICE);
		if (ret != 0)
			goto err;

		ret = mt_spi_map_buf(mdata, rx_dev, &xfer->rx_sg,
				     xfer->rx_buf, xfer->len,
				     DMA_FROM_DEVICE);
		if (ret != 0) {
			mt_spi_unmap_buf(mdata, tx_dev, &xfer->tx_sg,
					 DMA_TO_DEVICE);
			goto err;
		}
	}

	mt_spi_prepare_message(mdata);
	mt_spi_set_cs(mdata, false);
	mt_spi_transfer_one(mdata, xfer);

	/* Wait Transfer */
	mt_spi_wait_completion(mdata, 1000);

	mt_spi_set_cs(mdata, true);
	mt_spi_reset(mdata);

	if (dma) {
		mt_spi_unmap_buf(mdata, rx_dev, &xfer->rx_sg, DMA_FROM_DEVICE);
		mt_spi_unmap_buf(mdata, tx_dev, &xfer->tx_sg, DMA_TO_DEVICE);
	}

err:
	return ret;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   spi driver interrupt function.
 * @param[in]
 *	   irq: interrupt number.
 * @param[in]
 *	   dev_id: interface pointer to SPI master controller.
 * @return
 *	   IRQ_HANDLED.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mt_spi_interrupt(int irq, void *dev_id)
{
	u32 cmd, reg_val, cnt, remainder;
	struct mt_spi *mdata = dev_id;
	struct mt_spi_transfer *trans;

	trans = mdata->cur_transfer;

	reg_val = readl(mdata->base + SPI_STATUS0_REG);
	if (reg_val & MT_SPI_PAUSE_INT_STATUS)
		mdata->state = MT_SPI_PAUSED;
	else
		mdata->state = MT_SPI_IDLE;

	if (!mt_spi_can_dma(mdata, trans)) {
		if (trans->rx_buf) {
			cnt = mdata->xfer_len / 4;
			ioread32_rep(mdata->base + SPI_RX_DATA_REG,
				     trans->rx_buf, cnt);
			remainder = mdata->xfer_len % 4;
			if (remainder > 0) {
				reg_val = readl(mdata->base + SPI_RX_DATA_REG);
				memcpy(trans->rx_buf + (cnt * 4),
				       &reg_val, remainder);
			}
		}
		mt_spi_finalize_current_transfer(mdata);
		return IRQ_HANDLED;
	}

	if (mdata->tx_sgl)
		trans->tx_dma += mdata->xfer_len;
	if (mdata->rx_sgl)
		trans->rx_dma += mdata->xfer_len;

	if (mdata->tx_sgl && (mdata->tx_sgl_len == 0)) {
		mdata->tx_sgl = sg_next(mdata->tx_sgl);
		if (mdata->tx_sgl) {
			trans->tx_dma = sg_dma_address(mdata->tx_sgl);
			mdata->tx_sgl_len = sg_dma_len(mdata->tx_sgl);
		}
	}
	if (mdata->rx_sgl && (mdata->rx_sgl_len == 0)) {
		mdata->rx_sgl = sg_next(mdata->rx_sgl);
		if (mdata->rx_sgl) {
			trans->rx_dma = sg_dma_address(mdata->rx_sgl);
			mdata->rx_sgl_len = sg_dma_len(mdata->rx_sgl);
		}
	}

	if (!mdata->tx_sgl && !mdata->rx_sgl) {
		/* spi disable dma */
		cmd = readl(mdata->base + SPI_CMD_REG);
		cmd &= ~SPI_CMD_TX_DMA;
		cmd &= ~SPI_CMD_RX_DMA;
		writel(cmd, mdata->base + SPI_CMD_REG);

		mt_spi_finalize_current_transfer(mdata);
		return IRQ_HANDLED;
	}

	mt_spi_update_mdata_len(mdata);
	mt_spi_setup_packet(mdata);
	mt_spi_setup_dma_addr(mdata, trans);
	mt_spi_enable_transfer(mdata);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_spi_external_function
 * @par Description
 *     spi probe function
 * @param[out]
 *     pdev: platform_device setting.
 * @return
 *     0, spi probe successed.\n
 *     -ENOMEM, failed to alloc spi master
 *     -EINVAL, failed to probe of_node
 *     -EINVAL, No 'mediatek,pad-select' property
 *     -ENODEV, failed to determine base address
 *     error code from devm_ioremap_resource()
 *     error code from platform_get_irq()
 *     error code from devm_request_irq()
 *     error code from devm_clk_get()
 *     error code from clk_prepare_enable()
 *     error code from devm_spi_register_master()
 *     -EINVAL,pad_num does not match num_chipselect
 *     -EINVAL,cs_gpios not specified and num_chipselect
 *     error code from devm_gpio_request()
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If spi_alloc_master() fails, return -ENOMEM
 *     2. If of_match_node() fails, return -ENOMEM
 *     3. If of_property_count_u32_elems() fails, return -ENOMEM
 *     4. If platform_get_resource() fails, return -ENOMEM
 *     5. If devm_ioremap_resource() fails, return its error code
 *     6. If platform_get_irq() fails, return its error code
 *     7. If devm_request_irq() fails, return its error code
 *     8. If devm_clk_get() fails, return its error code
 *     9. If clk_prepare_enable() fails, return its error code
 *     10. If devm_spi_register_master() fails, return its error code
 *     11. if mdata->pad_num != master->num_chipselect, return -EINVAL
 *     12. if !master->cs_gpios && master->num_chipselect > 1, return -EINVAL
 *     13. If devm_gpio_request() fails, return its error code
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct mt_spi *mt_spi_probe(struct platform_device *pdev)
{
	struct mt_spi *mdata;
	struct resource *res;
	int irq, ret;

	mdata = devm_kzalloc(&pdev->dev, sizeof(struct mt_spi), GFP_KERNEL);
	if (!mdata) {
		return NULL;
	}
	mdata->dev = &pdev->dev;
	mdata->mode = SPI_MODE_0;

	ret = of_property_read_u32(pdev->dev.of_node, "spi-max-frequency", &mdata->max_speed_hz);
	if (ret) {
		dev_err(&pdev->dev, "%s has no valid 'spi-max-frequency' property (%d)\n",
			pdev->dev.of_node->full_name, ret);
		goto err;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "failed to determine base address\n");
		goto err;
	}

	mdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mdata->base)) {
		ret = PTR_ERR(mdata->base);
		goto err;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed to get irq (%d)\n", irq);
		ret = irq;
		goto err;
	}

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	ret = devm_request_irq(&pdev->dev, irq, mt_spi_interrupt,
			       IRQF_TRIGGER_NONE, dev_name(&pdev->dev), mdata);
	if (ret) {
		dev_err(&pdev->dev, "failed to register irq (%d)\n", ret);
		goto err;
	}

#if defined(CONFIG_COMMON_CLK_MT3612)
	mdata->parent_clk = devm_clk_get(&pdev->dev, "parent-clk");
	if (IS_ERR(mdata->parent_clk)) {
		ret = PTR_ERR(mdata->parent_clk);
		dev_err(&pdev->dev, "failed to get parent-clk: %d\n", ret);
		goto err;
	}

	mdata->sel_clk = devm_clk_get(&pdev->dev, "sel-clk");
	if (IS_ERR(mdata->sel_clk)) {
		ret = PTR_ERR(mdata->sel_clk);
		dev_err(&pdev->dev, "failed to get sel-clk: %d\n", ret);
		goto err;
	}

	mdata->spi_clk = devm_clk_get(&pdev->dev, "spi-clk");
	if (IS_ERR(mdata->spi_clk)) {
		ret = PTR_ERR(mdata->spi_clk);
		dev_err(&pdev->dev, "failed to get spi-clk: %d\n", ret);
		goto err;
	}

	ret = clk_prepare_enable(mdata->spi_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable spi_clk (%d)\n", ret);
		goto err;
	}

	ret = clk_set_parent(mdata->sel_clk, mdata->parent_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to clk_set_parent (%d)\n", ret);
		clk_disable_unprepare(mdata->spi_clk);
		goto err;
	}
#endif

	init_completion(&mdata->xfer_completion);

	platform_set_drvdata(pdev, mdata);

	return mdata;

err:
	return NULL;
}

/** @ingroup IP_group_spi_external_function
 * @par Description
 *     SPI remove function used to close SPI driver
 * @param[out]
 *     pdev: platform_device setting.
 * @return
 *     0, remove successed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mt_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct mt_spi *mdata = spi_master_get_devdata(master);

	pm_runtime_disable(&pdev->dev);

	mt_spi_reset(mdata);
	spi_master_put(master);

	return 0;
}
