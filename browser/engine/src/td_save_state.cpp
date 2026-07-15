/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "td_save_state.h"

#include "protocol.h"

#include <limits.h>

namespace cnc {
namespace web {

namespace {

const uint32_t kTdSaveMagic = UINT32_C(0x53574454); /* "TDWS" */
const uint16_t kTdSaveVersion1 = 1u;
const uint16_t kTdSaveVersion2 = 2u;
const uint16_t kTdSaveHeaderSizeV1 = 32u;
const uint16_t kTdSaveHeaderSizeV2 = 40u;
const uint32_t kTdSaveFirstUpdate = 1u << 0u;

} // namespace

TdDeterministicSaveState::TdDeterministicSaveState()
    : random_seed(0u)
    , first_update(false)
    , sabotaged_structure(-1)
{
}

bool EncodeTdSavePayload(const std::vector<uint8_t>& legacy_save,
                         const TdDeterministicSaveState& state,
                         std::vector<uint8_t>& payload)
{
    if (legacy_save.size() > UINT32_MAX - kTdSaveHeaderSizeV2) {
        return false;
    }
    const uint32_t legacy_size = static_cast<uint32_t>(legacy_save.size());
    const uint32_t total_size = kTdSaveHeaderSizeV2 + legacy_size;
    const uint64_t legacy_hash = HashBytes(legacy_save.empty() ? NULL : &legacy_save[0], legacy_size);
    Writer writer;
    if (!writer.Reserve(total_size) || !writer.U32(kTdSaveMagic) || !writer.U16(kTdSaveVersion2)
        || !writer.U16(kTdSaveHeaderSizeV2) || !writer.U32(total_size) || !writer.U32(legacy_size)
        || !writer.U64(legacy_hash) || !writer.U32(state.random_seed)
        || !writer.U32(state.first_update ? kTdSaveFirstUpdate : 0u)
        || !writer.I32(state.sabotaged_structure) || !writer.U32(0u)
        || !writer.Bytes(legacy_save.empty() ? NULL : &legacy_save[0], legacy_size)) {
        return false;
    }
    payload.swap(writer.Data());
    return true;
}

TdSavePayloadResult DecodeTdSavePayload(const uint8_t* payload,
                                        uint32_t payload_size,
                                        TdDeterministicSaveState& state,
                                        const uint8_t*& legacy_save,
                                        uint32_t& legacy_save_size)
{
    legacy_save = payload;
    legacy_save_size = payload_size;
    state = TdDeterministicSaveState();
    if (payload == NULL || payload_size < sizeof(uint32_t)) {
        return TD_SAVE_PAYLOAD_INVALID;
    }

    Reader reader(payload, payload_size);
    uint32_t magic = 0u;
    if (!reader.U32(magic)) {
        return TD_SAVE_PAYLOAD_INVALID;
    }
    if (magic != kTdSaveMagic) {
        return TD_SAVE_PAYLOAD_LEGACY;
    }

    uint16_t version = 0u;
    uint16_t header_size = 0u;
    uint32_t total_size = 0u;
    uint32_t encoded_legacy_size = 0u;
    uint64_t encoded_legacy_hash = 0u;
    uint32_t flags = 0u;
    if (!reader.U16(version) || !reader.U16(header_size) || !reader.U32(total_size)
        || !reader.U32(encoded_legacy_size) || !reader.U64(encoded_legacy_hash)
        || !reader.U32(state.random_seed) || !reader.U32(flags)
        || total_size != payload_size || (flags & ~kTdSaveFirstUpdate) != 0u) {
        legacy_save = payload;
        legacy_save_size = payload_size;
        state = TdDeterministicSaveState();
        return TD_SAVE_PAYLOAD_INVALID;
    }
    if (version == kTdSaveVersion1) {
        if (header_size != kTdSaveHeaderSizeV1) {
            legacy_save = payload;
            legacy_save_size = payload_size;
            state = TdDeterministicSaveState();
            return TD_SAVE_PAYLOAD_INVALID;
        }
        /* Version 1 predated browser preservation of SabotagedType. */
        state.sabotaged_structure = -1;
    } else if (version == kTdSaveVersion2) {
        uint32_t reserved = 0u;
        if (header_size != kTdSaveHeaderSizeV2 || !reader.I32(state.sabotaged_structure)
            || !reader.U32(reserved) || reserved != 0u) {
            legacy_save = payload;
            legacy_save_size = payload_size;
            state = TdDeterministicSaveState();
            return TD_SAVE_PAYLOAD_INVALID;
        }
    } else {
        legacy_save = payload;
        legacy_save_size = payload_size;
        state = TdDeterministicSaveState();
        return TD_SAVE_PAYLOAD_INVALID;
    }
    if (reader.Position() != header_size || encoded_legacy_size != reader.Remaining()
        || !reader.Bytes(encoded_legacy_size, legacy_save) || reader.Remaining() != 0u
        || encoded_legacy_hash != HashBytes(legacy_save, encoded_legacy_size)) {
        legacy_save = payload;
        legacy_save_size = payload_size;
        state = TdDeterministicSaveState();
        return TD_SAVE_PAYLOAD_INVALID;
    }
    legacy_save_size = encoded_legacy_size;
    state.first_update = (flags & kTdSaveFirstUpdate) != 0u;
    return TD_SAVE_PAYLOAD_OK;
}

} // namespace web
} // namespace cnc
