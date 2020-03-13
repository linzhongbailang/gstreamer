
DVR_TOP = $(shell pwd -P)/../../

include $(DVR_TOP)/Makefile.Inc

TOP_DIR		:= .
SRC_PATH	:= $(TOP_DIR)/isomp4
INC_PATH	:= $(TOP_DIR)/isomp4

CFLAGS += $(addprefix -I, $(INC_PATH))
CFLAGS += -DG_DISABLE_CAST_CHECKS -fPIC -I$(SPECIFIED_INCLUDE_PATH) -g -ggdb

CPPFLAGS += $(CFLAGS)

PLUGIN_TARGET := libgstisomp4.so

C_SRCS := $(wildcard $(SRC_PATH)/*.c) 
CPP_SRCS := $(wildcard $(SRC_PATH)/*.cpp) 

C_OBJS := $(patsubst %.c, %.o, $(filter %.c, $(C_SRCS)))
CPP_OBJS := $(patsubst %.cpp, %.o, $(filter %.cpp, $(CPP_SRCS)))

OBJS := $(C_OBJS) $(CPP_OBJS)

all release debug: $(PLUGIN_TARGET)

$(PLUGIN_TARGET) : $(OBJS)
	$(CXX) -shared $(OBJS) $(LIBS) -o $@ -lgstcanbuf -lgsttag-1.0 -lgstrtp-1.0 -lgstriff-1.0
	
$(patsubst %.c, %.o, $(C_SRCS)): %.o : %.c
	$(CC) $(CFLAGS) -c $<  -o $@
	
$(patsubst %.cpp, %.o, $(CPP_SRCS)): %.o : %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(PLUGIN_TARGET)

cleanall:
	rm -rf $(OBJS) $(PLUGIN_TARGET)

install: 
	install $(PLUGIN_TARGET) $(PLUGIN_DIR)
