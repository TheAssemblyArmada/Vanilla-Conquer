// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef COMMON_DEBUGSTRING_H
#define COMMON_DEBUGSTRING_H

/*
** If we aren't building a debug build, then don't even expose the logging interface.
*/
#ifdef _DEBUG
void Debug_String_Log(unsigned level, const char* file, int line, const char* fmt, ...);
void Debug_String_File(const char* file);
#else
#define Debug_String_Log(level, file, line, fmt, ...) ((void)0)
#define Debug_String_File(file)                       ((void)0)
#endif

/* Default to debug logging */
#ifndef LOGGING_LEVEL
#define LOGGING_LEVEL 5
#endif

#define LOGLEVEL_NONE  0
#define LOGLEVEL_FATAL 1
#define LOGLEVEL_ERROR 2
#define LOGLEVEL_WARN  3
#define LOGLEVEL_INFO  4
#define LOGLEVEL_DEBUG 5
#define LOGLEVEL_TRACE 6

/* Conditionally define the function like macros for logging levels, allows only certain logging to be compiled into client program. */
#if LOGLEVEL_TRACE <= LOGGING_LEVEL && defined _DEBUG
#define DBG_TRACE(x, ...) Debug_String_Log(LOGLEVEL_TRACE, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DBG_TRACE(x, ...) ((void)0)
#endif

#if LOGLEVEL_DEBUG <= LOGGING_LEVEL && defined _DEBUG
#define DBG_LOG(x, ...) Debug_String_Log(LOGLEVEL_DEBUG, __FILE__, __LINE__, x, ##__VA_ARGS__)
#else
#define DBG_LOG(x, ...) ((void)0)
#endif

#if LOGLEVEL_INFO <= LOGGING_LEVEL && defined _DEBUG
#define DBG_INFO(x, ...) Debug_String_Log(LOGLEVEL_INFO, __FILE__, __LINE__, x, ##__VA_ARGS__)
#else
#define DBG_INFO(x, ...) ((void)0)
#endif

#if LOGLEVEL_WARN <= LOGGING_LEVEL && defined _DEBUG
#define DBG_WARN(x, ...) Debug_String_Log(LOGLEVEL_WARN, __FILE__, __LINE__, x, ##__VA_ARGS__)
#else
#define DBG_WARN(x, ...) ((void)0)
#endif

#if LOGLEVEL_ERROR <= LOGGING_LEVEL && defined _DEBUG
#define DBG_ERROR(x, ...) Debug_String_Log(LOGLEVEL_ERROR, __FILE__, __LINE__, x, ##__VA_ARGS__)
#else
#define DBG_ERROR(x, ...) ((void)0)
#endif

#if LOGLEVEL_FATAL <= LOGGING_LEVEL && defined _DEBUG
#define DBG_FATAL(x, ...)                                                                                              \
    do {                                                                                                               \
        Debug_String_Log(LOGLEVEL_FATAL, __FILE__, __LINE__, x, ##__VA_ARGS__);                                        \
        exit(1);                                                                                                       \
    } while (false)
#else
#define DBG_FATAL(x, ...) (exit(1))
#endif

#endif
