/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_TD_SAVE_STATE_H
#define CNC_WEB_TD_SAVE_STATE_H

#include <stdint.h>

#include <vector>

namespace cnc {
namespace web {

struct TdDeterministicSaveState
{
    TdDeterministicSaveState();

    uint32_t random_seed;
    bool first_update;
    /* The original TD save omits this campaign-significant global. */
    int32_t sabotaged_structure;
};

enum TdSavePayloadResult
{
    TD_SAVE_PAYLOAD_OK,
    TD_SAVE_PAYLOAD_LEGACY,
    TD_SAVE_PAYLOAD_INVALID
};

/* Wraps the opaque legacy save with browser-only deterministic state that the
 * original save format does not retain. */
bool EncodeTdSavePayload(const std::vector<uint8_t>& legacy_save,
                         const TdDeterministicSaveState& state,
                         std::vector<uint8_t>& payload);

/* The returned legacy bytes alias payload and remain valid for its lifetime. */
TdSavePayloadResult DecodeTdSavePayload(const uint8_t* payload,
                                        uint32_t payload_size,
                                        TdDeterministicSaveState& state,
                                        const uint8_t*& legacy_save,
                                        uint32_t& legacy_save_size);

} // namespace web
} // namespace cnc

#endif /* CNC_WEB_TD_SAVE_STATE_H */
