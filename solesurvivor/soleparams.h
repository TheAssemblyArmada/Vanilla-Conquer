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

#ifndef SOLEPARAMS_H
#define SOLEPARAMS_H

#include <stdint.h>

#pragma pack(push, 1)
struct GameParamStruct
{
    uint32_t TimeLimit;
    uint32_t ScoreLimit;
    uint32_t LifeLimit;
    uint32_t CaptureTheFlag;
    uint32_t Football;
    uint32_t FootballNumFlags;
    uint32_t NumCTFStructures;
    uint32_t UnkInt;
    uint32_t AIUnitsPer10min;
    uint32_t MaxAIUnits;
    uint32_t AIBuildingsPer10min;
    uint32_t MaxAIBuildings;
    uint32_t IsMaxNumAIsScaled;
    uint32_t ResetTeamsInCTF;
    uint32_t AllowFlagSitting;
    uint32_t HealthBars;
    uint32_t FreeRadarForAll;
    uint32_t LosePowerups;
    uint32_t MinPlayers;
    uint32_t IonCannon;
    uint32_t TeamCrates;
    uint32_t SuperSeconds;
    uint32_t ArmageddonTimer;
    uint32_t UnkBool : 1;
    uint32_t NoReshroud : 1;
    uint32_t IsLadder : 1;
    uint32_t IsCrates : 1;
    uint32_t Steel;
    uint32_t Green;
    uint32_t Orange;
    uint32_t IsSquadChannel;
    uint32_t PasswordCountdownSeconds;
    uint32_t NumTeams;
    uint32_t PlayersPerTeam;
    uint32_t AllowNoTeam;
    uint32_t AllowPickTeam;
    char ChannelName[80];
    uint32_t CrateDensityOverride;
    uint32_t IsAutoTeaming;
    uint32_t SuperInvuln;
};
#pragma pack(pop)

extern GameParamStruct GameParams;

void Read_Server_INI_Params(GameParamStruct* params);

#endif