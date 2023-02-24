/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Xudong Chen <xudong.chen@mediatek.com>
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
 * @file i2c-mt65xx.c
 * This i2c driver is used to control MediaTek I2C controller.\n
 * It provides the interfaces which will be used in Linux i2c\n
 * framework.
 */

/**
 * @defgroup IP_group_i2c I2C
 *
 *   @{
 *       @defgroup IP_group_i2c_external EXTERNAL
 *         The external API document for I2C. \n
 *
 *         @{
 *            @defgroup IP_group_i2c_external_function 1.function
 *              none.
 *            @defgroup IP_group_i2c_external_struct 2.structure
 *              none.
 *            @defgroup IP_group_i2c_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_i2c_external_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_i2c_external_def 5.define
 *              none.
 *         @}
 *
 *       @defgroup IP_group_i2c_internal INTERNAL
 *         The internal API document for I2C. \n
 *
 *         @{
 *            @defgroup IP_group_i2c_internal_function 1.function
 *              Internal function in i2c driver.
 *            @defgroup IP_group_i2c_internal_struct 2.structure
 *              Internal structure in i2c driver.
 *            @defgroup IP_group_i2c_internal_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_i2c_internal_enum 4.enumeration
 *              Internal enumeration in i2c driver.
 *            @defgroup IP_group_i2c_internal_def 5.define
 *              Internal define in i2c driver.
 *         @}
 *   @}
 */

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

/** @ingroup IP_group_i2c_internal_def
 * @{
 */
#ifdef CONFIG_MACH_FPGA
#define I2C_SOURCE_CLOCK		6000000
#else
#define I2C_SOURCE_CLOCK		136500000
#endif

#define I2C_RS_TRANSFER			BIT(4)
#define I2C_ACKERR			BIT(1)
#define I2C_TRANSAC_COMP		BIT(0)
#define I2C_TRANSAC_START		BIT(0)
#define I2C_RS_MUL_CNFG			BIT(15)
#define I2C_RS_MUL_TRIG			BIT(14)
#define I2C_IO_CONFIG_OPEN_DRAIN	0x0003
#define I2C_IO_CONFIG_PUSH_PULL		0x0000
#define I2C_CONTROL_DEFAULT		0x0d00
#define I2C_SOFT_RST			0x0001
#define I2C_FIFO_ADDR_CLR		0x0001
#define I2C_DELAY_LEN			0x0002
#define I2C_ST_START_CON		0x8001
#define I2C_FS_START_CON		0x1800
#define I2C_TIME_CLR_VALUE		0x0000
#define I2C_TIME_DEFAULT_VALUE		0x0003
#define I2C_WRRD_TRANAC_VALUE		0x0002
#define I2C_RD_TRANAC_VALUE		0x0001
#define I2C_SCL_HIGH_LOW_RATIO_DEFAULT	0x00c3
#define I2C_SCL_MIS_COMP_POINT_DEFAULT	0x0000
#define I2C_STA_STOP_AC_TIMING_DEFAULT	0x0000
#define I2C_SDA_TIMING_DEFAULT		0x0000

#define I2C_FIFO_SIZE			16
#define I2C_DEFAULT_CLK_DIV		5
#define I2C_DEFAULT_SPEED		100000	/* hz */
#define MAX_FS_MODE_SPEED		400000
#define MAX_FS_PLUS_SPEED		1000000
#define MAX_SAMPLE_CNT_DIV		8
#define MAX_STEP_CNT_DIV		64

#define I2C_CONTROL_RS                  BIT(1)
#define I2C_CONTROL_DMA_EN              BIT(2)
#define I2C_CONTROL_CLK_EXT_EN          BIT(3)
#define I2C_CONTROL_DIR_CHANGE          BIT(4)
#define I2C_CONTROL_ACKERR_DET_EN       BIT(5)
#define I2C_CONTROL_TRANSFER_LEN_CHANGE BIT(6)
#define I2C_CONTROL_AYNCS_MODE          BIT(9)

#define I2C_DRV_NAME		"i2c-mt36xx"
/** @}
 */

/** @ingroup IP_group_i2c_internal_enum
 * @brief I2C transfer operation mode.
 */
enum mtk_trans_op {
	I2C_MASTER_WR = 1,
	I2C_MASTER_RD,
	I2C_MASTER_WRRD,
};

/** @ingroup IP_group_i2c_internal_enum
 * @brief I2C controller register offset.
 */
enum I2C_REGS_OFFSET {
	OFFSET_DATA_PORT = 0x0,
	OFFSET_SLAVE_ADDR = 0x04,
	OFFSET_INTR_MASK = 0x08,
	OFFSET_INTR_STAT = 0x0c,
	OFFSET_CONTROL = 0x10,
	OFFSET_TRANSFER_LEN = 0x14,
	OFFSET_TRANSAC_LEN = 0x18,
	OFFSET_DELAY_LEN = 0x1c,
	OFFSET_TIMING = 0x20,
	OFFSET_START = 0x24,
	OFFSET_EXT_CONF = 0x28,
	OFFSET_FIFO_STAT1 = 0x2c,
	OFFSET_FIFO_STAT = 0x30,
	OFFSET_FIFO_THRESH = 0x34,
	OFFSET_FIFO_ADDR_CLR = 0x38,
	OFFSET_IO_CONFIG = 0x40,
	OFFSET_MULTI_MASTER = 0x44,
	OFFSET_SOFTRESET = 0x50,
	OFFSET_DEBUGSTAT = 0x64,
	OFFSET_TRANSFER_LEN_AUX = 0x6c,
	OFFSET_CLOCK_DIV = 0x70,
	OFFSET_SCL_HIGH_LOW_RATIO = 0x74,
	OFFSET_SCL_MIS_COMP_POINT = 0x7c,
	OFFSET_STA_STOP_AC_TIMING = 0x80,
	OFFSET_SDA_TIMING = 0x88,
};

/** @ingroup IP_group_i2c_internal_struct
 * @brief Struct keeping I2C controller hardward information.
 */
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

/** @ingroup IP_group_i2c_internal_struct
 * @brief Struct keeping i2c driver data.
 */
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
	/** scl high low ratio register value */
	u16 scl_high_low_ratio;
	/** scl mismatch comparison register value */
	u16 scl_mis_comp_point;
	/** star stop ac timing register value */
	u16 sta_stop_ac_timing;
	u16 sda_timing;
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

/**
 * @brief transaction size limitation of I2C controller.
 */
static const struct i2c_adapter_quirks mt3612_i2c_quirks = {
	.max_num_msgs = 255,
};

/**
 * @brief hardward information of I2C controller.
 */
static const struct mtk_i2c_compatible mt3612_compat = {
	.quirks = &mt3612_i2c_quirks,
	.auto_restart = 1,
	.internal_div = 1,
	.mem_extension = 2,
};

/**
 * @brief I2C Open Framework Device ID.\n
 * Attach specific name to platform device with device tree.
 */
static const struct of_device_id mtk_i2c_of_match[] = {
	{ .compatible = "mediatek,mt3612-i2c", .data = &mt3612_compat },
	{}
};
MODULE_DEVICE_TABLE(of, mtk_i2c_of_match);

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Enable i2c module clock.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains i2c module\n
 *     clock information.
 * @return
 *     0, enable clock successfully.\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_clock_enable(struct mtk_i2c *i2c)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	int ret;

	ret = clk_prepare_enable(i2c->clk_main);
	if (ret)
		return ret;

	return 0;
#else
	return 0;
#endif
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Disable i2c module clock.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains i2c module\n
 *     clock information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_i2c_clock_disable(struct mtk_i2c *i2c)
{
#if defined(CONFIG_COMMON_CLK_MT3612)
	clk_disable_unprepare(i2c->clk_main);
#endif
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Initialize i2c hardware, soft reset i2c controller, then\n
 *     configure io mode, speed and control registers.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address, ioconfig and i2c hardware information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_i2c_init_hw(struct mtk_i2c *i2c)
{
	u16 control_reg;

	writew(I2C_SOFT_RST, i2c->base + OFFSET_SOFTRESET);

	/* Set ioconfig */
	if (i2c->use_push_pull)
		writew(I2C_IO_CONFIG_PUSH_PULL, i2c->base + OFFSET_IO_CONFIG);
	else
		writew(I2C_IO_CONFIG_OPEN_DRAIN, i2c->base + OFFSET_IO_CONFIG);

	if (i2c->dev_comp->internal_div)
		writew((i2c->clk_inter_div - 1), i2c->base + OFFSET_CLOCK_DIV);

	writew(i2c->timing_reg, i2c->base + OFFSET_TIMING);

	writew(i2c->scl_high_low_ratio, i2c->base + OFFSET_SCL_HIGH_LOW_RATIO);
	writew(i2c->scl_mis_comp_point, i2c->base + OFFSET_SCL_MIS_COMP_POINT);
	writew(i2c->sta_stop_ac_timing, i2c->base + OFFSET_STA_STOP_AC_TIMING);
	writew(i2c->sda_timing, i2c->base + OFFSET_SDA_TIMING);
	control_reg = I2C_CONTROL_DEFAULT | I2C_CONTROL_ACKERR_DET_EN |
		      I2C_CONTROL_CLK_EXT_EN;
#ifdef CONFIG_MACH_FPGA
	if ((i2c->id >= 11) && (i2c->id <= 16))
		control_reg &= ~(I2C_CONTROL_CLK_EXT_EN);
#endif
	writew(control_reg, i2c->base + OFFSET_CONTROL);

	writew(I2C_DELAY_LEN, i2c->base + OFFSET_DELAY_LEN);
}

 /** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Calculate i2c speed.\n
 *     Hardware design:\n
 *     i2c_bus_freq = source_clk / (2 * sample_cnt * step_cnt)\n
 *     The calculation want to pick the highest bus frequency that\n
 *     is still less than or equal to i2c->speed_hz. The\n
 *     calculation try to get sample_cnt and step_cnt.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains i2c driver data.
 * @param[in]
 *     clk_src: i2c module source clock.
 * @param[in]
 *     target_speed: i2c target speed.
 * @param[out]
 *     timing_step_cnt: i2c step_cnt value.
 * @param[out]
 *     timing_sample_cnt: i2c sample_cnt value.
 * @return
 *     0, calculate speed successfully.\n
 *     -EINVAL, calculate speed fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Target speed is too low, calculate speed fail, return\n
 *     -EINVAL.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_calculate_speed(struct mtk_i2c *i2c, unsigned int clk_src,
				   unsigned int target_speed,
				   unsigned int *timing_step_cnt,
				   unsigned int *timing_sample_cnt)
{
	unsigned int step_cnt;
	unsigned int sample_cnt;
	unsigned int max_step_cnt;
	unsigned int base_sample_cnt = MAX_SAMPLE_CNT_DIV;
	unsigned int base_step_cnt;
	unsigned int opt_div;
	unsigned int best_mul;
	unsigned int cnt_mul;

	max_step_cnt = MAX_STEP_CNT_DIV;
	base_step_cnt = max_step_cnt;
	/* Find the best combination */
	opt_div = DIV_ROUND_UP(clk_src >> 1, target_speed);
	best_mul = MAX_SAMPLE_CNT_DIV * max_step_cnt;

	/* Search for the best pair (sample_cnt, step_cnt) with
	 * 0 < sample_cnt < MAX_SAMPLE_CNT_DIV
	 * 0 < step_cnt < max_step_cnt
	 * sample_cnt * step_cnt >= opt_div
	 * optimizing for sample_cnt * step_cnt being minimal
	 */
	for (sample_cnt = 1; sample_cnt <= MAX_SAMPLE_CNT_DIV; sample_cnt++) {
		step_cnt = DIV_ROUND_UP(opt_div, sample_cnt);
		cnt_mul = step_cnt * sample_cnt;
		if (step_cnt > max_step_cnt)
			continue;

		if (cnt_mul < best_mul) {
			best_mul = cnt_mul;
			base_sample_cnt = sample_cnt;
			base_step_cnt = step_cnt;
			if (best_mul == opt_div)
				break;
		}
	}

	sample_cnt = base_sample_cnt;
	step_cnt = base_step_cnt;

	if ((clk_src / (2 * sample_cnt * step_cnt)) > target_speed) {
		/* In this case, hardware can't support such
		 * low i2c_bus_freq
		 */
		dev_dbg(i2c->dev, "Unsupported speed (%uhz)\n",	target_speed);
		return -EINVAL;
	}

	*timing_step_cnt = step_cnt - 1;
	*timing_sample_cnt = sample_cnt - 1;

	return 0;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Calculate i2c speed and store the sample_cnt and step_cnt.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains members which
 *     can store sample_cnt and step_cnt.
 * @param[in]
 *     parent_clk: i2c module source clock.
 * @return
 *     0, set speed successfully.\n
 *     error code from mtk_i2c_calculate_speed().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If mtk_i2c_calculate_speed() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_set_speed(struct mtk_i2c *i2c, unsigned int parent_clk)
{
	unsigned int clk_src;
	unsigned int step_cnt;
	unsigned int sample_cnt;
	unsigned int target_speed;
	int ret;

	clk_src = parent_clk / i2c->clk_src_div;
	target_speed = i2c->speed_hz;

	ret = mtk_i2c_calculate_speed(i2c, clk_src, target_speed,
				      &step_cnt, &sample_cnt);
	if (ret < 0)
		return ret;

	i2c->timing_reg = (sample_cnt << 8) | step_cnt;

	return 0;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Unmap dma memory of i2c and copy data to i2c_msg data buffer\n
 *     after read operation complete.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains operation\n
 *     mode, i2c_msg, interrupt status and dma information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_i2c_dma_unmap(struct mtk_i2c *i2c)
{
	struct dma_chan *chan = (i2c->op == I2C_MASTER_WR) ?
				i2c->dma_tx : i2c->dma_rx;
	enum dma_data_direction map_dir = (i2c->op == I2C_MASTER_WR) ?
					  DMA_TO_DEVICE :
					  ((i2c->op == I2C_MASTER_RD) ?
					  DMA_FROM_DEVICE : DMA_BIDIRECTIONAL);
	unsigned int sg_len = sg_dma_len(&i2c->sg);
	u16 dma_len = (sg_len & 0xFFFF) > ((sg_len >> 16) & 0xFFFF) ?
		      (sg_len & 0xFFFF) : ((sg_len >> 16) & 0xFFFF);
	u16 data_len;
	unsigned char *data_buf;

	dma_unmap_single(chan->device->dev, sg_dma_address(&i2c->sg), dma_len,
			 map_dir);
	i2c->dma_direction = DMA_NONE;

	if (i2c->op != I2C_MASTER_WR) {
		data_len = (i2c->op == I2C_MASTER_RD) ? i2c->msgs->len :
				   (i2c->msgs + 1)->len;
		data_buf = (i2c->op == I2C_MASTER_RD) ? i2c->msgs->buf :
				   (i2c->msgs + 1)->buf;
		memcpy(data_buf, i2c->v_buf, data_len);
	}

	kfree(i2c->v_buf);

	i2c->irq_stat |= BIT(14);
	if (i2c->irq_stat & BIT(15))
		complete(&i2c->msg_complete);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Terminate dmaengine of i2c and unmap dma memory.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains dma information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_i2c_cleanup_dma(struct mtk_i2c *i2c)
{
	if (i2c->dma_direction == DMA_NONE)
		return;
	else if (i2c->dma_direction == DMA_FROM_DEVICE)
		dmaengine_terminate_all(i2c->dma_rx);
	else if (i2c->dma_direction == DMA_TO_DEVICE)
		dmaengine_terminate_all(i2c->dma_tx);

	mtk_i2c_dma_unmap(i2c);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     i2c dma callback function, unmap dma memory of i2c.
 * @param[out]
 *     data: mtk_i2c pointer, struct mtk_i2c contains dma information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_i2c_dma_callback(void *data)
{
	struct mtk_i2c *i2c = data;

	mtk_i2c_dma_unmap(i2c);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Map dma memory and use dmaengine for i2c.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains operation\n
 *     mode and dma information.
 * @param[out]
 *     msgs: i2c_msg pointer, struct i2c_msg contains slave\n
 *     address, operation mode, msg length and data buffer.
 * @return
 *     0, dma config and start successfully.\n
 *     -ENOMEM, memory allocate or dma_map_single() or\n
 *     dmaengine_prep_slave_sg() or dma_submit_error() fails.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If memory allocate or dma_map_single() or\n
 *     dmaengine_prep_slave_sg() or dma_submit_error() fails,\n
 *     return -ENOMEM.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_dma(struct mtk_i2c *i2c, struct i2c_msg *msgs)
{
	enum dma_transfer_direction dir = (i2c->op == I2C_MASTER_WR) ?
					  DMA_MEM_TO_DEV :
					  DMA_DEV_TO_MEM;
	enum dma_data_direction map_dir = (i2c->op == I2C_MASTER_WR) ?
					  DMA_TO_DEVICE :
					  ((i2c->op == I2C_MASTER_RD) ?
					  DMA_FROM_DEVICE : DMA_BIDIRECTIONAL);
	struct dma_chan *chan = (i2c->op == I2C_MASTER_WR) ?
				i2c->dma_tx : i2c->dma_rx;
	struct dma_async_tx_descriptor *txdesc;
	dma_addr_t dma_addr;
	dma_cookie_t cookie;
	u16 dma_len = (i2c->op == I2C_MASTER_WRRD) ?
		      ((msgs->len > (msgs + 1)->len) ?
		      msgs->len : (msgs + 1)->len) : msgs->len;
	int len;

	i2c->v_buf = kzalloc(dma_len, GFP_KERNEL);
	if (!i2c->v_buf)
		return -ENOMEM;

	if (i2c->op == I2C_MASTER_WR) {
		memcpy(i2c->v_buf, msgs->buf, msgs->len);
		len = msgs->len;
	} else if (i2c->op == I2C_MASTER_RD) {
		len = msgs->len;
	} else {
		memcpy(i2c->v_buf, msgs->buf, msgs->len);
		len = (msgs->len << 16) | (msgs + 1)->len;
	}
	dma_addr = dma_map_single(chan->device->dev, i2c->v_buf, dma_len,
				  map_dir);
	if (dma_mapping_error(chan->device->dev, dma_addr)) {
		dev_dbg(chan->device->dev, "dma map failed, using PIO\n");
		kfree(i2c->v_buf);
		return -ENOMEM;
	}
	sg_dma_len(&i2c->sg) = len;
	sg_dma_address(&i2c->sg) = dma_addr;
	i2c->dma_direction = (i2c->op == I2C_MASTER_WR) ?
			     DMA_TO_DEVICE : DMA_FROM_DEVICE;
	txdesc = dmaengine_prep_slave_sg(chan, &i2c->sg, 1, dir,
					 DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	if (!txdesc) {
		dev_dbg(chan->device->dev, "dma prep slave sg failed, using PIO\n");
		mtk_i2c_cleanup_dma(i2c);
		return -ENOMEM;
	}

	txdesc->callback = mtk_i2c_dma_callback;
	txdesc->callback_param = i2c;
	cookie = dmaengine_submit(txdesc);
	if (dma_submit_error(cookie)) {
		dev_dbg(chan->device->dev, "submitting dma failed, using PIO\n");
		mtk_i2c_cleanup_dma(i2c);
		return -ENOMEM;
	}
	dma_async_issue_pending(chan);

	return 0;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Request dma channel for i2c.
 * @param[out]
 *     dev: device point.
 * @param[in]
 *     dir: dma_transfer_direction.
 * @param[in]
 *     port_addr: i2c fifo port address.
 * @return
 *     dma_chan pointer, request dma channel successfully.\n
 *     NULL, dma_request_slave_channel() or\n
 *     dmaengine_slave_config() fails.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dma_request_slave_channel() or\n
 *     dmaengine_slave_config() fails, return NULL.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static struct dma_chan *mtk_i2c_request_dma_chan(struct device *dev,
						 enum dma_transfer_direction
						 dir, dma_addr_t port_addr)
{
	struct dma_chan *chan;
	struct dma_slave_config cfg;
	char *chan_name = "apdma-i2c";
	int ret;

	chan = dma_request_slave_channel(dev, chan_name);
	if (!chan) {
		dev_dbg(dev, "request DMA channel failed!\n");
		return chan;
	}

	if (IS_ERR(chan)) {
		ret = PTR_ERR(chan);
		dev_dbg(dev, "request_channel failed for %s (%d)\n",
			chan_name, ret);
		return chan;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.direction = dir;
	if (dir == DMA_MEM_TO_DEV) {
		cfg.dst_addr = port_addr;
		cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	} else {
		cfg.src_addr = port_addr;
		cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	}

	ret = dmaengine_slave_config(chan, &cfg);
	if (ret) {
		dev_dbg(dev, "slave_config failed for %s (%d)\n",
			chan_name, ret);
		dma_release_channel(chan);
		chan = NULL;
		return chan;
	}

	dev_dbg(dev, "got DMA channel for %s\n", chan_name);
	return chan;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Get dma channel for i2c.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains operation\n
 *     mode, dma information and i2c driver data.
 * @return
 *     0, get dma channel successfully.\n
 *     -EBUSY, dma_chan is used.\n
 *     -EBADR, mtk_i2c_request_dma_chan() fails.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If dma_chan is used, return -EBUSY.\n
 *     2. If mtk_i2c_request_dma_chan() fails, return -EBADR.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_request_dma(struct mtk_i2c *i2c)
{
	struct dma_chan *chan;
	enum dma_transfer_direction dir;

	chan = (i2c->op == I2C_MASTER_WR) ? i2c->dma_tx : i2c->dma_rx;
	if (PTR_ERR(chan) != -EPROBE_DEFER) {
		i2c->dma_en = false;
		return -EBUSY;
	}

	dir = (i2c->op == I2C_MASTER_WR) ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM;
	chan = mtk_i2c_request_dma_chan(i2c->dev, dir, (dma_addr_t)(i2c->base +
					OFFSET_DATA_PORT));

	if (!chan) {
		i2c->dma_rx = ERR_PTR(-EPROBE_DEFER);
		i2c->dma_tx = ERR_PTR(-EPROBE_DEFER);
		i2c->dma_en = false;
		return -EBADR;
	}

	if (i2c->op == I2C_MASTER_WR)
		i2c->dma_tx = chan;
	else
		i2c->dma_rx = chan;

	i2c->dma_en = true;

	return 0;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Release dma channel.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains dma information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_i2c_release_dma(struct mtk_i2c *i2c)
{
	if (!IS_ERR(i2c->dma_tx)) {
		dma_release_channel(i2c->dma_tx);
		i2c->dma_tx = ERR_PTR(-EPROBE_DEFER);
	}

	if (!IS_ERR(i2c->dma_rx)) {
		dma_release_channel(i2c->dma_rx);
		i2c->dma_rx = ERR_PTR(-EPROBE_DEFER);
	}
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Config i2c register and trigger transfer.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address, operation mode, interrupt status and i2c driver data.
 * @param[out]
 *     msgs: i2c_msg pointer, struct i2c_msg contains slave\n
 *     address, operation mode, msg length and data buffer.
 * @param[in]
 *     num: i2c_msg number.
 * @param[in]
 *     left_num: left i2c_msg number.
 * @return
 *     0, i2c transfer successfully.\n
 *     -ENOMEM, memory allocate or dma_map_single() fails.\n
 *     -ETIMEDOUT, i2c transfer timeout.\n
 *     -ENXIO, i2c transfer ack error.\n
 *     -EIO, i2c receive data length does not equal to request data\n
 *     length.\n
 *     error code from mtk_i2c_dma().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Memory allocate or dma_map_single() fails, return -ENOMEM.\n
 *     2. i2c transfer timeout, return -ETIMEDOUT.\n
 *     3. i2c transfer ack error, return -ENXIO.\n
 *     4. i2c receive data length does not equal to request data\n
 *     length, return -EIO.\n
 *     5. If mtk_i2c_dma() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_do_transfer(struct mtk_i2c *i2c, struct i2c_msg *msgs,
			       int num, int left_num)
{
	u8 *data_buf;
	u16 data_len;
	u16 read_len;
	u16 addr_reg;
	u16 start_reg;
	u16 control_reg;
	u16 restart_flag = 0;
	int ret;
#ifdef CONFIG_MACH_FPGA
	unsigned int tmo_poll = 0xffff;
#endif

	i2c->irq_stat = 0;

	if (i2c->auto_restart)
		restart_flag = I2C_RS_TRANSFER;

	reinit_completion(&i2c->msg_complete);

	control_reg = readw(i2c->base + OFFSET_CONTROL) &
			~(I2C_CONTROL_DIR_CHANGE | I2C_CONTROL_RS);
	if (left_num >= 1)
		control_reg |= I2C_CONTROL_RS;

	if (i2c->op == I2C_MASTER_WRRD)
		control_reg |= I2C_CONTROL_DIR_CHANGE | I2C_CONTROL_RS;

	if (i2c->dma_en)
		control_reg |= I2C_CONTROL_AYNCS_MODE | I2C_CONTROL_DMA_EN;
	else
		control_reg &= ~(I2C_CONTROL_AYNCS_MODE | I2C_CONTROL_DMA_EN);

	writew(control_reg, i2c->base + OFFSET_CONTROL);

	/* set start condition */
	if (i2c->speed_hz <= I2C_DEFAULT_SPEED)
		writew(I2C_ST_START_CON, i2c->base + OFFSET_EXT_CONF);
	else
		writew(I2C_FS_START_CON, i2c->base + OFFSET_EXT_CONF);

	addr_reg = msgs->addr << 1;
	if (i2c->op == I2C_MASTER_RD)
		addr_reg |= 0x1;

	writew(addr_reg, i2c->base + OFFSET_SLAVE_ADDR);

	/* Clear interrupt status */
	writew(restart_flag | I2C_ACKERR | I2C_TRANSAC_COMP,
	       i2c->base + OFFSET_INTR_STAT);
	writew(I2C_FIFO_ADDR_CLR, i2c->base + OFFSET_FIFO_ADDR_CLR);

#ifdef CONFIG_MACH_FPGA
	if (i2c->poll_en)
		writew(0, i2c->base + OFFSET_INTR_MASK);
	else
#endif
		/* Enable interrupt */
		writew(restart_flag | I2C_ACKERR | I2C_TRANSAC_COMP,
		       i2c->base + OFFSET_INTR_MASK);

	/* Set transfer and transaction len */
	if (i2c->op == I2C_MASTER_WRRD) {
		writew(msgs->len, i2c->base + OFFSET_TRANSFER_LEN);
		writew((msgs + 1)->len, i2c->base + OFFSET_TRANSFER_LEN_AUX);
		writew(I2C_WRRD_TRANAC_VALUE, i2c->base + OFFSET_TRANSAC_LEN);
	} else {
		writew(msgs->len, i2c->base + OFFSET_TRANSFER_LEN);
		writew(num, i2c->base + OFFSET_TRANSAC_LEN);
	}

	/* Prepare buffer data to start transfer */
	if (i2c->dma_en) {
		ret = mtk_i2c_dma(i2c, msgs);
		if (ret < 0)
			return ret;
	} else {
		if (i2c->op != I2C_MASTER_RD) {
			data_buf = msgs->buf;
			data_len = msgs->len;

			while (data_len--)
				writew_relaxed(*(data_buf++),
					       i2c->base + OFFSET_DATA_PORT);
		}
	}

	if (!i2c->auto_restart) {
		start_reg = I2C_TRANSAC_START;
	} else {
		start_reg = I2C_TRANSAC_START | I2C_RS_MUL_TRIG;
		if (left_num >= 1)
			start_reg |= I2C_RS_MUL_CNFG;
	}
	writew(start_reg, i2c->base + OFFSET_START);

#ifdef CONFIG_MACH_FPGA
	if (i2c->poll_en) {
		for (;;) {
			i2c->irq_stat = readw(i2c->base + OFFSET_INTR_STAT);

			if (i2c->irq_stat & (I2C_TRANSAC_COMP | restart_flag)) {
				ret = 1;
				break;
			}

			tmo_poll--;
			if (tmo_poll == 0) {
				ret = 0;
				break;
			}
		}
	} else
#endif
		ret = wait_for_completion_timeout(&i2c->msg_complete,
						  i2c->adap.timeout);

	/* Clear interrupt mask */
	writew(~(restart_flag | I2C_ACKERR | I2C_TRANSAC_COMP),
	       i2c->base + OFFSET_INTR_MASK);

	if (ret == 0) {
		dev_dbg(i2c->dev, "addr: %x, transfer timeout\n", msgs->addr);
		mtk_i2c_init_hw(i2c);
		return -ETIMEDOUT;
	}

	completion_done(&i2c->msg_complete);

	if (i2c->irq_stat & I2C_ACKERR) {
		dev_dbg(i2c->dev, "addr: %x, transfer ACK error\n", msgs->addr);
		mtk_i2c_init_hw(i2c);
		return -ENXIO;
	}

	if (!i2c->dma_en && i2c->op != I2C_MASTER_WR) {
		data_buf = (i2c->op == I2C_MASTER_RD) ? msgs->buf :
			   (msgs + 1)->buf;
		data_len = (i2c->op == I2C_MASTER_RD) ? msgs->len :
			   (msgs + 1)->len;
		read_len = readw(i2c->base + OFFSET_FIFO_STAT1) & 0x1f;

		if (read_len == data_len) {
			while (data_len--)
				*(data_buf++) = readw_relaxed(i2c->base +
							      OFFSET_DATA_PORT);
		} else {
			dev_dbg(i2c->dev,
				"data_len %x, read_len %x, fifo read error\n",
				data_len, read_len);
			mtk_i2c_init_hw(i2c);
			return -EIO;
		}
	}

	return 0;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     The callback of i2c_algorithm master_xfer for i2c core. Set\n
 *     i2c transfer mode according to i2c_msg and i2c hardware\n
 *     information, then call mtk_i2c_do_transfer() to config i2c\n
 *     register and trigger transfer.
 * @param[out]
 *     adap: i2c_adapter pointer, struct i2c_adapter contains\n
 *     adapter data struct mtk_i2c.
 * @param[out]
 *     msgs: i2c_msg pointer, struct i2c_msg contains slave\n
 *     address, operation mode, msg length and data buffer.
 * @param[in]
 *     num: i2c_msg number.
 * @return
 *     i2c_msg number, i2c transfer successfully.\n
 *     -EINVAL, msg length is 0 or msg data buffer is NULL.\n
 *     error code from mtk_i2c_clock_enable().\n
 *     error code from mtk_i2c_request_dma().\n
 *     error code from mtk_i2c_do_transfer().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If msg length is 0 or msg data buffer is NULL, return\n
 *     -EINVAL.
 *     2. If mtk_i2c_clock_enable() fails, return its error code.\n
 *     3. If mtk_i2c_request_dma() fails, return its error code.\n
 *     4. If mtk_i2c_do_transfer() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_transfer(struct i2c_adapter *adap,
			    struct i2c_msg msgs[], int num)
{
	int ret;
	int left_num = num;
	int num_cnt;
	struct mtk_i2c *i2c = i2c_get_adapdata(adap);

	for (num_cnt = 0; num_cnt < num; num_cnt++) {
		if (!msgs[num_cnt].len)
			return -EINVAL;
	}

	ret = mtk_i2c_clock_enable(i2c);
	if (ret)
		return ret;

	i2c->main_control = true;
	i2c->auto_restart = i2c->dev_comp->auto_restart;

	/* checking if we can skip restart and optimize using WRRD mode */
	if (i2c->auto_restart && num == 2) {
		if (!(msgs[0].flags & I2C_M_RD) && (msgs[1].flags & I2C_M_RD) &&
		    msgs[0].addr == msgs[1].addr) {
			i2c->auto_restart = 0;
		}
	}

	while (left_num--) {
		if (!msgs->buf) {
			dev_dbg(i2c->dev, "data buffer is NULL.\n");
			ret = -EINVAL;
			goto err_exit;
		}

		if (msgs->flags & I2C_M_RD)
			i2c->op = I2C_MASTER_RD;
		else
			i2c->op = I2C_MASTER_WR;

		if (!i2c->auto_restart) {
			if (num > 1) {
				/* combined two messages into one transaction */
				i2c->op = I2C_MASTER_WRRD;
				left_num--;
			}
		}

		if ((msgs->len > i2c->fifo_max_size) ||
		    ((i2c->op == I2C_MASTER_WRRD) &&
		    ((msgs + 1)->len > i2c->fifo_max_size))) {
			i2c->msgs = msgs;

			ret = mtk_i2c_request_dma(i2c);
			if (ret < 0)
				goto err_exit;
		} else
			i2c->dma_en = false;

		/* len <= fifo_size: FIFO mode, len > fifo_size: DMA mode. */
		ret = mtk_i2c_do_transfer(i2c, msgs, num, left_num);

		if (i2c->dma_en)
			mtk_i2c_release_dma(i2c);

		if (ret < 0)
			goto err_exit;

		msgs++;
	}
	/* the return value is number of executed messages */
	ret = num;

err_exit:
	i2c->main_control = false;
	mtk_i2c_clock_disable(i2c);
	return ret;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Interrupt function of i2c driver.
 * @param[in]
 *     irqno: interrupt number.
 * @param[out]
 *     dev_id: mtk_i2c pointer, struct mtk_i2c contains register\n
 *     base address, operation mode, interrupt status and i2c\n
 *     driver data.
 * @return
 *     IRQ_HANDLED, interrupt function always return this value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static irqreturn_t mtk_i2c_irq(int irqno, void *dev_id)
{
	struct mtk_i2c *i2c = dev_id;
	u16 restart_flag = 0;
	u16 intr_stat;

	if ((!i2c->main_control) && ((i2c->id >= 17) && (i2c->id <= 22))) {
		dev_dbg(i2c->dev, "no main_control id=%d, intr_stat=0x%x.\n",
			i2c->id, readw(i2c->base + OFFSET_INTR_STAT));
		return IRQ_HANDLED;
	}

	if (i2c->auto_restart)
		restart_flag = I2C_RS_TRANSFER;

	intr_stat = readw(i2c->base + OFFSET_INTR_STAT);
	writew(intr_stat, i2c->base + OFFSET_INTR_STAT);

	/*
	 * when occurs ack error, i2c controller generate two interrupts
	 * first is the ack error interrupt, then the complete interrupt
	 * i2c->irq_stat need keep the two interrupt value.
	 */
	i2c->irq_stat |= intr_stat;

	if (i2c->irq_stat & (I2C_TRANSAC_COMP | restart_flag)) {
		if (i2c->dma_en) {
			i2c->irq_stat |= BIT(15);
			if (i2c->irq_stat & BIT(14))
				complete(&i2c->msg_complete);
		} else
			complete(&i2c->msg_complete);
	}

	return IRQ_HANDLED;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     The callback of i2c_algorithm functionality for i2c core. It\n
 *     shows functions which i2c driver supports.
 * @param[in]
 *     adap: i2c_adapter pointer, not used.
 * @return
 *     I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL, the functions which i2c\n
 *     driver supports.
 * @par Boundary case and Limitation
 *      none.
 * @par Error case and Error handling
 *      none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static u32 mtk_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

/**
 * @brief set callback functions to the members of struct\n
 * i2c_algorithm.
 */
static const struct i2c_algorithm mtk_i2c_algorithm = {
	.master_xfer = mtk_i2c_transfer,
	.functionality = mtk_i2c_functionality,
};

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Parse i2c device tree information.
 * @param[in]
 *     np: i2c device_node.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains i2c bus\n
 *     number, speed, clock div and ioconfig information.
 * @return
 *     0, parse i2c device tree information successfully.\n
 *     -EINVAL, clock div is 0.\n
 *     error code from of_property_read_u32().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If clock div is 0, return -EINVAL.\n
 *     2. If of_property_read_u32() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_parse_dt(struct device_node *np, struct mtk_i2c *i2c)
{
	u16 i2c_timings[4];
	int ret;

	ret = of_property_read_u32(np, "id", &i2c->id);
	if (ret < 0)
		return ret;

	ret = of_property_read_u32(np, "clock-frequency", &i2c->speed_hz);
	if (ret < 0)
		i2c->speed_hz = I2C_DEFAULT_SPEED;

	ret = of_property_read_u32(np, "clock-div", &i2c->clk_src_div);
	if (ret < 0)
		return ret;

	if (i2c->clk_src_div == 0)
		return -EINVAL;

	if (i2c->dev_comp->internal_div) {
		ret = of_property_read_u32(np, "clock-inter-div",
					   &i2c->clk_inter_div);
		if (ret < 0)
			i2c->clk_inter_div = I2C_DEFAULT_CLK_DIV;

		if (i2c->clk_inter_div == 0)
			return -EINVAL;
	}
	ret = of_property_read_u16_array(np, "i2c-timings", i2c_timings,
					 ARRAY_SIZE(i2c_timings));
	if (!ret) {
		i2c->scl_high_low_ratio = i2c_timings[0];
		i2c->scl_mis_comp_point = i2c_timings[1];
		i2c->sta_stop_ac_timing = i2c_timings[2];
		i2c->sda_timing = i2c_timings[3];
	} else {
		i2c->scl_high_low_ratio = I2C_SCL_HIGH_LOW_RATIO_DEFAULT;
		i2c->scl_mis_comp_point = I2C_SCL_MIS_COMP_POINT_DEFAULT;
		i2c->sta_stop_ac_timing = I2C_STA_STOP_AC_TIMING_DEFAULT;
		i2c->sda_timing = I2C_SDA_TIMING_DEFAULT;
	}

	i2c->use_push_pull = of_property_read_bool(np,
						   "mediatek,use-push-pull");

	return 0;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     i2c probe function.
 * @param[out]
 *     pdev: platform_device pointer.
 * @return
 *     0, i2c driver probe successfully.\n
 *     -ENOMEM, memory allocate fails.\n
 *     -EINVAL, of_match_node() or mtk_i2c_parse_dt() or\n
 *     mtk_i2c_set_speed() fails.\n
 *     PTR_ERR(), devm_ioremap_resource() or devm_clk_get() fails.\n
 *     error code from platform_get_irq().\n
 *     error code from dma_set_mask().\n
 *     error code from mtk_i2c_clock_enable().\n
 *     error code from devm_request_irq().\n
 *     error code from i2c_add_numbered_adapter().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Memory allocate fails, return -ENOMEM.\n
 *     2. If of_match_node() or mtk_i2c_parse_dt() or\n
 *     mtk_i2c_set_speed() fails, return -EINVAL.\n
 *     3. If devm_ioremap_resource() or devm_clk_get() fails,\n
 *     return PTR_ERR().\n
 *     4. If platform_get_irq() fails, return its error code.\n
 *     5. If dma_set_mask() fails, return its error code.\n
 *     6. If mtk_i2c_clock_enable() fails, return its error code.\n
 *     7. If devm_request_irq() fails, return its error code.\n
 *     8. If i2c_add_numbered_adapter() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct mtk_i2c *i2c;
	struct resource *res;
	int irq;
	int ret = 0;

	i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c->base))
		return PTR_ERR(i2c->base);

	sg_init_table(&i2c->sg, 1);
	i2c->dma_direction = DMA_NONE;
	i2c->dma_rx = ERR_PTR(-EPROBE_DEFER);
	i2c->dma_tx = ERR_PTR(-EPROBE_DEFER);

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0)
		return irq;

	init_completion(&i2c->msg_complete);

	of_id = of_match_node(mtk_i2c_of_match, pdev->dev.of_node);
	if (!of_id)
		return -EINVAL;

	i2c->dev_comp = of_id->data;
	i2c->adap.dev.of_node = pdev->dev.of_node;
	i2c->dev = &pdev->dev;
	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &mtk_i2c_algorithm;
	i2c->adap.quirks = i2c->dev_comp->quirks;
	i2c->adap.timeout = 2 * HZ;
	i2c->adap.retries = 1;

	strlcpy(i2c->adap.name, I2C_DRV_NAME, sizeof(i2c->adap.name));

	ret = mtk_i2c_parse_dt(pdev->dev.of_node, i2c);
	if (ret)
		return -EINVAL;

	i2c->adap.nr = i2c->id;

	if (i2c->dev_comp->internal_div)
		i2c->clk_src_div *= i2c->clk_inter_div;

#if defined(CONFIG_COMMON_CLK_MT3612)
	i2c->clk_top_source = devm_clk_get(&pdev->dev, "top-source");
	if (IS_ERR(i2c->clk_top_source)) {
		dev_err(&pdev->dev, "cannot get top source clock\n");
		return PTR_ERR(i2c->clk_top_source);
	}

	i2c->clk_top_sel = devm_clk_get(&pdev->dev, "top-sel");
	if (IS_ERR(i2c->clk_top_sel)) {
		dev_err(&pdev->dev, "cannot get top sel clock\n");
		return PTR_ERR(i2c->clk_top_sel);
	}

	ret = clk_set_parent(i2c->clk_top_sel, i2c->clk_top_source);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to top clk_set_parent(%d)\n", ret);
		return ret;
	}

	i2c->clk_peri_source = devm_clk_get(&pdev->dev, "peri-source");
	if (IS_ERR(i2c->clk_peri_source)) {
		dev_err(&pdev->dev, "cannot get peri source clock\n");
		return PTR_ERR(i2c->clk_peri_source);
	}

	i2c->clk_peri_sel = devm_clk_get(&pdev->dev, "peri-sel");
	if (IS_ERR(i2c->clk_peri_sel)) {
		dev_err(&pdev->dev, "cannot get peri sel clock\n");
		return PTR_ERR(i2c->clk_peri_sel);
	}

	ret = clk_set_parent(i2c->clk_peri_sel, i2c->clk_peri_source);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to peri clk_set_parent(%d)\n", ret);
		return ret;
	}

	i2c->clk_main = devm_clk_get(&pdev->dev, "main");
	if (IS_ERR(i2c->clk_main)) {
		dev_err(&pdev->dev, "cannot get main clock\n");
		return PTR_ERR(i2c->clk_main);
	}

	ret = mtk_i2c_set_speed(i2c, clk_get_rate(i2c->clk_main));
	if (ret) {
		dev_err(&pdev->dev, "Failed to set the speed.\n");
		return -EINVAL;
	}
#else
	ret = mtk_i2c_set_speed(i2c, I2C_SOURCE_CLOCK);
	if (ret) {
		dev_err(&pdev->dev, "Failed to set the speed.\n");
		return -EINVAL;
	}
#endif

#ifdef CONFIG_MACH_FPGA
	if ((i2c->id >= 11) && (i2c->id <= 16))
		i2c->poll_en = true;
	else
		i2c->poll_en = false;
#endif

	i2c->fifo_max_size = I2C_FIFO_SIZE;

	if (i2c->dev_comp->mem_extension == 1) {
		ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(33));
		if (ret) {
			dev_err(&pdev->dev, "dma_set_mask return error.\n");
			return ret;
		}
	} else if (i2c->dev_comp->mem_extension == 2) {
		ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(36));
		if (ret) {
			dev_err(&pdev->dev, "dma_set_mask return error.\n");
			return ret;
		}
	}

	ret = mtk_i2c_clock_enable(i2c);
	if (ret) {
		dev_err(&pdev->dev, "clock enable failed!\n");
		return ret;
	}
	mtk_i2c_init_hw(i2c);
	mtk_i2c_clock_disable(i2c);

	ret = devm_request_irq(&pdev->dev, irq, mtk_i2c_irq,
			       IRQF_TRIGGER_NONE, I2C_DRV_NAME, i2c);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"Request I2C IRQ %d fail\n", irq);
		return ret;
	}

	i2c_set_adapdata(&i2c->adap, i2c);
	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add i2c bus to i2c core\n");
		return ret;
	}

	platform_set_drvdata(pdev, i2c);

	return 0;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     i2c remove function, it is used to close i2c driver.
 * @param[in]
 *     pdev: platform_device pointer.
 * @return
 *     0, remove successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_remove(struct platform_device *pdev)
{
	struct mtk_i2c *i2c = platform_get_drvdata(pdev);

	if (i2c->dma_en)
		mtk_i2c_release_dma(i2c);

	i2c_del_adapter(&i2c->adap);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     i2c resume function, initialize i2c hardware.
 * @param[out]
 *     dev: device pointer, struct device contains driver data\n
 *     struct mtk_i2c.
 * @return
 *     0, i2c resume successfully.\n
 *     error code from mtk_i2c_clock_enable().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If mtk_i2c_clock_enable() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_resume(struct device *dev)
{
	int ret;
	struct mtk_i2c *i2c = dev_get_drvdata(dev);

	/* enable i2c clock to sync i2c register configuration */
	ret = mtk_i2c_clock_enable(i2c);
	if (ret) {
		dev_err(dev, "clock enable failed!\n");
		return ret;
	}

	mtk_i2c_init_hw(i2c);

	mtk_i2c_clock_disable(i2c);

	return 0;
}
#endif

/**
 * @brief set power management of i2c driver.
 */
static const struct dev_pm_ops mtk_i2c_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(NULL, mtk_i2c_resume)
};

/**
 * @brief platform driver information of i2c which is used\n
 * to register i2c driver.
 */
static struct platform_driver mtk_i2c_driver = {
	.probe = mtk_i2c_probe,
	.remove = mtk_i2c_remove,
	.driver = {
		.name = I2C_DRV_NAME,
		.pm = &mtk_i2c_pm,
		.of_match_table = of_match_ptr(mtk_i2c_of_match),
	},
};

static int __init mtk_i2c_adap_init(void)
{
	return platform_driver_register(&mtk_i2c_driver);
}
subsys_initcall(mtk_i2c_adap_init);

static void __exit mtk_i2c_adap_exit(void)
{
	platform_driver_unregister(&mtk_i2c_driver);
}
module_exit(mtk_i2c_adap_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek I2C Bus Driver");
MODULE_AUTHOR("Xudong Chen <xudong.chen@mediatek.com>");
