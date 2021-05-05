if (BUILD_TOOLS)
    enable_language(C) # For miniposix.
    add_subdirectory(miniposix)
    add_subdirectory(stringtool)
endif()
