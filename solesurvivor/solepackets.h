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

extern int PacketLength[PACKET_COUNT];

#endif
