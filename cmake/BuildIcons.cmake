function(make_icon)
	cmake_parse_arguments(
		ARG
		""             # Boolean args
		"INPUT;OUTPUT" # List of single-value args
		""             # Multi-valued args
		${ARGN})

	if(NOT ARG_INPUT)
		message(FATAL_ERROR "INPUT is required")
	endif()

	if (NOT IS_ABSOLUTE "${ARG_INPUT}")
		get_filename_component(ARG_INPUT "${ARG_INPUT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	endif ()

	if(NOT EXISTS "${ARG_INPUT}")
		message(FATAL_ERROR "INPUT does not exist: ${ARG_INPUT}")
	endif()
    
    find_package(ImageMagick COMPONENTS convert)
    
    if(NOT ImageMagick_FOUND)
        message(WARNING "ImageMagick was not found, icons will not be generated.")
    else()
        get_filename_component(ARG_INPUT_FN "${ARG_INPUT}" NAME_WLE)
        set(ICON_FILE "${CMAKE_BINARY_DIR}/${ARG_INPUT_FN}.ico")
        add_custom_command(
			OUTPUT ${ICON_FILE}
			COMMAND ${ImageMagick_convert_EXECUTABLE}
            ARGS -background none ${ARG_INPUT} -define icon:auto-resize="256,64,48,32,16" ${ICON_FILE}
			MAIN_DEPENDENCY ${ARG_INPUT}
			COMMENT "ICO Generation: ${ARG_INPUT_FN}.ico")
        if(ARG_OUTPUT)
            set("${ARG_OUTPUT}" "${ICON_FILE}" PARENT_SCOPE)
        endif()
    endif()
endfunction()