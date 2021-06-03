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
#include "soleparams.h"
#include "soleglobals.h"
#include "function.h"
#include "common/ini.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

GameParamStruct GameParams;

void Read_Server_INI_Params(GameParamStruct* params)
{
    if (GameToPlay == GAME_SERVER && !OfflineMode) {
        CCFileClass file("SERVER.INI");
        INIClass ini;
        ini.Load(file);

        params->TimeLimit = ini.Get_Int("GameParms", "TimeLimit", 0);
        params->ScoreLimit = ini.Get_Int("GameParms", "ScoreLimit", 0);
        params->LifeLimit = ini.Get_Int("GameParms", "LifeLimit", 0);
        params->CaptureTheFlag = ini.Get_Int("GameParms", "CaptureTheFlag", 0);
        params->NumCTFStructures = ini.Get_Int("GameParms", "NumCTFStructures", 4);
        if (params->NumCTFStructures >= 0) {
            if (params->NumCTFStructures > 20) {
                params->NumCTFStructures = 20;
            }
        } else {
            params->NumCTFStructures = 0;
        }
        params->AIUnitsPer10min = ini.Get_Int("GameParms", "AIUnitsPer10min", 3);
        params->MaxAIUnits = ini.Get_Int("GameParms", "MaxAIUnits", 5);
        params->AIBuildingsPer10min = ini.Get_Int("GameParms", "AIBuildingsPer10min", 3);
        params->MaxAIBuildings = ini.Get_Int("GameParms", "MaxAIBuildings", 5);
        params->IsMaxNumAIsScaled = ini.Get_Int("GameParms", "IsMaxNumAIsScaled", 0);
        params->NoReshroud = ini.Get_Int("GameParms", "NoReshroud", 0);
        params->IsLadder = ini.Get_Int("GameParms", "IsLadderGame", 1);
        params->IsCrates = ini.Get_Int("GameParms", "IsCrates", 1);
        params->ResetTeamsInCTF = ini.Get_Int("GameParms", "ResetTeamsInCTF", 1);
        params->AllowFlagSitting = ini.Get_Int("GameParms", "AllowFlagSitting", 1);
        params->HealthBars = ini.Get_Int("GameParms", "HealthBars", 0);
        if (params->HealthBars >= 1) {
            Special.HealthBarDisplayMode = SpecialClass::HB_DAMAGED;
        }
        if (params->HealthBars >= 2) {
            Special.HealthBarDisplayMode = SpecialClass::HB_ALWAYS;
        }
        params->FreeRadarForAll = ini.Get_Int("GameParms", "FreeRadarForAll", 0);
        if (!params->IsCrates) {
            params->FreeRadarForAll = 1;
        }
        params->LosePowerups = ini.Get_Int("GameParms", "LosePowerups", 1);
        params->Football = ini.Get_Int("GameParms", "Football", 0);
        if (params->CaptureTheFlag) {
            params->Football = 0;
        }
        params->FootballNumFlags = ini.Get_Int("GameParms", "FootballNumFlags", 0);
        if (params->FootballNumFlags >= 1) {
            if (params->FootballNumFlags > 2) {
                params->FootballNumFlags = 2;
            }
        } else {
            params->FootballNumFlags = 1;
        }
        params->MinPlayers = ini.Get_Int("GameParms", "MinPlayers", 2);
        params->IonCannon = ini.Get_Int("GameParms", "IonCannon", 1);
        params->TeamCrates = ini.Get_Int("GameParms", "TeamCrates", 1);
        params->SuperSeconds = ini.Get_Int("GameParms", "SuperSeconds", 30);
        params->ArmageddonTimer = ini.Get_Int("GameParms", "ArmageddonTimer", 800);
        params->SuperInvuln = ini.Get_Int("GameParms", "SuperInvuln", 1);
        Steel = ini.Get_Int("Crates", "Steel", 50);
        Green = ini.Get_Int("Crates", "Green", 100);
        Orange = ini.Get_Int("Crates", "Orange", 10);
        if (Steel < 0 || Green < 0 || Orange < 0 || Orange + Green + Steel > 1000) {
            Steel = 50;
            Green = 100;
            Orange = 10;
        }
        params->Steel = Steel;
        params->Green = Green;
        params->Orange = Orange;
        if (params->ArmageddonTimer < 100) {
            params->ArmageddonTimer = 100;
        }
        params->UnkBool = true;
        params->CrateDensityOverride = ini.Get_Int("GameParms", "CrateDensityOverride", 0);
        params->NumTeams = ini.Get_Int("GameParms", "NumTeams", 999);
        params->PlayersPerTeam = ini.Get_Int("GameParms", "PlayersPerTeam", 999);
        params->AllowNoTeam = ini.Get_Int("GameParms", "AllowNoTeam", 999);
        params->AllowPickTeam = ini.Get_Int("GameParms", "AllowPickTeam", 999);
        ini.Get_String("Login", "Channel", "Unknown", params->ChannelName, sizeof(params->ChannelName));
        params->IsSquadChannel = ini.Get_Int("GameParms", "IsSquadChannel", 0);
        params->PasswordCountdownSeconds = ini.Get_Int("GameParms", "PasswordCountdownSeconds", 30);
        params->IsAutoTeaming = ini.Get_Int("GameParms", "IsAutoTeaming", 0);
        if (params->IsAutoTeaming && params->NumTeams > 1) {
            params->AllowNoTeam = 1;
            params->AllowPickTeam = 0;
        } else {
            params->IsAutoTeaming = 0;
        }
        for (int i = 0; i < 25; ++i) {
            // TODO Having to cast away const here, need to fix that properly.
            char key_name[20];
            sprintf(key_name, "WEAPON_%02d_DAMAGE", i);
            ((WeaponTypeClass*)&Weapons[i])->Attack = ini.Get_Int("Weapons", key_name, 100);
            sprintf(key_name, "WEAPON_%02d_ROF", i);
            ((WeaponTypeClass*)&Weapons[i])->ROF = ini.Get_Int("Weapons", key_name, 100);
            sprintf(key_name, "WEAPON_%02d_RANGE", i);
            ((WeaponTypeClass*)&Weapons[i])->Range = ini.Get_Int("Weapons", key_name, 100);
        }
    }
}
