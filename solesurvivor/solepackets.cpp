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
#include "solepackets.h"

static_assert(sizeof(ConnectionPacket) == 33, "Connection packet not expected size.");
static_assert(sizeof(GameOptionsPacket) == 422, "Game Options packet not expected size.");
static_assert(sizeof(NewDeletePacket) == 191, "New/Delete packet not expected size.");
static_assert(sizeof(HealthPacket) == 202, "Health packet not expected size.");
static_assert(sizeof(TeamPacket) == 202, "Team packet not expected size.");
static_assert(sizeof(CrushPacket) == 204, "Crush packet not expected size.");
static_assert(sizeof(CapturePacket) == 204, "Capture packet not expected size.");
static_assert(sizeof(CargoPacket) == 202, "Cargo packet not expected size.");
static_assert(sizeof(FlagPacket) == 202, "Flag packet not expected size.");
static_assert(sizeof(CTFPacket) == 204, "CTF packet not expected size.");
static_assert(sizeof(MovementPacket) == 200, "Movement packet not expected size.");
static_assert(sizeof(TargetPacket) == 204, "Target packet not expected size.");

int PacketLength[PACKET_COUNT] = {
    sizeof(GenericPacket),
    sizeof(ConnectionPacket),
    4,
    207,
    sizeof(GameOptionsPacket),
    204,
    7,
    7,
    207,
    4,
    174,
    sizeof(NewDeletePacket),
    sizeof(HealthPacket),
    sizeof(TeamPacket),
    sizeof(CrushPacket),
    sizeof(CapturePacket),
    sizeof(CargoPacket),
    sizeof(FlagPacket),
    sizeof(CTFPacket),
    sizeof(MovementPacket),
    sizeof(TargetPacket),
    202,
    204,
    204,
    196,
    202,
    4,
    4,
    11,
    89,
    11,
    7,
};
