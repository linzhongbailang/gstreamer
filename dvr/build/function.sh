#!/bin/bash

start_build()
{
	SOURCE_PATH=${GSTREAMER_ROOT_SOURCE_PATH}/${MODULE_NAME}
	
	pushd ${SOURCE_PATH}
	
	./autogen.sh --noconfigure --nocheck

	err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "${MODULE_NAME} autogen.sh failed, $err_code"
		exit $err_code
	fi	
	
	./configure --host=${HOST} --prefix=${INSTALL_PATH} --with-sysroot=${SYS_ROOT_DIR} --disable-silent-rules --disable-examples --disable-valgrind ${OPTIONS} 

	err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "${MODULE_NAME} configuration failed, $err_code"
		exit $err_code
	fi

	make -j10

	err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "${MODULE_NAME} make failed, $err_code"
		exit $err_code
	fi

	make install 

	err_code=$?
	if [ "$err_code" != "0" ]
	then
		echo "${MODULE_NAME} make install failed, $err_code"
		exit $err_code
	fi

	popd
}

