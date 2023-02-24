/*
 * Copyright (c) 2015 MediaTek Inc.
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

#ifndef __MTK_DRRX_REG_H__
#define __MTK_DRRX_REG_H__

/**
 * @file mtk_dprx_reg_int.h
 * Header of register definition
 */

/** @ingroup IP_group_dprx_internal_def
 * @{
 */
#define DPRX_BASE   0x00000000

#define SLAVE_ID_00 0x00000000
#define SLAVE_ID_01 0x00100000
#define SLAVE_ID_02 0x00200000
#define SLAVE_ID_03 0x00300000
#define SLAVE_ID_04 0x00400000
#define SLAVE_ID_05 0x00500000
#define SLAVE_ID_06 0x00600000
#define SLAVE_ID_07 0x00700000
#define SLAVE_ID_08 0x00800000

#define WRAP_BASE   0x00000000

#define WRAPPER_0   0x00000000
#define WRAPPER_1   0x00000100

#define ADDR_PWD_SEL (DPRX_BASE + SLAVE_ID_00 + 0x0000)
#define HDCP1_PWD_SW_EN_FLDMASK                             0x10
#define HDCP1_PWD_SW_EN_FLDMASK_POS                         4
#define HDCP1_PWD_SW_EN_FLDMASK_LEN                         1

#define AUX_PWD_SW_EN_FLDMASK                               0x1
#define AUX_PWD_SW_EN_FLDMASK_POS                           0
#define AUX_PWD_SW_EN_FLDMASK_LEN                           1

#define ADDR_RST_SEL_0 (DPRX_BASE + SLAVE_ID_00 + 0x0010)
#define HDCP2_AUTO_RST_FLDMASK                              0x40
#define HDCP2_AUTO_RST_FLDMASK_POS                          6
#define HDCP2_AUTO_RST_FLDMASK_LEN                          1

#define HDCP1DEC_AUTO_RST_FLDMASK                           0x20
#define HDCP1DEC_AUTO_RST_FLDMASK_POS                       5
#define HDCP1DEC_AUTO_RST_FLDMASK_LEN                       1

#define HDCP1AUTH_AUTO_RST_FLDMASK                          0x10
#define HDCP1AUTH_AUTO_RST_FLDMASK_POS                      4
#define HDCP1AUTH_AUTO_RST_FLDMASK_LEN                      1

#define AUD_AUTO_RST_FLDMASK                                0x8
#define AUD_AUTO_RST_FLDMASK_POS                            3
#define AUD_AUTO_RST_FLDMASK_LEN                            1

#define VID_AUTO_RST_FLDMASK                                0x4
#define VID_AUTO_RST_FLDMASK_POS                            2
#define VID_AUTO_RST_FLDMASK_LEN                            1

#define MAINLINK_AUTO_RST_FLDMASK                           0x2
#define MAINLINK_AUTO_RST_FLDMASK_POS                       1
#define MAINLINK_AUTO_RST_FLDMASK_LEN                       1

#define RXTOP_AUTO_RST_FLDMASK                              0x1
#define RXTOP_AUTO_RST_FLDMASK_POS                          0
#define RXTOP_AUTO_RST_FLDMASK_LEN                          1

#define ADDR_RST_SEL_1 (DPRX_BASE + SLAVE_ID_00 + 0x0014)
#define MAIN_VID_CHANGE_RST_EN_FLDMASK                      0x10
#define MAIN_VID_CHANGE_RST_EN_FLDMASK_POS                  4
#define MAIN_VID_CHANGE_RST_EN_FLDMASK_LEN                  1

#define ALIGN_STATUS_RST_EN_FLDMASK                         0x8
#define ALIGN_STATUS_RST_EN_FLDMASK_POS                     3
#define ALIGN_STATUS_RST_EN_FLDMASK_LEN                     1

#define LANE_DEC_RST_EN_FLDMASK                             0x4
#define LANE_DEC_RST_EN_FLDMASK_POS                         2
#define LANE_DEC_RST_EN_FLDMASK_LEN                         1

#define AUDIOPLL_AUTO_RST_FLDMASK                           0x2
#define AUDIOPLL_AUTO_RST_FLDMASK_POS                       1
#define AUDIOPLL_AUTO_RST_FLDMASK_LEN                       1

#define VIDEOPLL_AUTO_RST_FLDMASK                           0x1
#define VIDEOPLL_AUTO_RST_FLDMASK_POS                       0
#define VIDEOPLL_AUTO_RST_FLDMASK_LEN                       1

#define ADDR_RST_CTRL_0 (DPRX_BASE + SLAVE_ID_00 + 0x0018)
#define RST_SW_RESET_FLDMASK                                0x2
#define RST_SW_RESET_FLDMASK_POS                            1
#define RST_SW_RESET_FLDMASK_LEN                            1

#define RST_LOGIC_RESET_FLDMASK                             0x1
#define RST_LOGIC_RESET_FLDMASK_POS                         0
#define RST_LOGIC_RESET_FLDMASK_LEN                         1

#define ADDR_RST_CTRL_1 (DPRX_BASE + SLAVE_ID_00 + 0x001C)
#define RST_SERDESCTRL_RESET_FLDMASK                        0x80
#define RST_SERDESCTRL_RESET_FLDMASK_POS                    7
#define RST_SERDESCTRL_RESET_FLDMASK_LEN                    1

#define RST_HDCP2_RESET_FLDMASK                             0x40
#define RST_HDCP2_RESET_FLDMASK_POS                         6
#define RST_HDCP2_RESET_FLDMASK_LEN                         1

#define RST_HDCP1DEC_RESET_FLDMASK                          0x20
#define RST_HDCP1DEC_RESET_FLDMASK_POS                      5
#define RST_HDCP1DEC_RESET_FLDMASK_LEN                      1

#define RST_HDCP1AUTH_RESET_FLDMASK                         0x10
#define RST_HDCP1AUTH_RESET_FLDMASK_POS                     4
#define RST_HDCP1AUTH_RESET_FLDMASK_LEN                     1

#define RST_VID_RESET_FLDMASK                               0x8
#define RST_VID_RESET_FLDMASK_POS                           3
#define RST_VID_RESET_FLDMASK_LEN                           1

#define RST_MAINLINK_RESET_FLDMASK                          0x4
#define RST_MAINLINK_RESET_FLDMASK_POS                      2
#define RST_MAINLINK_RESET_FLDMASK_LEN                      1

#define RST_RXTOP_RESET_FLDMASK                             0x2
#define RST_RXTOP_RESET_FLDMASK_POS                         1
#define RST_RXTOP_RESET_FLDMASK_LEN                         1

#define RST_AUX_CH_RESET_FLDMASK                            0x1
#define RST_AUX_CH_RESET_FLDMASK_POS                        0
#define RST_AUX_CH_RESET_FLDMASK_LEN                        1

#define ADDR_RST_CTRL_2 (DPRX_BASE + SLAVE_ID_00 + 0x0020)
#define RST_DSC_DEC_RESET_FLDMASK                           0x20
#define RST_DSC_DEC_RESET_FLDMASK_POS                       5
#define RST_DSC_DEC_RESET_FLDMASK_LEN                       1

#define RST_FEC_DEC_RESET_FLDMASK                           0x10
#define RST_FEC_DEC_RESET_FLDMASK_POS                       4
#define RST_FEC_DEC_RESET_FLDMASK_LEN                       1

#define RST_VIDEOPLL_RESET_FLDMASK                          0x8
#define RST_VIDEOPLL_RESET_FLDMASK_POS                      3
#define RST_VIDEOPLL_RESET_FLDMASK_LEN                      1

#define RST_AUDIOPLL_RESET_FLDMASK                          0x4
#define RST_AUDIOPLL_RESET_FLDMASK_POS                      2
#define RST_AUDIOPLL_RESET_FLDMASK_LEN                      1

#define RST_AUDIOMNADJ_RESET_FLDMASK                        0x2
#define RST_AUDIOMNADJ_RESET_FLDMASK_POS                    1
#define RST_AUDIOMNADJ_RESET_FLDMASK_LEN                    1

#define RST_AUDIO_RESET_FLDMASK                             0x1
#define RST_AUDIO_RESET_FLDMASK_POS                         0
#define RST_AUDIO_RESET_FLDMASK_LEN                         1

#define ADDR_POWER_STATUS (DPRX_BASE + SLAVE_ID_00 + 0x0040)
#define AUX_STATUS_FLDMASK                                  0x4
#define AUX_STATUS_FLDMASK_POS                              2
#define AUX_STATUS_FLDMASK_LEN                              1

#define MAIN_LINK_STATUS_FLDMASK                            0x2
#define MAIN_LINK_STATUS_FLDMASK_POS                        1
#define MAIN_LINK_STATUS_FLDMASK_LEN                        1

#define VIDEO_STATUS_FLDMASK                                0x1
#define VIDEO_STATUS_FLDMASK_POS                            0
#define VIDEO_STATUS_FLDMASK_LEN                            1

#define ADDR_SW_INTR_CTRL (DPRX_BASE + SLAVE_ID_00 + 0x0044)
#define PLL_INTR_FLDMASK                                    0x80
#define PLL_INTR_FLDMASK_POS                                7
#define PLL_INTR_FLDMASK_LEN                                1

#define PLL_INT_MASK_FLDMASK                                0x40
#define PLL_INT_MASK_FLDMASK_POS                            6
#define PLL_INT_MASK_FLDMASK_LEN                            1

#define ADDR_INTR (DPRX_BASE + SLAVE_ID_00 + 0x0048)
#define VIDEO_ON_INT_FLDMASK                                0x80
#define VIDEO_ON_INT_FLDMASK_POS                            7
#define VIDEO_ON_INT_FLDMASK_LEN                            1

#define HDCP1_INTR_FLDMASK                                  0x40
#define HDCP1_INTR_FLDMASK_POS                              6
#define HDCP1_INTR_FLDMASK_LEN                              1

#define SERDES_INTR_FLDMASK                                 0x20
#define SERDES_INTR_FLDMASK_POS                             5
#define SERDES_INTR_FLDMASK_LEN                             1

#define VIDEO_INTR_FLDMASK                                  0x10
#define VIDEO_INTR_FLDMASK_POS                              4
#define VIDEO_INTR_FLDMASK_LEN                              1

#define AUDIO_INTR_FLDMASK                                  0x8
#define AUDIO_INTR_FLDMASK_POS                              3
#define AUDIO_INTR_FLDMASK_LEN                              1

#define DPCD_INTR_FLDMASK                                   0x4
#define DPCD_INTR_FLDMASK_POS                               2
#define DPCD_INTR_FLDMASK_LEN                               1

#define DP_IP_INTR_FLDMASK                                  0x2
#define DP_IP_INTR_FLDMASK_POS                              1
#define DP_IP_INTR_FLDMASK_LEN                              1

#define MAIN_LINK_INTR_FLDMASK                              0x1
#define MAIN_LINK_INTR_FLDMASK_POS                          0
#define MAIN_LINK_INTR_FLDMASK_LEN                          1

#define ADDR_INTR_MASK (DPRX_BASE + SLAVE_ID_00 + 0x004C)
#define INTR_MASK_FLDMASK                                   0xff
#define INTR_MASK_FLDMASK_POS                               0
#define INTR_MASK_FLDMASK_LEN                               8

#define ADDR_INTR_MASK_VID (DPRX_BASE + SLAVE_ID_00 + 0x0050)
#define INTR_MASK_VID_FLDMASK                               0xff
#define INTR_MASK_VID_FLDMASK_POS                           0
#define INTR_MASK_VID_FLDMASK_LEN                           8

#define ADDR_INTR_MASK_AUD (DPRX_BASE + SLAVE_ID_00 + 0x0054)
#define INTR_MASK_AUD_FLDMASK                               0xff
#define INTR_MASK_AUD_FLDMASK_POS                           0
#define INTR_MASK_AUD_FLDMASK_LEN                           8

#define ADDR_INTR_VID (DPRX_BASE + SLAVE_ID_00 + 0x0058)
#define MAIN_LINK_INTR_VID_FLDMASK                          0x1
#define MAIN_LINK_INTR_VID_FLDMASK_POS                      0
#define MAIN_LINK_INTR_VID_FLDMASK_LEN                      1

#define ADDR_INTR_AUD (DPRX_BASE + SLAVE_ID_00 + 0x005C)
#define AUDIO_INTR_AUD_FLDMASK                              0x8
#define AUDIO_INTR_AUD_FLDMASK_POS                          3
#define AUDIO_INTR_AUD_FLDMASK_LEN                          1

#define ADDR_INTR_PLUG_MASK (DPRX_BASE + SLAVE_ID_00 + 0x0060)
#define TX_UNPLUG_AMUTE_MASK_FLDMASK                        0x80
#define TX_UNPLUG_AMUTE_MASK_FLDMASK_POS                    7
#define TX_UNPLUG_AMUTE_MASK_FLDMASK_LEN                    1

#define TX_UNPLUG_VMUTE_MASK_FLDMASK                        0x20
#define TX_UNPLUG_VMUTE_MASK_FLDMASK_POS                    5
#define TX_UNPLUG_VMUTE_MASK_FLDMASK_LEN                    1

#define TX_UNPLUG_INT_MASK_FLDMASK                          0x8
#define TX_UNPLUG_INT_MASK_FLDMASK_POS                      3
#define TX_UNPLUG_INT_MASK_FLDMASK_LEN                      1

#define TX_PLUG_INT_MASK_FLDMASK                            0x4
#define TX_PLUG_INT_MASK_FLDMASK_POS                        2
#define TX_PLUG_INT_MASK_FLDMASK_LEN                        1

#define TX_UNPLUG_INT_FLDMASK                               0x2
#define TX_UNPLUG_INT_FLDMASK_POS                           1
#define TX_UNPLUG_INT_FLDMASK_LEN                           1

#define TX_PLUG_INT_FLDMASK                                 0x1
#define TX_PLUG_INT_FLDMASK_POS                             0
#define TX_PLUG_INT_FLDMASK_LEN                             1

#define ADDR_DPCD_REV (DPRX_BASE + SLAVE_ID_01 + 0x0000)
#define DPCD_MAJOR_REV_FLDMASK                              0xf0
#define DPCD_MAJOR_REV_FLDMASK_POS                          4
#define DPCD_MAJOR_REV_FLDMASK_LEN                          4

#define DPCD_MINOR_REV_FLDMASK                              0xf
#define DPCD_MINOR_REV_FLDMASK_POS                          0
#define DPCD_MINOR_REV_FLDMASK_LEN                          4

#define ADDR_MAX_LINK_RATE (DPRX_BASE + SLAVE_ID_01 + 0x0004)
#define MAX_LINK_RATE_FLDMASK                               0xff
#define MAX_LINK_RATE_FLDMASK_POS                           0
#define MAX_LINK_RATE_FLDMASK_LEN                           8

#define ADDR_MAX_LANE_COUNT (DPRX_BASE + SLAVE_ID_01 + 0x0008)
#define ENHANCE_FRAME_CAP_FLDMASK                           0x80
#define ENHANCE_FRAME_CAP_FLDMASK_POS                       7
#define ENHANCE_FRAME_CAP_FLDMASK_LEN                       1

#define TPS3_SUPPORTED_FLDMASK                              0x40
#define TPS3_SUPPORTED_FLDMASK_POS                          6
#define TPS3_SUPPORTED_FLDMASK_LEN                          1

#define POST_LT_ADJ_REQ_SUPPORTED_FLDMASK                   0x20
#define POST_LT_ADJ_REQ_SUPPORTED_FLDMASK_POS               5
#define POST_LT_ADJ_REQ_SUPPORTED_FLDMASK_LEN               1

#define MAX_LANE_COUNT_FLDMASK                              0x1f
#define MAX_LANE_COUNT_FLDMASK_POS                          0
#define MAX_LANE_COUNT_FLDMASK_LEN                          5

#define ADDR_MAX_DOWNSPREAD (DPRX_BASE + SLAVE_ID_01 + 0x000c)
#define TPS4_SUPPORTED_FLDMASK                              0x80
#define TPS4_SUPPORTED_FLDMASK_POS                          7
#define TPS4_SUPPORTED_FLDMASK_LEN                          1

#define NO_AUX_TRANSACTION_LINK_TRAINING_FLDMASK            0x40
#define NO_AUX_TRANSACTION_LINK_TRAINING_FLDMASK_POS        6
#define NO_AUX_TRANSACTION_LINK_TRAINING_FLDMASK_LEN        1

#define MAX_DOWNSPREAD_FLDMASK                              0x1
#define MAX_DOWNSPREAD_FLDMASK_POS                          0
#define MAX_DOWNSPREAD_FLDMASK_LEN                          1

#define ADDR_NORP_DP_PWR_VOLTAGE_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0010)
#define VOL18V_DP_PWR_CAP_FLDMASK                           0x80
#define VOL18V_DP_PWR_CAP_FLDMASK_POS                       7
#define VOL18V_DP_PWR_CAP_FLDMASK_LEN                       1

#define VOL12V_DP_PWR_CAP_FLDMASK                           0x40
#define VOL12V_DP_PWR_CAP_FLDMASK_POS                       6
#define VOL12V_DP_PWR_CAP_FLDMASK_LEN                       1

#define VOL5V_DP_PWR_CAP_FLDMASK                            0x20
#define VOL5V_DP_PWR_CAP_FLDMASK_POS                        5
#define VOL5V_DP_PWR_CAP_FLDMASK_LEN                        1

#define NORP_FLDMASK                                        0x1
#define NORP_FLDMASK_POS                                    0
#define NORP_FLDMASK_LEN                                    1

#define ADDR_DOWN_STREAM_PORT_PRESENT (DPRX_BASE + SLAVE_ID_01 + 0x0014)
#define DETAILED_CAP_INFO_AVAILABLE_FLDMASK                 0x10
#define DETAILED_CAP_INFO_AVAILABLE_FLDMASK_POS             4
#define DETAILED_CAP_INFO_AVAILABLE_FLDMASK_LEN             1

#define FORMAT_CONVERSION_FLDMASK                           0x8
#define FORMAT_CONVERSION_FLDMASK_POS                       3
#define FORMAT_CONVERSION_FLDMASK_LEN                       1

#define DFP_TYPE_FLDMASK                                    0x6
#define DFP_TYPE_FLDMASK_POS                                1
#define DFP_TYPE_FLDMASK_LEN                                2

#define DFP_PRESENT_FLDMASK                                 0x1
#define DFP_PRESENT_FLDMASK_POS                             0
#define DFP_PRESENT_FLDMASK_LEN                             1

#define ADDR_MAIN_LINK_CHANNEL_CODING (DPRX_BASE + SLAVE_ID_01 + 0x0018)
#define ANSI8B10B_FLDMASK                                   0x1
#define ANSI8B10B_FLDMASK_POS                               0
#define ANSI8B10B_FLDMASK_LEN                               1

#define ADDR_DOWN_STREAM_PORT_COUNT (DPRX_BASE + SLAVE_ID_01 + 0x001C)
#define OUI_SUPPORT_FLDMASK                                 0x80
#define OUI_SUPPORT_FLDMASK_POS                             7
#define OUI_SUPPORT_FLDMASK_LEN                             1

#define MSA_TIMING_PAR_IGNORED_FLDMASK                      0x40
#define MSA_TIMING_PAR_IGNORED_FLDMASK_POS                  6
#define MSA_TIMING_PAR_IGNORED_FLDMASK_LEN                  1

#define DFP_COUNT_FLDMASK                                   0xf
#define DFP_COUNT_FLDMASK_POS                               0
#define DFP_COUNT_FLDMASK_LEN                               4

#define ADDR_RECEIVE_PORT0_CAP_0 (DPRX_BASE + SLAVE_ID_01 + 0x0020)
#define BUFFER_SIZE_PER_PORT_0_FLDMASK                      0x20
#define BUFFER_SIZE_PER_PORT_0_FLDMASK_POS                  5
#define BUFFER_SIZE_PER_PORT_0_FLDMASK_LEN                  1

#define BUFFER_SIZE_UNIT_0_FLDMASK                          0x10
#define BUFFER_SIZE_UNIT_0_FLDMASK_POS                      4
#define BUFFER_SIZE_UNIT_0_FLDMASK_LEN                      1

#define HBLANK_EXPANSION_CAPABLE_0_FLDMASK                  0x8
#define HBLANK_EXPANSION_CAPABLE_0_FLDMASK_POS              3
#define HBLANK_EXPANSION_CAPABLE_0_FLDMASK_LEN              1

#define ASSOCIATED_TO_PRECEDING_PORT_0_FLDMASK              0x4
#define ASSOCIATED_TO_PRECEDING_PORT_0_FLDMASK_POS          2
#define ASSOCIATED_TO_PRECEDING_PORT_0_FLDMASK_LEN          1

#define LOCAL_EDID_PRESENT_0_FLDMASK                        0x2
#define LOCAL_EDID_PRESENT_0_FLDMASK_POS                    1
#define LOCAL_EDID_PRESENT_0_FLDMASK_LEN                    1

#define ADDR_RECEIVE_PORT0_CAP_1 (DPRX_BASE + SLAVE_ID_01 + 0x0024)
#define BUFFER_SIZE_0_FLDMASK                               0xff
#define BUFFER_SIZE_0_FLDMASK_POS                           0
#define BUFFER_SIZE_0_FLDMASK_LEN                           8

#define ADDR_RECEIVE_PORT1_CAP_0 (DPRX_BASE + SLAVE_ID_01 + 0x0028)
#define BUFFER_SIZE_PER_PORT_1_FLDMASK                      0x20
#define BUFFER_SIZE_PER_PORT_1_FLDMASK_POS                  5
#define BUFFER_SIZE_PER_PORT_1_FLDMASK_LEN                  1

#define BUFFER_SIZE_UNIT_1_FLDMASK                          0x10
#define BUFFER_SIZE_UNIT_1_FLDMASK_POS                      4
#define BUFFER_SIZE_UNIT_1_FLDMASK_LEN                      1

#define HBLANK_EXPANSION_CAPABLE_1_FLDMASK                  0x8
#define HBLANK_EXPANSION_CAPABLE_1_FLDMASK_POS              3
#define HBLANK_EXPANSION_CAPABLE_1_FLDMASK_LEN              1

#define ASSOCIATED_TO_PRECEDING_PORT_1_FLDMASK              0x4
#define ASSOCIATED_TO_PRECEDING_PORT_1_FLDMASK_POS          2
#define ASSOCIATED_TO_PRECEDING_PORT_1_FLDMASK_LEN          1

#define LOCAL_EDID_PRESENT_1_FLDMASK                        0x2
#define LOCAL_EDID_PRESENT_1_FLDMASK_POS                    1
#define LOCAL_EDID_PRESENT_1_FLDMASK_LEN                    1

#define ADDR_RECEIVE_PORT1_CAP_1 (DPRX_BASE + SLAVE_ID_01 + 0x002C)
#define BUFFER_SIZE_1_FLDMASK                               0xff
#define BUFFER_SIZE_1_FLDMASK_POS                           0
#define BUFFER_SIZE_1_FLDMASK_LEN                           8

#define ADDR_I2C_SPEED_CONTROL_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0030)
#define I2C_SPEED_CONTROL_CAP_FLDMASK                       0xff
#define I2C_SPEED_CONTROL_CAP_FLDMASK_POS                   0
#define I2C_SPEED_CONTROL_CAP_FLDMASK_LEN                   8

#define ADDR_EDP_CONFIG_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0034)
#define DPCD_DISPLAY_CONTROL_CAPABLE_FLDMASK                0x8
#define DPCD_DISPLAY_CONTROL_CAPABLE_FLDMASK_POS            3
#define DPCD_DISPLAY_CONTROL_CAPABLE_FLDMASK_LEN            1

#define FRAMING_CHANGE_CAPABLE_FLDMASK                      0x2
#define FRAMING_CHANGE_CAPABLE_FLDMASK_POS                  1
#define FRAMING_CHANGE_CAPABLE_FLDMASK_LEN                  1

#define ALTERNATE_SCRAMBLER_RESET_CAPABLE_FLDMASK           0x1
#define ALTERNATE_SCRAMBLER_RESET_CAPABLE_FLDMASK_POS       0
#define ALTERNATE_SCRAMBLER_RESET_CAPABLE_FLDMASK_LEN       1

#define ADDR_TRAINING_AUX_RD_INTERVAL (DPRX_BASE + SLAVE_ID_01 + 0x0038)
#define EXTENDED_RX_CAP_FIELD_PRESENT_FLDMASK               0x80
#define EXTENDED_RX_CAP_FIELD_PRESENT_FLDMASK_POS           7
#define EXTENDED_RX_CAP_FIELD_PRESENT_FLDMASK_LEN           1

#define TRAINING_AUX_RD_INTERVAL_FLDMASK                    0x7f
#define TRAINING_AUX_RD_INTERVAL_FLDMASK_POS                0
#define TRAINING_AUX_RD_INTERVAL_FLDMASK_LEN                7

#define ADDR_ADAPTOR_CAP (DPRX_BASE + SLAVE_ID_01 + 0x003C)
#define ALTERNATE_I2C_PATTERN_CAP_FLDMASK                   0x2
#define ALTERNATE_I2C_PATTERN_CAP_FLDMASK_POS               1
#define ALTERNATE_I2C_PATTERN_CAP_FLDMASK_LEN               1

#define FORCE_LOAD_SENSE_CAP_FLDMASK                        0x1
#define FORCE_LOAD_SENSE_CAP_FLDMASK_POS                    0
#define FORCE_LOAD_SENSE_CAP_FLDMASK_LEN                    1

#define ADDR_MSTM_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0084)
#define MST_CAP_FLDMASK                                     0x1
#define MST_CAP_FLDMASK_POS                                 0
#define MST_CAP_FLDMASK_LEN                                 1

#define ADDR_NUM_OF_AUDIO_ENDPOINTS (DPRX_BASE + SLAVE_ID_01 + 0x0088)
#define NUM_OF_AUDIO_ENDPOINTS_FLDMASK                      0xff
#define NUM_OF_AUDIO_ENDPOINTS_FLDMASK_POS                  0
#define NUM_OF_AUDIO_ENDPOINTS_FLDMASK_LEN                  8

#define ADDR_AV_SYNC_DATA_BLOCK_AG_VG_FACTOR (DPRX_BASE + SLAVE_ID_01 + 0x008C)
#define AV_SYNC_DATA_BLOCK_AG_VG_FACTOR_FLDMASK             0xff
#define AV_SYNC_DATA_BLOCK_AG_VG_FACTOR_FLDMASK_POS         0
#define AV_SYNC_DATA_BLOCK_AG_VG_FACTOR_FLDMASK_LEN         8

#define ADDR_AV_SYNC_DATA_BLOCK_0 (DPRX_BASE + SLAVE_ID_01 + 0x0090)
#define AV_SYNC_DATA_BLOCK_0_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_0_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_0_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_1 (DPRX_BASE + SLAVE_ID_01 + 0x0094)
#define AV_SYNC_DATA_BLOCK_1_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_1_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_1_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_2 (DPRX_BASE + SLAVE_ID_01 + 0x0098)
#define AV_SYNC_DATA_BLOCK_2_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_2_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_2_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_3 (DPRX_BASE + SLAVE_ID_01 + 0x009C)
#define AV_SYNC_DATA_BLOCK_3_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_3_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_3_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_4 (DPRX_BASE + SLAVE_ID_01 + 0x00A0)
#define AV_SYNC_DATA_BLOCK_4_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_4_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_4_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_5 (DPRX_BASE + SLAVE_ID_01 + 0x00A4)
#define AV_SYNC_DATA_BLOCK_5_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_5_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_5_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_6 (DPRX_BASE + SLAVE_ID_01 + 0x00A8)
#define AV_SYNC_DATA_BLOCK_6_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_6_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_6_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_7 (DPRX_BASE + SLAVE_ID_01 + 0x00AC)
#define AV_SYNC_DATA_BLOCK_7_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_7_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_7_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_8 (DPRX_BASE + SLAVE_ID_01 + 0x00B0)
#define AV_SYNC_DATA_BLOCK_8_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_8_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_8_FLDMASK_LEN                    8

#define ADDR_AV_SYNC_DATA_BLOCK_9 (DPRX_BASE + SLAVE_ID_01 + 0x00B4)
#define AV_SYNC_DATA_BLOCK_9_FLDMASK                        0xff
#define AV_SYNC_DATA_BLOCK_9_FLDMASK_POS                    0
#define AV_SYNC_DATA_BLOCK_9_FLDMASK_LEN                    8

#define ADDR_GUID_0 (DPRX_BASE + SLAVE_ID_01 + 0x00C0)
#define GUID_0_FLDMASK                                      0xff
#define GUID_0_FLDMASK_POS                                  0
#define GUID_0_FLDMASK_LEN                                  8

#define ADDR_GUID_1 (DPRX_BASE + SLAVE_ID_01 + 0x00C4)
#define GUID_1_FLDMASK                                      0xff
#define GUID_1_FLDMASK_POS                                  0
#define GUID_1_FLDMASK_LEN                                  8

#define ADDR_GUID_2 (DPRX_BASE + SLAVE_ID_01 + 0x00C8)
#define GUID_2_FLDMASK                                      0xff
#define GUID_2_FLDMASK_POS                                  0
#define GUID_2_FLDMASK_LEN                                  8

#define ADDR_GUID_3 (DPRX_BASE + SLAVE_ID_01 + 0x00CC)
#define GUID_3_FLDMASK                                      0xff
#define GUID_3_FLDMASK_POS                                  0
#define GUID_3_FLDMASK_LEN                                  8

#define ADDR_GUID_4 (DPRX_BASE + SLAVE_ID_01 + 0x00D0)
#define GUID_4_FLDMASK                                      0xff
#define GUID_4_FLDMASK_POS                                  0
#define GUID_4_FLDMASK_LEN                                  8

#define ADDR_GUID_5 (DPRX_BASE + SLAVE_ID_01 + 0x00D4)
#define GUID_5_FLDMASK                                      0xff
#define GUID_5_FLDMASK_POS                                  0
#define GUID_5_FLDMASK_LEN                                  8

#define ADDR_GUID_6 (DPRX_BASE + SLAVE_ID_01 + 0x00D8)
#define GUID_6_FLDMASK                                      0xff
#define GUID_6_FLDMASK_POS                                  0
#define GUID_6_FLDMASK_LEN                                  8

#define ADDR_GUID_7 (DPRX_BASE + SLAVE_ID_01 + 0x00DC)
#define GUID_7_FLDMASK                                      0xff
#define GUID_7_FLDMASK_POS                                  0
#define GUID_7_FLDMASK_LEN                                  8

#define ADDR_GUID_8 (DPRX_BASE + SLAVE_ID_01 + 0x00E0)
#define GUID_8_FLDMASK                                      0xff
#define GUID_8_FLDMASK_POS                                  0
#define GUID_8_FLDMASK_LEN                                  8

#define ADDR_GUID_9 (DPRX_BASE + SLAVE_ID_01 + 0x00E4)
#define GUID_9_FLDMASK                                      0xff
#define GUID_9_FLDMASK_POS                                  0
#define GUID_9_FLDMASK_LEN                                  8

#define ADDR_GUID_A (DPRX_BASE + SLAVE_ID_01 + 0x00E8)
#define GUID_A_FLDMASK                                      0xff
#define GUID_A_FLDMASK_POS                                  0
#define GUID_A_FLDMASK_LEN                                  8

#define ADDR_GUID_B (DPRX_BASE + SLAVE_ID_01 + 0x00EC)
#define GUID_B_FLDMASK                                      0xff
#define GUID_B_FLDMASK_POS                                  0
#define GUID_B_FLDMASK_LEN                                  8

#define ADDR_GUID_C (DPRX_BASE + SLAVE_ID_01 + 0x00F0)
#define GUID_C_FLDMASK                                      0xff
#define GUID_C_FLDMASK_POS                                  0
#define GUID_C_FLDMASK_LEN                                  8

#define ADDR_GUID_D (DPRX_BASE + SLAVE_ID_01 + 0x00F4)
#define GUID_D_FLDMASK                                      0xff
#define GUID_D_FLDMASK_POS                                  0
#define GUID_D_FLDMASK_LEN                                  8

#define ADDR_GUID_E (DPRX_BASE + SLAVE_ID_01 + 0x00F8)
#define GUID_E_FLDMASK                                      0xff
#define GUID_E_FLDMASK_POS                                  0
#define GUID_E_FLDMASK_LEN                                  8

#define ADDR_GUID_F (DPRX_BASE + SLAVE_ID_01 + 0x00FC)
#define GUID_F_FLDMASK                                      0xff
#define GUID_F_FLDMASK_POS                                  0
#define GUID_F_FLDMASK_LEN                                  8

#define ADDR_RX_GTC_VALUE_0 (DPRX_BASE + SLAVE_ID_01 + 0x0150)
#define RX_GTC_VALUE_0_FLDMASK                              0xff
#define RX_GTC_VALUE_0_FLDMASK_POS                          0
#define RX_GTC_VALUE_0_FLDMASK_LEN                          8

#define ADDR_RX_GTC_VALUE_1 (DPRX_BASE + SLAVE_ID_01 + 0x0154)
#define RX_GTC_VALUE_1_FLDMASK                              0xff
#define RX_GTC_VALUE_1_FLDMASK_POS                          0
#define RX_GTC_VALUE_1_FLDMASK_LEN                          8

#define ADDR_RX_GTC_VALUE_2 (DPRX_BASE + SLAVE_ID_01 + 0x0158)
#define RX_GTC_VALUE_2_FLDMASK                              0xff
#define RX_GTC_VALUE_2_FLDMASK_POS                          0
#define RX_GTC_VALUE_2_FLDMASK_LEN                          8

#define ADDR_RX_GTC_VALUE_3 (DPRX_BASE + SLAVE_ID_01 + 0x015C)
#define RX_GTC_VALUE_3_FLDMASK                              0xff
#define RX_GTC_VALUE_3_FLDMASK_POS                          0
#define RX_GTC_VALUE_3_FLDMASK_LEN                          8

#define ADDR_RX_GTC_MSTR_REQ (DPRX_BASE + SLAVE_ID_01 + 0x0160)
#define TX_GTC_VALUE_PHASE_SKEW_EN_FLDMASK                  0x2
#define TX_GTC_VALUE_PHASE_SKEW_EN_FLDMASK_POS              1
#define TX_GTC_VALUE_PHASE_SKEW_EN_FLDMASK_LEN              1

#define RX_GTC_MSTR_REQ_FLDMASK                             0x1
#define RX_GTC_MSTR_REQ_FLDMASK_POS                         0
#define RX_GTC_MSTR_REQ_FLDMASK_LEN                         1

#define ADDR_RX_GTC_FREQ_LOCK_DONE (DPRX_BASE + SLAVE_ID_01 + 0x0164)
#define RX_GTC_FREQ_LOCK_DONE_FLDMASK                       0x1
#define RX_GTC_FREQ_LOCK_DONE_FLDMASK_POS                   0
#define RX_GTC_FREQ_LOCK_DONE_FLDMASK_LEN                   1

#define ADDR_RX_GTC_PHASE_SKEW_OFFSET_0 (DPRX_BASE + SLAVE_ID_01 + 0x0168)
#define RX_GTC_PHASE_SKEW_OFFSET_0_FLDMASK                  0xff
#define RX_GTC_PHASE_SKEW_OFFSET_0_FLDMASK_POS              0
#define RX_GTC_PHASE_SKEW_OFFSET_0_FLDMASK_LEN              8

#define ADDR_RX_GTC_PHASE_SKEW_OFFSET_1 (DPRX_BASE + SLAVE_ID_01 + 0x016C)
#define RX_GTC_PHASE_SKEW_OFFSET_1_FLDMASK                  0xff
#define RX_GTC_PHASE_SKEW_OFFSET_1_FLDMASK_POS              0
#define RX_GTC_PHASE_SKEW_OFFSET_1_FLDMASK_LEN              8

#define ADDR_DSC_SUPPORT (DPRX_BASE + SLAVE_ID_01 + 0x0180)
#define DSC_SUPPORT_FLDMASK                                 0x1
#define DSC_SUPPORT_FLDMASK_POS                             0
#define DSC_SUPPORT_FLDMASK_LEN                             1

#define ADDR_DSC_ALGORITHM_REVISION (DPRX_BASE + SLAVE_ID_01 + 0x0184)
#define DSC_MIN_VER_FLDMASK                                 0xf0
#define DSC_MIN_VER_FLDMASK_POS                             4
#define DSC_MIN_VER_FLDMASK_LEN                             4

#define DSC_MAJ_VER_FLDMASK                                 0xf
#define DSC_MAJ_VER_FLDMASK_POS                             0
#define DSC_MAJ_VER_FLDMASK_LEN                             4

#define ADDR_DSC_RC_BUF_BLOCK_SIZE (DPRX_BASE + SLAVE_ID_01 + 0x0188)
#define RC_BUF_BLOCK_SIZE_FLDMASK                           0x3
#define RC_BUF_BLOCK_SIZE_FLDMASK_POS                       0
#define RC_BUF_BLOCK_SIZE_FLDMASK_LEN                       2

#define ADDR_DSC_RC_BUF_SIZE (DPRX_BASE + SLAVE_ID_01 + 0x018C)
#define DSC_BUF_SIZE_FLDMASK                                0xff
#define DSC_BUF_SIZE_FLDMASK_POS                            0
#define DSC_BUF_SIZE_FLDMASK_LEN                            8

#define ADDR_DSC_SLICE_CAP_1 (DPRX_BASE + SLAVE_ID_01 + 0x0190)
#define DSC_SLICE_CAP_1_FLDMASK                             0xff
#define DSC_SLICE_CAP_1_FLDMASK_POS                         0
#define DSC_SLICE_CAP_1_FLDMASK_LEN                         8

#define ADDR_DSC_LINE_BUF_BPP (DPRX_BASE + SLAVE_ID_01 + 0x0194)
#define DSC_LINE_BUF_BPP_FLDMASK                            0xf
#define DSC_LINE_BUF_BPP_FLDMASK_POS                        0
#define DSC_LINE_BUF_BPP_FLDMASK_LEN                        4

#define ADDR_DSC_BLOCK_PREDICTION_SUPPORT (DPRX_BASE + SLAVE_ID_01 + 0x0198)
#define DSC_BLOCK_PREDICTION_SUPPORT_FLDMASK                0x1
#define DSC_BLOCK_PREDICTION_SUPPORT_FLDMASK_POS            0
#define DSC_BLOCK_PREDICTION_SUPPORT_FLDMASK_LEN            1

#define ADDR_DSC_DEC_COLOR_FORMAT_CAP (DPRX_BASE + SLAVE_ID_01 + 0x01A4)
#define NATIVEYCBCR420_SUPPORT_FLDMASK                      0x10
#define NATIVEYCBCR420_SUPPORT_FLDMASK_POS                  4
#define NATIVEYCBCR420_SUPPORT_FLDMASK_LEN                  1

#define NATIVEYCBCR422_SUPPORT_FLDMASK                      0x8
#define NATIVEYCBCR422_SUPPORT_FLDMASK_POS                  3
#define NATIVEYCBCR422_SUPPORT_FLDMASK_LEN                  1

#define SIMPLEYCBCR422_SUPPORT_FLDMASK                      0x4
#define SIMPLEYCBCR422_SUPPORT_FLDMASK_POS                  2
#define SIMPLEYCBCR422_SUPPORT_FLDMASK_LEN                  1

#define YCBCR444_SUPPORT_FLDMASK                            0x2
#define YCBCR444_SUPPORT_FLDMASK_POS                        1
#define YCBCR444_SUPPORT_FLDMASK_LEN                        1

#define RGB_SUPPORT_FLDMASK                                 0x1
#define RGB_SUPPORT_FLDMASK_POS                             0
#define RGB_SUPPORT_FLDMASK_LEN                             1

#define ADDR_DSC_DEC_COLOR_DEPTH_CAP (DPRX_BASE + SLAVE_ID_01 + 0x01A8)
#define BPC12_SUPPORT_FLDMASK                               0x8
#define BPC12_SUPPORT_FLDMASK_POS                           3
#define BPC12_SUPPORT_FLDMASK_LEN                           1

#define BPC10_SUPPORT_FLDMASK                               0x4
#define BPC10_SUPPORT_FLDMASK_POS                           2
#define BPC10_SUPPORT_FLDMASK_LEN                           1

#define BPC8_SUPPORT_FLDMASK                                0x2
#define BPC8_SUPPORT_FLDMASK_POS                            1
#define BPC8_SUPPORT_FLDMASK_LEN                            1

#define ADDR_PEAK_DSC_THROUGHPUT (DPRX_BASE + SLAVE_ID_01 + 0x01AC)
#define THROUGHPUT_MODE_1_FLDMASK                           0xf0
#define THROUGHPUT_MODE_1_FLDMASK_POS                       4
#define THROUGHPUT_MODE_1_FLDMASK_LEN                       4

#define THROUGHPUT_MODE_0_FLDMASK                           0xf
#define THROUGHPUT_MODE_0_FLDMASK_POS                       0
#define THROUGHPUT_MODE_0_FLDMASK_LEN                       4

#define ADDR_DSC_MAX_SLICE_WIDTH (DPRX_BASE + SLAVE_ID_01 + 0x01B0)
#define DSC_MAX_SLICE_WIDTH_FLDMASK                         0xff
#define DSC_MAX_SLICE_WIDTH_FLDMASK_POS                     0
#define DSC_MAX_SLICE_WIDTH_FLDMASK_LEN                     8

#define ADDR_DSC_SLICE_CAP_2 (DPRX_BASE + SLAVE_ID_01 + 0x01B4)
#define DSC_SLICE_CAP_2_FLDMASK                             0xff
#define DSC_SLICE_CAP_2_FLDMASK_POS                         0
#define DSC_SLICE_CAP_2_FLDMASK_LEN                         8

#define ADDR_BITS_PER_PIXEL_INCR (DPRX_BASE + SLAVE_ID_01 + 0x01BC)
#define BITS_PER_PIXEL_INCR_FLDMASK                         0x7
#define BITS_PER_PIXEL_INCR_FLDMASK_POS                     0
#define BITS_PER_PIXEL_INCR_FLDMASK_LEN                     3

#define ADDR_DFP0_0_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0200)
#define DFP0_0_CAP_FLDMASK                                  0xff
#define DFP0_0_CAP_FLDMASK_POS                              0
#define DFP0_0_CAP_FLDMASK_LEN                              8

#define ADDR_DFP0_1_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0204)
#define DFP0_1_CAP_FLDMASK                                  0xff
#define DFP0_1_CAP_FLDMASK_POS                              0
#define DFP0_1_CAP_FLDMASK_LEN                              8

#define ADDR_DFP0_2_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0208)
#define DFP0_2_CAP_FLDMASK                                  0xff
#define DFP0_2_CAP_FLDMASK_POS                              0
#define DFP0_2_CAP_FLDMASK_LEN                              8

#define ADDR_DFP0_3_CAP (DPRX_BASE + SLAVE_ID_01 + 0x020C)
#define DFP0_3_CAP_FLDMASK                                  0xff
#define DFP0_3_CAP_FLDMASK_POS                              0
#define DFP0_3_CAP_FLDMASK_LEN                              8

#define ADDR_DFP1_0_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0210)
#define DFP1_0_CAP_FLDMASK                                  0xff
#define DFP1_0_CAP_FLDMASK_POS                              0
#define DFP1_0_CAP_FLDMASK_LEN                              8

#define ADDR_DFP1_1_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0214)
#define DFP1_1_CAP_FLDMASK                                  0xff
#define DFP1_1_CAP_FLDMASK_POS                              0
#define DFP1_1_CAP_FLDMASK_LEN                              8

#define ADDR_DFP1_2_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0218)
#define DFP1_2_CAP_FLDMASK                                  0xff
#define DFP1_2_CAP_FLDMASK_POS                              0
#define DFP1_2_CAP_FLDMASK_LEN                              8

#define ADDR_DFP1_3_CAP (DPRX_BASE + SLAVE_ID_01 + 0x021C)
#define DFP1_3_CAP_FLDMASK                                  0xff
#define DFP1_3_CAP_FLDMASK_POS                              0
#define DFP1_3_CAP_FLDMASK_LEN                              8

#define ADDR_DFP2_0_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0220)
#define DFP2_0_CAP_FLDMASK                                  0xff
#define DFP2_0_CAP_FLDMASK_POS                              0
#define DFP2_0_CAP_FLDMASK_LEN                              8

#define ADDR_DFP2_1_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0224)
#define DFP2_1_CAP_FLDMASK                                  0xff
#define DFP2_1_CAP_FLDMASK_POS                              0
#define DFP2_1_CAP_FLDMASK_LEN                              8

#define ADDR_DFP2_2_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0228)
#define DFP2_2_CAP_FLDMASK                                  0xff
#define DFP2_2_CAP_FLDMASK_POS                              0
#define DFP2_2_CAP_FLDMASK_LEN                              8

#define ADDR_DFP2_3_CAP (DPRX_BASE + SLAVE_ID_01 + 0x022C)
#define DFP2_3_CAP_FLDMASK                                  0xff
#define DFP2_3_CAP_FLDMASK_POS                              0
#define DFP2_3_CAP_FLDMASK_LEN                              8

#define ADDR_DFP3_0_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0230)
#define DFP3_0_CAP_FLDMASK                                  0xff
#define DFP3_0_CAP_FLDMASK_POS                              0
#define DFP3_0_CAP_FLDMASK_LEN                              8

#define ADDR_DFP3_1_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0234)
#define DFP3_1_CAP_FLDMASK                                  0xff
#define DFP3_1_CAP_FLDMASK_POS                              0
#define DFP3_1_CAP_FLDMASK_LEN                              8

#define ADDR_DFP3_2_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0238)
#define DFP3_2_CAP_FLDMASK                                  0xff
#define DFP3_2_CAP_FLDMASK_POS                              0
#define DFP3_2_CAP_FLDMASK_LEN                              8

#define ADDR_DFP3_3_CAP (DPRX_BASE + SLAVE_ID_01 + 0x023C)
#define DFP3_3_CAP_FLDMASK                                  0xff
#define DFP3_3_CAP_FLDMASK_POS                              0
#define DFP3_3_CAP_FLDMASK_LEN                              8

#define ADDR_FEC_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0240)
#define FEC_ERROR_REPORTING_POLICY_SUPPORTED_FLDMASK        0x80
#define FEC_ERROR_REPORTING_POLICY_SUPPORTED_FLDMASK_POS    7
#define FEC_ERROR_REPORTING_POLICY_SUPPORTED_FLDMASK_LEN    1

#define PARITY_ERR_COUNT_CAP_FLDMASK                        0x20
#define PARITY_ERR_COUNT_CAP_FLDMASK_POS                    5
#define PARITY_ERR_COUNT_CAP_FLDMASK_LEN                    1

#define PARITY_BLOCK_ERR_COUNT_CAP_FLDMASK                  0x10
#define PARITY_BLOCK_ERR_COUNT_CAP_FLDMASK_POS              4
#define PARITY_BLOCK_ERR_COUNT_CAP_FLDMASK_LEN              1

#define BIT_ERROR_COUNT_CAP_FLDMASK                         0x8
#define BIT_ERROR_COUNT_CAP_FLDMASK_POS                     3
#define BIT_ERROR_COUNT_CAP_FLDMASK_LEN                     1

#define CORRECTED_BLK_ERR_CNT_CAP_FLDMASK                   0x4
#define CORRECTED_BLK_ERR_CNT_CAP_FLDMASK_POS               2
#define CORRECTED_BLK_ERR_CNT_CAP_FLDMASK_LEN               1

#define UNCORRECTED_BLK_ERR_CNT_CAP_FLDMASK                 0x2
#define UNCORRECTED_BLK_ERR_CNT_CAP_FLDMASK_POS             1
#define UNCORRECTED_BLK_ERR_CNT_CAP_FLDMASK_LEN             1

#define FEC_CAP_FLDMASK                                     0x1
#define FEC_CAP_FLDMASK_POS                                 0
#define FEC_CAP_FLDMASK_LEN                                 1

#define ADDR_LINK_BW_SET (DPRX_BASE + SLAVE_ID_01 + 0x0400)
#define LINK_BW_SET_FLDMASK                                 0xff
#define LINK_BW_SET_FLDMASK_POS                             0
#define LINK_BW_SET_FLDMASK_LEN                             8

#define ADDR_LANE_COUNT_SET (DPRX_BASE + SLAVE_ID_01 + 0x0404)
#define ENHANCED_FRAME_EN_FLDMASK                           0x80
#define ENHANCED_FRAME_EN_FLDMASK_POS                       7
#define ENHANCED_FRAME_EN_FLDMASK_LEN                       1

#define POST_LT_ADJ_REQ_GRANTED_FLDMASK                     0x20
#define POST_LT_ADJ_REQ_GRANTED_FLDMASK_POS                 5
#define POST_LT_ADJ_REQ_GRANTED_FLDMASK_LEN                 1

#define LANE_COUNT_SET_FLDMASK                              0x1f
#define LANE_COUNT_SET_FLDMASK_POS                          0
#define LANE_COUNT_SET_FLDMASK_LEN                          5

#define ADDR_TRAINING_PATTERN_SET (DPRX_BASE + SLAVE_ID_01 + 0x0408)
#define SYMBOL_ERROR_CNT_SEL_FLDMASK                        0xc0
#define SYMBOL_ERROR_CNT_SEL_FLDMASK_POS                    6
#define SYMBOL_ERROR_CNT_SEL_FLDMASK_LEN                    2

#define SCRAMBLING_DISABLE_FLDMASK                          0x20
#define SCRAMBLING_DISABLE_FLDMASK_POS                      5
#define SCRAMBLING_DISABLE_FLDMASK_LEN                      1

#define RECOVERED_CLOCK_OUT_EN_FLDMASK                      0x10
#define RECOVERED_CLOCK_OUT_EN_FLDMASK_POS                  4
#define RECOVERED_CLOCK_OUT_EN_FLDMASK_LEN                  1

#define TRAINING_PATTERN_SEL_FLDMASK                        0xf
#define TRAINING_PATTERN_SEL_FLDMASK_POS                    0
#define TRAINING_PATTERN_SEL_FLDMASK_LEN                    4

#define ADDR_TRAINING_LANE0_SET (DPRX_BASE + SLAVE_ID_01 + 0x040C)
#define MAX_PRE_EMPHASIS_REACHED_LANE0_FLDMASK              0x20
#define MAX_PRE_EMPHASIS_REACHED_LANE0_FLDMASK_POS          5
#define MAX_PRE_EMPHASIS_REACHED_LANE0_FLDMASK_LEN          1

#define PRE_EMPHASIS_SET_LANE0_FLDMASK                      0x18
#define PRE_EMPHASIS_SET_LANE0_FLDMASK_POS                  3
#define PRE_EMPHASIS_SET_LANE0_FLDMASK_LEN                  2

#define MAX_CURRENT_REACHED_LANE0_FLDMASK                   0x4
#define MAX_CURRENT_REACHED_LANE0_FLDMASK_POS               2
#define MAX_CURRENT_REACHED_LANE0_FLDMASK_LEN               1

#define DRIVE_CURRENT_SET_LANE0_FLDMASK                     0x3
#define DRIVE_CURRENT_SET_LANE0_FLDMASK_POS                 0
#define DRIVE_CURRENT_SET_LANE0_FLDMASK_LEN                 2

#define ADDR_TRAINING_LANE1_SET (DPRX_BASE + SLAVE_ID_01 + 0x0410)
#define MAX_PRE_EMPHASIS_REACHED_LANE1_FLDMASK              0x20
#define MAX_PRE_EMPHASIS_REACHED_LANE1_FLDMASK_POS          5
#define MAX_PRE_EMPHASIS_REACHED_LANE1_FLDMASK_LEN          1

#define PRE_EMPHASIS_SET_LANE1_FLDMASK                      0x18
#define PRE_EMPHASIS_SET_LANE1_FLDMASK_POS                  3
#define PRE_EMPHASIS_SET_LANE1_FLDMASK_LEN                  2

#define MAX_CURRENT_REACHED_LANE1_FLDMASK                   0x4
#define MAX_CURRENT_REACHED_LANE1_FLDMASK_POS               2
#define MAX_CURRENT_REACHED_LANE1_FLDMASK_LEN               1

#define DRIVE_CURRENT_SET_LANE1_FLDMASK                     0x3
#define DRIVE_CURRENT_SET_LANE1_FLDMASK_POS                 0
#define DRIVE_CURRENT_SET_LANE1_FLDMASK_LEN                 2

#define ADDR_TRAINING_LANE2_SET (DPRX_BASE + SLAVE_ID_01 + 0x0414)
#define MAX_PRE_EMPHASIS_REACHED_LANE2_FLDMASK              0x20
#define MAX_PRE_EMPHASIS_REACHED_LANE2_FLDMASK_POS          5
#define MAX_PRE_EMPHASIS_REACHED_LANE2_FLDMASK_LEN          1

#define PRE_EMPHASIS_SET_LANE2_FLDMASK                      0x18
#define PRE_EMPHASIS_SET_LANE2_FLDMASK_POS                  3
#define PRE_EMPHASIS_SET_LANE2_FLDMASK_LEN                  2

#define MAX_CURRENT_REACHED_LANE2_FLDMASK                   0x4
#define MAX_CURRENT_REACHED_LANE2_FLDMASK_POS               2
#define MAX_CURRENT_REACHED_LANE2_FLDMASK_LEN               1

#define DRIVE_CURRENT_SET_LANE2_FLDMASK                     0x3
#define DRIVE_CURRENT_SET_LANE2_FLDMASK_POS                 0
#define DRIVE_CURRENT_SET_LANE2_FLDMASK_LEN                 2

#define ADDR_TRAINING_LANE3_SET (DPRX_BASE + SLAVE_ID_01 + 0x0418)
#define MAX_PRE_EMPHASIS_REACHED_LANE3_FLDMASK              0x20
#define MAX_PRE_EMPHASIS_REACHED_LANE3_FLDMASK_POS          5
#define MAX_PRE_EMPHASIS_REACHED_LANE3_FLDMASK_LEN          1

#define PRE_EMPHASIS_SET_LANE3_FLDMASK                      0x18
#define PRE_EMPHASIS_SET_LANE3_FLDMASK_POS                  3
#define PRE_EMPHASIS_SET_LANE3_FLDMASK_LEN                  2

#define MAX_CURRENT_REACHED_LANE3_FLDMASK                   0x4
#define MAX_CURRENT_REACHED_LANE3_FLDMASK_POS               2
#define MAX_CURRENT_REACHED_LANE3_FLDMASK_LEN               1

#define DRIVE_CURRENT_SET_LANE3_FLDMASK                     0x3
#define DRIVE_CURRENT_SET_LANE3_FLDMASK_POS                 0
#define DRIVE_CURRENT_SET_LANE3_FLDMASK_LEN                 2

#define ADDR_DOWNSPREAD_CTRL (DPRX_BASE + SLAVE_ID_01 + 0x041C)
#define MSA_TIMING_PAR_IGNORE_EN_FLDMASK                    0x80
#define MSA_TIMING_PAR_IGNORE_EN_FLDMASK_POS                7
#define MSA_TIMING_PAR_IGNORE_EN_FLDMASK_LEN                1

#define SPREAD_AMP_FLDMASK                                  0x10
#define SPREAD_AMP_FLDMASK_POS                              4
#define SPREAD_AMP_FLDMASK_LEN                              1

#define ADDR_MAIN_LINK_CHANNEL_CODING_SET (DPRX_BASE + SLAVE_ID_01 + 0x0420)
#define SET_ANSI8B10B_FLDMASK                               0x1
#define SET_ANSI8B10B_FLDMASK_POS                           0
#define SET_ANSI8B10B_FLDMASK_LEN                           1

#define ADDR_EDP_CONFIG_SET (DPRX_BASE + SLAVE_ID_01 + 0x0428)
#define PANEL_SELF_TEST_ENABLE_FLDMASK                      0x80
#define PANEL_SELF_TEST_ENABLE_FLDMASK_POS                  7
#define PANEL_SELF_TEST_ENABLE_FLDMASK_LEN                  1

#define FRAMING_CHANGE_ENABLE_FLDMASK                       0x2
#define FRAMING_CHANGE_ENABLE_FLDMASK_POS                   1
#define FRAMING_CHANGE_ENABLE_FLDMASK_LEN                   1

#define ALTERNATE_SCRAMBLER_RESET_ENABLE_FLDMASK            0x1
#define ALTERNATE_SCRAMBLER_RESET_ENABLE_FLDMASK_POS        0
#define ALTERNATE_SCRAMBLER_RESET_ENABLE_FLDMASK_LEN        1

#define ADDR_LINK_QUAL_LANE0_SET (DPRX_BASE + SLAVE_ID_01 + 0x042C)
#define LANE_QUAL_PATTERN_SET_LANE0_FLDMASK                 0x7
#define LANE_QUAL_PATTERN_SET_LANE0_FLDMASK_POS             0
#define LANE_QUAL_PATTERN_SET_LANE0_FLDMASK_LEN             3

#define ADDR_LINK_QUAL_LANE1_SET (DPRX_BASE + SLAVE_ID_01 + 0x0430)
#define LANE_QUAL_PATTERN_SET_LANE1_FLDMASK                 0x7
#define LANE_QUAL_PATTERN_SET_LANE1_FLDMASK_POS             0
#define LANE_QUAL_PATTERN_SET_LANE1_FLDMASK_LEN             3

#define ADDR_LINK_QUAL_LANE2_SET (DPRX_BASE + SLAVE_ID_01 + 0x0434)
#define LANE_QUAL_PATTERN_SET_LANE2_FLDMASK                 0x7
#define LANE_QUAL_PATTERN_SET_LANE2_FLDMASK_POS             0
#define LANE_QUAL_PATTERN_SET_LANE2_FLDMASK_LEN             3

#define ADDR_LINK_QUAL_LANE3_SET (DPRX_BASE + SLAVE_ID_01 + 0x0438)
#define LANE_QUAL_PATTERN_SET_LANE3_FLDMASK                 0x7
#define LANE_QUAL_PATTERN_SET_LANE3_FLDMASK_POS             0
#define LANE_QUAL_PATTERN_SET_LANE3_FLDMASK_LEN             3

#define ADDR_MSTM_CTRL (DPRX_BASE + SLAVE_ID_01 + 0x0444)
#define UPSTREAM_IS_SRC_FLDMASK                             0x4
#define UPSTREAM_IS_SRC_FLDMASK_POS                         2
#define UPSTREAM_IS_SRC_FLDMASK_LEN                         1

#define UP_REQ_EN_FLDMASK                                   0x2
#define UP_REQ_EN_FLDMASK_POS                               1
#define UP_REQ_EN_FLDMASK_LEN                               1

#define MST_EN_FLDMASK                                      0x1
#define MST_EN_FLDMASK_POS                                  0
#define MST_EN_FLDMASK_LEN                                  1

#define ADDR_AUDIO_DELAY_7_0 (DPRX_BASE + SLAVE_ID_01 + 0x0448)
#define AUDIO_DELAY_7_0_FLDMASK                             0xff
#define AUDIO_DELAY_7_0_FLDMASK_POS                         0
#define AUDIO_DELAY_7_0_FLDMASK_LEN                         8

#define ADDR_AUDIO_DELAY_15_8 (DPRX_BASE + SLAVE_ID_01 + 0x044C)
#define AUDIO_DELAY_15_8_FLDMASK                            0xff
#define AUDIO_DELAY_15_8_FLDMASK_POS                        0
#define AUDIO_DELAY_15_8_FLDMASK_LEN                        8

#define ADDR_AUDIO_DELAY_23_16 (DPRX_BASE + SLAVE_ID_01 + 0x0450)
#define AUDIO_DELAY_23_16_FLDMASK                           0xff
#define AUDIO_DELAY_23_16_FLDMASK_POS                       0
#define AUDIO_DELAY_23_16_FLDMASK_LEN                       8

#define ADDR_TX_GTC_CAPABILITY (DPRX_BASE + SLAVE_ID_01 + 0x0454)
#define TX_GTC_SLAVE_CAP_FLDMASK                            0x10
#define TX_GTC_SLAVE_CAP_FLDMASK_POS                        4
#define TX_GTC_SLAVE_CAP_FLDMASK_LEN                        1

#define TX_GTC_CAP_FLDMASK                                  0x8
#define TX_GTC_CAP_FLDMASK_POS                              3
#define TX_GTC_CAP_FLDMASK_LEN                              1

#define LINK_RATE_SET_FLDMASK                               0x7
#define LINK_RATE_SET_FLDMASK_POS                           0
#define LINK_RATE_SET_FLDMASK_LEN                           3

#define ADDR_UPSTRM_DP_PWR_NEED (DPRX_BASE + SLAVE_ID_01 + 0x0460)
#define DP_PWR_NOT_NEEDED_BY_UPSTRM_FLDMASK                 0x1
#define DP_PWR_NOT_NEEDED_BY_UPSTRM_FLDMASK_POS             0
#define DP_PWR_NOT_NEEDED_BY_UPSTRM_FLDMASK_LEN             1

#define ADDR_EXT_DPRX_SLEEP_WAKE_TIMEOUT_GRANT                                 \
	(DPRX_BASE + SLAVE_ID_01 + 0x0464)
#define EXT_DPRX_SLEEP_WAKE_TIMEOUT_GRANT_FLDMASK           0x1
#define EXT_DPRX_SLEEP_WAKE_TIMEOUT_GRANT_FLDMASK_POS       0
#define EXT_DPRX_SLEEP_WAKE_TIMEOUT_GRANT_FLDMASK_LEN       1

#define ADDR_FEC_CONFIG (DPRX_BASE + SLAVE_ID_01 + 0x0480)
#define FEC_LANE_SELECT_FLDMASK                             0x30
#define FEC_LANE_SELECT_FLDMASK_POS                         4
#define FEC_LANE_SELECT_FLDMASK_LEN                         2

#define FEC_ERROR_COUNT_SEL_FLDMASK                         0xe
#define FEC_ERROR_COUNT_SEL_FLDMASK_POS                     1
#define FEC_ERROR_COUNT_SEL_FLDMASK_LEN                     3

#define FEC_READY_FLDMASK                                   0x1
#define FEC_READY_FLDMASK_POS                               0
#define FEC_READY_FLDMASK_LEN                               1

#define ADDR_TX_GTC_VALUE_7_0 (DPRX_BASE + SLAVE_ID_01 + 0x0550)
#define TX_GTC_VALUE_7_0_FLDMASK                            0xff
#define TX_GTC_VALUE_7_0_FLDMASK_POS                        0
#define TX_GTC_VALUE_7_0_FLDMASK_LEN                        8

#define ADDR_TX_GTC_VALUE_15_8 (DPRX_BASE + SLAVE_ID_01 + 0x0554)
#define TX_GTC_VALUE_15_8_FLDMASK                           0xff
#define TX_GTC_VALUE_15_8_FLDMASK_POS                       0
#define TX_GTC_VALUE_15_8_FLDMASK_LEN                       8

#define ADDR_TX_GTC_VALUE_23_16 (DPRX_BASE + SLAVE_ID_01 + 0x0558)
#define TX_GTC_VALUE_23_16_FLDMASK                          0xff
#define TX_GTC_VALUE_23_16_FLDMASK_POS                      0
#define TX_GTC_VALUE_23_16_FLDMASK_LEN                      8

#define ADDR_TX_GTC_VALUE_31_24 (DPRX_BASE + SLAVE_ID_01 + 0x055C)
#define TX_GTC_VALUE_31_24_FLDMASK                          0xff
#define TX_GTC_VALUE_31_24_FLDMASK_POS                      0
#define TX_GTC_VALUE_31_24_FLDMASK_LEN                      8

#define ADDR_RX_GTC_VALUE_PHASE_SKEW_EN (DPRX_BASE + SLAVE_ID_01 + 0x0560)
#define RX_GTC_VALUE_PHASE_SKEW_EN_FLDMASK                  0x1
#define RX_GTC_VALUE_PHASE_SKEW_EN_FLDMASK_POS              0
#define RX_GTC_VALUE_PHASE_SKEW_EN_FLDMASK_LEN              1

#define ADDR_TX_GTC_FREQ_LOCK_DONE (DPRX_BASE + SLAVE_ID_01 + 0x0564)
#define TX_GTC_FREQ_LOCK_DONE_FLDMASK                       0x1
#define TX_GTC_FREQ_LOCK_DONE_FLDMASK_POS                   0
#define TX_GTC_FREQ_LOCK_DONE_FLDMASK_LEN                   1

#define ADDR_TX_GTC_PHASE_SKEW_OFFSET_7_0 (DPRX_BASE + SLAVE_ID_01 + 0x0568)
#define TX_GTC_PHASE_SKEW_OFFSET_7_0_FLDMASK                0xff
#define TX_GTC_PHASE_SKEW_OFFSET_7_0_FLDMASK_POS            0
#define TX_GTC_PHASE_SKEW_OFFSET_7_0_FLDMASK_LEN            8

#define ADDR_TX_GTC_PHASE_SKEW_OFFSET_15_8 (DPRX_BASE + SLAVE_ID_01 + 0x056C)
#define TX_GTC_PHASE_SKEW_OFFSET_15_8_FLDMASK               0xff
#define TX_GTC_PHASE_SKEW_OFFSET_15_8_FLDMASK_POS           0
#define TX_GTC_PHASE_SKEW_OFFSET_15_8_FLDMASK_LEN           8

#define ADDR_DSC_ENABLE (DPRX_BASE + SLAVE_ID_01 + 0x0580)
#define DSC_EN_FLDMASK                                      0x1
#define DSC_EN_FLDMASK_POS                                  0
#define DSC_EN_FLDMASK_LEN                                  1

#define ADDR_ADAPTOR_CTRL (DPRX_BASE + SLAVE_ID_01 + 0x0680)
#define FORCE_LOAD_SENSE_FLDMASK                            0x1
#define FORCE_LOAD_SENSE_FLDMASK_POS                        0
#define FORCE_LOAD_SENSE_FLDMASK_LEN                        1

#define ADDR_BRANCH_DEVICE_SEL (DPRX_BASE + SLAVE_ID_01 + 0x0684)
#define HPD_TYPE_FLDMASK                                    0x1
#define HPD_TYPE_FLDMASK_POS                                0
#define HPD_TYPE_FLDMASK_LEN                                1

#define ADDR_PAYLOAD_ALLOCATE_SET (DPRX_BASE + SLAVE_ID_01 + 0x0700)
#define VC_PAYLOAD_ALLOCATE_SET_FLDMASK                     0x7f
#define VC_PAYLOAD_ALLOCATE_SET_FLDMASK_POS                 0
#define VC_PAYLOAD_ALLOCATE_SET_FLDMASK_LEN                 7

#define ADDR_PAYLOAD_ALLOCATE_START_TIME_SLOT (DPRX_BASE + SLAVE_ID_01 + 0x0704)
#define VC_PAYLOAD_ALLOCATE_START_TIME_SLOT_FLDMASK         0x3f
#define VC_PAYLOAD_ALLOCATE_START_TIME_SLOT_FLDMASK_POS     0
#define VC_PAYLOAD_ALLOCATE_START_TIME_SLOT_FLDMASK_LEN     6

#define ADDR_PAYLOAD_ALLOCATE_TIME_SLOT_COUNT (DPRX_BASE + SLAVE_ID_01 + 0x0708)
#define VC_PAYLOAD_ALLOCATE_TIME_SLOT_COUNT_FLDMASK         0x3f
#define VC_PAYLOAD_ALLOCATE_TIME_SLOT_COUNT_FLDMASK_POS     0
#define VC_PAYLOAD_ALLOCATE_TIME_SLOT_COUNT_FLDMASK_LEN     6

#define ADDR_SINK_COUNT (DPRX_BASE + SLAVE_ID_01 + 0x0800)
#define CP_READY_FLDMASK                                    0x40
#define CP_READY_FLDMASK_POS                                6
#define CP_READY_FLDMASK_LEN                                1

#define SINK_COUNT_FLDMASK                                  0x3f
#define SINK_COUNT_FLDMASK_POS                              0
#define SINK_COUNT_FLDMASK_LEN                              6

#define ADDR_DEVICE_SERVICE_IRQ_VECTOR (DPRX_BASE + SLAVE_ID_01 + 0x0804)
#define SINK_SPECIAL_IRQ_FLDMASK                            0x40
#define SINK_SPECIAL_IRQ_FLDMASK_POS                        6
#define SINK_SPECIAL_IRQ_FLDMASK_LEN                        1

#define MCCS_IRQ_FLDMASK                                    0x8
#define MCCS_IRQ_FLDMASK_POS                                3
#define MCCS_IRQ_FLDMASK_LEN                                1

#define CP_IRQ_FLDMASK                                      0x4
#define CP_IRQ_FLDMASK_POS                                  2
#define CP_IRQ_FLDMASK_LEN                                  1

#define AUTOMATED_TEST_REQUEST_FLDMASK                      0x2
#define AUTOMATED_TEST_REQUEST_FLDMASK_POS                  1
#define AUTOMATED_TEST_REQUEST_FLDMASK_LEN                  1

#define ADDR_LANE0_1_STATUS (DPRX_BASE + SLAVE_ID_01 + 0x0808)
#define LANE1_SYMBOL_LOCKED_FLDMASK                         0x40
#define LANE1_SYMBOL_LOCKED_FLDMASK_POS                     6
#define LANE1_SYMBOL_LOCKED_FLDMASK_LEN                     1

#define LANE1_CHANNEL_EQ_DONE_FLDMASK                       0x20
#define LANE1_CHANNEL_EQ_DONE_FLDMASK_POS                   5
#define LANE1_CHANNEL_EQ_DONE_FLDMASK_LEN                   1

#define LANE1_CR_DONE_FLDMASK                               0x10
#define LANE1_CR_DONE_FLDMASK_POS                           4
#define LANE1_CR_DONE_FLDMASK_LEN                           1

#define LANE0_SYMBOL_LOCKED_FLDMASK                         0x4
#define LANE0_SYMBOL_LOCKED_FLDMASK_POS                     2
#define LANE0_SYMBOL_LOCKED_FLDMASK_LEN                     1

#define LANE0_CHANNEL_EQ_DONE_FLDMASK                       0x2
#define LANE0_CHANNEL_EQ_DONE_FLDMASK_POS                   1
#define LANE0_CHANNEL_EQ_DONE_FLDMASK_LEN                   1

#define LANE0_CR_DONE_FLDMASK                               0x1
#define LANE0_CR_DONE_FLDMASK_POS                           0
#define LANE0_CR_DONE_FLDMASK_LEN                           1

#define ADDR_LANE2_3_STATUS (DPRX_BASE + SLAVE_ID_01 + 0x080C)
#define LANE3_SYMBOL_LOCKED_FLDMASK                         0x40
#define LANE3_SYMBOL_LOCKED_FLDMASK_POS                     6
#define LANE3_SYMBOL_LOCKED_FLDMASK_LEN                     1

#define LANE3_CHANNEL_EQ_DONE_FLDMASK                       0x20
#define LANE3_CHANNEL_EQ_DONE_FLDMASK_POS                   5
#define LANE3_CHANNEL_EQ_DONE_FLDMASK_LEN                   1

#define LANE3_CR_DONE_FLDMASK                               0x10
#define LANE3_CR_DONE_FLDMASK_POS                           4
#define LANE3_CR_DONE_FLDMASK_LEN                           1

#define LANE2_SYMBOL_LOCKED_FLDMASK                         0x4
#define LANE2_SYMBOL_LOCKED_FLDMASK_POS                     2
#define LANE2_SYMBOL_LOCKED_FLDMASK_LEN                     1

#define LANE2_CHANNEL_EQ_DONE_FLDMASK                       0x2
#define LANE2_CHANNEL_EQ_DONE_FLDMASK_POS                   1
#define LANE2_CHANNEL_EQ_DONE_FLDMASK_LEN                   1

#define LANE2_CR_DONE_FLDMASK                               0x1
#define LANE2_CR_DONE_FLDMASK_POS                           0
#define LANE2_CR_DONE_FLDMASK_LEN                           1

#define ADDR_LANE_ALIGN_STATUS_UPDATED (DPRX_BASE + SLAVE_ID_01 + 0x0810)
#define LINK_STATUS_UPDATED_FLDMASK                         0x80
#define LINK_STATUS_UPDATED_FLDMASK_POS                     7
#define LINK_STATUS_UPDATED_FLDMASK_LEN                     1

#define DOWNSTREAM_PORT_STATUS_CHANGED_FLDMASK              0x40
#define DOWNSTREAM_PORT_STATUS_CHANGED_FLDMASK_POS          6
#define DOWNSTREAM_PORT_STATUS_CHANGED_FLDMASK_LEN          1

#define POST_LT_ADJ_REQ_IN_PROGRESS_FLDMASK                 0x2
#define POST_LT_ADJ_REQ_IN_PROGRESS_FLDMASK_POS             1
#define POST_LT_ADJ_REQ_IN_PROGRESS_FLDMASK_LEN             1

#define INTERLANE_ALIGN_DONE_FLDMASK                        0x1
#define INTERLANE_ALIGN_DONE_FLDMASK_POS                    0
#define INTERLANE_ALIGN_DONE_FLDMASK_LEN                    1

#define ADDR_SINK_STATUS (DPRX_BASE + SLAVE_ID_01 + 0x0814)
#define RECEIVE_PORT_1_STATUS_FLDMASK                       0x2
#define RECEIVE_PORT_1_STATUS_FLDMASK_POS                   1
#define RECEIVE_PORT_1_STATUS_FLDMASK_LEN                   1

#define RECEIVE_PORT_0_STATUS_FLDMASK                       0x1
#define RECEIVE_PORT_0_STATUS_FLDMASK_POS                   0
#define RECEIVE_PORT_0_STATUS_FLDMASK_LEN                   1

#define ADDR_ADJUST_REQUEST_LANE0_1 (DPRX_BASE + SLAVE_ID_01 + 0x0818)
#define PRE_EMPHASIS_LANE1_FLDMASK                          0xc0
#define PRE_EMPHASIS_LANE1_FLDMASK_POS                      6
#define PRE_EMPHASIS_LANE1_FLDMASK_LEN                      2

#define VOLTAGE_SWING_LANE1_FLDMASK                         0x30
#define VOLTAGE_SWING_LANE1_FLDMASK_POS                     4
#define VOLTAGE_SWING_LANE1_FLDMASK_LEN                     2

#define PRE_EMPHASIS_LANE0_FLDMASK                          0xc
#define PRE_EMPHASIS_LANE0_FLDMASK_POS                      2
#define PRE_EMPHASIS_LANE0_FLDMASK_LEN                      2

#define VOLTAGE_SWING_LANE0_FLDMASK                         0x3
#define VOLTAGE_SWING_LANE0_FLDMASK_POS                     0
#define VOLTAGE_SWING_LANE0_FLDMASK_LEN                     2

#define ADDR_ADJUST_REQUEST_LANE2_3 (DPRX_BASE + SLAVE_ID_01 + 0x081C)
#define PRE_EMPHASIS_LANE3_FLDMASK                          0xc0
#define PRE_EMPHASIS_LANE3_FLDMASK_POS                      6
#define PRE_EMPHASIS_LANE3_FLDMASK_LEN                      2

#define VOLTAGE_SWING_LANE3_FLDMASK                         0x30
#define VOLTAGE_SWING_LANE3_FLDMASK_POS                     4
#define VOLTAGE_SWING_LANE3_FLDMASK_LEN                     2

#define PRE_EMPHASIS_LANE2_FLDMASK                          0xc
#define PRE_EMPHASIS_LANE2_FLDMASK_POS                      2
#define PRE_EMPHASIS_LANE2_FLDMASK_LEN                      2

#define VOLTAGE_SWING_LANE2_FLDMASK                         0x3
#define VOLTAGE_SWING_LANE2_FLDMASK_POS                     0
#define VOLTAGE_SWING_LANE2_FLDMASK_LEN                     2

#define ADDR_DSC_STATUS (DPRX_BASE + SLAVE_ID_01 + 0x083C)
#define CHUNK_LEN_ERROR_FLDMASK                             0x4
#define CHUNK_LEN_ERROR_FLDMASK_POS                         2
#define CHUNK_LEN_ERROR_FLDMASK_LEN                         1

#define RC_BUF_OVERFLOW_FLDMASK                             0x2
#define RC_BUF_OVERFLOW_FLDMASK_POS                         1
#define RC_BUF_OVERFLOW_FLDMASK_LEN                         1

#define RC_BUF_UNDERRUN_FLDMASK                             0x1
#define RC_BUF_UNDERRUN_FLDMASK_POS                         0
#define RC_BUF_UNDERRUN_FLDMASK_LEN                         1

#define ADDR_SYMBOL_ERROR_COUNT_LANE0_0 (DPRX_BASE + SLAVE_ID_01 + 0x0840)
#define ERROR_COUNT_LANE0_7_0_FLDMASK                       0xff
#define ERROR_COUNT_LANE0_7_0_FLDMASK_POS                   0
#define ERROR_COUNT_LANE0_7_0_FLDMASK_LEN                   8

#define ADDR_SYMBOL_ERROR_COUNT_LANE0_1 (DPRX_BASE + SLAVE_ID_01 + 0x0844)
#define ERROR_COUNT_LANE0_VALID_FLDMASK                     0x80
#define ERROR_COUNT_LANE0_VALID_FLDMASK_POS                 7
#define ERROR_COUNT_LANE0_VALID_FLDMASK_LEN                 1

#define ERROR_COUNT_LANE0_14_8_FLDMASK                      0x7f
#define ERROR_COUNT_LANE0_14_8_FLDMASK_POS                  0
#define ERROR_COUNT_LANE0_14_8_FLDMASK_LEN                  7

#define ADDR_SYMBOL_ERROR_COUNT_LANE1_0 (DPRX_BASE + SLAVE_ID_01 + 0x0848)
#define ERROR_COUNT_LANE1_7_0_FLDMASK                       0xff
#define ERROR_COUNT_LANE1_7_0_FLDMASK_POS                   0
#define ERROR_COUNT_LANE1_7_0_FLDMASK_LEN                   8

#define ADDR_SYMBOL_ERROR_COUNT_LANE1_1 (DPRX_BASE + SLAVE_ID_01 + 0x084C)
#define ERROR_COUNT_LANE1_VALID_FLDMASK                     0x80
#define ERROR_COUNT_LANE1_VALID_FLDMASK_POS                 7
#define ERROR_COUNT_LANE1_VALID_FLDMASK_LEN                 1

#define ERROR_COUNT_LANE1_14_8_FLDMASK                      0x7f
#define ERROR_COUNT_LANE1_14_8_FLDMASK_POS                  0
#define ERROR_COUNT_LANE1_14_8_FLDMASK_LEN                  7

#define ADDR_SYMBOL_ERROR_COUNT_LANE2_0 (DPRX_BASE + SLAVE_ID_01 + 0x0850)
#define ERROR_COUNT_LANE2_7_0_FLDMASK                       0xff
#define ERROR_COUNT_LANE2_7_0_FLDMASK_POS                   0
#define ERROR_COUNT_LANE2_7_0_FLDMASK_LEN                   8

#define ADDR_SYMBOL_ERROR_COUNT_LANE2_1 (DPRX_BASE + SLAVE_ID_01 + 0x0854)
#define ERROR_COUNT_LANE2_VALID_FLDMASK                     0x80
#define ERROR_COUNT_LANE2_VALID_FLDMASK_POS                 7
#define ERROR_COUNT_LANE2_VALID_FLDMASK_LEN                 1

#define ERROR_COUNT_LANE2_14_8_FLDMASK                      0x7f
#define ERROR_COUNT_LANE2_14_8_FLDMASK_POS                  0
#define ERROR_COUNT_LANE2_14_8_FLDMASK_LEN                  7

#define ADDR_SYMBOL_ERROR_COUNT_LANE3_0 (DPRX_BASE + SLAVE_ID_01 + 0x0858)
#define ERROR_COUNT_LANE3_7_0_FLDMASK                       0xff
#define ERROR_COUNT_LANE3_7_0_FLDMASK_POS                   0
#define ERROR_COUNT_LANE3_7_0_FLDMASK_LEN                   8

#define ADDR_SYMBOL_ERROR_COUNT_LANE3_1 (DPRX_BASE + SLAVE_ID_01 + 0x085C)
#define ERROR_COUNT_LANE3_VALID_FLDMASK                     0x80
#define ERROR_COUNT_LANE3_VALID_FLDMASK_POS                 7
#define ERROR_COUNT_LANE3_VALID_FLDMASK_LEN                 1

#define ERROR_COUNT_LANE3_14_8_FLDMASK                      0x7f
#define ERROR_COUNT_LANE3_14_8_FLDMASK_POS                  0
#define ERROR_COUNT_LANE3_14_8_FLDMASK_LEN                  7

#define ADDR_TEST_REQUEST (DPRX_BASE + SLAVE_ID_01 + 0x0860)
#define PHY_TEST_PATTERN_FLDMASK                            0x8
#define PHY_TEST_PATTERN_FLDMASK_POS                        3
#define PHY_TEST_PATTERN_FLDMASK_LEN                        1

#define TEST_EDID_READ_FLDMASK                              0x4
#define TEST_EDID_READ_FLDMASK_POS                          2
#define TEST_EDID_READ_FLDMASK_LEN                          1

#define TEST_PATTERN_FLDMASK                                0x2
#define TEST_PATTERN_FLDMASK_POS                            1
#define TEST_PATTERN_FLDMASK_LEN                            1

#define TEST_LINK_TRAINING_FLDMASK                          0x1
#define TEST_LINK_TRAINING_FLDMASK_POS                      0
#define TEST_LINK_TRAINING_FLDMASK_LEN                      1

#define ADDR_TEST_LINK_RATE (DPRX_BASE + SLAVE_ID_01 + 0x0864)
#define TEST_LINK_RATE_FLDMASK                              0xff
#define TEST_LINK_RATE_FLDMASK_POS                          0
#define TEST_LINK_RATE_FLDMASK_LEN                          8

#define ADDR_TEST_LANE_COUNT (DPRX_BASE + SLAVE_ID_01 + 0x0880)
#define TEST_LANE_COUNT_FLDMASK                             0x1f
#define TEST_LANE_COUNT_FLDMASK_POS                         0
#define TEST_LANE_COUNT_FLDMASK_LEN                         5

#define ADDR_TEST_PATTERN (DPRX_BASE + SLAVE_ID_01 + 0x0884)
#define SINK_TEST_PATTERN_FLDMASK                           0xff
#define SINK_TEST_PATTERN_FLDMASK_POS                       0
#define SINK_TEST_PATTERN_FLDMASK_LEN                       8

#define ADDR_TEST_H_TOTAL_H (DPRX_BASE + SLAVE_ID_01 + 0x0888)
#define TEST_H_TOTAL_H_FLDMASK                              0xff
#define TEST_H_TOTAL_H_FLDMASK_POS                          0
#define TEST_H_TOTAL_H_FLDMASK_LEN                          8

#define ADDR_TEST_H_TOTAL_L (DPRX_BASE + SLAVE_ID_01 + 0x088C)
#define TEST_H_TOTAL_L_FLDMASK                              0xff
#define TEST_H_TOTAL_L_FLDMASK_POS                          0
#define TEST_H_TOTAL_L_FLDMASK_LEN                          8

#define ADDR_TEST_V_TOTAL_H (DPRX_BASE + SLAVE_ID_01 + 0x0890)
#define TEST_V_TOTAL_H_FLDMASK                              0xff
#define TEST_V_TOTAL_H_FLDMASK_POS                          0
#define TEST_V_TOTAL_H_FLDMASK_LEN                          8

#define ADDR_TEST_V_TOTAL_L (DPRX_BASE + SLAVE_ID_01 + 0x0894)
#define TEST_V_TOTAL_L_FLDMASK                              0xff
#define TEST_V_TOTAL_L_FLDMASK_POS                          0
#define TEST_V_TOTAL_L_FLDMASK_LEN                          8

#define ADDR_TEST_H_START_H (DPRX_BASE + SLAVE_ID_01 + 0x0898)
#define TEST_H_START_H_FLDMASK                              0xff
#define TEST_H_START_H_FLDMASK_POS                          0
#define TEST_H_START_H_FLDMASK_LEN                          8

#define ADDR_TEST_H_START_L (DPRX_BASE + SLAVE_ID_01 + 0x089C)
#define TEST_H_START_L_FLDMASK                              0xff
#define TEST_H_START_L_FLDMASK_POS                          0
#define TEST_H_START_L_FLDMASK_LEN                          8

#define ADDR_TEST_V_START_H (DPRX_BASE + SLAVE_ID_01 + 0x08A0)
#define TEST_V_START_H_FLDMASK                              0xff
#define TEST_V_START_H_FLDMASK_POS                          0
#define TEST_V_START_H_FLDMASK_LEN                          8

#define ADDR_TEST_V_START_L (DPRX_BASE + SLAVE_ID_01 + 0x08A4)
#define TEST_V_START_L_FLDMASK                              0xff
#define TEST_V_START_L_FLDMASK_POS                          0
#define TEST_V_START_L_FLDMASK_LEN                          8

#define ADDR_TEST_HSYNC_H (DPRX_BASE + SLAVE_ID_01 + 0x08A8)
#define TEST_HSYNC_POLARITY_FLDMASK                         0x80
#define TEST_HSYNC_POLARITY_FLDMASK_POS                     7
#define TEST_HSYNC_POLARITY_FLDMASK_LEN                     1

#define TEST_HSYNC_H_FLDMASK                                0x7f
#define TEST_HSYNC_H_FLDMASK_POS                            0
#define TEST_HSYNC_H_FLDMASK_LEN                            7

#define ADDR_TEST_HSYNC_L (DPRX_BASE + SLAVE_ID_01 + 0x08AC)
#define TEST_HSYNC_L_FLDMASK                                0xff
#define TEST_HSYNC_L_FLDMASK_POS                            0
#define TEST_HSYNC_L_FLDMASK_LEN                            8

#define ADDR_TEST_VSYNC_H (DPRX_BASE + SLAVE_ID_01 + 0x08B0)
#define TEST_VSYNC_POLARITY_FLDMASK                         0x80
#define TEST_VSYNC_POLARITY_FLDMASK_POS                     7
#define TEST_VSYNC_POLARITY_FLDMASK_LEN                     1

#define TEST_VSYNC_H_FLDMASK                                0x7f
#define TEST_VSYNC_H_FLDMASK_POS                            0
#define TEST_VSYNC_H_FLDMASK_LEN                            7

#define ADDR_TEST_VSYNC_L (DPRX_BASE + SLAVE_ID_01 + 0x08B4)
#define TEST_VSYNC_L_FLDMASK                                0xff
#define TEST_VSYNC_L_FLDMASK_POS                            0
#define TEST_VSYNC_L_FLDMASK_LEN                            8

#define ADDR_TEST_H_WIDTH_H (DPRX_BASE + SLAVE_ID_01 + 0x08B8)
#define TEST_H_WIDTH_H_FLDMASK                              0xff
#define TEST_H_WIDTH_H_FLDMASK_POS                          0
#define TEST_H_WIDTH_H_FLDMASK_LEN                          8

#define ADDR_TEST_H_WIDTH_L (DPRX_BASE + SLAVE_ID_01 + 0x08BC)
#define TEST_H_WIDTH_L_FLDMASK                              0xff
#define TEST_H_WIDTH_L_FLDMASK_POS                          0
#define TEST_H_WIDTH_L_FLDMASK_LEN                          8

#define ADDR_TEST_H_HEIGHT_H (DPRX_BASE + SLAVE_ID_01 + 0x08C0)
#define TEST_H_HEIGHT_H_FLDMASK                             0xff
#define TEST_H_HEIGHT_H_FLDMASK_POS                         0
#define TEST_H_HEIGHT_H_FLDMASK_LEN                         8

#define ADDR_TEST_H_HEIGHT_L (DPRX_BASE + SLAVE_ID_01 + 0x08C4)
#define TEST_H_HEIGHT_L_FLDMASK                             0xff
#define TEST_H_HEIGHT_L_FLDMASK_POS                         0
#define TEST_H_HEIGHT_L_FLDMASK_LEN                         8

#define ADDR_TEST_MISC_0 (DPRX_BASE + SLAVE_ID_01 + 0x08C8)
#define TEST_BIT_DEPTH_FLDMASK                              0xe0
#define TEST_BIT_DEPTH_FLDMASK_POS                          5
#define TEST_BIT_DEPTH_FLDMASK_LEN                          3

#define TEST_YCBCR_COEF_FLDMASK                             0x10
#define TEST_YCBCR_COEF_FLDMASK_POS                         4
#define TEST_YCBCR_COEF_FLDMASK_LEN                         1

#define TEST_DYN_RANGE_FLDMASK                              0x8
#define TEST_DYN_RANGE_FLDMASK_POS                          3
#define TEST_DYN_RANGE_FLDMASK_LEN                          1

#define TEST_COLOR_FORMAT_FLDMASK                           0x6
#define TEST_COLOR_FORMAT_FLDMASK_POS                       1
#define TEST_COLOR_FORMAT_FLDMASK_LEN                       2

#define TEST_SYNC_CLOCK_FLDMASK                             0x1
#define TEST_SYNC_CLOCK_FLDMASK_POS                         0
#define TEST_SYNC_CLOCK_FLDMASK_LEN                         1

#define ADDR_TEST_MISC_1 (DPRX_BASE + SLAVE_ID_01 + 0x08CC)
#define TEST_INTERLACED_FLDMASK                             0x2
#define TEST_INTERLACED_FLDMASK_POS                         1
#define TEST_INTERLACED_FLDMASK_LEN                         1

#define TEST_REFRESH_DENOMINATOR_FLDMASK                    0x1
#define TEST_REFRESH_DENOMINATOR_FLDMASK_POS                0
#define TEST_REFRESH_DENOMINATOR_FLDMASK_LEN                1

#define ADDR_TEST_REFRESH_RATE_NUMERATOR (DPRX_BASE + SLAVE_ID_01 + 0x08D0)
#define TEST_REFRESH_RATE_NUMERATOR_FLDMASK                 0xff
#define TEST_REFRESH_RATE_NUMERATOR_FLDMASK_POS             0
#define TEST_REFRESH_RATE_NUMERATOR_FLDMASK_LEN             8

#define ADDR_TEST_CRC_R_CR_L (DPRX_BASE + SLAVE_ID_01 + 0x0900)
#define TEST_CRC_R_CR_L_FLDMASK                             0xff
#define TEST_CRC_R_CR_L_FLDMASK_POS                         0
#define TEST_CRC_R_CR_L_FLDMASK_LEN                         8

#define ADDR_TEST_CRC_R_CR_H (DPRX_BASE + SLAVE_ID_01 + 0x0904)
#define TEST_CRC_R_CR_H_FLDMASK                             0xff
#define TEST_CRC_R_CR_H_FLDMASK_POS                         0
#define TEST_CRC_R_CR_H_FLDMASK_LEN                         8

#define ADDR_TEST_CRC_G_Y_L (DPRX_BASE + SLAVE_ID_01 + 0x0908)
#define TEST_CRC_G_Y_L_FLDMASK                              0xff
#define TEST_CRC_G_Y_L_FLDMASK_POS                          0
#define TEST_CRC_G_Y_L_FLDMASK_LEN                          8

#define ADDR_TEST_CRC_G_Y_H (DPRX_BASE + SLAVE_ID_01 + 0x090C)
#define TEST_CRC_G_Y_H_FLDMASK                              0xff
#define TEST_CRC_G_Y_H_FLDMASK_POS                          0
#define TEST_CRC_G_Y_H_FLDMASK_LEN                          8

#define ADDR_TEST_CRC_B_CB_L (DPRX_BASE + SLAVE_ID_01 + 0x0910)
#define TEST_CRC_B_CR_L_FLDMASK                             0xff
#define TEST_CRC_B_CR_L_FLDMASK_POS                         0
#define TEST_CRC_B_CR_L_FLDMASK_LEN                         8

#define ADDR_TEST_CRC_B_CB_H (DPRX_BASE + SLAVE_ID_01 + 0x0914)
#define TEST_CRC_B_CB_H_FLDMASK                             0xff
#define TEST_CRC_B_CB_H_FLDMASK_POS                         0
#define TEST_CRC_B_CB_H_FLDMASK_LEN                         8

#define ADDR_TEST_SINK_MISC (DPRX_BASE + SLAVE_ID_01 + 0x0918)
#define TEST_CRC_SUPPORTED_FLDMASK                          0x20
#define TEST_CRC_SUPPORTED_FLDMASK_POS                      5
#define TEST_CRC_SUPPORTED_FLDMASK_LEN                      1

#define TEST_CRC_COUNT_FLDMASK                              0xf
#define TEST_CRC_COUNT_FLDMASK_POS                          0
#define TEST_CRC_COUNT_FLDMASK_LEN                          4

#define ADDR_PHY_TEST_PATTERN (DPRX_BASE + SLAVE_ID_01 + 0x0920)
#define PHY_TEST_PATTERN_SEL_FLDMASK                        0x7
#define PHY_TEST_PATTERN_SEL_FLDMASK_POS                    0
#define PHY_TEST_PATTERN_SEL_FLDMASK_LEN                    3

#define ADDR_HBR2_COMP_SCR_RST_0 (DPRX_BASE + SLAVE_ID_01 + 0x0928)
#define HBR2_COMP_SCR_RST_0_FLDMASK                         0xff
#define HBR2_COMP_SCR_RST_0_FLDMASK_POS                     0
#define HBR2_COMP_SCR_RST_0_FLDMASK_LEN                     8

#define ADDR_HBR2_COMP_SCR_RST_1 (DPRX_BASE + SLAVE_ID_01 + 0x092C)
#define HBR2_COMP_SCR_RST_1_FLDMASK                         0xff
#define HBR2_COMP_SCR_RST_1_FLDMASK_POS                     0
#define HBR2_COMP_SCR_RST_1_FLDMASK_LEN                     8

#define ADDR_TEST_80BIT_PAT0 (DPRX_BASE + SLAVE_ID_01 + 0x0940)
#define TEST_80BIT_PAT0_FLDMASK                             0xff
#define TEST_80BIT_PAT0_FLDMASK_POS                         0
#define TEST_80BIT_PAT0_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT1 (DPRX_BASE + SLAVE_ID_01 + 0x0944)
#define TEST_80BIT_PAT1_FLDMASK                             0xff
#define TEST_80BIT_PAT1_FLDMASK_POS                         0
#define TEST_80BIT_PAT1_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT2 (DPRX_BASE + SLAVE_ID_01 + 0x0948)
#define TEST_80BIT_PAT2_FLDMASK                             0xff
#define TEST_80BIT_PAT2_FLDMASK_POS                         0
#define TEST_80BIT_PAT2_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT3 (DPRX_BASE + SLAVE_ID_01 + 0x094C)
#define TEST_80BIT_PAT3_FLDMASK                             0xff
#define TEST_80BIT_PAT3_FLDMASK_POS                         0
#define TEST_80BIT_PAT3_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT4 (DPRX_BASE + SLAVE_ID_01 + 0x0950)
#define TEST_80BIT_PAT4_FLDMASK                             0xff
#define TEST_80BIT_PAT4_FLDMASK_POS                         0
#define TEST_80BIT_PAT4_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT5 (DPRX_BASE + SLAVE_ID_01 + 0x0954)
#define TEST_80BIT_PAT5_FLDMASK                             0xff
#define TEST_80BIT_PAT5_FLDMASK_POS                         0
#define TEST_80BIT_PAT5_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT6 (DPRX_BASE + SLAVE_ID_01 + 0x0958)
#define TEST_80BIT_PAT6_FLDMASK                             0xff
#define TEST_80BIT_PAT6_FLDMASK_POS                         0
#define TEST_80BIT_PAT6_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT7 (DPRX_BASE + SLAVE_ID_01 + 0x095C)
#define TEST_80BIT_PAT7_FLDMASK                             0xff
#define TEST_80BIT_PAT7_FLDMASK_POS                         0
#define TEST_80BIT_PAT7_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT8 (DPRX_BASE + SLAVE_ID_01 + 0x0960)
#define TEST_80BIT_PAT8_FLDMASK                             0xff
#define TEST_80BIT_PAT8_FLDMASK_POS                         0
#define TEST_80BIT_PAT8_FLDMASK_LEN                         8

#define ADDR_TEST_80BIT_PAT9 (DPRX_BASE + SLAVE_ID_01 + 0x0964)
#define TEST_80BIT_PAT9_FLDMASK                             0xff
#define TEST_80BIT_PAT9_FLDMASK_POS                         0
#define TEST_80BIT_PAT9_FLDMASK_LEN                         8

#define ADDR_CONTINUE_80BIT_AUXCH_CAP (DPRX_BASE + SLAVE_ID_01 + 0x0968)
#define CONTINUE_80BIT_AUXCH_CAP_FLDMASK                    0x1
#define CONTINUE_80BIT_AUXCH_CAP_FLDMASK_POS                0
#define CONTINUE_80BIT_AUXCH_CAP_FLDMASK_LEN                1

#define ADDR_CONTINUE_80BIT_AUXCH_CTRL (DPRX_BASE + SLAVE_ID_01 + 0x096C)
#define DURATION_CTRL_FLDMASK                               0x6
#define DURATION_CTRL_FLDMASK_POS                           1
#define DURATION_CTRL_FLDMASK_LEN                           2

#define CONTINUE_80BIT_AUXCH_EN_FLDMASK                     0x1
#define CONTINUE_80BIT_AUXCH_EN_FLDMASK_POS                 0
#define CONTINUE_80BIT_AUXCH_EN_FLDMASK_LEN                 1

#define ADDR_TEST_RESPONSE (DPRX_BASE + SLAVE_ID_01 + 0x0980)
#define TEST_EDID_CHECKSUM_WRITE_FLDMASK                    0x4
#define TEST_EDID_CHECKSUM_WRITE_FLDMASK_POS                2
#define TEST_EDID_CHECKSUM_WRITE_FLDMASK_LEN                1

#define TEST_NAK_FLDMASK                                    0x2
#define TEST_NAK_FLDMASK_POS                                1
#define TEST_NAK_FLDMASK_LEN                                1

#define TEST_ACK_FLDMASK                                    0x1
#define TEST_ACK_FLDMASK_POS                                0
#define TEST_ACK_FLDMASK_LEN                                1

#define ADDR_TEST_EDID_CHECKSUM (DPRX_BASE + SLAVE_ID_01 + 0x0984)
#define TEST_EDID_CHECKSUM_FLDMASK                          0xff
#define TEST_EDID_CHECKSUM_FLDMASK_POS                      0
#define TEST_EDID_CHECKSUM_FLDMASK_LEN                      8

#define ADDR_DSC_CRC_0_L (DPRX_BASE + SLAVE_ID_01 + 0x0988)
#define DSC_CRC_0_L_FLDMASK                                 0xff
#define DSC_CRC_0_L_FLDMASK_POS                             0
#define DSC_CRC_0_L_FLDMASK_LEN                             8

#define ADDR_DSC_CRC_0_H (DPRX_BASE + SLAVE_ID_01 + 0x098C)
#define DSC_CRC_0_H_FLDMASK                                 0xff
#define DSC_CRC_0_H_FLDMASK_POS                             0
#define DSC_CRC_0_H_FLDMASK_LEN                             8

#define ADDR_DSC_CRC_1_L (DPRX_BASE + SLAVE_ID_01 + 0x0990)
#define DSC_CRC_1_L_FLDMASK                                 0xff
#define DSC_CRC_1_L_FLDMASK_POS                             0
#define DSC_CRC_1_L_FLDMASK_LEN                             8

#define ADDR_DSC_CRC_1_H (DPRX_BASE + SLAVE_ID_01 + 0x0994)
#define DSC_CRC_1_H_FLDMASK                                 0xff
#define DSC_CRC_1_H_FLDMASK_POS                             0
#define DSC_CRC_1_H_FLDMASK_LEN                             8

#define ADDR_DSC_CRC_2_L (DPRX_BASE + SLAVE_ID_01 + 0x0998)
#define DSC_CRC_2_L_FLDMASK                                 0xff
#define DSC_CRC_2_L_FLDMASK_POS                             0
#define DSC_CRC_2_L_FLDMASK_LEN                             8

#define ADDR_DSC_CRC_2_H (DPRX_BASE + SLAVE_ID_01 + 0x099C)
#define DSC_CRC_2_H_FLDMASK                                 0xff
#define DSC_CRC_2_H_FLDMASK_POS                             0
#define DSC_CRC_2_H_FLDMASK_LEN                             8

#define ADDR_TEST_SINK (DPRX_BASE + SLAVE_ID_01 + 0x09C0)
#define PHY_SINK_TEST_LANE_EN_FLDMASK                       0x80
#define PHY_SINK_TEST_LANE_EN_FLDMASK_POS                   7
#define PHY_SINK_TEST_LANE_EN_FLDMASK_LEN                   1

#define PHY_SINK_TEST_LANE_SEL_FLDMASK                      0x30
#define PHY_SINK_TEST_LANE_SEL_FLDMASK_POS                  4
#define PHY_SINK_TEST_LANE_SEL_FLDMASK_LEN                  2

#define TEST_SINK_START_FLDMASK                             0x1
#define TEST_SINK_START_FLDMASK_POS                         0
#define TEST_SINK_START_FLDMASK_LEN                         1

#define ADDR_TEST_AUDIO_MODE (DPRX_BASE + SLAVE_ID_01 + 0x09C4)
#define TEST_AUDIO_CHANNEL_COUNT_FLDMASK                    0xf0
#define TEST_AUDIO_CHANNEL_COUNT_FLDMASK_POS                4
#define TEST_AUDIO_CHANNEL_COUNT_FLDMASK_LEN                4

#define TEST_AUDIO_SAMPLING_RATE_FLDMASK                    0xf
#define TEST_AUDIO_SAMPLING_RATE_FLDMASK_POS                0
#define TEST_AUDIO_SAMPLING_RATE_FLDMASK_LEN                4

#define ADDR_TEST_AUDIO_PATTERN_TYPE (DPRX_BASE + SLAVE_ID_01 + 0x09C8)
#define TEST_AUDIO_PATTERN_FLDMASK                          0xff
#define TEST_AUDIO_PATTERN_FLDMASK_POS                      0
#define TEST_AUDIO_PATTERN_FLDMASK_LEN                      8

#define ADDR_TEST_AUDIO_PERIOD_CH_1 (DPRX_BASE + SLAVE_ID_01 + 0x09CC)
#define TEST_AUDIO_PERIOD_CH1_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH1_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH1_FLDMASK_LEN                   4

#define ADDR_TEST_AUDIO_PERIOD_CH_2 (DPRX_BASE + SLAVE_ID_01 + 0x09D0)
#define TEST_AUDIO_PERIOD_CH2_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH2_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH2_FLDMASK_LEN                   4

#define ADDR_TEST_AUDIO_PERIOD_CH_3 (DPRX_BASE + SLAVE_ID_01 + 0x09D4)
#define TEST_AUDIO_PERIOD_CH3_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH3_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH3_FLDMASK_LEN                   4

#define ADDR_TEST_AUDIO_PERIOD_CH_4 (DPRX_BASE + SLAVE_ID_01 + 0x09D8)
#define TEST_AUDIO_PERIOD_CH4_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH4_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH4_FLDMASK_LEN                   4

#define ADDR_TEST_AUDIO_PERIOD_CH_5 (DPRX_BASE + SLAVE_ID_01 + 0x09DC)
#define TEST_AUDIO_PERIOD_CH5_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH5_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH5_FLDMASK_LEN                   4

#define ADDR_TEST_AUDIO_PERIOD_CH_6 (DPRX_BASE + SLAVE_ID_01 + 0x09E0)
#define TEST_AUDIO_PERIOD_CH6_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH6_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH6_FLDMASK_LEN                   4

#define ADDR_TEST_AUDIO_PERIOD_CH_7 (DPRX_BASE + SLAVE_ID_01 + 0x09E4)
#define TEST_AUDIO_PERIOD_CH7_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH7_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH7_FLDMASK_LEN                   4

#define ADDR_TEST_AUDIO_PERIOD_CH_8 (DPRX_BASE + SLAVE_ID_01 + 0x09E8)
#define TEST_AUDIO_PERIOD_CH8_FLDMASK                       0xf
#define TEST_AUDIO_PERIOD_CH8_FLDMASK_POS                   0
#define TEST_AUDIO_PERIOD_CH8_FLDMASK_LEN                   4

#define ADDR_FEC_STATUS (DPRX_BASE + SLAVE_ID_01 + 0x0A00)
#define FEC_DECODE_DIS_DETECTED_FLDMASK                     0x2
#define FEC_DECODE_DIS_DETECTED_FLDMASK_POS                 1
#define FEC_DECODE_DIS_DETECTED_FLDMASK_LEN                 1

#define FEC_DECODE_EN_DETECTED_FLDMASK                      0x1
#define FEC_DECODE_EN_DETECTED_FLDMASK_POS                  0
#define FEC_DECODE_EN_DETECTED_FLDMASK_LEN                  1

#define ADDR_FEC_ERROR_COUNT_L (DPRX_BASE + SLAVE_ID_01 + 0x0A04)
#define FEC_ERROR_COUNT_7_0_FLDMASK                         0xff
#define FEC_ERROR_COUNT_7_0_FLDMASK_POS                     0
#define FEC_ERROR_COUNT_7_0_FLDMASK_LEN                     8

#define ADDR_FEC_ERROR_COUNT_H (DPRX_BASE + SLAVE_ID_01 + 0x0A08)
#define FEC_ERROR_COUNT_VALID_FLDMASK                       0x80
#define FEC_ERROR_COUNT_VALID_FLDMASK_POS                   7
#define FEC_ERROR_COUNT_VALID_FLDMASK_LEN                   1

#define FEC_ERROR_COUNT_14_8_FLDMASK                        0x7f
#define FEC_ERROR_COUNT_14_8_FLDMASK_POS                    0
#define FEC_ERROR_COUNT_14_8_FLDMASK_LEN                    7

#define ADDR_SOURCE_IEEE_OUI_1 (DPRX_BASE + SLAVE_ID_01 + 0x0C00)
#define SOURCE_IEEE_OUI_1_FLDMASK                           0xff
#define SOURCE_IEEE_OUI_1_FLDMASK_POS                       0
#define SOURCE_IEEE_OUI_1_FLDMASK_LEN                       8

#define ADDR_SOURCE_IEEE_OUI_2 (DPRX_BASE + SLAVE_ID_01 + 0x0C04)
#define SOURCE_IEEE_OUI_2_FLDMASK                           0xff
#define SOURCE_IEEE_OUI_2_FLDMASK_POS                       0
#define SOURCE_IEEE_OUI_2_FLDMASK_LEN                       8

#define ADDR_SOURCE_IEEE_OUI_3 (DPRX_BASE + SLAVE_ID_01 + 0x0C08)
#define SOURCE_IEEE_OUI_3_FLDMASK                           0xff
#define SOURCE_IEEE_OUI_3_FLDMASK_POS                       0
#define SOURCE_IEEE_OUI_3_FLDMASK_LEN                       8

#define ADDR_SOURCE_CHIP_ID_1 (DPRX_BASE + SLAVE_ID_01 + 0x0C0C)
#define SOURCE_CHIP_ID_1_FLDMASK                            0xff
#define SOURCE_CHIP_ID_1_FLDMASK_POS                        0
#define SOURCE_CHIP_ID_1_FLDMASK_LEN                        8

#define ADDR_SOURCE_CHIP_ID_2 (DPRX_BASE + SLAVE_ID_01 + 0x0C10)
#define SOURCE_CHIP_ID_2_FLDMASK                            0xff
#define SOURCE_CHIP_ID_2_FLDMASK_POS                        0
#define SOURCE_CHIP_ID_2_FLDMASK_LEN                        8

#define ADDR_SOURCE_CHIP_ID_3 (DPRX_BASE + SLAVE_ID_01 + 0x0C14)
#define SOURCE_CHIP_ID_3_FLDMASK                            0xff
#define SOURCE_CHIP_ID_3_FLDMASK_POS                        0
#define SOURCE_CHIP_ID_3_FLDMASK_LEN                        8

#define ADDR_SOURCE_CHIP_ID_4 (DPRX_BASE + SLAVE_ID_01 + 0x0C18)
#define SOURCE_CHIP_ID_4_FLDMASK                            0xff
#define SOURCE_CHIP_ID_4_FLDMASK_POS                        0
#define SOURCE_CHIP_ID_4_FLDMASK_LEN                        8

#define ADDR_SOURCE_CHIP_ID_5 (DPRX_BASE + SLAVE_ID_01 + 0x0C1C)
#define SOURCE_CHIP_ID_5_FLDMASK                            0xff
#define SOURCE_CHIP_ID_5_FLDMASK_POS                        0
#define SOURCE_CHIP_ID_5_FLDMASK_LEN                        8

#define ADDR_SOURCE_CHIP_ID_6 (DPRX_BASE + SLAVE_ID_01 + 0x0C20)
#define SOURCE_CHIP_ID_6_FLDMASK                            0xff
#define SOURCE_CHIP_ID_6_FLDMASK_POS                        0
#define SOURCE_CHIP_ID_6_FLDMASK_LEN                        8

#define ADDR_SOURCE_CHIP_REVISION (DPRX_BASE + SLAVE_ID_01 + 0x0C24)
#define SOURCE_CHIP_MAJOR_REV_FLDMASK                       0xf0
#define SOURCE_CHIP_MAJOR_REV_FLDMASK_POS                   4
#define SOURCE_CHIP_MAJOR_REV_FLDMASK_LEN                   4

#define SOURCE_CHIP_MINOR_REV_FLDMASK                       0xf
#define SOURCE_CHIP_MINOR_REV_FLDMASK_POS                   0
#define SOURCE_CHIP_MINOR_REV_FLDMASK_LEN                   4

#define ADDR_SOURCE_FW_REVISION_MAJ (DPRX_BASE + SLAVE_ID_01 + 0x0C28)
#define SOURCE_FW_MAJOR_REV_FLDMASK                         0xff
#define SOURCE_FW_MAJOR_REV_FLDMASK_POS                     0
#define SOURCE_FW_MAJOR_REV_FLDMASK_LEN                     8

#define ADDR_SOURCE_FW_REVISION_MIN (DPRX_BASE + SLAVE_ID_01 + 0x0C2C)
#define SOURCE_FW_MINOR_REV_FLDMASK                         0xff
#define SOURCE_FW_MINOR_REV_FLDMASK_POS                     0
#define SOURCE_FW_MINOR_REV_FLDMASK_LEN                     8

#define ADDR_SINK_IEEE_OUI_1 (DPRX_BASE + SLAVE_ID_01 + 0x1000)
#define SINK_IEEE_OUI_1_FLDMASK                             0xff
#define SINK_IEEE_OUI_1_FLDMASK_POS                         0
#define SINK_IEEE_OUI_1_FLDMASK_LEN                         8

#define ADDR_SINK_IEEE_OUI_2 (DPRX_BASE + SLAVE_ID_01 + 0x1004)
#define SINK_IEEE_OUI_2_FLDMASK                             0xff
#define SINK_IEEE_OUI_2_FLDMASK_POS                         0
#define SINK_IEEE_OUI_2_FLDMASK_LEN                         8

#define ADDR_SINK_IEEE_OUI_3 (DPRX_BASE + SLAVE_ID_01 + 0x1008)
#define SINK_IEEE_OUI_3_FLDMASK                             0xff
#define SINK_IEEE_OUI_3_FLDMASK_POS                         0
#define SINK_IEEE_OUI_3_FLDMASK_LEN                         8

#define ADDR_SINK_CHIP_ID_1 (DPRX_BASE + SLAVE_ID_01 + 0x100C)
#define SINK_CHIP_ID_1_FLDMASK                              0xff
#define SINK_CHIP_ID_1_FLDMASK_POS                          0
#define SINK_CHIP_ID_1_FLDMASK_LEN                          8

#define ADDR_SINK_CHIP_ID_2 (DPRX_BASE + SLAVE_ID_01 + 0x1010)
#define SINK_CHIP_ID_2_FLDMASK                              0xff
#define SINK_CHIP_ID_2_FLDMASK_POS                          0
#define SINK_CHIP_ID_2_FLDMASK_LEN                          8

#define ADDR_SINK_CHIP_ID_3 (DPRX_BASE + SLAVE_ID_01 + 0x1014)
#define SINK_CHIP_ID_3_FLDMASK                              0xff
#define SINK_CHIP_ID_3_FLDMASK_POS                          0
#define SINK_CHIP_ID_3_FLDMASK_LEN                          8

#define ADDR_SINK_CHIP_ID_4 (DPRX_BASE + SLAVE_ID_01 + 0x1018)
#define SINK_CHIP_ID_4_FLDMASK                              0xff
#define SINK_CHIP_ID_4_FLDMASK_POS                          0
#define SINK_CHIP_ID_4_FLDMASK_LEN                          8

#define ADDR_SINK_CHIP_ID_5 (DPRX_BASE + SLAVE_ID_01 + 0x101C)
#define SINK_CHIP_ID_5_FLDMASK                              0xff
#define SINK_CHIP_ID_5_FLDMASK_POS                          0
#define SINK_CHIP_ID_5_FLDMASK_LEN                          8

#define ADDR_SINK_CHIP_ID_6 (DPRX_BASE + SLAVE_ID_01 + 0x1020)
#define SINK_CHIP_ID_6_FLDMASK                              0xff
#define SINK_CHIP_ID_6_FLDMASK_POS                          0
#define SINK_CHIP_ID_6_FLDMASK_LEN                          8

#define ADDR_SINK_CHIP_REVISION (DPRX_BASE + SLAVE_ID_01 + 0x1024)
#define SINK_CHIP_MAJOR_REV_FLDMASK                         0xf0
#define SINK_CHIP_MAJOR_REV_FLDMASK_POS                     4
#define SINK_CHIP_MAJOR_REV_FLDMASK_LEN                     4

#define SINK_CHIP_MINOR_REV_FLDMASK                         0xf
#define SINK_CHIP_MINOR_REV_FLDMASK_POS                     0
#define SINK_CHIP_MINOR_REV_FLDMASK_LEN                     4

#define ADDR_SINK_FW_MAJOR_REV (DPRX_BASE + SLAVE_ID_01 + 0x1028)
#define SINK_FW_MAJOR_REV_FLDMASK                           0xff
#define SINK_FW_MAJOR_REV_FLDMASK_POS                       0
#define SINK_FW_MAJOR_REV_FLDMASK_LEN                       8

#define ADDR_SINK_FW_MINOR_REV (DPRX_BASE + SLAVE_ID_01 + 0x102C)
#define SINK_FW_MINOR_REV_FLDMASK                           0xff
#define SINK_FW_MINOR_REV_FLDMASK_POS                       0
#define SINK_FW_MINOR_REV_FLDMASK_LEN                       8

#define ADDR_VENDOR_SPECIFIC_00 (DPRX_BASE + SLAVE_ID_01 + 0x1040)
#define VENDOR_SPECIFIC_00_FLDMASK                          0xff
#define VENDOR_SPECIFIC_00_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_00_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_01 (DPRX_BASE + SLAVE_ID_01 + 0x1044)
#define VENDOR_SPECIFIC_01_FLDMASK                          0xff
#define VENDOR_SPECIFIC_01_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_01_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_02 (DPRX_BASE + SLAVE_ID_01 + 0x1048)
#define VENDOR_SPECIFIC_02_FLDMASK                          0xff
#define VENDOR_SPECIFIC_02_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_02_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_03 (DPRX_BASE + SLAVE_ID_01 + 0x104c)
#define VENDOR_SPECIFIC_03_FLDMASK                          0xff
#define VENDOR_SPECIFIC_03_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_03_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_04 (DPRX_BASE + SLAVE_ID_01 + 0x1050)
#define VENDOR_SPECIFIC_04_FLDMASK                          0xff
#define VENDOR_SPECIFIC_04_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_04_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_05 (DPRX_BASE + SLAVE_ID_01 + 0x1054)
#define VENDOR_SPECIFIC_05_FLDMASK                          0xff
#define VENDOR_SPECIFIC_05_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_05_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_06 (DPRX_BASE + SLAVE_ID_01 + 0x1058)
#define VENDOR_SPECIFIC_06_FLDMASK                          0xff
#define VENDOR_SPECIFIC_06_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_06_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_07 (DPRX_BASE + SLAVE_ID_01 + 0x105c)
#define VENDOR_SPECIFIC_07_FLDMASK                          0xff
#define VENDOR_SPECIFIC_07_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_07_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_08 (DPRX_BASE + SLAVE_ID_01 + 0x1060)
#define VENDOR_SPECIFIC_08_FLDMASK                          0xff
#define VENDOR_SPECIFIC_08_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_08_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_09 (DPRX_BASE + SLAVE_ID_01 + 0x1064)
#define VENDOR_SPECIFIC_09_FLDMASK                          0xff
#define VENDOR_SPECIFIC_09_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_09_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_0A (DPRX_BASE + SLAVE_ID_01 + 0x1068)
#define VENDOR_SPECIFIC_0A_FLDMASK                          0xff
#define VENDOR_SPECIFIC_0A_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_0A_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_0B (DPRX_BASE + SLAVE_ID_01 + 0x106c)
#define VENDOR_SPECIFIC_0B_FLDMASK                          0xff
#define VENDOR_SPECIFIC_0B_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_0B_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_0C (DPRX_BASE + SLAVE_ID_01 + 0x1070)
#define VENDOR_SPECIFIC_0C_FLDMASK                          0xff
#define VENDOR_SPECIFIC_0C_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_0C_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_0D (DPRX_BASE + SLAVE_ID_01 + 0x1074)
#define VENDOR_SPECIFIC_0D_FLDMASK                          0xff
#define VENDOR_SPECIFIC_0D_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_0D_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_0E (DPRX_BASE + SLAVE_ID_01 + 0x1078)
#define VENDOR_SPECIFIC_0E_FLDMASK                          0xff
#define VENDOR_SPECIFIC_0E_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_0E_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_0F (DPRX_BASE + SLAVE_ID_01 + 0x107c)
#define VENDOR_SPECIFIC_0F_FLDMASK                          0xff
#define VENDOR_SPECIFIC_0F_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_0F_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_10 (DPRX_BASE + SLAVE_ID_01 + 0x1080)
#define VENDOR_SPECIFIC_10_FLDMASK                          0xff
#define VENDOR_SPECIFIC_10_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_10_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_11 (DPRX_BASE + SLAVE_ID_01 + 0x1084)
#define VENDOR_SPECIFIC_11_FLDMASK                          0xff
#define VENDOR_SPECIFIC_11_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_11_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_12 (DPRX_BASE + SLAVE_ID_01 + 0x1088)
#define VENDOR_SPECIFIC_12_FLDMASK                          0xff
#define VENDOR_SPECIFIC_12_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_12_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_13 (DPRX_BASE + SLAVE_ID_01 + 0x108c)
#define VENDOR_SPECIFIC_13_FLDMASK                          0xff
#define VENDOR_SPECIFIC_13_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_13_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_14 (DPRX_BASE + SLAVE_ID_01 + 0x1090)
#define VENDOR_SPECIFIC_14_FLDMASK                          0xff
#define VENDOR_SPECIFIC_14_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_14_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_15 (DPRX_BASE + SLAVE_ID_01 + 0x1094)
#define VENDOR_SPECIFIC_15_FLDMASK                          0xff
#define VENDOR_SPECIFIC_15_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_15_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_16 (DPRX_BASE + SLAVE_ID_01 + 0x1098)
#define VENDOR_SPECIFIC_16_FLDMASK                          0xff
#define VENDOR_SPECIFIC_16_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_16_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_17 (DPRX_BASE + SLAVE_ID_01 + 0x109c)
#define VENDOR_SPECIFIC_17_FLDMASK                          0xff
#define VENDOR_SPECIFIC_17_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_17_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_18 (DPRX_BASE + SLAVE_ID_01 + 0x10a0)
#define VENDOR_SPECIFIC_18_FLDMASK                          0xff
#define VENDOR_SPECIFIC_18_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_18_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_19 (DPRX_BASE + SLAVE_ID_01 + 0x10a4)
#define VENDOR_SPECIFIC_19_FLDMASK                          0xff
#define VENDOR_SPECIFIC_19_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_19_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_1A (DPRX_BASE + SLAVE_ID_01 + 0x10a8)
#define VENDOR_SPECIFIC_1A_FLDMASK                          0xff
#define VENDOR_SPECIFIC_1A_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_1A_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_1B (DPRX_BASE + SLAVE_ID_01 + 0x10ac)
#define VENDOR_SPECIFIC_1B_FLDMASK                          0xff
#define VENDOR_SPECIFIC_1B_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_1B_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_1C (DPRX_BASE + SLAVE_ID_01 + 0x10b0)
#define VENDOR_SPECIFIC_1C_FLDMASK                          0xff
#define VENDOR_SPECIFIC_1C_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_1C_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_1D (DPRX_BASE + SLAVE_ID_01 + 0x10b4)
#define VENDOR_SPECIFIC_1D_FLDMASK                          0xff
#define VENDOR_SPECIFIC_1D_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_1D_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_1E (DPRX_BASE + SLAVE_ID_01 + 0x10b8)
#define VENDOR_SPECIFIC_1E_FLDMASK                          0xff
#define VENDOR_SPECIFIC_1E_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_1E_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_1F (DPRX_BASE + SLAVE_ID_01 + 0x10bc)
#define VENDOR_SPECIFIC_1F_FLDMASK                          0xff
#define VENDOR_SPECIFIC_1F_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_1F_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_20 (DPRX_BASE + SLAVE_ID_01 + 0x10c0)
#define VENDOR_SPECIFIC_20_FLDMASK                          0xff
#define VENDOR_SPECIFIC_20_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_20_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_21 (DPRX_BASE + SLAVE_ID_01 + 0x10c4)
#define VENDOR_SPECIFIC_21_FLDMASK                          0xff
#define VENDOR_SPECIFIC_21_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_21_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_22 (DPRX_BASE + SLAVE_ID_01 + 0x10c8)
#define VENDOR_SPECIFIC_22_FLDMASK                          0xff
#define VENDOR_SPECIFIC_22_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_22_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_23 (DPRX_BASE + SLAVE_ID_01 + 0x10cc)
#define VENDOR_SPECIFIC_23_FLDMASK                          0xff
#define VENDOR_SPECIFIC_23_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_23_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_24 (DPRX_BASE + SLAVE_ID_01 + 0x10d0)
#define VENDOR_SPECIFIC_24_FLDMASK                          0xff
#define VENDOR_SPECIFIC_24_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_24_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_25 (DPRX_BASE + SLAVE_ID_01 + 0x10d4)
#define VENDOR_SPECIFIC_25_FLDMASK                          0xff
#define VENDOR_SPECIFIC_25_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_25_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_26 (DPRX_BASE + SLAVE_ID_01 + 0x10d8)
#define VENDOR_SPECIFIC_26_FLDMASK                          0xff
#define VENDOR_SPECIFIC_26_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_26_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_27 (DPRX_BASE + SLAVE_ID_01 + 0x10dc)
#define VENDOR_SPECIFIC_27_FLDMASK                          0xff
#define VENDOR_SPECIFIC_27_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_27_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_28 (DPRX_BASE + SLAVE_ID_01 + 0x10e0)
#define VENDOR_SPECIFIC_28_FLDMASK                          0xff
#define VENDOR_SPECIFIC_28_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_28_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_29 (DPRX_BASE + SLAVE_ID_01 + 0x10e4)
#define VENDOR_SPECIFIC_29_FLDMASK                          0xff
#define VENDOR_SPECIFIC_29_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_29_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_2A (DPRX_BASE + SLAVE_ID_01 + 0x10e8)
#define VENDOR_SPECIFIC_2A_FLDMASK                          0xff
#define VENDOR_SPECIFIC_2A_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_2A_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_2B (DPRX_BASE + SLAVE_ID_01 + 0x10ec)
#define VENDOR_SPECIFIC_2B_FLDMASK                          0xff
#define VENDOR_SPECIFIC_2B_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_2B_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_2C (DPRX_BASE + SLAVE_ID_01 + 0x10f0)
#define VENDOR_SPECIFIC_2C_FLDMASK                          0xff
#define VENDOR_SPECIFIC_2C_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_2C_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_2D (DPRX_BASE + SLAVE_ID_01 + 0x10f4)
#define VENDOR_SPECIFIC_2D_FLDMASK                          0xff
#define VENDOR_SPECIFIC_2D_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_2D_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_2E (DPRX_BASE + SLAVE_ID_01 + 0x10f8)
#define VENDOR_SPECIFIC_2E_FLDMASK                          0xff
#define VENDOR_SPECIFIC_2E_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_2E_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_2F (DPRX_BASE + SLAVE_ID_01 + 0x10fc)
#define VENDOR_SPECIFIC_2F_FLDMASK                          0xff
#define VENDOR_SPECIFIC_2F_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_2F_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_30 (DPRX_BASE + SLAVE_ID_01 + 0x1100)
#define VENDOR_SPECIFIC_30_FLDMASK                          0xff
#define VENDOR_SPECIFIC_30_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_30_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_31 (DPRX_BASE + SLAVE_ID_01 + 0x1104)
#define VENDOR_SPECIFIC_31_FLDMASK                          0xff
#define VENDOR_SPECIFIC_31_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_31_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_32 (DPRX_BASE + SLAVE_ID_01 + 0x1108)
#define VENDOR_SPECIFIC_32_FLDMASK                          0xff
#define VENDOR_SPECIFIC_32_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_32_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_33 (DPRX_BASE + SLAVE_ID_01 + 0x110c)
#define VENDOR_SPECIFIC_33_FLDMASK                          0xff
#define VENDOR_SPECIFIC_33_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_33_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_34 (DPRX_BASE + SLAVE_ID_01 + 0x1110)
#define VENDOR_SPECIFIC_34_FLDMASK                          0xff
#define VENDOR_SPECIFIC_34_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_34_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_35 (DPRX_BASE + SLAVE_ID_01 + 0x1114)
#define VENDOR_SPECIFIC_35_FLDMASK                          0xff
#define VENDOR_SPECIFIC_35_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_35_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_36 (DPRX_BASE + SLAVE_ID_01 + 0x1118)
#define VENDOR_SPECIFIC_36_FLDMASK                          0xff
#define VENDOR_SPECIFIC_36_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_36_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_37 (DPRX_BASE + SLAVE_ID_01 + 0x111c)
#define VENDOR_SPECIFIC_37_FLDMASK                          0xff
#define VENDOR_SPECIFIC_37_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_37_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_38 (DPRX_BASE + SLAVE_ID_01 + 0x1120)
#define VENDOR_SPECIFIC_38_FLDMASK                          0xff
#define VENDOR_SPECIFIC_38_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_38_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_39 (DPRX_BASE + SLAVE_ID_01 + 0x1124)
#define VENDOR_SPECIFIC_39_FLDMASK                          0xff
#define VENDOR_SPECIFIC_39_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_39_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_3A (DPRX_BASE + SLAVE_ID_01 + 0x1128)
#define VENDOR_SPECIFIC_3A_FLDMASK                          0xff
#define VENDOR_SPECIFIC_3A_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_3A_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_3B (DPRX_BASE + SLAVE_ID_01 + 0x112c)
#define VENDOR_SPECIFIC_3B_FLDMASK                          0xff
#define VENDOR_SPECIFIC_3B_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_3B_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_3C (DPRX_BASE + SLAVE_ID_01 + 0x1130)
#define VENDOR_SPECIFIC_3C_FLDMASK                          0xff
#define VENDOR_SPECIFIC_3C_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_3C_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_3D (DPRX_BASE + SLAVE_ID_01 + 0x1134)
#define VENDOR_SPECIFIC_3D_FLDMASK                          0xff
#define VENDOR_SPECIFIC_3D_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_3D_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_3E (DPRX_BASE + SLAVE_ID_01 + 0x1138)
#define VENDOR_SPECIFIC_3E_FLDMASK                          0xff
#define VENDOR_SPECIFIC_3E_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_3E_FLDMASK_LEN                      8

#define ADDR_VENDOR_SPECIFIC_3F (DPRX_BASE + SLAVE_ID_01 + 0x113C)
#define VENDOR_SPECIFIC_3F_FLDMASK                          0xff
#define VENDOR_SPECIFIC_3F_FLDMASK_POS                      0
#define VENDOR_SPECIFIC_3F_FLDMASK_LEN                      8

#define ADDR_DPCD_INT_1 (DPRX_BASE + SLAVE_ID_01 + 0x13C4)
#define BW_CHANGE_FLDMASK                                   0x40
#define BW_CHANGE_FLDMASK_POS                               6
#define BW_CHANGE_FLDMASK_LEN                               1

#define EQ_TRAINING_FLDMASK                                 0x4
#define EQ_TRAINING_FLDMASK_POS                             2
#define EQ_TRAINING_FLDMASK_LEN                             1

#define CLK_RC_TRAINING_FLDMASK                             0x2
#define CLK_RC_TRAINING_FLDMASK_POS                         1
#define CLK_RC_TRAINING_FLDMASK_LEN                         1

#define ADDR_DPCD_INT_2 (DPRX_BASE + SLAVE_ID_01 + 0x13C8)
#define FLOATING_DPCD_7_INTR_0_FLDMASK                      0x80
#define FLOATING_DPCD_7_INTR_0_FLDMASK_POS                  7
#define FLOATING_DPCD_7_INTR_0_FLDMASK_LEN                  1

#define FLOATING_DPCD_6_INTR_0_FLDMASK                      0x40
#define FLOATING_DPCD_6_INTR_0_FLDMASK_POS                  6
#define FLOATING_DPCD_6_INTR_0_FLDMASK_LEN                  1

#define FLOATING_DPCD_5_INTR_0_FLDMASK                      0x20
#define FLOATING_DPCD_5_INTR_0_FLDMASK_POS                  5
#define FLOATING_DPCD_5_INTR_0_FLDMASK_LEN                  1

#define FLOATING_DPCD_4_INTR_0_FLDMASK                      0x10
#define FLOATING_DPCD_4_INTR_0_FLDMASK_POS                  4
#define FLOATING_DPCD_4_INTR_0_FLDMASK_LEN                  1

#define FLOATING_DPCD_3_INTR_0_FLDMASK                      0x8
#define FLOATING_DPCD_3_INTR_0_FLDMASK_POS                  3
#define FLOATING_DPCD_3_INTR_0_FLDMASK_LEN                  1

#define FLOATING_DPCD_2_INTR_0_FLDMASK                      0x4
#define FLOATING_DPCD_2_INTR_0_FLDMASK_POS                  2
#define FLOATING_DPCD_2_INTR_0_FLDMASK_LEN                  1

#define FLOATING_DPCD_1_INTR_0_FLDMASK                      0x2
#define FLOATING_DPCD_1_INTR_0_FLDMASK_POS                  1
#define FLOATING_DPCD_1_INTR_0_FLDMASK_LEN                  1

#define FLOATING_DPCD_0_INTR_0_FLDMASK                      0x1
#define FLOATING_DPCD_0_INTR_0_FLDMASK_POS                  0
#define FLOATING_DPCD_0_INTR_0_FLDMASK_LEN                  1

#define ADDR_DPCD_INT_3 (DPRX_BASE + SLAVE_ID_01 + 0x13CC)
#define FLOATING_DPCD_7_INTR_1_FLDMASK                      0x80
#define FLOATING_DPCD_7_INTR_1_FLDMASK_POS                  7
#define FLOATING_DPCD_7_INTR_1_FLDMASK_LEN                  1

#define FLOATING_DPCD_6_INTR_1_FLDMASK                      0x40
#define FLOATING_DPCD_6_INTR_1_FLDMASK_POS                  6
#define FLOATING_DPCD_6_INTR_1_FLDMASK_LEN                  1

#define FLOATING_DPCD_5_INTR_1_FLDMASK                      0x20
#define FLOATING_DPCD_5_INTR_1_FLDMASK_POS                  5
#define FLOATING_DPCD_5_INTR_1_FLDMASK_LEN                  1

#define FLOATING_DPCD_4_INTR_1_FLDMASK                      0x10
#define FLOATING_DPCD_4_INTR_1_FLDMASK_POS                  4
#define FLOATING_DPCD_4_INTR_1_FLDMASK_LEN                  1

#define FLOATING_DPCD_3_INTR_1_FLDMASK                      0x8
#define FLOATING_DPCD_3_INTR_1_FLDMASK_POS                  3
#define FLOATING_DPCD_3_INTR_1_FLDMASK_LEN                  1

#define FLOATING_DPCD_2_INTR_1_FLDMASK                      0x4
#define FLOATING_DPCD_2_INTR_1_FLDMASK_POS                  2
#define FLOATING_DPCD_2_INTR_1_FLDMASK_LEN                  1

#define FLOATING_DPCD_1_INTR_1_FLDMASK                      0x2
#define FLOATING_DPCD_1_INTR_1_FLDMASK_POS                  1
#define FLOATING_DPCD_1_INTR_1_FLDMASK_LEN                  1

#define FLOATING_DPCD_0_INTR_1_FLDMASK                      0x1
#define FLOATING_DPCD_0_INTR_1_FLDMASK_POS                  0
#define FLOATING_DPCD_0_INTR_1_FLDMASK_LEN                  1

#define ADDR_DPCD_INT_1_MASK (DPRX_BASE + SLAVE_ID_01 + 0x13D4)
#define DPCD_INT_1_MASK_FLDMASK                             0xff
#define DPCD_INT_1_MASK_FLDMASK_POS                         0
#define DPCD_INT_1_MASK_FLDMASK_LEN                         8

#define ADDR_DPCD_INT_2_MASK (DPRX_BASE + SLAVE_ID_01 + 0x13D8)
#define DPCD_INT_2_MASK_FLDMASK                             0xff
#define DPCD_INT_2_MASK_FLDMASK_POS                         0
#define DPCD_INT_2_MASK_FLDMASK_LEN                         8

#define ADDR_DPCD_INT_3_MASK (DPRX_BASE + SLAVE_ID_01 + 0x13DC)
#define DPCD_INT_3_MASK_FLDMASK                             0xff
#define DPCD_INT_3_MASK_FLDMASK_POS                         0
#define DPCD_INT_3_MASK_FLDMASK_LEN                         8

#define ADDR_SET_POWER (DPRX_BASE + SLAVE_ID_01 + 0x1800)
#define SET_DN_DEVICE_DP_PWR_18V_FLDMASK                    0x80
#define SET_DN_DEVICE_DP_PWR_18V_FLDMASK_POS                7
#define SET_DN_DEVICE_DP_PWR_18V_FLDMASK_LEN                1

#define SET_DN_DEVICE_DP_PWR_12V_FLDMASK                    0x40
#define SET_DN_DEVICE_DP_PWR_12V_FLDMASK_POS                6
#define SET_DN_DEVICE_DP_PWR_12V_FLDMASK_LEN                1

#define SET_DN_DEVICE_DP_PWR_5V_FLDMASK                     0x20
#define SET_DN_DEVICE_DP_PWR_5V_FLDMASK_POS                 5
#define SET_DN_DEVICE_DP_PWR_5V_FLDMASK_LEN                 1

#define SET_POWER_FLDMASK                                   0x7
#define SET_POWER_FLDMASK_POS                               0
#define SET_POWER_FLDMASK_LEN                               3

#define ADDR_SINK_COUNT_ESI (DPRX_BASE + SLAVE_ID_01 + 0x8008)
#define SINK_COUNT_ESI_FLDMASK                              0xff
#define SINK_COUNT_ESI_FLDMASK_POS                          0
#define SINK_COUNT_ESI_FLDMASK_LEN                          8

#define ADDR_DEVICE_SERVICE_IRQ_VECTOR_ESI0 (DPRX_BASE + SLAVE_ID_01 + 0x800C)
#define DEVICE_SERVICE_IRQ_VECTOR_ESI0_FLDMASK              0xff
#define DEVICE_SERVICE_IRQ_VECTOR_ESI0_FLDMASK_POS          0
#define DEVICE_SERVICE_IRQ_VECTOR_ESI0_FLDMASK_LEN          8

#define ADDR_DEVICE_SERVICE_IRQ_VECTOR_ESI1 (DPRX_BASE + SLAVE_ID_01 + 0x8010)
#define CEC_IRQ_FLDMASK                                     0x4
#define CEC_IRQ_FLDMASK_POS                                 2
#define CEC_IRQ_FLDMASK_LEN                                 1

#define LOCK_ACQUISITION_REQUEST_FLDMASK                    0x2
#define LOCK_ACQUISITION_REQUEST_FLDMASK_POS                1
#define LOCK_ACQUISITION_REQUEST_FLDMASK_LEN                1

#define RX_GTC_MSTR_REQ_STATUS_CHANGE_FLDMASK               0x1
#define RX_GTC_MSTR_REQ_STATUS_CHANGE_FLDMASK_POS           0
#define RX_GTC_MSTR_REQ_STATUS_CHANGE_FLDMASK_LEN           1

#define ADDR_LINK_SERVICE_IRQ_VECTOR_ESI0 (DPRX_BASE + SLAVE_ID_01 + 0x8014)
#define CONNECTED_OFF_ENTRY_REQUESTED_FLDMASK               0x10
#define CONNECTED_OFF_ENTRY_REQUESTED_FLDMASK_POS           4
#define CONNECTED_OFF_ENTRY_REQUESTED_FLDMASK_LEN           1

#define HDMI_LINK_STATUS_CHANGED_FLDMASK                    0x8
#define HDMI_LINK_STATUS_CHANGED_FLDMASK_POS                3
#define HDMI_LINK_STATUS_CHANGED_FLDMASK_LEN                1

#define STREAM_STATUS_CHANGED_FLDMASK                       0x4
#define STREAM_STATUS_CHANGED_FLDMASK_POS                   2
#define STREAM_STATUS_CHANGED_FLDMASK_LEN                   1

#define LINK_STATUS_CHANGED_FLDMASK                         0x2
#define LINK_STATUS_CHANGED_FLDMASK_POS                     1
#define LINK_STATUS_CHANGED_FLDMASK_LEN                     1

#define RX_CAP_CHANGED_FLDMASK                              0x1
#define RX_CAP_CHANGED_FLDMASK_POS                          0
#define RX_CAP_CHANGED_FLDMASK_LEN                          1

#define ADDR_LANE0_1_STATUS_ESI (DPRX_BASE + SLAVE_ID_01 + 0x8030)
#define LANE0_1_STATUS_ESI_FLDMASK                          0xff
#define LANE0_1_STATUS_ESI_FLDMASK_POS                      0
#define LANE0_1_STATUS_ESI_FLDMASK_LEN                      8

#define ADDR_LANE2_3_STATUS_ESI (DPRX_BASE + SLAVE_ID_01 + 0x8034)
#define LANE2_3_STATUS_ESI_FLDMASK                          0xff
#define LANE2_3_STATUS_ESI_FLDMASK_POS                      0
#define LANE2_3_STATUS_ESI_FLDMASK_LEN                      8

#define ADDR_LANE_ALIGN_STATUS_UPDATED_ESI (DPRX_BASE + SLAVE_ID_01 + 0x8038)
#define LANE_ALIGN_STATUS_UPDATED_ESI_FLDMASK               0xff
#define LANE_ALIGN_STATUS_UPDATED_ESI_FLDMASK_POS           0
#define LANE_ALIGN_STATUS_UPDATED_ESI_FLDMASK_LEN           8

#define ADDR_SINK_STATUS_ESI (DPRX_BASE + SLAVE_ID_01 + 0x803C)
#define SINK_STATUS_ESI_FLDMASK                             0xff
#define SINK_STATUS_ESI_FLDMASK_POS                         0
#define SINK_STATUS_ESI_FLDMASK_LEN                         8

#define ADDR_EXT_DPCD_REV (DPRX_BASE + SLAVE_ID_01 + 0x8800)
#define EXT_DPCD_MAJOR_REV_FLDMASK                          0xf0
#define EXT_DPCD_MAJOR_REV_FLDMASK_POS                      4
#define EXT_DPCD_MAJOR_REV_FLDMASK_LEN                      4

#define EXT_DPCD_MINOR_REV_FLDMASK                          0xf
#define EXT_DPCD_MINOR_REV_FLDMASK_POS                      0
#define EXT_DPCD_MINOR_REV_FLDMASK_LEN                      4

#define ADDR_EXT_MAX_LINK_RATE (DPRX_BASE + SLAVE_ID_01 + 0x8804)
#define EXT_MAX_LINK_RATE_FLDMASK                           0xff
#define EXT_MAX_LINK_RATE_FLDMASK_POS                       0
#define EXT_MAX_LINK_RATE_FLDMASK_LEN                       8

#define ADDR_EXT_MAX_LANE_COUNT (DPRX_BASE + SLAVE_ID_01 + 0x8808)
#define EXT_ENHANCE_FRAME_CAP_FLDMASK                       0x80
#define EXT_ENHANCE_FRAME_CAP_FLDMASK_POS                   7
#define EXT_ENHANCE_FRAME_CAP_FLDMASK_LEN                   1

#define EXT_TPS3_SUPPORTED_FLDMASK                          0x40
#define EXT_TPS3_SUPPORTED_FLDMASK_POS                      6
#define EXT_TPS3_SUPPORTED_FLDMASK_LEN                      1

#define EXT_POST_LT_ADJ_REQ_SUPPORTED_FLDMASK               0x20
#define EXT_POST_LT_ADJ_REQ_SUPPORTED_FLDMASK_POS           5
#define EXT_POST_LT_ADJ_REQ_SUPPORTED_FLDMASK_LEN           1

#define EXT_MAX_LANE_COUNT_FLDMASK                          0x1f
#define EXT_MAX_LANE_COUNT_FLDMASK_POS                      0
#define EXT_MAX_LANE_COUNT_FLDMASK_LEN                      5

#define ADDR_EXT_MAX_DOWNSPREAD (DPRX_BASE + SLAVE_ID_01 + 0x880c)
#define EXT_TPS4_SUPPORTED_FLDMASK                          0x80
#define EXT_TPS4_SUPPORTED_FLDMASK_POS                      7
#define EXT_TPS4_SUPPORTED_FLDMASK_LEN                      1

#define EXT_NO_AUX_TRANSACTION_LINK_TRAINING_FLDMASK        0x40
#define EXT_NO_AUX_TRANSACTION_LINK_TRAINING_FLDMASK_POS    6
#define EXT_NO_AUX_TRANSACTION_LINK_TRAINING_FLDMASK_LEN    1

#define EXT_MAX_DOWNSPREAD_FLDMASK                          0x1
#define EXT_MAX_DOWNSPREAD_FLDMASK_POS                      0
#define EXT_MAX_DOWNSPREAD_FLDMASK_LEN                      1

#define ADDR_EXT_NORP_DP_PWR_VOLTAGE_CAP (DPRX_BASE + SLAVE_ID_01 + 0x8810)
#define EXT_VOL18V_DP_PWR_CAP_FLDMASK                       0x80
#define EXT_VOL18V_DP_PWR_CAP_FLDMASK_POS                   7
#define EXT_VOL18V_DP_PWR_CAP_FLDMASK_LEN                   1

#define EXT_VOL12V_DP_PWR_CAP_FLDMASK                       0x40
#define EXT_VOL12V_DP_PWR_CAP_FLDMASK_POS                   6
#define EXT_VOL12V_DP_PWR_CAP_FLDMASK_LEN                   1

#define EXT_VOL5V_DP_PWR_CAP_FLDMASK                        0x20
#define EXT_VOL5V_DP_PWR_CAP_FLDMASK_POS                    5
#define EXT_VOL5V_DP_PWR_CAP_FLDMASK_LEN                    1

#define EXT_NORP_FLDMASK                                    0x1
#define EXT_NORP_FLDMASK_POS                                0
#define EXT_NORP_FLDMASK_LEN                                1

#define ADDR_EXT_DOWN_STREAM_PORT_PRESENT (DPRX_BASE + SLAVE_ID_01 + 0x8814)
#define EXT_DETAILED_CAP_INFO_AVAILABLE_FLDMASK             0x10
#define EXT_DETAILED_CAP_INFO_AVAILABLE_FLDMASK_POS         4
#define EXT_DETAILED_CAP_INFO_AVAILABLE_FLDMASK_LEN         1

#define EXT_FORMAT_CONVERSION_FLDMASK                       0x8
#define EXT_FORMAT_CONVERSION_FLDMASK_POS                   3
#define EXT_FORMAT_CONVERSION_FLDMASK_LEN                   1

#define EXT_DFP_TYPE_FLDMASK                                0x6
#define EXT_DFP_TYPE_FLDMASK_POS                            1
#define EXT_DFP_TYPE_FLDMASK_LEN                            2

#define EXT_DFP_PRESENT_FLDMASK                             0x1
#define EXT_DFP_PRESENT_FLDMASK_POS                         0
#define EXT_DFP_PRESENT_FLDMASK_LEN                         1

#define ADDR_EXT_MAIN_LINK_CHANNEL_CODING (DPRX_BASE + SLAVE_ID_01 + 0x8818)
#define EXT_ANSI8B10B_FLDMASK                               0x1
#define EXT_ANSI8B10B_FLDMASK_POS                           0
#define EXT_ANSI8B10B_FLDMASK_LEN                           1

#define ADDR_EXT_DOWN_STREAM_PORT_COUNT (DPRX_BASE + SLAVE_ID_01 + 0x881C)
#define EXT_OUI_SUPPORT_FLDMASK                             0x80
#define EXT_OUI_SUPPORT_FLDMASK_POS                         7
#define EXT_OUI_SUPPORT_FLDMASK_LEN                         1

#define EXT_MSA_TIMING_PAR_IGNORED_FLDMASK                  0x40
#define EXT_MSA_TIMING_PAR_IGNORED_FLDMASK_POS              6
#define EXT_MSA_TIMING_PAR_IGNORED_FLDMASK_LEN              1

#define EXT_DFP_COUNT_FLDMASK                               0xf
#define EXT_DFP_COUNT_FLDMASK_POS                           0
#define EXT_DFP_COUNT_FLDMASK_LEN                           4

#define ADDR_EXT_RECEIVE_PORT0_CAP_0 (DPRX_BASE + SLAVE_ID_01 + 0x8820)
#define EXT_ASSOCIATED_TO_PRECEDING_PORT_0_FLDMASK          0x4
#define EXT_ASSOCIATED_TO_PRECEDING_PORT_0_FLDMASK_POS      2
#define EXT_ASSOCIATED_TO_PRECEDING_PORT_0_FLDMASK_LEN      1

#define EXT_LOCAL_EDID_PRESENT_0_FLDMASK                    0x2
#define EXT_LOCAL_EDID_PRESENT_0_FLDMASK_POS                1
#define EXT_LOCAL_EDID_PRESENT_0_FLDMASK_LEN                1

#define ADDR_EXT_RECEIVE_PORT0_CAP_1 (DPRX_BASE + SLAVE_ID_01 + 0x8824)
#define EXT_BUFFER_SIZE_0_FLDMASK                           0xff
#define EXT_BUFFER_SIZE_0_FLDMASK_POS                       0
#define EXT_BUFFER_SIZE_0_FLDMASK_LEN                       8

#define ADDR_EXT_RECEIVE_PORT1_CAP_0 (DPRX_BASE + SLAVE_ID_01 + 0x8828)
#define EXT_ASSOCIATED_TO_PRECEDING_PORT_1_FLDMASK          0x4
#define EXT_ASSOCIATED_TO_PRECEDING_PORT_1_FLDMASK_POS      2
#define EXT_ASSOCIATED_TO_PRECEDING_PORT_1_FLDMASK_LEN      1

#define EXT_LOCAL_EDID_PRESENT_1_FLDMASK                    0x2
#define EXT_LOCAL_EDID_PRESENT_1_FLDMASK_POS                1
#define EXT_LOCAL_EDID_PRESENT_1_FLDMASK_LEN                1

#define ADDR_EXT_RECEIVE_PORT1_CAP_1 (DPRX_BASE + SLAVE_ID_01 + 0x882C)
#define EXT_BUFFER_SIZE_1_FLDMASK                           0xff
#define EXT_BUFFER_SIZE_1_FLDMASK_POS                       0
#define EXT_BUFFER_SIZE_1_FLDMASK_LEN                       8

#define ADDR_EXT_I2C_SPEED_CONTROL_CAP (DPRX_BASE + SLAVE_ID_01 + 0x8830)
#define EXT_I2C_SPEED_CONTROL_CAP_FLDMASK                   0xff
#define EXT_I2C_SPEED_CONTROL_CAP_FLDMASK_POS               0
#define EXT_I2C_SPEED_CONTROL_CAP_FLDMASK_LEN               8

#define ADDR_EXT_EDP_CONFIG_CAP (DPRX_BASE + SLAVE_ID_01 + 0x8834)
#define EXT_ALTERNATE_SCRAMBLER_RESET_CAPABLE_FLDMASK       0x1
#define EXT_ALTERNATE_SCRAMBLER_RESET_CAPABLE_FLDMASK_POS   0
#define EXT_ALTERNATE_SCRAMBLER_RESET_CAPABLE_FLDMASK_LEN   1

#define ADDR_EXT_TRAINING_AUX_RD_INTERVAL (DPRX_BASE + SLAVE_ID_01 + 0x8838)
#define EXT_EXTENDED_RX_CAP_FIELD_PRESENT_FLDMASK           0x80
#define EXT_EXTENDED_RX_CAP_FIELD_PRESENT_FLDMASK_POS       7
#define EXT_EXTENDED_RX_CAP_FIELD_PRESENT_FLDMASK_LEN       1

#define EXT_TRAINING_AUX_RD_INTERVAL_FLDMASK                0x7f
#define EXT_TRAINING_AUX_RD_INTERVAL_FLDMASK_POS            0
#define EXT_TRAINING_AUX_RD_INTERVAL_FLDMASK_LEN            7

#define ADDR_EXT_ADAPTOR_CAP (DPRX_BASE + SLAVE_ID_01 + 0x883C)
#define EXT_ALTERNATE_I2C_PATTERN_CAP_FLDMASK               0x2
#define EXT_ALTERNATE_I2C_PATTERN_CAP_FLDMASK_POS           1
#define EXT_ALTERNATE_I2C_PATTERN_CAP_FLDMASK_LEN           1

#define EXT_FORCE_LOAD_SENSE_CAP_FLDMASK                    0x1
#define EXT_FORCE_LOAD_SENSE_CAP_FLDMASK_POS                0
#define EXT_FORCE_LOAD_SENSE_CAP_FLDMASK_LEN                1

#define ADDR_EXT_DPRX_FEATURE_ENUM_LIST (DPRX_BASE + SLAVE_ID_01 + 0x8840)
#define VSC_EXT_CEA_SDP_CHAINING_SUPPORTED_FLDMASK          0x80
#define VSC_EXT_CEA_SDP_CHAINING_SUPPORTED_FLDMASK_POS      7
#define VSC_EXT_CEA_SDP_CHAINING_SUPPORTED_FLDMASK_LEN      1

#define VSC_EXT_CEA_SDP_SUPPORTED_FLDMASK                   0x40
#define VSC_EXT_CEA_SDP_SUPPORTED_FLDMASK_POS               6
#define VSC_EXT_CEA_SDP_SUPPORTED_FLDMASK_LEN               1

#define VSC_EXT_VESA_SDP_CHAINING_SUPPORTED_FLDMASK         0x20
#define VSC_EXT_VESA_SDP_CHAINING_SUPPORTED_FLDMASK_POS     5
#define VSC_EXT_VESA_SDP_CHAINING_SUPPORTED_FLDMASK_LEN     1

#define VSC_EXT_VESA_SDP_SUPPORTED_FLDMASK                  0x10
#define VSC_EXT_VESA_SDP_SUPPORTED_FLDMASK_POS              4
#define VSC_EXT_VESA_SDP_SUPPORTED_FLDMASK_LEN              1

#define VSC_SDP_EXT_FOR_COLORIMETRY_SUPPORT_FLDMASK         0x8
#define VSC_SDP_EXT_FOR_COLORIMETRY_SUPPORT_FLDMASK_POS     3
#define VSC_SDP_EXT_FOR_COLORIMETRY_SUPPORT_FLDMASK_LEN     1

#define AV_SYNC_CAP_FLDMASK                                 0x4
#define AV_SYNC_CAP_FLDMASK_POS                             2
#define AV_SYNC_CAP_FLDMASK_LEN                             1

#define SST_SPLIT_SDP_CAP_FLDMASK                           0x2
#define SST_SPLIT_SDP_CAP_FLDMASK_POS                       1
#define SST_SPLIT_SDP_CAP_FLDMASK_LEN                       1

#define GTC_CAP_FLDMASK                                     0x1
#define GTC_CAP_FLDMASK_POS                                 0
#define GTC_CAP_FLDMASK_LEN                                 1

#define ADDR_EXT_DPRX_SLEEP_WAKE_TIMEOUT_REQ (DPRX_BASE + SLAVE_ID_01 + 0x8844)
#define DPRX_SLEEP_WAKE_TIMEOUT_PERIOD_FLDMASK              0xff
#define DPRX_SLEEP_WAKE_TIMEOUT_PERIOD_FLDMASK_POS          0
#define DPRX_SLEEP_WAKE_TIMEOUT_PERIOD_FLDMASK_LEN          8

#define ADDR_VSC_EXT_VESA_SDP_MAX_CHAINING (DPRX_BASE + SLAVE_ID_01 + 0x8848)
#define VSC_EXT_VESA_SDP_MAX_CHAINING_COUNT_FLDMASK         0xff
#define VSC_EXT_VESA_SDP_MAX_CHAINING_COUNT_FLDMASK_POS     0
#define VSC_EXT_VESA_SDP_MAX_CHAINING_COUNT_FLDMASK_LEN     8

#define ADDR_VSC_EXT_CEA_SDP_MAX_CHAINING (DPRX_BASE + SLAVE_ID_01 + 0x884C)
#define VSC_EXT_CEA_SDP_MAX_CHAINING_COUNT_FLDMASK          0xff
#define VSC_EXT_CEA_SDP_MAX_CHAINING_COUNT_FLDMASK_POS      0
#define VSC_EXT_CEA_SDP_MAX_CHAINING_COUNT_FLDMASK_LEN      8

#define ADDR_PROTOCOL_CONVERTER_CONTROL_0 (DPRX_BASE + SLAVE_ID_01 + 0xC140)
#define PROTOCOL_CONVERTER_CONTROL_0_FLDMASK                0xff
#define PROTOCOL_CONVERTER_CONTROL_0_FLDMASK_POS            0
#define PROTOCOL_CONVERTER_CONTROL_0_FLDMASK_LEN            8

#define ADDR_PROTOCOL_CONVERTER_CONTROL_1 (DPRX_BASE + SLAVE_ID_01 + 0xC144)
#define PROTOCOL_CONVERTER_CONTROL_1_FLDMASK                0xff
#define PROTOCOL_CONVERTER_CONTROL_1_FLDMASK_POS            0
#define PROTOCOL_CONVERTER_CONTROL_1_FLDMASK_LEN            8

#define ADDR_PROTOCOL_CONVERTER_CONTROL_2 (DPRX_BASE + SLAVE_ID_01 + 0xC148)
#define PROTOCOL_CONVERTER_CONTROL_2_FLDMASK                0xff
#define PROTOCOL_CONVERTER_CONTROL_2_FLDMASK_POS            0
#define PROTOCOL_CONVERTER_CONTROL_2_FLDMASK_LEN            8

#define ADDR_OUTPUT_HTOTAL_L (DPRX_BASE + SLAVE_ID_01 + 0xC150)
#define OUTPUT_HTOTAL_7_0_FLDMASK                           0xff
#define OUTPUT_HTOTAL_7_0_FLDMASK_POS                       0
#define OUTPUT_HTOTAL_7_0_FLDMASK_LEN                       8

#define ADDR_OUTPUT_HTOTAL_H (DPRX_BASE + SLAVE_ID_01 + 0xC154)
#define OUTPUT_HTOTAL_15_8_FLDMASK                          0xff
#define OUTPUT_HTOTAL_15_8_FLDMASK_POS                      0
#define OUTPUT_HTOTAL_15_8_FLDMASK_LEN                      8

#define ADDR_OUTPUT_HSTART_L (DPRX_BASE + SLAVE_ID_01 + 0xC158)
#define OUTPUT_HSTART_7_0_FLDMASK                           0xff
#define OUTPUT_HSTART_7_0_FLDMASK_POS                       0
#define OUTPUT_HSTART_7_0_FLDMASK_LEN                       8

#define ADDR_OUTPUT_HSTART_H (DPRX_BASE + SLAVE_ID_01 + 0xC15C)
#define OUTPUT_HSTART_15_8_FLDMASK                          0xff
#define OUTPUT_HSTART_15_8_FLDMASK_POS                      0
#define OUTPUT_HSTART_15_8_FLDMASK_LEN                      8

#define ADDR_OUTPUT_HSP_HSW_L (DPRX_BASE + SLAVE_ID_01 + 0xC160)
#define OUTPUT_HSP_HSW_7_0_FLDMASK                          0xff
#define OUTPUT_HSP_HSW_7_0_FLDMASK_POS                      0
#define OUTPUT_HSP_HSW_7_0_FLDMASK_LEN                      8

#define ADDR_OUTPUT_HSP_HSW_H (DPRX_BASE + SLAVE_ID_01 + 0xC164)
#define OUTPUT_HSP_FLDMASK                                  0x80
#define OUTPUT_HSP_FLDMASK_POS                              7
#define OUTPUT_HSP_FLDMASK_LEN                              1

#define OUTPUT_HSP_HSW_14_8_FLDMASK                         0x7f
#define OUTPUT_HSP_HSW_14_8_FLDMASK_POS                     0
#define OUTPUT_HSP_HSW_14_8_FLDMASK_LEN                     7

#define ADDR_HSTART_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x00)
#define HSTART_15_8_FLDMASK                                 0xff
#define HSTART_15_8_FLDMASK_POS                             0
#define HSTART_15_8_FLDMASK_LEN                             8

#define ADDR_HSTART_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x04)
#define HSTART_7_0_FLDMASK                                  0xff
#define HSTART_7_0_FLDMASK_POS                              0
#define HSTART_7_0_FLDMASK_LEN                              8

#define ADDR_HSW_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x08)
#define HSW_15_8_FLDMASK                                    0xff
#define HSW_15_8_FLDMASK_POS                                0
#define HSW_15_8_FLDMASK_LEN                                8

#define ADDR_HSW_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x0c)
#define HSW_7_0_FLDMASK                                     0xff
#define HSW_7_0_FLDMASK_POS                                 0
#define HSW_7_0_FLDMASK_LEN                                 8

#define ADDR_HTOTAL_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x010)
#define HTOTAL_15_8_FLDMASK                                 0xff
#define HTOTAL_15_8_FLDMASK_POS                             0
#define HTOTAL_15_8_FLDMASK_LEN                             8

#define ADDR_HTOTAL_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x014)
#define HTOTAL_7_0_FLDMASK                                  0xff
#define HTOTAL_7_0_FLDMASK_POS                              0
#define HTOTAL_7_0_FLDMASK_LEN                              8

#define ADDR_HWIDTH_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x018)
#define HWIDTH_15_8_FLDMASK                                 0xff
#define HWIDTH_15_8_FLDMASK_POS                             0
#define HWIDTH_15_8_FLDMASK_LEN                             8

#define ADDR_HWIDTH_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x01c)
#define HWIDTH_7_0_FLDMASK                                  0xff
#define HWIDTH_7_0_FLDMASK_POS                              0
#define HWIDTH_7_0_FLDMASK_LEN                              8

#define ADDR_VHEIGHT_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x020)
#define VHEIGHT_15_8_FLDMASK                                0xff
#define VHEIGHT_15_8_FLDMASK_POS                            0
#define VHEIGHT_15_8_FLDMASK_LEN                            8

#define ADDR_VHEIGHT_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x024)
#define VHEIGHT_7_0_FLDMASK                                 0xff
#define VHEIGHT_7_0_FLDMASK_POS                             0
#define VHEIGHT_7_0_FLDMASK_LEN                             8

#define ADDR_MAIN_VSTART_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x028)
#define MAIN_VSTART_15_8_FLDMASK                            0xff
#define MAIN_VSTART_15_8_FLDMASK_POS                        0
#define MAIN_VSTART_15_8_FLDMASK_LEN                        8

#define ADDR_MAIN_VSTART_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x02c)
#define MAIN_VSTART_7_0_FLDMASK                             0xff
#define MAIN_VSTART_7_0_FLDMASK_POS                         0
#define MAIN_VSTART_7_0_FLDMASK_LEN                         8

#define ADDR_VSW_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x030)
#define VSW_15_8_FLDMASK                                    0xff
#define VSW_15_8_FLDMASK_POS                                0
#define VSW_15_8_FLDMASK_LEN                                8

#define ADDR_VSW_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x034)
#define VSW_7_0_FLDMASK                                     0xff
#define VSW_7_0_FLDMASK_POS                                 0
#define VSW_7_0_FLDMASK_LEN                                 8

#define ADDR_VTOTAL_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x038)
#define VTOTAL_15_8_FLDMASK                                 0xff
#define VTOTAL_15_8_FLDMASK_POS                             0
#define VTOTAL_15_8_FLDMASK_LEN                             8

#define ADDR_VTOTAL_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x03c)
#define VTOTAL_7_0_FLDMASK                                  0xff
#define VTOTAL_7_0_FLDMASK_POS                              0
#define VTOTAL_7_0_FLDMASK_LEN                              8

#define ADDR_MISC0 (DPRX_BASE + SLAVE_ID_02 + 0x040)
#define MISC0_FLDMASK                                       0xff
#define MISC0_FLDMASK_POS                                   0
#define MISC0_FLDMASK_LEN                                   8

#define ADDR_MISC1 (DPRX_BASE + SLAVE_ID_02 + 0x044)
#define MISC1_FLDMASK                                       0xff
#define MISC1_FLDMASK_POS                                   0
#define MISC1_FLDMASK_LEN                                   8

#define ADDR_HSTART15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x048)
#define HSTART15_8_DBG_FLDMASK                              0xff
#define HSTART15_8_DBG_FLDMASK_POS                          0
#define HSTART15_8_DBG_FLDMASK_LEN                          8

#define ADDR_HSTART7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x04c)
#define HSTART7_0_DBG_FLDMASK                               0xff
#define HSTART7_0_DBG_FLDMASK_POS                           0
#define HSTART7_0_DBG_FLDMASK_LEN                           8

#define ADDR_HSW15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x050)
#define HSW15_8_DBG_FLDMASK                                 0xff
#define HSW15_8_DBG_FLDMASK_POS                             0
#define HSW15_8_DBG_FLDMASK_LEN                             8

#define ADDR_HSW7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x054)
#define HSW7_0_DBG_FLDMASK                                  0xff
#define HSW7_0_DBG_FLDMASK_POS                              0
#define HSW7_0_DBG_FLDMASK_LEN                              8

#define ADDR_HTOTAL15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x058)
#define HTOTAL15_8_DBG_FLDMASK                              0xff
#define HTOTAL15_8_DBG_FLDMASK_POS                          0
#define HTOTAL15_8_DBG_FLDMASK_LEN                          8

#define ADDR_HTOTAL7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x05c)
#define HTOTAL7_0_DBG_FLDMASK                               0xff
#define HTOTAL7_0_DBG_FLDMASK_POS                           0
#define HTOTAL7_0_DBG_FLDMASK_LEN                           8

#define ADDR_HWIDTH15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x060)
#define HWIDTH15_8_DBG_FLDMASK                              0xff
#define HWIDTH15_8_DBG_FLDMASK_POS                          0
#define HWIDTH15_8_DBG_FLDMASK_LEN                          8

#define ADDR_HWIDTH7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x064)
#define HWIDTH7_0_DBG_FLDMASK                               0xff
#define HWIDTH7_0_DBG_FLDMASK_POS                           0
#define HWIDTH7_0_DBG_FLDMASK_LEN                           8

#define ADDR_VHEIGHT15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x068)
#define VHEIGHT15_8_DBG_FLDMASK                             0xff
#define VHEIGHT15_8_DBG_FLDMASK_POS                         0
#define VHEIGHT15_8_DBG_FLDMASK_LEN                         8

#define ADDR_VHEIGHT7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x06c)
#define VHEIGHT7_0_DBG_FLDMASK                              0xff
#define VHEIGHT7_0_DBG_FLDMASK_POS                          0
#define VHEIGHT7_0_DBG_FLDMASK_LEN                          8

#define ADDR_VSTART15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x070)
#define VSTART15_8_DBG_FLDMASK                              0xff
#define VSTART15_8_DBG_FLDMASK_POS                          0
#define VSTART15_8_DBG_FLDMASK_LEN                          8

#define ADDR_VSTART7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x074)
#define VSTART7_0_DBG_FLDMASK                               0xff
#define VSTART7_0_DBG_FLDMASK_POS                           0
#define VSTART7_0_DBG_FLDMASK_LEN                           8

#define ADDR_VSW15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x078)
#define VSW15_8_DBG_FLDMASK                                 0xff
#define VSW15_8_DBG_FLDMASK_POS                             0
#define VSW15_8_DBG_FLDMASK_LEN                             8

#define ADDR_VSW7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x07c)
#define VSW7_0_DBG_FLDMASK                                  0xff
#define VSW7_0_DBG_FLDMASK_POS                              0
#define VSW7_0_DBG_FLDMASK_LEN                              8

#define ADDR_VTOTAL_15_8_DBG (DPRX_BASE + SLAVE_ID_02 + 0x080)
#define VTOTAL_15_8_DBG_FLDMASK                             0xff
#define VTOTAL_15_8_DBG_FLDMASK_POS                         0
#define VTOTAL_15_8_DBG_FLDMASK_LEN                         8

#define ADDR_VTOTAL_7_0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x084)
#define VTOTAL_7_0_DBG_FLDMASK                              0xff
#define VTOTAL_7_0_DBG_FLDMASK_POS                          0
#define VTOTAL_7_0_DBG_FLDMASK_LEN                          8

#define ADDR_MISC0_DBG (DPRX_BASE + SLAVE_ID_02 + 0x088)
#define MISC0_DBG_FLDMASK                                   0xff
#define MISC0_DBG_FLDMASK_POS                               0
#define MISC0_DBG_FLDMASK_LEN                               8

#define ADDR_MISC1_DBG (DPRX_BASE + SLAVE_ID_02 + 0x08c)
#define MISC1_DBG_FLDMASK                                   0xff
#define MISC1_DBG_FLDMASK_POS                               0
#define MISC1_DBG_FLDMASK_LEN                               8

#define ADDR_MAIN_ATTR_CTRL_1 (DPRX_BASE + SLAVE_ID_02 + 0x094)
#define FRAME_SAME_FLDMASK                                  0xe0
#define FRAME_SAME_FLDMASK_POS                              5
#define FRAME_SAME_FLDMASK_LEN                              3

#define ADDR_MAIN_LINK_CTRL (DPRX_BASE + SLAVE_ID_02 + 0x09c)
#define FORCE_MN_READY_FLDMASK                              0x80
#define FORCE_MN_READY_FLDMASK_POS                          7
#define FORCE_MN_READY_FLDMASK_LEN                          1

#define ADDR_M_FORCE_VALUE_3 (DPRX_BASE + SLAVE_ID_02 + 0x0a0)
#define M_FORCE_VALUE_3_FLDMASK                             0xff
#define M_FORCE_VALUE_3_FLDMASK_POS                         0
#define M_FORCE_VALUE_3_FLDMASK_LEN                         8

#define ADDR_M_FORCE_VALUE_2 (DPRX_BASE + SLAVE_ID_02 + 0x0a4)
#define M_FORCE_VALUE_2_FLDMASK                             0xff
#define M_FORCE_VALUE_2_FLDMASK_POS                         0
#define M_FORCE_VALUE_2_FLDMASK_LEN                         8

#define ADDR_M_FORCE_VALUE_1 (DPRX_BASE + SLAVE_ID_02 + 0x0a8)
#define M_FORCE_VALUE_1_FLDMASK                             0xff
#define M_FORCE_VALUE_1_FLDMASK_POS                         0
#define M_FORCE_VALUE_1_FLDMASK_LEN                         8

#define ADDR_N_FORCE_VALUE_3 (DPRX_BASE + SLAVE_ID_02 + 0x0ac)
#define N_FORCE_VALUE_3_FLDMASK                             0xff
#define N_FORCE_VALUE_3_FLDMASK_POS                         0
#define N_FORCE_VALUE_3_FLDMASK_LEN                         8

#define ADDR_N_FORCE_VALUE_2 (DPRX_BASE + SLAVE_ID_02 + 0x0b0)
#define N_FORCE_VALUE_2_FLDMASK                             0xff
#define N_FORCE_VALUE_2_FLDMASK_POS                         0
#define N_FORCE_VALUE_2_FLDMASK_LEN                         8

#define ADDR_N_FORCE_VALUE_1 (DPRX_BASE + SLAVE_ID_02 + 0x0b4)
#define N_FORCE_VALUE_1_FLDMASK                             0xff
#define N_FORCE_VALUE_1_FLDMASK_POS                         0
#define N_FORCE_VALUE_1_FLDMASK_LEN                         8

#define ADDR_LINK_LAYER_STATE_1 (DPRX_BASE + SLAVE_ID_02 + 0x108)
#define MAIN_DATA_OUT_EN_FLDMASK                            0x10
#define MAIN_DATA_OUT_EN_FLDMASK_POS                        4
#define MAIN_DATA_OUT_EN_FLDMASK_LEN                        1

#define ADDR_VID_M_RPT_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x114)
#define VID_M_RPT_23_16_FLDMASK                             0xff
#define VID_M_RPT_23_16_FLDMASK_POS                         0
#define VID_M_RPT_23_16_FLDMASK_LEN                         8

#define ADDR_VID_M_RPT_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x118)
#define VID_M_RPT_15_8_FLDMASK                              0xff
#define VID_M_RPT_15_8_FLDMASK_POS                          0
#define VID_M_RPT_15_8_FLDMASK_LEN                          8

#define ADDR_VID_M_RPT_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x11C)
#define VID_M_RPT_7_0_FLDMASK                               0xff
#define VID_M_RPT_7_0_FLDMASK_POS                           0
#define VID_M_RPT_7_0_FLDMASK_LEN                           8

#define ADDR_NVID_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x120)
#define NVID_23_16_FLDMASK                                  0xff
#define NVID_23_16_FLDMASK_POS                              0
#define NVID_23_16_FLDMASK_LEN                              8

#define ADDR_NVID_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x124)
#define NVID_15_8_FLDMASK                                   0xff
#define NVID_15_8_FLDMASK_POS                               0
#define NVID_15_8_FLDMASK_LEN                               8

#define ADDR_NVID_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x128)
#define NVID_7_0_FLDMASK                                    0xff
#define NVID_7_0_FLDMASK_POS                                0
#define NVID_7_0_FLDMASK_LEN                                8

#define ADDR_VB_ID_INFO (DPRX_BASE + SLAVE_ID_02 + 0x12C)
#define VB_ID_FINAL_FLDMASK                                 0xff
#define VB_ID_FINAL_FLDMASK_POS                             0
#define VB_ID_FINAL_FLDMASK_LEN                             8

#define ADDR_INFORFRAM_READ_EN (DPRX_BASE + SLAVE_ID_02 + 0x130)
#define AUDIO_CM_READ_EN_FLDMASK                            0x80
#define AUDIO_CM_READ_EN_FLDMASK_POS                        7
#define AUDIO_CM_READ_EN_FLDMASK_LEN                        1

#define ISRC_READ_EN_FLDMASK                                0x40
#define ISRC_READ_EN_FLDMASK_POS                            6
#define ISRC_READ_EN_FLDMASK_LEN                            1

#define HDR_INFO_READ_EN_FLDMASK                            0x20
#define HDR_INFO_READ_EN_FLDMASK_POS                        5
#define HDR_INFO_READ_EN_FLDMASK_LEN                        1

#define VSC_PKG_READ_EN_FLDMASK                             0x10
#define VSC_PKG_READ_EN_FLDMASK_POS                         4
#define VSC_PKG_READ_EN_FLDMASK_LEN                         1

#define AUDIO_INFO_READ_EN_FLDMASK                          0x8
#define AUDIO_INFO_READ_EN_FLDMASK_POS                      3
#define AUDIO_INFO_READ_EN_FLDMASK_LEN                      1

#define AVI_INFO_READ_EN_FLDMASK                            0x4
#define AVI_INFO_READ_EN_FLDMASK_POS                        2
#define AVI_INFO_READ_EN_FLDMASK_LEN                        1

#define SPD_INFO_READ_EN_FLDMASK                            0x2
#define SPD_INFO_READ_EN_FLDMASK_POS                        1
#define SPD_INFO_READ_EN_FLDMASK_LEN                        1

#define MPEG_INFO_READ_EN_FLDMASK                           0x1
#define MPEG_INFO_READ_EN_FLDMASK_POS                       0
#define MPEG_INFO_READ_EN_FLDMASK_LEN                       1

#define ADDR_MAIN_LINK_INTR0 (DPRX_BASE + SLAVE_ID_02 + 0x13C)
#define HDR_INFO_CHANGE_FLDMASK                             0x80
#define HDR_INFO_CHANGE_FLDMASK_POS                         7
#define HDR_INFO_CHANGE_FLDMASK_LEN                         1

#define M_N_AUD_IND_FLDMASK                                 0x40
#define M_N_AUD_IND_FLDMASK_POS                             6
#define M_N_AUD_IND_FLDMASK_LEN                             1

#define VSC_PKG_CHANGE_FLDMASK                              0x20
#define VSC_PKG_CHANGE_FLDMASK_POS                          5
#define VSC_PKG_CHANGE_FLDMASK_LEN                          1

#define SPD_INFO_CHANGE_FLDMASK                             0x10
#define SPD_INFO_CHANGE_FLDMASK_POS                         4
#define SPD_INFO_CHANGE_FLDMASK_LEN                         1

#define AVI_INFO_CHANGE_FLDMASK                             0x8
#define AVI_INFO_CHANGE_FLDMASK_POS                         3
#define AVI_INFO_CHANGE_FLDMASK_LEN                         1

#define MPEG_INFO_CHANGE_FLDMASK                            0x4
#define MPEG_INFO_CHANGE_FLDMASK_POS                        2
#define MPEG_INFO_CHANGE_FLDMASK_LEN                        1

#define OTHER_INFO_RECEIVED_INT_FLDMASK                     0x2
#define OTHER_INFO_RECEIVED_INT_FLDMASK_POS                 1
#define OTHER_INFO_RECEIVED_INT_FLDMASK_LEN                 1

#define AUDIO_INFO_CHANGE_FLDMASK                           0x1
#define AUDIO_INFO_CHANGE_FLDMASK_POS                       0
#define AUDIO_INFO_CHANGE_FLDMASK_LEN                       1

#define ADDR_MAIN_LINK_INTR1 (DPRX_BASE + SLAVE_ID_02 + 0x140)
#define VIDEO_MUTE_INTR_FLDMASK                             0x80
#define VIDEO_MUTE_INTR_FLDMASK_POS                         7
#define VIDEO_MUTE_INTR_FLDMASK_LEN                         1

#define MSA_UPDATE_INTR_FLDMASK                             0x40
#define MSA_UPDATE_INTR_FLDMASK_POS                         6
#define MSA_UPDATE_INTR_FLDMASK_LEN                         1

#define AUDIO_CH_COUNT_CHANGE_INT_FLDMASK                   0x20
#define AUDIO_CH_COUNT_CHANGE_INT_FLDMASK_POS               5
#define AUDIO_CH_COUNT_CHANGE_INT_FLDMASK_LEN               1

#define AUDIO_M_N_CHANGE_INT_FLDMASK                        0x10
#define AUDIO_M_N_CHANGE_INT_FLDMASK_POS                    4
#define AUDIO_M_N_CHANGE_INT_FLDMASK_LEN                    1

#define MAIN_LOST_FLAG_FLDMASK                              0x8
#define MAIN_LOST_FLAG_FLDMASK_POS                          3
#define MAIN_LOST_FLAG_FLDMASK_LEN                          1

#define MAIN_FIFO_OVFL_FLDMASK                              0x2
#define MAIN_FIFO_OVFL_FLDMASK_POS                          1
#define MAIN_FIFO_OVFL_FLDMASK_LEN                          1

#define MAIN_FIFO_UNFL_FLDMASK                              0x1
#define MAIN_FIFO_UNFL_FLDMASK_POS                          0
#define MAIN_FIFO_UNFL_FLDMASK_LEN                          1

#define ADDR_MAIN_LINK_INTR2 (DPRX_BASE + SLAVE_ID_02 + 0x144)
#define INVALID_BPP_INTR_FLDMASK                            0x80
#define INVALID_BPP_INTR_FLDMASK_POS                        7
#define INVALID_BPP_INTR_FLDMASK_LEN                        1

#define INVALID_COLOR_SPACE_INTR_FLDMASK                    0x40
#define INVALID_COLOR_SPACE_INTR_FLDMASK_POS                6
#define INVALID_COLOR_SPACE_INTR_FLDMASK_LEN                1

#define VSC_PKG_HB2_ERR_INTR_FLDMASK                        0x20
#define VSC_PKG_HB2_ERR_INTR_FLDMASK_POS                    5
#define VSC_PKG_HB2_ERR_INTR_FLDMASK_LEN                    1

#define VSC_PKG_HB3_ERR_INTR_FLDMASK                        0x10
#define VSC_PKG_HB3_ERR_INTR_FLDMASK_POS                    4
#define VSC_PKG_HB3_ERR_INTR_FLDMASK_LEN                    1

#define BE_CNT_ERR_IN_LINE_INTR_FLDMASK                     0x8
#define BE_CNT_ERR_IN_LINE_INTR_FLDMASK_POS                 3
#define BE_CNT_ERR_IN_LINE_INTR_FLDMASK_LEN                 1

#define BS_CNT_ERR_IN_LINE_INTR_FLDMASK                     0x4
#define BS_CNT_ERR_IN_LINE_INTR_FLDMASK_POS                 2
#define BS_CNT_ERR_IN_LINE_INTR_FLDMASK_LEN                 1

#define BE_CNT_ERR_IN_FRAME_INTR_FLDMASK                    0x2
#define BE_CNT_ERR_IN_FRAME_INTR_FLDMASK_POS                1
#define BE_CNT_ERR_IN_FRAME_INTR_FLDMASK_LEN                1

#define BS_CNT_ERR_IN_FRAME_INTR_FLDMASK                    0x1
#define BS_CNT_ERR_IN_FRAME_INTR_FLDMASK_POS                0
#define BS_CNT_ERR_IN_FRAME_INTR_FLDMASK_LEN                1

#define ADDR_MAIN_LINK_INTR3 (DPRX_BASE + SLAVE_ID_02 + 0x148)
#define COMPRESSED_FRAME_FL_INTR_FLDMASK                    0x20
#define COMPRESSED_FRAME_FL_INTR_FLDMASK_POS                5
#define COMPRESSED_FRAME_FL_INTR_FLDMASK_LEN                1

#define COMPRESSED_FRAME_RS_INTR_FLDMASK                    0x10
#define COMPRESSED_FRAME_RS_INTR_FLDMASK_POS                4
#define COMPRESSED_FRAME_RS_INTR_FLDMASK_LEN                1

#define PPS_CHANGE_FLDMASK                                  0x4
#define PPS_CHANGE_FLDMASK_POS                              2
#define PPS_CHANGE_FLDMASK_LEN                              1

#define AUDIO_CM_CHANGE_FLDMASK                             0x2
#define AUDIO_CM_CHANGE_FLDMASK_POS                         1
#define AUDIO_CM_CHANGE_FLDMASK_LEN                         1

#define ISRC_CHANGE_FLDMASK                                 0x1
#define ISRC_CHANGE_FLDMASK_POS                             0
#define ISRC_CHANGE_FLDMASK_LEN                             1

#define ADDR_MAIN_LINK_INTR0_MASK (DPRX_BASE + SLAVE_ID_02 + 0x14C)
#define MAIN_LINK_INTR0_MASK_FLDMASK                        0xff
#define MAIN_LINK_INTR0_MASK_FLDMASK_POS                    0
#define MAIN_LINK_INTR0_MASK_FLDMASK_LEN                    8

#define ADDR_MAIN_LINK_INTR1_MASK (DPRX_BASE + SLAVE_ID_02 + 0x150)
#define MAIN_LINK_INTR1_MASK_FLDMASK                        0xff
#define MAIN_LINK_INTR1_MASK_FLDMASK_POS                    0
#define MAIN_LINK_INTR1_MASK_FLDMASK_LEN                    8

#define ADDR_MAIN_LINK_INTR2_MASK (DPRX_BASE + SLAVE_ID_02 + 0x154)
#define MAIN_LINK_INTR2_MASK_FLDMASK                        0xff
#define MAIN_LINK_INTR2_MASK_FLDMASK_POS                    0
#define MAIN_LINK_INTR2_MASK_FLDMASK_LEN                    8

#define ADDR_MAIN_LINK_INTR3_MASK (DPRX_BASE + SLAVE_ID_02 + 0x158)
#define MAIN_LINK_INTR3_MASK_FLDMASK                        0xff
#define MAIN_LINK_INTR3_MASK_FLDMASK_POS                    0
#define MAIN_LINK_INTR3_MASK_FLDMASK_LEN                    8

#define ADDR_MAIN_LINK_STATUS_0 (DPRX_BASE + SLAVE_ID_02 + 0x16C)
#define VIDEO_MN_READY_INT_FLDMASK                          0x2
#define VIDEO_MN_READY_INT_FLDMASK_POS                      1
#define VIDEO_MN_READY_INT_FLDMASK_LEN                      1

#define VIDEO_UNMUTE_INT_FLDMASK                            0x1
#define VIDEO_UNMUTE_INT_FLDMASK_POS                        0
#define VIDEO_UNMUTE_INT_FLDMASK_LEN                        1

#define ADDR_INFORFRAM_ERR_0 (DPRX_BASE + SLAVE_ID_02 + 0x174)
#define HDR_INFO_ERR_FLDMASK                                0x80
#define HDR_INFO_ERR_FLDMASK_POS                            7
#define HDR_INFO_ERR_FLDMASK_LEN                            1

#define VSC_PKG_ERR_FLDMASK                                 0x40
#define VSC_PKG_ERR_FLDMASK_POS                             6
#define VSC_PKG_ERR_FLDMASK_LEN                             1

#define AVI_FAIL_FLAG_FLDMASK                               0x20
#define AVI_FAIL_FLAG_FLDMASK_POS                           5
#define AVI_FAIL_FLAG_FLDMASK_LEN                           1

#define AUDIO_FAIL_FALG_FLDMASK                             0x10
#define AUDIO_FAIL_FALG_FLDMASK_POS                         4
#define AUDIO_FAIL_FALG_FLDMASK_LEN                         1

#define AVI_INFO_ERR_FLDMASK                                0x8
#define AVI_INFO_ERR_FLDMASK_POS                            3
#define AVI_INFO_ERR_FLDMASK_LEN                            1

#define AUDIO_INFO_ERR_FLDMASK                              0x4
#define AUDIO_INFO_ERR_FLDMASK_POS                          2
#define AUDIO_INFO_ERR_FLDMASK_LEN                          1

#define SPD_INFO_ERR_FLDMASK                                0x2
#define SPD_INFO_ERR_FLDMASK_POS                            1
#define SPD_INFO_ERR_FLDMASK_LEN                            1

#define MPEG_INFO_ERR_FLDMASK                               0x1
#define MPEG_INFO_ERR_FLDMASK_POS                           0
#define MPEG_INFO_ERR_FLDMASK_LEN                           1

#define ADDR_INFORFRAM_ERR_1 (DPRX_BASE + SLAVE_ID_02 + 0x178)
#define PPS_ERR_FLDMASK                                     0x4
#define PPS_ERR_FLDMASK_POS                                 2
#define PPS_ERR_FLDMASK_LEN                                 1

#define AUDIO_CM_ERR_FLDMASK                                0x2
#define AUDIO_CM_ERR_FLDMASK_POS                            1
#define AUDIO_CM_ERR_FLDMASK_LEN                            1

#define ISRC_ERR_FLDMASK                                    0x1
#define ISRC_ERR_FLDMASK_POS                                0
#define ISRC_ERR_FLDMASK_LEN                                1

#define ADDR_MPEG_INFO_HB3 (DPRX_BASE + SLAVE_ID_02 + 0x1A0)
#define MPEG_INFO_HB3_FLDMASK                               0xff
#define MPEG_INFO_HB3_FLDMASK_POS                           0
#define MPEG_INFO_HB3_FLDMASK_LEN                           8

#define ADDR_SPD_INFO_HB3 (DPRX_BASE + SLAVE_ID_02 + 0x1A4)
#define SPD_INFO_HB3_FLDMASK                                0xff
#define SPD_INFO_HB3_FLDMASK_POS                            0
#define SPD_INFO_HB3_FLDMASK_LEN                            8

#define ADDR_AVI_INFO_HB3 (DPRX_BASE + SLAVE_ID_02 + 0x1A8)
#define AVI_INFO_HB3_FLDMASK                                0xff
#define AVI_INFO_HB3_FLDMASK_POS                            0
#define AVI_INFO_HB3_FLDMASK_LEN                            8

#define ADDR_HDR_INFO_HB3 (DPRX_BASE + SLAVE_ID_02 + 0x1AC)
#define HDR_INFO_HB3_FLDMASK                                0xff
#define HDR_INFO_HB3_FLDMASK_POS                            0
#define HDR_INFO_HB3_FLDMASK_LEN                            8

#define ADDR_MPEG_INFO_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x1B0)
#define MPEG_INFO_7_0_FLDMASK                               0xff
#define MPEG_INFO_7_0_FLDMASK_POS                           0
#define MPEG_INFO_7_0_FLDMASK_LEN                           8

#define ADDR_MPEG_INFO_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x1B4)
#define MPEG_INFO_15_8_FLDMASK                              0xff
#define MPEG_INFO_15_8_FLDMASK_POS                          0
#define MPEG_INFO_15_8_FLDMASK_LEN                          8

#define ADDR_MPEG_INFO_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x1B8)
#define MPEG_INFO_23_16_FLDMASK                             0xff
#define MPEG_INFO_23_16_FLDMASK_POS                         0
#define MPEG_INFO_23_16_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x1BC)
#define MPEG_INFO_31_24_FLDMASK                             0xff
#define MPEG_INFO_31_24_FLDMASK_POS                         0
#define MPEG_INFO_31_24_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x1C0)
#define MPEG_INFO_39_32_FLDMASK                             0xff
#define MPEG_INFO_39_32_FLDMASK_POS                         0
#define MPEG_INFO_39_32_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x1C4)
#define MPEG_INFO_47_40_FLDMASK                             0xff
#define MPEG_INFO_47_40_FLDMASK_POS                         0
#define MPEG_INFO_47_40_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x1C8)
#define MPEG_INFO_55_48_FLDMASK                             0xff
#define MPEG_INFO_55_48_FLDMASK_POS                         0
#define MPEG_INFO_55_48_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x1CC)
#define MPEG_INFO_63_56_FLDMASK                             0xff
#define MPEG_INFO_63_56_FLDMASK_POS                         0
#define MPEG_INFO_63_56_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x1D0)
#define MPEG_INFO_71_64_FLDMASK                             0xff
#define MPEG_INFO_71_64_FLDMASK_POS                         0
#define MPEG_INFO_71_64_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x1D4)
#define MPEG_INFO_79_72_FLDMASK                             0xff
#define MPEG_INFO_79_72_FLDMASK_POS                         0
#define MPEG_INFO_79_72_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_87_80 (DPRX_BASE + SLAVE_ID_02 + 0x1D8)
#define MPEG_INFO_87_80_FLDMASK                             0xff
#define MPEG_INFO_87_80_FLDMASK_POS                         0
#define MPEG_INFO_87_80_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x1DC)
#define MPEG_INFO_95_88_FLDMASK                             0xff
#define MPEG_INFO_95_88_FLDMASK_POS                         0
#define MPEG_INFO_95_88_FLDMASK_LEN                         8

#define ADDR_MPEG_INFO_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x1E0)
#define MPEG_INFO_103_96_FLDMASK                            0xff
#define MPEG_INFO_103_96_FLDMASK_POS                        0
#define MPEG_INFO_103_96_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x1F0)
#define SPD_INFO_7_0_FLDMASK                                0xff
#define SPD_INFO_7_0_FLDMASK_POS                            0
#define SPD_INFO_7_0_FLDMASK_LEN                            8

#define ADDR_SPD_INFO_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x1F4)
#define SPD_INFO_15_8_FLDMASK                               0xff
#define SPD_INFO_15_8_FLDMASK_POS                           0
#define SPD_INFO_15_8_FLDMASK_LEN                           8

#define ADDR_SPD_INFO_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x1F8)
#define SPD_INFO_23_16_FLDMASK                              0xff
#define SPD_INFO_23_16_FLDMASK_POS                          0
#define SPD_INFO_23_16_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x1FC)
#define SPD_INFO_31_24_FLDMASK                              0xff
#define SPD_INFO_31_24_FLDMASK_POS                          0
#define SPD_INFO_31_24_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x200)
#define SPD_INFO_39_32_FLDMASK                              0xff
#define SPD_INFO_39_32_FLDMASK_POS                          0
#define SPD_INFO_39_32_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x204)
#define SPD_INFO_47_40_FLDMASK                              0xff
#define SPD_INFO_47_40_FLDMASK_POS                          0
#define SPD_INFO_47_40_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x208)
#define SPD_INFO_55_48_FLDMASK                              0xff
#define SPD_INFO_55_48_FLDMASK_POS                          0
#define SPD_INFO_55_48_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x20C)
#define SPD_INFO_63_56_FLDMASK                              0xff
#define SPD_INFO_63_56_FLDMASK_POS                          0
#define SPD_INFO_63_56_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x210)
#define SPD_INFO_71_64_FLDMASK                              0xff
#define SPD_INFO_71_64_FLDMASK_POS                          0
#define SPD_INFO_71_64_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x214)
#define SPD_INFO_79_72_FLDMASK                              0xff
#define SPD_INFO_79_72_FLDMASK_POS                          0
#define SPD_INFO_79_72_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_87_80 (DPRX_BASE + SLAVE_ID_02 + 0x218)
#define SPD_INFO_87_80_FLDMASK                              0xff
#define SPD_INFO_87_80_FLDMASK_POS                          0
#define SPD_INFO_87_80_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x21C)
#define SPD_INFO_95_88_FLDMASK                              0xff
#define SPD_INFO_95_88_FLDMASK_POS                          0
#define SPD_INFO_95_88_FLDMASK_LEN                          8

#define ADDR_SPD_INFO_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x220)
#define SPD_INFO_103_96_FLDMASK                             0xff
#define SPD_INFO_103_96_FLDMASK_POS                         0
#define SPD_INFO_103_96_FLDMASK_LEN                         8

#define ADDR_SPD_INFO_111_104 (DPRX_BASE + SLAVE_ID_02 + 0x224)
#define SPD_INFO_111_104_FLDMASK                            0xff
#define SPD_INFO_111_104_FLDMASK_POS                        0
#define SPD_INFO_111_104_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_119_112 (DPRX_BASE + SLAVE_ID_02 + 0x228)
#define SPD_INFO_119_112_FLDMASK                            0xff
#define SPD_INFO_119_112_FLDMASK_POS                        0
#define SPD_INFO_119_112_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_127_120 (DPRX_BASE + SLAVE_ID_02 + 0x22C)
#define SPD_INFO_127_120_FLDMASK                            0xff
#define SPD_INFO_127_120_FLDMASK_POS                        0
#define SPD_INFO_127_120_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_135_128 (DPRX_BASE + SLAVE_ID_02 + 0x230)
#define SPD_INFO_135_128_FLDMASK                            0xff
#define SPD_INFO_135_128_FLDMASK_POS                        0
#define SPD_INFO_135_128_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_143_136 (DPRX_BASE + SLAVE_ID_02 + 0x234)
#define SPD_INFO_143_136_FLDMASK                            0xff
#define SPD_INFO_143_136_FLDMASK_POS                        0
#define SPD_INFO_143_136_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_151_144 (DPRX_BASE + SLAVE_ID_02 + 0x238)
#define SPD_INFO_151_144_FLDMASK                            0xff
#define SPD_INFO_151_144_FLDMASK_POS                        0
#define SPD_INFO_151_144_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_159_152 (DPRX_BASE + SLAVE_ID_02 + 0x23C)
#define SPD_INFO_159_152_FLDMASK                            0xff
#define SPD_INFO_159_152_FLDMASK_POS                        0
#define SPD_INFO_159_152_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_167_160 (DPRX_BASE + SLAVE_ID_02 + 0x240)
#define SPD_INFO_167_160_FLDMASK                            0xff
#define SPD_INFO_167_160_FLDMASK_POS                        0
#define SPD_INFO_167_160_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_175_168 (DPRX_BASE + SLAVE_ID_02 + 0x244)
#define SPD_INFO_175_168_FLDMASK                            0xff
#define SPD_INFO_175_168_FLDMASK_POS                        0
#define SPD_INFO_175_168_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_183_176 (DPRX_BASE + SLAVE_ID_02 + 0x248)
#define SPD_INFO_183_176_FLDMASK                            0xff
#define SPD_INFO_183_176_FLDMASK_POS                        0
#define SPD_INFO_183_176_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_191_184 (DPRX_BASE + SLAVE_ID_02 + 0x24C)
#define SPD_INFO_191_184_FLDMASK                            0xff
#define SPD_INFO_191_184_FLDMASK_POS                        0
#define SPD_INFO_191_184_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_199_192 (DPRX_BASE + SLAVE_ID_02 + 0x250)
#define SPD_INFO_199_192_FLDMASK                            0xff
#define SPD_INFO_199_192_FLDMASK_POS                        0
#define SPD_INFO_199_192_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_207_200 (DPRX_BASE + SLAVE_ID_02 + 0x254)
#define SPD_INFO_207_200_FLDMASK                            0xff
#define SPD_INFO_207_200_FLDMASK_POS                        0
#define SPD_INFO_207_200_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_215_208 (DPRX_BASE + SLAVE_ID_02 + 0x258)
#define SPD_INFO_215_208_FLDMASK                            0xff
#define SPD_INFO_215_208_FLDMASK_POS                        0
#define SPD_INFO_215_208_FLDMASK_LEN                        8

#define ADDR_SPD_INFO_223_216 (DPRX_BASE + SLAVE_ID_02 + 0x25C)
#define SPD_INFO_223_216_FLDMASK                            0xff
#define SPD_INFO_223_216_FLDMASK_POS                        0
#define SPD_INFO_223_216_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x260)
#define AUDIO_INFO_7_0_FLDMASK                              0xff
#define AUDIO_INFO_7_0_FLDMASK_POS                          0
#define AUDIO_INFO_7_0_FLDMASK_LEN                          8

#define ADDR_AUDIO_INFO_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x264)
#define AUDIO_INFO_15_8_FLDMASK                             0xff
#define AUDIO_INFO_15_8_FLDMASK_POS                         0
#define AUDIO_INFO_15_8_FLDMASK_LEN                         8

#define ADDR_AUDIO_INFO_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x268)
#define AUDIO_INFO_23_16_FLDMASK                            0xff
#define AUDIO_INFO_23_16_FLDMASK_POS                        0
#define AUDIO_INFO_23_16_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x26C)
#define AUDIO_INFO_31_24_FLDMASK                            0xff
#define AUDIO_INFO_31_24_FLDMASK_POS                        0
#define AUDIO_INFO_31_24_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x270)
#define AUDIO_INFO_39_32_FLDMASK                            0xff
#define AUDIO_INFO_39_32_FLDMASK_POS                        0
#define AUDIO_INFO_39_32_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x274)
#define AUDIO_INFO_47_40_FLDMASK                            0xff
#define AUDIO_INFO_47_40_FLDMASK_POS                        0
#define AUDIO_INFO_47_40_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x278)
#define AUDIO_INFO_55_48_FLDMASK                            0xff
#define AUDIO_INFO_55_48_FLDMASK_POS                        0
#define AUDIO_INFO_55_48_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x27C)
#define AUDIO_INFO_63_56_FLDMASK                            0xff
#define AUDIO_INFO_63_56_FLDMASK_POS                        0
#define AUDIO_INFO_63_56_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x280)
#define AUDIO_INFO_71_64_FLDMASK                            0xff
#define AUDIO_INFO_71_64_FLDMASK_POS                        0
#define AUDIO_INFO_71_64_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x284)
#define AUDIO_INFO_79_72_FLDMASK                            0xff
#define AUDIO_INFO_79_72_FLDMASK_POS                        0
#define AUDIO_INFO_79_72_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_87_88 (DPRX_BASE + SLAVE_ID_02 + 0x288)
#define AUDIO_INFO_87_88_FLDMASK                            0xff
#define AUDIO_INFO_87_88_FLDMASK_POS                        0
#define AUDIO_INFO_87_88_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x28C)
#define AUDIO_INFO_95_88_FLDMASK                            0xff
#define AUDIO_INFO_95_88_FLDMASK_POS                        0
#define AUDIO_INFO_95_88_FLDMASK_LEN                        8

#define ADDR_AUDIO_INFO_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x290)
#define AUDIO_INFO_103_96_FLDMASK                           0xff
#define AUDIO_INFO_103_96_FLDMASK_POS                       0
#define AUDIO_INFO_103_96_FLDMASK_LEN                       8

#define ADDR_AUDIO_INFO_111_104 (DPRX_BASE + SLAVE_ID_02 + 0x294)
#define AUDIO_INFO_111_104_FLDMASK                          0xff
#define AUDIO_INFO_111_104_FLDMASK_POS                      0
#define AUDIO_INFO_111_104_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_HB2 (DPRX_BASE + SLAVE_ID_02 + 0x2A0)
#define VSC_PKG_HB_REV_FLDMASK                              0xff
#define VSC_PKG_HB_REV_FLDMASK_POS                          0
#define VSC_PKG_HB_REV_FLDMASK_LEN                          8

#define ADDR_VSC_PKG_HB3 (DPRX_BASE + SLAVE_ID_02 + 0x2A4)
#define VSC_PKG_HB_BYTE_FLDMASK                             0xff
#define VSC_PKG_HB_BYTE_FLDMASK_POS                         0
#define VSC_PKG_HB_BYTE_FLDMASK_LEN                         8

#define ADDR_VSC_PKG_DATA_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x2A8)
#define VSC_PKG_DATA_7_0_FLDMASK                            0xff
#define VSC_PKG_DATA_7_0_FLDMASK_POS                        0
#define VSC_PKG_DATA_7_0_FLDMASK_LEN                        8

#define ADDR_VSC_PKG_DATA_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x2AC)
#define VSC_PKG_DATA_15_8_FLDMASK                           0xff
#define VSC_PKG_DATA_15_8_FLDMASK_POS                       0
#define VSC_PKG_DATA_15_8_FLDMASK_LEN                       8

#define ADDR_VSC_PKG_DATA_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x2B0)
#define VSC_PKG_DATA_23_16_FLDMASK                          0xff
#define VSC_PKG_DATA_23_16_FLDMASK_POS                      0
#define VSC_PKG_DATA_23_16_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x2B4)
#define VSC_PKG_DATA_31_24_FLDMASK                          0xff
#define VSC_PKG_DATA_31_24_FLDMASK_POS                      0
#define VSC_PKG_DATA_31_24_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x2B8)
#define VSC_PKG_DATA_39_32_FLDMASK                          0xff
#define VSC_PKG_DATA_39_32_FLDMASK_POS                      0
#define VSC_PKG_DATA_39_32_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x2BC)
#define VSC_PKG_DATA_47_40_FLDMASK                          0xff
#define VSC_PKG_DATA_47_40_FLDMASK_POS                      0
#define VSC_PKG_DATA_47_40_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x2C0)
#define VSC_PKG_DATA_55_48_FLDMASK                          0xff
#define VSC_PKG_DATA_55_48_FLDMASK_POS                      0
#define VSC_PKG_DATA_55_48_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x2C4)
#define VSC_PKG_DATA_63_56_FLDMASK                          0xff
#define VSC_PKG_DATA_63_56_FLDMASK_POS                      0
#define VSC_PKG_DATA_63_56_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x2C8)
#define VSC_PKG_DATA_71_64_FLDMASK                          0xff
#define VSC_PKG_DATA_71_64_FLDMASK_POS                      0
#define VSC_PKG_DATA_71_64_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x2CC)
#define VSC_PKG_DATA_79_72_FLDMASK                          0xff
#define VSC_PKG_DATA_79_72_FLDMASK_POS                      0
#define VSC_PKG_DATA_79_72_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_87_80 (DPRX_BASE + SLAVE_ID_02 + 0x2D0)
#define VSC_PKG_DATA_87_80_FLDMASK                          0xff
#define VSC_PKG_DATA_87_80_FLDMASK_POS                      0
#define VSC_PKG_DATA_87_80_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x2D4)
#define VSC_PKG_DATA_95_88_FLDMASK                          0xff
#define VSC_PKG_DATA_95_88_FLDMASK_POS                      0
#define VSC_PKG_DATA_95_88_FLDMASK_LEN                      8

#define ADDR_VSC_PKG_DATA_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x2D8)
#define VSC_PKG_DATA_103_96_FLDMASK                         0xff
#define VSC_PKG_DATA_103_96_FLDMASK_POS                     0
#define VSC_PKG_DATA_103_96_FLDMASK_LEN                     8

#define ADDR_VSC_PKG_DATA_111_104 (DPRX_BASE + SLAVE_ID_02 + 0x2DC)
#define VSC_PKG_DATA_111_104_FLDMASK                        0xff
#define VSC_PKG_DATA_111_104_FLDMASK_POS                    0
#define VSC_PKG_DATA_111_104_FLDMASK_LEN                    8

#define ADDR_VSC_PKG_DATA_119_112 (DPRX_BASE + SLAVE_ID_02 + 0x2E0)
#define VSC_PKG_DATA_119_112_FLDMASK                        0xff
#define VSC_PKG_DATA_119_112_FLDMASK_POS                    0
#define VSC_PKG_DATA_119_112_FLDMASK_LEN                    8

#define ADDR_VSC_PKG_DATA_127_120 (DPRX_BASE + SLAVE_ID_02 + 0x2E4)
#define VSC_PKG_DATA_127_120_FLDMASK                        0xff
#define VSC_PKG_DATA_127_120_FLDMASK_POS                    0
#define VSC_PKG_DATA_127_120_FLDMASK_LEN                    8

#define ADDR_VSC_PKG_DATA_135_128 (DPRX_BASE + SLAVE_ID_02 + 0x2E8)
#define VSC_PKG_DATA_135_128_FLDMASK                        0xff
#define VSC_PKG_DATA_135_128_FLDMASK_POS                    0
#define VSC_PKG_DATA_135_128_FLDMASK_LEN                    8

#define ADDR_VSC_PKG_DATA_143_136 (DPRX_BASE + SLAVE_ID_02 + 0x2EC)
#define VSC_PKG_DATA_143_136_FLDMASK                        0xff
#define VSC_PKG_DATA_143_136_FLDMASK_POS                    0
#define VSC_PKG_DATA_143_136_FLDMASK_LEN                    8

#define ADDR_VSC_PKG_DATA_151_144 (DPRX_BASE + SLAVE_ID_02 + 0x2F0)
#define VSC_PKG_DATA_151_144_FLDMASK                        0xff
#define VSC_PKG_DATA_151_144_FLDMASK_POS                    0
#define VSC_PKG_DATA_151_144_FLDMASK_LEN                    8

#define ADDR_VSC_PKG_DATA_159_152 (DPRX_BASE + SLAVE_ID_02 + 0x2F4)
#define VSC_PKG_DATA_159_152_FLDMASK                        0xff
#define VSC_PKG_DATA_159_152_FLDMASK_POS                    0
#define VSC_PKG_DATA_159_152_FLDMASK_LEN                    8

#define ADDR_HDR_INFO_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x304)
#define HDR_INFO_7_0_FLDMASK                                0xff
#define HDR_INFO_7_0_FLDMASK_POS                            0
#define HDR_INFO_7_0_FLDMASK_LEN                            8

#define ADDR_HDR_INFO_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x308)
#define HDR_INFO_15_8_FLDMASK                               0xff
#define HDR_INFO_15_8_FLDMASK_POS                           0
#define HDR_INFO_15_8_FLDMASK_LEN                           8

#define ADDR_HDR_INFO_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x30C)
#define HDR_INFO_23_16_FLDMASK                              0xff
#define HDR_INFO_23_16_FLDMASK_POS                          0
#define HDR_INFO_23_16_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x310)
#define HDR_INFO_31_24_FLDMASK                              0xff
#define HDR_INFO_31_24_FLDMASK_POS                          0
#define HDR_INFO_31_24_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x314)
#define HDR_INFO_39_32_FLDMASK                              0xff
#define HDR_INFO_39_32_FLDMASK_POS                          0
#define HDR_INFO_39_32_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x318)
#define HDR_INFO_47_40_FLDMASK                              0xff
#define HDR_INFO_47_40_FLDMASK_POS                          0
#define HDR_INFO_47_40_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x31C)
#define HDR_INFO_55_48_FLDMASK                              0xff
#define HDR_INFO_55_48_FLDMASK_POS                          0
#define HDR_INFO_55_48_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x320)
#define HDR_INFO_63_56_FLDMASK                              0xff
#define HDR_INFO_63_56_FLDMASK_POS                          0
#define HDR_INFO_63_56_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x324)
#define HDR_INFO_71_64_FLDMASK                              0xff
#define HDR_INFO_71_64_FLDMASK_POS                          0
#define HDR_INFO_71_64_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x328)
#define HDR_INFO_79_72_FLDMASK                              0xff
#define HDR_INFO_79_72_FLDMASK_POS                          0
#define HDR_INFO_79_72_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_87_80 (DPRX_BASE + SLAVE_ID_02 + 0x32C)
#define HDR_INFO_87_80_FLDMASK                              0xff
#define HDR_INFO_87_80_FLDMASK_POS                          0
#define HDR_INFO_87_80_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x330)
#define HDR_INFO_95_88_FLDMASK                              0xff
#define HDR_INFO_95_88_FLDMASK_POS                          0
#define HDR_INFO_95_88_FLDMASK_LEN                          8

#define ADDR_HDR_INFO_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x334)
#define HDR_INFO_103_96_FLDMASK                             0xff
#define HDR_INFO_103_96_FLDMASK_POS                         0
#define HDR_INFO_103_96_FLDMASK_LEN                         8

#define ADDR_HDR_INFO_111_104 (DPRX_BASE + SLAVE_ID_02 + 0x338)
#define HDR_INFO_111_104_FLDMASK                            0xff
#define HDR_INFO_111_104_FLDMASK_POS                        0
#define HDR_INFO_111_104_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_119_112 (DPRX_BASE + SLAVE_ID_02 + 0x33C)
#define HDR_INFO_119_112_FLDMASK                            0xff
#define HDR_INFO_119_112_FLDMASK_POS                        0
#define HDR_INFO_119_112_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_127_120 (DPRX_BASE + SLAVE_ID_02 + 0x340)
#define HDR_INFO_127_120_FLDMASK                            0xff
#define HDR_INFO_127_120_FLDMASK_POS                        0
#define HDR_INFO_127_120_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_135_128 (DPRX_BASE + SLAVE_ID_02 + 0x344)
#define HDR_INFO_135_128_FLDMASK                            0xff
#define HDR_INFO_135_128_FLDMASK_POS                        0
#define HDR_INFO_135_128_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_143_136 (DPRX_BASE + SLAVE_ID_02 + 0x348)
#define HDR_INFO_143_136_FLDMASK                            0xff
#define HDR_INFO_143_136_FLDMASK_POS                        0
#define HDR_INFO_143_136_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_151_144 (DPRX_BASE + SLAVE_ID_02 + 0x34C)
#define HDR_INFO_151_144_FLDMASK                            0xff
#define HDR_INFO_151_144_FLDMASK_POS                        0
#define HDR_INFO_151_144_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_159_152 (DPRX_BASE + SLAVE_ID_02 + 0x350)
#define HDR_INFO_159_152_FLDMASK                            0xff
#define HDR_INFO_159_152_FLDMASK_POS                        0
#define HDR_INFO_159_152_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_167_160 (DPRX_BASE + SLAVE_ID_02 + 0x354)
#define HDR_INFO_167_160_FLDMASK                            0xff
#define HDR_INFO_167_160_FLDMASK_POS                        0
#define HDR_INFO_167_160_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_175_168 (DPRX_BASE + SLAVE_ID_02 + 0x358)
#define HDR_INFO_175_168_FLDMASK                            0xff
#define HDR_INFO_175_168_FLDMASK_POS                        0
#define HDR_INFO_175_168_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_183_176 (DPRX_BASE + SLAVE_ID_02 + 0x35C)
#define HDR_INFO_183_176_FLDMASK                            0xff
#define HDR_INFO_183_176_FLDMASK_POS                        0
#define HDR_INFO_183_176_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_191_184 (DPRX_BASE + SLAVE_ID_02 + 0x360)
#define HDR_INFO_191_184_FLDMASK                            0xff
#define HDR_INFO_191_184_FLDMASK_POS                        0
#define HDR_INFO_191_184_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_199_192 (DPRX_BASE + SLAVE_ID_02 + 0x364)
#define HDR_INFO_199_192_FLDMASK                            0xff
#define HDR_INFO_199_192_FLDMASK_POS                        0
#define HDR_INFO_199_192_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_207_200 (DPRX_BASE + SLAVE_ID_02 + 0x368)
#define HDR_INFO_207_200_FLDMASK                            0xff
#define HDR_INFO_207_200_FLDMASK_POS                        0
#define HDR_INFO_207_200_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_215_208 (DPRX_BASE + SLAVE_ID_02 + 0x36C)
#define HDR_INFO_215_208_FLDMASK                            0xff
#define HDR_INFO_215_208_FLDMASK_POS                        0
#define HDR_INFO_215_208_FLDMASK_LEN                        8

#define ADDR_HDR_INFO_223_216 (DPRX_BASE + SLAVE_ID_02 + 0x370)
#define HDR_INFO_223_216_FLDMASK                            0xff
#define HDR_INFO_223_216_FLDMASK_POS                        0
#define HDR_INFO_223_216_FLDMASK_LEN                        8

#define ADDR_AUDIO_CM_HB3 (DPRX_BASE + SLAVE_ID_02 + 0x3A4)
#define AUDIO_CM_HB3_FLDMASK                                0xff
#define AUDIO_CM_HB3_FLDMASK_POS                            0
#define AUDIO_CM_HB3_FLDMASK_LEN                            8

#define ADDR_AUDIO_CM_DATA_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x3A8)
#define AUDIO_CM_DATA_7_0_FLDMASK                           0xff
#define AUDIO_CM_DATA_7_0_FLDMASK_POS                       0
#define AUDIO_CM_DATA_7_0_FLDMASK_LEN                       8

#define ADDR_AUDIO_CM_DATA_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x3AC)
#define AUDIO_CM_DATA_15_8_FLDMASK                          0xff
#define AUDIO_CM_DATA_15_8_FLDMASK_POS                      0
#define AUDIO_CM_DATA_15_8_FLDMASK_LEN                      8

#define ADDR_AUDIO_CM_DATA_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x3B0)
#define AUDIO_CM_DATA_23_16_FLDMASK                         0xff
#define AUDIO_CM_DATA_23_16_FLDMASK_POS                     0
#define AUDIO_CM_DATA_23_16_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x3B4)
#define AUDIO_CM_DATA_31_24_FLDMASK                         0xff
#define AUDIO_CM_DATA_31_24_FLDMASK_POS                     0
#define AUDIO_CM_DATA_31_24_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x3B8)
#define AUDIO_CM_DATA_39_32_FLDMASK                         0xff
#define AUDIO_CM_DATA_39_32_FLDMASK_POS                     0
#define AUDIO_CM_DATA_39_32_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x3BC)
#define AUDIO_CM_DATA_47_40_FLDMASK                         0xff
#define AUDIO_CM_DATA_47_40_FLDMASK_POS                     0
#define AUDIO_CM_DATA_47_40_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x3C0)
#define AUDIO_CM_DATA_55_48_FLDMASK                         0xff
#define AUDIO_CM_DATA_55_48_FLDMASK_POS                     0
#define AUDIO_CM_DATA_55_48_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x3C4)
#define AUDIO_CM_DATA_63_56_FLDMASK                         0xff
#define AUDIO_CM_DATA_63_56_FLDMASK_POS                     0
#define AUDIO_CM_DATA_63_56_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x3C8)
#define AUDIO_CM_DATA_71_64_FLDMASK                         0xff
#define AUDIO_CM_DATA_71_64_FLDMASK_POS                     0
#define AUDIO_CM_DATA_71_64_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x3CC)
#define AUDIO_CM_DATA_79_72_FLDMASK                         0xff
#define AUDIO_CM_DATA_79_72_FLDMASK_POS                     0
#define AUDIO_CM_DATA_79_72_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_87_80 (DPRX_BASE + SLAVE_ID_02 + 0x3D0)
#define AUDIO_CM_DATA_87_80_FLDMASK                         0xff
#define AUDIO_CM_DATA_87_80_FLDMASK_POS                     0
#define AUDIO_CM_DATA_87_80_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x3D4)
#define AUDIO_CM_DATA_95_88_FLDMASK                         0xff
#define AUDIO_CM_DATA_95_88_FLDMASK_POS                     0
#define AUDIO_CM_DATA_95_88_FLDMASK_LEN                     8

#define ADDR_AUDIO_CM_DATA_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x3D8)
#define AUDIO_CM_DATA_103_96_FLDMASK                        0xff
#define AUDIO_CM_DATA_103_96_FLDMASK_POS                    0
#define AUDIO_CM_DATA_103_96_FLDMASK_LEN                    8

#define ADDR_AUDIO_CM_DATA_111_104 (DPRX_BASE + SLAVE_ID_02 + 0x3DC)
#define AUDIO_CM_DATA_111_104_FLDMASK                       0xff
#define AUDIO_CM_DATA_111_104_FLDMASK_POS                   0
#define AUDIO_CM_DATA_111_104_FLDMASK_LEN                   8

#define ADDR_AUDIO_CM_DATA_119_112 (DPRX_BASE + SLAVE_ID_02 + 0x3E0)
#define AUDIO_CM_DATA_119_112_FLDMASK                       0xff
#define AUDIO_CM_DATA_119_112_FLDMASK_POS                   0
#define AUDIO_CM_DATA_119_112_FLDMASK_LEN                   8

#define ADDR_AUDIO_CM_DATA_127_120 (DPRX_BASE + SLAVE_ID_02 + 0x3E4)
#define AUDIO_CM_DATA_127_120_FLDMASK                       0xff
#define AUDIO_CM_DATA_127_120_FLDMASK_POS                   0
#define AUDIO_CM_DATA_127_120_FLDMASK_LEN                   8

#define ADDR_ISRC_HB3 (DPRX_BASE + SLAVE_ID_02 + 0x3F4)
#define ISRC_HB3_FLDMASK                                    0xff
#define ISRC_HB3_FLDMASK_POS                                0
#define ISRC_HB3_FLDMASK_LEN                                8

#define ADDR_ISRC_DATA_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x3F8)
#define ISRC_DATA_7_0_FLDMASK                               0xff
#define ISRC_DATA_7_0_FLDMASK_POS                           0
#define ISRC_DATA_7_0_FLDMASK_LEN                           8

#define ADDR_ISRC_DATA_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x3FC)
#define ISRC_DATA_15_8_FLDMASK                              0xff
#define ISRC_DATA_15_8_FLDMASK_POS                          0
#define ISRC_DATA_15_8_FLDMASK_LEN                          8

#define ADDR_ISRC_DATA_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x400)
#define ISRC_DATA_23_16_FLDMASK                             0xff
#define ISRC_DATA_23_16_FLDMASK_POS                         0
#define ISRC_DATA_23_16_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x404)
#define ISRC_DATA_31_24_FLDMASK                             0xff
#define ISRC_DATA_31_24_FLDMASK_POS                         0
#define ISRC_DATA_31_24_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x408)
#define ISRC_DATA_39_32_FLDMASK                             0xff
#define ISRC_DATA_39_32_FLDMASK_POS                         0
#define ISRC_DATA_39_32_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x40C)
#define ISRC_DATA_47_40_FLDMASK                             0xff
#define ISRC_DATA_47_40_FLDMASK_POS                         0
#define ISRC_DATA_47_40_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x410)
#define ISRC_DATA_55_48_FLDMASK                             0xff
#define ISRC_DATA_55_48_FLDMASK_POS                         0
#define ISRC_DATA_55_48_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x414)
#define ISRC_DATA_63_56_FLDMASK                             0xff
#define ISRC_DATA_63_56_FLDMASK_POS                         0
#define ISRC_DATA_63_56_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x418)
#define ISRC_DATA_71_64_FLDMASK                             0xff
#define ISRC_DATA_71_64_FLDMASK_POS                         0
#define ISRC_DATA_71_64_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x41C)
#define ISRC_DATA_79_72_FLDMASK                             0xff
#define ISRC_DATA_79_72_FLDMASK_POS                         0
#define ISRC_DATA_79_72_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_87_80 (DPRX_BASE + SLAVE_ID_02 + 0x420)
#define ISRC_DATA_87_80_FLDMASK                             0xff
#define ISRC_DATA_87_80_FLDMASK_POS                         0
#define ISRC_DATA_87_80_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x424)
#define ISRC_DATA_95_88_FLDMASK                             0xff
#define ISRC_DATA_95_88_FLDMASK_POS                         0
#define ISRC_DATA_95_88_FLDMASK_LEN                         8

#define ADDR_ISRC_DATA_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x428)
#define ISRC_DATA_103_96_FLDMASK                            0xff
#define ISRC_DATA_103_96_FLDMASK_POS                        0
#define ISRC_DATA_103_96_FLDMASK_LEN                        8

#define ADDR_ISRC_DATA_111_104 (DPRX_BASE + SLAVE_ID_02 + 0x42C)
#define ISRC_DATA_111_104_FLDMASK                           0xff
#define ISRC_DATA_111_104_FLDMASK_POS                       0
#define ISRC_DATA_111_104_FLDMASK_LEN                       8

#define ADDR_ISRC_DATA_119_112 (DPRX_BASE + SLAVE_ID_02 + 0x430)
#define ISRC_DATA_119_112_FLDMASK                           0xff
#define ISRC_DATA_119_112_FLDMASK_POS                       0
#define ISRC_DATA_119_112_FLDMASK_LEN                       8

#define ADDR_ISRC_DATA_127_120 (DPRX_BASE + SLAVE_ID_02 + 0x434)
#define ISRC_DATA_127_120_FLDMASK                           0xff
#define ISRC_DATA_127_120_FLDMASK_POS                       0
#define ISRC_DATA_127_120_FLDMASK_LEN                       8

#define ADDR_AVI_INFO_7_0 (DPRX_BASE + SLAVE_ID_02 + 0x440)
#define AVI_INFO_7_0_FLDMASK                                0xff
#define AVI_INFO_7_0_FLDMASK_POS                            0
#define AVI_INFO_7_0_FLDMASK_LEN                            8

#define ADDR_AVI_INFO_15_8 (DPRX_BASE + SLAVE_ID_02 + 0x444)
#define AVI_INFO_15_8_FLDMASK                               0xff
#define AVI_INFO_15_8_FLDMASK_POS                           0
#define AVI_INFO_15_8_FLDMASK_LEN                           8

#define ADDR_AVI_INFO_23_16 (DPRX_BASE + SLAVE_ID_02 + 0x448)
#define AVI_INFO_23_16_FLDMASK                              0xff
#define AVI_INFO_23_16_FLDMASK_POS                          0
#define AVI_INFO_23_16_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_31_24 (DPRX_BASE + SLAVE_ID_02 + 0x44C)
#define AVI_INFO_31_24_FLDMASK                              0xff
#define AVI_INFO_31_24_FLDMASK_POS                          0
#define AVI_INFO_31_24_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_39_32 (DPRX_BASE + SLAVE_ID_02 + 0x450)
#define AVI_INFO_39_32_FLDMASK                              0xff
#define AVI_INFO_39_32_FLDMASK_POS                          0
#define AVI_INFO_39_32_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_47_40 (DPRX_BASE + SLAVE_ID_02 + 0x454)
#define AVI_INFO_47_40_FLDMASK                              0xff
#define AVI_INFO_47_40_FLDMASK_POS                          0
#define AVI_INFO_47_40_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_55_48 (DPRX_BASE + SLAVE_ID_02 + 0x458)
#define AVI_INFO_55_48_FLDMASK                              0xff
#define AVI_INFO_55_48_FLDMASK_POS                          0
#define AVI_INFO_55_48_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_63_56 (DPRX_BASE + SLAVE_ID_02 + 0x45C)
#define AVI_INFO_63_56_FLDMASK                              0xff
#define AVI_INFO_63_56_FLDMASK_POS                          0
#define AVI_INFO_63_56_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_71_64 (DPRX_BASE + SLAVE_ID_02 + 0x460)
#define AVI_INFO_71_64_FLDMASK                              0xff
#define AVI_INFO_71_64_FLDMASK_POS                          0
#define AVI_INFO_71_64_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_79_72 (DPRX_BASE + SLAVE_ID_02 + 0x464)
#define AVI_INFO_79_72_FLDMASK                              0xff
#define AVI_INFO_79_72_FLDMASK_POS                          0
#define AVI_INFO_79_72_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_87_80 (DPRX_BASE + SLAVE_ID_02 + 0x468)
#define AVI_INFO_87_80_FLDMASK                              0xff
#define AVI_INFO_87_80_FLDMASK_POS                          0
#define AVI_INFO_87_80_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_95_88 (DPRX_BASE + SLAVE_ID_02 + 0x46C)
#define AVI_INFO_95_88_FLDMASK                              0xff
#define AVI_INFO_95_88_FLDMASK_POS                          0
#define AVI_INFO_95_88_FLDMASK_LEN                          8

#define ADDR_AVI_INFO_103_96 (DPRX_BASE + SLAVE_ID_02 + 0x470)
#define AVI_INFO_103_96_FLDMASK                             0xff
#define AVI_INFO_103_96_FLDMASK_POS                         0
#define AVI_INFO_103_96_FLDMASK_LEN                         8

#define ADDR_AVI_INFO_111_104 (DPRX_BASE + SLAVE_ID_02 + 0x474)
#define AVI_INFO_111_104_FLDMASK                            0xff
#define AVI_INFO_111_104_FLDMASK_POS                        0
#define AVI_INFO_111_104_FLDMASK_LEN                        8

#define ADDR_AVI_INFO_119_112 (DPRX_BASE + SLAVE_ID_02 + 0x478)
#define AVI_INFO_119_112_FLDMASK                            0xff
#define AVI_INFO_119_112_FLDMASK_POS                        0
#define AVI_INFO_119_112_FLDMASK_LEN                        8

#define ADDR_AVI_INFO_127_120 (DPRX_BASE + SLAVE_ID_02 + 0x47C)
#define AVI_INFO_127_120_FLDMASK                            0xff
#define AVI_INFO_127_120_FLDMASK_POS                        0
#define AVI_INFO_127_120_FLDMASK_LEN                        8

#define ADDR_PPS_PKG_DATA_BYTE_0 (DPRX_BASE + SLAVE_ID_02 + 0x484)
#define PPS_PKG_DATA_BYTE_0_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_0_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_0_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_1 (DPRX_BASE + SLAVE_ID_02 + 0x488)
#define PPS_PKG_DATA_BYTE_1_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_1_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_1_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_2 (DPRX_BASE + SLAVE_ID_02 + 0x48c)
#define PPS_PKG_DATA_BYTE_2_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_2_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_2_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_3 (DPRX_BASE + SLAVE_ID_02 + 0x490)
#define PPS_PKG_DATA_BYTE_3_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_3_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_3_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_4 (DPRX_BASE + SLAVE_ID_02 + 0x494)
#define PPS_PKG_DATA_BYTE_4_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_4_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_4_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_5 (DPRX_BASE + SLAVE_ID_02 + 0x498)
#define PPS_PKG_DATA_BYTE_5_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_5_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_5_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_6 (DPRX_BASE + SLAVE_ID_02 + 0x49c)
#define PPS_PKG_DATA_BYTE_6_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_6_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_6_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_7 (DPRX_BASE + SLAVE_ID_02 + 0x4a0)
#define PPS_PKG_DATA_BYTE_7_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_7_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_7_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_8 (DPRX_BASE + SLAVE_ID_02 + 0x4a4)
#define PPS_PKG_DATA_BYTE_8_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_8_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_8_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_9 (DPRX_BASE + SLAVE_ID_02 + 0x4a8)
#define PPS_PKG_DATA_BYTE_9_FLDMASK                         0xff
#define PPS_PKG_DATA_BYTE_9_FLDMASK_POS                     0
#define PPS_PKG_DATA_BYTE_9_FLDMASK_LEN                     8

#define ADDR_PPS_PKG_DATA_BYTE_10 (DPRX_BASE + SLAVE_ID_02 + 0x4ac)
#define PPS_PKG_DATA_BYTE_10_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_10_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_10_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_11 (DPRX_BASE + SLAVE_ID_02 + 0x4b0)
#define PPS_PKG_DATA_BYTE_11_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_11_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_11_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_12 (DPRX_BASE + SLAVE_ID_02 + 0x4b4)
#define PPS_PKG_DATA_BYTE_12_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_12_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_12_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_13 (DPRX_BASE + SLAVE_ID_02 + 0x4b8)
#define PPS_PKG_DATA_BYTE_13_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_13_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_13_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_14 (DPRX_BASE + SLAVE_ID_02 + 0x4bc)
#define PPS_PKG_DATA_BYTE_14_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_14_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_14_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_15 (DPRX_BASE + SLAVE_ID_02 + 0x4c0)
#define PPS_PKG_DATA_BYTE_15_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_15_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_15_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_16 (DPRX_BASE + SLAVE_ID_02 + 0x4c4)
#define PPS_PKG_DATA_BYTE_16_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_16_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_16_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_17 (DPRX_BASE + SLAVE_ID_02 + 0x4c8)
#define PPS_PKG_DATA_BYTE_17_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_17_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_17_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_18 (DPRX_BASE + SLAVE_ID_02 + 0x4cc)
#define PPS_PKG_DATA_BYTE_18_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_18_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_18_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_19 (DPRX_BASE + SLAVE_ID_02 + 0x4d0)
#define PPS_PKG_DATA_BYTE_19_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_19_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_19_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_20 (DPRX_BASE + SLAVE_ID_02 + 0x4d4)
#define PPS_PKG_DATA_BYTE_20_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_20_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_20_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_21 (DPRX_BASE + SLAVE_ID_02 + 0x4d8)
#define PPS_PKG_DATA_BYTE_21_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_21_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_21_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_22 (DPRX_BASE + SLAVE_ID_02 + 0x4dc)
#define PPS_PKG_DATA_BYTE_22_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_22_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_22_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_23 (DPRX_BASE + SLAVE_ID_02 + 0x4e0)
#define PPS_PKG_DATA_BYTE_23_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_23_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_23_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_24 (DPRX_BASE + SLAVE_ID_02 + 0x4e4)
#define PPS_PKG_DATA_BYTE_24_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_24_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_24_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_25 (DPRX_BASE + SLAVE_ID_02 + 0x4e8)
#define PPS_PKG_DATA_BYTE_25_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_25_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_25_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_26 (DPRX_BASE + SLAVE_ID_02 + 0x4ec)
#define PPS_PKG_DATA_BYTE_26_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_26_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_26_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_27 (DPRX_BASE + SLAVE_ID_02 + 0x4f0)
#define PPS_PKG_DATA_BYTE_27_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_27_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_27_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_28 (DPRX_BASE + SLAVE_ID_02 + 0x4f4)
#define PPS_PKG_DATA_BYTE_28_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_28_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_28_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_29 (DPRX_BASE + SLAVE_ID_02 + 0x4f8)
#define PPS_PKG_DATA_BYTE_29_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_29_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_29_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_30 (DPRX_BASE + SLAVE_ID_02 + 0x4fc)
#define PPS_PKG_DATA_BYTE_30_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_30_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_30_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_31 (DPRX_BASE + SLAVE_ID_02 + 0x500)
#define PPS_PKG_DATA_BYTE_31_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_31_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_31_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_32 (DPRX_BASE + SLAVE_ID_02 + 0x504)
#define PPS_PKG_DATA_BYTE_32_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_32_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_32_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_33 (DPRX_BASE + SLAVE_ID_02 + 0x508)
#define PPS_PKG_DATA_BYTE_33_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_33_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_33_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_34 (DPRX_BASE + SLAVE_ID_02 + 0x50c)
#define PPS_PKG_DATA_BYTE_34_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_34_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_34_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_35 (DPRX_BASE + SLAVE_ID_02 + 0x510)
#define PPS_PKG_DATA_BYTE_35_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_35_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_35_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_36 (DPRX_BASE + SLAVE_ID_02 + 0x514)
#define PPS_PKG_DATA_BYTE_36_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_36_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_36_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_37 (DPRX_BASE + SLAVE_ID_02 + 0x518)
#define PPS_PKG_DATA_BYTE_37_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_37_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_37_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_38 (DPRX_BASE + SLAVE_ID_02 + 0x51c)
#define PPS_PKG_DATA_BYTE_38_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_38_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_38_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_39 (DPRX_BASE + SLAVE_ID_02 + 0x520)
#define PPS_PKG_DATA_BYTE_39_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_39_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_39_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_40 (DPRX_BASE + SLAVE_ID_02 + 0x524)
#define PPS_PKG_DATA_BYTE_40_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_40_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_40_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_41 (DPRX_BASE + SLAVE_ID_02 + 0x528)
#define PPS_PKG_DATA_BYTE_41_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_41_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_41_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_42 (DPRX_BASE + SLAVE_ID_02 + 0x52c)
#define PPS_PKG_DATA_BYTE_42_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_42_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_42_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_43 (DPRX_BASE + SLAVE_ID_02 + 0x530)
#define PPS_PKG_DATA_BYTE_43_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_43_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_43_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_44 (DPRX_BASE + SLAVE_ID_02 + 0x534)
#define PPS_PKG_DATA_BYTE_44_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_44_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_44_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_45 (DPRX_BASE + SLAVE_ID_02 + 0x538)
#define PPS_PKG_DATA_BYTE_45_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_45_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_45_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_46 (DPRX_BASE + SLAVE_ID_02 + 0x53c)
#define PPS_PKG_DATA_BYTE_46_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_46_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_46_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_47 (DPRX_BASE + SLAVE_ID_02 + 0x540)
#define PPS_PKG_DATA_BYTE_47_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_47_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_47_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_48 (DPRX_BASE + SLAVE_ID_02 + 0x544)
#define PPS_PKG_DATA_BYTE_48_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_48_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_48_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_49 (DPRX_BASE + SLAVE_ID_02 + 0x548)
#define PPS_PKG_DATA_BYTE_49_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_49_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_49_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_50 (DPRX_BASE + SLAVE_ID_02 + 0x54c)
#define PPS_PKG_DATA_BYTE_50_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_50_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_50_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_51 (DPRX_BASE + SLAVE_ID_02 + 0x550)
#define PPS_PKG_DATA_BYTE_51_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_51_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_51_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_52 (DPRX_BASE + SLAVE_ID_02 + 0x554)
#define PPS_PKG_DATA_BYTE_52_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_52_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_52_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_53 (DPRX_BASE + SLAVE_ID_02 + 0x558)
#define PPS_PKG_DATA_BYTE_53_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_53_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_53_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_54 (DPRX_BASE + SLAVE_ID_02 + 0x55c)
#define PPS_PKG_DATA_BYTE_54_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_54_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_54_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_55 (DPRX_BASE + SLAVE_ID_02 + 0x560)
#define PPS_PKG_DATA_BYTE_55_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_55_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_55_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_56 (DPRX_BASE + SLAVE_ID_02 + 0x564)
#define PPS_PKG_DATA_BYTE_56_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_56_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_56_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_57 (DPRX_BASE + SLAVE_ID_02 + 0x568)
#define PPS_PKG_DATA_BYTE_57_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_57_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_57_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_58 (DPRX_BASE + SLAVE_ID_02 + 0x56c)
#define PPS_PKG_DATA_BYTE_58_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_58_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_58_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_59 (DPRX_BASE + SLAVE_ID_02 + 0x570)
#define PPS_PKG_DATA_BYTE_59_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_59_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_59_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_60 (DPRX_BASE + SLAVE_ID_02 + 0x574)
#define PPS_PKG_DATA_BYTE_60_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_60_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_60_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_61 (DPRX_BASE + SLAVE_ID_02 + 0x578)
#define PPS_PKG_DATA_BYTE_61_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_61_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_61_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_62 (DPRX_BASE + SLAVE_ID_02 + 0x57c)
#define PPS_PKG_DATA_BYTE_62_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_62_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_62_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_63 (DPRX_BASE + SLAVE_ID_02 + 0x580)
#define PPS_PKG_DATA_BYTE_63_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_63_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_63_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_64 (DPRX_BASE + SLAVE_ID_02 + 0x584)
#define PPS_PKG_DATA_BYTE_64_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_64_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_64_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_65 (DPRX_BASE + SLAVE_ID_02 + 0x588)
#define PPS_PKG_DATA_BYTE_65_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_65_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_65_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_66 (DPRX_BASE + SLAVE_ID_02 + 0x58c)
#define PPS_PKG_DATA_BYTE_66_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_66_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_66_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_67 (DPRX_BASE + SLAVE_ID_02 + 0x590)
#define PPS_PKG_DATA_BYTE_67_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_67_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_67_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_68 (DPRX_BASE + SLAVE_ID_02 + 0x594)
#define PPS_PKG_DATA_BYTE_68_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_68_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_68_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_69 (DPRX_BASE + SLAVE_ID_02 + 0x598)
#define PPS_PKG_DATA_BYTE_69_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_69_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_69_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_70 (DPRX_BASE + SLAVE_ID_02 + 0x59c)
#define PPS_PKG_DATA_BYTE_70_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_70_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_70_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_71 (DPRX_BASE + SLAVE_ID_02 + 0x5a0)
#define PPS_PKG_DATA_BYTE_71_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_71_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_71_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_72 (DPRX_BASE + SLAVE_ID_02 + 0x5a4)
#define PPS_PKG_DATA_BYTE_72_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_72_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_72_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_73 (DPRX_BASE + SLAVE_ID_02 + 0x5a8)
#define PPS_PKG_DATA_BYTE_73_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_73_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_73_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_74 (DPRX_BASE + SLAVE_ID_02 + 0x5ac)
#define PPS_PKG_DATA_BYTE_74_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_74_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_74_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_75 (DPRX_BASE + SLAVE_ID_02 + 0x5b0)
#define PPS_PKG_DATA_BYTE_75_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_75_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_75_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_76 (DPRX_BASE + SLAVE_ID_02 + 0x5b4)
#define PPS_PKG_DATA_BYTE_76_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_76_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_76_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_77 (DPRX_BASE + SLAVE_ID_02 + 0x5b8)
#define PPS_PKG_DATA_BYTE_77_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_77_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_77_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_78 (DPRX_BASE + SLAVE_ID_02 + 0x5bc)
#define PPS_PKG_DATA_BYTE_78_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_78_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_78_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_79 (DPRX_BASE + SLAVE_ID_02 + 0x5c0)
#define PPS_PKG_DATA_BYTE_79_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_79_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_79_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_80 (DPRX_BASE + SLAVE_ID_02 + 0x5c4)
#define PPS_PKG_DATA_BYTE_80_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_80_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_80_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_81 (DPRX_BASE + SLAVE_ID_02 + 0x5c8)
#define PPS_PKG_DATA_BYTE_81_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_81_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_81_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_82 (DPRX_BASE + SLAVE_ID_02 + 0x5cc)
#define PPS_PKG_DATA_BYTE_82_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_82_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_82_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_83 (DPRX_BASE + SLAVE_ID_02 + 0x5d0)
#define PPS_PKG_DATA_BYTE_83_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_83_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_83_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_84 (DPRX_BASE + SLAVE_ID_02 + 0x5d4)
#define PPS_PKG_DATA_BYTE_84_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_84_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_84_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_85 (DPRX_BASE + SLAVE_ID_02 + 0x5d8)
#define PPS_PKG_DATA_BYTE_85_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_85_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_85_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_86 (DPRX_BASE + SLAVE_ID_02 + 0x5dc)
#define PPS_PKG_DATA_BYTE_86_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_86_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_86_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_87 (DPRX_BASE + SLAVE_ID_02 + 0x5e0)
#define PPS_PKG_DATA_BYTE_87_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_87_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_87_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_88 (DPRX_BASE + SLAVE_ID_02 + 0x5e4)
#define PPS_PKG_DATA_BYTE_88_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_88_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_88_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_89 (DPRX_BASE + SLAVE_ID_02 + 0x5e8)
#define PPS_PKG_DATA_BYTE_89_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_89_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_89_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_90 (DPRX_BASE + SLAVE_ID_02 + 0x5ec)
#define PPS_PKG_DATA_BYTE_90_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_90_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_90_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_91 (DPRX_BASE + SLAVE_ID_02 + 0x5f0)
#define PPS_PKG_DATA_BYTE_91_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_91_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_91_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_92 (DPRX_BASE + SLAVE_ID_02 + 0x5f4)
#define PPS_PKG_DATA_BYTE_92_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_92_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_92_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_93 (DPRX_BASE + SLAVE_ID_02 + 0x5f8)
#define PPS_PKG_DATA_BYTE_93_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_93_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_93_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_94 (DPRX_BASE + SLAVE_ID_02 + 0x5fc)
#define PPS_PKG_DATA_BYTE_94_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_94_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_94_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_95 (DPRX_BASE + SLAVE_ID_02 + 0x600)
#define PPS_PKG_DATA_BYTE_95_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_95_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_95_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_96 (DPRX_BASE + SLAVE_ID_02 + 0x604)
#define PPS_PKG_DATA_BYTE_96_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_96_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_96_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_97 (DPRX_BASE + SLAVE_ID_02 + 0x608)
#define PPS_PKG_DATA_BYTE_97_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_97_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_97_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_98 (DPRX_BASE + SLAVE_ID_02 + 0x60c)
#define PPS_PKG_DATA_BYTE_98_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_98_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_98_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_99 (DPRX_BASE + SLAVE_ID_02 + 0x610)
#define PPS_PKG_DATA_BYTE_99_FLDMASK                        0xff
#define PPS_PKG_DATA_BYTE_99_FLDMASK_POS                    0
#define PPS_PKG_DATA_BYTE_99_FLDMASK_LEN                    8

#define ADDR_PPS_PKG_DATA_BYTE_100 (DPRX_BASE + SLAVE_ID_02 + 0x614)
#define PPS_PKG_DATA_BYTE_100_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_100_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_100_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_101 (DPRX_BASE + SLAVE_ID_02 + 0x618)
#define PPS_PKG_DATA_BYTE_101_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_101_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_101_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_102 (DPRX_BASE + SLAVE_ID_02 + 0x61c)
#define PPS_PKG_DATA_BYTE_102_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_102_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_102_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_103 (DPRX_BASE + SLAVE_ID_02 + 0x620)
#define PPS_PKG_DATA_BYTE_103_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_103_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_103_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_104 (DPRX_BASE + SLAVE_ID_02 + 0x624)
#define PPS_PKG_DATA_BYTE_104_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_104_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_104_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_105 (DPRX_BASE + SLAVE_ID_02 + 0x628)
#define PPS_PKG_DATA_BYTE_105_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_105_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_105_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_106 (DPRX_BASE + SLAVE_ID_02 + 0x62c)
#define PPS_PKG_DATA_BYTE_106_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_106_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_106_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_107 (DPRX_BASE + SLAVE_ID_02 + 0x630)
#define PPS_PKG_DATA_BYTE_107_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_107_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_107_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_108 (DPRX_BASE + SLAVE_ID_02 + 0x634)
#define PPS_PKG_DATA_BYTE_108_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_108_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_108_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_109 (DPRX_BASE + SLAVE_ID_02 + 0x638)
#define PPS_PKG_DATA_BYTE_109_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_109_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_109_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_110 (DPRX_BASE + SLAVE_ID_02 + 0x63c)
#define PPS_PKG_DATA_BYTE_110_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_110_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_110_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_111 (DPRX_BASE + SLAVE_ID_02 + 0x640)
#define PPS_PKG_DATA_BYTE_111_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_111_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_111_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_112 (DPRX_BASE + SLAVE_ID_02 + 0x644)
#define PPS_PKG_DATA_BYTE_112_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_112_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_112_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_113 (DPRX_BASE + SLAVE_ID_02 + 0x648)
#define PPS_PKG_DATA_BYTE_113_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_113_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_113_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_114 (DPRX_BASE + SLAVE_ID_02 + 0x64c)
#define PPS_PKG_DATA_BYTE_114_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_114_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_114_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_115 (DPRX_BASE + SLAVE_ID_02 + 0x650)
#define PPS_PKG_DATA_BYTE_115_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_115_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_115_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_116 (DPRX_BASE + SLAVE_ID_02 + 0x654)
#define PPS_PKG_DATA_BYTE_116_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_116_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_116_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_117 (DPRX_BASE + SLAVE_ID_02 + 0x658)
#define PPS_PKG_DATA_BYTE_117_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_117_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_117_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_118 (DPRX_BASE + SLAVE_ID_02 + 0x65c)
#define PPS_PKG_DATA_BYTE_118_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_118_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_118_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_119 (DPRX_BASE + SLAVE_ID_02 + 0x660)
#define PPS_PKG_DATA_BYTE_119_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_119_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_119_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_120 (DPRX_BASE + SLAVE_ID_02 + 0x664)
#define PPS_PKG_DATA_BYTE_120_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_120_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_120_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_121 (DPRX_BASE + SLAVE_ID_02 + 0x668)
#define PPS_PKG_DATA_BYTE_121_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_121_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_121_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_122 (DPRX_BASE + SLAVE_ID_02 + 0x66c)
#define PPS_PKG_DATA_BYTE_122_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_122_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_122_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_123 (DPRX_BASE + SLAVE_ID_02 + 0x670)
#define PPS_PKG_DATA_BYTE_123_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_123_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_123_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_124 (DPRX_BASE + SLAVE_ID_02 + 0x674)
#define PPS_PKG_DATA_BYTE_124_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_124_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_124_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_125 (DPRX_BASE + SLAVE_ID_02 + 0x678)
#define PPS_PKG_DATA_BYTE_125_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_125_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_125_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_126 (DPRX_BASE + SLAVE_ID_02 + 0x67c)
#define PPS_PKG_DATA_BYTE_126_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_126_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_126_FLDMASK_LEN                   8

#define ADDR_PPS_PKG_DATA_BYTE_127 (DPRX_BASE + SLAVE_ID_02 + 0x680)
#define PPS_PKG_DATA_BYTE_127_FLDMASK                       0xff
#define PPS_PKG_DATA_BYTE_127_FLDMASK_POS                   0
#define PPS_PKG_DATA_BYTE_127_FLDMASK_LEN                   8

#define ADDR_MAIN_LINK_INTR1_MASK_VID (DPRX_BASE + SLAVE_ID_02 + 0x690)
#define MAIN_LINK_INTR1_MASK_VID_FLDMASK                    0xff
#define MAIN_LINK_INTR1_MASK_VID_FLDMASK_POS                0
#define MAIN_LINK_INTR1_MASK_VID_FLDMASK_LEN                8

#define ADDR_SYSTEM_STATUS_0 (DPRX_BASE + SLAVE_ID_03 + 0x034)
#define ALIGN_STATUS_FLDMASK                                0x10
#define ALIGN_STATUS_FLDMASK_POS                            4
#define ALIGN_STATUS_FLDMASK_LEN                            1

#define LANE_SYNC_STATUS_3_FLDMASK                          0x8
#define LANE_SYNC_STATUS_3_FLDMASK_POS                      3
#define LANE_SYNC_STATUS_3_FLDMASK_LEN                      1

#define LANE_SYNC_STATUS_2_FLDMASK                          0x4
#define LANE_SYNC_STATUS_2_FLDMASK_POS                      2
#define LANE_SYNC_STATUS_2_FLDMASK_LEN                      1

#define LANE_SYNC_STATUS_1_FLDMASK                          0x2
#define LANE_SYNC_STATUS_1_FLDMASK_POS                      1
#define LANE_SYNC_STATUS_1_FLDMASK_LEN                      1

#define LANE_SYNC_STATUS_0_FLDMASK                          0x1
#define LANE_SYNC_STATUS_0_FLDMASK_POS                      0
#define LANE_SYNC_STATUS_0_FLDMASK_LEN                      1

#define ADDR_SYSTEM_STATUS_1 (DPRX_BASE + SLAVE_ID_03 + 0x038)
#define AUX_CMD_RCV_FLDMASK                                 0x2
#define AUX_CMD_RCV_FLDMASK_POS                             1
#define AUX_CMD_RCV_FLDMASK_LEN                             1

#define AUX_CMD_REPLY_FLDMASK                               0x1
#define AUX_CMD_REPLY_FLDMASK_POS                           0
#define AUX_CMD_REPLY_FLDMASK_LEN                           1

#define ADDR_RC_TRAINING_RESULT (DPRX_BASE + SLAVE_ID_03 + 0x058)
#define RC_TRAINING_DONE_3_FLDMASK                          0x8
#define RC_TRAINING_DONE_3_FLDMASK_POS                      3
#define RC_TRAINING_DONE_3_FLDMASK_LEN                      1

#define RC_TRAINING_DONE_2_FLDMASK                          0x4
#define RC_TRAINING_DONE_2_FLDMASK_POS                      2
#define RC_TRAINING_DONE_2_FLDMASK_LEN                      1

#define RC_TRAINING_DONE_1_FLDMASK                          0x2
#define RC_TRAINING_DONE_1_FLDMASK_POS                      1
#define RC_TRAINING_DONE_1_FLDMASK_LEN                      1

#define RC_TRAINING_DONE_0_FLDMASK                          0x1
#define RC_TRAINING_DONE_0_FLDMASK_POS                      0
#define RC_TRAINING_DONE_0_FLDMASK_LEN                      1

#define ADDR_FEC_CTRL (DPRX_BASE + SLAVE_ID_03 + 0x070)
#define FEC_PH_STOP_CIPHER_EN_FLDMASK                       0x20
#define FEC_PH_STOP_CIPHER_EN_FLDMASK_POS                   5
#define FEC_PH_STOP_CIPHER_EN_FLDMASK_LEN                   1

#define FEC_PM_STOP_CIPHER_EN_FLDMASK                       0x10
#define FEC_PM_STOP_CIPHER_EN_FLDMASK_POS                   4
#define FEC_PM_STOP_CIPHER_EN_FLDMASK_LEN                   1

#define FEC_SEQ_STOP_CIPHER_EN_FLDMASK                      0x8
#define FEC_SEQ_STOP_CIPHER_EN_FLDMASK_POS                  3
#define FEC_SEQ_STOP_CIPHER_EN_FLDMASK_LEN                  1

#define ADDR_AUX_CH_SET (DPRX_BASE + SLAVE_ID_03 + 0x110)
#define PRE_CHARGE_SET_FLDMASK                              0x3
#define PRE_CHARGE_SET_FLDMASK_POS                          0
#define PRE_CHARGE_SET_FLDMASK_LEN                          2

#define ADDR_HDCP2_CTRL (DPRX_BASE + SLAVE_ID_03 + 0x140)
#define HDCP_VERSION_FLDMASK                                0x40
#define HDCP_VERSION_FLDMASK_POS                            6
#define HDCP_VERSION_FLDMASK_LEN                            1

#define HDCP2_FW_EN_FLDMASK                                 0x20
#define HDCP2_FW_EN_FLDMASK_POS                             5
#define HDCP2_FW_EN_FLDMASK_LEN                             1

#define BAUD_DIV_RATIO_FLDMASK                              0x1f
#define BAUD_DIV_RATIO_FLDMASK_POS                          0
#define BAUD_DIV_RATIO_FLDMASK_LEN                          5

#define ADDR_HDCP2_DEBUG_0 (DPRX_BASE + SLAVE_ID_03 + 0x144)
#define HDCP2_DEBUG_0_FLDMASK                               0xff
#define HDCP2_DEBUG_0_FLDMASK_POS                           0
#define HDCP2_DEBUG_0_FLDMASK_LEN                           8

#define ADDR_HDCP2_STATUS (DPRX_BASE + SLAVE_ID_03 + 0x154)
#define ENCRYPT_STATUS_FLDMASK                              0x2
#define ENCRYPT_STATUS_FLDMASK_POS                          1
#define ENCRYPT_STATUS_FLDMASK_LEN                          1

#define HDCP_AUTH_DONE_FLDMASK                              0x1
#define HDCP_AUTH_DONE_FLDMASK_POS                          0
#define HDCP_AUTH_DONE_FLDMASK_LEN                          1

#define ADDR_FLOATING_DPCD_ZONE_0_L (DPRX_BASE + SLAVE_ID_03 + 0x180)
#define FLOATING_DPCD_0_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_0_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_0_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_0_H (DPRX_BASE + SLAVE_ID_03 + 0x184)
#define FLOATING_DPCD_0_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_0_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_0_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_1_L (DPRX_BASE + SLAVE_ID_03 + 0x188)
#define FLOATING_DPCD_1_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_1_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_1_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_1_H (DPRX_BASE + SLAVE_ID_03 + 0x18c)
#define FLOATING_DPCD_1_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_1_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_1_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_2_L (DPRX_BASE + SLAVE_ID_03 + 0x190)
#define FLOATING_DPCD_2_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_2_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_2_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_2_H (DPRX_BASE + SLAVE_ID_03 + 0x194)
#define FLOATING_DPCD_2_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_2_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_2_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_3_L (DPRX_BASE + SLAVE_ID_03 + 0x198)
#define FLOATING_DPCD_3_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_3_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_3_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_3_H (DPRX_BASE + SLAVE_ID_03 + 0x19c)
#define FLOATING_DPCD_3_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_3_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_3_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_4_L (DPRX_BASE + SLAVE_ID_03 + 0x1a0)
#define FLOATING_DPCD_4_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_4_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_4_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_4_H (DPRX_BASE + SLAVE_ID_03 + 0x1a4)
#define FLOATING_DPCD_4_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_4_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_4_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_5_L (DPRX_BASE + SLAVE_ID_03 + 0x1a8)
#define FLOATING_DPCD_5_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_5_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_5_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_5_H (DPRX_BASE + SLAVE_ID_03 + 0x1ac)
#define FLOATING_DPCD_5_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_5_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_5_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_6_L (DPRX_BASE + SLAVE_ID_03 + 0x1b0)
#define FLOATING_DPCD_6_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_6_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_6_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_6_H (DPRX_BASE + SLAVE_ID_03 + 0x1b4)
#define FLOATING_DPCD_6_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_6_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_6_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_7_L (DPRX_BASE + SLAVE_ID_03 + 0x1b8)
#define FLOATING_DPCD_7_7_0_FLDMASK                         0xff
#define FLOATING_DPCD_7_7_0_FLDMASK_POS                     0
#define FLOATING_DPCD_7_7_0_FLDMASK_LEN                     8

#define ADDR_FLOATING_DPCD_ZONE_7_H (DPRX_BASE + SLAVE_ID_03 + 0x1bc)
#define FLOATING_DPCD_7_14_8_FLDMASK                        0x7f
#define FLOATING_DPCD_7_14_8_FLDMASK_POS                    0
#define FLOATING_DPCD_7_14_8_FLDMASK_LEN                    7

#define ADDR_FLOATING_DPCD_ZONE_0_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1c0)
#define FLOATING_DPCD_0_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_0_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_0_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_1_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1c4)
#define FLOATING_DPCD_1_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_1_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_1_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_2_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1c8)
#define FLOATING_DPCD_2_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_2_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_2_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_3_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1cc)
#define FLOATING_DPCD_3_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_3_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_3_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_4_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1d0)
#define FLOATING_DPCD_4_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_4_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_4_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_5_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1d4)
#define FLOATING_DPCD_5_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_5_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_5_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_6_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1d8)
#define FLOATING_DPCD_6_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_6_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_6_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_7_LSB_0 (DPRX_BASE + SLAVE_ID_03 + 0x1dc)
#define FLOATING_DPCD_7_LSB_0_FLDMASK                       0x1f
#define FLOATING_DPCD_7_LSB_0_FLDMASK_POS                   0
#define FLOATING_DPCD_7_LSB_0_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_0_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1e0)
#define FLOATING_DPCD_0_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_0_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_0_LSB_1_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_1_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1e4)
#define FLOATING_DPCD_1_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_1_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_1_LSB_1_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_2_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1e8)
#define FLOATING_DPCD_2_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_2_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_2_LSB_1_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_3_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1ec)
#define FLOATING_DPCD_3_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_3_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_3_LSB_1_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_4_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1f0)
#define FLOATING_DPCD_4_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_4_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_4_LSB_1_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_5_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1f4)
#define FLOATING_DPCD_5_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_5_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_5_LSB_1_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_6_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1f8)
#define FLOATING_DPCD_6_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_6_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_6_LSB_1_FLDMASK_LEN                   5

#define ADDR_FLOATING_DPCD_ZONE_7_LSB_1 (DPRX_BASE + SLAVE_ID_03 + 0x1fc)
#define FLOATING_DPCD_7_LSB_1_FLDMASK                       0x1f
#define FLOATING_DPCD_7_LSB_1_FLDMASK_POS                   0
#define FLOATING_DPCD_7_LSB_1_FLDMASK_LEN                   5

#define ADDR_NEW_MISC_CTRL (DPRX_BASE + SLAVE_ID_03 + 0x200)
#define AUX_INTER_NACK_SEL_FLDMASK                          0x2
#define AUX_INTER_NACK_SEL_FLDMASK_POS                      1
#define AUX_INTER_NACK_SEL_FLDMASK_LEN                      1

#define ADDR_HPD_IRQ_TIMER (DPRX_BASE + SLAVE_ID_03 + 0x244)
#define HPD_PULSE_TIMER_FLDMASK                             0xff
#define HPD_PULSE_TIMER_FLDMASK_POS                         0
#define HPD_PULSE_TIMER_FLDMASK_LEN                         8

#define ADDR_HPD_IRQ_PROC_TIMER (DPRX_BASE + SLAVE_ID_03 + 0x248)
#define HPD_PROCESS_TIMER_FLDMASK                           0xff
#define HPD_PROCESS_TIMER_FLDMASK_POS                       0
#define HPD_PROCESS_TIMER_FLDMASK_LEN                       8

#define ADDR_HPD_IRQ_MASK_H (DPRX_BASE + SLAVE_ID_03 + 0x24c)
#define R0_AVAILABLE_BEGIN_INTR_MASK_FLDMASK               0x80
#define R0_AVAILABLE_BEGIN_INTR_MASK_FLDMASK_POS           7
#define R0_AVAILABLE_BEGIN_INTR_MASK_FLDMASK_LEN           1

#define LINK_STATUS_CHG_INTR_MASK_FLDMASK                   0x40
#define LINK_STATUS_CHG_INTR_MASK_FLDMASK_POS               6
#define LINK_STATUS_CHG_INTR_MASK_FLDMASK_LEN               1

#define LINK_STATUS_UPDATE_BEGIN_INTR_MASK_FLDMASK          0x20
#define LINK_STATUS_UPDATE_BEGIN_INTR_MASK_FLDMASK_POS      5
#define LINK_STATUS_UPDATE_BEGIN_INTR_MASK_FLDMASK_LEN      1

#define LANE_ALIGN_DONE_CHG_INTR_MASK_FLDMASK               0x10
#define LANE_ALIGN_DONE_CHG_INTR_MASK_FLDMASK_POS           4
#define LANE_ALIGN_DONE_CHG_INTR_MASK_FLDMASK_LEN           1

#define LANE3_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK           0x8
#define LANE3_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_POS       3
#define LANE3_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_LEN       1

#define LANE2_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK           0x4
#define LANE2_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_POS       2
#define LANE2_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_LEN       1

#define LANE1_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK           0x2
#define LANE1_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_POS       1
#define LANE1_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_LEN       1

#define LANE0_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK           0x1
#define LANE0_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_POS       0
#define LANE0_SYMBOL_LOCKED_CHG_INTR_MASK_FLDMASK_LEN       1

#define ADDR_HPD_IRQ_MASK_L (DPRX_BASE + SLAVE_ID_03 + 0x250)
#define LANE3_EQ_DONE_CHG_INTR_MASK_FLDMASK                 0x80
#define LANE3_EQ_DONE_CHG_INTR_MASK_FLDMASK_POS             7
#define LANE3_EQ_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define LANE2_EQ_DONE_CHG_INTR_MASK_FLDMASK                 0x40
#define LANE2_EQ_DONE_CHG_INTR_MASK_FLDMASK_POS             6
#define LANE2_EQ_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define LANE1_EQ_DONE_CHG_INTR_MASK_FLDMASK                 0x20
#define LANE1_EQ_DONE_CHG_INTR_MASK_FLDMASK_POS             5
#define LANE1_EQ_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define LANE0_EQ_DONE_CHG_INTR_MASK_FLDMASK                 0x10
#define LANE0_EQ_DONE_CHG_INTR_MASK_FLDMASK_POS             4
#define LANE0_EQ_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define LANE3_CR_DONE_CHG_INTR_MASK_FLDMASK                 0x8
#define LANE3_CR_DONE_CHG_INTR_MASK_FLDMASK_POS             3
#define LANE3_CR_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define LANE2_CR_DONE_CHG_INTR_MASK_FLDMASK                 0x4
#define LANE2_CR_DONE_CHG_INTR_MASK_FLDMASK_POS             2
#define LANE2_CR_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define LANE1_CR_DONE_CHG_INTR_MASK_FLDMASK                 0x2
#define LANE1_CR_DONE_CHG_INTR_MASK_FLDMASK_POS             1
#define LANE1_CR_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define LANE0_CR_DONE_CHG_INTR_MASK_FLDMASK                 0x1
#define LANE0_CR_DONE_CHG_INTR_MASK_FLDMASK_POS             0
#define LANE0_CR_DONE_CHG_INTR_MASK_FLDMASK_LEN             1

#define ADDR_HPD_IRQ_WAIT_TIMER_H (DPRX_BASE + SLAVE_ID_03 + 0x268)
#define IRQ_MASK_FLDMASK                                    0x80
#define IRQ_MASK_FLDMASK_POS                                7
#define IRQ_MASK_FLDMASK_LEN                                1

#define SINK_CNT_CHG_INTR_MASK_FLDMASK                      0x40
#define SINK_CNT_CHG_INTR_MASK_FLDMASK_POS                  6
#define SINK_CNT_CHG_INTR_MASK_FLDMASK_LEN                  1

#define LINK_FAIL_BEGIN_INTR_MASK_FLDMASK                   0x20
#define LINK_FAIL_BEGIN_INTR_MASK_FLDMASK_POS               5
#define LINK_FAIL_BEGIN_INTR_MASK_FLDMASK_LEN               1

#define ADDR_DPIP_INTR (DPRX_BASE + SLAVE_ID_03 + 0x320)
#define ALIGN_STATUS_UNLOCK_INTR_FLDMASK                    0x10
#define ALIGN_STATUS_UNLOCK_INTR_FLDMASK_POS                4
#define ALIGN_STATUS_UNLOCK_INTR_FLDMASK_LEN                1

#define LINK_TRAINING_FAIL_INTR_FLDMASK                     0x8
#define LINK_TRAINING_FAIL_INTR_FLDMASK_POS                 3
#define LINK_TRAINING_FAIL_INTR_FLDMASK_LEN                 1

#define VID_FORMAT_CHANGE_FLDMASK                           0x1
#define VID_FORMAT_CHANGE_FLDMASK_POS                       0
#define VID_FORMAT_CHANGE_FLDMASK_LEN                       1

#define ADDR_DPIP_INTR_MASK (DPRX_BASE + SLAVE_ID_03 + 0x330)
#define DPIP_INTR_MASK_FLDMASK                              0x3f
#define DPIP_INTR_MASK_FLDMASK_POS                          0
#define DPIP_INTR_MASK_FLDMASK_LEN                          6

#define ADDR_PH_DBG_0 (DPRX_BASE + SLAVE_ID_04 + 0x0000)
#define CDR_LOCK_3_FLDMASK                                  0x8
#define CDR_LOCK_3_FLDMASK_POS                              3
#define CDR_LOCK_3_FLDMASK_LEN                              1

#define CDR_LOCK_2_FLDMASK                                  0x4
#define CDR_LOCK_2_FLDMASK_POS                              2
#define CDR_LOCK_2_FLDMASK_LEN                              1

#define CDR_LOCK_1_FLDMASK                                  0x2
#define CDR_LOCK_1_FLDMASK_POS                              1
#define CDR_LOCK_1_FLDMASK_LEN                              1

#define CDR_LOCK_0_FLDMASK                                  0x1
#define CDR_LOCK_0_FLDMASK_POS                              0
#define CDR_LOCK_0_FLDMASK_LEN                              1

#define ADDR_PH_DBG_1 (DPRX_BASE + SLAVE_ID_04 + 0x0004)
#define CDR_OK_3_FLDMASK                                    0x8
#define CDR_OK_3_FLDMASK_POS                                3
#define CDR_OK_3_FLDMASK_LEN                                1

#define CDR_OK_2_FLDMASK                                    0x4
#define CDR_OK_2_FLDMASK_POS                                2
#define CDR_OK_2_FLDMASK_LEN                                1

#define CDR_OK_1_FLDMASK                                    0x2
#define CDR_OK_1_FLDMASK_POS                                1
#define CDR_OK_1_FLDMASK_LEN                                1

#define CDR_OK_0_FLDMASK                                    0x1
#define CDR_OK_0_FLDMASK_POS                                0
#define CDR_OK_0_FLDMASK_LEN                                1

#define ADDR_SERDES_INTR_0_MASK (DPRX_BASE + SLAVE_ID_04 + 0x0018)
#define SERDES_INTR_0_MASK_FLDMASK                          0x1f
#define SERDES_INTR_0_MASK_FLDMASK_POS                      0
#define SERDES_INTR_0_MASK_FLDMASK_LEN                      5

#define ADDR_SERDES_INTR_1_MASK (DPRX_BASE + SLAVE_ID_04 + 0x001C)
#define SERDES_INTR_1_MASK_FLDMASK                          0x1f
#define SERDES_INTR_1_MASK_FLDMASK_POS                      0
#define SERDES_INTR_1_MASK_FLDMASK_LEN                      5

#define ADDR_SERDES_INTR_2_MASK (DPRX_BASE + SLAVE_ID_04 + 0x0020)
#define SERDES_INTR_2_MASK_FLDMASK                          0x1f
#define SERDES_INTR_2_MASK_FLDMASK_POS                      0
#define SERDES_INTR_2_MASK_FLDMASK_LEN                      5

#define ADDR_SERDES_INTR_3_MASK (DPRX_BASE + SLAVE_ID_04 + 0x0024)
#define SERDES_INTR_3_MASK_FLDMASK                          0x1f
#define SERDES_INTR_3_MASK_FLDMASK_POS                      0
#define SERDES_INTR_3_MASK_FLDMASK_LEN                      5

#define ADDR_SERDES_INTR_0 (DPRX_BASE + SLAVE_ID_04 + 0x0040)
#define ANY_LANE_CDR_LOCK_LOST_INTR_FLDMASK                 0x10
#define ANY_LANE_CDR_LOCK_LOST_INTR_FLDMASK_POS             4
#define ANY_LANE_CDR_LOCK_LOST_INTR_FLDMASK_LEN             1

#define LANE3_CDR_LOCK_LOST_INTR_FLDMASK                    0x8
#define LANE3_CDR_LOCK_LOST_INTR_FLDMASK_POS                3
#define LANE3_CDR_LOCK_LOST_INTR_FLDMASK_LEN                1

#define LANE2_CDR_LOCK_LOST_INTR_FLDMASK                    0x4
#define LANE2_CDR_LOCK_LOST_INTR_FLDMASK_POS                2
#define LANE2_CDR_LOCK_LOST_INTR_FLDMASK_LEN                1

#define LANE1_CDR_LOCK_LOST_INTR_FLDMASK                    0x2
#define LANE1_CDR_LOCK_LOST_INTR_FLDMASK_POS                1
#define LANE1_CDR_LOCK_LOST_INTR_FLDMASK_LEN                1

#define LANE0_CDR_LOCK_LOST_INTR_FLDMASK                    0x1
#define LANE0_CDR_LOCK_LOST_INTR_FLDMASK_POS                0
#define LANE0_CDR_LOCK_LOST_INTR_FLDMASK_LEN                1

#define ADDR_SERDES_INTR_1 (DPRX_BASE + SLAVE_ID_04 + 0x0044)
#define ALL_LANE_CDR_LOCK_DETECT_INTR_FLDMASK               0x10
#define ALL_LANE_CDR_LOCK_DETECT_INTR_FLDMASK_POS           4
#define ALL_LANE_CDR_LOCK_DETECT_INTR_FLDMASK_LEN           1

#define LANE3_CDR_LOCK_DETECT_INTR_FLDMASK                  0x8
#define LANE3_CDR_LOCK_DETECT_INTR_FLDMASK_POS              3
#define LANE3_CDR_LOCK_DETECT_INTR_FLDMASK_LEN              1

#define LANE2_CDR_LOCK_DETECT_INTR_FLDMASK                  0x4
#define LANE2_CDR_LOCK_DETECT_INTR_FLDMASK_POS              2
#define LANE2_CDR_LOCK_DETECT_INTR_FLDMASK_LEN              1

#define LANE1_CDR_LOCK_DETECT_INTR_FLDMASK                  0x2
#define LANE1_CDR_LOCK_DETECT_INTR_FLDMASK_POS              1
#define LANE1_CDR_LOCK_DETECT_INTR_FLDMASK_LEN              1

#define LANE0_CDR_LOCK_DETECT_INTR_FLDMASK                  0x1
#define LANE0_CDR_LOCK_DETECT_INTR_FLDMASK_POS              0
#define LANE0_CDR_LOCK_DETECT_INTR_FLDMASK_LEN              1

#define ADDR_AUD_CTRL0 (DPRX_BASE + SLAVE_ID_05 + 0x000)
#define AUD_SPDIF_MODE_FLDMASK                              0x80
#define AUD_SPDIF_MODE_FLDMASK_POS                          7
#define AUD_SPDIF_MODE_FLDMASK_LEN                          1

#define AUD_SPDIF_OUT_EN_FLDMASK                            0x40
#define AUD_SPDIF_OUT_EN_FLDMASK_POS                        6
#define AUD_SPDIF_OUT_EN_FLDMASK_LEN                        1

#define ADDR_AUD_INTR (DPRX_BASE + SLAVE_ID_05 + 0x010)
#define AUD_CH_STATUS_CHANGE_INT_FLDMASK                    0x80
#define AUD_CH_STATUS_CHANGE_INT_FLDMASK_POS                7
#define AUD_CH_STATUS_CHANGE_INT_FLDMASK_LEN                1

#define VBID_AUD_MUTE_INTR_FLDMASK                          0x40
#define VBID_AUD_MUTE_INTR_FLDMASK_POS                      6
#define VBID_AUD_MUTE_INTR_FLDMASK_LEN                      1

#define AUD_FIFO_UND_INT_FLDMASK                            0x20
#define AUD_FIFO_UND_INT_FLDMASK_POS                        5
#define AUD_FIFO_UND_INT_FLDMASK_LEN                        1

#define AUD_FIFO_OVER_INT_FLDMASK                           0x10
#define AUD_FIFO_OVER_INT_FLDMASK_POS                       4
#define AUD_FIFO_OVER_INT_FLDMASK_LEN                       1

#define AUD_LINKERR_INT_FLDMASK                             0x8
#define AUD_LINKERR_INT_FLDMASK_POS                         3
#define AUD_LINKERR_INT_FLDMASK_LEN                         1

#define AUD_DECERR_INT_FLDMASK                              0x4
#define AUD_DECERR_INT_FLDMASK_POS                          2
#define AUD_DECERR_INT_FLDMASK_LEN                          1

#define ADDR_AUD_INTR_MASK (DPRX_BASE + SLAVE_ID_05 + 0x014)
#define AUD_INTR_MASK_FLDMASK                               0xff
#define AUD_INTR_MASK_FLDMASK_POS                           0
#define AUD_INTR_MASK_FLDMASK_LEN                           8

#define ADDR_AUD_CS1 (DPRX_BASE + SLAVE_ID_05 + 0x018)
#define AUD_CS1_FLDMASK                                     0xff
#define AUD_CS1_FLDMASK_POS                                 0
#define AUD_CS1_FLDMASK_LEN                                 8

#define ADDR_AUD_CS2 (DPRX_BASE + SLAVE_ID_05 + 0x01C)
#define AUD_CS2_FLDMASK                                     0xff
#define AUD_CS2_FLDMASK_POS                                 0
#define AUD_CS2_FLDMASK_LEN                                 8

#define ADDR_AUD_CS3 (DPRX_BASE + SLAVE_ID_05 + 0x020)
#define AUD_CS3_FLDMASK                                     0xff
#define AUD_CS3_FLDMASK_POS                                 0
#define AUD_CS3_FLDMASK_LEN                                 8

#define ADDR_AUD_CS4 (DPRX_BASE + SLAVE_ID_05 + 0x024)
#define AUD_CS4_FLDMASK                                     0xff
#define AUD_CS4_FLDMASK_POS                                 0
#define AUD_CS4_FLDMASK_LEN                                 8

#define ADDR_AUD_CS5 (DPRX_BASE + SLAVE_ID_05 + 0x028)
#define AUD_CS5_FLDMASK                                     0xff
#define AUD_CS5_FLDMASK_POS                                 0
#define AUD_CS5_FLDMASK_LEN                                 8

#define ADDR_AUD_ACR_CTRL_1 (DPRX_BASE + SLAVE_ID_05 + 0x050)
#define MAUD_SEL_FLDMASK                                    0x02
#define MAUD_SEL_FLDMASK_POS                                1
#define MAUD_SEL_FLDMASK_LEN                                1

#define NAUD_SEL_FLDMASK                                    0x01
#define NAUD_SEL_FLDMASK_POS                                0
#define NAUD_SEL_FLDMASK_LEN                                1

#define ADDR_NAUD_SVAL_7_0 (DPRX_BASE + SLAVE_ID_05 + 0x054)
#define NAUD_SVAL_7_0_FLDMASK                              0xff
#define NAUD_SVAL_7_0_FLDMASK_POS                          0
#define NAUD_SVAL_7_0_FLDMASK_LEN                          8

#define ADDR_NAUD_SVAL_15_8 (DPRX_BASE + SLAVE_ID_05 + 0x058)
#define NAUD_SVAL_15_8_FLDMASK                             0xff
#define NAUD_SVAL_15_8_FLDMASK_POS                         0
#define NAUD_SVAL_15_8_FLDMASK_LEN                         8

#define ADDR_NAUD_SVAL_23_16 (DPRX_BASE + SLAVE_ID_05 + 0x05c)
#define NAUD_SVAL_23_16_FLDMASK                            0xff
#define NAUD_SVAL_23_16_FLDMASK_POS                        0
#define NAUD_SVAL_23_16_FLDMASK_LEN                        8


#define ADDR_DBG_NAUD_INI_7_0 (DPRX_BASE + SLAVE_ID_05 + 0x060)
#define DBG_NAUD_INI_7_0_FLDMASK                            0xff
#define DBG_NAUD_INI_7_0_FLDMASK_POS                        0
#define DBG_NAUD_INI_7_0_FLDMASK_LEN                        8

#define ADDR_DBG_NAUD_INI_15_8 (DPRX_BASE + SLAVE_ID_05 + 0x064)
#define DBG_NAUD_INI_15_8_FLDMASK                           0xff
#define DBG_NAUD_INI_15_8_FLDMASK_POS                       0
#define DBG_NAUD_INI_15_8_FLDMASK_LEN                       8

#define ADDR_DBG_NAUD_INI_23_16 (DPRX_BASE + SLAVE_ID_05 + 0x068)
#define DBG_NAUD_INI_23_16_FLDMASK                          0xff
#define DBG_NAUD_INI_23_16_FLDMASK_POS                      0
#define DBG_NAUD_INI_23_16_FLDMASK_LEN                      8

#define ADDR_MAUD_SVAL_7_0 (DPRX_BASE + SLAVE_ID_05 + 0x084)
#define MAUD_SVAL_7_0_FLDMASK                              0xff
#define MAUD_SVAL_7_0_FLDMASK_POS                          0
#define MAUD_SVAL_7_0_FLDMASK_LEN                          8

#define ADDR_MAUD_SVAL_15_8 (DPRX_BASE + SLAVE_ID_05 + 0x088)
#define MAUD_SVAL_15_8_FLDMASK                             0xff
#define MAUD_SVAL_15_8_FLDMASK_POS                         0
#define MAUD_SVAL_15_8_FLDMASK_LEN                         8

#define ADDR_MAUD_SVAL_23_16 (DPRX_BASE + SLAVE_ID_05 + 0x08c)
#define MAUD_SVAL_23_16_FLDMASK                            0xff
#define MAUD_SVAL_23_16_FLDMASK_POS                        0
#define MAUD_SVAL_23_16_FLDMASK_LEN                        8

#define ADDR_DBG_MAUD_INI_7_0 (DPRX_BASE + SLAVE_ID_05 + 0x090)
#define DBG_MAUD_INI_7_0_FLDMASK                            0xff
#define DBG_MAUD_INI_7_0_FLDMASK_POS                        0
#define DBG_MAUD_INI_7_0_FLDMASK_LEN                        8

#define ADDR_DBG_MAUD_INI_15_8 (DPRX_BASE + SLAVE_ID_05 + 0x094)
#define DBG_MAUD_INI_15_8_FLDMASK                           0xff
#define DBG_MAUD_INI_15_8_FLDMASK_POS                       0
#define DBG_MAUD_INI_15_8_FLDMASK_LEN                       8

#define ADDR_DBG_MAUD_INI_23_16 (DPRX_BASE + SLAVE_ID_05 + 0x098)
#define DBG_MAUD_INI_23_16_FLDMASK                          0xff
#define DBG_MAUD_INI_23_16_FLDMASK_POS                      0
#define DBG_MAUD_INI_23_16_FLDMASK_LEN                      8

#define ADDR_AUD_CTRL1 (DPRX_BASE + SLAVE_ID_05 + 0x0dc)
#define AUD_WSIZE_FLDMASK                                   0x20
#define AUD_WSIZE_FLDMASK_POS                               5
#define AUD_WSIZE_FLDMASK_LEN                               1

#define ADDR_AUD_CTRL2 (DPRX_BASE + SLAVE_ID_05 + 0x0e0)
#define AUD_I2S_MODE_FLDMASK                                0x10
#define AUD_I2S_MODE_FLDMASK_POS                            4
#define AUD_I2S_MODE_FLDMASK_LEN                            1

#define AUD_I2S_OUT_EN_FLDMASK                              0xf
#define AUD_I2S_OUT_EN_FLDMASK_POS                          0
#define AUD_I2S_OUT_EN_FLDMASK_LEN                          4

#define ADDR_AFC_SHIFT_B_L (DPRX_BASE + SLAVE_ID_05 + 0x114)
#define AFC_SHIFT_B1_FLDMASK                                0x38
#define AFC_SHIFT_B1_FLDMASK_POS                            3
#define AFC_SHIFT_B1_FLDMASK_LEN                            3

#define AFC_SHIFT_B0_FLDMASK                                0x7
#define AFC_SHIFT_B0_FLDMASK_POS                            0
#define AFC_SHIFT_B0_FLDMASK_LEN                            3

#define ADDR_AFC_SHIFT_B_H (DPRX_BASE + SLAVE_ID_05 + 0x118)
#define AFC_SHIFT_B3_FLDMASK                                0x38
#define AFC_SHIFT_B3_FLDMASK_POS                            3
#define AFC_SHIFT_B3_FLDMASK_LEN                            3

#define AFC_SHIFT_B2_FLDMASK                                0x7
#define AFC_SHIFT_B2_FLDMASK_POS                            0
#define AFC_SHIFT_B2_FLDMASK_LEN                            3

#define ADDR_DBG_AUD_CS_0 (DPRX_BASE + SLAVE_ID_05 + 0x264)
#define DBG_AUD_CS_0_FLDMASK                                0xff
#define DBG_AUD_CS_0_FLDMASK_POS                            0
#define DBG_AUD_CS_0_FLDMASK_LEN                            8

#define ADDR_DBG_AUD_CS_1 (DPRX_BASE + SLAVE_ID_05 + 0x268)
#define DBG_AUD_CS_1_FLDMASK                                0xff
#define DBG_AUD_CS_1_FLDMASK_POS                            0
#define DBG_AUD_CS_1_FLDMASK_LEN                            8

#define ADDR_DBG_AUD_CS_2 (DPRX_BASE + SLAVE_ID_05 + 0x26c)
#define DBG_AUD_CS_2_FLDMASK                                0xff
#define DBG_AUD_CS_2_FLDMASK_POS                            0
#define DBG_AUD_CS_2_FLDMASK_LEN                            8

#define ADDR_DBG_AUD_CS_3 (DPRX_BASE + SLAVE_ID_05 + 0x270)
#define DBG_AUD_CS_3_FLDMASK                                0xff
#define DBG_AUD_CS_3_FLDMASK_POS                            0
#define DBG_AUD_CS_3_FLDMASK_LEN                            8

#define ADDR_DBG_AUD_CS_4 (DPRX_BASE + SLAVE_ID_05 + 0x274)
#define DBG_AUD_CS_4_FLDMASK                                0xff
#define DBG_AUD_CS_4_FLDMASK_POS                            0
#define DBG_AUD_CS_4_FLDMASK_LEN                            8

#define ADDR_AUD_INTR_MASK_AUD (DPRX_BASE + SLAVE_ID_05 + 0x280)
#define AUD_INTR_MASK_AUD_FLDMASK                           0xff
#define AUD_INTR_MASK_AUD_FLDMASK_POS                       0
#define AUD_INTR_MASK_AUD_FLDMASK_LEN                       8

#define ADDR_AUD_ADD_INTR (DPRX_BASE + SLAVE_ID_05 + 0x2A0)
#define AUD_UNMUTE_INT_FLDMASK                              0x1
#define AUD_UNMUTE_INT_FLDMASK_POS                          0
#define AUD_UNMUTE_INT_FLDMASK_LEN                          1

#define ADDR_AUD_ADD_INTR_MASK (DPRX_BASE + SLAVE_ID_05 + 0x2A4)
#define AUD_INTR_ADD_MASK_FLDMASK                           0xff
#define AUD_INTR_ADD_MASK_FLDMASK_POS                       0
#define AUD_INTR_ADD_MASK_FLDMASK_LEN                       8

#define ADDR_H_RES_LOW (DPRX_BASE + SLAVE_ID_06 + 0x000)
#define H_RES_LOW_FLDMASK                                   0xff
#define H_RES_LOW_FLDMASK_POS                               0
#define H_RES_LOW_FLDMASK_LEN                               8

#define ADDR_H_RES_HIGH (DPRX_BASE + SLAVE_ID_06 + 0x004)
#define H_RES_HIGH_FLDMASK                                  0x1f
#define H_RES_HIGH_FLDMASK_POS                              0
#define H_RES_HIGH_FLDMASK_LEN                              5

#define ADDR_V_RES_LOW (DPRX_BASE + SLAVE_ID_06 + 0x008)
#define V_RES_LOW_FLDMASK                                   0xff
#define V_RES_LOW_FLDMASK_POS                               0
#define V_RES_LOW_FLDMASK_LEN                               8

#define ADDR_V_RES_HIGH (DPRX_BASE + SLAVE_ID_06 + 0x00c)
#define V_RES_HIGH_FLDMASK                                  0xf
#define V_RES_HIGH_FLDMASK_POS                              0
#define V_RES_HIGH_FLDMASK_LEN                              4

#define ADDR_ACT_PIX_LOW (DPRX_BASE + SLAVE_ID_06 + 0x010)
#define ACT_PIX_LOW_FLDMASK                                 0xff
#define ACT_PIX_LOW_FLDMASK_POS                             0
#define ACT_PIX_LOW_FLDMASK_LEN                             8

#define ADDR_ACT_PIX_HIGH (DPRX_BASE + SLAVE_ID_06 + 0x014)
#define ACT_PIX_HIGH_FLDMASK                                0x1f
#define ACT_PIX_HIGH_FLDMASK_POS                            0
#define ACT_PIX_HIGH_FLDMASK_LEN                            5

#define ADDR_ACT_LINE_LOW (DPRX_BASE + SLAVE_ID_06 + 0x018)
#define ACT_LINE_LOW_FLDMASK                                0xff
#define ACT_LINE_LOW_FLDMASK_POS                            0
#define ACT_LINE_LOW_FLDMASK_LEN                            8

#define ADDR_ACT_LINE_HIGH (DPRX_BASE + SLAVE_ID_06 + 0x01c)
#define ACT_LINE_HIGH_FLDMASK                               0xf
#define ACT_LINE_HIGH_FLDMASK_POS                           0
#define ACT_LINE_HIGH_FLDMASK_LEN                           4

#define ADDR_VSYNC_TO_ACT_LINE (DPRX_BASE + SLAVE_ID_06 + 0x020)
#define ACT_VSYNC_TO_LINE_FLDMASK                           0xff
#define ACT_VSYNC_TO_LINE_FLDMASK_POS                       0
#define ACT_VSYNC_TO_LINE_FLDMASK_LEN                       8

#define ADDR_ACT_LINE_TO_VSYNC (DPRX_BASE + SLAVE_ID_06 + 0x024)
#define ACT_LINE_TO_VSYNC_FLDMASK                           0xff
#define ACT_LINE_TO_VSYNC_FLDMASK_POS                       0
#define ACT_LINE_TO_VSYNC_FLDMASK_LEN                       8

#define ADDR_SYNC_STATUS (DPRX_BASE + SLAVE_ID_06 + 0x028)
#define VIDEO_TYPE_FLDMASK                                  0x4
#define VIDEO_TYPE_FLDMASK_POS                              2
#define VIDEO_TYPE_FLDMASK_LEN                              1

#define VSYNC_POL_FLDMASK                                   0x2
#define VSYNC_POL_FLDMASK_POS                               1
#define VSYNC_POL_FLDMASK_LEN                               1

#define HSYNC_POL_FLDMASK                                   0x1
#define HSYNC_POL_FLDMASK_POS                               0
#define HSYNC_POL_FLDMASK_LEN                               1

#define ADDR_H_F_PORCH_LOW (DPRX_BASE + SLAVE_ID_06 + 0x02c)
#define H_F_PORCH_LOW_FLDMASK                               0xff
#define H_F_PORCH_LOW_FLDMASK_POS                           0
#define H_F_PORCH_LOW_FLDMASK_LEN                           8

#define ADDR_H_F_PORCH_HIGH (DPRX_BASE + SLAVE_ID_06 + 0x030)
#define H_F_PORCH_HIGH_FLDMASK                              0xff
#define H_F_PORCH_HIGH_FLDMASK_POS                          0
#define H_F_PORCH_HIGH_FLDMASK_LEN                          8

#define ADDR_HSYNC_WIDTH_LOW (DPRX_BASE + SLAVE_ID_06 + 0x034)
#define HSYNC_WIDTH_LOW_FLDMASK                             0xff
#define HSYNC_WIDTH_LOW_FLDMASK_POS                         0
#define HSYNC_WIDTH_LOW_FLDMASK_LEN                         8

#define ADDR_HSYNC_WIDTH_HIGH (DPRX_BASE + SLAVE_ID_06 + 0x038)
#define HSYNC_WIDTH_HIGH_FLDMASK                            0x3
#define HSYNC_WIDTH_HIGH_FLDMASK_POS                        0
#define HSYNC_WIDTH_HIGH_FLDMASK_LEN                        2

#define ADDR_VID_STABLE_DET (DPRX_BASE + SLAVE_ID_06 + 0x040)
#define VSYNC_DET_CNT_EXTEND_FLDMASK                        0x18
#define VSYNC_DET_CNT_EXTEND_FLDMASK_POS                    3
#define VSYNC_DET_CNT_EXTEND_FLDMASK_LEN                    2

#define VID_STRM_STABLE_STATUS_FLDMASK                      0x4
#define VID_STRM_STABLE_STATUS_FLDMASK_POS                  2
#define VID_STRM_STABLE_STATUS_FLDMASK_LEN                  1

#define VSYNC_DET_CNT_FLDMASK                               0x3
#define VSYNC_DET_CNT_FLDMASK_POS                           0
#define VSYNC_DET_CNT_FLDMASK_LEN                           2

#define ADDR_CHK_CRC_CTRL (DPRX_BASE + SLAVE_ID_06 + 0x080)
#define CRC_COUNT_FLDMASK                                   0xf0
#define CRC_COUNT_FLDMASK_POS                               4
#define CRC_COUNT_FLDMASK_LEN                               4

#define CRC_EN_FLDMASK                                      0x1
#define CRC_EN_FLDMASK_POS                                  0
#define CRC_EN_FLDMASK_LEN                                  1

#define ADDR_CRC_R_CR_OUT_7_0 (DPRX_BASE + SLAVE_ID_06 + 0x84)
#define CRC_R_CR_OUT_7_0_FLDMASK                            0xff
#define CRC_R_CR_OUT_7_0_FLDMASK_POS                        0
#define CRC_R_CR_OUT_7_0_FLDMASK_LEN                        8

#define ADDR_CRC_R_CR_OUT_15_8 (DPRX_BASE + SLAVE_ID_06 + 0x88)
#define CRC_R_CR_OUT_15_8_FLDMASK                           0xff
#define CRC_R_CR_OUT_15_8_FLDMASK_POS                       0
#define CRC_R_CR_OUT_15_8_FLDMASK_LEN                       8

#define ADDR_CRC_G_Y_OUT_7_0 (DPRX_BASE + SLAVE_ID_06 + 0x8c)
#define CRC_G_Y_OUT_7_0_FLDMASK                             0xff
#define CRC_G_Y_OUT_7_0_FLDMASK_POS                         0
#define CRC_G_Y_OUT_7_0_FLDMASK_LEN                         8

#define ADDR_CRC_G_Y_OUT_15_8 (DPRX_BASE + SLAVE_ID_06 + 0x90)
#define CRC_G_Y_OUT_15_8_FLDMASK                            0xff
#define CRC_G_Y_OUT_15_8_FLDMASK_POS                        0
#define CRC_G_Y_OUT_15_8_FLDMASK_LEN                        8

#define ADDR_CRC_B_CB_OUT_7_0 (DPRX_BASE + SLAVE_ID_06 + 0x94)
#define CRC_B_CB_OUT_7_0_FLDMASK                            0xff
#define CRC_B_CB_OUT_7_0_FLDMASK_POS                        0
#define CRC_B_CB_OUT_7_0_FLDMASK_LEN                        8

#define ADDR_CRC_B_CB_OUT_15_8 (DPRX_BASE + SLAVE_ID_06 + 0x98)
#define CRC_B_CB_OUT_15_8_FLDMASK                           0xff
#define CRC_B_CB_OUT_15_8_FLDMASK_POS                       0
#define CRC_B_CB_OUT_15_8_FLDMASK_LEN                       8

#define ADDR_VID_INT (DPRX_BASE + SLAVE_ID_06 + 0x0c0)
#define VSYNC_DET_INT_FLDMASK                               0x20
#define VSYNC_DET_INT_FLDMASK_POS                           5
#define VSYNC_DET_INT_FLDMASK_LEN                           1

#define V_RES_CHANGED_FLDMASK                               0x10
#define V_RES_CHANGED_FLDMASK_POS                           4
#define V_RES_CHANGED_FLDMASK_LEN                           1

#define H_RES_CHANGED_FLDMASK                               0x8
#define H_RES_CHANGED_FLDMASK_POS                           3
#define H_RES_CHANGED_FLDMASK_LEN                           1

#define SYNC_POL_CHANGED_FLDMASK                            0x4
#define SYNC_POL_CHANGED_FLDMASK_POS                        2
#define SYNC_POL_CHANGED_FLDMASK_LEN                        1

#define VID_TYPE_CHANGED_FLDMASK                            0x2
#define VID_TYPE_CHANGED_FLDMASK_POS                        1
#define VID_TYPE_CHANGED_FLDMASK_LEN                        1

#define VID_STRM_STABLE_INT_FLDMASK                         0x1
#define VID_STRM_STABLE_INT_FLDMASK_POS                     0
#define VID_STRM_STABLE_INT_FLDMASK_LEN                     1

#define ADDR_VID_INT_MASK (DPRX_BASE + SLAVE_ID_06 + 0x0c4)
#define VID_INT_MASK_FLDMASK                                0xff
#define VID_INT_MASK_FLDMASK_POS                            0
#define VID_INT_MASK_FLDMASK_LEN                            8

#define ADDR_VPLL_CTRL_0 (DPRX_BASE + SLAVE_ID_07 + 0x00000)
#define VPLL_K_CNFG_FLDMASK                                 0xff
#define VPLL_K_CNFG_FLDMASK_POS                             0
#define VPLL_K_CNFG_FLDMASK_LEN                             8

#define ADDR_VPLL_CTRL_1 (DPRX_BASE + SLAVE_ID_07 + 0x00004)
#define VID_PLL_EN_FLDMASK                                  0x1
#define VID_PLL_EN_FLDMASK_POS                              0
#define VID_PLL_EN_FLDMASK_LEN                              1

#define ADDR_VPLL_STATUS (DPRX_BASE + SLAVE_ID_07 + 0x00024)
#define VPLL_DIV_START_FLDMASK                              0x1
#define VPLL_DIV_START_FLDMASK_POS                          0
#define VPLL_DIV_START_FLDMASK_LEN                          1

#define ADDR_APLL_CTRL (DPRX_BASE + SLAVE_ID_07 + 0x00034)
#define AUD_PLL_EN_FLDMASK                                  0x1
#define AUD_PLL_EN_FLDMASK_POS                              0
#define AUD_PLL_EN_FLDMASK_LEN                              1

#define ADDR_APLL_CLK_SEL (DPRX_BASE + SLAVE_ID_07 + 0x00038)
#define AUD_CLK_SEL_FLDMASK                                 0x7
#define AUD_CLK_SEL_FLDMASK_POS                             0
#define AUD_CLK_SEL_FLDMASK_LEN                             3

#define ADDR_APLL_STATUS (DPRX_BASE + SLAVE_ID_07 + 0x00058)
#define APLL_DIV_START_FLDMASK                              0x1
#define APLL_DIV_START_FLDMASK_POS                          0
#define APLL_DIV_START_FLDMASK_LEN                          1

#define ADDR_BKSV_0 (DPRX_BASE + SLAVE_ID_08 + 0xA0000)
#define BKSV_0_FLDMASK                                      0xff
#define BKSV_0_FLDMASK_POS                                  0
#define BKSV_0_FLDMASK_LEN                                  8

#define ADDR_BKSV_1 (DPRX_BASE + SLAVE_ID_08 + 0xA0004)
#define BKSV_1_FLDMASK                                      0xff
#define BKSV_1_FLDMASK_POS                                  0
#define BKSV_1_FLDMASK_LEN                                  8

#define ADDR_BKSV_2 (DPRX_BASE + SLAVE_ID_08 + 0xA0008)
#define BKSV_2_FLDMASK                                      0xff
#define BKSV_2_FLDMASK_POS                                  0
#define BKSV_2_FLDMASK_LEN                                  8

#define ADDR_BKSV_3 (DPRX_BASE + SLAVE_ID_08 + 0xA000C)
#define BKSV_3_FLDMASK                                      0xff
#define BKSV_3_FLDMASK_POS                                  0
#define BKSV_3_FLDMASK_LEN                                  8

#define ADDR_BKSV_4 (DPRX_BASE + SLAVE_ID_08 + 0xA0010)
#define BKSV_4_FLDMASK                                      0xff
#define BKSV_4_FLDMASK_POS                                  0
#define BKSV_4_FLDMASK_LEN                                  8

#define ADDR_HDCP_R0_0 (DPRX_BASE + SLAVE_ID_08 + 0xA0014)
#define HDCP_R0_0_FLDMASK                                   0xff
#define HDCP_R0_0_FLDMASK_POS                               0
#define HDCP_R0_0_FLDMASK_LEN                               8

#define ADDR_HDCP_R0_1 (DPRX_BASE + SLAVE_ID_08 + 0xA0018)
#define HDCP_R0_1_FLDMASK                                   0xff
#define HDCP_R0_1_FLDMASK_POS                               0
#define HDCP_R0_1_FLDMASK_LEN                               8

#define ADDR_SOURCE_AKSV_0 (DPRX_BASE + SLAVE_ID_08 + 0xA001C)
#define SOURCE_AKSV_0_FLDMASK                               0xff
#define SOURCE_AKSV_0_FLDMASK_POS                           0
#define SOURCE_AKSV_0_FLDMASK_LEN                           8

#define ADDR_SOURCE_AKSV_1 (DPRX_BASE + SLAVE_ID_08 + 0xA0020)
#define SOURCE_AKSV_1_FLDMASK                               0xff
#define SOURCE_AKSV_1_FLDMASK_POS                           0
#define SOURCE_AKSV_1_FLDMASK_LEN                           8

#define ADDR_SOURCE_AKSV_2 (DPRX_BASE + SLAVE_ID_08 + 0xA0024)
#define SOURCE_AKSV_2_FLDMASK                               0xff
#define SOURCE_AKSV_2_FLDMASK_POS                           0
#define SOURCE_AKSV_2_FLDMASK_LEN                           8

#define ADDR_SOURCE_AKSV_3 (DPRX_BASE + SLAVE_ID_08 + 0xA0028)
#define SOURCE_AKSV_3_FLDMASK                               0xff
#define SOURCE_AKSV_3_FLDMASK_POS                           0
#define SOURCE_AKSV_3_FLDMASK_LEN                           8

#define ADDR_SOURCE_AKSV_4 (DPRX_BASE + SLAVE_ID_08 + 0xA002C)
#define SOURCE_AKSV_4_FLDMASK                               0xff
#define SOURCE_AKSV_4_FLDMASK_POS                           0
#define SOURCE_AKSV_4_FLDMASK_LEN                           8

#define ADDR_SOURCE_AN_0 (DPRX_BASE + SLAVE_ID_08 + 0xA0030)
#define SOURCE_AN_0_FLDMASK                                 0xff
#define SOURCE_AN_0_FLDMASK_POS                             0
#define SOURCE_AN_0_FLDMASK_LEN                             8

#define ADDR_SOURCE_AN_1 (DPRX_BASE + SLAVE_ID_08 + 0xA0034)
#define SOURCE_AN_1_FLDMASK                                 0xff
#define SOURCE_AN_1_FLDMASK_POS                             0
#define SOURCE_AN_1_FLDMASK_LEN                             8

#define ADDR_SOURCE_AN_2 (DPRX_BASE + SLAVE_ID_08 + 0xA0038)
#define SOURCE_AN_2_FLDMASK                                 0xff
#define SOURCE_AN_2_FLDMASK_POS                             0
#define SOURCE_AN_2_FLDMASK_LEN                             8

#define ADDR_SOURCE_AN_3 (DPRX_BASE + SLAVE_ID_08 + 0xA003C)
#define SOURCE_AN_3_FLDMASK                                 0xff
#define SOURCE_AN_3_FLDMASK_POS                             0
#define SOURCE_AN_3_FLDMASK_LEN                             8

#define ADDR_SOURCE_AN_4 (DPRX_BASE + SLAVE_ID_08 + 0xA0040)
#define SOURCE_AN_4_FLDMASK                                 0xff
#define SOURCE_AN_4_FLDMASK_POS                             0
#define SOURCE_AN_4_FLDMASK_LEN                             8

#define ADDR_SOURCE_AN_5 (DPRX_BASE + SLAVE_ID_08 + 0xA0044)
#define SOURCE_AN_5_FLDMASK                                 0xff
#define SOURCE_AN_5_FLDMASK_POS                             0
#define SOURCE_AN_5_FLDMASK_LEN                             8

#define ADDR_SOURCE_AN_6 (DPRX_BASE + SLAVE_ID_08 + 0xA0048)
#define SOURCE_AN_6_FLDMASK                                 0xff
#define SOURCE_AN_6_FLDMASK_POS                             0
#define SOURCE_AN_6_FLDMASK_LEN                             8

#define ADDR_SOURCE_AN_7 (DPRX_BASE + SLAVE_ID_08 + 0xA004C)
#define SOURCE_AN_7_FLDMASK                                 0xff
#define SOURCE_AN_7_FLDMASK_POS                             0
#define SOURCE_AN_7_FLDMASK_LEN                             8

#define ADDR_BCAPS (DPRX_BASE + SLAVE_ID_08 + 0xA00A0)
#define HDCP_CAPABLE_FLDMASK                                0x1
#define HDCP_CAPABLE_FLDMASK_POS                            0
#define HDCP_CAPABLE_FLDMASK_LEN                            1

#define ADDR_BSTATUS (DPRX_BASE + SLAVE_ID_08 + 0xA00A4)
#define REAUTH_REQ_FLDMASK                                  0x8
#define REAUTH_REQ_FLDMASK_POS                              3
#define REAUTH_REQ_FLDMASK_LEN                              1

#define LINK_INTEGRITY_FAILURE_FLDMASK                      0x4
#define LINK_INTEGRITY_FAILURE_FLDMASK_POS                  2
#define LINK_INTEGRITY_FAILURE_FLDMASK_LEN                  1

#define R0_AVAILABLE_FLDMASK                                0x2
#define R0_AVAILABLE_FLDMASK_POS                            1
#define R0_AVAILABLE_FLDMASK_LEN                            1

#define ADDR_HDCP_STATUS (DPRX_BASE + SLAVE_ID_08 + 0xA0404)
#define CRC32_DONE_FLDMASK                                  0x40
#define CRC32_DONE_FLDMASK_POS                              6
#define CRC32_DONE_FLDMASK_LEN                              1

#define CRC32_ERR_FLDMASK                                   0x20
#define CRC32_ERR_FLDMASK_POS                               5
#define CRC32_ERR_FLDMASK_LEN                               1

#define HDCP_REG_ROM_RW_DONE_FLDMASK                        0x10
#define HDCP_REG_ROM_RW_DONE_FLDMASK_POS                    4
#define HDCP_REG_ROM_RW_DONE_FLDMASK_LEN                    1

#define HDCP_REG_BIST_ERR_FLDMASK                           0x4
#define HDCP_REG_BIST_ERR_FLDMASK_POS                       2
#define HDCP_REG_BIST_ERR_FLDMASK_LEN                       1

#define HDCP_REG_DECRYPT_FLDMASK                            0x2
#define HDCP_REG_DECRYPT_FLDMASK_POS                        1
#define HDCP_REG_DECRYPT_FLDMASK_LEN                        1

#define HDCP_REG_AUTH_STATUS_FLDMASK                        0x1
#define HDCP_REG_AUTH_STATUS_FLDMASK_POS                    0
#define HDCP_REG_AUTH_STATUS_FLDMASK_LEN                    1

#define ADDR_HDCP_KEY_ROM_OP (DPRX_BASE + SLAVE_ID_08 + 0xA0408)
#define CRC32_START_FLDMASK                                 0x2
#define CRC32_START_FLDMASK_POS                             1
#define CRC32_START_FLDMASK_LEN                             1

#define HDCP_ROM_READ_EN_FLDMASK                            0x1
#define HDCP_ROM_READ_EN_FLDMASK_POS                        0
#define HDCP_ROM_READ_EN_FLDMASK_LEN                        1

#define ADDR_HDCP_ADDR_H (DPRX_BASE + SLAVE_ID_08 + 0xA040C)
#define HDCP_ROM_ADDR_H_FLDMASK                             0x1
#define HDCP_ROM_ADDR_H_FLDMASK_POS                         0
#define HDCP_ROM_ADDR_H_FLDMASK_LEN                         1

#define ADDR_HDCP_ADDR_L (DPRX_BASE + SLAVE_ID_08 + 0xA0410)
#define HDCP_ROM_ADDR_L_FLDMASK                             0xff
#define HDCP_ROM_ADDR_L_FLDMASK_POS                         0
#define HDCP_ROM_ADDR_L_FLDMASK_LEN                         8

#define ADDR_HDCP_INT (DPRX_BASE + SLAVE_ID_08 + 0xA046c)
#define R0_AVAILABLE_INTR_FLDMASK                           0x8
#define R0_AVAILABLE_INTR_FLDMASK_POS                       3
#define R0_AVAILABLE_INTR_FLDMASK_LEN                       1

#define REG_LINK_FAIL_INT_FLDMASK                           0x4
#define REG_LINK_FAIL_INT_FLDMASK_POS                       2
#define REG_LINK_FAIL_INT_FLDMASK_LEN                       1

#define HDCP_REG_AUTH_OK_INT_FLDMASK                        0x2
#define HDCP_REG_AUTH_OK_INT_FLDMASK_POS                    1
#define HDCP_REG_AUTH_OK_INT_FLDMASK_LEN                    1

#define HDCP_REG_AUTH_START_INT_FLDMASK                     0x1
#define HDCP_REG_AUTH_START_INT_FLDMASK_POS                 0
#define HDCP_REG_AUTH_START_INT_FLDMASK_LEN                 1

#define ADDR_HDCP_INT_MASK (DPRX_BASE + SLAVE_ID_08 + 0xA0470)
#define HDCP_INT_MASK_FLDMASK                               0xf
#define HDCP_INT_MASK_FLDMASK_POS                           0
#define HDCP_INT_MASK_FLDMASK_LEN                           4

#define ADDR_HDCP_INT_AEC_EN (DPRX_BASE + SLAVE_ID_08 + 0xA0474)
#define HDCP_INT_AEC_EN_FLDMASK                             0xf
#define HDCP_INT_AEC_EN_FLDMASK_POS                         0
#define HDCP_INT_AEC_EN_FLDMASK_LEN                         4

#define DPRX_TXDETECT_00 (WRAP_BASE + WRAPPER_0 + 0x00)
#define RG_MAC_TX_ATTACH_SEL_FLDMASK                        0x10
#define RG_MAC_TX_ATTACH_SEL_FLDMASK_POS                    4
#define RG_MAC_TX_ATTACH_SEL_FLDMASK_LEN                    1

#define DPRX_TXDETECT_04 (WRAP_BASE + WRAPPER_0 + 0x04)
#define RG_MAC_TX_ASSERT_CNT_FLDMASK                        0x3fffff
#define RG_MAC_TX_ASSERT_CNT_FLDMASK_POS                    0
#define RG_MAC_TX_ASSERT_CNT_FLDMASK_LEN                    22

#define RG_MAC_BYPASS_DPCD_DSC_EN_FLDMASK                   0x800000
#define RG_MAC_BYPASS_DPCD_DSC_EN_FLDMASK_POS               23
#define RG_MAC_BYPASS_DPCD_DSC_EN_FLDMASK_LEN               1

#define DPRX_TXDETECT_08 (WRAP_BASE + WRAPPER_0 + 0x08)
#define RG_MAC_TX_DEASSERT_CNT_FLDMASK                      0x3fffff
#define RG_MAC_TX_DEASSERT_CNT_FLDMASK_POS                  0
#define RG_MAC_TX_DEASSERT_CNT_FLDMASK_LEN                  22

#define RG_MAC_VBID_STOP_HCNT_EN_FLDMASK                    0x400000
#define RG_MAC_VBID_STOP_HCNT_EN_FLDMASK_POS                22
#define RG_MAC_VBID_STOP_HCNT_EN_FLDMASK_LEN                1

#define RG_MAC_SIMPLE422_SWAP_FLDMASK                       0x800000
#define RG_MAC_SIMPLE422_SWAP_FLDMASK_POS                   23
#define RG_MAC_SIMPLE422_SWAP_FLDMASK_LEN                   1

#define DPRX_WRAPCTL_28 (WRAP_BASE + WRAPPER_0 + 0x28)
#define RG_MAC_WRAP_SW_HPD_RDY_FLDMASK                      0x2
#define RG_MAC_WRAP_SW_HPD_RDY_FLDMASK_POS                  1
#define RG_MAC_WRAP_SW_HPD_RDY_FLDMASK_LEN                  1

#define RG_MAC_WRAP_CFG2_FLDMASK                            0x10000
#define RG_MAC_WRAP_CFG2_FLDMASK_POS                        16
#define RG_MAC_WRAP_CFG2_FLDMASK_LEN                        1

#define DPRX_INTCTL_40 (WRAP_BASE + WRAPPER_0 + 0x40)
#define RG_MAC_WRAP_INT_HW_MASK_FLDMASK                     0x1
#define RG_MAC_WRAP_INT_HW_MASK_FLDMASK_POS                 0
#define RG_MAC_WRAP_INT_HW_MASK_FLDMASK_LEN                 1

#define RG_MAC_WRAP_AUDIO_MUTE_SW_FLDMASK                   0x200
#define RG_MAC_WRAP_AUDIO_MUTE_SW_FLDMASK_POS               9
#define RG_MAC_WRAP_AUDIO_MUTE_SW_FLDMASK_LEN               1

#define RG_MAC_WRAP_AUDIO_MUTE_INV_FLDMASK                  0x400
#define RG_MAC_WRAP_AUDIO_MUTE_INV_FLDMASK_POS              10
#define RG_MAC_WRAP_AUDIO_MUTE_INV_FLDMASK_LEN              1

#define RG_MAC_WRAP_AUDIO_MUTE_FLDMASK                      0x800
#define RG_MAC_WRAP_AUDIO_MUTE_FLDMASK_POS                  11
#define RG_MAC_WRAP_AUDIO_MUTE_FLDMASK_LEN                  1

#define RG_MAC_WRAP_FRC_AUDIO_MUTE_FLDMASK                  0x1000
#define RG_MAC_WRAP_FRC_AUDIO_MUTE_FLDMASK_POS              12
#define RG_MAC_WRAP_FRC_AUDIO_MUTE_FLDMASK_LEN              1

#define RG_MAC_WRAP_VIDEO_MUTE_SW_FLDMASK                   0x20000
#define RG_MAC_WRAP_VIDEO_MUTE_SW_FLDMASK_POS               17
#define RG_MAC_WRAP_VIDEO_MUTE_SW_FLDMASK_LEN               1

#define RG_MAC_WRAP_VIDEO_MUTE_INV_FLDMASK                  0x40000
#define RG_MAC_WRAP_VIDEO_MUTE_INV_FLDMASK_POS              18
#define RG_MAC_WRAP_VIDEO_MUTE_INV_FLDMASK_LEN              1

#define RG_MAC_WRAP_VIDEO_MUTE_FLDMASK                      0x80000
#define RG_MAC_WRAP_VIDEO_MUTE_FLDMASK_POS                  19
#define RG_MAC_WRAP_VIDEO_MUTE_FLDMASK_LEN                  1

#define RG_MAC_WRAP_FRC_VIDEO_MUTE_FLDMASK                  0x100000
#define RG_MAC_WRAP_FRC_VIDEO_MUTE_FLDMASK_POS              20
#define RG_MAC_WRAP_FRC_VIDEO_MUTE_FLDMASK_LEN              1

#define DPRX_AFIFO_44 (WRAP_BASE + WRAPPER_0 + 0x44)
#define RG_MAC_VID_WR_SW_RSTB_FLDMASK                       0x8
#define RG_MAC_VID_WR_SW_RSTB_FLDMASK_POS                   3
#define RG_MAC_VID_WR_SW_RSTB_FLDMASK_LEN                   1

#define RG_MAC_VID_RD_SW_RSTB_FLDMASK                       0x10
#define RG_MAC_VID_RD_SW_RSTB_FLDMASK_POS                   4
#define RG_MAC_VID_RD_SW_RSTB_FLDMASK_LEN                   1

#define RG_MAC_DSC_WR_SW_RSTB_FLDMASK                       0x800
#define RG_MAC_DSC_WR_SW_RSTB_FLDMASK_POS                   11
#define RG_MAC_DSC_WR_SW_RSTB_FLDMASK_LEN                   1

#define RG_MAC_DSC_RD_SW_RSTB_FLDMASK                       0x1000
#define RG_MAC_DSC_RD_SW_RSTB_FLDMASK_POS                   12
#define RG_MAC_DSC_RD_SW_RSTB_FLDMASK_LEN                   1

#define DPRX_MISC_48 (WRAP_BASE + WRAPPER_0 + 0x48)
#define RG_MAC_AUD_SCK_INV_FLDMASK                          0x8
#define RG_MAC_AUD_SCK_INV_FLDMASK_POS                      3
#define RG_MAC_AUD_SCK_INV_FLDMASK_LEN                      1

#define RG_MAC_AUD_WS_INV_FLDMASK                           0x10
#define RG_MAC_AUD_WS_INV_FLDMASK_POS                       4
#define RG_MAC_AUD_WS_INV_FLDMASK_LEN                       1

#define DPRX_LT_4C (WRAP_BASE + WRAPPER_0 + 0x4C)
#define RG_MAC_LT_USING_TABLE_FLDMASK                       0x1
#define RG_MAC_LT_USING_TABLE_FLDMASK_POS                   0
#define RG_MAC_LT_USING_TABLE_FLDMASK_LEN                   1

#define DPRX_LT_50 (WRAP_BASE + WRAPPER_0 + 0x50)
#define RG_MAC_LT_RBR_CR_TABLE_1_FLDMASK                    0xf
#define RG_MAC_LT_RBR_CR_TABLE_1_FLDMASK_POS                0
#define RG_MAC_LT_RBR_CR_TABLE_1_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_CR_TABLE_2_FLDMASK                    0xf0
#define RG_MAC_LT_RBR_CR_TABLE_2_FLDMASK_POS                4
#define RG_MAC_LT_RBR_CR_TABLE_2_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_CR_TABLE_3_FLDMASK                    0xf00
#define RG_MAC_LT_RBR_CR_TABLE_3_FLDMASK_POS                8
#define RG_MAC_LT_RBR_CR_TABLE_3_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_CR_TABLE_4_FLDMASK                    0xf000
#define RG_MAC_LT_RBR_CR_TABLE_4_FLDMASK_POS                12
#define RG_MAC_LT_RBR_CR_TABLE_4_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_CR_TABLE_5_FLDMASK                    0xf0000
#define RG_MAC_LT_RBR_CR_TABLE_5_FLDMASK_POS                16
#define RG_MAC_LT_RBR_CR_TABLE_5_FLDMASK_LEN                4

#define DPRX_LT_54 (WRAP_BASE + WRAPPER_0 + 0x54)
#define RG_MAC_LT_HBR_CR_TABLE_1_FLDMASK                    0xf
#define RG_MAC_LT_HBR_CR_TABLE_1_FLDMASK_POS                0
#define RG_MAC_LT_HBR_CR_TABLE_1_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_CR_TABLE_2_FLDMASK                    0xf0
#define RG_MAC_LT_HBR_CR_TABLE_2_FLDMASK_POS                4
#define RG_MAC_LT_HBR_CR_TABLE_2_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_CR_TABLE_3_FLDMASK                    0xf00
#define RG_MAC_LT_HBR_CR_TABLE_3_FLDMASK_POS                8
#define RG_MAC_LT_HBR_CR_TABLE_3_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_CR_TABLE_4_FLDMASK                    0xf000
#define RG_MAC_LT_HBR_CR_TABLE_4_FLDMASK_POS                12
#define RG_MAC_LT_HBR_CR_TABLE_4_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_CR_TABLE_5_FLDMASK                    0xf0000
#define RG_MAC_LT_HBR_CR_TABLE_5_FLDMASK_POS                16
#define RG_MAC_LT_HBR_CR_TABLE_5_FLDMASK_LEN                4

#define DPRX_LT_58 (WRAP_BASE + WRAPPER_0 + 0x58)
#define RG_MAC_LT_HBR2_CR_TABLE_1_FLDMASK                   0xf
#define RG_MAC_LT_HBR2_CR_TABLE_1_FLDMASK_POS               0
#define RG_MAC_LT_HBR2_CR_TABLE_1_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_CR_TABLE_2_FLDMASK                   0xf0
#define RG_MAC_LT_HBR2_CR_TABLE_2_FLDMASK_POS               4
#define RG_MAC_LT_HBR2_CR_TABLE_2_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_CR_TABLE_3_FLDMASK                   0xf00
#define RG_MAC_LT_HBR2_CR_TABLE_3_FLDMASK_POS               8
#define RG_MAC_LT_HBR2_CR_TABLE_3_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_CR_TABLE_4_FLDMASK                   0xf000
#define RG_MAC_LT_HBR2_CR_TABLE_4_FLDMASK_POS               12
#define RG_MAC_LT_HBR2_CR_TABLE_4_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_CR_TABLE_5_FLDMASK                   0xf0000
#define RG_MAC_LT_HBR2_CR_TABLE_5_FLDMASK_POS               16
#define RG_MAC_LT_HBR2_CR_TABLE_5_FLDMASK_LEN               4

#define DPRX_LT_5C (WRAP_BASE + WRAPPER_0 + 0x5C)
#define RG_MAC_LT_HBR3_CR_TABLE_1_FLDMASK                   0xf
#define RG_MAC_LT_HBR3_CR_TABLE_1_FLDMASK_POS               0
#define RG_MAC_LT_HBR3_CR_TABLE_1_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_CR_TABLE_2_FLDMASK                   0xf0
#define RG_MAC_LT_HBR3_CR_TABLE_2_FLDMASK_POS               4
#define RG_MAC_LT_HBR3_CR_TABLE_2_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_CR_TABLE_3_FLDMASK                   0xf00
#define RG_MAC_LT_HBR3_CR_TABLE_3_FLDMASK_POS               8
#define RG_MAC_LT_HBR3_CR_TABLE_3_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_CR_TABLE_4_FLDMASK                   0xf000
#define RG_MAC_LT_HBR3_CR_TABLE_4_FLDMASK_POS               12
#define RG_MAC_LT_HBR3_CR_TABLE_4_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_CR_TABLE_5_FLDMASK                   0xf0000
#define RG_MAC_LT_HBR3_CR_TABLE_5_FLDMASK_POS               16
#define RG_MAC_LT_HBR3_CR_TABLE_5_FLDMASK_LEN               4

#define DPRX_LT_60 (WRAP_BASE + WRAPPER_0 + 0x60)
#define RG_MAC_LT_RBR_EQ_TABLE_1_FLDMASK                    0xf
#define RG_MAC_LT_RBR_EQ_TABLE_1_FLDMASK_POS                0
#define RG_MAC_LT_RBR_EQ_TABLE_1_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_EQ_TABLE_2_FLDMASK                    0xf0
#define RG_MAC_LT_RBR_EQ_TABLE_2_FLDMASK_POS                4
#define RG_MAC_LT_RBR_EQ_TABLE_2_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_EQ_TABLE_3_FLDMASK                    0xf00
#define RG_MAC_LT_RBR_EQ_TABLE_3_FLDMASK_POS                8
#define RG_MAC_LT_RBR_EQ_TABLE_3_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_EQ_TABLE_4_FLDMASK                    0xf000
#define RG_MAC_LT_RBR_EQ_TABLE_4_FLDMASK_POS                12
#define RG_MAC_LT_RBR_EQ_TABLE_4_FLDMASK_LEN                4

#define RG_MAC_LT_RBR_EQ_TABLE_5_FLDMASK                    0xf0000
#define RG_MAC_LT_RBR_EQ_TABLE_5_FLDMASK_POS                16
#define RG_MAC_LT_RBR_EQ_TABLE_5_FLDMASK_LEN                4

#define DPRX_LT_64 (WRAP_BASE + WRAPPER_0 + 0x64)
#define RG_MAC_LT_HBR_EQ_TABLE_1_FLDMASK                    0xf
#define RG_MAC_LT_HBR_EQ_TABLE_1_FLDMASK_POS                0
#define RG_MAC_LT_HBR_EQ_TABLE_1_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_EQ_TABLE_2_FLDMASK                    0xf0
#define RG_MAC_LT_HBR_EQ_TABLE_2_FLDMASK_POS                4
#define RG_MAC_LT_HBR_EQ_TABLE_2_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_EQ_TABLE_3_FLDMASK                    0xf00
#define RG_MAC_LT_HBR_EQ_TABLE_3_FLDMASK_POS                8
#define RG_MAC_LT_HBR_EQ_TABLE_3_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_EQ_TABLE_4_FLDMASK                    0xf000
#define RG_MAC_LT_HBR_EQ_TABLE_4_FLDMASK_POS                12
#define RG_MAC_LT_HBR_EQ_TABLE_4_FLDMASK_LEN                4

#define RG_MAC_LT_HBR_EQ_TABLE_5_FLDMASK                    0xf0000
#define RG_MAC_LT_HBR_EQ_TABLE_5_FLDMASK_POS                16
#define RG_MAC_LT_HBR_EQ_TABLE_5_FLDMASK_LEN                4

#define DPRX_LT_68 (WRAP_BASE + WRAPPER_0 + 0x68)
#define RG_MAC_LT_HBR2_EQ_TABLE_1_FLDMASK                   0xf
#define RG_MAC_LT_HBR2_EQ_TABLE_1_FLDMASK_POS               0
#define RG_MAC_LT_HBR2_EQ_TABLE_1_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_EQ_TABLE_2_FLDMASK                   0xf0
#define RG_MAC_LT_HBR2_EQ_TABLE_2_FLDMASK_POS               4
#define RG_MAC_LT_HBR2_EQ_TABLE_2_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_EQ_TABLE_3_FLDMASK                   0xf00
#define RG_MAC_LT_HBR2_EQ_TABLE_3_FLDMASK_POS               8
#define RG_MAC_LT_HBR2_EQ_TABLE_3_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_EQ_TABLE_4_FLDMASK                   0xf000
#define RG_MAC_LT_HBR2_EQ_TABLE_4_FLDMASK_POS               12
#define RG_MAC_LT_HBR2_EQ_TABLE_4_FLDMASK_LEN               4

#define RG_MAC_LT_HBR2_EQ_TABLE_5_FLDMASK                   0xf0000
#define RG_MAC_LT_HBR2_EQ_TABLE_5_FLDMASK_POS               16
#define RG_MAC_LT_HBR2_EQ_TABLE_5_FLDMASK_LEN               4

#define DPRX_LT_6C (WRAP_BASE + WRAPPER_0 + 0x6C)
#define RG_MAC_LT_HBR3_EQ_TABLE_1_FLDMASK                   0xf
#define RG_MAC_LT_HBR3_EQ_TABLE_1_FLDMASK_POS               0
#define RG_MAC_LT_HBR3_EQ_TABLE_1_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_EQ_TABLE_2_FLDMASK                   0xf0
#define RG_MAC_LT_HBR3_EQ_TABLE_2_FLDMASK_POS               4
#define RG_MAC_LT_HBR3_EQ_TABLE_2_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_EQ_TABLE_3_FLDMASK                   0xf00
#define RG_MAC_LT_HBR3_EQ_TABLE_3_FLDMASK_POS               8
#define RG_MAC_LT_HBR3_EQ_TABLE_3_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_EQ_TABLE_4_FLDMASK                   0xf000
#define RG_MAC_LT_HBR3_EQ_TABLE_4_FLDMASK_POS               12
#define RG_MAC_LT_HBR3_EQ_TABLE_4_FLDMASK_LEN               4

#define RG_MAC_LT_HBR3_EQ_TABLE_5_FLDMASK                   0xf0000
#define RG_MAC_LT_HBR3_EQ_TABLE_5_FLDMASK_POS               16
#define RG_MAC_LT_HBR3_EQ_TABLE_5_FLDMASK_LEN               4

#define DPRX_MISC_74 (WRAP_BASE + WRAPPER_0 + 0x74)
#define RG_MAC_LOGIC_RST_B_FLDMASK                          0x1
#define RG_MAC_LOGIC_RST_B_FLDMASK_POS                      0
#define RG_MAC_LOGIC_RST_B_FLDMASK_LEN                      1

#define DPRX_AUD_MUTE_84 (WRAP_BASE + WRAPPER_0 + 0x84)
#define RG_MAC_AUD_UNMUTE_CNT_EN_FLDMASK                    0x1
#define RG_MAC_AUD_UNMUTE_CNT_EN_FLDMASK_POS                0
#define RG_MAC_AUD_UNMUTE_CNT_EN_FLDMASK_LEN                1

#define RG_MAC_AUD_UNMUTE_CNT_SEL_FLDMASK                   0xffff0000L
#define RG_MAC_AUD_UNMUTE_CNT_SEL_FLDMASK_POS               16
#define RG_MAC_AUD_UNMUTE_CNT_SEL_FLDMASK_LEN               16

#define DPRX_DDDS_8C (WRAP_BASE + WRAPPER_0 + 0x8C)
#define RG_MAC_DDDS_VIDEO_HS_SOURCE_FLDMASK                 0x1
#define RG_MAC_DDDS_VIDEO_HS_SOURCE_FLDMASK_POS             0
#define RG_MAC_DDDS_VIDEO_HS_SOURCE_FLDMASK_LEN             1

#define RG_MAC_DDDS_DSC_HS_SOURCE_FLDMASK                   0x2
#define RG_MAC_DDDS_DSC_HS_SOURCE_FLDMASK_POS               1
#define RG_MAC_DDDS_DSC_HS_SOURCE_FLDMASK_LEN               1

#define RG_MAC_DDDS_VIDEO_HS_MODE_FLDMASK                   0x4
#define RG_MAC_DDDS_VIDEO_HS_MODE_FLDMASK_POS               2
#define RG_MAC_DDDS_VIDEO_HS_MODE_FLDMASK_LEN               1

#define RG_MAC_DDDS_VIDEO_VS_MODE_FLDMASK                   0x8
#define RG_MAC_DDDS_VIDEO_VS_MODE_FLDMASK_POS               3
#define RG_MAC_DDDS_VIDEO_VS_MODE_FLDMASK_LEN               1

#define RG_MAC_DDDS_DSC_HS_MODE_FLDMASK                     0x10
#define RG_MAC_DDDS_DSC_HS_MODE_FLDMASK_POS                 4
#define RG_MAC_DDDS_DSC_HS_MODE_FLDMASK_LEN                 1

#define RG_MAC_DDDS_DSC_VS_MODE_FLDMASK                     0x20
#define RG_MAC_DDDS_DSC_VS_MODE_FLDMASK_POS                 5
#define RG_MAC_DDDS_DSC_VS_MODE_FLDMASK_LEN                 1

#define DPRX_VHSYNC_90 (WRAP_BASE + WRAPPER_0 + 0x90)
#define RG_MAC_RT_VID_CNT_EN_FLDMASK                        0x1
#define RG_MAC_RT_VID_CNT_EN_FLDMASK_POS                    0
#define RG_MAC_RT_VID_CNT_EN_FLDMASK_LEN                    1

#define RG_MAC_RT_VID_H_GEN_EN_FLDMASK                      0x2
#define RG_MAC_RT_VID_H_GEN_EN_FLDMASK_POS                  1
#define RG_MAC_RT_VID_H_GEN_EN_FLDMASK_LEN                  1

#define RG_MAC_RT_VID_VSYNC_INT_EN_FLDMASK                  0x4
#define RG_MAC_RT_VID_VSYNC_INT_EN_FLDMASK_POS              2
#define RG_MAC_RT_VID_VSYNC_INT_EN_FLDMASK_LEN              1

#define RG_MAC_RT_VID_VSYNC_INT_CLR_FLDMASK                 0x8
#define RG_MAC_RT_VID_VSYNC_INT_CLR_FLDMASK_POS             3
#define RG_MAC_RT_VID_VSYNC_INT_CLR_FLDMASK_LEN             1

#define RG_MAC_RT_VID_VSYNC_EXCEED_INT_EN_FLDMASK           0x10
#define RG_MAC_RT_VID_VSYNC_EXCEED_INT_EN_FLDMASK_POS       4
#define RG_MAC_RT_VID_VSYNC_EXCEED_INT_EN_FLDMASK_LEN       1

#define RG_MAC_RT_VID_VSYNC_EXCEED_INT_CLR_FLDMASK          0x20
#define RG_MAC_RT_VID_VSYNC_EXCEED_INT_CLR_FLDMASK_POS      5
#define RG_MAC_RT_VID_VSYNC_EXCEED_INT_CLR_FLDMASK_LEN      1

#define RG_MAC_RT_VID_SKIP_1ST_VSYNC_FLDMASK                0x40
#define RG_MAC_RT_VID_SKIP_1ST_VSYNC_FLDMASK_POS            6
#define RG_MAC_RT_VID_SKIP_1ST_VSYNC_FLDMASK_LEN            1

#define RG_MAC_RT_VID_RESERVE_FLDMASK                       0xffff0000L
#define RG_MAC_RT_VID_RESERVE_FLDMASK_POS                   16
#define RG_MAC_RT_VID_RESERVE_FLDMASK_LEN                   16

#define DPRX_VHSYNC_94 (WRAP_BASE + WRAPPER_0 + 0x94)
#define RG_MAC_RT_VID_H_TOTAL_FLDMASK                       0x1fff
#define RG_MAC_RT_VID_H_TOTAL_FLDMASK_POS                   0
#define RG_MAC_RT_VID_H_TOTAL_FLDMASK_LEN                   13

#define RG_MAC_RT_VID_V_TOTAL_FLDMASK                       0x1fff0000
#define RG_MAC_RT_VID_V_TOTAL_FLDMASK_POS                   16
#define RG_MAC_RT_VID_V_TOTAL_FLDMASK_LEN                   13

#define DPRX_VHSYNC_98 (WRAP_BASE + WRAPPER_0 + 0x98)
#define RG_MAC_RT_VID_REFINE_NUM_FLDMASK                    0x1fff
#define RG_MAC_RT_VID_REFINE_NUM_FLDMASK_POS                0
#define RG_MAC_RT_VID_REFINE_NUM_FLDMASK_LEN                13

#define RG_MAC_RT_VID_REFINE_DIR_FLDMASK                    0x10000
#define RG_MAC_RT_VID_REFINE_DIR_FLDMASK_POS                16
#define RG_MAC_RT_VID_REFINE_DIR_FLDMASK_LEN                1

#define DPRX_VHSYNC_9C (WRAP_BASE + WRAPPER_0 + 0x9C)
#define RG_MAC_RT_VID_CNT_FLDMASK                           0xffffff
#define RG_MAC_RT_VID_CNT_FLDMASK_POS                       0
#define RG_MAC_RT_VID_CNT_FLDMASK_LEN                       24

#define RG_MAC_RT_VID_CNT_TH_FLDMASK                        0xff000000L
#define RG_MAC_RT_VID_CNT_TH_FLDMASK_POS                    24
#define RG_MAC_RT_VID_CNT_TH_FLDMASK_LEN                    8

#define DPRX_RO_A0 (WRAP_BASE + WRAPPER_0 + 0xA0)
#define RGS_MAC_WRAP_CFG1_FLDMASK                           0x1
#define RGS_MAC_WRAP_CFG1_FLDMASK_POS                       0
#define RGS_MAC_WRAP_CFG1_FLDMASK_LEN                       1

#define DPRX_VHSYNC_D0 (WRAP_BASE + WRAPPER_0 + 0xD0)
#define RG_MAC_RT_DSC_CNT_EN_FLDMASK                        0x1
#define RG_MAC_RT_DSC_CNT_EN_FLDMASK_POS                    0
#define RG_MAC_RT_DSC_CNT_EN_FLDMASK_LEN                    1

#define RG_MAC_RT_DSC_H_GEN_EN_FLDMASK                      0x2
#define RG_MAC_RT_DSC_H_GEN_EN_FLDMASK_POS                  1
#define RG_MAC_RT_DSC_H_GEN_EN_FLDMASK_LEN                  1

#define RG_MAC_RT_DSC_VSYNC_INT_EN_FLDMASK                  0x4
#define RG_MAC_RT_DSC_VSYNC_INT_EN_FLDMASK_POS              2
#define RG_MAC_RT_DSC_VSYNC_INT_EN_FLDMASK_LEN              1

#define RG_MAC_RT_DSC_VSYNC_INT_CLR_FLDMASK                 0x8
#define RG_MAC_RT_DSC_VSYNC_INT_CLR_FLDMASK_POS             3
#define RG_MAC_RT_DSC_VSYNC_INT_CLR_FLDMASK_LEN             1

#define RG_MAC_RT_DSC_VSYNC_EXCEED_INT_EN_FLDMASK           0x10
#define RG_MAC_RT_DSC_VSYNC_EXCEED_INT_EN_FLDMASK_POS       4
#define RG_MAC_RT_DSC_VSYNC_EXCEED_INT_EN_FLDMASK_LEN       1

#define RG_MAC_RT_DSC_VSYNC_EXCEED_INT_CLR_FLDMASK          0x20
#define RG_MAC_RT_DSC_VSYNC_EXCEED_INT_CLR_FLDMASK_POS      5
#define RG_MAC_RT_DSC_VSYNC_EXCEED_INT_CLR_FLDMASK_LEN      1

#define RG_MAC_RT_DSC_SKIP_1ST_VSYNC_FLDMASK                0x40
#define RG_MAC_RT_DSC_SKIP_1ST_VSYNC_FLDMASK_POS            6
#define RG_MAC_RT_DSC_SKIP_1ST_VSYNC_FLDMASK_LEN            1

#define RG_MAC_RT_DSC_RESERVE_FLDMASK                       0xffff0000L
#define RG_MAC_RT_DSC_RESERVE_FLDMASK_POS                   16
#define RG_MAC_RT_DSC_RESERVE_FLDMASK_LEN                   16

#define DPRX_VHSYNC_D4 (WRAP_BASE + WRAPPER_0 + 0xD4)
#define RG_MAC_RT_DSC_H_TOTAL_FLDMASK                       0x1fff
#define RG_MAC_RT_DSC_H_TOTAL_FLDMASK_POS                   0
#define RG_MAC_RT_DSC_H_TOTAL_FLDMASK_LEN                   13

#define RG_MAC_RT_DSC_V_TOTAL_FLDMASK                       0x1fff0000
#define RG_MAC_RT_DSC_V_TOTAL_FLDMASK_POS                   16
#define RG_MAC_RT_DSC_V_TOTAL_FLDMASK_LEN                   13

#define DPRX_VHSYNC_D8 (WRAP_BASE + WRAPPER_0 + 0xD8)
#define RG_MAC_RT_DSC_REFINE_NUM_FLDMASK                    0x1fff
#define RG_MAC_RT_DSC_REFINE_NUM_FLDMASK_POS                0
#define RG_MAC_RT_DSC_REFINE_NUM_FLDMASK_LEN                13

#define RG_MAC_RT_DSC_REFINE_DIR_FLDMASK                    0x10000
#define RG_MAC_RT_DSC_REFINE_DIR_FLDMASK_POS                16
#define RG_MAC_RT_DSC_REFINE_DIR_FLDMASK_LEN                1

#define DPRX_VHSYNC_DC (WRAP_BASE + WRAPPER_0 + 0xDC)
#define RG_MAC_RT_DSC_CNT_FLDMASK                           0xffffff
#define RG_MAC_RT_DSC_CNT_FLDMASK_POS                       0
#define RG_MAC_RT_DSC_CNT_FLDMASK_LEN                       24

#define RG_MAC_RT_DSC_CNT_TH_FLDMASK                        0xff000000L
#define RG_MAC_RT_DSC_CNT_TH_FLDMASK_POS                    24
#define RG_MAC_RT_DSC_CNT_TH_FLDMASK_LEN                    8

#define DPRX_VHSYNC_RO_E0 (WRAP_BASE + WRAPPER_0 + 0xE0)
#define RGS_MAC_RT_VID_CNT_FLDMASK                          0xffffff
#define RGS_MAC_RT_VID_CNT_FLDMASK_POS                      0
#define RGS_MAC_RT_VID_CNT_FLDMASK_LEN                      24

#define RGS_MAC_RT_VID_VSYNC_INT_FLDMASK                    0x1000000
#define RGS_MAC_RT_VID_VSYNC_INT_FLDMASK_POS                24
#define RGS_MAC_RT_VID_VSYNC_INT_FLDMASK_LEN                1

#define RGS_MAC_RT_VID_VSYNC_EXCEED_INT_FLDMASK             0x2000000
#define RGS_MAC_RT_VID_VSYNC_EXCEED_INT_FLDMASK_POS         25
#define RGS_MAC_RT_VID_VSYNC_EXCEED_INT_FLDMASK_LEN         1

#define DPRX_VHSYNC_RO_E4 (WRAP_BASE + WRAPPER_0 + 0xE4)
#define RGS_MAC_RT_DSC_CNT_FLDMASK                          0xffffff
#define RGS_MAC_RT_DSC_CNT_FLDMASK_POS                      0
#define RGS_MAC_RT_DSC_CNT_FLDMASK_LEN                      24

#define RGS_MAC_RT_DSC_VSYNC_INT_FLDMASK                    0x1000000
#define RGS_MAC_RT_DSC_VSYNC_INT_FLDMASK_POS                24
#define RGS_MAC_RT_DSC_VSYNC_INT_FLDMASK_LEN                1

#define RGS_MAC_RT_DSC_VSYNC_EXCEED_INT_FLDMASK             0x2000000
#define RGS_MAC_RT_DSC_VSYNC_EXCEED_INT_FLDMASK_POS         25
#define RGS_MAC_RT_DSC_VSYNC_EXCEED_INT_FLDMASK_LEN         1

#define DPRX_MBIST_9C (WRAP_BASE + WRAPPER_1 + 0x9C)
#define RG_MAC_HDCP22_SETTING_DONE_FLDMASK                  0x100
#define RG_MAC_HDCP22_SETTING_DONE_FLDMASK_POS              8
#define RG_MAC_HDCP22_SETTING_DONE_FLDMASK_LEN              1

#define LINK_BW_SET_GCE      (0x24000000 + ADDR_LINK_BW_SET)
#define DPRX_WRAPCTL_28_GCE  (0x11E70000 + DPRX_WRAPCTL_28)
#define TRAINING_PATTERN_GCE (0x24000000 + ADDR_TRAINING_PATTERN_SET)
#define DPRX_GCE_STATUS      (0x24000000 + ADDR_RX_GTC_PHASE_SKEW_OFFSET_0)
#define DPRX_DIG_GLB_10      0x11E60010
#define DPRX_DIG_GLB_18      0x11E60018
#define DPRX_DIG_GLB_28      0x11E60028
#define DPRX_DIG_L0_TRX_24   0x11E64024
#define DPRX_DIG_L1_TRX_24   0x11E64124
#define DPRX_DIG_L2_TRX_24   0x11E64224
#define DPRX_DIG_L3_TRX_24   0x11E64324
#define DPRX_DIG_L0_TRX_38   0x11E64038
#define DPRX_DIG_L1_TRX_38   0x11E64138
#define DPRX_DIG_L2_TRX_38   0x11E64238
#define DPRX_DIG_L3_TRX_38   0x11E64338
#define DPRX_DIG_L0_RX_30    0x11E65030
#define DPRX_DIG_L1_RX_30    0x11E65130
#define DPRX_DIG_L2_RX_30    0x11E65230
#define DPRX_DIG_L3_RX_30    0x11E65330
#define DPRX_DIG_L0_RX2_34   0x11E66034
#define DPRX_DIG_L1_RX2_34   0x11E66134
#define DPRX_DIG_L2_RX2_34   0x11E66234
#define DPRX_DIG_L3_RX2_34   0x11E66334
#define DPRX_ANA_L0_TRX_34   0x11E6A034
#define DPRX_ANA_L1_TRX_34   0x11E6A134
#define DPRX_ANA_L2_TRX_34   0x11E6A234
#define DPRX_ANA_L3_TRX_34   0x11E6A334
#define DPRX_ANA_L0_TRX_60   0x11E6A060
#define DPRX_ANA_L1_TRX_60   0x11E6A160
#define DPRX_ANA_L2_TRX_60   0x11E6A260
#define DPRX_ANA_L3_TRX_60   0x11E6A360
#define DPRX_ANA_L0_TRX_64   0x11E6A064
#define DPRX_ANA_L1_TRX_64   0x11E6A164
#define DPRX_ANA_L2_TRX_64   0x11E6A264
#define DPRX_ANA_L3_TRX_64   0x11E6A364
#define DPRX_ANA_L0_TRX_70   0x11E6A070
#define DPRX_ANA_L1_TRX_70   0x11E6A170
#define DPRX_ANA_L2_TRX_70   0x11E6A270
#define DPRX_ANA_L3_TRX_70   0x11E6A370
#define DPRX_ANA_L0_TRX_84   0x11E6A084
#define DPRX_ANA_L1_TRX_84   0x11E6A184
#define DPRX_ANA_L2_TRX_84   0x11E6A284
#define DPRX_ANA_L3_TRX_84   0x11E6A384

/** @}
 */

#endif /*__MTK_DRRX_REG_H__*/
