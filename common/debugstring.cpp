#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#elif defined(SDL2_BUILD)
#include <SDL_messagebox.h>
#endif

static class DebugStateClass
{
public:
    DebugStateClass()
        : File(nullptr)
    {
        /* Windows doesn't attach to the console by default so printing to stderr does nothing. */
#ifdef _WIN32
        /* Attach to the console that started us if any */
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            /* We attached successfully, lets redirect IO to the consoles handles */
            freopen("CONIN$", "r", stdin);
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }
#endif
    }

    ~DebugStateClass()
    {
        if (File != nullptr) {
            fclose(File);
        }

        File = nullptr;
    }

    FILE* File;
} DebugState;

/**
 * Main log function, intended to be used from behind macros that pass in file and line details.
 */
void Debug_String_Log(unsigned level, const char* file, int line, const char* fmt, ...)
{
    static const char* levels[] = {"NONE", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    assert(level <= 6);

    /* If we have a file pointer set we are logging to a file */
    if (DebugState.File != nullptr) {
        va_list args;
        fprintf(DebugState.File, "%-5s %s:%d: ", levels[level], file, line);
        va_start(args, fmt);
        vfprintf(DebugState.File, fmt, args);
        fprintf(DebugState.File, "\n");
        va_end(args);
        fflush(DebugState.File);
    }

    /* Don't print file and line numbers to stderr to avoid clogging it up with too much info */
    va_list args;
    fprintf(stderr, "%-5s: ", levels[level]);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    fflush(stderr);
}

void Debug_String_File(const char* file)
{
    if (DebugState.File != nullptr) {
        fclose(DebugState.File);
    }

    DebugState.File = fopen(file, "w");
}

int FatalErrorMessageBox(const char* fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);

    /* Prefer WinAPI MessageBoxA on Windows.  */
#ifdef _WIN32
    return MessageBoxA(NULL, (LPCSTR) "Fatal Error", (LPCSTR)buffer, MB_OK | MB_ICONERROR);
#elif defined(SDL2_BUILD)
    return SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", buffer, NULL);
#else
    return 0;
#endif
}
