/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_CLASSIC_SURFACE_H
#define CNC_WEB_CLASSIC_SURFACE_H

#include <stdint.h>

#include <vector>

namespace cnc {
namespace web {

struct ClassicSurfaceRect
{
    ClassicSurfaceRect();

    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct ClassicSurfaceEncoding
{
    ClassicSurfaceEncoding();

    std::vector<uint8_t> payload;
    ClassicSurfaceRect dirty;
    uint32_t pixel_count;
    uint32_t canonical_payload_size;
    uint64_t canonical_payload_hash;
    bool delta;
};

/*
 * Encodes a tightly packed indexed surface. With no compatible baseline the
 * payload is the format-1 full surface. Otherwise it is a format-2 minimal
 * bounding rectangle (or an empty rectangle when no pixels changed).
 *
 * The canonical hash always describes the equivalent format-1 payload so
 * snapshot state hashes do not depend on whether a client saw a bootstrap or
 * delta encoding.
 */
bool EncodeClassicSurface(const uint8_t* current,
                          uint32_t width,
                          uint32_t height,
                          const uint8_t* previous,
                          uint32_t previous_width,
                          uint32_t previous_height,
                          bool has_baseline,
                          ClassicSurfaceEncoding& encoding);

} // namespace web
} // namespace cnc

#endif /* CNC_WEB_CLASSIC_SURFACE_H */
