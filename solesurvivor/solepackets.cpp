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

int PacketLength[PACKET_COUNT] = {
    3,
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
    191, // Should be sizeof(NewDeletePacket)? Code handling it in original is odd.
    202,
    202,
    204,
    204,
    202,
    202,
    204,
    200,
    204,
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
