add_executable(vanillamix makemix.cpp mixnamedb.cpp mixnamedb.h mixnamedb_data.cpp crc32.cpp crc32.h fastini.cpp fastini.h)
target_link_libraries(vanillamix PUBLIC common miniposix)

if (WIN32)
    target_compile_definitions(vanillamix PRIVATE -DNOMINMAX)
endif()