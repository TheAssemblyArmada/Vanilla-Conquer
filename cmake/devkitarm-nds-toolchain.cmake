set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR armv5te)

set(DEVKITARM $ENV{DEVKITARM})
set(DEVKITPRO $ENV{DEVKITPRO})

set(CMAKE_C_COMPILER "${DEVKITARM}/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITARM}/bin/arm-none-eabi-g++")
set(CMAKE_AR "${DEVKITARM}/bin/arm-none-eabi-gcc-ar")
set(CMAKE_RANLIB "${DEVKITARM}/bin/arm-none-eabi-gcc-ranlib")
set(NDSTOOL "${DEVKITARM}/bin/ndstool")

set(CMAKE_FIND_ROOT_PATH ${DEVKITPRO})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".la")

set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES ${DEVKITARM}/include ${DEVKITPRO}/libnds/include ${DEVKITPRO}/libgba/include)

set(NDS TRUE)

SET(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Shared libs not available" )

add_definitions(-DARM9 -D_NDS)

set(ARCH "-march=armv5te -mtune=arm946e-s")
set(CMAKE_C_FLAGS " -fomit-frame-pointer -fno-rtti -fno-exceptions -ffast-math" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C++ flags")
set(CMAKE_EXE_LINKER_FLAGS "-specs=ds_arm9.specs -g -mthumb -mthumb-interwork -Wl,-Map,vanilla.map")

# Define paths to include libraries
include_directories("${DEVKITPRO}/libnds/include")

# Define paths to libraries.
link_libraries("-L${DEVKITARM}/lib")
link_libraries("-L${DEVKITARM}/arm-none-eabi/lib")
link_libraries("-L${DEVKITPRO}/libnds/lib")
link_libraries("-L${DEVKITPRO}/libgba/lib")

# Link with those libraries
link_libraries("-lc")    # C library
link_libraries("-lfat")
link_libraries("-lnds9")
