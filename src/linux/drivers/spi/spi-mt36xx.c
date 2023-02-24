/*
 * Copyright (c) 2015 MediaTek Inc.
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

/**
 * @file spi-mt36xx.c
 * The SPI driver provides the interfaces which will be used in linux
 * spi framework to control MTK SPI hardware module.
 */

/**
 * @defgroup IP_group_spi SPI
 *
 *	@{
 *		@defgroup IP_group_spi_external EXTERNAL
 *			The external API document for SPI. \n
 *
 *		@{
 *			@defgroup IP_group_spi_external_function 1.function
 *				none.
 *			@defgroup IP_group_spi_external_struct 2.structure
 *				none.
 *			@defgroup IP_group_spi_external_typedef 3.typedef
 *				none.
 *			@defgroup IP_group_spi_external_enum 4.enumeration
 *				none.
 *			@defgroup IP_group_spi_external_def 5.define
 *				none.
 *		@}
 *
 *		@defgroup IP_group_spi_internal INTERNAL
 *			The internal API document for SPI. \n
 *
 *		@{
 *			@defgroup IP_group_spi_internal_function 1.function
 *				Internal function to register to spi framework.
 *			@defgroup IP_group_spi_internal_struct 2.structure
 *				Internal structure used for spi.
 *			@defgroup IP_group_spi_internal_typedef 3.typedef
 *				none. Follow Linux coding style,\n
 *				no typedef used.
 *			@defgroup IP_group_spi_internal_enum 4.enumeration
 *				none. No enumeration in spi driver.
 *			@defgroup IP_group_spi_internal_def 5.define
 *				none. No extra define in spi driver.
 *		@}
 *	@}
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
#include <linux/platform_data/spi-mt65xx.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spi.h>

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

#define MTK_SPI_PAUSE_INT_STATUS 0x2

#define MTK_SPI_IDLE 0
#define MTK_SPI_PAUSED 1

#define MTK_SPI_MAX_FIFO_SIZE 32
#define MTK_SPI_PACKET_SIZE 1024

/**
  * @}
  */

/** @ingroup IP_group_spi_internal_struct
 * @brief Struct keeping spi driver data
 */
struct mtk_spi_compatible {
	/** some IC need padsel */
	bool need_pad_sel;
};

/** @ingroup IP_group_spi_internal_struct
 * @brief Struct keeping spi driver data
 */
struct mtk_spi {
	/** to get spi bus base address  */
	void __iomem *base;
	/** used for get or put spi status  */
	u32 state;
	/** used for get spi padsel number */
	int pad_num;
	/** used for get clk pointer */
	struct clk *parent_clk, *sel_clk, *spi_clk;
	/** used for save current spi_transfer pointer */
	struct spi_transfer *cur_transfer;
	/** used for save current spi_transfer length */
	u32 xfer_len;
	/** used for get tx/rx scatterlist pointer */
	struct scatterlist *tx_sgl, *rx_sgl;
	/** used for get tx/rx scatterlist length */
	u32 tx_sgl_len, rx_sgl_len;
	/** used for IC compatible */
	const struct mtk_spi_compatible *dev_comp;
};

/** @ingroup IP_group_spi_internal_struct
 * @brief SPI compatible structure.\n
 * This structure is used to register itself.
 */
static const struct mtk_spi_compatible mtk_common_compat;

/** @ingroup IP_group_spi_internal_struct
 * @brief A piece of default chip info unless the platform supplies it.
 * This structure is used to register itself.
 */
static const struct mtk_chip_config mtk_default_chip_info = {
	.rx_mlsb = 1,
	.tx_mlsb = 1,
	.get_tick_dly = 0,
};

/** @ingroup IP_group_spi_internal_struct
 * @brief SPI Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id mtk_spi_of_match[] = {
	{.compatible = "mediatek,mt3612-spi",
	 .data = (void *)&mtk_common_compat,
	},
	{}
};

MODULE_DEVICE_TABLE(of, mtk_spi_of_match);

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *     soft-reset spi HW.
 * @param[in]
 *     mdata: mtk spi driver data pointer.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_spi_reset(struct mtk_spi *mdata)
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
 *	   master: interface pointer to SPI master controller.
 * @param[in]
 *	   msg: one multi-segment SPI transaction pointer.
 * @return
 *	   0, always successed.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_spi_prepare_message(struct spi_master *master,
				   struct spi_message *msg)
{
	u16 cpha, cpol;
	u32 reg_val;
	struct spi_device *spi = msg->spi;
	struct mtk_chip_config *chip_config = spi->controller_data;
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	cpha = spi->mode & SPI_CPHA ? 1 : 0;
	cpol = spi->mode & SPI_CPOL ? 1 : 0;

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
	if (chip_config->tx_mlsb)
		reg_val |= SPI_CMD_TXMSBF;
	else
		reg_val &= ~SPI_CMD_TXMSBF;
	if (chip_config->rx_mlsb)
		reg_val |= SPI_CMD_RXMSBF;
	else
		reg_val &= ~SPI_CMD_RXMSBF;

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
	reg_val |= chip_config->get_tick_dly << SPI_CFG1_GET_TICK_DLY_OFFSET;
	writel(reg_val, mdata->base + SPI_CFG1_REG);

	return 0;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	triggle spi hw to pull up/down cs.
 * @param[in]
 *	spi: spi device pointer.
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
static void mtk_spi_set_cs(struct spi_device *spi, bool enable)
{
	u32 reg_val;
	struct mtk_spi *mdata = spi_master_get_devdata(spi->master);

	reg_val = readl(mdata->base + SPI_CMD_REG);
	if (!enable) {
		reg_val |= SPI_CMD_PAUSE_EN;
		writel(reg_val, mdata->base + SPI_CMD_REG);
	} else {
		reg_val &= ~SPI_CMD_PAUSE_EN;
		writel(reg_val, mdata->base + SPI_CMD_REG);
		mdata->state = MTK_SPI_IDLE;
		mtk_spi_reset(mdata);
	}
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   prepare hw configure setting for spi_transfer.
 *	   for example, set clk,cs time.
 * @param[in]
 *	   master: interface pointer to SPI master controller.
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
static void mtk_spi_prepare_transfer(struct spi_master *master,
				     struct spi_transfer *xfer)
{
	u32 spi_clk_hz, div, sck_time, cs_time, reg_val = 0;
	struct mtk_spi *mdata = spi_master_get_devdata(master);

#if defined(CONFIG_COMMON_CLK_MT3612)
	spi_clk_hz = clk_get_rate(mdata->spi_clk);
#else
	spi_clk_hz = 6000000;
#endif
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
 *	   master: interface pointer to SPI master controller.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_spi_setup_packet(struct spi_master *master)
{
	u32 packet_size, packet_loop, reg_val;
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	packet_size = min_t(u32, mdata->xfer_len, MTK_SPI_PACKET_SIZE);
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
 *	   master: interface pointer to SPI master controller.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_spi_enable_transfer(struct spi_master *master)
{
	u32 cmd;
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	cmd = readl(mdata->base + SPI_CMD_REG);
	if (mdata->state == MTK_SPI_IDLE)
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
static int mtk_spi_get_mult_delta(u32 xfer_len)
{
	u32 mult_delta;

	if (xfer_len > MTK_SPI_PACKET_SIZE)
		mult_delta = xfer_len % MTK_SPI_PACKET_SIZE;
	else
		mult_delta = 0;

	return mult_delta;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   update transfer length.
 * @param[in]
 *	   master: interface pointer to SPI master controller.
 * @return
 *	   none.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_spi_update_mdata_len(struct spi_master *master)
{
	int mult_delta;
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	if (mdata->tx_sgl_len && mdata->rx_sgl_len) {
		if (mdata->tx_sgl_len > mdata->rx_sgl_len) {
			mult_delta = mtk_spi_get_mult_delta(mdata->rx_sgl_len);
			mdata->xfer_len = mdata->rx_sgl_len - mult_delta;
			mdata->rx_sgl_len = mult_delta;
			mdata->tx_sgl_len -= mdata->xfer_len;
		} else {
			mult_delta = mtk_spi_get_mult_delta(mdata->tx_sgl_len);
			mdata->xfer_len = mdata->tx_sgl_len - mult_delta;
			mdata->tx_sgl_len = mult_delta;
			mdata->rx_sgl_len -= mdata->xfer_len;
		}
	} else if (mdata->tx_sgl_len) {
		mult_delta = mtk_spi_get_mult_delta(mdata->tx_sgl_len);
		mdata->xfer_len = mdata->tx_sgl_len - mult_delta;
		mdata->tx_sgl_len = mult_delta;
	} else if (mdata->rx_sgl_len) {
		mult_delta = mtk_spi_get_mult_delta(mdata->rx_sgl_len);
		mdata->xfer_len = mdata->rx_sgl_len - mult_delta;
		mdata->rx_sgl_len = mult_delta;
	}
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   write tx/rx dma address to tx/rx dma pointer.
 * @param[in]
 *	   master: interface pointer to SPI master controller.
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
static void mtk_spi_setup_dma_addr(struct spi_master *master,
				   struct spi_transfer *xfer)
{
	struct mtk_spi *mdata = spi_master_get_devdata(master);

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
 *	   master: interface pointer to SPI master controller.
 * @param[in]
 *	   spi: spi device pointer.
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
static int mtk_spi_fifo_transfer(struct spi_master *master,
				 struct spi_device *spi,
				 struct spi_transfer *xfer)
{
	int cnt, remainder;
	u32 reg_val;
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	mdata->cur_transfer = xfer;
	mdata->xfer_len = xfer->len;
	mtk_spi_prepare_transfer(master, xfer);
	mtk_spi_setup_packet(master);

	cnt = xfer->len / 4;
	iowrite32_rep(mdata->base + SPI_TX_DATA_REG, xfer->tx_buf, cnt);

	remainder = xfer->len % 4;
	if (remainder > 0) {
		reg_val = 0;
		memcpy(&reg_val, xfer->tx_buf + (cnt * 4), remainder);
		writel(reg_val, mdata->base + SPI_TX_DATA_REG);
	}

	mtk_spi_enable_transfer(master);

	return 1;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   config spi dma mode transfer and triggle transfer.
 * @param[in]
 *	   master: interface pointer to SPI master controller.
 * @param[in]
 *	   spi: spi device pointer.
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
static int mtk_spi_dma_transfer(struct spi_master *master,
				struct spi_device *spi,
				struct spi_transfer *xfer)
{
	int cmd;
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	mdata->tx_sgl = NULL;
	mdata->rx_sgl = NULL;
	mdata->tx_sgl_len = 0;
	mdata->rx_sgl_len = 0;
	mdata->cur_transfer = xfer;

	mtk_spi_prepare_transfer(master, xfer);

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

	mtk_spi_update_mdata_len(master);
	mtk_spi_setup_packet(master);
	mtk_spi_setup_dma_addr(master, xfer);
	mtk_spi_enable_transfer(master);

	return 1;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   set spi HW to do one transfer,\n
 *	   this function is registered to spi framework callback.
 * @param[in]
 *	   master: interface pointer to SPI master controller.
 * @param[in]
 *	   spi: spi device pointer.
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
static int mtk_spi_transfer_one(struct spi_master *master,
				struct spi_device *spi,
				struct spi_transfer *xfer)
{
	if (master->can_dma(master, spi, xfer))
		return mtk_spi_dma_transfer(master, spi, xfer);
	else
		return mtk_spi_fifo_transfer(master, spi, xfer);
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   according to one transfer length to use dma or fifo mode.\n
 *	   this function is registered to spi framework callback.
 * @param[in]
 *	   master: interface pointer to SPI master controller.
 * @param[in]
 *	   spi: spi device pointer.
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
static bool mtk_spi_can_dma(struct spi_master *master,
			    struct spi_device *spi, struct spi_transfer *xfer)
{
	return xfer->len > MTK_SPI_MAX_FIFO_SIZE;
}

/**
 * @ingroup IP_group_spi_internal_function
 * @par Description
 *	   setup mtk spi data.\n
 *	   this function is registered to spi framework callback.
 * @param[in]
 *	   spi: spi device pointer.
 * @return
 *	   0, always success.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_spi_setup(struct spi_device *spi)
{
	struct mtk_spi *mdata = spi_master_get_devdata(spi->master);

	if (!spi->controller_data)
		spi->controller_data = (void *)&mtk_default_chip_info;

	if (mdata->dev_comp->need_pad_sel && gpio_is_valid(spi->cs_gpio))
		gpio_direction_output(spi->cs_gpio, !(spi->mode & SPI_CS_HIGH));

	return 0;
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
static irqreturn_t mtk_spi_interrupt(int irq, void *dev_id)
{
	u32 cmd, reg_val, cnt, remainder;
	struct spi_master *master = dev_id;
	struct mtk_spi *mdata = spi_master_get_devdata(master);
	struct spi_transfer *trans = mdata->cur_transfer;

	reg_val = readl(mdata->base + SPI_STATUS0_REG);
	if (reg_val & MTK_SPI_PAUSE_INT_STATUS)
		mdata->state = MTK_SPI_PAUSED;
	else
		mdata->state = MTK_SPI_IDLE;

	if (!master->can_dma(master, master->cur_msg->spi, trans)) {
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
		spi_finalize_current_transfer(master);
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

		spi_finalize_current_transfer(master);
		return IRQ_HANDLED;
	}

	mtk_spi_update_mdata_len(master);
	mtk_spi_setup_packet(master);
	mtk_spi_setup_dma_addr(master, trans);
	mtk_spi_enable_transfer(master);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_spi_internal_function
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
static int mtk_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct mtk_spi *mdata;
	const struct of_device_id *of_id;
	struct resource *res;
	int i, irq, ret;

	master = spi_alloc_master(&pdev->dev, sizeof(*mdata));

	if (!master) {
		dev_err(&pdev->dev, "failed to alloc spi master\n");
		return -ENOMEM;
	}

#ifdef CONFIG_SPI_MT36XX_ENABLE_AUTO_RUNTIME_PM
	master->auto_runtime_pm = true;
#else
	master->auto_runtime_pm = false;
#endif
	master->dev.of_node = pdev->dev.of_node;
	master->mode_bits = SPI_CPOL | SPI_CPHA;

	master->set_cs = mtk_spi_set_cs;
	master->prepare_message = mtk_spi_prepare_message;
	master->transfer_one = mtk_spi_transfer_one;
	master->can_dma = mtk_spi_can_dma;
	master->setup = mtk_spi_setup;
	master->rt = 1;

	of_id = of_match_node(mtk_spi_of_match, pdev->dev.of_node);
	if (!of_id) {
		dev_err(&pdev->dev, "failed to probe of_node\n");
		ret = -EINVAL;
		goto err_put_master;
	}

	mdata = spi_master_get_devdata(master);
	mdata->dev_comp = of_id->data;
	master->flags = SPI_MASTER_MUST_TX;

	if (mdata->dev_comp->need_pad_sel) {
		mdata->pad_num = of_property_count_u32_elems(pdev->dev.of_node,
							     "mediatek,pad-select");
		if (mdata->pad_num < 0) {
			dev_err(&pdev->dev,
				"No 'mediatek,pad-select' property\n");
			ret = -EINVAL;
			goto err_put_master;
		}
	}

	platform_set_drvdata(pdev, master);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "failed to determine base address\n");
		goto err_put_master;
	}

	mdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mdata->base)) {
		ret = PTR_ERR(mdata->base);
		goto err_put_master;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed to get irq (%d)\n", irq);
		ret = irq;
		goto err_put_master;
	}

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	ret = devm_request_irq(&pdev->dev, irq, mtk_spi_interrupt,
			       IRQF_TRIGGER_NONE, dev_name(&pdev->dev), master);
	if (ret) {
		dev_err(&pdev->dev, "failed to register irq (%d)\n", ret);
		goto err_put_master;
	}
#if defined(CONFIG_COMMON_CLK_MT3612)
	mdata->parent_clk = devm_clk_get(&pdev->dev, "parent-clk");
	if (IS_ERR(mdata->parent_clk)) {
		ret = PTR_ERR(mdata->parent_clk);
		dev_err(&pdev->dev, "failed to get parent-clk: %d\n", ret);
		goto err_put_master;
	}

	mdata->sel_clk = devm_clk_get(&pdev->dev, "sel-clk");
	if (IS_ERR(mdata->sel_clk)) {
		ret = PTR_ERR(mdata->sel_clk);
		dev_err(&pdev->dev, "failed to get sel-clk: %d\n", ret);
		goto err_put_master;
	}

	mdata->spi_clk = devm_clk_get(&pdev->dev, "spi-clk");
	if (IS_ERR(mdata->spi_clk)) {
		ret = PTR_ERR(mdata->spi_clk);
		dev_err(&pdev->dev, "failed to get spi-clk: %d\n", ret);
		goto err_put_master;
	}

	ret = clk_prepare_enable(mdata->spi_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable spi_clk (%d)\n", ret);
		goto err_put_master;
	}

	ret = clk_set_parent(mdata->sel_clk, mdata->parent_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to clk_set_parent (%d)\n", ret);
		clk_disable_unprepare(mdata->spi_clk);
		goto err_put_master;
	}
#endif

	pm_runtime_enable(&pdev->dev);

	ret = devm_spi_register_master(&pdev->dev, master);
	if (ret) {
		dev_err(&pdev->dev, "failed to register master (%d)\n", ret);
		goto err_disable_runtime_pm;
	}

	if (mdata->dev_comp->need_pad_sel) {
		if (mdata->pad_num != master->num_chipselect) {
			dev_err(&pdev->dev,
				"pad_num does not match num_chipselect(%d != %d)\n",
				mdata->pad_num, master->num_chipselect);
			ret = -EINVAL;
			goto err_disable_runtime_pm;
		}

		if (!master->cs_gpios && master->num_chipselect > 1) {
			dev_err(&pdev->dev,
				"cs_gpios not specified and num_chipselect > 1\n");
			ret = -EINVAL;
			goto err_disable_runtime_pm;
		}

		if (master->cs_gpios) {
			for (i = 0; i < master->num_chipselect; i++) {
				ret = devm_gpio_request(&pdev->dev,
							master->cs_gpios[i],
							dev_name(&pdev->dev));
				if (ret) {
					dev_err(&pdev->dev,
						"can't get CS GPIO %i\n", i);
					goto err_disable_runtime_pm;
				}
			}
		}
	}

#ifdef CONFIG_SPI_MT36XX_ENABLE_AUTO_RUNTIME_PM
#if defined(CONFIG_COMMON_CLK_MT3612)
	clk_disable_unprepare(mdata->spi_clk);
#endif
#endif

	return 0;

err_disable_runtime_pm:
	pm_runtime_disable(&pdev->dev);
err_put_master:
	spi_master_put(master);

	return ret;
}

/** @ingroup IP_group_spi_internal_function
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
static int mtk_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	pm_runtime_disable(&pdev->dev);

	mtk_spi_reset(mdata);
	spi_master_put(master);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
/** @ingroup IP_group_spi_internal_function
 * @par Description
 *	   mtk spi driver suspend to register PM framework.
 * @param[in]
 *	   dev: device pointer.
 * @return
 *	   0, suspend successed.
 *	   error code from spi_master_suspend().
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   1. If spi_master_suspend() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_spi_suspend(struct device *dev)
{
	int ret;
	struct spi_master *master = dev_get_drvdata(dev);

#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_spi *mdata = spi_master_get_devdata(master);
#endif
	ret = spi_master_suspend(master);
	if (ret)
		return ret;

#if defined(CONFIG_COMMON_CLK_MT3612)
	if (!pm_runtime_suspended(dev))
		clk_disable_unprepare(mdata->spi_clk);
#endif

	return ret;
}

/** @ingroup IP_group_spi_internal_function
 * @par Description
 *	   mtk spi driver resume to register PM framework.
 * @param[in]
 *	   dev: device pointer.
 * @return
 *	   0, resume successed.
 *	   error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   1. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_spi_resume(struct device *dev)
{
	int ret = 0;
	struct spi_master *master = dev_get_drvdata(dev);

#if defined(CONFIG_COMMON_CLK_MT3612)
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	if (!pm_runtime_suspended(dev)) {
		ret = clk_prepare_enable(mdata->spi_clk);
		if (ret < 0) {
			dev_err(dev, "failed to enable spi_clk (%d)\n", ret);
			return ret;
		}
	}
#endif

	ret = spi_master_resume(master);

#if defined(CONFIG_COMMON_CLK_MT3612)
	if (ret < 0)
		clk_disable_unprepare(mdata->spi_clk);
#endif

	return ret;
}
#endif /* CONFIG_PM_SLEEP */

#ifdef CONFIG_PM
/** @ingroup IP_group_spi_internal_function
 * @par Description
 *	   mtk spi driver runtime suspend to register runtime PM framework.
 * @param[in]
 *	   dev: device pointer.
 * @return
 *	   0, runtime suspend successed.
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_spi_runtime_suspend(struct device *dev)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	struct spi_master *master = dev_get_drvdata(dev);
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	clk_disable_unprepare(mdata->spi_clk);
#endif

	return 0;
}
/** @ingroup IP_group_spi_internal_function
 * @par Description
 *	   mtk spi driver runtime resume to register runtime PM framework.
 * @param[in]
 *	   dev: device pointer.
 * @return
 *	   0, runtime suspend successed.
 *	   error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *	   none.
 * @par Error case and Error handling
 *	   1. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_spi_runtime_resume(struct device *dev)
{
	int ret = 0;

#if defined(CONFIG_COMMON_CLK_MT3612)
	struct spi_master *master = dev_get_drvdata(dev);
	struct mtk_spi *mdata = spi_master_get_devdata(master);

	ret = clk_prepare_enable(mdata->spi_clk);
	if (ret < 0) {
		dev_err(dev, "failed to enable spi_clk (%d)\n", ret);
		return ret;
	}
#endif

	return ret;
}
#endif /* CONFIG_PM */

/** @ingroup IP_group_spi_internal_struct
 * @brief SPI device PM callbacks structure.\n
 * This structure is used to register itself.
 */
static const struct dev_pm_ops mtk_spi_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_spi_suspend, mtk_spi_resume)
	SET_RUNTIME_PM_OPS(mtk_spi_runtime_suspend,
	mtk_spi_runtime_resume, NULL)
};

/** @ingroup IP_group_spi_internal_struct
 * @brief SPI platform driver structure.\n
 * This structure is used to register itself.
 */
static struct platform_driver mtk_spi_driver = {
	.probe = mtk_spi_probe,
	.remove = mtk_spi_remove,
	.driver = {
		   .name = "mtk-spi",
		   .pm = &mtk_spi_pm,
		   .of_match_table = mtk_spi_of_match,
		   },
};

module_platform_driver(mtk_spi_driver);

MODULE_DESCRIPTION("MTK SPI Controller driver");
MODULE_AUTHOR("Leilk Liu <leilk.liu@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:mtk-spi");
