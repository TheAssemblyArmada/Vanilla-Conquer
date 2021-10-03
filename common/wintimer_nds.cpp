
#include "timer.h"
#include <stdio.h>
#include <nds.h>

// Global timers that the library or user can count on existing.
TimerClass WinTickCount(BT_SYSTEM);
WinTimerClass WindowsTimer(60);

// Implement a system timer for the Nintendo DS.
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

    if (freq != 60) {
        printf("Only 60FPS timers supported\n");
        while (1)
            ;
    }
}

WinTimerClass::~WinTimerClass()
{
}

void WinTimerClass::Init(unsigned int freq)
{
    if (freq != 60) {
        printf("Only 60FPS timers supported\n");
        while (1)
            ;
    }

    WindowsTimer = WinTimerClass(freq);
}

// Number of Ticks passed.
static unsigned Ticks;

// Update the number of ticks passed.  We use this to implement a 60FPS
// timer. Altough vanilla-conquer uses a milliseconds based timer, original
// game used a 60FPS timer.
void Timer_VBlank()
{
    Ticks++;
}

unsigned int WinTimerClass::Get_System_Tick_Count()
{
    return Ticks - Start;
}

unsigned int WinTimerClass::Get_User_Tick_Count()
{
    return Ticks - Start;
}

unsigned long long WinTimerClass::Now()
{
    return Ticks;
}
