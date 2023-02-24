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

/**
 * @file tmrio-mt3612.c
 * Timer I/O Linux Driver. Total 16 channels, each has 16 bit Timer-Counter.
 * Each channel has it's own iterrrupt number and clock source.
 * Clock Source is 26 MHz with devider from 1 to 51968.
 * Input Mode consists "Pulse Width Mode" and "Pulse Number Mode"
 * Output Mode will change GPIO output level as internal counter reaches a
 * preset coumpare value.
 */

/**
 * @defgroup IP_group_timerio TIMER_IO
 *   The timerio dirver provide settings for MT3611 timerio block.
 *
 *   @{
 *       @defgroup IP_group_timerio_external EXTERNAL
 *         The external API document for timerio. \n
 *         Detail descriptions are in these following sections
 *         @{
 *            @defgroup IP_group_timerio_external_function 1.function
 *              External function for timerio module settings.
 *            @defgroup IP_group_timerio_external_struct 2.structure
 *              none. Native Linux Driver.
 *            @defgroup IP_group_timerio_external_typedef 3.typedef
 *              none. Native Linux Driver.
 *            @defgroup IP_group_timerio_external_enum 4.enumeration
 *              none. Native Linux Driver.
 *            @defgroup IP_group_sysirq_external_def 5.define
 *              none. Native Linux Driver.
 *         @}
 *       @defgroup IP_group_timerio_internal INTERNAL
 *         The internal API document for timerio. \n
 *         Detail descriptions are in these following sections
 *         @{
 *            @defgroup IP_group_timerio_internal_function 1.function
 *              Internal function for timerio module init.
 *            @defgroup IP_group_timerio_internal_struct 2.structure
 *              Internal structure used for timerio.
 *            @defgroup IP_group_timerio_internal_typedef 3.typedef
 *              none. Follow Linux coding style, no typedef used.
 *            @defgroup IP_group_timerio_internal_enum 4.enumeration
 *              Internal emumeration used for timerio.
 *            @defgroup IP_group_timerio_internal_def 5.define
 *              none. No extra define in timerio driver.
 *         @}
 *     @}
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/of_irq.h>
#include <generated/autoconf.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/stddef.h>
#include <linux/bitops.h>
#include <soc/mediatek/mtk_tmrio.h>
#include <tmrio-mt3612.h>
#include <timer_io_reg.h>

void __iomem *tmrio_base;

/** @ingroup IP_group_timerio_internal_struct
 * @brief Private chip data structure for timerio driver.
*/
struct mt_tmrio {
	const char *name;
	atomic_t ref;
	dev_t devno;
	spinlock_t lock;
	unsigned long power_flag;
	struct device dev;
};

/** @ingroup IP_group_timerio_internal_struct
 * @brief Private chip data structure for timerio driver.
*/
static struct mt_tmrio tmrio_dat = {
	.name = TMRIO_DEVICE,
	.ref = ATOMIC_INIT(0),
	.power_flag = 0,
	.lock = __SPIN_LOCK_UNLOCKED(tmrio_dat.lock)
};

static struct mt_tmrio *tmrio_dev = &tmrio_dat;

#ifdef	DEBUG_EN
static void REG_DUMP(unsigned long addr)
{
	unsigned long val = 0;

	val = READREG32(addr);
	pr_debug("Reg(0x%x)= 0x%x\n", addr, val);
}
#endif

/** @ingroup IP_group_timerio_internal_function
 * @par Description
 *     Timer I/O Interrupt Handler.
 * @param[in]
 *     irq: GIC Interrupt ID.
 * @param[in]
 *     data: tmrio device ID pointer.
 * @return
 *     0, IRQ NOT HANDLED\n
 *     1, IRQ HANDLED
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t tmrio_handler(int irq, void *data)
{
	unsigned int dev_id = (u32)((uintptr_t) data);
	unsigned int val = 0;

	pr_debug("tmrio_handler\n");

	tmrio_disable(dev_id);
	val = tmrio_is_irq_pending(dev_id);
	if (val)
		tmrio_ack_irq(dev_id);

	return IRQ_HANDLED;
}

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Enable Timer IO Function.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     0, timerio enable done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_enable(enum TMRIO_CH tmrio_ch)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	val |= TMR_EN0;
	WRITEREG32(val, addr);

#ifdef	DEBUG_EN
	pr_debug("tmrio_enable\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_enable);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Disable Timer IO Function.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     0, timerio disable done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_disable(enum TMRIO_CH tmrio_ch)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	val &= (~TMR_EN0);
	WRITEREG32(val, addr);

#ifdef	DEBUG_EN
	pr_debug("tmrio_disable\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_disable);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Soft Reset Timer IO Fnction.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     0, timerio soft reset done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_reset(enum TMRIO_CH tmrio_ch)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	val &= (~TIMER0_SW_RST);
	val |= TIMER0_SW_RST;
	WRITEREG32(val, addr);
	val &= (~TIMER0_SW_RST);
	WRITEREG32(val, addr);
#ifdef	DEBUG_EN
	pr_debug("tmrio_reset\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_reset);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Soft Clear Reset Timer IO Fnction.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     0, timerio soft clear reset done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_reset_clear(enum TMRIO_CH tmrio_ch)
{
	void __iomem *addr;
	unsigned int value;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	value = READREG32(addr) & (~TIMER0_SW_RST);
	WRITEREG32(value, addr);

	return 0;
}
EXPORT_SYMBOL(tmrio_reset_clear);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Sets timerio function mode.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @param[in]
 *     mode: Selects timerio mode
 * @param[in]
 *     hlp_sel: sets input signal polarity
 * @param[in]
 *     gpio_sel: sets output signal polarity
 * @return
 *     0, timerio set mode done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 *     hlp_sel, Value is 0 or 1.
 *     gpio_sel, Value is 0 or 1.
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_set_mode(enum TMRIO_CH tmrio_ch, enum TMRIO_MODE mode,
		     unsigned int hlp_sel, unsigned int gpio_sel)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	if (mode >= TMRIO_MODE_MAX)
		return -EINVAL;

	if ((hlp_sel > 1) || (gpio_sel > 1))
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	val &= (~(PWN_SEL0 | INOUT_MODE0));
	val &= (~HLP_SEL0);
	val &= (~GPIO_SEL0);

	if (hlp_sel)
		val |= HLP_SEL0;

	if (gpio_sel)
		val |= GPIO_SEL0;

	val |= (mode << TMRIO_MODE_SHIFT_BIT);

	if (mode == TMRIO_OUTPUT_MODE)
		val |= PWN_SEL0;

	WRITEREG32(val, addr);
#ifdef	DEBUG_EN
	pr_debug("tmrio_set_mode\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_set_mode);

/** @ingroup IP_group_timerio_internal_function
 * @par Description
 *     Clear Interrupt Status and 16-bit Counter
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     0, timerio irq_ack clear done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_ack_irq(enum TMRIO_CH tmrio_ch)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	pr_debug("tmrio_ack_irq, tmrio_ch=%d\n", tmrio_ch);
	val &= (~TIMER0_IRQ_CLR);
	val |= TIMER0_IRQ_CLR;
	WRITEREG32(val, addr);
#ifdef	DEBUG_EN
	pr_debug("tmrio_ack_irq\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_ack_irq);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Get Timer IO Inetrrupt Status.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     0, timerio irq not ack.\n
 *     1, timerio irq ack.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
bool tmrio_is_irq_pending(enum TMRIO_CH tmrio_ch)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	val &= TIMER0_IRQ_ACK;

	return (val == TIMER0_IRQ_ACK);
}
EXPORT_SYMBOL(tmrio_is_irq_pending);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Sets Debounce Function for Timer IO Input Signal.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @param[in]
 *     deben: Debounce Function Enable.
 * @param[in]
 *     debouncetime: Debounce Time Select
 * @return
 *     0, timerio debounce time setting done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15.
 *     deben, Value is 0 or 1.
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_debounce_function(enum TMRIO_CH tmrio_ch, unsigned int deben,
			      enum TMRIO_DEB debouncetime)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX || debouncetime > TMRIO_DEB_9)
		return -EINVAL;

	if (deben > 1)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	val &= (~DEB_EN0);
	val &= (~DEB_SEL0);
	if (deben)
		val |= DEB_EN0;

	val |= (debouncetime << TMRIO_DEBEN_SHIFT_BIT);
	WRITEREG32(val, addr);

#ifdef	DEBUG_EN
	pr_debug("tmrio_deb_func\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_debounce_function);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Sets Timer IO 16bit-Counter Clock Frequency.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @param[in]
 *     ckgen:    Set 16bit-Counter Clock Frequency
 * @return
 *     0, timerio ckgen setting done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_ckgen_function(enum TMRIO_CH tmrio_ch, enum TMRIO_CKGEN ckgen)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX || ckgen > TMRIO_CKGEN_31)
		return -EINVAL;

	addr = TMRIO0_CON + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);
	val &= (~CK_GEN_0);
	val |= (ckgen << TMRIO_CKGEN_SHIFT_BIT);
	WRITEREG32(val, addr);

#ifdef	DEBUG_EN
	pr_debug("tmrio_ckgen_func\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_ckgen_function);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Get Timer-IO 16bit-Counter Value.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     16bit-Counter Value.Value is from 0 to 65535.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_get_cnt(enum TMRIO_CH tmrio_ch)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_CNT + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);

#ifdef	DEBUG_EN
	pr_debug("tmrio_get_cnt\n");
	REG_DUMP(addr);
#endif

	return val;
}
EXPORT_SYMBOL(tmrio_get_cnt);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Sets Timer IO 16bit-Comparae Value.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @param[in]
 *         comp: 16-bit Timer IO Compare Value.
 * @return
 *     0, timerio compare value setting done.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 *     comp,Value is from 0 to 65535.
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_set_cmp(enum TMRIO_CH tmrio_ch, unsigned short comp)
{
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX || comp > 0xFFFF)
		return -EINVAL;

	addr = TMRIO0_COMP_VAL + (tmrio_ch * TMRIO_CH_OFFSET);
	WRITEREG32(comp, addr);

#ifdef	DEBUG_EN
	pr_debug("tmrio_set_cmp\n");
	REG_DUMP(addr);
#endif
	return 0;
}
EXPORT_SYMBOL(tmrio_set_cmp);

/** @ingroup IP_group_timerio_external_function
 * @par Description
 *     Get Timer IO 16bit-Comparae Value.
 * @param[in]
 *     tmrio_ch: Selects Timer IO Channel
 * @return
 *     16-bit Timer IO Compare Value. Value is from 0 to 65535.\n
 *     -EINVAL, wrong parameter.
 * @par Boundary case and Limitation
 *     SW Reset needs to be Released
 *     tmrio_ch, Value is from 0 to 15
 * @par Error case and Error handing
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int tmrio_get_cmp(enum TMRIO_CH tmrio_ch)
{
	unsigned long val;
	void __iomem *addr;

	if (tmrio_ch >= TMRIO_MAX)
		return -EINVAL;

	addr = TMRIO0_COMP_VAL + (tmrio_ch * TMRIO_CH_OFFSET);
	val = READREG32(addr);

#ifdef	DEBUG_EN
	pr_debug("tmrio_get_cmp\n");
	REG_DUMP(addr);
#endif
	return val;
}
EXPORT_SYMBOL(tmrio_get_cmp);

static const struct of_device_id tmrio_of_match[] = {
	{.compatible = "mediatek,mt3611-tmrio",},
	{.compatible = "mediatek,mt3612-tmrio",},
	{},
};

MODULE_DEVICE_TABLE(of, tmrio_of_match);

static int mt_tmrio_probe(struct platform_device *pdev)
{
	int irqnr[TMRIO_MAX];
	unsigned int i = 0;

	tmrio_base = of_iomap(pdev->dev.of_node, 0);

	if (!tmrio_base) {
		pr_debug("TMRIO iomap failed\n");
		return -ENODEV;
	};

#ifdef CONFIG_OF
	pr_debug("tmrio base(CONFIG_OF): 0x%p\n", tmrio_base);
#else
	pr_debug("tmrio base: 0x%p\n", tmrio_base);
#endif

	platform_set_drvdata(pdev, tmrio_dev);

	for (i = 0; i < TMRIO_MAX; i++) {
		irqnr[i] = platform_get_irq(pdev, i);
		pr_debug("tmrio irqnr[%d]: %d\n", i, irqnr[i]);
		if (irqnr[i] <= 0) {
			pr_debug("TMRIO get irq failed\n");
			return -EINVAL;
		};
	}

	if (request_irq(irqnr[TMRIO_0], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_0)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_1], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_1)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_2], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_2)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_3], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_3)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_4], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_4)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_5], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_5)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_6], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_6)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_7], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_7)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_8], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_8)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_9], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_9)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_10], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_10)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_11], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_11)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_12], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_12)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_13], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_13)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_14], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_14)) {
		pr_debug("failed to setup irq\n");
	}

	if (request_irq(irqnr[TMRIO_15], tmrio_handler,
			IRQF_TRIGGER_LOW, TMRIO_DEVICE, (void *)TMRIO_15)) {
		pr_debug("failed to setup irq\n");
	}

	return RSUCCESS;
}

static int mt_tmrio_remove(struct platform_device *pdev)
{
	if (!pdev) {
		pr_debug("TMRIO device is not exist\n");
		return -EBADADDR;
	}

	pr_debug("mt_tmrio_remove\n");
	return RSUCCESS;
}

static void mt_tmrio_shutdown(struct platform_device *pdev)
{
	pr_debug("mt_tmrio_shutdown\n");
}

struct platform_driver tmrio_plat_driver = {
	.probe = mt_tmrio_probe,
	.remove = mt_tmrio_remove,
	.shutdown = mt_tmrio_shutdown,
	.driver = {
		   .name = "mt3611-tmrio",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(tmrio_of_match),
		   },
};

static int mt_tmrio_init(void)
{
	int ret;

	pr_debug("mt_tmrio_init\n");

	ret = platform_driver_register(&tmrio_plat_driver);

	if (ret < 0) {
		pr_debug("tmrio platform_driver_register error\n");
		goto out;
	}

out:
	return ret;
}

static void mt_tmrio_exit(void)
{
	platform_driver_unregister(&tmrio_plat_driver);
}

module_init(mt_tmrio_init);
module_exit(mt_tmrio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION(" This tmrio module is for mtk chip of mediatek");
