/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocol.h"

#include <limits.h>
#include <string.h>

#include <algorithm>
#include <new>

namespace cnc {
namespace web {

namespace {

bool CheckedAdd(uint32_t left, uint32_t right, uint32_t& result)
{
    if (left > UINT32_MAX - right) {
        return false;
    }
    result = left + right;
    return true;
}

bool CheckedMultiply(uint32_t left, uint32_t right, uint32_t& result)
{
    if (left != 0u && right > UINT32_MAX / left) {
        return false;
    }
    result = left * right;
    return true;
}

bool HasEmbeddedNul(const std::string& value)
{
    return value.find('\0') != std::string::npos;
}

} // namespace

Reader::Reader(const uint8_t* data, uint32_t size)
    : data_(data)
    , size_(data == NULL ? 0u : size)
    , position_(0u)
{
}

bool Reader::U8(uint8_t& value)
{
    if (Remaining() < 1u) {
        return false;
    }
    value = data_[position_++];
    return true;
}

bool Reader::U16(uint16_t& value)
{
    if (Remaining() < 2u) {
        return false;
    }
    value = static_cast<uint16_t>(data_[position_])
        | static_cast<uint16_t>(static_cast<uint16_t>(data_[position_ + 1u]) << 8u);
    position_ += 2u;
    return true;
}

bool Reader::U32(uint32_t& value)
{
    if (Remaining() < 4u) {
        return false;
    }
    value = static_cast<uint32_t>(data_[position_]) | (static_cast<uint32_t>(data_[position_ + 1u]) << 8u)
        | (static_cast<uint32_t>(data_[position_ + 2u]) << 16u)
        | (static_cast<uint32_t>(data_[position_ + 3u]) << 24u);
    position_ += 4u;
    return true;
}

bool Reader::I32(int32_t& value)
{
    uint32_t bits = 0u;
    if (!U32(bits)) {
        return false;
    }
    memcpy(&value, &bits, sizeof(value));
    return true;
}

bool Reader::U64(uint64_t& value)
{
    uint32_t low = 0u;
    uint32_t high = 0u;
    if (!U32(low) || !U32(high)) {
        return false;
    }
    value = static_cast<uint64_t>(low) | (static_cast<uint64_t>(high) << 32u);
    return true;
}

bool Reader::Bytes(uint32_t count, const uint8_t*& value)
{
    if (Remaining() < count) {
        return false;
    }
    value = data_ + position_;
    position_ += count;
    return true;
}

bool Reader::String(uint32_t count, std::string& value)
{
    const uint8_t* bytes = NULL;
    if (!Bytes(count, bytes)) {
        return false;
    }
    try {
        value.assign(reinterpret_cast<const char*>(bytes), count);
    } catch (const std::bad_alloc&) {
        return false;
    }
    return true;
}

bool Reader::Skip(uint32_t count)
{
    const uint8_t* ignored = NULL;
    return Bytes(count, ignored);
}

uint32_t Reader::Position() const
{
    return position_;
}

uint32_t Reader::Remaining() const
{
    return size_ - position_;
}

Writer::Writer()
{
}

bool Writer::Reserve(uint32_t size)
{
    try {
        data_.reserve(size);
    } catch (const std::bad_alloc&) {
        return false;
    }
    return true;
}

bool Writer::Grow(uint32_t size)
{
    uint32_t target = 0u;
    if (!CheckedAdd(Size(), size, target)) {
        return false;
    }
    try {
        data_.resize(target);
    } catch (const std::bad_alloc&) {
        return false;
    }
    return true;
}

bool Writer::U8(uint8_t value)
{
    if (!Grow(1u)) {
        return false;
    }
    data_[Size() - 1u] = value;
    return true;
}

bool Writer::U16(uint16_t value)
{
    if (!Grow(2u)) {
        return false;
    }
    const uint32_t offset = Size() - 2u;
    data_[offset] = static_cast<uint8_t>(value & 0xffu);
    data_[offset + 1u] = static_cast<uint8_t>((value >> 8u) & 0xffu);
    return true;
}

bool Writer::U32(uint32_t value)
{
    if (!Grow(4u)) {
        return false;
    }
    return PatchU32(Size() - 4u, value);
}

bool Writer::I16(int16_t value)
{
    uint16_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return U16(bits);
}

bool Writer::I32(int32_t value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return U32(bits);
}

bool Writer::U64(uint64_t value)
{
    return U32(static_cast<uint32_t>(value & UINT64_C(0xffffffff)))
        && U32(static_cast<uint32_t>((value >> 32u) & UINT64_C(0xffffffff)));
}

bool Writer::Bytes(const void* data, uint32_t size)
{
    if (size == 0u) {
        return true;
    }
    if (data == NULL) {
        return false;
    }
    const uint32_t offset = Size();
    if (!Grow(size)) {
        return false;
    }
    memcpy(&data_[offset], data, size);
    return true;
}

bool Writer::Zeros(uint32_t size)
{
    const uint32_t offset = Size();
    if (!Grow(size)) {
        return false;
    }
    if (size != 0u) {
        memset(&data_[offset], 0, size);
    }
    return true;
}

bool Writer::FixedString(const char* value, uint32_t source_limit, uint32_t output_size)
{
    uint32_t length = 0u;
    if (value != NULL) {
        while (length < source_limit && value[length] != '\0') {
            ++length;
        }
    }
    length = std::min(length, output_size);
    if (!Bytes(value, length)) {
        return false;
    }
    return Zeros(output_size - length);
}

bool Writer::PatchU32(uint32_t offset, uint32_t value)
{
    if (offset > Size() || Size() - offset < 4u) {
        return false;
    }
    data_[offset] = static_cast<uint8_t>(value & 0xffu);
    data_[offset + 1u] = static_cast<uint8_t>((value >> 8u) & 0xffu);
    data_[offset + 2u] = static_cast<uint8_t>((value >> 16u) & 0xffu);
    data_[offset + 3u] = static_cast<uint8_t>((value >> 24u) & 0xffu);
    return true;
}

uint32_t Writer::Size() const
{
    return static_cast<uint32_t>(data_.size());
}

const std::vector<uint8_t>& Writer::Data() const
{
    return data_;
}

std::vector<uint8_t>& Writer::Data()
{
    return data_;
}

bool DecodeHeader(Reader& reader, uint16_t expected_kind, MessageHeader& header)
{
    return reader.U32(header.magic) && reader.U16(header.version) && reader.U16(header.kind)
        && reader.U32(header.byte_size) && reader.U32(header.count) && header.magic == CNC_WEB_MAGIC_MESSAGE
        && header.version == CNC_WEB_PROTOCOL_VERSION && header.kind == expected_kind;
}

bool DecodeStartConfig(const uint8_t* data, uint32_t size, StartConfig& config)
{
    if (data == NULL || size < CNC_WEB_START_FIXED_SIZE_V1) {
        return false;
    }
    Reader reader(data, size);
    MessageHeader header;
    uint32_t content_size = 0u;
    uint32_t override_size = 0u;
    uint32_t strings_size = 0u;
    uint32_t expected_size = 0u;
    if (!DecodeHeader(reader, CNC_WEB_MESSAGE_START_V1, header) || header.byte_size != size || header.count != 1u
        || !reader.U32(config.seed) || !reader.I32(config.scenario) || !reader.I32(config.variation)
        || !reader.I32(config.direction) || !reader.I32(config.build_level)
        || !reader.I32(config.sabotaged_structure) || !reader.U32(config.faction) || !reader.U32(config.game_mode)
        || !reader.U64(config.player_id) || !reader.U32(content_size) || !reader.U32(override_size)
        || !reader.U64(config.content_id_hash) || config.content_id_hash == 0u
        || content_size == 0u || content_size > 4096u || override_size > 255u
        || !CheckedAdd(content_size, override_size, strings_size)
        || !CheckedAdd(CNC_WEB_START_FIXED_SIZE_V1, strings_size, expected_size) || expected_size != size
        || !reader.String(content_size, config.content_directory)
        || !reader.String(override_size, config.override_map_name) || reader.Remaining() != 0u
        || HasEmbeddedNul(config.content_directory) || HasEmbeddedNul(config.override_map_name)) {
        return false;
    }
    return config.scenario >= 0 && config.build_level >= 0
        && config.faction >= CNC_WEB_FACTION_GDI && config.faction <= CNC_WEB_FACTION_JURASSIC
        && config.game_mode >= CNC_WEB_GAME_CAMPAIGN && config.game_mode <= CNC_WEB_GAME_SKIRMISH;
}

bool DecodeCommandBatch(const uint8_t* data, uint32_t size, CommandBatch& batch)
{
    if (data == NULL || size < CNC_WEB_COMMAND_BATCH_FIXED_SIZE_V1) {
        return false;
    }
    Reader reader(data, size);
    MessageHeader header;
    uint16_t record_size = 0u;
    uint16_t reserved = 0u;
    uint32_t records_size = 0u;
    uint32_t expected_size = 0u;
    if (!DecodeHeader(reader, CNC_WEB_MESSAGE_COMMAND_BATCH_V1, header) || header.byte_size != size
        || header.count > 4096u || !reader.U32(batch.target_tick) || !reader.U16(record_size)
        || !reader.U16(reserved) || !reader.U64(batch.player_id) || record_size != CNC_WEB_COMMAND_RECORD_SIZE_V1
        || reserved != 0u || !CheckedMultiply(header.count, record_size, records_size)
        || !CheckedAdd(CNC_WEB_COMMAND_BATCH_FIXED_SIZE_V1, records_size, expected_size) || expected_size != size) {
        return false;
    }

    try {
        batch.commands.clear();
        batch.commands.reserve(header.count);
    } catch (const std::bad_alloc&) {
        return false;
    }
    for (uint32_t index = 0u; index < header.count; ++index) {
        Command command;
        if (!reader.U16(command.type) || !reader.U16(command.flags)
            || command.type < CNC_WEB_COMMAND_INPUT || command.type > CNC_WEB_COMMAND_SELECT_OBJECT) {
            return false;
        }
        for (uint32_t argument = 0u; argument < 7u; ++argument) {
            if (!reader.I32(command.args[argument])) {
                return false;
            }
        }
        batch.commands.push_back(command);
    }
    return reader.Remaining() == 0u;
}

bool WriteMessageHeader(Writer& writer, uint16_t kind, uint32_t byte_size, uint32_t count)
{
    return writer.U32(CNC_WEB_MAGIC_MESSAGE) && writer.U16(CNC_WEB_PROTOCOL_VERSION) && writer.U16(kind)
        && writer.U32(byte_size) && writer.U32(count);
}

uint64_t HashBytes(const uint8_t* data, uint32_t size, uint64_t seed)
{
    uint64_t value = seed;
    if (data == NULL) {
        return value;
    }
    for (uint32_t index = 0u; index < size; ++index) {
        value ^= static_cast<uint64_t>(data[index]);
        value *= UINT64_C(1099511628211);
    }
    return value;
}

uint64_t HashString(const std::string& value, uint64_t seed)
{
    return HashBytes(reinterpret_cast<const uint8_t*>(value.data()), static_cast<uint32_t>(value.size()), seed);
}

uint32_t CanonicalSeed(uint32_t seed)
{
    /* The legacy engine treats zero as "choose a wall-clock seed". */
    return seed == 0u ? UINT32_C(0x6d2b79f5) : seed;
}

} // namespace web
} // namespace cnc
