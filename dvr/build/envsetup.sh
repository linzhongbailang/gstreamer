#!/bin/bash

##
##
##COMPILE tools
##
##
#PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-
PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
AS=${PREFIX}gcc
CC=${PREFIX}gcc
CXX=${PREFIX}g++
LD=${PREFIX}ld
AR=${PREFIX}ar
RANLIB=${PREFIX}ranlib
SIZE=${PREFIX}size
STRIP=${PREFIX}strip
CONV=${PREFIX}conv
OBJDUMP=${PREFIX}objdump

export CC
export NM
export STRIP
export AR
export RANLIB

##
##CPU options
##
CPUFLAGS="-mcpu=cortex-a15 -mtune=cortex-a15 -mfloat-abi=hard -mfpu=neon -marm"

##
## asm options
##
ASFLAGS=${CPUFLAGS}

##
## gcc options
## 
export CFLAGS=${CPUFLAGS}

##
## plugin configure options
## 

CURRENT_PATH=`pwd`

VERSION=1.2.3
SYS_ROOT_DIR=${CURRENT_PATH}/../../rootfs
HOST=arm-linux-gnueabihf
INSTALL_PATH=${SYS_ROOT_DIR}/usr

DVR_TOP=${CURRENT_PATH}/../
GSTREAMER_ROOT_SOURCE_PATH=${DVR_TOP}/gstreamer
SDK_SOURCE_PATH=${DVR_TOP}/dvr_sdk
SPECIFIED_INCLUDE_PATH=${SYS_ROOT_DIR}/usr/include
SPECIFIED_LIB_PATH=${SYS_ROOT_DIR}/usr/lib

export PKG_CONFIG_PATH=${SYS_ROOT_DIR}/usr/lib/pkgconfig
export PATH=$PATH:/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin
