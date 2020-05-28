aux_source_directory(src VIDEOMIXER_SRC)

include_directories(src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb -DDISABLE_ORC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -ggdb -DDISABLE_ORC")

include_directories("${J6_ROOTFS}/usr/include/omap")
include_directories("${J6_ROOTFS}/usr/include/libdrm")

add_library(gstvideomixer SHARED ${VIDEOMIXER_SRC})
target_link_libraries(gstvideomixer
        gstreamer-1.0 gstvideo-1.0 gstaudio-1.0
	dce drm drm_omap gstdmabuf-1.0 gstdrm-1.0)

set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib/gstreamer-1.0")
