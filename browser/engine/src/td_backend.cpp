/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "backend.h"
#include "classic_surface.h"
#include "content_preflight.h"
#include "determinism.h"
#include "static_map_delta.h"
#include "td_save_state.h"

#include "common/ccfile.h"
#include "common/wwstd.h"
#include "tiberiandawn/dllinterface.h"
#include "tiberiandawn/function.h"
#include "tiberiandawn/externs.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <new>
#include <utility>
#include <vector>

typedef void(__cdecl* LegacyEventCallback)(const EventCallbackStruct& event);

extern "C" void __cdecl CNC_Init(const char* command_line, LegacyEventCallback event_callback);
extern "C" bool __cdecl CNC_Start_Instance_Variation(int scenario_index,
                                                       int scenario_variation,
                                                       int scenario_direction,
                                                       int build_level,
                                                       const char* faction,
                                                       const char* game_type,
                                                       const char* content_directory,
                                                       int sabotaged_structure,
                                                       const char* override_map_name);
extern "C" bool __cdecl CNC_Advance_Instance(unsigned long long player_id);
extern "C" bool __cdecl CNC_Get_Game_State(GameStateRequestEnum state_type,
                                             unsigned long long player_id,
                                             unsigned char* buffer,
                                             unsigned int buffer_size);
extern "C" bool __cdecl CNC_Get_Visible_Page(unsigned char* buffer, unsigned int& width, unsigned int& height);
extern "C" bool __cdecl CNC_Get_Palette(unsigned char (&palette)[256][3]);
extern "C" bool __cdecl CNC_Set_Multiplayer_Data(int scenario_index,
                                                   CNCMultiplayerOptionsStruct& game_options,
                                                   int num_players,
                                                   CNCPlayerInfoStruct* player_list,
                                                   int max_players);
extern "C" void __cdecl CNC_Handle_Input(InputRequestEnum input_event,
                                          unsigned char special_key_flags,
                                          unsigned long long player_id,
                                          int x1,
                                          int y1,
                                          int x2,
                                          int y2);
extern "C" void __cdecl CNC_Handle_Structure_Request(StructureRequestEnum request_type,
                                                      unsigned long long player_id,
                                                      int object_id);
extern "C" void __cdecl CNC_Handle_Unit_Request(UnitRequestEnum request_type, unsigned long long player_id);
extern "C" void __cdecl CNC_Handle_Sidebar_Request(SidebarRequestEnum request_type,
                                                    unsigned long long player_id,
                                                    int buildable_type,
                                                    int buildable_id,
                                                    short cell_x,
                                                    short cell_y);
extern "C" void __cdecl CNC_Handle_SuperWeapon_Request(SuperWeaponRequestEnum request_type,
                                                        unsigned long long player_id,
                                                        int buildable_type,
                                                        int buildable_id,
                                                        int x,
                                                        int y);
extern "C" void __cdecl CNC_Handle_ControlGroup_Request(ControlGroupRequestEnum request_type,
                                                         unsigned long long player_id,
                                                         unsigned char control_group_index);
extern "C" void __cdecl CNC_Handle_Game_Request(GameRequestEnum request_type);
extern "C" void __cdecl CNC_Handle_Debug_Request(DebugRequestEnum debug_request_type,
                                                    unsigned long long player_id,
                                                    const char* object_name,
                                                    int x,
                                                    int y,
                                                    bool unshroud,
                                                    bool enemy);
extern "C" bool __cdecl CNC_Clear_Object_Selection(unsigned long long player_id);
extern "C" bool __cdecl CNC_Select_Object(unsigned long long player_id, int object_type, int object_id);
extern "C" bool __cdecl CNC_Save_Load(bool save, const char* file_name, const char* game_type);
extern "C" void __cdecl CNC_Web_Shutdown(void);

extern int CustomSeed;
extern int Seed;
extern bool CNCFirstUpdate;

/* Narrow release-acceptance bridge. The browser worker exposes this only to
 * an intentional loopback acceptance session; the general debug interface
 * remains unavailable to the browser protocol. */
extern "C" CNC_WEB_EXPORT int CNC_Web_Acceptance_Force_Victory(void)
{
    if (GameToPlay != GAME_NORMAL || PlayerPtr == NULL || PlayerWins || PlayerLoses) {
        return 0;
    }
    CNC_Handle_Debug_Request(DEBUG_REQUEST_END_GAME, 0u, "WIN", 0, 0, false, false);
    return PlayerWins && !PlayerLoses ? 1 : 0;
}

namespace cnc {
namespace web {

namespace {

const uint32_t kScratchSize = 32u * 1024u * 1024u;
const uint32_t kMaximumLegacyEntries = 262144u;
const uint32_t kMaximumCommandBatchSize = 4096u;
const uint32_t kClassicMaximumWidth = 128u * 24u;
const uint32_t kClassicMaximumHeight = 128u * 24u;
const char kTemporarySavePath[] = "/cnc-web-engine-save.sav";

BackendEventSink g_event_sink = NULL;
void* g_event_sink_context = NULL;

std::string SafeString(const char* value, uint32_t limit)
{
    if (value == NULL) {
        return std::string();
    }
    uint32_t length = 0u;
    while (length < limit && value[length] != '\0') {
        ++length;
    }
    return std::string(value, length);
}

int32_t Low32(long long value)
{
    return static_cast<int32_t>(static_cast<unsigned long long>(value) & UINT64_C(0xffffffff));
}

int32_t High32(long long value)
{
    return static_cast<int32_t>((static_cast<unsigned long long>(value) >> 32u) & UINT64_C(0xffffffff));
}

void LegacyEvent(const EventCallbackStruct& source)
{
    if (g_event_sink == NULL) {
        return;
    }
    BackendEvent event;
    BackendEvent campaign_outcome;
    bool emit_campaign_outcome = false;
    event.player_id = static_cast<uint64_t>(source.GlyphXPlayerID);
    switch (source.EventType) {
    case CALLBACK_EVENT_SOUND_EFFECT:
        event.type = CNC_WEB_EVENT_SOUND;
        event.args[0] = source.SoundEffect.SFXIndex;
        event.args[1] = source.SoundEffect.Variation;
        event.args[2] = source.SoundEffect.PixelX;
        event.args[3] = source.SoundEffect.PixelY;
        event.args[4] = source.SoundEffect.SoundEffectPriority;
        event.args[5] = source.SoundEffect.SoundEffectContext;
        event.text1 = SafeString(source.SoundEffect.SoundEffectName, sizeof(source.SoundEffect.SoundEffectName));
        break;
    case CALLBACK_EVENT_SPEECH:
        event.type = CNC_WEB_EVENT_SPEECH;
        event.args[0] = source.Speech.SpeechIndex;
        event.text1 = SafeString(source.Speech.SpeechName, sizeof(source.Speech.SpeechName));
        break;
    case CALLBACK_EVENT_GAME_OVER:
        event.type = CNC_WEB_EVENT_GAME_OVER;
        event.flags = static_cast<uint16_t>((source.GameOver.Multiplayer ? 1u : 0u)
                                            | (source.GameOver.IsHuman ? 2u : 0u)
                                            | (source.GameOver.PlayerWins ? 4u : 0u));
        event.args[0] = source.GameOver.Score;
        event.args[1] = source.GameOver.Leadership;
        event.args[2] = source.GameOver.Efficiency;
        event.args[3] = source.GameOver.RemainingCredits;
        event.args[4] = source.GameOver.SabotagedStructureType;
        event.args[5] = source.GameOver.TimerRemaining;
        event.text1 = SafeString(source.GameOver.MovieName, 256u);
        event.text2 = SafeString(source.GameOver.AfterScoreMovieName, 256u);
        /* Do_Win is deliberately not run by the DLL tick path. Capture the
         * exact state it would carry before GAME_OVER tells the host to stop
         * advancing and tear down this instance. */
        if (!source.GameOver.Multiplayer && source.GameOver.IsHuman && PlayerPtr != NULL
            && PlayerPtr->Class != NULL
            && (PlayerPtr->Class->House == HOUSE_GOOD || PlayerPtr->Class->House == HOUSE_BAD)) {
            campaign_outcome.type = CNC_WEB_EVENT_CAMPAIGN_OUTCOME;
            campaign_outcome.flags = event.flags;
            campaign_outcome.player_id = event.player_id;
            campaign_outcome.args[0] = PlayerPtr->Credits;
            campaign_outcome.args[1] = static_cast<int32_t>(PlayerPtr->NukePieces & 0x07u);
            campaign_outcome.args[2] = source.GameOver.SabotagedStructureType;
            campaign_outcome.args[3] = Low32(static_cast<long long>(Scen.RandomNumber.Seed));
            campaign_outcome.args[4] = Scen.Scenario;
            campaign_outcome.args[5] = PlayerPtr->Class->House;
            campaign_outcome.text1 = SafeString(Scen.ScenarioName, sizeof(Scen.ScenarioName));
            emit_campaign_outcome = true;
        }
        break;
    case CALLBACK_EVENT_DEBUG_PRINT:
        event.type = CNC_WEB_EVENT_DEBUG;
        event.text1 = SafeString(source.DebugPrint.PrintString, 4096u);
        break;
    case CALLBACK_EVENT_MOVIE:
        event.type = CNC_WEB_EVENT_MOVIE;
        event.flags = source.Movie.Immediate ? 1u : 0u;
        event.args[0] = source.Movie.Theme;
        event.text1 = SafeString(source.Movie.MovieName, 256u);
        break;
    case CALLBACK_EVENT_MESSAGE: {
        event.type = CNC_WEB_EVENT_MESSAGE;
        uint32_t timeout_bits = 0u;
        memcpy(&timeout_bits, &source.Message.TimeoutSeconds, sizeof(timeout_bits));
        event.args[0] = static_cast<int32_t>(timeout_bits);
        event.args[1] = static_cast<int32_t>(source.Message.MessageType);
        event.args[2] = Low32(source.Message.MessageParam1);
        event.args[3] = High32(source.Message.MessageParam1);
        event.text1 = SafeString(source.Message.Message, 4096u);
        break;
    }
    case CALLBACK_EVENT_UPDATE_MAP_CELL:
        event.type = CNC_WEB_EVENT_MAP_CELL;
        event.args[0] = source.UpdateMapCell.CellX;
        event.args[1] = source.UpdateMapCell.CellY;
        event.text1 = SafeString(source.UpdateMapCell.TemplateTypeName,
                                 sizeof(source.UpdateMapCell.TemplateTypeName));
        break;
    case CALLBACK_EVENT_ACHIEVEMENT:
        event.type = CNC_WEB_EVENT_ACHIEVEMENT;
        event.text1 = SafeString(source.Achievement.AchievementType, 256u);
        event.text2 = SafeString(source.Achievement.AchievementReason, 1024u);
        break;
    case CALLBACK_EVENT_STORE_CARRYOVER_OBJECTS:
        event.type = CNC_WEB_EVENT_CARRYOVER;
        break;
    case CALLBACK_EVENT_SPECIAL_WEAPON_TARGETTING:
        event.type = CNC_WEB_EVENT_SPECIAL_WEAPON;
        event.args[0] = source.SpecialWeaponTargetting.Type;
        event.args[1] = source.SpecialWeaponTargetting.ID;
        event.args[2] = static_cast<int32_t>(source.SpecialWeaponTargetting.WeaponType);
        event.text1 = SafeString(source.SpecialWeaponTargetting.Name, sizeof(source.SpecialWeaponTargetting.Name));
        break;
    case CALLBACK_EVENT_BRIEFING_SCREEN:
        event.type = CNC_WEB_EVENT_BRIEFING;
        break;
    case CALLBACK_EVENT_CENTER_CAMERA:
        event.type = CNC_WEB_EVENT_CAMERA;
        /* The remaster callback carries legacy leptons. Browser presentation
         * and every other positional EventV1 field use absolute world pixels. */
        event.args[0] = Lepton_To_Pixel(source.CenterCamera.CoordX);
        event.args[1] = Lepton_To_Pixel(source.CenterCamera.CoordY);
        break;
    case CALLBACK_EVENT_PING:
        event.type = CNC_WEB_EVENT_PING;
        event.args[0] = Lepton_To_Pixel(source.Ping.CoordX);
        event.args[1] = Lepton_To_Pixel(source.Ping.CoordY);
        break;
    default:
        return;
    }
    if (emit_campaign_outcome) {
        g_event_sink(g_event_sink_context, campaign_outcome);
    }
    g_event_sink(g_event_sink_context, event);
}

void RuntimeDiagnostic(const StartConfig& config,
                       uint32_t code,
                       uint16_t flags,
                       cnc_web_status_t status,
                       const char* identifier,
                       const std::string& detail)
{
    if (g_event_sink == NULL) {
        return;
    }
    BackendEvent event;
    event.type = CNC_WEB_EVENT_RUNTIME_DIAGNOSTIC;
    event.flags = flags;
    event.player_id = config.player_id;
    event.args[0] = static_cast<int32_t>(code);
    event.args[1] = status;
    event.args[2] = config.scenario;
    event.args[3] = config.variation;
    event.args[4] = config.direction;
    event.args[5] = config.build_level;
    event.text1 = identifier == NULL ? std::string() : std::string(identifier);
    event.text2 = detail;
    g_event_sink(g_event_sink_context, event);
}

bool LegacyFileAvailable(const std::string& name)
{
    CCFileClass file(name.c_str());
    return file.Is_Available();
}

bool PushSection(std::vector<SnapshotSection>& sections,
                 uint16_t kind,
                 uint32_t count,
                 Writer& payload,
                 uint16_t flags = 0u)
{
    try {
        SnapshotSection section;
        section.kind = kind;
        section.flags = flags;
        section.count = count;
        section.payload.swap(payload.Data());
        sections.push_back(std::move(section));
    } catch (const std::bad_alloc&) {
        return false;
    }
    return true;
}

uint32_t ObjectFlags(const CNCObjectStruct& object)
{
    return (object.IsSelectable ? 1u << 0u : 0u) | (object.IsRepairing ? 1u << 1u : 0u)
        | (object.IsDumping ? 1u << 2u : 0u) | (object.IsTheaterSpecific ? 1u << 3u : 0u)
        | (object.CanRepair ? 1u << 4u : 0u) | (object.CanDemolish ? 1u << 5u : 0u)
        | (object.CanDemolishUnit ? 1u << 6u : 0u) | (object.RecentlyCreated ? 1u << 7u : 0u)
        | (object.IsALoaner ? 1u << 8u : 0u) | (object.IsFactory ? 1u << 9u : 0u)
        | (object.IsPrimaryFactory ? 1u << 10u : 0u)
        | (object.IsAntiGround ? 1u << 12u : 0u) | (object.IsAntiAircraft ? 1u << 13u : 0u)
        | (object.IsSubSurface ? 1u << 14u : 0u) | (object.IsNominal ? 1u << 15u : 0u)
        | (object.IsDog ? 1u << 16u : 0u) | (object.IsIronCurtain ? 1u << 17u : 0u)
        | (object.IsInFormation ? 1u << 18u : 0u)
        | (object.CanHarvest ? 1u << 20u : 0u) | (object.CanPlaceBombs ? 1u << 21u : 0u)
        | (object.IsFixedWingedAircraft ? 1u << 22u : 0u) | (object.IsFake ? 1u << 23u : 0u);
}

int32_t CameraWorldX()
{
    return Lepton_To_Pixel(Coord_X(Map.TacticalCoord));
}

int32_t CameraWorldY()
{
    return Lepton_To_Pixel(Coord_Y(Map.TacticalCoord));
}

bool InputUsesWorldPoint(InputRequestEnum request)
{
    return (request >= INPUT_REQUEST_MOUSE_MOVE && request <= INPUT_REQUEST_COMMAND_AT_POSITION)
        || (request >= INPUT_REQUEST_MOD_GAME_COMMAND_1_AT_POSITION
            && request <= INPUT_REQUEST_MOD_GAME_COMMAND_4_AT_POSITION);
}

bool InputUsesWorldArea(InputRequestEnum request)
{
    return request == INPUT_REQUEST_MOUSE_AREA || request == INPUT_REQUEST_MOUSE_AREA_ADDITIVE;
}

bool WorldPointCanNormalize(int32_t x, int32_t y)
{
    /* Leave ample headroom for camera and tactical-window offsets. */
    const int32_t limit = INT32_C(0x3fffffff);
    return x >= -limit && x <= limit && y >= -limit && y <= limit;
}

cnc_web_status_t NormalizeCommand(const Command& source, Command& normalized)
{
    normalized = source;
    if (source.type != CNC_WEB_COMMAND_INPUT && source.flags != 0u) {
        return CNC_WEB_INVALID_ARGUMENT;
    }

    switch (source.type) {
    case CNC_WEB_COMMAND_INPUT: {
        if (source.args[0] < INPUT_REQUEST_NONE
            || source.args[0] > INPUT_REQUEST_MOD_GAME_COMMAND_4_AT_POSITION
            || (source.flags & ~static_cast<uint16_t>(CNC_WEB_MODIFIER_CTRL | CNC_WEB_MODIFIER_ALT
                                                       | CNC_WEB_MODIFIER_SHIFT)) != 0u) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        const InputRequestEnum request = static_cast<InputRequestEnum>(source.args[0]);
        if (!InputUsesWorldPoint(request)) {
            break;
        }
        if (!WorldPointCanNormalize(source.args[1], source.args[2])
            || (InputUsesWorldArea(request) && !WorldPointCanNormalize(source.args[3], source.args[4]))) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        const int32_t camera_x = CameraWorldX();
        const int32_t camera_y = CameraWorldY();
        const int32_t tactical_x = InputUsesWorldArea(request) ? 0 : Map.TacPixelX;
        const int32_t tactical_y = InputUsesWorldArea(request) ? 0 : Map.TacPixelY;
        if (!WorldToLegacyPixel(source.args[1], camera_x, tactical_x, normalized.args[1])
            || !WorldToLegacyPixel(source.args[2], camera_y, tactical_y, normalized.args[2])) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        if (InputUsesWorldArea(request)
            && (!WorldToLegacyPixel(source.args[3], camera_x, 0, normalized.args[3])
                || !WorldToLegacyPixel(source.args[4], camera_y, 0, normalized.args[4]))) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        break;
    }
    case CNC_WEB_COMMAND_STRUCTURE:
        if (source.args[0] < INPUT_STRUCTURE_NONE || source.args[0] > INPUT_STRUCTURE_CANCEL) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        break;
    case CNC_WEB_COMMAND_UNIT:
        if (source.args[0] < INPUT_UNIT_NONE || source.args[0] > INPUT_UNIT_QUEUED_MOVEMENT_OFF) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        break;
    case CNC_WEB_COMMAND_SIDEBAR:
        if (source.args[0] < SIDEBAR_REQUEST_START_CONSTRUCTION
            || source.args[0] > SIDEBAR_REQUEST_CANCEL_CONSTRUCTION_MULTI || source.args[3] < INT16_MIN
            || source.args[3] > INT16_MAX || source.args[4] < INT16_MIN || source.args[4] > INT16_MAX) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        break;
    case CNC_WEB_COMMAND_SUPERWEAPON:
        if (source.args[0] != SUPERWEAPON_REQUEST_PLACE_SUPER_WEAPON
            || !WorldPointCanNormalize(source.args[3], source.args[4])
            || !WorldToLegacyPixel(source.args[3], CameraWorldX(), Map.TacPixelX, normalized.args[3])
            || !WorldToLegacyPixel(source.args[4], CameraWorldY(), Map.TacPixelY, normalized.args[4])) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        break;
    case CNC_WEB_COMMAND_CONTROL_GROUP:
        if (source.args[0] < CONTROL_GROUP_REQUEST_CREATE
            || source.args[0] > CONTROL_GROUP_REQUEST_ADDITIVE_SELECTION || source.args[1] < 0
            || source.args[1] > 9) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        break;
    case CNC_WEB_COMMAND_GAME:
        if (source.args[0] < INPUT_GAME_MOVIE_DONE || source.args[0] > INPUT_GAME_LOADING_DONE) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        break;
    case CNC_WEB_COMMAND_CLEAR_SELECTION:
    case CNC_WEB_COMMAND_SELECT_OBJECT:
        break;
    default:
        return CNC_WEB_INVALID_ARGUMENT;
    }
    return CNC_WEB_OK;
}

bool WriteObject(Writer& writer, const CNCObjectStruct& object, int32_t camera_x, int32_t camera_y)
{
    uint32_t can_move = 0u;
    uint32_t can_fire = 0u;
    for (uint32_t index = 0u; index < MAX_HOUSES; ++index) {
        can_move |= object.CanMove[index] ? 1u << index : 0u;
        can_fire |= object.CanFire[index] ? 1u << index : 0u;
    }
    const uint32_t occupy_count =
        static_cast<uint32_t>(std::max(0, std::min(object.OccupyListLength, MAX_OCCUPY_CELLS)));
    const uint32_t pip_count = static_cast<uint32_t>(std::max(0, std::min(object.NumPips, MAX_OBJECT_PIPS)));
    const uint32_t line_count = static_cast<uint32_t>(std::max(0, std::min(object.NumLines, MAX_OBJECT_LINES)));
    int32_t position_x = object.PositionX;
    int32_t position_y = object.PositionY;
    /* The remaster layer exporter obtains every object position from Coord_To_Pixel. */
    if (!LegacyWindowPixelToWorld(position_x, camera_x, position_x)
        || !LegacyWindowPixelToWorld(position_y, camera_y, position_y)) {
        return false;
    }
    if (!writer.FixedString(object.TypeName, sizeof(object.TypeName), 16u)
        || !writer.FixedString(object.AssetName, sizeof(object.AssetName), 16u)
        || !writer.FixedString(object.ProductionAssetName, sizeof(object.ProductionAssetName), 16u)
        || !writer.FixedString(object.OverrideDisplayName, 256u, 64u) || !writer.I32(object.Type)
        || !writer.I32(object.ID) || !writer.I32(object.BaseObjectID) || !writer.I32(object.BaseObjectType)
        || !writer.I32(position_x) || !writer.I32(position_y) || !writer.I32(object.Width)
        || !writer.I32(object.Height) || !writer.I32(object.Altitude) || !writer.I32(object.SortOrder)
        || !writer.I32(object.Scale) || !writer.I32(object.DrawFlags) || !writer.I16(object.MaxStrength)
        || !writer.I16(object.Strength) || !writer.U16(object.ShapeIndex) || !writer.U16(object.CellX)
        || !writer.U16(object.CellY) || !writer.U16(object.CenterCoordX) || !writer.U16(object.CenterCoordY)
        || !writer.I16(object.SimLeptonX) || !writer.I16(object.SimLeptonY) || !writer.U8(object.DimensionX)
        || !writer.U8(object.DimensionY) || !writer.U8(object.Rotation) || !writer.U8(object.MaxSpeed)
        || !writer.U8(static_cast<uint8_t>(object.Owner)) || !writer.U8(static_cast<uint8_t>(object.RemapColor))
        || !writer.U8(static_cast<uint8_t>(object.SubObject)) || !writer.U8(object.Cloak)
        || !writer.U8(object.ControlGroup) || !writer.Zeros(1u) || !writer.U32(object.IsSelectedMask)
        || !writer.U32(object.FlashingFlags) || !writer.U32(object.VisibleFlags) || !writer.U32(object.SpiedByFlags)
        || !writer.U32(ObjectFlags(object)) || !writer.U32(can_move) || !writer.U32(can_fire)
        || !writer.U16(static_cast<uint16_t>(occupy_count)) || !writer.U16(static_cast<uint16_t>(pip_count))
        || !writer.U16(static_cast<uint16_t>(std::max(0, std::min(object.MaxPips, MAX_OBJECT_PIPS))))
        || !writer.U16(static_cast<uint16_t>(std::max(0, std::min(object.NumLines, MAX_OBJECT_LINES))))) {
        return false;
    }
    for (uint32_t index = 0u; index < MAX_OCCUPY_CELLS; ++index) {
        if (!writer.I16(index < occupy_count ? object.OccupyList[index] : 0)) {
            return false;
        }
    }
    for (uint32_t index = 0u; index < MAX_OBJECT_PIPS; ++index) {
        if (!writer.I32(index < pip_count ? object.Pips[index] : 0)) {
            return false;
        }
    }
    for (uint32_t index = 0u; index < MAX_OBJECT_LINES; ++index) {
        const CNCObjectLineStruct& line = object.Lines[index];
        int32_t x = index < line_count ? line.X : 0;
        int32_t y = index < line_count ? line.Y : 0;
        int32_t x1 = index < line_count ? line.X1 : 0;
        int32_t y1 = index < line_count ? line.Y1 : 0;
        if (index < line_count
            && (!LegacyWindowPixelToWorld(x, camera_x, x) || !LegacyWindowPixelToWorld(y, camera_y, y)
                || !LegacyWindowPixelToWorld(x1, camera_x, x1)
                || !LegacyWindowPixelToWorld(y1, camera_y, y1))) {
            return false;
        }
        if (!writer.I32(x) || !writer.I32(y) || !writer.I32(x1) || !writer.I32(y1)
            || !writer.I32(index < line_count ? line.Frame : 0)
            || !writer.U8(index < line_count ? line.Color : 0u) || !writer.Zeros(3u)) {
            return false;
        }
    }
    for (uint32_t index = 0u; index < MAX_HOUSES; ++index) {
        if (!writer.U8(static_cast<uint8_t>(object.ActionWithSelected[index]))) {
            return false;
        }
    }
    return true;
}

bool SerializeStatic(const std::vector<uint8_t>& scratch,
                     std::vector<SnapshotSection>& sections,
                     std::vector<uint8_t>& previous_full_payload,
                     bool& has_baseline)
{
    const CNCMapDataStruct* map = reinterpret_cast<const CNCMapDataStruct*>(&scratch[0]);
    const int64_t raw_count = static_cast<int64_t>(map->MapCellWidth) * static_cast<int64_t>(map->MapCellHeight);
    const uint32_t count = raw_count < 0 ? 0u : static_cast<uint32_t>(std::min<int64_t>(raw_count, MAX_EXPORT_CELLS));
    Writer writer;
    if (!writer.I32(map->MapCellX) || !writer.I32(map->MapCellY) || !writer.I32(map->MapCellWidth)
        || !writer.I32(map->MapCellHeight) || !writer.I32(map->OriginalMapCellX)
        || !writer.I32(map->OriginalMapCellY) || !writer.I32(map->OriginalMapCellWidth)
        || !writer.I32(map->OriginalMapCellHeight) || !writer.I32(map->Theater)
        || !writer.FixedString(map->ScenarioName, sizeof(map->ScenarioName), 264u) || !writer.U32(count)) {
        return false;
    }
    for (uint32_t index = 0u; index < count; ++index) {
        if (!writer.FixedString(map->StaticCells[index].TemplateTypeName,
                                sizeof(map->StaticCells[index].TemplateTypeName),
                                32u)
            || !writer.I32(map->StaticCells[index].IconNumber)) {
            return false;
        }
    }

    StaticMapEncoding encoding;
    if (!EncodeStaticMap(writer.Data(), count, previous_full_payload, has_baseline, encoding)) {
        return false;
    }
    try {
        SnapshotSection section;
        section.kind = CNC_WEB_SECTION_STATIC_MAP;
        section.flags = 0u;
        section.count = count;
        section.payload.swap(encoding.payload);
        section.has_canonical_payload_hash = true;
        section.canonical_count = count;
        section.canonical_payload_size = encoding.canonical_payload_size;
        section.canonical_payload_hash = encoding.canonical_payload_hash;
        sections.push_back(std::move(section));

        /* An identical retained update leaves the existing complete baseline
         * intact. A bootstrap or logical change replaces it only after every
         * wire/canonical allocation above has succeeded. */
        if (!encoding.retained) {
            previous_full_payload = writer.Data();
        }
    } catch (const std::bad_alloc&) {
        return false;
    }
    has_baseline = true;
    return true;
}

bool SerializeDynamic(const std::vector<uint8_t>& scratch,
                      std::vector<SnapshotSection>& sections,
                      int32_t camera_x,
                      int32_t camera_y)
{
    const CNCDynamicMapStruct* map = reinterpret_cast<const CNCDynamicMapStruct*>(&scratch[0]);
    const uint32_t count = map->Count < 0 ? 0u : static_cast<uint32_t>(std::min(map->Count, static_cast<int>(kMaximumLegacyEntries)));
    const size_t needed = offsetof(CNCDynamicMapStruct, Entries) + static_cast<size_t>(count) * sizeof(CNCDynamicMapEntryStruct);
    if (needed > scratch.size()) {
        return false;
    }
    Writer writer;
    if (!writer.U32(map->VortexActive ? 1u : 0u) || !writer.I32(map->VortexX) || !writer.I32(map->VortexY)
        || !writer.I32(map->VortexWidth) || !writer.I32(map->VortexHeight)) {
        return false;
    }
    for (uint32_t index = 0u; index < count; ++index) {
        const CNCDynamicMapEntryStruct& entry = map->Entries[index];
        const uint16_t flags = static_cast<uint16_t>((entry.IsSmudge ? 1u : 0u) | (entry.IsOverlay ? 2u : 0u)
                                                     | (entry.IsResource ? 4u : 0u) | (entry.IsSellable ? 8u : 0u)
                                                     | (entry.IsTheaterShape ? 16u : 0u) | (entry.IsFlag ? 32u : 0u));
        int32_t position_x = entry.PositionX;
        int32_t position_y = entry.PositionY;
        /* Every dynamic entry is positioned through Coord_To_Pixel, including flag entries. */
        if (!LegacyWindowPixelToWorld(position_x, camera_x, position_x)
            || !LegacyWindowPixelToWorld(position_y, camera_y, position_y)) {
            return false;
        }
        if (!writer.FixedString(entry.AssetName, sizeof(entry.AssetName), 16u) || !writer.I32(position_x)
            || !writer.I32(position_y) || !writer.I32(entry.Width) || !writer.I32(entry.Height)
            || !writer.I32(entry.DrawFlags) || !writer.I16(entry.Type)
            || !writer.U8(static_cast<uint8_t>(entry.Owner)) || !writer.U8(entry.ShapeIndex) || !writer.U8(entry.CellX)
            || !writer.U8(entry.CellY) || !writer.U16(flags) || !writer.Zeros(4u)) {
            return false;
        }
    }
    return PushSection(sections, CNC_WEB_SECTION_DYNAMIC_MAP, count, writer);
}

bool SerializeObjects(const std::vector<uint8_t>& scratch,
                      std::vector<SnapshotSection>& sections,
                      int32_t camera_x,
                      int32_t camera_y)
{
    const CNCObjectListStruct* list = reinterpret_cast<const CNCObjectListStruct*>(&scratch[0]);
    const uint32_t count = list->Count < 0 ? 0u : static_cast<uint32_t>(std::min(list->Count, static_cast<int>(kMaximumLegacyEntries)));
    const size_t needed = offsetof(CNCObjectListStruct, Objects) + static_cast<size_t>(count) * sizeof(CNCObjectStruct);
    if (needed > scratch.size()) {
        return false;
    }
    Writer writer;
    if (!writer.Reserve(count * 472u)) {
        return false;
    }
    for (uint32_t index = 0u; index < count; ++index) {
        const uint32_t before = writer.Size();
        if (!WriteObject(writer, list->Objects[index], camera_x, camera_y) || writer.Size() - before != 472u) {
            return false;
        }
    }
    return PushSection(sections, CNC_WEB_SECTION_OBJECTS, count, writer);
}

bool SerializeSidebar(const std::vector<uint8_t>& scratch, std::vector<SnapshotSection>& sections)
{
    const CNCSidebarStruct* sidebar = reinterpret_cast<const CNCSidebarStruct*>(&scratch[0]);
    const int64_t raw_count = static_cast<int64_t>(sidebar->EntryCount[0]) + sidebar->EntryCount[1];
    const uint32_t count = raw_count < 0 ? 0u : static_cast<uint32_t>(std::min<int64_t>(raw_count, 4096));
    const size_t needed = offsetof(CNCSidebarStruct, Entries) + static_cast<size_t>(count) * sizeof(CNCSidebarEntryStruct);
    if (needed > scratch.size()) {
        return false;
    }
    const uint32_t flags = (sidebar->RepairBtnEnabled ? 1u : 0u) | (sidebar->SellBtnEnabled ? 2u : 0u)
        | (sidebar->RadarMapActive ? 4u : 0u);
    Writer writer;
    if (!writer.I32(sidebar->EntryCount[0]) || !writer.I32(sidebar->EntryCount[1]) || !writer.I32(sidebar->Credits)
        || !writer.I32(sidebar->CreditsCounter) || !writer.I32(sidebar->Tiberium)
        || !writer.I32(sidebar->MaxTiberium) || !writer.I32(sidebar->PowerProduced)
        || !writer.I32(sidebar->PowerDrained) || !writer.I32(sidebar->MissionTimer)
        || !writer.U32(sidebar->UnitsKilled) || !writer.U32(sidebar->BuildingsKilled)
        || !writer.U32(sidebar->UnitsLost) || !writer.U32(sidebar->BuildingsLost)
        || !writer.U32(sidebar->TotalHarvestedCredits) || !writer.U32(flags)) {
        return false;
    }
    for (uint32_t index = 0u; index < count; ++index) {
        const CNCSidebarEntryStruct& entry = sidebar->Entries[index];
        const uint32_t placement_count =
            static_cast<uint32_t>(std::max(0, std::min(entry.PlacementListLength, MAX_OCCUPY_CELLS)));
        uint32_t progress = 0u;
        memcpy(&progress, &entry.Progress, sizeof(progress));
        const uint32_t entry_flags = (entry.Completed ? 1u : 0u) | (entry.Constructing ? 2u : 0u)
            | (entry.ConstructionOnHold ? 4u : 0u) | (entry.Busy ? 8u : 0u)
            | (entry.BuildableViaCapture ? 16u : 0u) | (entry.Fake ? 32u : 0u);
        if (!writer.FixedString(entry.AssetName, sizeof(entry.AssetName), 16u) || !writer.I32(entry.BuildableType)
            || !writer.I32(entry.BuildableID) || !writer.I32(entry.Type) || !writer.I32(entry.SuperWeaponType)
            || !writer.I32(entry.Cost) || !writer.I32(entry.PowerProvided) || !writer.I32(entry.BuildTime)
            || !writer.U32(progress)
            || !writer.U32(placement_count)
            || !writer.U32(entry_flags)) {
            return false;
        }
        for (uint32_t cell = 0u; cell < MAX_OCCUPY_CELLS; ++cell) {
            if (!writer.I16(cell < placement_count ? entry.PlacementList[cell] : 0)) {
                return false;
            }
        }
    }
    return PushSection(sections, CNC_WEB_SECTION_SIDEBAR, count, writer);
}

bool SerializePlacement(const std::vector<uint8_t>& scratch, std::vector<SnapshotSection>& sections)
{
    const CNCPlacementInfoStruct* placement = reinterpret_cast<const CNCPlacementInfoStruct*>(&scratch[0]);
    const uint32_t count = placement->Count < 0 ? 0u : static_cast<uint32_t>(std::min(placement->Count, MAX_EXPORT_CELLS));
    const size_t needed = offsetof(CNCPlacementInfoStruct, CellInfo)
        + static_cast<size_t>(count) * sizeof(CNCPlacementCellInfoStruct);
    if (needed > scratch.size()) {
        return false;
    }
    Writer writer;
    for (uint32_t index = 0u; index < count; ++index) {
        const uint8_t flags = static_cast<uint8_t>((placement->CellInfo[index].PassesProximityCheck ? 1u : 0u)
                                                   | (placement->CellInfo[index].GenerallyClear ? 2u : 0u));
        if (!writer.U8(flags)) {
            return false;
        }
    }
    return PushSection(sections, CNC_WEB_SECTION_PLACEMENT, count, writer);
}

bool SerializeShroud(const std::vector<uint8_t>& scratch, std::vector<SnapshotSection>& sections)
{
    const CNCShroudStruct* shroud = reinterpret_cast<const CNCShroudStruct*>(&scratch[0]);
    const uint32_t count = shroud->Count < 0 ? 0u : static_cast<uint32_t>(std::min(shroud->Count, MAX_EXPORT_CELLS));
    const size_t needed = offsetof(CNCShroudStruct, Entries) + static_cast<size_t>(count) * sizeof(CNCShroudEntryStruct);
    if (needed > scratch.size()) {
        return false;
    }
    Writer writer;
    for (uint32_t index = 0u; index < count; ++index) {
        const CNCShroudEntryStruct& entry = shroud->Entries[index];
        const uint8_t flags = static_cast<uint8_t>((entry.IsVisible ? 1u : 0u) | (entry.IsMapped ? 2u : 0u)
                                                   | (entry.IsJamming ? 4u : 0u));
        if (!writer.U8(static_cast<uint8_t>(entry.ShadowIndex)) || !writer.U8(flags)) {
            return false;
        }
    }
    return PushSection(sections, CNC_WEB_SECTION_SHROUD, count, writer);
}

bool SerializeOccupiers(const std::vector<uint8_t>& scratch, std::vector<SnapshotSection>& sections)
{
    uint32_t offset = 0u;
    int32_t raw_cell_count = 0;
    if (scratch.size() < sizeof(raw_cell_count)) {
        return false;
    }
    memcpy(&raw_cell_count, &scratch[0], sizeof(raw_cell_count));
    offset += sizeof(raw_cell_count);
    const uint32_t cell_count = raw_cell_count < 0 ? 0u : static_cast<uint32_t>(std::min(raw_cell_count, MAX_EXPORT_CELLS));
    Writer writer;
    for (uint32_t cell = 0u; cell < cell_count; ++cell) {
        int32_t raw_count = 0;
        if (offset > scratch.size() || scratch.size() - offset < sizeof(raw_count)) {
            return false;
        }
        memcpy(&raw_count, &scratch[offset], sizeof(raw_count));
        offset += sizeof(raw_count);
        const uint32_t count = raw_count < 0 ? 0u : static_cast<uint32_t>(std::min(raw_count, 4096));
        if (!writer.U32(count)) {
            return false;
        }
        for (uint32_t index = 0u; index < count; ++index) {
            CNCOccupierObjectStruct object;
            if (offset > scratch.size() || scratch.size() - offset < sizeof(object)) {
                return false;
            }
            memcpy(&object, &scratch[offset], sizeof(object));
            offset += sizeof(object);
            if (!writer.I32(object.Type) || !writer.I32(object.ID)) {
                return false;
            }
        }
        /* The legacy exporter advances one extra object slot between cells. */
        if (offset > scratch.size() || scratch.size() - offset < sizeof(CNCOccupierObjectStruct)) {
            return false;
        }
        offset += sizeof(CNCOccupierObjectStruct);
    }
    return PushSection(sections, CNC_WEB_SECTION_OCCUPIERS, cell_count, writer);
}

bool SerializePlayer(const std::vector<uint8_t>& scratch,
                     std::vector<SnapshotSection>& sections,
                     uint8_t& home_x,
                     uint8_t& home_y)
{
    if (scratch.size() < sizeof(CNCPlayerInfoStruct)) {
        return false;
    }
    const CNCPlayerInfoStruct* player = reinterpret_cast<const CNCPlayerInfoStruct*>(&scratch[0]);
    const uint32_t action_count = std::min(player->ActionWithSelectedCount, static_cast<unsigned int>(MAX_EXPORT_CELLS));
    home_x = player->HomeCellX;
    home_y = player->HomeCellY;
    const uint32_t flags = (player->IsAI ? 1u : 0u) | (player->IsDefeated ? 2u : 0u)
        | (player->IsRadarJammed ? 4u : 0u);
    Writer writer;
    if (!writer.FixedString(player->Name, sizeof(player->Name), 64u) || !writer.U8(player->House)
        || !writer.U8(player->HomeCellX) || !writer.U8(player->HomeCellY) || !writer.U8(0u)
        || !writer.I32(player->ColorIndex) || !writer.U64(player->GlyphxPlayerID) || !writer.I32(player->Team)
        || !writer.I32(player->StartLocationIndex) || !writer.U32(flags) || !writer.U32(player->AllyFlags)
        || !writer.U32(player->SpiedPowerFlags) || !writer.U32(player->SpiedMoneyFlags)
        || !writer.I32(player->SelectedID) || !writer.I32(player->SelectedType) || !writer.U32(player->ScreenShake)
        || !writer.U32(action_count)) {
        return false;
    }
    for (uint32_t index = 0u; index < MAX_HOUSES; ++index) {
        if (!writer.I32(player->SpiedInfo[index].Power) || !writer.I32(player->SpiedInfo[index].Drain)
            || !writer.I32(player->SpiedInfo[index].Money)) {
            return false;
        }
    }
    for (uint32_t index = 0u; index < action_count; ++index) {
        if (!writer.U8(static_cast<uint8_t>(player->ActionWithSelected[index]))) {
            return false;
        }
    }
    return PushSection(sections, CNC_WEB_SECTION_PLAYER, 1u, writer);
}

bool SerializeCamera(std::vector<SnapshotSection>& sections,
                     uint8_t home_x,
                     uint8_t home_y,
                     int32_t world_x,
                     int32_t world_y)
{
    Writer writer;
    if (!writer.I32(world_x) || !writer.I32(world_y) || !writer.I32(Lepton_To_Pixel(Map.TacLeptonWidth))
        || !writer.I32(Lepton_To_Pixel(Map.TacLeptonHeight)) || !writer.I32(home_x * CELL_PIXEL_W)
        || !writer.I32(home_y * CELL_PIXEL_H)) {
        return false;
    }
    return PushSection(sections, CNC_WEB_SECTION_CAMERA, 1u, writer);
}

bool SerializeClassic(std::vector<SnapshotSection>& sections,
                      std::vector<uint8_t>& current,
                      std::vector<uint8_t>& previous,
                      uint32_t& previous_width,
                      uint32_t& previous_height,
                      bool& has_baseline)
{
    if (Map.MapCellWidth <= 0 || Map.MapCellHeight <= 0
        || static_cast<uint32_t>(Map.MapCellWidth) > kClassicMaximumWidth / 24u
        || static_cast<uint32_t>(Map.MapCellHeight) > kClassicMaximumHeight / 24u) {
        return false;
    }
    unsigned int width = static_cast<unsigned int>(Map.MapCellWidth) * 24u;
    unsigned int height = static_cast<unsigned int>(Map.MapCellHeight) * 24u;
    const uint32_t capacity = width * height;
    try {
        current.resize(capacity);
        previous.reserve(capacity);
    } catch (const std::bad_alloc&) {
        return false;
    }
    if (!CNC_Get_Visible_Page(&current[0], width, height)) {
        return true;
    }
    if (width == 0u || height == 0u || width > kClassicMaximumWidth || height > kClassicMaximumHeight
        || height > UINT32_MAX / width || width * height > capacity) {
        return false;
    }
    const uint32_t pixel_count = width * height;
    current.resize(pixel_count);

    ClassicSurfaceEncoding encoding;
    if (!EncodeClassicSurface(&current[0],
                              width,
                              height,
                              previous.empty() ? NULL : &previous[0],
                              previous_width,
                              previous_height,
                              has_baseline,
                              encoding)) {
        return false;
    }
    const uint32_t transmitted_pixels = encoding.delta ? encoding.dirty.width * encoding.dirty.height : pixel_count;
    try {
        SnapshotSection section;
        section.kind = CNC_WEB_SECTION_CLASSIC_SURFACE;
        section.flags = 0u;
        section.count = transmitted_pixels;
        section.payload.swap(encoding.payload);
        section.has_canonical_payload_hash = true;
        section.canonical_count = pixel_count;
        section.canonical_payload_size = encoding.canonical_payload_size;
        section.canonical_payload_hash = encoding.canonical_payload_hash;
        sections.push_back(std::move(section));
    } catch (const std::bad_alloc&) {
        return false;
    }

    /*
     * Rotate the two reserved frame buffers instead of copying the complete
     * indexed surface after every delta encode. On the next snapshot,
     * current.resize() reuses the old baseline allocation before the renderer
     * fills it, while previous retains the frame that was just encoded.
     */
    previous.swap(current);
    previous_width = width;
    previous_height = height;
    has_baseline = true;
    return true;
}

bool SerializePalette(std::vector<SnapshotSection>& sections)
{
    unsigned char palette[256][3];
    if (!CNC_Get_Palette(palette)) {
        return true;
    }
    Writer writer;
    if (!writer.Bytes(palette, sizeof(palette))) {
        return false;
    }
    return PushSection(sections, CNC_WEB_SECTION_PALETTE, 256u, writer);
}

bool ReadFile(const char* path, std::vector<uint8_t>& bytes)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            fclose(file);
        }
        return false;
    }
    const long size = ftell(file);
    if (size < 0 || static_cast<unsigned long>(size) > UINT32_MAX || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    try {
        bytes.resize(static_cast<uint32_t>(size));
    } catch (const std::bad_alloc&) {
        fclose(file);
        return false;
    }
    const bool result = bytes.empty() || fread(&bytes[0], 1u, bytes.size(), file) == bytes.size();
    fclose(file);
    return result;
}

bool WriteFile(const char* path, const uint8_t* bytes, uint32_t size)
{
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const bool result = size == 0u || fwrite(bytes, 1u, size, file) == size;
    const bool closed = fclose(file) == 0;
    return result && closed;
}

class TiberianDawnBackend : public Backend
{
public:
    TiberianDawnBackend()
        : game_mode_(CNC_WEB_GAME_CAMPAIGN)
        , primary_player_id_(0u)
        , initialized_(false)
        , start_attempted_(false)
        , static_has_baseline_(false)
        , classic_previous_width_(0u)
        , classic_previous_height_(0u)
        , classic_has_baseline_(false)
    {
    }

    virtual cnc_web_status_t Initialize(BackendEventSink sink, void* sink_context)
    {
        g_event_sink = sink;
        g_event_sink_context = sink_context;
        ResetLogicalClock();
        try {
            scratch_.resize(kScratchSize);
            normalized_commands_.reserve(kMaximumCommandBatchSize);
        } catch (const std::bad_alloc&) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        return CNC_WEB_OK;
    }

    virtual void Shutdown()
    {
        if (initialized_) {
            CNC_Web_Shutdown();
            initialized_ = false;
        }
        g_event_sink = NULL;
        g_event_sink_context = NULL;
        ResetSnapshotBaselines();
        remove(kTemporarySavePath);
    }

    virtual cnc_web_status_t Start(const StartConfig& config)
    {
        const ContentPreflight preflight = ValidateContentMount(config);
        for (std::vector<ContentIssue>::const_iterator issue = preflight.issues.begin();
             issue != preflight.issues.end();
             ++issue) {
            const bool optional = !issue->required;
            RuntimeDiagnostic(config,
                              optional ? CNC_WEB_DIAGNOSTIC_OPTIONAL_CONTENT_MISSING
                                       : CNC_WEB_DIAGNOSTIC_CONTENT_ERROR,
                              optional ? CNC_WEB_DIAGNOSTIC_WARNING : CNC_WEB_DIAGNOSTIC_ERROR,
                              optional ? CNC_WEB_OK : preflight.status,
                              optional ? "engine.content.optional-missing" : "engine.content.invalid",
                              issue->name + ": " + issue->detail);
        }
        if (preflight.status != CNC_WEB_OK) {
            return preflight.status;
        }
        if (start_attempted_) {
            RuntimeDiagnostic(config,
                              CNC_WEB_DIAGNOSTIC_START_FAILED,
                              CNC_WEB_DIAGNOSTIC_ERROR,
                              CNC_WEB_INVALID_STATE,
                              "engine.start.retry-requires-new-instance",
                              "legacy initialization has already been attempted");
            return CNC_WEB_INVALID_STATE;
        }

        std::string command_line;
        if (!BuildLegacyStartupCommand(config.content_directory, command_line)) {
            RuntimeDiagnostic(config,
                              CNC_WEB_DIAGNOSTIC_CONTENT_ERROR,
                              CNC_WEB_DIAGNOSTIC_ERROR,
                              CNC_WEB_INVALID_ARGUMENT,
                              "engine.content.invalid-root",
                              config.content_directory);
            return CNC_WEB_INVALID_ARGUMENT;
        }

        RuntimeDiagnostic(config,
                          CNC_WEB_DIAGNOSTIC_STARTING,
                          0u,
                          CNC_WEB_OK,
                          "engine.start.begin",
                          preflight.scenario_root);
        start_attempted_ = true;
        BeginDeterministicStartup(config.seed);
        CNC_Init(command_line.c_str(), LegacyEvent);
        initialized_ = true;
        /* Read_Scenario_Ini applies the destination mission's percentage and
         * cap to this raw cash value. It must be installed after CNC_Init has
         * reset globals but before Start_Scenario reads the INI. */
        Scen.CarryOverMoney = config.has_campaign_transition ? config.carry_over_money : 0;
        if (config.has_campaign_transition) {
            /* Unlike a fresh zero seed, a carried zero is live RNG state. */
            Scen.RandomNumber.Seed = config.seed;
        }
        CustomSeed = static_cast<int>(config.seed);
        Seed = static_cast<int>(config.seed);
        game_mode_ = config.game_mode;
        primary_player_id_ = config.game_mode == CNC_WEB_GAME_SKIRMISH && config.player_id == 0u ? 1u : config.player_id;

        if (!preflight.scenario_root.empty()) {
            const std::string ini_name = preflight.scenario_root + ".INI";
            if (!LegacyFileAvailable(ini_name)) {
                EndDeterministicStartup();
                RuntimeDiagnostic(config,
                                  CNC_WEB_DIAGNOSTIC_SCENARIO_MISSING,
                                  CNC_WEB_DIAGNOSTIC_ERROR,
                                  CNC_WEB_CONTENT_MISMATCH,
                                  "engine.scenario.missing",
                                  ini_name);
                return CNC_WEB_CONTENT_MISMATCH;
            }
            const std::string bin_name = preflight.scenario_root + ".BIN";
            if (!LegacyFileAvailable(bin_name)) {
                RuntimeDiagnostic(config,
                                  CNC_WEB_DIAGNOSTIC_OPTIONAL_CONTENT_MISSING,
                                  CNC_WEB_DIAGNOSTIC_WARNING,
                                  CNC_WEB_OK,
                                  "engine.scenario.binary-missing",
                                  bin_name + " (the scenario INI must contain MapPack fallback data)");
            }
        }
        const char* faction = config.faction == CNC_WEB_FACTION_GDI
            ? "GDI"
            : (config.faction == CNC_WEB_FACTION_NOD ? "NOD" : "Jurassic");
        const char* game_type = config.game_mode == CNC_WEB_GAME_CAMPAIGN ? "GAME_NORMAL" : "GAME_GLYPHX_MULTIPLAYER";

        if (config.game_mode == CNC_WEB_GAME_SKIRMISH) {
            CNCMultiplayerOptionsStruct options;
            memset(&options, 0, sizeof(options));
            options.MPlayerCount = 2;
            options.MPlayerBases = 1;
            options.MPlayerCredits = 5000;
            options.MPlayerTiberium = 1;
            options.MPlayerGoodies = 1;
            options.MPlayerGhosts = 1;
            options.MPlayerSolo = 1;
            options.MPlayerUnitCount = 0;
            options.EnableSuperweapons = true;

            CNCPlayerInfoStruct players[2];
            memset(players, 0, sizeof(players));
            strncpy(players[0].Name, "Commander", sizeof(players[0].Name) - 1u);
            players[0].House = config.faction == CNC_WEB_FACTION_NOD ? HOUSE_BAD : HOUSE_GOOD;
            players[0].ColorIndex = 0;
            players[0].GlyphxPlayerID = config.player_id == 0u ? 1u : config.player_id;
            players[0].Team = 0;
            players[0].StartLocationIndex = 0;
            players[0].IsAI = false;
            strncpy(players[1].Name, "Computer", sizeof(players[1].Name) - 1u);
            players[1].House = players[0].House == HOUSE_GOOD ? HOUSE_BAD : HOUSE_GOOD;
            players[1].ColorIndex = 1;
            players[1].GlyphxPlayerID = players[0].GlyphxPlayerID + 1u;
            players[1].Team = 1;
            players[1].StartLocationIndex = 1;
            players[1].IsAI = true;
            if (!CNC_Set_Multiplayer_Data(config.scenario, options, 2, players, 2)) {
                EndDeterministicStartup();
                RuntimeDiagnostic(config,
                                  CNC_WEB_DIAGNOSTIC_START_FAILED,
                                  CNC_WEB_DIAGNOSTIC_ERROR,
                                  CNC_WEB_INVALID_ARGUMENT,
                                  "engine.start.multiplayer-invalid",
                                  preflight.scenario_root);
                return CNC_WEB_INVALID_ARGUMENT;
            }
        }

        const char* override_name = config.override_map_name.empty() ? NULL : config.override_map_name.c_str();
        const bool started = CNC_Start_Instance_Variation(config.scenario,
                                                          config.variation,
                                                          config.direction,
                                                          config.build_level,
                                                          faction,
                                                          game_type,
                                                          config.content_directory.c_str(),
                                                          config.sabotaged_structure,
                                                          override_name);
        EndDeterministicStartup();
        if (!started) {
            RuntimeDiagnostic(config,
                              CNC_WEB_DIAGNOSTIC_START_FAILED,
                              CNC_WEB_DIAGNOSTIC_ERROR,
                              CNC_WEB_CONTENT_MISMATCH,
                              "engine.start.scenario-failed",
                              preflight.scenario_root);
            return CNC_WEB_CONTENT_MISMATCH;
        }
        if (config.has_campaign_transition) {
            if (PlayerPtr == NULL) {
                RuntimeDiagnostic(config,
                                  CNC_WEB_DIAGNOSTIC_START_FAILED,
                                  CNC_WEB_DIAGNOSTIC_ERROR,
                                  CNC_WEB_FATAL,
                                  "engine.start.campaign-transition-no-player",
                                  preflight.scenario_root);
                return CNC_WEB_FATAL;
            }
            /* Clear_Scenario constructs the new house. The original Do_Win
             * restores the three collection bits only after Start_Scenario. */
            PlayerPtr->NukePieces = config.nuke_pieces & 0x07u;
        }
        ResetSnapshotBaselines();
        RuntimeDiagnostic(config,
                          CNC_WEB_DIAGNOSTIC_START_READY,
                          0u,
                          CNC_WEB_OK,
                          "engine.start.ready",
                          preflight.scenario_root);
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t ValidateCommands(uint64_t player_id, const std::vector<Command>& commands)
    {
        (void)player_id;
        Command normalized;
        for (std::vector<Command>::const_iterator command = commands.begin(); command != commands.end(); ++command) {
            const cnc_web_status_t status = NormalizeCommand(*command, normalized);
            if (status != CNC_WEB_OK) {
                return status;
            }
        }
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t ApplyCommands(uint64_t player_id, const std::vector<Command>& commands)
    {
        if (game_mode_ == CNC_WEB_GAME_SKIRMISH && player_id == 0u) {
            player_id = primary_player_id_;
        }
        if (commands.size() > normalized_commands_.capacity()) {
            return CNC_WEB_INVALID_ARGUMENT;
        }
        normalized_commands_.resize(commands.size());
        for (size_t index = 0u; index < commands.size(); ++index) {
            const cnc_web_status_t status = NormalizeCommand(commands[index], normalized_commands_[index]);
            if (status != CNC_WEB_OK) {
                return status;
            }
        }

        for (std::vector<Command>::const_iterator command = normalized_commands_.begin();
             command != normalized_commands_.end();
             ++command) {
            switch (command->type) {
            case CNC_WEB_COMMAND_INPUT:
                CNC_Handle_Input(static_cast<InputRequestEnum>(command->args[0]),
                                 static_cast<unsigned char>(command->flags),
                                 player_id,
                                 command->args[1],
                                 command->args[2],
                                 command->args[3],
                                 command->args[4]);
                break;
            case CNC_WEB_COMMAND_STRUCTURE:
                CNC_Handle_Structure_Request(static_cast<StructureRequestEnum>(command->args[0]),
                                             player_id,
                                             command->args[1]);
                break;
            case CNC_WEB_COMMAND_UNIT:
                CNC_Handle_Unit_Request(static_cast<UnitRequestEnum>(command->args[0]), player_id);
                break;
            case CNC_WEB_COMMAND_SIDEBAR:
                CNC_Handle_Sidebar_Request(static_cast<SidebarRequestEnum>(command->args[0]),
                                           player_id,
                                           command->args[1],
                                           command->args[2],
                                           static_cast<short>(command->args[3]),
                                           static_cast<short>(command->args[4]));
                break;
            case CNC_WEB_COMMAND_SUPERWEAPON:
                CNC_Handle_SuperWeapon_Request(static_cast<SuperWeaponRequestEnum>(command->args[0]),
                                               player_id,
                                               command->args[1],
                                               command->args[2],
                                               command->args[3],
                                               command->args[4]);
                break;
            case CNC_WEB_COMMAND_CONTROL_GROUP:
                CNC_Handle_ControlGroup_Request(static_cast<ControlGroupRequestEnum>(command->args[0]),
                                                player_id,
                                                static_cast<unsigned char>(command->args[1]));
                break;
            case CNC_WEB_COMMAND_GAME:
                CNC_Handle_Game_Request(static_cast<GameRequestEnum>(command->args[0]));
                break;
            case CNC_WEB_COMMAND_CLEAR_SELECTION:
                CNC_Clear_Object_Selection(player_id);
                break;
            case CNC_WEB_COMMAND_SELECT_OBJECT:
                CNC_Select_Object(player_id, command->args[0], command->args[1]);
                break;
            default:
                return CNC_WEB_FATAL;
            }
        }
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t Advance(uint64_t player_id, bool& out_running)
    {
        if (game_mode_ == CNC_WEB_GAME_SKIRMISH && player_id == 0u) {
            player_id = primary_player_id_;
        }
        AdvanceLogicalClock();
        out_running = CNC_Advance_Instance(player_id);
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t Snapshot(uint64_t player_id, std::vector<SnapshotSection>& sections)
    {
        sections.clear();
        try {
            sections.reserve(11u);
        } catch (const std::bad_alloc&) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        std::vector<uint8_t>& scratch = scratch_;
        const unsigned long long effective_player = game_mode_ == CNC_WEB_GAME_CAMPAIGN
            ? 0u
            : static_cast<unsigned long long>(player_id == 0u ? primary_player_id_ : player_id);
        uint8_t home_x = 0u;
        uint8_t home_y = 0u;
        const int32_t camera_x = CameraWorldX();
        const int32_t camera_y = CameraWorldY();

        memset(&scratch[0], 0, sizeof(CNCMapDataStruct));
        if (!CNC_Get_Game_State(GAME_STATE_STATIC_MAP, effective_player, &scratch[0], scratch.size())
            || !SerializeStatic(scratch, sections, static_previous_, static_has_baseline_)) {
            return CNC_WEB_FATAL;
        }
        memset(&scratch[0], 0, offsetof(CNCDynamicMapStruct, Entries));
        CNC_Get_Game_State(GAME_STATE_DYNAMIC_MAP, effective_player, &scratch[0], scratch.size());
        if (!SerializeDynamic(scratch, sections, camera_x, camera_y)) {
            return CNC_WEB_FATAL;
        }
        memset(&scratch[0], 0, offsetof(CNCObjectListStruct, Objects));
        CNC_Get_Game_State(GAME_STATE_LAYERS, effective_player, &scratch[0], scratch.size());
        if (!SerializeObjects(scratch, sections, camera_x, camera_y)) {
            return CNC_WEB_FATAL;
        }
        memset(&scratch[0], 0, offsetof(CNCSidebarStruct, Entries));
        if (CNC_Get_Game_State(GAME_STATE_SIDEBAR, effective_player, &scratch[0], scratch.size())
            && !SerializeSidebar(scratch, sections)) {
            return CNC_WEB_FATAL;
        }
        memset(&scratch[0], 0, offsetof(CNCPlacementInfoStruct, CellInfo));
        if (CNC_Get_Game_State(GAME_STATE_PLACEMENT, effective_player, &scratch[0], scratch.size())
            && !SerializePlacement(scratch, sections)) {
            return CNC_WEB_FATAL;
        }
        memset(&scratch[0], 0, offsetof(CNCShroudStruct, Entries));
        if (!CNC_Get_Game_State(GAME_STATE_SHROUD, effective_player, &scratch[0], scratch.size())
            || !SerializeShroud(scratch, sections)) {
            return CNC_WEB_FATAL;
        }
        memset(&scratch[0], 0, sizeof(CNCOccupierHeaderStruct));
        if (!CNC_Get_Game_State(GAME_STATE_OCCUPIER, effective_player, &scratch[0], scratch.size())
            || !SerializeOccupiers(scratch, sections)) {
            return CNC_WEB_FATAL;
        }
        memset(&scratch[0], 0, sizeof(CNCPlayerInfoStruct));
        if (CNC_Get_Game_State(GAME_STATE_PLAYER_INFO, effective_player, &scratch[0], scratch.size())
            && !SerializePlayer(scratch, sections, home_x, home_y)) {
            return CNC_WEB_FATAL;
        }
        if (!SerializeCamera(sections, home_x, home_y, camera_x, camera_y)
            || !SerializeClassic(sections,
                                 classic_current_,
                                 classic_previous_,
                                 classic_previous_width_,
                                 classic_previous_height_,
                                 classic_has_baseline_)
            || !SerializePalette(sections)) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t Save(std::vector<uint8_t>& bytes)
    {
        return CaptureSavePayload(bytes);
    }

    virtual cnc_web_status_t DeterministicState(std::vector<uint8_t>& bytes)
    {
        Writer writer;
        const uint8_t nuke_pieces = PlayerPtr == NULL ? UINT8_MAX
                                                      : static_cast<uint8_t>(PlayerPtr->NukePieces & 0x07u);
        if (!writer.U32(Scen.RandomNumber.Seed) || !writer.U8(CNCFirstUpdate ? 1u : 0u)
            || !writer.I32(static_cast<int32_t>(SabotagedType)) || !writer.I32(Scen.CarryOverMoney)
            || !writer.U8(nuke_pieces)) {
            return CNC_WEB_OUT_OF_MEMORY;
        }
        bytes.swap(writer.Data());
        return CNC_WEB_OK;
    }

    virtual cnc_web_status_t Load(const uint8_t* bytes,
                                  uint32_t size,
                                  uint32_t game_mode,
                                  uint32_t tick,
                                  uint64_t player_id)
    {
        if (bytes == NULL || size == 0u) {
            return CNC_WEB_IO_ERROR;
        }

        TdDeterministicSaveState requested_state;
        const uint8_t* requested_legacy_save = NULL;
        uint32_t requested_legacy_size = 0u;
        const TdSavePayloadResult decoded =
            DecodeTdSavePayload(bytes, size, requested_state, requested_legacy_save, requested_legacy_size);
        if (decoded != TD_SAVE_PAYLOAD_OK
            || requested_state.first_update != (game_mode == CNC_WEB_GAME_CAMPAIGN && tick == 0u)
            || requested_state.sabotaged_structure < static_cast<int32_t>(STRUCT_NONE)
            || requested_state.sabotaged_structure >= static_cast<int32_t>(STRUCT_COUNT)) {
            return CNC_WEB_IO_ERROR;
        }

        std::vector<uint8_t> rollback_save;
        if (CaptureSavePayload(rollback_save) != CNC_WEB_OK) {
            return CNC_WEB_IO_ERROR;
        }
        TdDeterministicSaveState rollback_state;
        const uint8_t* rollback_legacy_save = NULL;
        uint32_t rollback_legacy_size = 0u;
        if (DecodeTdSavePayload(&rollback_save[0],
                                static_cast<uint32_t>(rollback_save.size()),
                                rollback_state,
                                rollback_legacy_save,
                                rollback_legacy_size)
            != TD_SAVE_PAYLOAD_OK) {
            return CNC_WEB_FATAL;
        }

        const uint32_t previous_game_mode = game_mode_;
        const uint64_t previous_player_id = primary_player_id_;
        remove(kTemporarySavePath);
        if (!WriteFile(kTemporarySavePath, requested_legacy_save, requested_legacy_size)) {
            remove(kTemporarySavePath);
            return CNC_WEB_IO_ERROR;
        }
        game_mode_ = game_mode;
        const bool loaded = CNC_Save_Load(false, kTemporarySavePath, GameTypeName());
        remove(kTemporarySavePath);
        if (loaded) {
            Scen.RandomNumber.Seed = requested_state.random_seed;
            CNCFirstUpdate = requested_state.first_update;
            SabotagedType = static_cast<StructType>(requested_state.sabotaged_structure);
            primary_player_id_ = game_mode == CNC_WEB_GAME_SKIRMISH && player_id == 0u ? 1u : player_id;
            SetLogicalTick(tick);
            ResetSnapshotBaselines();
            return CNC_WEB_OK;
        }

        game_mode_ = previous_game_mode;
        primary_player_id_ = previous_player_id;
        const bool rollback_written = WriteFile(kTemporarySavePath, rollback_legacy_save, rollback_legacy_size);
        const bool recovered = rollback_written && CNC_Save_Load(false, kTemporarySavePath, GameTypeName());
        if (recovered) {
            Scen.RandomNumber.Seed = rollback_state.random_seed;
            CNCFirstUpdate = rollback_state.first_update;
            SabotagedType = static_cast<StructType>(rollback_state.sabotaged_structure);
        }
        remove(kTemporarySavePath);

        StartConfig diagnostic_config;
        diagnostic_config.seed = 0u;
        diagnostic_config.scenario = 0;
        diagnostic_config.variation = 0;
        diagnostic_config.direction = 0;
        diagnostic_config.build_level = 0;
        diagnostic_config.sabotaged_structure = -1;
        diagnostic_config.faction = CNC_WEB_FACTION_GDI;
        diagnostic_config.game_mode = game_mode_;
        diagnostic_config.player_id = primary_player_id_;
        diagnostic_config.content_id_hash = 1u;
        RuntimeDiagnostic(diagnostic_config,
                          CNC_WEB_DIAGNOSTIC_SAVE_RECOVERED,
                          recovered ? CNC_WEB_DIAGNOSTIC_WARNING : CNC_WEB_DIAGNOSTIC_ERROR,
                          CNC_WEB_IO_ERROR,
                          recovered ? "engine.save.load-rejected-state-restored"
                                    : "engine.save.load-rejected-restore-failed",
                          "opaque legacy save payload was rejected");
        return recovered ? CNC_WEB_IO_ERROR : CNC_WEB_FATAL;
    }

private:
    cnc_web_status_t CaptureSavePayload(std::vector<uint8_t>& bytes)
    {
        std::vector<uint8_t> legacy_save;
        remove(kTemporarySavePath);
        if (!CNC_Save_Load(true, kTemporarySavePath, GameTypeName())
            || !ReadFile(kTemporarySavePath, legacy_save) || legacy_save.empty()) {
            remove(kTemporarySavePath);
            return CNC_WEB_IO_ERROR;
        }
        remove(kTemporarySavePath);
        TdDeterministicSaveState state;
        state.random_seed = Scen.RandomNumber.Seed;
        state.first_update = CNCFirstUpdate;
        state.sabotaged_structure = static_cast<int32_t>(SabotagedType);
        return EncodeTdSavePayload(legacy_save, state, bytes) ? CNC_WEB_OK : CNC_WEB_OUT_OF_MEMORY;
    }

    void ResetSnapshotBaselines()
    {
        static_previous_.clear();
        static_has_baseline_ = false;
        classic_current_.clear();
        classic_previous_.clear();
        classic_previous_width_ = 0u;
        classic_previous_height_ = 0u;
        classic_has_baseline_ = false;
    }

    const char* GameTypeName() const
    {
        return game_mode_ == CNC_WEB_GAME_CAMPAIGN ? "GAME_NORMAL" : "GAME_GLYPHX_MULTIPLAYER";
    }

    uint32_t game_mode_;
    uint64_t primary_player_id_;
    bool initialized_;
    bool start_attempted_;
    std::vector<uint8_t> scratch_;
    std::vector<Command> normalized_commands_;
    std::vector<uint8_t> static_previous_;
    bool static_has_baseline_;
    std::vector<uint8_t> classic_current_;
    std::vector<uint8_t> classic_previous_;
    uint32_t classic_previous_width_;
    uint32_t classic_previous_height_;
    bool classic_has_baseline_;
};

} // namespace

Backend* CreateBackend()
{
    return new (std::nothrow) TiberianDawnBackend();
}

void DestroyBackend(Backend* backend)
{
    delete backend;
}

} // namespace web
} // namespace cnc
