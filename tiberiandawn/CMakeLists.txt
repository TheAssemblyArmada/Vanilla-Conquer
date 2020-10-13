set(TIBDAWN_SRC
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
    cdata.cpp
    cell.cpp
    checkbox.cpp
    cheklist.cpp
    colrlist.cpp
    combat.cpp
    combuf.cpp
    comqueue.cpp
    confdlg.cpp
    connect.cpp
    conquer.cpp
    const.cpp
    control.cpp
    coord.cpp
    credits.cpp
    crew.cpp
    debug.cpp
    descdlg.cpp
    dial8.cpp
    dialog.cpp
    display.cpp
    door.cpp
    drive.cpp
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
    gadget.cpp
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
    ini.cpp
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
    miscasma.asm
    mission.cpp
    monoc.cpp
    morphpal.cpp
    mouse.cpp
    mplayer.cpp
    msgbox.cpp
    msglist.cpp
    netdlg.cpp
    noseqcon.cpp
    nullconn.cpp
    nulldlg.cpp
    nullmgr.cpp
    object.cpp
    odata.cpp
    options.cpp
    overlay.cpp
    power.cpp
    profile.cpp
    queue.cpp
    radar.cpp
    radio.cpp
    rand.cpp
    reinf.cpp
    rules.cpp
    saveload.cpp
    scenario.cpp
    score.cpp
    scroll.cpp
    sdata.cpp
    seqconn.cpp
    shape.cpp
    shapebtn.cpp
    sidebar.cpp
    slider.cpp
    smudge.cpp
    sounddlg.cpp
    special.cpp
    stamp.asm
    startup.cpp
    stats.cpp
    super.cpp
    tab.cpp
    tarcom.cpp
    target.cpp
    tcpip.cpp
    tdata.cpp
    team.cpp
    teamtype.cpp
    techno.cpp
    template.cpp
    terrain.cpp
    textbtn.cpp
    theme.cpp
    toggle.cpp
    trigger.cpp
    turret.cpp
    txtlabel.cpp
    udata.cpp
    unit.cpp
    utracker.cpp
    visudlg.cpp
    winstub.cpp
)

if(CMAKE_CONFIGURATION_TYPES)
    # Glob all the header files together to add to the project for msvc/xcode.
    # Not ideal as CMake won't notice if you add any until something else prompts a CMake run
    # but this is largely a convenience feature for MSVC/XCode users.
    file(GLOB_RECURSE TIBDAWN_HEADERS "*.h")
    set(_TIBDAWN_SOURCES ${TIBDAWN_SRC})

    foreach(_TIBDAWN_SOURCE IN ITEMS ${_TIBDAWN_SOURCES})
        get_filename_component(_TIBDAWN_SOURCE_PATH "${_TIBDAWN_SOURCE}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _TIBDAWN_GROUP_PATH "${_TIBDAWN_SOURCE_PATH}")
        string(REPLACE "/" "\\" _TIBDAWN_GROUP_PATH "${_TIBDAWN_GROUP_PATH}")
        source_group("Source Files\\${_TIBDAWN_GROUP_PATH}" FILES "${_TIBDAWN_SOURCE}")
    endforeach()

    foreach(_TIBDAWN_HEADER IN ITEMS ${TIBDAWN_HEADERS})
        get_filename_component(_TIBDAWN_HEADER_PATH "${_TIBDAWN_HEADER}" PATH)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _TIBDAWN_HGROUP_PATH "${_TIBDAWN_HEADER_PATH}")
        string(REPLACE "/" "\\" _TIBDAWN_HGROUP_PATH "${_TIBDAWN_HGROUP_PATH}")
        source_group("Header Files\\${_TIBDAWN_HGROUP_PATH}" FILES "${_TIBDAWN_HEADER}")
    endforeach()
endif()

include(ProductVersion)

if(BUILD_REMASTERTD)
    include(BuildIcons)

    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Generate windows version info.
        generate_product_version(
            REMASTERTD_RC
            NAME "Vanilla Conquer TiberianDawn DLL"
            BUNDLE "Vanilla Conquer"
            VERSION_MAJOR 1
            VERSION_MINOR 7
            COMPANY_NAME "Vanilla Conquer"
            COMPANY_COPYRIGHT "Based on code released under GPLv3 by EA"
            ORIGINAL_FILENAME "TiberianDawn.dll"
            RCFILE_PREFIX "RemasterTD"
        )
    endif()
    add_library(TiberianDawn SHARED 
        dllinterface.cpp
        dllinterfaceeditor.cpp
        sidebarglyphx.cpp
        ${TIBDAWN_SRC}
        ${TIBDAWN_HEADERS}
        ${REMASTERTD_RC}
    )
    target_compile_definitions(TiberianDawn PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${REMASTER_DEFS})
    target_include_directories(TiberianDawn PUBLIC ${CMAKE_SOURCE_DIR} .)
    target_link_libraries(TiberianDawn commonr ${REMASTER_LIBS} ${STATIC_LIBS})
    set_target_properties(TiberianDawn PROPERTIES PREFIX "")
endif()

if(BUILD_VANILLATD)
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Create the custom command for generating the ico file for windows.
        make_icon(INPUT "${CMAKE_SOURCE_DIR}/resources/vanillatd_icon.svg" OUTPUT VANILLATD_ICON)
        # Generate windows version info.
        generate_product_version(
            VANILLATD_RC
            NAME "Vanilla Conquer VanillaTD EXE"
            BUNDLE "Vanilla Conquer"
            VERSION_MAJOR 1
            VERSION_MINOR 7
            COMPANY_NAME "Vanilla Conquer"
            COMPANY_COPYRIGHT "Based on code released under GPLv3 by EA"
            ORIGINAL_FILENAME "vanillatd.exe"
            RCFILE_PREFIX "VanillaTD"
            ICON "${VANILLATD_ICON}"
        )
    endif()
    # A target must include the icon for the custom command to run.
    add_executable(VanillaTD ${TIBDAWN_SRC} ${TIBDAWN_HEADERS} ${VANILLATD_RC} ${VANILLATD_ICON})
    target_compile_definitions(VanillaTD PUBLIC $<$<CONFIG:Debug>:_DEBUG> ${VANILLA_DEFS})
    target_include_directories(VanillaTD PUBLIC ${CMAKE_SOURCE_DIR} .)
    target_link_libraries(VanillaTD commonv ${VANILLA_LIBS} ${STATIC_LIBS})
    set_target_properties(VanillaTD PROPERTIES OUTPUT_NAME vanillatd)
    if(MAP_EDITORTD)
        target_compile_definitions(VanillaTD PUBLIC INTERNAL_VERSION)
    endif()
    # Control if we auto generate a console and which "main" function we link using MSVC.
    if(MSVC)
        set_target_properties(VanillaTD PROPERTIES LINK_FLAGS "/subsystem:windows /ENTRY:WinMainCRTStartup")
    endif()
    if(WIN32 AND NOT MSVC)
        set_target_properties(VanillaTD PROPERTIES LINK_FLAGS "-mwindows")
        target_compile_definitions(VanillaTD PUBLIC main=_main)
    endif()
endif()
