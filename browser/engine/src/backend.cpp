/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "backend.h"

#include "protocol.h"

#include <limits.h>
#include <string.h>

namespace cnc {
namespace web {

SnapshotSection::SnapshotSection()
    : kind(0u)
    , flags(0u)
    , count(0u)
    , has_canonical_payload_hash(false)
    , canonical_count(0u)
    , canonical_payload_size(0u)
    , canonical_payload_hash(0u)
{
}

BackendEvent::BackendEvent()
    : type(0u)
    , flags(0u)
    , player_id(0u)
{
    memset(args, 0, sizeof(args));
}

bool WorldToLegacyPixel(int32_t world,
                        int32_t camera_world,
                        int32_t tactical_screen_offset,
                        int32_t& legacy_screen)
{
    const int64_t converted = static_cast<int64_t>(world) - camera_world + tactical_screen_offset;
    if (converted < INT32_MIN || converted > INT32_MAX) {
        return false;
    }
    legacy_screen = static_cast<int32_t>(converted);
    return true;
}

bool LegacyWindowPixelToWorld(int32_t legacy_window, int32_t camera_world, int32_t& world)
{
    const int64_t converted = static_cast<int64_t>(legacy_window) + camera_world;
    if (converted < INT32_MIN || converted > INT32_MAX) {
        return false;
    }
    world = static_cast<int32_t>(converted);
    return true;
}

uint64_t HashSnapshotSection(uint64_t hash, const SnapshotSection& section)
{
    Writer canonical;
    const uint32_t logical_count = section.has_canonical_payload_hash ? section.canonical_count : section.count;
    if (!canonical.U16(section.kind) || !canonical.U16(section.flags) || !canonical.U32(logical_count)) {
        return 0u;
    }
    hash = HashBytes(&canonical.Data()[0], canonical.Size(), hash);
    if (section.has_canonical_payload_hash) {
        Writer logical_payload;
        if (!logical_payload.U32(section.canonical_payload_size)
            || !logical_payload.U64(section.canonical_payload_hash)) {
            return 0u;
        }
        return HashBytes(&logical_payload.Data()[0], logical_payload.Size(), hash);
    }
    return HashBytes(section.payload.empty() ? NULL : &section.payload[0],
                     static_cast<uint32_t>(section.payload.size()),
                     hash);
}

} // namespace web
} // namespace cnc
