
DVR_TOP = $(shell pwd -P)/../../
DVR_SDK_TOP = $(shell pwd -P)/../

include $(DVR_TOP)/Makefile.Inc

SUBDIRS = $(DVR_SDK_TOP)/source/ComSvc \
		$(DVR_SDK_TOP)/source/Osa \
		$(DVR_SDK_TOP)/source/MediaManager \
		$(DVR_SDK_TOP)/source/Player \
		$(DVR_SDK_TOP)/source/Recorder \
		$(DVR_SDK_TOP)/source/DvrSystemControl \
		$(DVR_SDK_TOP)/source/Monitor \
		$(DVR_SDK_TOP)/source/Storage \
		$(DVR_SDK_TOP)/source/Photo \
		$(DVR_SDK_TOP)/source/ThumbNail \
		$(DVR_SDK_TOP)/source/MetaData


TARGET = libdvr_sdk.so


all: create_lib_dir BUILD TARGET

TARGET:	
	$(CXX) -shared $(wildcard install/*.o) $(LIBS) -L$(DVR_SDK_TOP)/build/ext_lib -lwin32 -lavutil -lswscale -ldce -lgstdrm-1.0 -ldrm -lgstcanbuf -ljpeg -o $(TARGET)

BUILD:
	for dir in $(SUBDIRS); do if test -d $$dir; then ( make -C $$dir) ||exit 1; fi done

clean:
	rm -rf install
	rm -rf $(TARGET)
	for dir in $(SUBDIRS); do if test -d $$dir; then ( make -C $$dir clean) ||exit 1; fi done 	

create_lib_dir:
	mkdir -p install

