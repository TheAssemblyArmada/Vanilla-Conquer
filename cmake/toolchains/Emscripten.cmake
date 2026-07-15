# Thin, repository-local entrypoint for the Emscripten SDK toolchain.
# Keep the SDK outside the source tree and select it with EMSDK.
if(NOT DEFINED ENV{EMSDK} OR "$ENV{EMSDK}" STREQUAL "")
    message(FATAL_ERROR "EMSDK is not set. Source emsdk_env.sh before configuring the web-td preset.")
endif()

set(_CNC_EMSCRIPTEN_TOOLCHAIN
    "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
if(NOT EXISTS "${_CNC_EMSCRIPTEN_TOOLCHAIN}")
    message(FATAL_ERROR "Emscripten toolchain not found at ${_CNC_EMSCRIPTEN_TOOLCHAIN}")
endif()

include("${_CNC_EMSCRIPTEN_TOOLCHAIN}")
