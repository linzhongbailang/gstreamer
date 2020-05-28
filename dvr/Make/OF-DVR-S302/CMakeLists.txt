cmake_minimum_required(VERSION 2.8)

set(DVR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../..")

set(CMAKE_CXX_FLAGS "-mcpu=cortex-a15 -mtune=cortex-a15 -mfloat-abi=hard -mfpu=neon -marm -g -ggdb")
set(CMAKE_C_FLAGS "-mcpu=cortex-a15 -mtune=cortex-a15 -mfloat-abi=hard -mfpu=neon -marm -g -ggdb")

set(CMAKE_MODULE_PATH ${DVR_PATH}/cmake.modules2)

#定义产品宏
#add_definitions(-DPRODUCT_COMPILE_TYPE_S302)

#OSD 显示IACC功能图标和纵横向加速度值
#add_definitions(-DOSD_IACC)

if(NOT toolchain_FOUND)
    message("[dvr] Start finding toolchain")
    find_package(toolchain REQUIRED)
else()
    message("[dvr] Already found toolchain")
endif()

if(NOT rootfs_FOUND)
    message("[dvr] Start finding rootfs")
    find_package(rootfs REQUIRED)
else()
    message("[dvr] Already found rootfs")
endif()

if(NOT j6armcomm_FOUND)
    message("[dvr] Start finding j6armcomm")
    find_package(j6armcomm OPTIONAL_COMPONENTS)
else()
    message("[dvr] Already found j6armcomm")
endif()

if(NOT graphic_engine_FOUND)
    message("[dvr] Start finding graphic_engine")
    find_package(graphic_engine OPTIONAL_COMPONENTS)
else()
    message("[dvr] Already found graphic_engine")
endif()

include_directories(dvr_sdk/include)
include_directories(dvr_sdk/build/ext_inc/libwin32)
include_directories(${J6_ROOTFS}/usr/include)
include_directories(${J6_ROOTFS}/usr/include/gstreamer-1.0)
include_directories(${J6_ROOTFS}/usr/lib/gstreamer-1.0/include)
include_directories(${J6_ROOTFS}/usr/include/glib-2.0)
include_directories(${J6_ROOTFS}/usr/lib/glib-2.0/include)
link_directories(${J6_ROOTFS}/usr/lib)

add_subdirectory(${DVR_PATH}/dvr_sdk dvr_sdk.out)
add_subdirectory(${DVR_PATH}/gstreamer/gst-canbuf gstreamer/gst-canbuf.out)
add_subdirectory(${DVR_PATH}/gstreamer/gst-isomp4 gstreamer/gst-isomp4.out)
add_subdirectory(${DVR_PATH}/gstreamer/gst-multifile gstreamer/gst-multifile.out)
#add_subdirectory(${DVR_PATH}/gstreamer/gst-videomixer gstreamer/gst-videomixer)

add_subdirectory(${DVR_PATH}/gstreamer/gst-ti-ducati gstreamer/gst-ti-ducati.out)
#add_subdirectory(${DVR_PATH}/gstreamer/gst-ti-wayland gstreamer/gst-ti-wayland)
add_subdirectory(${DVR_PATH}/gstreamer/libdce gstreamer/libdce.out)
if(j6armcomm_FOUND)
    add_subdirectory(${DVR_PATH}/dvr_glue dvr_glue.out)
endif()
