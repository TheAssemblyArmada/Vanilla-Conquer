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
#include "internet.h"

bool PlanetWestwoodIsHost;
unsigned PlanetWestwoodGameID;
unsigned PlanetWestwoodStartTime;
bool GameStatisticsPacketSent;
bool ConnectionLost;
int WChatMaxAhead;
// TODO: Made these sizes up, to verify/adjust as needed.
char PlanetWestwoodHandle[12];
char PlanetWestwoodPassword[8];
char PlanetWestwoodIPAddress[IP_ADDRESS_MAX];
unsigned short PlanetWestwoodPortNumber = 1234;
int WChatSendRate;
bool SpawnedFromWChat;
int ShowCommand;
