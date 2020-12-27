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

class SystemTimerClass
{
public:
    long operator()() const;
    operator long() const;

private:
    static unsigned long long Start;

    static unsigned long long Now();
};

class FrameTimerClass
{
public:
    long operator()() const;
    operator long() const;
};

/**********************************************************************
**	This class is solely used as a parameter to a constructor that does
**	absolutely no initialization to the object being constructed. By using
**	this method, it is possible to load and save data directly from a
**	class that has virtual functions. The construction process automatically
**	takes care of initializing the virtual function table pointer and the
**	rest of the constructor doesn't initialize any data members. After loading
**	into a class object, simply perform an in-place new operation.
*/
#ifndef NOINITCLASS
#define NOINITCLASS
struct NoInitClass
{
public:
    void operator()(void) const {};
};
#endif

/*
**    This is a timer class that watches a constant rate timer (specified by the parameter
**    type class) and provides basic timer functionality. It is possible to set the start value
**    WITHOUT damaging or otherwise affecting any other timer that may be built upon the same
**    specified timer class object. Treat an object of this type as if it were a "magic" integral
**    long that automatically advances at the speed of the timer class object controlling it.
*/
template <class T> class BasicTimerClass
{
public:
    // Constructor allows assignment as if class was integral 'long' type.
    BasicTimerClass(unsigned long set = 0)
        : Started(Timer() - set)
    {
    }

    BasicTimerClass(NoInitClass const&)
        : Started(Timer())
    {
    }

    // Fetch current value of timer.
    virtual unsigned long Value() const
    {
        return Timer() - Started;
    }

    // Conversion operator to allow consistent treatment with integral types.
    virtual operator unsigned long() const
    {
        return Value();
    }

    // Function operator to allow timer object definition to be cascaded.
    virtual unsigned long operator()() const
    {
        return Value();
    }

protected:
    T Timer;               // Timer regulator (ticks at constant rate).
    unsigned long Started; // Time started.
};

/*
**    This timer class functions similarly to the basic timer class. In addition to the
**    normal timer operation, this class has the ability to be stopped and started at
**    will. If you have no need to start or stop the timer, then use the basic timer
**    class instead.
*/
template <class T> class TTimerClass : public BasicTimerClass<T>
{
    using BasicTimerClass<T>::Started;

public:
    // Constructor allows assignment as if class was integral 'long' type.
    TTimerClass(unsigned long set = 0)
        : BasicTimerClass<T>(set)
        , Accumulated(0)
    {
    }

    TTimerClass(NoInitClass const& x)
        : BasicTimerClass<T>(x)
        , Accumulated(0)
    {
    }

    // This routine will return with the current value of the timer. It takes into account
    // whether the timer has stopped or not so as to always return the correct value regardless
    // of that condition.
    virtual unsigned long Value() const
    {
        unsigned long value = Accumulated;
        if (Started != 0xFFFFFFFFU) {
            value += BasicTimerClass<T>::Value();
        }
        return value;
    }

    // Stops (pauses) the timer.
    void Stop()
    {
        if (Started != 0xFFFFFFFFU) {
            Accumulated += BasicTimerClass<T>::operator unsigned long();
            Started = 0xFFFFFFFFU;
        }
    }

    // Starts (resumes) the timer.
    void Start()
    {
        if (Started == 0xFFFFFFFFU) {
            Started = this->Timer();
        }
    }

    // Queries whether the timer is currently active.
    bool Is_Active() const
    {
        return (Started != 0xFFFFFFFFU);
    }

    void Set(unsigned long set, bool start = true)
    {
        Started = this->Timer();
        Accumulated = set;
        if (start) {
            Start();
        }
    }

private:
    unsigned long Accumulated; // Total accumulated ticks.
};

/*
**    This timer counts down from the specified (or constructed) value down towards zero.
**    The countdown rate is controlled by the timer object specified. This timer object can
**    be started or stopped. It can also be tested to see if it has expired or not. An expired
**    count down timer is one that has value of zero. You can treat this class object as if it
**    were an integral "magic" long that automatically counts down toward zero.
*/
template <class T> class CDTimerClass : public BasicTimerClass<T>
{
    using BasicTimerClass<T>::Started;

public:
    // Constructor allows assignment as if class was integral 'long' type.
    CDTimerClass(unsigned long set = 0)
        : BasicTimerClass<T>(0)
        , DelayTime(set)
        , WasStarted(false)
    {
    }

    CDTimerClass(NoInitClass const& x)
        : BasicTimerClass<T>(x)
        , DelayTime(0)
        , WasStarted(false)
    {
    }

    // Fetches current value of count down timer.
    virtual unsigned long Value() const
    {
        unsigned long remain = DelayTime;
        if (Started != 0xFFFFFFFFU) {
            unsigned long value = BasicTimerClass<T>::Value();
            if (value < remain) {
                return (remain - value);
            } else {
                return 0;
            }
        }
        return remain;
    }

    // Stops (pauses) the timer.
    void Stop()
    {
        if (Started != 0xFFFFFFFFU) {
            DelayTime = *this;
            Started = 0xFFFFFFFFU;
        }
    }

    // Starts (resumes) the timer.
    void Start()
    {
        WasStarted = true;
        if (Started == 0xFFFFFFFFU) {
            Started = this->Timer();
        }
    }

    // Queries whether the timer is currently active.
    bool Is_Active() const
    {
        return (Started != 0xFFFFFFFFU);
    }

    bool Was_Started() const
    {
        return WasStarted;
    }

    void Clear()
    {
        Started = 0xFFFFFFFFU;
        DelayTime = 0;
    }

    void Set(unsigned long set, bool start = true)
    {
        Started = this->Timer();
        DelayTime = set;
        if (start) {
            Start();
        }
    }

    bool Expired()
    {
        return (Value() == 0);
    }

private:
    unsigned long DelayTime; // Ticks remaining before countdown timer expires.
    bool WasStarted;
};

// Ugly stubs only for "don't touch anything"

typedef enum BaseTimerEnum
{
    BT_SYSTEM, // System timer (60 / second).
} BaseTimerEnum;

class TimerClass : public TTimerClass<SystemTimerClass>
{
public:
    TimerClass(BaseTimerEnum timer = BT_SYSTEM, bool start = false)
        : TTimerClass<SystemTimerClass>()
    {
        if (start) {
            Start();
        }
    }

    unsigned long Time() const
    {
        return Value();
    }
};

class TCountDownTimerClass : public CDTimerClass<FrameTimerClass>
{
public:
    TCountDownTimerClass(long set = 0)
        : CDTimerClass<FrameTimerClass>(set)
    {
    }

    unsigned long Time() const
    {
        return Value();
    }
};

class CountDownTimerClass : public CDTimerClass<SystemTimerClass>
{
public:
    CountDownTimerClass(BaseTimerEnum timer, long set, bool on = false)
        : CDTimerClass<SystemTimerClass>(set)
    {
        if (on) {
            Start();
        }
    }

    CountDownTimerClass(BaseTimerEnum timer = BT_SYSTEM, bool on = false)
        : CDTimerClass<SystemTimerClass>(0)
    {
        if (on) {
            Start();
        }
    }

    unsigned long Time() const
    {
        return Value();
    }
};

class WinTimerClass
{
public:
    static void Init(int freq)
    {
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// externs  //////////////////////////////////////////
extern TimerClass WinTickCount;
extern CDTimerClass<SystemTimerClass> CountDown;
extern bool TimerSystemOn;

#endif // TIMER_H
