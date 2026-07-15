/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_PROTOCOL_H
#define CNC_WEB_PROTOCOL_H

#include <stdint.h>

/* All multi-byte fields are unsigned little-endian unless documented signed. */
enum
{
    CNC_WEB_PROTOCOL_VERSION = 1u,
    CNC_WEB_MESSAGE_HEADER_SIZE_V1 = 16u,
    CNC_WEB_START_FIXED_SIZE_V1 = 72u,
    CNC_WEB_COMMAND_BATCH_FIXED_SIZE_V1 = 32u,
    CNC_WEB_COMMAND_RECORD_SIZE_V1 = 32u,
    CNC_WEB_SNAPSHOT_FIXED_SIZE_V1 = 40u,
    CNC_WEB_SECTION_HEADER_SIZE_V1 = 16u,
    CNC_WEB_EVENT_FIXED_SIZE_V1 = 64u,
    CNC_WEB_SAVE_FIXED_SIZE_V1 = 60u
};

/* Bytes "CNCW", "CNCS", and "CNCE" when read as little-endian uint32. */
enum
{
    CNC_WEB_MAGIC_MESSAGE = 0x57434E43u,
    CNC_WEB_MAGIC_SAVE = 0x53434E43u,
    CNC_WEB_MAGIC_ENGINE_SAVE = 0x45434E43u
};

enum
{
    CNC_WEB_MESSAGE_START_V1 = 1u,
    CNC_WEB_MESSAGE_COMMAND_BATCH_V1 = 2u,
    CNC_WEB_MESSAGE_SNAPSHOT_V1 = 3u,
    CNC_WEB_MESSAGE_EVENT_V1 = 4u,
    CNC_WEB_MESSAGE_SAVE_V1 = 5u
};

enum
{
    CNC_WEB_FACTION_GDI = 1u,
    CNC_WEB_FACTION_NOD = 2u,
    CNC_WEB_FACTION_JURASSIC = 3u
};

enum
{
    CNC_WEB_GAME_CAMPAIGN = 1u,
    CNC_WEB_GAME_SKIRMISH = 2u
};

/*
 * StartV1 layout after the common 16-byte header:
 *   +16 seed u32, +20 scenario i32, +24 variation i32, +28 direction i32,
 *   +32 build_level i32, +36 sabotaged_structure i32, +40 faction u32,
 *   +44 game_mode u32, +48 player_id u64, +56 content_dir_length u32,
 *   +60 override_map_length u32, +64 content_id_hash u64, followed by the two
 *   non-NUL UTF-8 strings. content_id_hash identifies the pack revision and
 *   must remain stable when the same extracted content is mounted elsewhere.
 * content_dir is a canonical absolute POSIX directory whose segments use
 * ASCII letters, digits, dot, underscore, or hyphen, without traversal. The
 * legacy files live directly at that root using their uppercase names. The
 * GDI mission-one slice requires CCLOCAL, CONQUER, GENERAL, SOUNDS, SPEECH,
 * TEMPERAT, TEMPICNH, TRANSIT, and UPDATEC MIX archives plus loose
 * SCG01EA.INI and SCG01EA.BIN. LOCAL, SCORES, UPDATA, UPDATE, MOVIES, and a
 * loose TEMPERAT.PAL are optional and reported as diagnostics.
 */

/* Normalized command categories. The seven signed arguments are type-specific. */
enum
{
    CNC_WEB_COMMAND_INPUT = 1u,
    CNC_WEB_COMMAND_STRUCTURE = 2u,
    CNC_WEB_COMMAND_UNIT = 3u,
    CNC_WEB_COMMAND_SIDEBAR = 4u,
    CNC_WEB_COMMAND_SUPERWEAPON = 5u,
    CNC_WEB_COMMAND_CONTROL_GROUP = 6u,
    CNC_WEB_COMMAND_GAME = 7u,
    CNC_WEB_COMMAND_CLEAR_SELECTION = 8u,
    CNC_WEB_COMMAND_SELECT_OBJECT = 9u
};

/* Modifier flags accepted by CNC_WEB_COMMAND_INPUT. */
enum
{
    CNC_WEB_MODIFIER_CTRL = 1u << 0,
    CNC_WEB_MODIFIER_ALT = 1u << 1,
    CNC_WEB_MODIFIER_SHIFT = 1u << 2
};

/*
 * CommandBatchV1 layout after the common header:
 *   +16 target_tick u32, +20 record_size u16, +22 reserved u16,
 *   +24 player_id u64, then count records.
 * CommandRecordV1 is: type u16, flags u16, then args[7] i32.
 * Pixel positions on the wire are absolute map-world pixels so recordings do
 * not depend on a client's camera. INPUT stores the legacy request enum in
 * args[0], its first world point in args[1..2], and for mouse-area requests a
 * second world point in args[3..4]. SUPERWEAPON uses args[3..4] for its world
 * point. Sidebar placement positions remain map cells, not pixels. The TD
 * adapter translates world positions to its viewport-relative input API.
 */

enum
{
    CNC_WEB_SECTION_STATIC_MAP = 1u,
    CNC_WEB_SECTION_DYNAMIC_MAP = 2u,
    CNC_WEB_SECTION_OBJECTS = 3u,
    CNC_WEB_SECTION_SIDEBAR = 4u,
    CNC_WEB_SECTION_PLACEMENT = 5u,
    CNC_WEB_SECTION_SHROUD = 6u,
    CNC_WEB_SECTION_OCCUPIERS = 7u,
    CNC_WEB_SECTION_PLAYER = 8u,
    CNC_WEB_SECTION_CLASSIC_SURFACE = 9u,
    CNC_WEB_SECTION_PALETTE = 10u,
    CNC_WEB_SECTION_CAMERA = 11u
};

enum
{
    CNC_WEB_STATIC_MAP_FIXED_SIZE_V1 = 304u,
    CNC_WEB_STATIC_CELL_RECORD_SIZE_V1 = 36u,
    CNC_WEB_DYNAMIC_MAP_FIXED_SIZE_V1 = 20u,
    CNC_WEB_DYNAMIC_MAP_RECORD_SIZE_V1 = 48u,
    CNC_WEB_OBJECT_RECORD_SIZE_V1 = 472u,
    CNC_WEB_SIDEBAR_FIXED_SIZE_V1 = 60u,
    CNC_WEB_SIDEBAR_RECORD_SIZE_V1 = 128u,
    CNC_WEB_PLAYER_FIXED_SIZE_V1 = 504u,
    CNC_WEB_CAMERA_SIZE_V1 = 24u,
    CNC_WEB_CLASSIC_SURFACE_FIXED_SIZE_V1 = 16u,
    CNC_WEB_CLASSIC_SURFACE_DELTA_FIXED_SIZE_V1 = 32u,
    CNC_WEB_PALETTE_SIZE_V1 = 256u * 3u
};

enum
{
    CNC_WEB_CLASSIC_SURFACE_FORMAT_FULL = 1u,
    CNC_WEB_CLASSIC_SURFACE_FORMAT_DELTA = 2u
};

enum
{
    CNC_WEB_SNAPSHOT_FLAG_TERMINAL = 1u << 0u
};

enum
{
    CNC_WEB_EVENT_SOUND = 1u,
    CNC_WEB_EVENT_SPEECH = 2u,
    CNC_WEB_EVENT_GAME_OVER = 3u,
    CNC_WEB_EVENT_DEBUG = 4u,
    CNC_WEB_EVENT_MOVIE = 5u,
    CNC_WEB_EVENT_MESSAGE = 6u,
    CNC_WEB_EVENT_MAP_CELL = 7u,
    CNC_WEB_EVENT_ACHIEVEMENT = 8u,
    CNC_WEB_EVENT_CARRYOVER = 9u,
    CNC_WEB_EVENT_SPECIAL_WEAPON = 10u,
    CNC_WEB_EVENT_BRIEFING = 11u,
    CNC_WEB_EVENT_CAMERA = 12u,
    CNC_WEB_EVENT_PING = 13u,
    CNC_WEB_EVENT_RUNTIME_DIAGNOSTIC = 14u,
    CNC_WEB_EVENT_CAMPAIGN_OUTCOME = 15u
};

enum
{
    CNC_WEB_DIAGNOSTIC_STARTING = 1u,
    CNC_WEB_DIAGNOSTIC_OPTIONAL_CONTENT_MISSING = 2u,
    CNC_WEB_DIAGNOSTIC_CONTENT_ERROR = 3u,
    CNC_WEB_DIAGNOSTIC_SCENARIO_MISSING = 4u,
    CNC_WEB_DIAGNOSTIC_START_FAILED = 5u,
    CNC_WEB_DIAGNOSTIC_START_READY = 6u,
    CNC_WEB_DIAGNOSTIC_SAVE_RECOVERED = 7u
};

enum
{
    CNC_WEB_DIAGNOSTIC_WARNING = 1u << 0u,
    CNC_WEB_DIAGNOSTIC_ERROR = 1u << 1u
};

/*
 * Every message starts with:
 *   magic u32, protocol_version u16, message_kind u16,
 *   total_byte_size u32, item_count u32.
 *
 * SnapshotV1 continues with tick u32, base_tick u32, state_hash u64,
 * section_count u32, flags u32. Snapshot flag bit 0 marks the terminal
 * game-over state. Each section has kind u16, flags u16,
 * payload_size u32, item_count u32, reserved u32, then payload bytes. The
 * state_hash also includes engine-private deterministic state (such as the
 * live simulation RNG) that is deliberately not serialized as a section.
 * A bootstrap snapshot uses its own tick as base_tick. Later materialized
 * snapshots use the preceding materialized snapshot tick as base_tick; ticks
 * may be skipped when transfer backpressure coalesces snapshots. A receiver
 * must not apply a retained or delta section without its matching base.
 *
 * EventV1 continues with tick u32, event_type u16, flags u16, player_id u64,
 * args[6] i32, text1_length u32, text2_length u32, then UTF-8 text bytes.
 *
 * WebSaveV1 continues with tick u32, seed u32, player_id u64, payload_hash
 * u64, engine_id u32, content_id_hash u64, payload_size u32, flags u32,
 * followed by the opaque original engine save.
 *
 * RUNTIME_DIAGNOSTIC events use args[0] for a CNC_WEB_DIAGNOSTIC_* code,
 * args[1] for the associated cnc_web_status_t, and args[2..5] for scenario,
 * variation, direction, and build level. text1 is a stable dotted diagnostic
 * identifier and text2 is a human-readable path/detail. Diagnostics remain
 * pollable when start fails. MOVIE events require the host to pause or omit
 * presentation, then submit COMMAND_GAME with INPUT_GAME_MOVIE_DONE (argument
 * zero) before resuming ticks. BRIEFING is informational in Tiberian Dawn;
 * INPUT_GAME_LOADING_DONE is not required by the TD core. CAMERA and PING use
 * args[0]/args[1] for absolute map-world x/y pixels.
 *
 * CAMPAIGN_OUTCOME is emitted immediately before the matching single-player
 * GAME_OVER event. Its flags mirror GAME_OVER. args[0] is the signed raw cash
 * balance carried by the original game (HouseClass::Credits, deliberately not
 * Available_Money), args[1] is the three-bit Nod nuke-piece state, args[2] is
 * the sabotaged structure type, args[3] is the terminal scenario RNG seed bit
 * pattern, args[4] is the scenario number, and args[5] is the legacy house
 * (0 GDI, 1 Nod). text1 is the uppercase scenario root. The host must
 * correlate it with GAME_OVER and the active catalog mission before offering
 * campaign progression.
 */

/*
 * Snapshot section payloads (all records are tightly packed wire bytes, not
 * native structs):
 *
 * STATIC_MAP: map x/y/width/height i32, original x/y/width/height i32,
 * theater i32, scenario UTF-8 char[264], repeated-cell-count u32; each cell is
 * template_name char[32], icon i32. The section envelope count is the cell
 * count and cells are row-major. A full/bootstrap payload is the fixed 304
 * bytes followed by count cell records. When the complete logical payload is
 * unchanged from base_tick, a retained payload contains exactly the fixed 304
 * bytes and no cell records; the metadata and logical envelope count remain
 * current. Any metadata, template, icon, dimension, or ordering change emits
 * a full replacement. State hashing canonicalizes either representation to
 * the complete logical payload, so its value does not depend on snapshot
 * history. A bootstrap is emitted after start/load.
 *
 * DYNAMIC_MAP: vortex_active u32, vortex x/y/width/height i32; each 48-byte
 * record is asset char[16], x/y/width/height/draw_flags i32, type i16,
 * owner i8, shape/cell_x/cell_y u8, flags u16, reserved[4]. Flag bits are
 * smudge, overlay, resource, sellable, theater-shape, and flag-object. Pixel
 * x/y values are absolute map-world pixels even when the retained legacy
 * draw_flags contain SHAPE_WIN_REL.
 *
 * OBJECTS: each 472-byte record is type_name[16], asset_name[16], production
 * asset[16], override_display_name[64]; type/id/base_id/base_type and
 * x/y/width/height/altitude/sort_order/scale/draw_flags i32; max_strength and
 * strength i16; shape/cell_x/cell_y/center_x/center_y u16; sim_x/sim_y i16;
 * dimension_x/dimension_y/rotation/max_speed/owner/remap/subobject/cloak/
 * control_group u8; reserved u8; selected/flashing/visible/spied/object_flags/
 * can_move_mask/can_fire_mask u32; occupy_count/pip_count/max_pips/line_count
 * u16; occupy[36] i16; pips[18] i32; three lines of x/y/x1/y1/frame i32,
 * color u8, reserved[3]; action_with_selected[32] u8. Object x/y and line
 * endpoint pixels are absolute map-world pixels; draw_flags remain rendering
 * metadata and do not change that coordinate space. OBJECT flags in order
 * from bit 0: selectable, repairing, dumping, theater-specific, can-repair,
 * can-demolish, can-demolish-unit, recently-created, loaner, factory,
 * primary-factory, reserved, anti-ground, anti-air, sub-surface, nominal,
 * dog, iron-curtain, in-formation, reserved, can-harvest, can-place-bombs,
 * fixed-wing, fake.
 * action_with_selected values are the stable remaster DLL action enum:
 * none, move, no-move, enter, self/deploy, attack, attack-out-of-range,
 * guard, select, capture, sabotage, heal, damage, toggle-primary,
 * cannot-deploy, repair, and cannot-repair.
 *
 * SIDEBAR: column counts i32; credits/counter/tiberium/max-tiberium/power
 * produced/power drained/mission timer i32; units-killed/buildings-killed/
 * units-lost/buildings-lost/harvested u32; flags u32. Each 128-byte entry is
 * asset[16], buildable_type/id/object_type/superweapon/cost/power/build_time
 * i32, progress IEEE-754 bits u32, placement_count u32, flags u32, and
 * placement[36] i16. The placement values are legacy CELL offsets whose row
 * stride is 128 cells in the MEGAMAPS WebTD module (offset = dy * 128 + dx).
 * Header flags are repair/sell/radar; entry flags are completed/constructing/
 * on-hold/busy/via-capture/fake.
 *
 * PLACEMENT: one u8 per row-major cell (bit 0 proximity, bit 1 clear). Its
 * origin, width, height, and row order are the expanded map bounds from
 * STATIC_MAP (map x/y/width/height), not the original map bounds. SIDEBAR
 * place-command cell x/y arguments are zero-based relative to that expanded
 * origin.
 * SHROUD: two bytes per row-major cell: shadow i8 and visible/mapped/jammed
 * flags u8. OCCUPIERS: for each row-major cell, object_count u32 followed by
 * object_type/id i32 pairs.
 *
 * PLAYER: name[64], house/home_x/home_y/reserved u8, color i32, player_id u64,
 * team/start_location i32, flags/ally/spied-power/spied-money u32, selected_id/
 * selected_type i32, screen_shake/action_count u32, 32 power/drain/money i32
 * triples, then action_count u8 action values. CAMERA is six i32 values:
 * world x/y, viewport width/height, and home world x/y. The camera is an
 * engine hint; smooth pan/zoom remains browser-owned.
 *
 * CLASSIC_SURFACE format 1 is a bootstrap/full image: full_width/full_height/
 * pitch/format u32 followed by pitch*full_height R8 palette-index pixels.
 * Format 2 is a delta: full_width/full_height/rect_pitch/format u32, then
 * rect_x/rect_y/rect_width/rect_height u32, followed by tightly packed,
 * row-major rect pixels. An unchanged format-2 surface has a zero rectangle
 * and no pixels. A bootstrap is emitted after start/load; later snapshots use
 * deltas. The section item count is the number of transmitted pixels (the full
 * image for format 1, the dirty rectangle for format 2). State hashing uses
 * the logical full pixel count and canonicalizes either representation to the
 * equivalent full image, so its value is independent of snapshot history. The
 * surface world origin is the STATIC_MAP original map x/y multiplied by the
 * 24-pixel cell size. PALETTE is exactly 256 RGB triplets in index order.
 */

#endif /* CNC_WEB_PROTOCOL_H */
