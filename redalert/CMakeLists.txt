set(REDALERT_SRC
    aadata.cpp
    abstract.cpp
    adata.cpp
    aircraft.cpp
    anim.cpp
    audio.cpp
    bar.cpp
    base.cpp
    bbdata.cpp
    bdata.cpp
    bench.cpp
    bigcheck.cpp
    building.cpp
    bullet.cpp
    cargo.cpp
    carry.cpp
    ccini.cpp
    ccptr.cpp
    cdata.cpp
    cell.cpp
    checkbox.cpp
    cheklist.cpp
    colrlist.cpp
    combat.cpp
    confdlg.cpp
    conquer.cpp
    const.cpp
    coord.cpp
    crate.cpp
    credits.cpp
    debug.cpp
    dial8.cpp
    dialog.cpp
    dibfile.cpp
    dibutil.cpp
    display.cpp
    door.cpp
    drive.cpp
    drop.cpp
    edit.cpp
    egos.cpp
    ending.cpp
    event.cpp
    expand.cpp
    face.cpp
    facing.cpp
    factory.cpp
    findpath.cpp
    flasher.cpp
    fly.cpp
    foot.cpp
    fuse.cpp
    gamedlg.cpp
    gauge.cpp
    globals.cpp
    goptions.cpp
    gscreen.cpp
    hdata.cpp
    heap.cpp
    help.cpp
    house.cpp
    iconlist.cpp
    iconset.cpp
    idata.cpp
    infantry.cpp
    init.cpp
    intro.cpp
    iomap.cpp
    ioobj.cpp
    ipx.cpp
    ipx95.cpp
    ipxconn.cpp
    ipxgconn.cpp
    ipxmgr.cpp
    jshell.cpp
    layer.cpp
    list.cpp
    loaddlg.cpp
    logic.cpp
    map.cpp
    mapeddlg.cpp
    mapedit.cpp
    mapedplc.cpp
    mapedsel.cpp
    mapedtm.cpp
    mapsel.cpp
    menus.cpp
    miscasm.cpp
    mission.cpp
    monoc.cpp
    mouse.cpp
    mplayer.cpp
    msgbox.cpp
    msglist.cpp
    netdlg.cpp
    nulldlg.cpp
    object.cpp
    odata.cpp
    options.cpp
    overlay.cpp
    power.cpp
    profile.cpp
    queue.cpp
    radar.cpp
    radio.cpp
    rawolapi.cpp
    reinf.cpp
    rules.cpp
    saveload.cpp
    scenario.cpp
    score.cpp
    scroll.cpp
    sdata.cpp
    seditdlg.cpp
    sendfile.cpp
    session.cpp
    shapebtn.cpp
    sidebar.cpp
    slider.cpp
    smudge.cpp
    sounddlg.cpp
    special.cpp
    startup.cpp
    statbtn.cpp
    stats.cpp
    super.cpp
    tab.cpp
    taction.cpp
    target.cpp
    tdata.cpp
    team.cpp
    teamtype.cpp
    techno.cpp
    template.cpp
    terrain.cpp
    tevent.cpp
    textbtn.cpp
    theme.cpp
    tooltip.cpp
    tracker.cpp
    trigger.cpp
    trigtype.cpp
    txtlabel.cpp
    udata.cpp
    unit.cpp
    vdata.cpp
    version.cpp
    vessel.cpp
    visudlg.cpp
    vortex.cpp
    warhead.cpp
    weapon.cpp
    winstub.cpp
    wol_cgam.cpp
    wol_chat.cpp
    wol_dnld.cpp
    wol_gsup.cpp
    wol_logn.cpp
    wol_main.cpp
    wol_opt.cpp
    wolapiob.cpp
    woledit.cpp
    wolstrng.cpp
)

if(CMAKE_CONFIGURATION_TYPES)
    # Glob all the header files together to add to the project for msvc/xcode.
    # Not ideal as CMake won't notice if you add any until something else prompts a CMake run
    # but this is largely a convenience feature for MSVC/XCode users.
    file(GLOB_RECURSE REDALERT_HEADERS "*.h")
    set(_REDALERT_SOURCES ${REDALERT_SRC})

    foreach(_REDALERT_SOURCE IN ITEMS ${_REDALERT_SOURCES})
        get_filename_component(_REDALERT_SOURCE_PATH "${_REDALERT_SOURCE}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _REDALERT_GROUP_PATH "${_REDALERT_SOURCE_PATH}")
        string(REPLACE "/" "\\" _REDALERT_GROUP_PATH "${_REDALERT_GROUP_PATH}")
        source_group("Source Files\\${_REDALERT_GROUP_PATH}" FILES "${_REDALERT_SOURCE}")
    endforeach()

    foreach(_REDALERT_HEADER IN ITEMS ${REDALERT_HEADERS})
        get_filename_component(_REDALERT_HEADER_PATH "${_REDALERT_HEADER}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _REDALERT_HGROUP_PATH "${_REDALERT_HEADER_PATH}")
        string(REPLACE "/" "\\" _REDALERT_HGROUP_PATH "${_REDALERT_HGROUP_PATH}")
        source_group("Header Files\\${_REDALERT_HGROUP_PATH}" FILES "${_REDALERT_HEADER}")
    endforeach()
endif()

include(ProductVersion)

if(BUILD_REMASTERRA)
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Generate windows version info.
        generate_product_version(
            REMASTERRA_RC
            NAME "Vanilla Conquer RedAlert DLL"
            BUNDLE "Vanilla Conquer"
            VERSION_MAJOR 3
            VERSION_MINOR 3
            COMPANY_NAME "Vanilla Conquer"
            COMPANY_COPYRIGHT "Based on code released under GPLv3 by EA"
            ORIGINAL_FILENAME "RedAlert.dll"
            RCFILE_PREFIX "RemasterRA"
        )
    endif()
    add_library(RedAlert SHARED 
        dllinterface.cpp
        dllinterfaceeditor.cpp
        sidebarglyphx.cpp
        ${REDALERT_SRC}
        ${REDALERT_HEADERS}
        ${REMASTERRA_RC}
    )
    target_compile_definitions(RedAlert PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${REMASTER_DEFS})
    target_include_directories(RedAlert PUBLIC ${CMAKE_SOURCE_DIR} .)
    target_link_libraries(RedAlert commonr ${REMASTER_LIBS} ${STATIC_LIBS})
    set_target_properties(RedAlert PROPERTIES PREFIX "")

    add_custom_command(TARGET RedAlert POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/remaster_mods/Vanilla_RA ${CMAKE_BINARY_DIR}/Vanilla_RA
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Vanilla_RA/Data
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:RedAlert> ${CMAKE_BINARY_DIR}/Vanilla_RA/Data/
    )
endif()

if(BUILD_VANILLARA)
    include(BuildIcons)
    # Create the custom command for generating the ico file for windows.
    make_icon(INPUT "${CMAKE_SOURCE_DIR}/resources/vanillara_icon.svg" OUTPUT VANILLARA_ICON)

    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Generate windows version info.
        generate_product_version(
            VANILLARA_RC
            NAME "VanillaRA"
            BUNDLE "Vanilla Conquer"
            VERSION_MAJOR 1
            VERSION_MINOR 0
            COMPANY_NAME "Vanilla Conquer"
            COMPANY_COPYRIGHT "Based on code released under GPLv3 by EA"
            ORIGINAL_FILENAME "vanillara.exe"
            RCFILE_PREFIX "VanillaRA"
            ICON "${VANILLARA_ICON}"
        )
    endif()

    # A target must include the icon for the custom command to run.
    add_executable(VanillaRA ${REDALERT_SRC} ${REDALERT_HEADERS} ${VANILLARA_RC} ${VANILLARA_ICON})

    if(MAKE_BUNDLE)
        set_source_files_properties(${VANILLARA_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set_target_properties(VanillaRA PROPERTIES
            MACOSX_BUNDLE True
            MACOSX_BUNDLE_GUI_IDENTIFIER com.vanilla-conquer.vanillara
            MACOSX_BUNDLE_BUNDLE_NAME VanillaRA
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
            MACOSX_BUNDLE_COPYRIGHT "Based on code released under GPLv3 by EA"
            MACOSX_BUNDLE_ICON_FILE "vanillara_icon.icns"
        )
    endif()

    target_compile_definitions(VanillaRA PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${VANILLA_DEFS})
    target_include_directories(VanillaRA PUBLIC ${CMAKE_SOURCE_DIR} .)
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
        target_link_options(VanillaRA PRIVATE /subsystem:windows /ENTRY:mainCRTStartup)
    endif()
    if(WIN32 AND NOT MSVC)
        set_target_properties(VanillaRA PROPERTIES LINK_FLAGS "-mwindows")
    endif()
    if(BUILD_WITH_UBSAN)
        target_compile_options(VanillaRA PUBLIC
                    -fsanitize=undefined,float-divide-by-zero,integer,implicit-conversion,implicit-integer-truncation,implicit-integer-arithmetic-value-change,local-bounds,nullability
                    -g3 -fno-omit-frame-pointer)
        target_link_options(VanillaRA PUBLIC -fsanitize=undefined,float-divide-by-zero,integer,implicit-conversion,implicit-integer-truncation,implicit-integer-arithmetic-value-change,local-bounds,nullability)
    endif()
    if(BUILD_WITH_ASAN)
        target_compile_options(VanillaRA PUBLIC
                    -fsanitize=address
                    -g3 -fno-omit-frame-pointer)
            target_link_options(VanillaRA PUBLIC -fsanitize=address)
    endif()
endif()