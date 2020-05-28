file(GLOB_RECURSE dvr_c_srcs "*.c")
file(GLOB_RECURSE dvr_cxx_srcs "*.c??")

# add compile options
add_compile_options(-fpermissive)

set_property(SOURCE ${DVR_PATH}/dvr_glue/source/framework/DvrAppMain.cpp
    PROPERTY COMPILE_DEFINITIONS __ONLINE_DEBUG_ENABLE__)
set_property(SOURCE ${DVR_PATH}/dvr_glue/DbgOnline/server/debug_online_server.cpp
    PROPERTY COMPILE_DEFINITIONS __ONLINE_DEBUG_ENABLE__)
set_property(SOURCE ${DVR_PATH}/dvr_glue/DbgOnline/dbgCases/dvr_dbgonline.cpp
    PROPERTY COMPILE_DEFINITIONS __ONLINE_DEBUG_ENABLE__)

# remove debug_online_client.cpp
set(dvr_debug_online_client ${DVR_PATH}/dvr_glue/DbgOnline/client/debug_online_client.cpp)
list(REMOVE_ITEM dvr_cxx_srcs ${dvr_debug_online_client})

# include j6common
include_directories(${j6armcomm_INCLUDE_DIR})
include_directories(${graphic_engine_INCLUDE_DIR})

include_directories(${DVR_PATH}/dvr_glue/source)
include_directories(${DVR_PATH}/dvr_glue/include)
include_directories(${DVR_PATH}/dvr_sdk/include)

link_directories(${J6_ROOTFS}/usr/lib)

add_library(dvr SHARED ${dvr_c_srcs} ${dvr_cxx_srcs})

target_link_libraries(dvr dvr_sdk graphic_engine)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
