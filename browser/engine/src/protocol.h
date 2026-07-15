/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_INTERNAL_PROTOCOL_H
#define CNC_WEB_INTERNAL_PROTOCOL_H

#include "cnc_web.h"
#include "cnc_web_protocol.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace cnc {
namespace web {

struct MessageHeader
{
    uint32_t magic;
    uint16_t version;
    uint16_t kind;
    uint32_t byte_size;
    uint32_t count;
};

struct StartConfig
{
    StartConfig()
        : seed(0u)
        , scenario(0)
        , variation(0)
        , direction(0)
        , build_level(0)
        , sabotaged_structure(-1)
        , faction(CNC_WEB_FACTION_GDI)
        , game_mode(CNC_WEB_GAME_CAMPAIGN)
        , player_id(0u)
        , content_id_hash(0u)
        , has_campaign_transition(false)
        , carry_over_money(0)
        , nuke_pieces(0u)
    {
    }

    uint32_t seed;
    int32_t scenario;
    int32_t variation;
    int32_t direction;
    int32_t build_level;
    int32_t sabotaged_structure;
    uint32_t faction;
    uint32_t game_mode;
    uint64_t player_id;
    uint64_t content_id_hash;
    std::string content_directory;
    std::string override_map_name;
    /* Adapter-owned companion state; these fields are not part of StartV1. */
    bool has_campaign_transition;
    int32_t carry_over_money;
    uint32_t nuke_pieces;
};

struct Command
{
    uint16_t type;
    uint16_t flags;
    int32_t args[7];
};

struct CommandBatch
{
    uint32_t target_tick;
    uint64_t player_id;
    std::vector<Command> commands;
};

class Reader
{
public:
    Reader(const uint8_t* data, uint32_t size);

    bool U8(uint8_t& value);
    bool U16(uint16_t& value);
    bool U32(uint32_t& value);
    bool I32(int32_t& value);
    bool U64(uint64_t& value);
    bool Bytes(uint32_t count, const uint8_t*& value);
    bool String(uint32_t count, std::string& value);
    bool Skip(uint32_t count);

    uint32_t Position() const;
    uint32_t Remaining() const;

private:
    const uint8_t* data_;
    uint32_t size_;
    uint32_t position_;
};

class Writer
{
public:
    Writer();

    bool Reserve(uint32_t size);
    bool U8(uint8_t value);
    bool U16(uint16_t value);
    bool U32(uint32_t value);
    bool I16(int16_t value);
    bool I32(int32_t value);
    bool U64(uint64_t value);
    bool Bytes(const void* data, uint32_t size);
    bool Zeros(uint32_t size);
    bool FixedString(const char* value, uint32_t source_limit, uint32_t output_size);
    bool PatchU32(uint32_t offset, uint32_t value);

    uint32_t Size() const;
    const std::vector<uint8_t>& Data() const;
    std::vector<uint8_t>& Data();

private:
    bool Grow(uint32_t size);
    std::vector<uint8_t> data_;
};

bool DecodeHeader(Reader& reader, uint16_t expected_kind, MessageHeader& header);
bool DecodeStartConfig(const uint8_t* data, uint32_t size, StartConfig& config);
bool DecodeCommandBatch(const uint8_t* data, uint32_t size, CommandBatch& batch);

bool WriteMessageHeader(Writer& writer, uint16_t kind, uint32_t byte_size, uint32_t count);
uint64_t HashBytes(const uint8_t* data, uint32_t size, uint64_t seed = UINT64_C(14695981039346656037));
uint64_t HashString(const std::string& value, uint64_t seed = UINT64_C(14695981039346656037));
uint32_t CanonicalSeed(uint32_t seed);

} // namespace web
} // namespace cnc

#endif /* CNC_WEB_INTERNAL_PROTOCOL_H */
