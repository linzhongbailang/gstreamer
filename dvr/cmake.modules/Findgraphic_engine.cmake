IF(NOT graphic_engine_FOUND AND NOT graphic_engine_SOURCE_FOUND)

        set(GRAPHIC_ENGINE_PATH "${PROJECT_SOURCE_DIR}/../graphic_engine")
	
	IF(NOT EXISTS ${GRAPHIC_ENGINE_PATH})
		MESSAGE(FATAL_ERROR "graphic_engine directory not found.")
	ENDIF()	
	IF(NOT DEFINED ENV{GPU_ROOT_DIR})
		SET(graphic_engine_root_dir ${GRAPHIC_ENGINE_PATH})
	ELSE()
		SET(graphic_engine_root_dir $ENV{GPU_ROOT_DIR})
	ENDIF()

	find_path(graphic_engine_INCLUDE_DIR GPU_Module_Interface.h
			PATHS ${graphic_engine_root_dir}/include)
		
	message("set graphic_engine_root_dir as ${graphic_engine_root_dir}")

	find_library(graphic_engine_LIBRARY NAMES graphic_engine PATHS ${graphic_engine_root_dir}/lib)
	find_path(graphic_engine_LIBRARY_DIR libgraphic_engine.so PATHS ${graphic_engine_root_dir}/lib)


	IF(graphic_engine_INCLUDE_DIR)
		SET(graphic_engine_FOUND TRUE)
		MESSAGE(STATUS "Found graphic_engine library, inc:${graphic_engine_INCLUDE_DIR} lib:${graphic_engine_LIBRARY_DIR}")
	ELSE()
		MESSAGE(WARNING "[dvr] Any precompiled graphic_engine library or source not found.")
        MESSAGE("[dvr] Wouldn't compile 'libdvr.so'!!!")
	ENDIF()
ENDIF()

