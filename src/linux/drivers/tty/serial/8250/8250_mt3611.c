/*
 * Mediatek 8250 driver.
 *
 * Copyright (c) 2014 MundoReader S.L.
 * Author: Matthias Brugger <matthias.bgg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file 8250_mt3611.c
 */

/**
 * @defgroup IP_group_uart UART
 *   There provides basic functions of UART.
 *
 *   @{
 *       @defgroup IP_group_uart_external EXTERNAL
 *         The external API document for UART. \n
 *
 *         @{
 *             @defgroup IP_group_uart_external_function 1.function
 *               none. No extra function in UART driver.
 *             @defgroup IP_group_uart_external_struct 2.structure
 *               none. No extra structure in UART driver.
 *             @defgroup IP_group_uart_external_typedef 3.typedef
 *               none. No extra typedef in UART driver.
 *             @defgroup IP_group_uart_external_enum 4.enumeration
 *               none. No extra enumeration in UART driver.
 *             @defgroup IP_group_uart_external_def 5.define
 *               none. No extra define in UART driver.
 *         @}
 *
 *       @defgroup IP_group_uart_internal INTERNAL
 *         The internal API document for UART. \n
 *
 *         @{
 *             @defgroup IP_group_uart_internal_function 1.function
 *               Internal function used for UART.
 *             @defgroup IP_group_uart_internal_struct 2.structure
 *               Internal structure used for UART.
 *             @defgroup IP_group_uart_internal_typedef 3.typedef
 *               none. No extra typedef in UART driver.
 *             @defgroup IP_group_uart_internal_enum 4.enumeration
 *               Internal enumeration used for UART.
 *             @defgroup IP_group_uart_internal_def 5.define
 *               Internal define used for UART.
 *         @}
 *   @}
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/libfdt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/console.h>
#include <linux/dma-mapping.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

#include "8250.h"

/** @ingroup IP_group_uart_internal_def
 * @brief brief for below all define.
 * @{
 */
/* Highspeed register */
#define UART_MTK_HIGHS			0x09
/* Sample count register */
#define UART_MTK_SAMPLE_COUNT		0x0a
/* Sample point register */
#define UART_MTK_SAMPLE_POINT		0x0b
/* UART Rate Fix Register */
#define MTK_UART_RATE_FIX		0x0d
#define UART_MTK_ESCAPE_DAT		0x10
#define UART_MTK_ESCAPE_EN		0x11
#define UART_MTK_DMA_EN			0x13
#define UART_MTK_FRACDIV_L		0x15
#define UART_MTK_FRACDIV_M		0x16
#define UART_ESCAPE_CH			0x77
#define UART_IER_XOFFI			BIT(5)
#define UART_IER_RTSI			BIT(6)
#define UART_IER_CTSI			BIT(7)
#define UART_EFR_EN			BIT(4)
#define UART_EFR_AUTO_RTS		BIT(6)
#define UART_EFR_AUTO_CTS		BIT(7)
#define UART_EFR_SW_CTRL_MASK		(0xf << 0)
#define UART_EFR_NO_SW_CTRL		(0)
#define UART_EFR_NO_FLOW_CTRL		(0)
#define UART_EFR_AUTO_RTSCTS		(UART_EFR_AUTO_RTS | UART_EFR_AUTO_CTS)
#define UART_FCR_FIFOE			BIT(0)
#define UART_FCR_CLRR			BIT(1)
#define UART_FCR_CLRT			BIT(2)
#define UART_DMA_EN_RX_DMA_EN		BIT(0)
#define UART_DMA_EN_TX_DMA_EN		BIT(1)
#define UART_DMA_EN_TO_CNT_AUTORST	BIT(2)
/* TX/RX XON1/XOFF1 flow control */
#define UART_EFR_XON1_XOFF1		(0xa)
/* TX/RX XON2/XOFF2 flow control */
#define UART_EFR_XON2_XOFF2		(0x5)
/* TX/RX XON1,2/XOFF1,2 flow control */
#define UART_EFR_XON12_XOFF12		(0xf)
#define TX_TRIGGER			1
#define RX_TRIGGER			8192
/**
 * @}
 */

#ifdef CONFIG_SERIAL_8250_DMA
/** @ingroup IP_group_uart_internal_enum
 * @brief dma rx status.
 */
enum dma_rx_status {
	/** 0:RX start */
	DMA_RX_START = 0,
	/** 1:RX is running */
	DMA_RX_RUNNING = 1,
	/** 2:RX Shutdown */
	DMA_RX_SHUTDOWN = 2,
};
#endif

/** @ingroup IP_group_uart_internal_struct
 * @brief Used to define uart related information.
 */
struct mtk8250_data {
	/** the port line number */
	int line;
	/** dma rx buffer position */
	int rxpos;
	/** uart module clock */
	struct clk *uart_clk;
#if defined(CONFIG_COMMON_CLK_MT3612) || defined(CONFIG_COMMON_CLK_MT3611)
	/** uart pericfg clock */
	struct clk *pericfg_clk;
#endif
	/** dma related information */
	struct uart_8250_dma *dma;
#ifdef CONFIG_SERIAL_8250_DMA
	/** dma rx status */
	enum dma_rx_status rxstatus;
#endif
};

/** @ingroup IP_group_uart_internal_enum
 * @brief Flow control mode.
 */
enum {
	/** NO flow Control */
	UART_FC_NONE,
	/** MTK SW Flow Control, diff from Linux */
	UART_FC_SW,
	/** HW Flow Control */
	UART_FC_HW,
};

const u16 fraction_L_mapping[] = {
	0x00, 0x01, 0x05, 0x15, 0x55, 0x57, 0x57, 0x77, 0x7f, 0xff, 0xff
};

const u16 fraction_M_mapping[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03
};

#ifdef CONFIG_SERIAL_8250_DMA
static int mtk8250_rx_dma(struct uart_8250_port *up);

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     dma rx callback function.
 * @param[in]
 *     param: pointer to the description of uart port.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __mtkdma_rx_complete(void *param)
{
	struct uart_8250_port *up = param;
	struct uart_8250_dma *dma = up->dma;
	struct mtk8250_data *data = up->port.private_data;
	struct tty_port *tty_port = &up->port.state->port;
	struct dma_tx_state state;
	unsigned char *ptr;
	int copied;
	int count;

	dma_sync_single_for_cpu(dma->rxchan->device->dev, dma->rx_addr,
				dma->rx_size, DMA_FROM_DEVICE);

	dmaengine_tx_status(dma->rxchan, dma->rx_cookie, &state);

	if (data->rxstatus == DMA_RX_SHUTDOWN)
		return;

	count = dma->rx_size - state.residue;
	if ((data->rxpos + count) <= dma->rx_size) {
		ptr = (unsigned char *)(data->rxpos + dma->rx_buf);
		copied = tty_insert_flip_string(tty_port, ptr, count);
		data->rxpos = data->rxpos + count;
	} else {
		ptr = (unsigned char *)(data->rxpos + dma->rx_buf);
		copied = tty_insert_flip_string(tty_port, ptr,
						(dma->rx_size - data->rxpos));

		ptr = (unsigned char *)(dma->rx_buf);
		data->rxpos = (data->rxpos + count - dma->rx_size);
		copied += tty_insert_flip_string(tty_port, ptr,  data->rxpos);
	}
	up->port.icount.rx += copied;

	tty_flip_buffer_push(tty_port);

	mtk8250_rx_dma(up);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     dma rx function.
 * @param[in]
 *     up: pointer to the description of uart port.
 * @return
 *     return 0 if success.\n
 *     rerurn -EBUSY if fail.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_rx_dma(struct uart_8250_port *up)
{
	struct uart_8250_dma *dma = up->dma;
	struct dma_async_tx_descriptor *desc;

	desc = dmaengine_prep_slave_single(dma->rxchan, dma->rx_addr,
					   dma->rx_size, DMA_DEV_TO_MEM,
					   DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc)
		return -EBUSY;

	desc->callback = __mtkdma_rx_complete;
	desc->callback_param = up;

	dma->rx_cookie = dmaengine_submit(desc);

	dma_sync_single_for_device(dma->rxchan->device->dev, dma->rx_addr,
				   dma->rx_size, DMA_FROM_DEVICE);

	dma_async_issue_pending(dma->rxchan);

	return 0;
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     dma rx enbale function.
 * @param[in]
 *     up: pointer to the description of uart port.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_dma_enable(struct uart_8250_port *up)
{
	struct uart_port *port = &up->port;
	struct uart_8250_dma *dma = up->dma;
	struct mtk8250_data *data = up->port.private_data;
	int tmp = 0;

	dma->rxconf.direction = DMA_DEV_TO_MEM;
	dma->rxconf.src_addr_width = dma->rx_size / 1024;
	dma->rxconf.src_addr = dma->rx_addr;

	dma->txconf.direction = DMA_MEM_TO_DEV;
	dma->txconf.dst_addr_width = UART_XMIT_SIZE / 1024;
	dma->txconf.dst_addr = dma->tx_addr;

	tmp = UART_FCR_FIFOE | UART_FCR_CLRR | UART_FCR_CLRT;
	serial_port_out(port, UART_FCR, tmp);

	tmp = serial_port_in(port, UART_MTK_DMA_EN);
	tmp |= UART_DMA_EN_RX_DMA_EN | UART_DMA_EN_TO_CNT_AUTORST;
	serial_port_out(port, UART_MTK_DMA_EN, tmp);

	tmp = serial_port_in(port, UART_MTK_DMA_EN);
	tmp |= UART_DMA_EN_TX_DMA_EN;
	serial_port_out(port, UART_MTK_DMA_EN, tmp);

	tmp = serial_port_in(port, UART_LCR);
	serial_port_out(port, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_port_out(port, UART_EFR, UART_EFR_ECB);
	serial_port_out(port, UART_LCR, tmp);

	dmaengine_slave_config(dma->rxchan, &dma->rxconf);
	dmaengine_slave_config(dma->txchan, &dma->txconf);

	if (data->rxstatus == DMA_RX_START) {
		data->rxstatus = DMA_RX_RUNNING;
		data->rxpos = 0;
		mtk8250_rx_dma(up);
	}
}
#endif

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Open the uart port.
 * @param[in]
 *     up: pointer to the description of uart port.
 * @return
 *     If return value is 0, function success.\n
 *     If return value is non-zero, function fail.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_startup(struct uart_port *port)
{
#ifdef CONFIG_SERIAL_8250_DMA
	struct uart_8250_port *up =
	    container_of(port, struct uart_8250_port, port);
	struct mtk8250_data *data = port->private_data;

	if (up->dma)
		data->rxstatus = DMA_RX_START;
#endif

	return serial8250_do_startup(port);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Close the uart port.
 * @param[in]
 *     up: pointer to the description of uart port.
 * @return
 *     None.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk8250_shutdown(struct uart_port *port)
{
#ifdef CONFIG_SERIAL_8250_DMA
	struct uart_8250_port *up =
	    container_of(port, struct uart_8250_port, port);
	struct mtk8250_data *data = port->private_data;

	if (up->dma)
		data->rxstatus = DMA_RX_SHUTDOWN;
#endif

	return serial8250_do_shutdown(port);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Disbale the specific interrupt of uart.
 * @param[in]
 *     port: pointer to the description of uart port.
 * @param[in]
 *     mask: interrupt mask.
 * @return
 *     None.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk8250_uart_disable_intrs(struct uart_port *port,
				       const long mask)
{
	unsigned int tmp = serial_port_in(port, UART_IER);

	tmp &= ~(mask);
	serial_port_out(port, UART_IER, tmp);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Enable the specific interrupt of uart.
 * @param[in]
 *     port: pointer to the description of uart port.
 * @param[in]
 *     mask: interrupt mask.
 * @return
 *     None.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk8250_uart_enable_intrs(struct uart_port *port,
				      const long mask)
{
	unsigned int tmp = serial_port_in(port, UART_IER);

	tmp |= mask;
	serial_port_out(port, UART_IER, tmp);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Set the specific flow control of uart.
 * @param[in]
 *     port: pointer to the description of uart port.
 * @param[in]
 *     mode: flow control mode.
 * @return
 *     None.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk8250_uart_set_flow_ctrl(struct uart_port *port, int mode)
{
	unsigned int tmp = serial_port_in(port, UART_LCR);
	unsigned long old;

	serial_port_out(port, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_port_out(port, UART_EFR, UART_EFR_ECB);
	serial_port_out(port, UART_LCR, tmp);
	tmp = serial_port_in(port, UART_LCR);

	switch (mode) {
	case UART_FC_NONE:
		serial_port_out(port, UART_MTK_ESCAPE_DAT, UART_ESCAPE_CH);
		serial_port_out(port, UART_MTK_ESCAPE_EN, 0x00);
		serial_port_out(port, UART_LCR, UART_LCR_CONF_MODE_B);
		old = serial_port_in(port, UART_EFR);
		old &= ~(UART_EFR_AUTO_RTSCTS | UART_EFR_XON12_XOFF12);
		serial_port_out(port, UART_EFR, old);
		serial_port_out(port, UART_LCR, tmp);
		mtk8250_uart_disable_intrs(port,
					   UART_IER_XOFFI | UART_IER_RTSI |
					   UART_IER_CTSI);
		break;

	case UART_FC_HW:
		serial_port_out(port, UART_MTK_ESCAPE_DAT, UART_ESCAPE_CH);
		serial_port_out(port, UART_MTK_ESCAPE_EN, 0x00);
		serial_port_out(port, UART_MCR, UART_MCR_RTS);
		serial_port_out(port, UART_LCR, UART_LCR_CONF_MODE_B);

		/* disable all flow control setting */
		old = serial_port_in(port, UART_EFR);
		old &= ~(UART_EFR_AUTO_RTSCTS | UART_EFR_XON12_XOFF12);
		serial_port_out(port, UART_EFR, old);

		/* enable hw flow control */
		old = serial_port_in(port, UART_EFR);
		serial_port_out(port, UART_EFR, old | UART_EFR_AUTO_RTSCTS);
		serial_port_out(port, UART_LCR, tmp);
		mtk8250_uart_disable_intrs(port, UART_IER_XOFFI);
		mtk8250_uart_enable_intrs(port, UART_IER_CTSI | UART_IER_RTSI);
		break;

	/* MTK software flow control */
	case UART_FC_SW:
		serial_port_out(port, UART_MTK_ESCAPE_DAT, UART_ESCAPE_CH);
		serial_port_out(port, UART_MTK_ESCAPE_EN, 0x01);
		serial_port_out(port, UART_LCR, UART_LCR_CONF_MODE_B);

		/* disable all flow control setting */
		old = serial_port_in(port, UART_EFR);
		old &= ~(UART_EFR_AUTO_RTSCTS | UART_EFR_XON12_XOFF12);
		serial_port_out(port, UART_EFR, old);

		/* enable sw flow control */
		old = serial_port_in(port, UART_EFR);
		serial_port_out(port, UART_EFR, old | UART_EFR_XON1_XOFF1);
		serial_port_out(port, UART_XON1,
				START_CHAR(port->state->port.tty));
		serial_port_out(port, UART_XOFF1,
				STOP_CHAR(port->state->port.tty));
		serial_port_out(port, UART_LCR, tmp);
		mtk8250_uart_disable_intrs(port, UART_IER_CTSI | UART_IER_RTSI);
		mtk8250_uart_enable_intrs(port, UART_IER_XOFFI);
		break;
	}
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Set the specific parameters of uart.
 * @param[in]
 *     port: pointer to the description of uart port.
 * @param[in]
 *     termios: pointer to the new parameters.
 * @param[in]
 *     old: pointer to the old parameters.
 * @return
 *     None.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void
mtk8250_set_termios(struct uart_port *port, struct ktermios *termios,
		    struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud, quot;
	int mode;

	struct uart_8250_port *up =
	    container_of(port, struct uart_8250_port, port);

#ifdef CONFIG_SERIAL_8250_DMA
	if (up->dma)
		mtk_dma_enable(up);
#endif

	serial8250_do_set_termios(port, termios, old);

	/*
	 * Mediatek UARTs use an extra highspeed register (UART_MTK_HIGHS)
	 *
	 * We need to recalcualte the quot register, as the claculation depends
	 * on the value in the highspeed register.
	 *
	 * Some baudrates are not supported by the chip, so we use the next
	 * lower rate supported and update termios c_flag.
	 *
	 * If highspeed register is set to 3, we need to specify sample count
	 * and sample point to increase accuracy. If not, we reset the
	 * registers to their default values.
	 */
	baud = uart_get_baud_rate(port, termios, old,
				  port->uartclk / 16 / 0xffff, port->uartclk);

	if (baud <= 115200) {
		serial_port_out(port, UART_MTK_HIGHS, 0x0);
		quot = uart_get_divisor(port, baud);
	} else if (baud <= 576000) {
		serial_port_out(port, UART_MTK_HIGHS, 0x2);

		/* Set to next lower baudrate supported */
		if ((baud == 500000) || (baud == 576000))
			baud = 460800;
		quot = DIV_ROUND_UP(port->uartclk, 4 * baud);
	} else {
		u32 fraction = ((port->uartclk * 10) / baud) % 10;

		serial_port_out(port, UART_MTK_FRACDIV_L,
				fraction_L_mapping[fraction]);
		serial_port_out(port, UART_MTK_FRACDIV_M,
				fraction_M_mapping[fraction]);
		serial_port_out(port, UART_MTK_HIGHS, 0x3);
		quot = DIV_ROUND_UP(port->uartclk, 256 * baud);
	}

	/*
	 * Ok, we're now changing the port state.  Do it with
	 * interrupts disabled.
	 */
	spin_lock_irqsave(&port->lock, flags);

	/* set DLAB we have cval saved in up->lcr from the call to the core */
	serial_port_out(port, UART_LCR, up->lcr | UART_LCR_DLAB);
	serial_dl_write(up, quot);

	/* reset DLAB */
	serial_port_out(port, UART_LCR, up->lcr);

	if (baud > 460800) {
		unsigned int tmp;

		tmp = port->uartclk / (quot * baud);
		serial_port_out(port, UART_MTK_SAMPLE_COUNT, tmp - 1);
		serial_port_out(port, UART_MTK_SAMPLE_POINT, (tmp - 2) >> 1);
	} else {
		serial_port_out(port, UART_MTK_SAMPLE_COUNT, 0x00);
		serial_port_out(port, UART_MTK_SAMPLE_POINT, 0xff);
	}

	if ((termios->c_cflag & CRTSCTS) && (!(termios->c_iflag & CRTSCTS)))
		mode = UART_FC_HW;
	else if (termios->c_iflag & CRTSCTS)
		mode = UART_FC_SW;
	else
		mode = UART_FC_NONE;

	mtk8250_uart_set_flow_ctrl(port, mode);

	spin_unlock_irqrestore(&port->lock, flags);

	/* Don't rewrite B0 */
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
}
/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Runtime suspend function.
 * @param[in]
 *     dev: pionter to the device.
 * @return
 *     always return 0.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_runtime_suspend(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);

#if defined(CONFIG_COMMON_CLK_MT3612) || defined(CONFIG_COMMON_CLK_MT3611)
	clk_disable_unprepare(data->pericfg_clk);
#endif
	clk_disable_unprepare(data->uart_clk);

	return 0;
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Runtime resume function.
 * @param[in]
 *     dev: pionter to the device.
 * @return
 *     If return value is 0, function success.\n
 *     If return value is non-zero, function fail.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_runtime_resume(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);
	int err;

	err = clk_prepare_enable(data->uart_clk);
	if (err) {
		dev_warn(dev, "Can't enable clock\n");
		return err;
	}

#if defined(CONFIG_COMMON_CLK_MT3612) || defined(CONFIG_COMMON_CLK_MT3611)
	err = clk_prepare_enable(data->pericfg_clk);
	if (err) {
		dev_warn(dev, "Can't enable bus clock\n");
		return err;
	}
#endif

	return 0;
}


/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Runtime resume function.
 * @param[in]
 *     port: pointer to the description of uart port.
 * @param[in]
 *     state: new state.
 * @param[in]
 *     old: old state.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void
mtk8250_do_pm(struct uart_port *port, unsigned int state, unsigned int old)
{
	if (!state)
		pm_runtime_get_sync(port->dev);

	serial8250_do_pm(port, state, old);

	if (state)
		pm_runtime_put_sync_suspend(port->dev);
}

#ifdef CONFIG_SERIAL_8250_DMA
/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     DMA filter function.
 * @param[in]
 *     chan: pointer to the dma channel.
 * @param[in]
 *     param: pointer to the filter parameter.
 * @return
 *     always return false.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool the_no_dma_filter_fn(struct dma_chan *chan, void *param)
{
	return false;
}
#endif

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Get uart related clock when probe.
 * @param[in]
 *     pdev: pointer to the device.
 * @param[in]
 *     p: pointer to the uart port.
 * @param[in]
 *     data: pointer to the uart port related information.
 * @return
 *     If return value is 0, function success.\n
 *     If return value is non-zero, function fail.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_probe_of(struct platform_device *pdev,
			    struct uart_port *p, struct mtk8250_data *data)
{
#ifdef CONFIG_SERIAL_8250_DMA
	int dmacnt;
#endif
	int err;

	data->uart_clk = devm_clk_get(&pdev->dev, "baud");
	if (IS_ERR(data->uart_clk)) {
		/*
		 * For compatibility with older device trees try unnamed
		 * clk when no baud clk can be found.
		 */
		data->uart_clk = devm_clk_get(&pdev->dev, NULL);
		if (IS_ERR(data->uart_clk)) {
			dev_warn(&pdev->dev, "Can't get uart baud clock\n");
			return PTR_ERR(data->uart_clk);
		}

		return 0;
	}

	err = clk_prepare_enable(data->uart_clk);
	if (err) {
		dev_warn(&pdev->dev, "Can't enable baud clock\n");
		return err;
	}

#if defined(CONFIG_COMMON_CLK_MT3612) || defined(CONFIG_COMMON_CLK_MT3611)
	data->pericfg_clk = devm_clk_get(&pdev->dev, "uart-pericfg-clk");
	if (IS_ERR(data->pericfg_clk)) {
		/*
		 * For compatibility with older device trees try unnamed
		 * clk when no baud clk can be found.
		 */
		data->pericfg_clk = devm_clk_get(&pdev->dev, NULL);
		if (IS_ERR(data->pericfg_clk)) {
			dev_warn(&pdev->dev, "Can't get uart pericfg gate clock\n");
			return PTR_ERR(data->pericfg_clk);
		}

		return 0;
	}

	err = clk_prepare_enable(data->pericfg_clk);
	if (err) {
		dev_warn(&pdev->dev, "Can't enable uart pericfg gate clock\n");
		return err;
	}
#endif

	data->dma = NULL;
#ifdef CONFIG_SERIAL_8250_DMA
	dmacnt = of_property_count_strings(pdev->dev.of_node, "dma-names");
	if (dmacnt == 2) {
		data->dma =
		    devm_kzalloc(&pdev->dev, sizeof(*data->dma), GFP_KERNEL);
		data->dma->fn = the_no_dma_filter_fn;
		data->dma->rx_size = RX_TRIGGER;
		data->dma->rxconf.src_maxburst = RX_TRIGGER;
		data->dma->txconf.dst_maxburst = TX_TRIGGER;
		data->dma->rxconf.slave_id = 1;
		data->dma->txconf.slave_id = 1;
	}
#endif

	return 0;
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Uart probe function.
 * @param[in]
 *     pdev: pointer to the device.
 * @return
 *     If return value is 0, function success.\n
 *     If return value is non-zero, function fail.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_probe(struct platform_device *pdev)
{
	struct uart_8250_port uart = { };
	struct resource *regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct resource *irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	struct mtk8250_data *data;
	int err;

	if (!regs || !irq) {
		dev_err(&pdev->dev, "no registers/irq defined\n");
		return -EINVAL;
	}

	uart.port.membase = devm_ioremap(&pdev->dev, regs->start,
					 resource_size(regs));
	if (!uart.port.membase)
		return -ENOMEM;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		err = mtk8250_probe_of(pdev, &uart.port, data);
		if (err)
			return err;
	} else {
		return -ENODEV;
	}

	spin_lock_init(&uart.port.lock);
	uart.port.mapbase = regs->start;
	uart.port.irq = irq->start;
	uart.port.pm = mtk8250_do_pm;
	uart.port.type = PORT_16550;
	uart.port.flags = UPF_BOOT_AUTOCONF | UPF_FIXED_PORT;
	uart.port.dev = &pdev->dev;
	uart.port.iotype = UPIO_MEM32;
	uart.port.regshift = 2;
	uart.port.private_data = data;
	uart.port.shutdown = mtk8250_shutdown;
	uart.port.startup = mtk8250_startup;
	uart.port.set_termios = mtk8250_set_termios;
	uart.port.uartclk = clk_get_rate(data->uart_clk);

#ifdef CONFIG_SERIAL_8250_DMA
	if (data->dma)
		uart.dma = data->dma;
#endif

	/* Disable Rate Fix function */
	writel(0x0, uart.port.membase +
	       (MTK_UART_RATE_FIX << uart.port.regshift));

	platform_set_drvdata(pdev, data);

	pm_runtime_enable(&pdev->dev);

	if (!pm_runtime_enabled(&pdev->dev)) {
		err = mtk8250_runtime_resume(&pdev->dev);
		if (err)
			return err;
	}

	data->line = serial8250_register_8250_port(&uart);

	if (data->line < 0)
		return data->line;

	pr_info("mt3611, mtk8250_probe end.\n");

	return 0;
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Uart remove function.
 * @param[in]
 *     pdev: pointer to the device.
 * @return
 *     always return 0.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_remove(struct platform_device *pdev)
{
	struct mtk8250_data *data = platform_get_drvdata(pdev);

	pm_runtime_get_sync(&pdev->dev);

	serial8250_unregister_port(data->line);
#if defined(CONFIG_COMMON_CLK_MT3612) || defined(CONFIG_COMMON_CLK_MT3611)
	clk_disable_unprepare(data->pericfg_clk);
#endif
	clk_disable_unprepare(data->uart_clk);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	if (!pm_runtime_status_suspended(&pdev->dev))
		mtk8250_runtime_suspend(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Uart suspend function.
 * @param[in]
 *     pdev: pointer to the device.
 * @return
 *     always return 0.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_suspend(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);

	serial8250_suspend_port(data->line);

	return 0;
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Uart resume function.
 * @param[in]
 *     pdev: pointer to the device.
 * @return
 *     always return 0.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk8250_resume(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);

	serial8250_resume_port(data->line);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static const struct dev_pm_ops mtk8250_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk8250_suspend, mtk8250_resume)
	SET_RUNTIME_PM_OPS(mtk8250_runtime_suspend, mtk8250_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk8250_of_match[] = {
	{.compatible = "mediatek,mt3611-uart"},
	{ /* Sentinel */ }
};

MODULE_DEVICE_TABLE(of, mtk8250_of_match);

static struct platform_driver mtk8250_platform_driver = {
	.driver = {
		   .name = "mt3611-uart",
		   .pm = &mtk8250_pm_ops,
		   .of_match_table = mtk8250_of_match,
		   },
	.probe = mtk8250_probe,
	.remove = mtk8250_remove,
};

module_platform_driver(mtk8250_platform_driver);

#ifdef CONFIG_SERIAL_8250_CONSOLE

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Read register value for early console.
 * @param[in]
 *     dev: pointer to the device.
 * @param[in]
 *     offset: register address offset.
 * @return
 *     register value.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static uint8_t __init mtk8250_early_in(struct uart_port *port, int offset)
{
	return readl(port->membase + (offset << 2));
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     Write register value for early console.
 * @param[in]
 *     dev: pointer to the device.
 * @param[in]
 *     offset: register address offset.
 * @param[in]
 *     value: register new value.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __init mtk8250_early_out(struct uart_port *port, int offset,
				     uint8_t value)
{
	writel(value, port->membase + (offset << 2));
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     early console: Putchar function.
 * @param[in]
 *     port: pointer to the uart port.
 * @param[in]
 *     c: specifies the character to send.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __init mtk8250_early_putchar(struct uart_port *port, int c)
{
	u8 lsr;

	do {
		lsr = mtk8250_early_in(port, UART_LSR);
	} while ((lsr & UART_LSR_THRE) == 0);

	mtk8250_early_out(port, UART_TX, c);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     early console: write a console message to a serial port.
 * @param[in]
 *     con: pointer to the console device.
 * @param[in]
 *     s: array of characters.
 * @param[in]
 *     n: number of characters in string to write.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __init mtk8250_early_write(struct console *con,
				const char *s, unsigned n)
{
	struct earlycon_device *dev = con->data;

	uart_console_write(&dev->port, s, n, mtk8250_early_putchar);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     early console: Setup clock function.
 * @param[in]
 *     dev: pointer to the earlycon_device.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __init mtk8250_early_console_setup_clock(struct earlycon_device
						     *dev)
{
	void *fdt = initial_boot_params;
	const __be32 *prop;
	int offset;

	offset = fdt_path_offset(fdt, "/uart_clk");
	if (offset < 0) {
		offset = fdt_path_offset(fdt, "/dummy26m");
		if (offset < 0)
			return;
	}

	prop = fdt_getprop(fdt, offset, "clock-frequency", NULL);
	if (!prop)
		return;

	dev->port.uartclk = be32_to_cpup(prop);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     early console: setup the console baudrate.
 * @param[in]
 *     port: pointer to the uart port.
 * @param[in]
 *     load_baud: uart port baudrate.
 * @param[in]
 *     uartclk: uart port clock.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void __init mtk8250_early_baud_setup(struct uart_port *port,
					    u32 load_baud,
					    u32 uartclk)
{
	u32 quot, tmp;
	u32 fraction = ((uartclk * 10) / load_baud) % 10;
	u8 lcr = mtk8250_early_in(port, UART_LCR) | UART_LCR_WLEN8;

	mtk8250_early_out(port, UART_MTK_FRACDIV_L,
			  fraction_L_mapping[fraction]);
	mtk8250_early_out(port, UART_MTK_FRACDIV_M,
			  fraction_M_mapping[fraction]);
	mtk8250_early_out(port, UART_MTK_HIGHS, 0x3);

	quot = DIV_ROUND_UP(uartclk, 256 * load_baud);
	tmp = uartclk / (quot * load_baud);

	mtk8250_early_out(port, UART_LCR, lcr | UART_LCR_DLAB);
	mtk8250_early_out(port, UART_DLL, (quot & 0x00ff));
	mtk8250_early_out(port, UART_DLM, ((quot >> 8) & 0x00ff));
	mtk8250_early_out(port, UART_LCR, lcr);

	mtk8250_early_out(port, UART_MTK_SAMPLE_COUNT, tmp - 1);
	mtk8250_early_out(port, UART_MTK_SAMPLE_POINT, (tmp - 2) >> 1);

	/* enable FIFO and clear it */
	mtk8250_early_out(port, UART_FCR, 0x7);
}

/** @ingroup IP_group_uart_internal_function
 * @par Description
 *     early console: setup the console.
 * @param[in]
 *     device: pointer to the console device.
 * @param[in]
 *     options: pointer to the uart port options.
 * @return
 *     none.
 * @par Boundary case and limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int __init mtk8250_early_setup(struct earlycon_device *device,
				      const char *options)
{
	u32 baud, uartclk;

	if (!device->port.membase)
		return -ENODEV;

	mtk8250_early_console_setup_clock(device);
	baud = device->baud ? : 115200;
	uartclk = device->port.uartclk;

	mtk8250_early_baud_setup(&device->port, baud, uartclk);

	device->con->write = mtk8250_early_write;

	return 1;
}

EARLYCON_DECLARE(mtk8250, mtk8250_early_setup);
OF_EARLYCON_DECLARE(mtk8250, "mediatek,mt3611-uart", mtk8250_early_setup);
#endif

MODULE_AUTHOR("Matthias Brugger");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek 8250 serial port driver");
