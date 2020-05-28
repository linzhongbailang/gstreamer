#!/bin/bash

. ./envsetup.sh
. ./function.sh

export GLIB_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/glib-2.0 -I${SPECIFIED_LIB_PATH}/glib-2.0/include"
export ZLIB_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"

export GLIB_LIBS="-L${SPECIFIED_LIB_PATH} -lglib-2.0 -lgobject-2.0 -lgmodule-2.0 -lffi -lz"
export ZLIB_LIBS="-L${SPECIFIED_LIB_PATH} -lz"

MODULE_NAME=gstreamer-1.2.3
OPTIONS="--disable-tests"

start_build

rm -r ${INSTALL_PATH}/lib/*.la
rm -r ${INSTALL_PATH}/lib/gstreamer-1.0/*.la

exit 0