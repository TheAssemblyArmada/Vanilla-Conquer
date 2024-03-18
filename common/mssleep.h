/** 
* @file
* @brief  Cross platform wrapper to create a sleep function with millisecond precision.
*/

#ifndef MSSLEEP_H
#define MSSLEEP_H

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#include <winbase.h>
#elif defined(_WIN32)
#include <synchapi.h>
#elif defined VITA
#include <psp2/kernel/threadmgr.h>
#else /* Assuming recent posix*/
#define __USE_POSIX199309
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#endif

/**
 * Yield the current thread for at least us microseconds.
 */
static inline void us_sleep(unsigned us)
{
#ifdef _WIN32
    Sleep((us + 999) / 1000);
#else
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
#endif
}

/**
 * Yield the current thread for at least ms milliseconds.
 */
static inline void ms_sleep(unsigned ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

#endif /* MSSLEEP_H */
