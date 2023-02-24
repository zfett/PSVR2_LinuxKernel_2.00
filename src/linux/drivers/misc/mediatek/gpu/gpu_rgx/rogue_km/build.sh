#!/bin/sh
WORKSPACE_PATH=/mfs/mtkslt0207/mtk10337/3612-main
export KERNELDIR=$WORKSPACE_PATH/build/tmp/work/mt3612_fpga-poky-linux/linux-mtk-a64/4.4-r0/linux-mtk-a64-4.4/
export ARCH=arm64
export MIPS_ELF_ROOT=/site/mtkoss/Codescape_MIPS_SDK/Toolchains/mips-mti-elf/2014.07-1
export CROSS_COMPILE=$WORKSPACE_PATH/build/tmp/work/aarch64-poky-linux/gcc-runtime/7.3.0-r0/recipe-sysroot-native/usr/bin/aarch64-poky-linux/aarch64-poky-linux-
export SYSROOT=$WORKSPACE_PATH/build/tmp/work/aarch64-poky-linux/gcc-runtime/7.3.0-r0/recipe-sysroot

export PVR_BUILD_DIR=3612_linux
#export PVR_BUILD_DIR=nohw_linux
#export WINDOW_SYSTEM=nullws
#export PDUMP=1
#export RGX_BVNC=22.93.208.318
#export DBGLINK=1
export RGX_BVNC=22.49.21.16
mosesq make V=1 $2 -f Makefile.standalone BUILD=debug 2>&1 | tee make_km.log
export OUTDIR=binary_*/target_aarch64
${CROSS_COMPILE}strip --strip-debug  --strip-unneeded  $OUTDIR/*.ko
