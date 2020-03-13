aux_source_directory(src WAYLAND_SRC)

include_directories("${J6_ROOTFS}/usr/include/dce")
include_directories("${J6_ROOTFS}/usr/include/omap")
include_directories("${J6_ROOTFS}/usr/include/libdrm")

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

add_library(gstwaylandsink SHARED ${WAYLAND_SRC})

target_link_libraries(gstwaylandsink stdc++ gstallocators-1.0
        gstreamer-1.0 gstvideo-1.0 gstaudio-1.0
        gsttag-1.0 gstbase-1.0 gstpbutils-1.0
        gobject-2.0 glib-2.0 gio-2.0 gmodule-2.0
	gstdrm-1.0 gstdmabuf-1.0
	dce drm drm_omap wayland-client
        z ffi pthread)
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib/gstreamer-1.0")
