set(ASM_DIALECT "_JWASM")
set(CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST jwasm)
include(CMakeDetermineASMCompiler)
set(ASM_DIALECT)
