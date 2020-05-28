if(NOT j6armcomm_FOUND AND NOT j6armcomm_SOURCE_FOUND)

    set(ARM_COMM_PATH "${PROJECT_SOURCE_DIR}/../j6_arm_common")
	
    IF(NOT EXISTS ${ARM_COMM_PATH})
        MESSAGE(FATAL_ERROR "j6_arm_common directory not found.")
    ENDIF()	
    if(NOT DEFINED ENV{J6ARMCOMM_ROOT_DIR})
        set(j6armcomm_root_dir ${ARM_COMM_PATH})
    else()
        set(j6armcomm_root_dir $ENV{J6ARMCOMM_ROOT_DIR})
    endif()

    message("[dvr] Set j6armcomm_root_dir as ${j6armcomm_root_dir}")

    #包含目录下的reuse.h是必存在的文件，以此为特征
    find_path(j6armcomm_INCLUDE_DIR reuse.h
            PATHS ${j6armcomm_root_dir}/include)

    #找到库目录和库文件，
    find_library(j6armcomm_LIBRARY NAMES j6armcomm PATHS ${j6armcomm_root_dir}/lib)

    #库文件存在的话库目录必存在
    if(j6armcomm_INCLUDE_DIR)
        set(j6armcomm_FOUND TRUE)
    else()
        message(WARNING "[dvr] Any precompiled j6armcomm library or source not found.")
        message("[dvr] Wouldn't compile 'libdvr.so'!!!")
    endif()
endif()
