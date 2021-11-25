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
#include "soleglobals.h"
#include "common/sockets.h"

bool LogTeams;
bool OfflineMode;
bool WDTRadarAdded;
bool ClientAICalled;
bool server_534780 = true; // Seems to be related to if some server logic is processed.
bool somestate_591BCC;
bool UseAltArt;
CELL FlagHomes[4];

HousesType Side;
HousesType SolePlayerHouse;
RTTIType ChosenRTTI;
int ChosenType;
int Steel;
int Green;
int Orange;
int TeamScores[4];
char TeamMessages[10][80];
int GameOption_577AEC;
int GameOption_577AF0;
int GameOption_577B00;
int CurrentVoiceTheme = 1;
unsigned GameOptionsBitfield;
bool GameOption_577B08;
bool GameOption_577B0C;
bool SoleEnhancedDefense;
bool DebugLogTeams;

int ClientFPS;
int LastServerAIFrame;
int CommStatsSpeedScale;
int RecievedBytesSec;
int SentBytesSec;
int SentTCP;
int SentUDP;
int RecievedTCP;
int RecievedUDP;
CountDownTimerClass CountDownTimerClass_590454;
CountDownTimerClass TransmisionStatsTimer;
CountDownTimerClass ServerCountDownTimerClass_5721D4;
char SoleHost[40];

int Density = 200;
int CrateDensity;

ProtocolClass* ListenerProtocol;
ListenerClass* Listener;

DynamicVectorClass<ReliableProtocolClass*> ReliableProtocols;
DynamicVectorClass<ReliableCommClass*> ReliableComms;
DynamicVectorClass<ReliableProtocolClass*> AdminProtocols;
DynamicVectorClass<ReliableCommClass*> AdminComms;

DynamicVectorClass<SolePlayerClass*> SolePlayers;

DynamicVectorClass<NewDeletePacket*> NewDeletePackets;

#ifdef _WIN32
HINSTANCE hWSockInstance = GetModuleHandleA(nullptr);
#endif

// Global class to ensure winsock is started and cleaned up correctly.
class SocketStartupGlobal
{
public:
    SocketStartupGlobal()
    {
        socket_startup();
    }

    ~SocketStartupGlobal()
    {
        socket_cleanup();
    }
} SocketStartup;
