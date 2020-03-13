aux_source_directory(isomp4 ISOMP4_SRC)

include_directories(isomp4)
include_directories(${PROJECT_SOURCE_DIR}/gstreamer/gst-canbuf/src)

add_library(gstisomp4 SHARED ${ISOMP4_SRC})
target_link_libraries(gstisomp4 gstcanbuf
        gstreamer-1.0 gstvideo-1.0 gstaudio-1.0
        gsttag-1.0 gstbase-1.0 gstpbutils-1.0
        gobject-2.0 glib-2.0 gio-2.0 gmodule-2.0
        gstrtp-1.0 gstriff-1.0
        z ffi pthread)
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib/gstreamer-1.0")
