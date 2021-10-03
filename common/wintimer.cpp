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

// Implementation of system timers.

#include "timer.h"
#include <chrono>
using namespace std::chrono;

// Global timers that the library or user can count on existing.
TimerClass WinTickCount(BT_SYSTEM);
WinTimerClass WindowsTimer(60);

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Code ////////////////////////////////////////

/***************************************************************************
 * WinTimerClass::WinTimerClass -- Initialize the WW timer system.         *
 *                                                                         *
 *                                                                         *
 * INPUT: unsigned int : user timer frequency.                             *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/5/95 3:47PM : ST Created.                                          *
 *=========================================================================*/
WinTimerClass::WinTimerClass()
    : Frequency(60)
    , Start(Now())
{
    WinTickCount.Start();
    TimerSystemOn = true;
}

WinTimerClass::WinTimerClass(unsigned int freq)
    : Frequency(freq)
    , Start(Now())
{
    WinTickCount.Start();
    TimerSystemOn = true;
}

/***************************************************************************
 * WinTimerClass::~WinTimerClass -- Removes the timer system.              *
 *                                                                         *
 *                                                                         *
 * INPUT:   NONE.                                                          *
 *                                                                         *
 * OUTPUT:  bool was it removed successfuly                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/5/95 3:47PM : ST Created.                                          *
 *=========================================================================*/
WinTimerClass::~WinTimerClass()
{
}

void WinTimerClass::Init(unsigned int freq)
{
    WindowsTimer = WinTimerClass(freq);
}

/***********************************************************************************************
 * WinTimerClass::Get_System_Tick_Count -- returns the system tick count                       *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   tick count                                                                        *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/5/95 4:02PM ST : Created                                                              *
 *=============================================================================================*/

unsigned int WinTimerClass::Get_System_Tick_Count()
{
    if (Frequency == 0) {
        return 0;
    }
    unsigned long long delta = Now() - Start;
    return (unsigned int)(delta / (1000 / Frequency));
}

/***********************************************************************************************
 * WinTimerClass::Get_User_Tick_Count -- returns the user tick count                           *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   tick count                                                                        *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/5/95 4:02PM ST : Created                                                              *
 *=============================================================================================*/

unsigned int WinTimerClass::Get_User_Tick_Count()
{
    if (Frequency == 0) {
        return 0;
    }
    unsigned long long delta = Now() - Start;
    return (unsigned int)(delta / (1000 / Frequency));
}

unsigned long long WinTimerClass::Now()
{
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
