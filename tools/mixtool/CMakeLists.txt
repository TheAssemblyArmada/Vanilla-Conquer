add_executable(komkommermix makemix.cpp mixnamedb.cpp mixnamedb.h mixnamedb_data.cpp crc32.cpp crc32.h fastini.cpp fastini.h)
target_link_libraries(komkommermix PUBLIC common miniposix ${STATIC_LIBS})

if (WIN32)
    target_compile_definitions(komkommermix PRIVATE -DNOMINMAX)
endif()