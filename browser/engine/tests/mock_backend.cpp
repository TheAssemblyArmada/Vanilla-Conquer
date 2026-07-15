/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "backend.h"
#include "td_save_state.h"

#include <string.h>

namespace cnc {
namespace web {

namespace {

class MockBackend : public Backend
{
public:
    MockBackend()
        : sink_(NULL)
        , sink_context_(NULL)
        , seed_(0u)
        , tick_(0u)
        , accumulator_(0u)
        , random_seed_(1u)
        , first_update_(false)
        , carry_over_money_(0)
        , nuke_pieces_(7u)
        , sabotaged_structure_(-1)
        , terminal_on_next_advance_(false)
    {
    }

    virtual cnc_web_status_t Initialize(BackendEventSink sink, void* sink_context)
    {
        sink_ = sink;
        sink_context_ = sink_context;
        return CNC_WEB_OK;
    }

    virtual void Shutdown() {}

    virtual cnc_web_status_t Start(const StartConfig& config)
    {
        if (config.content_directory == "/missing") {
            /* Deliberately fail without emitting an adapter event so the
             * public ABI's structured fallback diagnostic is exercised. */
            return CNC_WEB_CONTENT_MISMATCH;
        }
        seed_ = config.seed;
        tick_ = 0u;
        accumulator_ = config.seed;
        random_seed_ = config.has_campaign_transition ? config.seed : (config.seed == 0u ? 1u : config.seed);
        first_update_ = config.game_mode == CNC_WEB_GAME_CAMPAIGN;
        carry_over_money_ = config.has_campaign_transition ? config.carry_over_money : 0;
        nuke_pieces_ = config.has_campaign_transition ? config.nuke_pieces : 7u;
        sabotaged_structure_ = config.sabotaged_structure;
        BackendEvent event;
        event.type = CNC_WEB_EVENT_DEBUG;
        event.args[0] = config.has_campaign_transition ? 1 : 0;
        event.args[1] = carry_over_money_;
        event.args[2] = static_cast<int32_t>(nuke_pieces_);
        event.args[3] = static_cast<int32_t>(config.seed);
        event.text1 = "mock-started";
        sink_(sink_context_, event);
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t ValidateCommands(uint64_t player_id, const std::vector<Command>& commands)
    {
        (void)player_id;
        for (std::vector<Command>::const_iterator command = commands.begin(); command != commands.end(); ++command) {
            if (command->type < CNC_WEB_COMMAND_INPUT || command->type > CNC_WEB_COMMAND_SELECT_OBJECT
                || (command->type == CNC_WEB_COMMAND_INPUT
                    && (command->flags & ~static_cast<uint16_t>(CNC_WEB_MODIFIER_CTRL | CNC_WEB_MODIFIER_ALT
                                                                | CNC_WEB_MODIFIER_SHIFT)) != 0u)) {
                return CNC_WEB_INVALID_ARGUMENT;
            }
        }
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t ApplyCommands(uint64_t player_id, const std::vector<Command>& commands)
    {
        accumulator_ ^= player_id;
        for (std::vector<Command>::const_iterator command = commands.begin(); command != commands.end(); ++command) {
            if (command->type == CNC_WEB_COMMAND_GAME && command->args[0] == 1) {
                terminal_on_next_advance_ = true;
            }
            accumulator_ = accumulator_ * UINT64_C(1099511628211) + command->type + command->flags;
            for (uint32_t argument = 0u; argument < 7u; ++argument) {
                accumulator_ ^= static_cast<uint32_t>(command->args[argument]);
            }
        }
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t Advance(uint64_t player_id, bool& out_running)
    {
        ++tick_;
        if (first_update_) {
            first_update_ = false;
            accumulator_ += player_id + tick_;
        } else {
            random_seed_ ^= random_seed_ << 13u;
            random_seed_ ^= random_seed_ >> 17u;
            random_seed_ ^= random_seed_ << 5u;
            accumulator_ += player_id + tick_ + random_seed_;
        }
        out_running = !terminal_on_next_advance_;
        if (!out_running) {
            BackendEvent event;
            event.type = CNC_WEB_EVENT_CAMPAIGN_OUTCOME;
            event.flags = 6u;
            event.player_id = player_id;
            event.args[0] = carry_over_money_;
            event.args[1] = static_cast<int32_t>(nuke_pieces_);
            event.args[2] = sabotaged_structure_;
            event.args[3] = static_cast<int32_t>(random_seed_);
            event.args[4] = 1;
            event.args[5] = 0;
            event.text1 = "SCG01EA";
            sink_(sink_context_, event);
            event.type = CNC_WEB_EVENT_GAME_OVER;
            event.flags = 6u;
            event.text1.clear();
            sink_(sink_context_, event);
            event.type = CNC_WEB_EVENT_SOUND;
            event.flags = 0u;
            sink_(sink_context_, event);
        }
        terminal_on_next_advance_ = false;
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t Snapshot(uint64_t player_id, std::vector<SnapshotSection>& sections)
    {
        sections.clear();
        SnapshotSection section;
        section.kind = CNC_WEB_SECTION_PLAYER;
        section.flags = 0u;
        section.count = 1u;
        Writer writer;
        if (!writer.U32(seed_) || !writer.U32(tick_) || !writer.U64(player_id) || !writer.U64(accumulator_)) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        section.payload.swap(writer.Data());
        sections.push_back(section);
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t DeterministicState(std::vector<uint8_t>& bytes)
    {
        Writer writer;
        if (!writer.U32(random_seed_) || !writer.U8(first_update_ ? 1u : 0u)
            || !writer.I32(carry_over_money_) || !writer.U32(nuke_pieces_)
            || !writer.I32(sabotaged_structure_)) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        bytes.swap(writer.Data());
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t Save(std::vector<uint8_t>& bytes)
    {
        Writer writer;
        if (!writer.U32(CNC_WEB_MAGIC_ENGINE_SAVE) || !writer.U32(seed_) || !writer.U32(tick_)
            || !writer.U64(accumulator_) || !writer.I32(carry_over_money_) || !writer.U32(nuke_pieces_)) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        TdDeterministicSaveState state;
        state.random_seed = random_seed_;
        state.first_update = first_update_;
        state.sabotaged_structure = sabotaged_structure_;
        return EncodeTdSavePayload(writer.Data(), state, bytes) ? CNC_WEB_OK : CNC_WEB_OUT_OF_MEMORY;
    }

    virtual cnc_web_status_t Load(const uint8_t* bytes,
                                  uint32_t size,
                                  uint32_t game_mode,
                                  uint32_t tick,
                                  uint64_t player_id)
    {
        (void)game_mode;
        (void)player_id;
        TdDeterministicSaveState state;
        const uint8_t* legacy_save = NULL;
        uint32_t legacy_save_size = 0u;
        if (DecodeTdSavePayload(bytes, size, state, legacy_save, legacy_save_size) != TD_SAVE_PAYLOAD_OK) {
            return CNC_WEB_IO_ERROR;
        }
        Reader reader(legacy_save, legacy_save_size);
        uint32_t magic = 0u;
        if (!reader.U32(magic) || magic != CNC_WEB_MAGIC_ENGINE_SAVE || !reader.U32(seed_) || !reader.U32(tick_)
            || !reader.U64(accumulator_) || !reader.I32(carry_over_money_) || !reader.U32(nuke_pieces_)
            || reader.Remaining() != 0u || nuke_pieces_ > 7u) {
            return CNC_WEB_IO_ERROR;
        }
        if (tick_ != tick) {
            return CNC_WEB_IO_ERROR;
        }
        random_seed_ = state.random_seed;
        first_update_ = state.first_update;
        sabotaged_structure_ = state.sabotaged_structure;
        terminal_on_next_advance_ = false;
        return CNC_WEB_OK;
    }

private:
    BackendEventSink sink_;
    void* sink_context_;
    uint32_t seed_;
    uint32_t tick_;
    uint64_t accumulator_;
    uint32_t random_seed_;
    bool first_update_;
    int32_t carry_over_money_;
    uint32_t nuke_pieces_;
    int32_t sabotaged_structure_;
    bool terminal_on_next_advance_;
};

} // namespace

Backend* CreateBackend()
{
    return new (std::nothrow) MockBackend();
}

void DestroyBackend(Backend* backend)
{
    delete backend;
}

} // namespace web
} // namespace cnc
