if(NOT BUILD_WEBTD AND NOT BUILD_TESTS)
    return()
endif()

add_library(cnc_web_adapter STATIC
    src/backend.cpp
    src/classic_surface.cpp
    src/content_preflight.cpp
    src/protocol.cpp
    src/static_map_delta.cpp
    src/td_save_state.cpp
    src/web_api.cpp
)
target_include_directories(cnc_web_adapter PUBLIC include PRIVATE src)
target_compile_features(cnc_web_adapter PUBLIC cxx_std_11)
set_target_properties(cnc_web_adapter PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(EMSCRIPTEN)
    # The adapter converts allocation failures into CNC_WEB_OUT_OF_MEMORY.
    target_compile_options(cnc_web_adapter PRIVATE -fexceptions)
endif()

if(BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    add_executable(test_cnc_web
        tests/mock_backend.cpp
        tests/web_api_test.cpp
    )
    target_include_directories(test_cnc_web PRIVATE include src)
    target_link_libraries(test_cnc_web PRIVATE cnc_web_adapter)
    target_compile_features(test_cnc_web PRIVATE cxx_std_11)
    add_test(NAME cnc_web COMMAND test_cnc_web)

    add_executable(test_cnc_web_determinism
        src/determinism.cpp
        tests/determinism_test.cpp
    )
    target_include_directories(test_cnc_web_determinism PRIVATE include src)
    target_compile_features(test_cnc_web_determinism PRIVATE cxx_std_11)
    add_test(NAME cnc_web_determinism COMMAND test_cnc_web_determinism)

    add_executable(test_cnc_web_content_preflight
        tests/content_preflight_test.cpp
    )
    target_include_directories(test_cnc_web_content_preflight PRIVATE include src)
    target_link_libraries(test_cnc_web_content_preflight PRIVATE cnc_web_adapter)
    target_compile_features(test_cnc_web_content_preflight PRIVATE cxx_std_11)
    add_test(NAME cnc_web_content_preflight COMMAND test_cnc_web_content_preflight)

    add_executable(test_cnc_web_classic_surface
        tests/classic_surface_test.cpp
    )
    target_include_directories(test_cnc_web_classic_surface PRIVATE include src)
    target_link_libraries(test_cnc_web_classic_surface PRIVATE cnc_web_adapter)
    target_compile_features(test_cnc_web_classic_surface PRIVATE cxx_std_11)
    add_test(NAME cnc_web_classic_surface COMMAND test_cnc_web_classic_surface)

    add_executable(test_cnc_web_static_map_delta
        tests/static_map_delta_test.cpp
    )
    target_include_directories(test_cnc_web_static_map_delta PRIVATE include src)
    target_link_libraries(test_cnc_web_static_map_delta PRIVATE cnc_web_adapter)
    target_compile_features(test_cnc_web_static_map_delta PRIVATE cxx_std_11)
    add_test(NAME cnc_web_static_map_delta COMMAND test_cnc_web_static_map_delta)

    add_executable(test_cnc_web_td_save_state
        tests/td_save_state_test.cpp
    )
    target_include_directories(test_cnc_web_td_save_state PRIVATE include src)
    target_link_libraries(test_cnc_web_td_save_state PRIVATE cnc_web_adapter)
    target_compile_features(test_cnc_web_td_save_state PRIVATE cxx_std_11)
    add_test(NAME cnc_web_td_save_state COMMAND test_cnc_web_td_save_state)

    add_executable(test_cnc_web_td_legacy_input_guards
        tests/td_legacy_input_guards_test.cpp
    )
    target_compile_features(test_cnc_web_td_legacy_input_guards PRIVATE cxx_std_11)
    add_test(NAME cnc_web_td_legacy_input_guards
        COMMAND test_cnc_web_td_legacy_input_guards ${CMAKE_SOURCE_DIR}/tiberiandawn/dllinterface.cpp)

    # This target is intentionally compile-only: it proves the public headers
    # remain valid C without adding a second mock-backend executable.
    add_library(test_cnc_web_c_header OBJECT tests/abi_header_c_test.c)
    target_include_directories(test_cnc_web_c_header PRIVATE include)

    if(TARGET tests)
        add_dependencies(tests
            test_cnc_web
            test_cnc_web_classic_surface
            test_cnc_web_content_preflight
            test_cnc_web_determinism
            test_cnc_web_static_map_delta
            test_cnc_web_td_save_state
            test_cnc_web_td_legacy_input_guards
            test_cnc_web_c_header
        )
    endif()
endif()
