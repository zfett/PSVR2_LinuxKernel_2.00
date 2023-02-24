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

#ifndef _MSDC_CUST_H_
#define _MSDC_CUST_H_

#include <dt-bindings/mmc/mt3611-msdc.h>
#include <dt-bindings/clock/mt3611-clk.h>



/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
/* Names used for device tree lookup */
#define DT_COMPATIBLE_NAME      "mediatek,mt3611-mmc"
#define MSDC0_IOCFG_NAME        "mediatek,mt3612-pctl-h-syscfg"
#define MSDC0_CLK_NAME          "msdc0-clock"

/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(CONFIG_MACH_FPGA)
/* MSDCPLL register offset */
#define MSDCPLL_CON0_OFFSET     (0x250)
#define MSDCPLL_CON1_OFFSET     (0x254)
#define MSDCPLL_CON2_OFFSET     (0x258)
#define MSDCPLL_PWR_CON0_OFFSET (0x25c)
/* Clock config register offset */
#define MSDC_CLK_CFG_3_OFFSET   (0x130)

#define MSDC_PERI_PDN_SET0_OFFSET       (0x0008)
#define MSDC_PERI_PDN_CLR0_OFFSET       (0x0010)
#define MSDC_PERI_PDN_STA0_OFFSET       (0x0018)
#endif

#define MSDC0_SRC_0             26000000
#define MSDC0_SRC_1             400000000
#define MSDC0_SRC_2             208000000
#define MSDC0_SRC_3             400000000
#define MSDC0_SRC_4             182000000
#define MSDC0_SRC_5             364000000
#define MSDC0_SRC_6             200000000
#define MSDC0_SRC_7             312000000

#define MSDC_SRC_FPGA		26000000

/* #define MSDC0_CG_NAME           MT_CG_INFRA0_MSDC0_CG_STA */

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
/*--------------------------------------------------------------------------*/
/* MSDC0~1 GPIO and IO Pad Configuration Base                               */
/*--------------------------------------------------------------------------*/
#define MSDC0_IO_PAD_BASE       (msdc_io_cfg_bases[0])

/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register                                               */
/*--------------------------------------------------------------------------*/
/* MSDC0 */
#define MSDC0_GPIO_IES_ADDR    (MSDC0_IO_PAD_BASE + 0x0)
#define MSDC0_GPIO_SMT_ADDR    (MSDC0_IO_PAD_BASE + 0x100)
#define MSDC0_GPIO_TDSEL1_ADDR (MSDC0_IO_PAD_BASE + 0x210)
#define MSDC0_GPIO_TDSEL2_ADDR (MSDC0_IO_PAD_BASE + 0x220)
#define MSDC0_GPIO_RDSEL0_ADDR (MSDC0_IO_PAD_BASE + 0x300)
#define MSDC0_GPIO_RDSEL1_ADDR (MSDC0_IO_PAD_BASE + 0x310)
#define MSDC0_GPIO_RDSEL2_ADDR (MSDC0_IO_PAD_BASE + 0x320)
#define MSDC0_GPIO_DRV1_ADDR   (MSDC0_IO_PAD_BASE + 0x610)
#define MSDC0_GPIO_DRV2_ADDR   (MSDC0_IO_PAD_BASE + 0x620)
#define MSDC0_GPIO_PUPD0_ADDR  (MSDC0_IO_PAD_BASE + 0x700)
#define MSDC0_GPIO_PUPD1_ADDR  (MSDC0_IO_PAD_BASE + 0x710)


#define MSDC0_SMT_ALL_MASK      (0xfff <<  10)
#define MSDC0_IES_ALL_MASK      (0xfff <<  10)

/* MSDC0 TDSEL1 mask*/
#define MSDC0_TDSEL1_DAT2_MASK   (0xf  << 28)
#define MSDC0_TDSEL1_DAT1_MASK   (0xf  << 24)
#define MSDC0_TDSEL1_DAT0_MASK   (0xf  << 20)
#define MSDC0_TDSEL1_CMD_MASK    (0xf  << 16)
#define MSDC0_TDSEL1_DSL_MASK    (0xf  << 12)
#define MSDC0_TDSEL1_CLK_MASK    (0xf  <<  8)
#define MSDC0_TDSEL1_ALL_MASK    (0xffffff << 8)

/* MSDC0 TDSEL2 mask*/
#define MSDC0_TDSEL2_RSTB_MASK   (0xf  << 20)
#define MSDC0_TDSEL2_DAT7_MASK   (0xf  << 16)
#define MSDC0_TDSEL2_DAT6_MASK   (0xf  << 12)
#define MSDC0_TDSEL2_DAT5_MASK   (0xf  << 8)
#define MSDC0_TDSEL2_DAT4_MASK   (0xf  << 4)
#define MSDC0_TDSEL2_DAT3_MASK   (0xf  << 0)
#define MSDC0_TDSEL2_ALL_MASK    (0xffffff << 0)

/* MSDC0 RDSEL0 mask*/
#define MSDC0_RDSEL0_DSL_MASK    (0x3f  << 26)
#define MSDC0_RDSEL0_CLK_MASK    (0x3f  << 20)
#define MSDC0_RDSEL0_ALL_MASK    (0xffff << 20)

/* MSDC0 RDSEL1 mask*/
#define MSDC0_RDSEL1_DAT3_MASK   (0x3f  << 24)
#define MSDC0_RDSEL1_DAT2_MASK   (0x3f  << 18)
#define MSDC0_RDSEL1_DAT1_MASK   (0x3f  << 12)
#define MSDC0_RDSEL1_DAT0_MASK   (0x3f  << 6)
#define MSDC0_RDSEL1_CMD_MASK    (0x3f  << 0)
#define MSDC0_RDSEL1_ALL_MASK    (0x3fffffff << 0)

/* MSDC0 RDSEL2 mask*/
#define MSDC0_RDSEL2_RST_MASK    (0x3f << 24)
#define MSDC0_RDSEL2_DAT7_MASK   (0x3f << 18)
#define MSDC0_RDSEL2_DAT6_MASK   (0x3f << 12)
#define MSDC0_RDSEL2_DAT5_MASK   (0x3f << 6)
#define MSDC0_RDSEL2_DAT4_MASK   (0x3f << 0)
#define MSDC0_RDSEL2_ALL_MASK    (0x3fffffff << 0)

/* MSDC0 DRV1 mask*/
#define MSDC0_DRV1_DAT2_MASK     (0xf  << 28)
#define MSDC0_DRV1_DAT1_MASK     (0xf  << 24)
#define MSDC0_DRV1_DAT0_MASK     (0xf  << 20)
#define MSDC0_DRV1_CMD_MASK      (0xf  << 16)
#define MSDC0_DRV1_DSL_MASK      (0xf  << 12)
#define MSDC0_DRV1_CLK_MASK      (0xf  << 8)
#define MSDC0_DRV1_ALL_MASK      (0xffffff << 8)

/* MSDC0 DRV2 mask*/
#define MSDC0_DRV2_RSTB_MASK   (0xf  << 20)
#define MSDC0_DRV2_DAT7_MASK   (0xf  << 16)
#define MSDC0_DRV2_DAT6_MASK   (0xf  << 12)
#define MSDC0_DRV2_DAT5_MASK   (0xf  << 8)
#define MSDC0_DRV2_DAT4_MASK   (0xf  << 4)
#define MSDC0_DRV2_DAT3_MASK   (0xf  << 0)
#define MSDC0_DRV2_ALL_MASK    (0xffffff << 0)

/* MSDC0 PUPD mask */
#define MSDC0_PUPD0_MASK        (0xFFFFFFFF << 0)
#define MSDC0_PUPD1_MASK        (0xFFFF << 0)

#define IES_ENABLE		1
#define IES_DISABLE		0

#define SMT_ENABLE		1
#define SMT_DISABLE		0

#define IO_DRV_2MA		0
#define IO_DRV_4MA		1
#define IO_DRV_6MA		2
#define IO_DRV_8MA		3
#define IO_DRV_10MA		4
#define IO_DRV_12MA		5
#define IO_DRV_14MA		6
#define IO_DRV_16MA		7

/**************************************************************/
/* Section 5: Adjustable Driver Parameter                     */
/**************************************************************/
#define HOST_MAX_BLKSZ          (2048)

#define MSDC_OCR_AVAIL          (MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 \
				| MMC_VDD_31_32 | MMC_VDD_32_33)
/* data timeout counter. 1048576 * 3 sclk. */
#define DEFAULT_DTOC            (3)

#define MAX_DMA_CNT             (64 * 1024 - 512)
				/* a WIFI transaction may be 50K */
#define MAX_DMA_CNT_SDIO        (0xFFFFFFFF - 255)
				/* a LTE  transaction may be 128K */

#define MAX_HW_SGMTS            (MAX_BD_NUM)
#define MAX_PHY_SGMTS           (MAX_BD_NUM)
#define MAX_SGMT_SZ             (MAX_DMA_CNT)
#define MAX_SGMT_SZ_SDIO        (MAX_DMA_CNT_SDIO)

#define HOST_MAX_NUM            (1)
#ifdef CONFIG_PWR_LOSS_MTK_TEST
#define MAX_REQ_SZ              (512 * 65536)
#else
#define MAX_REQ_SZ              (512 * 1024)
#endif

#ifdef CONFIG_MACH_FPGA
#define HOST_MAX_MCLK           (200000000)
#else
#define HOST_MAX_MCLK           (200000000)
#endif
#define HOST_MIN_MCLK           (260000)

#endif /* _MSDC_CUST_MT3611_H_ */
