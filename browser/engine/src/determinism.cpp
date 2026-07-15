/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "determinism.h"

void* cnc_web_program_instance = NULL;

namespace cnc {
namespace web {

namespace {
uint64_t g_logical_tick = 0u;
uint32_t g_startup_seed = 0u;
bool g_is_starting = false;
}

void ResetLogicalClock()
{
    g_logical_tick = 0u;
}

void SetLogicalTick(uint64_t tick)
{
    g_logical_tick = tick;
}

void AdvanceLogicalClock()
{
    ++g_logical_tick;
}

void BeginDeterministicStartup(uint32_t seed)
{
    g_logical_tick = 0u;
    g_startup_seed = seed;
    g_is_starting = true;
}

void EndDeterministicStartup()
{
    g_is_starting = false;
}

uint64_t LogicalMilliseconds()
{
    if (g_is_starting) {
        return g_startup_seed;
    }
    /* Exact rational conversion avoids accumulating a rounded 66/67 ms step. */
    return (g_logical_tick * UINT64_C(1000)) / UINT64_C(15);
}

time_t LogicalSeconds()
{
    /* During startup this is an RNG entropy hook, so retain every seed bit. */
    return g_is_starting ? static_cast<time_t>(g_startup_seed)
                         : static_cast<time_t>(LogicalMilliseconds() / UINT64_C(1000));
}

} // namespace web
} // namespace cnc

extern "C" uint32_t cnc_web_legacy_time_ms(void)
{
    return static_cast<uint32_t>(cnc::web::LogicalMilliseconds() & UINT64_C(0xffffffff));
}

extern "C" time_t cnc_web_legacy_time_seconds(time_t* result)
{
    const time_t value = cnc::web::LogicalSeconds();
    if (result != NULL) {
        *result = value;
    }
    return value;
}
