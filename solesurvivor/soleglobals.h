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

#ifndef SOLEGLOBALS_H
#define SOLEGLOBALS_H

#include "common/timer.h"
#include "common/vector.h"
#include "function.h"

extern bool LogTeams;
extern bool OfflineMode;
extern bool WDTRadarAdded;
extern bool ClientAICalled;
extern bool server_534780; // Possibly indicates server is remote?
extern bool somestate_591BCC;
extern bool UseAltArt;

extern HousesType Side;
extern HousesType SolePlayerHouse;
extern RTTIType ChosenRTTI;
extern int ChosenType;
extern int Steel;
extern int Green;
extern int Orange;
extern int TeamScores[4];
extern char TeamMessages[10][80];
extern int GameOption_577AEC;
extern int GameOption_577AF0;
extern int GameOption_577B00;
extern int CurrentVoiceTheme;
extern unsigned GameOptionsBitfield;
extern bool GameOption_577B08;
extern bool GameOption_577B0C;
extern bool SoleEnhancedDefense;
extern bool DebugLogTeams;

extern int ClientFPS;
extern int LastServerAIFrame;
extern int CommStatsSpeedScale;
extern int RecievedBytesSec;
extern int SentBytesSec;
extern int SentTCP;
extern int SentUDP;
extern int RecievedTCP;
extern int RecievedUDP;
extern CountDownTimerClass CountDownTimerClass_590454;
extern CountDownTimerClass TransmisionStatsTimer;
extern CountDownTimerClass ServerCountDownTimerClass_5721D4;
extern char SoleHost[40];

extern int Density;
extern int CrateDensity;

class ProtocolClass;
class ListenerClass;
extern ProtocolClass* ListenerProtocol;
extern ListenerClass* Listener;

class ReliableProtocolClass;
class ReliableCommClass;
extern DynamicVectorClass<ReliableProtocolClass*> ReliableProtocols;
extern DynamicVectorClass<ReliableCommClass*> ReliableComms;
extern DynamicVectorClass<ReliableProtocolClass*> AdminProtocols;
extern DynamicVectorClass<ReliableCommClass*> AdminComms;

class SolePlayerClass;
extern DynamicVectorClass<SolePlayerClass*> SolePlayers;

#ifdef _WIN32
extern HINSTANCE hWSockInstance;
#endif

#endif
