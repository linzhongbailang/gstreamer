if(NOT toolchain_FOUND)
    # 如果没设置ARCH环境变量，默认为ARM编译
    if(NOT DEFINED ARCH)
        if(NOT DEFINED ENV{ARCH})
        set(ENV{ARCH} "arm")
        endif()
    
        set(arch $ENV{ARCH})
    else()
        set(arch $ENV{ARCH})
        set(ENV{ARCH} ${arch})
    endif()
    
    if(${arch} MATCHES "arm")
        message("[dvr] Compile for ARM")
        if(NOT DEFINED ENV{TOOLCHAIN_DIR}) 
            SET(toolchain_dir "/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux")
        else()
            SET(toolchain_dir $ENV{TOOLCHAIN_DIR})
        endif()
        message("[dvr] Toolchain_dir: ${toolchain_dir}")
        set(CMAKE_CXX_COMPILER ${toolchain_dir}/bin/arm-linux-gnueabihf-g++)
        set(CMAKE_C_COMPILER ${toolchain_dir}/bin/arm-linux-gnueabihf-gcc)
        set(CMAKE_LINKER ${toolchain_dir}/bin/arm-linux-gnueabihf-ld)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -mcpu=cortex-a15 -mfpu=neon -marm -mword-relocations -mno-unaligned-access -g -Wno-psabi -O1 -Dlinux")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -mcpu=cortex-a15 -mfpu=neon -marm -mword-relocations -mno-unaligned-access -Wno-psabi -g -O1 -Dlinux")
    endif()
    
    set(toolchain_FOUND TRUE)
endif()
