aux_source_directory(src DUCATI_SRC)
include_directories("${J6_ROOTFS}/usr/include/dce")
include_directories("${J6_ROOTFS}/usr/include/omap")
include_directories("${J6_ROOTFS}/usr/include/libdrm")
add_library(gstducati SHARED ${DUCATI_SRC})
target_link_libraries(gstducati gstcanbuf
        gstreamer-1.0 gstvideo-1.0 gstaudio-1.0
        gsttag-1.0 gstbase-1.0 gstpbutils-1.0
        gobject-2.0 glib-2.0 gio-2.0 gmodule-2.0
	gstdrm-1.0 gstdmabuf-1.0 gstallocators-1.0
	dce drm drm_omap mmrpc
        z ffi pthread)
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib/gstreamer-1.0")