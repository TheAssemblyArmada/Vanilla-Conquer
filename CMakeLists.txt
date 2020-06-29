cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(NOT WIN32)
    # until we are truly portable, assume MinGW
    set(CMAKE_TOOLCHAIN_FILE "cmake/mingw-toolchain.cmake")
endif()

project(VanillaConquer)

if(MSVC)
    enable_language(ASM_MASM)
    set(CMAKE_ASM_MASM_FLAGS "/Zm /Cp /safeseh" CACHE INTERNAL "MASM flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zp1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zp1")
    add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)
else()
    set(ASM_DIALECT "_JWASM")
    enable_language(ASM_JWASM)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -fpermissive -fpack-struct=1 -fdata-sections -ffunction-sections -Wl,--gc-sections -DNOMINMAX")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -fpermissive -fpack-struct=1 -fdata-sections -ffunction-sections -Wl,--gc-sections -DNOMINMAX")
    set(STATIC_LIBS "-static-libstdc++ -static-libgcc")
endif()

add_definitions(-DENGLISH -DTRUE_FALSE_DEFINED -DWIN32 -D_WINDOWS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)

set(COMMON_LIBS winmm)

set(VANILLA_DEFS "")
set(VANILLA_LIBS ddraw dxguid)

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

set(COMMON_SRC
    common/miscasm.asm
    common/irandoma.asm
    common/irandom.cpp
    common/drawmisca.asm
    common/drawmisc.cpp
    common/xordelta.asm
    common/auduncmp.c
    common/bitblit.c
    common/setfpal.c
    common/soscodec.c
    common/winasm.c
    common/wwmouse.c
    common/alloc.cpp
    common/buffer.cpp
    common/gbuffer.cpp
    common/tobuff.asm
    common/font.cpp
    common/timer.cpp
    common/timerdwn.cpp
    common/palette.cpp
    common/buffglbl.cpp
    common/ddraw.cpp
    common/iff.cpp
    common/load.cpp
    common/loadfont.cpp
    common/loadpal.cpp
    common/mouseww.cpp
    common/dipthong.cpp
    common/wsa.cpp
    common/_diptabl.cpp
    common/drawrect.cpp
    common/set_font.cpp
    common/rawfile.cpp
    common/getshape.cpp
    common/timerini.cpp
    common/windows.cpp
    common/winhide.cpp
    common/writepcx.cpp
    common/delay.cpp
    common/file.cpp
    common/packet.cpp
    common/field.cpp
)

set(COMMONR_SRC
    common/mouseww.cpp
    common/soundio_null.cpp
    common/tcpip_null.cpp
)

set(COMMONV_SRC
    common/mouseww.cpp
    common/setvideomode.cpp
    common/tcpip_null.cpp
)

if(DSOUND)
    list(APPEND COMMONV_SRC common/soundio.cpp)
    list(APPEND VANILLA_LIBS dsound)
else()
    list(APPEND COMMONV_SRC common/soundio_null.cpp)
endif()

file(GLOB_RECURSE COMMON_HEADERS "common/*.h")

add_library(common STATIC ${COMMON_SRC} ${COMMON_HEADERS})
target_link_libraries(common PUBLIC ${COMMON_LIBS})

if(BUILD_REMASTERTD OR BUILD_REMASTERRA)
    add_library(commonr STATIC ${COMMONR_SRC})
    target_compile_definitions(commonr PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${REMASTER_DEFS})
    target_link_libraries(commonr PUBLIC common ${COMMON_LIBS} ${REMASTER_LIBS})
endif()

# These options currently require directx components from the aug 2007 SDK.
if(BUILD_VANILLATD OR BUILD_VANILLARA)
    add_library(commonv STATIC ${COMMONV_SRC})
    target_compile_definitions(commonv PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${VANILLA_DEFS})
    target_link_libraries(commonv PUBLIC common ${COMMON_LIBS} ${VANILLA_LIBS})
endif()

set(TIBDAWN_SRC
    tiberiandawn/keyfbuff.asm
    tiberiandawn/txtprnt.asm
    tiberiandawn/aadata.cpp
    tiberiandawn/abstract.cpp
    tiberiandawn/adata.cpp
    tiberiandawn/aircraft.cpp
    tiberiandawn/anim.cpp
    tiberiandawn/audio.cpp
    tiberiandawn/base.cpp
    tiberiandawn/bbdata.cpp
    tiberiandawn/bdata.cpp
    tiberiandawn/building.cpp
    tiberiandawn/bullet.cpp
    tiberiandawn/cargo.cpp
    tiberiandawn/ccfile.cpp
    tiberiandawn/cdata.cpp
    tiberiandawn/cdfile.cpp
    tiberiandawn/cell.cpp
    tiberiandawn/checkbox.cpp
    tiberiandawn/cheklist.cpp
    tiberiandawn/colrlist.cpp
    tiberiandawn/combat.cpp
    tiberiandawn/combuf.cpp
    tiberiandawn/comqueue.cpp
    tiberiandawn/confdlg.cpp
    tiberiandawn/connect.cpp
    tiberiandawn/conquer.cpp
    tiberiandawn/const.cpp
    tiberiandawn/control.cpp
    tiberiandawn/coord.cpp
    tiberiandawn/credits.cpp
    tiberiandawn/crew.cpp
    tiberiandawn/debug.cpp
    tiberiandawn/descdlg.cpp
    tiberiandawn/dial8.cpp
    tiberiandawn/dialog.cpp
    tiberiandawn/display.cpp
    tiberiandawn/door.cpp
    tiberiandawn/drive.cpp
    tiberiandawn/edit.cpp
    tiberiandawn/ending.cpp
    tiberiandawn/event.cpp
    tiberiandawn/expand.cpp
    tiberiandawn/facing.cpp
    tiberiandawn/factory.cpp
    tiberiandawn/findpath.cpp
    tiberiandawn/flasher.cpp
    tiberiandawn/fly.cpp
    tiberiandawn/foot.cpp
    tiberiandawn/fuse.cpp
    tiberiandawn/gadget.cpp
    tiberiandawn/gamedlg.cpp
    tiberiandawn/gauge.cpp
    tiberiandawn/globals.cpp
    tiberiandawn/goptions.cpp
    tiberiandawn/gscreen.cpp
    tiberiandawn/hdata.cpp
    tiberiandawn/heap.cpp
    tiberiandawn/help.cpp
    tiberiandawn/house.cpp
    tiberiandawn/idata.cpp
    tiberiandawn/infantry.cpp
    tiberiandawn/ini.cpp
    tiberiandawn/init.cpp
    tiberiandawn/interpal.cpp
    tiberiandawn/intro.cpp
    tiberiandawn/iomap.cpp
    tiberiandawn/ioobj.cpp
    tiberiandawn/ipx.cpp
    tiberiandawn/ipx95.cpp
    tiberiandawn/ipxaddr.cpp
    tiberiandawn/ipxconn.cpp
    tiberiandawn/ipxgconn.cpp
    tiberiandawn/ipxmgr.cpp
    tiberiandawn/jshell.cpp
    tiberiandawn/keyframe.cpp
    tiberiandawn/layer.cpp
    tiberiandawn/lib.cpp
    tiberiandawn/link.cpp
    tiberiandawn/list.cpp
    tiberiandawn/loaddlg.cpp
    tiberiandawn/logic.cpp
    tiberiandawn/map.cpp
    tiberiandawn/mapeddlg.cpp
    tiberiandawn/mapedit.cpp
    tiberiandawn/mapedplc.cpp
    tiberiandawn/mapedsel.cpp
    tiberiandawn/mapedtm.cpp
    tiberiandawn/mapsel.cpp
    tiberiandawn/menus.cpp
    tiberiandawn/miscasm.cpp
    tiberiandawn/miscasma.asm
    tiberiandawn/mission.cpp
    tiberiandawn/mixfile.cpp
    tiberiandawn/monoc.cpp
    tiberiandawn/mouse.cpp
    tiberiandawn/mplayer.cpp
    tiberiandawn/msgbox.cpp
    tiberiandawn/msglist.cpp
    tiberiandawn/netdlg.cpp
    tiberiandawn/noseqcon.cpp
    tiberiandawn/nullconn.cpp
    tiberiandawn/nulldlg.cpp
    tiberiandawn/nullmgr.cpp
    tiberiandawn/object.cpp
    tiberiandawn/odata.cpp
    tiberiandawn/options.cpp
    tiberiandawn/overlay.cpp
    tiberiandawn/power.cpp
    tiberiandawn/profile.cpp
    tiberiandawn/queue.cpp
    tiberiandawn/radar.cpp
    tiberiandawn/radio.cpp
    tiberiandawn/rand.cpp
    tiberiandawn/reinf.cpp
    tiberiandawn/rules.cpp
    tiberiandawn/saveload.cpp
    tiberiandawn/scenario.cpp
    tiberiandawn/score.cpp
    tiberiandawn/scroll.cpp
    tiberiandawn/sdata.cpp
    tiberiandawn/seqconn.cpp
    tiberiandawn/shape.cpp
    tiberiandawn/shapebtn.cpp
    tiberiandawn/sidebar.cpp
    tiberiandawn/slider.cpp
    tiberiandawn/smudge.cpp
    tiberiandawn/sounddlg.cpp
    tiberiandawn/special.cpp
    tiberiandawn/stamp.asm
    tiberiandawn/startup.cpp
    tiberiandawn/stats.cpp
    tiberiandawn/super.cpp
    tiberiandawn/tab.cpp
    tiberiandawn/tarcom.cpp
    tiberiandawn/target.cpp
    tiberiandawn/tcpip.cpp
    tiberiandawn/tdata.cpp
    tiberiandawn/team.cpp
    tiberiandawn/teamtype.cpp
    tiberiandawn/techno.cpp
    tiberiandawn/template.cpp
    tiberiandawn/terrain.cpp
    tiberiandawn/textbtn.cpp
    tiberiandawn/theme.cpp
    tiberiandawn/toggle.cpp
    tiberiandawn/trigger.cpp
    tiberiandawn/turret.cpp
    tiberiandawn/txtlabel.cpp
    tiberiandawn/udata.cpp
    tiberiandawn/unit.cpp
    tiberiandawn/utracker.cpp
    tiberiandawn/vector.cpp
    tiberiandawn/visudlg.cpp
    tiberiandawn/iconset.cpp
    tiberiandawn/wwkeyboard.cpp
    tiberiandawn/morphpal.cpp
    tiberiandawn/newdel.cpp
    tiberiandawn/winstub.cpp
)

if(CMAKE_CONFIGURATION_TYPES)
    # Glob all the header files together to add to the project for msvc/xcode.
    # Not ideal as CMake won't notice if you add any until something else prompts a CMake run
    # but this is largely a convenience feature for MSVC/XCode users.
    file(GLOB_RECURSE TIBDAWN_HEADERS "tiberiandawn/*.h")
    set(_TIBDAWN_SOURCES ${TIBDAWN_SRC})

    foreach(_TIBDAWN_SOURCE IN ITEMS ${_TIBDAWN_SOURCES})
        get_filename_component(_TIBDAWN_SOURCE_PATH "${_TIBDAWN_SOURCE}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/tiberiandawn" "" _TIBDAWN_GROUP_PATH "${_TIBDAWN_SOURCE_PATH}")
        string(REPLACE "/" "\\" _TIBDAWN_GROUP_PATH "${_TIBDAWN_GROUP_PATH}")
        source_group("Source Files\\${_TIBDAWN_GROUP_PATH}" FILES "${_TIBDAWN_SOURCE}")
    endforeach()

    foreach(_TIBDAWN_HEADER IN ITEMS ${TIBDAWN_HEADERS})
        get_filename_component(_TIBDAWN_HEADER_PATH "${_TIBDAWN_HEADER}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/tiberiandawn" "" _TIBDAWN_HGROUP_PATH "${_TIBDAWN_HEADER_PATH}")
        string(REPLACE "/" "\\" _TIBDAWN_HGROUP_PATH "${_TIBDAWN_HGROUP_PATH}")
        source_group("Header Files\\${_TIBDAWN_HGROUP_PATH}" FILES "${_TIBDAWN_HEADER}")
    endforeach()
endif()

if(BUILD_REMASTERTD)
    add_library(TiberianDawn SHARED 
        tiberiandawn/dllinterface.cpp
        tiberiandawn/dllinterfaceeditor.cpp
        tiberiandawn/sidebarglyphx.cpp
        ${TIBDAWN_SRC}
        ${TIBDAWN_HEADERS}
    )
    target_compile_definitions(TiberianDawn PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${REMASTER_DEFS})
    target_include_directories(TiberianDawn PUBLIC . tiberiandawn/win32lib tiberiandawn)
    target_link_libraries(TiberianDawn commonr ${REMASTER_LIBS} ${STATIC_LIBS})
    set_target_properties(TiberianDawn PROPERTIES PREFIX "")
endif()

if(BUILD_VANILLATD)
    add_executable(VanillaTD ${TIBDAWN_SRC} ${TIBDAWN_HEADERS})
    target_compile_definitions(VanillaTD PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${VANILLA_DEFS})
    target_include_directories(VanillaTD PUBLIC . tiberiandawn/win32lib tiberiandawn)
    target_link_libraries(VanillaTD commonv ${VANILLA_LIBS} ${STATIC_LIBS})
    set_target_properties(VanillaTD PROPERTIES OUTPUT_NAME vanillatd)
    if(MAP_EDITORTD)
        target_compile_definitions(VanillaTD PUBLIC INTERNAL_VERSION)
    endif()
    # Control if we auto generate a console and which "main" function we link using MSVC.
    if(MSVC)
        set_target_properties(VanillaTD PROPERTIES LINK_FLAGS "/subsystem:windows /ENTRY:WinMainCRTStartup")
    endif()
endif()

set(REDALERT_SRC
    redalert/lcwcomp.asm
    redalert/txtprnt.asm
    redalert/keyfbuff.asm
    redalert/_wsproto.cpp
    redalert/2keyfram.cpp
    redalert/aadata.cpp
    redalert/abstract.cpp
    redalert/adata.cpp
    redalert/aircraft.cpp
    redalert/anim.cpp
    redalert/audio.cpp
    redalert/b64pipe.cpp
    redalert/b64straw.cpp
    redalert/bar.cpp
    redalert/base.cpp
    redalert/base64.cpp
    redalert/bbdata.cpp
    redalert/bdata.cpp
    redalert/bench.cpp
    redalert/bfiofile.cpp
    redalert/bigcheck.cpp
    redalert/blowfish.cpp
    redalert/blowpipe.cpp
    redalert/blwstraw.cpp
    redalert/buff.cpp
    redalert/building.cpp
    redalert/bullet.cpp
    redalert/cargo.cpp
    redalert/carry.cpp
    redalert/ccfile.cpp
    redalert/ccini.cpp
    redalert/ccmpath.cpp
    redalert/ccptr.cpp
    redalert/ccten.cpp
    redalert/cdata.cpp
    redalert/cdfile.cpp
    redalert/cell.cpp
    redalert/checkbox.cpp
    redalert/cheklist.cpp
    redalert/colrlist.cpp
    redalert/combat.cpp
    redalert/combuf.cpp
    redalert/comqueue.cpp
    redalert/confdlg.cpp
    redalert/connect.cpp
    redalert/conquer.cpp
    redalert/const.cpp
    redalert/control.cpp
    redalert/coord.cpp
    redalert/crate.cpp
    redalert/crc.cpp
    redalert/crcpipe.cpp
    redalert/crcstraw.cpp
    redalert/credits.cpp
    redalert/crew.cpp
    redalert/cstraw.cpp
    redalert/debug.cpp
    redalert/descdlg.cpp
    redalert/dial8.cpp
    redalert/dialog.cpp
    redalert/dibfile.cpp
    redalert/dibutil.cpp
    redalert/display.cpp
    redalert/door.cpp
    redalert/drive.cpp
    redalert/drop.cpp
    redalert/dynavec.cpp
    redalert/edit.cpp
    redalert/egos.cpp
    redalert/ending.cpp
    redalert/event.cpp
    redalert/expand.cpp
    redalert/face.cpp
    redalert/facing.cpp
    redalert/factory.cpp
    redalert/findpath.cpp
    redalert/fixed.cpp
    redalert/flasher.cpp
    redalert/fly.cpp
    redalert/foot.cpp
    redalert/fuse.cpp
    redalert/gadget.cpp
    redalert/gamedlg.cpp
    redalert/gauge.cpp
    redalert/globals.cpp
    redalert/goptions.cpp
    redalert/gscreen.cpp
    redalert/hdata.cpp
    redalert/heap.cpp
    redalert/help.cpp
    redalert/house.cpp
    redalert/hsv.cpp
    redalert/iconlist.cpp
    redalert/idata.cpp
    redalert/infantry.cpp
    redalert/ini.cpp
    redalert/inibin.cpp
    redalert/inicode.cpp
    redalert/init.cpp
    redalert/int.cpp
    redalert/interpal.cpp
    redalert/intro.cpp
    redalert/iomap.cpp
    redalert/ioobj.cpp
    redalert/ipx.cpp
    redalert/ipx95.cpp
    redalert/ipxaddr.cpp
    redalert/ipxconn.cpp
    redalert/ipxgconn.cpp
    redalert/ipxmgr.cpp
    redalert/jshell.cpp
    redalert/key.cpp
    redalert/layer.cpp
    redalert/lcw.cpp
    redalert/lcwpipe.cpp
    redalert/lcwstraw.cpp
    redalert/lcwuncmp.cpp
    redalert/link.cpp
    redalert/list.cpp
    redalert/loaddlg.cpp
    redalert/logic.cpp
    redalert/lzo1x_c.cpp
    redalert/lzo1x_d.cpp
    redalert/lzopipe.cpp
    redalert/lzostraw.cpp
    redalert/lzw.cpp
    redalert/lzwpipe.cpp
    redalert/lzwstraw.cpp
    redalert/map.cpp
    redalert/mapeddlg.cpp
    redalert/mapedit.cpp
    redalert/mapedplc.cpp
    redalert/mapedsel.cpp
    redalert/mapedtm.cpp
    redalert/mapsel.cpp
    redalert/mci.cpp
    redalert/mcimovie.cpp
    redalert/menus.cpp
    redalert/miscasm.cpp
    redalert/mission.cpp
    redalert/mixfile.cpp
    redalert/monoc.cpp
    redalert/mouse.cpp
    redalert/mp.cpp
    redalert/mpgset.cpp
    redalert/mplayer.cpp
    redalert/mpmgrw.cpp
    redalert/mpu.cpp
    redalert/msgbox.cpp
    redalert/msglist.cpp
    redalert/netdlg.cpp
    redalert/object.cpp
    redalert/odata.cpp
    redalert/options.cpp
    redalert/overlay.cpp
    redalert/palettec.cpp
    redalert/pipe.cpp
    redalert/pk.cpp
    redalert/pkpipe.cpp
    redalert/pkstraw.cpp
    redalert/power.cpp
    redalert/profile.cpp
    redalert/queue.cpp
    redalert/radar.cpp
    redalert/radio.cpp
    redalert/ramfile.cpp
    redalert/rand.cpp
    redalert/random.cpp
    redalert/rawolapi.cpp
    redalert/readline.cpp
    redalert/rect.cpp
    redalert/reinf.cpp
    redalert/rgb.cpp
    redalert/rndstraw.cpp
    redalert/rotbmp.cpp
    redalert/rules.cpp
    redalert/saveload.cpp
    redalert/scenario.cpp
    redalert/score.cpp
    redalert/scroll.cpp
    redalert/sdata.cpp
    redalert/seditdlg.cpp
    redalert/sendfile.cpp
    redalert/seqconn.cpp
    redalert/session.cpp
    redalert/sha.cpp
    redalert/shape.cpp
    redalert/shapebtn.cpp
    redalert/shapipe.cpp
    redalert/shastraw.cpp
    redalert/sidebar.cpp
    redalert/slider.cpp
    redalert/smudge.cpp
    redalert/sounddlg.cpp
    redalert/special.cpp
    redalert/stamp.asm
    redalert/sprite.cpp
    redalert/startup.cpp
    redalert/statbtn.cpp
    redalert/stats.cpp
    redalert/straw.cpp
    redalert/super.cpp
    redalert/surface.cpp
    redalert/tab.cpp
    redalert/taction.cpp
    redalert/target.cpp
    redalert/tcpip.cpp
    redalert/tdata.cpp
    redalert/team.cpp
    redalert/teamtype.cpp
    redalert/techno.cpp
    redalert/template.cpp
    redalert/tenmgr.cpp
    redalert/terrain.cpp
    redalert/tevent.cpp
    redalert/textbtn.cpp
    redalert/theme.cpp
    redalert/toggle.cpp
    redalert/tooltip.cpp
    redalert/tracker.cpp
    redalert/trigger.cpp
    redalert/trigtype.cpp
    redalert/txtlabel.cpp
    redalert/udata.cpp
    redalert/udpaddr.cpp
    redalert/unit.cpp
    redalert/utracker.cpp
    redalert/vdata.cpp
    redalert/vector.cpp
    redalert/version.cpp
    redalert/vessel.cpp
    redalert/visudlg.cpp
    redalert/vortex.cpp
    redalert/w95trace.cpp
    redalert/warhead.cpp
    redalert/weapon.cpp
    redalert/iconset.cpp
    redalert/winstub.cpp
    redalert/wolapiob.cpp
    redalert/woledit.cpp
    redalert/wolstrng.cpp
    redalert/wol_cgam.cpp
    redalert/wol_chat.cpp
    redalert/wol_dnld.cpp
    redalert/wol_gsup.cpp
    redalert/wol_logn.cpp
    redalert/wol_main.cpp
    redalert/wol_opt.cpp
    redalert/wsproto.cpp
    redalert/wspudp.cpp
    redalert/xpipe.cpp
    redalert/xstraw.cpp
)

if(CMAKE_CONFIGURATION_TYPES)
    # Glob all the header files together to add to the project for msvc/xcode.
    # Not ideal as CMake won't notice if you add any until something else prompts a CMake run
    # but this is largely a convenience feature for MSVC/XCode users.
    file(GLOB_RECURSE REDALERT_HEADERS "redalert/*.h")
    set(_REDALERT_SOURCES ${REDALERT_SRC})

    foreach(_REDALERT_SOURCE IN ITEMS ${_REDALERT_SOURCES})
        get_filename_component(_REDALERT_SOURCE_PATH "${_REDALERT_SOURCE}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/redalert" "" _REDALERT_GROUP_PATH "${_REDALERT_SOURCE_PATH}")
        string(REPLACE "/" "\\" _REDALERT_GROUP_PATH "${_REDALERT_GROUP_PATH}")
        source_group("Source Files\\${_REDALERT_GROUP_PATH}" FILES "${_REDALERT_SOURCE}")
    endforeach()

    foreach(_REDALERT_HEADER IN ITEMS ${REDALERT_HEADERS})
        get_filename_component(_REDALERT_HEADER_PATH "${_REDALERT_HEADER}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/redalert" "" _REDALERT_HGROUP_PATH "${_REDALERT_HEADER_PATH}")
        string(REPLACE "/" "\\" _REDALERT_HGROUP_PATH "${_REDALERT_HGROUP_PATH}")
        source_group("Header Files\\${_REDALERT_HGROUP_PATH}" FILES "${_REDALERT_HEADER}")
    endforeach()
endif()

if(BUILD_REMASTERRA)
    add_library(RedAlert SHARED 
        redalert/dllinterface.cpp
        redalert/dllinterfaceeditor.cpp
        redalert/sidebarglyphx.cpp
        ${REDALERT_SRC}
        ${REDALERT_HEADERS}
    )
    target_compile_definitions(RedAlert PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${REMASTER_DEFS})
    target_include_directories(RedAlert PUBLIC . redalert/win32lib redalert)
    target_link_libraries(RedAlert commonr ${REMASTER_LIBS} ${STATIC_LIBS})
    set_target_properties(RedAlert PROPERTIES PREFIX "")
endif()

if(BUILD_VANILLARA)
    add_executable(VanillaRA ${REDALERT_SRC} ${REDALERT_HEADERS})
    target_compile_definitions(VanillaRA PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${VANILLA_DEFS})
    target_include_directories(VanillaRA PUBLIC . redalert/win32lib redalert)
    target_link_libraries(VanillaRA commonv ${VANILLA_LIBS} ${STATIC_LIBS})
    set_target_properties(VanillaRA PROPERTIES OUTPUT_NAME vanillara)
    if(MAP_EDITORRA)
        target_compile_definitions(VanillaRA PUBLIC INTERNAL_VERSION)
    endif()
    if(NETWORKING)
        target_compile_definitions(VanillaRA PUBLIC WINSOCK_IPX)
    endif()
    # Control if we auto generate a console and which "main" function we link using MSVC.
    if(MSVC)
        set_target_properties(VanillaRA PROPERTIES LINK_FLAGS "/subsystem:windows /ENTRY:WinMainCRTStartup")
    endif()
endif()

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
