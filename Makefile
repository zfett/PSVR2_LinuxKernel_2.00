# SIE CONFIDENTIAL
#
#   Copyright (C) 2020 Sony Interactive Entertainment Inc.
#                      All Rights Reserved.
#

TOP_PATH             := $(CURDIR)
include $(TOP_PATH)/build/common.mk

OUT                  := $(TOP_PATH)/out/$(SIE_BUILD_TYPE)/linux
LINUX_KERNEL_PATH    := $(TOP_PATH)/src/linux
KERN_IMAGE           := $(OUT)/arch/arm64/Image

# FIXME
KBUILD_DEFCONFIG     := sie_$(SIE_BUILD_TYPE)_mt3612_asic_a0_defconfig

MAKE_ARG             := ARCH=arm64 O=$(OUT) CROSS_COMPILE="$(CCACHE) $(ARM64_COMPILE_PREFIX)" KBUILD_IMAGE=Image

all: kern_mod

kern_mod: $(KERN_IMAGE)
	@$(MAKE) -C ./src/kern_module/mtk_wrapper all
	@$(MAKE) -C ./src/kern_module/log all
	@$(MAKE) -C ./src/kern_module/fts all
	@$(MAKE) -C ./src/kern_module/sieaudio all
	@$(MAKE) -C ./src/kern_module/sieimu all
	@$(MAKE) -C ./src/kern_module/sieusb all

$(KERN_IMAGE): $(OUT)/.config
	@$(MAKE) -C $(LINUX_KERNEL_PATH) $(MAKE_ARG) all

$(OUT)/.config:
	@$(MAKE) -C $(LINUX_KERNEL_PATH) $(MAKE_ARG) $(KBUILD_DEFCONFIG)

clean:
	@$(RM) -fr $(TOP_PATH)/out

.PHONY : all kern_mod clean

