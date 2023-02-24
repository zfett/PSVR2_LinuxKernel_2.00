# SIE CONFIDENTIAL
#
#   Copyright (C) 2019 Sony Interactive Entertainment Inc.
#                      All Rights Reserved.
#

TOP_PATH             := $(realpath $(CURDIR)/../../..)
include $(TOP_PATH)/build/common.mk

LINUX_PATH           := $(CUR_BUILD_OUT_PATH)/linux
TARGET               := log
OUT                  := $(CUR_BUILD_OUT_PATH)/kern_module/$(TARGET)
ifeq ($(SIE_BUILD_TYPE), develop)
INSTALL_NFS          := $(NFS_ROOT)/sie/module/$(TARGET).ko
endif

obj-m                := log.o
$(TARGET)-y          := log_main.o

ccflags-y            += -I$(PWD)/../include
CFLAGS_MODULE        += -Werror
MAKE_ARG             := ARCH=arm64 O=$(LINUX_PATH) CROSS_COMPILE="$(CCACHE) $(ARM64_COMPILE_PREFIX)"

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
	@rm -fr $(OUT) $(INSTALL_RAMFS) $(INSTALL_NFS)

.PHONY : all touch-makefile clean

