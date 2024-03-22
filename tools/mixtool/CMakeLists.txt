add_executable(vanillamix makemix.cpp mixnamedb.cpp mixnamedb.h mixnamedb_data.cpp crc32.cpp crc32.h fastini.cpp fastini.h)
target_link_libraries(vanillamix PUBLIC common miniposix ${STATIC_LIBS})

if (WIN32)
    target_compile_definitions(vanillamix PRIVATE -DNOMINMAX)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "m68k-atari-mint")
    target_link_options(vanillamix PRIVATE -Wl,-stack,512k)
endif()
