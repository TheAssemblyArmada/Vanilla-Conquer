# Embed some git version information in the binary.
set(GIT_PRE_CONFIGURE_FILE "gitinfo.cpp.in")
set(GIT_POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/gitinfo.cpp")
include(GitWatcher)

set(COMMON_SRC
    ${GIT_POST_CONFIGURE_FILE}
    _diptabl.cpp
    alloc.cpp
    auduncmp.cpp
    b64pipe.cpp
    b64straw.cpp
    base64.cpp
    bfiofile.cpp
    blowfish.cpp
    blowpipe.cpp
    blwstraw.cpp
    buff.cpp
    buffer.cpp
    buffglbl.cpp
    ccfile.cpp
    cdfile.cpp
    cliprect.cpp
    combuf.cpp
    control.cpp
    crc.cpp
    crew.cpp
    cstraw.cpp
    debugstring.cpp
    delay.cpp
    dipthong.cpp
    drawbuff.cpp
    drawline.cpp
    drawmisc.cpp
    face.cpp
    fading.cpp
    field.cpp
    file.cpp
    filepcx.cpp
    fixed.cpp
    font.cpp
    gadget.cpp
    getshape.cpp
    graphicsviewport.cpp
    hsv.cpp
    iff.cpp
    ini.cpp
    int.cpp
    irandom.cpp
    keybuff.cpp
    keyframe.cpp
    lcw.cpp
    lcwpipe.cpp
    lcwstraw.cpp
    linear.cpp
    link.cpp
    load.cpp
    memrev.cpp
    misc.cpp
    mixfile.cpp
    mp.cpp
    newdel.cpp
    packet.cpp
    palette.cpp
    palettec.cpp
    pipe.cpp
    pk.cpp
    pkpipe.cpp
    pkstraw.cpp
    random.cpp
    rand.cpp
    rawfile.cpp
    readline.cpp
    rect.cpp
    rgb.cpp
    rndstraw.cpp
    settings.cpp
    sha.cpp
    shape.cpp
    shapipe.cpp
    shastraw.cpp
    soscodec.cpp
    stamp.cpp
    straw.cpp
    timer.cpp
    timerdwn.cpp
    tobuff.cpp
    toggle.cpp
    utracker.cpp
    vector.cpp
    winasm.cpp
    windows.cpp
    winhide.cpp
    wsa.cpp
    xordelta.cpp
    xpipe.cpp
    xstraw.cpp
)

if (WIN32)
    list(APPEND COMMON_SRC file_win.cpp)
else()
    list(APPEND COMMON_SRC file_posix.cpp)
endif()

set(COMMONR_SRC
    debugstring.cpp
    framelimit.cpp
    gbuffer.cpp
    interpal.cpp
    soundio_null.cpp
    tcpip_null.cpp
    video_null.cpp
    wwkeyboard.cpp
    wwmouse.cpp
)

set(COMMONV_SRC
    debugstring.cpp
    framelimit.cpp
    gbuffer.cpp
    interpal.cpp
    tcpip.cpp
    tcpip_null.cpp
    unvqbuff.cpp
    vqaconfig.cpp
    vqadrawer.cpp
    vqaloader.cpp
    vqapalette.cpp
    vqatask.cpp
    vqaver.cpp
    wwkeyboard.cpp
    wwmouse.cpp
)

if(DSOUND)
    list(APPEND COMMONV_SRC soundio.cpp vqaaudio_dsound.cpp)
    list(APPEND VANILLA_LIBS dsound)
elseif(OPENAL)
    list(APPEND COMMONV_SRC soundio_openal.cpp vqaaudio_openal.cpp)
else()
    list(APPEND COMMONV_SRC soundio_null.cpp vqaaudio_null.cpp)
endif()

if(DDRAW)
    list(APPEND COMMONV_SRC video_ddraw.cpp)
    list(APPEND VANILLA_LIBS ddraw)
elseif(SDL2)
    list(APPEND COMMONV_SRC video_sdl2.cpp)
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
target_compile_definitions(common PRIVATE FIXIT_FAST_LOAD)
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
    if(DSOUND)
        target_compile_definitions(commonv PUBLIC DSOUND_BUILD)
    endif()
    if(OPENAL)
        target_compile_definitions(commonv PUBLIC OPENAL_BUILD)
    endif()
    if(SDL2)
        target_compile_definitions(commonv PUBLIC SDL2_BUILD)
    endif()
endif()
