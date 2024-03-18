include (CMakeParseArguments)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(GenerateVPKCurrentDir ${CMAKE_CURRENT_LIST_DIR})

function(generate_vita_package target)
    set (options)
    set (oneValueArgs
        APP_NAME
        TITLE_ID
        ICON
        TEMPLATE
        LOAD_IMAGE
        BACKGROUND
        VERSION
    )
    set (multiValueArgs)
    cmake_parse_arguments(VITA "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    get_target_property(VITA_EXECUTABLE ${target} OUTPUT_NAME)
    
    set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")
    vita_create_self(${VITA_EXECUTABLE}.self ${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE})
    
    if(NOT VITA_APP_NAME OR "${VITA_APP_NAME}" STREQUAL "")
        set(VITA_APP_NAME "VitaApp")
    endif()
    
    if(NOT VITA_TITLE_ID OR "${VITA_TITLE_ID}" STREQUAL "")
        set(VITA_TITLE_ID "UNKN00000")
    endif()
    
    if(NOT VITA_VERSION OR "${VITA_VERSION}" STREQUAL "")
        set(VITA_VERSION "01.00")
    endif()
    
    if(NOT VITA_TEMPLATE OR "${VITA_TEMPLATE}" STREQUAL "")
        set(VITA_TEMPLATE ${GenerateVPKCurrentDir}/template.xml)
        list(APPEND VPK_FILE_LIST FILE ${VITA_TEMPLATE} sce_sys/livearea/contents/template.xml)
    endif()
    
    if(VITA_ICON)
        find_package(ImageMagick COMPONENTS magick)
    
        if(ImageMagick_magick_FOUND)
            set(ImageMagick_convert_FOUND TRUE)
            set(ImageMagick_convert_EXECUTABLE ${ImageMagick_magick_EXECUTABLE})
            set(ImageMagick_convert_ARGS convert)
        else()
            find_package(ImageMagick COMPONENTS convert)
        endif()
        
        if(NOT ImageMagick_convert_FOUND)
            message(WARNING "ImageMagick was not found, icons will not be generated.")
        endif()
        
        add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_icon0.png"
            COMMAND ${ImageMagick_convert_EXECUTABLE} ${ImageMagick_convert_ARGS} -background none -density 72 -resize 128x128 -units "PixelsPerInch" -type Palette -colors 255 ${VITA_ICON} "${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_icon0.png"
            MAIN_DEPENDENCY ${VITA_ICON}
            COMMENT "Vita Bubble Icon Generation: ${VITA_EXECUTABLE}_icon0.png"
        )
        
        add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_startup.png"
            COMMAND ${ImageMagick_convert_EXECUTABLE} ${ImageMagick_convert_ARGS} -background none -density 72 -resize 158x158 -gravity center -extent 280x158 -type Palette -units "PixelsPerInch" -colors 255 ${VITA_ICON} "${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_startup.png"
            MAIN_DEPENDENCY ${VITA_ICON}
            COMMENT "Vita Startup Icon Generation: ${VITA_EXECUTABLE}_startup.png"
        )
        
        add_custom_target(${VITA_EXECUTABLE}_icons 
            DEPENDS "${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_icon0.png" "${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_startup.png"
        )
        
        list(APPEND VPK_FILE_LIST FILE ${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_icon0.png sce_sys/icon0.png)
        list(APPEND VPK_FILE_LIST FILE ${CMAKE_BINARY_DIR}/${VITA_EXECUTABLE}_startup.png sce_sys/livearea/contents/startup.png)
        add_dependencies(${VITA_EXECUTABLE}.self-self ${VITA_EXECUTABLE}_icons)
    endif()
    
    if(VITA_LOAD_IMAGE)
        list(APPEND VPK_FILE_LIST FILE ${VITA_LOAD_IMAGE} sce_sys/pic0.png)
    endif()
    
    if(VITA_BACKGROUND)
        list(APPEND VPK_FILE_LIST FILE ${VITA_BACKGROUND} sce_sys/livearea/contents/bg.png)
    endif()
    
    set(VITA_PACK_VPK_FLAGS "")
    
    vita_create_vpk(${VITA_EXECUTABLE}.vpk ${VITA_TITLE_ID} ${CMAKE_CURRENT_BINARY_DIR}/${VITA_EXECUTABLE}.self
        VERSION ${VITA_VERSION}
        NAME ${VITA_APP_NAME}
        ${VPK_FILE_LIST}
    )
    add_dependencies(${VITA_EXECUTABLE}.self-self ${target})
endfunction()