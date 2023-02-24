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

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <linux/time.h>
#include <linux/delay.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>

#include <linux/completion.h>
#include <linux/scatterlist.h>

#include "autok.h"
#include "mtk_sd.h"

#define AUTOK_CMD_TIMEOUT               (HZ / 10)	/* 100ms */
#define AUTOK_DAT_TIMEOUT               (HZ * 3)	/* 1s x 3 */
#define HS400_ONE_CYCLE                 (50)
#define HS400_ACTUAL_CYCLE              (50 - 10 * 2)
#define MSDC_FIFO_THD_1K                (1024)
#define TUNE_CMD_TX_CNT                 (20)
#define TUNE_DAT_TX_CNT                 (3)
#define CHECK_QSR                       (0x800D)
/* #define TUNE_DATA_TX_ADDR (0x358000) */
/* Use negative value to represent address from end of device,
 * 33 blocks used by SGPT at end of device,
 * 32768 blocks used by flashinfo immediate before SGPT
 */
#define TUNE_DATA_TX_ADDR (-33-1)
#define CMDQ
#define AUTOK_LATCH_CK_EMMC_TUNE_TIMES  (10)
#define AUTOK_LATCH_CK_SDIO_TUNE_TIMES  (20)
#define AUTOK_LATCH_CK_SD_TUNE_TIMES    (20)
#define AUTOK_CMD_TIMES                 (20)
#define AUTOK_TUNING_INACCURACY         (3)
#define AUTOK_MARGIN_THOLD              (5)
#define AUTOK_BD_WIDTH_REF              (3)
#define AUTOK_READ                      0
#define AUTOK_WRITE                     1
#define AUTOK_FINAL_CKGEN_SEL           (0)
#define SCALE_TA_CNTR                   (8)
#define SCALE_CMD_RSP_TA_CNTR           (8)
#define SCALE_WDAT_CRC_TA_CNTR          (8)
#define SCALE_INT_DAT_LATCH_CK_SEL      (8)
#define SCALE_INTERNAL_DLY_CNTR         (32)
#define SCALE_PAD_DAT_DLY_CNTR          (32)
#define TUNING_INACCURACY (2)

/* autok platform specific setting */
#define AUTOK_CKGEN_VALUE                       (0)
#define AUTOK_CMD_LATCH_EN_HS400_PORT0_VALUE    (0)
#define AUTOK_CRC_LATCH_EN_HS400_PORT0_VALUE    (0)
#define AUTOK_CMD_LATCH_EN_HS200_PORT0_VALUE    (0)
#define AUTOK_CRC_LATCH_EN_HS200_PORT0_VALUE    (0)
#define AUTOK_CMD_LATCH_EN_SDR104_PORT1_VALUE   (0)
#define AUTOK_CRC_LATCH_EN_SDR104_PORT1_VALUE   (0)
#define AUTOK_CMD_LATCH_EN_HS_VALUE             (0)
#define AUTOK_CRC_LATCH_EN_HS_VALUE             (0)
#define AUTOK_CMD_TA_VALUE                      (0)
#define AUTOK_CRC_TA_VALUE                      (0)
#define AUTOK_BUSY_MA_VALUE                     (1)

/* autok msdc TX init setting */
#define AUTOK_MSDC0_HS400_CLKTXDLY            0
#define AUTOK_MSDC0_HS400_CMDTXDLY            9
#define AUTOK_MSDC0_HS400_DAT0TXDLY           0
#define AUTOK_MSDC0_HS400_DAT1TXDLY           0
#define AUTOK_MSDC0_HS400_DAT2TXDLY           0
#define AUTOK_MSDC0_HS400_DAT3TXDLY           0
#define AUTOK_MSDC0_HS400_DAT4TXDLY           0
#define AUTOK_MSDC0_HS400_DAT5TXDLY           0
#define AUTOK_MSDC0_HS400_DAT6TXDLY           0
#define AUTOK_MSDC0_HS400_DAT7TXDLY           0
#define AUTOK_MSDC0_HS400_TXSKEW              0
#define AUTOK_MSDC0_DDR50_DDRCKD              1
#define AUTOK_MSDC_DDRCKD                     0
#define AUTOK_MSDC0_CLKTXDLY                  0
#define AUTOK_MSDC0_CMDTXDLY                  0
#define AUTOK_MSDC0_DAT0TXDLY                 0
#define AUTOK_MSDC0_DAT1TXDLY                 0
#define AUTOK_MSDC0_DAT2TXDLY                 0
#define AUTOK_MSDC0_DAT3TXDLY                 0
#define AUTOK_MSDC0_DAT4TXDLY                 0
#define AUTOK_MSDC0_DAT5TXDLY                 0
#define AUTOK_MSDC0_DAT6TXDLY                 0
#define AUTOK_MSDC0_DAT7TXDLY                 0
#define AUTOK_MSDC0_TXSKEW                    0
#define AUTOK_MSDC1_CLK_TX_VALUE              0
#define AUTOK_MSDC1_CLK_SDR104_TX_VALUE       0
#define AUTOK_MSDC2_CLK_TX_VALUE              0
#define PORT0_PB0_RD_DAT_SEL_VALID

enum TUNE_TYPE {
	TUNE_CMD = 0,
	TUNE_DATA,
	TUNE_LATCH_CK,
};

enum TUNE_TX_TYPE {
	TX_CMD = 0,
	TX_DATA,
};

#define autok_msdc_retry(expr, retry, cnt) \
	do { \
		int backup = cnt; \
		while (retry) { \
			if (!(expr)) \
				break; \
			if (cnt-- == 0) { \
				retry--; cnt = backup; \
			} \
		} \
	WARN_ON(retry == 0); \
} while (0)

#define autok_msdc_reset() \
	do { \
		int retry = 3, cnt = 1000; \
		MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_RST); \
		/* ensure reset operation be sequential  */ \
		mb(); \
		autok_msdc_retry(MSDC_READ32(MSDC_CFG) & \
		MSDC_CFG_RST, retry, cnt); \
	} while (0)

#define msdc_rxfifocnt() \
	((MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_txfifocnt() \
	((MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)

#define wait_cond_tmo(cond, tmo) \
	do { \
		unsigned long timeout = jiffies + tmo; \
		while (1) { \
			if ((cond) || (tmo == 0)) \
				break; \
			if (time_after(jiffies, timeout)) \
				tmo = 0; \
		} \
	} while (0)

#define msdc_clear_fifo() \
	do { \
		int retry = 5, cnt = 1000; \
		MSDC_SET_BIT32(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
		/* ensure fifo clear operation be sequential  */ \
		mb(); \
		autok_msdc_retry(MSDC_READ32(MSDC_FIFOCS) & \
		MSDC_FIFOCS_CLR, retry, cnt); \
	} while (0)

struct AUTOK_PARAM_RANGE {
	unsigned int start;
	unsigned int end;
};

struct AUTOK_PARAM_INFO {
	struct AUTOK_PARAM_RANGE range;
	char *param_name;
};

struct BOUND_INFO {
	unsigned int Bound_Start;
	unsigned int Bound_End;
	unsigned int Bound_width;
	bool is_fullbound;
};

#define BD_MAX_CNT 4		/* Max Allowed Boundary Number */
struct AUTOK_SCAN_RES {
	/* Bound info record, currently only allow max to 2 bounds exist,
	 * but in extreme case, may have 4 bounds
	 */
	struct BOUND_INFO bd_info[BD_MAX_CNT];
	/* Bound cnt record, must be in rang [0,3] */
	unsigned int bd_cnt;
	/* Full boundary cnt record */
	unsigned int fbd_cnt;
};

struct AUTOK_REF_INFO {
	/* inf[0] - rising edge res, inf[1] - falling edge res */
	struct AUTOK_SCAN_RES scan_info[2];
	/* optimised sample edge select */
	unsigned int opt_edge_sel;
	/* optimised dly cnt sel */
	unsigned int opt_dly_cnt;
	/* 1clk cycle equal how many delay cell cnt, if cycle_cnt is 0,
	 * that is cannot calc cycle_cnt by current Boundary info
	 */
	unsigned int cycle_cnt;
};

struct BOUND_INFO_NEW {
	unsigned char Bound_Start;
	unsigned char Bound_End;
};

#define BD_MAX_CNT_NEW 32	/* Max Allowed Boundary Number */
struct AUTOK_SCAN_RES_NEW {
	/* Bound info record, currently only allow max to 32 fail bounds
	 * exist
	 */
	struct BOUND_INFO_NEW fail_bd_info[BD_MAX_CNT_NEW];
	struct BOUND_INFO_NEW pass_bd_info[BD_MAX_CNT_NEW];
	/* Bound cnt record */
	unsigned char fail_bd_cnt;
	unsigned char pass_bd_cnt;
};

struct AUTOK_REF_INFO_NEW {
	/* inf[0] - rising edge res, inf[1] - falling edge res */
	struct AUTOK_SCAN_RES_NEW scan_info[2];
	/* optimised sample edge select */
	unsigned int opt_edge_sel;
	/* optimised dly cnt sel */
	unsigned int opt_dly_cnt;
	/* 1clk cycle equal how many delay cell cnt, if cycle_cnt is 0,
	 * that is cannot calc cycle_cnt by current Boundary info
	 */
	unsigned int cycle_cnt;
};
struct AUTOK_TX_RES {
	unsigned int tx_result;
	unsigned int tx_err;
};

struct AUTOK_TX_PARA {
	unsigned int tx_cmd;
	unsigned int tx_dat;
	unsigned int tx_err_type;
};

enum AUTOK_TX_SCAN_STA_E {
	START_POSITION = 0,
	PASS_POSITION,
	FAIL_POSITION,
};

enum AUTOK_SCAN_WIN {
	CMD_RISE,
	CMD_FALL,
	DAT_RISE,
	DAT_FALL,
	DS_WIN,
};

enum EXD_RW_FLAG {
	EXT_READ = 0,
	EXT_WRITE,
};

#define TX_BD_MAX_CNT 16	/* Max Boundary Number */
struct AUTOK_TX_SCAN_RES {
	/* Bound info record, currently only allow max to
	 * 16 fail bounds exist
	 */
	struct AUTOK_PARAM_RANGE pass_bd_info[TX_BD_MAX_CNT];
	struct AUTOK_PARAM_RANGE fail_bd_info[TX_BD_MAX_CNT];
	/* Bound cnt record */
	unsigned int pass_bd_cnt;
	unsigned int fail_bd_cnt;
};

unsigned int autok_debug_level = AUTOK_DBG_RES;

const struct AUTOK_PARAM_INFO autok_param_info[] = {
	{{0, 1}, "CMD_EDGE"},
	{{0, 1}, "RDATA_EDGE"},
	{{0, 1}, "RD_FIFO_EDGE"},
	{{0, 1}, "WD_FIFO_EDGE"},
	{{0, 31}, "CMD_RD_D_DLY1"},
	{{0, 1}, "CMD_RD_D_DLY1_SEL"},
	{{0, 31}, "CMD_RD_D_DLY2"},
	{{0, 1}, "CMD_RD_D_DLY2_SEL"},
	{{0, 31}, "DAT_RD_D_DLY1"},
	{{0, 1}, "DAT_RD_D_DLY1_SEL"},
	{{0, 31}, "DAT_RD_D_DLY2"},
	{{0, 1}, "DAT_RD_D_DLY2_SEL"},
	{{0, 7}, "INT_DAT_LATCH_CK"},
	{{0, 31}, "EMMC50_DS_Z_DLY1"},
	{{0, 1}, "EMMC50_DS_Z_DLY1_SEL"},
	{{0, 31}, "EMMC50_DS_Z_DLY2"},
	{{0, 1}, "EMMC50_DS_Z_DLY2_SEL"},
	{{0, 31}, "EMMC50_DS_ZDLY_DLY"},
	{{0, 31}, "EMMC50_CMD_TX_DLY"},
	{{0, 31}, "EMMC50_DAT0_TX_DLY"},
	{{0, 31}, "EMMC50_DAT1_TX_DLY"},
	{{0, 31}, "EMMC50_DAT2_TX_DLY"},
	{{0, 31}, "EMMC50_DAT3_TX_DLY"},
	{{0, 31}, "EMMC50_DAT4_TX_DLY"},
	{{0, 31}, "EMMC50_DAT5_TX_DLY"},
	{{0, 31}, "EMMC50_DAT6_TX_DLY"},
	{{0, 31}, "EMMC50_DAT7_TX_DLY"},
	/* Timming Related Mux & Common Setting Config */
	{{0, 1}, "READ_DATA_SMPL_SEL"},
	{{0, 1}, "WRITE_DATA_SMPL_SEL"},
	{{0, 1}, "DATA_DLYLINE_SEL"},
	{{0, 1}, "MSDC_WCRC_ASYNC_FIFO_SEL"},
	{{0, 1}, "MSDC_RESP_ASYNC_FIFO_SEL"},
	/* eMMC50 Function Mux */
	{{0, 1}, "EMMC50_WDATA_MUX_EN"},
	{{0, 1}, "EMMC50_CMD_MUX_EN"},
	{{0, 1}, "EMMC50_CMD_RESP_LATCH"},
	{{0, 1}, "EMMC50_WDATA_EDGE"},
	/* Common Setting Config */
	{{0, 31}, "CKGEN_MSDC_DLY_SEL"},
	{{1, 7}, "CMD_RSP_TA_CNTR"},
	{{1, 7}, "WRDAT_CRCS_TA_CNTR"},
	{{0, 31}, "PAD_CLK_TXDLY_TUNE"},
};

/**********************************************************
* AutoK Basic Interface Implenment                        *
**********************************************************/
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for sending command used for tuning
 * @param *host: msdc host request for tuning
 * @param opcode: command code for tuning
 * @param tune_type_value: indicate CMD or DAT type tuning
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_send_tune_cmd(struct msdc_host *host, unsigned int opcode,
			       enum TUNE_TYPE tune_type_value)
{
	void __iomem *base = host->base;
	unsigned int value;
	unsigned int raw_cmd = 0;
	unsigned int arg = 0;
	unsigned int sts = 0;
	unsigned int wints = 0;
	unsigned long tmo = 0;
	unsigned long write_tmo = 0;
	unsigned int left = 0;
	unsigned int fifo_have = 0;
	unsigned int fifo_1k_cnt = 0;
	unsigned int i = 0;
	int ret = E_RESULT_PASS;

	switch (opcode) {
	case MMC_SEND_EXT_CSD:
		raw_cmd = (512 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (8);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			MSDC_WRITE32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			MSDC_WRITE32(SDC_BLK_NUM, 1);
		break;
	case MMC_STOP_TRANSMISSION:
		raw_cmd = (1 << 14) | (7 << 7) | (12);
		arg = 0;
		break;
	case MMC_SEND_STATUS:
		raw_cmd = (1 << 7) | (13);
		arg = (1 << 16);
		break;
	case CHECK_QSR:
		raw_cmd = (1 << 7) | (13);
		arg = (1 << 16) | (1 << 15);
		break;
	case MMC_READ_SINGLE_BLOCK:
		left = 512;
		raw_cmd = (512 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (17);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			MSDC_WRITE32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			MSDC_WRITE32(SDC_BLK_NUM, 1);
		break;
	case MMC_SEND_TUNING_BLOCK:
		left = 64;
		raw_cmd = (64 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (19);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			MSDC_WRITE32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			MSDC_WRITE32(SDC_BLK_NUM, 1);
		break;
	case MMC_SEND_TUNING_BLOCK_HS200:
		left = 128;
		raw_cmd = (128 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (21);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			MSDC_WRITE32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			MSDC_WRITE32(SDC_BLK_NUM, 1);
		break;
	case MMC_WRITE_BLOCK:
		raw_cmd = (512 << 16) | (1 << 13) | (1 << 11) | (1 << 7) | (24);
		if (TUNE_DATA_TX_ADDR >= 0)
			arg = TUNE_DATA_TX_ADDR;
		else
			arg = host->total_sectors + TUNE_DATA_TX_ADDR;
		break;
	case SD_IO_RW_DIRECT:
		break;
	case SD_IO_RW_EXTENDED:
		break;
	}

	tmo = AUTOK_DAT_TIMEOUT;
	wait_cond_tmo(!(MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY), tmo);
	if (tmo == 0) {
		AUTOK_RAWPRINT("[AUTOK]MSDC busy tmo1 cmd%d goto end...\n",
			      opcode);
		ret = E_RESULT_FATAL_ERR;
		goto end;
	}

	/* clear fifo */
	if (tune_type_value == TUNE_CMD) {
		MSDC_WRITE32(MSDC_INT, MSDC_INT_CMDTMO |
			    MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR);
	} else if (tune_type_value == TUNE_DATA) {
		autok_msdc_reset();
		msdc_clear_fifo();
		MSDC_WRITE32(MSDC_INT, 0xffffffff);
	}

	/* start command */
	MSDC_WRITE32(SDC_ARG, arg);
	MSDC_WRITE32(SDC_CMD, raw_cmd);

	/* wait interrupt status */
	wints = MSDC_INT_CMDTMO | MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR;
	tmo = AUTOK_CMD_TIMEOUT;
	wait_cond_tmo(((sts = MSDC_READ32(MSDC_INT)) & wints), tmo);
	if (tmo == 0) {
		AUTOK_RAWPRINT("[AUTOK]CMD%d wait int tmo\r\n", opcode);
		ret = E_RESULT_CMD_TMO;
		goto end;
	}

	MSDC_WRITE32(MSDC_INT, (sts & wints));
	if (sts == 0) {
		ret = E_RESULT_CMD_TMO;
		goto end;
	}

	if (sts & MSDC_INT_CMDRDY) {
		if (tune_type_value == TUNE_CMD) {
			ret = E_RESULT_PASS;
			goto end;
		}
	} else if (sts & MSDC_INT_RSPCRCERR) {
		ret = E_RESULT_RSP_CRC;
		goto end;
	} else if (sts & MSDC_INT_CMDTMO) {
		ret = E_RESULT_CMD_TMO;
		goto end;
	}

	if ((tune_type_value != TUNE_LATCH_CK)
	    && (tune_type_value != TUNE_DATA))
		goto skip_tune_latch_ck_and_tune_data;
	tmo = jiffies + AUTOK_DAT_TIMEOUT;
	while ((MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY) && (tmo != 0)) {
		if (time_after(jiffies, tmo))
			tmo = 0;
		if (tune_type_value == TUNE_LATCH_CK) {
			fifo_have = msdc_rxfifocnt();
			if ((opcode == MMC_SEND_TUNING_BLOCK_HS200)
			    || (opcode == MMC_READ_SINGLE_BLOCK)
			    || (opcode == MMC_SEND_EXT_CSD)
			    || (opcode == MMC_SEND_TUNING_BLOCK)) {
				MSDC_SET_FIELD(MSDC_DBG_SEL, 0xffff << 0, 0x0b);
				MSDC_GET_FIELD(MSDC_DBG_OUT, 0x7ff << 0,
					       fifo_1k_cnt);
				if ((fifo_1k_cnt >= MSDC_FIFO_THD_1K)
				    && (fifo_have >= MSDC_FIFO_SZ)) {
					value = MSDC_READ32(MSDC_RXDATA);
					value = MSDC_READ32(MSDC_RXDATA);
					value = MSDC_READ32(MSDC_RXDATA);
					value = MSDC_READ32(MSDC_RXDATA);
				}
			}
		} else if ((tune_type_value == TUNE_DATA) &&
			   (opcode == MMC_WRITE_BLOCK)) {
			for (i = 0; i < 64; i++) {
				MSDC_WRITE32(MSDC_TXDATA, 0x5af00fa5);
				MSDC_WRITE32(MSDC_TXDATA, 0x33cc33cc);
			}

			write_tmo = AUTOK_DAT_TIMEOUT;
			wait_cond_tmo(!(MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY),
				      write_tmo);
			if (write_tmo == 0) {
				AUTOK_RAWPRINT
				    ("MSDC busy tmo2 when wr cmd%d goto end\n",
				     opcode);
				ret = E_RESULT_FATAL_ERR;
				goto end;
			}
		}
	}
	if (tmo == 0) {
		AUTOK_RAWPRINT("[AUTOK]MSDC busy tmo3 cmd%d goto end...\n",
			       opcode);
		ret = E_RESULT_FATAL_ERR;
		goto end;
	}

	sts = MSDC_READ32(MSDC_INT);
	wints = MSDC_INT_XFER_COMPL | MSDC_INT_DATCRCERR | MSDC_INT_DATTMO;
	if (sts) {
		/* clear status */
		MSDC_WRITE32(MSDC_INT, (sts & wints));
		if (sts & MSDC_INT_XFER_COMPL)
			ret = E_RESULT_PASS;
		if (MSDC_INT_DATCRCERR & sts)
			ret = E_RESULT_DAT_CRC;
		if (MSDC_INT_DATTMO & sts)
			ret = E_RESULT_DAT_TMO;
	}

skip_tune_latch_ck_and_tune_data:
	tmo = AUTOK_DAT_TIMEOUT;
	wait_cond_tmo(!(MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY), tmo);
	if (tmo == 0) {
		AUTOK_RAWPRINT("[AUTOK]MSDC busy tmo4 cmd%d goto end...\n",
			       opcode);
		ret = E_RESULT_FATAL_ERR;
		goto end;
	}
	if ((tune_type_value == TUNE_CMD) || (tune_type_value == TUNE_DATA))
		msdc_clear_fifo();

end:
	if (opcode == MMC_STOP_TRANSMISSION) {
		tmo = AUTOK_DAT_TIMEOUT;
		wait_cond_tmo(((MSDC_READ32(MSDC_PS) & 0x10000) == 0x10000),
			      tmo);
		if (tmo == 0) {
			AUTOK_RAWPRINT
			    ("[AUTOK]DTA0 busy tmo cmd%d goto end...\n",
			     opcode);
			ret = E_RESULT_FATAL_ERR;
		}
	}

	return ret;
}


/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Collect information of calibration window for 32 taps
 * @param *res_str: string for calibration window
 * @param result: indicator for calibration window index
 * @return
 *     Count for pass delay
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_simple_score(char *res_str, unsigned int result)
{
	unsigned int bit = 0;
	unsigned int num = 0;
	unsigned int old = 0;

	if (result == 0) {
		strcpy(res_str, "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
		return 32;
	}
	if (result == 0xFFFFFFFF) {
		strcpy(res_str, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
		return 0;
	}

	/* calc continue zero number */
	while (bit < 32) {
		if (result & (1 << bit)) {
			res_str[bit] = 'X';
			bit++;
			if (old < num)
				old = num;
			num = 0;
			continue;
		}
		res_str[bit] = 'O';
		bit++;
		num++;
	}
	if (num > old)
		old = num;

	res_str[32] = '\0';
	return old;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Collect information of calibration window for 64 taps
 * @param *res_str64: string for calibration window
 * @param result: indicator for calibration window index
 * @return
 *     Count for pass delay
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_simple_score64(char *res_str64, u64 result64)
{
	unsigned int bit = 0;
	unsigned int num = 0;
	unsigned int old = 0;

	if (result64 == 0) {
		/* maybe result is 0 */
		strcpy(res_str64,
		       "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
		return 64;
	}
	if (result64 == 0xFFFFFFFFFFFFFFFF) {
		strcpy(res_str64,
		       "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
		return 0;
	}

	/* calc continue zero number */
	while (bit < 64) {
		if (result64 & ((u64) (1LL << bit))) {
			res_str64[bit] = 'X';
			bit++;
			if (old < num)
				old = num;
			num = 0;
			continue;
		}
		res_str64[bit] = 'O';
		bit++;
		num++;
	}
	if (num > old)
		old = num;

	res_str64[64] = '\0';
	return old;
}

enum {
	RD_SCAN_NONE,
	RD_SCAN_PAD_BOUND_S,
	RD_SCAN_PAD_BOUND_E,
	RD_SCAN_PAD_MARGIN,
};

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Collect information of calibration result
 * @param raw_dat: indicator for calibration window index
 * @param *scan_res: scan result information
 * @param bd_filter: check count for bd filter
 * @return
 *     If 0, it mean boundary collect OK. \n
 *	 If -1, it mean boundary collect error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_check_scan_res64(u64 raw_dat, struct AUTOK_SCAN_RES *scan_res,
				  unsigned int bd_filter)
{
	unsigned int bit;
	struct BOUND_INFO *bd = (struct BOUND_INFO *)scan_res->bd_info;
	unsigned int raw_scan_sta = RD_SCAN_NONE;

	for (bit = 0; bit < 64; bit++) {
		if (raw_dat & (1LL << bit)) {
			switch (raw_scan_sta) {
			case RD_SCAN_NONE:
				raw_scan_sta = RD_SCAN_PAD_BOUND_S;
				bd->Bound_Start = 0;
				bd->Bound_width = 1;
				scan_res->bd_cnt += 1;
				break;
			case RD_SCAN_PAD_MARGIN:
				raw_scan_sta = RD_SCAN_PAD_BOUND_S;
				bd->Bound_Start = bit;
				bd->Bound_width = 1;
				scan_res->bd_cnt += 1;
				break;
			case RD_SCAN_PAD_BOUND_E:
				if ((bit - bd->Bound_End) <= bd_filter) {
					raw_scan_sta = RD_SCAN_PAD_BOUND_S;

					bd->Bound_width +=
					    (bit - bd->Bound_End);
					bd->Bound_End = 0;

					/* update full bound info */
					if (bd->is_fullbound) {
						bd->is_fullbound = 0;
						scan_res->fbd_cnt -= 1;
					}
				} else {
					/* No filter Check and Get the next
					 * boundary information
					 */
					raw_scan_sta = RD_SCAN_PAD_BOUND_S;
					bd++;
					bd->Bound_Start = bit;
					bd->Bound_width = 1;
					scan_res->bd_cnt += 1;
					if (scan_res->bd_cnt > BD_MAX_CNT) {
						AUTOK_RAWPRINT
						("more than %d Boud Exist\r\n",
						 BD_MAX_CNT);
						return -1;
					}
				}
				break;
			case RD_SCAN_PAD_BOUND_S:
				bd->Bound_width++;
				break;
			default:
				break;
			}
		} else {
			switch (raw_scan_sta) {
			case RD_SCAN_NONE:
				raw_scan_sta = RD_SCAN_PAD_MARGIN;
				break;
			case RD_SCAN_PAD_BOUND_S:
				raw_scan_sta = RD_SCAN_PAD_BOUND_E;
				bd->Bound_End = bit - 1;
				/* update full bound info */
				if (bd->Bound_Start > 0) {
					bd->is_fullbound = 1;
					scan_res->fbd_cnt += 1;
				}
				break;
			case RD_SCAN_PAD_MARGIN:
			case RD_SCAN_PAD_BOUND_E:
			default:
				break;
			}
		}
	}
	if ((bd->Bound_End == 0) && (bd->Bound_width != 0))
		bd->Bound_End = bd->Bound_Start + bd->Bound_width - 1;

	return 0;
}

#if !defined(CONFIG_MACH_FPGA)
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Collect information of calibration result
 * @param raw_dat: indicator for calibration window index
 * @param *scan_res: scan result information
 * @param bd_filter: check count for bd filter
 * @return
 *     If 0, it mean boundary collect OK. \n
 *	 If -1, it mean boundary collect error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_check_scan_res64_new(u64 rawdat,
				      struct AUTOK_SCAN_RES_NEW *scan_res,
				      unsigned int bd_filter)
{
	unsigned int bit;
	int i, j;
	unsigned char fail_bd_info_cnt = 0;
	unsigned char pass_bd_info_cnt = 0;
	enum AUTOK_TX_SCAN_STA_E RawScanSta = START_POSITION;

	/* check scan window boundary */
	for (bit = 0; bit < 64; bit++) {
		if (rawdat & (1LL << bit)) {
			switch (RawScanSta) {
			case START_POSITION:
				RawScanSta = FAIL_POSITION;
				scan_res->fail_bd_info[fail_bd_info_cnt++]
				.Bound_Start = bit;
				scan_res->fail_bd_cnt++;
				break;
			case PASS_POSITION:
				RawScanSta = FAIL_POSITION;
				if (bit == 63) {
					scan_res->
					fail_bd_info[fail_bd_info_cnt++]
					.Bound_Start = bit;
					scan_res->
					fail_bd_info[fail_bd_info_cnt - 1]
					.Bound_End = bit;
				} else
					scan_res->
					fail_bd_info[fail_bd_info_cnt++]
					.Bound_Start = bit;
				scan_res->pass_bd_info[pass_bd_info_cnt -
					1].Bound_End = bit - 1;
				scan_res->fail_bd_cnt++;
				break;
			case FAIL_POSITION:
				RawScanSta = FAIL_POSITION;
				if (bit == 63)
					scan_res->
					fail_bd_info[fail_bd_info_cnt - 1]
					.Bound_End = bit;
				break;
			default:
				break;
			}
		} else {
			switch (RawScanSta) {
			case START_POSITION:
				RawScanSta = PASS_POSITION;
				scan_res->pass_bd_info[pass_bd_info_cnt++]
				.Bound_Start = bit;
				scan_res->pass_bd_cnt++;
				break;
			case PASS_POSITION:
				RawScanSta = PASS_POSITION;
				if (bit == 63)
					scan_res->
					pass_bd_info[pass_bd_info_cnt - 1]
					.Bound_End = bit;
				break;
			case FAIL_POSITION:
				RawScanSta = PASS_POSITION;
				if (bit == 63) {
					scan_res->
					pass_bd_info[pass_bd_info_cnt++]
					.Bound_Start = bit;
					scan_res->
					pass_bd_info[pass_bd_info_cnt - 1]
					.Bound_End = bit;
				} else
					scan_res->
					pass_bd_info[pass_bd_info_cnt++]
					.Bound_Start = bit;
				scan_res->fail_bd_info[fail_bd_info_cnt -
					1].Bound_End = bit - 1;
				scan_res->pass_bd_cnt++;
				break;
			default:
				break;
			}
		}
	}

	for (i = scan_res->fail_bd_cnt; i >= 0; i--) {
		if (i > scan_res->fail_bd_cnt)
			break;
		if ((i >= 1) && ((scan_res->fail_bd_info[i].Bound_Start
			- scan_res->fail_bd_info[i - 1].Bound_End - 1) <
			bd_filter)) {
			scan_res->fail_bd_info[i - 1].Bound_End =
				scan_res->fail_bd_info[i].Bound_End;
			scan_res->fail_bd_info[i].Bound_Start = 0;
			scan_res->fail_bd_info[i].Bound_End = 0;
			for (j = i; j < (scan_res->fail_bd_cnt - 1); j++) {
				scan_res->fail_bd_info[j].Bound_Start =
					scan_res->fail_bd_info[j +
					1].Bound_Start;
				scan_res->fail_bd_info[j].Bound_End =
					scan_res->fail_bd_info[j + 1].Bound_End;
			}
			scan_res->fail_bd_info[scan_res->fail_bd_cnt -
				1].Bound_Start = 0;
			scan_res->fail_bd_info[scan_res->fail_bd_cnt -
				1].Bound_End = 0;
			scan_res->fail_bd_cnt--;
		}
	}

	return 0;
}
#endif

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Calculate OK delay count
 * @param result: information of calibration result window
 * @return
 *     cnt: OK delay count
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_calc_bit_cnt(unsigned int result)
{
	unsigned int cnt = 0;

	if (result == 0)
		return 0;
	else if (result == 0xFFFFFFFF)
		return 32;

	do {
		if (result & 0x1)
			cnt++;

		result = result >> 1;
	} while (result);

	return cnt;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Calculate OK delay count
 * @param *raw_list: information of calibration pass window
 * @param *pmid: optimal delay index
 * @return
 *     If 0, find optimal delay setting. \n
 *     If -1, pass window is small or fail to find optimal delay.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_ta_sel(u32 *raw_list, u32 *pmid)
{
	int i = 0;
	unsigned int raw = 0;
	unsigned int r_start = 0;	/* range start */
	unsigned int r_stop = 7;	/* range stop */
	unsigned int score = 0;
	unsigned int score_max = 0;
	unsigned int score_max_id = 0;
	unsigned int inaccuracy = TUNING_INACCURACY;

	for (i = 0; i <= 7; i++) {
		score = autok_calc_bit_cnt(raw_list[i] ^ 0xFFFFFFFF);
		if (score > score_max) {
			score_max = score;
			score_max_id = i;
		}
	}
	if (score_max < 5)
		return -1;

	raw = raw_list[score_max_id];
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]The maximum offset is %d\r\n",
		       score_max_id);
FIND:
	for (i = 0; i <= 7; i++) {
		if (autok_calc_bit_cnt(raw_list[i] ^ raw) <= inaccuracy) {
			r_start = i;
			break;
		}
	}
	for (i = 7; i >= 0; i--) {
		if (autok_calc_bit_cnt(raw_list[i] ^ raw) <= inaccuracy) {
			r_stop = i;
			break;
		}
	}
	if ((r_start + 2) <= r_stop) {
		/* At least get the TA of which the margin has either left 1T
		 * and right 1T
		 */
		*pmid = (r_start + r_stop + 1) / 2;
	} else {
		inaccuracy++;
		if (inaccuracy < 5) {
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "Enlarge the inaccuracy[%d]\r\n",
				       inaccuracy);
			goto FIND;
		}
	}
	if (*pmid) {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]Find suitable range[%d %d], TA_sel=%d\r\n",
			       r_start, r_stop, *pmid);
	} else {
		*pmid = score_max_id;
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "Un-expected pattern, pls check!, TA_sel=%d\r\n",
			       *pmid);
	}

	return 0;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Software flow to calculate optimal delay for rising/falling edge
 *     sampling edge
 * @param *info: information for edge sel and optimal cnt
 * @return
 *     If 0, find optimal delay setting. \n
 *     If -1, optimal delay setting search fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_pad_dly_sel(struct AUTOK_REF_INFO *info)
{
	struct AUTOK_SCAN_RES *bd_info_r = NULL;
	struct AUTOK_SCAN_RES *bd_info_f = NULL;
	struct BOUND_INFO *bd_prev = NULL;
	struct BOUND_INFO *bd_next = NULL;
	struct BOUND_INFO *bd_tmp = NULL;
	unsigned int f_bound_cnt_r = 0;	/* Full Boundary count */
	unsigned int bound_cnt_r = 0;
	unsigned int bound_cnt_f = 0;
	unsigned int cycle_cnt = 64;
	int bd_mid_prev = 0;
	int bd_mid_next = 0;
	int bd_width = 3;
	int dly_sel_f = 0;
	int dly_sel_r = 0;
	int mg_lost_f = 0;	/* for falling edge margin compress */
	int mg_lost_r = 0;	/* for rising edge margin compress */
	unsigned int i, j;
	struct AUTOK_SCAN_RES *bd_info_temp[2] = {NULL};
	unsigned int pass_bd_size[BD_MAX_CNT + 1];
	unsigned int max_pass_loca = 0;
	unsigned int max_size = 0;
	unsigned int ret = 0;

	bd_info_r = &(info->scan_info[0]);
	bd_info_f = &(info->scan_info[1]);
	f_bound_cnt_r = bd_info_r->fbd_cnt;
	bound_cnt_r = bd_info_r->bd_cnt;
	bound_cnt_f = bd_info_f->bd_cnt;

	/*
	* for corner case
	* xxxxxxxxxxxxxxxxxxxx rising only has one boundary,but all fail
	* oooooooooxxooooooo falling has normal boundary
	* or
	* ooooooooooooxooooo rising has normal boundary
	* xxxxxxxxxxxxxxxxxxxx falling only has one boundary,but all fail
	*/
	if ((bd_info_r->bd_cnt == 1) && (bd_info_f->bd_cnt == 1)
		&& (bd_info_r->bd_info[0].Bound_Start == 0)
		&& (bd_info_r->bd_info[0].Bound_End == 63)
		&& (bd_info_f->bd_info[0].Bound_Start == 0)
		&& (bd_info_f->bd_info[0].Bound_End == 63)) {
		AUTOK_RAWPRINT("[ATUOK]Err: can not find bound both edge\n");
		return -1;
	}
	for (j = 0; j < 2; j++) {
		if (j == 0) {
			bd_info_temp[0] = bd_info_r;
			bd_info_temp[1] = bd_info_f;
		} else {
			bd_info_temp[0] = bd_info_f;
			bd_info_temp[1] = bd_info_r;
		}
		if ((bd_info_temp[0]->bd_cnt == 1)
			&& (bd_info_temp[0]->bd_info[0].Bound_Start == 0)
			&& (bd_info_temp[0]->bd_info[0].Bound_End == 63)) {
			if (j == 0)
				info->opt_edge_sel = 1;
			else
				info->opt_edge_sel = 0;
			/* can not know 1T size,need check max pass bound,
			 * select mid
			 */
			switch (bd_info_temp[1]->bd_cnt) {
			case 4:
				pass_bd_size[0] = bd_info_temp[1]->
					bd_info[0].Bound_Start - 0;
				pass_bd_size[1] = bd_info_temp[1]->
					bd_info[1].Bound_Start -
					bd_info_temp[1]->
					bd_info[0].Bound_End;
				pass_bd_size[2] = bd_info_temp[1]->
					bd_info[2].Bound_Start	-
					bd_info_temp[1]->
					bd_info[1].Bound_End;
				pass_bd_size[3] = bd_info_temp[1]->
					bd_info[3].Bound_Start -
					bd_info_temp[1]->
					bd_info[2].Bound_End;
				pass_bd_size[4] = 63 -
					bd_info_temp[1]->bd_info[3].Bound_End;
				max_size = pass_bd_size[0];
				max_pass_loca = 0;
				for (i = 0; i < 5; i++) {
					if (pass_bd_size[i] >= max_size) {
						max_size = pass_bd_size[i];
						max_pass_loca = i;
					}
				}
				if (max_pass_loca == 0)
					info->opt_dly_cnt = bd_info_temp[1]->
					bd_info[0].Bound_Start / 2;
				else if (max_pass_loca == 4)
					info->opt_dly_cnt = (63 +
					bd_info_temp[1]->
					bd_info[3].Bound_End) / 2;
				else {
					info->opt_dly_cnt = (bd_info_temp[1]->
					bd_info[max_pass_loca].Bound_Start +
					bd_info_temp[1]->
					bd_info[max_pass_loca - 1].Bound_End) /
					2;
				}
				break;
			case 3:
				pass_bd_size[0] = bd_info_temp[1]->
						bd_info[0].Bound_Start - 0;
				pass_bd_size[1] = bd_info_temp[1]->
						bd_info[1].Bound_Start
					- bd_info_temp[1]->bd_info[0].Bound_End;
				pass_bd_size[2] = bd_info_temp[1]->
						bd_info[2].Bound_Start
					- bd_info_temp[1]->bd_info[1].Bound_End;
				pass_bd_size[3] = 63 - bd_info_temp[1]->
						bd_info[2].Bound_End;
				max_size = pass_bd_size[0];
				max_pass_loca = 0;
				for (i = 0; i < 4; i++) {
					if (pass_bd_size[i] >= max_size) {
						max_size = pass_bd_size[i];
						max_pass_loca = i;
					}
				}
				if (max_pass_loca == 0)
					info->opt_dly_cnt =
					bd_info_temp[1]->
					bd_info[0].Bound_Start / 2;
				else if (max_pass_loca == 3)
					info->opt_dly_cnt = (63 +
					bd_info_temp[1]->
					bd_info[2].Bound_End) / 2;
				else {
					info->opt_dly_cnt = (bd_info_temp[1]->
					bd_info[max_pass_loca].Bound_Start
					+ bd_info_temp[1]->
					bd_info[max_pass_loca - 1].Bound_End) /
					2;
				}
				break;
			case 2:
				pass_bd_size[0] = bd_info_temp[1]->
						bd_info[0].Bound_Start - 0;
				pass_bd_size[1] = bd_info_temp[1]->
						bd_info[1].Bound_Start
					- bd_info_temp[1]->bd_info[0].Bound_End;
				pass_bd_size[2] = 63 - bd_info_temp[1]->
						bd_info[1].Bound_End;
				max_size = pass_bd_size[0];
				max_pass_loca = 0;
				for (i = 0; i < 3; i++) {
					if (pass_bd_size[i] >= max_size) {
						max_size = pass_bd_size[i];
						max_pass_loca = i;
					}
				}
				if (max_pass_loca == 0)
					info->opt_dly_cnt = bd_info_temp[1]->
					bd_info[0].Bound_Start / 2;
				else if (max_pass_loca == 2)
					info->opt_dly_cnt = (63 +
					bd_info_temp[1]->
					bd_info[1].Bound_End) / 2;
				else {
					info->opt_dly_cnt = (bd_info_temp[1]->
					bd_info[max_pass_loca].Bound_Start
					+ bd_info_temp[1]->
					bd_info[max_pass_loca - 1].Bound_End) /
					2;
				}
				break;
			case 1:
				pass_bd_size[0] = bd_info_temp[1]->
						bd_info[0].Bound_Start - 0;
				pass_bd_size[1] = 63 - bd_info_temp[1]->
						bd_info[0].Bound_End;
				max_size = pass_bd_size[0];
				max_pass_loca = 0;
				for (i = 0; i < 2; i++) {
					if (pass_bd_size[i] >= max_size) {
						max_size = pass_bd_size[i];
						max_pass_loca = i;
					}
				}
				if (max_pass_loca == 0)
					info->opt_dly_cnt = bd_info_temp[1]->
					bd_info[0].Bound_Start / 2;
				else
					info->opt_dly_cnt =
					(63 + bd_info_temp[1]->
					bd_info[0].Bound_End) /
					2;
				break;
			case 0:
				info->opt_dly_cnt = 31;
				break;
			default:
				break;
			}
			return ret;
		}
	}

	switch (f_bound_cnt_r) {
	case 4:		/* SSSS Corner may cover 2~3T */
	case 3:
		AUTOK_RAWPRINT
		    ("[ATUOK]Warning: Too Many Full boundary count:%d\r\n",
		     f_bound_cnt_r);
	case 2:		/* mode_1 : 2 full boudary */
		for (i = 0; i < BD_MAX_CNT; i++) {
			if (bd_info_r->bd_info[i].is_fullbound) {
				if (bd_prev == NULL) {
					bd_prev = &(bd_info_r->bd_info[i]);
				} else {
					bd_next = &(bd_info_r->bd_info[i]);
					break;
				}
			}
		}

		if (bd_prev && bd_next) {
			bd_mid_prev =
			    (bd_prev->Bound_Start + bd_prev->Bound_End) / 2;
			bd_mid_next =
			    (bd_next->Bound_Start + bd_next->Bound_End) / 2;
			/* while in 2 full bound case, bd_width calc */
			bd_width =
			    (bd_prev->Bound_width + bd_next->Bound_width) / 2;
			cycle_cnt = bd_mid_next - bd_mid_prev;
			/* delay count sel at rising edge */
			if (bd_mid_prev >= cycle_cnt / 2) {
				dly_sel_r = bd_mid_prev - cycle_cnt / 2;
				mg_lost_r = 0;
			} else if ((cycle_cnt / 2 - bd_mid_prev) >
				   AUTOK_MARGIN_THOLD) {
				dly_sel_r = bd_mid_prev + cycle_cnt / 2;
				mg_lost_r = 0;
			} else {
				dly_sel_r = 0;
				mg_lost_r = cycle_cnt / 2 - bd_mid_prev;
			}
			/* delay count sel at falling edge */
			bd_tmp = &(bd_info_r->bd_info[0]);
			if (bd_tmp->is_fullbound) {
				/* ooooxxxooooooxxxooo */
				dly_sel_f = bd_mid_prev;
				mg_lost_f = 0;
			} else {
				/* xooooooxxxoooooooxxxoo */
				if (bd_tmp->Bound_End > bd_width / 2) {
					dly_sel_f =
					    (bd_tmp->Bound_End) -
					    (bd_width / 2);
					mg_lost_f = 0;
				} else {
					dly_sel_f = 0;
					mg_lost_f =
					    (bd_width / 2) -
					    (bd_tmp->Bound_End);
				}
			}
		} else {
			/* error can not find 2 foull boary */
			AUTOK_RAWPRINT
			    ("err can not find 2 foull boudary @ Mode_1");
			return -1;
		}
		break;

	case 1:		/* rising edge find one full boundary */
		if (bound_cnt_r > 1) {
			/* mode_2: 1 full boundary and boundary count > 1 */
			bd_prev = &(bd_info_r->bd_info[0]);
			bd_next = &(bd_info_r->bd_info[1]);

			if (bd_prev->is_fullbound)
				bd_width = bd_prev->Bound_width;
			else
				bd_width = bd_next->Bound_width;

			if ((bd_prev->is_fullbound) ||
				(bd_next->is_fullbound)) {
				if (bd_prev->Bound_Start > 0)
					cycle_cnt =
					    bd_next->Bound_Start -
					    bd_prev->Bound_Start;
				else
					cycle_cnt =
					    bd_next->Bound_End -
					    bd_prev->Bound_End;

				/* delay count sel@rising & falling edge */
				if (bd_prev->is_fullbound) {
					bd_mid_prev =
					    (bd_prev->Bound_Start +
					     bd_prev->Bound_End) / 2;
					dly_sel_f = bd_mid_prev;
					mg_lost_f = 0;
					if (bd_mid_prev >= cycle_cnt / 2) {
						dly_sel_r =
						    bd_mid_prev -
						    cycle_cnt / 2;
						mg_lost_r = 0;
					} else
					    if ((cycle_cnt / 2 - bd_mid_prev) >
						AUTOK_MARGIN_THOLD) {
						dly_sel_r =
						    bd_mid_prev +
						    cycle_cnt / 2;
						mg_lost_r = 0;
					} else {
						dly_sel_r = 0;
						mg_lost_r =
						    cycle_cnt / 2 -
						    bd_mid_prev;
					}
				} else {
					/* first boundary not full boudary */
					bd_mid_next =
					    (bd_next->Bound_Start +
					     bd_next->Bound_End) / 2;
					dly_sel_r =
					    bd_mid_next - cycle_cnt / 2;
					mg_lost_r = 0;
					if (bd_prev->Bound_End >
						bd_width / 2) {
						dly_sel_f =
						    (bd_prev->Bound_End) -
						    (bd_width / 2);
						mg_lost_f = 0;
					} else {
						dly_sel_f = 0;
						mg_lost_f =
						    (bd_width / 2) -
						    (bd_prev->Bound_End);
					}
				}
			} else {
				return -1;
			}
		} else if (bound_cnt_f > 0) {
			/* mode_3: 1 full boundary and only one boundary exist
			 * @rising edge
			 */
			bd_prev = &(bd_info_r->bd_info[0]);
			bd_next = &(bd_info_f->bd_info[0]);
			bd_mid_prev =
			    (bd_prev->Bound_Start + bd_prev->Bound_End) / 2;
			bd_width = bd_prev->Bound_width;

			if (bd_next->Bound_Start == 0) {
				cycle_cnt =
				    (bd_prev->Bound_End -
				     bd_next->Bound_End) * 2;
			} else if (bd_next->Bound_End == 63) {
				cycle_cnt =
				    (bd_next->Bound_Start -
				     bd_prev->Bound_Start) * 2;
			} else {
				bd_mid_next =
				    (bd_next->Bound_Start +
				     bd_next->Bound_End) / 2;

				if (bd_mid_next > bd_mid_prev)
					cycle_cnt =
					    (bd_mid_next - bd_mid_prev) * 2;
				else
					cycle_cnt =
					    (bd_mid_prev - bd_mid_next) * 2;
			}

			dly_sel_f = bd_mid_prev;
			mg_lost_f = 0;

			if (bd_mid_prev >= cycle_cnt / 2) {
				dly_sel_r = bd_mid_prev - cycle_cnt / 2;
				mg_lost_r = 0;
			} else if (cycle_cnt / 2 - bd_mid_prev
				<= AUTOK_MARGIN_THOLD) {
				dly_sel_r = 0;
				mg_lost_r = cycle_cnt / 2 - bd_mid_prev;
			} else if (cycle_cnt / 2 + bd_mid_prev <= 63) {
				dly_sel_r = cycle_cnt / 2 + bd_mid_prev;
				mg_lost_r = 0;
			} else if (32 - bd_mid_prev <= AUTOK_MARGIN_THOLD) {
				dly_sel_r = 0;
				mg_lost_r = cycle_cnt / 2 - bd_mid_prev;
			} else {	/* case 5 */
				dly_sel_r = 63;
				mg_lost_r = bd_mid_prev + cycle_cnt / 2 - 63;
			}
		} else {
			/* mode_4: falling edge no boundary found &
			 * rising edge only one full boundary exist
			 */
			bd_prev = &(bd_info_r->bd_info[0]);
			bd_mid_prev =
			    (bd_prev->Bound_Start + bd_prev->Bound_End) / 2;
			bd_width = bd_prev->Bound_width;

			if (bd_prev->Bound_End > (64 - bd_prev->Bound_Start))
				cycle_cnt = 2 * (bd_prev->Bound_End + 1);
			else
				cycle_cnt = 2 * (64 - bd_prev->Bound_Start);

			dly_sel_r = 0xFF;
			mg_lost_r = 0xFF;
			dly_sel_f = bd_mid_prev;
			mg_lost_f = 0xFF;

			AUTOK_RAWPRINT("[AUTOK]Warning: 1T > %d\r\n",
				       cycle_cnt);
		}
		break;

	case 0:		/* rising edge cannot find full boudary */
		if (bound_cnt_r == 2) {
			bd_prev = &(bd_info_r->bd_info[0]);
			bd_next = &(bd_info_f->bd_info[0]);

			if (bd_next->is_fullbound) {
				/* mode_5: rising_edge 2 boundary
				 * (not full bound), falling edge 1
				 * full boundary
				 */
				bd_width = bd_next->Bound_width;
				cycle_cnt =
				    2 * (bd_next->Bound_End -
					 bd_prev->Bound_End);
				bd_mid_next =
				    (bd_next->Bound_Start +
				     bd_next->Bound_End) / 2;
				dly_sel_r = bd_mid_next;
				mg_lost_r = 0;
				if (bd_prev->Bound_End >= bd_width / 2) {
					dly_sel_f =
					    bd_prev->Bound_End - bd_width / 2;
					mg_lost_f = 0;
				} else {
					dly_sel_f = 0;
					mg_lost_f =
					    bd_width / 2 - bd_prev->Bound_End;
				}
			} else {
				/* for falling edge there must be one full
				 * boundary between two
				 * bounary_mid at rising.This is a corner case,
				 * falling boundary
				 * may  scan miss.
				 * xoooooooooooooooox or xoooooooooooooooox  or
				 * xoooooooooooooooox oooooooooooooooooo
				 * xxoooooooooooooooo      ooooooooooooooooox
				 */
				info->cycle_cnt = bd_info_r->
					bd_info[1].Bound_End
					- bd_info_r->bd_info[0].Bound_Start;
				if (bound_cnt_f == 0) {
					info->opt_edge_sel = 1;
					info->opt_dly_cnt = 0;
				} else {
					info->opt_edge_sel = 0;
					info->opt_dly_cnt =
						(bd_info_r->
						bd_info[1].Bound_End
						+ bd_info_r->
						bd_info[0].Bound_Start) / 2;
				}
				return ret;
			}
		} else if (bound_cnt_r == 1) {
			if (bound_cnt_f > 1) {
				/* when rising_edge have only one boundary
				 * (not full bound),
				 * falling edge should not more than 1Bound
				 * exist.
				 * this is a corner case, rising boundary may
				 * scan miss.
				 * xooooooooooooooooo
				 * oooxooooooooxooooo
				 */
				info->cycle_cnt =
				(bd_info_f->bd_info[1].Bound_End
				+ bd_info_f->bd_info[1].Bound_Start) / 2
				- (bd_info_f->bd_info[0].Bound_End
				+ bd_info_f->bd_info[0].Bound_Start) / 2;
				info->opt_edge_sel = 1;
				info->opt_dly_cnt = ((bd_info_f->
				bd_info[1].Bound_End
				+ bd_info_f->bd_info[1].Bound_Start) / 2
				+ (bd_info_f->bd_info[0].Bound_End
				+ bd_info_f->bd_info[0].Bound_Start) / 2) / 2;
				return ret;
			} else if (bound_cnt_f == 1) {
				/* mode_6:rising edge only 1 boundary (not full
				 * Bound)
				 * & falling edge have only 1 bound too
				 */
				bd_prev = &(bd_info_r->bd_info[0]);
				bd_next = &(bd_info_f->bd_info[0]);
				if (bd_next->is_fullbound) {
					bd_width = bd_next->Bound_width;
				} else {
					if (bd_next->Bound_width >
					    bd_prev->Bound_width)
						bd_width =
						    (bd_next->Bound_width + 1);
					else
						bd_width =
						    (bd_prev->Bound_width + 1);

					if (bd_width < AUTOK_BD_WIDTH_REF)
						bd_width = AUTOK_BD_WIDTH_REF;
				}	/* Boundary width calc done */

				if (bd_prev->Bound_Start == 0) {
					/* Current Desing Not Allowed */
					if (bd_next->Bound_Start == 0) {
						/* Current Desing Not Allowed
						 * this is a corner case,
						 *boundary may scan error.
						 * xooooooooooooooooo
						 * xooooooooooooooooo
						 */
						info->cycle_cnt =
						2 * (64 - (bd_info_r->
						bd_info[0].Bound_End
						+ bd_info_r->
						bd_info[0].Bound_Start) / 2);
						info->opt_edge_sel = 0;
						info->opt_dly_cnt = 31;
						return ret;
					}

					cycle_cnt =
					    (bd_next->Bound_Start -
					     bd_prev->Bound_End +
					     bd_width) * 2;
				} else if (bd_prev->Bound_End == 63) {
					/* Current Desing Not Allowed */
					if (bd_next->Bound_End == 63) {
						/* Current Desing Not Allowed
						* this is a corner case,
						* boundary may scan error.
						* ooooooooooooooooox
						* ooooooooooooooooox
						*/
						info->cycle_cnt =
						bd_info_r->bd_info[0].Bound_End
						+ bd_info_r->
						bd_info[0].Bound_Start;
						info->opt_edge_sel = 0;
						info->opt_dly_cnt = 31;
						return ret;
					}

					cycle_cnt =
					    (bd_prev->Bound_Start -
					     bd_next->Bound_End +
					     bd_width) * 2;
				}

				/* cycle count calc done */
				/* calc optimise delay count */
				if (bd_prev->Bound_Start == 0) {
					/* falling edge sel */
					if (bd_prev->Bound_End >=
					bd_width / 2) {
						dly_sel_f =
						    bd_prev->Bound_End -
						    bd_width / 2;
						mg_lost_f = 0;
					} else {
						dly_sel_f = 0;
						mg_lost_f =
						    bd_width / 2 -
						    bd_prev->Bound_End;
					}

					/* rising edge sel */
					if (bd_prev->Bound_End - bd_width / 2 +
					    cycle_cnt / 2 > 63) {
						dly_sel_r = 63;
						mg_lost_r =
						    bd_prev->Bound_End -
						    bd_width / 2 +
						    cycle_cnt / 2 - 63;
					} else {
						dly_sel_r =
						    bd_prev->Bound_End -
						    bd_width / 2 +
						    cycle_cnt / 2;
						mg_lost_r = 0;
					}
				} else if (bd_prev->Bound_End == 63) {
					/* falling edge sel */
					if (bd_prev->Bound_Start +
					    bd_width / 2 < 63) {
						dly_sel_f =
						    bd_prev->Bound_Start +
						    bd_width / 2;
						mg_lost_f = 0;
					} else {
						dly_sel_f = 63;
						mg_lost_f =
						    bd_prev->Bound_Start +
						    bd_width / 2 - 63;
					}

					/* rising edge sel */
					if ((int)(bd_prev->Bound_Start +
					    bd_width / 2 - cycle_cnt / 2) <
					    0) {
						dly_sel_r = 0;
						mg_lost_r =
						    cycle_cnt / 2 -
						    (bd_prev->Bound_Start +
						     bd_width / 2);
					} else {
						dly_sel_r =
						    bd_prev->Bound_Start +
						    bd_width / 2 -
						    cycle_cnt / 2;
						mg_lost_r = 0;
					}
				} else {
					return -1;
				}
			} else if (bound_cnt_f == 0) {
				/* mode_7: rising edge only one bound (not
				 * full), falling no boundary
				 */
				cycle_cnt = 128;
				bd_prev = &(bd_info_r->bd_info[0]);
				if (bd_prev->Bound_Start == 0) {
					dly_sel_f = 0;
					dly_sel_r = 63;
				} else if (bd_prev->Bound_End == 63) {
					dly_sel_f = 63;
					dly_sel_r = 0xFF;
				} else {
					return -1;
				}
				mg_lost_f = 0xFF;
				mg_lost_r = 0xFF;

				AUTOK_RAWPRINT("[AUTOK]Warning: 1T > %d\r\n",
					       cycle_cnt);
			}
		} else if (bound_cnt_r == 0) {
			if (bound_cnt_f > 1) {
				/* falling edge not allowed two boundary Exist
				 * for this case
				 * this is a corner case,rising boundary may
				 * scan miss.
				 * oooooooooooooooooo
				 * oooxooooooooxooooo
				 */
				info->cycle_cnt =
				(bd_info_f->bd_info[1].Bound_End
				+ bd_info_f->bd_info[1].Bound_Start) / 2
				- (bd_info_f->bd_info[0].Bound_End
				+ bd_info_f->bd_info[0].Bound_Start) / 2;
				info->opt_edge_sel = 0;
				info->opt_dly_cnt =
				(bd_info_f->bd_info[0].Bound_End
				+ bd_info_f->bd_info[0].Bound_Start) / 2;
				return ret;
			} else if (bound_cnt_f > 0) {
				/* mode_8: falling edge have one
				 * Boundary exist
				 */
				bd_prev = &(bd_info_f->bd_info[0]);

				/* this boundary is full bound */
				if (bd_prev->is_fullbound) {
					bd_mid_prev =
					    (bd_prev->Bound_Start +
					     bd_prev->Bound_End) / 2;

					if (bd_prev->Bound_End >
					    (64 - bd_prev->Bound_Start))
						cycle_cnt =
						    2 * (bd_prev->Bound_End +
							 1);
					else
						cycle_cnt =
						    2 * (64 -
							 bd_prev->Bound_Start);

					dly_sel_r = bd_mid_prev;
					mg_lost_r = 0xFF;
					dly_sel_f = 0xFF;
					mg_lost_f = 0xFF;
				} else {
					cycle_cnt = 128;

					dly_sel_r =
					    (bd_prev->Bound_Start ==
					     0) ? 0 : 63;
					mg_lost_r = 0xFF;
					dly_sel_f = 0xFF;
					mg_lost_f = 0xFF;
				}

				pr_info("[AUTOK]Warning: 1T > %d\n",
					cycle_cnt);
			} else {
				/* falling edge no boundary exist no
				 *  need tuning
				 */
				cycle_cnt = 128;
				dly_sel_f = 0;
				mg_lost_f = 0xFF;
				dly_sel_r = 0;
				mg_lost_r = 0xFF;
				pr_info("[AUTOK]Warning: 1T > %d\n",
					cycle_cnt);
			}
		} else {
			/* Error if bound_cnt > 3 there must be at least one
			 * full boundary exist
			 */
			return -1;
		}
		break;

	default:
		/* warning if boundary count > 4 (from current hw design, this
		 * case cannot happen)
		 */
		return -1;
	}

	/* Select Optimised Sample edge & delay count (the small one) */
	info->cycle_cnt = cycle_cnt;
	if (dly_sel_r <= dly_sel_f) {
		info->opt_edge_sel = 0;
		info->opt_dly_cnt = dly_sel_r;
	} else {
		info->opt_edge_sel = 1;
		info->opt_dly_cnt = dly_sel_f;
	}
	AUTOK_RAWPRINT("[AUTOK]Analysis Result: 1T = %d\r\n", cycle_cnt);
	return ret;
}

#if SINGLE_EDGE_ONLINE_TUNE
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Software flow to calculate optimal delay for single edge
 * @param *info: information for edge sel and optimal cnt
 * @param cycle_cnt_ref: information for cycle cnt
 * @param *dly_sel: optimal delay setting
 * @return
 *     If 0, find optimal delay setting. \n
 *     If -1, optimal delay setting search fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int
autok_pad_dly_sel_single_edge(struct AUTOK_SCAN_RES *info,
			      unsigned int cycle_cnt_ref,
			      unsigned int *dly_sel)
{
	struct BOUND_INFO *bd_prev = NULL;
	struct BOUND_INFO *bd_next = NULL;
	unsigned int bound_cnt = 0;
	unsigned int bd_mid_prev = 0;
	int dly_sel = 0;
	int mg_lost = 0;
	unsigned int ret = 0;

	bound_cnt = info->bd_cnt;
	if (bound_cnt > 1) {
		bd_prev = &(info->bd_info[0]);
		bd_next = &(info->bd_info[1]);
		if (!(bd_prev->is_fullbound)) {
			/* mode_1: at least 2 Bound and Boud0_Start == 0 */
			dly_sel =
			    (bd_prev->Bound_End + bd_next->Bound_Start) / 2;
			mg_lost = (dly_sel > 31) ? (dly_sel - 31) : 0;
			dly_sel = (dly_sel > 31) ? 31 : dly_sel;

		} else {
			/* mode_2: at least 2 Bound found and
			 * Bound0_Start != 0
			 */
			bd_mid_prev =
			    (bd_prev->Bound_Start + bd_prev->Bound_End) / 2;
			if (bd_mid_prev >= cycle_cnt_ref / 2) {
				dly_sel = bd_mid_prev - cycle_cnt_ref / 2;
				mg_lost = 0;
			} else if (cycle_cnt_ref / 2 - bd_mid_prev <
				   AUTOK_MARGIN_THOLD) {
				dly_sel = 0;
				mg_lost = cycle_cnt_ref / 2 - bd_mid_prev;
			} else {
				dly_sel =
				    (bd_prev->Bound_End +
				     bd_next->Bound_Start) / 2;
				if ((dly_sel > 31)
				    && (dly_sel - 31 < AUTOK_MARGIN_THOLD)) {
					dly_sel = 31;
					mg_lost = dly_sel - 31;
				} else {
					/* dly_sel = dly_sel; */
					mg_lost = 0;
				}
			}
		}
	} else if (bound_cnt > 0) {
		/* only one bound fond */
		bd_prev = &(info->bd_info[0]);
		if (bd_prev->is_fullbound) {
			/* mode_3: Bound_S != 0 */
			bd_mid_prev =
			    (bd_prev->Bound_Start + bd_prev->Bound_End) / 2;
			if (bd_mid_prev >= cycle_cnt_ref / 2) {
				dly_sel = bd_mid_prev - cycle_cnt_ref / 2;
				mg_lost = 0;
			} else if (cycle_cnt_ref / 2 - bd_mid_prev <
				   AUTOK_MARGIN_THOLD) {
				dly_sel = 0;
				mg_lost = cycle_cnt_ref / 2 - bd_mid_prev;
			} else if ((bd_mid_prev > 31 - AUTOK_MARGIN_THOLD)
				   || (bd_prev->Bound_Start >= 16)) {
				dly_sel = 0;
				mg_lost = cycle_cnt_ref / 2 - bd_mid_prev;
			} else if (bd_mid_prev + cycle_cnt_ref / 2 <= 63) {
				/* Left Margin not enough must need to
				 * select the right side
				 */
				dly_sel = bd_mid_prev + cycle_cnt_ref / 2;
				mg_lost = 0;
			} else {
				dly_sel = 63;
				mg_lost = bd_mid_prev + cycle_cnt_ref / 2 - 63;
			}
		} else if (bd_prev->Bound_Start == 0) {
			/* mode_4 : Only one Boud and Boud_S = 0  (Currently 1T
			 * nearly equal 64 )
			 */

			/* May not exactly by for Cycle_Cnt enough can don't
			 * care
			 */
			bd_mid_prev =
			    (bd_prev->Bound_Start + bd_prev->Bound_End) / 2;
			if (bd_prev->Bound_Start + cycle_cnt_ref / 2 >= 31) {
				dly_sel = 31;
				mg_lost = bd_mid_prev + cycle_cnt_ref / 2 - 31;
			} else {
				dly_sel = bd_mid_prev + cycle_cnt_ref / 2;
				mg_lost = 0;
			}
		} else {
			/* mode_5: Only one Boud and Boud_E = 64 */

			/* May not exactly by for Cycle_Cnt enough can don't
			 * care
			 */
			bd_mid_prev =
			    (bd_prev->Bound_Start + bd_prev->Bound_End) / 2;
			if (bd_prev->Bound_Start < cycle_cnt_ref / 2) {
				dly_sel = 0;
				mg_lost = cycle_cnt_ref / 2 - bd_mid_prev;
			} else if (bd_mid_prev - cycle_cnt_ref / 2 > 31) {
				dly_sel = 31;
				mg_lost = bd_mid_prev - cycle_cnt_ref / 2 - 31;
			} else {
				dly_sel = bd_mid_prev - cycle_cnt_ref / 2;
				mg_lost = 0;
			}
		}
	} else {		/*mode_6: no bound foud */
		dly_sel = 31;
		mg_lost = 0xFF;
	}
	*dly_sel = dly_sel;
	if (dly_sel > 31) {
		AUTOK_RAWPRINT
		("Warning Dly Sel %d > 31 effected by Voltage Swing\r\n",
		     dly_sel);
	}

	return ret;
}
#endif

#if !defined(CONFIG_MACH_FPGA)
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function of calculating optimal delay for DS signal
 * @param *info: information for edge sel and optimal cnt
 * @param *dly_sel: optimal delay setting
 * @return
 *     If 0, find optimal delay setting. \n
 *     If -1, optimal delay setting search fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_ds_dly_sel(struct AUTOK_SCAN_RES_NEW *info,
			    unsigned int *dly_sel)
{
	int delay_sel = 0;
	unsigned int max_pass_win;
	unsigned char max_pass_win_position;
	unsigned char i;

	max_pass_win_position = 0;
	max_pass_win = info->pass_bd_info[0].Bound_End
		- info->pass_bd_info[0].Bound_Start;
	for (i = 0; i < info->pass_bd_cnt; i++)
		if ((info->pass_bd_info[i].Bound_End
			- info->pass_bd_info[i].Bound_Start) > max_pass_win) {
			max_pass_win = info->pass_bd_info[i].Bound_End
				- info->pass_bd_info[i].Bound_Start;
			max_pass_win_position = i;
		}
	delay_sel =
		(info->pass_bd_info[max_pass_win_position].Bound_Start
		+ info->pass_bd_info[max_pass_win_position].Bound_End) / 2;
	*dly_sel = delay_sel;
	return max_pass_win;
}
#endif

/*************************************************************************
* FUNCTION
*  msdc_autok_adjust_param
*
* DESCRIPTION
*  This function for auto-K, adjust msdc parameter
*
* PARAMETERS
*    host: msdc host manipulator pointer
*    param: enum of msdc parameter
*    value: value of msdc parameter
*    rw: AUTOK_READ/AUTOK_WRITE
*
* RETURN VALUES
*    If 0, success. \n
*    If -1, parameter input error
*************************************************************************/
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for identifying input autok parameter if OK
 * @param *host: msdc host manipulator pointer
 * @param param: enum of msdc parameter
 * @param *value: value of msdc parameter
 * @param rw: AUTOK_READ/AUTOK_WRITE
 * @return
 *     If 0, success. \n
 *     If -1, parameter input error
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int msdc_autok_adjust_param(struct msdc_host *host,
				   enum AUTOK_PARAM param, u32 *value, int rw)
{
	void __iomem *base = host->base;
#if !defined(CONFIG_MACH_FPGA)
	void __iomem *base_top = host->base_top;
#endif
	uint32_t *reg;
	u32 field = 0;

	switch (param) {
	case READ_DATA_SMPL_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_R_D_SMPL_SEL);
		break;
	case WRITE_DATA_SMPL_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_W_D_SMPL_SEL);
		break;
	case DATA_DLYLINE_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CONTROL;
		field = (u32) (DATA_K_VALUE_SEL);
#else
		return 0;
#endif
		break;
	case MSDC_DAT_TUNE_SEL:	/* 0-Dat tune 1-CLk tune ; */
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CONTROL;
		field = (u32) (PAD_RXDLY_SEL);
#else
		return 0;
#endif
		break;
	case MSDC_WCRC_ASYNC_FIFO_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT2;
		field = (u32) (MSDC_PB2_CFGCRCSTS);
		break;
	case MSDC_RESP_ASYNC_FIFO_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT2;
		field = (u32) (MSDC_PB2_CFGRESP);
		break;
	case CMD_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_RSPL);
		break;
	case RDATA_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_R_D_SMPL);
		break;
	case RD_FIFO_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT0;
		field = (u32) (MSDC_PB0_RD_DAT_SEL);
		break;
	case WD_FIFO_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT2;
		field = (u32) (MSDC_PB2_CFGCRCSTSEDGE);
		break;
	case CMD_RD_D_DLY1:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CMD;
		field = (u32) (PAD_CMD_RXDLY);
#else
		return 0;
#endif
		break;
	case CMD_RD_D_DLY1_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CMD;
		field = (u32) (PAD_CMD_RD_RXDLY_SEL);
#else
		return 0;
#endif
		break;
	case CMD_RD_D_DLY2:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CMD;
		field = (u32) (PAD_CMD_RXDLY2);
#else
		return 0;
#endif
		break;
	case CMD_RD_D_DLY2_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CMD;
		field = (u32) (PAD_CMD_RD_RXDLY2_SEL);
#else
		return 0;
#endif
		break;
	case DAT_RD_D_DLY1:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CONTROL;
		field = (u32) (PAD_DAT_RD_RXDLY);
#else
		return 0;
#endif
		break;
	case DAT_RD_D_DLY1_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CONTROL;
		field = (u32) (PAD_DAT_RD_RXDLY_SEL);
#else
		return 0;
#endif
		break;
	case DAT_RD_D_DLY2:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CONTROL;
		field = (u32) (PAD_DAT_RD_RXDLY2);
#else
		return 0;
#endif
		break;
	case DAT_RD_D_DLY2_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CONTROL;
		field = (u32) (PAD_DAT_RD_RXDLY2_SEL);
#else
		return 0;
#endif
		break;
	case INT_DAT_LATCH_CK:
		if ((rw == AUTOK_WRITE) && (*value > 7))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT0;
		field = (u32) (MSDC_PB0_INT_DAT_LATCH_CK_SEL);
		break;
	case CKGEN_MSDC_DLY_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT0;
		field = (u32) (MSDC_PB0_CKGEN_MSDC_DLY_SEL);
		break;
	case CMD_RSP_TA_CNTR:
		if ((rw == AUTOK_WRITE) && (*value > 7))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT1;
		field = (u32) (MSDC_PB1_CMD_RSP_TA_CNTR);
		break;
	case WRDAT_CRCS_TA_CNTR:
		if ((rw == AUTOK_WRITE) && (*value > 7))
			return -1;
		reg = (u32 *) MSDC_PATCH_BIT1;
		field = (u32) (MSDC_PB1_WRDAT_CRCS_TA_CNTR);
		break;
	case PAD_CLK_TXDLY_TUNE:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_CTL0;
		field = (u32) (PAD_CLK_TXDLY);
#else
		return 0;
#endif
		break;
	case EMMC50_WDATA_MUX_EN:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) EMMC50_CFG0;
		field = (u32) (MSDC_EMMC50_CFG_CRC_STS_SEL);
		break;
	case EMMC50_CMD_MUX_EN:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) EMMC50_CFG0;
		field = (u32) (MSDC_EMMC50_CFG_CMD_RESP_SEL);
		break;
	case EMMC50_WDATA_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
		reg = (u32 *) EMMC50_CFG0;
		field = (u32) (MSDC_EMMC50_CFG_CRC_STS_EDGE);
		break;
	case EMMC50_DS_Z_DLY1:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DS_TUNE;
		field = (u32) (PAD_DS_DLY1);
#else
		return 0;
#endif
		break;
	case EMMC50_DS_Z_DLY1_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DS_TUNE;
		field = (u32) (PAD_DS_DLY_SEL);
#else
		return 0;
#endif
		break;
	case EMMC50_DS_Z_DLY2:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DS_TUNE;
		field = (u32) (PAD_DS_DLY2);
#else
		return 0;
#endif
		break;
	case EMMC50_DS_Z_DLY2_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DS_TUNE;
		field = (u32) (PAD_DS_DLY2_SEL);
#else
		return 0;
#endif
		break;
	case EMMC50_DS_ZDLY_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DS_TUNE;
		field = (u32) (PAD_DS_DLY3);
#else
		return 0;
#endif
		break;
	case EMMC50_CMD_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) EMMC_TOP_CMD;
		field = (u32) (PAD_CMD_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT0_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT0_TUNE;
		field = (u32) (PAD_DAT0_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT1_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT1_TUNE;
		field = (u32) (PAD_DAT1_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT2_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT2_TUNE;
		field = (u32) (PAD_DAT2_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT3_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT3_TUNE;
		field = (u32) (PAD_DAT3_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT4_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT4_TUNE;
		field = (u32) (PAD_DAT4_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT5_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT5_TUNE;
		field = (u32) (PAD_DAT5_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT6_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT6_TUNE;
		field = (u32) (PAD_DAT6_TX_DLY);
#else
		return 0;
#endif
		break;
	case EMMC50_DAT7_TX_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31))
			return -1;
#if !defined(CONFIG_MACH_FPGA)
		reg = (u32 *) TOP_EMMC50_PAD_DAT7_TUNE;
		field = (u32) (PAD_DAT7_TX_DLY);
#else
		return 0;
#endif
		break;
	default:
		pr_debug("[%s] Value of [enum AUTOK_PARAM param] is wrong\n",
			 __func__);
		return -1;
	}

	if (rw == AUTOK_READ)
		MSDC_GET_FIELD(reg, field, *value);
	else if (rw == AUTOK_WRITE) {
		MSDC_SET_FIELD(reg, field, *value);

		if (param == CKGEN_MSDC_DLY_SEL)
			mdelay(1);
	} else {
		pr_debug("[%s] Value of [int rw] is wrong\n", __func__);
		return -1;
	}

	return 0;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Function for check tune result if meet expected range for each parameter
 * @param param_id: enum of msdc parameter
 * @param result: value of msdc parameter
 * @param *autok_tune_res: optimal setting for each parameter
 * @return
 *     If 0, success. \n
 *     If -1, parameter over range
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_param_update(enum AUTOK_PARAM param_id, unsigned int result,
			      u8 *autok_tune_res)
{
	if (param_id < TUNING_PARAM_COUNT) {
		if ((result > autok_param_info[param_id].range.end) ||
		    (result < autok_param_info[param_id].range.start)) {
			return -1;
		}
		autok_tune_res[param_id] = (u8) result;
		return 0;
	}
	AUTOK_RAWPRINT("[AUTOK]param not found\r\n");

	return -1;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Apply optiaml delay to hardware
 * @param *host: msdc host manipulator pointer
 * @param *autok_tune_res: autok tune result for each parameter
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_param_apply(struct msdc_host *host, u8 *autok_tune_res)
{
	unsigned int i = 0;
	unsigned int value = 0;

	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		value = (u8) autok_tune_res[i];
		msdc_autok_adjust_param(host, i, &value, AUTOK_WRITE);
	}

	return 0;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Dump optiaml delay for each parameter
 * @param *host: msdc host manipulator pointer
 * @param *autok_tune_res: autok tune result for each parameter
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_result_dump(struct msdc_host *host, u8 *autok_tune_res)
{
	AUTOK_RAWPRINT("[AUTOK]CMD [EDGE:%d DLY1:%d DLY2:%d ]\r\n",
		       autok_tune_res[0], autok_tune_res[4], autok_tune_res[6]);
	AUTOK_RAWPRINT
	    ("[AUTOK]DAT [RDAT_EDGE:%d RD_FIFO_EDGE:%d WD_FIFO_EDGE:%d]\r\n",
	     autok_tune_res[1], autok_tune_res[2], autok_tune_res[3]);
	AUTOK_RAWPRINT("[AUTOK]DAT [LATCH_CK:%d DLY1:%d DLY2:%d]\r\n",
		       autok_tune_res[12], autok_tune_res[8],
		       autok_tune_res[10]);
	AUTOK_RAWPRINT("[AUTOK]DS  [DLY1:%d DLY2:%d DLY3:%d]\r\n",
		       autok_tune_res[13], autok_tune_res[15],
		       autok_tune_res[17]);
	AUTOK_RAWPRINT("[AUTOK]CT  [TX:%d]\r\n", autok_tune_res[18]);
	AUTOK_RAWPRINT("[AUTOK]DT  [D0:%d D1:%d D2:%d D3:%d]\r\n",
		       autok_tune_res[19], autok_tune_res[20],
		       autok_tune_res[21], autok_tune_res[22]);
	AUTOK_RAWPRINT("[AUTOK]DT  [D4:%d D5:%d D6:%d D7:%d]\r\n",
		       autok_tune_res[23], autok_tune_res[24],
		       autok_tune_res[25], autok_tune_res[26]);

	return 0;
}

#if AUTOK_PARAM_DUMP_ENABLE
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Dump optiaml delay for each parameter
 * @param *host: msdc host manipulator pointer
 * @return
 *     Always 0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_register_dump(struct msdc_host *host)
{
	unsigned int i = 0;
	unsigned int value = 0;
	u8 autok_tune_res[TUNING_PARAM_COUNT];

	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		msdc_autok_adjust_param(host, i, &value, AUTOK_READ);
		autok_tune_res[i] = value;
	}
	pr_info("[AUTOK]CMD [EDGE:%d DLY1:%d DLY2:%d ]\r\n",
	       autok_tune_res[0], autok_tune_res[4], autok_tune_res[6]);
	pr_info("[AUTOK]DAT [RDAT_EDGE:%d RD_FIFO_EDGE:%d WD_FIFO_EDGE:%d]\n",
		autok_tune_res[1], autok_tune_res[2], autok_tune_res[3]);
	pr_info("[AUTOK]DAT [LATCH_CK:%d DLY1:%d DLY2:%d]\r\n",
		autok_tune_res[12], autok_tune_res[8], autok_tune_res[10]);
	pr_info("[AUTOK]DS  [DLY1:%d DLY2:%d DLY3:%d]\r\n",
		autok_tune_res[13], autok_tune_res[15], autok_tune_res[17]);
	pr_info("[AUTOK]CT  [TX:%d]\r\n", autok_tune_res[18]);
	pr_info("[AUTOK]DT  [D0:%d D1:%d D2:%d D3:%d]\r\n",
		autok_tune_res[19], autok_tune_res[20],
		autok_tune_res[21], autok_tune_res[22]);
	pr_info("[AUTOK]DT  [D4:%d D5:%d D6:%d D7:%d]\r\n",
		autok_tune_res[23], autok_tune_res[24],
		autok_tune_res[25], autok_tune_res[26]);

	return 0;
}
#endif

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Initialize tuning parameter valuer
 * @param &host: msdc host manipulator pointer
 * @param *res: Initial value for each parameter
 * @return
 *     If 0, initial OK. \n
 *     Others, fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void autok_tuning_parameter_init(struct msdc_host *host, u8 *res)
{
	unsigned int ret = 0;
	/* void __iomem *base = host->base; */

	/* MSDC_SET_FIELD(MSDC_PATCH_BIT2, 7<<29, 2); */
	/* MSDC_SET_FIELD(MSDC_PATCH_BIT2, 7<<16, 4); */

	ret = autok_param_apply(host, res);
}

/*******************************************************
* Function: msdc_autok_adjust_paddly                   *
* Param : value - delay cnt from 0 to 63               *
*         pad_sel - 0 for cmd pad and 1 for data pad   *
*******************************************************/
/** @ingroup type_group_linux_eMMC_def
 * @brief eMMC pad delay definitions
 * @{
 */
/** Define for CMD pad dly */
#define CMD_PAD_RDLY 0
/** Define for DAT pad dly */
#define DAT_PAD_RDLY 1
/** Define for DS pad dly */
#define DS_PAD_RDLY 2
/**
 * @}
 */
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Initialize tuning parameter valuer
 * @param *host: msdc host manipulator pointer
 * @param *value: Initial value for tuning
 * @param pad_sel: select cmd or dat pad
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void msdc_autok_adjust_paddly(struct msdc_host *host,
				     unsigned int *value, unsigned int pad_sel)
{
	unsigned int cfg_l = 0;
	unsigned int cfg_l_sel = 0;
	unsigned int cfg_h = 0;
	unsigned int cfg_h_sel = 0;
	unsigned int dly_cnt = *value;

	cfg_l = (dly_cnt > 31) ? (31) : dly_cnt;
	cfg_h = (dly_cnt > 31) ? (dly_cnt - 32) : 0;

	cfg_l_sel = (cfg_l > 0) ? 1 : 0;
	cfg_h_sel = (cfg_h > 0) ? 1 : 0;
	switch (pad_sel) {
	case CMD_PAD_RDLY:
		msdc_autok_adjust_param(host, CMD_RD_D_DLY1, &cfg_l,
					AUTOK_WRITE);
		msdc_autok_adjust_param(host, CMD_RD_D_DLY2, &cfg_h,
					AUTOK_WRITE);

		msdc_autok_adjust_param(host, CMD_RD_D_DLY1_SEL, &cfg_l_sel,
					AUTOK_WRITE);
		msdc_autok_adjust_param(host, CMD_RD_D_DLY2_SEL, &cfg_h_sel,
					AUTOK_WRITE);
		break;
	case DAT_PAD_RDLY:
		msdc_autok_adjust_param(host, DAT_RD_D_DLY1, &cfg_l,
					AUTOK_WRITE);
		msdc_autok_adjust_param(host, DAT_RD_D_DLY2, &cfg_h,
					AUTOK_WRITE);

		msdc_autok_adjust_param(host, DAT_RD_D_DLY1_SEL, &cfg_l_sel,
					AUTOK_WRITE);
		msdc_autok_adjust_param(host, DAT_RD_D_DLY2_SEL, &cfg_h_sel,
					AUTOK_WRITE);
		break;
	case DS_PAD_RDLY:
		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY1, &cfg_l,
					AUTOK_WRITE);
		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY2, &cfg_h,
					AUTOK_WRITE);

		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY1_SEL, &cfg_l_sel,
					AUTOK_WRITE);
		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY2_SEL, &cfg_h_sel,
					AUTOK_WRITE);
		break;
	}
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Update tuning result for each parameter
 * @param pad_sel: select pad name for update
 * @param dly_cnt: Initial value for tuning
 * @param *autok_tune_res: autok tuning result
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void autok_paddly_update(unsigned int pad_sel, unsigned int dly_cnt,
				u8 *autok_tune_res)
{
	unsigned int cfg_l = 0;
	unsigned int cfg_l_sel = 0;
	unsigned int cfg_h = 0;
	unsigned int cfg_h_sel = 0;

	cfg_l = (dly_cnt > 31) ? (31) : dly_cnt;
	cfg_h = (dly_cnt > 31) ? (dly_cnt - 32) : 0;

	cfg_l_sel = (cfg_l > 0) ? 1 : 0;
	cfg_h_sel = (cfg_h > 0) ? 1 : 0;
	switch (pad_sel) {
	case CMD_PAD_RDLY:
		autok_param_update(CMD_RD_D_DLY1, cfg_l, autok_tune_res);
		autok_param_update(CMD_RD_D_DLY2, cfg_h, autok_tune_res);

		autok_param_update(CMD_RD_D_DLY1_SEL, cfg_l_sel,
				   autok_tune_res);
		autok_param_update(CMD_RD_D_DLY2_SEL, cfg_h_sel,
				   autok_tune_res);
		break;
	case DAT_PAD_RDLY:
		autok_param_update(DAT_RD_D_DLY1, cfg_l, autok_tune_res);
		autok_param_update(DAT_RD_D_DLY2, cfg_h, autok_tune_res);

		autok_param_update(DAT_RD_D_DLY1_SEL, cfg_l_sel,
				   autok_tune_res);
		autok_param_update(DAT_RD_D_DLY2_SEL, cfg_h_sel,
				   autok_tune_res);
		break;
	case DS_PAD_RDLY:
		autok_param_update(EMMC50_DS_Z_DLY1, cfg_l, autok_tune_res);
		autok_param_update(EMMC50_DS_Z_DLY2, cfg_h, autok_tune_res);

		autok_param_update(EMMC50_DS_Z_DLY1_SEL, cfg_l_sel,
				   autok_tune_res);
		autok_param_update(EMMC50_DS_Z_DLY2_SEL, cfg_h_sel,
				   autok_tune_res);
		break;
	}
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     apply tuning result for each item
 * @param scan_win: delay scan window information
 * @param scan_window: scan result
 * @param *autok_tune_res: autok tuning result
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static void msdc_autok_window_apply(enum AUTOK_SCAN_WIN scan_win,
				    u64 scan_window,
				    unsigned char *autok_tune_res)
{
	switch (scan_win) {
	case CMD_RISE:
		autok_tune_res[CMD_SCAN_R0] = (scan_window >> 0) & 0xff;
		autok_tune_res[CMD_SCAN_R1] = (scan_window >> 8) & 0xff;
		autok_tune_res[CMD_SCAN_R2] = (scan_window >> 16) & 0xff;
		autok_tune_res[CMD_SCAN_R3] = (scan_window >> 24) & 0xff;
		autok_tune_res[CMD_SCAN_R4] = (scan_window >> 32) & 0xff;
		autok_tune_res[CMD_SCAN_R5] = (scan_window >> 40) & 0xff;
		autok_tune_res[CMD_SCAN_R6] = (scan_window >> 48) & 0xff;
		autok_tune_res[CMD_SCAN_R7] = (scan_window >> 56) & 0xff;
		break;
	case CMD_FALL:
		autok_tune_res[CMD_SCAN_F0] = (scan_window >> 0) & 0xff;
		autok_tune_res[CMD_SCAN_F1] = (scan_window >> 8) & 0xff;
		autok_tune_res[CMD_SCAN_F2] = (scan_window >> 16) & 0xff;
		autok_tune_res[CMD_SCAN_F3] = (scan_window >> 24) & 0xff;
		autok_tune_res[CMD_SCAN_F4] = (scan_window >> 32) & 0xff;
		autok_tune_res[CMD_SCAN_F5] = (scan_window >> 40) & 0xff;
		autok_tune_res[CMD_SCAN_F6] = (scan_window >> 48) & 0xff;
		autok_tune_res[CMD_SCAN_F7] = (scan_window >> 56) & 0xff;
		break;
	case DAT_RISE:
		autok_tune_res[DAT_SCAN_R0] = (scan_window >> 0) & 0xff;
		autok_tune_res[DAT_SCAN_R1] = (scan_window >> 8) & 0xff;
		autok_tune_res[DAT_SCAN_R2] = (scan_window >> 16) & 0xff;
		autok_tune_res[DAT_SCAN_R3] = (scan_window >> 24) & 0xff;
		autok_tune_res[DAT_SCAN_R4] = (scan_window >> 32) & 0xff;
		autok_tune_res[DAT_SCAN_R5] = (scan_window >> 40) & 0xff;
		autok_tune_res[DAT_SCAN_R6] = (scan_window >> 48) & 0xff;
		autok_tune_res[DAT_SCAN_R7] = (scan_window >> 56) & 0xff;
		break;
	case DAT_FALL:
		autok_tune_res[DAT_SCAN_F0] = (scan_window >> 0) & 0xff;
		autok_tune_res[DAT_SCAN_F1] = (scan_window >> 8) & 0xff;
		autok_tune_res[DAT_SCAN_F2] = (scan_window >> 16) & 0xff;
		autok_tune_res[DAT_SCAN_F3] = (scan_window >> 24) & 0xff;
		autok_tune_res[DAT_SCAN_F4] = (scan_window >> 32) & 0xff;
		autok_tune_res[DAT_SCAN_F5] = (scan_window >> 40) & 0xff;
		autok_tune_res[DAT_SCAN_F6] = (scan_window >> 48) & 0xff;
		autok_tune_res[DAT_SCAN_F7] = (scan_window >> 56) & 0xff;
		break;
	case DS_WIN:
		autok_tune_res[DS_SCAN_0] = (scan_window >> 0) & 0xff;
		autok_tune_res[DS_SCAN_1] = (scan_window >> 8) & 0xff;
		autok_tune_res[DS_SCAN_2] = (scan_window >> 16) & 0xff;
		autok_tune_res[DS_SCAN_3] = (scan_window >> 24) & 0xff;
		autok_tune_res[DS_SCAN_4] = (scan_window >> 32) & 0xff;
		autok_tune_res[DS_SCAN_5] = (scan_window >> 40) & 0xff;
		autok_tune_res[DS_SCAN_6] = (scan_window >> 48) & 0xff;
		autok_tune_res[DS_SCAN_7] = (scan_window >> 56) & 0xff;
		break;
	}
}

/*******************************************************
* Exectue tuning IF Implenment                         *
*******************************************************/
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Update tuning value to each parameter
 * @param *host: msdc host manipulator pointer
 * @param param: parameter item for update
 * @param value: value to update
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static int autok_write_param(struct msdc_host *host, enum AUTOK_PARAM param,
			     u32 value)
{
	msdc_autok_adjust_param(host, param, &value, AUTOK_WRITE);

	return 0;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Select path setting for each tuning parameter
 * @param *host: msdc host manipulator pointer
 * @return
 *     Always 0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int autok_path_sel(struct msdc_host *host)
{
	void __iomem *base = host->base;

	autok_write_param(host, READ_DATA_SMPL_SEL, 0);
	autok_write_param(host, WRITE_DATA_SMPL_SEL, 0);

	/* clK tune all data Line share dly */
	autok_write_param(host, DATA_DLYLINE_SEL, 0);

	/* data tune mode select */
	autok_write_param(host, MSDC_DAT_TUNE_SEL, 0);
	autok_write_param(host, MSDC_WCRC_ASYNC_FIFO_SEL, 1);
	autok_write_param(host, MSDC_RESP_ASYNC_FIFO_SEL, 0);

	/* eMMC50 Function Mux */
	/* write path switch to emmc45 */
	autok_write_param(host, EMMC50_WDATA_MUX_EN, 0);

	/* response path switch to emmc45 */
	autok_write_param(host, EMMC50_CMD_MUX_EN, 0);
	autok_write_param(host, EMMC50_WDATA_EDGE, 0);

	/* Common Setting Config */
	autok_write_param(host, CKGEN_MSDC_DLY_SEL, AUTOK_CKGEN_VALUE);
	autok_write_param(host, CMD_RSP_TA_CNTR, AUTOK_CMD_TA_VALUE);
	autok_write_param(host, WRDAT_CRCS_TA_CNTR, AUTOK_CRC_TA_VALUE);

	MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA,
		       AUTOK_BUSY_MA_VALUE);

	/* LATCH_TA_EN Config for WCRC Path HS FS mode */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
		       AUTOK_CRC_LATCH_EN_HS_VALUE);
	/* LATCH_TA_EN Config for CMD Path HS FS mode */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
		       AUTOK_CMD_LATCH_EN_HS_VALUE);

	/* DDR50 byte swap issue design fix feature enable */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, 1 << 19, 1);

	return 0;
}
EXPORT_SYMBOL(autok_path_sel);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Initialize path and setting for SDR104 mode tuning
 * @param *host: msdc host manipulator pointer
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int autok_init_sdr104(struct msdc_host *host)
{
	void __iomem *base = host->base;

	/* driver may miss data tune path setting in the interim */
	autok_path_sel(host);

	/* if any specific config need modify add here */
	if (host->sclk <= 100000000) {
		/* LATCH_TA_EN Config for WCRC Path HS FS mode */
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
			       AUTOK_CRC_LATCH_EN_HS_VALUE);
		/* LATCH_TA_EN Config for CMD Path HS FS mode */
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
			       AUTOK_CMD_LATCH_EN_HS_VALUE);
	} else {
		/* LATCH_TA_EN Config for WCRC Path SDR104 mode */
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
			       AUTOK_CRC_LATCH_EN_SDR104_PORT1_VALUE);
		/* LATCH_TA_EN Config for CMD Path SDR104 mode */
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
			       AUTOK_CMD_LATCH_EN_SDR104_PORT1_VALUE);
	}
	/* enable dvfs feature */
	/* if (host->hw->host_function == MSDC_SDIO) */
	/*      MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_DVFS_EN, 1); */
	MSDC_SET_FIELD(SDC_FIFO_CFG, SDC_FIFO_CFG_WR_VALID_SEL, 0);
	MSDC_SET_FIELD(SDC_FIFO_CFG, SDC_FIFO_CFG_RD_VALID_SEL, 0);

	return 0;
}
EXPORT_SYMBOL(autok_init_sdr104);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Initialize path and setting for eMMC HS200 mode tuning
 * @param &host: msdc host manipulator pointer
 * @return
 *     0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int autok_init_hs200(struct msdc_host *host)
{
	void __iomem *base = host->base;

	/* driver may miss data tune path setting in the interim */
	autok_path_sel(host);

	/* if any specific config need modify add here */
	/* LATCH_TA_EN Config for WCRC Path non_HS400 */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
		       AUTOK_CRC_LATCH_EN_HS200_PORT0_VALUE);
	/* LATCH_TA_EN Config for CMD Path non_HS400 */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
		       AUTOK_CMD_LATCH_EN_HS200_PORT0_VALUE);

	MSDC_SET_FIELD(SDC_FIFO_CFG, SDC_FIFO_CFG_WR_VALID_SEL, 0);
	MSDC_SET_FIELD(SDC_FIFO_CFG, SDC_FIFO_CFG_RD_VALID_SEL, 0);

	return 0;
}
EXPORT_SYMBOL(autok_init_hs200);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Initialize path and setting for eMMC HS400 mode tuning
 * @param &host: msdc host manipulator pointer
 * @return
 *     Always 0
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int autok_init_hs400(struct msdc_host *host)
{
	void __iomem *base = host->base;
	/* driver may miss data tune path setting in the interim */
	autok_path_sel(host);

	/* if any specific config need modify add here */
	/* LATCH_TA_EN Config for WCRC Path HS400 */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
		       AUTOK_CRC_LATCH_EN_HS400_PORT0_VALUE);
	/* LATCH_TA_EN Config for CMD Path HS400 */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
		       AUTOK_CMD_LATCH_EN_HS400_PORT0_VALUE);
	/* write path switch to emmc50 */
	autok_write_param(host, EMMC50_WDATA_MUX_EN, 1);
	/* Specifical for HS400 Path Sel */
	autok_write_param(host, MSDC_WCRC_ASYNC_FIFO_SEL, 0);

	return 0;
}
EXPORT_SYMBOL(autok_init_hs400);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Online tuning execution for eMMC HS400 mode
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, tuning OK. \n
 *     If -1, tuing fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int execute_online_tuning_hs400(struct msdc_host *host, u8 *res)
{
#if !defined(CONFIG_MACH_FPGA)
	void __iomem *base = host->base;
	unsigned int ret = 0;
	unsigned int response;
	unsigned int cmd_edge = 0;
	u64 raw_data_64 = 0LL;
	unsigned int score = 0;
	unsigned int j, k, cycle_value;
	struct AUTOK_REF_INFO cmd_dat_info;
	struct AUTOK_SCAN_RES *bd_info;
	struct AUTOK_REF_INFO_NEW ds_info;
	struct AUTOK_SCAN_RES_NEW *info;
	char tune_result_str64[65];
	u8 p_autok_tune_res[TUNING_PARA_SCAN_COUNT];
	unsigned int opcode = MMC_SEND_STATUS;
	unsigned int dat_dly = 0;
	static unsigned char tx_online_tune;
#if HS400_DSCLK_NEED_TUNING
	u32 raw_data = 0;
#endif
#if AUTOK_OFFLINE_TUNE_TX_ENABLE
	struct AUTOK_TX_PARA emmc50_tx_para;
#endif
	unsigned int pre_cmd_tx, pre_dat_tx;

	autok_init_hs400(host);
	memset((void *)p_autok_tune_res, 0,
	       sizeof(p_autok_tune_res) / sizeof(u8));
	/* check tx tune res */
	if (tx_online_tune) {
		MSDC_GET_FIELD(EMMC50_PAD_CMD_TUNE,
			       MSDC_EMMC50_PAD_CMD_TUNE_TXDLY, pre_cmd_tx);
		MSDC_GET_FIELD(EMMC50_PAD_DAT01_TUNE,
			       MSDC_EMMC50_PAD_DAT0_TXDLY, pre_dat_tx);
		p_autok_tune_res[EMMC50_CMD_TX_DLY] = pre_cmd_tx;
		p_autok_tune_res[EMMC50_DAT0_TX_DLY] = pre_dat_tx;
		p_autok_tune_res[EMMC50_DAT1_TX_DLY] = pre_dat_tx;
		p_autok_tune_res[EMMC50_DAT2_TX_DLY] = pre_dat_tx;
		p_autok_tune_res[EMMC50_DAT3_TX_DLY] = pre_dat_tx;
		p_autok_tune_res[EMMC50_DAT4_TX_DLY] = pre_dat_tx;
		p_autok_tune_res[EMMC50_DAT5_TX_DLY] = pre_dat_tx;
		p_autok_tune_res[EMMC50_DAT6_TX_DLY] = pre_dat_tx;
		p_autok_tune_res[EMMC50_DAT7_TX_DLY] = pre_dat_tx;
	}
	/* Step1 : Tuning Cmd Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	memset(&cmd_dat_info, 0, sizeof(struct AUTOK_REF_INFO));

	cmd_edge = 0;
	do {
		bd_info = (struct AUTOK_SCAN_RES *)&
			   (cmd_dat_info.scan_info[cmd_edge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge,
					AUTOK_WRITE);
		raw_data_64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_CMD);
				if ((ret &
				     (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) !=
				    0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				} else if (ret == E_RESULT_FATAL_ERR)
					return -1;
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		pr_info("[AUTOK]CMD %d \t %d \t %s\r\n", cmd_edge, score,
		       tune_result_str64);
		if (cmd_edge)
			msdc_autok_window_apply(CMD_FALL, raw_data_64,
						p_autok_tune_res);
		else
			msdc_autok_window_apply(CMD_RISE, raw_data_64,
						p_autok_tune_res);
		if (autok_check_scan_res64(raw_data_64, bd_info,
		    AUTOK_TUNING_INACCURACY) != 0)
			return -1;

		cmd_edge ^= 0x1;
	} while (cmd_edge);

	if (autok_pad_dly_sel(&cmd_dat_info) == 0) {
		autok_param_update(CMD_EDGE, cmd_dat_info.opt_edge_sel,
				   p_autok_tune_res);
		autok_paddly_update(CMD_PAD_RDLY, cmd_dat_info.opt_dly_cnt,
				    p_autok_tune_res);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]==Analysis Failed!!==\r\n");
	}
	/* DLY3 keep default value 20 */
	p_autok_tune_res[EMMC50_DS_ZDLY_DLY] = 20;
	cycle_value = cmd_dat_info.cycle_cnt;
	/* Step2 : Tuning DS Clk Path-ZCLK only tune DLY1 */
#ifdef CMDQ
	opcode = MMC_SEND_EXT_CSD;	/* can also use MMC_READ_SINGLE_BLOCK */
#else
	opcode = MMC_READ_SINGLE_BLOCK;
#endif
	autok_tuning_parameter_init(host, p_autok_tune_res);
	/* check device status */
	ret = autok_send_tune_cmd(host, 13, TUNE_CMD);
	if (ret == E_RESULT_PASS) {
		response = MSDC_READ32(SDC_RESP0);
		AUTOK_RAWPRINT("[AUTOK]current device status 0x%08x\r\n",
			       response);
	} else
		AUTOK_RAWPRINT
		    ("[AUTOK]CMD error while check device status\r\n");
	/* check QSR status */
	ret = autok_send_tune_cmd(host, CHECK_QSR, TUNE_CMD);
	if (ret == E_RESULT_PASS) {
		response = MSDC_READ32(SDC_RESP0);
		AUTOK_RAWPRINT("[AUTOK]current QSR 0x%08x\r\n", response);
	} else
		AUTOK_RAWPRINT("[AUTOK]CMD error while check QSR\r\n");
	/* tune data pad delay , find data pad boundary */
	for (j = 0; j < 32; j++) {
		msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
		for (k = 0; k < AUTOK_CMD_TIMES / 4; k++) {
			ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
			if ((ret & (E_RESULT_CMD_TMO |
				E_RESULT_RSP_CRC)) != 0) {
				return -1;
			} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO))
				   != 0)
				break;
			else if (ret == E_RESULT_FATAL_ERR)
				return -1;
		}
		if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
			p_autok_tune_res[DAT_RD_D_DLY1] = j;
			if (j)
				p_autok_tune_res[DAT_RD_D_DLY1_SEL] = 1;
			break;
		}
	}
	autok_tuning_parameter_init(host, p_autok_tune_res);
	memset(&ds_info, 0, sizeof(struct AUTOK_REF_INFO_NEW));
	info = (struct AUTOK_SCAN_RES_NEW *)&(ds_info.scan_info[0]);
	raw_data_64 = 0LL;
	/* tune DS delay , base on data pad boundary */
	for (j = 0; j < 32; j++) {
		msdc_autok_adjust_paddly(host, &j, DS_PAD_RDLY);
		for (k = 0; k < AUTOK_CMD_TIMES / 4; k++) {
			ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
			if ((ret & (E_RESULT_CMD_TMO |
				E_RESULT_RSP_CRC)) != 0) {
				AUTOK_RAWPRINT
				    ("Autok CMD Fail when tune DS Delay\r\n");
				return -1;
			} else if ((ret & (E_RESULT_DAT_CRC |
				   E_RESULT_DAT_TMO)) != 0) {
				raw_data_64 |= (u64) (1LL << j);
				break;
			} else if (ret == E_RESULT_FATAL_ERR)
				return -1;
		}
	}
	raw_data_64 |= 0xffffffff00000000;
	score = autok_simple_score64(tune_result_str64, raw_data_64);
	pr_info("[AUTOK] DLY1/2 %d \t %d \t %s\r\n",
	       cmd_edge, score, tune_result_str64);
	msdc_autok_window_apply(DS_WIN, raw_data_64, p_autok_tune_res);
	if (autok_check_scan_res64_new(raw_data_64, info, 0) != 0)
		return -1;
	autok_ds_dly_sel(info, &dat_dly);
	autok_paddly_update(DS_PAD_RDLY, dat_dly, p_autok_tune_res);

#if HS400_DSCLK_NEED_TUNING
	/* Step3 : Tuning DS Clk Path-ZDLY */
	p_autok_tune_res[EMMC50_DS_Z_DLY1] = 8;
	p_autok_tune_res[EMMC50_DS_Z_DLY1_SEL] = 1;
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan DS(ZDLY) Clk Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DS_ZDLY_DLY DS_ZCLK_DLY1~2 \r\n");
	bd_info = (struct AUTOK_SCAN_RES *)&(cmd_dat_info.scan_info[1]);
	raw_data = 0;
	for (j = 0; j < 32; j++) {
		msdc_autok_adjust_param(host, EMMC50_DS_ZDLY_DLY, &j,
					AUTOK_WRITE);
		for (k = 0; k < AUTOK_CMD_TIMES / 4; k++) {
			ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
			if ((ret & (E_RESULT_CMD_TMO |
			    E_RESULT_RSP_CRC)) != 0) {
				AUTOK_RAWPRINT
				    ("Autok CMD Fail when tune DS Delay\r\n");
				return -1;
			} else if ((ret & (E_RESULT_DAT_CRC |
				   E_RESULT_DAT_TMO))
				   != 0) {
				raw_data |= (u64) (1 << j);
				break;
			}
		}
	}
	score = autok_simple_score(tune_result_str64, raw_data);
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", cmd_edge,
		       score, tune_result_str64);
	if (autok_check_scan_res64(raw_data, bd_info) != 0)
		return -1;

	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "Edge:%d \t BoundaryCnt:%d FullBoundaryCnt:%d\n",
		       cmd_edge, bd_info->bd_cnt, bd_info->fbd_cnt);

	for (i = 0; i < BD_MAX_CNT; i++) {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "BoundInf[%d]:S:%d E:%d W:%d FullBound:%d\r\n",
			       i, bd_info->bd_info[i].Bound_Start,
			       bd_info->bd_info[i].Bound_End,
			       bd_info->bd_info[i].Bound_width,
			       bd_info->bd_info[i].is_fullbound);
	}

	autok_ds_dly_sel(info, &dat_dly);
	autok_param_update(EMMC50_DS_ZDLY_DLY, dat_dly, p_autok_tune_res);
#endif

	autok_tuning_parameter_init(host, p_autok_tune_res);
#if AUTOK_OFFLINE_TUNE_TX_ENABLE
	if (tx_online_tune == 0) {
		tx_online_tune = 1;
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DATA tx addr :%x\r\n",
			       host->total_sectors + TUNE_DATA_TX_ADDR);
		if (host->total_sectors == 0)
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]DATA tx addr = 0 error\r\n");
		if ((TUNE_DATA_TX_ADDR >= 0) || (host->total_sectors > 0))
			emmc50_tx_para =
			    autok_offline_tuning_TX(host, cycle_value);
		else
			goto tune_done;
		if (emmc50_tx_para.tx_err_type == 0) {
			p_autok_tune_res[EMMC50_CMD_TX_DLY] =
			    emmc50_tx_para.tx_cmd;
			p_autok_tune_res[EMMC50_DAT0_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			p_autok_tune_res[EMMC50_DAT1_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			p_autok_tune_res[EMMC50_DAT2_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			p_autok_tune_res[EMMC50_DAT3_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			p_autok_tune_res[EMMC50_DAT4_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			p_autok_tune_res[EMMC50_DAT5_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			p_autok_tune_res[EMMC50_DAT6_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			p_autok_tune_res[EMMC50_DAT7_TX_DLY] =
			    emmc50_tx_para.tx_dat;
			autok_tuning_parameter_init(host, p_autok_tune_res);
		}
	}
tune_done:
#endif
	autok_result_dump(host, p_autok_tune_res);
#if AUTOK_PARAM_DUMP_ENABLE
	autok_register_dump(host);
#endif
	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
		       sizeof(p_autok_tune_res) / sizeof(u8));
	}
#endif
	return 0;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Online cmd tuning execution
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, tuning OK. \n
 *     If -1, tuing fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int execute_cmd_online_tuning(struct msdc_host *host, u8 *res)
{
#if !defined(CONFIG_MACH_FPGA)
	void __iomem *base_top = host->base_top;
#endif
	unsigned int ret = 0;
	unsigned int cmd_edge = 0;
	u64 raw_data_64 = 0LL;
	unsigned int score = 0;
	unsigned int j, k;	/* cycle_value */
	struct AUTOK_REF_INFO cmd_dat_info;
	struct AUTOK_SCAN_RES *bd_info;
	char tune_result_str64[65];
	u8 p_autok_tune_res[5];
	unsigned int opcode = MMC_SEND_STATUS;

	memset((void *)p_autok_tune_res, 0,
	       sizeof(p_autok_tune_res) / sizeof(u8));

	/* Tuning Cmd Path */
	memset(&cmd_dat_info, 0, sizeof(struct AUTOK_REF_INFO));

	cmd_edge = 0;
	do {
		bd_info = (struct AUTOK_SCAN_RES *)&
			   (cmd_dat_info.scan_info[cmd_edge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge,
					AUTOK_WRITE);
		raw_data_64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_CMD);
				if ((ret &
				     (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) !=
				    0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				} else if (ret == E_RESULT_FATAL_ERR)
					return -1;
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD %d \t %d \t %s\r\n",
			       cmd_edge, score, tune_result_str64);
		if (autok_check_scan_res64(raw_data_64, bd_info,
		    AUTOK_TUNING_INACCURACY) != 0)
			return -1;

		cmd_edge ^= 0x1;
	} while (cmd_edge);

	if (autok_pad_dly_sel(&cmd_dat_info) == 0) {
		msdc_autok_adjust_param(host, CMD_EDGE,
					&cmd_dat_info.opt_edge_sel,
					AUTOK_WRITE);
#if !defined(CONFIG_MACH_FPGA)
		MSDC_GET_FIELD(EMMC_TOP_CMD, PAD_CMD_RXDLY,
			      p_autok_tune_res[1]);
		MSDC_GET_FIELD(EMMC_TOP_CMD, PAD_CMD_RD_RXDLY_SEL,
			      p_autok_tune_res[2]);
		MSDC_GET_FIELD(EMMC_TOP_CMD, PAD_CMD_RXDLY2,
			      p_autok_tune_res[3]);
		MSDC_GET_FIELD(EMMC_TOP_CMD, PAD_CMD_RD_RXDLY2_SEL,
			      p_autok_tune_res[4]);
#else
			return 0;
#endif
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]==Analysis Failed!!==\r\n");
	}

	AUTOK_RAWPRINT("[AUTOK]CMD [EDGE:%d DLY1:%d DLY2:%d]\r\n",
		       p_autok_tune_res[0], p_autok_tune_res[1],
		       p_autok_tune_res[3]);

	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
		       sizeof(p_autok_tune_res) / sizeof(u8));
	}

	return 0;
}

/* online tuning for latch ck */
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Online tuning execution for eMMC HS400 mode
 * @param *host: msdc host manipulator pointer
 * @param opcode: speed mode for tuning
 * @param latch_ck_initial_value: initial value for latch clock tuning
 * @return
 *     If 0, tuning OK. \n
 *     If -1, tuing fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int autok_execute_tuning_latch_ck(struct msdc_host *host, unsigned int opcode,
				  unsigned int latch_ck_initail_value)
{
	unsigned int ret = 0;
	unsigned int j, k;
	void __iomem *base = host->base;
	unsigned int tune_time;

	MSDC_WRITE32(MSDC_INT, 0xffffffff);
	switch (host->hw->host_function) {
	case MSDC_EMMC:
		tune_time = AUTOK_LATCH_CK_EMMC_TUNE_TIMES;
		break;
	case MSDC_SD:
		tune_time = AUTOK_LATCH_CK_SD_TUNE_TIMES;
		break;
	case MSDC_SDIO:
		tune_time = AUTOK_LATCH_CK_SDIO_TUNE_TIMES;
		break;
	default:
		tune_time = AUTOK_LATCH_CK_SDIO_TUNE_TIMES;
		break;
	}
	for (j = latch_ck_initail_value; j < 8;
	j += (host->hclk / host->sclk)) {
		host->tune_latch_ck_cnt = 0;
		msdc_clear_fifo();
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			       j);
		for (k = 0; k < tune_time; k++) {
			if (opcode == MMC_SEND_TUNING_BLOCK_HS200) {
				switch (k) {
				case 0:
					host->tune_latch_ck_cnt = 1;
					break;
				default:
					host->tune_latch_ck_cnt = k;
					break;
				}
			} else if (opcode == MMC_SEND_TUNING_BLOCK) {
				switch (k) {
				case 0:
				case 1:
				case 2:
					host->tune_latch_ck_cnt = 1;
					break;
				default:
					host->tune_latch_ck_cnt = k - 1;
					break;
				}
			} else if (opcode == MMC_SEND_EXT_CSD) {
				host->tune_latch_ck_cnt = k + 1;
			} else
				host->tune_latch_ck_cnt++;
			ret = autok_send_tune_cmd(host, opcode, TUNE_LATCH_CK);
			if ((ret & (E_RESULT_CMD_TMO |
				E_RESULT_RSP_CRC)) != 0) {
				AUTOK_RAWPRINT
				    ("Autok CMD Fail while tune LATCH CK\r\n");
				break;
			} else if ((ret & (E_RESULT_DAT_CRC |
				   E_RESULT_DAT_TMO)) != 0) {
				AUTOK_RAWPRINT
				    ("Autok  tune LATCH_CK error %d\r\n",
				     j);
				break;
			}
		}
		if (ret == 0) {
			MSDC_SET_FIELD(MSDC_PATCH_BIT0,
				       MSDC_PB0_INT_DAT_LATCH_CK_SEL, j);
			break;
		}
	}
	host->tune_latch_ck_cnt = 0;

	return j;
}

/* online tuning for eMMC4.5(hs200) */
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Online tuning execution for eMMC HS200 mode
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, tuning OK. \n
 *     If -1, tuing fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int execute_online_tuning_hs200(struct msdc_host *host, u8 *res)
{
#if !defined(CONFIG_MACH_FPGA)
	void __iomem *base = host->base;
	unsigned int ret = 0;
	unsigned int response;
	unsigned int cmd_edge = 0;
	unsigned int dat_edge = 0;
	u64 raw_data_64 = 0LL;
	unsigned int score = 0;
	unsigned int j, k;
	struct AUTOK_REF_INFO cmd_dat_info;
	struct AUTOK_SCAN_RES *bd_info;
	char tune_result_str64[65];
	u8 p_autok_tune_res[TUNING_PARA_SCAN_COUNT];
	unsigned int opcode = MMC_SEND_STATUS;
#if SINGLE_EDGE_ONLINE_TUNE
	unsigned int dat_dly = 0;
#endif

	autok_init_hs200(host);
	memset((void *)p_autok_tune_res, 0,
	       sizeof(p_autok_tune_res) / sizeof(u8));

	/* Step1 : Tuning Cmd Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	memset(&cmd_dat_info, 0, sizeof(struct AUTOK_REF_INFO));

	cmd_edge = 0;
	do {
		bd_info = (struct AUTOK_SCAN_RES *)&
			   (cmd_dat_info.scan_info[cmd_edge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge,
					AUTOK_WRITE);
		raw_data_64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_CMD);
				if ((ret &
				     (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) !=
				    0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				} else if (ret == E_RESULT_FATAL_ERR)
					return -1;
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD %d \t %d \t %s\r\n",
			       cmd_edge, score, tune_result_str64);
		if (cmd_edge)
			msdc_autok_window_apply(CMD_FALL, raw_data_64,
						p_autok_tune_res);
		else
			msdc_autok_window_apply(CMD_RISE, raw_data_64,
						p_autok_tune_res);

		if (autok_check_scan_res64(raw_data_64, bd_info,
		    AUTOK_TUNING_INACCURACY) != 0) {
			host->autok_error = -1;
			return -1;
		}

		cmd_edge ^= 0x1;
	} while (cmd_edge);

	if (autok_pad_dly_sel(&cmd_dat_info) == 0) {
		autok_param_update(CMD_EDGE, cmd_dat_info.opt_edge_sel,
				   p_autok_tune_res);
		autok_paddly_update(CMD_PAD_RDLY, cmd_dat_info.opt_dly_cnt,
				    p_autok_tune_res);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]==Analysis Failed!!==\r\n");
	}

	/* Step2 Tuning Data Path (Only Rising Edge Used) */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	/* check device status */
	ret = autok_send_tune_cmd(host, 13, TUNE_CMD);
	if (ret == E_RESULT_PASS) {
		response = MSDC_READ32(SDC_RESP0);
		AUTOK_RAWPRINT("[AUTOK]current device status 0x%08x\r\n",
			       response);
	} else
		AUTOK_RAWPRINT
		    ("[AUTOK]CMD error while check device status\r\n");
	opcode = MMC_SEND_TUNING_BLOCK_HS200;
	memset(&cmd_dat_info, 0, sizeof(struct AUTOK_REF_INFO));

	dat_edge = 0;
	do {
		bd_info = (struct AUTOK_SCAN_RES *)&
			   (cmd_dat_info.scan_info[dat_edge]);
		msdc_autok_adjust_param(host, RD_FIFO_EDGE, &dat_edge,
					AUTOK_WRITE);
		raw_data_64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_DATA);
				if ((ret &
				     (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) !=
				    0) {
					AUTOK_RAWPRINT
					    ("Autok CMD Fail when tune Rd\n");
					return -1;
				} else if ((ret &
					 (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO))
					!= 0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				} else if (ret == E_RESULT_FATAL_ERR)
					return -1;
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DAT %d \t %d \t %s\r\n",
			       dat_edge, score, tune_result_str64);
		if (dat_edge)
			msdc_autok_window_apply(DAT_FALL, raw_data_64,
						p_autok_tune_res);
		else
			msdc_autok_window_apply(DAT_RISE, raw_data_64,
						p_autok_tune_res);
		if (autok_check_scan_res64(raw_data_64, bd_info,
					   AUTOK_TUNING_INACCURACY) != 0) {
			host->autok_error = -1;
			return -1;
		}

		dat_edge ^= 0x1;
	} while (dat_edge);

	if (autok_pad_dly_sel(&cmd_dat_info) == 0) {
		autok_param_update(RD_FIFO_EDGE, cmd_dat_info.opt_edge_sel,
				   p_autok_tune_res);
		autok_paddly_update(DAT_PAD_RDLY, cmd_dat_info.opt_dly_cnt,
				    p_autok_tune_res);
		autok_param_update(WD_FIFO_EDGE, cmd_dat_info.opt_edge_sel,
				   p_autok_tune_res);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]==Analysis Failed!!==\r\n");
	}

	autok_tuning_parameter_init(host, p_autok_tune_res);

	/* Step3 : Tuning LATCH CK  */
#if 0
	opcode = MMC_SEND_TUNING_BLOCK_HS200;
	p_autok_tune_res[INT_DAT_LATCH_CK] =
	    autok_execute_tuning_latch_ck(host, opcode,
					  p_autok_tune_res[INT_DAT_LATCH_CK]);
#endif

	autok_result_dump(host, p_autok_tune_res);

#if AUTOK_PARAM_DUMP_ENABLE
	autok_register_dump(host);
#endif
	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
		       sizeof(p_autok_tune_res) / sizeof(u8));
	}
#endif
	return 0;
}

/* online tuning for SDIO/SD */
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Online tuning execution for SD/SDIO mode
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, tuning OK. \n
 *     If -1, tuing fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int execute_online_tuning(struct msdc_host *host, u8 *res)
{
	void __iomem *base = host->base;
	unsigned int ret = 0;
	unsigned int cmd_edge = 0;
	unsigned int dat_edge = 0;
	u64 raw_data_64 = 0LL;
	unsigned int score = 0;
	unsigned int j, k;
	unsigned int opcode = MMC_SEND_TUNING_BLOCK;
	struct AUTOK_REF_INFO cmd_dat_info;
	struct AUTOK_SCAN_RES *bd_info;
	char tune_result_str64[65];
	u8 p_autok_tune_res[TUNING_PARA_SCAN_COUNT];

	autok_init_sdr104(host);
	memset((void *)p_autok_tune_res, 0,
	       sizeof(p_autok_tune_res) / sizeof(u8));

	/* Step1 : Tuning Cmd Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	memset(&cmd_dat_info, 0, sizeof(struct AUTOK_REF_INFO));

	cmd_edge = 0;
	do {
		bd_info = (struct AUTOK_SCAN_RES *)&
			   (cmd_dat_info.scan_info[cmd_edge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge,
					AUTOK_WRITE);
		raw_data_64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_CMD);
				if ((ret & E_RESULT_RSP_CRC) != 0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				} else if ((ret & E_RESULT_CMD_TMO) != 0) {
					autok_msdc_reset();
					msdc_clear_fifo();
					MSDC_WRITE32(MSDC_INT, 0xffffffff);
					raw_data_64 |= (u64) (1LL << j);
					break;
				} else if (ret == E_RESULT_FATAL_ERR)
					return -1;
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD %d \t %d \t %s\r\n",
			       cmd_edge, score, tune_result_str64);
		if (cmd_edge)
			msdc_autok_window_apply(CMD_FALL, raw_data_64,
						p_autok_tune_res);
		else
			msdc_autok_window_apply(CMD_RISE, raw_data_64,
						p_autok_tune_res);
		if (autok_check_scan_res64(raw_data_64, bd_info,
		   AUTOK_TUNING_INACCURACY) != 0) {
			host->autok_error = -1;
			return -1;
		}

		cmd_edge ^= 0x1;
	} while (cmd_edge);

	if (autok_pad_dly_sel(&cmd_dat_info) == 0) {
		autok_param_update(CMD_EDGE, cmd_dat_info.opt_edge_sel,
				   p_autok_tune_res);
		autok_paddly_update(CMD_PAD_RDLY, cmd_dat_info.opt_dly_cnt,
				    p_autok_tune_res);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]==Analysis Failed!!==\r\n");
	}

	/* Step2 : Tuning Data Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	memset(&cmd_dat_info, 0, sizeof(struct AUTOK_REF_INFO));

	dat_edge = 0;
	do {
		bd_info = (struct AUTOK_SCAN_RES *)&
			   (cmd_dat_info.scan_info[dat_edge]);
		msdc_autok_adjust_param(host, RD_FIFO_EDGE, &dat_edge,
					AUTOK_WRITE);
		raw_data_64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_DATA);
				if ((ret &
				     (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) !=
				    0) {
					AUTOK_RAWPRINT
					    ("Autok CMD Fail when tun Rd\r\n");
					host->autok_error = -1;
					return -1;
				} else if ((ret &
					 (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO))
					!= 0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				} else if (ret == E_RESULT_FATAL_ERR)
					return -1;
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DAT %d \t %d \t %s\r\n",
			       dat_edge, score, tune_result_str64);
		if (dat_edge)
			msdc_autok_window_apply(DAT_FALL, raw_data_64,
						p_autok_tune_res);
		else
			msdc_autok_window_apply(DAT_RISE, raw_data_64,
						p_autok_tune_res);

		if (autok_check_scan_res64(raw_data_64, bd_info,
		    AUTOK_TUNING_INACCURACY) != 0) {
			host->autok_error = -1;
			return -1;
		}

		dat_edge ^= 0x1;
	} while (dat_edge);

	if (autok_pad_dly_sel(&cmd_dat_info) == 0) {
		autok_param_update(RD_FIFO_EDGE, cmd_dat_info.opt_edge_sel,
				   p_autok_tune_res);
		autok_paddly_update(DAT_PAD_RDLY, cmd_dat_info.opt_dly_cnt,
				    p_autok_tune_res);
		autok_param_update(WD_FIFO_EDGE, cmd_dat_info.opt_edge_sel,
				   p_autok_tune_res);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]==Analysis Failed!!==\r\n");
	}

	autok_tuning_parameter_init(host, p_autok_tune_res);

	autok_result_dump(host, p_autok_tune_res);
#if AUTOK_PARAM_DUMP_ENABLE
	autok_register_dump(host);
#endif
	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
		       sizeof(p_autok_tune_res) / sizeof(u8));
	}
	host->autok_error = 0;

	return 0;
}

/********************************************************************
* Offline tune for eMMC	                                            *
* Common Interface                                                  *
********************************************************************/
/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Offline tuning execution for eMMC HS400 mode
 * @param *host: msdc host manipulator pointer
 * @return
 *     If 0, tuning OK. \n
 *     If -1, tuing fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int execute_offline_tuning_hs400(struct msdc_host *host)
{
#if !defined(CONFIG_MACH_FPGA)
	unsigned int ret = 0;

	unsigned int i, j, k;
	unsigned int ckgen_sel = 0;
	unsigned int cmd_edge = 0;
	unsigned int res_cmd_ta = 1;

	u64 raw_data_64 = 0LL;
	unsigned int score = 0;
	char tune_result_str32[33];
	char tune_result_str64[65];
	u32 g_ta_raw[SCALE_TA_CNTR];
	u32 g_ta_score[SCALE_TA_CNTR];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];

	unsigned int opcode = MMC_READ_SINGLE_BLOCK;

	autok_init_hs400(host);
	memset((void *)p_autok_tune_res, 0,
	       sizeof(p_autok_tune_res) / sizeof(u8));
	p_autok_tune_res[EMMC50_DS_Z_DLY1] = 7;
	p_autok_tune_res[EMMC50_DS_Z_DLY1_SEL] = 1;
	p_autok_tune_res[EMMC50_DS_ZDLY_DLY] = 24;

	/************************************************************
	*                                                           *
	*  Step 1 : Caculate CMD_RSP_TA                             *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT
	    ("[AUTOK]Step1.Scan CMD_RSP_TA Base on Cmd Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t CMD_RSP_TA_CNTR \t PAD_CMD_D_DLY \r\n");

	cmd_edge = 0;
	do {
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge, AUTOK_WRITE);
		for (i = 0; i < 8; i++) {
			g_ta_raw[i] = 0;
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &i,
						AUTOK_WRITE);
			for (j = 0; j < 32; j++) {
				msdc_autok_adjust_param(host, CMD_RD_D_DLY1, &j,
							AUTOK_WRITE);

				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret =
					    autok_send_tune_cmd(host, opcode,
								TUNE_CMD);

					if ((ret &
					     (E_RESULT_CMD_TMO |
					      E_RESULT_RSP_CRC)) != 0) {
						g_ta_raw[i] |= (1 << j);
						break;
					}
				}
			}
			g_ta_score[i] =
			    autok_simple_score(tune_result_str32, g_ta_raw[i]);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]%d \t %d\t %d \t %s\r\n",
				       cmd_edge, i, g_ta_score[i],
				       tune_result_str32);
		}

		if (autok_ta_sel(g_ta_raw, &res_cmd_ta) == 0) {
			/* autok_param_update(CMD_RSP_TA_CNTR,
			 * res_cmd_ta, p_autok_tune_res);
			 */
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR,
						&res_cmd_ta, AUTOK_WRITE);
			AUTOK_RAWPRINT("[AUTOK]CMD_TA Sel:%d\r\n", res_cmd_ta);
			break;
		}
		AUTOK_RAWPRINT
		    ("Internal Boundary Occours,sw cmd edg for rescan!\r\n");
		cmd_edge ^= 1;
	} while (cmd_edge);

	/************************************************************
	*                                                           *
	*  Step 2 : Scan CMD Pad Delay Base on CMD_EDGE & CKGEN     *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Scan CMD Pad Data Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t CKGEN \t CMD_RD_R_DLY1~2 \r\n");
	cmd_edge = 0;

	do {
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge, AUTOK_WRITE);

		for (i = 0; i < 32; i++) {
			raw_data_64 = 0LL;
			msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i,
						AUTOK_WRITE);

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j,
							 CMD_PAD_RDLY);
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret =
					    autok_send_tune_cmd(host, opcode,
								TUNE_CMD);
					if ((ret &
					     (E_RESULT_CMD_TMO |
					      E_RESULT_RSP_CRC)) != 0) {
						raw_data_64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64,
						     raw_data_64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]%d \t %d \t %d \t %s\r\n",
				       cmd_edge, i, score, tune_result_str64);
		}

		cmd_edge ^= 0x1;
	} while (cmd_edge);

	ckgen_sel = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &ckgen_sel,
				AUTOK_WRITE);
	/************************************************************
	*                                                           *
	*  Step 3 : Scan Dat Pad Delay Base on Data_EDGE & CKGEN    *
	*                                                           *
	************************************************************/
	opcode = MMC_SEND_TUNING_BLOCK_HS200;
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan DS Clk Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DS_ZDLY_DLY DS_ZCLK_DLY1~2 \r\n");
	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, EMMC50_DS_ZDLY_DLY, &i,
					AUTOK_WRITE);
		cmd_edge = 0;
		raw_data_64 = 0LL;

		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, DS_PAD_RDLY);
rd_retry0:
			for (k = 0; k < AUTOK_CMD_TIMES; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_DATA);
				if ((ret &
				     (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) !=
				    0) {
					cmd_edge ^= 0x1;
					msdc_autok_adjust_param(host, CMD_EDGE,
								&cmd_edge,
								AUTOK_WRITE);
					goto rd_retry0;
				} else
				    if ((ret &
					 (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO))
					!= 0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] %d \t %d \t %s\r\n",
			       i, cmd_edge, score, tune_result_str64);
	}

	/* CMD 17 Single block read */
	opcode = MMC_READ_SINGLE_BLOCK;
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan DS Clk Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DS_ZDLY_DLY DS_ZCLK_DLY1~2 \r\n");

	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, EMMC50_DS_ZDLY_DLY, &i,
					AUTOK_WRITE);
		cmd_edge = 0;
		raw_data_64 = 0LL;

		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, DS_PAD_RDLY);
rd_retry1:
			for (k = 0; k < AUTOK_CMD_TIMES; k++) {
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_DATA);
				if ((ret &
				     (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) !=
				    0) {
					cmd_edge ^= 0x1;
					msdc_autok_adjust_param(host, CMD_EDGE,
								&cmd_edge,
								AUTOK_WRITE);
					goto rd_retry1;
				} else
				    if ((ret &
					 (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO))
					!= 0) {
					raw_data_64 |= (u64) (1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, raw_data_64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] %d \t %d \t %s\r\n",
			       i, cmd_edge, score, tune_result_str64);
	}

	autok_tuning_parameter_init(host, p_autok_tune_res);

	return ret;
#else
	return 0;
#endif
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Offline tuning execution for SD/SDIO and eMMC HS200 mode
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, tuning OK. \n
 *     If -1, tuing fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int execute_offline_tuning(struct msdc_host *host)
{
	/* AUTOK_CODE_RELEASE */
	unsigned int ret = 0;
	unsigned int i, j, k;
	unsigned int ckgen_sel = 0;
	unsigned int cmd_edge = 0;
	unsigned int dat_edge = 0;
	unsigned int res_cmd_ta = 1;

	u64 raw_data_64 = 0LL;
	unsigned int score = 0;
	char tune_result_str32[33];
	char tune_result_str64[65];
	u32 g_ta_raw[SCALE_TA_CNTR];
	u32 g_ta_score[SCALE_TA_CNTR];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];
	unsigned int opcode = MMC_SEND_TUNING_BLOCK;

	memset((void *)p_autok_tune_res, 0,
	       sizeof(p_autok_tune_res) / sizeof(u8));
	if (host->hw->host_function == MSDC_SDIO) {
		opcode = MMC_SEND_TUNING_BLOCK;
		autok_init_sdr104(host);
	} else if (host->hw->host_function == MSDC_EMMC) {
		opcode = MMC_SEND_TUNING_BLOCK_HS200;
		autok_init_hs200(host);
	}

	/************************************************************
	*                                                           *
	*  Step 1 : Caculate CMD_RSP_TA                             *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT
	    ("[AUTOK]Step1.Scan CMD_RSP_TA Base on Cmd Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t CMD_RSP_TA_CNTR \t PAD_CMD_D_DLY \r\n");

	cmd_edge = 0;
	do {
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge, AUTOK_WRITE);
		for (i = 0; i < 8; i++) {
			g_ta_raw[i] = 0;
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &i,
						AUTOK_WRITE);
			for (j = 0; j < 32; j++) {
				msdc_autok_adjust_param(host, CMD_RD_D_DLY1, &j,
							AUTOK_WRITE);

				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret =
					    autok_send_tune_cmd(host, opcode,
								TUNE_CMD);

					if ((ret &
					     (E_RESULT_CMD_TMO |
					      E_RESULT_RSP_CRC)) != 0) {
						g_ta_raw[i] |= (1 << j);
						break;
					}
				}
			}
			g_ta_score[i] =
			    autok_simple_score(tune_result_str32, g_ta_raw[i]);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]%d \t %d\t %d \t %s\r\n",
				       cmd_edge, i, g_ta_score[i],
				       tune_result_str32);
		}

		if (autok_ta_sel(g_ta_raw, &res_cmd_ta) == 0) {
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR,
						&res_cmd_ta, AUTOK_WRITE);
			AUTOK_RAWPRINT("[AUTOK]CMD_TA Sel:%d\r\n", res_cmd_ta);
			break;
		}
		AUTOK_RAWPRINT
		    ("[AUTOK]Internal Boundary Occ,sw cmd edg rescan!\r\n");
		cmd_edge ^= 1;
	} while (cmd_edge);

	/************************************************************
	*                                                           *
	*  Step 2 : Scan CMD Pad Delay Base on CMD_EDGE & CKGEN     *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Scan CMD Pad Data Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t CKGEN \t CMD_RD_R_DLY1~2 \r\n");
	cmd_edge = 0;

	do {
		msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge, AUTOK_WRITE);

		for (i = 0; i < 32; i++) {
			raw_data_64 = 0LL;
			msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i,
						AUTOK_WRITE);

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j,
							 CMD_PAD_RDLY);
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret =
					    autok_send_tune_cmd(host, opcode,
								TUNE_CMD);
					if ((ret &
					     (E_RESULT_CMD_TMO |
					      E_RESULT_RSP_CRC)) != 0) {
						raw_data_64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64,
						     raw_data_64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]%d \t %d \t %d \t %s\r\n",
				       cmd_edge, i, score, tune_result_str64);
		}

		cmd_edge ^= 0x1;
	} while (cmd_edge);

	/************************************************************
	*                                                           *
	*  Step 3 : Scan Dat Pad Delay Base on Data_EDGE & CKGEN    *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan Dat Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t DAT_EDGE \t CKGEN \t DAT_RD_R_DLY1~2 \r\n");
	cmd_edge = 0;

	do {
		msdc_autok_adjust_param(host, RD_FIFO_EDGE, &dat_edge,
					AUTOK_WRITE);
		for (i = 0; i < 32; i++) {
			raw_data_64 = 0LL;
			msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i,
						AUTOK_WRITE);

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j,
							 DAT_PAD_RDLY);
rd_retry:
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret =
					    autok_send_tune_cmd(host, opcode,
								TUNE_DATA);
					if ((ret &
					     (E_RESULT_CMD_TMO |
					      E_RESULT_RSP_CRC)) != 0) {
						cmd_edge ^= 0x1;
						msdc_autok_adjust_param(host,
								CMD_EDGE,
								&cmd_edge,
								AUTOK_WRITE);
						goto rd_retry;
					} else
					    if ((ret &
						 (E_RESULT_DAT_CRC |
						  E_RESULT_DAT_TMO)) != 0) {
						raw_data_64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64,
						     raw_data_64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK][%d] \t %d \t %d \t %d \t %s\r\n",
				       cmd_edge, dat_edge, i, score,
				       tune_result_str64);
		}

		dat_edge ^= 0x1;
	} while (dat_edge);

	/* Debug Info for reference */
	/************************************************************
	*                                                           *
	*  Step [1] : Scan CMD Pad Delay Base on CMD_EDGE & CKGEN   *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step[1].Scan CMD Pad Data Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t CKGEN \t CMD_RD_R_DLY1~2 \r\n");
	cmd_edge = 0;

	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i,
					AUTOK_WRITE);

		do {
			msdc_autok_adjust_param(host, CMD_EDGE, &cmd_edge,
						AUTOK_WRITE);
			raw_data_64 = 0LL;

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j,
							 CMD_PAD_RDLY);
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret =
					    autok_send_tune_cmd(host, opcode,
								TUNE_CMD);
					if ((ret &
					     (E_RESULT_CMD_TMO |
					      E_RESULT_RSP_CRC)) != 0) {
						raw_data_64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64,
						     raw_data_64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]%d \t %d \t %d \t %s\r\n",
				       cmd_edge, i, score, tune_result_str64);
			cmd_edge ^= 0x1;

		} while (cmd_edge);
	}

	/************************************************************
	*                                                           *
	*  Step [2] : Scan Dat Pad Delay Base on Data_EDGE & CKGEN  *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step[2].Scan Dat Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t DAT_EDGE \t CKGEN \t DAT_RD_R_DLY1~2 \r\n");
	cmd_edge = 0;
	dat_edge = 0;
	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i,
					AUTOK_WRITE);

		do {
			msdc_autok_adjust_param(host, RD_FIFO_EDGE, &dat_edge,
						AUTOK_WRITE);
			raw_data_64 = 0LL;

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j,
							 DAT_PAD_RDLY);
rd_retry1:
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret =
					    autok_send_tune_cmd(host, opcode,
								TUNE_DATA);
					if ((ret &
					     (E_RESULT_CMD_TMO |
					      E_RESULT_RSP_CRC)) != 0) {
						cmd_edge ^= 0x1;
						msdc_autok_adjust_param(host,
								CMD_EDGE,
								&cmd_edge,
								AUTOK_WRITE);
						goto rd_retry1;
					} else
					    if ((ret &
						 (E_RESULT_DAT_CRC |
						  E_RESULT_DAT_TMO)) != 0) {
						raw_data_64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64,
						     raw_data_64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK][%d] \t %d \t %d \t %d \t %s\r\n",
				       cmd_edge, dat_edge, i, score,
				       tune_result_str64);

			dat_edge ^= 0x1;
		} while (dat_edge);

	}

	ckgen_sel = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &ckgen_sel,
				AUTOK_WRITE);

	AUTOK_RAWPRINT("[AUTOK]Offline Tune Alg Complete\r\n");

	return 0;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     TX setting config for autok flow
 * @param *host: msdc host manipulator pointer
 * @param *ios: mmc iso pointer
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void autok_msdc_tx_setting(struct msdc_host *host, struct mmc_ios *ios)
{
	void __iomem *base = host->base;
#if !defined(CONFIG_MACH_FPGA)
	void __iomem *base_top = host->base_top;
#endif

	if (ios->timing == MMC_TIMING_MMC_HS400) {
		MSDC_SET_FIELD(EMMC50_CFG0,
			      MSDC_EMMC50_CFG_TXSKEW_SEL,
			      AUTOK_MSDC0_HS400_TXSKEW);
#if !defined(CONFIG_MACH_FPGA)
		MSDC_SET_FIELD(TOP_EMMC50_PAD_CTL0,
			      PAD_CLK_TXDLY,
			      AUTOK_MSDC0_HS400_CLKTXDLY);
		MSDC_SET_FIELD(EMMC_TOP_CMD,
			      PAD_CMD_TX_DLY,
			      AUTOK_MSDC0_HS400_CMDTXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT0_TUNE,
			      PAD_DAT0_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT0TXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT1_TUNE,
			      PAD_DAT1_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT1TXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT2_TUNE,
			      PAD_DAT2_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT2TXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT3_TUNE,
			      PAD_DAT3_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT3TXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT4_TUNE,
			      PAD_DAT4_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT4TXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT5_TUNE,
			      PAD_DAT5_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT5TXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT6_TUNE,
			      PAD_DAT6_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT6TXDLY);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT7_TUNE,
			      PAD_DAT7_TX_DLY,
			      AUTOK_MSDC0_HS400_DAT7TXDLY);
#endif
	} else if (ios->timing == MMC_TIMING_MMC_HS200) {
		MSDC_SET_FIELD(EMMC50_CFG0,
					  MSDC_EMMC50_CFG_TXSKEW_SEL,
					  AUTOK_MSDC0_HS400_TXSKEW);
	} else {
		if (ios->timing == MMC_TIMING_MMC_DDR52) {
			MSDC_SET_FIELD(MSDC_IOCON,
				      MSDC_IOCON_DDR50CKD,
				      AUTOK_MSDC0_DDR50_DDRCKD);
		} else {
			MSDC_SET_FIELD(MSDC_IOCON,
				      MSDC_IOCON_DDR50CKD, 0);
		}
#if !defined(CONFIG_MACH_FPGA)
			MSDC_SET_FIELD(TOP_EMMC50_PAD_CTL0,
				      PAD_CLK_TXDLY,
				      AUTOK_MSDC0_CLKTXDLY);
			MSDC_SET_FIELD(EMMC_TOP_CMD,
				      PAD_CMD_TX_DLY,
				      AUTOK_MSDC0_CMDTXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT0_TUNE,
				      PAD_DAT0_TX_DLY,
				      AUTOK_MSDC0_DAT0TXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT1_TUNE,
				      PAD_DAT1_TX_DLY,
				      AUTOK_MSDC0_DAT1TXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT2_TUNE,
				      PAD_DAT2_TX_DLY,
				      AUTOK_MSDC0_DAT2TXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT3_TUNE,
				      PAD_DAT3_TX_DLY,
				      AUTOK_MSDC0_DAT3TXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT4_TUNE,
				      PAD_DAT4_TX_DLY,
				      AUTOK_MSDC0_DAT4TXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT5_TUNE,
				      PAD_DAT5_TX_DLY,
				      AUTOK_MSDC0_DAT5TXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT6_TUNE,
				      PAD_DAT6_TX_DLY,
				      AUTOK_MSDC0_DAT6TXDLY);
			MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT7_TUNE,
				      PAD_DAT7_TX_DLY,
				      AUTOK_MSDC0_DAT7TXDLY);
#endif
	}
}
EXPORT_SYMBOL(autok_msdc_tx_setting);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Edge change for different error type
 * @param *host: msdc host manipulator pointer
 * @param *ios: mmc ios pointer
 * @param error_type: enum for ERROR type
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
void autok_low_speed_switch_edge(struct msdc_host *host, struct mmc_ios *ios,
				 enum ERROR_TYPE error_type)
{
	void __iomem *base = host->base;
	unsigned int orig_resp_edge, orig_crc_fifo_edge, orig_read_edge,
	    orig_read_fifo_edge;
	unsigned int cur_resp_edge, cur_crc_fifo_edge, cur_read_edge,
	    cur_read_fifo_edge;

	AUTOK_RAWPRINT
	    ("[AUTOK][low speed switch edge]=========start========\r\n");
	if (host->id == 0) {
		switch (error_type) {
		case CMD_ERROR:
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, orig_resp_edge);
			MSDC_SET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, orig_resp_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, cur_resp_edge);
			AUTOK_RAWPRINT
			    ("[AUTOK][CMD err]pre_edge = %d cur_edge = %d\r\n",
			     orig_resp_edge, cur_resp_edge);
			break;
		case DATA_ERROR:
#ifdef PORT0_PB0_RD_DAT_SEL_VALID
			if (ios->timing == MMC_TIMING_MMC_DDR52) {
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL_SEL, 0);
				MSDC_GET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL,
					       orig_read_edge);
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       orig_read_edge ^ 0x1);
				MSDC_SET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL, 0);
				MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       cur_read_edge);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       cur_read_fifo_edge);
				AUTOK_RAWPRINT
				    ("[rd err]PB0_BIT3_VALID DDR pre_edge = %d",
				     orig_read_edge);
				AUTOK_RAWPRINT
				    ("cur_edge = %d cur_fifo_edge = %d\r\n",
				     cur_read_edge, cur_read_fifo_edge);
			} else {
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL_SEL, 0);
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL, 0);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       orig_read_fifo_edge);
				MSDC_SET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       orig_read_fifo_edge ^ 0x1);
				MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       cur_read_edge);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       cur_read_fifo_edge);
				AUTOK_RAWPRINT
				    ("[rd err]PB0[3]_VALID orig_fifo_edge = %d",
				     orig_read_fifo_edge);
				AUTOK_RAWPRINT
				    ("cur_edge = %d cur_fifo_edge = %d\r\n",
				     cur_read_edge, cur_read_fifo_edge);
			}
#else
			MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_R_D_SMPL, orig_read_edge);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL, 0);
			MSDC_SET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_R_D_SMPL,
				       orig_read_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
				       cur_read_edge);
			AUTOK_RAWPRINT
			    ("[rd err]PB0_INVALID pre_edge=%d cur_edge=%d\r\n",
			     orig_read_edge, cur_read_edge);
#endif
			break;
		case CRC_STATUS_ERROR:
			MSDC_GET_FIELD(MSDC_PATCH_BIT2,
				       MSDC_PB2_CFGCRCSTSEDGE,
				       orig_crc_fifo_edge);
			MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTSEDGE,
				       orig_crc_fifo_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTSEDGE,
				       cur_crc_fifo_edge);
			AUTOK_RAWPRINT
			    ("[wr err]orig_fifo_edge=%d cur_fifo_edge = %d\r\n",
			     orig_crc_fifo_edge, cur_crc_fifo_edge);
			break;
		}
	} else if (host->id == 1) {
		switch (error_type) {
		case CMD_ERROR:
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, orig_resp_edge);
			MSDC_SET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, orig_resp_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, cur_resp_edge);
			AUTOK_RAWPRINT
			    ("[AUTOK][CMD err]pre_edge = %d cur_edge = %d\r\n",
			     orig_resp_edge, cur_resp_edge);
			break;
		case DATA_ERROR:
#ifdef PORT1_PB0_RD_DAT_SEL_VALID
			if (ios->timing == MMC_TIMING_UHS_DDR50) {
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL_SEL, 0);
				MSDC_GET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL,
					       orig_read_edge);
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       orig_read_edge ^ 0x1);
				MSDC_SET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL, 0);
				MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       cur_read_edge);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       cur_read_fifo_edge);
				AUTOK_RAWPRINT
				    ("[rd err]PB0[3]_VALID DDR pre_edge = %d",
				     orig_read_edge);
				AUTOK_RAWPRINT
				    (" cur_edge = %d cur_fifo_edge = %d\r\n",
				     cur_read_edge, cur_read_fifo_edge);
			} else {
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL_SEL, 0);
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL, 0);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       orig_read_fifo_edge);
				MSDC_SET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       orig_read_fifo_edge ^ 0x1);
				MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       cur_read_edge);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       cur_read_fifo_edge);
				AUTOK_RAWPRINT
				    ("[rd err]PB0[3]_VALID orig_fifo_edge = %d",
				     orig_read_fifo_edge);
				AUTOK_RAWPRINT
				    (" cur_edge = %d cur_fifo_edge = %d\r\n",
				     cur_read_edge, cur_read_fifo_edge);
			}
#else
			MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_R_D_SMPL, orig_read_edge);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL, 0);
			MSDC_SET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_R_D_SMPL,
				       orig_read_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
				       cur_read_edge);
			AUTOK_RAWPRINT
			    ("[rd err]PB0_INVALID pre_edge=%d cur_edge=%d\r\n",
			     orig_read_edge, cur_read_edge);
#endif
			break;
		case CRC_STATUS_ERROR:
			MSDC_GET_FIELD(MSDC_PATCH_BIT2,
				       MSDC_PB2_CFGCRCSTSEDGE,
				       orig_crc_fifo_edge);
			MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTSEDGE,
				       orig_crc_fifo_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTSEDGE,
				       cur_crc_fifo_edge);
			AUTOK_RAWPRINT
			    ("[wr err]orig_fifo_edge=%d cur_fifo_edge=%d\r\n",
			     orig_crc_fifo_edge, cur_crc_fifo_edge);
			break;
		}
	} else if (host->id == 2) {
		switch (error_type) {
		case CMD_ERROR:
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, orig_resp_edge);
			MSDC_SET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, orig_resp_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_RSPL, cur_resp_edge);
			AUTOK_RAWPRINT
			    ("[CMD err]pre_edge = %d cur_edge = %d\r\n",
			     orig_resp_edge, cur_resp_edge);
			break;
		case DATA_ERROR:
#ifdef PORT2_PB0_RD_DAT_SEL_VALID
			if (ios->timing == MMC_TIMING_UHS_DDR50) {
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL_SEL, 0);
				MSDC_GET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL,
					       orig_read_edge);
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       orig_read_edge ^ 0x1);
				MSDC_SET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL, 0);
				MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       cur_read_edge);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       cur_read_fifo_edge);
				AUTOK_RAWPRINT
				    ("[rd err]PB0[3]_VALID DDR pre_edge = %d",
				     orig_read_edge);
				AUTOK_RAWPRINT
				    (" cur_edge = %d cur_fifo_edge = %d\r\n",
				     cur_read_edge, cur_read_fifo_edge);
			} else {
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL_SEL, 0);
				MSDC_SET_FIELD(MSDC_IOCON,
					       MSDC_IOCON_R_D_SMPL, 0);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       orig_read_fifo_edge);
				MSDC_SET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       orig_read_fifo_edge ^ 0x1);
				MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
					       cur_read_edge);
				MSDC_GET_FIELD(MSDC_PATCH_BIT0,
					       MSDC_PB0_RD_DAT_SEL,
					       cur_read_fifo_edge);
				AUTOK_RAWPRINT
				    ("[rd err]PB0[3]_VALID orig_fifo_edge = %d",
				     orig_read_fifo_edge);
				AUTOK_RAWPRINT
				    (" cur_edge = %d cur_fifo_edge = %d\r\n",
				     cur_read_edge, cur_read_fifo_edge);
			}
#else
			MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
			MSDC_GET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_R_D_SMPL, orig_read_edge);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL, 0);
			MSDC_SET_FIELD(MSDC_IOCON,
				       MSDC_IOCON_R_D_SMPL,
				       orig_read_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL,
				       cur_read_edge);
			AUTOK_RAWPRINT
			    ("PB0[3]_INVALID pre_edge=%d cur_edge = %d\r\n",
			     orig_read_edge, cur_read_edge);
#endif
			break;
		case CRC_STATUS_ERROR:
			MSDC_GET_FIELD(MSDC_PATCH_BIT2,
				       MSDC_PB2_CFGCRCSTSEDGE,
				       orig_crc_fifo_edge);
			MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTSEDGE,
				       orig_crc_fifo_edge ^ 0x1);
			MSDC_GET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTSEDGE,
				       cur_crc_fifo_edge);
			AUTOK_RAWPRINT
			    ("[wr err]orig_fifo_edge=%dcur_fifo_edge=%d\r\n",
			     orig_crc_fifo_edge, cur_crc_fifo_edge);
			break;
		}
	}
	AUTOK_RAWPRINT
	    ("[AUTOK][low speed switch edge]=========end========\r\n");
}
EXPORT_SYMBOL(autok_low_speed_switch_edge);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     Online TX setting change for SDIO tuning
 * @param tx_raw_scan: TX delay scan setting
 * @param cycle_value: tuning cycle
 * @param tx_type: TX type for change
 * @return
 *    tx_res: TX tuning result
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct AUTOK_TX_RES sdio_tx_online_select(unsigned int tx_raw_scan,
					  unsigned int cycle_value,
					  enum TUNE_TX_TYPE tx_type)
{
	unsigned char bit;
	int i, j;
	unsigned char pass_bd_info_cnt = 0;
	unsigned char fail_bd_info_cnt = 0;
	unsigned int tx_select;
	enum AUTOK_TX_SCAN_STA_E cmd_tx_sta = START_POSITION;
	struct AUTOK_TX_SCAN_RES tx_bond;
	struct AUTOK_TX_RES tx_res;

	tx_res.tx_err = 0;
	tx_res.tx_result = 0;

	for (i = 0; i < TX_BD_MAX_CNT; i++) {
		tx_bond.fail_bd_info[i].end = 0;
		tx_bond.fail_bd_info[i].start = 0;
		tx_bond.pass_bd_info[i].end = 0;
		tx_bond.pass_bd_info[i].start = 0;
	}
	tx_bond.fail_bd_cnt = 0;
	tx_bond.pass_bd_cnt = 0;
	/* check scan tx boundary */
	for (bit = 0; bit < 32; bit++) {
		if (tx_raw_scan & (1 << bit)) {
			switch (cmd_tx_sta) {
			case START_POSITION:
				cmd_tx_sta = FAIL_POSITION;
				tx_bond.fail_bd_info[fail_bd_info_cnt++].start =
				    bit;
				tx_bond.fail_bd_cnt++;
				break;

			case PASS_POSITION:
				cmd_tx_sta = FAIL_POSITION;
				if (bit == 31) {
					tx_bond.
					    fail_bd_info[fail_bd_info_cnt++].
					    start = bit;
					tx_bond.fail_bd_info[fail_bd_info_cnt -
							     1].end = bit;
				} else
					tx_bond.
					    fail_bd_info[fail_bd_info_cnt++].
					    start = bit;
				tx_bond.pass_bd_info[pass_bd_info_cnt - 1].end =
				    bit - 1;
				tx_bond.fail_bd_cnt++;
				break;

			case FAIL_POSITION:
				cmd_tx_sta = FAIL_POSITION;
				if (bit == 31)
					tx_bond.fail_bd_info[fail_bd_info_cnt -
							     1].end = bit;
				break;

			default:
				break;
			}
		} else {
			switch (cmd_tx_sta) {
			case START_POSITION:
				cmd_tx_sta = PASS_POSITION;
				tx_bond.pass_bd_info[pass_bd_info_cnt++].start =
				    bit;
				tx_bond.pass_bd_cnt++;
				break;

			case PASS_POSITION:
				cmd_tx_sta = PASS_POSITION;
				if (bit == 31)
					tx_bond.pass_bd_info[pass_bd_info_cnt -
							     1].end = bit;
				break;

			case FAIL_POSITION:
				cmd_tx_sta = PASS_POSITION;
				if (bit == 31) {
					tx_bond.
					    pass_bd_info[pass_bd_info_cnt++].
					    start = bit;
					tx_bond.pass_bd_info[pass_bd_info_cnt -
							     1].end = bit;
				} else
					tx_bond.
					    pass_bd_info[pass_bd_info_cnt++].
					    start = bit;
				tx_bond.fail_bd_info[fail_bd_info_cnt - 1].end =
				    bit - 1;
				tx_bond.pass_bd_cnt++;
				break;

			default:
				break;
			}
		}
	}
	for (i = 0; i < tx_bond.fail_bd_cnt; i++) {
		AUTOK_RAWPRINT("[ATUOK TX]pre fail bd: S-%d E-%d\r\n",
			       tx_bond.fail_bd_info[i].start,
			       tx_bond.fail_bd_info[i].end);
	}
	/* analysis scan window and select a suitable value */
	for (i = tx_bond.fail_bd_cnt; i >= 0; i--) {
		if (i > tx_bond.fail_bd_cnt)
			break;
		if ((i >= 1) && ((tx_bond.fail_bd_info[i].start
				  - tx_bond.fail_bd_info[i - 1].end - 1) <=
				 AUTOK_MARGIN_THOLD)) {
			tx_bond.fail_bd_info[i - 1].end =
			    tx_bond.fail_bd_info[i].end;
			tx_bond.fail_bd_info[i].start = 0;
			tx_bond.fail_bd_info[i].end = 0;
			for (j = i; j < (tx_bond.fail_bd_cnt - 1); j++) {
				tx_bond.fail_bd_info[j].start =
				    tx_bond.fail_bd_info[j + 1].start;
				tx_bond.fail_bd_info[j].end =
				    tx_bond.fail_bd_info[j + 1].end;
			}
			tx_bond.fail_bd_info[tx_bond.fail_bd_cnt - 1].start = 0;
			tx_bond.fail_bd_info[tx_bond.fail_bd_cnt - 1].end = 0;
			tx_bond.fail_bd_cnt--;
		}
	}
	for (i = 0; i < tx_bond.fail_bd_cnt; i++) {
		AUTOK_RAWPRINT("[ATUOK TX]cur fail bd: S-%d E-%d\r\n",
			       tx_bond.fail_bd_info[i].start,
			       tx_bond.fail_bd_info[i].end);
	}
	switch (tx_type) {
	case TX_CMD:
		switch (tx_bond.fail_bd_cnt) {
		case 0:
			tx_res.tx_result = 15;
			break;
		case 1:
			tx_select = (tx_bond.fail_bd_info[0].end
				     + tx_bond.fail_bd_info[0].start) / 2;
			if (tx_select >= cycle_value / 2)
				tx_res.tx_result = tx_select - cycle_value / 2;
			else if ((31 - tx_select) >= cycle_value / 2)
				tx_res.tx_result = tx_select + cycle_value / 2;
			else if (tx_bond.fail_bd_info[0].start >
				 (31 - tx_bond.fail_bd_info[0].end))
				tx_res.tx_result = 0;
			else
				tx_res.tx_result = 31;
			break;
		default:
			tx_res.tx_err = 1;
			AUTOK_RAWPRINT
			    ("[ATUOK TX]too more fail boundary %d\r\n",
			     tx_bond.fail_bd_cnt);
			break;
		}
		break;
	case TX_DATA:
		/* max data pass window is T/4 , fail boundary is 3T/4 */
		switch (tx_bond.fail_bd_cnt) {
		case 0:
			tx_res.tx_result = 15;
			break;
		case 1:
			/* ooooooxxxxxxxxxxxxxxxxxxx */
			/* ooooooxxxxxxxxxxxxxxxxxoo */
			/* xxxoooooooooooooooooooo */
			/* oxxoooooooooooooooooooo */
			if ((tx_bond.fail_bd_info[0].end == 31)
			    || (tx_bond.pass_bd_info[0].start == 0)) {
				tx_select = tx_bond.fail_bd_info[0].start - 1;
				if (tx_select >= cycle_value / 4)
					tx_res.tx_result =
					    tx_select - cycle_value / 4;
				else if (tx_select < cycle_value / 4)
					tx_res.tx_result = 0;
			} else if ((tx_bond.fail_bd_info[0].start == 0)
				   || (tx_bond.pass_bd_info[0].end == 31)) {
				tx_select = 31 - tx_bond.fail_bd_info[0].end;
				if (tx_select >= cycle_value / 4)
					tx_res.tx_result =
					    tx_bond.fail_bd_info[0].end +
					    cycle_value / 4;
				else if (tx_select < cycle_value / 4)
					tx_res.tx_result = 31;
			} else {
				tx_res.tx_err = 1;
				AUTOK_RAWPRINT
				    ("[ATUOK TX]abnormal tx boundary\r\n");
			}
			break;
		case 2:
			/* xxooooooooooooooooooxxx */
			if ((tx_bond.fail_bd_info[0].start == 0)
			    && (tx_bond.fail_bd_info[1].end == 31))
				tx_res.tx_result = (tx_bond.fail_bd_info[0].end
						    +
						    tx_bond.fail_bd_info[1].
						    start) / 2;
			else {
				tx_res.tx_err = 1;
				AUTOK_RAWPRINT
				    ("[ATUOK TX]abnormal tx boundary\r\n");
			}
			break;
		default:
			tx_res.tx_err = 1;
			AUTOK_RAWPRINT
			    ("[ATUOK TX]too more fail boundary %d\r\n",
			     tx_bond.fail_bd_cnt);
			break;
		}
		break;
	default:
		break;
	}
	return tx_res;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     TX online tuning for dat apth
 * @param *host: msdc host manipulator pointer
 * @param opcode: operating code for tuing
 * @param cycle_value: tuning cycle
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct AUTOK_TX_RES autok_online_tuning_data_TX(struct msdc_host *host,
						unsigned int opcode,
						unsigned int cycle_value)
{
	int ret = 0;
	void __iomem *base = host->base;
	void __iomem *base_top = host->base_top;
	unsigned int response;
	unsigned int tune_tx_value;
	unsigned char tune_cnt;
	unsigned char i;
	unsigned char tune_crc_cnt[32];
	unsigned char tune_pass_cnt[32];
	unsigned char tune_tmo_cnt[32];
	char tune_result[33];
	unsigned int dat_tx[8];
	unsigned int dat_tx_result;
	struct AUTOK_TX_RES dat_tx_sel = { 0 };
	/* store tx setting */
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT0_TUNE, PAD_DAT0_TX_DLY,
		      dat_tx[0]);
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT1_TUNE, PAD_DAT1_TX_DLY,
		      dat_tx[1]);
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT2_TUNE, PAD_DAT2_TX_DLY,
		      dat_tx[2]);
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT3_TUNE, PAD_DAT3_TX_DLY,
		      dat_tx[3]);
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT4_TUNE, PAD_DAT4_TX_DLY,
		      dat_tx[4]);
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT5_TUNE, PAD_DAT5_TX_DLY,
		      dat_tx[5]);
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT6_TUNE, PAD_DAT6_TX_DLY,
		      dat_tx[6]);
	MSDC_GET_FIELD(TOP_EMMC50_PAD_DAT7_TUNE, PAD_DAT7_TX_DLY,
		      dat_tx[7]);

	AUTOK_RAWPRINT("[AUTOK][tune data TX]==start==\r\n");
	/* Tuning Data TX */
	dat_tx_result = 0;
	for (tune_tx_value = 0; tune_tx_value < 32; tune_tx_value++) {
		tune_tmo_cnt[tune_tx_value] = 0;
		tune_crc_cnt[tune_tx_value] = 0;
		tune_pass_cnt[tune_tx_value] = 0;
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT0_TUNE,
			      PAD_DAT0_TX_DLY, tune_tx_value);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT1_TUNE,
			      PAD_DAT1_TX_DLY, tune_tx_value);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT2_TUNE,
			      PAD_DAT2_TX_DLY, tune_tx_value);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT3_TUNE,
			      PAD_DAT3_TX_DLY, tune_tx_value);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT4_TUNE,
			      PAD_DAT4_TX_DLY, tune_tx_value);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT5_TUNE,
			      PAD_DAT5_TX_DLY, tune_tx_value);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT6_TUNE,
			      PAD_DAT6_TX_DLY, tune_tx_value);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT7_TUNE,
			      PAD_DAT7_TX_DLY, tune_tx_value);

		for (tune_cnt = 0; tune_cnt < TUNE_DAT_TX_CNT; tune_cnt++) {
			if (opcode == MMC_WRITE_BLOCK) {
				/* check device status */
				response = 0;
				while (((response >> 9) & 0xF) != 4) {
					ret =
					    autok_send_tune_cmd(host,
								MMC_SEND_STATUS,
								TUNE_CMD);
					if ((ret &
					     (E_RESULT_RSP_CRC |
					      E_RESULT_CMD_TMO |
					      E_RESULT_FATAL_ERR)) != 0) {
						AUTOK_RAWPRINT
						    ("tune cmd13 err\r\n");
						AUTOK_RAWPRINT
						    ("tune data TX fail\r\n");
						dat_tx_sel.tx_err = 1;
						goto end;
					}
					response = MSDC_READ32(SDC_RESP0);
					if ((((response >> 9) & 0xF) == 5)
					    || (((response >> 9) & 0xF) == 6))
						autok_send_tune_cmd(host,
							MMC_STOP_TRANSMISSION,
							TUNE_CMD);
				}
				/* send cmd24 write one block data */
				ret =
				    autok_send_tune_cmd(host, opcode,
							TUNE_DATA);
				response = MSDC_READ32(SDC_RESP0);
			}
			if ((ret &
			     (E_RESULT_RSP_CRC | E_RESULT_CMD_TMO |
			      E_RESULT_FATAL_ERR)) != 0) {
				AUTOK_RAWPRINT
				    ("[AUTOK]--tune data TX cmd%d error--\n",
				     opcode);
				AUTOK_RAWPRINT
				    ("[AUTOK]------tune data TX fail------\n");
				dat_tx_sel.tx_err = 1;
				goto end;
			}
			if ((ret & E_RESULT_DAT_TMO) != 0) {
				tune_tmo_cnt[tune_tx_value]++;
			} else if ((ret & (E_RESULT_DAT_CRC)) != 0) {
				dat_tx_result |= 1 << tune_tx_value;
				tune_crc_cnt[tune_tx_value]++;
			} else if (ret == E_RESULT_PASS) {
				tune_pass_cnt[tune_tx_value]++;
			} else if (ret == E_RESULT_FATAL_ERR) {
				dat_tx_sel.tx_err = 1;
				goto end;
			}
		}
#if 0
		AUTOK_RAWPRINT
		    ("[AUTOK]data_TX value = %d tmo = %d crc = %d pass = %d\n",
		     tune_tx_value, tune_tmo_cnt[tune_tx_value],
		     tune_crc_cnt[tune_tx_value], tune_pass_cnt[tune_tx_value]);
#endif
	}
	/* print result */
	for (i = 0; i < 32; i++) {
		if ((tune_tmo_cnt[i] != 0) || (tune_crc_cnt[i] != 0))
			tune_result[i] = 'X';
		else if (tune_pass_cnt[i] == TUNE_DAT_TX_CNT)
			tune_result[i] = 'O';
	}
	tune_result[32] = '\0';
	AUTOK_RAWPRINT("[AUTOK]tune_data_TX 0 - 31      %s\r\n", tune_result);
	AUTOK_RAWPRINT("[AUTOK][tune data TX]==end==\r\n");
	/* select best DATA TX delay value */
	dat_tx_sel = sdio_tx_online_select(dat_tx_result, cycle_value, TX_DATA);
	AUTOK_RAWPRINT("[AUTOK][tune data TX]tx sel %d\r\n",
		       dat_tx_sel.tx_result);
	if (dat_tx_sel.tx_err != 0) {
		/* restore data tx setting */
		AUTOK_RAWPRINT("[AUTOK][tune dat TX]==err may occur==\r\n");
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT0_TUNE,
			      PAD_DAT0_TX_DLY, dat_tx[0]);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT1_TUNE,
			      PAD_DAT1_TX_DLY, dat_tx[1]);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT2_TUNE,
			      PAD_DAT2_TX_DLY, dat_tx[2]);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT3_TUNE,
			      PAD_DAT3_TX_DLY, dat_tx[3]);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT4_TUNE,
			      PAD_DAT4_TX_DLY, dat_tx[4]);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT5_TUNE,
			      PAD_DAT5_TX_DLY, dat_tx[5]);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT6_TUNE,
			      PAD_DAT6_TX_DLY, dat_tx[6]);
		MSDC_SET_FIELD(TOP_EMMC50_PAD_DAT7_TUNE,
			      PAD_DAT7_TX_DLY, dat_tx[7]);
	}
end:
	return dat_tx_sel;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     TX path offline tuning
 * @param *host: msdc host manipulator pointer
 * @param value: parameter value for tuning
 * @return
 *     none
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
struct AUTOK_TX_PARA autok_offline_tuning_TX(struct msdc_host *host,
					     unsigned int value)
{
	int ret = 0;
	void __iomem *base = host->base;
	void __iomem *base_top = host->base_top;
	unsigned int tune_tx_value;
	unsigned char tune_cnt;
	unsigned char i;
	unsigned char tune_crc_cnt[32];
	unsigned char tune_pass_cnt[32];
	unsigned char tune_tmo_cnt[32];
	unsigned int cmd_tx_result;
	struct AUTOK_TX_RES cmd_tx_sel;
	struct AUTOK_TX_RES dat_tx_sel;
	struct AUTOK_TX_PARA tx_para = { 0 };
	char tune_result[33];
	unsigned int cmd_tx;

	AUTOK_RAWPRINT("[AUTOK][tune cmd TX]==start==\r\n");
	/* store tx setting */
	MSDC_GET_FIELD(EMMC50_PAD_CMD_TUNE, MSDC_EMMC50_PAD_CMD_TUNE_TXDLY,
		       cmd_tx);
	/* Step1 : Tuning Cmd TX */
	cmd_tx_result = 0;
	for (tune_tx_value = 0; tune_tx_value < 32; tune_tx_value++) {
		tune_tmo_cnt[tune_tx_value] = 0;
		tune_crc_cnt[tune_tx_value] = 0;
		tune_pass_cnt[tune_tx_value] = 0;
		MSDC_SET_FIELD(EMMC_TOP_CMD,
			      PAD_CMD_TX_DLY, tune_tx_value);
		for (tune_cnt = 0; tune_cnt < TUNE_CMD_TX_CNT; tune_cnt++) {
			ret =
			    autok_send_tune_cmd(host, MMC_SEND_STATUS,
						TUNE_CMD);
			if ((ret & E_RESULT_CMD_TMO) != 0) {
				tune_tmo_cnt[tune_tx_value]++;
				cmd_tx_result |= 1 << tune_tx_value;
			} else if ((ret & (E_RESULT_RSP_CRC)) != 0)
				tune_crc_cnt[tune_tx_value]++;
			else if (ret == E_RESULT_PASS)
				tune_pass_cnt[tune_tx_value]++;
		}
#if 0
		AUTOK_RAWPRINT
		    ("[AUTOK]cmd_TX value = %d tmo = %d crc = %d pass = %d\n",
		     tune_tx_value, tune_tmo_cnt[tune_tx_value],
		     tune_crc_cnt[tune_tx_value], tune_pass_cnt[tune_tx_value]);
#endif
	}
	/* print result */
	for (i = 0; i < 32; i++) {
		if ((tune_tmo_cnt[i] != 0) || (tune_crc_cnt[i] != 0))
			tune_result[i] = 'X';
		else if (tune_pass_cnt[i] == TUNE_CMD_TX_CNT)
			tune_result[i] = 'O';
	}
	tune_result[32] = '\0';
	AUTOK_RAWPRINT("[AUTOK]tune_cmd_TX 0 - 31      %s\r\n", tune_result);
	AUTOK_RAWPRINT("[AUTOK][tune cmd TX]==end==\r\n");
	/* select best CMD TX delay value */
	cmd_tx_sel = sdio_tx_online_select(cmd_tx_result, value, TX_CMD);
	tx_para.tx_err_type = cmd_tx_sel.tx_err;
	tx_para.tx_cmd = cmd_tx_sel.tx_result;
	AUTOK_RAWPRINT("[AUTOK][tune cmd TX]tx sel %d\r\n",
		       cmd_tx_sel.tx_result);
	/* restore cmd tx setting */
	if (cmd_tx_sel.tx_err != 0) {
		MSDC_SET_FIELD(EMMC_TOP_CMD,
			      PAD_CMD_TX_DLY, cmd_tx);
		AUTOK_RAWPRINT("[AUTOK][tune cmd TX]==err may occur==\r\n");
		goto out;
	}
	/* Step2 : Tuning Data TX */
	dat_tx_sel = autok_online_tuning_data_TX(host, MMC_WRITE_BLOCK, value);
	tx_para.tx_dat = dat_tx_sel.tx_result;
	tx_para.tx_err_type = dat_tx_sel.tx_err;
out:
	return tx_para;
}

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     execute auto tuing function
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, success. \n
 *     If others, error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int autok_execute_tuning(struct msdc_host *host, u8 *res)
{
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;
	u8 autok_tune_res[TUNING_PARAM_COUNT];
	unsigned int i = 0;
	unsigned int value = 0;

	do_gettimeofday(&tm_s);

	int_en = MSDC_READ32(MSDC_INTEN);
	MSDC_WRITE32(MSDC_INTEN, 0);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, 1);
	MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_STOP_DLY_SEL, 6);
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_POPENCNT, 0);

	/* store pre autok parameter */
	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		msdc_autok_adjust_param(host, i, &value, AUTOK_READ);
		autok_tune_res[i] = value;
	}

#if AUTOK_OFFLINE_TUNE_ENABLE
	if (execute_offline_tuning(host) != 0)
		AUTOK_RAWPRINT("[AUTOK] ==Error: Autok OFFLINE Failed==\r\n");
#endif
	ret = execute_online_tuning(host, res);
	if (ret != 0) {
		AUTOK_RAWPRINT("[AUTOK] ==Error: Autok Failed==\r\n");
		AUTOK_RAWPRINT("[AUTOK] ==restore pre autok parameters==\r\n");
		/* restore pre autok parameter */
		for (i = 0; i < TUNING_PARAM_COUNT; i++) {
			value = (u8) autok_tune_res[i];
			msdc_autok_adjust_param(host, i, &value, AUTOK_WRITE);
		}
	}

	autok_msdc_reset();
	msdc_clear_fifo();
	MSDC_WRITE32(MSDC_INT, 0xffffffff);
	MSDC_WRITE32(MSDC_INTEN, int_en);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	tm_val =
	    (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec -
						  tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK]==Time Cost:%d ms==\r\n", tm_val);

	return ret;
}
EXPORT_SYMBOL(autok_execute_tuning);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     execute tuning function for eMMC HS400 mode
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, success. \n
 *     If others, error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int hs400_execute_tuning(struct msdc_host *host, u8 *res)
{
#if !defined(CONFIG_MACH_FPGA)
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;
	u8 autok_tune_res[TUNING_PARAM_COUNT];
	unsigned int i = 0;
	unsigned int value = 0, mode;

	MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 1);
	MSDC_GET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, mode);
	do_gettimeofday(&tm_s);
	int_en = MSDC_READ32(MSDC_INTEN);
	MSDC_WRITE32(MSDC_INTEN, 0);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, 1);

	/* store pre autok parameter */
	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		msdc_autok_adjust_param(host, i, &value, AUTOK_READ);
		autok_tune_res[i] = value;
	}

#if HS400_OFFLINE_TUNE_ENABLE
	if (execute_offline_tuning_hs400(host) != 0)
		AUTOK_RAWPRINT("[AUTOK] == offline tuning Failed==\r\n");
#endif
	ret = execute_online_tuning_hs400(host, res);
	if (ret != 0) {
		AUTOK_RAWPRINT("[AUTOK] ==Error: Autok HS400 Failed==\r\n");
		AUTOK_RAWPRINT("[AUTOK] ==restore pre autok parameters==\r\n");
		/* restore pre autok parameter */
		for (i = 0; i < TUNING_PARAM_COUNT; i++) {
			value = (u8) autok_tune_res[i];
			msdc_autok_adjust_param(host, i, &value, AUTOK_WRITE);
		}
	}

	autok_msdc_reset();
	msdc_clear_fifo();
	MSDC_WRITE32(MSDC_INT, 0xffffffff);
	MSDC_WRITE32(MSDC_INTEN, int_en);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	tm_val =
	    (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec -
						  tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK][HS400]==Time Cost:%d ms==\r\n", tm_val);

	return ret;
#else
	return 0;		/* test */
#endif
}
EXPORT_SYMBOL(hs400_execute_tuning);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     execute cmd path tuning function for eMMC HS400
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, success. \n
 *     If others, error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int hs400_execute_tuning_cmd(struct msdc_host *host, u8 *res)
{
#if !defined(CONFIG_MACH_FPGA)
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;

	do_gettimeofday(&tm_s);
	int_en = MSDC_READ32(MSDC_INTEN);
	MSDC_WRITE32(MSDC_INTEN, 0);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, 1);

	autok_init_hs400(host);
	ret = execute_cmd_online_tuning(host, res);
	if (ret != 0)
		AUTOK_RAWPRINT
		    ("[AUTOK only for cmd] ==Error: Autok HS400 Failed==\r\n");

	autok_msdc_reset();
	msdc_clear_fifo();
	MSDC_WRITE32(MSDC_INT, 0xffffffff);
	MSDC_WRITE32(MSDC_INTEN, int_en);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	tm_val =
	    (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec -
						  tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT
	    ("[AUTOK][HS400 only for cmd]==Time Cost:%d ms==\r\n",
	     tm_val);

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(hs400_execute_tuning_cmd);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     execute tuning function for eMMC HS200
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, success. \n
 *     If others, error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int hs200_execute_tuning(struct msdc_host *host, u8 *res)
{
#if !defined(CONFIG_MACH_FPGA)
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;
	u8 autok_tune_res[TUNING_PARAM_COUNT];
	unsigned int i = 0;
	unsigned int value = 0;

	do_gettimeofday(&tm_s);
	int_en = MSDC_READ32(MSDC_INTEN);
	MSDC_WRITE32(MSDC_INTEN, 0);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, 1);

	/* store pre autok parameter */
	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		msdc_autok_adjust_param(host, i, &value, AUTOK_READ);
		autok_tune_res[i] = value;
	}

#if HS200_OFFLINE_TUNE_ENABLE
	if (execute_offline_tuning(host) != 0)
		AUTOK_RAWPRINT("[AUTOK] ==Error:offline tuning Failed==\r\n");
#endif
	MSDC_WRITE32(MSDC_INT, 0xffffffff);
	ret = execute_online_tuning_hs200(host, res);
	if (ret != 0) {
		AUTOK_RAWPRINT("[AUTOK] ==Error: Autok HS200 Failed==\r\n");
		AUTOK_RAWPRINT("[AUTOK] ==restore pre autok parameters==\r\n");
		/* restore pre autok parameter */
		for (i = 0; i < TUNING_PARAM_COUNT; i++) {
			value = (u8) autok_tune_res[i];
			msdc_autok_adjust_param(host, i, &value, AUTOK_WRITE);
		}
	}

	autok_msdc_reset();
	msdc_clear_fifo();
	MSDC_WRITE32(MSDC_INT, 0xffffffff);
	MSDC_WRITE32(MSDC_INTEN, int_en);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	tm_val =
	    (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec -
						  tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK][HS200]==Time Cost:%d ms==\r\n", tm_val);

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(hs200_execute_tuning);

/** @ingroup type_group_linux_eMMC_InFn
 * @par Description
 *     execute cmd path tuning function for eMMC HS200
 * @param *host: msdc host manipulator pointer
 * @param *res: tuning result
 * @return
 *     If 0, success. \n
 *     If others, error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
int hs200_execute_tuning_cmd(struct msdc_host *host, u8 *res)
{
#if !defined(CONFIG_MACH_FPGA)
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;

	do_gettimeofday(&tm_s);
	int_en = MSDC_READ32(MSDC_INTEN);
	MSDC_WRITE32(MSDC_INTEN, 0);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, 1);
	autok_init_hs200(host);
	ret = execute_cmd_online_tuning(host, res);
	if (ret != 0)
		AUTOK_RAWPRINT
		    ("[AUTOK only for cmd] ==Error: Autok HS200 Failed==\r\n");

	autok_msdc_reset();
	msdc_clear_fifo();
	MSDC_WRITE32(MSDC_INT, 0xffffffff);
	MSDC_WRITE32(MSDC_INTEN, int_en);
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	tm_val =
	    (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec -
						  tm_s.tv_usec) / 1000;

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(hs200_execute_tuning_cmd);
