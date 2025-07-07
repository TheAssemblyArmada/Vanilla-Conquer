# Print some information
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

if (NOT VC_CXX_FLAGS)
    set(VC_CXX_FLAGS $ENV{VC_CXX_FLAGS}) 
endif()

# Make release builds have debug information too.
if(MSVC)
    string(REPLACE "INCREMENTAL" "INCREMENTAL:NO" replacementFlags ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DYNAMICBASE:NO /NXCOMPAT:NO /INCREMENTAL:NO ${replacementFlags}")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "/DYNAMICBASE:NO /NXCOMPAT:NO /INCREMENTAL:NO ${replacementFlags}")

    string(REPLACE "INCREMENTAL" "INCREMENTAL:NO" replacementFlags ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/INCREMENTAL:NO ${replacementFlags}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "/INCREMENTAL:NO ${replacementFlags}")

    # Create PDB for Release as long as debug info was generated during compile.
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /DEBUG:FULL /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " /DEBUG:FULL /OPT:REF /OPT:ICF")
    
    # Multithreaded build in Visual Studio/MSBuild.
    list(APPEND VC_CXX_FLAGS /MP)
    # Enforce strict __cplusplus version
    list(APPEND VC_CXX_FLAGS /Zc:__cplusplus)

else()
    # We go a bit wild here and assume any other compiler we are going to use supports -g for debug info.
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -g")
    string(APPEND CMAKE_C_FLAGS_RELEASE " -g")
 
    # GCC and Clang don't check new allocations by default while MSVC does.
    message(STATUS "Checking whether compiler supports -fcheck-new")
    check_cxx_compiler_flag("-Werror -fcheck-new" HAVE_CHECK_NEW)

    if(HAVE_CHECK_NEW)
        message(STATUS "yes")
        list(APPEND VC_CXX_FLAGS -fcheck-new)
    endif()

    # Some platforms default to an unsigned char, this fixes that.
    message(STATUS "Checking whether compiler supports -fsigned-char")
    check_cxx_compiler_flag("-Werror -fsigned-char" HAVE_SIGNED_CHAR)

    if(HAVE_SIGNED_CHAR)
        message(STATUS "yes")
        list(APPEND VC_CXX_FLAGS -fsigned-char)
    endif()

    # Not MSVC, but is WIN32... probably mingw
    if(WIN32)
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -static-libstdc++ -static-libgcc")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " -static-libstdc++ -static-libgcc")
    endif()
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Ensures only ISO features are used
