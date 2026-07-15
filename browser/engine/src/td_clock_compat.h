/* Force the legacy timer implementation onto the simulation's logical clock. */
#ifndef CNC_WEB_TD_CLOCK_COMPAT_H
#define CNC_WEB_TD_CLOCK_COMPAT_H

#include "td_platform_compat.h"

#if defined(__EMSCRIPTEN__)
#include <chrono>

namespace std {
namespace chrono {

class cnc_web_system_clock
{
public:
    typedef milliseconds::rep rep;
    typedef milliseconds::period period;
    typedef milliseconds duration;
    typedef chrono::time_point<cnc_web_system_clock, duration> time_point;
    static const bool is_steady = true;

    static time_point now()
    {
        return time_point(duration(cnc_web_legacy_time_ms()));
    }
};

} // namespace chrono
} // namespace std

#define system_clock cnc_web_system_clock
#endif

#endif /* CNC_WEB_TD_CLOCK_COMPAT_H */
