cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(NOT WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # until we are truly portable, assume MinGW if no toolchain is set (use -DCMAKE_TOOLCHAIN_FILE= to force native build)
    set(CMAKE_TOOLCHAIN_FILE "cmake/mingw-toolchain.cmake")
    message(STATUS "Cross-compiling for Win32 with MinGW-W64")
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(VanillaConquer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
    enable_language(ASM_MASM)
    set(CMAKE_ASM_MASM_FLAGS "/Zm /Cp /safeseh" CACHE INTERNAL "MASM flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zp1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zp1")
    add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)
else()
    set(ASM_DIALECT "_JWASM")
    enable_language(ASM_JWASM)
    set(CMAKE_CXX_FLAGS_DEBUG "-gstabs3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -fpermissive -fpack-struct=1 -fdata-sections -ffunction-sections -Wl,--gc-sections -DNOMINMAX")
    set(CMAKE_C_FLAGS_DEBUG "-gstabs3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -fpermissive -fpack-struct=1 -fdata-sections -ffunction-sections -Wl,--gc-sections -DNOMINMAX")
    set(STATIC_LIBS "-static-libstdc++ -static-libgcc")
endif()

add_definitions(-DENGLISH -DTRUE_FALSE_DEFINED)

if(WIN32 OR CMAKE_CROSSCOMPILING)
    add_definitions(-DWIN32 -D_WINDOWS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

set(COMMON_LIBS winmm)

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

enable_testing()
add_subdirectory(tests)

# For now build the dll files by default only for windows while there is only the proprietary GlyphX frontend for them.
if(WIN32)
    set(DEFAULT_REMASTER ON)
else()
    set(DEFAULT_REMASTER OFF)
endif()

option(BUILD_REMASTERTD "Build Tiberian Dawn remaster dll." ${DEFAULT_REMASTER})
option(BUILD_REMASTERRA "Build Red Alert remaster dll." ${DEFAULT_REMASTER})
option(BUILD_VANILLATD "Build Tiberian Dawn executable." ON)
option(BUILD_VANILLARA "Build Red Alert executable." ON)
option(MAP_EDITORTD "Include internal scenario editor in Tiberian Dawn build." OFF)
option(MAP_EDITORRA "Include internal scenario editor in Red Alert build." OFF)
option(NETWORKING "Enable network play." ON)
option(DSOUND "Enable DirectSound audio." ON)
option(DDRAW "Enable DirectDraw video backend." ON)
option(USE_ASM "Use x86 assembly optimizations." ON)

if(NOT USE_ASM)
    add_definitions(-DNOASM)
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

add_subdirectory(common)
add_subdirectory(tiberiandawn)
add_subdirectory(redalert)

message(STATUS "VanillaConquer will be built with the following configuration:")

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

if(USE_ASM)
    message(STATUS "  Using x86 assembly optimizations.")
else()
    message(STATUS "  Using C replacements for x86 assembly.")
endif()
