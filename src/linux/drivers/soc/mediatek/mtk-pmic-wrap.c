/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Flora Fu, MediaTek
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
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>

/**
 * @defgroup IP_group_linux_pwrap PMIC WRAPPER
 *    The PMIC SPI Wrapper is the bridge for communication between AP and
 *    PMIC in MT3612.\n
 *    This is Mediatek inside protocol SPI transmit for linux driver.\n
 *
 *    @{
 *	@defgroup IP_group_linux_pwrap_external EXTERNAL
 *	  The external API document for LINUX PMIC WRAP. \n
 *	  @{
 *	    @defgroup type_group_linux_pwrap_API 1.function
 *	      None.
 *	    @defgroup type_group_linux_pwrap_ext_struct 2.structure
 *	      None.
 *	    @defgroup type_group_linux_pwrap_ext_typedef 3.typedef
 *	      None.
 *	    @defgroup type_group_linux_pwrap_ext_enum 4.enumeration
 *	      None.
 *	    @defgroup type_group_linux_pwrap_ext_def 5.define
 *	      None.
 *	  @}
 *
 *	@defgroup IP_group_linux_pwrap_internal INTERNAL
 *	  The internal API document for LINUX PMIC WRAP. \n
 *	  @{
 *	    @defgroup type_group_linux_pwrap_InFn 1.function
 *	      This is LINUX_PMIC WRAP internal function.
 *	    @defgroup type_group_linux_pwrap_struct 2.structure
 *	      This is LINUX_PMIC WRAP structure.
 *	    @defgroup type_group_linux_pwrap_typedef 3.typedef
 *	      None.
 *	    @defgroup type_group_linux_pwrap_enum 4.enumeration
 *	      This is LINUX_PMIC WRAP enumeration.
 *	    @defgroup type_group_linux_pwrap_def 5.define
 *	      This is LINUX_PMIC WRAP define.
 *	     @{
 *             @defgroup type_group_linux_pwrap_def_01 5.1.Register/Macro Define
 *               PMIC wrap SW register and macro define.
 *             @defgroup type_group_linux_pwrap_def_02 5.2.PMIC Register Define
 *               PMIC MT6355 register define.
 *	     @}
 *	  @}
 *    @}
 */

#define PWRAP_MT8135_BRIDGE_IORD_ARB_EN		0x4
#define PWRAP_MT8135_BRIDGE_WACS3_EN		0x10
#define PWRAP_MT8135_BRIDGE_INIT_DONE3		0x14
#define PWRAP_MT8135_BRIDGE_WACS4_EN		0x24
#define PWRAP_MT8135_BRIDGE_INIT_DONE4		0x28
#define PWRAP_MT8135_BRIDGE_INT_EN		0x38
#define PWRAP_MT8135_BRIDGE_TIMER_EN		0x48
#define PWRAP_MT8135_BRIDGE_WDT_UNIT		0x50
#define PWRAP_MT8135_BRIDGE_WDT_SRC_EN		0x54

/** @ingroup type_group_linux_pwrap_def_01
 * @{
 */
/* macro for wrapper status */
#define PWRAP_GET_WACS_RDATA(x)		(((x) >> 0) & 0x0000ffff)
#define PWRAP_GET_WACS_FSM(x)		(((x) >> 16) & 0x00000007)
#define PWRAP_GET_WACS_REQ(x)		(((x) >> 19) & 0x00000001)
#define PWRAP_STATE_SYNC_IDLE0		(1 << 20)
#define PWRAP_STATE_INIT_DONE0		(1 << 21)

/* macro for WACS FSM */
#define PWRAP_WACS_FSM_IDLE		0x00
#define PWRAP_WACS_FSM_REQ		0x02
#define PWRAP_WACS_FSM_WFDLE		0x04
#define PWRAP_WACS_FSM_WFVLDCLR		0x06
#define PWRAP_WACS_INIT_DONE		0x01
#define PWRAP_WACS_WACS_SYNC_IDLE	0x01
#define PWRAP_WACS_SYNC_BUSY		0x00
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_def_02
 * @{
 */
/* macro for device wrapper default value */
#define PWRAP_DEW_READ_TEST_VAL		0x5aa5
#define PWRAP_DEW_WRITE_TEST_VAL	0xa55a
#define SIGNATURE_DEFAULT_VAL		0x83
#define MT3612_ARBITER_SRC_EN		(BIT(14) | BIT(13) | BIT(11) | BIT(5))
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_def_01
 * @{
 */
/* macro for manual command */
#define PWRAP_MAN_CMD_SPI_WRITE_NEW	(1 << 14)
#define PWRAP_MAN_CMD_SPI_WRITE		(1 << 13)
#define PWRAP_MAN_CMD_OP_CSH		(0x0 << 8)
#define PWRAP_MAN_CMD_OP_CSL		(0x1 << 8)
#define PWRAP_MAN_CMD_OP_CK		(0x2 << 8)
#define PWRAP_MAN_CMD_OP_OUTS		(0x8 << 8)
#define PWRAP_MAN_CMD_OP_OUTD		(0x9 << 8)
#define PWRAP_MAN_CMD_OP_OUTQ		(0xa << 8)

/* macro for Watch Dog Timer Source */
#define PWRAP_WDT_SRC_EN_STAUPD_TRIG		(1 << 25)
#define PWRAP_WDT_SRC_EN_HARB_STAUPD_DLE	(1 << 20)
#define PWRAP_WDT_SRC_EN_HARB_STAUPD_ALE	(1 << 6)
#define PWRAP_WDT_SRC_MASK_ALL			0xffffffff
#define PWRAP_WDT_SRC_MASK_NO_STAUPD	~(PWRAP_WDT_SRC_EN_STAUPD_TRIG | \
					  PWRAP_WDT_SRC_EN_HARB_STAUPD_DLE | \
					  PWRAP_WDT_SRC_EN_HARB_STAUPD_ALE)
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_enum
 * @brief defines for slave device wrapper registers\n
 */
enum dew_regs {
	PWRAP_DEW_BASE,
	PWRAP_DEW_DIO_EN,
	PWRAP_DEW_READ_TEST,
	PWRAP_DEW_WRITE_TEST,
	PWRAP_DEW_CRC_EN,
	PWRAP_DEW_CRC_VAL,
	PWRAP_DEW_MON_GRP_SEL,
	PWRAP_DEW_CIPHER_KEY_SEL,
	PWRAP_DEW_CIPHER_IV_SEL,
	PWRAP_DEW_CIPHER_RDY,
	PWRAP_DEW_CIPHER_MODE,
	PWRAP_DEW_CIPHER_SWRST,
	PWRAP_DEW_RDDMY_NO,
/** MT6355,MT3615 regs */
	PWRAP_DEW_INT_STA,
/** MT6355,MT3615 regs */
	PWRAP_DEW_PWR_HOLD,
/** MT3615 only regs */
	PWRAP_DEW_HWCID,
};

static const u32 mt3615_regs[] = {
	[PWRAP_DEW_BASE] = 0x0000,
	[PWRAP_DEW_HWCID] = 0x0008,
	[PWRAP_DEW_DIO_EN] = 0x040c,
	[PWRAP_DEW_READ_TEST] = 0x040e,
	[PWRAP_DEW_WRITE_TEST] = 0x0410,
	[PWRAP_DEW_CRC_VAL] = 0x0416,
	[PWRAP_DEW_RDDMY_NO] = 0x0424,
	[PWRAP_DEW_INT_STA] = 0x0452,
	[PWRAP_DEW_PWR_HOLD] = 0x0808,
};

/** @ingroup type_group_linux_pwrap_enum
 * @brief defines for pmic wrapper registers\n
 */
enum pwrap_regs {
	PWRAP_MUX_SEL,
	PWRAP_WRAP_EN,
	PWRAP_DIO_EN,
	PWRAP_SIDLY,
	PWRAP_CSHEXT_WRITE,
	PWRAP_CSHEXT_READ,
	PWRAP_CSLEXT_START,
	PWRAP_CSLEXT_END,
	PWRAP_STAUPD_PRD,
	PWRAP_STAUPD_GRPEN,
	PWRAP_STAUPD_MAN_TRIG,
	PWRAP_STAUPD_STA,
	PWRAP_WRAP_STA,
	PWRAP_HARB_INIT,
	PWRAP_HARB_HPRIO,
	PWRAP_HIPRIO_ARB_EN,
	PWRAP_HARB_STA0,
	PWRAP_HARB_STA1,
	PWRAP_MAN_EN,
	PWRAP_MAN_CMD,
	PWRAP_MAN_RDATA,
	PWRAP_MAN_VLDCLR,
	PWRAP_WACS0_EN,
	PWRAP_INIT_DONE0,
	PWRAP_WACS0_CMD,
	PWRAP_WACS0_RDATA,
	PWRAP_WACS0_VLDCLR,
	PWRAP_WACS1_EN,
	PWRAP_INIT_DONE1,
	PWRAP_WACS1_CMD,
	PWRAP_WACS1_RDATA,
	PWRAP_WACS1_VLDCLR,
	PWRAP_WACS2_EN,
	PWRAP_INIT_DONE2,
	PWRAP_WACS2_CMD,
	PWRAP_WACS2_RDATA,
	PWRAP_WACS2_VLDCLR,
	PWRAP_INT_EN,
	PWRAP_INT_FLG_RAW,
	PWRAP_INT_FLG,
	PWRAP_INT_CLR,
	PWRAP_SIG_ADR,
	PWRAP_SIG_MODE,
	PWRAP_SIG_VALUE,
	PWRAP_SIG_ERRVAL,
	PWRAP_CRC_EN,
	PWRAP_TIMER_EN,
	PWRAP_TIMER_STA,
	PWRAP_WDT_UNIT,
	PWRAP_WDT_SRC_EN,
	PWRAP_WDT_FLG,
	PWRAP_DEBUG_INT_SEL,
	PWRAP_CIPHER_KEY_SEL,
	PWRAP_CIPHER_IV_SEL,
	PWRAP_CIPHER_RDY,
	PWRAP_CIPHER_MODE,
	PWRAP_CIPHER_SWRST,
	PWRAP_DCM_EN,
	PWRAP_DCM_DBC_PRD,
	PWRAP_RDDMY,
/** MT3612 only regs */
	PWRAP_CSLEXT_WRITE,
/** MT3612 only regs */
	PWRAP_CSLEXT_READ,
/** MT3612 only regs */
	PWRAP_EINT_STA0_ADR,
/** MT3612 only regs */
	PWRAP_INT0_EN,
/** MT3612 only regs */
	PWRAP_INT0_FLG_RAW,
/** MT3612 only regs */
	PWRAP_INT0_FLG,
/** MT3612 only regs */
	PWRAP_INT0_CLR,
/** MT3612 only regs */
	PWRAP_INT1_EN,
/** MT3612 only regs */
	PWRAP_INT1_FLG_RAW,
/** MT3612 only regs */
	PWRAP_INT1_FLG,
/** MT3612 only regs */
	PWRAP_INT1_CLR,
};
/**
 * @}
 */

static int mt3612_regs[] = {
	[PWRAP_MUX_SEL] = 0x0,
	[PWRAP_WRAP_EN] = 0x4,
	[PWRAP_DIO_EN] = 0x8,
	[PWRAP_SIDLY] = 0xc,
	[PWRAP_RDDMY] = 0x10,
	[PWRAP_CSHEXT_WRITE] = 0x14,
	[PWRAP_CSHEXT_READ] = 0x18,
	[PWRAP_CSLEXT_WRITE] = 0x1c,
	[PWRAP_CSLEXT_READ] = 0x20,
	[PWRAP_STAUPD_PRD] = 0x24,
	[PWRAP_STAUPD_GRPEN] = 0x28,
	[PWRAP_EINT_STA0_ADR] = 0x2c,
	[PWRAP_HARB_INIT] = 0x4c,
	[PWRAP_HARB_HPRIO] = 0x50,
	[PWRAP_HIPRIO_ARB_EN] = 0x54,
	[PWRAP_MAN_EN] = 0x60,
	[PWRAP_MAN_CMD] = 0x64,
	[PWRAP_MAN_RDATA] = 0x68,
	[PWRAP_MAN_VLDCLR] = 0x6c,
	[PWRAP_WACS2_EN] = 0x98,	/* Original: PWRAP_WACS0_EN */
	[PWRAP_INIT_DONE2] = 0x9c,	/* Original: PWRAP_INIT_DONE0 */
	[PWRAP_WACS2_CMD] = 0xa0,	/* Original: PWRAP_WACS0_CMD */
	[PWRAP_WACS2_RDATA] = 0xa4,	/* Original: PWRAP_WACS0_RDATA */
	[PWRAP_WACS2_VLDCLR] = 0xa8,	/* Original: PWRAP_WACS0_VLDCLR */
	[PWRAP_INT0_EN] = 0xc0,
	[PWRAP_INT0_FLG_RAW] = 0xc4,
	[PWRAP_INT0_FLG] = 0xc8,
	[PWRAP_INT0_CLR] = 0xcc,
	[PWRAP_INT1_EN] = 0xd0,
	[PWRAP_INT1_FLG_RAW] = 0xd4,
	[PWRAP_INT1_FLG] = 0xd8,
	[PWRAP_INT1_CLR] = 0xdc,
	[PWRAP_SIG_ADR] = 0xe0,
	[PWRAP_SIG_MODE] = 0xe4,
	[PWRAP_SIG_VALUE] = 0xe8,
	[PWRAP_SIG_ERRVAL] = 0xec,
	[PWRAP_TIMER_EN] = 0xf4,
	[PWRAP_TIMER_STA] = 0xf8,
	[PWRAP_WDT_UNIT] = 0xfC,
	[PWRAP_WDT_SRC_EN] = 0x100,
	[PWRAP_WDT_FLG] = 0x104,
	[PWRAP_DCM_EN] = 0x1cc,
	[PWRAP_DCM_DBC_PRD] = 0x1d4,
};

/** @ingroup type_group_linux_pwrap_enum
 * @brief defines for pmic type\n
 */
enum pmic_type {
	PMIC_MT3615,
};
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_enum
 * @brief defines for pmic wrapper type\n
 */
enum pwrap_type {
	PWRAP_MT3612,
};
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_struct
 * @brief For slave pmic register setting\n
 */
struct pwrap_slv_type {
/** For pmic slave register access */
	const u32 *dew_regs;
/** Enumeration for pmic type */
	enum pmic_type type;
};
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_struct
 * @brief For pmic wrapper clock and reset setting\n
 */
struct pmic_wrapper {
/** Declare for device structure */
	struct device *dev;
/** Declare for pmic wrap base address */
	void __iomem *base;
/** Declare for register mapping structure */
	struct regmap *regmap;
/** Declare for pmic wrapper type structure */
	const struct pmic_wrapper_type *master;
/** Declare for pmic slave type structure */
	const struct pwrap_slv_type *slave;
/** Declare for topcksys pmic spi clock gate */
	struct clk *clk_spi;
/** Declare for topcksys pmic gspi clock gate */
	struct clk *clk_gspi;
/** Declare for infra pmic tmr clock gate */
	struct clk *clk_cg_tmr;
/** Declare for infra pmic ap clock gate */
	struct clk *clk_cg_ap;
/** Declare for infra pmic md clock gate */
	struct clk *clk_cg_md;
/** Declare for infra pmic conn clock gate */
	struct clk *clk_cg_conn;
/** Declare for infra pmic gspi clock gate */
	struct clk *clk_cg_gspi;
/** Declare for pmic wrap reset control */
	struct reset_control *rstc;

/** Declare for pmic wrap reset control bridge */
	struct reset_control *rstc_bridge;
/** Declare for pmic wrap bridge base address */
	void __iomem *bridge_base;
};
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_struct
 * @brief For pmic wrapper configuraiton setting\n
 */
struct pmic_wrapper_type {
/** For pmic wrap register access */
	int *regs;
/** Enumeration for pmic wrap type */
	enum pwrap_type type;
/** For arbitor enable setting */
	u32 arb_en_all;
/** For interrupt enable setting */
	u32 int_en_all;
/** For spi write format setting */
	u32 spi_w;
/** For wdt source setting */
	u32 wdt_src;
/** For pmic wrap bridge selection */
	int has_bridge:1;
/** For init_reg_clock callback function */
	int (*init_reg_clock)(struct pmic_wrapper *wrp);
/** For init_soc_specific callback function */
	int (*init_soc_specific)(struct pmic_wrapper *wrp);
};
/**
 * @}
 */

struct pmic_wrapper *mt3615_pmic_wrapper;

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap read function.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @param[in]
 *     reg: set read register.
 * @return
 *     return read data.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static u32 pwrap_readl(struct pmic_wrapper *wrp, enum pwrap_regs reg)
{
	return readl(wrp->base + wrp->master->regs[reg]);
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap write function.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @param[in]
 *     val: set write data.
 * @param[in]
       reg: set write register.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void pwrap_writel(struct pmic_wrapper *wrp, u32 val, enum pwrap_regs reg)
{
	writel(val, wrp->base + wrp->master->regs[reg]);
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     Wait PMIC wrap finite-state machine in idle state.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     0, indicate in idle state.\n
 *     1, indicate in non-idle state.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool pwrap_is_fsm_idle(struct pmic_wrapper *wrp)
{
	u32 val = pwrap_readl(wrp, PWRAP_WACS2_RDATA);

	return PWRAP_GET_WACS_FSM(val) == PWRAP_WACS_FSM_IDLE;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     Wait PMIC wrap finite-state machine valid flag clearing.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     0, indicate the valid flag clearing.\n
 *     1, indicate the valid flag not clearing.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool pwrap_is_fsm_vldclr(struct pmic_wrapper *wrp)
{
	u32 val = pwrap_readl(wrp, PWRAP_WACS2_RDATA);

	return PWRAP_GET_WACS_FSM(val) == PWRAP_WACS_FSM_WFVLDCLR;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     Clear finite-state machine valid flag.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline void pwrap_leave_fsm_vldclr(struct pmic_wrapper *wrp)
{
	if (pwrap_is_fsm_vldclr(wrp))
		pwrap_writel(wrp, 1, PWRAP_WACS2_VLDCLR);
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     Wait PMIC wrap SPI SYNC module in idle state.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     0, indicate in idle state.\n
 *     1, indicate in non-idle state.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool pwrap_is_sync_idle(struct pmic_wrapper *wrp)
{
	return pwrap_readl(wrp, PWRAP_WACS2_RDATA) & PWRAP_STATE_SYNC_IDLE0;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     Wait PMIC wrap finite-state machine and SPI SYNC module in idle state.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     0, indicate in idle state.\n
 *     1, indicate in non-idle state.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static bool pwrap_is_fsm_idle_and_sync_idle(struct pmic_wrapper *wrp)
{
	u32 val = pwrap_readl(wrp, PWRAP_WACS2_RDATA);

	return (PWRAP_GET_WACS_FSM(val) == PWRAP_WACS_FSM_IDLE) &&
	    (val & PWRAP_STATE_SYNC_IDLE0);
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     Wait PMIC wrap state ready.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @param[in]
 *     *fp: define a pointer function for checking state.
 * @return
 *     0, If return value is 0 for success.\n
 *     -ETIMEDOUT, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_wait_for_state(struct pmic_wrapper *wrp,
				bool (*fp)(struct pmic_wrapper *))
{
	unsigned long timeout;

	timeout = jiffies + usecs_to_jiffies(10000);

	do {
		if (time_after(jiffies, timeout))
			return fp(wrp) ? 0 : -ETIMEDOUT;
		if (fp(wrp))
			return 0;
	} while (1);
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap write command.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @param[in]
 *     adr: set write address.
 * @param[in]
 *     wdata: set write value.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_write(struct pmic_wrapper *wrp, u32 adr, u32 wdata)
{
	int ret;

	ret = pwrap_wait_for_state(wrp, pwrap_is_fsm_idle);
	if (ret) {
		pwrap_leave_fsm_vldclr(wrp);
		return ret;
	}

	pwrap_writel(wrp, (1 << 31) | ((adr >> 1) << 16) | wdata,
		     PWRAP_WACS2_CMD);

	return 0;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap read command.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @param[in]
 *     adr: set read address.
 * @param[out]
 *     *rdata: return read data.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_read(struct pmic_wrapper *wrp, u32 adr, u32 *rdata)
{
	int ret;

	ret = pwrap_wait_for_state(wrp, pwrap_is_fsm_idle);
	if (ret) {
		pwrap_leave_fsm_vldclr(wrp);
		return ret;
	}

	pwrap_writel(wrp, (adr >> 1) << 16, PWRAP_WACS2_CMD);

	ret = pwrap_wait_for_state(wrp, pwrap_is_fsm_vldclr);
	if (ret)
		return ret;

	*rdata = PWRAP_GET_WACS_RDATA(pwrap_readl(wrp, PWRAP_WACS2_RDATA));

	pwrap_writel(wrp, 1, PWRAP_WACS2_VLDCLR);

	return 0;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap read register mapping for main CPU.
 * @param[in]
 *     *context: a pointer for pmic_wrapper structure.
 * @param[in]
 *     adr: set read address.
 * @param[out]
 *     *rdata: return read data.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_regmap_read(void *context, u32 adr, u32 *rdata)
{
	return pwrap_read(context, adr, rdata);
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap write register mapping for main CPU.
 * @param[in]
 *     *context: a pointer for pmic_wrapper structure.
 * @param[in]
 *     adr: set write address.
 * @param[in]
 *     wdata: set write data.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_regmap_write(void *context, u32 adr, u32 wdata)
{
	return pwrap_write(context, adr, wdata);
}

/** @ingroup type_group_linux_pwrap_API
 * @par Description
 *     PMIC wrap read register mapping for main CPU.
 * @param[in]
 *     reg_addr: set read address.
 * @param[in]
 *     val: read register field value.
 * @param[in]
 *     mask: mask register bit field.
 * @param[in]
 *     shift: field start bit.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t pwrap_read_reg(uint32_t reg_addr, uint32_t *val,
				   uint32_t mask, uint32_t shift)
{
	int32_t ret = 0;
	uint32_t reg_val = 0;

	pr_err("[PWRAP]Func:%s reg_num = 0x%x\n", __func__, reg_addr);
	ret = pwrap_read(mt3615_pmic_wrapper, (unsigned int)reg_addr,
		(unsigned int *)&reg_val);

	if (ret != 0) {
		pr_err("[PWRAP]Reg[%x]= pmic_wrap read data fail\n"
			, reg_addr);
		return ret;
	}

	reg_val &= (mask << shift);
	*val = (reg_val >> shift);

	return ret;
}
EXPORT_SYMBOL(pwrap_read_reg);

/** @ingroup type_group_linux_pwrap_API
 * @par Description
 *     PMIC wrap write register mapping for main CPU.
 * @param[in]
 *     reg_addr: set read address.
 * @param[in]
 *     val: written register field value.
 * @param[in]
 *     mask: mask register bit field.
 * @param[in]
 *     shift: field start bit.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int32_t pwrap_write_reg(uint32_t reg_addr, uint32_t val,
			    uint32_t mask, uint32_t shift)
{
	int32_t ret = 0;
	uint32_t reg_val = 0;

	pr_err("[PWRAP]reg_addr = 0x%x , val = %d\n", reg_addr, val);
	ret = pwrap_read(mt3615_pmic_wrapper, (unsigned int)reg_addr,
		(unsigned int *)&reg_val);
	if (ret != 0) {
		pr_err("[PWRAP]Reg[%x]= pmic_wrap read data fail\n"
			, reg_addr);
		return ret;
	}
	reg_val &= ~(mask << shift);
	reg_val |= ((val & mask) << shift);
	ret = pwrap_write(mt3615_pmic_wrapper, (unsigned int)reg_addr,
		(unsigned int)reg_val);
	if (ret != 0) {
		pr_err("[PWRAP]Reg[%x]= pmic_wrap read data fail\n"
			, reg_addr);
		return ret;
	}
	return ret;
}
EXPORT_SYMBOL(pwrap_write_reg);


/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     Reset SPI slave function.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_reset_spislave(struct pmic_wrapper *wrp)
{
	int ret, i;

	pwrap_writel(wrp, 0, PWRAP_HIPRIO_ARB_EN);
	pwrap_writel(wrp, 0, PWRAP_WRAP_EN);
	pwrap_writel(wrp, 1, PWRAP_MUX_SEL);
	pwrap_writel(wrp, 1, PWRAP_MAN_EN);
	pwrap_writel(wrp, 0, PWRAP_DIO_EN);

	pwrap_writel(wrp, wrp->master->spi_w | PWRAP_MAN_CMD_OP_CSL,
		     PWRAP_MAN_CMD);
	pwrap_writel(wrp, wrp->master->spi_w | PWRAP_MAN_CMD_OP_OUTS,
		     PWRAP_MAN_CMD);
	pwrap_writel(wrp, wrp->master->spi_w | PWRAP_MAN_CMD_OP_CSH,
		     PWRAP_MAN_CMD);

	for (i = 0; i < 4; i++)
		pwrap_writel(wrp, wrp->master->spi_w | PWRAP_MAN_CMD_OP_OUTS,
			     PWRAP_MAN_CMD);

	ret = pwrap_wait_for_state(wrp, pwrap_is_sync_idle);
	if (ret) {
		dev_err(wrp->dev, "%s fail, ret=%d\n", __func__, ret);
		return ret;
	}

	pwrap_writel(wrp, 0, PWRAP_MAN_EN);
	pwrap_writel(wrp, 0, PWRAP_MUX_SEL);

	return 0;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     SPI input signal calibration.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     0, If return value is 0 for success.\n
 *     -EIO, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_init_sidly(struct pmic_wrapper *wrp)
{
	u32 rdata;
	u32 i;
	u32 pass = 0;
	signed char dly[16] = {
		-1, 0, 1, 0, 2, -1, 1, 1, 3, -1, -1, -1, 3, -1, 2, 1
	};

	for (i = 0; i < 4; i++) {
		pwrap_writel(wrp, i, PWRAP_SIDLY);
		pwrap_read(wrp, wrp->slave->dew_regs[PWRAP_DEW_READ_TEST],
			   &rdata);
		if (rdata == PWRAP_DEW_READ_TEST_VAL) {
			dev_dbg(wrp->dev, "[Read Test] pass, SIDLY=%x\n", i);
			pass |= 1 << i;
		}
	}

	if (dly[pass] < 0) {
		dev_err(wrp->dev, "sidly pass range 0x%x not continuous\n",
			pass);
		return -EIO;
	}

	pwrap_writel(wrp, dly[pass], PWRAP_SIDLY);

	return 0;
}


/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrapper spi clock setting.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     Always return 0.\n
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_mt3612_init_reg_clock(struct pmic_wrapper *wrp)
{
	pwrap_writel(wrp, 0x8, PWRAP_RDDMY);
	pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_RDDMY_NO], 0x8);

	/* TBD,need to decide in SOC Chip */
	pwrap_writel(wrp, 0xf, PWRAP_CSHEXT_WRITE);
	pwrap_writel(wrp, 0xf, PWRAP_CSHEXT_READ);
	pwrap_writel(wrp, 0xff, PWRAP_CSLEXT_WRITE);
	pwrap_writel(wrp, 0xff, PWRAP_CSLEXT_READ);

	return 0;
}

static bool pwrap_is_cipher_ready(struct pmic_wrapper *wrp)
{
	return pwrap_readl(wrp, PWRAP_CIPHER_RDY) & 1;
}

static bool pwrap_is_pmic_cipher_ready(struct pmic_wrapper *wrp)
{
	u32 rdata;
	int ret;

	ret = pwrap_read(wrp, wrp->slave->dew_regs[PWRAP_DEW_CIPHER_RDY],
			 &rdata);
	if (ret)
		return 0;

	return rdata == 1;
}

static int pwrap_init_cipher(struct pmic_wrapper *wrp)
{
	int ret;
	u32 rdata;

	pwrap_writel(wrp, 0x1, PWRAP_CIPHER_SWRST);
	pwrap_writel(wrp, 0x0, PWRAP_CIPHER_SWRST);
	pwrap_writel(wrp, 0x1, PWRAP_CIPHER_KEY_SEL);
	pwrap_writel(wrp, 0x2, PWRAP_CIPHER_IV_SEL);

	/* Config cipher mode @PMIC */
	pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_CIPHER_SWRST], 0x1);
	pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_CIPHER_SWRST], 0x0);
	pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_CIPHER_KEY_SEL], 0x1);
	pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_CIPHER_IV_SEL], 0x2);

	/* wait for cipher data ready@AP */
	ret = pwrap_wait_for_state(wrp, pwrap_is_cipher_ready);
	if (ret) {
		dev_err(wrp->dev, "cipher data ready@AP fail, ret=%d\n", ret);
		return ret;
	}

	/* wait for cipher data ready@PMIC */
	ret = pwrap_wait_for_state(wrp, pwrap_is_pmic_cipher_ready);
	if (ret) {
		dev_err(wrp->dev,
			"timeout waiting for cipher data ready@PMIC\n");
		return ret;
	}

	/* wait for cipher mode idle */
	pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_CIPHER_MODE], 0x1);
	ret = pwrap_wait_for_state(wrp, pwrap_is_fsm_idle_and_sync_idle);
	if (ret) {
		dev_err(wrp->dev, "cipher mode idle fail, ret=%d\n", ret);
		return ret;
	}

	pwrap_writel(wrp, 1, PWRAP_CIPHER_MODE);

	/* Write Test */
	if (pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_WRITE_TEST],
			PWRAP_DEW_WRITE_TEST_VAL) ||
	    pwrap_read(wrp, wrp->slave->dew_regs[PWRAP_DEW_WRITE_TEST],
		       &rdata) || (rdata != PWRAP_DEW_WRITE_TEST_VAL)) {
		dev_err(wrp->dev, "rdata=0x%04X\n", rdata);
		return -EFAULT;
	}

	return 0;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrapper initialization.
 * @param[in]
 *     *wrp: a pointer for pmic_wrapper structure.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_init(struct pmic_wrapper *wrp)
{
	int ret;
	u32 rdata;

/*  Mark temporary, wait reset module ready
 *	reset_control_reset(wrp->rstc);
 *	if (wrp->rstc_bridge)
 *		reset_control_reset(wrp->rstc_bridge);
 */

	/* Init Read Test */
	pwrap_read(wrp, wrp->slave->dew_regs[PWRAP_DEW_READ_TEST], &rdata);
	if (rdata == PWRAP_DEW_READ_TEST_VAL) {
		ret = 0;
		dev_info(wrp->dev, "[PWRAP]Init Read test Pass!\n");
		return ret;
	}

	dev_info(wrp->dev, "PWRAP Init read test OK!\n");

	/* Reset SPI slave */
	ret = pwrap_reset_spislave(wrp);
	if (ret)
		return ret;

	pwrap_writel(wrp, 1, PWRAP_WRAP_EN);

	pwrap_writel(wrp, wrp->master->arb_en_all, PWRAP_HIPRIO_ARB_EN);

	pwrap_writel(wrp, 1, PWRAP_WACS2_EN);

	ret = wrp->master->init_reg_clock(wrp);
	if (ret)
		return ret;

	/* Setup serial input delay */
	ret = pwrap_init_sidly(wrp);
	if (ret)
		return ret;

	/* Enable dual IO mode */
	pwrap_write(wrp, wrp->slave->dew_regs[PWRAP_DEW_DIO_EN], 1);

	/* Check IDLE & INIT_DONE in advance */
	ret = pwrap_wait_for_state(wrp, pwrap_is_fsm_idle_and_sync_idle);
	if (ret) {
		dev_err(wrp->dev, "%s fail, ret=%d\n", __func__, ret);
		return ret;
	}

	pwrap_writel(wrp, 1, PWRAP_DIO_EN);

	/* Read Test */
	pwrap_read(wrp, wrp->slave->dew_regs[PWRAP_DEW_READ_TEST], &rdata);
	if (rdata != PWRAP_DEW_READ_TEST_VAL) {
		dev_err(wrp->dev,
			"Read test failed after switch to DIO mode: 0x%04x != 0x%04x\n",
			PWRAP_DEW_READ_TEST_VAL, rdata);
		return -EFAULT;
	}

	dev_info(wrp->dev, "PWRAP Init read test OK!\n");

	if (wrp->master->type != PWRAP_MT3612) {
		/* Enable encryption */
		ret = pwrap_init_cipher(wrp);
		if (ret)
			return ret;
	}

	if (wrp->master->type == PWRAP_MT3612) {
		/* Using Signature checking */
		pwrap_writel(wrp, 0x1, PWRAP_SIG_MODE);
		pwrap_writel(wrp, wrp->slave->dew_regs[PWRAP_DEW_CRC_VAL],
			     PWRAP_SIG_ADR);
		pwrap_writel(wrp, SIGNATURE_DEFAULT_VAL, PWRAP_SIG_VALUE);

		pwrap_writel(wrp, wrp->slave->dew_regs[PWRAP_DEW_INT_STA],
			     PWRAP_EINT_STA0_ADR);
	} else {
		/* Signature checking - using CRC */
		if (pwrap_write
		    (wrp, wrp->slave->dew_regs[PWRAP_DEW_CRC_EN], 0x1))
			return -EFAULT;

		pwrap_writel(wrp, 0x1, PWRAP_CRC_EN);
		pwrap_writel(wrp, 0x0, PWRAP_SIG_MODE);
		pwrap_writel(wrp, wrp->slave->dew_regs[PWRAP_DEW_CRC_VAL],
			     PWRAP_SIG_ADR);
	}
	pwrap_writel(wrp, wrp->master->arb_en_all, PWRAP_HIPRIO_ARB_EN);

	if (wrp->master->type != PWRAP_MT3612) {
		pwrap_writel(wrp, 0x1, PWRAP_WACS0_EN);
		pwrap_writel(wrp, 0x1, PWRAP_WACS1_EN);
	}
	pwrap_writel(wrp, 0x1, PWRAP_WACS2_EN);
	pwrap_writel(wrp, 0x5, PWRAP_STAUPD_PRD);
	if (wrp->master->type == PWRAP_MT3612)
		pwrap_writel(wrp, 0x05, PWRAP_STAUPD_GRPEN);
	else
		pwrap_writel(wrp, 0xff, PWRAP_STAUPD_GRPEN);

	if (wrp->master->init_soc_specific) {
		ret = wrp->master->init_soc_specific(wrp);
		if (ret)
			return ret;
	}

	/* Setup the init done registers */
	pwrap_writel(wrp, 1, PWRAP_INIT_DONE2);
	if (wrp->master->type != PWRAP_MT3612) {
		pwrap_writel(wrp, 1, PWRAP_INIT_DONE0);
		pwrap_writel(wrp, 1, PWRAP_INIT_DONE1);
	}

	if (wrp->master->has_bridge) {
		writel(1, wrp->bridge_base + PWRAP_MT8135_BRIDGE_INIT_DONE3);
		writel(1, wrp->bridge_base + PWRAP_MT8135_BRIDGE_INIT_DONE4);
	}

	return 0;
}

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap IRQ handler.
 * @param[in]
 *     irqno: set IRQ number.
 * @param[in]
 *     *dev_id: set device ID.
 * @return
 *     Return IRQ_HANDLED.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */

/*static irqreturn_t pwrap_interrupt(int irqno, void *dev_id)
*{
*	u32 rdata;
*	struct pmic_wrapper *wrp = dev_id;
*	if (wrp->master->type == PWRAP_MT3612)
*		rdata = pwrap_readl(wrp, PWRAP_INT0_FLG);
*	else
*		rdata = pwrap_readl(wrp, PWRAP_INT_FLG);
*	dev_err(wrp->dev, "unexpected interrupt int=0x%x\n", rdata);
*	if (wrp->master->type == PWRAP_MT3612)
*		pwrap_writel(wrp, 0xffffffff, PWRAP_INT0_CLR);
*	else
*		pwrap_writel(wrp, 0xffffffff, PWRAP_INT_CLR);
*	return IRQ_HANDLED;
*}
*/

/** @ingroup type_group_linux_pwrap_struct
 * @brief For pmic wrap register mapping\n
 */
static const struct regmap_config pwrap_regmap_config = {
	.reg_bits = 16,
	.val_bits = 16,
	.reg_stride = 2,
	.reg_read = pwrap_regmap_read,
	.reg_write = pwrap_regmap_write,
	.max_register = 0xffff,
};
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_struct
 * @brief Defines for MT3615 pmic\n
 */
static const struct pwrap_slv_type pmic_mt3615 = {
	.dew_regs = mt3615_regs,
	.type = PMIC_MT3615,
};

/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_struct
 * @brief Defines for compatible pmic\n
 */
static const struct of_device_id of_slave_match_tbl[] = {
	{
		.compatible = "mediatek,mt3615",
		.data = &pmic_mt3615,
	}, {
		/* sentinel */
	}
};
/**
 * @}
 */

MODULE_DEVICE_TABLE(of, of_slave_match_tbl);

/** @ingroup type_group_linux_pwrap_struct
 * @brief Defines for pmic wrap initial configuration\n
 */
static const struct pmic_wrapper_type pwrap_mt3612 = {
	.regs = mt3612_regs,
	.type = PWRAP_MT3612,
	.arb_en_all = MT3612_ARBITER_SRC_EN,
	.int_en_all = 0xffffffff,
	.spi_w = PWRAP_MAN_CMD_SPI_WRITE,
	.wdt_src = PWRAP_WDT_SRC_MASK_ALL,
	.has_bridge = 0,
	.init_reg_clock = pwrap_mt3612_init_reg_clock,
	.init_soc_specific = 0,
};
/**
 * @}
 */

/** @ingroup type_group_linux_pwrap_struct
 * @brief Defines for compatible pmic wrap\n
 */
static const struct of_device_id of_pwrap_match_tbl[] = {
	{
		.compatible = "mediatek,mt3612-pwrap",
		.data = &pwrap_mt3612,
	}, {
		/* sentinel */
	}
};
/**
 * @}
 */

MODULE_DEVICE_TABLE(of, of_pwrap_match_tbl);

/** @ingroup type_group_linux_pwrap_InFn
 * @par Description
 *     PMIC wrap probe function.
 * @param[in]
 *     *pdev: a pointer for platform_device structure.
 * @return
 *     0, If return value is 0 for success.\n
 *     Non-zero, error code for failure.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int pwrap_probe(struct platform_device *pdev)
{
	int ret;
	/*int irg;*/
	struct pmic_wrapper *wrp;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *of_id =
	    of_match_device(of_pwrap_match_tbl, &pdev->dev);
	const struct of_device_id *of_slave_id = NULL;
	struct resource *res;
	u32 rdata;

	dev_info(&pdev->dev, "PWRAP Probe start\n");

	if (!of_id) {
		dev_err(&pdev->dev, "Error: No device match found\n");
		return -ENODEV;
	}

	if (pdev->dev.of_node->child)
		of_slave_id = of_match_node(of_slave_match_tbl,
					    pdev->dev.of_node->child);
	if (!of_slave_id) {
		dev_dbg(&pdev->dev, "slave pmic should be defined in dts\n");
		return -EINVAL;
	}

	wrp = devm_kzalloc(&pdev->dev, sizeof(*wrp), GFP_KERNEL);
	if (!wrp)
		return -ENOMEM;

	platform_set_drvdata(pdev, wrp);

	wrp->master = of_id->data;
	wrp->slave = of_slave_id->data;
	wrp->dev = &pdev->dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pwrap");
	wrp->base = devm_ioremap_resource(wrp->dev, res);
	if (IS_ERR(wrp->base))
		return PTR_ERR(wrp->base);

	wrp->rstc = devm_reset_control_get(wrp->dev, "pwrap");
	if (IS_ERR(wrp->rstc)) {
		ret = PTR_ERR(wrp->rstc);
		dev_dbg(wrp->dev, "cannot get pwrap reset: %d\n", ret);
		return ret;
	}

	if (wrp->master->has_bridge) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   "pwrap-bridge");
		wrp->bridge_base = devm_ioremap_resource(wrp->dev, res);
		if (IS_ERR(wrp->bridge_base))
			return PTR_ERR(wrp->bridge_base);

		wrp->rstc_bridge =
		    devm_reset_control_get(wrp->dev, "pwrap-bridge");
		if (IS_ERR(wrp->rstc_bridge)) {
			ret = PTR_ERR(wrp->rstc_bridge);
			dev_dbg(wrp->dev, "cannot get pwrap-bridge reset: %d\n",
				ret);
			return ret;
		}
	}

	wrp->clk_spi = devm_clk_get(wrp->dev, "spi");/*kiki_test start*/
	if (IS_ERR(wrp->clk_spi)) {
		dev_err(wrp->dev, "get clock(%ld) ng!\n",
			PTR_ERR(wrp->clk_spi));
		return PTR_ERR(wrp->clk_spi);
	}
	wrp->clk_gspi = devm_clk_get(wrp->dev, "gspi");
	if (IS_ERR(wrp->clk_gspi)) {
		dev_err(wrp->dev, "get clock(%ld) ng!\n",
			PTR_ERR(wrp->clk_gspi));
		return PTR_ERR(wrp->clk_gspi);
	}
	wrp->clk_cg_tmr = devm_clk_get(wrp->dev, "cg_tmr");
	if (IS_ERR(wrp->clk_cg_tmr)) {
		dev_err(wrp->dev, "get clock(%ld) ng!\n",
			PTR_ERR(wrp->clk_cg_tmr));
		return PTR_ERR(wrp->clk_cg_tmr);
	}
	wrp->clk_cg_ap = devm_clk_get(wrp->dev, "cg_ap");
	if (IS_ERR(wrp->clk_cg_ap)) {
		dev_err(wrp->dev, "get clock(%ld) ng!\n",
			PTR_ERR(wrp->clk_cg_ap));
		return PTR_ERR(wrp->clk_cg_ap);
	}
	wrp->clk_cg_md = devm_clk_get(wrp->dev, "cg_md");
	if (IS_ERR(wrp->clk_cg_md)) {
		dev_err(wrp->dev, "get clock(%ld) ng!\n",
			PTR_ERR(wrp->clk_cg_md));
		return PTR_ERR(wrp->clk_cg_md);
	}
	wrp->clk_cg_conn = devm_clk_get(wrp->dev, "cg_conn");
	if (IS_ERR(wrp->clk_cg_conn)) {
		dev_err(wrp->dev, "get clock(%ld) ng!\n",
			PTR_ERR(wrp->clk_cg_conn));
		return PTR_ERR(wrp->clk_cg_conn);
	}
	wrp->clk_cg_gspi = devm_clk_get(wrp->dev, "cg_gspi");
	if (IS_ERR(wrp->clk_cg_gspi)) {
		dev_err(wrp->dev, "get clock(%ld) ng!\n",
			PTR_ERR(wrp->clk_cg_gspi));
		return PTR_ERR(wrp->clk_cg_gspi);
	}

	ret = clk_prepare_enable(wrp->clk_spi);
	if (ret)
		return ret;
	ret = clk_prepare_enable(wrp->clk_gspi);
	if (ret)
		goto err_out1;
	ret = clk_prepare_enable(wrp->clk_cg_tmr);
	if (ret)
		goto err_out2;
	ret = clk_prepare_enable(wrp->clk_cg_ap);
	if (ret)
		goto err_out3;
	ret = clk_prepare_enable(wrp->clk_cg_md);
	if (ret)
		goto err_out4;
	ret = clk_prepare_enable(wrp->clk_cg_conn);
	if (ret)
		goto err_out5;
	ret = clk_prepare_enable(wrp->clk_cg_gspi);
	if (ret)
		goto err_out6;

	mt3615_pmic_wrapper = wrp;
	/* Enable internal dynamic clock */
	pwrap_writel(wrp, 1, PWRAP_DCM_EN);
	pwrap_writel(wrp, 0, PWRAP_DCM_DBC_PRD);

	/*
	 * The PMIC could already be initialized by the bootloader.
	 * Skip initialization here in this case.
	 */
	if (!pwrap_readl(wrp, PWRAP_INIT_DONE2)) {
		ret = pwrap_init(wrp);
		if (ret) {
			dev_dbg(wrp->dev, "init failed with %d\n", ret);
			goto err_out6;
		}
	}

	if (!(pwrap_readl(wrp, PWRAP_WACS2_RDATA) & PWRAP_STATE_INIT_DONE0)) {
		dev_dbg(wrp->dev, "initialization isn't finished\n");
		ret = -ENODEV;
		goto err_out6;
	}

	/* Initialize watchdog, may not be done by the bootloader */
	pwrap_writel(wrp, 0xf, PWRAP_WDT_UNIT);
	/*
	 * Since STAUPD was not used on mt8173 platform,
	 * so STAUPD of WDT_SRC which should be turned off
	 */
	pwrap_writel(wrp, wrp->master->wdt_src, PWRAP_WDT_SRC_EN);
	pwrap_writel(wrp, 0x1, PWRAP_TIMER_EN);

	if (wrp->master->type == PWRAP_MT3612) {
/*		pwrap_writel(wrp, wrp->master->int_en_all, PWRAP_INT0_EN);*/
/*		pwrap_writel(wrp, wrp->master->int_en_all, PWRAP_INT1_EN);*/
		pwrap_writel(wrp, 0x0, PWRAP_INT0_EN);
		pwrap_writel(wrp, 0x0, PWRAP_INT1_EN);
		dev_info(wrp->dev, "[PWRAP]MT3612\n");
	} else {
		pwrap_writel(wrp, wrp->master->int_en_all, PWRAP_INT_EN);
	}

	/* Read Test */
	pwrap_read(wrp, wrp->slave->dew_regs[PWRAP_DEW_READ_TEST], &rdata);
	if (rdata != PWRAP_DEW_READ_TEST_VAL)
		dev_err(wrp->dev, "Read test failed !\n");

	dev_info(wrp->dev, "PWRAP Initial Done! rdata = 0x%04x\n", rdata);

/*	irq = platform_get_irq(pdev, 0);
*	ret =
*	    devm_request_irq(wrp->dev, irq, pwrap_interrupt, IRQF_TRIGGER_HIGH,
*			     "mt-pmic-pwrap", wrp);
*	if (ret)
*		goto err_out2;
*/
	wrp->regmap =
	    devm_regmap_init(wrp->dev, NULL, wrp, &pwrap_regmap_config);
	if (IS_ERR(wrp->regmap)) {
		ret = PTR_ERR(wrp->regmap);
		goto err_out6;
	}

	ret = of_platform_populate(np, NULL, NULL, wrp->dev);
	if (ret) {
		dev_dbg(wrp->dev, "failed to create child devices at %s\n",
			np->full_name);
		goto err_out6;
	}

	pwrap_regmap_read(wrp, wrp->slave->dew_regs[PWRAP_DEW_HWCID], &rdata);
	dev_info(wrp->dev, "PWRAP Probe Done! HWCID = 0x%04x\n", rdata);

	return 0;

err_out6:
	clk_disable_unprepare(wrp->clk_cg_gspi);
err_out5:
	clk_disable_unprepare(wrp->clk_cg_conn);
err_out4:
	clk_disable_unprepare(wrp->clk_cg_md);
err_out3:
	clk_disable_unprepare(wrp->clk_cg_ap);
err_out2:
	clk_disable_unprepare(wrp->clk_cg_tmr);
err_out1:
	clk_disable_unprepare(wrp->clk_gspi);

	return ret;
}

/** @ingroup type_group_linux_pwrap_struct
 * @brief Defines for pmic wrap platform driver and probe function\n
 */
static struct platform_driver pwrap_drv = {
	.driver = {
		   .name = "mt-pmic-pwrap",
		   .of_match_table = of_match_ptr(of_pwrap_match_tbl),
		   },
	.probe = pwrap_probe,
};
/**
 * @}
 */

module_platform_driver(pwrap_drv);

MODULE_AUTHOR("Tony Chang, MediaTek");
MODULE_DESCRIPTION("MediaTek MT3612 PMIC Wrapper Driver");
MODULE_LICENSE("GPL v2");
