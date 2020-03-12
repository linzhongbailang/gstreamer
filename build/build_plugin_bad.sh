#!/bin/bash

. ./envsetup.sh
. ./function.sh

export GLIB_LIBS="-L${SPECIFIED_LIB_PATH} -lglib-2.0 -lgobject-2.0 -lffi -lz -lgmodule-2.0"
export GIO_LIBS="-L${SPECIFIED_LIB_PATH} -lgio-2.0"
export EGL_LIBS="-L${SPECIFIED_LIB_PATH} -lEGL -lIMGegl -ldrm_omap -lsrv_um -ldrm "
export DBUS_LIBS="-L${SPECIFIED_LIB_PATH} -ldbus-1"
export G_UDEV_LIBS="-L${SPECIFIED_LIB_PATH} -lgudev-1.0"
export CURL_LIBS="-L${SPECIFIED_LIB_PATH} -lcurl"
#export LIBXML2_LIBS="-L${SPECIFIED_LIB_PATH} -lxml2"
export WAYLAND_LIBS="-L${SPECIFIED_LIB_PATH} -lwayland-client"
export PULSE_LIBS="-L${SPECIFIED_LIB_PATH}/pulseaudio -lpulse"
export GST_LIBS="-L${SPECIFIED_LIB_PATH} -lgstreamer-1.0 ${GLIB_LIBS} ${ORC_LIBS}"
export GST_BASE_LIBS="-L${SPECIFIED_LIB_PATH} -lgstbase-1.0"
export GST_PLUGINS_BASE_LIBS="-L${SPECIFIED_LIB_PATH} -lgstvideo-1.0 -lgstaudio-1.0 -lgsttag-1.0 ${GST_BASE_LIBS}"
export GST_CHECK_LIBS="${GST_LIBS} -L${SPECIFIED_LIB_PATH} -lgstcheck-1.0 -lm"
export DRM_LIBS="-L${SPECIFIED_LIB_PATH} -ldrm_omap"
export LIBDCE_LIBS="-L${SPECIFIED_LIB_PATH} -ldce"

export GLIB_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/glib-2.0 -I${SPECIFIED_LIB_PATH}/glib-2.0/include"
export GIO_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export EGL_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export DBUS_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/dbus-1.0 -I${SPECIFIED_LIB_PATH}/dbus-1.0/include"
export G_UDEV_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gudev-1.0"
export CURL_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
#export LIBXML2_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/libxml2"
export WAYLAND_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export X11_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}"
export GST_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0 ${GLIB_CFLAGS}"
export GST_BASE_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0"
export GST_PLUGINS_BASE_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0"
export GST_CHECK_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/gstreamer-1.0 ${GLIB_CFLAGS}"
export DRM_CFLAGS="-I${SPECIFIED_INCLUDE_PATH} -I${SPECIFIED_INCLUDE_PATH}/libdrm -I${SPECIFIED_INCLUDE_PATH}/omap"
export LIBDCE_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/dce"


MODULE_NAME=gst-plugins-bad-1.2.3

OPTIONS="--disable-orc --disable-dash --disable-smoothstreaming --disable-wayland --disable-sbc --disable-vdpau --disable-decklink --disable-uvch264 --disable-bluez --disable-eglgles --disable-curl"

start_build

rm -r ${INSTALL_PATH}/lib/*.la
rm -r ${INSTALL_PATH}/lib/gstreamer-1.0/*.la

exit 0

