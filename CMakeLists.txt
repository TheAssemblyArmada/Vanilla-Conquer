cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

enable_language(ASM_MASM)
set(CMAKE_ASM_MASM_FLAGS "/Zm /Cp /safeseh" CACHE INTERNAL "MASM flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zp1")
add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)

project(VanillaConquer)

find_package(ClangFormat)
include(ClangFormat)

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

if(NOT BUILD_VANILLATD AND MAP_EDTITORTD)
    message(WARNING "Internal scenario editor requires a Tiberian Dawn executable to be built, but it was not enabled.")
    set(BUILD_VANILLATD TRUE)
endif()

if(NOT BUILD_VANILLARA AND MAP_EDTITORRA)
    message(WARNING "Internal scenario editor requires a Red Alert executable to be built, but it is not enabled.")
    set(BUILD_VANILLARA TRUE)
endif()

set(TIBDAWN_SRC
    tiberiandawn/win32lib/tobuff.asm
    tiberiandawn/win32lib/xordelta.asm
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
    tiberiandawn/ccdde.cpp
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
    tiberiandawn/dde.cpp
    tiberiandawn/debug.cpp
    tiberiandawn/descdlg.cpp
    tiberiandawn/dial8.cpp
    tiberiandawn/dialog.cpp
    tiberiandawn/display.cpp
    tiberiandawn/door.cpp
    tiberiandawn/dpmi.cpp
    tiberiandawn/drive.cpp
    tiberiandawn/edit.cpp
    tiberiandawn/ending.cpp
    tiberiandawn/event.cpp
    tiberiandawn/expand.cpp
    tiberiandawn/facing.cpp
    tiberiandawn/factory.cpp
    tiberiandawn/field.cpp
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
    tiberiandawn/internet.cpp
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
    tiberiandawn/packet.cpp
    tiberiandawn/power.cpp
    tiberiandawn/profile.cpp
    tiberiandawn/queue.cpp
    tiberiandawn/radar.cpp
    tiberiandawn/radio.cpp
    tiberiandawn/rand.cpp
    tiberiandawn/rawfile.cpp
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
    tiberiandawn/win32lib/alloc.cpp
    tiberiandawn/win32lib/buffer.cpp
    tiberiandawn/win32lib/buffglbl.cpp
    tiberiandawn/win32lib/ddraw.cpp
    tiberiandawn/win32lib/dipthong.cpp
    tiberiandawn/win32lib/drawmisc.cpp
    tiberiandawn/win32lib/drawrect.cpp
    tiberiandawn/win32lib/font.cpp
    tiberiandawn/win32lib/gbuffer.cpp
    tiberiandawn/win32lib/getshape.cpp
    tiberiandawn/win32lib/iconset.cpp
    tiberiandawn/win32lib/iff.cpp
    tiberiandawn/win32lib/irandom.cpp
    tiberiandawn/win32lib/keyboard.cpp
    tiberiandawn/win32lib/load.cpp
    tiberiandawn/win32lib/loadfont.cpp
    tiberiandawn/win32lib/loadpal.cpp
    tiberiandawn/win32lib/mem.cpp
    tiberiandawn/win32lib/morphpal.cpp
    tiberiandawn/win32lib/mouseww.cpp
    tiberiandawn/win32lib/newdel.cpp
    tiberiandawn/win32lib/palette.cpp
    tiberiandawn/win32lib/regionsz.cpp
    tiberiandawn/win32lib/set_font.cpp
    tiberiandawn/win32lib/timer.cpp
    tiberiandawn/win32lib/timerdwn.cpp
    tiberiandawn/win32lib/timerini.cpp
    tiberiandawn/win32lib/windows.cpp
    tiberiandawn/win32lib/winhide.cpp
    tiberiandawn/win32lib/writepcx.cpp
    tiberiandawn/win32lib/wsa.cpp
    tiberiandawn/win32lib/_diptabl.cpp
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
    target_compile_definitions(TiberianDawn PUBLIC TRUE_FALSE_DEFINED WIN32 $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE _USRDLL TIBERIANDAWN_EXPORTS REMASTER_BUILD)
    target_include_directories(TiberianDawn PUBLIC tiberiandawn/win32lib tiberiandawn)
    target_link_libraries(TiberianDawn wsock32 ws2_32 winmm)
endif()

if(BUILD_VANILLATD)
    add_executable(VanillaTD ${TIBDAWN_SRC} ${TIBDAWN_HEADERS})
    target_compile_definitions(VanillaTD PUBLIC TRUE_FALSE_DEFINED WIN32 $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
    target_include_directories(VanillaTD PUBLIC tiberiandawn/win32lib tiberiandawn)
    target_link_libraries(VanillaTD wsock32 ws2_32 winmm)
    set_target_properties(VanillaTD PROPERTIES OUTPUT_NAME vanillatd)
    if(MAP_EDITORTD)
        target_compile_definitions(VanillaTD PUBLIC SCENARIO_EDITOR CHEAT_KEYS VIRGIN_CHEAT_KEYS)
    endif()
    # Control if we auto generate a console and which "main" function we link using MSVC.
    if(MSVC)
        set_target_properties(VanillaTD PROPERTIES LINK_FLAGS "/subsystem:windows /ENTRY:WinMainCRTStartup")
    endif()
endif()

set(REDALERT_SRC
    redalert/lcwcomp.asm
    redalert/txtprnt.asm
    redalert/win32lib/tobuff.asm
    redalert/keyfbuff.asm
    redalert/2keyfram.cpp
    redalert/aadata.cpp
    redalert/abstract.cpp
    redalert/adata.cpp
    redalert/adpcm.cpp
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
    redalert/ccdde.cpp
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
    redalert/class.cpp
    redalert/colrlist.cpp
    redalert/combat.cpp
    redalert/combuf.cpp
    redalert/cominit.cpp
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
    redalert/dde.cpp
    redalert/debug.cpp
    redalert/descdlg.cpp
    redalert/dial8.cpp
    redalert/dialog.cpp
    redalert/dibfile.cpp
    redalert/dibutil.cpp
    redalert/display.cpp
    redalert/door.cpp
    redalert/dpmi.cpp
    redalert/drive.cpp
    redalert/drop.cpp
    redalert/dtable.cpp
    redalert/dynavec.cpp
    redalert/edit.cpp
    redalert/egos.cpp
    redalert/ending.cpp
    redalert/event.cpp
    redalert/expand.cpp
    redalert/face.cpp
    redalert/facing.cpp
    redalert/factory.cpp
    redalert/field.cpp
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
    redalert/internet.cpp
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
    redalert/itable.cpp
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
    redalert/nullconn.cpp
    redalert/nulldlg.cpp
    redalert/nullmgr.cpp
    redalert/object.cpp
    redalert/odata.cpp
    redalert/options.cpp
    redalert/overlay.cpp
    redalert/packet.cpp
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
    redalert/rawfile.cpp
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
    redalert/sprite.cpp
    redalert/startup.cpp
    redalert/statbtn.cpp
    redalert/stats.cpp
    redalert/straw.cpp
    redalert/stub.cpp
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
    redalert/win32lib/alloc.cpp
    redalert/win32lib/buffer.cpp
    redalert/win32lib/buffglbl.cpp
    redalert/win32lib/ddraw.cpp
    redalert/win32lib/delay.cpp
    redalert/win32lib/dipthong.cpp
    redalert/win32lib/drawmisc.cpp
    redalert/win32lib/drawrect.cpp
    redalert/win32lib/font.cpp
    redalert/win32lib/gbuffer.cpp
    redalert/win32lib/getshape.cpp
    redalert/win32lib/iconset.cpp
    redalert/win32lib/iff.cpp
    redalert/win32lib/irandom.cpp
    redalert/win32lib/load.cpp
    redalert/win32lib/loadfont.cpp
    redalert/win32lib/mouseww.cpp
    redalert/win32lib/palette.cpp
    redalert/win32lib/set_font.cpp
    redalert/win32lib/timer.cpp
    redalert/win32lib/timerdwn.cpp
    redalert/win32lib/timerini.cpp
    redalert/win32lib/windows.cpp
    redalert/win32lib/winhide.cpp
    redalert/win32lib/writepcx.cpp
    redalert/win32lib/_diptabl.cpp
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
    redalert/wspipx.cpp
    redalert/wsproto.cpp
    redalert/wspudp.cpp
    redalert/xpipe.cpp
    redalert/xstraw.cpp
)

if(CMAKE_CONFIGURATION_TYPES)
    # Glob all the header files together to add to the project for msvc/xcode.
    # Not ideal as CMake won't notice if you add any until something else prompts a CMake run
    # but this is largely a convenience feature for MSVC/XCode users.
    file(GLOB_RECURSE REDALERT_HEADERS "tiberiandawn/*.h")
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
    target_compile_definitions(RedAlert PUBLIC TRUE_FALSE_DEFINED ENGLISH WIN32 $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE _USRDLL REDALERT_EXPORTS REMASTER_BUILD)
    target_include_directories(RedAlert PUBLIC redalert/win32lib redalert)
    target_link_libraries(RedAlert wsock32 ws2_32 winmm)
endif()

if(BUILD_VANILLARA)
    add_executable(VanillaRA ${REDALERT_SRC} ${REDALERT_HEADERS})
    target_compile_definitions(VanillaRA PUBLIC TRUE_FALSE_DEFINED ENGLISH WIN32 $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
    target_include_directories(VanillaRA PUBLIC redalert/win32lib redalert)
    target_link_libraries(VanillaRA wsock32 ws2_32 winmm)
    set_target_properties(VanillaRA PROPERTIES OUTPUT_NAME vanillara)
    if(MAP_EDITORRA)
        target_compile_definitions(VanillaRA PUBLIC SCENARIO_EDITOR INTERNAL_VERSION)
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
