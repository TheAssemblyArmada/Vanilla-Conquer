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
#include "ccini.h"

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
    , AttackDelay(5)
    , PowerEmergencyFraction(3, 4)
    , AirstripRatio(".12")
    , AirstripLimit(5)
    , HelipadRatio(".12")
    , HelipadLimit(5)
    , TeslaRatio(".16")
    , TeslaLimit(10)
    , AARatio(".14")
    , AALimit(10)
    , DefenseRatio(".5")
    , DefenseLimit(40)
    , WarRatio(".1")
    , WarLimit(2)
    , BarracksRatio(".16")
    , BarracksLimit(2)
    , RefineryLimit(4)
    , RefineryRatio(".16")
    , BaseSizeAdd(3)
    , PowerSurplus(50)
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
    , IsComputerParanoid(false)
    , IsCompEasyBonus(false)
    , IsFineDifficulty(false)
    , IsMCVDeploy(false)
    , IsSeparate(false)
    , IsTreeTarget(false)
    , IsTGrowth(true)
    , IsTSpread(true)
    , IsNamed(false)
    , IsSmartDefense(false)
    , IsScatter(false)
    , AllowSuperWeapons(true)
    , IsThreePoint(false)
    , IsBoxing(false)
{
#ifndef REMASTER_BUILD
    Diff[DIFF_EASY].FirepowerBias = "1.2";
    Diff[DIFF_EASY].GroundspeedBias = "1.2";
    Diff[DIFF_EASY].AirspeedBias = "1.2";
    Diff[DIFF_EASY].ArmorBias = "0.3";
    Diff[DIFF_EASY].ROFBias = "0.8";
    Diff[DIFF_EASY].CostBias = "0.8";
    Diff[DIFF_EASY].BuildSpeedBias = "0.6";
    Diff[DIFF_EASY].RepairDelay = "0.001";
    Diff[DIFF_EASY].BuildDelay = "0.001";
    Diff[DIFF_EASY].IsBuildSlowdown = false;
    Diff[DIFF_EASY].IsWallDestroyer = true;
    Diff[DIFF_EASY].IsContentScan = true;

    Diff[DIFF_NORMAL].FirepowerBias = 1;
    Diff[DIFF_NORMAL].GroundspeedBias = 1;
    Diff[DIFF_NORMAL].AirspeedBias = 1;
    Diff[DIFF_NORMAL].ArmorBias = 1;
    Diff[DIFF_NORMAL].ROFBias = 1;
    Diff[DIFF_NORMAL].CostBias = 1;
    Diff[DIFF_NORMAL].BuildSpeedBias = 1;
    Diff[DIFF_NORMAL].RepairDelay = "0.02";
    Diff[DIFF_NORMAL].BuildDelay = "0.03";
    Diff[DIFF_NORMAL].IsBuildSlowdown = true;
    Diff[DIFF_NORMAL].IsWallDestroyer = true;
    Diff[DIFF_NORMAL].IsContentScan = true;

    Diff[DIFF_HARD].FirepowerBias = "0.9";
    Diff[DIFF_HARD].GroundspeedBias = "0.9";
    Diff[DIFF_HARD].AirspeedBias = "0.9";
    Diff[DIFF_HARD].ArmorBias = "1.05";
    Diff[DIFF_HARD].ROFBias = "1.05";
    Diff[DIFF_HARD].CostBias = 1;
    Diff[DIFF_HARD].BuildSpeedBias = 1;
    Diff[DIFF_HARD].RepairDelay = "0.05";
    Diff[DIFF_HARD].BuildDelay = "0.1";
    Diff[DIFF_HARD].IsBuildSlowdown = true;
    Diff[DIFF_HARD].IsWallDestroyer = true;
    Diff[DIFF_HARD].IsContentScan = true;
#endif
}

/***********************************************************************************************
 * Difficulty_Get -- Fetch the difficulty bias values.                                         *
 *                                                                                             *
 *    This will fetch the difficulty bias values for the section specified.                    *
 *                                                                                             *
 * INPUT:   ini   -- Reference the INI database to fetch the values from.                      *
 *                                                                                             *
 *          diff  -- Reference to the difficulty class object to fill in with the values.      *
 *                                                                                             *
 *          section  -- The section identifier to lift the values from.                        *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/11/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Difficulty_Get(CCINIClass& ini, DifficultyClass& diff, char const* section)
{
    if (ini.Is_Present(section)) {
        diff.FirepowerBias = ini.Get_Fixed(section, "FirePower", diff.FirepowerBias);
        diff.GroundspeedBias = ini.Get_Fixed(section, "Groundspeed", diff.GroundspeedBias);
        diff.AirspeedBias = ini.Get_Fixed(section, "Airspeed", diff.AirspeedBias);
        diff.ArmorBias = ini.Get_Fixed(section, "Armor", diff.ArmorBias);
        diff.ROFBias = ini.Get_Fixed(section, "ROF", diff.ROFBias);
        diff.CostBias = ini.Get_Fixed(section, "Cost", diff.CostBias);
        diff.RepairDelay = ini.Get_Fixed(section, "RepairDelay", diff.RepairDelay);
        diff.BuildDelay = ini.Get_Fixed(section, "BuildDelay", diff.BuildDelay);
        diff.IsBuildSlowdown = ini.Get_Bool(section, "BuildSlowdown", diff.IsBuildSlowdown);
        diff.BuildSpeedBias = ini.Get_Fixed(section, "BuildTime", diff.BuildSpeedBias);
        diff.IsWallDestroyer = ini.Get_Bool(section, "DestroyWalls", diff.IsWallDestroyer);
        diff.IsContentScan = ini.Get_Bool(section, "ContentScan", diff.IsContentScan);
    }
}

/***********************************************************************************************
 * Difficulty_Put -- Fetch the difficulty bias values.                                         *
 *                                                                                             *
 *    This will fetch the difficulty bias values for the section specified.                    *
 *                                                                                             *
 * INPUT:   ini   -- Reference the INI database to fetch the values from.                      *
 *                                                                                             *
 *          diff  -- Reference to the difficulty class object to fill in with the values.      *
 *                                                                                             *
 *          section  -- The section identifier to lift the values from.                        *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/11/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Difficulty_Put(CCINIClass& ini, DifficultyClass& diff, char const* section)
{
    ini.Put_Fixed(section, "FirePower", diff.FirepowerBias);
    ini.Put_Fixed(section, "Groundspeed", diff.GroundspeedBias);
    ini.Put_Fixed(section, "Airspeed", diff.AirspeedBias);
    ini.Put_Fixed(section, "Armor", diff.ArmorBias);
    ini.Put_Fixed(section, "ROF", diff.ROFBias);
    ini.Put_Fixed(section, "Cost", diff.CostBias);
    ini.Put_Fixed(section, "RepairDelay", diff.RepairDelay);
    ini.Put_Fixed(section, "BuildDelay", diff.BuildDelay);
    ini.Put_Bool(section, "BuildSlowdown", diff.IsBuildSlowdown);
    ini.Put_Fixed(section, "BuildTime", diff.BuildSpeedBias);
    ini.Put_Bool(section, "DestroyWalls", diff.IsWallDestroyer);
    ini.Put_Bool(section, "ContentScan", diff.IsContentScan);
}

/***********************************************************************************************
 * RulesClass::Process -- Fetch the bulk of the rule data from the control file.               *
 *                                                                                             *
 *    This routine will fetch the rule data from the control file.                             *
 *                                                                                             *
 * INPUT:   file  -- Reference to the rule file to process.                                    *
 *                                                                                             *
 * OUTPUT:  bool; Was the rule file processed?                                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/17/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::Process(CCINIClass& ini)
{
    General(ini);
    AI(ini);
    IQ(ini);
    Difficulty(ini);

    return (true);
}

/***********************************************************************************************
 * RulesClass::Process -- Fetch the bulk of the rule data from the control file.               *
 *                                                                                             *
 *    This routine will fetch the rule data from the control file.                             *
 *                                                                                             *
 * INPUT:   file  -- Reference to the rule file to process.                                    *
 *                                                                                             *
 * OUTPUT:  bool; Was the rule file processed?                                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/17/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::Export(CCINIClass& ini)
{
    Export_General(ini);
    Export_AI(ini);
    Export_IQ(ini);
    Export_Difficulty(ini);

    return (true);
}

/***********************************************************************************************
 * RulesClass::General -- Process the general main game rules.                                 *
 *                                                                                             *
 *    This fetches the control constants uses for regular game processing. Any game behavior   *
 *    controlling values that don't properly fit in any of the other catagories will be        *
 *    stored here.                                                                             *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the database to fetch the values from.                       *
 *                                                                                             *
 * OUTPUT:  bool; Was the general section found and processed?                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::General(CCINIClass& ini)
{
    static char const* const GENERAL = "General";

    if (ini.Is_Present(GENERAL)) {
        IsFineDifficulty = ini.Get_Bool(GENERAL, "FineDiffControl", IsFineDifficulty);
        IsScatter = ini.Get_Bool(GENERAL, "PlayerScatter", IsScatter);
        IsSmartDefense = ini.Get_Bool(GENERAL, "PlayerReturnFire", IsSmartDefense);
        IsNamed = ini.Get_Bool(GENERAL, "NamedCivilians", IsNamed);
        IsTGrowth = ini.Get_Bool(GENERAL, "TiberiumGrows", IsTGrowth);
        IsTSpread = ini.Get_Bool(GENERAL, "TiberiumSpreads", IsTSpread);
        IsTreeTarget = ini.Get_Bool(GENERAL, "TreeTargeting", IsTreeTarget);
        IsSeparate = ini.Get_Bool(GENERAL, "SeparateAircraft", IsSeparate);
        IsMCVDeploy = ini.Get_Bool(GENERAL, "MCVUndeploy", IsMCVDeploy);
        IsThreePoint = ini.Get_Bool(GENERAL, "ThreePointTurns", IsThreePoint);
        IsBoxing = ini.Get_Bool(GENERAL, "HandToHand", IsBoxing);

        return (true);
    }

    return (false);
}

/***********************************************************************************************
 * RulesClass::General -- Process the general main game rules.                                 *
 *                                                                                             *
 *    This fetches the control constants uses for regular game processing. Any game behavior   *
 *    controlling values that don't properly fit in any of the other catagories will be        *
 *    stored here.                                                                             *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the database to fetch the values from.                       *
 *                                                                                             *
 * OUTPUT:  bool; Was the general section found and processed?                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::Export_General(CCINIClass& ini)
{
    static char const GENERAL[] = "General";

    ini.Put_Bool(GENERAL, "FineDiffControl", IsFineDifficulty);
    ini.Put_Bool(GENERAL, "PlayerScatter", IsScatter);
    ini.Put_Bool(GENERAL, "PlayerReturnFire", IsSmartDefense);
    ini.Put_Bool(GENERAL, "NamedCivilians", IsNamed);
    ini.Put_Bool(GENERAL, "TiberiumGrows", IsTGrowth);
    ini.Put_Bool(GENERAL, "TiberiumSpreads", IsTSpread);
    ini.Put_Bool(GENERAL, "TreeTargeting", IsTreeTarget);
    ini.Put_Bool(GENERAL, "SeparateAircraft", IsSeparate);
    ini.Put_Bool(GENERAL, "MCVUndeploy", IsMCVDeploy);
    ini.Put_Bool(GENERAL, "ThreePointTurns", IsThreePoint);
    ini.Put_Bool(GENERAL, "HandToHand", IsBoxing);

    return (true);
}

/***********************************************************************************************
 * RulesClass::AI -- Processes the AI control constants from the database.                     *
 *                                                                                             *
 *    This will examine the database specified and set the AI override values accordingly.     *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that holds the AI overrides.                *
 *                                                                                             *
 * OUTPUT:  bool; Was the AI section found and processed?                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::AI(CCINIClass& ini)
{
    static const char AI[] = "AI";

    if (ini.Is_Present(AI)) {
        AttackInterval = ini.Get_Fixed(AI, "AttackInterval", AttackInterval);
        AttackDelay = ini.Get_Fixed(AI, "AttackDelay", AttackDelay);
        InfantryReserve = ini.Get_Int(AI, "InfantryReserve", InfantryReserve);
        InfantryBaseMult = ini.Get_Int(AI, "InfantryBaseMult", InfantryBaseMult);
        PowerSurplus = ini.Get_Int(AI, "PowerSurplus", PowerSurplus);
        BaseSizeAdd = ini.Get_Int(AI, "BaseSizeAdd", BaseSizeAdd);
        RefineryRatio = ini.Get_Fixed(AI, "RefineryRatio", RefineryRatio);
        RefineryLimit = ini.Get_Int(AI, "RefineryLimit", RefineryLimit);
        BarracksRatio = ini.Get_Fixed(AI, "BarracksRatio", BarracksRatio);
        BarracksLimit = ini.Get_Int(AI, "BarracksLimit", BarracksLimit);
        WarRatio = ini.Get_Fixed(AI, "WarRatio", WarRatio);
        WarLimit = ini.Get_Int(AI, "WarLimit", WarLimit);
        DefenseRatio = ini.Get_Fixed(AI, "DefenseRatio", DefenseRatio);
        DefenseLimit = ini.Get_Int(AI, "DefenseLimit", DefenseLimit);
        AARatio = ini.Get_Fixed(AI, "AARatio", AARatio);
        AALimit = ini.Get_Int(AI, "AALimit", AALimit);
        TeslaRatio = ini.Get_Fixed(AI, "ObeliskRatio", TeslaRatio);
        TeslaLimit = ini.Get_Int(AI, "ObeliskLimit", TeslaLimit);
        HelipadRatio = ini.Get_Fixed(AI, "HelipadRatio", HelipadRatio);
        HelipadLimit = ini.Get_Int(AI, "HelipadLimit", HelipadLimit);
        AirstripRatio = ini.Get_Fixed(AI, "AirstripRatio", AirstripRatio);
        AirstripLimit = ini.Get_Int(AI, "AirstripLimit", AirstripLimit);
        IsCompEasyBonus = ini.Get_Bool(AI, "CompEasyBonus", IsCompEasyBonus);
        IsComputerParanoid = ini.Get_Bool(AI, "Paranoid", IsComputerParanoid);
        PowerEmergencyFraction = ini.Get_Fixed(AI, "PowerEmergency", PowerEmergencyFraction);
        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * RulesClass::Export_AI -- Processes the AI control constants to the database.                *
 *                                                                                             *
 *    This will examine the database specified and set the AI override values accordingly.     *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that holds the AI overrides.                *
 *                                                                                             *
 * OUTPUT:  bool; Was the AI section found and processed?                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::Export_AI(CCINIClass& ini)
{
    static const char AI[] = "AI";

    ini.Put_Fixed(AI, "AttackInterval", AttackInterval);
    ini.Put_Fixed(AI, "AttackDelay", AttackDelay);
    ini.Put_Int(AI, "InfantryReserve", InfantryReserve);
    ini.Put_Int(AI, "InfantryBaseMult", InfantryBaseMult);
    ini.Put_Int(AI, "PowerSurplus", PowerSurplus);
    ini.Put_Int(AI, "BaseSizeAdd", BaseSizeAdd);
    ini.Put_Fixed(AI, "RefineryRatio", RefineryRatio);
    ini.Put_Int(AI, "RefineryLimit", RefineryLimit);
    ini.Put_Fixed(AI, "BarracksRatio", BarracksRatio);
    ini.Put_Int(AI, "BarracksLimit", BarracksLimit);
    ini.Put_Fixed(AI, "WarRatio", WarRatio);
    ini.Put_Int(AI, "WarLimit", WarLimit);
    ini.Put_Fixed(AI, "DefenseRatio", DefenseRatio);
    ini.Put_Int(AI, "DefenseLimit", DefenseLimit);
    ini.Put_Fixed(AI, "AARatio", AARatio);
    ini.Put_Int(AI, "AALimit", AALimit);
    ini.Put_Fixed(AI, "ObeliskRatio", TeslaRatio);
    ini.Put_Int(AI, "ObeliskLimit", TeslaLimit);
    ini.Put_Fixed(AI, "HelipadRatio", HelipadRatio);
    ini.Put_Int(AI, "HelipadLimit", HelipadLimit);
    ini.Put_Fixed(AI, "AirstripRatio", AirstripRatio);
    ini.Put_Int(AI, "AirstripLimit", AirstripLimit);
    ini.Put_Bool(AI, "CompEasyBonus", IsCompEasyBonus);
    ini.Put_Bool(AI, "Paranoid", IsComputerParanoid);
    ini.Put_Fixed(AI, "PowerEmergency", PowerEmergencyFraction);
    return (true);
}

/***********************************************************************************************
 * RulesClass::IQ -- Fetches the IQ control values from the INI database.                      *
 *                                                                                             *
 *    This will scan the database specified and retrieve the IQ control values from it. These  *
 *    IQ control values are what gives the IQ rating meaning. It fundimentally controls how    *
 *    the computer behaves.                                                                    *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to read the IQ controls from.               *
 *                                                                                             *
 * OUTPUT:  bool; Was the IQ section found and processed?                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/11/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::IQ(CCINIClass& ini)
{
    static const char IQCONTROL[] = "IQ";

    if (ini.Is_Present(IQCONTROL)) {
        MaxIQ = ini.Get_Int(IQCONTROL, "MaxIQLevels", MaxIQ);
        IQSuperWeapons = ini.Get_Int(IQCONTROL, "SuperWeapons", IQSuperWeapons);
        IQProduction = ini.Get_Int(IQCONTROL, "Production", IQProduction);
        IQGuardArea = ini.Get_Int(IQCONTROL, "GuardArea", IQGuardArea);
        IQRepairSell = ini.Get_Int(IQCONTROL, "RepairSell", IQRepairSell);
        IQCrush = ini.Get_Int(IQCONTROL, "AutoCrush", IQCrush);
        IQScatter = ini.Get_Int(IQCONTROL, "Scatter", IQScatter);
        IQContentScan = ini.Get_Int(IQCONTROL, "ContentScan", IQContentScan);
        IQAircraft = ini.Get_Int(IQCONTROL, "Aircraft", IQAircraft);
        IQHarvester = ini.Get_Int(IQCONTROL, "Harvester", IQHarvester);
        IQSellBack = ini.Get_Int(IQCONTROL, "SellBack", IQSellBack);

        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * RulesClass::Export_IQ -- Exports the IQ control values from the INI database.               *
 *                                                                                             *
 *    This will scan the database specified and retrieve the IQ control values from it. These  *
 *    IQ control values are what gives the IQ rating meaning. It fundimentally controls how    *
 *    the computer behaves.                                                                    *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to read the IQ controls from.               *
 *                                                                                             *
 * OUTPUT:  bool; Was the IQ section found and processed?                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/11/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::Export_IQ(CCINIClass& ini)
{
    static const char IQCONTROL[] = "IQ";

    ini.Put_Int(IQCONTROL, "MaxIQLevels", MaxIQ);
    ini.Put_Int(IQCONTROL, "SuperWeapons", IQSuperWeapons);
    ini.Put_Int(IQCONTROL, "Production", IQProduction);
    ini.Put_Int(IQCONTROL, "GuardArea", IQGuardArea);
    ini.Put_Int(IQCONTROL, "RepairSell", IQRepairSell);
    ini.Put_Int(IQCONTROL, "AutoCrush", IQCrush);
    ini.Put_Int(IQCONTROL, "Scatter", IQScatter);
    ini.Put_Int(IQCONTROL, "ContentScan", IQContentScan);
    ini.Put_Int(IQCONTROL, "Aircraft", IQAircraft);
    ini.Put_Int(IQCONTROL, "Harvester", IQHarvester);
    ini.Put_Int(IQCONTROL, "SellBack", IQSellBack);

    return (true);
}

/***********************************************************************************************
 * RulesClass::Difficulty -- Fetch the various difficulty group settings.                      *
 *                                                                                             *
 *    This routine is used to fetch the various group settings for the difficulty levels.      *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that has the difficulty setting values.     *
 *                                                                                             *
 * OUTPUT:  bool; Was the difficulty section found and processed.                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/10/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::Difficulty(CCINIClass& ini)
{
#ifndef REMASTER_BUILD
    Difficulty_Get(ini, Diff[DIFF_EASY], "Easy");
    Difficulty_Get(ini, Diff[DIFF_NORMAL], "Normal");
    Difficulty_Get(ini, Diff[DIFF_HARD], "Difficult");
#endif
    return (true);
}

/***********************************************************************************************
 * RulesClass::Export_Difficulty -- Export the various difficulty group settings.              *
 *                                                                                             *
 *    This routine is used to fetch the various group settings for the difficulty levels.      *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that has the difficulty setting values.     *
 *                                                                                             *
 * OUTPUT:  bool; Was the difficulty section found and processed.                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/10/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RulesClass::Export_Difficulty(CCINIClass& ini)
{
#ifndef REMASTER_BUILD
    Difficulty_Put(ini, Diff[DIFF_EASY], "Easy");
    Difficulty_Put(ini, Diff[DIFF_NORMAL], "Normal");
    Difficulty_Put(ini, Diff[DIFF_HARD], "Difficult");
#endif
    return (true);
}

/***********************************************************************************************
 * Is_MCV_Deploy -- Check if MCV can be deployed.                                              *
 *                                                                                             *
 *    This routine is used to check if the Construction Yard can revert back into an MCV.      *
 *    It allows the special variables to override anything set by the rules.                   *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Can the Construction Yard revert back into an MCV.                           *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/24/2019 SKY : Created.                                                                 *
 *=============================================================================================*/
bool Is_MCV_Deploy()
{
    return Special.UseMCVDeploy ? Special.IsMCVDeploy : Rule.IsMCVDeploy;
}