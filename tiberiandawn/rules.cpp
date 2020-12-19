//
// Copyright 2020 Electronic Arts Inc.
//
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

/* $Header: /CounterStrike/RULES.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : RULES.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 05/12/96                                                     *
 *                                                                                             *
 *                  Last Update : September 10, 1996 [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   DifficultyClass::DifficultyClass -- Default constructor for difficulty class object.      *
 *   RulesClass::RulesClass -- Default constructor for rules class object.                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"

/***********************************************************************************************
 * DifficultyClass::DifficultyClass -- Default constructor for difficulty class object.        *
 *                                                                                             *
 *    This is the default constructor for the difficulty class object. Although it initializes *
 *    the rule data with default values, it is expected that they will all be overridden by    *
 *    the rules control file.                                                                  *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/18/2019 SKY : Created.                                                                 *
 *=============================================================================================*/
DifficultyClass::DifficultyClass(void)
    : FirepowerBias(1)
    , GroundspeedBias(1)
    , AirspeedBias(1)
    , ArmorBias(1)
    , ROFBias(1)
    , CostBias(1)
    , BuildSpeedBias(1)
    , RepairDelay(0.02f)
    , BuildDelay(0.03f)
    , IsBuildSlowdown(false)
    , IsWallDestroyer(true)
    , IsContentScan(false)
{
}

/***********************************************************************************************
 * RulesClass::RulesClass -- Default constructor for rules class object.                       *
 *                                                                                             *
 *    This is the default constructor for the rules class object. Although it initializes the  *
 *    rule data with default values, it is expected that they will all be overridden by the    *
 *    rules control file.                                                                      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/17/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
RulesClass::RulesClass(void)
    : AttackInterval(3)
    , BaseSizeAdd(3)
    , PowerSurplus(50)
    , AttackDelay(5)
    , PowerEmergencyFraction(0x00C0)
    , HelipadRatio(0x1E)
    , HelipadLimit(5)
    , AARatio(0x0024)
    , AALimit(10)
    , DefenseRatio(0x0066)
    , DefenseLimit(40)
    , WarRatio(0x0019)
    , WarLimit(2)
    , BarracksRatio(0x0028)
    , BarracksLimit(2)
    , RefineryLimit(4)
    , RefineryRatio(0x0028)
    , AirstripRatio(0x001E)
    , AirstripLimit(5)
    , MaxIQ(5)
    , IQSuperWeapons(4)
    , IQProduction(5)
    , IQGuardArea(4)
    , IQRepairSell(1)
    , IQCrush(2)
    , IQScatter(3)
    , IQContentScan(4)
    , IQAircraft(4)
    , IQHarvester(2)
    , IQSellBack(2)
    , InfantryReserve(3000)
    , InfantryBaseMult(1)
    , IsComputerParanoid(true)
    , IsCompEasyBonus(false)
    , IsFineDifficulty(false)
    , AllowSuperWeapons(true)
{
#ifndef REMASTER_BUILD
    Diff[DIFF_EASY].FirepowerBias = 1.2;
    Diff[DIFF_EASY].GroundspeedBias = 1.2;
    Diff[DIFF_EASY].AirspeedBias = 1.2;
    Diff[DIFF_EASY].ArmorBias = 0.3;
    Diff[DIFF_EASY].ROFBias = 0.8;
    Diff[DIFF_EASY].CostBias = 0.8;
    Diff[DIFF_EASY].BuildSpeedBias = 0.6;
    Diff[DIFF_EASY].RepairDelay = 0.001;
    Diff[DIFF_EASY].BuildDelay = 0.001;
    Diff[DIFF_EASY].IsBuildSlowdown = false;
    Diff[DIFF_EASY].IsWallDestroyer = true;
    Diff[DIFF_EASY].IsContentScan = true;

    Diff[DIFF_NORMAL].FirepowerBias = 1.0;
    Diff[DIFF_NORMAL].GroundspeedBias = 1.0;
    Diff[DIFF_NORMAL].AirspeedBias = 1.0;
    Diff[DIFF_NORMAL].ArmorBias = 1.0;
    Diff[DIFF_NORMAL].ROFBias = 1.0;
    Diff[DIFF_NORMAL].CostBias = 1.0;
    Diff[DIFF_NORMAL].BuildSpeedBias = 1.0;
    Diff[DIFF_NORMAL].RepairDelay = 0.02;
    Diff[DIFF_NORMAL].BuildDelay = 0.03;
    Diff[DIFF_NORMAL].IsBuildSlowdown = true;
    Diff[DIFF_NORMAL].IsWallDestroyer = true;
    Diff[DIFF_NORMAL].IsContentScan = true;

    Diff[DIFF_HARD].FirepowerBias = 0.9;
    Diff[DIFF_HARD].GroundspeedBias = 0.9;
    Diff[DIFF_HARD].AirspeedBias = 0.9;
    Diff[DIFF_HARD].ArmorBias = 1.05;
    Diff[DIFF_HARD].ROFBias = 1.05;
    Diff[DIFF_HARD].CostBias = 1.0;
    Diff[DIFF_HARD].BuildSpeedBias = 1.0;
    Diff[DIFF_HARD].RepairDelay = 0.05;
    Diff[DIFF_HARD].BuildDelay = 0.1;
    Diff[DIFF_HARD].IsBuildSlowdown = true;
    Diff[DIFF_HARD].IsWallDestroyer = true;
    Diff[DIFF_HARD].IsContentScan = true;
#endif
}
