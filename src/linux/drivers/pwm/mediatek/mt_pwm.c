/*******************************************************************************
*
* Copyright (c) 2012, Media Teck.inc
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public Licence,
* version 2, as publish by the Free Software Foundation.
*
* This program is distributed and in hope it will be useful, but WITHOUT
* ANY WARRNTY; without even the implied warranty of MERCHANTABITLITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
*/

/**
 * @file mt_pwm.c
 */

/**
 * @defgroup IP_group_pwm PWM
 *   There are total 32 pwm channels in MT3611,\n
 *   Each channel has it's own Clock Source,ect.\n
 *
 *   @{
 *       @defgroup IP_group_pwm_external EXTERNAL
 *         The external API document for pwm. \n
 *
 *         @{
 *             @defgroup IP_group_pwm_external_function 1.function
 *               none. No extra function used in pwm driver.
 *             @defgroup IP_group_pwm_external_struct 2.structure
 *               none. No extra structure used in pwm driver.
 *             @defgroup IP_group_pwm_external_typedef 3.typedef
 *               none. No extra typedef used.
 *             @defgroup IP_group_pwm_external_enum 4.enumeration
 *               none. No extra enumeration in pwm driver.
 *             @defgroup IP_group_pwm_external_def 5.define
 *               none. No extra define in pwm driver.
 *         @}
 *
 *       @defgroup IP_group_pwm_internal INTERNAL
 *         The internal API document for pwm. \n
 *
 *         @{
 *             @defgroup IP_group_pwm_internal_function 1.function
 *               Internal function used in pwm driver.
 *             @defgroup IP_group_pwm_internal_struct 2.structure
 *               Internal structure used in pwm driver.
 *             @defgroup IP_group_pwm_internal_typedef 3.typedef
 *               none. No extra typedef used  in pwm driver.
 *             @defgroup IP_group_pwm_internal_enum 4.enumeration
 *               Internal enumeration used in pwm driver.
 *             @defgroup IP_group_pwm_internal_def 5.define
 *               Internal define used in pwm driver.
 *         @}
 *   @}
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
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
#include <linux/of_device.h>
#include <linux/clk.h>

#include "mt_pwm_hal.h"
#include "mt_pwm_reg.h"
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include "mt_pwm.h"
#include "mt_pwm_debug.h"

/**
 * @brief a global variable of pwm chip base address.
 */

void __iomem *pwm_base;

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Cast a member of a structure out to the containing structure.
 * @param
 *     chip: struct pwm_chip - abstract a PWM controller.
 * @return
 *     error code from container_of()
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

static inline struct mtk_pwm *to_mtk_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct mtk_pwm, chip);
}

/**
 * @brief pwm platform device  structure init value.
 */

static struct mtk_pwm pwm_dat = {
	.name = PWM_DEVICE,
	.ref = ATOMIC_INIT(0),
	.power_flag = 0,
	.lock = __SPIN_LOCK_UNLOCKED(pwm_dat.lock)
};

/**
 * @brief a global variable of pwm platform device  structure.
 */

static struct mtk_pwm *pwm_dev = &pwm_dat;

#ifdef CONFIG_COMMON_CLK_MT3612
static int mtk_pwm_clk_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pwm *mediatek_pwm = to_mtk_pwm(chip);
	int ret = 0;

	ret = clk_prepare_enable(mediatek_pwm->groupclk);
	if (ret < 0) {
		clk_disable_unprepare(mediatek_pwm->groupclk);
		return ret;
	}
	ret = clk_prepare_enable(mediatek_pwm->pwmclk);
	if (ret < 0) {
		clk_disable_unprepare(mediatek_pwm->pwmclk);
		return ret;
	}
	if (pwm->hwpwm == PWM0)
		ret =  clk_prepare_enable(mediatek_pwm->pwm1clk);
	if (pwm->hwpwm == PWM1)
		ret =  clk_prepare_enable(mediatek_pwm->pwm2clk);
	if (pwm->hwpwm == PWM2)
		ret =  clk_prepare_enable(mediatek_pwm->pwm3clk);
	if (pwm->hwpwm == PWM3)
		ret =  clk_prepare_enable(mediatek_pwm->pwm4clk);
	if (pwm->hwpwm == PWM4)
		ret =  clk_prepare_enable(mediatek_pwm->pwm5clk);
	if (pwm->hwpwm == PWM5)
		ret =  clk_prepare_enable(mediatek_pwm->pwm6clk);
	if (pwm->hwpwm == PWM6)
		ret =  clk_prepare_enable(mediatek_pwm->pwm7clk);
	if (pwm->hwpwm == PWM7)
		ret =  clk_prepare_enable(mediatek_pwm->pwm8clk);
	if (pwm->hwpwm == PWM8)
		ret =  clk_prepare_enable(mediatek_pwm->pwm9clk);
	if (pwm->hwpwm == PWM9)
		ret =  clk_prepare_enable(mediatek_pwm->pwm10clk);
	if (pwm->hwpwm == PWM10)
		ret =  clk_prepare_enable(mediatek_pwm->pwm11clk);
	if (pwm->hwpwm == PWM11)
		ret =  clk_prepare_enable(mediatek_pwm->pwm12clk);
	if (pwm->hwpwm == PWM12)
		ret =  clk_prepare_enable(mediatek_pwm->pwm13clk);
	if (pwm->hwpwm == PWM13)
		ret =  clk_prepare_enable(mediatek_pwm->pwm14clk);
	if (pwm->hwpwm == PWM14)
		ret =  clk_prepare_enable(mediatek_pwm->pwm15clk);
	if (pwm->hwpwm == PWM15)
		ret =  clk_prepare_enable(mediatek_pwm->pwm16clk);
	if (pwm->hwpwm == PWM16)
		ret =  clk_prepare_enable(mediatek_pwm->pwm17clk);
	if (pwm->hwpwm == PWM17)
		ret =  clk_prepare_enable(mediatek_pwm->pwm18clk);
	if (pwm->hwpwm == PWM18)
		ret =  clk_prepare_enable(mediatek_pwm->pwm19clk);
	if (pwm->hwpwm == PWM19)
		ret =  clk_prepare_enable(mediatek_pwm->pwm20clk);
	if (pwm->hwpwm == PWM20)
		ret =  clk_prepare_enable(mediatek_pwm->pwm21clk);
	if (pwm->hwpwm == PWM21)
		ret =  clk_prepare_enable(mediatek_pwm->pwm22clk);
	if (pwm->hwpwm == PWM22)
		ret =  clk_prepare_enable(mediatek_pwm->pwm23clk);
	if (pwm->hwpwm == PWM23)
		ret =  clk_prepare_enable(mediatek_pwm->pwm24clk);
	if (pwm->hwpwm == PWM24)
		ret =  clk_prepare_enable(mediatek_pwm->pwm25clk);
	if (pwm->hwpwm == PWM25)
		ret =  clk_prepare_enable(mediatek_pwm->pwm26clk);
	if (pwm->hwpwm == PWM26)
		ret =  clk_prepare_enable(mediatek_pwm->pwm27clk);
	if (pwm->hwpwm == PWM27)
		ret =  clk_prepare_enable(mediatek_pwm->pwm28clk);
	if (pwm->hwpwm == PWM28)
		ret =  clk_prepare_enable(mediatek_pwm->pwm29clk);
	if (pwm->hwpwm == PWM29)
		ret =  clk_prepare_enable(mediatek_pwm->pwm30clk);
	if (pwm->hwpwm == PWM30)
		ret =  clk_prepare_enable(mediatek_pwm->pwm31clk);
	if (pwm->hwpwm == PWM31)
		ret =  clk_prepare_enable(mediatek_pwm->pwm32clk);

	return ret;
}

static void mtk_pwm_clk_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pwm *mediatek_pwm = to_mtk_pwm(chip);

	if (pwm->hwpwm == PWM0)
		clk_disable_unprepare(mediatek_pwm->pwm1clk);
	if (pwm->hwpwm == PWM1)
		clk_disable_unprepare(mediatek_pwm->pwm2clk);
	if (pwm->hwpwm == PWM2)
		clk_disable_unprepare(mediatek_pwm->pwm3clk);
	if (pwm->hwpwm == PWM3)
		clk_disable_unprepare(mediatek_pwm->pwm4clk);
	if (pwm->hwpwm == PWM4)
		clk_disable_unprepare(mediatek_pwm->pwm5clk);
	if (pwm->hwpwm == PWM5)
		clk_disable_unprepare(mediatek_pwm->pwm6clk);
	if (pwm->hwpwm == PWM6)
		clk_disable_unprepare(mediatek_pwm->pwm7clk);
	if (pwm->hwpwm == PWM7)
		clk_disable_unprepare(mediatek_pwm->pwm8clk);
	if (pwm->hwpwm == PWM8)
		clk_disable_unprepare(mediatek_pwm->pwm9clk);
	if (pwm->hwpwm == PWM9)
		clk_disable_unprepare(mediatek_pwm->pwm10clk);
	if (pwm->hwpwm == PWM10)
		clk_disable_unprepare(mediatek_pwm->pwm11clk);
	if (pwm->hwpwm == PWM11)
		clk_disable_unprepare(mediatek_pwm->pwm12clk);
	if (pwm->hwpwm == PWM12)
		clk_disable_unprepare(mediatek_pwm->pwm13clk);
	if (pwm->hwpwm == PWM13)
		clk_disable_unprepare(mediatek_pwm->pwm14clk);
	if (pwm->hwpwm == PWM14)
		clk_disable_unprepare(mediatek_pwm->pwm15clk);
	if (pwm->hwpwm == PWM15)
		clk_disable_unprepare(mediatek_pwm->pwm16clk);
	if (pwm->hwpwm == PWM16)
		clk_disable_unprepare(mediatek_pwm->pwm17clk);
	if (pwm->hwpwm == PWM17)
		clk_disable_unprepare(mediatek_pwm->pwm18clk);
	if (pwm->hwpwm == PWM18)
		clk_disable_unprepare(mediatek_pwm->pwm19clk);
	if (pwm->hwpwm == PWM19)
		clk_disable_unprepare(mediatek_pwm->pwm20clk);
	if (pwm->hwpwm == PWM20)
		clk_disable_unprepare(mediatek_pwm->pwm21clk);
	if (pwm->hwpwm == PWM21)
		clk_disable_unprepare(mediatek_pwm->pwm22clk);
	if (pwm->hwpwm == PWM22)
		clk_disable_unprepare(mediatek_pwm->pwm23clk);
	if (pwm->hwpwm == PWM23)
		clk_disable_unprepare(mediatek_pwm->pwm24clk);
	if (pwm->hwpwm == PWM24)
		clk_disable_unprepare(mediatek_pwm->pwm25clk);
	if (pwm->hwpwm == PWM25)
		clk_disable_unprepare(mediatek_pwm->pwm26clk);
	if (pwm->hwpwm == PWM26)
		clk_disable_unprepare(mediatek_pwm->pwm27clk);
	if (pwm->hwpwm == PWM27)
		clk_disable_unprepare(mediatek_pwm->pwm28clk);
	if (pwm->hwpwm == PWM28)
		clk_disable_unprepare(mediatek_pwm->pwm29clk);
	if (pwm->hwpwm == PWM29)
		clk_disable_unprepare(mediatek_pwm->pwm30clk);
	if (pwm->hwpwm == PWM30)
		clk_disable_unprepare(mediatek_pwm->pwm31clk);
	if (pwm->hwpwm == PWM31)
		clk_disable_unprepare(mediatek_pwm->pwm32clk);
	clk_disable_unprepare(mediatek_pwm->pwmclk);
	clk_disable_unprepare(mediatek_pwm->groupclk);
}
#endif

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm channel enable.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_enable(u32 pwm_no)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid!\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number is not between PWM0~PWM31\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_enable_hal(pwm_no);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}


/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm channel disable.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_disable(u32 pwm_no)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number is not between PWM0~PWM31\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_disable_hal(pwm_no);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm enable seqmode.
 * @par Parameters
 *     none.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mt_set_pwm_enable_seqmode(void)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_enable_seqmode_hal();
	spin_unlock_irqrestore(&dev->lock, flags);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm setting clk.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     clksrc: 26M/1625 = 16KHz,32K,Block.
 * @param[in]
 *     div: Clock division.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT, division excesses CLK_DIV_MAX |
 *     clksrc excesses CLK_BLOCK_BY_1625_OR_32K.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
*/
static s32 mt_set_pwm_clk(u32 pwm_no, u32 clksrc, u32 div)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	if (div >= CLK_DIV_MAX) {
		PWMDBG("division excesses CLK_DIV_MAX\n");
		return -EPARMNOSUPPORT;
	}

	if ((clksrc & 0x7FFFFFFF) > CLK_BLOCK_BY_1625_OR_32K) {
		PWMDBG("clksrc excesses CLK_BLOCK_BY_1625_OR_32K\n");
		return -EPARMNOSUPPORT;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_clk_hal(pwm_no, clksrc, div);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm data src config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 is fifo mode, 1 is memory mode.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT, not support parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_con_datasrc(u32 pwm_no, u32 val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("pwm device doesn't exist\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	if (mt_set_pwm_con_datasrc_hal(pwm_no, val)) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EPARMNOSUPPORT;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm mode config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 is period mode,1 is random mode.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT,  not support parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_con_mode(u32 pwm_no, u32 val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	if (mt_set_pwm_con_mode_hal(pwm_no, val)) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EPARMNOSUPPORT;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  idle val config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 means that  idle state is not put out,idle_false.
 *	 1 means that idle state is put out,idle_true.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT,  not support parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_con_idleval(u32 pwm_no, u16 val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	if (mt_set_pwm_con_idleval_hal(pwm_no, val)) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EPARMNOSUPPORT;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm guard val config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 means that  guard state is not put out,guard_false.
 *	 1 means that guard state is put out,guard_true.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_con_guardval(u32 pwm_no, u16 val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	if (mt_set_pwm_con_guardval_hal(pwm_no, val)) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EPARMNOSUPPORT;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm stopbits config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     stpbit: Stop bits.
 * @param[in]
 *     srcsel: 0 is fifo mode, 1 is memory mode.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT,  not support parameter.
 * @par Boundary case and Limitation
 *     stop bits should be less then 0x3f
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_con_stpbit(u32 pwm_no, u32 stpbit, u32 srcsel)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	if ((srcsel == PWM_FIFO) && (stpbit > 0x3f)) {
		PWMDBG(
		"stpbit execesses the most of 0x3f in fifo mode\n");
		return -EPARMNOSUPPORT;
	} else if ((srcsel == MEMORY) && (stpbit > 0x1f)) {
		PWMDBG(
		"stpbit excesses the most of 0x1f in memory mode\n");
		return -EPARMNOSUPPORT;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_con_stpbit_hal(pwm_no, stpbit, srcsel);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm oldmode enable or not.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     val: 0 means disable oldmode,oldmode_disable.
 *	 1 means enable oldmode,oldmode_enable.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT,  not support parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_con_oldmode(u32 pwm_no, u32 val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	if (mt_set_pwm_con_oldmode_hal(pwm_no, val)) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EPARMNOSUPPORT;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm high duration config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     dur_val: High duration value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_hidur(u32 pwm_no, u16 dur_val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_hidur_hal(pwm_no, dur_val);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm low duration config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     dur_val: Low duration value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_lowdur(u32 pwm_no, u16 dur_val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_lowdur_hal(pwm_no, dur_val);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  guard duration config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     dur_val: Guard duration value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_guarddur(u32 pwm_no, u16 dur_val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_guarddur_hal(pwm_no, dur_val);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf0 addr config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     addr:  Buf0 data address value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_buf0_addr(u32 pwm_no, u32 addr)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_buf0_addr_hal(pwm_no, addr);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf0 size config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     size:  Buf0 size value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_buf0_size(u32 pwm_no, u16 size)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_buf0_size_hal(pwm_no, size);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf1 addr config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     addr:  Buf1 data address value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_buf1_addr(u32 pwm_no, u32 addr)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_buf1_addr_hal(pwm_no, addr);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  buf1 size config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     size:  Buf1 size value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_buf1_size(u32 pwm_no, u16 size)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_buf1_size_hal(pwm_no, size);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;

}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  send data0 config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     data:  Send data0 value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_send_data0(u32 pwm_no, u32 data)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_send_data0_hal(pwm_no, data);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  send data1 config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     data:  Send data1 value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_send_data1(u32 pwm_no, u32 data)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_send_data1_hal(pwm_no, data);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  wave num config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     num:  Wave num value.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_wave_num(u32 pwm_no, u16 num)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_wave_num_hal(pwm_no, num);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  data width config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     width: The data width of the wave.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_data_width(u32 pwm_no, u16 width)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_data_width_hal(pwm_no, width);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  thresh config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     thresh:  The thresh of the wave.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_thresh(u32 pwm_no, u16 thresh)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG(" pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_thresh_hal(pwm_no, thresh);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  set pwm valid config.
 * @param[in]
 *     pwm_no: Pwm channel,pwm0~pwm31.
 * @param[in]
 *     buf_valid_bit: For buf0: bit0 and bit1 should be set 1.
 *     for buf1: bit2 and bit3 should be set 1.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_valid(u32 pwm_no, u32 buf_valid_bit)
{				/*sst 0  for BUF0 bit or set 1 for BUF1 bit */
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if (pwm_no > PWM_MAX) {
		PWMDBG(" pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_valid_hal(pwm_no, buf_valid_bit);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  set delayduration config.
 * @param[in]
 *     pwm_delay_reg: Register of delay SEQ mode .
 * @param[in]
 *     val: Delay duration value when using SEQ mode.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_delay_duration(unsigned long pwm_delay_reg, u16 val)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_delay_duration_hal(pwm_delay_reg, val);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm  set delay clock config.
 * @param[in]
 *     pwm_delay_reg: Register of delay SEQ mode .
 * @param[in]
 *     clksrc: Pwm delay clock src when using SEQ mode.
 * @return
 *     0, function success.\n
 *     -EBADADDR, device doesn't exist.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_delay_clock(unsigned long pwm_delay_reg, u32 clksrc)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("device doesn't exist\n");
		return -EBADADDR;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_delay_clock_hal(pwm_delay_reg, clksrc);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
  * @par Description
  *     Pwm 3dlcm Eable.
  * @param[in]
  *     enable: 3dlcm enable true or false.
  * @return
  *     0, function success.\n
  *     -EINVALID, wrong parameter.
  * @par Boundary case and Limitation
  *     none.
  * @par Error case and Error handling
  *     none.
  * @par Call graph and Caller graph (refer to the graph below)
  * @par Refer to the source code
  */
static s32 mt_set_pwm_3dlcm_enable(u8 enable)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_3dlcm_enable_hal(enable);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;

}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm 3dlcm Pwm channel Auxiliary Inverse or the Same as Base.
 * @param[in]
 *     pwm_no: Auxiliary Pwm channel,pwm0~pwm31.
 * @param[in]
 *     inv: Inverse or the Same as Base.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_3dlcm_inv(u32 pwm_no, u8 inv)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}
	if ((pwm_no < 8) || (pwm_no > 15)) {
		PWMDBG("only PWM8~PWM15 support 3dlcm mode\n");
		return -EEXCESSPWMNO;
	}
	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_3dlcm_inv_hal(pwm_no, inv);
	spin_unlock_irqrestore(&dev->lock, flags);
	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm 3dlcm Setting Pwm channel as Auxiliary.
 * @param[in]
 *     pwm_no: Auxiliary Pwm channel,pwm0~pwm31.
 * @param[in]
 *     enable: 3dlcm Enable True or False.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_3dlcm_aux(u32 pwm_no, u8 enable)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}
	if ((pwm_no < 8) || (pwm_no > 15)) {
		PWMDBG("only PWM8~PWM15 support 3dlcm mode\n");
		return -EEXCESSPWMNO;
	}
	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_3dlcm_aux_hal(pwm_no, enable);
	spin_unlock_irqrestore(&dev->lock, flags);
	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm 3dlcm Setting Pwm channel as Base.
 * @param[in]
 *     pwm_no: Base Pwm channel,pwm0~pwm31.
 * @param[in]
 *     enable: 3dlcm Enable True or False.
 * @return
 *     0, function success.\n
 *     -EINVALID, wrong parameter.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static s32 mt_set_pwm_3dlcm_base(u32 pwm_no, u8 enable)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return -EINVALID;
	}

	if ((pwm_no < 8) || (pwm_no > 15)) {
		PWMDBG("only PWM8~PWM15 support 3dlcm mode\n");
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_set_pwm_3dlcm_base_hal(pwm_no, enable);
	spin_unlock_irqrestore(&dev->lock, flags);

	return RSUCCESS;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm clk enable or disable.
 * @param[in]
 *     enable: Pwm 26M clk enable or not.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void mt_pwm_26M_clk_enable(u32 enable)
{
	unsigned long flags;
	struct mtk_pwm *dev = pwm_dev;

	if (!dev) {
		PWMDBG("dev is not valid\n");
		return;
	}

	spin_lock_irqsave(&dev->lock, flags);
	mt_pwm_26M_clk_enable_hal(enable);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description.
 *     Pwm special config.
 * @param[in]
 *     conf: Pwm special config setting.
 * @return
 *     0, the Pwm General config success.\n
 *     -EINVALID, wrong parameter,pwm mode  | clock soruce |clock division.
 *     -EEXCESSPWMNO, exceed max pwm channel.
 *     -EPARMNOSUPPORT,  not support parameter.
 *     -ERROR,  parameters match error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
s32 pwm_set_spec_config(const struct pwm_spec_config *conf)
{
	if (conf->pwm_no > PWM_MAX) {
		PWMDBG("pwm number excess PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	if (conf->mode >= PWM_MODE_INVALID) {
		PWMDBG("PWM mode invalid\n");
		return -EINVALID;
	}

	if (conf->clk_src >= PWM_CLK_SRC_INVALID) {
		PWMDBG("PWM clock source invalid\n");
		return -EINVALID;
	}

	if (conf->clk_div >= CLK_DIV_MAX) {
		PWMDBG("PWM clock division invalid\n");
		return -EINVALID;
	}

	if ((conf->mode == PWM_MODE_OLD
	     && (conf->clk_src == PWM_CLK_NEW_MODE_BLOCK))
	    || (conf->mode != PWM_MODE_OLD
		&& (conf->clk_src == PWM_CLK_OLD_MODE_32K
		    || conf->clk_src == PWM_CLK_OLD_MODE_BLOCK))) {

		PWMDBG("parameters match error\n");
		return -ERROR;
	}

	if ((conf->mode == PWM_MODE_DELAY)
	    && (conf->pwm_no > PWM7)) {
		PWMDBG("pwm number excess PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	if ((conf->mode == PWM_MODE_3DLCM)
	    && ((conf->pwm_no > PWM15) || (conf->pwm_no < PWM8))) {
		PWMDBG("pwm number excess PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	switch (conf->mode) {
	case PWM_MODE_OLD:
		PWMDBG("PWM_MODE_OLD\n");
		mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_ENABLE);
		mt_set_pwm_con_idleval(conf->pwm_no,
				       conf->PWM_MODE_OLD_REGS.idle_value);
		mt_set_pwm_con_guardval(conf->pwm_no,
					conf->PWM_MODE_OLD_REGS.guard_value);
		mt_set_pwm_guarddur(conf->pwm_no,
				    conf->PWM_MODE_OLD_REGS.gduration);
		mt_set_pwm_wave_num(conf->pwm_no,
				    conf->PWM_MODE_OLD_REGS.wave_num);
		mt_set_pwm_data_width(conf->pwm_no,
				      conf->PWM_MODE_OLD_REGS.data_width);
		mt_set_pwm_thresh(conf->pwm_no, conf->PWM_MODE_OLD_REGS.thresh);
		PWMDBG("PWM set old mode finish\n");
		break;

	case PWM_MODE_FIFO:
		PWMDBG("PWM_MODE_FIFO\n");
		mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
		mt_set_pwm_con_datasrc(conf->pwm_no, PWM_FIFO);
		mt_set_pwm_con_mode(conf->pwm_no, PERIOD);
		mt_set_pwm_con_idleval(conf->pwm_no,
				       conf->PWM_MODE_FIFO_REGS.idle_value);
		mt_set_pwm_con_guardval(conf->pwm_no,
					conf->PWM_MODE_FIFO_REGS.guard_value);
		mt_set_pwm_hidur(conf->pwm_no,
				 conf->PWM_MODE_FIFO_REGS.high_duration);
		mt_set_pwm_lowdur(conf->pwm_no,
				  conf->PWM_MODE_FIFO_REGS.low_duration);
		mt_set_pwm_guarddur(conf->pwm_no,
				    conf->PWM_MODE_FIFO_REGS.gduration);
		mt_set_pwm_send_data0(conf->pwm_no,
				      conf->PWM_MODE_FIFO_REGS.send_data0);
		mt_set_pwm_send_data1(conf->pwm_no,
				      conf->PWM_MODE_FIFO_REGS.send_data1);
		mt_set_pwm_wave_num(conf->pwm_no,
				    conf->PWM_MODE_FIFO_REGS.wave_num);
		mt_set_pwm_con_stpbit(conf->pwm_no,
				      conf->PWM_MODE_FIFO_REGS.
				      stop_bitpos_value, PWM_FIFO);
		break;

	case PWM_MODE_MEMORY:
		PWMDBG("PWM_MODE_MEMORY\n");
		mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
		mt_set_pwm_con_datasrc(conf->pwm_no, MEMORY);
		mt_set_pwm_con_mode(conf->pwm_no, PERIOD);
		mt_set_pwm_con_idleval(conf->pwm_no,
				       conf->PWM_MODE_MEMORY_REGS.idle_value);
		mt_set_pwm_con_guardval(conf->pwm_no,
					conf->PWM_MODE_MEMORY_REGS.guard_value);
		mt_set_pwm_hidur(conf->pwm_no,
				 conf->PWM_MODE_MEMORY_REGS.high_duration);
		mt_set_pwm_lowdur(conf->pwm_no,
				  conf->PWM_MODE_MEMORY_REGS.low_duration);
		mt_set_pwm_guarddur(conf->pwm_no,
				    conf->PWM_MODE_MEMORY_REGS.gduration);
		mt_set_pwm_buf0_addr(conf->pwm_no,
				     conf->PWM_MODE_MEMORY_REGS.buf0_base_addr);
		mt_set_pwm_buf0_size(conf->pwm_no,
				     conf->PWM_MODE_MEMORY_REGS.buf0_size);
		mt_set_pwm_wave_num(conf->pwm_no,
				    conf->PWM_MODE_MEMORY_REGS.wave_num);
		mt_set_pwm_con_stpbit(conf->pwm_no,
				      conf->PWM_MODE_MEMORY_REGS.
				      stop_bitpos_value, MEMORY);
		break;

	case PWM_MODE_RANDOM:
		PWMDBG("PWM_MODE_RANDOM\n");
		mt_set_pwm_disable(conf->pwm_no);
		mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
		mt_set_pwm_con_datasrc(conf->pwm_no, MEMORY);
		mt_set_pwm_con_mode(conf->pwm_no, RAND);
		mt_set_pwm_con_idleval(conf->pwm_no,
				       conf->PWM_MODE_RANDOM_REGS.idle_value);
		mt_set_pwm_con_guardval(conf->pwm_no,
					conf->PWM_MODE_RANDOM_REGS.guard_value);
		mt_set_pwm_hidur(conf->pwm_no,
				 conf->PWM_MODE_RANDOM_REGS.high_duration);
		mt_set_pwm_lowdur(conf->pwm_no,
				  conf->PWM_MODE_RANDOM_REGS.low_duration);
		mt_set_pwm_guarddur(conf->pwm_no,
				    conf->PWM_MODE_RANDOM_REGS.gduration);
		mt_set_pwm_buf0_addr(conf->pwm_no,
				     conf->PWM_MODE_RANDOM_REGS.buf0_base_addr);
		mt_set_pwm_buf0_size(conf->pwm_no,
				     conf->PWM_MODE_RANDOM_REGS.buf0_size);
		mt_set_pwm_buf1_addr(conf->pwm_no,
				     conf->PWM_MODE_RANDOM_REGS.buf1_base_addr);
		mt_set_pwm_buf1_size(conf->pwm_no,
				     conf->PWM_MODE_RANDOM_REGS.buf1_size);
		mt_set_pwm_wave_num(conf->pwm_no,
				    conf->PWM_MODE_RANDOM_REGS.wave_num);
		mt_set_pwm_con_stpbit(conf->pwm_no,
				      conf->PWM_MODE_RANDOM_REGS.
				      stop_bitpos_value, MEMORY);
		mt_set_pwm_valid(conf->pwm_no, BUF0_VALID);
		mt_set_pwm_valid(conf->pwm_no, BUF1_VALID);
		break;

	case PWM_MODE_3DLCM:
		PWMDBG("PWM_MODE_3DLCM test\n");
		mt_set_pwm_disable(PWM8);
		mt_set_pwm_3dlcm_enable(1);
		mt_set_pwm_3dlcm_base(PWM8,
				      conf->PWM_MODE_3DLCM_REGS.
				      pwm8_as_base_channel);
		mt_set_pwm_3dlcm_aux(PWM9,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm9_as_aux_channel);
		mt_set_pwm_3dlcm_inv(PWM9,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm9_inverse_base_channel);
		mt_set_pwm_3dlcm_aux(PWM10,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm10_as_aux_channel);
		mt_set_pwm_3dlcm_inv(PWM10,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm10_inverse_base_channel);
		mt_set_pwm_3dlcm_aux(PWM11,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm11_as_aux_channel);
		mt_set_pwm_3dlcm_inv(PWM11,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm11_inverse_base_channel);
		mt_set_pwm_3dlcm_aux(PWM12,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm12_as_aux_channel);
		mt_set_pwm_3dlcm_inv(PWM12,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm12_inverse_base_channel);
		mt_set_pwm_3dlcm_aux(PWM13,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm13_as_aux_channel);
		mt_set_pwm_3dlcm_inv(PWM13,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm13_inverse_base_channel);
		mt_set_pwm_3dlcm_aux(PWM14,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm14_as_aux_channel);
		mt_set_pwm_3dlcm_inv(PWM14,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm14_inverse_base_channel);
		mt_set_pwm_3dlcm_aux(PWM15,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm15_as_aux_channel);
		mt_set_pwm_3dlcm_inv(PWM15,
				     conf->PWM_MODE_3DLCM_REGS.
				     pwm15_inverse_base_channel);
		mt_set_pwm_enable(PWM8);
		break;

	case PWM_MODE_DELAY:
		PWMDBG("PWM_MODE_DELAY\n");
		mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
		mt_set_pwm_enable_seqmode();
		mt_set_pwm_disable(PWM1);
		mt_set_pwm_disable(PWM2);
		mt_set_pwm_disable(PWM3);
		mt_set_pwm_disable(PWM4);
		mt_set_pwm_disable(PWM5);
		mt_set_pwm_disable(PWM6);
		mt_set_pwm_disable(PWM7);
		mt_set_pwm_delay_duration((unsigned long)PWM1_DELAY,
					  conf->PWM_MODE_DELAY_REGS.
					  pwm1_delay_dur);
		mt_set_pwm_delay_clock((unsigned long)PWM1_DELAY,
				       conf->PWM_MODE_DELAY_REGS.
				       pwm1_delay_clk);
		mt_set_pwm_delay_duration((unsigned long)PWM2_DELAY,
					  conf->PWM_MODE_DELAY_REGS.
					  pwm2_delay_dur);
		mt_set_pwm_delay_clock((unsigned long)PWM2_DELAY,
				       conf->PWM_MODE_DELAY_REGS.
				       pwm2_delay_clk);
		mt_set_pwm_delay_duration((unsigned long)PWM3_DELAY,
					  conf->PWM_MODE_DELAY_REGS.
					  pwm3_delay_dur);
		mt_set_pwm_delay_clock((unsigned long)PWM3_DELAY,
				       conf->PWM_MODE_DELAY_REGS.
				       pwm3_delay_clk);
		mt_set_pwm_delay_duration((unsigned long)PWM4_DELAY,
					  conf->PWM_MODE_DELAY_REGS.
					  pwm4_delay_dur);
		mt_set_pwm_delay_clock((unsigned long)PWM4_DELAY,
				       conf->PWM_MODE_DELAY_REGS.
				       pwm4_delay_clk);
		mt_set_pwm_delay_duration((unsigned long)PWM5_DELAY,
					  conf->PWM_MODE_DELAY_REGS.
					  pwm5_delay_dur);
		mt_set_pwm_delay_clock((unsigned long)PWM5_DELAY,
				       conf->PWM_MODE_DELAY_REGS.
				       pwm5_delay_clk);
		mt_set_pwm_delay_duration((unsigned long)PWM6_DELAY,
					  conf->PWM_MODE_DELAY_REGS.
					  pwm6_delay_dur);
		mt_set_pwm_delay_clock((unsigned long)PWM6_DELAY,
				       conf->PWM_MODE_DELAY_REGS.
				       pwm6_delay_clk);
		mt_set_pwm_delay_duration((unsigned long)PWM7_DELAY,
					  conf->PWM_MODE_DELAY_REGS.
					  pwm7_delay_dur);
		mt_set_pwm_delay_clock((unsigned long)PWM7_DELAY,
				       conf->PWM_MODE_DELAY_REGS.
				       pwm7_delay_clk);
		mt_set_pwm_enable(PWM1);
		mt_set_pwm_enable(PWM2);
		mt_set_pwm_enable(PWM3);
		mt_set_pwm_enable(PWM4);
		mt_set_pwm_enable(PWM5);
		mt_set_pwm_enable(PWM6);
		mt_set_pwm_enable(PWM7);
		break;
	default:
		break;
	}

	switch (conf->clk_src) {
	case PWM_CLK_OLD_MODE_BLOCK:
		mt_set_pwm_clk(conf->pwm_no, CLK_BLOCK, conf->clk_div);
		PWMDBG("Enable oldmode and set clock block\n");
		break;
	case PWM_CLK_OLD_MODE_32K:
		mt_set_pwm_clk(conf->pwm_no,
			       0x80000000 | CLK_BLOCK_BY_1625_OR_32K,
			       conf->clk_div);
		PWMDBG("Enable oldmode and set clock 32K\n");
		break;
	case PWM_CLK_NEW_MODE_BLOCK:
		mt_set_pwm_clk(conf->pwm_no, CLK_BLOCK, conf->clk_div);
		PWMDBG("Enable newmode and set clock block\n");
		break;
	case PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625:
		mt_set_pwm_clk(conf->pwm_no, CLK_BLOCK_BY_1625_OR_32K,
			       conf->clk_div);
		PWMDBG("Enable newmode and set clock 32K\n");
		break;
	default:
		break;
	}

	mb();			/* For memory barrier */
	if ((conf->mode != PWM_MODE_DELAY) && (conf->mode != PWM_MODE_3DLCM))
		mt_set_pwm_enable(conf->pwm_no);
	PWMDBG("mt_set_pwm_enable\n");

	return RSUCCESS;

}

/**
 * @brief a global variable of pwm platform device  structure.
 */

struct platform_device pwm_plat_dev = {
	.name = "mt-pwm",
};

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Enable pwm output toggling.
 * @param chip:
 *     abstract a pwm controller.
 * @param
 *     pwm: pwm channel object.
 * @return
 *     0 ,success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	mtk_pwm_clk_enable(chip, pwm);
#endif
	mt_set_pwm_enable(pwm->hwpwm);
	return 0;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Disable pwm output toggling.
 * @param[in]
 *     chip: abstract a pwm controller.
 * @param[in]
 *     pwm: pwm channel object.
 * @return
 *     0 ,success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void mtk_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
#ifdef CONFIG_COMMON_CLK_MT3612
	mtk_pwm_clk_disable(chip, pwm);
#endif
	mt_set_pwm_disable(pwm->hwpwm);
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     configure duty cycles and period length for this pwm.
 * @param[in]
 *     chip: abstract a pwm controller.
 * @param[in]
 *     pwm: pwm channel object.
 * @param[in]
 *     duty_ns: pwm waveform duty.
 * @param[in]
 *     period_ns: pwm waveform period.
 * @return
 *     0 ,success.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			       int duty_ns, int period_ns)
{
	static const unsigned int prescalers[] = {
		1, 2, 4, 8, 16, 32, 64, 128 };
	struct pwm_spec_config conf;
	unsigned int prescaler;
	u32 period, high_width;
	u64 rate;

	/*
	 * Find period, high_width and clk_div to suit duty_ns and period_ns.
	 * Calculate proper div value to keep period value in the bound.
	 *
	 * period_ns = 10^9 * clk_div * (period + 1) / PWM_CLK_RATE
	 * duty_ns = 10^9 * clk_div * high_width / PWM_CLK_RATE
	 *
	 * period = (PWM_CLK_RATE * period_ns) / (10^9 * clk_div) - 1
	 * high_width = (PWM_CLK_RATE * duty_ns) / (10^9 * clk_div)
	 */
#ifdef CONFIG_COMMON_CLK_MT3612
	mtk_pwm_clk_enable(chip, pwm);
#endif
	conf.mode = PWM_MODE_OLD;
	conf.pwm_no = pwm->hwpwm;

	rate = PWM_26M_CLK;
	for (prescaler = 0; prescaler < ARRAY_SIZE(prescalers); ++prescaler) {
		period = (rate / prescalers[prescaler] /
				  (NSEC_PER_SEC / period_ns))-1;
		if (period <= 0x1FFF)
			break;
	}
	if (prescaler == ARRAY_SIZE(prescalers) || period == 0)
		return -EINVALID;
	if (duty_ns) {
		high_width = (rate / prescalers[prescaler] /
					 (NSEC_PER_SEC / duty_ns))-1;
		if (high_width > period)
			return -EINVALID;
	} else {
		high_width = 0;
	}
	conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;
	conf.clk_div =  prescaler;
	conf.PWM_MODE_OLD_REGS.idle_value = IDLE_FALSE;
	conf.PWM_MODE_OLD_REGS.guard_value = GUARD_FALSE;
	conf.PWM_MODE_OLD_REGS.data_width = period;
	conf.PWM_MODE_OLD_REGS.thresh = high_width;
	conf.PWM_MODE_OLD_REGS.gduration = 0;
	conf.PWM_MODE_OLD_REGS.wave_num = 0;
	mt_pwm_26M_clk_enable_hal(1);
	pwm_set_spec_config(&conf);
	PWMDBG("pwm OLD mode channel=%d\n", conf.pwm_no);
#ifdef CONFIG_COMMON_CLK_MT3612
	mtk_pwm_clk_disable(chip, pwm);
#endif
	return 0;
}

/**
 * @brief a global pwm controller operations define for framework.
 */

static const struct pwm_ops mtk_pwm_ops = {
	.config = mtk_pwm_config,
	.enable = mtk_pwm_enable,
	.disable = mtk_pwm_disable,
	.owner = THIS_MODULE,
};

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm platform device probe.
 * @param[out]
 *     pdev: Platform device.
 * @return
 *     0, the platform_driver success.\n
 *     -EBADADDR, plaform device is not exist.
 *     -ENOMEM, allocate mem fail.
 *     -ENODEV, PWM iomap failed.
 *     error code from pwmchip_add().
 *     error code from device_create_file().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_pwm_probe(struct platform_device *pdev)
{
	struct mtk_pwm *mediatek_pwm;
	struct resource *res;
	int ret;

	if (!pdev) {
		PWMDBG("The plaform device is not exist\n");
		return -EINVALID;
	}
	PWMDBG("mt_pwm_init\n");

	mediatek_pwm = devm_kzalloc(&pdev->dev,
		sizeof(*mediatek_pwm), GFP_KERNEL);
	if (!mediatek_pwm)
		return -ENOMEM;

#ifdef CONFIG_COMMON_CLK_MT3612
	mediatek_pwm->groupclk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_MAIN);
	if (IS_ERR(mediatek_pwm->groupclk))
		return PTR_ERR(mediatek_pwm->groupclk);
	mediatek_pwm->fpwmclk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_SEL);
	if (IS_ERR(mediatek_pwm->fpwmclk))
		return PTR_ERR(mediatek_pwm->fpwmclk);
	mediatek_pwm->pwmclk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM);
	if (IS_ERR(mediatek_pwm->pwmclk))
		return PTR_ERR(mediatek_pwm->pwmclk);
	mediatek_pwm->pwm1clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM1);
	if (IS_ERR(mediatek_pwm->pwm1clk))
		return PTR_ERR(mediatek_pwm->pwm1clk);
	mediatek_pwm->pwm2clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM2);
	if (IS_ERR(mediatek_pwm->pwm2clk))
		return PTR_ERR(mediatek_pwm->pwm2clk);
	mediatek_pwm->pwm3clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM3);
	if (IS_ERR(mediatek_pwm->pwm3clk))
		return PTR_ERR(mediatek_pwm->pwm3clk);
	mediatek_pwm->pwm4clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM4);
	if (IS_ERR(mediatek_pwm->pwm4clk))
		return PTR_ERR(mediatek_pwm->pwm4clk);
	mediatek_pwm->pwm5clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM5);
	if (IS_ERR(mediatek_pwm->pwm5clk))
		return PTR_ERR(mediatek_pwm->pwm5clk);
	mediatek_pwm->pwm6clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM6);
	if (IS_ERR(mediatek_pwm->pwm6clk))
		return PTR_ERR(mediatek_pwm->pwm6clk);
	mediatek_pwm->pwm7clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM7);
	if (IS_ERR(mediatek_pwm->pwm7clk))
		return PTR_ERR(mediatek_pwm->pwm7clk);
	mediatek_pwm->pwm8clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM8);
	if (IS_ERR(mediatek_pwm->pwm8clk))
		return PTR_ERR(mediatek_pwm->pwm8clk);
	mediatek_pwm->pwm9clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM9);
	if (IS_ERR(mediatek_pwm->pwm9clk))
		return PTR_ERR(mediatek_pwm->pwm9clk);
	mediatek_pwm->pwm10clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM10);
	if (IS_ERR(mediatek_pwm->pwm10clk))
		return PTR_ERR(mediatek_pwm->pwm10clk);
	mediatek_pwm->pwm11clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM11);
	if (IS_ERR(mediatek_pwm->pwm11clk))
		return PTR_ERR(mediatek_pwm->pwm11clk);
	mediatek_pwm->pwm12clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM12);
	if (IS_ERR(mediatek_pwm->pwm12clk))
		return PTR_ERR(mediatek_pwm->pwm12clk);
	mediatek_pwm->pwm13clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM13);
	if (IS_ERR(mediatek_pwm->pwm13clk))
		return PTR_ERR(mediatek_pwm->pwm13clk);
	mediatek_pwm->pwm14clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM14);
	if (IS_ERR(mediatek_pwm->pwm14clk))
		return PTR_ERR(mediatek_pwm->pwm14clk);
	mediatek_pwm->pwm15clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM15);
	if (IS_ERR(mediatek_pwm->pwm15clk))
		return PTR_ERR(mediatek_pwm->pwm15clk);
	mediatek_pwm->pwm16clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM16);
	if (IS_ERR(mediatek_pwm->pwm16clk))
		return PTR_ERR(mediatek_pwm->pwm16clk);
	mediatek_pwm->pwm17clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM17);
	if (IS_ERR(mediatek_pwm->pwm17clk))
		return PTR_ERR(mediatek_pwm->pwm17clk);
	mediatek_pwm->pwm18clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM17);
	if (IS_ERR(mediatek_pwm->pwm18clk))
		return PTR_ERR(mediatek_pwm->pwm18clk);
	mediatek_pwm->pwm18clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM18);
	if (IS_ERR(mediatek_pwm->pwm18clk))
		return PTR_ERR(mediatek_pwm->pwm18clk);
	mediatek_pwm->pwm19clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM19);
	if (IS_ERR(mediatek_pwm->pwm19clk))
		return PTR_ERR(mediatek_pwm->pwm19clk);
	mediatek_pwm->pwm20clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM20);
	if (IS_ERR(mediatek_pwm->pwm20clk))
		return PTR_ERR(mediatek_pwm->pwm20clk);
	mediatek_pwm->pwm21clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM21);
	if (IS_ERR(mediatek_pwm->pwm21clk))
		return PTR_ERR(mediatek_pwm->pwm21clk);
	mediatek_pwm->pwm22clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM22);
	if (IS_ERR(mediatek_pwm->pwm22clk))
		return PTR_ERR(mediatek_pwm->pwm22clk);
	mediatek_pwm->pwm23clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM23);
	if (IS_ERR(mediatek_pwm->pwm23clk))
		return PTR_ERR(mediatek_pwm->pwm23clk);
	mediatek_pwm->pwm24clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM24);
	if (IS_ERR(mediatek_pwm->pwm24clk))
		return PTR_ERR(mediatek_pwm->pwm24clk);
	mediatek_pwm->pwm25clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM25);
	if (IS_ERR(mediatek_pwm->pwm25clk))
		return PTR_ERR(mediatek_pwm->pwm25clk);
	mediatek_pwm->pwm26clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM26);
	if (IS_ERR(mediatek_pwm->pwm26clk))
		return PTR_ERR(mediatek_pwm->pwm26clk);
	mediatek_pwm->pwm27clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM27);
	if (IS_ERR(mediatek_pwm->pwm27clk))
		return PTR_ERR(mediatek_pwm->pwm27clk);
	mediatek_pwm->pwm28clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM28);
	if (IS_ERR(mediatek_pwm->pwm28clk))
		return PTR_ERR(mediatek_pwm->pwm28clk);
	mediatek_pwm->pwm29clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM29);
	if (IS_ERR(mediatek_pwm->pwm29clk))
		return PTR_ERR(mediatek_pwm->pwm29clk);
	mediatek_pwm->pwm30clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM30);
	if (IS_ERR(mediatek_pwm->pwm30clk))
		return PTR_ERR(mediatek_pwm->pwm30clk);
	mediatek_pwm->pwm31clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM31);
	if (IS_ERR(mediatek_pwm->pwm31clk))
		return PTR_ERR(mediatek_pwm->pwm31clk);
	mediatek_pwm->pwm32clk = devm_clk_get(&pdev->dev, PWM_CLK_NAME_PWM32);
	if (IS_ERR(mediatek_pwm->pwm32clk))
		return PTR_ERR(mediatek_pwm->pwm32clk);
#endif
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	pwm_base = devm_ioremap_resource(&pdev->dev, res);
	if (!pwm_base) {
		PWMDBG("PWM iomap failed\n");
		return -ENODEV;
	};

	mediatek_pwm->chip.dev = &pdev->dev;
	mediatek_pwm->chip.ops = &mtk_pwm_ops;
	mediatek_pwm->chip.base = -1;
	mediatek_pwm->chip.npwm = 32;
	ret = pwmchip_add(&mediatek_pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add PWM chip %d\n", ret);
		return ret;
	}
	PWMDBG("pwm base: 0x%p\n", pwm_base);

	platform_set_drvdata(pdev, mediatek_pwm);

#if defined(CONFIG_PWM_MTK_DEBUG)
	mtk_pwm_debug_init(mediatek_pwm);
#endif

	return ret;
}

/** @ingroup IP_group_pwm_internal_function
 * @par Description
 *     Pwm platform device remove.
 * @param[out]
 *     pdev: Platform device.
 * @return
 *     0, the platform_driver success.\n
 *     -EBADADDR, plaform device is not exist.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mt_pwm_remove(struct platform_device *pdev)
{
	struct mtk_pwm *mediatek_pwm = NULL;

	if (!pdev) {
		PWMDBG("The plaform device is not exist\n");
		return -EBADADDR;
	}
	mediatek_pwm = platform_get_drvdata(pdev);
	pwmchip_remove(&mediatek_pwm->chip);
#ifdef CONFIG_COMMON_CLK_MT3612
	clk_disable_unprepare(mediatek_pwm->pwm1clk);
	clk_disable_unprepare(mediatek_pwm->pwm2clk);
	clk_disable_unprepare(mediatek_pwm->pwm3clk);
	clk_disable_unprepare(mediatek_pwm->pwm4clk);
	clk_disable_unprepare(mediatek_pwm->pwm5clk);
	clk_disable_unprepare(mediatek_pwm->pwm6clk);
	clk_disable_unprepare(mediatek_pwm->pwm7clk);
	clk_disable_unprepare(mediatek_pwm->pwm8clk);
	clk_disable_unprepare(mediatek_pwm->pwm9clk);
	clk_disable_unprepare(mediatek_pwm->pwm10clk);
	clk_disable_unprepare(mediatek_pwm->pwm11clk);
	clk_disable_unprepare(mediatek_pwm->pwm12clk);
	clk_disable_unprepare(mediatek_pwm->pwm13clk);
	clk_disable_unprepare(mediatek_pwm->pwm14clk);
	clk_disable_unprepare(mediatek_pwm->pwm15clk);
	clk_disable_unprepare(mediatek_pwm->pwm16clk);
	clk_disable_unprepare(mediatek_pwm->pwm17clk);
	clk_disable_unprepare(mediatek_pwm->pwm18clk);
	clk_disable_unprepare(mediatek_pwm->pwm19clk);
	clk_disable_unprepare(mediatek_pwm->pwm20clk);
	clk_disable_unprepare(mediatek_pwm->pwm21clk);
	clk_disable_unprepare(mediatek_pwm->pwm22clk);
	clk_disable_unprepare(mediatek_pwm->pwm23clk);
	clk_disable_unprepare(mediatek_pwm->pwm24clk);
	clk_disable_unprepare(mediatek_pwm->pwm25clk);
	clk_disable_unprepare(mediatek_pwm->pwm26clk);
	clk_disable_unprepare(mediatek_pwm->pwm27clk);
	clk_disable_unprepare(mediatek_pwm->pwm28clk);
	clk_disable_unprepare(mediatek_pwm->pwm29clk);
	clk_disable_unprepare(mediatek_pwm->pwm30clk);
	clk_disable_unprepare(mediatek_pwm->pwm31clk);
	clk_disable_unprepare(mediatek_pwm->pwm32clk);
	clk_disable_unprepare(mediatek_pwm->pwmclk);
	clk_disable_unprepare(mediatek_pwm->groupclk);
#endif
	PWMDBG("mt_pwm_remove\n");
	return RSUCCESS;
}

/**
 * @brief a global Struct used for matching a pwm device.
 */

static const struct of_device_id pwm_of_match[] = {
	{.compatible = "mediatek,mt3612-pwm",},
	{},
};

MODULE_DEVICE_TABLE(of, pwm_of_match);

/**
 * @brief a global pwm platform driver define used for matching a pwm device.
 */

struct platform_driver pwm_plat_driver = {
	.driver = {
		.name = "mediatek-pwm",
		.owner = THIS_MODULE,
		.of_match_table = pwm_of_match,
	},
	.probe = mt_pwm_probe,
	.remove = mt_pwm_remove,
};
module_platform_driver(pwm_plat_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION(" This module is for mtk chip of mediatek");
