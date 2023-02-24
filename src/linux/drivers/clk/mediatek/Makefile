obj-y += clk-mtk.o clk-pll.o clk-gate.o clk-apmixed.o
obj-$(CONFIG_RESET_CONTROLLER) += reset.o
obj-$(CONFIG_COMMON_CLK_MT3612) += clk-mt3612.o clk-mt3612-topon.o
ifeq ($(CONFIG_COMMON_CLK_MT3612), y)
obj-m += clk-bringup.o
endif
obj-$(CONFIG_MTK_FREQ_METER_VERIFY) += clk-freq-meter.o
