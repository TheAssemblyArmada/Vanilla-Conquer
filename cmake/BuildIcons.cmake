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
    
    find_package(ImageMagick COMPONENTS magick)
    
    if(ImageMagick_magick_FOUND)
        set(ImageMagick_convert_FOUND TRUE)
        set(ImageMagick_convert_EXECUTABLE ${ImageMagick_magick_EXECUTABLE})
        set(ImageMagick_convert_ARGS convert)
    else()
        find_package(ImageMagick COMPONENTS convert)
    endif()
    
    
    if(NOT ImageMagick_convert_FOUND)
        message(WARNING "ImageMagick was not found, icons will not be generated.")
    elseif(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin") # Make icns file.
        find_package(iconutil REQUIRED)
        if(iconutil_FOUND)
            get_filename_component(ARG_INPUT_FN "${ARG_INPUT}" NAME_WLE)
            set(ICON_FILE "${CMAKE_BINARY_DIR}/${ARG_INPUT_FN}.icns")
            set(ICON_DIR "${CMAKE_BINARY_DIR}/${ARG_INPUT_FN}.iconset")
            set(ICON_SIZES "16;32;128;256;512")
            set(ICON_DENSITIES "72;144")
            
            foreach(size IN LISTS ICON_SIZES)
                foreach(density IN LISTS ICON_DENSITIES)
                    if(density STREQUAL 144)
                        set(xtwo "@2x")
                        math(EXPR _size "${size} * 2")
                    else()
                        set(xtwo "")
                        set(_size ${size})
                    endif()
                    list(APPEND IMAGEMAGICK_CMDS
                        COMMAND ${ImageMagick_convert_EXECUTABLE} ${ImageMagick_convert_ARGS} -background none -density "${density}" -resize "${_size}x${_size}" -units "PixelsPerInch" ${ARG_INPUT} "${ICON_DIR}/icon_${size}x${size}${xtwo}.png"
                    )
                endforeach()
            endforeach()
            
            add_custom_command(
                OUTPUT "${ICON_FILE}"
                COMMAND "${CMAKE_COMMAND}" -E remove_directory "${ICON_DIR}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${ICON_DIR}"
                ${IMAGEMAGICK_CMDS}
                COMMAND "${iconutil_EXECUTABLE}" ${iconutil_OPTIONS} -c icns -o "${ICON_FILE}" "${ICON_DIR}"
                MAIN_DEPENDENCY ${ARG_INPUT}
                COMMENT "ICNS Generation: ${ARG_INPUT_FN}.icns"
                VERBATIM
            )
            
            if(ARG_OUTPUT)
                set("${ARG_OUTPUT}" "${ICON_FILE}" PARENT_SCOPE)
            endif()
        else()
            message(WARNING "Could not find suitable tool to build .icns file with.")
        endif()
    elseif(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows") # Make windows icon
        get_filename_component(ARG_INPUT_FN "${ARG_INPUT}" NAME_WLE)
        set(ICON_FILE "${CMAKE_BINARY_DIR}/${ARG_INPUT_FN}.ico")
        add_custom_command(
			OUTPUT ${ICON_FILE}
			COMMAND ${ImageMagick_convert_EXECUTABLE}
            ARGS ${ImageMagick_convert_ARGS} -background none ${ARG_INPUT} -define icon:auto-resize="256,64,48,32,16" ${ICON_FILE}
			MAIN_DEPENDENCY ${ARG_INPUT}
			COMMENT "ICO Generation: ${ARG_INPUT_FN}.ico")
        if(ARG_OUTPUT)
            set("${ARG_OUTPUT}" "${ICON_FILE}" PARENT_SCOPE)
        endif()
    else()
        message(INFO "Icon currently not processed for this platform.")
    endif()
endfunction()
