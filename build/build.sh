#!/bin/bash

#prepare
#setup environment

. ./envsetup.sh

subdir="${GSTREAMER_ROOT_SOURCE_PATH}/gstreamer-1.2.3  \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-plugins-base-1.2.3 \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-plugins-good-1.2.3 \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-plugins-bad-1.2.3 \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-canbuf \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-isomp4 \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-multifile \
		${GSTREAMER_ROOT_SOURCE_PATH}/gst-ti-ducati" \

if(($#==1)) ; then
    if [ "$1"x = "clean"x ] ; then
        for dir in ${subdir}; do
			pushd ${dir}
			make clean
			popd
		done		
        exit $?
    fi
fi


echo "--------------start build gstreamer----------------"
./build_gstreamer.sh

err_code=$?
if [ "$err_code" != "0" ]
then
	echo "Build Gstreamer Failed, $err_code"
	exit $err_code
fi

echo "--------------start build plugin base----------------"
./build_plugin_base.sh

err_code=$?
if [ "$err_code" != "0" ]
then
	echo "Build base plugin Failed, $err_code"
	exit $err_code
fi

./build_plugin_good.sh

err_code=$?
if [ "$err_code" != "0" ]
then
	echo "Build good plugin Failed, $err_code"
	exit $err_code
fi

./build_plugin_bad.sh

err_code=$?
if [ "$err_code" != "0" ]
then
	echo "Build bad plugin Failed, $err_code"
	exit $err_code
fi


#generate SDK
MY_PATH=`pwd`
SDK_PATH=${MY_PATH}/../${HOST}

if [ -d ${SDK_PATH} ]
then
	rm -rf ${SDK_PATH}
	mkdir -p ${SDK_PATH}/bin
	mkdir -p ${SDK_PATH}/lib
	mkdir -p ${SDK_PATH}/include
else
	mkdir -p ${SDK_PATH}/bin
	mkdir -p ${SDK_PATH}/lib
	mkdir -p ${SDK_PATH}/include
fi

cp -rf ${INSTALL_PATH}/bin/gst-* ${SDK_PATH}/bin/

err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "copy binary failed, $err_code"
		exit $err_code
	fi


cp -rf ${INSTALL_PATH}/lib/libgst* ${SDK_PATH}/lib/

err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "copy libs failed, $err_code"
		exit $err_code
	fi

cp -rf ${INSTALL_PATH}/lib/gstreamer-1.0 ${SDK_PATH}/lib/

err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "copy plug-ins failed, $err_code"
		exit $err_code
	fi
	
cp -rf ${INSTALL_PATH}/lib/gstreamer1.0 ${SDK_PATH}/lib/

err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "copy scaner failed, $err_code"
		exit $err_code
	fi
	
cp -rf ${INSTALL_PATH}/include/gstreamer-1.0 ${SDK_PATH}/include/

err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "copy header files failed, $err_code"
		exit $err_code
	fi
	
	
	