#!/bin/bash

. ./envsetup.sh

subdir="${GSTREAMER_ROOT_SOURCE_PATH}/gst-canbuf \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-isomp4 \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-multifile \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-ti-ducati" \


for dir in ${subdir}; do
	pushd ${dir}
	make clean
	make 
	make install
	popd
done		

exit 0