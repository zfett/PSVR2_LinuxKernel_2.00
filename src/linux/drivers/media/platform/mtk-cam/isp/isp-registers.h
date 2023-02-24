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

#ifndef __ISP_REGISTERS_H
#define __ISP_REGISTERS_H

#define ISP_REGISTER_WIDTH 0x04

/**
 *    CAM interrupt status
 */
/* normal siganl */
#define VS_INT_ST           (1L<<0)
#define TG_INT1_ST          (1L<<1)
#define TG_INT2_ST          (1L<<2)
#define EXPDON_ST           (1L<<3)
#define HW_PASS1_DON_ST     (1L<<11)
#define SOF_INT_ST          (1L<<12)
#define SW_PASS1_DON_ST     (1L<<30)
/* err status */
#define TG_ERR_ST           (1L<<4)
#define TG_GBERR_ST         (1L<<5)
#define CQ_CODE_ERR_ST      (1L<<6)
#define CQ_APB_ERR_ST       (1L<<7)
#define CQ_VS_ERR_ST        (1L<<8)
#define RRZO_ERR_ST         (1L<<18)
#define IMGO_ERR_ST         (1L<<20)
#define AAO_ERR_ST          (1L<<21)
#define BNR_ERR_ST          (1L<<24)
#define LSC_ERR_ST          (1L<<25)
#define DMA_ERR_ST          (1L<<29)
/**
 *    CAM DMA done status
 */
#define IMGO_DONE_ST        (1L<<0)
#define RRZO_DONE_ST        (1L<<2)
#define AAO_DONE_ST         (1L<<7)
#define BPCI_DONE_ST        (1L<<9)
#define LSCI_DONE_ST        (1L<<10)

/**
 *    CAMSV interrupt status
 */
/* normal signal */
#define SV_VS1_ST           (1L<<0)
#define SV_TG_ST1           (1L<<1)
#define SV_TG_ST2           (1L<<2)
#define SV_EXPDON_ST        (1L<<3)
#define SV_SOF_INT_ST       (1L<<7)
#define SV_HW_PASS1_DON_ST  (1L<<10)
#define SV_SW_PASS1_DON_ST  (1L<<20)
/* err status */
#define SV_TG_ERR           (1L<<4)
#define SV_TG_GBERR         (1L<<5)
#define SV_IMGO_ERR         (1L<<16)
#define SV_IMGO_OVERRUN     (1L<<17)

/**
 *    BE interrupt status
 */
/* normal signal */
#define BE_EOT_INT_ST       (1L<<0)
#define BE_SOF_INT_ST       (1L<<1)
#define BE_DONE_ST          (1L<<2)
/* error status */
#define BE_LABEL_ERR        (1L<<4)
#define BE_MERGE_STACK_ERR  (1L<<5)
#define BE_DATA_TABEL_ERR   (1L<<6)

#define CAM_REG_CTL_RAW_INT_EN	0x0020
#define CAM_REG_CTL_RAW_INT_STATUS 0x0024
#define CAM_REG_CTL_RAW_INT_STATUSX	0x0028
#define CAM_REG_CTL_RAW_INT2_EN 0x0030
#define CAM_REG_CTL_RAW_INT2_STATUS 0x0034
#define CAM_REG_CTL_RAW_INT2_STATUSX 0x0038
#define CAM_REG_SPECIAL_FUN_EN 0x0218
#define CAM_REG_CTL_SW_CTL 0x0040
#define CAM_REG_CTL_SW_PASS1_DONE 0x005C
#define CAM_REG_IMGO_ERR_STAT 0x0444
#define CAM_REG_RRZO_ERR_STAT 0x0448
#define CAM_REG_AAO_ERR_STAT 0x044C
#define CAM_REG_PSO_FH_SPARE_2 0x0DB0
#define CAM_REG_PSO_FH_SPARE_11 0x0DD4
#define CAM_REG_AAO_FH_SPARE_2 0x0EB0
#define CAM_REG_IMGO_CON 0x023C
#define CAM_REG_IMGO_CON2 0x0240
#define CAM_REG_IMGO_CON3 0x0244
#define CAM_REG_RRZO_CON 0x026C
#define CAM_REG_RRZO_CON2 0x0270
#define CAM_REG_RRZO_CON3 0x0274
#define CAM_REG_AAO_CON 0x029C
#define CAM_REG_AAO_CON2 0x02A0
#define CAM_REG_AAO_CON3 0x02A4
#define CAM_REG_PSO_CON 0x0D9C
#define CAM_REG_PSO_CON2 0x0DA0
#define CAM_REG_PSO_CON3 0x0DA4

#define CAM_UNI_REG_TOP_SW_CTL 0x0008

#define CAMSV_REG_CTL_INT_STATUS 0x01c
#define CAMSV_REG_CTL_SW_CTL 0x020
#define CAMSV_REG_SPARE0 0x024
#define CAMSV_REG_SPARE1 0x028
#define CAMSV_FBC_IMGO_ENQ_ADDR 0x118
#define CAMSV_IMGO_BASE_ADDR 0x220

#define DIP_REG_CTL_INT_STATUS 0x0030
#define DIP_REG_CTL_CQ_INT_STATUS 0x0034
#define DIP_REG_CTL_CQ_INT2_STATUS 0x0038
#define DIP_REG_CTL_CQ_INT3_STATUS 0x003C
#define DIP_REG_CTL_SW_CTL 0x006C
#define DIP_REG_IMG3O_ERR_STAT 0x064C
#define DIP_REG_IMGI_ERR_STAT 0x0660
#define DIP_REG_IMG3O_CON 0x02AC
#define DIP_REG_IMG3O_CON2 0x02B0
#define DIP_REG_IMG3O_CON3 0x02B4

/* BE registers */
#define BE_REG_CTL_CFG 0x0010
#define BE_REG_CTL_SW_RST 0x0014
#define BE_REG_CTL_DB_CFG 0x01C
#define BE_REG_CTL_INT_EN 0x0028
#define BE_REG_CTL_INT_STATUS 0x002C
#define BE_REG_CTL_INT_STATUSX 0x0030
#define BE_REG_CTL_STATUS 0X0034
#define BE_WDMA_CTRL0 0x0088
#define BE_WDMA_CTRL1 0x008C
#define BE_WDMA_CTRL2 0x0090
#define BE_REG_CTL_WDMA_STATUS 0x00B8
#define BE_SW_REG_00  0x0100
#define BE_SW_REG_01  0x0104
#define BE_SW_REG_02  0x0108
#define BE_SW_REG_03  0x010C
#define BE_SW_REG_04  0x0110
#define BE_SW_REG_05  0x0114
#define BE_SW_REG_06  0x0118
#define BE_SW_REG_07  0x011C
#define BE_SW_REG_08  0x0120
#define BE_SW_REG_09  0x0124
#define BE_SW_REG_10  0x0128
#define BE_SW_REG_11  0x012C
#define BE_SW_REG_12  0x0130
#define BE_SW_REG_13  0x0134
#define BE_SW_REG_14  0x0138

/* cam top registers */
#define CAMSYS_REG_CG_CON           0x0
#define CAMSYS_REG_CG_SET           0x4
#define CAMSYS_REG_CG_CLR           0x8
#define CAMSYS_REG_SW_RST           0xC
#define CAMSYS_REG_BE_CTL           0x1B0


/* dip top registers */
#define IMGSYS_REG_CG_CON          0x0
#define IMGSYS_REG_CG_SET          0x4
#define IMGSYS_REG_CG_CLR          0x8
#define IMGSYS_REG_SW_RST          0xC

#define RBFC_RSTCTL 0x054
#define RBFC_DCM_CON 0x0A0

/* CSI registers */
#define MIPI_REG_ANA08_CSI0A                 0x8
#define MIPI_REG_ANA0C_CSI0A                 0xC
#define MIPI_REG_ANA10_CSI0A                0x10

#endif /* __ISP_REGISTERS_H */
