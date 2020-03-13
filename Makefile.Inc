##
##
##COMPILE tools
##
##
PREFIX=/home/taohaiwu/works/project/adas/c211/ti_components/os_tools/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
AS	= $(PREFIX)as
CC	= $(PREFIX)gcc
CXX	= $(PREFIX)g++
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar
RANLIB	= $(PREFIX)ranlib
SIZE	= $(PREFIX)size
STRIP	= $(PREFIX)strip
CONV	= $(PREFIX)conv
OBJDUMP = $(PREFIX)objdump


##
##CPU options
##
CPUFLAGS = -mcpu=cortex-a15 -mtune=cortex-a15 -mfloat-abi=hard -mfpu=neon -marm

##
## asm options
##
ASFLAGS = -mcpu=cortex-a15 -mfpu=neon

##
## gcc options
## 
CFLAGS = $(CPUFLAGS)
CPPFLAGS = $(CPUFLAGS)

##
## plugin configure options
## 
CONFIGOPTION = --host=arm-linux-gnueabihf

RELEASEDIR := $(DVR_TOP)/../../rootfs/common/targetfs
SPECIFIED_INCLUDE_PATH=${RELEASEDIR}/usr/include
SPECIFIED_LIB_PATH=${RELEASEDIR}/usr/lib
PLUGIN_DIR=${SPECIFIED_LIB_PATH}/gstreamer-1.0

LIBS := -L${SPECIFIED_LIB_PATH} 

##ffi, zlib
LIBS += -lz -lffi

##glib-2.0
LIBS += -lglib-2.0 
CFLAGS += -I${SPECIFIED_INCLUDE_PATH}/glib-2.0 -I${SPECIFIED_LIB_PATH}/glib-2.0/include 
CPPFLAGS += -I${SPECIFIED_INCLUDE_PATH}/glib-2.0 -I${SPECIFIED_LIB_PATH}/glib-2.0/include 

##gobject-2.0
LIBS += -lgobject-2.0 

##gmodule-2.0
LIBS += -lgmodule-2.0

##gio-2.0
LIBS += -lgio-2.0

##gstpbutils-1.0
LIBS += -lgstpbutils-1.0

##gstreamer-1.0
LIBS += -lgstreamer-1.0
CFLAGS += -I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0
CPPFLAGS += -I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0

##gstreamer base
LIBS += -lgstbase-1.0

##gstreamer tag
LIBS += -lgsttag-1.0

##gstreamer audio
LIBS += -lgstaudio-1.0

##gstreamer video
LIBS += -lgstvideo-1.0
