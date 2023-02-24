# SIE CONFIDENTIAL
#
#   Copyright (C) 2019 Sony Interactive Entertainment Inc.
#                      All Rights Reserved.
#

TOP_PATH             := $(realpath $(CURDIR)/../../..)
include $(TOP_PATH)/build/common.mk

LINUX_PATH           := $(CUR_BUILD_OUT_PATH)/linux
TARGET               := mtk_wrapper
OUT                  := $(CUR_BUILD_OUT_PATH)/kern_module/$(TARGET)
ifeq ($(SIE_BUILD_TYPE), develop)
INSTALL_NFS          := $(NFS_ROOT)/sie/module/$(TARGET).ko
endif

CFLAGS_MODULE       += -DCV_CMDQ_ENABLE

ifeq ($(CONFIG_MACH_MT3612EP),)
# Temporary define. Please delete after MTK will define.
CONFIG_MACH_MT3612  := y
CFLAGS_MODULE       += -DCONFIG_MACH_MT3612
endif

ifeq ($(SUPPORT_HDR), true)
CFLAGS_MODULE       += -DSUPPORT_HDR
endif

obj-m                := mtk_wrapper.o
$(TARGET)-y          := mtk_wrapper_main.o
$(TARGET)-y          += mtk_wrapper_dprx.o
$(TARGET)-y          += mtk_wrapper_util.o
$(TARGET)-y          += mtk_wrapper_dsi.o
$(TARGET)-y          += mtk_wrapper_dscenc.o
$(TARGET)-y          += mtk_wrapper_resizer.o
$(TARGET)-y          += mtk_wrapper_cv.o
$(TARGET)-y          += mtk_wrapper_thermal.o

$(TARGET)-$(CONFIG_MACH_MT3612)   += cv/mtk_cv_common.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += cv/mtk_fm_dev.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += cv/mtk_warpa_fe.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_mutex.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_slicer.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_mmsyscfg.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_rdma.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_wdma.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_crop.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_p2s.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_lhc.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_ts.o
$(TARGET)-$(CONFIG_MACH_MT3612)   += mtk_wrapper_gce.o

ccflags-y            += -I$(PWD)/../include
CFLAGS_MODULE        += -Werror
MAKE_ARG             := ARCH=arm64 O=$(LINUX_PATH) CROSS_COMPILE="$(CCACHE) $(ARM64_COMPILE_PREFIX)"

ifeq ($(SIE_BUILD_TYPE),release)
  CFLAGS_MODULE += -DCONFIG_MTK_WRAPPER_RELEASE
else ifeq ($(SIE_BUILD_TYPE),factory)
  CFLAGS_MODULE += -DCONFIG_MTK_WRAPPER_FACTORY
else ifeq ($(SIE_BUILD_TYPE),develop)
  CFLAGS_MODULE += -DCONFIG_MTK_WRAPPER_DEBUG
  $(TARGET)-$(CONFIG_DEBUG_FS)      += debugfs/mtk_wrapper_debugfs.o
  $(TARGET)-$(CONFIG_DEBUG_FS)      += debugfs/mtk_wrapper_slicer_debugfs.o
else
  $(error Not found current_build_type. Please Do "make setup" at TOP directory.)
endif
ifeq ($(SIE_GAZE_DEBUG_BUILD), 1)
  CFLAGS_MODULE += -DCONFIG_MTK_WRAPPER_GAZE_DEBUG
endif

all: $(INSTALL_NFS) $(OUT)/$(TARGET).ko.strip

$(INSTALL_NFS): $(OUT)/$(TARGET).ko
	@echo INSTALL $@
	@mkdir -p $(dir $@)
	@install -m 744 $(OUT)/$(TARGET).ko $(dir $@)

$(OUT)/$(TARGET).ko.strip: $(OUT)/$(TARGET).ko
	@$(ARM64_COMPILE_PREFIX)strip -s --strip-unneeded $< -o $@

$(OUT)/$(TARGET).ko: touch-makefile
	@$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(OUT) src=$$PWD $(MAKE_ARG)

touch-makefile:
	@mkdir -p $(OUT)
	@touch $(OUT)/Makefile

clean:
	@rm -fr $(OUT) $(INSTALL_NFS)

.PHONY : all touch-makefile clean

