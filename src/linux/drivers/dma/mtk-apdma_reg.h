#ifndef __AP_DMA_REG_H__
#define __AP_DMA_REG_H__

/* ----------------- Register Definitions ------------------- */
#define AP_DMA_GLOBAL_INT_FLAG				0x00000000
	#define I2C0					BIT(0)
	#define I2C1					BIT(1)
	#define I2C2					BIT(2)
	#define I2C3					BIT(3)
	#define I2C4					BIT(4)
	#define I2C5					BIT(5)
	#define I2C6					BIT(6)
	#define I2C7					BIT(7)
	#define I2C8					BIT(8)
	#define I2C9					BIT(9)
	#define I2C10					BIT(10)
	#define I2C11					BIT(11)
	#define UART0_TX				BIT(12)
	#define UART0_RX				BIT(13)
	#define UART1_TX				BIT(14)
	#define UART1_RX				BIT(15)
	#define UART2_TX				BIT(16)
	#define UART2_RX				BIT(17)
	#define UART3_TX				BIT(18)
	#define UART3_RX				BIT(19)
	#define UART4_TX				BIT(20)
	#define UART4_RX				BIT(21)
#define AP_DMA_GLOBAL_RST				0x00000004
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_GLOBAL_RUNNING_STATUS			0x00000008
	#define I2C0					BIT(0)
	#define I2C1					BIT(1)
	#define I2C2					BIT(2)
	#define I2C3					BIT(3)
	#define I2C4					BIT(4)
	#define I2C5					BIT(5)
	#define I2C6					BIT(6)
	#define I2C7					BIT(7)
	#define I2C8					BIT(8)
	#define I2C9					BIT(9)
	#define I2C10					BIT(10)
	#define I2C11					BIT(11)
	#define UART0_TX				BIT(12)
	#define UART0_RX				BIT(13)
	#define UART1_TX				BIT(14)
	#define UART1_RX				BIT(15)
	#define UART2_TX				BIT(16)
	#define UART2_RX				BIT(17)
	#define UART3_TX				BIT(18)
	#define UART3_RX				BIT(19)
	#define UART4_TX				BIT(20)
	#define UART4_RX				BIT(21)
#define AP_DMA_GLOBAL_SLOW_DOWN				0x0000000c
	#define R_SLOW_EN				BIT(0)
	#define R_SLOW_CNT				GENMASK(15, 6)
	#define W_SLOW_EN				BIT(16)
	#define W_SLOW_CNT				GENMASK(31, 22)
#define AP_DMA_GLOBAL_SEC_EN				0x00000010
	#define I2C0					BIT(0)
	#define I2C1					BIT(1)
	#define I2C2					BIT(2)
	#define I2C3					BIT(3)
	#define I2C4					BIT(4)
	#define I2C5					BIT(5)
	#define I2C6					BIT(6)
	#define I2C7					BIT(7)
	#define I2C8					BIT(8)
	#define I2C9					BIT(9)
	#define I2C10					BIT(10)
	#define I2C11					BIT(11)
	#define UART0_TX				BIT(12)
	#define UART0_RX				BIT(13)
	#define UART1_TX				BIT(14)
	#define UART1_RX				BIT(15)
	#define UART2_TX				BIT(16)
	#define UART2_RX				BIT(17)
	#define UART3_TX				BIT(18)
	#define UART3_RX				BIT(19)
	#define UART4_TX				BIT(20)
	#define UART4_RX				BIT(21)
	#define DLOCK					BIT(30)
	#define LOCK					BIT(31)
#define AP_DMA_GLOBAL_GSEC_EN				0x00000014
	#define GSEC_EN					BIT(0)
	#define GLOCK					BIT(31)
#define AP_DMA_GLOBAL_VIO_DBG1				0x00000018
	#define LAT_ADDR				GENMASK(31, 0)
#define AP_DMA_GLOBAL_VIO_DBG0				0x0000001c
	#define W_VID_SEC				BIT(28)
	#define R_VID_SEC				BIT(29)
	#define CLR					BIT(31)
#define AP_DMA_I2C0_SEC_EN				0x00000020
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C1_SEC_EN				0x00000024
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C2_SEC_EN				0x00000028
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C3_SEC_EN				0x0000002c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C4_SEC_EN				0x00000030
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C5_SEC_EN				0x00000034
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C6_SEC_EN				0x00000038
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C7_SEC_EN				0x0000003c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C8_SEC_EN				0x00000040
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C9_SEC_EN				0x00000044
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C10_SEC_EN				0x00000048
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C11_SEC_EN				0x0000004c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART0_TX_SEC_EN				0x00000050
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART0_RX_SEC_EN				0x00000054
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART1_TX_SEC_EN				0x00000058
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART1_RX_SEC_EN				0x0000005c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART2_TX_SEC_EN				0x00000060
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART2_RX_SEC_EN				0x00000064
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART3_TX_SEC_EN				0x00000068
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART3_RX_SEC_EN				0x0000006c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART4_TX_SEC_EN				0x00000070
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART4_RX_SEC_EN				0x00000074
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_0_INT_FLAG				0x00000080
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_0_INT_EN				0x00000084
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_0_EN					0x00000088
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_0_RST				0x0000008c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_0_STOP				0x00000090
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_0_FLUSH				0x00000094
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_0_CON				0x00000098
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_0_TX_MEM_ADDR			0x0000009c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_0_RX_MEM_ADDR			0x000000a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_0_TX_LEN				0x000000a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_0_RX_LEN				0x000000a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_0_INT_BUF_SIZE			0x000000b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_0_TX_MEM_ADDR2			0x000000d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_0_RX_MEM_ADDR2			0x000000d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_1_INT_FLAG				0x00000100
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_1_INT_EN				0x00000104
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_1_EN					0x00000108
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_1_RST				0x0000010c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_1_STOP				0x00000110
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_1_FLUSH				0x00000114
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_1_CON				0x00000118
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_1_TX_MEM_ADDR			0x0000011c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_1_RX_MEM_ADDR			0x00000120
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_1_TX_LEN				0x00000124
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_1_RX_LEN				0x00000128
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_1_INT_BUF_SIZE			0x00000138
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_1_TX_MEM_ADDR2			0x00000154
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_1_RX_MEM_ADDR2			0x00000158
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_2_INT_FLAG				0x00000180
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_2_INT_EN				0x00000184
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_2_EN					0x00000188
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_2_RST				0x0000018c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_2_STOP				0x00000190
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_2_FLUSH				0x00000194
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_2_CON				0x00000198
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_2_TX_MEM_ADDR			0x0000019c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_2_RX_MEM_ADDR			0x000001a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_2_TX_LEN				0x000001a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_2_RX_LEN				0x000001a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_2_INT_BUF_SIZE			0x000001b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_2_TX_MEM_ADDR2			0x000001d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_2_RX_MEM_ADDR2			0x000001d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_3_INT_FLAG				0x00000200
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_3_INT_EN				0x00000204
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_3_EN					0x00000208
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_3_RST				0x0000020c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_3_STOP				0x00000210
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_3_FLUSH				0x00000214
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_3_CON				0x00000218
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_3_TX_MEM_ADDR			0x0000021c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_3_RX_MEM_ADDR			0x00000220
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_3_TX_LEN				0x00000224
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_3_RX_LEN				0x00000228
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_3_INT_BUF_SIZE			0x00000238
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_3_TX_MEM_ADDR2			0x00000254
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_3_RX_MEM_ADDR2			0x00000258
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_4_INT_FLAG				0x00000280
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_4_INT_EN				0x00000284
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_4_EN					0x00000288
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_4_RST				0x0000028c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_4_STOP				0x00000290
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_4_FLUSH				0x00000294
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_4_CON				0x00000298
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_4_TX_MEM_ADDR			0x0000029c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_4_RX_MEM_ADDR			0x000002a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_4_TX_LEN				0x000002a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_4_RX_LEN				0x000002a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_4_INT_BUF_SIZE			0x000002b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_4_TX_MEM_ADDR2			0x000002d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_4_RX_MEM_ADDR2			0x000002d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_5_INT_FLAG				0x00000300
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_5_INT_EN				0x00000304
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_5_EN					0x00000308
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_5_RST				0x0000030c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_5_STOP				0x00000310
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_5_FLUSH				0x00000314
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_5_CON				0x00000318
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_5_TX_MEM_ADDR			0x0000031c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_5_RX_MEM_ADDR			0x00000320
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_5_TX_LEN				0x00000324
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_5_RX_LEN				0x00000328
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_5_INT_BUF_SIZE			0x00000338
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_5_TX_MEM_ADDR2			0x00000354
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_5_RX_MEM_ADDR2			0x00000358
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_6_INT_FLAG				0x00000380
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_6_INT_EN				0x00000384
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_6_EN					0x00000388
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_6_RST				0x0000038c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_6_STOP				0x00000390
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_6_FLUSH				0x00000394
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_6_CON				0x00000398
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_6_TX_MEM_ADDR			0x0000039c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_6_RX_MEM_ADDR			0x000003a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_6_TX_LEN				0x000003a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_6_RX_LEN				0x000003a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_6_INT_BUF_SIZE			0x000003b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_6_TX_MEM_ADDR2			0x000003d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_6_RX_MEM_ADDR2			0x000003d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_7_INT_FLAG				0x00000400
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_7_INT_EN				0x00000404
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_7_EN					0x00000408
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_7_RST				0x0000040c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_7_STOP				0x00000410
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_7_FLUSH				0x00000414
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_7_CON				0x00000418
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_7_TX_MEM_ADDR			0x0000041c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_7_RX_MEM_ADDR			0x00000420
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_7_TX_LEN				0x00000424
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_7_RX_LEN				0x00000428
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_7_INT_BUF_SIZE			0x00000438
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_7_TX_MEM_ADDR2			0x00000454
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_7_RX_MEM_ADDR2			0x00000458
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_8_INT_FLAG				0x00000480
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_8_INT_EN				0x00000484
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_8_EN					0x00000488
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_8_RST				0x0000048c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_8_STOP				0x00000490
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_8_FLUSH				0x00000494
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_8_CON				0x00000498
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_8_TX_MEM_ADDR			0x0000049c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_8_RX_MEM_ADDR			0x000004a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_8_TX_LEN				0x000004a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_8_RX_LEN				0x000004a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_8_INT_BUF_SIZE			0x000004b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_8_TX_MEM_ADDR2			0x000004d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_8_RX_MEM_ADDR2			0x000004d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_9_INT_FLAG				0x00000500
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_9_INT_EN				0x00000504
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_9_EN					0x00000508
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_9_RST				0x0000050c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_9_STOP				0x00000510
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_9_FLUSH				0x00000514
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_9_CON				0x00000518
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_9_TX_MEM_ADDR			0x0000051c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_9_RX_MEM_ADDR			0x00000520
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_9_TX_LEN				0x00000524
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_9_RX_LEN				0x00000528
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_9_INT_BUF_SIZE			0x00000538
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_9_TX_MEM_ADDR2			0x00000554
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_9_RX_MEM_ADDR2			0x00000558
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_10_INT_FLAG				0x00000580
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_10_INT_EN				0x00000584
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_10_EN				0x00000588
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_10_RST				0x0000058c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_10_STOP				0x00000590
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_10_FLUSH				0x00000594
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_10_CON				0x00000598
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_10_TX_MEM_ADDR			0x0000059c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_10_RX_MEM_ADDR			0x000005a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_10_TX_LEN				0x000005a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_10_RX_LEN				0x000005a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_10_INT_BUF_SIZE			0x000005b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_10_TX_MEM_ADDR2			0x000005d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_10_RX_MEM_ADDR2			0x000005d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_11_INT_FLAG				0x00000600
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_11_INT_EN				0x00000604
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_11_EN				0x00000608
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_11_RST				0x0000060c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_11_STOP				0x00000610
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_11_FLUSH				0x00000614
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_11_CON				0x00000618
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_11_TX_MEM_ADDR			0x0000061c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_11_RX_MEM_ADDR			0x00000620
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_11_TX_LEN				0x00000624
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_11_RX_LEN				0x00000628
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_11_INT_BUF_SIZE			0x00000638
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_11_TX_MEM_ADDR2			0x00000654
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_11_RX_MEM_ADDR2			0x00000658
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_0_TX_INT_FLAG			0x00000680
	#define FLAG0					BIT(0)
#define AP_DMA_UART_0_TX_INT_EN				0x00000684
	#define INTEN					BIT(0)
#define AP_DMA_UART_0_TX_EN				0x00000688
	#define EN					BIT(0)
#define AP_DMA_UART_0_TX_RST				0x0000068c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_0_TX_STOP				0x00000690
	#define STOP					BIT(0)
#define AP_DMA_UART_0_TX_FLUSH				0x00000694
	#define FLUSH					BIT(0)
#define AP_DMA_UART_0_TX_VFF_ADDR			0x0000069c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_0_TX_VFF_LEN			0x000006a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_0_TX_VFF_THRE			0x000006a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_0_TX_VFF_WPT			0x000006ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_0_TX_VFF_RPT			0x000006b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_0_TX_INT_BUF_SIZE			0x000006b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_0_TX_VFF_VALID_SIZE			0x000006bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_0_TX_VFF_LEFT_SIZE			0x000006c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_0_TX_VFF_ADDR2			0x000006d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_0_RX_INT_FLAG			0x00000700
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_0_RX_INT_EN				0x00000704
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_0_RX_EN				0x00000708
	#define EN					BIT(0)
#define AP_DMA_UART_0_RX_RST				0x0000070c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_0_RX_STOP				0x00000710
	#define STOP					BIT(0)
#define AP_DMA_UART_0_RX_FLUSH				0x00000714
	#define FLUSH					BIT(0)
#define AP_DMA_UART_0_RX_VFF_ADDR			0x0000071c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_0_RX_VFF_LEN			0x00000724
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_0_RX_VFF_THRE			0x00000728
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_0_RX_VFF_WPT			0x0000072c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_0_RX_VFF_RPT			0x00000730
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_0_RX_FLOW_CTRL_THRE			0x00000734
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_0_RX_INT_BUF_SIZE			0x00000738
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_0_RX_VFF_VALID_SIZE			0x0000073c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_0_RX_VFF_LEFT_SIZE			0x00000740
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_0_RX_VFF_ADDR2			0x00000754
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_1_TX_INT_FLAG			0x00000780
	#define FLAG0					BIT(0)
#define AP_DMA_UART_1_TX_INT_EN				0x00000784
	#define INTEN					BIT(0)
#define AP_DMA_UART_1_TX_EN				0x00000788
	#define EN					BIT(0)
#define AP_DMA_UART_1_TX_RST				0x0000078c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_1_TX_STOP				0x00000790
	#define STOP					BIT(0)
#define AP_DMA_UART_1_TX_FLUSH				0x00000794
	#define FLUSH					BIT(0)
#define AP_DMA_UART_1_TX_VFF_ADDR			0x0000079c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_1_TX_VFF_LEN			0x000007a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_1_TX_VFF_THRE			0x000007a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_1_TX_VFF_WPT			0x000007ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_1_TX_VFF_RPT			0x000007b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_1_TX_INT_BUF_SIZE			0x000007b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_1_TX_VFF_VALID_SIZE			0x000007bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_1_TX_VFF_LEFT_SIZE			0x000007c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_1_TX_VFF_ADDR2			0x000007d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_1_RX_INT_FLAG			0x00000800
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_1_RX_INT_EN				0x00000804
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_1_RX_EN				0x00000808
	#define EN					BIT(0)
#define AP_DMA_UART_1_RX_RST				0x0000080c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_1_RX_STOP				0x00000810
	#define STOP					BIT(0)
#define AP_DMA_UART_1_RX_FLUSH				0x00000814
	#define FLUSH					BIT(0)
#define AP_DMA_UART_1_RX_VFF_ADDR			0x0000081c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_1_RX_VFF_LEN			0x00000824
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_1_RX_VFF_THRE			0x00000828
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_1_RX_VFF_WPT			0x0000082c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_1_RX_VFF_RPT			0x00000830
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_1_RX_FLOW_CTRL_THRE			0x00000834
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_1_RX_INT_BUF_SIZE			0x00000838
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_1_RX_VFF_VALID_SIZE			0x0000083c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_1_RX_VFF_LEFT_SIZE			0x00000840
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_1_RX_VFF_ADDR2			0x00000854
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_2_TX_INT_FLAG			0x00000880
	#define FLAG0					BIT(0)
#define AP_DMA_UART_2_TX_INT_EN				0x00000884
	#define INTEN					BIT(0)
#define AP_DMA_UART_2_TX_EN				0x00000888
	#define EN					BIT(0)
#define AP_DMA_UART_2_TX_RST				0x0000088c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_2_TX_STOP				0x00000890
	#define STOP					BIT(0)
#define AP_DMA_UART_2_TX_FLUSH				0x00000894
	#define FLUSH					BIT(0)
#define AP_DMA_UART_2_TX_VFF_ADDR			0x0000089c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_2_TX_VFF_LEN			0x000008a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_2_TX_VFF_THRE			0x000008a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_2_TX_VFF_WPT			0x000008ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_2_TX_VFF_RPT			0x000008b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_2_TX_INT_BUF_SIZE			0x000008b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_2_TX_VFF_VALID_SIZE			0x000008bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_2_TX_VFF_LEFT_SIZE			0x000008c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_2_TX_VFF_ADDR2			0x000008d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_2_RX_INT_FLAG			0x00000900
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_2_RX_INT_EN				0x00000904
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_2_RX_EN				0x00000908
	#define EN					BIT(0)
#define AP_DMA_UART_2_RX_RST				0x0000090c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_2_RX_STOP				0x00000910
	#define STOP					BIT(0)
#define AP_DMA_UART_2_RX_FLUSH				0x00000914
	#define FLUSH					BIT(0)
#define AP_DMA_UART_2_RX_VFF_ADDR			0x0000091c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_2_RX_VFF_LEN			0x00000924
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_2_RX_VFF_THRE			0x00000928
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_2_RX_VFF_WPT			0x0000092c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_2_RX_VFF_RPT			0x00000930
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_2_RX_FLOW_CTRL_THRE			0x00000934
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_2_RX_INT_BUF_SIZE			0x00000938
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_2_RX_VFF_VALID_SIZE			0x0000093c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_2_RX_VFF_LEFT_SIZE			0x00000940
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_2_RX_VFF_ADDR2			0x00000954
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_3_TX_INT_FLAG			0x00000980
	#define FLAG0					BIT(0)
#define AP_DMA_UART_3_TX_INT_EN				0x00000984
	#define INTEN					BIT(0)
#define AP_DMA_UART_3_TX_EN				0x00000988
	#define EN					BIT(0)
#define AP_DMA_UART_3_TX_RST				0x0000098c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_3_TX_STOP				0x00000990
	#define STOP					BIT(0)
#define AP_DMA_UART_3_TX_FLUSH				0x00000994
	#define FLUSH					BIT(0)
#define AP_DMA_UART_3_TX_VFF_ADDR			0x0000099c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_3_TX_VFF_LEN			0x000009a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_3_TX_VFF_THRE			0x000009a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_3_TX_VFF_WPT			0x000009ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_3_TX_VFF_RPT			0x000009b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_3_TX_INT_BUF_SIZE			0x000009b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_3_TX_VFF_VALID_SIZE			0x000009bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_3_TX_VFF_LEFT_SIZE			0x000009c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_3_TX_VFF_ADDR2			0x000009d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_3_RX_INT_FLAG			0x00000a00
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_3_RX_INT_EN				0x00000a04
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_3_RX_EN				0x00000a08
	#define EN					BIT(0)
#define AP_DMA_UART_3_RX_RST				0x00000a0c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_3_RX_STOP				0x00000a10
	#define STOP					BIT(0)
#define AP_DMA_UART_3_RX_FLUSH				0x00000a14
	#define FLUSH					BIT(0)
#define AP_DMA_UART_3_RX_VFF_ADDR			0x00000a1c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_3_RX_VFF_LEN			0x00000a24
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_3_RX_VFF_THRE			0x00000a28
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_3_RX_VFF_WPT			0x00000a2c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_3_RX_VFF_RPT			0x00000a30
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_3_RX_FLOW_CTRL_THRE			0x00000a34
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_3_RX_INT_BUF_SIZE			0x00000a38
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_3_RX_VFF_VALID_SIZE			0x00000a3c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_3_RX_VFF_LEFT_SIZE			0x00000a40
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_3_RX_VFF_ADDR2			0x00000a54
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_4_TX_INT_FLAG			0x00000a80
	#define FLAG0					BIT(0)
#define AP_DMA_UART_4_TX_INT_EN				0x00000a84
	#define INTEN					BIT(0)
#define AP_DMA_UART_4_TX_EN				0x00000a88
	#define EN					BIT(0)
#define AP_DMA_UART_4_TX_RST				0x00000a8c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_4_TX_STOP				0x00000a90
	#define STOP					BIT(0)
#define AP_DMA_UART_4_TX_FLUSH				0x00000a94
	#define FLUSH					BIT(0)
#define AP_DMA_UART_4_TX_VFF_ADDR			0x00000a9c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_4_TX_VFF_LEN			0x00000aa4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_4_TX_VFF_THRE			0x00000aa8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_4_TX_VFF_WPT			0x00000aac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_4_TX_VFF_RPT			0x00000ab0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_4_TX_INT_BUF_SIZE			0x00000ab8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_4_TX_VFF_VALID_SIZE			0x00000abc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_4_TX_VFF_LEFT_SIZE			0x00000ac0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_4_TX_VFF_ADDR2			0x00000ad4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_4_RX_INT_FLAG			0x00000b00
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_4_RX_INT_EN				0x00000b04
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_4_RX_EN				0x00000b08
	#define EN					BIT(0)
#define AP_DMA_UART_4_RX_RST				0x00000b0c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_4_RX_STOP				0x00000b10
	#define STOP					BIT(0)
#define AP_DMA_UART_4_RX_FLUSH				0x00000b14
	#define FLUSH					BIT(0)
#define AP_DMA_UART_4_RX_VFF_ADDR			0x00000b1c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_4_RX_VFF_LEN			0x00000b24
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_4_RX_VFF_THRE			0x00000b28
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_4_RX_VFF_WPT			0x00000b2c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_4_RX_VFF_RPT			0x00000b30
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_4_RX_FLOW_CTRL_THRE			0x00000b34
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_4_RX_INT_BUF_SIZE			0x00000b38
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_4_RX_VFF_VALID_SIZE			0x00000b3c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_4_RX_VFF_LEFT_SIZE			0x00000b40
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_4_RX_VFF_ADDR2			0x00000b54
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_GLOBAL_INT_FLAG_P			0x00001000
	#define I2C12					BIT(0)
	#define I2C13					BIT(1)
	#define I2C14					BIT(2)
	#define I2C15					BIT(3)
	#define I2C16					BIT(4)
	#define I2C_11_CCU				BIT(5)
	#define I2C_12_CCU				BIT(6)
	#define I2C_13_CCU				BIT(7)
	#define I2C_14_CCU				BIT(8)
	#define I2C_15_CCU				BIT(9)
	#define I2C_16_CCU				BIT(10)
	#define UART5_TX				BIT(12)
	#define UART5_RX				BIT(13)
	#define UART6_TX				BIT(14)
	#define UART6_RX				BIT(15)
	#define UART7_TX				BIT(16)
	#define UART7_RX				BIT(17)
	#define UART8_TX				BIT(18)
	#define UART8_RX				BIT(19)
	#define UART9_TX				BIT(20)
	#define UART9_RX				BIT(21)
#define AP_DMA_GLOBAL_RST_P				0x00001004
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_GLOBAL_RUNNING_STATUS_P			0x00001008
	#define I2C12					BIT(0)
	#define I2C13					BIT(1)
	#define I2C14					BIT(2)
	#define I2C15					BIT(3)
	#define I2C16					BIT(4)
	#define I2C_11_CCU				BIT(5)
	#define I2C_12_CCU				BIT(6)
	#define I2C_13_CCU				BIT(7)
	#define I2C_14_CCU				BIT(8)
	#define I2C_15_CCU				BIT(9)
	#define I2C_16_CCU				BIT(10)
	#define UART5_TX				BIT(12)
	#define UART5_RX				BIT(13)
	#define UART6_TX				BIT(14)
	#define UART6_RX				BIT(15)
	#define UART7_TX				BIT(16)
	#define UART7_RX				BIT(17)
	#define UART8_TX				BIT(18)
	#define UART8_RX				BIT(19)
	#define UART9_TX				BIT(20)
	#define UART9_RX				BIT(21)
#define AP_DMA_GLOBAL_SLOW_DOWN_P			0x0000100c
	#define R_SLOW_EN				BIT(0)
	#define R_SLOW_CNT				GENMASK(15, 6)
	#define W_SLOW_EN				BIT(16)
	#define W_SLOW_CNT				GENMASK(31, 22)
#define AP_DMA_GLOBAL_SEC_EN_P				0x00001010
	#define I2C12					BIT(0)
	#define I2C13					BIT(1)
	#define I2C14					BIT(2)
	#define I2C15					BIT(3)
	#define I2C16					BIT(4)
	#define I2C_11_CCU				BIT(5)
	#define I2C_12_CCU				BIT(6)
	#define I2C_13_CCU				BIT(7)
	#define I2C_14_CCU				BIT(8)
	#define I2C_15_CCU				BIT(9)
	#define I2C_16_CCU				BIT(10)
	#define UART5_TX				BIT(12)
	#define UART5_RX				BIT(13)
	#define UART6_TX				BIT(14)
	#define UART6_RX				BIT(15)
	#define UART7_TX				BIT(16)
	#define UART7_RX				BIT(17)
	#define UART8_TX				BIT(18)
	#define UART8_RX				BIT(19)
	#define UART9_TX				BIT(20)
	#define UART9_RX				BIT(21)
	#define DLOCK					BIT(30)
	#define LOCK					BIT(31)
#define AP_DMA_GLOBAL_GSEC_EN_P				0x00001014
	#define GSEC_EN					BIT(0)
	#define GLOCK					BIT(31)
#define AP_DMA_GLOBAL_VIO_DBG1_P			0x00001018
	#define LAT_ADDR				GENMASK(31, 0)
#define AP_DMA_GLOBAL_VIO_DBG0_P			0x0000101c
	#define W_VID_SEC				BIT(28)
	#define R_VID_SEC				BIT(29)
	#define CLR					BIT(31)
#define AP_DMA_I2C12_SEC_EN				0x00001020
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C13_SEC_EN				0x00001024
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C14_SEC_EN				0x00001028
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C15_SEC_EN				0x0000102c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C16_SEC_EN				0x00001030
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_11_CCU_SEC_EN			0x00001034
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_12_CCU_SEC_EN			0x00001038
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_13_CCU_SEC_EN			0x0000103c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_14_CCU_SEC_EN			0x00001040
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_15_CCU_SEC_EN			0x00001044
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_16_CCU_SEC_EN			0x00001048
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART5_TX_SEC_EN				0x00001050
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART5_RX_SEC_EN				0x00001054
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART6_TX_SEC_EN				0x00001058
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART6_RX_SEC_EN				0x0000105c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART7_TX_SEC_EN				0x00001060
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART7_RX_SEC_EN				0x00001064
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART8_TX_SEC_EN				0x00001068
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART8_RX_SEC_EN				0x0000106c
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART9_TX_SEC_EN				0x00001070
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_UART9_RX_SEC_EN				0x00001074
	#define SEC_EN					BIT(0)
	#define DOMAIN_CFG				GENMASK(24, 20)
#define AP_DMA_I2C_12_INT_FLAG				0x00001080
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_12_INT_EN				0x00001084
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_12_EN				0x00001088
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_12_RST				0x0000108c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_12_STOP				0x00001090
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_12_FLUSH				0x00001094
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_12_CON				0x00001098
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_12_TX_MEM_ADDR			0x0000109c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_12_RX_MEM_ADDR			0x000010a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_12_TX_LEN				0x000010a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_12_RX_LEN				0x000010a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_12_INT_BUF_SIZE			0x000010b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_12_TX_MEM_ADDR2			0x000010d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_12_RX_MEM_ADDR2			0x000010d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_13_INT_FLAG				0x00001100
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_13_INT_EN				0x00001104
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_13_EN				0x00001108
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_13_RST				0x0000110c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_13_STOP				0x00001110
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_13_FLUSH				0x00001114
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_13_CON				0x00001118
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_13_TX_MEM_ADDR			0x0000111c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_13_RX_MEM_ADDR			0x00001120
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_13_TX_LEN				0x00001124
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_13_RX_LEN				0x00001128
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_13_INT_BUF_SIZE			0x00001138
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_13_TX_MEM_ADDR2			0x00001154
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_13_RX_MEM_ADDR2			0x00001158
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_14_INT_FLAG				0x00001180
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_14_INT_EN				0x00001184
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_14_EN				0x00001188
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_14_RST				0x0000118c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_14_STOP				0x00001190
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_14_FLUSH				0x00001194
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_14_CON				0x00001198
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_14_TX_MEM_ADDR			0x0000119c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_14_RX_MEM_ADDR			0x000011a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_14_TX_LEN				0x000011a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_14_RX_LEN				0x000011a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_14_INT_BUF_SIZE			0x000011b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_14_TX_MEM_ADDR2			0x000011d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_14_RX_MEM_ADDR2			0x000011d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_15_INT_FLAG				0x00001200
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_15_INT_EN				0x00001204
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_15_EN				0x00001208
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_15_RST				0x0000120c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_15_STOP				0x00001210
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_15_FLUSH				0x00001214
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_15_CON				0x00001218
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_15_TX_MEM_ADDR			0x0000121c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_15_RX_MEM_ADDR			0x00001220
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_15_TX_LEN				0x00001224
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_15_RX_LEN				0x00001228
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_15_INT_BUF_SIZE			0x00001238
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_15_TX_MEM_ADDR2			0x00001254
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_15_RX_MEM_ADDR2			0x00001258
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_16_INT_FLAG				0x00001280
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_16_INT_EN				0x00001284
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_16_EN				0x00001288
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_16_RST				0x0000128c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_16_STOP				0x00001290
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_16_FLUSH				0x00001294
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_16_CON				0x00001298
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_16_TX_MEM_ADDR			0x0000129c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_16_RX_MEM_ADDR			0x000012a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_16_TX_LEN				0x000012a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_16_RX_LEN				0x000012a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_16_INT_BUF_SIZE			0x000012b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_16_TX_MEM_ADDR2			0x000012d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_16_RX_MEM_ADDR2			0x000012d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_11_CCU_INT_FLAG			0x00001300
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_11_CCU_INT_EN			0x00001304
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_11_CCU_EN				0x00001308
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_11_CCU_RST				0x0000130c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_11_CCU_STOP				0x00001310
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_11_CCU_FLUSH				0x00001314
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_11_CCU_CON				0x00001318
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_11_CCU_TX_MEM_ADDR			0x0000131c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_11_CCU_RX_MEM_ADDR			0x00001320
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_11_CCU_TX_LEN			0x00001324
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_11_CCU_RX_LEN			0x00001328
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_11_CCU_INT_BUF_SIZE			0x00001338
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_11_CCU_TX_MEM_ADDR2			0x00001354
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_11_CCU_RX_MEM_ADDR2			0x00001358
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_12_CCU_INT_FLAG			0x00001380
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_12_CCU_INT_EN			0x00001384
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_12_CCU_EN				0x00001388
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_12_CCU_RST				0x0000138c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_12_CCU_STOP				0x00001390
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_12_CCU_FLUSH				0x00001394
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_12_CCU_CON				0x00001398
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_12_CCU_TX_MEM_ADDR			0x0000139c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_12_CCU_RX_MEM_ADDR			0x000013a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_12_CCU_TX_LEN			0x000013a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_12_CCU_RX_LEN			0x000013a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_12_CCU_INT_BUF_SIZE			0x000013b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_12_CCU_TX_MEM_ADDR2			0x000013d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_12_CCU_RX_MEM_ADDR2			0x000013d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_13_CCU_INT_FLAG			0x00001400
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_13_CCU_INT_EN			0x00001404
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_13_CCU_EN				0x00001408
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_13_CCU_RST				0x0000140c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_13_CCU_STOP				0x00001410
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_13_CCU_FLUSH				0x00001414
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_13_CCU_CON				0x00001418
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_13_CCU_TX_MEM_ADDR			0x0000141c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_13_CCU_RX_MEM_ADDR			0x00001420
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_13_CCU_TX_LEN			0x00001424
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_13_CCU_RX_LEN			0x00001428
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_13_CCU_INT_BUF_SIZE			0x00001438
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_13_CCU_TX_MEM_ADDR2			0x00001454
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_13_CCU_RX_MEM_ADDR2			0x00001458
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_14_CCU_INT_FLAG			0x00001480
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_14_CCU_INT_EN			0x00001484
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_14_CCU_EN				0x00001488
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_14_CCU_RST				0x0000148c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_14_CCU_STOP				0x00001490
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_14_CCU_FLUSH				0x00001494
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_14_CCU_CON				0x00001498
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_14_CCU_TX_MEM_ADDR			0x0000149c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_14_CCU_RX_MEM_ADDR			0x000014a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_14_CCU_TX_LEN			0x000014a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_14_CCU_RX_LEN			0x000014a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_14_CCU_INT_BUF_SIZE			0x000014b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_14_CCU_TX_MEM_ADDR2			0x000014d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_14_CCU_RX_MEM_ADDR2			0x000014d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_15_CCU_INT_FLAG			0x00001500
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_15_CCU_INT_EN			0x00001504
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_15_CCU_EN				0x00001508
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_15_CCU_RST				0x0000150c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_15_CCU_STOP				0x00001510
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_15_CCU_FLUSH				0x00001514
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_15_CCU_CON				0x00001518
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_15_CCU_TX_MEM_ADDR			0x0000151c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_15_CCU_RX_MEM_ADDR			0x00001520
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_15_CCU_TX_LEN			0x00001524
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_15_CCU_RX_LEN			0x00001528
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_15_CCU_INT_BUF_SIZE			0x00001538
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_15_CCU_TX_MEM_ADDR2			0x00001554
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_15_CCU_RX_MEM_ADDR2			0x00001558
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_16_CCU_INT_FLAG			0x00001580
	#define TX_FLAG					BIT(0)
	#define RX_FLAG					BIT(1)
	#define FLAG_0					BIT(2)
#define AP_DMA_I2C_16_CCU_INT_EN			0x00001584
	#define INTEN_TX_FLAG				BIT(0)
	#define INTEN_RX_FLAG				BIT(1)
	#define INTEN_FLAG_0				BIT(2)
#define AP_DMA_I2C_16_CCU_EN				0x00001588
	#define EN					BIT(0)
	#define MASK_DTX2RX				BIT(1)
#define AP_DMA_I2C_16_CCU_RST				0x0000158c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_I2C_16_CCU_STOP				0x00001590
	#define STOP					BIT(0)
	#define PAUSE					BIT(1)
#define AP_DMA_I2C_16_CCU_FLUSH				0x00001594
	#define FLUSH					BIT(0)
#define AP_DMA_I2C_16_CCU_CON				0x00001598
	#define DIR					BIT(0)
	#define FIX_EN					BIT(1)
#define AP_DMA_I2C_16_CCU_TX_MEM_ADDR			0x0000159c
	#define TX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_16_CCU_RX_MEM_ADDR			0x000015a0
	#define RX_MEM_ADDR				GENMASK(31, 0)
#define AP_DMA_I2C_16_CCU_TX_LEN			0x000015a4
	#define TX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_16_CCU_RX_LEN			0x000015a8
	#define RX_LEN					GENMASK(15, 0)
#define AP_DMA_I2C_16_CCU_INT_BUF_SIZE			0x000015b8
	#define INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_I2C_16_CCU_TX_MEM_ADDR2			0x000015d4
	#define TX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_I2C_16_CCU_RX_MEM_ADDR2			0x000015d8
	#define RX_MEM_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_5_TX_INT_FLAG			0x00001680
	#define FLAG0					BIT(0)
#define AP_DMA_UART_5_TX_INT_EN				0x00001684
	#define INTEN					BIT(0)
#define AP_DMA_UART_5_TX_EN				0x00001688
	#define EN					BIT(0)
#define AP_DMA_UART_5_TX_RST				0x0000168c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_5_TX_STOP				0x00001690
	#define STOP					BIT(0)
#define AP_DMA_UART_5_TX_FLUSH				0x00001694
	#define FLUSH					BIT(0)
#define AP_DMA_UART_5_TX_VFF_ADDR			0x0000169c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_5_TX_VFF_LEN			0x000016a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_5_TX_VFF_THRE			0x000016a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_5_TX_VFF_WPT			0x000016ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_5_TX_VFF_RPT			0x000016b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_5_TX_INT_BUF_SIZE			0x000016b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_5_TX_VFF_VALID_SIZE			0x000016bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_5_TX_VFF_LEFT_SIZE			0x000016c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_5_TX_VFF_ADDR2			0x000016d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_5_RX_INT_FLAG			0x00001700
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_5_RX_INT_EN				0x00001704
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_5_RX_EN				0x00001708
	#define EN					BIT(0)
#define AP_DMA_UART_5_RX_RST				0x0000170c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_5_RX_STOP				0x00001710
	#define STOP					BIT(0)
#define AP_DMA_UART_5_RX_FLUSH				0x00001714
	#define FLUSH					BIT(0)
#define AP_DMA_UART_5_RX_VFF_ADDR			0x0000171c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_5_RX_VFF_LEN			0x00001724
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_5_RX_VFF_THRE			0x00001728
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_5_RX_VFF_WPT			0x0000172c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_5_RX_VFF_RPT			0x00001730
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_5_RX_FLOW_CTRL_THRE			0x00001734
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_5_RX_INT_BUF_SIZE			0x00001738
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_5_RX_VFF_VALID_SIZE			0x0000173c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_5_RX_VFF_LEFT_SIZE			0x00001740
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_5_RX_VFF_ADDR2			0x00001754
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_6_TX_INT_FLAG			0x00001780
	#define FLAG0					BIT(0)
#define AP_DMA_UART_6_TX_INT_EN				0x00001784
	#define INTEN					BIT(0)
#define AP_DMA_UART_6_TX_EN				0x00001788
	#define EN					BIT(0)
#define AP_DMA_UART_6_TX_RST				0x0000178c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_6_TX_STOP				0x00001790
	#define STOP					BIT(0)
#define AP_DMA_UART_6_TX_FLUSH				0x00001794
	#define FLUSH					BIT(0)
#define AP_DMA_UART_6_TX_VFF_ADDR			0x0000179c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_6_TX_VFF_LEN			0x000017a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_6_TX_VFF_THRE			0x000017a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_6_TX_VFF_WPT			0x000017ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_6_TX_VFF_RPT			0x000017b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_6_TX_INT_BUF_SIZE			0x000017b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_6_TX_VFF_VALID_SIZE			0x000017bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_6_TX_VFF_LEFT_SIZE			0x000017c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_6_TX_VFF_ADDR2			0x000017d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_6_RX_INT_FLAG			0x00001800
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_6_RX_INT_EN				0x00001804
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_6_RX_EN				0x00001808
	#define EN					BIT(0)
#define AP_DMA_UART_6_RX_RST				0x0000180c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_6_RX_STOP				0x00001810
	#define STOP					BIT(0)
#define AP_DMA_UART_6_RX_FLUSH				0x00001814
	#define FLUSH					BIT(0)
#define AP_DMA_UART_6_RX_VFF_ADDR			0x0000181c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_6_RX_VFF_LEN			0x00001824
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_6_RX_VFF_THRE			0x00001828
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_6_RX_VFF_WPT			0x0000182c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_6_RX_VFF_RPT			0x00001830
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_6_RX_FLOW_CTRL_THRE			0x00001834
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_6_RX_INT_BUF_SIZE			0x00001838
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_6_RX_VFF_VALID_SIZE			0x0000183c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_6_RX_VFF_LEFT_SIZE			0x00001840
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_6_RX_VFF_ADDR2			0x00001854
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_7_TX_INT_FLAG			0x00001880
	#define FLAG0					BIT(0)
#define AP_DMA_UART_7_TX_INT_EN				0x00001884
	#define INTEN					BIT(0)
#define AP_DMA_UART_7_TX_EN				0x00001888
	#define EN					BIT(0)
#define AP_DMA_UART_7_TX_RST				0x0000188c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_7_TX_STOP				0x00001890
	#define STOP					BIT(0)
#define AP_DMA_UART_7_TX_FLUSH				0x00001894
	#define FLUSH					BIT(0)
#define AP_DMA_UART_7_TX_VFF_ADDR			0x0000189c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_7_TX_VFF_LEN			0x000018a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_7_TX_VFF_THRE			0x000018a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_7_TX_VFF_WPT			0x000018ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_7_TX_VFF_RPT			0x000018b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_7_TX_INT_BUF_SIZE			0x000018b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_7_TX_VFF_VALID_SIZE			0x000018bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_7_TX_VFF_LEFT_SIZE			0x000018c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_7_TX_VFF_ADDR2			0x000018d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_7_RX_INT_FLAG			0x00001900
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_7_RX_INT_EN				0x00001904
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_7_RX_EN				0x00001908
	#define EN					BIT(0)
#define AP_DMA_UART_7_RX_RST				0x0000190c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_7_RX_STOP				0x00001910
	#define STOP					BIT(0)
#define AP_DMA_UART_7_RX_FLUSH				0x00001914
	#define FLUSH					BIT(0)
#define AP_DMA_UART_7_RX_VFF_ADDR			0x0000191c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_7_RX_VFF_LEN			0x00001924
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_7_RX_VFF_THRE			0x00001928
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_7_RX_VFF_WPT			0x0000192c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_7_RX_VFF_RPT			0x00001930
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_7_RX_FLOW_CTRL_THRE			0x00001934
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_7_RX_INT_BUF_SIZE			0x00001938
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_7_RX_VFF_VALID_SIZE			0x0000193c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_7_RX_VFF_LEFT_SIZE			0x00001940
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_7_RX_VFF_ADDR2			0x00001954
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_8_TX_INT_FLAG			0x00001980
	#define FLAG0					BIT(0)
#define AP_DMA_UART_8_TX_INT_EN				0x00001984
	#define INTEN					BIT(0)
#define AP_DMA_UART_8_TX_EN				0x00001988
	#define EN					BIT(0)
#define AP_DMA_UART_8_TX_RST				0x0000198c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_8_TX_STOP				0x00001990
	#define STOP					BIT(0)
#define AP_DMA_UART_8_TX_FLUSH				0x00001994
	#define FLUSH					BIT(0)
#define AP_DMA_UART_8_TX_VFF_ADDR			0x0000199c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_8_TX_VFF_LEN			0x000019a4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_8_TX_VFF_THRE			0x000019a8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_8_TX_VFF_WPT			0x000019ac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_8_TX_VFF_RPT			0x000019b0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_8_TX_INT_BUF_SIZE			0x000019b8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_8_TX_VFF_VALID_SIZE			0x000019bc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_8_TX_VFF_LEFT_SIZE			0x000019c0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_8_TX_VFF_ADDR2			0x000019d4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_8_RX_INT_FLAG			0x00001a00
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_8_RX_INT_EN				0x00001a04
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_8_RX_EN				0x00001a08
	#define EN					BIT(0)
#define AP_DMA_UART_8_RX_RST				0x00001a0c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_8_RX_STOP				0x00001a10
	#define STOP					BIT(0)
#define AP_DMA_UART_8_RX_FLUSH				0x00001a14
	#define FLUSH					BIT(0)
#define AP_DMA_UART_8_RX_VFF_ADDR			0x00001a1c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_8_RX_VFF_LEN			0x00001a24
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_8_RX_VFF_THRE			0x00001a28
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_8_RX_VFF_WPT			0x00001a2c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_8_RX_VFF_RPT			0x00001a30
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_8_RX_FLOW_CTRL_THRE			0x00001a34
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_8_RX_INT_BUF_SIZE			0x00001a38
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_8_RX_VFF_VALID_SIZE			0x00001a3c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_8_RX_VFF_LEFT_SIZE			0x00001a40
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_8_RX_VFF_ADDR2			0x00001a54
	#define RX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_9_TX_INT_FLAG			0x00001a80
	#define FLAG0					BIT(0)
#define AP_DMA_UART_9_TX_INT_EN				0x00001a84
	#define INTEN					BIT(0)
#define AP_DMA_UART_9_TX_EN				0x00001a88
	#define EN					BIT(0)
#define AP_DMA_UART_9_TX_RST				0x00001a8c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_9_TX_STOP				0x00001a90
	#define STOP					BIT(0)
#define AP_DMA_UART_9_TX_FLUSH				0x00001a94
	#define FLUSH					BIT(0)
#define AP_DMA_UART_9_TX_VFF_ADDR			0x00001a9c
	#define TX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_9_TX_VFF_LEN			0x00001aa4
	#define TX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_9_TX_VFF_THRE			0x00001aa8
	#define TX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_9_TX_VFF_WPT			0x00001aac
	#define TX_VFF_WPT				GENMASK(15, 0)
	#define TX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_9_TX_VFF_RPT			0x00001ab0
	#define TX_VFF_RPT				GENMASK(15, 0)
	#define TX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_9_TX_INT_BUF_SIZE			0x00001ab8
	#define TX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_9_TX_VFF_VALID_SIZE			0x00001abc
	#define TX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_9_TX_VFF_LEFT_SIZE			0x00001ac0
	#define TX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_9_TX_VFF_ADDR2			0x00001ad4
	#define TX_VFF_ADDR2				GENMASK(3, 0)
#define AP_DMA_UART_9_RX_INT_FLAG			0x00001b00
	#define FLAG0					BIT(0)
	#define FLAG1					BIT(1)
#define AP_DMA_UART_9_RX_INT_EN				0x00001b04
	#define INTEN0					BIT(0)
	#define INTEN1					BIT(1)
#define AP_DMA_UART_9_RX_EN				0x00001b08
	#define EN					BIT(0)
#define AP_DMA_UART_9_RX_RST				0x00001b0c
	#define WARM_RST				BIT(0)
	#define HARD_RST				BIT(1)
#define AP_DMA_UART_9_RX_STOP				0x00001b10
	#define STOP					BIT(0)
#define AP_DMA_UART_9_RX_FLUSH				0x00001b14
	#define FLUSH					BIT(0)
#define AP_DMA_UART_9_RX_VFF_ADDR			0x00001b1c
	#define RX_VFF_ADDR				GENMASK(31, 3)
#define AP_DMA_UART_9_RX_VFF_LEN			0x00001b24
	#define RX_VFF_LEN				GENMASK(15, 3)
#define AP_DMA_UART_9_RX_VFF_THRE			0x00001b28
	#define RX_VFF_THRE				GENMASK(15, 0)
#define AP_DMA_UART_9_RX_VFF_WPT			0x00001b2c
	#define RX_VFF_WPT				GENMASK(15, 0)
	#define RX_VFF_WPT_WRAP				BIT(16)
#define AP_DMA_UART_9_RX_VFF_RPT			0x00001b30
	#define RX_VFF_RPT				GENMASK(15, 0)
	#define RX_VFF_RPT_WRAP				BIT(16)
#define AP_DMA_UART_9_RX_FLOW_CTRL_THRE			0x00001b34
	#define RX_FLOW_CTRL_THRE			GENMASK(7, 0)
#define AP_DMA_UART_9_RX_INT_BUF_SIZE			0x00001b38
	#define RX_INT_BUF_SIZE				GENMASK(4, 0)
#define AP_DMA_UART_9_RX_VFF_VALID_SIZE			0x00001b3c
	#define RX_VFF_VALID_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_9_RX_VFF_LEFT_SIZE			0x00001b40
	#define RX_VFF_LEFT_SIZE			GENMASK(15, 0)
#define AP_DMA_UART_9_RX_VFF_ADDR2			0x00001b54
	#define RX_VFF_ADDR2				GENMASK(3, 0)

/*
 * For MT3612, due to the re-arangement of APDMA Channels
 * There is a hole in APDMA's Register Map
 * Because one I2C channel becomes RESERVED, the channel base of MT3612
 * calculation method is different from legacy design
 */
#define MTK_APDMA_LEGACY_RESERVED	(33)
#define MTK_APDMA_MODULE1_START_CH	(22)

#define MTK_APDMA0_CHANIO(base, n)	((base) + 0x80 * (n + 1))

#define MTK_APDMA1_RESERVED_CH_IDX	(MTK_APDMA_LEGACY_RESERVED	\
					- MTK_APDMA_MODULE1_START_CH)
#define MTK_APDMA1_CHANIO(base, n)	(((n) >= MTK_APDMA1_RESERVED_CH_IDX) ? \
					 (base) + 0x80 * ((n) + 2) :	\
					 (base) + 0x80 * ((n) + 1))

#endif /*__AP_DMA_REG_H__*/
