// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef SOLEPACKETS_H
#define SOLEPACKETS_H

#include "soleparams.h"
#include "function.h"
#include <stdint.h>

enum PacketType
{
    PACKET_0,
    PACKET_CONNECTION,
    PACKET_SIDEBAR,
    PACKET_EVENT,
    PACKET_GAME_OPTIONS,
    PACKET_PLAYER_ACTIONS,
    PACKET_FRAMERATE,
    PACKET_OBJECT,
    PACKET_WAIT1,
    PACKET_WAIT2,
    PACKET_HOUSE_TEAM,
    PACKET_NEW_DELETE_OBJ,
    PACKET_HEALTH,
    PACKET_TEAM,
    PACKET_E,
    PACKET_CAPTURE,
    PACKET_CARGO,
    PACKET_FLAG,
    PACKET_CTF,
    PACKET_MOVEMENT,
    PACKET_TARGET,
    PACKET_FIRE_AT,
    PACKET_DO_TURN,
    PACKET_CRATE,
    PACKET_PCP,
    PACKET_TECHNO,
    PACKET_UI_EVENT,
    PACKET_GAME_RESULTS,
    PACKET_SCENARIO_CHANGE,
    PACKET_MESSAGE,
    PACKET_OBFUSCATED_MESSAGE,
    PACKET_SERVER_PASSWORD,
    PACKET_COUNT,
};

#pragma pack(push, 1)
struct GenericPacket
{
    uint16_t PacketLength;
    uint8_t SomeID;
};

struct ConnectionPacket
{
    uint16_t PacketLength;
    uint8_t PacketType;
    char PlayerName[12];
    uint8_t Side;
    uint8_t ChosenRTTI;
    int32_t ChosenType;
    int32_t field_15;
    int32_t field_19;
    int32_t VersionNumber;
};

struct GameOptionsPacket
{
    uint16_t PacketLength;
    uint8_t PacketType;
    uint8_t PlayerHouse;
    uint8_t Side;
    int32_t dword5;
    uint8_t Scenario;
    int32_t dwordA;
    uint32_t Bitfield1;
    uint8_t BuildLevel;
    uint8_t MPlayerUnitCount;
    uint32_t Special;
    GameParamStruct GameParams;
    int32_t TimerVal;
    uint32_t Bitfield2;
    int32_t TeamScores[4];
    uint8_t Damages[25];
    uint8_t RateOfFires[25];
    int32_t Ranges[25];
};

struct NewDeletePacket
{
    bool ToDelete;
    int32_t Target;
    uint32_t Coord;
    uint8_t Owner;
    uint8_t Mission;
    uint8_t Type;
    uint8_t Strength;
    uint8_t Speed;
    uint8_t Damage;
    uint8_t RateOfFire;
    uint8_t Range;
};

#pragma pack(pop)

extern int PacketLength[PACKET_COUNT];

#endif
