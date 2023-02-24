#
# Copyright (C) 2015 MediaTek Inc.
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

obj-$(CONFIG_MTK_DPRX) += mtk_dprx_module.o

mtk_dprx_module-objs := mtk_dprx_drv.o mtk_dprx_os.o mtk_dprx_isr.o mtk_dprx_avc.o mtk_dprx_if.o
ifneq ($(CONFIG_DEBUG_FS),)
	mtk_dprx_module-objs += mtk_dprx_cmd.o
endif
