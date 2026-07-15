/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_STATIC_MAP_DELTA_H
#define CNC_WEB_STATIC_MAP_DELTA_H

#include <stdint.h>

#include <vector>

namespace cnc {
namespace web {

struct StaticMapEncoding
{
    StaticMapEncoding();

    std::vector<uint8_t> payload;
    uint32_t canonical_payload_size;
    uint64_t canonical_payload_hash;
    bool retained;
};

/*
 * Preserves the existing full STATIC_MAP wire payload for a bootstrap or any
 * logical change. When a compatible baseline is byte-for-byte identical, the
 * returned payload contains only the fixed metadata; the receiver retains the
 * preceding full cell array. The canonical fields always describe the full
 * logical payload so state hashes are independent of snapshot history.
 */
bool EncodeStaticMap(const std::vector<uint8_t>& full_payload,
                     uint32_t cell_count,
                     const std::vector<uint8_t>& previous_full_payload,
                     bool has_baseline,
                     StaticMapEncoding& encoding);

} // namespace web
} // namespace cnc

#endif /* CNC_WEB_STATIC_MAP_DELTA_H */
