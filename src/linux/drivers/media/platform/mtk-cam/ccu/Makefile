# Makefile for Mediatek CCU driver

ccflags-y += -I$(srctree)/drivers/media/platform/mtk-ccu

mtk-ccu-objs += ccu-dev.o
mtk-ccu-objs += ccu-hw.o
mtk-ccu-objs += ccu-util.o
mtk-ccu-objs += ccu-mailbox.o

obj-$(CONFIG_VIDEO_MEDIATEK_CCU) += mtk-ccu.o

