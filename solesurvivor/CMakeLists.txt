set(SOLE_SRC
    aadata.cpp
    abstract.cpp
    adata.cpp
    aircraft.cpp
    anim.cpp
    audio.cpp
    base.cpp
    bbdata.cpp
    bdata.cpp
    building.cpp
    bullet.cpp
    cargo.cpp
    ccini.cpp
    cdata.cpp
    cell.cpp
    checkbox.cpp
    cheklist.cpp
    colrlist.cpp
    combat.cpp
    confdlg.cpp
    connect.cpp
    conquer.cpp
    const.cpp
    coord.cpp
    credits.cpp
    debug.cpp
    dial8.cpp
    dialog.cpp
    display.cpp
    door.cpp
    drive.cpp
    drop.cpp
    edit.cpp
    ending.cpp
    event.cpp
    expand.cpp
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
    iconset.cpp
    idata.cpp
    infantry.cpp
    init.cpp
    intro.cpp
    iomap.cpp
    ioobj.cpp
    ipx.cpp
    ipx95.cpp
    ipxaddr.cpp
    ipxconn.cpp
    ipxgconn.cpp
    ipxmgr.cpp
    jshell.cpp
    layer.cpp
    lib.cpp
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
    noseqcon.cpp
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
    reinf.cpp
    rules.cpp
    saveload.cpp
    scenario.cpp
    scenarioini.cpp
    score.cpp
    scroll.cpp
    sdata.cpp
    shapebtn.cpp
    sidebar.cpp
    slider.cpp
    smudge.cpp
    soleglobals.cpp
    solehelp.cpp
    soleparams.cpp
    solerandom.cpp
    sounddlg.cpp
    sounddlg.cpp
    sounddlg.cpp
    special.cpp
    startup.cpp
    stats.cpp
    super.cpp
    tab.cpp
    tarcom.cpp
    target.cpp
    tdata.cpp
    team.cpp
    teamtype.cpp
    techno.cpp
    template.cpp
    terrain.cpp
    textbtn.cpp
    theme.cpp
    trigger.cpp
    turret.cpp
    txtlabel.cpp
    udata.cpp
    unit.cpp
    visudlg.cpp
    voicethemes.cpp
    winstub.cpp
)

if(CMAKE_CONFIGURATION_TYPES)
    # Glob all the header files together to add to the project for msvc/xcode.
    # Not ideal as CMake won't notice if you add any until something else prompts a CMake run
    # but this is largely a convenience feature for MSVC/XCode users.
    file(GLOB_RECURSE SOLE_HEADERS "*.h")
    set(_SOLE_SOURCES ${SOLE_SRC})

    foreach(_SOLE_SOURCE IN ITEMS ${_SOLE_SOURCES})
        get_filename_component(_SOLE_SOURCE_PATH "${_SOLE_SOURCE}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _SOLE_GROUP_PATH "${_SOLE_SOURCE_PATH}")
        string(REPLACE "/" "\\" _SOLE_GROUP_PATH "${_SOLE_GROUP_PATH}")
        source_group("Source Files\\${_SOLE_GROUP_PATH}" FILES "${_SOLE_SOURCE}")
    endforeach()

    foreach(_SOLE_HEADER IN ITEMS ${SOLE_HEADERS})
        get_filename_component(_SOLE_HEADER_PATH "${_SOLE_HEADER}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _SOLE_HGROUP_PATH "${_SOLE_HEADER_PATH}")
        string(REPLACE "/" "\\" _SOLE_HGROUP_PATH "${_SOLE_HGROUP_PATH}")
        source_group("Header Files\\${_SOLE_HGROUP_PATH}" FILES "${_SOLE_HEADER}")
    endforeach()
endif()

include(ProductVersion)

if(BUILD_VANILLASS)
    include(BuildIcons)

    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Create the custom command for generating the ico file for windows.
        # make_icon(INPUT "${CMAKE_SOURCE_DIR}/resources/vanillatss_icon.svg" OUTPUT VANILLASS_ICON)
        # Generate windows version info.
        generate_product_version(
            VANILLASS_RC
            NAME "Vanilla Conquer VanillaSS EXE"
            BUNDLE "Vanilla Conquer"
            VERSION_MAJOR 1
            VERSION_MINOR 0
            COMPANY_NAME "Vanilla Conquer"
            COMPANY_COPYRIGHT "Based on code released under GPLv3 by EA"
            ORIGINAL_FILENAME "vanillass.exe"
            RCFILE_PREFIX "VanillaSS"
            ICON "${VANILLASS_ICON}"
        )
    endif()

    # A target must include the icon for the custom command to run.
    add_executable(VanillaSS ${SOLE_SRC} ${SOLE_HEADERS} ${VANILLASS_RC} ${VANILLASS_ICON})
    target_compile_definitions(VanillaSS PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${VANILLA_DEFS} MEGAMAPS)
    target_include_directories(VanillaSS PUBLIC ${CMAKE_SOURCE_DIR} .)
    target_link_libraries(VanillaSS commonv ${VANILLA_LIBS} ${STATIC_LIBS})
    set_target_properties(VanillaSS PROPERTIES OUTPUT_NAME vanillass)
    if(MAP_EDITORTD)
        target_compile_definitions(VanillaSS PUBLIC INTERNAL_VERSION)
    endif()
    # Control if we auto generate a console and which "main" function we link using MSVC.
    if(MSVC)
        target_link_options(VanillaSS PRIVATE /subsystem:windows /ENTRY:mainCRTStartup)
    endif()
    if(WIN32 AND NOT MSVC)
        set_target_properties(VanillaSS PROPERTIES LINK_FLAGS "-mwindows")
    endif()
endif()
