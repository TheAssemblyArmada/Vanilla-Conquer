# String table convertor program
find_package(ICU COMPONENTS data i18n io tu uc)

add_executable(stringtool strgen.cpp ini2str.cpp strini.cpp ini2str.h strini.h)
target_link_libraries(stringtool PRIVATE common miniposix)

if (NOT ICU_FOUND AND NOT WIN32)
    message(FATAL_ERROR "ICU library is required for none windows platforms.")
elseif (ICU_FOUND)
    target_link_libraries(stringtool PRIVATE ICU::data ICU::i18n ICU::io ICU::tu ICU::uc)
    target_compile_definitions(stringtool PRIVATE -DBUILD_WITH_ICU)
endif()


# Control if we auto generate a console and which "main" function we link using MSVC.
if(MSVC)
    target_link_options(stringtool PRIVATE /subsystem:console /ENTRY:mainCRTStartup)
endif()
if(WIN32 AND NOT MSVC)
    set_target_properties(stringtool PROPERTIES LINK_FLAGS "-mconsole")
endif()
