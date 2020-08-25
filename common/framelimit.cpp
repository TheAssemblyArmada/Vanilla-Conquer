#include "framelimit.h"
#include "wwmouse.h"
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

#include "mssleep.h"

extern WWMouseClass* WWMouse;

void Frame_Limiter()
{
    // Crude limiter to limit refresh loops to occuring 120 times a second.
    constexpr int64_t _ms_per_tick = 1000 / 120;
    static auto _last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto diff = now - _last;
    _last = now;
    auto remaining = _ms_per_tick - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();

    if (remaining > 0) {
        ms_sleep(unsigned(remaining));
    }

#ifdef SDL2_BUILD
    WWMouse->Process_Mouse();
#endif
}
