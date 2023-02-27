if(CLANG_FORMAT_FOUND)
    if(CLANG_FORMAT_VERSION VERSION_LESS "14.0.0")
        message(WARNING "clang-format ${CLANG_FORMAT_VERSION} is older than 14.0.0, formatting may yield unexpected results")
    elseif(CLANG_FORMAT_VERSION VERSION_GREATER "14.0.6")
        message(WARNING "clang-format ${CLANG_FORMAT_VERSION} is newer than 14.0.6, formatting may yield unexpected results")
    else()
        message(STATUS "Found clang-format ${CLANG_FORMAT_VERSION}, 'format' target enabled")
    endif()

    set(GLOB_PATTERNS
        common/*.cpp
        common/*.h
        tiberiandawn/*.cpp
        tiberiandawn/*.h
        redalert/*.cpp
        redalert/*.h
        tests/*.cpp
        tests/*.h
        tools/*.cpp
        tools/*.h
    )

    file(GLOB_RECURSE ALL_SOURCE_FILES RELATIVE ${CMAKE_SOURCE_DIR} ${GLOB_PATTERNS})

    add_custom_target(format)
    foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
        add_custom_command(
            TARGET format
            PRE_BUILD
            COMMAND ${CLANG_FORMAT_EXECUTABLE} -style=file -i --verbose \"${SOURCE_FILE}\"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endforeach()
else()
    message(WARNING "clang-format not found, 'format' target unavailable")
endif()
