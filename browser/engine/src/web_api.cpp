/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cnc_web.h"

#include "backend.h"
#include "protocol.h"

#include <string.h>

#include <algorithm>
#include <deque>
#include <new>
#include <vector>

namespace cnc {
namespace web {

namespace {

const uint32_t kMaximumAdvanceTicks = 1024u;
const uint32_t kMaximumQueuedCommands = 65536u;
const uint32_t kMaximumFutureTicks = 15u * 60u * 4u;
const uint32_t kMaximumEvents = 2048u;
const uint32_t kEngineIdTiberianDawn = 1u;

struct ScheduledBatch
{
    uint32_t tick;
    uint64_t player_id;
    std::vector<Command> commands;
};

struct Instance
{
    Instance()
        : handle(CNC_WEB_INVALID_HANDLE)
        , backend(NULL)
        , started(false)
        , tick(0u)
        , seed(0u)
        , player_id(0u)
        , game_mode(CNC_WEB_GAME_CAMPAIGN)
        , content_id_hash(0u)
        , has_campaign_transition(false)
        , carry_over_money(0)
        , nuke_pieces(0u)
        , terminal(false)
        , event_tick(0u)
        , queued_command_count(0u)
        , cached_snapshot_tick(UINT32_MAX)
        , has_snapshot_base(false)
        , last_snapshot_tick(0u)
        , state_hash(0u)
    {
    }

    cnc_web_handle_t handle;
    Backend* backend;
    bool started;
    uint32_t tick;
    uint32_t seed;
    uint64_t player_id;
    uint32_t game_mode;
    uint64_t content_id_hash;
    std::string content_directory;
    bool has_campaign_transition;
    int32_t carry_over_money;
    uint32_t nuke_pieces;
    bool terminal;
    uint32_t event_tick;
    uint32_t queued_command_count;
    std::vector<ScheduledBatch> scheduled;
    std::deque<std::vector<uint8_t> > events;
    std::vector<uint8_t> cached_snapshot;
    uint32_t cached_snapshot_tick;
    bool has_snapshot_base;
    uint32_t last_snapshot_tick;
    std::vector<uint8_t> cached_save;
    uint64_t state_hash;
};

Instance* g_instance = NULL;
uint32_t g_next_handle = 1u;

Instance* Find(cnc_web_handle_t handle)
{
    return g_instance != NULL && handle != CNC_WEB_INVALID_HANDLE && g_instance->handle == handle ? g_instance : NULL;
}

bool AddFits(uint32_t left, uint32_t right)
{
    return left <= UINT32_MAX - right;
}

void InvalidateCachedState(Instance& instance)
{
    instance.cached_snapshot.clear();
    instance.cached_snapshot_tick = UINT32_MAX;
    instance.cached_save.clear();
    instance.state_hash = 0u;
}

void ResetSnapshotChain(Instance& instance)
{
    InvalidateCachedState(instance);
    instance.has_snapshot_base = false;
    instance.last_snapshot_tick = 0u;
}

bool EncodeEvent(uint32_t tick, const BackendEvent& event, std::vector<uint8_t>& output)
{
    if (event.text1.size() > UINT32_MAX || event.text2.size() > UINT32_MAX) {
        return false;
    }
    const uint32_t text1_size = static_cast<uint32_t>(event.text1.size());
    const uint32_t text2_size = static_cast<uint32_t>(event.text2.size());
    if (!AddFits(CNC_WEB_EVENT_FIXED_SIZE_V1, text1_size)
        || !AddFits(CNC_WEB_EVENT_FIXED_SIZE_V1 + text1_size, text2_size)) {
        return false;
    }
    const uint32_t byte_size = CNC_WEB_EVENT_FIXED_SIZE_V1 + text1_size + text2_size;
    Writer writer;
    if (!writer.Reserve(byte_size) || !WriteMessageHeader(writer, CNC_WEB_MESSAGE_EVENT_V1, byte_size, 1u)
        || !writer.U32(tick) || !writer.U16(event.type) || !writer.U16(event.flags) || !writer.U64(event.player_id)) {
        return false;
    }
    for (uint32_t argument = 0u; argument < 6u; ++argument) {
        if (!writer.I32(event.args[argument])) {
            return false;
        }
    }
    if (!writer.U32(text1_size) || !writer.U32(text2_size) || !writer.Bytes(event.text1.data(), text1_size)
        || !writer.Bytes(event.text2.data(), text2_size) || writer.Size() != byte_size) {
        return false;
    }
    output.swap(writer.Data());
    return true;
}

void OnBackendEvent(void* context, const BackendEvent& event)
{
    Instance* instance = static_cast<Instance*>(context);
    if (instance == NULL) {
        return;
    }
    std::vector<uint8_t> encoded;
    if (!EncodeEvent(instance->event_tick, event, encoded)) {
        return;
    }
    if (instance->events.size() >= kMaximumEvents) {
        /* Preserve bounded memory. Debug messages are expendable; otherwise drop oldest. */
        if (event.type == CNC_WEB_EVENT_DEBUG) {
            return;
        }
        instance->events.pop_front();
    }
    instance->events.push_back(encoded);
}

cnc_web_status_t BuildSnapshot(Instance& instance)
{
    if (!instance.started) {
        return CNC_WEB_INVALID_STATE;
    }
    if (instance.cached_snapshot_tick == instance.tick && !instance.cached_snapshot.empty()) {
        return CNC_WEB_OK;
    }

    std::vector<SnapshotSection> sections;
    cnc_web_status_t status = instance.backend->Snapshot(instance.player_id, sections);
    if (status != CNC_WEB_OK) {
        return status;
    }
    if (sections.size() > UINT32_MAX) {
        return CNC_WEB_OUT_OF_MEMORY;
    }
    const uint32_t base_tick = instance.has_snapshot_base ? instance.last_snapshot_tick : instance.tick;

    uint32_t total_size = CNC_WEB_SNAPSHOT_FIXED_SIZE_V1;
    Writer canonical_tick;
    if (!canonical_tick.U32(instance.tick)) {
        return CNC_WEB_OUT_OF_MEMORY;
    }
    uint64_t hash = HashBytes(&canonical_tick.Data()[0], canonical_tick.Size());
    for (std::vector<SnapshotSection>::const_iterator section = sections.begin(); section != sections.end(); ++section) {
        if (section->payload.size() > UINT32_MAX
            || !AddFits(CNC_WEB_SECTION_HEADER_SIZE_V1, static_cast<uint32_t>(section->payload.size()))
            || !AddFits(total_size,
                        CNC_WEB_SECTION_HEADER_SIZE_V1 + static_cast<uint32_t>(section->payload.size()))) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        total_size += CNC_WEB_SECTION_HEADER_SIZE_V1 + static_cast<uint32_t>(section->payload.size());
        hash = HashSnapshotSection(hash, *section);
        if (hash == 0u) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
    }

    std::vector<uint8_t> deterministic_state;
    status = instance.backend->DeterministicState(deterministic_state);
    if (status != CNC_WEB_OK) {
        return status;
    }
    SnapshotSection deterministic_section;
    deterministic_section.kind = UINT16_MAX;
    deterministic_section.count = 1u;
    deterministic_section.payload.swap(deterministic_state);
    hash = HashSnapshotSection(hash, deterministic_section);
    if (hash == 0u) {
        return CNC_WEB_OUT_OF_MEMORY;
    }

    Writer writer;
    if (!writer.Reserve(total_size)
        || !WriteMessageHeader(writer,
                               CNC_WEB_MESSAGE_SNAPSHOT_V1,
                               total_size,
                               static_cast<uint32_t>(sections.size()))
        || !writer.U32(instance.tick) || !writer.U32(base_tick) || !writer.U64(hash)
        || !writer.U32(static_cast<uint32_t>(sections.size()))
        || !writer.U32(instance.terminal ? CNC_WEB_SNAPSHOT_FLAG_TERMINAL : 0u)) {
        return CNC_WEB_OUT_OF_MEMORY;
    }
    for (std::vector<SnapshotSection>::const_iterator section = sections.begin(); section != sections.end(); ++section) {
        const uint32_t payload_size = static_cast<uint32_t>(section->payload.size());
        if (!writer.U16(section->kind) || !writer.U16(section->flags) || !writer.U32(payload_size)
            || !writer.U32(section->count) || !writer.U32(0u)
            || !writer.Bytes(section->payload.empty() ? NULL : &section->payload[0], payload_size)) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
    }
    if (writer.Size() != total_size) {
        return CNC_WEB_FATAL;
    }
    instance.cached_snapshot.swap(writer.Data());
    instance.cached_snapshot_tick = instance.tick;
    instance.has_snapshot_base = true;
    instance.last_snapshot_tick = instance.tick;
    instance.state_hash = hash;
    return CNC_WEB_OK;
}

cnc_web_status_t CopyCached(std::vector<uint8_t>& source,
                            uint8_t* buffer,
                            uint32_t capacity,
                            uint32_t* out_written,
                            bool consume)
{
    if (out_written == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (source.size() > UINT32_MAX) {
        return CNC_WEB_OUT_OF_MEMORY;
    }
    const uint32_t required = static_cast<uint32_t>(source.size());
    *out_written = required;
    if (buffer == NULL || capacity < required) {
        return CNC_WEB_NEED_BUFFER;
    }
    if (required != 0u) {
        memcpy(buffer, &source[0], required);
    }
    if (consume) {
        source.clear();
    }
    return CNC_WEB_OK;
}

cnc_web_status_t BuildSave(Instance& instance)
{
    if (!instance.started) {
        return CNC_WEB_INVALID_STATE;
    }
    if (!instance.cached_save.empty()) {
        return CNC_WEB_OK;
    }

    std::vector<uint8_t> engine_save;
    cnc_web_status_t status = instance.backend->Save(engine_save);
    if (status != CNC_WEB_OK) {
        return status;
    }
    if (engine_save.size() > UINT32_MAX
        || !AddFits(CNC_WEB_SAVE_FIXED_SIZE_V1, static_cast<uint32_t>(engine_save.size()))) {
        return CNC_WEB_OUT_OF_MEMORY;
    }
    const uint32_t payload_size = static_cast<uint32_t>(engine_save.size());
    const uint32_t total_size = CNC_WEB_SAVE_FIXED_SIZE_V1 + payload_size;
    const uint64_t payload_hash = HashBytes(engine_save.empty() ? NULL : &engine_save[0], payload_size);
    const uint64_t content_hash = instance.content_id_hash;
    Writer writer;
    if (!writer.Reserve(total_size)
        || !WriteMessageHeader(writer, CNC_WEB_MESSAGE_SAVE_V1, total_size, 1u) || !writer.U32(instance.tick)
        || !writer.U32(instance.seed) || !writer.U64(instance.player_id) || !writer.U64(payload_hash)
        || !writer.U32(kEngineIdTiberianDawn) || !writer.U64(content_hash) || !writer.U32(payload_size)
        || !writer.U32(instance.game_mode)
        || !writer.Bytes(engine_save.empty() ? NULL : &engine_save[0], payload_size)) {
        return CNC_WEB_OUT_OF_MEMORY;
    }
    instance.cached_save.swap(writer.Data());
    return CNC_WEB_OK;
}

} // namespace

} // namespace web
} // namespace cnc

extern "C" {

uint32_t cnc_web_abi_version(void)
{
    return CNC_WEB_ABI_VERSION;
}

cnc_web_status_t cnc_web_create(uint32_t requested_abi_version, cnc_web_handle_t* out_handle)
{
    using namespace cnc::web;
    if (out_handle == NULL || requested_abi_version != CNC_WEB_ABI_VERSION) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    *out_handle = CNC_WEB_INVALID_HANDLE;
    if (g_instance != NULL) {
        /* The original TD core stores simulation state in globals. */
        return CNC_WEB_INVALID_STATE;
    }

    Instance* instance = new (std::nothrow) Instance();
    if (instance == NULL) {
        return CNC_WEB_OUT_OF_MEMORY;
    }
    instance->backend = CreateBackend();
    if (instance->backend == NULL) {
        delete instance;
        return CNC_WEB_OUT_OF_MEMORY;
    }
    cnc_web_status_t status = instance->backend->Initialize(OnBackendEvent, instance);
    if (status != CNC_WEB_OK) {
        DestroyBackend(instance->backend);
        delete instance;
        return status;
    }
    if (g_next_handle == CNC_WEB_INVALID_HANDLE) {
        ++g_next_handle;
    }
    instance->handle = g_next_handle++;
    g_instance = instance;
    *out_handle = instance->handle;
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_destroy(cnc_web_handle_t handle)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    instance->backend->Shutdown();
    DestroyBackend(instance->backend);
    g_instance = NULL;
    delete instance;
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_set_campaign_transition(cnc_web_handle_t handle,
                                                  int32_t carry_over_money,
                                                  uint32_t nuke_pieces)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || nuke_pieces > 7u) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (instance->started || instance->has_campaign_transition) {
        return CNC_WEB_INVALID_STATE;
    }
    instance->has_campaign_transition = true;
    instance->carry_over_money = carry_over_money;
    instance->nuke_pieces = nuke_pieces;
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_start(cnc_web_handle_t handle, const uint8_t* start_config, uint32_t start_config_size)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || start_config == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (instance->started) {
        return CNC_WEB_INVALID_STATE;
    }
    StartConfig config;
    if (!DecodeStartConfig(start_config, start_config_size, config)) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (instance->has_campaign_transition && config.game_mode != CNC_WEB_GAME_CAMPAIGN) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    config.has_campaign_transition = instance->has_campaign_transition;
    config.carry_over_money = instance->carry_over_money;
    config.nuke_pieces = instance->nuke_pieces;
    /* A campaign boundary carries the live scenario RNG bit-for-bit. Zero is
     * a valid live state even though a standalone zero means "choose a seed"
     * to the legacy startup path. */
    if (!config.has_campaign_transition) {
        config.seed = CanonicalSeed(config.seed);
    }
    const size_t events_before_start = instance->events.size();
    cnc_web_status_t status = instance->backend->Start(config);
    if (status != CNC_WEB_OK) {
        /* A backend may fail before its richer adapter diagnostic reaches the
         * host. Preserve the ABI guarantee that every decoded StartV1 failure
         * has at least one pollable, structured explanation. */
        if (instance->events.size() == events_before_start) {
            BackendEvent event;
            event.type = CNC_WEB_EVENT_RUNTIME_DIAGNOSTIC;
            event.flags = CNC_WEB_DIAGNOSTIC_ERROR;
            event.player_id = config.player_id;
            event.args[0] = status == CNC_WEB_CONTENT_MISMATCH ? CNC_WEB_DIAGNOSTIC_CONTENT_ERROR
                                                               : CNC_WEB_DIAGNOSTIC_START_FAILED;
            event.args[1] = status;
            event.args[2] = config.scenario;
            event.args[3] = config.variation;
            event.args[4] = config.direction;
            event.args[5] = config.build_level;
            event.text1 = status == CNC_WEB_CONTENT_MISMATCH ? "runtime.content.missing"
                                                              : "runtime.start.failed";
            event.text2 = config.content_directory;
            OnBackendEvent(instance, event);
        }
        return status;
    }
    instance->started = true;
    instance->tick = 0u;
    instance->seed = config.seed;
    instance->player_id = config.player_id;
    instance->game_mode = config.game_mode;
    instance->content_id_hash = config.content_id_hash;
    instance->content_directory = config.content_directory;
    instance->terminal = false;
    instance->event_tick = 0u;
    ResetSnapshotChain(*instance);
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_submit_commands(cnc_web_handle_t handle,
                                          const uint8_t* command_batch,
                                          uint32_t command_batch_size)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || command_batch == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (!instance->started) {
        return CNC_WEB_INVALID_STATE;
    }
    if (instance->terminal) {
        return CNC_WEB_INVALID_STATE;
    }
    CommandBatch decoded;
    if (!DecodeCommandBatch(command_batch, command_batch_size, decoded) || decoded.target_tick <= instance->tick
        || decoded.target_tick - instance->tick > kMaximumFutureTicks
        || decoded.commands.size() > kMaximumQueuedCommands - instance->queued_command_count) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    ScheduledBatch scheduled;
    scheduled.tick = decoded.target_tick;
    scheduled.player_id = decoded.player_id == 0u ? instance->player_id : decoded.player_id;
    cnc_web_status_t status = instance->backend->ValidateCommands(scheduled.player_id, decoded.commands);
    if (status != CNC_WEB_OK) {
        return status;
    }
    scheduled.commands.swap(decoded.commands);
    instance->queued_command_count += static_cast<uint32_t>(scheduled.commands.size());
    try {
        std::vector<ScheduledBatch>::iterator position = instance->scheduled.end();
        while (position != instance->scheduled.begin() && (position - 1)->tick > scheduled.tick) {
            --position;
        }
        instance->scheduled.insert(position, scheduled);
    } catch (const std::bad_alloc&) {
        instance->queued_command_count -= static_cast<uint32_t>(scheduled.commands.size());
        return CNC_WEB_OUT_OF_MEMORY;
    }
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_advance(cnc_web_handle_t handle, uint32_t tick_count, uint32_t* out_advanced)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (out_advanced != NULL) {
        *out_advanced = 0u;
    }
    if (instance == NULL || tick_count == 0u || tick_count > kMaximumAdvanceTicks) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (!instance->started) {
        return CNC_WEB_INVALID_STATE;
    }
    if (instance->terminal) {
        return CNC_WEB_OK;
    }

    uint32_t advanced = 0u;
    while (advanced < tick_count) {
        if (instance->tick == UINT32_MAX) {
            return CNC_WEB_FATAL;
        }
        const uint32_t next_tick = instance->tick + 1u;
        instance->event_tick = next_tick;
        for (std::vector<ScheduledBatch>::const_iterator batch = instance->scheduled.begin();
             batch != instance->scheduled.end() && batch->tick == next_tick;
             ++batch) {
            cnc_web_status_t status = instance->backend->ValidateCommands(batch->player_id, batch->commands);
            if (status != CNC_WEB_OK) {
                instance->event_tick = instance->tick;
                InvalidateCachedState(*instance);
                if (out_advanced != NULL) {
                    *out_advanced = advanced;
                }
                return status;
            }
        }
        /* Clear cached views before the first call that may mutate the core. */
        InvalidateCachedState(*instance);
        while (!instance->scheduled.empty() && instance->scheduled.front().tick == next_tick) {
            ScheduledBatch& batch = instance->scheduled.front();
            cnc_web_status_t status = instance->backend->ApplyCommands(batch.player_id, batch.commands);
            if (status != CNC_WEB_OK) {
                instance->event_tick = instance->tick;
                InvalidateCachedState(*instance);
                if (out_advanced != NULL) {
                    *out_advanced = advanced;
                }
                return status;
            }
            instance->queued_command_count -= static_cast<uint32_t>(batch.commands.size());
            instance->scheduled.erase(instance->scheduled.begin());
        }
        bool running = true;
        cnc_web_status_t status = instance->backend->Advance(instance->player_id, running);
        if (status != CNC_WEB_OK) {
            instance->event_tick = instance->tick;
            InvalidateCachedState(*instance);
            if (out_advanced != NULL) {
                *out_advanced = advanced;
            }
            return status;
        }
        instance->tick = next_tick;
        instance->event_tick = next_tick;
        ++advanced;
        if (!running) {
            instance->terminal = true;
            instance->scheduled.clear();
            instance->queued_command_count = 0u;
            break;
        }
    }
    if (out_advanced != NULL) {
        *out_advanced = advanced;
    }
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_snapshot_size(cnc_web_handle_t handle, uint32_t* out_size)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || out_size == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    cnc_web_status_t status = BuildSnapshot(*instance);
    if (status == CNC_WEB_OK) {
        *out_size = static_cast<uint32_t>(instance->cached_snapshot.size());
    }
    return status;
}

cnc_web_status_t cnc_web_write_snapshot(cnc_web_handle_t handle,
                                         uint8_t* buffer,
                                         uint32_t capacity,
                                         uint32_t* out_written)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    cnc_web_status_t status = BuildSnapshot(*instance);
    return status == CNC_WEB_OK ? CopyCached(instance->cached_snapshot, buffer, capacity, out_written, false) : status;
}

cnc_web_status_t cnc_web_event_size(cnc_web_handle_t handle, uint32_t* out_size)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || out_size == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (instance->events.empty()) {
        *out_size = 0u;
        return CNC_WEB_OK;
    }
    *out_size = static_cast<uint32_t>(instance->events.front().size());
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_poll_event(cnc_web_handle_t handle,
                                     uint8_t* buffer,
                                     uint32_t capacity,
                                     uint32_t* out_written)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || out_written == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (instance->events.empty()) {
        *out_written = 0u;
        return CNC_WEB_OK;
    }
    cnc_web_status_t status = CopyCached(instance->events.front(), buffer, capacity, out_written, false);
    if (status == CNC_WEB_OK) {
        instance->events.pop_front();
    }
    return status;
}

cnc_web_status_t cnc_web_save_size(cnc_web_handle_t handle, uint32_t* out_size)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || out_size == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    cnc_web_status_t status = BuildSave(*instance);
    if (status == CNC_WEB_OK) {
        *out_size = static_cast<uint32_t>(instance->cached_save.size());
    }
    return status;
}

cnc_web_status_t cnc_web_write_save(cnc_web_handle_t handle,
                                     uint8_t* buffer,
                                     uint32_t capacity,
                                     uint32_t* out_written)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    cnc_web_status_t status = BuildSave(*instance);
    return status == CNC_WEB_OK ? CopyCached(instance->cached_save, buffer, capacity, out_written, false) : status;
}

cnc_web_status_t cnc_web_load_save(cnc_web_handle_t handle, const uint8_t* save_data, uint32_t save_data_size)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || save_data == NULL || save_data_size < CNC_WEB_SAVE_FIXED_SIZE_V1) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    if (!instance->started) {
        return CNC_WEB_INVALID_STATE;
    }
    Reader reader(save_data, save_data_size);
    MessageHeader header;
    uint32_t tick = 0u;
    uint32_t seed = 0u;
    uint64_t player_id = 0u;
    uint64_t payload_hash = 0u;
    uint32_t engine_id = 0u;
    uint64_t content_hash = 0u;
    uint32_t payload_size = 0u;
    uint32_t game_mode = 0u;
    const uint8_t* payload = NULL;
    if (!DecodeHeader(reader, CNC_WEB_MESSAGE_SAVE_V1, header) || header.byte_size != save_data_size
        || header.count != 1u || !reader.U32(tick) || !reader.U32(seed) || !reader.U64(player_id)
        || !reader.U64(payload_hash) || !reader.U32(engine_id) || !reader.U64(content_hash)
        || !reader.U32(payload_size) || !reader.U32(game_mode) || payload_size != reader.Remaining()
        || !reader.Bytes(payload_size, payload) || engine_id != kEngineIdTiberianDawn
        || content_hash != instance->content_id_hash || payload_hash != HashBytes(payload, payload_size)
        || (game_mode != CNC_WEB_GAME_CAMPAIGN && game_mode != CNC_WEB_GAME_SKIRMISH)) {
        return CNC_WEB_CONTENT_MISMATCH;
    }
    InvalidateCachedState(*instance);
    /* Events belong to the state being replaced; retain only load/rollback events. */
    instance->events.clear();
    cnc_web_status_t status = instance->backend->Load(payload, payload_size, game_mode, tick, player_id);
    if (status != CNC_WEB_OK) {
        InvalidateCachedState(*instance);
        return status;
    }
    instance->tick = tick;
    instance->seed = CanonicalSeed(seed);
    instance->player_id = player_id;
    instance->game_mode = game_mode;
    instance->terminal = false;
    instance->event_tick = tick;
    instance->scheduled.clear();
    instance->queued_command_count = 0u;
    ResetSnapshotChain(*instance);
    return CNC_WEB_OK;
}

cnc_web_status_t cnc_web_state_hash(cnc_web_handle_t handle, uint64_t* out_hash)
{
    using namespace cnc::web;
    Instance* instance = Find(handle);
    if (instance == NULL || out_hash == NULL) {
        return CNC_WEB_INVALID_ARGUMENT;
    }
    cnc_web_status_t status = BuildSnapshot(*instance);
    if (status == CNC_WEB_OK) {
        *out_hash = instance->state_hash;
    }
    return status;
}

} /* extern "C" */
