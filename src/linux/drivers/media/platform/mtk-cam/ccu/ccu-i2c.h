/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __CCU_I2C_H__
#define __CCU_I2C_H__

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>

/* The following structs are defined on i2c/busses/i2c-mt65xx.c */

enum mtk_trans_op {
	I2C_MASTER_WR = 1,
	I2C_MASTER_RD,
	I2C_MASTER_WRRD,
};

struct mtk_i2c_compatible {
	/** i2c adapter quirks */
	const struct i2c_adapter_quirks *quirks;
	/** set to 1 if i2c support repeated start */
	unsigned char auto_restart: 1;
	/** set to 1 if i2c have module clock_div register */
	unsigned char internal_div: 1;
	/** memory extension selection */
	unsigned char mem_extension: 2;
};

struct mtk_i2c {
	/** i2c host adapter */
	struct i2c_adapter adap;
	/** i2c adapter device */
	struct device *dev;
	/** i2c transfer completion */
	struct completion msg_complete;
	/** i2c base address */
	void __iomem *base;
	/** top source clock for i2c bus */
	struct clk *clk_top_source;
	/** top source clock select for i2c bus */
	struct clk *clk_top_sel;
	/** peri source clock for i2c bus */
	struct clk *clk_peri_source;
	/** peri source clock select for i2c bus */
	struct clk *clk_peri_sel;
	/** main clock for i2c bus */
	struct clk *clk_main;
	/** IO config push-pull mode */
	bool use_push_pull;
	/** interrupt status */
	u16 irq_stat;
	/** i2c bus number */
	unsigned int id;
	/** i2c speed in transfer */
	unsigned int speed_hz;
	/** i2c transfer operation mode */
	enum mtk_trans_op op;
	/** external source clock divide */
	unsigned int clk_src_div;
	/** internal module clock divide */
	unsigned int clk_inter_div;
	/** timing register value */
	u16 timing_reg;
	/** fifo max size */
	unsigned char fifo_max_size;
	/** multi-transfer repeated start enable */
	unsigned char auto_restart;
	/** dma mode flag */
	bool dma_en;
	/** main control flag */
	bool main_control;
#ifdef CONFIG_MACH_FPGA
	/** polling mode flag */
	bool poll_en;
#endif
	/** data buffer for dma map */
	unsigned char *v_buf;
	/** i2c_msg pointer for i2c transfer */
	struct i2c_msg *msgs;
	/** i2c tx dma_chan */
	struct dma_chan *dma_tx;
	/** i2c rx dma_chan */
	struct dma_chan *dma_rx;
	/** scatterlist for i2c dma mode */
	struct scatterlist sg;
	/** data transfer direction for i2c dma mode */
	enum dma_data_direction dma_direction;
	/** used for i2c compatible */
	const struct mtk_i2c_compatible *dev_comp;
};

#endif
