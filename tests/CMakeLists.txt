add_custom_target(tests)
add_dependencies(tests test_miscasm test_face)

add_executable(test_miscasm miscasm.cpp)
target_include_directories(test_miscasm PUBLIC .. ../common ../tiberiandawn ../tiberiandawn/win32lib)
target_compile_definitions(test_miscasm PUBLIC TRUE_FALSE_DEFINED ENGLISH WIN32 $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_miscasm PUBLIC common ${STATIC_LIBS})
add_test(NAME miscasm COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_miscasm>)

add_executable(test_face face.cpp)
target_include_directories(test_face PUBLIC .. ../common ../tiberiandawn ../tiberiandawn/win32lib)
target_compile_definitions(test_face PUBLIC TRUE_FALSE_DEFINED ENGLISH WIN32 $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_face PUBLIC common ${STATIC_LIBS})
add_test(NAME face COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_face>)
