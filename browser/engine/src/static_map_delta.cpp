/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "static_map_delta.h"

#include "cnc_web_protocol.h"
#include "protocol.h"

#include <limits.h>

#include <new>

namespace cnc {
namespace web {

StaticMapEncoding::StaticMapEncoding()
    : canonical_payload_size(0u)
    , canonical_payload_hash(0u)
    , retained(false)
{
}

namespace {

uint32_t ReadLittleEndianU32(const std::vector<uint8_t>& bytes, uint32_t offset)
{
    return static_cast<uint32_t>(bytes[offset]) | (static_cast<uint32_t>(bytes[offset + 1u]) << 8u)
        | (static_cast<uint32_t>(bytes[offset + 2u]) << 16u)
        | (static_cast<uint32_t>(bytes[offset + 3u]) << 24u);
}

} // namespace

bool EncodeStaticMap(const std::vector<uint8_t>& full_payload,
                     uint32_t cell_count,
                     const std::vector<uint8_t>& previous_full_payload,
                     bool has_baseline,
                     StaticMapEncoding& encoding)
{
    if (cell_count == 0u
        || cell_count > (UINT32_MAX - CNC_WEB_STATIC_MAP_FIXED_SIZE_V1) / CNC_WEB_STATIC_CELL_RECORD_SIZE_V1) {
        return false;
    }
    const uint32_t expected_size = CNC_WEB_STATIC_MAP_FIXED_SIZE_V1
        + cell_count * CNC_WEB_STATIC_CELL_RECORD_SIZE_V1;
    if (full_payload.size() != expected_size
        || ReadLittleEndianU32(full_payload, CNC_WEB_STATIC_MAP_FIXED_SIZE_V1 - 4u) != cell_count) {
        return false;
    }

    StaticMapEncoding next;
    next.retained = has_baseline && previous_full_payload == full_payload;
    next.canonical_payload_size = expected_size;
    next.canonical_payload_hash = HashBytes(&full_payload[0], expected_size);
    try {
        const uint32_t wire_size = next.retained ? CNC_WEB_STATIC_MAP_FIXED_SIZE_V1 : expected_size;
        next.payload.assign(full_payload.begin(), full_payload.begin() + wire_size);
    } catch (const std::bad_alloc&) {
        return false;
    }

    encoding.payload.swap(next.payload);
    encoding.canonical_payload_size = next.canonical_payload_size;
    encoding.canonical_payload_hash = next.canonical_payload_hash;
    encoding.retained = next.retained;
    return true;
}

} // namespace web
} // namespace cnc
