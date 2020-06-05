add_executable(test_miscasm miscasm.cpp ../common/misc.cpp ../common/miscasm.cpp ../tiberiandawn/rawfile.cpp)
target_include_directories(test_miscasm PUBLIC ../tiberiandawn ../tiberiandawn/win32lib)
target_compile_definitions(test_miscasm PUBLIC TRUE_FALSE_DEFINED WIN32 _DEBUG _WINDOWS _USRDLL INTEST)
add_test(miscasm test_miscasm)
