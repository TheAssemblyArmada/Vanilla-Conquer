/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_BACKEND_H
#define CNC_WEB_BACKEND_H

#include "protocol.h"

#include <stdint.h>

#include <string>
#include <vector>

namespace cnc {
namespace web {

struct SnapshotSection
{
    SnapshotSection();

    uint16_t kind;
    uint16_t flags;
    uint32_t count;
    std::vector<uint8_t> payload;
    /* Optional history-independent logical payload digest for delta sections. */
    bool has_canonical_payload_hash;
    uint32_t canonical_count;
    uint32_t canonical_payload_size;
    uint64_t canonical_payload_hash;
};

struct BackendEvent
{
    BackendEvent();

    uint16_t type;
    uint16_t flags;
    uint64_t player_id;
    int32_t args[6];
    std::string text1;
    std::string text2;
};

typedef void (*BackendEventSink)(void* context, const BackendEvent& event);

class Backend
{
public:
    virtual ~Backend() {}

    virtual cnc_web_status_t Initialize(BackendEventSink sink, void* sink_context) = 0;
    virtual void Shutdown() = 0;
    virtual cnc_web_status_t Start(const StartConfig& config) = 0;
    /* This must inspect the complete batch without mutating simulation state. */
    virtual cnc_web_status_t ValidateCommands(uint64_t player_id, const std::vector<Command>& commands) = 0;
    virtual cnc_web_status_t ApplyCommands(uint64_t player_id, const std::vector<Command>& commands) = 0;
    /* out_running becomes false after the final game-over tick. */
    virtual cnc_web_status_t Advance(uint64_t player_id, bool& out_running) = 0;
    virtual cnc_web_status_t Snapshot(uint64_t player_id, std::vector<SnapshotSection>& sections) = 0;
    /* Hidden deterministic state contributes to state_hash but is not exposed
     * as a browser snapshot section. */
    virtual cnc_web_status_t DeterministicState(std::vector<uint8_t>& bytes) = 0;
    virtual cnc_web_status_t Save(std::vector<uint8_t>& bytes) = 0;
    virtual cnc_web_status_t Load(const uint8_t* bytes,
                                  uint32_t size,
                                  uint32_t game_mode,
                                  uint32_t tick,
                                  uint64_t player_id) = 0;
};

/* Coordinate helpers shared by the TD adapter and native golden tests. */
bool WorldToLegacyPixel(int32_t world,
                        int32_t camera_world,
                        int32_t tactical_screen_offset,
                        int32_t& legacy_screen);
bool LegacyWindowPixelToWorld(int32_t legacy_window, int32_t camera_world, int32_t& world);

/* Hashes a section's logical content, canonicalizing delta representations. */
uint64_t HashSnapshotSection(uint64_t hash, const SnapshotSection& section);

Backend* CreateBackend();
void DestroyBackend(Backend* backend);

} // namespace web
} // namespace cnc

#endif /* CNC_WEB_BACKEND_H */
