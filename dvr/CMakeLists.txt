cmake_minimum_required(VERSION 2.8)

set(DVR_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "-mcpu=cortex-a15 -mtune=cortex-a15 -mfloat-abi=hard -mfpu=neon -marm -g -ggdb")
set(CMAKE_C_FLAGS "-mcpu=cortex-a15 -mtune=cortex-a15 -mfloat-abi=hard -mfpu=neon -marm -g -ggdb")

set(CMAKE_MODULE_PATH ${DVR_PATH}/cmake.modules)

#定义产品宏
add_definitions(-DDVR_FEATURE_V302)
add_definitions(-DOSD_IACC)

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

add_subdirectory(dvr_sdk)
add_subdirectory(gstreamer/gst-canbuf)
add_subdirectory(gstreamer/gst-isomp4)
add_subdirectory(gstreamer/gst-multifile)
#add_subdirectory(gstreamer/gst-videomixer)

add_subdirectory(gstreamer/gst-ti-ducati)
#add_subdirectory(gstreamer/gst-ti-wayland)
add_subdirectory(gstreamer/libdce)

if(j6armcomm_FOUND)
    add_subdirectory(dvr_glue)
endif()
