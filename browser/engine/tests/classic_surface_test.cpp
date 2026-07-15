/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "classic_surface.h"
#include "backend.h"
#include "cnc_web_protocol.h"
#include "protocol.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <vector>

using cnc::web::ClassicSurfaceEncoding;
using cnc::web::Reader;
using cnc::web::SnapshotSection;

namespace {

struct SurfaceHeader
{
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t format;
    uint32_t x;
    uint32_t y;
    uint32_t rect_width;
    uint32_t rect_height;
};

SurfaceHeader ReadSurfaceHeader(const std::vector<uint8_t>& payload)
{
    Reader reader(&payload[0], static_cast<uint32_t>(payload.size()));
    SurfaceHeader header = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
    assert(reader.U32(header.width) && reader.U32(header.height) && reader.U32(header.pitch)
           && reader.U32(header.format));
    if (header.format == CNC_WEB_CLASSIC_SURFACE_FORMAT_DELTA) {
        assert(reader.U32(header.x) && reader.U32(header.y) && reader.U32(header.rect_width)
               && reader.U32(header.rect_height));
    }
    return header;
}

void ApplyDelta(const ClassicSurfaceEncoding& encoding, std::vector<uint8_t>& destination)
{
    const SurfaceHeader header = ReadSurfaceHeader(encoding.payload);
    assert(header.format == CNC_WEB_CLASSIC_SURFACE_FORMAT_DELTA);
    assert(header.pitch == header.rect_width);
    assert(destination.size() == static_cast<size_t>(header.width) * header.height);
    const uint8_t* pixels = &encoding.payload[CNC_WEB_CLASSIC_SURFACE_DELTA_FIXED_SIZE_V1];
    for (uint32_t row = 0u; row < header.rect_height; ++row) {
        memcpy(&destination[(header.y + row) * header.width + header.x],
               pixels + row * header.pitch,
               header.rect_width);
    }
}

} // namespace

int main()
{
    const uint8_t first_pixels[] = {
        0u, 1u, 2u, 3u,
        4u, 5u, 6u, 7u,
        8u, 9u, 10u, 11u,
    };
    ClassicSurfaceEncoding full;
    assert(cnc::web::EncodeClassicSurface(first_pixels, 4u, 3u, NULL, 0u, 0u, false, full));
    assert(!full.delta);
    assert(full.payload.size() == CNC_WEB_CLASSIC_SURFACE_FIXED_SIZE_V1 + sizeof(first_pixels));
    assert(full.pixel_count == sizeof(first_pixels));
    assert(full.canonical_payload_size == full.payload.size());
    assert(full.canonical_payload_hash
           == cnc::web::HashBytes(&full.payload[0], static_cast<uint32_t>(full.payload.size())));
    SurfaceHeader header = ReadSurfaceHeader(full.payload);
    assert(header.width == 4u && header.height == 3u && header.pitch == 4u
           && header.format == CNC_WEB_CLASSIC_SURFACE_FORMAT_FULL);
    assert(memcmp(&full.payload[CNC_WEB_CLASSIC_SURFACE_FIXED_SIZE_V1], first_pixels, sizeof(first_pixels)) == 0);

    ClassicSurfaceEncoding unchanged;
    assert(cnc::web::EncodeClassicSurface(first_pixels, 4u, 3u, first_pixels, 4u, 3u, true, unchanged));
    assert(unchanged.delta);
    assert(unchanged.payload.size() == CNC_WEB_CLASSIC_SURFACE_DELTA_FIXED_SIZE_V1);
    assert(unchanged.canonical_payload_hash == full.canonical_payload_hash);
    header = ReadSurfaceHeader(unchanged.payload);
    assert(header.width == 4u && header.height == 3u && header.pitch == 0u
           && header.format == CNC_WEB_CLASSIC_SURFACE_FORMAT_DELTA);
    assert(header.x == 0u && header.y == 0u && header.rect_width == 0u && header.rect_height == 0u);

    std::vector<uint8_t> changed(first_pixels, first_pixels + sizeof(first_pixels));
    changed[3] = 13u;
    changed[9] = 19u;
    ClassicSurfaceEncoding delta;
    assert(cnc::web::EncodeClassicSurface(&changed[0], 4u, 3u, first_pixels, 4u, 3u, true, delta));
    assert(delta.delta);
    header = ReadSurfaceHeader(delta.payload);
    assert(header.width == 4u && header.height == 3u && header.pitch == 3u);
    assert(header.x == 1u && header.y == 0u && header.rect_width == 3u && header.rect_height == 3u);
    assert(delta.payload.size() == CNC_WEB_CLASSIC_SURFACE_DELTA_FIXED_SIZE_V1 + 9u);
    const uint8_t expected_delta[] = {1u, 2u, 13u, 5u, 6u, 7u, 19u, 10u, 11u};
    assert(memcmp(&delta.payload[CNC_WEB_CLASSIC_SURFACE_DELTA_FIXED_SIZE_V1],
                  expected_delta,
                  sizeof(expected_delta))
           == 0);
    std::vector<uint8_t> reconstructed(first_pixels, first_pixels + sizeof(first_pixels));
    ApplyDelta(delta, reconstructed);
    assert(reconstructed == changed);

    ClassicSurfaceEncoding canonical_full;
    assert(cnc::web::EncodeClassicSurface(&changed[0], 4u, 3u, NULL, 0u, 0u, false, canonical_full));
    assert(canonical_full.canonical_payload_hash == delta.canonical_payload_hash);
    assert(canonical_full.canonical_payload_size == delta.canonical_payload_size);
    assert(canonical_full.pixel_count == delta.pixel_count);

    SnapshotSection full_section;
    full_section.kind = CNC_WEB_SECTION_CLASSIC_SURFACE;
    full_section.count = canonical_full.pixel_count;
    full_section.payload = canonical_full.payload;
    full_section.has_canonical_payload_hash = true;
    full_section.canonical_count = canonical_full.pixel_count;
    full_section.canonical_payload_size = canonical_full.canonical_payload_size;
    full_section.canonical_payload_hash = canonical_full.canonical_payload_hash;
    SnapshotSection delta_section;
    delta_section.kind = CNC_WEB_SECTION_CLASSIC_SURFACE;
    /* Deliberately use a wire count that differs from the full section. */
    delta_section.count = delta.dirty.width * delta.dirty.height;
    delta_section.payload = delta.payload;
    delta_section.has_canonical_payload_hash = true;
    delta_section.canonical_count = delta.pixel_count;
    delta_section.canonical_payload_size = delta.canonical_payload_size;
    delta_section.canonical_payload_hash = delta.canonical_payload_hash;
    assert(cnc::web::HashSnapshotSection(UINT64_C(0xcbf29ce484222325), full_section)
           == cnc::web::HashSnapshotSection(UINT64_C(0xcbf29ce484222325), delta_section));

    ClassicSurfaceEncoding resized;
    assert(cnc::web::EncodeClassicSurface(first_pixels, 3u, 4u, first_pixels, 4u, 3u, true, resized));
    assert(!resized.delta);
    header = ReadSurfaceHeader(resized.payload);
    assert(header.width == 3u && header.height == 4u && header.format == CNC_WEB_CLASSIC_SURFACE_FORMAT_FULL);

    ClassicSurfaceEncoding invalid;
    assert(!cnc::web::EncodeClassicSurface(NULL, 4u, 3u, NULL, 0u, 0u, false, invalid));
    assert(!cnc::web::EncodeClassicSurface(first_pixels, 0u, 3u, NULL, 0u, 0u, false, invalid));
    assert(!cnc::web::EncodeClassicSurface(first_pixels, UINT32_MAX, 2u, NULL, 0u, 0u, false, invalid));

    const uint32_t representative_width = 64u * 24u;
    const uint32_t representative_height = 64u * 24u;
    const uint32_t representative_pixels = representative_width * representative_height;
    std::vector<uint8_t> representative(representative_pixels, 7u);
    ClassicSurfaceEncoding representative_full;
    assert(cnc::web::EncodeClassicSurface(&representative[0],
                                          representative_width,
                                          representative_height,
                                          NULL,
                                          0u,
                                          0u,
                                          false,
                                          representative_full));
    ClassicSurfaceEncoding representative_unchanged;
    assert(cnc::web::EncodeClassicSurface(&representative[0],
                                          representative_width,
                                          representative_height,
                                          &representative[0],
                                          representative_width,
                                          representative_height,
                                          true,
                                          representative_unchanged));
    std::vector<uint8_t> one_pixel_changed = representative;
    one_pixel_changed[representative_pixels / 2u] = 8u;
    ClassicSurfaceEncoding representative_one_pixel;
    assert(cnc::web::EncodeClassicSurface(&one_pixel_changed[0],
                                          representative_width,
                                          representative_height,
                                          &representative[0],
                                          representative_width,
                                          representative_height,
                                          true,
                                          representative_one_pixel));
    assert(representative_full.payload.size() == 2359312u);
    assert(representative_unchanged.payload.size() == 32u);
    assert(representative_one_pixel.payload.size() == 33u);
    printf("classic_surface_sizes_1536x1536 full=%u unchanged=%u one_pixel=%u\n",
           static_cast<unsigned int>(representative_full.payload.size()),
           static_cast<unsigned int>(representative_unchanged.payload.size()),
           static_cast<unsigned int>(representative_one_pixel.payload.size()));
    return 0;
}
