cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(FeatureSummary)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
endif()

project(VanillaConquer CXX)

option(BUILD_REMASTERTD "Build Tiberian Dawn remaster dll." OFF)
option(BUILD_REMASTERRA "Build Red Alert remaster dll." OFF)
option(BUILD_VANILLATD "Build Tiberian Dawn executable." ON)
option(BUILD_VANILLARA "Build Red Alert executable." ON)
option(MAP_EDITORTD "Include internal scenario editor in Tiberian Dawn build." OFF)
option(MAP_EDITORRA "Include internal scenario editor in Red Alert build." OFF)
option(NETWORKING "Enable network play." OFF)
option(DSOUND "Enable DirectSound audio. (deprecated)" OFF)
option(DDRAW "Enable DirectDraw video backend. (deprecated)" OFF)
option(SDL2 "Enable SDL2 video backend." ON)
option(OPENAL "Enable OpenAL audio backend." ON)
option(BUILD_TESTS "Build unit tests." OFF)
option(BUILD_TOOLS "Build modding utilities." OFF)
option(USE_CRASHPAD "Build CrashPad to use as the crash handler." OFF)

add_feature_info(RemasterTD BUILD_REMASTERTD, "Remastered Tiberian Dawn dll")
add_feature_info(RemasterRA BUILD_REMASTERRA "Remastered Red Alert dll")
add_feature_info(VanillaTD BUILD_VANILLATD "Red Alert executable")
add_feature_info(VanillaRA BUILD_VANILLARA "Tiberian Dawn executable")
add_feature_info(MapEditorTD MAP_EDITORTD "Include internal scenario editor in VanillaTD")
add_feature_info(MapEditorRA MAP_EDITORRA "Include internal scenario editor in VanillaRA")
add_feature_info(Networking NETWORKING "Networking support")
add_feature_info(DirectSound DSOUND "DirectSound audio backend (deprecated)")
add_feature_info(DirectDraw DDRAW "DirectDraw video backend (deprecated)")
add_feature_info(SDL2 SDL2 "SDL2 video backend")
add_feature_info(OpenAL OPENAL "OpenAL audio backend")
add_feature_info(Tests BUILD_TESTS "Unit tests")
add_feature_info(Tools BUILD_TOOLS "Mod developer tools")
add_feature_info(CrashPad USE_CRASHPAD "CrashPad exception handler")

if(NOT BUILD_VANILLATD AND NOT BUILD_VANILLARA)
    set(DSOUND OFF)
    set(DDRAW OFF)
    set(SDL2 OFF)
    set(OPENAL OFF)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
    if(WIN32)
        set(CMAKE_CXX_FLAGS_DEBUG "-gstabs3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
        set(STATIC_LIBS "-static-libstdc++ -static-libgcc")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -fcheck-new -fsigned-char -fdata-sections -ffunction-sections -DNOMINMAX")
endif()

add_definitions(-DENGLISH -DTRUE_FALSE_DEFINED)

if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
    set(COMMON_LIBS winmm)
endif()

set(VANILLA_DEFS "")
set(VANILLA_LIBS "")

set(REMASTER_DEFS _USRDLL REMASTER_BUILD)
set(REMASTER_LIBS "")

find_package(ClangFormat)
include(ClangFormat)

if(BUILD_TESTS)
    # Tests need to respect some options so need to occur after the options are set.
    enable_testing()
    add_subdirectory(tests)
endif()

if(NOT BUILD_VANILLATD AND MAP_EDITORTD)
    message(WARNING "Internal scenario editor requires a Tiberian Dawn executable to be built, but it was not enabled.")
    set(BUILD_VANILLATD TRUE)
endif()

if(NOT BUILD_VANILLARA AND MAP_EDITORRA)
    message(WARNING "Internal scenario editor requires a Red Alert executable to be built, but it is not enabled.")
    set(BUILD_VANILLARA TRUE)
endif()

if(NETWORKING)
    list(APPEND VANILLA_DEFS NETWORKING)
    if(WIN32)
        list(APPEND VANILLA_LIBS ws2_32)
    endif()
endif()

if(SDL2)
    find_package(SDL2 REQUIRED)
    list(APPEND VANILLA_LIBS ${SDL2_LIBRARY})
    include_directories(${SDL2_INCLUDE_DIR})
    # winuser.h defines VK_ macros but it can be prevented
    if(WIN32)
        add_definitions(-DNOVIRTUALKEYCODES)
    endif()
    set(DDRAW OFF)
endif()

if(OPENAL)
    find_package(OpenAL REQUIRED)
    list(APPEND VANILLA_LIBS OpenAL::OpenAL)
    set(DSOUND OFF)
endif()

if(USE_CRASHPAD)
    include(FetchContent)
    # Disables zlib tests from being run when they aren't built if CrashPad builds its own zlib.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CTestCustom.cmake ${CMAKE_BINARY_DIR})

    FetchContent_Declare(
        crashpad_cmake
        GIT_REPOSITORY https://github.com/TheAssemblyArmada/crashpad.git
        GIT_TAG        4949a4cee5f321d248829d70f8600762f8e43f88
    )

    # We don't use FetchContent_MakeAvailable here because we don't want all crashpad targets including, just our dependencies.
    FetchContent_GetProperties(crashpad_cmake)
    if(NOT crashpad_cmake_POPULATED)
        FetchContent_Populate(crashpad_cmake)
        add_subdirectory(${crashpad_cmake_SOURCE_DIR} ${crashpad_cmake_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        include(BuildIcons)
        include(ProductVersion)
        make_icon(INPUT "${CMAKE_SOURCE_DIR}/resources/vanillacrash_icon.svg" OUTPUT VANILLACRASH_ICON)
        generate_product_version(
            CRASHPAD_RC
            NAME "Vanilla Conquer Crash Handler"
            BUNDLE "Vanilla-Conquer"
            VERSION_MAJOR 1
            VERSION_MINOR 0
            COMPANY_NAME "Vanilla-Conquer"
            COMPANY_COPYRIGHT "Based on code released by Google under the Apache 2.0 license."
            ORIGINAL_FILENAME "crashpad_handler.exe"
            RCFILE_PREFIX "CrashPad"
            ICON "${VANILLACRASH_ICON}"
        )

        target_sources(crashpad_handler PRIVATE ${CRASHPAD_RC})
        # Hack to make the icon generate.
        add_custom_target(crash_icon DEPENDS ${VANILLACRASH_ICON})
        add_dependencies(crashpad_handler crash_icon)
    endif()

    set_target_properties(crashpad_handler PROPERTIES OUTPUT_NAME vc_crashhandler)
    list(APPEND VANILLA_DEFS BUILD_WITH_CRASHPAD)
endif()

add_subdirectory(common)
add_subdirectory(tiberiandawn)
add_subdirectory(redalert)
add_subdirectory(tools)

feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
