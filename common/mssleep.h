/** 
* @file
* @brief  Cross platform wrapper to create a sleep function with millisecond precision.
*/

#ifndef MSSLEEP_H
#define MSSLEEP_H

#ifdef _WIN32
#include <synchapi.h>
#else /* Assuming recent posix*/
#define __USE_POSIX199309
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#endif

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
