#!/bin/bash

. ./envsetup.sh
. ./function.sh

export GLIB_LIBS="-L${SPECIFIED_LIB_PATH} -lglib-2.0 -lgobject-2.0 -lgmodule-2.0 -lffi -lz"
export GIO_LIBS="-L${SPECIFIED_LIB_PATH} -lgio-2.0"
export ZLIB_LIBS="-L${SPECIFIED_LIB_PATH} -lz"
export ALSA_LIBS="-L${SPECIFIED_LIB_PATH} -lasound"
export OGG_LIBS="-L${SPECIFIED_LIB_PATH} -logg"
export PANGO_LIBS="-L${SPECIFIED_LIB_PATH} -lpango-1.0 -lpangocairo-1.0"
export THEORA_LIBS="-L${SPECIFIED_LIB_PATH} -ltheoraenc -ltheoradec -logg"

export GLIB_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/glib-2.0 -I${SPECIFIED_LIB_PATH}/glib-2.0/include"
export GIO_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export ZLIB_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export ALSA_CFLAGS="-I${SPECIFIED_INCLUDE_PATH} -I${SPECIFIED_INCLUDE_PATH}/alsa"
export OGG_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export PANGO_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/pango-1.0 -I${SPECIFIED_INCLUDE_PATH}/cairo"
export THEORA_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export VORBIS_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"

MODULE_NAME=gst-plugins-base-1.2.3

OPTIONS="--disable-orc --disable-zlib --disable-ivorbis --disable-vorbis --disable-vorbistest --disable-ogg --disable-alsa --disable-pango -disable-theora -disable-freetypetest --disable-cdparanoia --disable-libvisual"

start_build

rm -r ${INSTALL_PATH}/lib/*.la
rm -r ${INSTALL_PATH}/lib/gstreamer-1.0/*.la

exit 0
