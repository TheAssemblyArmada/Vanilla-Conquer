include(CheckIncludeFile)

# Check if we are missing any posix headers we consider required.
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(libgen.h HAVE_LIBGEN_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(fnmatch.h HAVE_FNMATCH_H)
include(CheckSymbolExists)

# Check if we are missing any functions known to exist in platform specific runtimes.
check_symbol_exists(strtrim "string.h" HAVE_STRTRIM)
check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
check_symbol_exists(strupr "string.h" HAVE_STRUPR)
check_symbol_exists(strlcat "string.h" HAVE_STRLCAT)
check_symbol_exists(strlcpy "string.h" HAVE_STRLCPY)
check_symbol_exists(strrev "string.h" HAVE_STRREV)
configure_file(string.h.in string.h @ONLY)

if(HAVE_FNMATCH_H)
    check_symbol_exists(fnmatch "fnmatch.h" HAVE_FNMATCH)
endif()

add_library(vc_compat STATIC)

if(WIN32)
    target_compile_definitions(vc_compat PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

target_sources(vc_compat PRIVATE
    compat/endianness.h
    string.c
    ${CMAKE_CURRENT_BINARY_DIR}/string.h
)

target_include_directories(vc_compat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/compat ${CMAKE_CURRENT_BINARY_DIR})

if(NOT HAVE_STRINGS_H)
    target_sources(vc_compat PRIVATE strings/strings.c strings/strings.h)
    target_include_directories(vc_compat PUBLIC strings)
endif()

if(NOT HAVE_LIBGEN_H)
    target_sources(vc_compat PRIVATE libgen/libgen.c libgen/libgen.h)
    target_include_directories(vc_compat PUBLIC libgen)
endif()

if(NOT HAVE_DIRENT_H)
    target_sources(vc_compat PRIVATE dirent/dirent.c dirent/dirent.h)
    target_include_directories(vc_compat PUBLIC dirent)
endif()

if(NOT HAVE_UNISTD_H)
    target_sources(vc_compat PRIVATE unistd/unistd.h)
    target_include_directories(vc_compat PUBLIC unistd)
endif()

if(NOT HAVE_GETOPT_H)
    target_sources(vc_compat PRIVATE getopt/getopt.c getopt/getopt.h)
    target_include_directories(vc_compat PUBLIC getopt)
endif()

if(NOT HAVE_FNMATCH)
    # TODO this should cover both windows and Vita SDK as it currently stands.
    # Option remains to split header and imlementation inclusion if needed.
    target_sources(vc_compat PRIVATE fnmatch/fnmatch.c fnmatch/fnmatch.h)
    target_include_directories(vc_compat PUBLIC fnmatch)
endif()
