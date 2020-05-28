
DVR_SDK_TOP = $(shell pwd -P)/../../
DVR_TOP = $(DVR_SDK_TOP)/../

include $(DVR_TOP)/Makefile.Inc
OBJ_DIR := $(DVR_SDK_TOP)/build/install

SRC_PATH	:= .
VPATH		:= . mp4_demux

INC = ./ \
	$(DVR_SDK_TOP)/include \
	$(DVR_SDK_TOP)/build/ext_inc/libwin32 \
	$(DVR_SDK_TOP)/source/Osa \
	$(DVR_SDK_TOP)/source/MediaManager/inc \
	$(DVR_SDK_TOP)/source/Player \
	$(DVR_SDK_TOP)/source/Recorder \
	$(DVR_SDK_TOP)/source/DvrSystemControl \
	$(DVR_SDK_TOP)/source/Monitor \
	$(SPECIFIED_INCLUDE_PATH) \
	$(SPECIFIED_INCLUDE_PATH)/omap \
	$(SPECIFIED_INCLUDE_PATH)/libdrm \
	$(SPECIFIED_INCLUDE_PATH)/dce

DEFINE = -fPIC -g -ggdb
CFLAGS += $(addprefix -I, $(INC)) $(DEFINE)
CPPFLAGS += $(addprefix -I, $(INC)) $(DEFINE) -std=c++11 -Wno-write-strings

C_SRCS := $(wildcard $(SRC_PATH)/*.c) 
CPP_SRCS := $(wildcard $(SRC_PATH)/*.cpp mp4_demux/*.cpp) 

C_OBJS := $(addprefix ./OBJS/, $(patsubst %.c, %.o, $(filter %.c, $(notdir $(C_SRCS)))))
CPP_OBJS := $(addprefix ./OBJS/, $(patsubst %.cpp, %.o, $(filter %.cpp, $(notdir $(CPP_SRCS)))))

OBJS := $(C_OBJS) $(CPP_OBJS)

.PHONY: all clean
all : create_obj_dir $(OBJS) 
	cp -fv $(OBJS) $(OBJ_DIR)

OBJS/%.o : %.c
	$(CC) $(CFLAGS) -c $<  -o $@
	
OBJS/%.o : %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

create_obj_dir:
	@mkdir -pv ./OBJS		

clean: 
	rm -rf OBJS
