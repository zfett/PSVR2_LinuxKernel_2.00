/*
 * Copyright (c) 2018 MediaTek Inc.
 * Authors:
 *	Houlong Wei <houlong.wei@mediatek.com>
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
 * @file mtk_mutex.c
 * This mutex driver is used to config MTK mutex hardware module.\n
 * It includes SOF source, componet id, ext signal and timeout value.
 */

/**
 * @defgroup IP_group_mutex DISP_MUTEX
 *     There are total 3 mutex hardwares in MT3612.\n
 *     Each hardware has it's own 16 mutexes supporting,\n
 *     16 timer debuggers and 8 delay resources.
 *
 *   @{
 *       @defgroup IP_group_mutex_external EXTERNAL
 *         The external API document for mutex. \n
 *
 *         @{
 *            @defgroup IP_group_mutex_external_function 1.function
 *              This is mutex external API.
 *            @defgroup IP_group_mutex_external_struct 2.structure
 *              This is mutex external structure.
 *            @defgroup IP_group_mutex_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_mutex_external_enum 4.enumeration
 *              This is mutex external enumeration.
 *            @defgroup IP_group_mutex_external_def 5.define
 *              none.
 *         @}
 *
 *       @defgroup IP_group_mutex_internal INTERNAL
 *         The internal API document for mutex. \n
 *
 *         @{
 *            @defgroup IP_group_mutex_internal_function 1.function
 *               module init and remove functions.
 *            @defgroup IP_group_mutex_internal_struct 2.structure
 *              Internal structure used for mutex.
 *            @defgroup IP_group_mutex_internal_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_mutex_internal_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_mutex_internal_def 5.define
 *              Mutex register definition and constant definition.
 *         @}
 *     @}
 */

#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#ifdef CONFIG_MTK_DEBUGFS
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#endif
#include <soc/mediatek/mtk_drv_def.h>
#include <soc/mediatek/mtk_mutex.h>
#include "mtk_mutex_reg.h"
#include <linux/pm_runtime.h>

#define MODULE_MAX_NUM		1

/** @ingroup IP_group_mutex_internal_def
 * @brief mutex configuration register
 * @{
 */
#define MUTEX_ENABLE(n)		(DISP_MUTEX0_EN + 0x20 * (n))
#define MUTEX_GETN(n)		(DISP_MUTEX0_GET + 0x20 * (n))
#define MUTEX_RESET(n)		(DISP_MUTEX0_RST + 0x20 * (n))
#define MUTEX_CTRL(n)		(DISP_MUTEX0_CTL + 0x20 * (n))
#define SOF_VSYNC_FALLING		0x0
#define EOF_VDE_FALLING			0x0
#define EOF_VDE_RISING			0x3
#define MUTEX_MODN0(n)		(DISP_MUTEX0_MOD0 + 0x20 * (n))
#define MUTEX_MODN1(n)		(DISP_MUTEX0_MOD1 + 0x20 * (n))
#define MUTEX_AH_ERR_MASK(n)	(DISP_MUTEX0_AH_ERR_MASK + 0x20 * (n))
#define MUTEX_DELAY_CFG(n)	(DISP_MUTEX_SYNC_DLY_0_CFG + 0x4 * (n))
#define MUTEX_TD_CFG(n)		(n > 3 ?\
				(DISP_MUTEX_TDEBUG_4_CFG0 + 0x14 * (n-4)) :\
				(DISP_MUTEX_TDEBUG_0_CFG0 + 0x14 * (n)))
#define TD_RESET			BIT(0)
#define TD_ENABLE			BIT(1)
#define TD_LOOP_MODE			BIT(2)
#define TD_SRC_EDGE			BIT(3)
#define TD_REF_EDGE			BIT(4)
#define EVENT_SEL			GENMASK(6, 5)
#define REF_SOF_MASK			GENMASK(15, 8)
#define SRC_SOF_MASK			GENMASK(23, 16)
#define MUTEX_TD_CFG1(n)	(n > 3 ?\
				(DISP_MUTEX_TDEBUG_4_CFG1 + 0x14 * (n-4)) :\
				(DISP_MUTEX_TDEBUG_0_CFG1 + 0x14 * (n)))
#define MUTEX_TD_CFG2(n)	(n > 3 ?\
				(DISP_MUTEX_TDEBUG_4_CFG2 + 0x14 * (n-4)) :\
				(DISP_MUTEX_TDEBUG_0_CFG2 + 0x14 * (n)))
#define MUTEX_TD_CFG3(n)	(n > 3 ?\
				(DISP_MUTEX_TDEBUG_4_CFG3 + 0x14 * (n-4)) :\
				(DISP_MUTEX_TDEBUG_0_CFG3 + 0x14 * (n)))
#define MUTEX_TD_CFG4(n)	(n > 3 ?\
				(DISP_MUTEX_TDEBUG_4_CFG4 + 0x14 * (n-4)) :\
				(DISP_MUTEX_TDEBUG_0_CFG4 + 0x14 * (n)))
#define MUTEX_EXTSIG_MUTEX_MASK(n)\
				(DISP_MUTEX_EXTSIG0_MUTEX_MASK + 0x8 * (n))
/**
 * @}
 */


/** @ingroup IP_group_mutex_internal_def
 * @brief Mutex Constant Definition
 * @{
 */
/** maxinum mutex unit number in one MUTEX hardware */
#define MUTEX_MAX_RES_NUM	16

/** maxinum mutex delay generator number in one MUTEX hardware */
#define MUTEX_MAX_DELAY_NUM	8

/** invalid ref sof id for timer debugger function */
#define INVALID_REF_SOF_ID	255

/** maxinum mutex delay time */
#define MUTEX_MAX_DELAY_IN_US	40000

/** maxinum mutex timer timeout threshold */
#define MUTEX_MAX_TIMER_THD_IN_US	1000000
/**
 * @}
 */


/** @ingroup IP_group_mutex_external_struct
 * @brief Mutex Resource Structure.
 */
struct mtk_mutex_res {
	/** resource id */
	int id;
	/** whether resource is occupied by other user or not */
	bool claimed;
};

/** @ingroup IP_group_mutex_internal_struct
 * @brief Mutex Driver Data Structure.
 */
struct mtk_mutex {
	/** mutex device node */
	struct device *dev;
	/** mutex clock node */
	struct clk *clk;
	/** mutex 26M clock node */
	struct clk *clk_26M;
	/** mutex register base */
	void __iomem *regs[MODULE_MAX_NUM];
	/** mutex resource lock */
	spinlock_t lock;
	/** MM clock rate */
	u32 mm_clock;
	/** mutex hardware number */
	u32 hw_nr;
	/** mutex delay resource number */
	u32 delay_res_num;
	/** mutex delay sof base */
	u32 delay_sof;
	/** mutex clock frequency */
	u32 mutex_clock;
	/** gce subsys */
	u32 gce_subsys[MODULE_MAX_NUM];
	/** gce subsys offset */
	u32 gce_subsys_offset[MODULE_MAX_NUM];
	/** mutex irq number */
	int irq;
	/** mutex resources */
	struct mtk_mutex_res mutex_res[MUTEX_MAX_RES_NUM];
	/** mutex delay resources */
	struct mtk_mutex_res mutex_delay_res[MUTEX_MAX_DELAY_NUM];
	/** callback variables */
	mtk_mmsys_cb			cb_func[MUTEX_IRQ_STA_NR];
	struct mtk_mmsys_cb_data	cb_data[MUTEX_IRQ_STA_NR];
	u32				cb_status[MUTEX_IRQ_STA_NR];
	u32 reg_base[MODULE_MAX_NUM];
#ifdef CONFIG_MTK_DEBUGFS
	struct dentry			**debugfs;
	const char			*file_name;
#endif
};

/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Check mutex_res is valid or not.
 * @param[in]
 *     mutex_res: mutex resource address.
 * @return
 *     0, mutex_res is valid.\n
 *     -EINVAL, if mutex_res is not valid.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If mutex_res is NULL, return -EINVAL.\n
 *     2. If user does not call mtk_mutex_get yet, return -EINVAL.\n
 *     3. If mutex_res is not matched with mutex->mutex_res[mutex_res->id],\n
 *     return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mutex_res_check(const struct mtk_mutex_res *mutex_res)
{
	struct mtk_mutex *mutex;

	if (IS_ERR_OR_NULL(mutex_res))
		return -EINVAL;

	if (!mutex_res->claimed)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);
	if (&mutex->mutex_res[mutex_res->id] != mutex_res) {
		dev_err(mutex->dev, "may data corruption\n");
		WARN_ON(true);
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Check mutex_delay_res is valid or not.
 * @param[in]
 *     mutex_delay_res: mutex delay resource address.
 * @return
 *     0, mutex_delay_res is valid.\n
 *     -EINVAL, if mutex_delay_res is not valid.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If mutex_delay_res is NULL, return -EINVAL.\n
 *     2. If user does not call mtk_mutex_delay_get yet, return -EINVAL.\n
 *     3. If mutex_delay_res is not matched with\n
 *     mutex->mutex_delay_res[mutex_delay_res->id], return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mutex_delay_res_chk(const struct mtk_mutex_res *mutex_delay_res)
{
	struct mtk_mutex *mutex;

	if (IS_ERR_OR_NULL(mutex_delay_res))
		return -EINVAL;

	if (!mutex_delay_res->claimed)
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);
	if (&mutex->mutex_delay_res[mutex_delay_res->id] != mutex_delay_res) {
		dev_err(mutex->dev, "may data corruption\n");
		WARN_ON(true);
		return -EINVAL;
	}

	return 0;
}

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Get Mutex Resource.\n
 *     It is pure SW function for getting Mutex SW resource. User needs to\n
 *     call it and get valid mutex resource before using Mutex. The mutex\n
 *     resource will be the input parameter of other mtk_mutex related APIs.
 * @param[in]
 *      dev: mutex device node.
 * @param[in]
 *      id: mutex resource id.
 * @return
 *     Specified mutex resource address, success to get mutex resource.\n
 *     ERR_PTR(-EINVAL), wrong input parameters.\n
 *     ERR_PTR(-EBUSY), if this mutex is used by other user.
 * @par Boundary case and Limitation
 *     1. dev can not be NULL.\n
 *     2. Input id should be less than the MUTEX_MAX_RES_NUM.
 * @par Error case and Error handling
 *     1. If dev is NULL, return ERR_PTR(-EINVAL).\n
 *     2. If id is more than or equal to the maximum resource number,\n
 *     return ERR_PTR(-EINVAL).\n
 *     3. If the specified mutex resource is used by other user,\n
 *     return ERR_PTR(-EBUSY).
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct mtk_mutex_res *mtk_mutex_get(const struct device *dev, unsigned int id)
{
	unsigned long flags;
	struct mtk_mutex *mutex;

	if (!dev)
		return ERR_PTR(-EINVAL);
	if (id >= MUTEX_MAX_RES_NUM)
		return ERR_PTR(-EINVAL);

	mutex = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(mutex))
		return ERR_PTR(-EINVAL);

	spin_lock_irqsave(&mutex->lock, flags);
	if (mutex->mutex_res[id].claimed) {
		spin_unlock_irqrestore(&mutex->lock, flags);
		return ERR_PTR(-EBUSY);
	}
	mutex->mutex_res[id].claimed = true;
	spin_unlock_irqrestore(&mutex->lock, flags);

	return &mutex->mutex_res[id];
}
EXPORT_SYMBOL(mtk_mutex_get);

/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Get Mutex id of the specified Mutex Resource.
 * @param[in]
 *     mutex_res: pointer of Mutex Resource.
 * @return
 *     id of the Mutex Resource.\n
 *     -EINVAL, if this Mutex Resource is invalid.
 * @par Boundary case and Limitation
 *     None
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_res2id(struct mtk_mutex_res *mutex_res)
{
	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	return mutex_res->id;
}
EXPORT_SYMBOL(mtk_mutex_res2id);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Release Mutex Resource.\n
 *     User can release the mutex resource by calling this API. Mutex resource\n
 *     can not be get again until it is released.
 * @param[out]
 *     mutex_res: mutex resource address.
 * @return
 *     0, release resource success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     If mutex_res is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_put(struct mtk_mutex_res *mutex_res)
{
	unsigned long flags;
	struct mtk_mutex *mutex;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	spin_lock_irqsave(&mutex->lock, flags);
	mutex_res->claimed = false;
	spin_unlock_irqrestore(&mutex->lock, flags);
	return 0;
}
EXPORT_SYMBOL(mtk_mutex_put);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Get Mutex Delay Resource.\n
 *     It is pure SW function for getting Mutex Delay SW resource. User needs\n
 *     to call it and get valid mutex delay resource. The mutex delay resource\n
 *     will be the input parameter of other mtk_mutex_delay related APIs.
 * @param[in]
 *     dev: mutex device node.
 * @param[in]
 *     id: mutex delay resource id.
 * @return
 *     Specified mutex resource address, success to get mutex delay resource.\n
 *     ERR_PTR(-EINVAL), wrong input parameters.\n
 *     ERR_PTR(-EBUSY), if this mutex delay resource is used by other user.
 * @par Boundary case and Limitation
 *     1. dev can not be NULL.\n
 *     2. Input id should be less than the HW delay resource number.
 * @par Error case and Error handling
 *     1. If dev is NULL, return ERR_PTR(-EINVAL).\n
 *     2. If id is more than or equal to the HW delay resource number,\n
 *     return ERR_PTR(-EINVAL).\n
 *     3. If the specified delay resource is blocked by other user,\n
 *     return ERR_PTR(-EBUSY).
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct mtk_mutex_res *mtk_mutex_delay_get(const struct device *dev,
					  unsigned int id)
{
	unsigned long flags;
	struct mtk_mutex *mutex;
	u32 i, cnt;

	if (!dev)
		return ERR_PTR(-EINVAL);

	mutex = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(mutex))
		return ERR_PTR(-EINVAL);
	if (id >= mutex->delay_res_num)
		return ERR_PTR(-EINVAL);

	spin_lock_irqsave(&mutex->lock, flags);
	if (mutex->mutex_delay_res[id].claimed) {
		spin_unlock_irqrestore(&mutex->lock, flags);
		return ERR_PTR(-EBUSY);
	}
	mutex->mutex_delay_res[id].claimed = true;
	spin_unlock_irqrestore(&mutex->lock, flags);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++)
		writel(F26M_ENABLE, mutex->regs[i] + DISP_MUTEX_F26M_CNT_CFG);

	return &mutex->mutex_delay_res[id];
}
EXPORT_SYMBOL(mtk_mutex_delay_get);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Release Mutex Delay Resource.\n
 *     User can release the mutex delay resource by calling this API. Mutex\n
 *     delay resource can not be get again until it is released.
 * @param[out]
 *     mutex_delay_res: mutex delay resource address.
 * @return
 *     0, release resource success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mutex_delay_res can not be NULL.\n
 *     2. Call mtk_mutex_delay_get function first before calling this API.
 * @par Error case and Error handling
 *     If mutex_delay_res is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_delay_put(struct mtk_mutex_res *mutex_delay_res)
{
	unsigned long flags;
	struct mtk_mutex *mutex;

	if (mtk_mutex_delay_res_chk(mutex_delay_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);

	spin_lock_irqsave(&mutex->lock, flags);
	mutex_delay_res->claimed = false;
	spin_unlock_irqrestore(&mutex->lock, flags);
	return 0;
}
EXPORT_SYMBOL(mtk_mutex_delay_put);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Turn On Mutex Clock.
 * @param[in]
 *     mutex_res: mutex resource.
 * @return
 *     0, successful to turn on clock.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from pm_runtime_get_sync().\n
 *     error code from clk_prepare_enable().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If pm_runtime_get_sync() fails, return its error code.\n
 *     3. If clk_prepare_enable() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_power_on(const struct mtk_mutex_res *mutex_res)
{
	struct mtk_mutex *mutex;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

#if defined(CONFIG_COMMON_CLK_MT3612)
	ret = pm_runtime_get_sync(mutex->dev);
	if (ret < 0) {
		dev_err(mutex->dev, "Failed to enable power domain: %d\n", ret);
		return ret;
	}

	if (mutex->clk) {
		ret = clk_prepare_enable(mutex->clk);
		if (ret) {
			pm_runtime_put(mutex->dev);
			dev_err(mutex->dev, "Failed to enable clock:%d\n", ret);
			return ret;
		}
	}

	if (mutex->clk_26M) {
		ret = clk_prepare_enable(mutex->clk_26M);
		if (ret) {
			clk_disable_unprepare(mutex->clk);
			pm_runtime_put(mutex->dev);
			dev_err(mutex->dev,
				"Failed to enable 26M clock:%d\n", ret);
			return ret;
		}
	}
#endif

	return ret;
}
EXPORT_SYMBOL(mtk_mutex_power_on);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Turn Off Mutex Clock.
 * @param[in]
 *     mutex_res: mutex resource.
 * @return
 *     0, successful to turn off clock.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     If mutex_res is invalid, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_power_off(const struct mtk_mutex_res *mutex_res)
{
	struct mtk_mutex *mutex;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

#if defined(CONFIG_COMMON_CLK_MT3612)
	if (mutex->clk_26M)
		clk_disable_unprepare(mutex->clk_26M);
	if (mutex->clk)
		clk_disable_unprepare(mutex->clk);
	pm_runtime_put(mutex->dev);
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_mutex_power_off);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Set Error Detection of One Mutex as the Source of Ext Signal. Each AH\n
 *     detection result can be the source of ext signal. The ext signal will\n
 *     be true if one of AH detection results is true. This API is used to add\n
 *     one of AH detection of mutex unit as the source of ext signal.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     ext_signal_id: ext signal id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. ext_signal_id should be defined in mtk_mutex_ext_signal_index and\n
 *     smaller than MUTEX_EXT_SIGNAL_MAX.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If ext_signal_id is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_add_to_ext_signal(const struct mtk_mutex_res *mutex_res,
				enum mtk_mutex_ext_signal_index ext_signal_id,
				struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_ext_signal(ext_signal_id))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				 MUTEX_EXTSIG_MUTEX_MASK(ext_signal_id & 0x1f));
			reg |= BIT(mutex_res->id);
			writel(reg, mutex->regs[i] +
				 MUTEX_EXTSIG_MUTEX_MASK(ext_signal_id & 0x1f));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
				  mutex->gce_subsys[i],
				  mutex->gce_subsys_offset[i] +
				  MUTEX_EXTSIG_MUTEX_MASK(ext_signal_id & 0x1f),
				  BIT(mutex_res->id),
				  BIT(mutex_res->id));
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_add_to_ext_signal);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Remove Error Detection of One Mutex from the Source of Ext Signal.\n
 *     Similar with mtk_mutex_add_to_ext_signal API, this API is used to\n
 *     remove AH detection result from ext signal.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     ext_signal_id: ext signal id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. ext_signal_id should be defined in mtk_mutex_ext_signal_index and\n
 *     smaller than MUTEX_EXT_SIGNAL_MAX.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If ext_signal_id is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_remove_from_ext_signal(const struct mtk_mutex_res *mutex_res,
				     enum mtk_mutex_ext_signal_index
				     ext_signal_id, struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_ext_signal(ext_signal_id))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				 MUTEX_EXTSIG_MUTEX_MASK(ext_signal_id & 0x1f));
			reg &= ~(BIT(mutex_res->id));
			writel(reg, mutex->regs[i] +
				 MUTEX_EXTSIG_MUTEX_MASK(ext_signal_id & 0x1f));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
				  mutex->gce_subsys[i],
				  mutex->gce_subsys_offset[i] +
				  MUTEX_EXTSIG_MUTEX_MASK(ext_signal_id & 0x1f),
				  0, BIT(mutex_res->id));
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_remove_from_ext_signal);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Clear Mutex Error Status.\n
 *     After AH function is enabled and error signal is detected, the error\n
 *     status bit will be true. User can clear the status bit by calling this\n
 *     API. Please recover HW from error state first and then clear error.\n
 *     Otherwise, the error status will be true again immediately.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, successful to clear error state.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_error_clear(const struct mtk_mutex_res *mutex_res,
			  struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
			reg &= (~(AH_ERROR_BIT | AH_REF_ERROR_BIT |
				  AH_EXT_SIGNAL_BIT));
			writel(reg, mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
						 mutex->gce_subsys[i],
						 mutex->gce_subsys_offset[i] +
						 MUTEX_ENABLE(mutex_res->id), 0,
						 (AH_ERROR_BIT |
						  AH_REF_ERROR_BIT |
						  AH_EXT_SIGNAL_BIT));
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_error_clear);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Select Which Error to be Monitored by AH Function.\n
 *     The AH function in MUTEX can monitor the error signal from sub-modules.\n
 *     With this API, you can choose which error id you want to monitor by AH\n
 *     function.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     error_id: error event id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. error_id should be defined in mtk_mutex_error_id and\n
 *     smaller than MUTEX_ERROR_ID_MAX.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If error_id is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_add_monitor(const struct mtk_mutex_res *mutex_res,
			  enum mtk_mutex_error_id error_id,
			  struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_err(error_id))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if ((error_id & 0x1f) > 15) {
			if (!handle) {
				reg = readl(mutex->regs[i] +
					    MUTEX_RESET(mutex_res->id));
				reg |= BIT(error_id & 0x1f);
				writel(reg, mutex->regs[i] +
					    MUTEX_RESET(mutex_res->id));
			} else
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_RESET(mutex_res->id),
						   BIT(error_id & 0x1f),
						   BIT(error_id & 0x1f));
		} else {
			if (!handle) {
				reg = readl(mutex->regs[i] +
					    MUTEX_AH_ERR_MASK(mutex_res->id));
				reg |= BIT(error_id & 0x1f);
				writel(reg, mutex->regs[i] +
					    MUTEX_AH_ERR_MASK(mutex_res->id));
			} else
				ret |= cmdq_pkt_write_value(handle[i],
					       mutex->gce_subsys[i],
					       mutex->gce_subsys_offset[i] +
					       MUTEX_AH_ERR_MASK(mutex_res->id),
					       BIT(error_id & 0x1f),
					       BIT(error_id & 0x1f));
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_add_monitor);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Remove Error being Monitored by AH Function.\n
 *     Similar with mtk_mutex_add_monitor API, this API is used to remove\n
 *     error signal which is detected by AH function.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     error_id: error event id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. error_id should be defined in mtk_mutex_error_id and\n
 *     smaller than MUTEX_ERROR_ID_MAX.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If error_id is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_remove_monitor(const struct mtk_mutex_res *mutex_res,
			     enum mtk_mutex_error_id error_id,
			     struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_err(error_id))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if ((error_id & 0x1f) > 15) {
			if (!handle) {
				reg = readl(mutex->regs[i] +
					    MUTEX_RESET(mutex_res->id));
				reg &= ~(BIT(error_id & 0x1f));
				writel(reg, mutex->regs[i] +
					    MUTEX_RESET(mutex_res->id));
			} else
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_RESET(mutex_res->id),
						   0, BIT(error_id & 0x1f));
		} else {
			if (!handle) {
				reg = readl(mutex->regs[i] +
					    MUTEX_AH_ERR_MASK(mutex_res->id));
				reg &= ~(BIT(error_id & 0x1f));
				writel(reg, mutex->regs[i] +
					    MUTEX_AH_ERR_MASK(mutex_res->id));
			} else
				ret |= cmdq_pkt_write_value(handle[i],
					       mutex->gce_subsys[i],
					       mutex->gce_subsys_offset[i] +
					       MUTEX_AH_ERR_MASK(mutex_res->id),
					       0, BIT(error_id & 0x1f));
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_remove_monitor);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Enable Mutex Auto Hold Function.\n
 *     User need to set up related parameters of AH function by calling\n
 *     mtk_mutex_add_to_ext_signal and mtk_mutex_add_monitor before\n
 *     enableing AH function.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     auto_off: enable AH function to auto disable mutex.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_error_monitor_enable(const struct mtk_mutex_res *mutex_res,
				   struct cmdq_pkt **handle, bool auto_off)
{
	struct mtk_mutex *mutex;
	int ret = 0;
	u32 i, reg, cnt;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
			reg |= AH_EN | AH_EVENT_EN | AH_EXT_SIGNAL_EN;
			if (auto_off)
				reg |= AH_AUTO_OFF_MUTEX_EN;
			writel(reg, mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
		} else {
			reg = AH_EN | AH_EVENT_EN | AH_EXT_SIGNAL_EN;
			if (auto_off)
				reg |= AH_AUTO_OFF_MUTEX_EN;
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_ENABLE(mutex_res->id),
						   reg,
						   (AH_EN |
						    AH_AUTO_OFF_MUTEX_EN |
						    AH_EVENT_EN |
						    AH_EXT_SIGNAL_EN));
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_error_monitor_enable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Disable Mutex Auto Hold Function.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_error_monitor_disable(const struct mtk_mutex_res *mutex_res,
				    struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	int ret = 0;
	u32 i, reg, cnt;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
			reg &= (~(AH_EN | AH_AUTO_OFF_MUTEX_EN | AH_EVENT_EN |
				  AH_EXT_SIGNAL_EN));
			writel(reg, mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
						 mutex->gce_subsys[i],
						 mutex->gce_subsys_offset[i] +
						 MUTEX_ENABLE(mutex_res->id), 0,
						 (AH_EN | AH_AUTO_OFF_MUTEX_EN |
						  AH_EVENT_EN |
						  AH_EXT_SIGNAL_EN));
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_error_monitor_disable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Select Mutex Delay SOF Source.\n
 *     We have DRAM frame buffer or SYSRAM line buffer between write side and\n
 *     read side in some place. That means the timing of write side and read\n
 *     side should be different. The write side should be triggered earlier\n
 *     than read side which means we need to generate a earlier SOF.\n
 *     The mtk_mutex_select_delay_sof API is used to select the source of SOF\n
 *     to generate the delayed SOF.
 * @param[in]
 *     mutex_delay_res: mutex delay resource.
 * @param[in]
 *     sof_id: start of frame id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_delay_res can not be NULL.\n
 *     2. Call mtk_mutex_delay_get function first before calling this API.\n
 *     3. The value of sof_id should be defined in mtk_mutex_sof_id.
 * @par Error case and Error handling
 *     1. If mutex_delay_res is invalid, return -EINVAL.\n
 *     2. If sof_id is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_select_delay_sof(const struct mtk_mutex_res *mutex_delay_res,
			       enum mtk_mutex_sof_id sof_id,
			       struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex =
	    container_of(mutex_delay_res, struct mtk_mutex,
			 mutex_delay_res[mutex_delay_res->id]);
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_delay_res_chk(mutex_delay_res) < 0)
		return -EINVAL;
	if (is_invalid_sof(sof_id))
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_DELAY_CFG(mutex_delay_res->id));
			reg &= (~MUTEX_SYNC_DLY_0_SYNC_SEL);
			reg |= (((sof_id & 0x3f) - 1) << 24);
			writel(reg, mutex->regs[i] +
				    MUTEX_DELAY_CFG(mutex_delay_res->id));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
					   mutex->gce_subsys[i],
					   mutex->gce_subsys_offset[i] +
					   MUTEX_DELAY_CFG(mutex_delay_res->id),
					   (((sof_id & 0x3f) - 1) << 24),
					   MUTEX_SYNC_DLY_0_SYNC_SEL);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_select_delay_sof);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Set Delay Value for Delay SOF.\n
 *     After use mtk_mutex_select_delay_sof to choose SOF source for delay\n
 *     SOF, the mtk_mutex_set_delay_us API is used to control the delay value.
 * @param[in]
 *     mutex_delay_res: mutex delay resource.
 * @param[in]
 *     delay_time: delay time in us.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_delay_res can not be NULL.\n
 *     2. Call mtk_mutex_delay_get function first before calling this API.\n
 *     3. delay_time can not be larger than MUTEX_MAX_DELAY_IN_US.
 * @par Error case and Error handling
 *     1. If mutex_delay_res is invalid, return -EINVAL.\n
 *     2. If delay_time is larger than MUTEX_MAX_DELAY_IN_US, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_set_delay_us(const struct mtk_mutex_res *mutex_delay_res,
			   u32 delay_time, struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, dly_value, cnt;
	int ret = 0;

	if (mtk_mutex_delay_res_chk(mutex_delay_res) < 0)
		return -EINVAL;
	if (delay_time > MUTEX_MAX_DELAY_IN_US)
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);
	dly_value = (u32)((u64)delay_time * (u64)mutex->mutex_clock / 1000000);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_DELAY_CFG(mutex_delay_res->id));
			reg &= (~MUTEX_SYNC_DLY_0_DLY_VALUE);
			reg |= dly_value;
			writel(reg, mutex->regs[i] +
				    MUTEX_DELAY_CFG(mutex_delay_res->id));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
					   mutex->gce_subsys[i],
					   mutex->gce_subsys_offset[i] +
					   MUTEX_DELAY_CFG(mutex_delay_res->id),
					   dly_value,
					   MUTEX_SYNC_DLY_0_DLY_VALUE);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_set_delay_us);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Set Delay Cycle Value for Delay SOF.\n
 *     After use mtk_mutex_select_delay_sof to choose SOF source for delay\n
 *     SOF, this API is used to control the delay value.
 * @param[in]
 *     mutex_delay_res: mutex delay resource.
 * @param[in]
 *     clock: base clock frequency that used to estimate the cycles.
 * @param[in]
 *     cycles: delay cycles base on the specified clock.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_delay_res can not be NULL.\n
 *     2. Call mtk_mutex_delay_get function first before calling this API.\n
 *     3. clock or cycles can not be 0.
 * @par Error case and Error handling
 *     1. If mutex_delay_res is invalid, return -EINVAL.\n
 *     2. If clock or cycles is 0, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_set_delay_cycle(const struct mtk_mutex_res *mutex_delay_res,
			   u32 clock, u32 cycles, struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, dly_value, cnt;
	int ret = 0;

	if (mtk_mutex_delay_res_chk(mutex_delay_res) < 0)
		return -EINVAL;
	if (clock == 0 || cycles == 0)
		return -EINVAL;
	/* maximum of mutex_clock max 26MHz */
	if (clock > 26000000)
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);

	dly_value = (cycles * mutex->mutex_clock - 1) / clock + 1;
	if (dly_value > MUTEX_SYNC_DLY_0_DLY_VALUE) {
		dev_err(mutex->dev, "delay cycle (0x%x) overflow\n", dly_value);
		return -EINVAL;
	}

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_DELAY_CFG(mutex_delay_res->id));
			reg &= (~MUTEX_SYNC_DLY_0_DLY_VALUE);
			reg |= dly_value;
			writel(reg, mutex->regs[i] +
				    MUTEX_DELAY_CFG(mutex_delay_res->id));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
					   mutex->gce_subsys[i],
					   mutex->gce_subsys_offset[i] +
					   MUTEX_DELAY_CFG(mutex_delay_res->id),
					   dly_value,
					   MUTEX_SYNC_DLY_0_DLY_VALUE);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_set_delay_cycle);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Get SOF ID of the given mutex delay resource.\n
 * @param[in]
 *     mutex_delay_res: mutex delay resource.
 * @return
 *     Positive value, it's the sof id.\n
 *     -EINVAL, wrong input parameters.\n
 * @par Boundary case and Limitation
 *     1. mutex_delay_res can not be NULL.\n
 *     2. Call mtk_mutex_delay_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_delay_res is invalid, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_get_delay_sof(const struct mtk_mutex_res *mutex_delay_res)
{
	struct mtk_mutex *mutex;

	if (mtk_mutex_delay_res_chk(mutex_delay_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);

	return (int)mutex->delay_sof + mutex_delay_res->id;
}
EXPORT_SYMBOL(mtk_mutex_get_delay_sof);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Enable Mutex Delay Generator.\n
 *     After setting mutex delay SOF source and delay value, this API can be\n
 *     used to enable mutex delay generator. Then the Mutex hardware will\n
 *     generates delayed SOF timing.
 * @param[in]
 *     mutex_delay_res: mutex delay resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_delay_res can not be NULL.\n
 *     2. Call mtk_mutex_delay_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_delay_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_delay_enable(const struct mtk_mutex_res *mutex_delay_res,
			   struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_delay_res_chk(mutex_delay_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			writel(BIT(mutex_delay_res->id),
			       mutex->regs[i] + DISP_MUTEX_SYNC_DLY_RST);
			writel(0, mutex->regs[i] + DISP_MUTEX_SYNC_DLY_RST);
			reg = readl(mutex->regs[i] + DISP_MUTEX_SYNC_DLY_EN);
			reg |= BIT(mutex_delay_res->id);
			writel(reg, mutex->regs[i] + DISP_MUTEX_SYNC_DLY_EN);
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   DISP_MUTEX_SYNC_DLY_RST,
						   BIT(mutex_delay_res->id),
						   BIT(mutex_delay_res->id));
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   DISP_MUTEX_SYNC_DLY_RST, 0,
						   BIT(mutex_delay_res->id));
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   DISP_MUTEX_SYNC_DLY_EN,
						   BIT(mutex_delay_res->id),
						   BIT(mutex_delay_res->id));
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_delay_enable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Disable Mutex Delay Generator.
 * @param[in]
 *     mutex_delay_res: mutex delay resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration is done.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_delay_res can not be NULL.\n
 *     2. Call mtk_mutex_delay_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_delay_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_delay_disable(const struct mtk_mutex_res *mutex_delay_res,
			    struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_delay_res_chk(mutex_delay_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_delay_res, struct mtk_mutex,
			     mutex_delay_res[mutex_delay_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] + DISP_MUTEX_SYNC_DLY_EN);
			reg &= ~(BIT(mutex_delay_res->id));
			writel(reg, mutex->regs[i] + DISP_MUTEX_SYNC_DLY_EN);
		} else
			ret |= cmdq_pkt_write_value(handle[i],
						  mutex->gce_subsys[i],
						  mutex->gce_subsys_offset[i] +
						  DISP_MUTEX_SYNC_DLY_EN, 0,
						  BIT(mutex_delay_res->id));
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_delay_disable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Select Timer Debugger Source and Reference ID.\n
 *     Mutex will accumulate cycle count between Source SOF and Reference SOF\n
 *     until timeout. In software, we use it as source of gce pre-sync signal\n
 *     which is used for synchronization of 4 mmcore partitions.\n
 *     Thus, src_sofid usually will be the SOF which is same as module's SOF\n
 *     in sub path and ref_sofid will be MUTEX_MMSYS_SOF_SINGLE.\n
 *     You can use mtk_mutex_set_timer_us API to set timeout threshold.\n
 *     Because MUTEX_MMSYS_SOF_SINGLE is triggered by SW, it will timeout\n
 *     and the MUTEX will send specific event to GCE. GCE will use this event\n
 *     as pre-sync signal.\n
 *     For detail of GCE pre-sync, please refer to GCE documents.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     src_sofid: source SOF id.
 * @param[in]
 *     ref_sofid: reference SOF id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. src_sofid and ref_sofid should be defined in mtk_mutex_sof_id.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If src_sofid or ref_sofid is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_select_timer_sof(const struct mtk_mutex_res *mutex_res,
			       enum mtk_mutex_sof_id src_sofid,
			       enum mtk_mutex_sof_id ref_sofid,
			       struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	if (is_invalid_sof(src_sofid) || is_invalid_sof(ref_sofid))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	src_sofid &= 0x3f;
	ref_sofid &= 0x3f;
	ref_sofid = (ref_sofid == 0) ? INVALID_REF_SOF_ID : ref_sofid;
	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
			reg &= (~(REF_SOF_MASK | SRC_SOF_MASK));
			reg |= ((src_sofid - 1) << 8) | ((ref_sofid - 1) << 16);
			writel(reg, mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
						 mutex->gce_subsys[i],
						 mutex->gce_subsys_offset[i] +
						 MUTEX_TD_CFG(mutex_res->id),
						 ((src_sofid - 1) << 8) |
						 ((ref_sofid - 1) << 16),
						 (REF_SOF_MASK | SRC_SOF_MASK));
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_select_timer_sof);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Select SOF timing edge of Timer Debugger.\n
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     src_neg: set which timing to search, true is negative, false is positive.
 * @param[in]
 *     ref_neg: set which timing to search, true is negative, false is positive.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_select_timer_sof_timing(const struct mtk_mutex_res *mutex_res,
			       bool src_neg, bool ref_neg,
			       struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, val = 0, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);
	if (src_neg)
		val |= TD_SRC_EDGE;
	if (ref_neg)
		val |= TD_REF_EDGE;

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < mutex->hw_nr; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
			reg &= ~(TD_SRC_EDGE | TD_REF_EDGE);
			reg |= val;
			writel(reg, mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						 mutex->gce_subsys[i],
						 mutex->gce_subsys_offset[i] +
						 MUTEX_TD_CFG(mutex_res->id),
						 val,
						 TD_SRC_EDGE | TD_REF_EDGE);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_select_timer_sof_timing);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Set Timer Debugger Timeout Value.\n
 *     There are two timeout threshold user needs to set. One is source SOF\n
 *     timeout threshold which will usually be set a large value to make sure\n
 *     that Mutex HW can detect the source SOF in time. The other is reference\n
 *     SOF timeout value which will usually be smaller than vsync period so\n
 *     Mutex HW can generate timeout event as GCE pre-sync signal.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     src_time: source SOF timeout value.
 * @param[in]
 *     ref_time: reference SOF timeout value.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. src_time or ref_time can not be larger than MUTEX_MAX_TIMER_THD_IN_US.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If src_time or ref_time is larger than MUTEX_MAX_TIMER_THD_IN_US,\n
 *     return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_set_timer_us(const struct mtk_mutex_res *mutex_res, u32 src_time,
			   u32 ref_time, struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	if ((src_time > MUTEX_MAX_TIMER_THD_IN_US) ||
	    (ref_time > MUTEX_MAX_TIMER_THD_IN_US))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	/*need to change 26M to 320M for real IC clock*/
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			writel(src_time * mutex->mm_clock, mutex->regs[i] +
					  MUTEX_TD_CFG1(mutex_res->id));
			writel(ref_time * mutex->mm_clock, mutex->regs[i] +
					  MUTEX_TD_CFG2(mutex_res->id));
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
					   mutex->gce_subsys[i],
					   mutex->gce_subsys_offset[i] +
					   MUTEX_TD_CFG1(mutex_res->id),
					   src_time * mutex->mm_clock,
					   MUTEX_TDEBUG_0_SRC_TIMEOUT);
			ret |= cmdq_pkt_write_value(handle[i],
					   mutex->gce_subsys[i],
					   mutex->gce_subsys_offset[i] +
					   MUTEX_TD_CFG2(mutex_res->id),
					   ref_time * mutex->mm_clock,
					   MUTEX_TDEBUG_0_REF_TIMEOUT);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_set_timer_us);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Enable Timer Debugger Function.\n
 *     Please set up parameters of timer debugger function by calling\n
 *     mtk_mutex_select_timer_sof and mtk_mutex_set_timer_us before enabled.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     loop: loop mode.
 * @param[in]
 *     event: timeout condition selection.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. event should be defined in mtk_mutex_timeout_event.\n
 *     3. loop should be either 1 (true) or 0 (false).\n
 *     4. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If event is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_timer_enable_ex(const struct mtk_mutex_res *mutex_res, bool loop,
			   enum mtk_mutex_timeout_event event,
			   struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_timeout_event(event))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id)) & (~TD_RESET);
			writel(reg | TD_RESET,
			       mutex->regs[i] + MUTEX_TD_CFG(mutex_res->id));
			writel(reg,
			       mutex->regs[i] + MUTEX_TD_CFG(mutex_res->id));

			reg = readl(mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
			reg &= (~(TD_ENABLE | TD_LOOP_MODE | EVENT_SEL));
			reg |= TD_ENABLE | (loop << 2) | (event << 5);
			writel(reg, mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_TD_CFG(mutex_res->id),
						   TD_RESET, TD_RESET);
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_TD_CFG(mutex_res->id),
						   0, TD_RESET);
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_TD_CFG(mutex_res->id),
						   TD_ENABLE | (loop << 2) |
						   (event << 5),
						   (TD_ENABLE | TD_LOOP_MODE |
						    EVENT_SEL));
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_timer_enable_ex);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Enable Timer Debugger Function.\n
 *     Please set up parameters of timer debugger function by calling\n
 *     mtk_mutex_select_timer_sof and mtk_mutex_set_timer_us before enabled.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     loop: loop mode.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     error code from mtk_mutex_timer_enable_ex().
 * @par Boundary case and Limitation
 *     1. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mtk_mutex_timer_enable_ex() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_timer_enable(const struct mtk_mutex_res *mutex_res, bool loop,
			   struct cmdq_pkt **handle)
{
	return mtk_mutex_timer_enable_ex(mutex_res, loop,
					MUTEX_TO_EVENT_REF_OVERFLOW, handle);
}
EXPORT_SYMBOL(mtk_mutex_timer_enable);

/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Get Timer Debugger Configuration Status.\n
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     status: pointer of struct mtk_mutex_timer_status.
 * @return
 *     0, configuration success.\n
 *     error code if parameter is invalid.
 * @par Boundary case and Limitation
 *     1. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If status is invalid, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_timer_get_status(const struct mtk_mutex_res *mutex_res,
				struct mtk_mutex_timer_status *status)
{
	struct mtk_mutex *mutex;
	unsigned int reg;
	int id;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (status == NULL)
		return -EINVAL;

	id = mutex_res->id;
	mutex = container_of(mutex_res, struct mtk_mutex, mutex_res[id]);
	reg = readl(mutex->regs[0] + MUTEX_TD_CFG(id));
	memcpy(status, &reg, sizeof(u32));
	status->src_timeout = readl(mutex->regs[0] + MUTEX_TD_CFG1(id));
	status->ref_timeout = readl(mutex->regs[0] + MUTEX_TD_CFG2(id));
	status->src_time = readl(mutex->regs[0] + MUTEX_TD_CFG3(id));
	status->ref_time = readl(mutex->regs[0] + MUTEX_TD_CFG4(id));
	status->mm_clock = mutex->mm_clock;

	return 0;
}
EXPORT_SYMBOL(mtk_mutex_timer_get_status);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Disable Timer Debugger Function.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_timer_disable(const struct mtk_mutex_res *mutex_res,
			    struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			reg = readl(mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
			reg &= (~(TD_ENABLE | TD_LOOP_MODE));
			writel(reg, mutex->regs[i] +
				    MUTEX_TD_CFG(mutex_res->id));
		} else
			ret |= cmdq_pkt_write_value(handle[i],
						 mutex->gce_subsys[i],
						 mutex->gce_subsys_offset[i] +
						 MUTEX_TD_CFG(mutex_res->id),
						 0, (TD_ENABLE | TD_LOOP_MODE));
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_timer_disable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Select Timer Debugger Source and Reference ID.\n
 *     Mutex will accumulate cycle count between Source SOF and Reference SOF\n
 *     until timeout. In software, we use it as source of gce pre-sync signal\n
 *     which is used for synchronization of 4 mmcore partitions.\n
 *     Thus, src_sofid usually will be the SOF which is same as module's SOF\n
 *     in sub path and ref_sofid will be MUTEX_MMSYS_SOF_SINGLE.\n
 *     You can use mtk_mutex_set_timer_us API to set timeout threshold.\n
 *     Because MUTEX_MMSYS_SOF_SINGLE is triggered by SW, it will timeout\n
 *     and the MUTEX will send specific event to GCE. GCE will use this event\n
 *     as pre-sync signal.\n
 *     For detail of GCE pre-sync, please refer to GCE documents.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     src_sofid: source SOF id.
 * @param[in]
 *     ref_sofid: reference SOF id.
 * @param[in]
 *     work_reg_idx_0: it specifies one of the working registers of GCE.
 * @param[in]
 *     work_reg_idx_1: it specifies one of the working registers of GCE.
 * @param[out]
 *     handle: CMDQ handle.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_writel_mask().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. handle can not be NULL.\n
 *     4. src_sofid and ref_sofid should be defined in mtk_mutex_sof_id.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If src_sofid or ref_sofid is invalid, return -EINVAL.\n
 *     3. If handle is NULL, return -EINVAL.\n
 *     4. If cmdq_pkt_writel_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_select_timer_sof_w_gce5(const struct mtk_mutex_res *mutex_res,
					enum mtk_mutex_sof_id src_sofid,
					enum mtk_mutex_sof_id ref_sofid,
					u16 work_reg_idx_0,
					u16 work_reg_idx_1,
					struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i,  cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_sof(src_sofid) || is_invalid_sof(ref_sofid))
		return -EINVAL;
	if (!handle)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	src_sofid &= 0x3f;
	ref_sofid &= 0x3f;
	ref_sofid = (ref_sofid == 0) ? INVALID_REF_SOF_ID : ref_sofid;
	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		ret |= cmdq_pkt_writel_mask(handle[i],
			mutex->reg_base[i] + MUTEX_TD_CFG(mutex_res->id),
			((src_sofid - 1) << 8) | ((ref_sofid - 1) << 16),
			(REF_SOF_MASK | SRC_SOF_MASK),
			work_reg_idx_0,
			work_reg_idx_1);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_select_timer_sof_w_gce5);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Set Timer Debugger Timeout Value.\n
 *     There are two timeout threshold user needs to set. One is source SOF\n
 *     timeout threshold which will usually be set a large value to make sure\n
 *     that Mutex HW can detect the source SOF in time. The other is reference\n
 *     SOF timeout value which will usually be smaller than vsync period so\n
 *     Mutex HW can generate timeout event as GCE pre-sync signal.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     src_time: source SOF timeout value.
 * @param[in]
 *     ref_time: reference SOF timeout value.
 * @param[in]
 *     work_reg_idx_0: it specifies one of the working registers of GCE.
 * @param[in]
 *     work_reg_idx_1: it specifies one of the working registers of GCE.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_writel_mask().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. handle can not be NULL.\n
 *     4. src_time or ref_time can not be larger than MUTEX_MAX_TIMER_THD_IN_US.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If handle is NULL, return -EINVAL.\n
 *     3. If src_time or ref_time is larger than MUTEX_MAX_TIMER_THD_IN_US,
 *     return -EINVAL.\n
 *     4. If cmdq_pkt_writel_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_set_timer_us_w_gce5(const struct mtk_mutex_res *mutex_res,
				u32 src_time, u32 ref_time,
				u16 work_reg_idx_0, u16 work_reg_idx_1,
				struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (!handle)
		return -EINVAL;
	if ((src_time > MUTEX_MAX_TIMER_THD_IN_US) ||
	    (ref_time > MUTEX_MAX_TIMER_THD_IN_US))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		ret |= cmdq_pkt_writel_mask(handle[i],
			mutex->reg_base[i] + MUTEX_TD_CFG1(mutex_res->id),
			src_time * mutex->mm_clock,
			MUTEX_TDEBUG_0_SRC_TIMEOUT,
			work_reg_idx_0,
			work_reg_idx_1);
		ret |= cmdq_pkt_writel_mask(handle[i],
			mutex->reg_base[i] + MUTEX_TD_CFG2(mutex_res->id),
			ref_time * mutex->mm_clock,
			MUTEX_TDEBUG_0_REF_TIMEOUT,
			work_reg_idx_0,
			work_reg_idx_1);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_set_timer_us_w_gce5);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Enable Timer Debugger Function.\n
 *     Please set up parameters of timer debugger function by calling\n
 *     mtk_mutex_select_timer_sof and mtk_mutex_set_timer_us before enabled.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     loop: loop mode.
 * @param[in]
 *     event: timeout condition selection.
 * @param[in]
 *     work_reg_idx_0: it specifies one of the working registers of GCE.
 * @param[in]
 *     work_reg_idx_1: it specifies one of the working registers of GCE.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_writel_mask().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. event should be defined in mtk_mutex_timeout_event.\n
 *     3. loop should be either 1 (true) or 0 (false).\n
 *     4. Call mtk_mutex_get function first before calling this API.\n
 *     5. handle can not be NULL.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If event is invalid, return -EINVAL.\n
 *     3. If handle is NULL, return -EINVAL.\n
 *     4. If cmdq_pkt_writel_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_timer_enable_ex_w_gce5(const struct mtk_mutex_res *mutex_res,
				bool loop, enum mtk_mutex_timeout_event event,
				u16 work_reg_idx_0, u16 work_reg_idx_1,
				struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_timeout_event(event))
		return -EINVAL;
	if (!handle)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		ret |= cmdq_pkt_writel_mask(handle[i],
			mutex->reg_base[i] + MUTEX_TD_CFG(mutex_res->id),
			TD_RESET,
			TD_RESET,
			work_reg_idx_0,
			work_reg_idx_1);
		ret |= cmdq_pkt_writel_mask(handle[i],
			mutex->reg_base[i] + MUTEX_TD_CFG(mutex_res->id),
			0,
			TD_RESET,
			work_reg_idx_0,
			work_reg_idx_1);
		ret |= cmdq_pkt_writel_mask(handle[i],
			mutex->reg_base[i] + MUTEX_TD_CFG(mutex_res->id),
			TD_ENABLE | (loop << 2) | (event << 5),
			TD_ENABLE | TD_LOOP_MODE | EVENT_SEL,
			work_reg_idx_0,
			work_reg_idx_1);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_timer_enable_ex_w_gce5);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Disable Timer Debugger Function.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     work_reg_idx_0: it specifies one of the working registers of GCE.
 * @param[in]
 *     work_reg_idx_1: it specifies one of the working registers of GCE.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_writel_mask().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. handle can not be NULL.\n
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If handle is NULL, return -EINVAL.\n
 *     3. If cmdq_pkt_writel_mask() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_timer_disable_w_gce5(const struct mtk_mutex_res *mutex_res,
				u16 work_reg_idx_0, u16 work_reg_idx_1,
				struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (!handle)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {

		ret |= cmdq_pkt_writel_mask(handle[i],
			mutex->reg_base[i] + MUTEX_TD_CFG(mutex_res->id),
			0,
			TD_ENABLE | TD_LOOP_MODE,
			work_reg_idx_0,
			work_reg_idx_1);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_timer_disable_w_gce5);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Select Mutex SOF and EOF Source.\n
 *     There are a lot of timing sources which can be Mutex SOF or EOF source.\n
 *     Please refer to mtk_mutex_sof_id enum to see the choice for different\n
 *     partitions. User need to choose proper SOF or EOF source according to\n
 *     different scenario.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     sofid: SOF source module id.
 * @param[in]
 *     eofid: EOF source module id.
 * @param[in]
 *     sof_timing: Specify the timing that Mutex sends SOF to its components.
 * @param[in]
 *     eof_timing: Specify the timing that Mutex sends EOF to its components.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. sofid and eofid should be defined in mtk_mutex_sof_id.
 *     4. sof_timing should be defined in mtk_mutex_sof_timing.
 *     5. eof_timing should be defined in mtk_mutex_eof_timing.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If sofid or eofid is invalid, return -EINVAL.\n
 *     3. If sof_timing or eof_timing is invalid, return -EINVAL.\n
 *     4. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_select_sof_eof(const struct mtk_mutex_res *mutex_res,
			enum mtk_mutex_sof_id sofid,
			enum mtk_mutex_sof_id eofid,
			enum mtk_mutex_sof_timing sof_timing,
			enum mtk_mutex_eof_timing eof_timing,
			struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (is_invalid_sof(sofid))
		return -EINVAL;
	if (is_invalid_sof(eofid))
		return -EINVAL;
	if (is_invalid_sof_timing(sof_timing))
		return -EINVAL;
	if (is_invalid_eof_timing(eof_timing))
		return -EINVAL;
	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			writel(MUTEX_GET,
				mutex->regs[i] + MUTEX_GETN(mutex_res->id));
			reg = readl(mutex->regs[i] +
				    MUTEX_CTRL(mutex_res->id));
			reg &= (~(MUTEX_SOF | MUTEX_SOF_TIMING |
				  MUTEX_EOF | MUTEX_EOF_TIMING));
			reg |= ((eofid & 0x3f) << 8) | (sofid & 0x3f);
			reg |= (eof_timing << 14) | (sof_timing << 6);
			writel(reg, mutex->regs[i] + MUTEX_CTRL(mutex_res->id));
			writel(0, mutex->regs[i] + MUTEX_GETN(mutex_res->id));
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   MUTEX_GET, MUTEX_GET);
			ret |= cmdq_pkt_write_value(handle[i],
						  mutex->gce_subsys[i],
						  mutex->gce_subsys_offset[i] +
						  MUTEX_CTRL(mutex_res->id),
						  ((eofid & 0x3f) << 8) |
						  (sofid & 0x3f) |
						  (eof_timing << 14) |
						  (sof_timing << 6),
						  (MUTEX_SOF |
						   MUTEX_SOF_TIMING |
						   MUTEX_EOF |
						   MUTEX_EOF_TIMING));
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   0, MUTEX_GET);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_select_sof_eof);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Select Mutex SOF Source.\n
 *     There are a lot of timing sources which can be Mutex SOF source.\n
 *     Please refer to mtk_mutex_sof_id enum to see the choice for different\n
 *     partitions. User need to choose proper SOF source according to\n
 *     different scenario.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     sofid: SOF id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     only_vsync: Whether SOF source has only vsync but no vde signal.\n
 *                 If the SOF source is from ISP or DP, set it as true.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. sofid should be defined in mtk_mutex_sof_id.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If sof_id is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_select_sof(const struct mtk_mutex_res *mutex_res,
			 enum mtk_mutex_sof_id sofid,
			 struct cmdq_pkt **handle, bool only_vsync)
{
	enum mtk_mutex_eof_timing eof_timing = MUTEX_EOF_VDE_FALLING;

	if (only_vsync)
		eof_timing = MUTEX_EOF_VDE_RISING;

	return mtk_mutex_select_sof_eof(mutex_res, sofid, sofid,
		MUTEX_SOF_VSYNC_FALLING, eof_timing, handle);
}
EXPORT_SYMBOL(mtk_mutex_select_sof);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Add Component to One Mutex Unit.\n
 *     All Components on the same mutex unit will receive the SOF/EOF signals\n
 *     at the same time. Thus, user needs to add the components which need to\n
 *     run with the same timing to the same mutex unit.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     compid: component id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. compid should be one of mtk_mutex_comp_id.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If compid is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_add_comp(const struct mtk_mutex_res *mutex_res,
		       enum mtk_mutex_comp_id compid, struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_comp(compid))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			writel(MUTEX_GET,
				mutex->regs[i] + MUTEX_GETN(mutex_res->id));
			if ((compid & 0x3f) > 31) {
				reg = readl(mutex->regs[i] +
					    MUTEX_MODN1(mutex_res->id));
				reg |= BIT(compid & 0x1f);
				writel(reg, mutex->regs[i] +
					    MUTEX_MODN1(mutex_res->id));
			} else {
				reg = readl(mutex->regs[i] +
					    MUTEX_MODN0(mutex_res->id));
				reg |= BIT(compid & 0x1f);
				writel(reg, mutex->regs[i] +
					    MUTEX_MODN0(mutex_res->id));
			}
			writel(0, mutex->regs[i] + MUTEX_GETN(mutex_res->id));
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   MUTEX_GET, MUTEX_GET);
			if ((compid & 0x3f) > 31)
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_MODN1(mutex_res->id),
						   BIT(compid & 0x1f),
						   BIT(compid & 0x1f));
			else
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_MODN0(mutex_res->id),
						   BIT(compid & 0x1f),
						   BIT(compid & 0x1f));
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   0, MUTEX_GET);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_add_comp);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Remove Component from one Mutex Unit.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[in]
 *     compid: component id.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.\n
 *     3. compid should be one of mtk_mutex_comp_id.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If compid is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_remove_comp(const struct mtk_mutex_res *mutex_res,
			  enum mtk_mutex_comp_id compid,
			  struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, cnt;
	int ret = 0;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;
	if (is_invalid_comp(compid))
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			writel(MUTEX_GET,
				mutex->regs[i] + MUTEX_GETN(mutex_res->id));
			if ((compid & 0x3f) > 31) {
				reg = readl(mutex->regs[i] +
					    MUTEX_MODN1(mutex_res->id));
				reg &= ~(BIT(compid & 0x1f));
				writel(reg, mutex->regs[i] +
					    MUTEX_MODN1(mutex_res->id));
			} else {
				reg = readl(mutex->regs[i] +
					    MUTEX_MODN0(mutex_res->id));
				reg &= ~(BIT(compid & 0x1f));
				writel(reg, mutex->regs[i] +
					    MUTEX_MODN0(mutex_res->id));
			}
			writel(0, mutex->regs[i] + MUTEX_GETN(mutex_res->id));
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   MUTEX_GET, MUTEX_GET);
			if ((compid & 0x3f) > 31)
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_MODN1(mutex_res->id),
						   0, BIT(compid & 0x1f));
			else
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_MODN0(mutex_res->id),
						   0, BIT(compid & 0x1f));
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   0, MUTEX_GET);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_remove_comp);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Set EOF mask of one component.
 * @param[in]
 *     dev: mutex device node.
 * @param[in]
 *     compid: component id.
 * @param[in]
 *     receive_eof: Set true if the component wants receiving eof signal from\n
 *     mutex, and vice versa.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. dev can not be NULL.\n
 *     2. compid should be one of mtk_mutex_comp_id.
 * @par Error case and Error handling
 *     1. If dev is invalid, return -EINVAL.\n
 *     2. If compid is invalid, return -EINVAL.\n
 *     3. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_set_comp_eof_mask(const struct device *dev,
			       enum mtk_mutex_comp_id compid, bool receive_eof,
			       struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, reg, offset, cnt;
	int ret = 0;

	if (!dev)
		return -EINVAL;
	if (is_invalid_comp(compid))
		return -EINVAL;
	mutex = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(mutex))
		return -EINVAL;

	if ((compid & 0x3f) > 31)
		offset = DISP_MUTEX_EOF_MASK_MSB;
	else
		offset = DISP_MUTEX_EOF_MASK_LSB;

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			void *addr;

			addr = mutex->regs[i] + offset;
			reg = readl(addr);
			if (receive_eof)
				reg &= ~(BIT(compid & 0x1f));
			else
				reg |= BIT(compid & 0x1f);
			writel(reg, addr);
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
				   mutex->gce_subsys[i],
				   mutex->gce_subsys_offset[i] + offset,
				   (receive_eof ? 0 : BIT(compid & 0x1f)),
				   BIT(compid & 0x1f));
		}
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mutex_set_comp_eof_mask);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Enable Mutex Engine.\n
 *     The Mutex HW will send SOF signal to components which are added to\n
 *     mutex after enabled. For SOF_SINGLE mode, the enable bit will become as\n
 *     0 automatically if all components on Mutex return ready ack. For other\n
 *     modes, Mutex will send SOF signal periodically according to SOF source.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value()/_poll() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_enable(const struct mtk_mutex_res *mutex_res,
		     struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	int ret = 0;
	u32 i, reg, cnt;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			void *addr;

			addr = mutex->regs[i] + MUTEX_ENABLE(mutex_res->id);
			reg = readl(addr) | MUTEX_EN;
			writel(reg, addr);
			addr = mutex->regs[i] + MUTEX_GETN(mutex_res->id);
			reg = readl(addr) | MUTEX_GET;
			writel(reg, addr);
			ret |= readl_poll_timeout_atomic(addr, reg,
					reg & INT_MUTEX_GET, 1, 1000);
			reg = readl(addr) & ~MUTEX_GET;
			writel(reg, addr);
			ret |= readl_poll_timeout_atomic(addr, reg,
					!(reg & INT_MUTEX_GET), 1, 1000);
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_ENABLE(mutex_res->id),
						   MUTEX_EN, MUTEX_EN);
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   MUTEX_GET, MUTEX_GET);
			ret |= cmdq_pkt_poll(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   INT_MUTEX_GET,
						   INT_MUTEX_GET);
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   0, MUTEX_GET);
			ret |= cmdq_pkt_poll(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   0, INT_MUTEX_GET);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_enable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Reset and Disable Mutex Engine.\n
 *     User does not need to call this API if it is SOF_SINGLE mode.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @param[in]
 *     reset: do mutex reset or not.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     1. mutex_res can not be NULL.\n
 *     2. Call mtk_mutex_get function first before calling this API.
 * @par Error case and Error handling
 *     1. If mutex_res is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_disable_ex(const struct mtk_mutex_res *mutex_res,
		      struct cmdq_pkt **handle, bool reset)
{
	struct mtk_mutex *mutex;
	int ret = 0;
	u32 i, reg, cnt;

	if (mtk_mutex_res_check(mutex_res) < 0)
		return -EINVAL;

	mutex = container_of(mutex_res, struct mtk_mutex,
			     mutex_res[mutex_res->id]);

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			writel(MUTEX_GET,
				mutex->regs[i] + MUTEX_GETN(mutex_res->id));
			reg = readl(mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
			reg &= ~MUTEX_EN;
			writel(reg, mutex->regs[i] +
				    MUTEX_ENABLE(mutex_res->id));
			writel(0, mutex->regs[i] + MUTEX_GETN(mutex_res->id));
			if (reset) {
				void *addr = mutex->regs[i] +
					MUTEX_RESET(mutex_res->id);

				writel(MUTEX_RST, addr);
				writel(0, addr);
			}
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   MUTEX_GET, MUTEX_GET);
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_ENABLE(mutex_res->id),
						   0, MUTEX_EN);
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_GETN(mutex_res->id),
						   0, MUTEX_GET);
			if (reset) {
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_RESET(mutex_res->id),
						   MUTEX_RST, MUTEX_RST);
				ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   MUTEX_RESET(mutex_res->id),
						   0, MUTEX_RST);
			}
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_mutex_disable_ex);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Disable and force reset Mutex Engine.\n
 *     User does not need to call this API if it is SOF_SINGLE mode.
 * @param[in]
 *     mutex_res: mutex resource.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     error code from mtk_mutex_disable_ex().
 * @par Boundary case and Limitation
 *     Same as mtk_mutex_disable_ex().
 * @par Error case and Error handling
 *     If mtk_mutex_disable_ex() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_disable(const struct mtk_mutex_res *mutex_res,
		      struct cmdq_pkt **handle)
{
	return mtk_mutex_disable_ex(mutex_res, handle, true);
}
EXPORT_SYMBOL(mtk_mutex_disable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Enable Mutex debug function.\n
 *     The Mutex HW will count the number of SOF signals it sends to components
 *     which are added to mutex after enabled.
 * @param[in]
 *     dev: mutex device node.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     dev can not be NULL.\n
 * @par Error case and Error handling
 *     1. If dev is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_debug_enable(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, cnt;
	int ret = 0;

	if (!dev)
		return -EINVAL;
	mutex = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(mutex))
		return -EINVAL;

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			void *addr;
			u32 reg;

			addr = mutex->regs[i] + DISP_MUTEX_DEBUG_OUT_SEL;
			reg = readl(addr) | DEBUG_EN;
			writel(reg, addr);
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   DISP_MUTEX_DEBUG_OUT_SEL,
						   DEBUG_EN, DEBUG_EN);
		}
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mutex_debug_enable);

/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Disable Mutex debug function.\n
 * @param[in]
 *     dev: mutex device node.
 * @param[out]
 *     handle: CMDQ handle, set NULL for CPU write directly.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.\n
 *     error code from cmdq_pkt_write_value().
 * @par Boundary case and Limitation
 *     dev can not be NULL.\n
 * @par Error case and Error handling
 *     1. If dev is invalid, return -EINVAL.\n
 *     2. If cmdq_pkt_write_value() fails, return its error code.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_debug_disable(const struct device *dev, struct cmdq_pkt **handle)
{
	struct mtk_mutex *mutex;
	u32 i, cnt;
	int ret = 0;

	if (!dev)
		return -EINVAL;
	mutex = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(mutex))
		return -EINVAL;

	cnt = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < cnt; i++) {
		if (!handle) {
			void *addr;
			u32 reg;

			addr = mutex->regs[i] + DISP_MUTEX_DEBUG_OUT_SEL;
			reg = readl(addr) & ~DEBUG_EN;
			writel(reg, addr);
		} else {
			ret |= cmdq_pkt_write_value(handle[i],
						   mutex->gce_subsys[i],
						   mutex->gce_subsys_offset[i] +
						   DISP_MUTEX_DEBUG_OUT_SEL,
						   0, DEBUG_EN);
		}
	}

	return ret;
}
EXPORT_SYMBOL(mtk_mutex_debug_disable);


/** @ingroup IP_group_mutex_external_function
 * @par Description
 *     Get SOF signals counter that Mutex sent to a component.\n
 * @param[in]
 *     dev: mutex device node.
 * @param[in]
 *     compid: component id.
 * @return
 *     0 or positive value, get counter success and the value is the counter.\n
 *     -EINVAL, wrong input parameters.\n
 * @par Boundary case and Limitation
 *     1. dev can not be NULL.\n
 *     2. Call mtk_mutex_debug_enable function first before calling this API.
 * @par Error case and Error handling
 *     1. If dev is invalid, return -EINVAL.\n
 *     2. If compid is invalid, return -EINVAL.\n
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_debug_get_sent_sof_count(const struct device *dev,
				       enum mtk_mutex_comp_id compid)
{
	struct mtk_mutex *mutex;
	int count = 0;
	void *addr;
	u32 i, reg, offset, field, loop;

	if (!dev)
		return -EINVAL;
	if (is_invalid_comp(compid))
		return -EINVAL;
	mutex = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(mutex))
		return -EINVAL;

	compid &= 0x3f;
	field = compid % 8;
	offset = DISP_MUTEX_SOF_COUNT0 + (compid / 8) * 4;

	loop = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < loop; i++) {
		addr = mutex->regs[i] + offset;
		reg = readl(addr);
		reg >>= field * 4;
		reg &= MODULE0_SOF_COUNT;
		count += reg;
	}

	return count;
}
EXPORT_SYMBOL(mtk_mutex_debug_get_sent_sof_count);

/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Register callback function for Mutex interrupt.
 * @param[in]
 *     dev: mutex device node.
 * @param[in]
 *     idx: index of mutex interrupt status.
 * @param[in]
 *     cb: Mutex irq callback function pointer assigned by user.
 * @param[in]
 *     status: Specify which irq status should be notified via callback.
 * @param[in]
 *     priv_data: Mutex irq callback function's priv_data.
 * @return
 *     0, configuration success.\n
 *     -EINVAL, wrong input parameters.
 * @par Boundary case and Limitation
 *     dev can not be NULL.
 * @par Error case and Error handling
 *     1. If dev is NULL, return -EINVAL.
 *     2. If idx >= MUTEX_IRQ_STA_NR, return -EINVAL.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int mtk_mutex_register_cb(struct device *dev, enum mtk_mutex_irq_sta_index idx,
			  mtk_mmsys_cb cb, u32 status, void *priv_data)
{
	struct mtk_mutex *mutex;
	unsigned long flags;
	const u16 offset[MUTEX_IRQ_STA_NR] = {
		DISP_MUTEX_INTEN,
		DISP_MUTEX_INTEN_1,
		DISP_MUTEX_INTEN_2,
		DISP_MUTEX_INTEN_3,
		DISP_MUTEX_INTEN_4,
		DISP_MUTEX_INTEN_5,
		DISP_MUTEX_INTEN_6,
		DISP_MUTEX_INTEN_7,
		DISP_MUTEX_INTEN_8,
		DISP_MUTEX_INTEN_9,
		DISP_MUTEX_INTEN_A
	};

	if (!dev)
		return -EINVAL;
	if (idx < 0 || idx >= MUTEX_IRQ_STA_NR)
		return -EINVAL;
	if (!cb)
		status = 0;

	mutex = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(mutex))
		return -EINVAL;
	spin_lock_irqsave(&mutex->lock, flags);
	mutex->cb_func[idx] = cb;
	mutex->cb_data[idx].priv_data = priv_data;
	mutex->cb_status[idx] = status;
	writel(status, mutex->regs[0] + offset[idx]);
	spin_unlock_irqrestore(&mutex->lock, flags);

	return 0;
}
EXPORT_SYMBOL(mtk_mutex_register_cb);


/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Mutex interrupt handler.
 * @param[in]
 *     irq: irq number.
 * @param[in]
 *     dev_id: pointer used to save device node pointer of Mutex.
 * @return
 *     IRQ_HANDLED, irq handler execute done\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static irqreturn_t mtk_mutex_irq_handler(int irq, void *dev_id)
{
	struct mtk_mutex *mutex = dev_id;
	enum mtk_mutex_irq_sta_index i;
	u32 int_sta;
	struct mtk_mmsys_cb_data cb_data;
	unsigned long flags;
	mtk_mmsys_cb cb;
	const u16 offset[MUTEX_IRQ_STA_NR] = {
		DISP_MUTEX_INTSTA,
		DISP_MUTEX_INTSTA_1,
		DISP_MUTEX_INTSTA_2,
		DISP_MUTEX_INTSTA_3,
		DISP_MUTEX_INTSTA_4,
		DISP_MUTEX_INTSTA_5,
		DISP_MUTEX_INTSTA_6,
		DISP_MUTEX_INTSTA_7,
		DISP_MUTEX_INTSTA_8,
		DISP_MUTEX_INTSTA_9,
		DISP_MUTEX_INTSTA_A
	};

	if (irq != mutex->irq)
		return IRQ_NONE;

	for (i = MUTEX_IRQ_STA_0; i < MUTEX_IRQ_STA_NR; i++) {
		spin_lock_irqsave(&mutex->lock, flags);
		cb_data = mutex->cb_data[i];
		cb = mutex->cb_func[i];
		cb_data.status = mutex->cb_status[i];
		spin_unlock_irqrestore(&mutex->lock, flags);
		if (!cb || cb_data.status == 0)
			continue;

		int_sta = readl(mutex->regs[0] + offset[i]);
		if (int_sta != 0)
			writel(~int_sta, mutex->regs[0] + offset[i]);

		if (int_sta & cb_data.status) {
			cb_data.part_id = 0;
			cb_data.status = int_sta;
			cb(&cb_data);
		}
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_MTK_DEBUGFS
static const char * const mutex_core_comp_name[] = {
	"mmsys",
	"slicer",
	"p2d0",
	"mdp_rdma0",
	"mdp_rdma1",
	"mdp_rdma2",
	"mdp_rdma3",
	"mdp_rdma_pvric0",
	"mdp_rdma_pvric1",
	"mdp_rdma_pvric2",
	"mdp_rdma_pvric3",
	"disp_rdma0",
	"disp_rdma1",
	"disp_rdma2",
	"disp_rdma3",
	"lhc0",
	"lhc1",
	"lhc2",
	"lhc3",
	"crop0",
	"crop1",
	"crop2",
	"crop3",
	"disp_wdma0",
	"disp_wdma1",
	"disp_wdma2",
	"disp_wdma3",
	"rsz0",
	"rsz1",
	"dsc0",
	"dsc1",
	"dsc2",
	"dsc3",
	"dsi0",
	"dsi1",
	"dsi2",
	"dsi3",
	"rbfc0",
	"rbfc1",
	"rbfc2",
	"rbfc3",
	"sbrc",
	"pat_gen_0",
	"pat_gen_1",
	"pat_gen_2",
	"pat_gen_3",
	"disp_pwm0",
	"reserved",
	"emi"
};

static const char * const mutex_gaze_comp_name[] = {
	"wpea0",
	"wdma_gaze",
	"rbfc_wpea0"
};

static const char * const mutex_common_comp_name[] = {
	"padding_0",
	"padding_1",
	"padding_2",
	"wpea1",
	"fe",
	"wdma1",
	"wdma2",
	"wdma3",
	"fm",
	"rbfc_wpea1",
	"mmcommon_rsz0",
	"mmcommon_rsz1"
};

static const char * const mutex_sof_eof_name[] = {
	"single",
	"dsi0_mutex",
	"dsi1_mutex",
	"dsi2_mutex",
	"dsi3_mutex",
	"dp",
	"dp_dsc",
	"rbfc_side0_0_wpea",
	"rbfc_side0_1_wpea",
	"rbfc_side1_0_wpea",
	"rbfc_side1_1_wpea",
	"rbfc_gaze0_wpea",
	"rbfc_gaze1_wpea",
	"rbfc_side0_cam_0",
	"rbfc_side0_cam_1",
	"rbfc_side1_cam_0",
	"rbfc_side1_cam_1",
	"rbfc_gaze0_cam",
	"rbfc_gaze1_cam",
	"strip_buffer_control",
	"img_side0_0",
	"img_side0_1",
	"img_side1_0",
	"img_side1_1",
	"img_gaze0",
	"img_gaze1",
	"dsi0",
	"dsi1",
	"dsi2",
	"dsi3",
	"reserved0",
	"reserved1",
	"reserved2",
	"sync_delay0",
	"sync_delay1",
	"sync_delay2",
	"sync_delay3",
	"sync_delay4",
	"sync_delay5",
	"sync_delay6",
	"sync_delay7"
};

static const char * const mutex_sof_timing[] = {
	"vsync_falling",
	"vsync_rising",
	"vde_rising",
	"vde_falling"
};

static const char * const mutex_eof_timing[] = {
	"vde_falling",
	"vsync_falling",
	"vsync_rising",
	"vde_rising"
};

static void mtk_mutex_debug_mutex_id_info(struct seq_file *s, struct mtk_mutex *mutex, u32 id)
{
	unsigned long val;
	void *addr;
	const char * const *name;
	const char *src, *timing;
	u32 bit, size, name_cnt, sof_id, eof_id, sof_timing, eof_timing;

	addr = mutex->regs[0];

	/* dump components information */
	if (mutex->reg_base[0] == 0x14001000) {
		name = mutex_core_comp_name;
		name_cnt = ARRAY_SIZE(mutex_core_comp_name);
	} else if (mutex->reg_base[0] == 0x14406000) {
		name = mutex_gaze_comp_name;
		name_cnt = ARRAY_SIZE(mutex_gaze_comp_name);
	} else {
		name = mutex_common_comp_name;
		name_cnt = ARRAY_SIZE(mutex_common_comp_name);
	}

	val = readl(addr + MUTEX_MODN0(id));
	seq_printf(s, "components-0: %08x\n", (u32)val);
	size = min(name_cnt, 32u);
	for_each_set_bit(bit, &val, size)
		seq_printf(s, "  %2u: %s\n", bit, name[bit]);

	if (name_cnt > 32u) {
		val = readl(addr + MUTEX_MODN1(id));
		seq_printf(s, "components-1: %08x\n", (u32)val);
		size = name_cnt - 32u;
		for_each_set_bit(bit, &val, size)
			seq_printf(s, "  %2u: %s\n", bit, name[32u + bit]);
	}

	/* dump timing information */
	val = readl(addr + MUTEX_CTRL(id));
	sof_id = val & MUTEX_SOF;
	sof_timing = (val & MUTEX_SOF_TIMING) >> 6;
	eof_id = (val & MUTEX_EOF) >> 8;
	eof_timing = (val & MUTEX_EOF_TIMING) >> 14;

	seq_printf(s, "ctrl: %08x\n", (u32)val);
	if (sof_id < ARRAY_SIZE(mutex_sof_eof_name))
		src = mutex_sof_eof_name[sof_id];
	else
		src = "reserved";
	if (sof_timing < ARRAY_SIZE(mutex_sof_timing))
		timing = mutex_sof_timing[sof_timing];
	else
		timing = "reserved";
	seq_printf(s, "  sof source: %s, %s\n", src, timing);

	if (eof_id < ARRAY_SIZE(mutex_sof_eof_name))
		src = mutex_sof_eof_name[eof_id];
	else
		src = "reserved";
	if (eof_timing < ARRAY_SIZE(mutex_eof_timing))
		timing = mutex_eof_timing[eof_timing];
	else
		timing = "reserved";
	seq_printf(s, "  eof source: %s, %s\n", src, timing);
}

static void mtk_mutex_debug_mutex_info_dump(struct seq_file *s, struct mtk_mutex *mutex)
{
	u32 i;

	for (i = 0; i < MUTEX_MAX_RES_NUM; i++) {
		if (mutex->mutex_res[i].claimed) {
			seq_printf(s, "------------- %s mutex[%u] info -----------\n",
				mutex->file_name, mutex->mutex_res[i].id);
			mtk_mutex_debug_mutex_id_info(s, mutex, i);
		}
	}
}

static int mtk_mutex_debug_show(struct seq_file *s, void *unused)
{
	struct mtk_mutex *mutex = s->private;

	mtk_mutex_debug_mutex_info_dump(s, mutex);

	return 0;
}

static void mtk_mutex_debug_reg_read(struct mtk_mutex *mutex, u32 offset,
				     u32 size)
{
	void *r;
	u32 i, j, t1, t2, loop;

	if (offset >= 0x1000 || (offset + size) > 0x1000) {
		dev_err(mutex->dev, "Specified register range overflow!\n");
		return;
	}

	t1 = rounddown(offset, 16);
	t2 = roundup(offset + size, 16);

	pr_err("---------------- %s reg_r 0x%x 0x%x ----------------\n",
		mutex->file_name, offset, size);

	loop = min_t(u32, mutex->hw_nr, (u32)ARRAY_SIZE(mutex->regs));
	for (i = 0; i < loop; i++) {
		if (mutex->hw_nr > 1)
			pr_err("-------- %s[%d]\n", mutex->file_name, i);
		for (j = t1; j < t2; j += 16) {
			r = mutex->regs[i] + j;
			pr_err("%X | %08X %08X %08X %08X\n",
				mutex->reg_base[i] + j,
				readl(r), readl(r + 4),
				readl(r + 8), readl(r + 12));
		}
	}
	pr_err("---------------------------------------------------\n");
}

static void mtk_mutex_debug_dump_sof_cnt(struct mtk_mutex *mutex, int *comp,
					 u32 comp_cnt)
{
	u32 i, name_cnt;
	int ret;
	const char * const *name;

	/* dump components information */
	if (mutex->reg_base[0] == 0x14001000) {
		name = mutex_core_comp_name;
		name_cnt = ARRAY_SIZE(mutex_core_comp_name);
	} else if (mutex->reg_base[0] == 0x14406000) {
		name = mutex_gaze_comp_name;
		name_cnt = ARRAY_SIZE(mutex_gaze_comp_name);
	} else {
		name = mutex_common_comp_name;
		name_cnt = ARRAY_SIZE(mutex_common_comp_name);
	}

	pr_err("---------------- %s dump sof count ----------------\n",
		mutex->file_name);

	if (comp_cnt == 0) {
		for (i = 0; i < name_cnt; i++) {
			ret = mtk_mutex_debug_get_sent_sof_count(mutex->dev, i);
			if (ret < 0)
				continue;
			pr_err("%-20s: %d\n", name[i], ret);
		}
	} else {
		for (i = 0; i < comp_cnt; i++) {
			ret = mtk_mutex_debug_get_sent_sof_count(mutex->dev,
								 comp[i]);
			if (ret < 0) {
				dev_err(mutex->dev, "invalid component %d\n",
					comp[i]);
				continue;
			}
			comp[i] &= 0x3f;
			pr_err("%-20s: %d\n", name[comp[i]], ret);
		}
	}
	pr_err("---------------------------------------------------\n");
}

static ssize_t mtk_mutex_debug_write(struct file *file, const char __user *ubuf,
				   size_t count, loff_t *ppos)
{
	struct mtk_mutex *mutex = ((struct seq_file *)file->private_data)->private;
	char buf[64];
	size_t buf_size = min(count, sizeof(buf) - 1);

	if (copy_from_user(buf, ubuf, buf_size))
		return -EFAULT;
	buf[buf_size] = '\0';

	dev_err(mutex->dev, "debug cmd: %s\n", buf);
	if (strncmp(buf, "reg_r", 5) == 0) {
		int ret;
		u32 offset = 0, size = 16;

		ret = sscanf(buf, "reg_r %x %x", &offset, &size);
		if (ret == 2)
			mtk_mutex_debug_reg_read(mutex, offset, size);
		else
			pr_err("Correct your input! (reg_r <offset> <size>)\n");

	} else if (strncmp(buf, "dbg_en", 6) == 0) {
		mtk_mutex_debug_enable(mutex->dev, NULL);
		dev_err(mutex->dev, "mutex debug enabled.\n");
	} else if (strncmp(buf, "sof_cnt", 7) == 0) {
		int ret;
		int m[16];
		const char *fmt;

		fmt = "sof_cnt %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d";
		ret = sscanf(buf, fmt,
			     &m[0], &m[1], &m[2], &m[3], &m[4], &m[5],
			     &m[6], &m[7], &m[8], &m[9], &m[10], &m[11],
			     &m[12], &m[13], &m[14], &m[15]);
		if (ret < 0)
			ret = 0;
		mtk_mutex_debug_dump_sof_cnt(mutex, m, ret);
	} else if (strncmp(buf, "dbg_dis", 7) == 0) {
		mtk_mutex_debug_disable(mutex->dev, NULL);
		dev_err(mutex->dev, "mutex debug disabled.\n");
	} else if (strncmp(buf, "eof_mask", 8) == 0) {
		int ret;
		u32 comp, receive;

		ret = sscanf(buf, "eof_mask %d %d", &comp, &receive);
		if (ret == 2)
			mtk_mutex_set_comp_eof_mask(mutex->dev,
				(enum mtk_mutex_comp_id)comp,
				(bool)receive, NULL);
		else
			pr_err("Usage (eof_mask <comp> <receive>)\n");
	} else {
		pr_err("[USAGE]\n");
		pr_err("   echo [ACTION]... > /sys/kernel/debug/mediatek/mutex/%s\n",
				mutex->file_name);
		pr_err("[ACTION]\n");
		pr_err("   reg_r <offset> <size>\n");
		pr_err("   dbg_en\n");
		pr_err("   sof_cnt <components list, all if not set>\n");
		pr_err("   dbg_dis\n");
		pr_err("   eof_mask <comp_id> <receive>\n");
	}

	return count;
}

static int mtk_mutex_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_mutex_debug_show, inode->i_private);
}

static const struct file_operations mtk_mutex_debug_fops = {
	.owner = THIS_MODULE,
	.open		= mtk_mutex_debug_open,
	.read		= seq_read,
	.write		= mtk_mutex_debug_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void mtk_mutex_debugfs_init(struct mtk_mutex *mutex, const char *name)
{
	static struct dentry *debugfs_dir;
	struct dentry *file;

	if (!mtk_debugfs_root) {
		dev_err(mutex->dev, "No mtk debugfs root!\n");
		return;
	}

	mutex->debugfs = &debugfs_dir;
	if (debugfs_dir == NULL) {
		debugfs_dir = debugfs_create_dir("mutex", mtk_debugfs_root);
		if (IS_ERR(debugfs_dir)) {
			dev_err(mutex->dev, "failed to create debug dir (%p)\n",
				debugfs_dir);
			debugfs_dir = NULL;
			return;
		}
	}
	file = debugfs_create_file(name, 0664, debugfs_dir,
				   (void *)mutex, &mtk_mutex_debug_fops);
	if (IS_ERR(file)) {
		dev_err(mutex->dev, "failed to create debug file (%p)\n", file);
		debugfs_remove_recursive(debugfs_dir);
		debugfs_dir = NULL;
		return;
	}
	mutex->file_name = name;
}
#endif

/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Get Necessary Hardware Information from Device Tree.\n
 *     There are HW number, delay generator number, and gce related\n
 *     information which will be parsed from device tree.
 * @param[out]
 *     pdev: platform device node.
 * @return
 *     If return value is 0, function success.
 *     Otherwise, mutex probe failed.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If there is any error during the probing flow,\n
 *     system error value will be returned.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mutex_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mutex *mutex;
	struct resource *regs;
	int i;
	int ret = 0;

	mutex = devm_kzalloc(dev, sizeof(*mutex), GFP_KERNEL);
	if (!mutex)
		return -ENOMEM;

	mutex->dev = dev;
	of_property_read_u32(dev->of_node, "hw-number", &mutex->hw_nr);
	if (mutex->hw_nr > MODULE_MAX_NUM) {
		dev_err(dev, "Wrong \"hw-number\" value %u, max %u\n",
			mutex->hw_nr, MODULE_MAX_NUM);
		return -EINVAL;
	}

	of_property_read_u32(dev->of_node, "delay-res", &mutex->delay_res_num);
	mutex->delay_sof = MUTEX_SOF_ID_MAX;
	of_property_read_u32(dev->of_node, "delay-sof", &mutex->delay_sof);
	of_property_read_u32(dev->of_node, "mm-clock", &mutex->mm_clock);
	ret = of_property_read_u32(dev->of_node, "mutex-clock",
		&mutex->mutex_clock);
	if (ret < 0) {
		dev_dbg(dev, "mutex-clock will use default.\n");
		mutex->mutex_clock = 26000000;
	}
	dev_dbg(dev, "mutex-clock is %u\n", mutex->mutex_clock);

	for (i = 0; i < MUTEX_MAX_RES_NUM; i++)
		mutex->mutex_res[i].id = i;

	for (i = 0; i < MUTEX_MAX_DELAY_NUM; i++)
		mutex->mutex_delay_res[i].id = i;

#if defined(CONFIG_COMMON_CLK_MT3612)
	ret = of_property_match_string(dev->of_node, "clock-names", "clk_26M");
	if (ret >= 0) {
		mutex->clk_26M = devm_clk_get(dev, "clk_26M");
		if (IS_ERR(mutex->clk_26M)) {
			dev_err(dev, "Failed to get mutex 26M clock\n");
			return PTR_ERR(mutex->clk_26M);
		}
	}
	ret = of_property_match_string(dev->of_node, "clock-names", "clk");
	if (ret >= 0) {
		mutex->clk = devm_clk_get(dev, "clk");
		if (IS_ERR(mutex->clk)) {
			dev_err(dev, "Failed to get mutex clock\n");
			return PTR_ERR(mutex->clk);
		}
	}
#endif
	for (i = 0; i < mutex->hw_nr; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		mutex->regs[i] = devm_ioremap_resource(dev, regs);
		if (IS_ERR(mutex->regs[i])) {
			dev_err(dev, "Failed to map mutex%d registers\n", i);
			return PTR_ERR(mutex->regs[i]);
		}
		mutex->gce_subsys_offset[i] = regs->start & 0xffff;
		mutex->reg_base[i] = regs->start;
	}

	ret = (int)(unsigned long)of_device_get_match_data(dev);
	if (ret == 1) {
		ret = platform_get_irq(pdev, 0);
		if (ret < 0) {
			dev_err(dev, "Failed to get irq %x\n", ret);
			return ret;
		}
		mutex->irq = ret;
		ret = devm_request_irq(dev, mutex->irq, mtk_mutex_irq_handler,
				       IRQF_TRIGGER_LOW, dev_name(dev), mutex);
		if (ret < 0) {
			dev_err(dev, "Failed to request irq %d:%d\n",
				mutex->irq, ret);
			return ret;
		}
	}
#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_enable(dev);
#endif

	of_property_read_u32_array(dev->of_node, "gce-subsys",
				   mutex->gce_subsys, mutex->hw_nr);
	spin_lock_init(&mutex->lock);

	platform_set_drvdata(pdev, mutex);
#ifdef CONFIG_MTK_DEBUGFS
	mtk_mutex_debugfs_init(mutex, pdev->name);
#endif

	return ret;
}

/** @ingroup IP_group_mutex_internal_function
 * @par Description
 *     Do Nothing.
 * @param[in]
 *     pdev: platform device node.
 * @return
 *     return value is 0.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int mtk_mutex_remove(struct platform_device *pdev)
{
	struct mtk_mutex *mutex;

	mutex = platform_get_drvdata(pdev);
#ifdef CONFIG_MTK_DEBUGFS
	if (mutex->debugfs) {
		debugfs_remove_recursive(*mutex->debugfs);
		*mutex->debugfs = NULL;
	}
#endif
#if defined(CONFIG_COMMON_CLK_MT3612)
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

/** @ingroup IP_group_mutex_internal_struct
 * @brief Mutex Open Framework Device ID.\n
 * This structure is used to attach specific names to\n
 * platform device for use with device tree.
 */
static const struct of_device_id mutex_driver_dt_match[] = {
	{.compatible = "mediatek,mt3612-mutex", .data = (void *)1},
	{},
};
MODULE_DEVICE_TABLE(of, mutex_driver_dt_match);

/** @ingroup IP_group_mutex_internal_struct
 * @brief Mutex platform driver structure.\n
 * This structure is used to register itself.
 */
struct platform_driver mtk_mutex_driver = {
	.probe = mtk_mutex_probe,
	.remove = mtk_mutex_remove,
	.driver = {
		   .name = "mediatek-mutex",
		   .owner = THIS_MODULE,
		   .of_match_table = mutex_driver_dt_match,
	},
};

module_platform_driver(mtk_mutex_driver);
MODULE_LICENSE("GPL");
