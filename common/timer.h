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
 *                 Project Name : Timer Class Functions                    *
 *                                                                         *
 *                    File Name : TIMER.H                                  *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : July 6, 1994                             *
 *                                                                         *
 *                  Last Update : July 12, 1994   [SKB]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef TIMER_H
#define TIMER_H

/*=========================================================================*/
/* The following prototypes are for the file: TIMERA.ASM                   */
/*=========================================================================*/

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Externs /////////////////////////////////////////////
extern bool TimerSystemOn;
extern int InTimerCallback; // true if we are currently in a callback

/*=========================================================================*/
typedef enum BaseTimerEnum
{
    BT_SYSTEM, // System timer (60 / second).
    BT_USER    // User controllable timer (? / second).
} BaseTimerEnum;

class TimerClass
{
public:
    // Constructor.  Timers set before low level init has been done will not
    // be able to be 'Started' or 'on' until timer system is in place.
    TimerClass(BaseTimerEnum timer = BT_SYSTEM, bool start = false);

    // No destructor.
    ~TimerClass(void)
    {
    }

    //
    int Set(int value, bool start = true); // Set initial timer value.
    int Stop(void);                        // Pause timer.
    int Start(void);                       // Resume timer.
    int Reset(bool start = true);          // Reset timer to zero.
    int Time(void);                        // Fetch current timer value.

protected:
    int Started;     // Time last started (0 == not paused).
    int Accumulated; //	Total accumulated ticks.

private:
    //		long (*Get_Ticks)(void);	// System timer fetch.
    BaseTimerEnum TickType;
    int Get_Ticks(void);
};

inline int TimerClass::Reset(bool start)
{
    return (Set(0, start));
}

class CountDownTimerClass : private TimerClass
{
public:
    // Constructor.  Timers set before low level init has been done will not
    // be able to be 'Started' or 'on' until timer system is in place.
    CountDownTimerClass(BaseTimerEnum timer, int set, bool on = false);
    CountDownTimerClass(BaseTimerEnum timer = BT_SYSTEM, bool on = false);

    // No destructor.
    ~CountDownTimerClass(void)
    {
    }

    // Public functions
    int Set(int set, bool start = true); // Set count down value.
    int Reset(bool start = true);        // Reset timer to zero.
    int Stop(void);                      // Pause timer.
    int Start(void);                     // Resume timer.
    int Time(void);                      // Fetch current count down value.

protected:
    int DelayTime; // Ticks remaining before countdown timer expires.
};

inline int CountDownTimerClass::Stop(void)
{
    TimerClass::Stop();
    return (Time());
}

inline int CountDownTimerClass::Start(void)
{
    TimerClass::Start();
    return (Time());
}

inline int CountDownTimerClass::Reset(bool start)
{
    return (TimerClass::Reset(start));
}

class SystemTimerClass
{
public:
    int operator()() const;
    operator int() const;
};

/*
**	Timer objects that fetch the appropriate timer value according to
**	the type of timer they are.
*/
extern int Frame;
class FrameTimerClass
{
public:
    int operator()(void) const
    {
        return (Frame);
    }

    operator int(void) const
    {
        return (Frame);
    }
};

class WinTimerClass
{
public:
    WinTimerClass();
    WinTimerClass(unsigned int freq = 60);
    ~WinTimerClass();

    static void Init(unsigned int freq = 60);

    unsigned int Get_System_Tick_Count();
    unsigned int Get_User_Tick_Count();

private:
    unsigned int Frequency; // Frequency of our windows timer in ticks per second
    unsigned long long Start;

    static unsigned long long Now();
};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// externs  //////////////////////////////////////////
extern TimerClass WinTickCount;
extern CountDownTimerClass CountDown;

#endif // TIMER_H
