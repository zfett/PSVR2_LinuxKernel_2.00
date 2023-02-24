# SIE CONFIDENTIAL
#
#   Copyright (C) 2019 Sony Interactive Entertainment Inc.
#                      All Rights Reserved.
#

ARM64_COMPILE_PREFIX ?= $(realpath $(TOP_PATH)/../../..)/host_tools/bin/aarch64-poky-linux/aarch64-poky-linux-
#SIE_BUILD_TYPE       ?= factory
SIE_BUILD_TYPE       ?= release

CUR_BUILD_OUT_PATH   := $(TOP_PATH)/out/$(SIE_BUILD_TYPE)
NFS_ROOT             := $(TOP_PATH)/out/nfsroot
LINUX_KERNEL_PATH    := $(TOP_PATH)/src/linux

