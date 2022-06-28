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

/* $Header: /CounterStrike/SCENARIO.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SCENARIO.H                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 02/26/96                                                     *
 *                                                                                             *
 *                  Last Update : February 26, 1996 [JLB]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef SCENARIO_H
#define SCENARIO_H

/*
**	This class holds the information about the current game being played. This information is
**	global to the scenario and is generally of a similar nature to the information that was held
**	in the controlling scenario INI file. It is safe to write this structure out as a whole since
**	it doesn't contain any embedded pointers.
*/
class ScenarioClass
{
public:
    // Constructor.
    ScenarioClass(void);

    /*
    **	This is the source of the random numbers used in the game. This controls
    **	the game logic and thus must be in sync with any networked machines.
    */
    RandomClass RandomNumber;

    /*
    **	This is the difficulty setting of the game.
    */
    DiffType Difficulty;  // For human player.
    DiffType CDifficulty; // For computer players.

    /*
    **	This is an array of waypoints; each waypoint corresponds to a letter of
    ** the alphabet, and points to a cell number.  -1 means unassigned.
    ** The CellClass has a bit that tells if that cell has a waypoint attached to
    ** it; the only way to find which waypoint it is, is to scan this array.  This
    ** shouldn't be needed often; usually, you know the waypoint & you want the CELL.
    */
    CELL Waypoint[WAYPT_COUNT];

    /*
    **	The scenario number.
    */
    int Scenario;

    /*
    **	The full name of the scenario (as it exists on disk).
    */
    char ScenarioName[_MAX_FNAME + _MAX_EXT];

    /*
    **	This is the full text of the briefing. This text will be
    **	displayed when the player commands the "restate mission
    **	objectives" operation.
    */
    char BriefingText[512];

    /*
    **	This is the theme to start playing at the beginning of the action
    **	movie. A score started in this fashion will continue to play as
    **	the game progresses.
    */
    ThemeType TransitTheme;

    /*
    **	The percentage of money that is allowed to be carried over into the
    **	following scenario.
    */
    int CarryOverPercent;

    /*
    **	This is the amount of money that was left over in the previous
    **	scenario.
    */
    int CarryOverMoney;

    /*
    **	This specifies the maximum amount of money that is allowed to be
    **	carried over from the previous scenario. This limits the amount
    **	regardless of what the carry over percentage is set to.
    */
    int CarryOverCap;

    /*
    **	This records the bookmark view locations the player has recorded.
    */
    CELL Views[4];
};

#endif
