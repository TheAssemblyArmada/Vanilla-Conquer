#include "framelimit.h"
#include "wwmouse.h"
#include "settings.h"
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

#include "mssleep.h"

extern WWMouseClass* WWMouse;

#if defined(SDL2_BUILD)
void Video_Render_Frame();
#endif

void Frame_Limiter(FrameLimitFlags flags)
{
    static auto frame_start = std::chrono::steady_clock::now();
#if defined(SDL2_BUILD)
    static auto render_avg = 0;

    auto render_start = std::chrono::steady_clock::now();
    auto render_remaining = std::chrono::duration_cast<std::chrono::milliseconds>(frame_start - render_start).count();

    if (!(flags & FrameLimitFlags::FL_FORCE_RENDER) && render_remaining > render_avg) {
        if (!(flags & FrameLimitFlags::FL_NO_BLOCK)) {
            ms_sleep(unsigned(render_remaining));
        } else {
            ms_sleep(1); // Unconditionally yield for minimum time.
        }
        return;
    }

    Video_Render_Frame();

    auto render_end = std::chrono::steady_clock::now();
    auto render_time = std::chrono::duration_cast<std::chrono::milliseconds>(render_end - render_start).count();

    // keep up some average so we have an idea if we need to skip a frame or not
    render_avg = (render_avg + render_time) / 2;
#endif

    if (Settings.Video.FrameLimit > 0 && !(flags & FrameLimitFlags::FL_NO_BLOCK)) {
#ifdef SDL2_BUILD
        auto frame_end = render_end;
#else
        auto frame_end = std::chrono::steady_clock::now();
#endif
        int64_t _ms_per_tick = 1000 / Settings.Video.FrameLimit;
        auto remaining =
            _ms_per_tick - std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count();
        if (remaining > 0) {
            ms_sleep(unsigned(remaining));
        }
        frame_start = std::chrono::steady_clock::now();
    }
}
