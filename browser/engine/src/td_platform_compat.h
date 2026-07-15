/*
 * Forced into the legacy TD translation units for the Emscripten build only.
 * It intentionally does not affect existing native targets.
 */
#ifndef CNC_WEB_TD_PLATFORM_COMPAT_H
#define CNC_WEB_TD_PLATFORM_COMPAT_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#if defined(__EMSCRIPTEN__)
/*
 * Emscripten prepends a compatibility string.h that declares strupr, while
 * the legacy library provides its own static inline. Prime both string header
 * guards under a temporary name so later includes do not conflict.
 */
#define strupr cnc_web_emscripten_strupr_declaration
#include <string.h>
#undef strupr

#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(value)
#endif
#ifndef __int64
#define __int64 long long
#endif

typedef void* HINSTANCE;
typedef uint32_t DWORD;
extern HINSTANCE cnc_web_program_instance;
#define ProgramInstance cnc_web_program_instance

inline DWORD GetModuleFileNameA(HINSTANCE, char* buffer, DWORD capacity)
{
    static const char module_name[] = "/tiberiandawn.wasm";
    if (buffer == NULL || capacity == 0u) {
        return 0u;
    }
    const DWORD length = static_cast<DWORD>(sizeof(module_name) - 1u);
    const DWORD copied = length < capacity - 1u ? length : capacity - 1u;
    memcpy(buffer, module_name, copied);
    buffer[copied] = '\0';
    return copied;
}

inline int MessageBoxA(void*, const char* message, const char*, unsigned int)
{
    if (message != NULL) {
        fprintf(stderr, "%s\n", message);
    }
    return 1;
}

#define MB_OK              0u
#define MB_ICONEXCLAMATION 0u
#define MB_ICONQUESTION    0u
#define MB_YESNO           0u
#define IDNO               0

extern "C" uint32_t cnc_web_legacy_time_ms(void);
extern "C" time_t cnc_web_legacy_time_seconds(time_t* result);
#define timeGetTime cnc_web_legacy_time_ms
#define time cnc_web_legacy_time_seconds

inline char* cnc_web_itoa(int value, char* buffer, int radix)
{
    if (buffer == NULL || (radix != 10 && radix != 16)) {
        return buffer;
    }
    snprintf(buffer, 32u, radix == 16 ? "%x" : "%d", value);
    return buffer;
}
#define itoa cnc_web_itoa

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(value) ((void)(value))
#endif
#endif

#endif /* CNC_WEB_TD_PLATFORM_COMPAT_H */
