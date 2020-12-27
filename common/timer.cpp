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
 *                    File Name : TIMER.CPP                                *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : July 6, 1994                             *
 *                                                                         *
 *                  Last Update : May 3, 1995   [SKB]                      *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "timer.h"

#include <chrono>

using namespace std::chrono;

// Global timers that the library or user can count on existing.
TimerClass WinTickCount;
CDTimerClass<SystemTimerClass> CountDown(false);
bool TimerSystemOn = true;
unsigned long long SystemTimerClass::Start = SystemTimerClass::Now();

long SystemTimerClass::operator()() const
{
    unsigned long long delta = Now() - Start;
    return (unsigned long)(delta / (1000 / 60));
}

SystemTimerClass::operator long() const
{
    return (*this)();
}

unsigned long long SystemTimerClass::Now()
{
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

extern long Frame;

long FrameTimerClass::operator()() const
{
    return Frame;
}

FrameTimerClass::operator long() const
{
    return (Frame);
}
