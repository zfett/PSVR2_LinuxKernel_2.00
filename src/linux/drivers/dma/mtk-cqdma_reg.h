#ifndef __CQ_DMA_REG_H__
#define __CQ_DMA_REG_H__

/* ----------------- Register Definitions ------------------- */
#define CQ_DMA_G_DMA_INT_FLAG				0x00000000
	#define FLAG					BIT(0)
#define CQ_DMA_G_DMA_INT_EN				0x00000004
	#define INTEN					BIT(0)
#define CQ_DMA_G_DMA_EN					0x00000008
	#define EN					BIT(0)
#define CQ_DMA_G_DMA_RST				0x0000000c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define CQ_DMA_G_DMA_STOP				0x00000010
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define CQ_DMA_G_DMA_CON				0x00000018
	#define FIX_EN					BIT(1)
	#define SLOW_EN					BIT(2)
	#define WADDR_FIX_EN				BIT(3)
	#define RADDR_FIX_EN				BIT(4)
	#define SLOW_CNT				GENMASK(14, 5)
	#define WRAP_EN					BIT(15)
	#define BURST_LEN				GENMASK(18, 16)
	#define WRAP_SEL				BIT(20)
	#define WSIZE					GENMASK(25, 24)
	#define RSIZE					GENMASK(29, 28)
	#define FLAG_ST					BIT(30)
#define CQ_DMA_G_DMA_SRC_ADDR				0x0000001c
	#define SRC_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_DST_ADDR				0x00000020
	#define DST_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_LEN1				0x00000024
	#define LEN1					GENMASK(19, 0)
#define CQ_DMA_G_DMA_LEN2				0x00000028
	#define LEN2					GENMASK(19, 0)
#define CQ_DMA_G_DMA_JUMP_ADDR				0x0000002c
	#define JUMP_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_INT_BUF_SIZE			0x00000030
	#define INT_BUF_SIZE				GENMASK(7, 0)
#define CQ_DMA_G_DMA_SEC_EN				0x0000003c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(5, 1)
	#define DLOCK					BIT(30)
	#define LOCK					BIT(31)
#define CQ_DMA_G_DMA_APB_LATADDR			0x00000040
	#define LAT_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_ABORT				0x00000044
	#define APB_ABORT				BIT(24)
	#define W_VID					BIT(28)
	#define R_VID					BIT(29)
	#define CLR					BIT(31)
#define CQ_DMA_G_DMA_DCM_EN				0x00000048
	#define DCM_EN					BIT(0)
#define CQ_DMA_G_DMA_SRC_ADDR2				0x00000060
	#define SRC_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_DST_ADDR2				0x00000064
	#define DST_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_JUMP_ADDR2				0x00000068
	#define JUMP_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_1_INT_FLAG				0x00000080
	#define FLAG					BIT(0)
#define CQ_DMA_G_DMA_1_INT_EN				0x00000084
	#define INTEN					BIT(0)
#define CQ_DMA_G_DMA_1_EN				0x00000088
	#define EN					BIT(0)
#define CQ_DMA_G_DMA_1_RST				0x0000008c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define CQ_DMA_G_DMA_1_STOP				0x00000090
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define CQ_DMA_G_DMA_1_CON				0x00000098
	#define FIX_EN					BIT(1)
	#define SLOW_EN					BIT(2)
	#define WADDR_FIX_EN				BIT(3)
	#define RADDR_FIX_EN				BIT(4)
	#define SLOW_CNT				GENMASK(14, 5)
	#define WRAP_EN					BIT(15)
	#define BURST_LEN				GENMASK(18, 16)
	#define WRAP_SEL				BIT(20)
	#define WSIZE					GENMASK(25, 24)
	#define RSIZE					GENMASK(29, 28)
	#define FLAG_ST					BIT(30)
#define CQ_DMA_G_DMA_1_SRC_ADDR				0x0000009c
	#define SRC_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_1_DST_ADDR				0x000000a0
	#define DST_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_1_LEN1				0x000000a4
	#define LEN1					GENMASK(19, 0)
#define CQ_DMA_G_DMA_1_LEN2				0x000000a8
	#define LEN2					GENMASK(19, 0)
#define CQ_DMA_G_DMA_1_JUMP_ADDR			0x000000ac
	#define JUMP_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_1_INT_BUF_SIZE			0x000000b0
	#define INT_BUF_SIZE				GENMASK(7, 0)
#define CQ_DMA_G_DMA_1_SEC_EN				0x000000bc
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(5, 1)
	#define DLOCK					BIT(30)
	#define LOCK					BIT(31)
#define CQ_DMA_G_DMA_1_APB_LATADDR			0x000000c0
	#define LAT_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_1_ABORT				0x000000c4
	#define APB_ABORT				BIT(24)
	#define W_VID					BIT(28)
	#define R_VID					BIT(29)
	#define CLR					BIT(31)
#define CQ_DMA_G_DMA_1_DCM_EN				0x000000c8
	#define DCM_EN					BIT(0)
#define CQ_DMA_G_DMA_1_SRC_ADDR2			0x000000e0
	#define SRC_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_1_DST_ADDR2			0x000000e4
	#define DST_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_1_JUMP_ADDR2			0x000000e8
	#define JUMP_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_2_INT_FLAG				0x00000100
	#define FLAG					BIT(0)
#define CQ_DMA_G_DMA_2_INT_EN				0x00000104
	#define INTEN					BIT(0)
#define CQ_DMA_G_DMA_2_EN				0x00000108
	#define EN					BIT(0)
#define CQ_DMA_G_DMA_2_RST				0x0000010c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define CQ_DMA_G_DMA_2_STOP				0x00000110
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define CQ_DMA_G_DMA_2_CON				0x00000118
	#define FIX_EN					BIT(1)
	#define SLOW_EN					BIT(2)
	#define WADDR_FIX_EN				BIT(3)
	#define RADDR_FIX_EN				BIT(4)
	#define SLOW_CNT				GENMASK(14, 5)
	#define WRAP_EN					BIT(15)
	#define BURST_LEN				GENMASK(18, 16)
	#define WRAP_SEL				BIT(20)
	#define WSIZE					GENMASK(25, 24)
	#define RSIZE					GENMASK(29, 28)
	#define FLAG_ST					BIT(30)
#define CQ_DMA_G_DMA_2_SRC_ADDR				0x0000011c
	#define SRC_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_2_DST_ADDR				0x00000120
	#define DST_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_2_LEN1				0x00000124
	#define LEN1					GENMASK(19, 0)
#define CQ_DMA_G_DMA_2_LEN2				0x00000128
	#define LEN2					GENMASK(19, 0)
#define CQ_DMA_G_DMA_2_JUMP_ADDR			0x0000012c
	#define JUMP_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_2_INT_BUF_SIZE			0x00000130
	#define INT_BUF_SIZE				GENMASK(7, 0)
#define CQ_DMA_G_DMA_2_SEC_EN				0x0000013c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(5, 1)
	#define DLOCK					BIT(30)
	#define LOCK					BIT(31)
#define CQ_DMA_G_DMA_2_APB_LATADDR			0x00000140
	#define LAT_ADDR				GENMASK(31, 0)
#define CQ_DMA_G_DMA_2_ABORT				0x00000144
	#define APB_ABORT				BIT(24)
	#define W_VID					BIT(28)
	#define R_VID					BIT(29)
	#define CLR					BIT(31)
#define CQ_DMA_G_DMA_2_DCM_EN				0x00000148
	#define DCM_EN					BIT(0)
#define CQ_DMA_G_DMA_2_SRC_ADDR2			0x00000160
	#define SRC_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_2_DST_ADDR2			0x00000164
	#define DST_ADDR2				GENMASK(3, 0)
#define CQ_DMA_G_DMA_2_JUMP_ADDR2			0x00000168
	#define JUMP_ADDR2				GENMASK(3, 0)

#endif /*__CQ_DMA_REG_H__*/
