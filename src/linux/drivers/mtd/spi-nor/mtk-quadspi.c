/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Bayi Cheng <bayi.cheng@mediatek.com>
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
 * @file mtk-quadspi.c
 * Serial NOR Flash Controller driver for MTK's IP, which is responsible\n
 * for access Serial NOR Flash.\n
 * It supports basic operations of read/write/erase.\n
 */

/**
 * @defgroup IP_group_snfc SERIAL_NOR_FLASH_CONTROLLER
 *     There provides basic functions of read/write/earse spi-nor flash.
 *
 *     @{
 *         @defgroup IP_group_snfc_external EXTERNAL
 *             The external API document for Serial NOR Flash Controller. \n
 *
 *             @{
 *                 @defgroup IP_group_snfc_external_function 1.function
 *                     none. Native Linux Driver.
 *                 @defgroup IP_group_snfc_external_struct 2.structure
 *                     none. Native Linux Driver.
 *                 @defgroup IP_group_snfc_external_typedef 3.typedef
 *                     none. Native Linux Driver.
 *                 @defgroup IP_group_snfc_external_enum 4.enumeration
 *                     none. Native Linux Driver.
 *                 @defgroup IP_group_snfc_external_def 5.define
 *                     none. Native Linux Driver.
 *             @}
 *
 *         @defgroup IP_group_snfc_internal INTERNAL
 *             The internal API document for Serial NOR Flash Controller. \n
 *
 *             @{
 *                 @defgroup IP_group_snfc_internal_function 1.function
 *                     Internal function used for Serial NOR Flash Controller.
 *                 @defgroup IP_group_snfc_internal_struct 2.structure
 *                     Internal structure used for Serial NOR Flash Controller.
 *                 @defgroup IP_group_snfc_internal_typedef 3.typedef
 *                     none. No typedef used.
 *                 @defgroup IP_group_snfc_internal_enum 4.enumeration
 *                     none. No enumeration used.
 *                 @defgroup IP_group_snfc_internal_def 5.define
 *                     Internal define used for Serial NOR Flash Controller.
 *             @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spi-nor.h>
#include "mtk-partition.h"

/** @ingroup IP_group_snfc_internal_def
 * @brief Serial NOR Flash Controller registers define
 * @{
 */
#define MTK_NOR_CMD_REG			0x00
#define MTK_NOR_CNT_REG			0x04
#define MTK_NOR_RDSR_REG		0x08
#define MTK_NOR_RDATA_REG		0x0c
#define MTK_NOR_RADR0_REG		0x10
#define MTK_NOR_RADR1_REG		0x14
#define MTK_NOR_RADR2_REG		0x18
#define MTK_NOR_WDATA_REG		0x1c
#define MTK_NOR_PRGDATA0_REG		0x20
#define MTK_NOR_PRGDATA1_REG		0x24
#define MTK_NOR_PRGDATA2_REG		0x28
#define MTK_NOR_PRGDATA3_REG		0x2c
#define MTK_NOR_PRGDATA4_REG		0x30
#define MTK_NOR_PRGDATA5_REG		0x34
#define MTK_NOR_SHREG0_REG		0x38
#define MTK_NOR_SHREG1_REG		0x3c
#define MTK_NOR_SHREG2_REG		0x40
#define MTK_NOR_SHREG3_REG		0x44
#define MTK_NOR_SHREG4_REG		0x48
#define MTK_NOR_SHREG5_REG		0x4c
#define MTK_NOR_SHREG6_REG		0x50
#define MTK_NOR_SHREG7_REG		0x54
#define MTK_NOR_SHREG8_REG		0x58
#define MTK_NOR_SHREG9_REG		0x5c
#define MTK_NOR_CFG1_REG		0x60
#define MTK_NOR_CFG2_REG		0x64
#define MTK_NOR_FLHCFG_REG		0x84
#define MTK_NOR_TIME_REG		0x94
#define MTK_NOR_PP_DATA_REG		0x98
#define MTK_NOR_DELSEL0_REG		0xa0
#define MTK_NOR_DELSEL1_REG		0xa4
#define MTK_NOR_INTRSTUS_REG		0xa8
#define MTK_NOR_INTREN_REG		0xac
#define MTK_NOR_CHKSUM_REG		0xbc
#define MTK_NOR_CMD2_REG		0xc0
#define MTK_NOR_WRPROT_REG		0xc4
#define MTK_NOR_RADR3_REG		0xc8
#define MTK_NOR_DUAL_REG		0xcc
#define MTK_NOR_DELSEL2_REG		0xd0
#define MTK_NOR_DELSEL3_REG		0xd4
#define MTK_NOR_DELSEL4_REG		0xd8
#define MTK_NOR_DUMMY			0x13c

/* commands for mtk nor controller */
#define MTK_NOR_READ_CMD		0x0
#define MTK_NOR_RDSR_CMD		0x2
#define MTK_NOR_PRG_CMD			0x4
#define MTK_NOR_WR_CMD			0x10
#define MTK_NOR_PIO_WR_CMD		0x90
#define MTK_NOR_WRSR_CMD		0x20
#define MTK_NOR_PIO_READ_CMD		0x81
#define MTK_NOR_WR_BUF_ENABLE		0x1
#define MTK_NOR_WR_BUF_DISABLE		0x0
#define MTK_NOR_ENABLE_SF_CMD		0x30
#define MTK_NOR_DUAD_ADDR_EN		0x8
#define MTK_NOR_QUAD_READ_EN		0x4
#define MTK_NOR_DUAL_ADDR_EN		0x2
#define MTK_NOR_DUAL_READ_EN		0x1
#define MTK_NOR_DUAL_DISABLE		0x0
#define MTK_NOR_FAST_READ		0x1

#define SFLASH_WRBUF_SIZE		128

/* Can shift up to 48 bits (6 bytes) of TX/RX */
#define MTK_NOR_MAX_RX_TX_SHIFT		6
/* can shift up to 56 bits (7 bytes) transfer by MTK_NOR_PRG_CMD */
#define MTK_NOR_MAX_SHIFT		7
/* nor controller 4-byte address mode enable bit */
#define MTK_NOR_4B_ADDR_EN		BIT(4)

/* Helpers for accessing the program data / shift data registers */
#define MTK_NOR_PRG_REG(n)		(MTK_NOR_PRGDATA0_REG + 4 * (n))
#define MTK_NOR_SHREG(n)		(MTK_NOR_SHREG0_REG + 4 * (n))

/* Dummy cycle for SPANSION sflash */
#define DUMMY_CFG_EN		0x80
#define DUMMY_NUM		0x7F

/**
 * @}
 */

/** @ingroup IP_group_snfc_internal_struct
 * @brief Serial NOR Flash Controller driver resource structure.
 */
struct mt3611_nor {
	struct spi_nor nor;
	struct device *dev;
	void __iomem *base;	/* nor flash base address */
	struct clk *spi_clk;
	struct clk *nor_clk;
};

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Set Serial NOR Flash Controller's read mode.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_nor_set_read_mode(struct mt3611_nor *mt3611_nor)
{
	struct spi_nor *nor = &mt3611_nor->nor;
	unsigned int dummy = 0;

	switch (nor->flash_read) {
	case SPI_NOR_FAST:
		writeb(nor->read_opcode, mt3611_nor->base +
		       MTK_NOR_PRGDATA3_REG);
		writeb(MTK_NOR_FAST_READ, mt3611_nor->base +
		       MTK_NOR_CFG1_REG);
		break;
	case SPI_NOR_DUAL:
		writeb(nor->read_opcode, mt3611_nor->base +
		       MTK_NOR_PRGDATA3_REG);
		writeb(MTK_NOR_DUAL_READ_EN, mt3611_nor->base +
		       MTK_NOR_DUAL_REG);
		if (nor->flags & SNOR_F_USE_DUMMY) {
			dummy = nor->read_dummy & DUMMY_NUM;
			dummy |= DUMMY_CFG_EN;
			writeb(dummy, mt3611_nor->base + MTK_NOR_DUMMY);
		}
		break;
	case SPI_NOR_QUAD:
		writeb(nor->read_opcode, mt3611_nor->base +
		       MTK_NOR_PRGDATA4_REG);
		if (nor->read_opcode == SPINOR_OP_READ_1_1_4)
			writeb(MTK_NOR_QUAD_READ_EN, mt3611_nor->base +
			       MTK_NOR_DUAL_REG);
		else
			writeb(MTK_NOR_QUAD_READ_EN | MTK_NOR_DUAD_ADDR_EN,
			       mt3611_nor->base + MTK_NOR_DUAL_REG);
		if (nor->flags & SNOR_F_USE_DUMMY) {
			dummy = nor->read_dummy & DUMMY_NUM;
			dummy |= DUMMY_CFG_EN;
			writeb(dummy, mt3611_nor->base + MTK_NOR_DUMMY);
		}
		break;
	default:
		if (nor->flags & SNOR_F_USE_DUMMY) {
			dummy &= ~DUMMY_CFG_EN;
			writeb(dummy, mt3611_nor->base + MTK_NOR_DUMMY);
		}
		writeb(MTK_NOR_DUAL_DISABLE, mt3611_nor->base +
		       MTK_NOR_DUAL_REG);
		break;
	}
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Do operation that command stands for.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @param[in]
 *     cmdval: command code defined in SPI NOR flash specification.
 * @return
 *     0, execute command done.\n
 *     error code from readl_poll_timeout().
 * @par Boundary case and limitation
 *     Parameter, cmdval should be the value defined in MTK_NOR_CMD_REG.
 * @par Error case and Error handling
 *     If readl_poll_timeout() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_execute_cmd(struct mt3611_nor *mt3611_nor, u8 cmdval)
{
	int reg;
	u8 val = cmdval & 0x1f;

	writeb(cmdval, mt3611_nor->base + MTK_NOR_CMD_REG);
	return readl_poll_timeout(mt3611_nor->base + MTK_NOR_CMD_REG, reg,
				  !(reg & val), 100, 10000);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Send command to NOR flash and get feedback from NOR flash.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @param[in]
 *     op: command code.
 * @param[in]
 *     tx: arguments with command code.
 * @param[in]
 *     txlen: the length of argument tx[]..
 * @param[out]
 *     rx: feedback with command code from NOR flash.
 * @param[in]
 *     rxlen: the length of feedback rx[].
 * @return
 *     0, do communication with NOR flash successfully.\n
 *     -EINVAL, invalid parameter.\n
 *     error code from mt3611_nor_execute_cmd().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     1. If parameter txlen+rxlen+1 is larger than max shift count,\n
 *     return -EINVAL.\n
 *     2. If mt3611_nor_execute_cmd() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_do_tx_rx(struct mt3611_nor *mt3611_nor, u8 op,
			       u8 *tx, int txlen, u8 *rx, int rxlen)
{
	int len = 1 + txlen + rxlen;
	int i, ret, idx;

	if (len > MTK_NOR_MAX_SHIFT)
		return -EINVAL;

	writeb(len * 8, mt3611_nor->base + MTK_NOR_CNT_REG);

	/* start at PRGDATA5, go down to PRGDATA0 */
	idx = MTK_NOR_MAX_RX_TX_SHIFT - 1;

	/* opcode */
	writeb(op, mt3611_nor->base + MTK_NOR_PRG_REG(idx));
	idx--;

	/* program TX data */
	for (i = 0; i < txlen; i++, idx--)
		writeb(tx[i], mt3611_nor->base + MTK_NOR_PRG_REG(idx));

	/* clear out rest of TX registers */
	while (idx >= 0) {
		writeb(0, mt3611_nor->base + MTK_NOR_PRG_REG(idx));
		idx--;
	}

	ret = mt3611_nor_execute_cmd(mt3611_nor, MTK_NOR_PRG_CMD);
	if (ret)
		return ret;

	/* restart at first RX byte */
	idx = rxlen - 1;

	/* read out RX data */
	for (i = 0; i < rxlen; i++, idx--)
		rx[i] = readb(mt3611_nor->base + MTK_NOR_SHREG(idx));

	return 0;
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Set value of NOR flash's status register.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @param[in]
 *     sr: the value to set into NOR flash's status register.
 * @return
 *     0, set NOR flash's status register's value done.\n
 *     error code from mt3611_nor_execute_cmd().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     If mt3611_nor_execute_cmd() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_wr_sr(struct mt3611_nor *mt3611_nor, u8 sr)
{
	writeb(sr, mt3611_nor->base + MTK_NOR_PRGDATA5_REG);
	writeb(8, mt3611_nor->base + MTK_NOR_CNT_REG);
	return mt3611_nor_execute_cmd(mt3611_nor, MTK_NOR_WRSR_CMD);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Enable write buffer for faster writing NOR flash.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @return
 *     error code from readb_poll_timeout().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     If readb_poll_timeout() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_write_buffer_enable(struct mt3611_nor *mt3611_nor)
{
	u8 reg;

	/* the bit0 of MTK_NOR_CFG2_REG is pre-fetch buffer
	 * 0: pre-fetch buffer use for read
	 * 1: pre-fetch buffer use for page program
	 */
	writel(MTK_NOR_WR_BUF_ENABLE, mt3611_nor->base + MTK_NOR_CFG2_REG);
	return readb_poll_timeout(mt3611_nor->base + MTK_NOR_CFG2_REG, reg,
				  0x01 == (reg & 0x01), 100, 10000);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Disable write buffer.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @return
 *     error code from readb_poll_timeout().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     If readb_poll_timeout() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_write_buffer_disable(struct mt3611_nor *mt3611_nor)
{
	u8 reg;

	writel(MTK_NOR_WR_BUF_DISABLE, mt3611_nor->base + MTK_NOR_CFG2_REG);
	return readb_poll_timeout(mt3611_nor->base + MTK_NOR_CFG2_REG, reg,
				  MTK_NOR_WR_BUF_DISABLE == (reg & 0x1), 100,
				  10000);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Set nor controller's address mode.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_nor_set_addr_width(struct mt3611_nor *mt3611_nor)
{
	u8 val;
	struct spi_nor *nor = &mt3611_nor->nor;

	val = readb(mt3611_nor->base + MTK_NOR_DUAL_REG);

	switch (nor->addr_width) {
	case 3:
		val &= ~MTK_NOR_4B_ADDR_EN;
		break;
	case 4:
		val |= MTK_NOR_4B_ADDR_EN;
		break;
	default:
		dev_warn(mt3611_nor->dev, "Unexpected address width %u.\n",
			 nor->addr_width);
		break;
	}

	writeb(val, mt3611_nor->base + MTK_NOR_DUAL_REG);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Set address for access operation.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @param[in]
 *     addr: the address to access.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_nor_set_addr(struct mt3611_nor *mt3611_nor, u32 addr)
{
	int i;

	mt3611_nor_set_addr_width(mt3611_nor);

	for (i = 0; i < 3; i++) {
		writeb(addr & 0xff,
			mt3611_nor->base + MTK_NOR_RADR0_REG + i * 4);
		addr >>= 8;
	}
	/* Last register is non-contiguous */
	writeb(addr & 0xff, mt3611_nor->base + MTK_NOR_RADR3_REG);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Read data from NOR flash.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @param[in]
 *     from: offset on flash where to read.
 * @param[in]
 *     length: the length of data to read.
 * @param[out]
 *     retlen: the length of buffer that read from NOR.
 * @param[out]
 *     buffer: the data read from NOR.
 * @return
 *     0, read data from NOR successfully. \n
 *     error code from mt3611_nor_execute_cmd().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     If mt3611_nor_execute_cmd() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_read(struct spi_nor *nor, loff_t from, size_t length,
			   size_t *retlen, u_char *buffer)
{
	int i, ret;
	int addr = (int)from;
	u8 *buf = (u8 *)buffer;
	struct mt3611_nor *mt3611_nor = nor->priv;

	/* set mode for fast read mode ,dual mode or quad mode */
	mt3611_nor_set_read_mode(mt3611_nor);
	mt3611_nor_set_addr(mt3611_nor, addr);

	for (i = 0; i < length; i++, (*retlen)++) {
		ret = mt3611_nor_execute_cmd(mt3611_nor, MTK_NOR_PIO_READ_CMD);
		if (ret < 0)
			return ret;
		buf[i] = readb(mt3611_nor->base + MTK_NOR_RDATA_REG);
	}
	return 0;
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Write NOR flash by byte mode.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @param[in]
 *     addr: address on flash where to write.
 * @param[in]
 *     length: the length of data buffer to write.
 * @param[in]
 *     data: the data to be written.
 * @return
 *     0, write data to NOR successfully. \n
 *     error code from mt3611_nor_execute_cmd().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     If mt3611_nor_execute_cmd() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_write_single_byte(struct mt3611_nor *mt3611_nor,
					int addr, int length, u8 *data)
{
	int i, ret;

	mt3611_nor_set_addr(mt3611_nor, addr);

	for (i = 0; i < length; i++) {
		writeb(*data++, mt3611_nor->base + MTK_NOR_WDATA_REG);
		ret = mt3611_nor_execute_cmd(mt3611_nor, MTK_NOR_PIO_WR_CMD);
		if (ret < 0)
			return ret;
	}
	return 0;
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Write data to NOR flash by page-program mode.
 * @param[in]
 *     mt3611_nor: module resource structure for getting base register address.
 * @param[in]
 *     addr: address on flash where to write.
 * @param[in]
 *     length: the length of data buffer that to write.
 * @param[in]
 *     data: the data to be written.
 * @return
 *     0, write data to NOR successfully. \n
 *     error code from mt3611_nor_execute_cmd().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     If mt3611_nor_execute_cmd() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_write_buffer(struct mt3611_nor *mt3611_nor, int addr,
				   const u8 *buf)
{
	int i, bufidx, data;

	mt3611_nor_set_addr(mt3611_nor, addr);

	bufidx = 0;
	for (i = 0; i < SFLASH_WRBUF_SIZE; i += 4) {
		data = buf[bufidx + 3]<<24 | buf[bufidx + 2]<<16 |
		       buf[bufidx + 1]<<8 | buf[bufidx];
		bufidx += 4;
		writel(data, mt3611_nor->base + MTK_NOR_PP_DATA_REG);
	}
	return mt3611_nor_execute_cmd(mt3611_nor, MTK_NOR_WR_CMD);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Write data to NOR flash.
 * @param[in]
 *     nor: spi_nor device for getting private data.
 * @param[in]
 *     to: address on flash where to write.
 * @param[in]
 *     len: the length of data buffer that to write.
 * @param[out]
 *     retlen: the length of data written to nor.
 * @param[in]
 *     buf: the data to be written.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt3611_nor_write(struct spi_nor *nor, loff_t to, size_t len,
			     size_t *retlen, const u_char *buf)
{
	int ret;
	struct mt3611_nor *mt3611_nor = nor->priv;

	ret = mt3611_nor_write_buffer_enable(mt3611_nor);
	if (ret < 0)
		dev_warn(mt3611_nor->dev, "write buffer enable failed!\n");

	while (len >= SFLASH_WRBUF_SIZE) {
		ret = mt3611_nor_write_buffer(mt3611_nor, to, buf);
		if (ret < 0)
			dev_err(mt3611_nor->dev, "write buffer failed!\n");
		len -= SFLASH_WRBUF_SIZE;
		to += SFLASH_WRBUF_SIZE;
		buf += SFLASH_WRBUF_SIZE;
		(*retlen) += SFLASH_WRBUF_SIZE;
	}
	ret = mt3611_nor_write_buffer_disable(mt3611_nor);
	if (ret < 0)
		dev_warn(mt3611_nor->dev, "write buffer disable failed!\n");

	if (len) {
		ret = mt3611_nor_write_single_byte(mt3611_nor, to, (int)len,
						   (u8 *)buf);
		if (ret < 0)
			dev_err(mt3611_nor->dev, "write single byte failed!\n");
		(*retlen) += len;
	}
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Read NOR flash's register value.
 * @param[in]
 *     nor: spi_nor device for getting private data.
 * @param[in]
 *     opcode: operation code defined in SPI NOR flash specification.
 * @param[out]
 *     buf: feedback data of command.
 * @param[in]
 *     len: the length of feedback data.
 * @return
 *     0, read NOR flash's register value successfully. \n
 *     error code from mt3611_nor_execute_cmd() or mt3611_nor_do_tx_rx().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *    1. If mt3611_nor_execute_cmd() fails, return its error code.\n
 *    2. If mt3611_nor_do_tx_rx() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len)
{
	int ret;
	struct mt3611_nor *mt3611_nor = nor->priv;

	switch (opcode) {
	case SPINOR_OP_RDSR:
		ret = mt3611_nor_execute_cmd(mt3611_nor, MTK_NOR_RDSR_CMD);
		if (ret < 0)
			return ret;
		if (len == 1)
			*buf = readb(mt3611_nor->base + MTK_NOR_RDSR_REG);
		else
			dev_err(mt3611_nor->dev, "len should be 1 for read status!\n");
		break;
	default:
		ret = mt3611_nor_do_tx_rx(mt3611_nor,
			opcode, NULL, 0, buf, len);
		break;
	}
	return ret;
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Write NOR flash's register.
 * @param[in]
 *     nor: spi_nor device for getting private data.
 * @param[in]
 *     opcode: operation code defined in SPI NOR flash specification.
 * @param[in]
 *     buf: value to write.
 * @param[in]
 *     len: the length of buf.
 * @return
 *     0, read NOR flash's register value successfully. \n
 *     error code from mt3611_nor_wr_sr() or mt3611_nor_do_tx_rx().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *    1. If mt3611_nor_wr_sr() fails, return its error code.\n
 *    2. If mt3611_nor_do_tx_rx() fails, return its error code.
 * @par Call graph and Caller graph(refer to the graph below)
 * @par Refer to the source code
 */
static int mt3611_nor_write_reg(struct spi_nor *nor, u8 opcode, u8 *buf,
				int len)
{
	int ret;
	struct mt3611_nor *mt3611_nor = nor->priv;

	switch (opcode) {
	case SPINOR_OP_WRSR:
		/* We only handle 1 byte */
		ret = mt3611_nor_wr_sr(mt3611_nor, *buf);
		break;
	default:
		ret = mt3611_nor_do_tx_rx(mt3611_nor,
			opcode, buf, len, NULL, 0);
		if (ret)
			dev_warn(mt3611_nor->dev, "write reg failure!\n");
		break;
	}
	return ret;
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     Driver module initialization.
 * @param[in]
 *     mt3611_nor: module resource structure.
 * @param[in]
 *     flash_node: NOR flash device node.
 * @return
 *     error code from spi_nor_scan() or mtd_device_register().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *    1. If spi_nor_scan() fails, return its error code.\n
 *    2. If mtd_device_register() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __init mtk_nor_init(struct mt3611_nor *mt3611_nor,
			       struct device_node *flash_node)
{
	int ret;
	struct spi_nor *nor;

	/* initialize controller to accept commands */
	writel(MTK_NOR_ENABLE_SF_CMD, mt3611_nor->base + MTK_NOR_WRPROT_REG);

	nor = &mt3611_nor->nor;
	nor->dev = mt3611_nor->dev;
	nor->priv = mt3611_nor;
	nor->mtd.dev.of_node = flash_node;

	/* fill the hooks to spi nor */
	nor->read = mt3611_nor_read;
	nor->read_reg = mt3611_nor_read_reg;
	nor->write = mt3611_nor_write;
	nor->write_reg = mt3611_nor_write_reg;
	nor->mtd.name = "mtk_nor";
	/* initialized with NULL */
	dev_warn(nor->dev, "Bayi: mtk_nor_init\n");
	ret = spi_nor_scan(nor, NULL, SPI_NOR_QUAD);
	if (ret)
		return ret;

	return mtk_partition_register(&nor->mtd);
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     driver probe function.
 * @param[in]
 *     pdev: platform device structure for getting resource defined in DTS.
 * @return
 *     0, probe successfully.\n
 *     -EINVAL, invalid parameter.\n
 *     -ENOMEM, no memory error.\n
 *     -ENODEV, no NOR flash node detected.\n
 *     error code from devm_ioremap_resource(), devm_clk_get(),\n
 *     clk_prepare_enable(), of_get_next_available_child(), or mtk_nor_init().
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     1. If device node is NULL, return -EINVAL.\n
 *     2. If memory allocate fail, return -ENOMEM.\n
 *     3. If devm_ioremap_resource() fails, return its error code.\n
 *     4. If devm_clk_get() fails, return its error code.\n
 *     5. If clk_prepare_enable() fails, return its error code.\n
 *     6. If of_get_next_available_child() fails, return -ENODEV.\n
 *     7. If mtk_nor_init() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_nor_drv_probe(struct platform_device *pdev)
{
	struct device_node *flash_np;
	struct resource *res;
	int ret;
	struct mt3611_nor *mt3611_nor;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "No DT found\n");
		return -EINVAL;
	}

	mt3611_nor = devm_kzalloc(&pdev->dev, sizeof(*mt3611_nor), GFP_KERNEL);
	if (!mt3611_nor)
		return -ENOMEM;
	platform_set_drvdata(pdev, mt3611_nor);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mt3611_nor->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mt3611_nor->base))
		return PTR_ERR(mt3611_nor->base);
#if defined(CONFIG_COMMON_CLK_MT3611) || defined(CONFIG_COMMON_CLK_MT3612)
	mt3611_nor->spi_clk = devm_clk_get(&pdev->dev, "spi");
	if (IS_ERR(mt3611_nor->spi_clk))
		return PTR_ERR(mt3611_nor->spi_clk);

	mt3611_nor->nor_clk = devm_clk_get(&pdev->dev, "sf");
	if (IS_ERR(mt3611_nor->nor_clk))
		return PTR_ERR(mt3611_nor->nor_clk);

	mt3611_nor->dev = &pdev->dev;
	ret = clk_prepare_enable(mt3611_nor->spi_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(mt3611_nor->nor_clk);
	if (ret) {
		clk_disable_unprepare(mt3611_nor->spi_clk);
		return ret;
	}
#endif

	/* only support one attached flash */
	flash_np = of_get_next_available_child(pdev->dev.of_node, NULL);
	if (!flash_np) {
		dev_err(&pdev->dev, "no SPI flash device to configure\n");
		ret = -ENODEV;
		goto nor_free;
	}
	ret = mtk_nor_init(mt3611_nor, flash_np);

nor_free:
#if defined(CONFIG_COMMON_CLK_MT3611) || defined(CONFIG_COMMON_CLK_MT3612)
	if (ret) {
		clk_disable_unprepare(mt3611_nor->spi_clk);
		clk_disable_unprepare(mt3611_nor->nor_clk);
	}
#endif
	return ret;
}

/** @ingroup IP_group_snfc_internal_function
 * @par Description
 *     remove driver from kernel.
 * @param[in]
 *     pdev: structure of platform device for getting driver private data.
 * @return
 *     0, remove done.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_nor_drv_remove(struct platform_device *pdev)
{
#if defined(CONFIG_COMMON_CLK_MT3611) || defined(CONFIG_COMMON_CLK_MT3612)
	struct mt3611_nor *mt3611_nor = platform_get_drvdata(pdev);
	clk_disable_unprepare(mt3611_nor->spi_clk);
	clk_disable_unprepare(mt3611_nor->nor_clk);
#endif
	return 0;
}

static const struct of_device_id mtk_nor_of_ids[] = {
	{ .compatible = "mediatek,mt8173-nor"},
	{ .compatible = "mediatek,mt3611-nor"},
	{ .compatible = "mediatek,mt3612-nor"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtk_nor_of_ids);

static struct platform_driver mtk_nor_driver = {
	.probe = mtk_nor_drv_probe,
	.remove = mtk_nor_drv_remove,
	.driver = {
		.name = "mtk-nor",
		.of_match_table = mtk_nor_of_ids,
	},
};

module_platform_driver(mtk_nor_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek SPI NOR Flash Driver");
