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

/***************************************************************************
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Temp timer for 32bit lib                 *
 *                                                                         *
 *                    File Name : TIMERINI.CPP                             *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : July 6, 1994                             *
 *                                                                         *
 *                  Last Update : July 6, 1994   [SKB]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Init_Timer_System -- Initialize the WW timer system.                  *
 *   Remove_Timer_System -- Removes the timer system.                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "wwstd.h"
#include "timer.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Global Data /////////////////////////////////////

#ifdef _WIN32

#include <mmsystem.h>
HANDLE TimerThreadHandle = 0; // Handle of timer thread
// Prototype for timer callback
void CALLBACK Timer_Callback(UINT event_id, UINT res1, DWORD user, DWORD res2, DWORD res3);

int InTimerCallback = 0; // Flag to say if we are in a timer callback
#endif

bool TimerSystemOn = false;

// Global timers that the library or user can count on existing.
TimerClass WinTickCount(BT_SYSTEM);
CountDownTimerClass CountDown(BT_SYSTEM, false);

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Code ////////////////////////////////////////

#pragma warning(disable : 4996)

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
WinTimerClass::WinTimerClass(unsigned int freq, bool partial)
{
#ifdef _WIN32
    bool success;

    //
    // Inform windows that we want a higher than normal
    // timer resolution
    //
#ifdef __SW_EP
    timeBeginPeriod(1000 / PROFILE_RATE);
    Frequency = PROFILE_RATE;
#else
    timeBeginPeriod(1000 / freq);
    Frequency = freq;
#endif

    SysTicks = 0;
    UserTicks = 0;

    //
    // Install the timer callback event handler
    //
    // TimerHandle = timeSetEvent ( 1000/freq , 1 , Timer_Callback , 0 , TIME_PERIODIC);
    // Add TIME_KILL_SYNCHRONOUS flag. ST - 2/13/2019 5:07PM
    TimerHandle = timeSetEvent(1000 / freq, 1, Timer_Callback, 0, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    TimerSystemOn = success = (TimerHandle != 0);

    if (success) {
        if (!partial) {
            WindowsTimer = this;
            WinTickCount.Start();
        }
    } else {
        char error_str[128];
        sprintf(error_str, "Error - timer system failed to start. Error code %d\n", GetLastError());
        OutputDebugString(error_str);
    }
#endif
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
WinTimerClass::~WinTimerClass(void)
{
#ifdef _WIN32
    if (TimerHandle) {
        timeKillEvent(TimerHandle);
        TimerHandle = 0; // ST - 2/13/2019 5:12PM
    }

    TimerSystemOn = FALSE;
    timeEndPeriod(1000 / Frequency);
#endif
}

/***********************************************************************************************
 * Timer_Callback -- Main timer callback. Equivalent to a timer interrupt handler              *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    uint timer ID                                                                     *
 *           uint reserved                                                                     *
 *           long 0 (application defined)                                                      *
 *           long reserved                                                                     *
 *           long reserved                                                                     *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/5/95 3:19PM ST : Created                                                              *
 *=============================================================================================*/

#ifdef _WIN32
void CALLBACK Timer_Callback(UINT, UINT, DWORD, DWORD, DWORD)
{
    // CONTEXT	context;

    InTimerCallback++;

    // Removed. ST - 2/13/2019 5:11PM
    // if (!TimerThreadHandle){
    //	DuplicateHandle (GetCurrentProcess(), GetCurrentThread() , GetCurrentProcess() ,&TimerThreadHandle , 0 , TRUE ,
    //DUPLICATE_SAME_ACCESS);
    //}

    if (WindowsTimer) {
        WindowsTimer->Update_Tick_Count();
    }
    InTimerCallback--;
}
#endif

/***********************************************************************************************
 * WinTimerClass::Update_Tick_Count -- update westwood timers                                  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/5/95 3:58PM ST : Created                                                              *
 *=============================================================================================*/

void WinTimerClass::Update_Tick_Count(void)
{
    /*
     *
     *  Increment westwood timers
     *
     */
    SysTicks++;
    UserTicks++;
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

unsigned WinTimerClass::Get_System_Tick_Count(void)
{
    return (SysTicks);
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

unsigned WinTimerClass::Get_User_Tick_Count(void)
{
    return (UserTicks);
}