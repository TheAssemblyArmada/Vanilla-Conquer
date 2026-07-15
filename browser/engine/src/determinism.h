/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_DETERMINISM_H
#define CNC_WEB_DETERMINISM_H

#include <stdint.h>
#include <time.h>

namespace cnc {
namespace web {

void ResetLogicalClock();
void SetLogicalTick(uint64_t tick);
void AdvanceLogicalClock();
void BeginDeterministicStartup(uint32_t seed);
void EndDeterministicStartup();
uint64_t LogicalMilliseconds();

} // namespace web
} // namespace cnc

extern "C" uint32_t cnc_web_legacy_time_ms(void);
extern "C" time_t cnc_web_legacy_time_seconds(time_t* result);

#endif /* CNC_WEB_DETERMINISM_H */
