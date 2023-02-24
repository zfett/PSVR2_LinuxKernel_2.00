/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#pragma once

#define IMU_SAMPLE_RATIO 2
#define IMU_INTERVAL 500  /* usec */

/* Bank 0 */
#define ICM426_REG_DEVICE_CONFIG			0x11
#define ICM426_REG_DRIVE_CONFIG				0x13
#define ICM426_REG_INT_CONFIG				0x14
#define ICM426_REG_FIFO_CONFIG				0x16
#define ICM426_REG_TEMP_DATA0_UI			0x1D
#define ICM426_REG_ACCEL_DATA_X0_UI			0x1F
#define ICM426_REG_GYRO_DATA_X0_UI			0x25
#define ICM426_REG_INT_STATUS				0x2D
#define ICM426_REG_FIFO_BYTE_COUNT1			0x2E
#define ICM426_REG_FIFO_BYTE_COUNT2			0x2F
#define ICM426_REG_FIFO_DATA				0x30
#define ICM426_REG_INT_STATUS2				0x37
#define ICM426_REG_INT_STATUS3				0x38
#define ICM426_REG_SIGNAL_PATH_RESET		0x4B
#define ICM426_REG_INTF_CONFIG0				0x4C
#define ICM426_REG_INTF_CONFIG1				0x4D
#define ICM426_REG_PWR_MGMT_0				0x4E
#define ICM426_REG_GYRO_CONFIG0				0x4F
#define ICM426_REG_ACCEL_CONFIG0			0x50
#define ICM426_REG_GYRO_CONFIG1				0x51
#define ICM426_REG_ACCEL_GYRO_CONFIG0		0x52
#define ICM426_REG_ACCEL_CONFIG1			0x53
#define ICM426_REG_TMST_CONFIG				0x54
#define ICM426_REG_APEX_CONFIG0				0x56
#define ICM426_REG_SMD_CONFIG				0x57
#define ICM426_REG_FIFO_CONFIG1				0x5F
#define ICM426_REG_FIFO_CONFIG2				0x60
#define ICM426_REG_INT_CONFIG0				0x63
#define ICM426_REG_INT_CONFIG1				0x64
#define ICM426_REG_INT_SOURCE0				0x65
#define ICM426_REG_INT_SOURCE1				0x66
#define ICM426_REG_INT_SOURCE3				0x68
#define ICM426_REG_INT_SOURCE4				0x69
#define ICM426_REG_FIFO_LOST_PKT0			0x6C
#define ICM426_REG_SELF_TEST_CONFIG			0x70
#define ICM426_REG_WHO_AM_I					0x75
#define ICM426_REG_REG_BANK_SEL				0x76

/* Bank 1 */
#define ICM426_REG_TMST_VAL0_B1				0x62
#define ICM426_REG_INTF_CONFIG4_B1			0x7A
#define ICM426_REG_INTF_CONFIG5_B1			0x7B

/* Bank 2 */
#define ICM426_REG_ACCEL_CONFIG_STATIC2_B2	0x03
#define ICM426_REG_ACCEL_CONFIG_STATIC3_B2	0x04
#define ICM426_REG_ACCEL_CONFIG_STATIC4_B2	0x05
#define ICM426_REG_XA_ST_DATA_B2			0x3B
#define ICM426_REG_YA_ST_DATA_B2			0x3C
#define ICM426_REG_ZA_ST_DATA_B2			0x3D

/* Bank 4 */
#define ICM426_REG_APEX_CONFIG1_B4			0x40
#define ICM426_REG_APEX_CONFIG2_B4			0x41
#define ICM426_REG_APEX_CONFIG3_B4			0x42
#define ICM426_REG_APEX_CONFIG4_B4			0x43
#define ICM426_REG_APEX_CONFIG5_B4			0x44
#define ICM426_REG_APEX_CONFIG6_B4			0x45
#define ICM426_REG_APEX_CONFIG7_B4			0x46
#define ICM426_REG_APEX_CONFIG8_B4			0x47
#define ICM426_REG_APEX_CONFIG9_B4			0x48
#define ICM426_REG_ACCEL_WOM_X_THR_B4		0x4A
#define ICM426_REG_ACCEL_WOM_Y_THR_B4		0x4B
#define ICM426_REG_ACCEL_WOM_Z_THR_B4		0x4C
#define ICM426_REG_INT_SOURCE6_B4			0x4D
#define ICM426_REG_INT_SOURCE7_B4			0x4E
#define ICM426_REG_OFFSET_USER_0_B4			0x77
#define ICM426_REG_OFFSET_USER_1_B4			0x78
#define ICM426_REG_OFFSET_USER_2_B4			0x79
#define ICM426_REG_OFFSET_USER_3_B4			0x7A
#define ICM426_REG_OFFSET_USER_4_B4			0x7B
#define ICM426_REG_OFFSET_USER_5_B4			0x7C
#define ICM426_REG_OFFSET_USER_6_B4			0x7D
#define ICM426_REG_OFFSET_USER_7_B4			0x7E
#define ICM426_REG_OFFSET_USER_8_B4			0x7F

/* Bank0 REG_REG_DEVICE_CONFIG(0x11) */
#define BIT_SOFT_RESET_ENABLE				0x01

/* Bank0 REG_INT_CONFIG(0x14) */
#define BIT_INT1_POLARITY_ACTIVE_LOW		0x00
#define BIT_INT1_POLARITY_ACTIVE_HIGH		0x01
#define BIT_INT1_DRIVE_CIRCUIT_OPEN_DRAIN	0x00
#define BIT_INT1_DRIVE_CIRCUIT_PUSH_PULL	0x02
#define BIT_INT1_PULSED_MODE				0x00
#define BIT_INT1_LATCHED_MODE				0x04

/* Bank0 REG_FIFO_CONFIG_REG(0x16) */
#define BIT_FIFO_MODE_BYPASS				0x00
#define BIT_FIFO_MODE_STREAM				0x40
#define BIT_FIFO_MODE_STOP_FULL				0x80

/* Bank0 REG_INT_STATUS(0x2D) */
#define BIT_INT_STATUS_AGC_RDY				0x01
#define BIT_INT_STATUS_FIFO_FULL			0x02
#define BIT_INT_STATUS_FIFO_THS				0x04
#define BIT_INT_STATUS_DRDY					0x08
#define BIT_INT_STATUS_RESET_DONE			0x10
#define BIT_INT_STATUS_PLL_DRY				0x20
#define BIT_INT_STATUS_UI_FSYNC				0x40

/* Bank0 REG_INT_STATUS2(0x37) */
#define BIT_INT_STATUS_WOM_X				0x01
#define BIT_INT_STATUS_WOM_Y				0x02
#define BIT_INT_STATUS_WOM_Z				0x04
#define BIT_INT_STATUS_SMD					0x08
#define BIT_INT_STATUS_WOM_XYZ \
	(BIT_INT_STATUS_WOM_X | BIT_INT_STATUS_WOM_Y | BIT_INT_STATUS_WOM_Z)

/* Bank0 REG_INT_STATUS3(0x38) */
#define BIT_INT_STATUS_TAP_DET				0x01
#define BIT_INT_STATUS_SLEEP_DET			0x02
#define BIT_INT_STATUS_RAISE_DET			0x04
#define BIT_INT_STATUS_TILT_DET				0x08
#define BIT_INT_STATUS_STEP_CNT_OVFL		0x10
#define BIT_INT_STATUS_STEP_DET				0x20
#define BIT_INT_STATUS_DMP_POWER_SAVE		0x40

/* Bank0 REG_SIGNAL_PATH_RESET(0x4B) */
#define BIT_TEMP_RST						0x01
#define BIT_FIFO_FLUSH						0x02
#define BIT_TMST_STROBE						0x04
#define BIT_ABORT_AND_RESET					0x08
#define BIT_S4S_RESTART						0x10
#define BIT_DMP_MEM_RESET_EN				0x20
#define BIT_DMP_INIT_EN						0x40

/* Bank0 REG_INTF_CONFIG0(0x4C) */
#define BIT_FIFO_COUNT_REC					0x40
#define BIT_COUNT_BIG_ENDIAN				0x20
#define BIT_SENS_DATA_BIG_ENDIAN			0x10
#define BIT_UI_SIFS_DISABLE_SPI				0x02
#define BIT_UI_SIFS_DISABLE_I2C				0x03

/* Bank0 REG_INTF_CONFIG1(0x4D) */
#define BIT_GYRO_AFSR_MODE_LFS				0x00
#define BIT_GYRO_AFSR_MODE_HFS				0x40
#define BIT_GYRO_AFSR_MODE_DYN				0xC0
#define BIT_ACCEL_AFSR_MODE_LFS				0x00
#define BIT_ACCEL_AFSR_MODE_HFS				0x10
#define BIT_ACCEL_AFSR_MODE_DYN				0x30
#define BIT_ACCEL_LP_CLK_SEL				0x08
#define BIT_RTC_MODE						0x04
#define BIT_CLK_SEL_RC						0x00
#define BIT_CLK_SEL_PLL						0x01
#define BIT_CLK_SEL_DIS						0x03

/* Bank0 REG_PWR_MGMT_0(0x4E) */
#define BIT_TEMP_DIS						0x20
#define BIT_IDLE							0x10
#define BIT_GYRO_MODE_OFF					0x00
#define BIT_GYRO_MODE_STBY					0x04
#define BIT_GYRO_MODE_LPM					0x08
#define BIT_GYRO_MODE_LNM					0x0C
#define BIT_ACCEL_MODE_OFF					0x00
#define BIT_ACCEL_MODE_LPM					0x02
#define BIT_ACCEL_MODE_LNM					0x03

/* Bank0 REG_GYRO_CONFIG0(0x4F) */
#define BIT_GYRO_FSR						0xE0
#define BIT_GYRO_FSR_2000					0x00
#define BIT_GYRO_FSR_1000					0x20
#define BIT_GYRO_FSR_500					0x40
#define BIT_GYRO_FSR_250					0x60
#define BIT_GYRO_FSR_125					0x80
#define BIT_GYRO_FSR_62						0xA0
#define BIT_GYRO_FSR_31						0xC0
#define BIT_GYRO_FSR_15						0xE0
#define BIT_GYRO_ODR						0x0F
#define BIT_GYRO_ODR_8000					0x03
#define BIT_GYRO_ODR_4000					0x04
#define BIT_GYRO_ODR_2000					0x05
#define BIT_GYRO_ODR_1000					0x06
#define BIT_GYRO_ODR_500					0x0F
#define BIT_GYRO_ODR_200					0x07
#define BIT_GYRO_ODR_100					0x08
#define BIT_GYRO_ODR_50						0x09
#define BIT_GYRO_ODR_25						0x0A
#define BIT_GYRO_ODR_12						0x0B

/* Bank0 REG_ACCEL_CONFIG0(0x50) */
#define BIT_ACCEL_FSR						0xE0
#define BIT_ACCEL_FSR_16G					0x00
#define BIT_ACCEL_FSR_8G					0x20
#define BIT_ACCEL_FSR_4G					0x40
#define BIT_ACCEL_FSR_2G					0x60
#define BIT_ACCEL_ODR						0x0F
#define BIT_ACCEL_ODR_8000					0x03
#define BIT_ACCEL_ODR_4000					0x04
#define BIT_ACCEL_ODR_2000					0x05
#define BIT_ACCEL_ODR_1000					0x06
#define BIT_ACCEL_ODR_500					0x0F
#define BIT_ACCEL_ODR_200					0x07
#define BIT_ACCEL_ODR_100					0x08
#define BIT_ACCEL_ODR_50					0x09
#define BIT_ACCEL_ODR_25					0x0A
#define BIT_ACCEL_ODR_12					0x0B
#define BIT_ACCEL_ODR_6						0x0C
#define BIT_ACCEL_ODR_3						0x0D
#define BIT_ACCEL_ODR_1						0x0E

/* Bank0 REG_GYRO_CONFIG1(0x51) */
#define BIT_TEMP_FILT_BW_BYPASS				0x00
#define BIT_TEMP_FILT_BW_170				0x20
#define BIT_TEMP_FILT_BW_82					0x40
#define BIT_TEMP_FILT_BW_40					0x60
#define BIT_TEMP_FILT_BW_20					0x80
#define BIT_TEMP_FILT_BW_10					0x90
#define BIT_TEMP_FILT_BW_5					0xC0
#define BIT_GYR_AVG_FLT_RATE_8KHZ			0x10
#define BIT_GYR_AVG_FLT_RATE_1KHZ			0x00
#define BIT_GYR_UI_FILT_ORD_IND_1			0x00
#define BIT_GYR_UI_FILT_ORD_IND_2			0x04
#define BIT_GYR_UI_FILT_ORD_IND_3			0x08
#define BIT_GYR_DEC2_M2_ORD_1				0x00
#define BIT_GYR_DEC2_M2_ORD_2				0x01
#define BIT_GYR_DEC2_M2_ORD_3				0x02

/* Bank0 REG_ACCEL_GYRO_CONFIG0(0x52) */
#define BIT_ACCEL_UI_LNM_BW_2_FIR			0x00
#define BIT_ACCEL_UI_LNM_BW_4_IIR			0x10
#define BIT_ACCEL_UI_LNM_BW_5_IIR			0x20
#define BIT_ACCEL_UI_LNM_BW_8_IIR			0x30
#define BIT_ACCEL_UI_LNM_BW_10_IIR			0x40
#define BIT_ACCEL_UI_LNM_BW_16_IIR			0x50
#define BIT_ACCEL_UI_LNM_BW_20_IIR			0x60
#define BIT_ACCEL_UI_LNM_BW_40_IIR			0x70
#define BIT_ACCEL_UI_LNM_AVG_1				0xF0
#define BIT_ACCEL_UI_LPM_BW_2_FIR			0x00
#define BIT_ACCEL_UI_LPM_AVG_1				0x10
#define BIT_ACCEL_UI_LPM_AVG_2				0x20
#define BIT_ACCEL_UI_LPM_AVG_3				0x30
#define BIT_ACCEL_UI_LPM_AVG_4				0x40
#define BIT_ACCEL_UI_LPM_AVG_8				0x50
#define BIT_ACCEL_UI_LPM_AVG_16				0x60
#define BIT_ACCEL_UI_LPM_AVG_32				0x70
#define BIT_ACCEL_UI_LPM_AVG_64				0x80
#define BIT_ACCEL_UI_LPM_AVG_128			0x90
#define BIT_GYRO_UI_LNM_BW_2_FIR			0x00
#define BIT_GYRO_UI_LNM_BW_4_IIR			0x01
#define BIT_GYRO_UI_LNM_BW_5_IIR			0x02
#define BIT_GYRO_UI_LNM_BW_8_IIR			0x03
#define BIT_GYRO_UI_LNM_BW_10_IIR			0x04
#define BIT_GYRO_UI_LNM_BW_16_IIR			0x05
#define BIT_GYRO_UI_LNM_BW_20_IIR			0x06
#define BIT_GYRO_UI_LNM_BW_40_IIR			0x07
#define BIT_GYRO_UI_LNM_AVG_1				0xF0
#define BIT_GYRO_UI_LPM_BW_2_FIR			0x00
#define BIT_GYRO_UI_LPM_AVG_1				0x01
#define BIT_GYRO_UI_LPM_AVG_2				0x02
#define BIT_GYRO_UI_LPM_AVG_3				0x03
#define BIT_GYRO_UI_LPM_AVG_4				0x04
#define BIT_GYRO_UI_LPM_AVG_8				0x05
#define BIT_GYRO_UI_LPM_AVG_16				0x06
#define BIT_GYRO_UI_LPM_AVG_32				0x07
#define BIT_GYRO_UI_LPM_AVG_64				0x08
#define BIT_GYRO_UI_LPM_AVG_128				0x09

/* Bank0 REG_ACCEL_CONFIG1(0x53) */
#define BIT_ACC_UI_FILT_ODR_IND_1			0x00
#define BIT_ACC_UI_FILT_ODR_IND_2			0x08
#define BIT_ACC_UI_FILT_ODR_IND_3			0x10
#define BIT_ACC_DEC2_M2_ORD_1				0x00
#define BIT_ACC_DEC2_M2_ORD_2				0x02
#define BIT_ACC_DEC2_M2_ORD_3				0x04
#define BIT_ACC_AVG_FLT_RATE_8KHZ			0x01
#define BIT_ACC_AVG_FLT_RATE_1KHZ			0x00

/* Bank0 REG_TMST_CONFIG(0x54) */
#define BIT_FIFO_RAM_ISO_ENA				0x40
#define BIT_EN_DREG_FIFO_D2A				0x20
#define BIT_TMST_TO_REGS_EN					0x10
#define BIT_TMST_RESOL_1US					0x00
#define BIT_TMST_RESOL_16US					0x08
#define BIT_TMST_DELTA_EN					0x04
/* #define BIT_TMST_FSYNC_EN				0x02 */
#define BIT_TMST_EN							0x01

/* Bank0 REG_APEX_CONFIG0(0x56) */
#define BIT_DMP_ODR_25HZ					0x00
#define BIT_DMP_ODR_50HZ					0x02
#define BIT_DMP_ODR_100HZ					0x03
#define BIT_RAISE_ENABLE					0x08
#define BIT_TILT_ENABLE						0x10
#define BIT_PEDO_ENABLE						0x20
#define BIT_TAP_ENABLE						0x40
#define BIT_DMP_POWER_SAVE_EN				0x80

/* Bank0 REG_SMD_CONFIG(0x57) */
#define BIT_WOM_INT_MODE_OR					0x00
#define BIT_WOM_INT_MODE_AND				0x08
#define BIT_WOM_MODE_INITIAL				0x00
#define BIT_WOM_MODE_PREV					0x04
#define BIT_SMD_MODE_OFF					0x00
#define BIT_SMD_MODE_OLD					0x01
#define BIT_SMD_MODE_SHORT					0x02
#define BIT_SMD_MODE_LONG					0x03

/* Bank0 REG_FIFO_CONFIG1(0x5F) */
#define BIT_FIFO_ACCEL_EN					0x01
#define BIT_FIFO_GYRO_EN					0x02
#define BIT_FIFO_TEMP_EN					0x04
#define BIT_FIFO_TMST_FSYNC_EN				0x08
#define BIT_FIFO_HIRES_EN					0x10
#define BIT_FIFO_WM_TH						0x20
#define BIT_FIFO_RESUME_PART_RD				0x40

/* Bank0 REG_INT_CONFIG1(0x64) */
#define BIT_INT_ASY_RST_DISABLE				0x10

/* Bank0 REG_INT_SOURCE0(0x65) */
#define BIT_INT_UI_AGC_RDY_INT1_EN			0x01
#define BIT_INT_FIFO_FULL_INT1_EN			0x02
#define BIT_INT_FIFO_THS_INT1_EN			0x04
#define BIT_INT_UI_DRDY_INT1_EN				0x08
#define BIT_INT_RESET_DONE_INT1_EN			0x10
#define BIT_INT_PLL_RDY_INT1_EN				0x20
#define BIT_INT_UI_FSYNC_INT1_EN			0x40

/* Bank0 REG_INT_SOURCE1(0x66) */
#define BIT_INT_WOM_X_INT1_EN				0x01
#define BIT_INT_WOM_Y_INT1_EN				0x02
#define BIT_INT_WOM_Z_INT1_EN				0x04
#define BIT_INT_SMD_INT1_EN					0x08
#define BIT_INT_WOM_XYZ_INT1_EN \
	(BIT_INT_WOM_X_INT1_EN | BIT_INT_WOM_Y_INT1_EN | BIT_INT_WOM_Z_INT1_EN)

/* Bank0 REG_SELF_TEST_CONFIG(0x70) */
#define BIT_SELF_TEST_REGULATOR_EN			0x40
#define BIT_TEST_AZ_EN						0x20
#define BIT_TEST_AY_EN						0x10
#define BIT_TEST_AX_EN						0x08
#define BIT_TEST_GZ_EN						0x04
#define BIT_TEST_GY_EN						0x02
#define BIT_TEST_GX_EN						0x01

/* Bank0 REG_SENSOR_SELFTEST_REG1 */
#define BIT_ACCEL_SELF_TEST_PASS			0x08
#define BIT_GYRO_SELF_TEST_PASS				0x04
#define BIT_ACCEL_SELF_TEST_DONE			0x02
#define BIT_GYRO_SELF_TEST_DONE				0x01

/* REG_REG_BANK_SEL(0x76) */
#define BIT_BANK_SEL_0						0x00
#define BIT_BANK_SEL_1						0x01
#define BIT_BANK_SEL_2						0x02
#define BIT_BANK_SEL_3						0x03
#define BIT_BANK_SEL_4						0x04

/* Bank1 REG_INTF_CONFIG5(0x7B) */
#define BIT_PIN9_FUNC_INT2					0x00
#define BIT_PIN9_FUNC_FSYNC					0x02
#define BIT_PIN9_FUNC_CLKIN					0x04
#define BIT_PIN9_FUNC_RSV					0x06

/* Bank4 REG_DRV_GYR_CFG0_REG */
#define GYRO_DRV_TEST_FSMFORCE_D2A_LINEAR_START_MODE			0x0D
#define GYRO_DRV_TEST_FSMFORCE_D2A_STEADY_STATE_AGC_REG_MODE	0x2A

/* Bank4 REG_DRV_GYR_CFG2_REG */
#define GYRO_DRV_SPARE2_D2A_EN				0x01

/* Bank4 REG_INT_SOURCE6 */
#define BIT_INT_TAP_DET_INT1_EN				0x01
#define BIT_INT_SLEEP_DET_INT1_EN			0x02
#define BIT_INT_RAISE_DET_INT1_EN			0x04
#define BIT_INT_TILT_DET_INT1_EN			0x08
#define BIT_INT_STEP_CNT_OVFL_INT1_EN		0x10
#define BIT_INT_STEP_DET_INT1_EN			0x20
#define BIT_INT_DMP_POWER_SAVE_INT1_EN		0x40

/* Bank4 REG_INT_SOURCE7 */
#define BIT_INT_TAP_DET_INT2_EN				0x01
#define BIT_INT_HIGHG_DET_INT2_EN			0x02
#define BIT_INT_LOWG_DET_INT2_EN			0x04
#define BIT_INT_TILT_DET_INT2_EN			0x80
#define BIT_INT_STEP_CNT_OVFL_INT2_EN		0x10
#define BIT_INT_STEP_DET_INT2_EN			0x20
#define BIT_INT_DMP_POWER_SAVE_INT2_EN		0x40

/* Bank4 REG_INT_SOURCE8 */
#define BIT_INT_AGC_RDY_IBI_EN				0x01
#define BIT_INT_FIFO_FULL_IBI_EN			0x02
#define BIT_INT_FIFO_THS_IBI_EN				0x04
#define BIT_INT_UI_DRDY_IBI_EN				0x08
#define BIT_INT_PLL_RDY_IBI_EN				0x10
#define BIT_INT_FSYNC_IBI_EN				0x20
#define BIT_INT_OIS1_DRDY_IBI_EN			0x40

/* Bank4 REG_INT_SOURCE9 */
#define BIT_INT_DMP_POWER_SAVE_IBI_EN		0x01
#define BIT_INT_WOM_X_IBI_EN				0x02
#define BIT_INT_WOM_Y_IBI_EN				0x04
#define BIT_INT_WOM_Z_IBI_EN				0x08
#define BIT_INT_SMD_IBI_EN					0x10

/* Bank4 REG_INT_SOURCE10 */
#define BIT_INT_TAP_DET_IBI_EN				0x01
#define BIT_INT_HIGHG_DET_IBI_EN			0x02
#define BIT_INT_LOWG_DET_IBI_EN				0x04
#define BIT_INT_TILT_DET_IBI_EN				0x08
#define BIT_INT_STEP_CNT_OVFL_IBI_EN		0x10
#define BIT_INT_STEP_DET_IBI_EN				0x20

/* fifo data packet header */
#define BIT_FIFO_HEAD_MSG					0x80
#define BIT_FIFO_HEAD_ACCEL					0x40
#define BIT_FIFO_HEAD_GYRO					0x20
#define BIT_FIFO_HEAD_20					0x10
#define BIT_FIFO_HEAD_TMSP_ODR				0x08
#define BIT_FIFO_HEAD_TMSP_NO_ODR			0x04
#define BIT_FIFO_HEAD_TMSP_FSYNC			0x0C
#define BIT_FIFO_HEAD_ODR_ACCEL				0x02
#define BIT_FIFO_HEAD_ODR_GYRO				0x01

/* data definitions */
#define FIFO_PACKET_BYTE_SINGLE		8
#define FIFO_PACKET_BYTE_6X			16
#define FIFO_PACKET_BYTE_HIRES		20
#define FIFO_COUNT_BYTE				2

/* sensor startup time */
#define INV_ICM42600_GYRO_START_TIME	100
#define INV_ICM42600_ACCEL_START_TIME	50

/*
 * INT configurations
 * Polarity: 0 -> Active Low, 1 -> Active High
 * Drive circuit: 0 -> Open Drain, 1 -> Push-Pull
 * Mode: 0 -> Pulse, 1 -> Latch
 */
#define INT_POLARITY		1
#define INT_DRIVE_CIRCUIT	1
#define INT_MODE			0

/** Describe the content of the FIFO header */
union icm426_fifo_header_t {
	unsigned char Byte;
	struct {
		unsigned char g_odr_change_bit:1;
		unsigned char a_odr_change_bit:1;
		unsigned char fsync_bit:1;
		unsigned char timestamp_bit:1;
		unsigned char twentybits_bit:1;
		unsigned char gyro_bit:1;
		unsigned char accel_bit:1;
		unsigned char msg_bit:1;
	} bits;
};

#define INVALID_VALUE_FIFO              ((s16)0x8000)
#define INVALID_VALUE_FIFO_1B           ((s8)0x80)

