# Embed some git version information in the binary.
set(GIT_PRE_CONFIGURE_FILE "gitinfo.c.in")
set(GIT_POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/gitinfo.c")
include(GitWatcher)

set(COMMON_SRC
    ${GIT_POST_CONFIGURE_FILE}
    miscasm.asm
    misc.c
    irandoma.asm
    irandom.cpp
    drawmisca.asm
    drawmisc.cpp
    xordelta.asm
    auduncmp.c
    bitblit.c
    soscodec.c
    winasm.c
    wwmouse.c
    alloc.cpp
    buffer.cpp
    gbuffer.cpp
    tobuff.asm
    face.cpp
    font.cpp
    timer.cpp
    timerdwn.cpp
    palette.cpp
    buffglbl.cpp
    iff.cpp
    load.cpp
    loadpal.cpp
    dipthong.cpp
    wsa.cpp
    _diptabl.cpp
    rawfile.cpp
    getshape.cpp
    timerini.cpp
    windows.cpp
    winhide.cpp
    writepcx.cpp
    delay.cpp
    file.cpp
    packet.cpp
    field.cpp
    rect.cpp
    cliprect.c
    txtprnt.asm
)

if (WIN32)
    list(APPEND COMMON_SRC file_win.cpp)
else()
    list(APPEND COMMON_SRC file_posix.cpp)
endif()

set(COMMONR_SRC
    mouseww.cpp
    soundio_null.cpp
    tcpip_null.cpp
    video_null.cpp
)

set(COMMONV_SRC
    mouseww.cpp
    tcpip_null.cpp
)

if(DSOUND)
    list(APPEND COMMONV_SRC soundio.cpp)
    list(APPEND VANILLA_LIBS dsound)
else()
    list(APPEND COMMONV_SRC soundio_null.cpp)
endif()

if(DDRAW)
    list(APPEND COMMONV_SRC video_ddraw.cpp)
    list(APPEND VANILLA_LIBS ddraw)
else()
    list(APPEND COMMONV_SRC video_null.cpp)
endif()

if(DSOUND OR DDRAW)
    list(APPEND VANILLA_LIBS dxguid)
endif()

file(GLOB_RECURSE COMMON_HEADERS "*.h")

add_library(common STATIC ${COMMON_SRC} ${COMMON_HEADERS})
target_link_libraries(common PUBLIC ${COMMON_LIBS})
target_include_directories(common PUBLIC .)
# Make build check state of git to check for uncommitted changes.
add_dependencies(common check_git)

if(BUILD_REMASTERTD OR BUILD_REMASTERRA)
    add_library(commonr STATIC ${COMMONR_SRC})
    target_compile_definitions(commonr PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${REMASTER_DEFS})
    target_link_libraries(commonr PUBLIC common ${REMASTER_LIBS})
endif()

# These options currently require directx components from the aug 2007 SDK.
if(BUILD_VANILLATD OR BUILD_VANILLARA)
    add_library(commonv STATIC ${COMMONV_SRC})
    target_compile_definitions(commonv PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${VANILLA_DEFS})
    target_link_libraries(commonv PUBLIC common ${VANILLA_LIBS})
endif()