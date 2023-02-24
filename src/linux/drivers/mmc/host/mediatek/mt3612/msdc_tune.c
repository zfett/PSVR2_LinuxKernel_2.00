/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/**
 * @file Msdc_tune.c
 * The Msdc_tune.c file include function for optmal delay tuning function when
 * error in eMMC host controller.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/gpio.h>
#include <linux/delay.h>

#include "mtk_sd.h"
#include <mmc/core/core.h>
#include "dbg.h"
#include "autok.h"
/* #include "autok_dvfs.h" */

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Set read/write sample edge setting and current delay setting
 *     in eMMC host.
 * @param *host: msdc host structure.
 * @param save_mode: 1:emmc suspend,2:sdio suspend,3:power tuing,4:power off
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_save_timing_setting(struct msdc_host *host, int save_mode)
{
	struct msdc_hw *hw = host->hw;
	void __iomem *base = host->base;
#if !defined(CONFIG_MACH_FPGA)
	void __iomem *base_top;
	uint8_t i;
#endif
	/* save_mode: 1 emmc_suspend
	 *	      2 sdio_suspend
	 *	      3 power_tuning
	 *	      4 power_off
	 */

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, hw->rdata_edge);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, hw->wdata_edge);

	if ((save_mode == 1) || (save_mode == 2)) {
		host->saved_para.hz = host->mclk;
		host->saved_para.sdc_cfg = MSDC_READ32(SDC_CFG);
		host->saved_para.timing = host->timing;
		host->saved_para.msdc_cfg = MSDC_READ32(MSDC_CFG);
		host->saved_para.iocon = MSDC_READ32(MSDC_IOCON);
	}

	if (save_mode == 1)
		host->saved_para.emmc50_cfg0 = MSDC_READ32(EMMC50_CFG0);
	if (save_mode == 2) {
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
			      host->saved_para.ds_dly1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
			      host->saved_para.ds_dly3);
		host->saved_para.emmc50_pad_cmd_tune =
			MSDC_READ32(EMMC50_PAD_CMD_TUNE);
		host->saved_para.emmc50_dat01 =
			MSDC_READ32(EMMC50_PAD_DAT01_TUNE);
		host->saved_para.emmc50_dat23 =
			MSDC_READ32(EMMC50_PAD_DAT23_TUNE);
		host->saved_para.emmc50_dat45 =
			MSDC_READ32(EMMC50_PAD_DAT45_TUNE);
		host->saved_para.emmc50_dat67 =
			MSDC_READ32(EMMC50_PAD_DAT67_TUNE);
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, host->saved_para.mode);
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKDIV, host->saved_para.div);
		MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			host->saved_para.int_dat_latch_ck_sel);
		MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL,
			host->saved_para.ckgen_msdc_dly_sel);
		MSDC_GET_FIELD(MSDC_INTEN, MSDC_INT_SDIOIRQ,
			host->saved_para.inten_sdio_irq);
	}

	host->saved_para.pb0 = MSDC_READ32(MSDC_PATCH_BIT0);
	host->saved_para.pb1 = MSDC_READ32(MSDC_PATCH_BIT1);
	host->saved_para.pb2 = MSDC_READ32(MSDC_PATCH_BIT2);
	host->saved_para.sdc_fifo_cfg = MSDC_READ32(SDC_FIFO_CFG);

#if !defined(CONFIG_MACH_FPGA)
	if (host->base_top) {
		base_top = host->base_top;
		host->saved_para.emmc_top_control
			= MSDC_READ32(EMMC_TOP_CONTROL);
		host->saved_para.emmc_top_cmd
			= MSDC_READ32(EMMC_TOP_CMD);
		host->saved_para.top_emmc50_pad_ctl0
			= MSDC_READ32(TOP_EMMC50_PAD_CTL0);
		host->saved_para.top_emmc50_pad_ds_tune
			= MSDC_READ32(TOP_EMMC50_PAD_DS_TUNE);
		for (i = 0; i < 8; i++) {
			host->saved_para.top_emmc50_pad_dat_tune[i]
				= MSDC_READ32(TOP_EMMC50_PAD_DAT0_TUNE + i * 4);
		}
	}
#endif
	/*msdc_dump_register(host);*/
}

/*  HS400 can not lower frequence
 *  1. Change to HS200 mode, reinit
 *  2. Lower frequence
 */

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for change lower spped mode and retune when error in HS400.
 * @param *mmc: current mmc host structure.
 * @return
 *     If return 0, it mean retune OK.\n
 *     If return -1, it mean retune fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int emmc_reinit_tuning(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	void __iomem *base = host->base;
	u32 div = 0;
	u32 mode = 0;
	unsigned int caps_hw_reset = 0;

	if (!mmc->card) {
		pr_err("mmc card = NULL, skip reset tuning\n");
		return -1;
	}

	if (mmc->card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS400) {
		mmc->card->mmc_avail_type &= ~EXT_CSD_CARD_TYPE_HS400;
		pr_err("msdc%d: witch to HS200 mode, reinit card\n", host->id);
		if (mmc->caps & MMC_CAP_HW_RESET) {
			caps_hw_reset = 1;
		} else {
			caps_hw_reset = 0;
			mmc->caps |= MMC_CAP_HW_RESET;
		}
		mmc->ios.timing = MMC_TIMING_LEGACY;
		mmc->ios.clock = 260000;
		msdc_ops_set_ios(mmc, &mmc->ios);
		if (mmc_hw_reset(mmc))
			pr_err("msdc%d switch HS200 failed\n", host->id);
		/* recovery MMC_CAP_HW_RESET */
		if (!caps_hw_reset)
			mmc->caps &= ~MMC_CAP_HW_RESET;
		goto done;
	}

	/* lower frequence */
	if (mmc->card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200) {
		mmc->card->mmc_avail_type &= ~EXT_CSD_CARD_TYPE_HS200;
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKDIV, div);
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, mode);
		div += 1;
		if (div > EMMC_MAX_FREQ_DIV) {
			pr_err("msdc%d: max lower freq: %d\n", host->id, div);
			return 1;
		}
		msdc_clk_stable(host, mode, div, 1);
		host->sclk =
			(div == 0) ? host->hclk / 4 : host->hclk / (4 * div);
		pr_err("msdc%d: HS200 mode lower frequence to %dMhz\n",
			host->id, host->sclk / 1000000);
	}

done:
	return 0;

}

/*
 * register as callback function of WIFI(combo_sdio_register_pm) .
 * can called by msdc_drv_suspend/resume too.
 */

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for restore timing setting when host resume
 * @param *host: resumed msdc host.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_restore_timing_setting(struct msdc_host *host)
{
	void __iomem *base = host->base, *base_top = host->base_top;
	int retry = 3;
	int emmc = (host->hw->host_function == MSDC_EMMC) ? 1 : 0;
	int sdio = (host->hw->host_function == MSDC_SDIO) ? 1 : 0;
	int i;

	if (sdio) {
		msdc_reset_hw(host->id); /* force bit5(BV18SDT) to 0 */
		host->saved_para.msdc_cfg =
			host->saved_para.msdc_cfg & 0xFFFFFFDF;
		MSDC_WRITE32(MSDC_CFG, host->saved_para.msdc_cfg);
	}

	do {
		msdc_set_mclk(host, host->saved_para.timing,
			host->saved_para.hz);
		if ((MSDC_READ32(MSDC_CFG) & 0xFFFFFF9F) ==
		    (host->saved_para.msdc_cfg & 0xFFFFFF9F))
			break;
	} while (retry--);

	MSDC_WRITE32(SDC_CFG, host->saved_para.sdc_cfg);
	MSDC_WRITE32(MSDC_IOCON, host->saved_para.iocon);
	if (!host->base_top) {
		MSDC_WRITE32(MSDC_PAD_TUNE0, host->saved_para.pad_tune0);
		MSDC_WRITE32(MSDC_PAD_TUNE1, host->saved_para.pad_tune1);
	}

	if (emmc)
		MSDC_WRITE32(MSDC_PATCH_BIT0, host->saved_para.pb0);
	MSDC_WRITE32(MSDC_PATCH_BIT1, host->saved_para.pb1);
	MSDC_WRITE32(MSDC_PATCH_BIT2, host->saved_para.pb2);
	MSDC_WRITE32(SDC_FIFO_CFG, host->saved_para.sdc_fifo_cfg);

	if (sdio) {
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			host->saved_para.int_dat_latch_ck_sel);
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL,
			host->saved_para.ckgen_msdc_dly_sel);
		MSDC_SET_FIELD(MSDC_INTEN, MSDC_INT_SDIOIRQ,
			host->saved_para.inten_sdio_irq);

		autok_init_sdr104(host);
#ifdef ENABLE_FOR_MSDC_KERNEL44
		if (vcorefs_get_hw_opp() == OPPI_PERF)
			autok_tuning_parameter_init(host,
				host->autok_res[AUTOK_VCORE_HIGH]);
		else
			autok_tuning_parameter_init(host,
				host->autok_res[AUTOK_VCORE_LOW]);
#else
		autok_tuning_parameter_init(host,
			host->autok_res[AUTOK_VCORE_HIGH]);
#endif
		/* sdio_dvfs_reg_restore(host); */

		host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
		host->mmc->rescan_entered = 0;
	}

	if (!host->base_top) {
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
			host->saved_para.ds_dly1);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
			host->saved_para.ds_dly3);
		MSDC_WRITE32(EMMC50_PAD_CMD_TUNE,
			host->saved_para.emmc50_pad_cmd_tune);
		MSDC_WRITE32(EMMC50_PAD_DAT01_TUNE,
			host->saved_para.emmc50_dat01);
		MSDC_WRITE32(EMMC50_PAD_DAT23_TUNE,
			host->saved_para.emmc50_dat23);
		MSDC_WRITE32(EMMC50_PAD_DAT45_TUNE,
			host->saved_para.emmc50_dat45);
		MSDC_WRITE32(EMMC50_PAD_DAT67_TUNE,
			host->saved_para.emmc50_dat67);
	}

	MSDC_WRITE32(EMMC50_CFG0, host->saved_para.emmc50_cfg0);
	if (host->base_top) {
		MSDC_WRITE32(EMMC_TOP_CONTROL,
			    host->saved_para.emmc_top_control);
		MSDC_WRITE32(EMMC_TOP_CMD,
			    host->saved_para.emmc_top_cmd);
		MSDC_WRITE32(TOP_EMMC50_PAD_CTL0,
			    host->saved_para.top_emmc50_pad_ctl0);
		MSDC_WRITE32(TOP_EMMC50_PAD_DS_TUNE,
			    host->saved_para.top_emmc50_pad_ds_tune);
		for (i = 0; i < 8; i++) {
			MSDC_WRITE32(TOP_EMMC50_PAD_DAT0_TUNE + i * 4,
				    host->saved_para.
				    top_emmc50_pad_dat_tune[i]);
		}
	}
	/*msdc_dump_register(host);*/
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for initialize tuning path setting for each speed mode
 * @param *host: msdc host request tuning.
 * @param *host: speed mode request tuning.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_init_tune_path(struct msdc_host *host, unsigned char timing)
{
	void __iomem *base = host->base, *base_top = host->base_top;

	MSDC_WRITE32(MSDC_PAD_TUNE0,   0x00000000);
	if (host->base_top) {
		MSDC_CLR_BIT32(EMMC_TOP_CONTROL, DATA_K_VALUE_SEL);
		MSDC_CLR_BIT32(EMMC_TOP_CONTROL, DELAY_EN);
		MSDC_CLR_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY);
		MSDC_CLR_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY_SEL);
		MSDC_CLR_BIT32(EMMC_TOP_CONTROL, PAD_RXDLY_SEL);
		MSDC_CLR_BIT32(EMMC_TOP_CMD, PAD_CMD_RXDLY);
		MSDC_CLR_BIT32(EMMC_TOP_CMD, PAD_CMD_RD_RXDLY_SEL);
		MSDC_CLR_BIT32(TOP_EMMC50_PAD_CTL0, PAD_CLK_TXDLY);
	}
	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_DDLSEL);
	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL);
	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_R_D_SMPL);
	if (timing == MMC_TIMING_MMC_HS400) {
		MSDC_CLR_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL);
		MSDC_CLR_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL);
		if (host->base_top) {
			MSDC_CLR_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY_SEL);
			MSDC_CLR_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY2_SEL);
		}
	} else {
		MSDC_SET_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL);
		MSDC_CLR_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL);
		if (host->base_top) {
			MSDC_SET_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY_SEL);
			MSDC_CLR_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY2_SEL);
		}
	}

	if (timing == MMC_TIMING_MMC_HS400)
		MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
	else
		MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);

	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL);
	MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP);
	MSDC_SET_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLYSEL);
	MSDC_CLR_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRRDLY2SEL);
	if ((timing == MMC_TIMING_MMC_HS400) && (host->base_top)) {
		MSDC_SET_BIT32(EMMC_TOP_CMD, PAD_CMD_RD_RXDLY_SEL);
		MSDC_CLR_BIT32(EMMC_TOP_CMD, PAD_CMD_RD_RXDLY2_SEL);
	}
	MSDC_CLR_BIT32(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL);
	autok_path_sel(host);
}


/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for initialize tuning timing setting.
 * @param *host: msdc host request tuning.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_init_tune_setting(struct msdc_host *host)
{
	void __iomem *base = host->base, *base_top = host->base_top;

	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CLKTXDLY,
		MSDC_CLKTXDLY);
	if (host->base_top) {
		MSDC_SET_FIELD(TOP_EMMC50_PAD_CTL0, PAD_CLK_TXDLY,
					  MSDC_CLKTXDLY);
	}
	MSDC_WRITE32(MSDC_IOCON, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY0, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY1, 0x00000000);
	MSDC_WRITE32(MSDC_PATCH_BIT0, MSDC_PB0_DEFAULT_VAL);
	MSDC_WRITE32(MSDC_PATCH_BIT1, MSDC_PB1_DEFAULT_VAL);

	/* Fix HS400 mode */
	MSDC_CLR_BIT32(EMMC50_CFG0, MSDC_EMMC50_CFG_TXSKEW_SEL);
	MSDC_SET_BIT32(MSDC_PATCH_BIT1, MSDC_PB1_DDR_CMD_FIX_SEL);

	/* DDR50 mode */
	MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_DDR50_SEL);

	/* 64T + 48T cmd <-> resp */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPWAITCNT,
		MSDC_PB2_DEFAULT_RESPWAITCNT);
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
		MSDC_PB2_DEFAULT_RESPSTENSEL);
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
		MSDC_PB2_DEFAULT_CRCSTSENSEL);
	autok_path_sel(host);
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for assign TX path delay for each speed mode.
 * @param *host: msdc host request tuning.
 * @param *ios: timing spec
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void msdc_ios_tune_setting(struct msdc_host *host, struct mmc_ios *ios)
{
	autok_msdc_tx_setting(host, ios);
}
