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

#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include "autok_dvfs.h"
#include "mtk_sd.h"

#define SDIO_AUTOK_HIGH_RES_PATH    "/data/sdio_autok_high"
#define SDIO_AUTOK_LOW_RES_PATH     "/data/sdio_autok_low"

#define SDIO_AUTOK_DIFF_MARGIN      3

#define SDIO_ABACKUP_REG_COUNT      10
static u32 sdio_reg_backup[2][SDIO_ABACKUP_REG_COUNT];

static struct file *msdc_file_open(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);

	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}

	return filp;
}

static int msdc_file_read(struct file *file,
unsigned long long offset, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, data, size, &offset);

	set_fs(oldfs);

	return ret;
}

static int msdc_file_write(struct file *file,
unsigned long long offset, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, data, size, &offset);

	set_fs(oldfs);

	return ret;
}

int sdio_autok_res_exist(struct msdc_host *host)
{
	struct file *filp = NULL;

	filp = msdc_file_open(SDIO_AUTOK_LOW_RES_PATH, O_RDONLY, 0644);
	if (filp == NULL) {
		pr_err("autok result open fail\n");
		return 0;
	}
	filp_close(filp, NULL);

	filp = msdc_file_open(SDIO_AUTOK_HIGH_RES_PATH, O_RDONLY, 0644);
	if (filp == NULL) {
		pr_err("autok result open fail\n");
		return 0;
	}
	filp_close(filp, NULL);

	return 1;
}

int sdio_autok_res_apply(struct msdc_host *host, int vcore)
{
	struct file *filp = NULL;
	size_t size;
	u8 *res;
	int ret = -1;
	int i;

	if (vcore <= AUTOK_VCORE_LOW) {
		res = host->autok_res[AUTOK_VCORE_LOW];
		filp = msdc_file_open(SDIO_AUTOK_LOW_RES_PATH, O_RDONLY, 0644);
	} else {
		res = host->autok_res[AUTOK_VCORE_HIGH];
		filp = msdc_file_open(SDIO_AUTOK_HIGH_RES_PATH, O_RDONLY, 0644);
	}

	if (filp == NULL) {
		pr_err("autok result open fail\n");
		return ret;
	}

	size = msdc_file_read(filp, 0, res, TUNING_PARA_SCAN_COUNT);
	if (size == TUNING_PARA_SCAN_COUNT) {
		autok_tuning_parameter_init(host, res);

		for (i = 1; i < TUNING_PARA_SCAN_COUNT; i++)
			pr_err("autok exist!,result[%d]=%d\n", i, res[i]);
		ret = 0;
	}

	filp_close(filp, NULL);

	return ret;
}

int sdio_autok_res_save(struct msdc_host *host, int vcore, u8 *res)
{
	struct file *filp = NULL;
	size_t size;
	int ret = -1;

	if (res == NULL)
		return ret;

	if (vcore <= AUTOK_VCORE_LOW) {
		memcpy((void *)host->autok_res[AUTOK_VCORE_LOW],
			(const void *)res, TUNING_PARA_SCAN_COUNT);
		filp = msdc_file_open(SDIO_AUTOK_LOW_RES_PATH,
			O_CREAT | O_WRONLY, 0644);
	} else {
		memcpy((void *)host->autok_res[AUTOK_VCORE_HIGH],
			(const void *)res, TUNING_PARA_SCAN_COUNT);
		filp = msdc_file_open(SDIO_AUTOK_HIGH_RES_PATH,
			O_CREAT | O_WRONLY, 0644);
	}

	if (filp == NULL) {
		pr_err("autok result open fail\n");
		return ret;
	}

	size = msdc_file_write(filp, 0, res, TUNING_PARA_SCAN_COUNT);
	if (size == TUNING_PARA_SCAN_COUNT)
		ret = 0;
	vfs_fsync(filp, 0);

	filp_close(filp, NULL);

	return ret;
}

int autok_res_check(u8 *res_h, u8 *res_l)
{
	int ret = 0;
	int i;

	for (i = 0; i < TUNING_PARA_SCAN_COUNT; i++) {
		if ((i == CMD_RD_D_DLY1) || (i == DAT_RD_D_DLY1)) {
			if ((res_h[i] > res_l[i]) &&
				(res_h[i] - res_l[i] > SDIO_AUTOK_DIFF_MARGIN))
				ret = -1;
			if ((res_l[i] > res_h[i]) &&
				(res_l[i] - res_h[i] > SDIO_AUTOK_DIFF_MARGIN))
				ret = -1;
		} else if ((i == CMD_RD_D_DLY1_SEL) ||
			(i == DAT_RD_D_DLY1_SEL)) {
			/* this is cover by previous check,
			 * just by pass if 0 and 1 in cmd/dat delay
			 */
		} else {
			if (res_h[i] != res_l[i])
				ret = -1;
		}
	}

#ifndef SDIO_HW_DVFS_CONDITIONAL
	ret = -1;
#endif
	pr_err("autok_res_check %d!\n", ret);

	return ret;
}

void sdio_dvfs_reg_restore(struct msdc_host *host)
{
	void __iomem *base = host->base;

	/* High Vcore */
	MSDC_WRITE32(MSDC_IOCON_1, sdio_reg_backup[0][0]);
	MSDC_WRITE32(MSDC_PATCH_BIT0_1, sdio_reg_backup[0][1]);
	MSDC_WRITE32(MSDC_PATCH_BIT1_1, sdio_reg_backup[0][2]);
	MSDC_WRITE32(MSDC_PATCH_BIT2_1, sdio_reg_backup[0][3]);
	MSDC_WRITE32(MSDC_PAD_TUNE0_1, sdio_reg_backup[0][4]);
	MSDC_WRITE32(MSDC_PAD_TUNE1_1, sdio_reg_backup[0][5]);
	MSDC_WRITE32(MSDC_DAT_RDDLY0_1, sdio_reg_backup[0][6]);
	MSDC_WRITE32(MSDC_DAT_RDDLY1_1, sdio_reg_backup[0][7]);
	MSDC_WRITE32(MSDC_DAT_RDDLY2_1, sdio_reg_backup[0][8]);
	MSDC_WRITE32(MSDC_DAT_RDDLY3_1, sdio_reg_backup[0][9]);

	/* Low Vcore */
	MSDC_WRITE32(MSDC_IOCON_2, sdio_reg_backup[1][0]);
	MSDC_WRITE32(MSDC_PATCH_BIT0_2, sdio_reg_backup[1][1]);
	MSDC_WRITE32(MSDC_PATCH_BIT1_2, sdio_reg_backup[1][2]);
	MSDC_WRITE32(MSDC_PATCH_BIT2_2, sdio_reg_backup[1][3]);
	MSDC_WRITE32(MSDC_PAD_TUNE0_2, sdio_reg_backup[1][4]);
	MSDC_WRITE32(MSDC_PAD_TUNE1_2, sdio_reg_backup[1][5]);
	MSDC_WRITE32(MSDC_DAT_RDDLY0_2, sdio_reg_backup[1][6]);
	MSDC_WRITE32(MSDC_DAT_RDDLY1_2, sdio_reg_backup[1][7]);
	MSDC_WRITE32(MSDC_DAT_RDDLY2_2, sdio_reg_backup[1][8]);
	MSDC_WRITE32(MSDC_DAT_RDDLY3_2, sdio_reg_backup[1][9]);
}

static void sdio_dvfs_reg_backup(struct msdc_host *host)
{
	void __iomem *base = host->base;

	/* High Vcore */
	sdio_reg_backup[0][0] = MSDC_READ32(MSDC_IOCON_1);
	sdio_reg_backup[0][1] = MSDC_READ32(MSDC_PATCH_BIT0_1);
	sdio_reg_backup[0][2] = MSDC_READ32(MSDC_PATCH_BIT1_1);
	sdio_reg_backup[0][3] = MSDC_READ32(MSDC_PATCH_BIT2_1);
	sdio_reg_backup[0][4] = MSDC_READ32(MSDC_PAD_TUNE0_1);
	sdio_reg_backup[0][5] = MSDC_READ32(MSDC_PAD_TUNE1_1);
	sdio_reg_backup[0][6] = MSDC_READ32(MSDC_DAT_RDDLY0_1);
	sdio_reg_backup[0][7] = MSDC_READ32(MSDC_DAT_RDDLY1_1);
	sdio_reg_backup[0][8] = MSDC_READ32(MSDC_DAT_RDDLY2_1);
	sdio_reg_backup[0][9] = MSDC_READ32(MSDC_DAT_RDDLY3_1);

	/* Low Vcore */
	sdio_reg_backup[1][0] = MSDC_READ32(MSDC_IOCON_2);
	sdio_reg_backup[1][1] = MSDC_READ32(MSDC_PATCH_BIT0_2);
	sdio_reg_backup[1][2] = MSDC_READ32(MSDC_PATCH_BIT1_2);
	sdio_reg_backup[1][3] = MSDC_READ32(MSDC_PATCH_BIT2_2);
	sdio_reg_backup[1][4] = MSDC_READ32(MSDC_PAD_TUNE0_2);
	sdio_reg_backup[1][5] = MSDC_READ32(MSDC_PAD_TUNE1_2);
	sdio_reg_backup[1][6] = MSDC_READ32(MSDC_DAT_RDDLY0_2);
	sdio_reg_backup[1][7] = MSDC_READ32(MSDC_DAT_RDDLY1_2);
	sdio_reg_backup[1][8] = MSDC_READ32(MSDC_DAT_RDDLY2_2);
	sdio_reg_backup[1][9] = MSDC_READ32(MSDC_DAT_RDDLY3_2);
}

void sdio_set_hw_dvfs(int vcore, int done, struct msdc_host *host)
{
	void __iomem *base = host->base;

	if (vcore >= AUTOK_VCORE_HIGH) {
		MSDC_WRITE32(MSDC_IOCON_1, MSDC_READ32(MSDC_IOCON));
		MSDC_WRITE32(MSDC_PATCH_BIT0_1, MSDC_READ32(MSDC_PATCH_BIT0));
		MSDC_WRITE32(MSDC_PATCH_BIT1_1, MSDC_READ32(MSDC_PATCH_BIT1));
		MSDC_WRITE32(MSDC_PATCH_BIT2_1, MSDC_READ32(MSDC_PATCH_BIT2));
		MSDC_WRITE32(MSDC_PAD_TUNE0_1, MSDC_READ32(MSDC_PAD_TUNE0));
		MSDC_WRITE32(MSDC_PAD_TUNE1_1, MSDC_READ32(MSDC_PAD_TUNE1));
		MSDC_WRITE32(MSDC_DAT_RDDLY0_1, MSDC_READ32(MSDC_DAT_RDDLY0));
		MSDC_WRITE32(MSDC_DAT_RDDLY1_1, MSDC_READ32(MSDC_DAT_RDDLY1));
		MSDC_WRITE32(MSDC_DAT_RDDLY2_1, MSDC_READ32(MSDC_DAT_RDDLY2));
		MSDC_WRITE32(MSDC_DAT_RDDLY3_1, MSDC_READ32(MSDC_DAT_RDDLY3));
	} else {
		MSDC_WRITE32(MSDC_IOCON_2, MSDC_READ32(MSDC_IOCON));
		MSDC_WRITE32(MSDC_PATCH_BIT0_2, MSDC_READ32(MSDC_PATCH_BIT0));
		MSDC_WRITE32(MSDC_PATCH_BIT1_2, MSDC_READ32(MSDC_PATCH_BIT1));
		MSDC_WRITE32(MSDC_PATCH_BIT2_2, MSDC_READ32(MSDC_PATCH_BIT2));
		MSDC_WRITE32(MSDC_PAD_TUNE0_2, MSDC_READ32(MSDC_PAD_TUNE0));
		MSDC_WRITE32(MSDC_PAD_TUNE1_2, MSDC_READ32(MSDC_PAD_TUNE1));
		MSDC_WRITE32(MSDC_DAT_RDDLY0_2, MSDC_READ32(MSDC_DAT_RDDLY0));
		MSDC_WRITE32(MSDC_DAT_RDDLY1_2, MSDC_READ32(MSDC_DAT_RDDLY1));
		MSDC_WRITE32(MSDC_DAT_RDDLY2_2, MSDC_READ32(MSDC_DAT_RDDLY2));
		MSDC_WRITE32(MSDC_DAT_RDDLY3_2, MSDC_READ32(MSDC_DAT_RDDLY3));
	}

	if (done) {
		/* Enable HW DVFS */
		MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_DVFS_EN, 1);
		MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_DVFS_HW, 1);

		/* Backup the register, restore when resume */
		sdio_dvfs_reg_backup(host);
	}
}

/* For backward compatible, remove later */
int wait_sdio_autok_ready(void *data)
{
	return 0;
}
EXPORT_SYMBOL(wait_sdio_autok_ready);

void sdio_autok_wait_dvfs_ready(void)
{
	int dvfs;

#ifdef ENABLE_FOR_MSDC_KERNEL44
	dvfs = is_vcorefs_can_work();

	/* DVFS not ready, just wait */
	while (dvfs == 0) {
		pr_err("DVFS not ready\n");
		msleep(100);
		dvfs = is_vcorefs_can_work();
	}
#else
	dvfs = -1;
#endif

	if (dvfs == -1)
		pr_err("DVFS feature not enable\n");

	if (dvfs == 1)
		pr_err("DVFS ready\n");
}

int sd_execute_dvfs_autok(struct msdc_host *host, u32 opcode, u8 *res)
{
	int ret = 0;

	if (host->mmc->ios.timing == MMC_TIMING_UHS_SDR104 ||
	    host->mmc->ios.timing == MMC_TIMING_UHS_SDR50) {
		if (host->is_autok_done == 0) {
			pr_err("[AUTOK]SDcard autok\n");
			ret = autok_execute_tuning(host,
				host->autok_res[AUTOK_VCORE_HIGH]);
			host->is_autok_done = 1;
		} else {
			autok_init_sdr104(host);
			autok_tuning_parameter_init(host,
				host->autok_res[AUTOK_VCORE_HIGH]);
		}
	}

	return ret;
}

int emmc_execute_dvfs_autok(struct msdc_host *host, u32 opcode, u8 *res)
{
	int ret = 0;

	if (host->mmc->ios.timing == MMC_TIMING_MMC_HS200) {
		if (opcode == MMC_SEND_STATUS) {
			pr_err("[AUTOK]eMMC HS200 Tune CMD only\n");
			ret = hs200_execute_tuning_cmd(host, res);
		} else {
			pr_err("[AUTOK]eMMC HS200 Tune\n");
			ret = hs200_execute_tuning(host, res);
		}
	} else if (host->mmc->ios.timing == MMC_TIMING_MMC_HS400) {
		if (opcode == MMC_SEND_STATUS) {
			pr_err("[AUTOK]eMMC HS400 Tune CMD only\n");
			ret = hs400_execute_tuning_cmd(host, res);
		} else {
			pr_err("[AUTOK]eMMC HS400 Tune\n");
			ret = hs400_execute_tuning(host, res);
		}
	}

	return ret;
}

void sdio_execute_dvfs_autok(struct msdc_host *host)
{
	int sdio_res_exist = 0;

	if (host->is_autok_done) {
		autok_init_sdr104(host);
#ifdef ENABLE_FOR_MSDC_KERNEL44
		/* Check which vcore setting to apply */
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
		return;
	}
	/* HQA need read autok setting from file */
	sdio_res_exist = sdio_autok_res_exist(host);

	pr_err("[AUTOK]SDIO SDR104 Tune\n");
	/* Wait DFVS ready for excute autok here */
	sdio_autok_wait_dvfs_ready();

#ifdef ENABLE_FOR_MSDC_KERNEL44
	/* Performance mode, return 0 pass */
	if (vcorefs_request_dvfs_opp(KIR_AUTOK_SDIO, OPPI_PERF) != 0)
		pr_err("vcorefs_request_dvfs_opp@OPPI_PERF fail!\n");
#endif
	if (sdio_res_exist)
		sdio_autok_res_apply(host, AUTOK_VCORE_HIGH);
	else
		autok_execute_tuning(host, host->autok_res[AUTOK_VCORE_HIGH]);
	sdio_set_hw_dvfs(AUTOK_VCORE_HIGH, 0, host);

#ifdef ENABLE_FOR_MSDC_KERNEL44
	/* Low power mode, return 0 pass */
	if (vcorefs_request_dvfs_opp(KIR_AUTOK_SDIO, OPPI_LOW_PWR) != 0)
		pr_err("vcorefs_request_dvfs_opp@OPPI_LOW_PWR fail!\n");
#endif
	if (sdio_res_exist)
		sdio_autok_res_apply(host, AUTOK_VCORE_LOW);
	else
		autok_execute_tuning(host, host->autok_res[AUTOK_VCORE_LOW]);
	sdio_set_hw_dvfs(AUTOK_VCORE_LOW, 1, host);

	if (autok_res_check(host->autok_res[AUTOK_VCORE_HIGH],
			host->autok_res[AUTOK_VCORE_LOW]) == 0) {
		pr_err("[AUTOK] No need change para when dvfs\n");
	} else {
		pr_err("[AUTOK] Need change para when dvfs\n");

#ifdef ENABLE_FOR_MSDC_KERNEL44
		/* Use HW DVFS */
		sdio_use_dvfs = 1;
		/* Enable DVFS handshake */
		spm_msdc_dvfs_setting(MSDC2_DVFS, 1);
#endif
	}

#ifdef ENABLE_FOR_MSDC_KERNEL44
	/* Un-request, return 0 pass */
	if (vcorefs_request_dvfs_opp(KIR_AUTOK_SDIO, OPPI_UNREQ) != 0)
		pr_err("vcorefs_request_dvfs_opp@OPPI_UNREQ fail!\n");
#endif
	host->is_autok_done = 1;
	complete(&host->autok_done);
}

int emmc_autok(void)
{
	struct msdc_host *host = mtk_msdc_host[0];
	struct mmc_host *mmc = host->mmc;

	if (mmc == NULL) {
		pr_err("eMMC device not ready\n");
		return -1;
	}

	pr_err("emmc autok\n");

#if 0 /* Wait Light confirm */
	mmc_claim_host(mmc);

#ifdef ENABLE_FOR_MSDC_KERNEL44
	/* Performance mode, return 0 pass */
	if (vcorefs_request_dvfs_opp(KIR_AUTOK_EMMC, OPPI_PERF) != 0)
		pr_err("vcorefs_request_dvfs_opp@OPPI_PERF fail!\n");
#endif

	emmc_execute_dvfs_autok(host, MMC_SEND_TUNING_BLOCK_HS200,
		host->autok_res[AUTOK_VCORE_HIGH]);

#ifdef ENABLE_FOR_MSDC_KERNEL44
	/* Low power mode, return 0 pass */
	if (vcorefs_request_dvfs_opp(KIR_AUTOK_EMMC, OPPI_LOW_PWR) != 0)
		pr_err("vcorefs_request_dvfs_opp@OPPI_PERF fail!\n");
#endif

	emmc_execute_dvfs_autok(host, MMC_SEND_TUNING_BLOCK_HS200,
		host->autok_res[AUTOK_VCORE_LOW]);

#ifdef ENABLE_FOR_MSDC_KERNEL44
	/* Un-request, return 0 pass */
	if (vcorefs_request_dvfs_opp(KIR_AUTOK_EMMC, OPPI_UNREQ) != 0)
		pr_err("vcorefs_request_dvfs_opp@OPPI_UNREQ fail!\n");
#endif

	mmc_release_host(mmc);
#endif

	return 0;
}
EXPORT_SYMBOL(emmc_autok);

