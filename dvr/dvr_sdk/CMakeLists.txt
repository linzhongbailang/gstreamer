file(GLOB_RECURSE dvr_c_src "source/*.c")
file(GLOB_RECURSE dvr_cxx_src "source/*.c??")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -std=c++11")

include_directories(include)
include_directories(build/ext_inc/libwin32)
include_directories(source/Osa)
include_directories(source/Osd)
include_directories(source/Monitor)
include_directories(source/Player)
include_directories(source/Recorder)
include_directories(source/ComSvc)
include_directories(source/Storage)
include_directories(source/FileSystem)
include_directories(source/MediaManager/inc)
include_directories(source/DvrSystemControl)
include_directories(source/MetaData)
include_directories(source/Photo)
include_directories(source/ThumbNail)
include_directories(${DVR_PATH}/gstreamer/gst-canbuf/src)

include_directories("${J6_ROOTFS}/usr/include/omap")
include_directories("${J6_ROOTFS}/usr/include/dce")
include_directories("${J6_ROOTFS}/usr/include/freetype2")
include_directories("${J6_ROOTFS}/usr/include/libdrm")

add_library(dvr_sdk SHARED ${dvr_c_src} ${dvr_cxx_src})

target_link_libraries(dvr_sdk gstcanbuf jpeg avutil swscale ${DVR_PATH}/dvr_sdk/build/ext_lib/libwin32.a
        gstreamer-1.0 gstvideo-1.0 gstaudio-1.0
        gsttag-1.0 gstbase-1.0 gstpbutils-1.0
        gobject-2.0 glib-2.0 gio-2.0 gmodule-2.0
        z ffi pthread dl dce mmrpc drm_omap)

target_link_libraries(dvr_sdk gstdrm-1.0 dce)

set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")
