cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release" FORCE)
endif()

option(FORCE_32BIT "Build 32-bit binaries on a 64-bit host" OFF)

if(FORCE_32BIT AND NOT MSVC)
    list(APPEND CMAKE_CXX_FLAGS_INIT "-m32")
endif()

project(VanillaConquer CXX)

# as long as there is no feature parity between win32 and the rest, set defaults based on platform
if(WIN32)
    set(DEFAULT_REMASTER ON)
    set(DEFAULT_VANILLATD ON)
    set(DEFAULT_VANILLARA ON)
    set(DEFAULT_DDRAW ON)
    set(DEFAULT_DSOUND ON)
    set(DEFAULT_NETWORKING ON)
    set(DEFAULT_SDL2 OFF)
    set(DEFAULT_OPENAL OFF)
else()
    set(DEFAULT_REMASTER OFF)
    set(DEFAULT_VANILLATD ON)
    set(DEFAULT_VANILLARA ON)
    set(DEFAULT_DDRAW OFF)
    set(DEFAULT_DSOUND OFF)
    set(DEFAULT_NETWORKING OFF)
    set(DEFAULT_SDL2 ON)
    set(DEFAULT_OPENAL ON)
endif()

option(BUILD_REMASTERTD "Build Tiberian Dawn remaster dll." ${DEFAULT_REMASTER})
option(BUILD_REMASTERRA "Build Red Alert remaster dll." ${DEFAULT_REMASTER})
option(BUILD_VANILLATD "Build Tiberian Dawn executable." ${DEFAULT_VANILLATD})
option(BUILD_VANILLARA "Build Red Alert executable." ${DEFAULT_VANILLARA})
option(MAP_EDITORTD "Include internal scenario editor in Tiberian Dawn build." OFF)
option(MAP_EDITORRA "Include internal scenario editor in Red Alert build." OFF)
option(NETWORKING "Enable network play." ${DEFAULT_NETWORKING})
option(DSOUND "Enable DirectSound audio." ${DEFAULT_DSOUND})
option(DDRAW "Enable DirectDraw video backend." ${DEFAULT_DDRAW})
option(SDL2 "Enable SDL2 video backend." ${DEFAULT_SDL2})
option(OPENAL "Enable OpenAL audio backend." ${DEFAULT_OPENAL})
option(BUILD_TESTS "Build unit tests." OFF)

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
endif()

if(WIN32)
    set(COMMON_LIBS winmm)
endif()

set(VANILLA_DEFS "")
set(VANILLA_LIBS "")

set(REMASTER_DEFS _USRDLL REMASTER_BUILD)
set(REMASTER_LIBS "")

# this is a convenience on Linux to run tests with wine
if(CMAKE_CROSSCOMPILING)
    find_program(WINE_EXECUTABLE NAMES wine)
    set(TARGET_SYSTEM_EMULATOR ${WINE_EXECUTABLE})
endif()

find_package(ClangFormat)
include(ClangFormat)

if(BUILD_TESTS)
    # Tests need to respect some options so need to occur after the options are set.
    enable_testing()
    add_subdirectory(tests)
endif()

if(NOT BUILD_VANILLATD AND MAP_EDTITORTD)
    message(WARNING "Internal scenario editor requires a Tiberian Dawn executable to be built, but it was not enabled.")
    set(BUILD_VANILLATD TRUE)
endif()

if(NOT BUILD_VANILLARA AND MAP_EDTITORRA)
    message(WARNING "Internal scenario editor requires a Red Alert executable to be built, but it is not enabled.")
    set(BUILD_VANILLARA TRUE)
endif()

if(NETWORKING)
    list(APPEND VANILLA_DEFS NETWORKING)
    list(APPEND VANILLA_LIBS wsock32 ws2_32)
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

add_subdirectory(common)
add_subdirectory(tiberiandawn)
add_subdirectory(redalert)

message(STATUS "VanillaConquer will be built with the following configuration:")

if(FORCE_32BIT)
    message(STATUS "  Forced 32-bit build on a 64-bit host.")
endif()

if(BUILD_REMASTERTD)
    message(STATUS "  Build a Tiberian Dawn Remastered dll.")
endif()

if(BUILD_REMASTERRA)
    message(STATUS "  Build a Red Alert Remastered dll.")
endif()

if(BUILD_VANILLATD)
    message(STATUS "  Build a Tiberian Dawn executable.")
endif()

if(BUILD_VANILLARA)
    message(STATUS "  Build a Red Alert executable.")
endif()

if(MAP_EDITORTD)
    message(STATUS "  Include scenario editor functionality in the Tiberian Dawn executable.")
endif()

if(MAP_EDITORRA)
    message(STATUS "  Include scenario editor functionality in the Red Alert executable.")
endif()

if(NETWORKING)
    message(STATUS "  Networking is enabled.")
else()
    message(STATUS "  Networking is disabled.")
endif()

if(DSOUND)
    message(STATUS "  DirectSound is enabled.")
else()
    message(STATUS "  DirectSound is disabled.")
endif()

if(DDRAW)
    message(STATUS "  DirectDraw is enabled.")
else()
    message(STATUS "  DirectDraw is disabled.")
endif()

if(SDL2)
    message(STATUS "  SDL2 is enabled.")
else()
    message(STATUS "  SDL2 is disabled.")
endif()

if(OPENAL)
    message(STATUS "  OpenAL is enabled.")
else()
    message(STATUS "  OpenAL is disabled.")
endif()

if(BUILD_TESTS)
    message(STATUS "  Unit tests will be built.")
else()
    message(STATUS "  Unit tests will not be built.")
endif()
