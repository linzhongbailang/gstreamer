#!/bin/bash

. ./envsetup.sh
. ./function.sh

export GLIB_LIBS="-L${SPECIFIED_LIB_PATH} -lglib-2.0 -lgobject-2.0 -lffi -lz -lgmodule-2.0"
export GIO_LIBS="-L${SPECIFIED_LIB_PATH} -lgio-2.0"
export GUDEV_LIBS="-L${SPECIFIED_LIB_PATH} -lgudev-1.0"
export LIBPNG_LIBS="-L${SPECIFIED_LIB_PATH} -lpng16"
export PULSE_LIBS="-L${SPECIFIED_LIB_PATH}/pulseaudio -lpulse"
export GTK_X11_LIBS="-L${SPECIFIED_LIB_PATH} -lX11-xcb"
export SOUP_LIBS="-L${SPECIFIED_LIB_PATH} -lsoup-2.4"
export SPEEX_LIBS="-L${SPECIFIED_LIB_PATH} -lspeex"
export FLAC_LIBS="-L${SPECIFIED_LIB_PATH} -lFLAC"
export CAIRO_LIBS="-L${SPECIFIED_LIB_PATH} -lcairo -lcairo-gobject -lcairo-script-interpreter"
export GDK_PIXBUF_LIBS="-L${SPECIFIED_LIB_PATH} -lgdk_pixbuf-2.0"
export JPEG_LIBS="-L${SPECIFIED_LIB_PATH} -ljpeg"
export GST_LIBS="-L${SPECIFIED_LIB_PATH} -lgstreamer-1.0 ${GLIB_LIBS}"
export GST_BASE_LIBS="-L${SPECIFIED_LIB_PATH} -lgstbase-1.0"
export GST_PLUGINS_BASE_LIBS="-L${SPECIFIED_LIB_PATH} -lgstvideo-1.0 -lgstaudio-1.0 -lgsttag-1.0"
export GST_CONTROLLER_LIBS="-L${SPECIFIED_LIB_PATH} -lgstcontroller-1.0"
export GST_NET_LIBS="-L${SPECIFIED_LIB_PATH} -lgstnet-1.0"
export GST_CHECK_LIBS="-L${SPECIFIED_LIB_PATH} -lgstcheck-1.0 -lm"

export GLIB_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/glib-2.0 -I${SPECIFIED_LIB_PATH}/glib-2.0/include"
export GIO_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export GUDEV_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gudev-1.0"
export LIBPNG_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export PULSE_CFLAGS="-I${SPECIFIED_INCLUDE_PATH} -D_REENTRANT"
export GTK_X11_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export SOUP_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/libsoup-2.4"
export SPEEX_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export FLAC_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export CAIRO_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/cairo"
export GDK_PIXBUF_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gdk-pixbuf-2.0"
export GST_CFLAGS="-I${SPECIFIED_INCLUDE_PATH} -I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0 ${GLIB_CFLAGS}"
export GST_BASE_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0"
export GST_PLUGINS_BASE_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0"
export GST_CONTROLLER_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0"
export GST_NET_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0"
export GST_CHECK_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0 ${GLIB_CFLAGS}"

MODULE_NAME=gst-plugins-good-1.2.3

OPTIONS="--disable-orc --disable-pulse --disable-soup --disable-taglib --disable-rtp --disable-rtpmanager --disable-gst_v4l2 --disable-speex --disable-libpng --disable-cairo --disable-gdk_pixbuf --disable-flac"

start_build

rm -r ${INSTALL_PATH}/lib/*.la
rm -r ${INSTALL_PATH}/lib/gstreamer-1.0/*.la

exit 0

