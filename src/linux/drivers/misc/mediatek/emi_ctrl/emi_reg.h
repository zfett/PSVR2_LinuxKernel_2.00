#ifndef __EMI_REG_REG_H__
#define __EMI_REG_REG_H__

/* ----------------- Register Definitions ------------------- */
#define EMI_ARBA					0x00000100
	#define M0_BANDWIDTH				GENMASK(5, 0)
	#define M0_ULTRA_DOM_BW				BIT(6)
	#define M0_DOUBLE_HALF_BW			GENMASK(9, 8)
	#define M0_BW_FILTER_LEN			GENMASK(11, 10)
	#define M0_BW_FILTER_EN				BIT(12)
	#define M0_PRE_ULTRA_DOM_BW			BIT(13)
	#define M0_MODE					BIT(14)
	#define M0_RD_LATENCY				GENMASK(23, 16)
	#define M0_WR_LATENCY				GENMASK(31, 24)
#define EMI_ARBC					0x00000110
	#define M2_BANDWIDTH				GENMASK(5, 0)
	#define M2_ULTRA_DOM_BW				BIT(6)
	#define M2_BW_FILTER_EN				BIT(12)
	#define M2_PRE_ULTRA_DOM_BW			BIT(13)
	#define M2_MODE					BIT(14)
	#define M2_RD_LATENCY				GENMASK(23, 16)
	#define M2_WR_LATENCY				GENMASK(31, 24)
#define EMI_ARBD					0x00000118
	#define M3_BANDWIDTH				GENMASK(5, 0)
	#define M3_ULTRA_DOM_BW				BIT(6)
	#define M3_BW_FILTER_EN				BIT(12)
	#define M3_PRE_ULTRA_DOM_BW			BIT(13)
	#define M3_MODE					BIT(14)
	#define M3_RD_LATENCY				GENMASK(23, 16)
	#define M3_WR_LATENCY				GENMASK(31, 24)
#define EMI_ARBE					0x00000120
	#define M4_BANDWIDTH				GENMASK(5, 0)
	#define M4_ULTRA_DOM_BW				BIT(6)
	#define M4_BW_FILTER_EN				BIT(12)
	#define M4_PRE_ULTRA_DOM_BW			BIT(13)
	#define M4_MODE					BIT(14)
	#define M4_RD_LATENCY				GENMASK(23, 16)
	#define M4_WD_LATENCY				GENMASK(31, 24)
#define EMI_ARBF					0x00000128
	#define M5_BANDWIDTH				GENMASK(5, 0)
	#define M5_ULTRA_DOM_BW				BIT(6)
	#define M5_BW_FILTER_EN				BIT(12)
	#define M5_PRE_ULTRA_DOM_BW			BIT(13)
	#define M5_MODE					BIT(14)
	#define M5_RD_LATENCY				GENMASK(23, 16)
	#define M5_WR_LATENCY				GENMASK(31, 24)
#define EMI_ARBG					0x00000130
	#define M6_BANDWIDTH				GENMASK(5, 0)
	#define M6_ULTRA_DOM_BW				BIT(6)
	#define M6_BW_FILTER_EN				BIT(12)
	#define M6_PRE_ULTRA_DOM_BW			BIT(13)
	#define M6_MODE					BIT(14)
	#define M6_RD_LATENCY				GENMASK(23, 16)
	#define M6_WR_LATENCY				GENMASK(31, 24)
#define EMI_ARBH					0x00000138
	#define M7_BANDWIDTH				GENMASK(5, 0)
	#define M7_ULTRA_DOM_BW				BIT(6)
	#define M7_BW_FILTER_EN				BIT(12)
	#define M7_PRE_ULTRA_DOM_BW			BIT(13)
	#define M7_MODE					BIT(14)
	#define M7_RD_LATENCY				GENMASK(23, 16)
	#define M7_WR_LATENCY				GENMASK(31, 24)

#endif /*__EMI_REG_REG_H__*/
