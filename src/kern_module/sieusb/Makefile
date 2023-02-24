# SIE CONFIDENTIAL
#
#   Copyright (C) 2021 Sony Interactive Entertainment Inc.
#                      All Rights Reserved.
#

TOP_PATH             := $(realpath $(CURDIR)/../../..)
include $(TOP_PATH)/build/common.mk

LINUX_PATH           := $(CUR_BUILD_OUT_PATH)/linux
TARGET               := sieusb
OUT                  := $(CUR_BUILD_OUT_PATH)/kern_module/$(TARGET)
ifeq ($(SIE_BUILD_TYPE), develop)
INSTALL_NFS          := $(NFS_ROOT)/sie/module/$(TARGET).ko
endif

obj-m                := sieusb.o
$(TARGET)-y          := usb.o
$(TARGET)-y          += util.o
$(TARGET)-y          += authentication.o
$(TARGET)-y          += audio_ctrl.o
$(TARGET)-y          += command.o
$(TARGET)-y          += mic.o
$(TARGET)-y          += status.o
$(TARGET)-y          += data.o
$(TARGET)-y          += bulkintr.o
$(TARGET)-y          += gbuf.o
ifneq ($(CONFIG_USB_MASS_STORAGE),)
$(TARGET)-y          += mass_storage.o
ccflags-y            += -I$(LINUX_KERNEL_PATH)/drivers/usb/gadget/function
endif

ccflags-y            += -I$(PWD)/../include
CFLAGS_MODULE        += -Werror
MAKE_ARG             := ARCH=arm64 O=$(LINUX_PATH) CROSS_COMPILE="$(CCACHE) $(ARM64_COMPILE_PREFIX)"
EXTRA_SYMBOLS        := $(OUT)/../sieaudio/Module.symvers $(OUT)/../sieimu/Module.symvers

all: $(INSTALL_NFS) $(OUT)/$(TARGET).ko.strip

$(INSTALL_NFS): $(OUT)/$(TARGET).ko
	@echo INSTALL $@
	@mkdir -p $(dir $@)
	@install -m 744 $(OUT)/$(TARGET).ko $(dir $@)

$(OUT)/$(TARGET).ko.strip: $(OUT)/$(TARGET).ko
	@$(ARM64_COMPILE_PREFIX)strip -s --strip-unneeded $< -o $@

$(OUT)/$(TARGET).ko: touch-makefile
	@$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(OUT) src=$$PWD KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(MAKE_ARG)

touch-makefile:
	@mkdir -p $(OUT)
	@touch $(OUT)/Makefile

clean:
	@rm -fr $(OUT) $(INSTALL_NFS)

.PHONY : all touch-makefile clean
