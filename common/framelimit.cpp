#include "framelimit.h"
#include "wwmouse.h"
#include "settings.h"
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

#include "mssleep.h"

extern WWMouseClass* WWMouse;

#ifdef SDL2_BUILD
void Video_Render_Frame();
#endif

void Frame_Limiter()
{
    static auto frame_start = std::chrono::steady_clock::now();
#ifdef SDL2_BUILD
    Video_Render_Frame();
#endif

    if (Settings.Video.FrameLimit > 0) {
        auto frame_end = std::chrono::steady_clock::now();
        int64_t _ms_per_tick = 1000 / Settings.Video.FrameLimit;
        auto remaining =
            _ms_per_tick - std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count();
        if (remaining > 0) {
            ms_sleep(unsigned(remaining));
        }
        frame_start = frame_end;
    }
}
