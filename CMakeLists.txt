cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(FeatureSummary)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
endif()

project(VanillaConquer CXX ASM)

option(BUILD_REMASTERTD "Build Tiberian Dawn remaster dll." OFF)
option(BUILD_REMASTERRA "Build Red Alert remaster dll." OFF)
option(BUILD_VANILLATD "Build Tiberian Dawn executable." ON)
option(BUILD_VANILLARA "Build Red Alert executable." ON)
option(MAP_EDITORTD "Include internal scenario editor in Tiberian Dawn build." OFF)
option(MAP_EDITORRA "Include internal scenario editor in Red Alert build." OFF)
option(NETWORKING "Enable network play." ON)
option(DSOUND "Enable DirectSound audio. (deprecated)" OFF)
option(DDRAW "Enable DirectDraw video backend. (deprecated)" OFF)
option(SDL2 "Enable SDL2 video backend." ON)
option(OPENAL "Enable OpenAL audio backend." ON)
option(BUILD_TESTS "Build unit tests." OFF)
option(BUILD_TOOLS "Build modding utilities." OFF)
option(BUILD_WITH_UBSAN "Build with undefined behavior sanitizer" OFF)
option(BUILD_WITH_ASAN "Build with address sanitizer" OFF)

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    option(DSOUND "Enable DirectSound audio. (deprecated)" OFF)
    option(DDRAW "Enable DirectDraw video backend. (deprecated)" OFF)
    option(SDL2 "Enable SDL2 video backend." ON)
    option(OPENAL "Enable OpenAL audio backend." ON)
    add_feature_info(DirectSound DSOUND "DirectSound audio backend (deprecated)")
    add_feature_info(DirectDraw DDRAW "DirectDraw video backend (deprecated)")
    add_feature_info(SDL2 SDL2 "SDL2 video backend")
    add_feature_info(OpenAL OPENAL "OpenAL audio backend")
elseif(NDS)
    set(SDL2 FALSE)
    set(OPENAL FALSE)
    set(DSOUND FALSE)
    set(DDRAW FALSE)
    set(NETWORKING FALSE)
else()
    set(SDL2 TRUE)
    set(OPENAL TRUE)
endif()

if(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    option(MAKE_BUNDLE "Create a standard app bundle rather than command line program." ON)
    add_feature_info(MakeBundle MAKE_BUNDLE "App bundles will be generated")
    if(MAKE_BUNDLE)
        set(MAKE_BUNDLE_OPTION MACOSX_BUNDLE)
    endif()
endif()

add_feature_info(RemasterTD BUILD_REMASTERTD, "Remastered Tiberian Dawn dll")
add_feature_info(RemasterRA BUILD_REMASTERRA "Remastered Red Alert dll")
add_feature_info(VanillaTD BUILD_VANILLATD "Tiberian Dawn executable")
add_feature_info(VanillaRA BUILD_VANILLARA "Red Alert executable")
add_feature_info(MapEditorTD MAP_EDITORTD "Include internal scenario editor in VanillaTD")
add_feature_info(MapEditorRA MAP_EDITORRA "Include internal scenario editor in VanillaRA")
add_feature_info(Networking NETWORKING "Networking support")
add_feature_info(Tests BUILD_TESTS "Unit tests")
add_feature_info(Tools BUILD_TOOLS "Mod developer tools")

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
    elseif(NDS)
        # We need to enable optimizations, else the code doesn't fit in
        # correct address region.
        set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g3")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "-g3")
        set(CMAKE_FIND_FRAMEWORK "LAST")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -Wwrite-strings -Werror=write-strings -fcheck-new -fsigned-char -fdata-sections -ffunction-sections -DNOMINMAX")
else()
    set(CMAKE_CXX_FLAGS "/Zc:strictStrings")
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

if(NDS)
    # This directory contains code that will run on the ARM7 chip. So we don't
    # define the arch variables until this directory has already been compiled.
    add_subdirectory(arm7)

    # The code below will run on ARM9 chip.
    add_definitions(-DARM9)

    set(ARCH "-mthumb -mthumb-interwork -mcpu=arm946e-s -mtune=arm946e-s")
    set(CMAKE_C_FLAGS "${ARCH} ${CMAKE_C_CFLAGS} -fomit-frame-pointer -fno-rtti -fno-exceptions -ffast-math -fno-unwind-tables")
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -specs=ds_arm9.specs -mthumb -mthumb-interwork -Wl,-Map,vanilla.map")

    # Link with those libraries.
    link_libraries("-lc")    # C library
    link_libraries("-lfat")
    link_libraries("-lnds9")
endif()

if(BUILD_TESTS)
    # Tests need to respect some options so need to occur after the options are set.
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(common)
add_subdirectory(tiberiandawn)
add_subdirectory(redalert)
add_subdirectory(tools)

feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
