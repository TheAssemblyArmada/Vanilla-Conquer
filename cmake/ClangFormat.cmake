if(CLANG_FORMAT_FOUND)
    if (CLANG_FORMAT_VERSION VERSION_GREATER "8.0.0")
        message(STATUS "Found clang-format ${CLANG_FORMAT_VERSION}, 'format' target enabled")
        if (CLANG_FORMAT_VERSION VERSION_GREATER "10.0.0")
            message(WARNING "clang-format ${CLANG_FORMAT_VERSION} is newer than tested, formatting may yield unexpected results")
        endif()

        set(GLOB_PATTERNS
            common/*.cpp
            common/*.h
            tiberiandawn/*.cpp
            tiberiandawn/*.h
            redalert/*.cpp
            redalert/*.h
        )

        file(GLOB_RECURSE ALL_SOURCE_FILES RELATIVE ${CMAKE_SOURCE_DIR} ${GLOB_PATTERNS})

        add_custom_target(format)
        foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
            add_custom_command(
                TARGET format
                PRE_BUILD
                COMMAND clang-format -style=file -i --verbose \"${SOURCE_FILE}\"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        endforeach()
    else()
        message(WARNING "clang-format found but ${CLANG_FORMAT_VERSION} is too old, 'format' target unavailable")
    endif()
else()
    message(WARNING "clang-format not found, 'format' target unavailable")
endif()
