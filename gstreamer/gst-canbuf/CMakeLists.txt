aux_source_directory(src CANBUF_SRC)
add_library(gstcanbuf SHARED ${CANBUF_SRC})
target_link_libraries(gstcanbuf
        gstreamer-1.0 gstvideo-1.0 gstaudio-1.0
        gsttag-1.0 gstbase-1.0 gstpbutils-1.0
        gobject-2.0 glib-2.0 gio-2.0 gmodule-2.0
        z ffi pthread)
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")