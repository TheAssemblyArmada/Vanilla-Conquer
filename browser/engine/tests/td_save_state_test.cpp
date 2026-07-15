/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocol.h"
#include "td_save_state.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include <vector>

namespace {

const uint32_t kTdSaveMagic = UINT32_C(0x53574454); /* "TDWS" */

std::vector<uint8_t> VersionOnePayload(const std::vector<uint8_t>& legacy,
                                       uint32_t random_seed,
                                       bool first_update)
{
    const uint32_t header_size = 32u;
    const uint32_t total_size = header_size + static_cast<uint32_t>(legacy.size());
    cnc::web::Writer writer;
    assert(writer.U32(kTdSaveMagic));
    assert(writer.U16(1u));
    assert(writer.U16(header_size));
    assert(writer.U32(total_size));
    assert(writer.U32(static_cast<uint32_t>(legacy.size())));
    assert(writer.U64(cnc::web::HashBytes(legacy.empty() ? NULL : &legacy[0],
                                         static_cast<uint32_t>(legacy.size()))));
    assert(writer.U32(random_seed));
    assert(writer.U32(first_update ? 1u : 0u));
    assert(writer.Bytes(legacy.empty() ? NULL : &legacy[0], static_cast<uint32_t>(legacy.size())));
    assert(writer.Size() == total_size);
    return writer.Data();
}

} // namespace

int main()
{
    const uint8_t legacy_data[] = {0x43u, 0x4eu, 0x43u, 0x45u, 0x01u, 0x02u, 0x03u};
    const std::vector<uint8_t> legacy(legacy_data, legacy_data + sizeof(legacy_data));

    cnc::web::TdDeterministicSaveState written_state;
    written_state.random_seed = UINT32_C(0xfedcba98);
    written_state.first_update = true;
    written_state.sabotaged_structure = 11;
    std::vector<uint8_t> version_two;
    assert(cnc::web::EncodeTdSavePayload(legacy, written_state, version_two));
    assert(version_two.size() == legacy.size() + 40u);

    cnc::web::TdDeterministicSaveState decoded_state;
    const uint8_t* decoded_legacy = NULL;
    uint32_t decoded_legacy_size = 0u;
    assert(cnc::web::DecodeTdSavePayload(&version_two[0],
                                        static_cast<uint32_t>(version_two.size()),
                                        decoded_state,
                                        decoded_legacy,
                                        decoded_legacy_size)
           == cnc::web::TD_SAVE_PAYLOAD_OK);
    assert(decoded_state.random_seed == written_state.random_seed);
    assert(decoded_state.first_update == written_state.first_update);
    assert(decoded_state.sabotaged_structure == written_state.sabotaged_structure);
    assert(decoded_legacy_size == legacy.size());
    assert(std::vector<uint8_t>(decoded_legacy, decoded_legacy + decoded_legacy_size) == legacy);

    std::vector<uint8_t> version_one = VersionOnePayload(legacy, 0u, false);
    assert(cnc::web::DecodeTdSavePayload(&version_one[0],
                                        static_cast<uint32_t>(version_one.size()),
                                        decoded_state,
                                        decoded_legacy,
                                        decoded_legacy_size)
           == cnc::web::TD_SAVE_PAYLOAD_OK);
    assert(decoded_state.random_seed == 0u);
    assert(!decoded_state.first_update);
    assert(decoded_state.sabotaged_structure == -1);
    assert(decoded_legacy_size == legacy.size());

    std::vector<uint8_t> malformed = version_two;
    malformed[36] = 1u; /* Version-2 reserved word must stay zero. */
    assert(cnc::web::DecodeTdSavePayload(&malformed[0],
                                        static_cast<uint32_t>(malformed.size()),
                                        decoded_state,
                                        decoded_legacy,
                                        decoded_legacy_size)
           == cnc::web::TD_SAVE_PAYLOAD_INVALID);

    assert(cnc::web::DecodeTdSavePayload(&legacy[0],
                                        static_cast<uint32_t>(legacy.size()),
                                        decoded_state,
                                        decoded_legacy,
                                        decoded_legacy_size)
           == cnc::web::TD_SAVE_PAYLOAD_LEGACY);
    assert(decoded_legacy == &legacy[0] && decoded_legacy_size == legacy.size());
    return 0;
}
