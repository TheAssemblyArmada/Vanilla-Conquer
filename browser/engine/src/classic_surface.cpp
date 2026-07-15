/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "classic_surface.h"

#include "cnc_web_protocol.h"
#include "protocol.h"

#include <limits.h>
#include <string.h>

namespace cnc {
namespace web {

ClassicSurfaceRect::ClassicSurfaceRect()
    : x(0u)
    , y(0u)
    , width(0u)
    , height(0u)
{
}

ClassicSurfaceEncoding::ClassicSurfaceEncoding()
    : pixel_count(0u)
    , canonical_payload_size(0u)
    , canonical_payload_hash(0u)
    , delta(false)
{
}

namespace {

bool SurfaceSize(uint32_t width, uint32_t height, uint32_t& pixel_count)
{
    if (width == 0u || height == 0u || height > UINT32_MAX / width) {
        return false;
    }
    pixel_count = width * height;
    return pixel_count <= UINT32_MAX - CNC_WEB_CLASSIC_SURFACE_FIXED_SIZE_V1;
}

ClassicSurfaceRect FindDirtyRect(const uint8_t* current,
                                 const uint8_t* previous,
                                 uint32_t width,
                                 uint32_t height)
{
    ClassicSurfaceRect dirty;
    uint32_t minimum_x = width;
    uint32_t minimum_y = height;
    uint32_t maximum_x = 0u;
    uint32_t maximum_y = 0u;
    bool changed = false;

    for (uint32_t y = 0u; y < height; ++y) {
        const uint8_t* current_row = current + y * width;
        const uint8_t* previous_row = previous + y * width;
        if (memcmp(current_row, previous_row, width) == 0) {
            continue;
        }

        uint32_t left = 0u;
        while (left < width && current_row[left] == previous_row[left]) {
            ++left;
        }
        uint32_t right = width;
        while (right > left && current_row[right - 1u] == previous_row[right - 1u]) {
            --right;
        }

        if (!changed || left < minimum_x) {
            minimum_x = left;
        }
        if (!changed || right > maximum_x) {
            maximum_x = right;
        }
        if (!changed) {
            minimum_y = y;
        }
        maximum_y = y + 1u;
        changed = true;
    }

    if (changed) {
        dirty.x = minimum_x;
        dirty.y = minimum_y;
        dirty.width = maximum_x - minimum_x;
        dirty.height = maximum_y - minimum_y;
    }
    return dirty;
}

bool WriteFullHeader(Writer& writer, uint32_t width, uint32_t height)
{
    return writer.U32(width) && writer.U32(height) && writer.U32(width)
        && writer.U32(CNC_WEB_CLASSIC_SURFACE_FORMAT_FULL);
}

} // namespace

bool EncodeClassicSurface(const uint8_t* current,
                          uint32_t width,
                          uint32_t height,
                          const uint8_t* previous,
                          uint32_t previous_width,
                          uint32_t previous_height,
                          bool has_baseline,
                          ClassicSurfaceEncoding& encoding)
{
    uint32_t pixel_count = 0u;
    if (current == NULL || !SurfaceSize(width, height, pixel_count)) {
        return false;
    }

    const bool delta = has_baseline && previous != NULL && previous_width == width && previous_height == height;
    const ClassicSurfaceRect dirty = delta ? FindDirtyRect(current, previous, width, height) : ClassicSurfaceRect();
    if (delta && dirty.width != 0u && dirty.height > UINT32_MAX / dirty.width) {
        return false;
    }
    const uint32_t dirty_pixels = delta ? dirty.width * dirty.height : 0u;
    if (delta && dirty_pixels > UINT32_MAX - CNC_WEB_CLASSIC_SURFACE_DELTA_FIXED_SIZE_V1) {
        return false;
    }
    const uint32_t payload_size = delta ? CNC_WEB_CLASSIC_SURFACE_DELTA_FIXED_SIZE_V1 + dirty_pixels
                                        : CNC_WEB_CLASSIC_SURFACE_FIXED_SIZE_V1 + pixel_count;

    Writer canonical_header;
    Writer payload;
    if (!WriteFullHeader(canonical_header, width, height) || !payload.Reserve(payload_size)) {
        return false;
    }
    const uint64_t header_hash = HashBytes(&canonical_header.Data()[0], canonical_header.Size());
    const uint64_t canonical_hash = HashBytes(current, pixel_count, header_hash);

    if (!delta) {
        if (!WriteFullHeader(payload, width, height) || !payload.Bytes(current, pixel_count)) {
            return false;
        }
    } else {
        if (!payload.U32(width) || !payload.U32(height) || !payload.U32(dirty.width)
            || !payload.U32(CNC_WEB_CLASSIC_SURFACE_FORMAT_DELTA) || !payload.U32(dirty.x)
            || !payload.U32(dirty.y) || !payload.U32(dirty.width) || !payload.U32(dirty.height)) {
            return false;
        }
        for (uint32_t row = 0u; row < dirty.height; ++row) {
            const uint8_t* source = current + (dirty.y + row) * width + dirty.x;
            if (!payload.Bytes(source, dirty.width)) {
                return false;
            }
        }
    }
    if (payload.Size() != payload_size) {
        return false;
    }

    encoding.payload.swap(payload.Data());
    encoding.dirty = dirty;
    encoding.pixel_count = pixel_count;
    encoding.canonical_payload_size = CNC_WEB_CLASSIC_SURFACE_FIXED_SIZE_V1 + pixel_count;
    encoding.canonical_payload_hash = canonical_hash;
    encoding.delta = delta;
    return true;
}

} // namespace web
} // namespace cnc
