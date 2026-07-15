/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "backend.h"
#include "cnc_web_protocol.h"
#include "protocol.h"
#include "static_map_delta.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <vector>

using cnc::web::SnapshotSection;
using cnc::web::StaticMapEncoding;

namespace {

void WriteU32(std::vector<uint8_t>& bytes, uint32_t offset, uint32_t value)
{
    assert(offset <= bytes.size() && bytes.size() - offset >= 4u);
    bytes[offset] = static_cast<uint8_t>(value & 0xffu);
    bytes[offset + 1u] = static_cast<uint8_t>((value >> 8u) & 0xffu);
    bytes[offset + 2u] = static_cast<uint8_t>((value >> 16u) & 0xffu);
    bytes[offset + 3u] = static_cast<uint8_t>((value >> 24u) & 0xffu);
}

std::vector<uint8_t> FullPayload(uint32_t cell_count)
{
    std::vector<uint8_t> payload(CNC_WEB_STATIC_MAP_FIXED_SIZE_V1
                                 + cell_count * CNC_WEB_STATIC_CELL_RECORD_SIZE_V1,
                                 0u);
    WriteU32(payload, 8u, cell_count);
    WriteU32(payload, 12u, 1u);
    WriteU32(payload, CNC_WEB_STATIC_MAP_FIXED_SIZE_V1 - 4u, cell_count);
    for (uint32_t cell = 0u; cell < cell_count; ++cell) {
        const uint32_t offset = CNC_WEB_STATIC_MAP_FIXED_SIZE_V1
            + cell * CNC_WEB_STATIC_CELL_RECORD_SIZE_V1;
        payload[offset] = static_cast<uint8_t>('A' + cell % 26u);
        WriteU32(payload, offset + 32u, cell);
    }
    return payload;
}

SnapshotSection Section(const StaticMapEncoding& encoding, uint32_t count)
{
    SnapshotSection section;
    section.kind = CNC_WEB_SECTION_STATIC_MAP;
    section.count = count;
    section.payload = encoding.payload;
    section.has_canonical_payload_hash = true;
    section.canonical_count = count;
    section.canonical_payload_size = encoding.canonical_payload_size;
    section.canonical_payload_hash = encoding.canonical_payload_hash;
    return section;
}

} // namespace

int main()
{
    const uint32_t count = 4u;
    const std::vector<uint8_t> full_payload = FullPayload(count);
    const std::vector<uint8_t> no_baseline;

    StaticMapEncoding bootstrap;
    assert(cnc::web::EncodeStaticMap(full_payload, count, no_baseline, false, bootstrap));
    assert(!bootstrap.retained);
    assert(bootstrap.payload == full_payload);
    assert(bootstrap.canonical_payload_size == full_payload.size());
    assert(bootstrap.canonical_payload_hash
           == cnc::web::HashBytes(&full_payload[0], static_cast<uint32_t>(full_payload.size())));

    StaticMapEncoding retained;
    assert(cnc::web::EncodeStaticMap(full_payload, count, full_payload, true, retained));
    assert(retained.retained);
    assert(retained.payload.size() == CNC_WEB_STATIC_MAP_FIXED_SIZE_V1);
    assert(std::equal(retained.payload.begin(), retained.payload.end(), full_payload.begin()));
    assert(retained.canonical_payload_size == bootstrap.canonical_payload_size);
    assert(retained.canonical_payload_hash == bootstrap.canonical_payload_hash);

    const SnapshotSection full_section = Section(bootstrap, count);
    const SnapshotSection retained_section = Section(retained, count);
    assert(cnc::web::HashSnapshotSection(UINT64_C(0xcbf29ce484222325), full_section)
           == cnc::web::HashSnapshotSection(UINT64_C(0xcbf29ce484222325), retained_section));

    std::vector<uint8_t> changed_cell = full_payload;
    changed_cell[CNC_WEB_STATIC_MAP_FIXED_SIZE_V1 + CNC_WEB_STATIC_CELL_RECORD_SIZE_V1 + 3u] ^= 1u;
    StaticMapEncoding changed;
    assert(cnc::web::EncodeStaticMap(changed_cell, count, full_payload, true, changed));
    assert(!changed.retained && changed.payload == changed_cell);
    assert(changed.canonical_payload_hash != bootstrap.canonical_payload_hash);

    std::vector<uint8_t> changed_metadata = full_payload;
    changed_metadata[36u] = 'Z';
    StaticMapEncoding metadata;
    assert(cnc::web::EncodeStaticMap(changed_metadata, count, full_payload, true, metadata));
    assert(!metadata.retained && metadata.payload == changed_metadata);

    StaticMapEncoding reset;
    assert(cnc::web::EncodeStaticMap(full_payload, count, full_payload, false, reset));
    assert(!reset.retained && reset.payload == full_payload);

    std::vector<uint8_t> malformed = full_payload;
    malformed.pop_back();
    StaticMapEncoding invalid;
    assert(!cnc::web::EncodeStaticMap(malformed, count, no_baseline, false, invalid));
    malformed = full_payload;
    WriteU32(malformed, CNC_WEB_STATIC_MAP_FIXED_SIZE_V1 - 4u, count - 1u);
    assert(!cnc::web::EncodeStaticMap(malformed, count, no_baseline, false, invalid));
    assert(!cnc::web::EncodeStaticMap(full_payload, 0u, no_baseline, false, invalid));
    assert(!cnc::web::EncodeStaticMap(full_payload, UINT32_MAX, no_baseline, false, invalid));

    const std::vector<uint8_t> representative_64 = FullPayload(64u * 64u);
    StaticMapEncoding representative_64_retained;
    assert(cnc::web::EncodeStaticMap(representative_64,
                                     64u * 64u,
                                     representative_64,
                                     true,
                                     representative_64_retained));
    assert(representative_64.size() == 147760u);
    assert(representative_64_retained.payload.size() == 304u);

    const std::vector<uint8_t> representative_128 = FullPayload(128u * 128u);
    StaticMapEncoding representative_128_retained;
    assert(cnc::web::EncodeStaticMap(representative_128,
                                     128u * 128u,
                                     representative_128,
                                     true,
                                     representative_128_retained));
    assert(representative_128.size() == 590128u);
    assert(representative_128_retained.payload.size() == 304u);
    printf("static_map_sizes full_64=%u retained_64=%u full_128=%u retained_128=%u\n",
           static_cast<unsigned int>(representative_64.size()),
           static_cast<unsigned int>(representative_64_retained.payload.size()),
           static_cast<unsigned int>(representative_128.size()),
           static_cast<unsigned int>(representative_128_retained.payload.size()));
    return 0;
}
