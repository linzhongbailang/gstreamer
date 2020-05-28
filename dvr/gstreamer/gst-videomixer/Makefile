
DVR_TOP = $(shell pwd -P)/../../

include $(DVR_TOP)/Makefile.Inc

TOP_DIR		:= .
SRC_PATH	:= $(TOP_DIR)/src
INC_PATH	:= $(TOP_DIR)/src

CFLAGS += $(addprefix -I, $(INC_PATH))
CFLAGS += -DG_DISABLE_CAST_CHECKS -fPIC -I$(SPECIFIED_INCLUDE_PATH) -I$(SPECIFIED_INCLUDE_PATH)/dce -I$(SPECIFIED_INCLUDE_PATH)/libdrm -I$(SPECIFIED_INCLUDE_PATH)/omap -g -ggdb -DDISABLE_ORC

CPPFLAGS += $(CFLAGS)

PLUGIN_TARGET :=libgstvideomixer.so 

C_SRCS := $(wildcard $(SRC_PATH)/*.c) 
CPP_SRCS := $(wildcard $(SRC_PATH)/*.cpp) 
ASM_SRCS := $(wildcard $(SRC_PATH)/*.s) 

C_OBJS := $(patsubst %.c, %.o, $(filter %.c, $(C_SRCS)))
CPP_OBJS := $(patsubst %.cpp, %.o, $(filter %.cpp, $(CPP_SRCS)))
ASM_OBJS := $(patsubst %.s, %.o, $(filter %.s, $(ASM_SRCS)))

OBJS := $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS)

all release debug: $(PLUGIN_TARGET)

$(PLUGIN_TARGET) : $(OBJS)
	$(CXX) -shared $(OBJS) $(LIBS) -o $@ -ldce -ldrm -ldrm_omap -lgstdmabuf-1.0 -lgstdrm-1.0
	
$(patsubst %.c, %.o, $(C_SRCS)): %.o : %.c
	$(CC) $(CFLAGS) -c $<  -o $@
	
$(patsubst %.cpp, %.o, $(CPP_SRCS)): %.o : %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@
	
$(patsubst %.s, %.o, $(ASM_SRCS)): %.o : %.s
	$(AS) $(ASFLAGS) -o $@ $< 

clean:
	rm -rf $(OBJS) $(PLUGIN_TARGET)

cleanall:
	rm -rf $(OBJS) $(PLUGIN_TARGET)

install: 
	install $(PLUGIN_TARGET) $(PLUGIN_DIR)
