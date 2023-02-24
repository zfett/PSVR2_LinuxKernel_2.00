#
# Copyright (C) 2018 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

# Makefile for Mediatek ISP driver

ccflags-y += -I$(srctree)/drivers/media/platform/mtk-isp

mtk-isp-objs += isp-dev.o isp-util.o isp-modules/isp-module-base.o
mtk-isp-module-cam-objs += isp-modules/isp-module-cam.o
mtk-isp-module-camsv-objs += isp-modules/isp-module-camsv.o
mtk-isp-module-dip-objs += isp-modules/isp-module-dip.o
mtk-isp-module-rbfc-objs += isp-modules/isp-module-rbfc.o
mtk-isp-module-seninf-objs += isp-modules/isp-module-seninf.o
mtk-isp-module-be-objs += isp-modules/isp-module-be.o

obj-$(CONFIG_VIDEO_MEDIATEK_ISP) += mtk-isp.o
obj-$(CONFIG_VIDEO_MEDIATEK_ISP) += mtk-isp-module-cam.o
obj-$(CONFIG_VIDEO_MEDIATEK_ISP) += mtk-isp-module-camsv.o
obj-$(CONFIG_VIDEO_MEDIATEK_ISP) += mtk-isp-module-dip.o
obj-$(CONFIG_VIDEO_MEDIATEK_ISP) += mtk-isp-module-rbfc.o
obj-$(CONFIG_VIDEO_MEDIATEK_ISP) += mtk-isp-module-seninf.o
obj-$(CONFIG_VIDEO_MEDIATEK_ISP) += mtk-isp-module-be.o
