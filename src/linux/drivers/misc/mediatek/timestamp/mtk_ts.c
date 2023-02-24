/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors: David Yeh <david.yeh@mediatek.com>
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
 * @file mtk_ts.c
 *     This timestamp driver is used to control MTK timestamp hardware module.
 */

/**
 * @defgroup IP_group_ts TimeStamp
 *     MTK timestamp hardware module is an engine that samples timestamp and
 *     its associated metadata from different sources such as DSI & I2S.\n
 *     The timestamp driver is aimed to provide the interface to setup
 *     timestamp hardware module and atomically get the timestamp\n
 *     and its associated metadata sampling from DSI, I2S, DP RX audio/video,
 *     sgz ISP/CameraSync and IMU pins.\n
 *     Timestamp includes 42-bit long 27MHz STC (System Time Clock) and
 *     32-bit long 1MHz VTS (Vision TimeStamp).\n
 *     Metadata includes DP RX Vsync/Hsync counters and IMU interrupt counter.
 *
 *     @{
 *         @defgroup IP_group_timestamp_external EXTERNAL
 *             The external API document for timestamp.
 *             @{
 *                 @defgroup IP_group_timestamp_external_function 1.function
 *                     External function for timestamp.
 *                 @defgroup IP_group_timestamp_external_struct 2.structure
 *                     None.
 *                 @defgroup IP_group_timestamp_external_typedef 3.typedef
 *                     None. Follow Linux coding style, no typedef used.
 *                 @defgroup IP_group_timestamp_external_enum 4.enumeration
 *                     External enumeration for timestamp.
 *                 @defgroup IP_group_timestamp_external_def 5.define
 *                     None.
 *             @}
 *
 *         @defgroup IP_group_timestamp_internal INTERNAL
 *             The internal API document for timestamp.
 *             @{
 *                 @defgroup IP_group_timestamp_internal_function 1.function
 *                     Internal function for timestamp and module init.
 *                 @defgroup IP_group_timestamp_internal_struct 2.structure
 *                     Internal structure used for timestamp.
 *                 @defgroup IP_group_timestamp_internal_typedef 3.typedef
 *                     None. Follow Linux coding style, no typedef used.
 *                 @defgroup IP_group_timestamp_internal_enum 4.enumeration
 *                     None. No enumeration in timestamp driver.
 *                 @defgroup IP_group_timestamp_internal_def 5.define
 *                     Internal definition used for timestamp.
 *             @}
 *     @}
 */

#include <asm-generic/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <soc/mediatek/mtk_ts.h>
#include "mtk_ts_int.h"

/** @ingroup IP_group_timestamp_internal_function
 * @par Description
 *     Register mask write common function.
 * @param[in]
 *     regs: register base.
 * @param[in]
 *     offset: register offset.
 * @param[in]
 *     value: write data value.
 * @param[in]
 *     mask: write mask value.
 * @return
 *     0, write successfully.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int reg_write_mask(void __iomem *regs, u32 offset, u32 value, u32 mask)
{
	u32 reg;

	reg = readl(regs + offset);
	reg &= ~mask;
	reg |= value;
	writel(reg, regs + offset);

	return 0;
}

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Set VTS value.
 * @param[in]
 *     dev: timestamp device node.
 * @param[in]
 *     vts: target VTS
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_set_vts(const struct device *dev, const u32 vts)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u32 cnt;

	if (!dev)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	cnt = readl(regs + TIMESTAMP_08);
	writel(vts - cnt, regs + TIMESTAMP_09);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_set_vts);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Set STC value.
 * @param[in]
 *     dev: timestamp device node.
 * @param[in]
 *     stc_base_31_0: bits[31:0] = stc_base[31:0]
 * @param[in]
 *     stc_base_32: bits[0:0] = stc_base[32:32]
 * @param[in]
 *     stc_ext_8_0: bits[8:0] = stc_ext[8:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.\n
 *     -EBUSY, if timeout.
 * @par Boundary case and Limitation
 *     Parameter, stc_base_32, is 0~1.\n
 *     Parameter, stc_ext_8_0, is 0~299.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.\n
 *     If set control ack has timeout, return -EBUSY.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_set_stc(const struct device *dev,
		   const u32 stc_base_31_0,
		   const u32 stc_base_32,
		   const u32 stc_ext_8_0)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	int value, i;

	if (!dev)
		return -EINVAL;

	if (stc_ext_8_0 > 299)
		dev_warn(dev, "unexpected stc_ext_8_0 %d!\n", stc_ext_8_0);

	if (stc_base_32 > 1)
		dev_warn(dev, "unexpected stc_base_32 %d!\n", stc_base_32);

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(STC_CNTL_CLR, regs + TIMESTAMP_11);
	writel(stc_base_31_0, regs + TIMESTAMP_13);
	value = (stc_base_32 << STC_LOAD_VALUE_BASE_32_SHIFTER) &
		STC_LOAD_VALUE_BASE_32_;
	value |= (stc_ext_8_0 & STC_LOAD_VALUE_EXT);
	writel(value, regs + TIMESTAMP_14);
	writel(STC_CNTL_LOAD, regs + TIMESTAMP_11);
	for (i = 0; i < ACK_STC_CNTL_POLLING_LOOP; i++) {
		value = readl(regs + TIMESTAMP_11) & ACK_STC_CNTL;
		if (value == ACK_STC_CNTL_LOAD)
			return 0;
	}

	dev_err(dev, "mtk_ts_set_stc wait stc ctrl ack timeout\n");
	return -EBUSY;
}
EXPORT_SYMBOL(mtk_ts_set_stc);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Select IMU0 trigger edge.
 * @param[in]
 *     dev: timestamp device node.
 * @param[in]
 *     edge: trigger edge\n
 *     0: fall+rise edge\n
 *     1: fall edge\n
 *     2: fall+rise edge\n
 *     3: rise edge
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     Parameter, edge, is 0~3.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_select_imu_0_trigger_edge(const struct device *dev,
				    const enum mtk_ts_imu_trigger_edge edge)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u32 v;

	if (edge < 0 || edge > 3)
		return -EINVAL;

	if (!dev)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;
	v = edge << IMU_TRIG_EDGE_SHIFTER;

	return reg_write_mask(regs, TS_IMU_0_1, v, TS_IMU_0_IMU_TRIG_EDGE);
}
EXPORT_SYMBOL(mtk_ts_select_imu_0_trigger_edge);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Select IMU1 trigger edge.
 * @param[in]
 *     dev: timestamp device node.
 * @param[in]
 *     edge: trigger edge\n
 *     0: fall+rise edge\n
 *     1: fall edge\n
 *     2: fall+rise edge\n
 *     3: rise edge
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     Parameter, edge, is 0~3.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_select_imu_1_trigger_edge(const struct device *dev,
				    const enum mtk_ts_imu_trigger_edge edge)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u32 v;

	if (edge < 0 || edge > 3)
		return -EINVAL;

	if (!dev)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;
	v = edge << IMU_TRIG_EDGE_SHIFTER;

	return reg_write_mask(regs, TS_IMU_1_1, v, TS_IMU_1_IMU_TRIG_EDGE);
}
EXPORT_SYMBOL(mtk_ts_select_imu_1_trigger_edge);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get current timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     stc: associated STC.\n
 *     stc_base[32:0] = stc[41:9]\n
 *     stc_ext[8:0] = stc[8:0]\n
 *     stc_27m = stc_base * 300 + stc_ext
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_current_time(const struct device *dev,
		u32 *vts, u64 *stc, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u64 stc_m, stc_l;

	if (!dev || !vts || !stc || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_CURRENT_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TIMESTAMP_03);
	*vts = readl(regs + TIMESTAMP_04);
	stc_l = readl(regs + TIMESTAMP_05) &
		TS_STC_IN_CURRENT_LSB;
	stc_m = readl(regs + TIMESTAMP_06);
	*stc = (stc_m << STC_MSB_SHIFTER) | stc_l;

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_current_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get DSI timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     stc: associated STC.\n
 *     stc_base[32:0] = stc[41:9]\n
 *     stc_ext[8:0] = stc[8:0]\n
 *     stc_27m = stc_base * 300 + stc_ext
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_dsi_time(const struct device *dev,
		u64 *stc, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u64 stc_m, stc_l;

	if (!dev || !stc || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_DSI_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_DSI_2);
	stc_l = readl(regs + TS_DSI_4) &
		STA_DSI_STC_CNT_LATCH_LSB;
	stc_m = readl(regs + TS_DSI_5);
	*stc = (stc_m << STC_MSB_SHIFTER) | stc_l;

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_dsi_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get audio HDMIDP timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     stc: associated STC.\n
 *     stc_base[32:0] = stc[41:9]\n
 *     stc_ext[8:0] = stc[8:0]\n
 *     stc_27m = stc_base * 300 + stc_ext
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_audio_hdmidp_time(const struct device *dev,
		u64 *stc, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u64 stc_m, stc_l;

	if (!dev || !stc || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_HDMIDP_IRQ_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_HDMIDP_IRQ_2);
	stc_l = readl(regs + TS_HDMIDP_IRQ_4) &
		STA_HDMIDP_IRQ_STC_CNT_LATCH_LSB;
	stc_m = readl(regs + TS_HDMIDP_IRQ_5);
	*stc = (stc_m << STC_MSB_SHIFTER) | stc_l;

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_audio_hdmidp_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get audio common timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     stc: associated STC.\n
 *     stc_base[32:0] = stc[41:9]\n
 *     stc_ext[8:0] = stc[8:0]\n
 *     stc_27m = stc_base * 300 + stc_ext
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_audio_common_time(const struct device *dev,
		u64 *stc, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u64 stc_m, stc_l;

	if (!dev || !stc || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_COMMON_IRQ_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_COMMON_IRQ_2);
	stc_l = readl(regs + TS_COMMON_IRQ_4) &
		STA_COMMON_IRQ_STC_CNT_LATCH_LSB;
	stc_m = readl(regs + TS_COMMON_IRQ_5);
	*stc = (stc_m << STC_MSB_SHIFTER) | stc_l;

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_audio_common_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get audio TDMOUT timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     stc: associated STC.\n
 *     stc_base[32:0] = stc[41:9]\n
 *     stc_ext[8:0] = stc[8:0]\n
 *     stc_27m = stc_base * 300 + stc_ext
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_audio_tdmout_time(const struct device *dev,
		u64 *stc, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u64 stc_m, stc_l;

	if (!dev || !stc || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_TDMOUT_IRQ_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_TDMOUT_IRQ_2);
	stc_l = readl(regs + TS_TDMOUT_IRQ_4) &
		STA_TDMOUT_IRQ_STC_CNT_LATCH_LSB;
	stc_m = readl(regs + TS_TDMOUT_IRQ_5);
	*stc = (stc_m << STC_MSB_SHIFTER) | stc_l;

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_audio_tdmout_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get DP RX video timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     stc: associated STC.\n
 *     stc_base[32:0] = stc[41:9]\n
 *     stc_ext[8:0] = stc[8:0]\n
 *     stc_27m = stc_base * 300 + stc_ext
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_dprx_video_time(const struct device *dev,
		u64 *stc, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u64 stc_m, stc_l;

	if (!dev || !stc || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_DPRX_VIDEO_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_DPRX_VIDEO_IRQ_2);
	stc_l = readl(regs + TS_DPRX_VIDEO_IRQ_4) &
		STA_DPRX_VIDEO_STC_CNT_LATCH_LSB;
	stc_m = readl(regs + TS_DPRX_VIDEO_IRQ_5);
	*stc = (stc_m << STC_MSB_SHIFTER) | stc_l;

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_dprx_video_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_0_a_p1 camera vision timestamp and its associated
 *     metadata for IR path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_0_a_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP6_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP6_2);
	*vts = readl(regs + TS_ISP6_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_0_a_p1_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_0_b_p1 camera vision timestamp and its associated
 *     metadata for IR path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_0_b_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP7_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP7_2);
	*vts = readl(regs + TS_ISP7_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_0_b_p1_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_1_a_p1 camera vision timestamp and its associated
 *     metadata for IR path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_1_a_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP8_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP8_2);
	*vts = readl(regs + TS_ISP8_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_1_a_p1_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_1_b_p1 camera vision timestamp and its associated
 *     metadata for IR path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_1_b_p1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP9_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP9_2);
	*vts = readl(regs + TS_ISP9_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_1_b_p1_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_0_a_p2 camera vision timestamp and its associated
 *     metadata for W path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_0_a_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP0_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP0_2);
	*vts = readl(regs + TS_ISP0_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_0_a_p2_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_0_b_p2 camera vision timestamp and its associated
 *     metadata for W path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_0_b_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP1_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP1_2);
	*vts = readl(regs + TS_ISP1_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_0_b_p2_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_1_a_p2 camera vision timestamp and its associated
 *     metadata for W path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_1_a_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP2_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP2_2);
	*vts = readl(regs + TS_ISP2_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_1_a_p2_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_1_b_p2 camera vision timestamp and its associated
 *     metadata for W path.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_1_b_p2_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP3_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP3_2);
	*vts = readl(regs + TS_ISP3_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_1_b_p2_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get gaze_0 camera vision timestamp and its associated
 *     metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_gaze_0_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP4_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP4_2);
	*vts = readl(regs + TS_ISP4_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_gaze_0_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get gaze_1 camera vision timestamp and its associated
 *     metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_gaze_1_vision_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_ISP5_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_ISP5_2);
	*vts = readl(regs + TS_ISP5_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_gaze_1_vision_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_0 camera sync timestamp and its associated
 *     metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_0_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_SIDE_VS_0_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_SIDE_VS_0_2);
	*vts = readl(regs + TS_SIDE_VS_0_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_0_camera_sync_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get side_1 camera sync timestamp and its associated
 *     metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_side_1_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_SIDE_VS_1_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_SIDE_VS_1_2);
	*vts = readl(regs + TS_SIDE_VS_1_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_side_1_camera_sync_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get gaze R camera sync timestamp and its associated
 *     metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_gaze_0_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_GAZE_VS_0_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_GAZE_VS_0_2);
	*vts = readl(regs + TS_GAZE_VS_0_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_gaze_0_camera_sync_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get gaze L camera sync timestamp and its associated
 *     metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_gaze_1_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_GAZE_VS_1_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_GAZE_VS_1_2);
	*vts = readl(regs + TS_GAZE_VS_1_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_gaze_1_camera_sync_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get gaze LED camera sync timestamp and its associated
 *     metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_gaze_2_camera_sync_time(const struct device *dev,
		u32 *vts, u32 *dp_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;

	if (!dev || !vts || !dp_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_GAZE_VS_2_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_GAZE_VS_2_2);
	*vts = readl(regs + TS_GAZE_VS_2_4);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_gaze_2_camera_sync_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get IMU0 timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @param[out]
 *     imu_counter: associated IMU interrupt counter.
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_imu_0_time(const struct device *dev,
		u32 *vts, u32 *dp_counter, u16 *imu_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u32 tmp;

	if (!dev || !vts || !dp_counter || !imu_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_IMU_0_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_IMU_0_2);
	*vts = readl(regs + TS_IMU_0_4);
	tmp = readl(regs + TIMESTAMP_02) & STA_IMU0_CURRENT_CNT;
	*imu_counter = (u16)(tmp >> STA_IMU0_CURRENT_CNT_SHIFTER);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_imu_0_time);

/** @ingroup IP_group_timestamp_external_function
 * @par Description
 *     Atomically get IMU1 timestamp and its associated metadata.
 * @param[in]
 *     dev: timestamp device node.
 * @param[out]
 *     vts: associated VTS.
 * @param[out]
 *     dp_counter: associated DP RX Vsync/Hsync counters.\n
 *     dp_rx_hsync[15:0] = dp_counter[31:16]\n
 *     dp_rx_vsync[15:0] = dp_counter[15:0]
 * @param[out]
 *     imu_counter: associated IMU interrupt counter.\n
 *     (only valid when TS_IMU_1_IMU_TRIG_EDGE is 3: rise edge)
 * @return
 *     0, succeeded to get data.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If parameter is wrong, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_ts_get_imu_1_time(const struct device *dev,
		u32 *vts, u32 *dp_counter, u16 *imu_counter)
{
	struct mtk_ts *ts;
	void __iomem *regs;
	u32 tmp;

	if (!dev || !vts || !dp_counter || !imu_counter)
		return -EINVAL;

	ts = dev_get_drvdata(dev);
	if (!ts)
		return -EINVAL;

	regs = ts->regs;

	writel(TS_IMU_1_TRIG_LATCH, regs + TIMESTAMP_01);
	*dp_counter = readl(regs + TS_IMU_1_2);
	*vts = readl(regs + TS_IMU_1_4);
	tmp = readl(regs + TIMESTAMP_02) & STA_IMU1_CURRENT_CNT;
	*imu_counter = (u16)(tmp >> STA_IMU1_CURRENT_CNT_SHIFTER);

	return 0;
}
EXPORT_SYMBOL(mtk_ts_get_imu_1_time);

/** @ingroup IP_group_timestamp_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, probe succeeds.\n
 *     Otherwise, timestamp probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error in the probe flow,
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_ts_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ts *ts;
	struct resource *res;

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->dev = dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ts->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(ts->regs)) {
		dev_err(dev, "Failed to map timestamp registers\n");
		return PTR_ERR(ts->regs);
	}

	of_node_put(dev->of_node);
	platform_set_drvdata(pdev, ts);

	return 0;
}

/** @ingroup IP_group_timestamp_internal_function
 * @par Description
 *     platform_driver remove function.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     0, remove succeeds.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_ts_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id ts_driver_dt_match[] = {
	{ .compatible = "mediatek,mt3612-timestamp" },
	{},
};
MODULE_DEVICE_TABLE(of, ts_driver_dt_match);

struct platform_driver mtk_ts_driver = {
	.probe		= mtk_ts_probe,
	.remove		= mtk_ts_remove,
	.driver		= {
		.name	= "mediatek-timestamp",
		.owner	= THIS_MODULE,
		.of_match_table = ts_driver_dt_match,
	},
};

module_platform_driver(mtk_ts_driver);
MODULE_LICENSE("GPL");
