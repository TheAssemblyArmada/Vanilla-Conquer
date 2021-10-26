# - Find SDL2
# Find the SDL2 headers and libraries
#
#  SDL2::SDL2 - Imported target to use for building a library
#  SDL2::SDL2main - Imported interface target to use if you want SDL and SDLmain.
#  SDL2_FOUND - True if SDL2 was found.
#  SDL2_DYNAMIC - If we found a DLL version of SDL (meaning you might want to copy a DLL from SDL2::SDL2)
#
# Original Author:
# 2015 Ryan Pavlik <ryan.pavlik@gmail.com> <abiryan@ryand.net>
#
# Copyright Sensics, Inc. 2015.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Set up architectures (for windows) and prefixes (for mingw builds)
if(WIN32)
    if(MINGW)
        include(MinGWSearchPathExtras OPTIONAL)
        if(MINGWSEARCH_TARGET_TRIPLE)
            set(SDL_PREFIX ${MINGWSEARCH_TARGET_TRIPLE})
        endif()
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL_LIB_PATH_SUFFIX lib/x64)
        if(NOT MSVC AND NOT SDL_PREFIX)
            set(SDL_PREFIX x86_64-w64-mingw32)
        endif()
    else()
        set(SDL_LIB_PATH_SUFFIX lib/x86)
        if(NOT MSVC AND NOT SDL_PREFIX)
            set(SDL_PREFIX i686-w64-mingw32)
        endif()
    endif()
else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL_LIB_PATH_SUFFIX lib/x86_64-linux-gnu)
    else()
        set(SDL_LIB_PATH_SUFFIX lib/i386-linux-gnu)
    endif()
endif()

if(SDL_PREFIX)
    set(SDL_ORIGPREFIXPATH ${CMAKE_PREFIX_PATH})
    if(SDL_ROOT_DIR)
        list(APPEND CMAKE_PREFIX_PATH "${SDL_ROOT_DIR}")
    endif()
    if(CMAKE_PREFIX_PATH)
        foreach(_prefix ${CMAKE_PREFIX_PATH})
            list(APPEND CMAKE_PREFIX_PATH "${_prefix}/${SDL_PREFIX}")
        endforeach()
    endif()
    if(MINGWSEARCH_PREFIXES)
        list(APPEND CMAKE_PREFIX_PATH ${MINGWSEARCH_PREFIXES})
    endif()
endif()

# Invoke pkgconfig for hints
find_package(PkgConfig QUIET)
set(SDL_INCLUDE_HINTS)
set(SDL_LIB_HINTS)
if(PKG_CONFIG_FOUND)
    pkg_search_module(SDLPC QUIET sdl1)
    if(SDLPC_INCLUDE_DIRS)
        set(SDL_INCLUDE_HINTS ${SDLPC_INCLUDE_DIRS})
    endif()
    if(SDLPC_LIBRARY_DIRS)
        set(SDL_LIB_HINTS ${SDLPC_LIBRARY_DIRS})
    endif()
endif()

include(FindPackageHandleStandardArgs)

find_library(SDL_LIBRARY
    NAMES
    SDL
    HINTS
    ${SDL_LIB_HINTS}
    PATHS
    ${SDL_ROOT_DIR}
    ENV SDLDIR
    PATH_SUFFIXES lib SDL ${SDL_LIB_PATH_SUFFIX})

set(_sdl2_framework FALSE)
# Some special-casing if we've found/been given a framework.
# Handles whether we're given the library inside the framework or the framework itself.
if(APPLE AND "${SDL_LIBRARY}" MATCHES "(/[^/]+)*.framework(/.*)?$")
    set(_sdl2_framework TRUE)
    set(SDL_FRAMEWORK "${SDL_LIBRARY}")
    # Move up in the directory tree as required to get the framework directory.
    while("${SDL_FRAMEWORK}" MATCHES "(/[^/]+)*.framework(/.*)$" AND NOT "${SDL_FRAMEWORK}" MATCHES "(/[^/]+)*.framework$")
        get_filename_component(SDL_FRAMEWORK "${SDL_FRAMEWORK}" DIRECTORY)
    endwhile()
    if("${SDL_FRAMEWORK}" MATCHES "(/[^/]+)*.framework$")
        set(SDL_FRAMEWORK_NAME ${CMAKE_MATCH_1})
        # If we found a framework, do a search for the header ahead of time that will be more likely to get the framework header.
        find_path(SDL_INCLUDE_DIR
            NAMES
            SDL.h
            HINTS
            "${SDL_FRAMEWORK}/Headers/")
    else()
        # For some reason we couldn't get the framework directory itself.
        # Shouldn't happen, but might if something is weird.
        unset(SDL_FRAMEWORK)
    endif()
endif()

find_path(SDL_INCLUDE_DIR
    NAMES
    SDL.h # this file was introduced with SDL2
    HINTS
    ${SDL_INCLUDE_HINTS}
    PATHS
    ${SDL_ROOT_DIR}
    ENV SDLDIR
    PATH_SUFFIXES include include/sdl include/SDL SDL)

if(WIN32 AND SDL_LIBRARY)
    find_file(SDL_RUNTIME_LIBRARY
        NAMES
        SDL.dll
        libSDL.dll
        HINTS
        ${SDL_LIB_HINTS}
        PATHS
        ${SDL_ROOT_DIR}
        ENV SDLDIR
        PATH_SUFFIXES bin lib ${SDL_LIB_PATH_SUFFIX})
endif()


if(WIN32 OR ANDROID OR IOS OR (APPLE AND NOT _sdl2_framework))
    set(SDL_EXTRA_REQUIRED SDL_SDLMAIN_LIBRARY)
    find_library(SDL_SDLMAIN_LIBRARY
        NAMES
        SDLmain
        PATHS
        ${SDL_ROOT_DIR}
        ENV SDLDIR
        PATH_SUFFIXES lib ${SDL_LIB_PATH_SUFFIX})
endif()

if(MINGW AND NOT SDLPC_FOUND)
    find_library(SDL_MINGW_LIBRARY mingw32)
    find_library(SDL_MWINDOWS_LIBRARY mwindows)
endif()

if(SDL_PREFIX)
    # Restore things the way they used to be.
    set(CMAKE_PREFIX_PATH ${SDL_ORIGPREFIXPATH})
endif()

# handle the QUIETLY and REQUIRED arguments and set QUATLIB_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL
    DEFAULT_MSG
    SDL_LIBRARY
    SDL_INCLUDE_DIR
    ${SDL_EXTRA_REQUIRED})

if(SDL_FOUND)
    if(NOT TARGET SDL::SDL)
        # Create SDL::SDL
        if(WIN32 AND SDL_RUNTIME_LIBRARY)
            set(SDL_DYNAMIC TRUE)
            add_library(SDL::SDL SHARED IMPORTED)
            set_target_properties(SDL::SDL
                PROPERTIES
                IMPORTED_IMPLIB "${SDL_LIBRARY}"
                IMPORTED_LOCATION "${SDL_RUNTIME_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SDL_INCLUDE_DIR}"
            )
        else()
            add_library(SDL::SDL UNKNOWN IMPORTED)
            if(SDL_FRAMEWORK AND SDL_FRAMEWORK_NAME)
                # Handle the case that SDL is a framework and we were able to decompose it above.
                set_target_properties(SDL::SDL PROPERTIES
                    IMPORTED_LOCATION "${SDL_FRAMEWORK}/${SDL_FRAMEWORK_NAME}")
            elseif(_sdl2_framework AND SDL_LIBRARY MATCHES "(/[^/]+)*.framework$")
                # Handle the case that SDL is a framework and SDL_LIBRARY is just the framework itself.

                # This takes the basename of the framework, without the extension,
                # and sets it (as a child of the framework) as the imported location for the target.
                # This is the library symlink inside of the framework.
                set_target_properties(SDL::SDL PROPERTIES
                    IMPORTED_LOCATION "${SDL_LIBRARY}/${CMAKE_MATCH_1}")
            else()
                # Handle non-frameworks (including non-Mac), as well as the case that we're given the library inside of the framework
                set_target_properties(SDL::SDL PROPERTIES
                    IMPORTED_LOCATION "${SDL_LIBRARY}")
            endif()
            set_target_properties(SDL::SDL
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SDL_INCLUDE_DIR}"
            )
        endif()

        if(APPLE)
            # Need Cocoa here, is always a framework
            find_library(SDL_COCOA_LIBRARY Cocoa)
            list(APPEND SDL_EXTRA_REQUIRED SDL_COCOA_LIBRARY)
            if(SDL_COCOA_LIBRARY)
                set_target_properties(SDL::SDL PROPERTIES
                        IMPORTED_LINK_INTERFACE_LIBRARIES ${SDL_COCOA_LIBRARY})
            endif()
        endif()


        # Compute what to do with SDLmain
        set(SDLMAIN_LIBRARIES SDL::SDL)
        add_library(SDL::SDLmain INTERFACE IMPORTED)
        if(SDL_SDLMAIN_LIBRARY)
            add_library(SDL::SDLmain_real STATIC IMPORTED)
            set_target_properties(SDL::SDLmain_real
                PROPERTIES
                IMPORTED_LOCATION "${SDL_SDLMAIN_LIBRARY}")
            set(SDLMAIN_LIBRARIES SDL::SDLmain_real ${SDLMAIN_LIBRARIES})
        endif()
        if(MINGW)
            # MinGW requires some additional libraries to appear earlier in the link line.
            if(SDLPC_LIBRARIES)
                # Use pkgconfig-suggested extra libraries if available.
                list(REMOVE_ITEM SDLPC_LIBRARIES SDLmain SDL)
                set(SDLMAIN_LIBRARIES ${SDLPC_LIBRARIES} ${SDLMAIN_LIBRARIES})
            else()
                # fall back to extra libraries specified in pkg-config in
                # an official binary distro of SDL for MinGW I downloaded
                if(SDL_MINGW_LIBRARY)
                    set(SDLMAIN_LIBRARIES ${SDL_MINGW_LIBRARY} ${SDLMAIN_LIBRARIES})
                endif()
                if(SDL_MWINDOWS_LIBRARY)
                    set(SDLMAIN_LIBRARIES ${SDL_MWINDOWS_LIBRARY} ${SDLMAIN_LIBRARIES})
                endif()
            endif()
            set_target_properties(SDL::SDLmain
                PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "main=SDL_main")
        endif()
        set_target_properties(SDL::SDLmain
            PROPERTIES
            INTERFACE_LINK_LIBRARIES "${SDLMAIN_LIBRARIES}")
    endif()
    mark_as_advanced(SDL_ROOT_DIR)
endif()

mark_as_advanced(SDL_LIBRARY
    SDL_RUNTIME_LIBRARY
    SDL_INCLUDE_DIR
    SDL_SDLMAIN_LIBRARY
    SDL_COCOA_LIBRARY
    SDL_MINGW_LIBRARY
    SDL_MWINDOWS_LIBRARY)
