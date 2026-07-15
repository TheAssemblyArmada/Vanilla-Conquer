/*
 * Copyright 2026 The Vanilla Conquer Contributors
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. See License.txt at the repository root.
 */

#ifndef CNC_WEB_H
#define CNC_WEB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define CNC_WEB_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define CNC_WEB_EXPORT __attribute__((used, visibility("default")))
#else
#define CNC_WEB_EXPORT
#endif

/*
 * The ABI only exposes fixed-width scalar values and caller-owned byte
 * buffers. No C++ object, compiler-sized enum, bool, reference, or engine
 * pointer crosses this boundary. Pointer parameters below are offsets into
 * the caller's address space (linear memory in WebAssembly); pointers are
 * never stored in a wire payload.
 */
typedef uint32_t cnc_web_handle_t;
typedef int32_t cnc_web_status_t;

enum
{
    /* Version 2 adds retained STATIC_MAP snapshot payloads. Function
     * signatures and the persistent WebSaveV1 envelope remain unchanged. */
    CNC_WEB_ABI_VERSION = 2u,
    CNC_WEB_INVALID_HANDLE = 0u
};

enum
{
    CNC_WEB_OK = 0,
    CNC_WEB_NEED_BUFFER = 1,
    CNC_WEB_INVALID_ARGUMENT = 2,
    CNC_WEB_INVALID_STATE = 3,
    CNC_WEB_CONTENT_MISMATCH = 4,
    CNC_WEB_IO_ERROR = 5,
    CNC_WEB_OUT_OF_MEMORY = 6,
    CNC_WEB_FATAL = 7
};

CNC_WEB_EXPORT uint32_t cnc_web_abi_version(void);

CNC_WEB_EXPORT cnc_web_status_t cnc_web_create(uint32_t requested_abi_version,
                                                cnc_web_handle_t* out_handle);
CNC_WEB_EXPORT cnc_web_status_t cnc_web_destroy(cnc_web_handle_t handle);

/*
 * Supplies the state that the original campaign carries across a successful
 * mission boundary. This is an additive companion to StartV1 so existing
 * recordings and save envelopes keep their version-1 wire layout. It may be
 * called at most once, before cnc_web_start, and is only valid when the
 * following start selects campaign mode. carry_over_money is the legacy
 * HouseClass::Credits value (cash only, excluding stored Tiberium);
 * nuke_pieces is the three-bit Nod campaign collection state.
 */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_set_campaign_transition(cnc_web_handle_t handle,
                                                                 int32_t carry_over_money,
                                                                 uint32_t nuke_pieces);

/* start_config is a CNC_WEB_MESSAGE_START_V1 message. */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_start(cnc_web_handle_t handle,
                                               const uint8_t* start_config,
                                               uint32_t start_config_size);

/* command_batch is a CNC_WEB_MESSAGE_COMMAND_BATCH_V1 message. */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_submit_commands(cnc_web_handle_t handle,
                                                         const uint8_t* command_batch,
                                                         uint32_t command_batch_size);

/*
 * Advances at most tick_count fixed 15 Hz simulation ticks. Commands for a
 * tick are applied immediately before that tick. The game-over tick is the
 * final tick; a larger batch stops there and later calls succeed with zero
 * advanced ticks until a save is loaded. out_advanced may be null.
 */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_advance(cnc_web_handle_t handle,
                                                 uint32_t tick_count,
                                                 uint32_t* out_advanced);

/*
 * snapshot_size materializes and caches the current SnapshotV1. Reads are
 * repeatable until the simulation advances or a save is loaded, so a size
 * query and subsequent write always refer to the same tick.
 */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_snapshot_size(cnc_web_handle_t handle, uint32_t* out_size);
CNC_WEB_EXPORT cnc_web_status_t cnc_web_write_snapshot(cnc_web_handle_t handle,
                                                        uint8_t* buffer,
                                                        uint32_t capacity,
                                                        uint32_t* out_written);

/* event_size peeks the next EventV1; poll_event consumes it on success. */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_event_size(cnc_web_handle_t handle, uint32_t* out_size);
CNC_WEB_EXPORT cnc_web_status_t cnc_web_poll_event(cnc_web_handle_t handle,
                                                    uint8_t* buffer,
                                                    uint32_t capacity,
                                                    uint32_t* out_written);

/* save_size creates and caches a WebSaveV1 containing the original save bytes. */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_save_size(cnc_web_handle_t handle, uint32_t* out_size);
CNC_WEB_EXPORT cnc_web_status_t cnc_web_write_save(cnc_web_handle_t handle,
                                                    uint8_t* buffer,
                                                    uint32_t capacity,
                                                    uint32_t* out_written);
/*
 * A load is content-bound and transactional at the TD adapter: malformed
 * legacy payloads are rejected after restoring the pre-load engine state.
 * CNC_WEB_FATAL means that restoration itself failed and the instance must be
 * destroyed.
 */
CNC_WEB_EXPORT cnc_web_status_t cnc_web_load_save(cnc_web_handle_t handle,
                                                   const uint8_t* save_data,
                                                   uint32_t save_data_size);

CNC_WEB_EXPORT cnc_web_status_t cnc_web_state_hash(cnc_web_handle_t handle, uint64_t* out_hash);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CNC_WEB_H */
