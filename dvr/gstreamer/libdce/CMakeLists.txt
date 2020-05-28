aux_source_directory(src DCE_SRC)

include_directories("${J6_ROOTFS}/usr/include/dce")
include_directories("${J6_ROOTFS}/usr/include/omap")
include_directories("${J6_ROOTFS}/usr/include/libdrm")

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -DBUILDOS_LINUX=1 -DDCE_DEBUG_LEVEL=1 -DDCE_DEBUG_ENABLE=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -DBUILDOS_LINUX=1 -DDCE_DEBUG_LEVEL=1 -DDCE_DEBUG_ENABLE=1")

add_library(dce.so.1.0.0 SHARED ${DCE_SRC})

set_target_properties(dce.so.1.0.0 PROPERTIES SUFFIX "")

target_link_libraries(dce.so.1.0.0 stdc++ gstallocators-1.0
        gstreamer-1.0 gstvideo-1.0 gstaudio-1.0
        gsttag-1.0 gstbase-1.0 gstpbutils-1.0
        gobject-2.0 glib-2.0 gio-2.0 gmodule-2.0
	gstdrm-1.0 gstdmabuf-1.0
	drm drm_omap wayland-client mmrpc
        z ffi pthread)

set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")
