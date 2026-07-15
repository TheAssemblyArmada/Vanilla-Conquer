/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cnc_web.h"
#include "cnc_web_protocol.h"
#include "backend.h"
#include "protocol.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <string>
#include <vector>

using cnc::web::Reader;
using cnc::web::Writer;

namespace {

std::vector<uint8_t> StartMessage(uint32_t seed)
{
    const char content[] = "/content";
    const uint32_t size = CNC_WEB_START_FIXED_SIZE_V1 + sizeof(content) - 1u;
    Writer writer;
    assert(cnc::web::WriteMessageHeader(writer, CNC_WEB_MESSAGE_START_V1, size, 1u));
    assert(writer.U32(seed));
    assert(writer.I32(1));
    assert(writer.I32(0));
    assert(writer.I32(0));
    assert(writer.I32(1));
    assert(writer.I32(-1));
    assert(writer.U32(CNC_WEB_FACTION_GDI));
    assert(writer.U32(CNC_WEB_GAME_CAMPAIGN));
    assert(writer.U64(42u));
    assert(writer.U32(sizeof(content) - 1u));
    assert(writer.U32(0u));
    assert(writer.U64(UINT64_C(0xfedcba9876543210)));
    assert(writer.Bytes(content, sizeof(content) - 1u));
    return writer.Data();
}

std::vector<uint8_t> CommandMessage(uint32_t tick)
{
    const uint32_t size = CNC_WEB_COMMAND_BATCH_FIXED_SIZE_V1 + CNC_WEB_COMMAND_RECORD_SIZE_V1;
    Writer writer;
    assert(cnc::web::WriteMessageHeader(writer, CNC_WEB_MESSAGE_COMMAND_BATCH_V1, size, 1u));
    assert(writer.U32(tick));
    assert(writer.U16(CNC_WEB_COMMAND_RECORD_SIZE_V1));
    assert(writer.U16(0u));
    assert(writer.U64(42u));
    assert(writer.U16(CNC_WEB_COMMAND_INPUT));
    assert(writer.U16(CNC_WEB_MODIFIER_SHIFT));
    for (int32_t argument = 0; argument < 7; ++argument) {
        assert(writer.I32(argument));
    }
    return writer.Data();
}

std::vector<uint8_t> MixedInvalidCommandMessage(uint32_t tick)
{
    const uint32_t count = 2u;
    const uint32_t size = CNC_WEB_COMMAND_BATCH_FIXED_SIZE_V1 + count * CNC_WEB_COMMAND_RECORD_SIZE_V1;
    Writer writer;
    assert(cnc::web::WriteMessageHeader(writer, CNC_WEB_MESSAGE_COMMAND_BATCH_V1, size, count));
    assert(writer.U32(tick));
    assert(writer.U16(CNC_WEB_COMMAND_RECORD_SIZE_V1));
    assert(writer.U16(0u));
    assert(writer.U64(42u));
    for (uint32_t index = 0u; index < count; ++index) {
        assert(writer.U16(CNC_WEB_COMMAND_INPUT));
        assert(writer.U16(index == 0u ? CNC_WEB_MODIFIER_SHIFT : UINT16_C(0x8000)));
        for (int32_t argument = 0; argument < 7; ++argument) {
            assert(writer.I32(argument));
        }
    }
    return writer.Data();
}

std::vector<uint8_t> TerminalCommandMessage(uint32_t tick)
{
    const uint32_t size = CNC_WEB_COMMAND_BATCH_FIXED_SIZE_V1 + CNC_WEB_COMMAND_RECORD_SIZE_V1;
    Writer writer;
    assert(cnc::web::WriteMessageHeader(writer, CNC_WEB_MESSAGE_COMMAND_BATCH_V1, size, 1u));
    assert(writer.U32(tick));
    assert(writer.U16(CNC_WEB_COMMAND_RECORD_SIZE_V1));
    assert(writer.U16(0u));
    assert(writer.U64(42u));
    assert(writer.U16(CNC_WEB_COMMAND_GAME));
    assert(writer.U16(0u));
    assert(writer.I32(1));
    for (uint32_t argument = 1u; argument < 7u; ++argument) {
        assert(writer.I32(0));
    }
    return writer.Data();
}

void CheckHeader(const std::vector<uint8_t>& bytes, uint16_t expected_kind)
{
    Reader reader(&bytes[0], static_cast<uint32_t>(bytes.size()));
    cnc::web::MessageHeader header;
    assert(cnc::web::DecodeHeader(reader, expected_kind, header));
    assert(header.byte_size == bytes.size());
}

void PatchU64(std::vector<uint8_t>& bytes, uint32_t offset, uint64_t value)
{
    assert(offset <= bytes.size() && bytes.size() - offset >= 8u);
    for (uint32_t index = 0u; index < 8u; ++index) {
        bytes[offset + index] = static_cast<uint8_t>((value >> (index * 8u)) & UINT64_C(0xff));
    }
}

void PatchU32(std::vector<uint8_t>& bytes, uint32_t offset, uint32_t value)
{
    assert(offset <= bytes.size() && bytes.size() - offset >= 4u);
    for (uint32_t index = 0u; index < 4u; ++index) {
        bytes[offset + index] = static_cast<uint8_t>((value >> (index * 8u)) & UINT32_C(0xff));
    }
}

struct SnapshotClock
{
    uint32_t tick;
    uint32_t base_tick;
};

std::vector<uint8_t> CopySnapshot(cnc_web_handle_t handle)
{
    uint32_t size = 0u;
    uint32_t written = 0u;
    assert(cnc_web_snapshot_size(handle, &size) == CNC_WEB_OK);
    assert(size >= CNC_WEB_SNAPSHOT_FIXED_SIZE_V1);
    std::vector<uint8_t> snapshot(size);
    assert(cnc_web_write_snapshot(handle, &snapshot[0], size, &written) == CNC_WEB_OK);
    assert(written == size);
    return snapshot;
}

SnapshotClock ReadSnapshotClock(const std::vector<uint8_t>& snapshot)
{
    Reader reader(&snapshot[0], static_cast<uint32_t>(snapshot.size()));
    cnc::web::MessageHeader header;
    SnapshotClock clock = {UINT32_MAX, UINT32_MAX};
    assert(cnc::web::DecodeHeader(reader, CNC_WEB_MESSAGE_SNAPSHOT_V1, header));
    assert(reader.U32(clock.tick) && reader.U32(clock.base_tick));
    return clock;
}

} // namespace

int main()
{
    /* Snapshot representation changes are negotiated by ABI version 2; the
     * persistent Start/Command/Event/WebSave wire protocol remains version 1. */
    assert(CNC_WEB_ABI_VERSION == 2u);
    assert(CNC_WEB_PROTOCOL_VERSION == 1u);
    int32_t converted = 0;
    assert(cnc::web::WorldToLegacyPixel(140, 100, 8, converted) && converted == 48);
    assert(cnc::web::LegacyWindowPixelToWorld(40, 100, converted) && converted == 140);
    assert(!cnc::web::WorldToLegacyPixel(INT32_MAX, INT32_MIN, 0, converted));

    assert(cnc_web_abi_version() == CNC_WEB_ABI_VERSION);
    cnc_web_handle_t handle = 99u;
    assert(cnc_web_create(CNC_WEB_ABI_VERSION + 1u, &handle) == CNC_WEB_INVALID_ARGUMENT);
    assert(handle == 99u);
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);
    assert(handle != CNC_WEB_INVALID_HANDLE);

    cnc_web_handle_t second = 0u;
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &second) == CNC_WEB_INVALID_STATE);
    assert(cnc_web_advance(handle, 1u, NULL) == CNC_WEB_INVALID_STATE);
    assert(cnc_web_set_campaign_transition(CNC_WEB_INVALID_HANDLE, 0, 0u) == CNC_WEB_INVALID_ARGUMENT);
    assert(cnc_web_set_campaign_transition(handle, 0, 8u) == CNC_WEB_INVALID_ARGUMENT);
    assert(cnc_web_set_campaign_transition(handle, INT32_MIN, 5u) == CNC_WEB_OK);
    assert(cnc_web_set_campaign_transition(handle, 0, 0u) == CNC_WEB_INVALID_STATE);

    std::vector<uint8_t> start = StartMessage(0u);
    std::vector<uint8_t> skirmish_start = start;
    PatchU32(skirmish_start, 44u, CNC_WEB_GAME_SKIRMISH);
    assert(cnc_web_start(handle, &skirmish_start[0], static_cast<uint32_t>(skirmish_start.size()))
           == CNC_WEB_INVALID_ARGUMENT);
    std::vector<uint8_t> failed_start = start;
    const char missing[] = "/missing";
    assert(sizeof(missing) - 1u == failed_start.size() - CNC_WEB_START_FIXED_SIZE_V1);
    memcpy(&failed_start[CNC_WEB_START_FIXED_SIZE_V1], missing, sizeof(missing) - 1u);
    assert(cnc_web_start(handle, &failed_start[0], static_cast<uint32_t>(failed_start.size()))
           == CNC_WEB_CONTENT_MISMATCH);
    uint32_t event_size = 0u;
    uint32_t written = 0u;
    assert(cnc_web_event_size(handle, &event_size) == CNC_WEB_OK);
    assert(event_size >= CNC_WEB_EVENT_FIXED_SIZE_V1);
    std::vector<uint8_t> failed_event(event_size);
    assert(cnc_web_poll_event(handle, &failed_event[0], event_size, &written) == CNC_WEB_OK);
    Reader failed_reader(&failed_event[0], static_cast<uint32_t>(failed_event.size()));
    cnc::web::MessageHeader failed_header;
    uint32_t failed_tick = 99u;
    uint16_t failed_type = 0u;
    uint16_t failed_flags = 0u;
    uint64_t failed_player = 0u;
    int32_t failed_code = 0;
    int32_t failed_status = 0;
    assert(cnc::web::DecodeHeader(failed_reader, CNC_WEB_MESSAGE_EVENT_V1, failed_header));
    assert(failed_reader.U32(failed_tick) && failed_reader.U16(failed_type)
           && failed_reader.U16(failed_flags) && failed_reader.U64(failed_player)
           && failed_reader.I32(failed_code) && failed_reader.I32(failed_status));
    assert(failed_tick == 0u && failed_type == CNC_WEB_EVENT_RUNTIME_DIAGNOSTIC);
    assert((failed_flags & CNC_WEB_DIAGNOSTIC_ERROR) != 0u);
    assert(failed_code == CNC_WEB_DIAGNOSTIC_CONTENT_ERROR && failed_status == CNC_WEB_CONTENT_MISMATCH);
    assert(cnc_web_destroy(handle) == CNC_WEB_OK);
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);

    assert(cnc_web_set_campaign_transition(handle, 1234, 5u) == CNC_WEB_OK);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size() - 1u)) == CNC_WEB_INVALID_ARGUMENT);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_OK);
    assert(cnc_web_set_campaign_transition(handle, 0, 0u) == CNC_WEB_INVALID_STATE);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_INVALID_STATE);

    std::vector<uint8_t> commands = CommandMessage(1u);
    assert(cnc_web_submit_commands(handle, &commands[0], static_cast<uint32_t>(commands.size())) == CNC_WEB_OK);
    assert(cnc_web_submit_commands(handle, &commands[0], static_cast<uint32_t>(commands.size() - 1u))
           == CNC_WEB_INVALID_ARGUMENT);
    uint32_t advanced = 0u;
    assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK);
    assert(advanced == 1u);
    assert(cnc_web_submit_commands(handle, &commands[0], static_cast<uint32_t>(commands.size()))
           == CNC_WEB_INVALID_ARGUMENT);

    uint32_t snapshot_size = 0u;
    assert(cnc_web_snapshot_size(handle, &snapshot_size) == CNC_WEB_OK);
    assert(snapshot_size > CNC_WEB_SNAPSHOT_FIXED_SIZE_V1);
    std::vector<uint8_t> short_snapshot(snapshot_size - 1u);
    assert(cnc_web_write_snapshot(handle, &short_snapshot[0], snapshot_size - 1u, &written) == CNC_WEB_NEED_BUFFER);
    assert(written == snapshot_size);
    std::vector<uint8_t> snapshot(snapshot_size);
    assert(cnc_web_write_snapshot(handle, &snapshot[0], snapshot_size, &written) == CNC_WEB_OK);
    assert(written == snapshot_size);
    CheckHeader(snapshot, CNC_WEB_MESSAGE_SNAPSHOT_V1);

    uint64_t first_hash = 0u;
    uint64_t second_hash = 0u;
    assert(cnc_web_state_hash(handle, &first_hash) == CNC_WEB_OK);
    assert(cnc_web_state_hash(handle, &second_hash) == CNC_WEB_OK);
    assert(first_hash != 0u && first_hash == second_hash);

    assert(cnc_web_event_size(handle, &event_size) == CNC_WEB_OK);
    assert(event_size > CNC_WEB_EVENT_FIXED_SIZE_V1);
    std::vector<uint8_t> event(event_size);
    assert(cnc_web_poll_event(handle, &event[0], event_size, &written) == CNC_WEB_OK);
    CheckHeader(event, CNC_WEB_MESSAGE_EVENT_V1);
    Reader start_event_reader(&event[0], static_cast<uint32_t>(event.size()));
    cnc::web::MessageHeader start_event_header;
    uint32_t start_event_tick = 99u;
    uint16_t start_event_type = 0u;
    uint16_t start_event_flags = 0u;
    uint64_t start_event_player = 0u;
    int32_t start_event_args[6] = {0};
    assert(cnc::web::DecodeHeader(start_event_reader, CNC_WEB_MESSAGE_EVENT_V1, start_event_header));
    assert(start_event_reader.U32(start_event_tick) && start_event_reader.U16(start_event_type)
           && start_event_reader.U16(start_event_flags) && start_event_reader.U64(start_event_player));
    for (uint32_t index = 0u; index < 6u; ++index) assert(start_event_reader.I32(start_event_args[index]));
    assert(start_event_tick == 0u && start_event_type == CNC_WEB_EVENT_DEBUG);
    assert(start_event_args[0] == 1 && start_event_args[1] == 1234 && start_event_args[2] == 5
           && start_event_args[3] == 0);
    assert(cnc_web_event_size(handle, &event_size) == CNC_WEB_OK && event_size == 0u);

    uint32_t save_size = 0u;
    assert(cnc_web_save_size(handle, &save_size) == CNC_WEB_OK);
    assert(save_size > CNC_WEB_SAVE_FIXED_SIZE_V1);
    std::vector<uint8_t> save(save_size);
    assert(cnc_web_write_save(handle, &save[0], save_size, &written) == CNC_WEB_OK);

    /* Engine-private deterministic state contributes to state_hash even when
     * every browser-visible legacy snapshot field is identical. */
    uint64_t saved_state_hash = 0u;
    uint64_t altered_hidden_state_hash = 0u;
    assert(cnc_web_state_hash(handle, &saved_state_hash) == CNC_WEB_OK);
    std::vector<uint8_t> altered_hidden_state = save;
    assert(altered_hidden_state.size() > CNC_WEB_SAVE_FIXED_SIZE_V1 + 24u);
    altered_hidden_state[CNC_WEB_SAVE_FIXED_SIZE_V1 + 24u] ^= 1u;
    PatchU64(altered_hidden_state,
             32u,
             cnc::web::HashBytes(&altered_hidden_state[CNC_WEB_SAVE_FIXED_SIZE_V1],
                                 static_cast<uint32_t>(altered_hidden_state.size() - CNC_WEB_SAVE_FIXED_SIZE_V1)));
    assert(cnc_web_load_save(handle,
                             &altered_hidden_state[0],
                             static_cast<uint32_t>(altered_hidden_state.size())) == CNC_WEB_OK);
    assert(cnc_web_state_hash(handle, &altered_hidden_state_hash) == CNC_WEB_OK);
    assert(altered_hidden_state_hash != saved_state_hash);
    assert(cnc_web_load_save(handle, &save[0], save_size) == CNC_WEB_OK);

    /* TDWS v2 also restores the campaign-significant SabotagedType global
     * that the opaque legacy save omits. */
    std::vector<uint8_t> altered_sabotage_state = save;
    PatchU32(altered_sabotage_state, CNC_WEB_SAVE_FIXED_SIZE_V1 + 32u, 11u);
    PatchU64(altered_sabotage_state,
             32u,
             cnc::web::HashBytes(&altered_sabotage_state[CNC_WEB_SAVE_FIXED_SIZE_V1],
                                 static_cast<uint32_t>(altered_sabotage_state.size()
                                                       - CNC_WEB_SAVE_FIXED_SIZE_V1)));
    assert(cnc_web_load_save(handle,
                             &altered_sabotage_state[0],
                             static_cast<uint32_t>(altered_sabotage_state.size())) == CNC_WEB_OK);
    uint64_t altered_sabotage_hash = 0u;
    assert(cnc_web_state_hash(handle, &altered_sabotage_hash) == CNC_WEB_OK);
    assert(altered_sabotage_hash != saved_state_hash);
    assert(cnc_web_load_save(handle, &save[0], save_size) == CNC_WEB_OK);

    std::vector<uint8_t> backend_rejected = save;
    backend_rejected[CNC_WEB_SAVE_FIXED_SIZE_V1] ^= 1u;
    PatchU64(backend_rejected,
             32u,
             cnc::web::HashBytes(&backend_rejected[CNC_WEB_SAVE_FIXED_SIZE_V1],
                                 static_cast<uint32_t>(backend_rejected.size() - CNC_WEB_SAVE_FIXED_SIZE_V1)));
    uint64_t before_rejected_load = 0u;
    uint64_t after_rejected_load = 0u;
    assert(cnc_web_state_hash(handle, &before_rejected_load) == CNC_WEB_OK);
    assert(cnc_web_load_save(handle, &backend_rejected[0], static_cast<uint32_t>(backend_rejected.size()))
           == CNC_WEB_IO_ERROR);
    assert(cnc_web_state_hash(handle, &after_rejected_load) == CNC_WEB_OK);
    assert(after_rejected_load == before_rejected_load);

    assert(cnc_web_load_save(handle, &save[0], save_size) == CNC_WEB_OK);
    save[44] ^= 1u;
    assert(cnc_web_load_save(handle, &save[0], save_size) == CNC_WEB_CONTENT_MISMATCH);
    save[44] ^= 1u;
    save.back() ^= 1u;
    assert(cnc_web_load_save(handle, &save[0], save_size) == CNC_WEB_CONTENT_MISMATCH);

    assert(cnc_web_destroy(handle) == CNC_WEB_OK);
    assert(cnc_web_destroy(handle) == CNC_WEB_INVALID_ARGUMENT);

    /* A semantically invalid tail must reject the entire batch before any command is applied. */
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_OK);
    std::vector<uint8_t> mixed = MixedInvalidCommandMessage(1u);
    assert(cnc_web_submit_commands(handle, &mixed[0], static_cast<uint32_t>(mixed.size()))
           == CNC_WEB_INVALID_ARGUMENT);
    assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK && advanced == 1u);
    uint64_t rejected_batch_hash = 0u;
    assert(cnc_web_state_hash(handle, &rejected_batch_hash) == CNC_WEB_OK);
    assert(cnc_web_destroy(handle) == CNC_WEB_OK);

    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_OK);
    assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK && advanced == 1u);
    uint64_t clean_hash = 0u;
    assert(cnc_web_state_hash(handle, &clean_hash) == CNC_WEB_OK);
    assert(clean_hash == rejected_batch_hash);
    assert(cnc_web_destroy(handle) == CNC_WEB_OK);

    /* The game-over tick is final: batched advance stops and later ticks are stable no-ops. */
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_OK);
    assert(cnc_web_save_size(handle, &save_size) == CNC_WEB_OK);
    std::vector<uint8_t> pre_terminal_save(save_size);
    assert(cnc_web_write_save(handle, &pre_terminal_save[0], save_size, &written) == CNC_WEB_OK);
    assert(cnc_web_event_size(handle, &event_size) == CNC_WEB_OK && event_size > 0u);
    std::vector<uint8_t> terminal_event(event_size);
    assert(cnc_web_poll_event(handle, &terminal_event[0], event_size, &written) == CNC_WEB_OK); // mock-started
    std::vector<uint8_t> terminal = TerminalCommandMessage(1u);
    assert(cnc_web_submit_commands(handle, &terminal[0], static_cast<uint32_t>(terminal.size())) == CNC_WEB_OK);
    assert(cnc_web_advance(handle, 10u, &advanced) == CNC_WEB_OK && advanced == 1u);
    assert(cnc_web_advance(handle, 10u, &advanced) == CNC_WEB_OK && advanced == 0u);
    assert(cnc_web_event_size(handle, &event_size) == CNC_WEB_OK && event_size >= CNC_WEB_EVENT_FIXED_SIZE_V1);
    terminal_event.resize(event_size);
    assert(cnc_web_poll_event(handle, &terminal_event[0], event_size, &written) == CNC_WEB_OK);
    Reader outcome_reader(&terminal_event[0], static_cast<uint32_t>(terminal_event.size()));
    cnc::web::MessageHeader outcome_header;
    uint32_t outcome_tick = 0u;
    uint16_t outcome_type = 0u;
    uint16_t outcome_flags = 0u;
    uint64_t outcome_player = 0u;
    int32_t outcome_args[6] = {0};
    uint32_t outcome_text1_size = 0u;
    uint32_t outcome_text2_size = 0u;
    std::string outcome_text1;
    std::string outcome_text2;
    assert(cnc::web::DecodeHeader(outcome_reader, CNC_WEB_MESSAGE_EVENT_V1, outcome_header));
    assert(outcome_reader.U32(outcome_tick) && outcome_reader.U16(outcome_type)
           && outcome_reader.U16(outcome_flags) && outcome_reader.U64(outcome_player));
    for (uint32_t index = 0u; index < 6u; ++index) assert(outcome_reader.I32(outcome_args[index]));
    assert(outcome_reader.U32(outcome_text1_size) && outcome_reader.U32(outcome_text2_size)
           && outcome_reader.String(outcome_text1_size, outcome_text1)
           && outcome_reader.String(outcome_text2_size, outcome_text2) && outcome_reader.Remaining() == 0u);
    assert(outcome_tick == 1u && outcome_type == CNC_WEB_EVENT_CAMPAIGN_OUTCOME && outcome_flags == 6u);
    assert(outcome_args[0] == 0 && outcome_args[1] == 7 && outcome_args[2] == -1
           && outcome_args[4] == 1 && outcome_args[5] == 0);
    assert(outcome_text1 == "SCG01EA" && outcome_text2.empty());
    assert(cnc_web_event_size(handle, &event_size) == CNC_WEB_OK && event_size >= CNC_WEB_EVENT_FIXED_SIZE_V1);
    terminal_event.resize(event_size);
    assert(cnc_web_poll_event(handle, &terminal_event[0], event_size, &written) == CNC_WEB_OK);
    Reader game_over_reader(&terminal_event[0], static_cast<uint32_t>(terminal_event.size()));
    cnc::web::MessageHeader game_over_header;
    uint32_t game_over_tick = 0u;
    uint16_t game_over_type = 0u;
    assert(cnc::web::DecodeHeader(game_over_reader, CNC_WEB_MESSAGE_EVENT_V1, game_over_header));
    assert(game_over_reader.U32(game_over_tick) && game_over_reader.U16(game_over_type));
    assert(game_over_tick == 1u && game_over_type == CNC_WEB_EVENT_GAME_OVER);
    commands = CommandMessage(2u);
    assert(cnc_web_submit_commands(handle, &commands[0], static_cast<uint32_t>(commands.size()))
           == CNC_WEB_INVALID_STATE);
    assert(cnc_web_snapshot_size(handle, &snapshot_size) == CNC_WEB_OK);
    std::vector<uint8_t> terminal_snapshot(snapshot_size);
    assert(cnc_web_write_snapshot(handle, &terminal_snapshot[0], snapshot_size, &written) == CNC_WEB_OK);
    Reader terminal_reader(&terminal_snapshot[0], static_cast<uint32_t>(terminal_snapshot.size()));
    cnc::web::MessageHeader terminal_header;
    uint32_t terminal_tick = 0u;
    uint32_t terminal_base_tick = 0u;
    uint64_t terminal_hash = 0u;
    uint32_t terminal_sections = 0u;
    uint32_t terminal_flags = 0u;
    assert(cnc::web::DecodeHeader(terminal_reader, CNC_WEB_MESSAGE_SNAPSHOT_V1, terminal_header));
    assert(terminal_reader.U32(terminal_tick) && terminal_reader.U32(terminal_base_tick)
           && terminal_reader.U64(terminal_hash) && terminal_reader.U32(terminal_sections)
           && terminal_reader.U32(terminal_flags));
    assert((terminal_flags & CNC_WEB_SNAPSHOT_FLAG_TERMINAL) != 0u);
    assert(cnc_web_load_save(handle, &pre_terminal_save[0], static_cast<uint32_t>(pre_terminal_save.size()))
           == CNC_WEB_OK);
    assert(cnc_web_event_size(handle, &event_size) == CNC_WEB_OK && event_size == 0u);
    assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK && advanced == 1u);
    assert(cnc_web_destroy(handle) == CNC_WEB_OK);

    /* A tick-zero save must restore the campaign-only first-update branch. */
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_OK);
    assert(cnc_web_save_size(handle, &save_size) == CNC_WEB_OK);
    std::vector<uint8_t> tick_zero_save(save_size);
    assert(cnc_web_write_save(handle, &tick_zero_save[0], save_size, &written) == CNC_WEB_OK);
    assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK && advanced == 1u);
    uint64_t first_tick_hash = 0u;
    assert(cnc_web_state_hash(handle, &first_tick_hash) == CNC_WEB_OK);
    assert(cnc_web_load_save(handle, &tick_zero_save[0], static_cast<uint32_t>(tick_zero_save.size()))
           == CNC_WEB_OK);
    assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK && advanced == 1u);
    uint64_t replayed_first_tick_hash = 0u;
    assert(cnc_web_state_hash(handle, &replayed_first_tick_hash) == CNC_WEB_OK);
    assert(replayed_first_tick_hash == first_tick_hash);
    assert(cnc_web_destroy(handle) == CNC_WEB_OK);

    /* The deterministic RNG hidden from the legacy payload must branch/replay exactly. */
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_OK);
    assert(cnc_web_advance(handle, 3u, &advanced) == CNC_WEB_OK && advanced == 3u);
    assert(cnc_web_save_size(handle, &save_size) == CNC_WEB_OK);
    std::vector<uint8_t> branch_save(save_size);
    assert(cnc_web_write_save(handle, &branch_save[0], save_size, &written) == CNC_WEB_OK);
    std::vector<uint64_t> branch_hashes;
    for (uint32_t index = 0u; index < 5u; ++index) {
        assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK && advanced == 1u);
        uint64_t hash = 0u;
        assert(cnc_web_state_hash(handle, &hash) == CNC_WEB_OK);
        branch_hashes.push_back(hash);
    }
    assert(cnc_web_load_save(handle, &branch_save[0], static_cast<uint32_t>(branch_save.size())) == CNC_WEB_OK);
    for (uint32_t index = 0u; index < branch_hashes.size(); ++index) {
        assert(cnc_web_advance(handle, 1u, &advanced) == CNC_WEB_OK && advanced == 1u);
        uint64_t hash = 0u;
        assert(cnc_web_state_hash(handle, &hash) == CNC_WEB_OK);
        assert(hash == branch_hashes[index]);
    }
    assert(cnc_web_destroy(handle) == CNC_WEB_OK);

    /* base_tick names the preceding materialized snapshot, not necessarily
     * tick - 1. A successful load starts a fresh self-based bootstrap chain. */
    assert(cnc_web_create(CNC_WEB_ABI_VERSION, &handle) == CNC_WEB_OK);
    assert(cnc_web_start(handle, &start[0], static_cast<uint32_t>(start.size())) == CNC_WEB_OK);
    SnapshotClock clock = ReadSnapshotClock(CopySnapshot(handle));
    assert(clock.tick == 0u && clock.base_tick == 0u);
    assert(cnc_web_advance(handle, 3u, &advanced) == CNC_WEB_OK && advanced == 3u);
    clock = ReadSnapshotClock(CopySnapshot(handle));
    assert(clock.tick == 3u && clock.base_tick == 0u);

    assert(cnc_web_save_size(handle, &save_size) == CNC_WEB_OK);
    std::vector<uint8_t> base_tick_save(save_size);
    assert(cnc_web_write_save(handle, &base_tick_save[0], save_size, &written) == CNC_WEB_OK);
    CheckHeader(base_tick_save, CNC_WEB_MESSAGE_SAVE_V1);
    assert(cnc_web_advance(handle, 2u, &advanced) == CNC_WEB_OK && advanced == 2u);
    clock = ReadSnapshotClock(CopySnapshot(handle));
    assert(clock.tick == 5u && clock.base_tick == 3u);

    assert(cnc_web_load_save(handle, &base_tick_save[0], static_cast<uint32_t>(base_tick_save.size()))
           == CNC_WEB_OK);
    clock = ReadSnapshotClock(CopySnapshot(handle));
    assert(clock.tick == 3u && clock.base_tick == 3u);
    assert(cnc_web_destroy(handle) == CNC_WEB_OK);
    return 0;
}
