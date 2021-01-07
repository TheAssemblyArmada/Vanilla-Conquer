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

/* $Header: /CounterStrike/JSHELL.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : JSHELL.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 03/13/95                                                     *
 *                                                                                             *
 *                  Last Update : March 13, 1995 [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef JSHELL_H
#define JSHELL_H

#include <assert.h>

struct NoInitClass;

/*
**	These templates allow enumeration types to have simple bitwise
**	arithmatic performed. The operators must be instatiated for the
**	enumerated types desired.
*/
template <class T> inline T operator++(T& a)
{
    a = (T)((int)a + (int)1);
    return (a);
}
template <class T> inline T operator++(T& a, int)
{
    T aa = a;
    a = (T)((int)a + (int)1);
    return (aa);
}
template <class T> inline T operator--(T& a)
{
    a = (T)((int)a - (int)1);
    return (a);
}
template <class T> inline T operator--(T& a, int)
{
    T aa = a;
    a = (T)((int)a - (int)1);
    return (aa);
}
template <class T> inline T operator|(T t1, T t2)
{
    return ((T)((int)t1 | (int)t2));
}
template <class T> inline T operator&(T t1, T t2)
{
    return ((T)((int)t1 & (int)t2));
}
template <class T> inline T operator~(T t1)
{
    return ((T)(~(int)t1));
}

#ifdef NOMINMAX
inline int min(int a, int b)
{
    return a < b ? a : b;
}

inline int max(int a, int b)
{
    return a > b ? a : b;
}
#endif

template <class T> inline void swap(T& value1, T& value2)
{
    T temp = value1;
    value1 = value2;
    value2 = temp;
}
int swap(int, int);
long swap(long, long);

template <class T> inline T Bound(T original, T minval, T maxval)
{
    if (original < minval)
        return (minval);
    if (original > maxval)
        return (maxval);
    return (original);
};
int Bound(signed int, signed int, signed int);
unsigned Bound(unsigned, unsigned, unsigned);
long Bound(long, long, long);

template <class T> T _rotl(T X, int n)
{
    return ((T)((((X) << n) | ((X) >> ((sizeof(T) * 8) - n)))));
}

/*
**	This macro serves as a general way to determine the number of elements
**	within an array.
*/
#define ARRAY_LENGTH(x) int(sizeof(x) / sizeof(x[0]))
#define ARRAY_SIZE(x)   int(sizeof(x) / sizeof(x[0]))

/*
**	The shape flags are likely to be "or"ed together and other such bitwise
**	manipulations. These instatiated operator templates allow this.
*/
inline ShapeFlags_Type operator|(ShapeFlags_Type, ShapeFlags_Type);
inline ShapeFlags_Type operator&(ShapeFlags_Type, ShapeFlags_Type);
inline ShapeFlags_Type operator~(ShapeFlags_Type);

#include "common/miscasm.h"

template <class T> void Bubble_Sort(T* array, int count)
{
    if (array != NULL && count > 1) {
        bool swapflag;

        do {
            swapflag = false;
            for (int index = 0; index < count - 1; index++) {
                if (array[index] > array[index + 1]) {
                    T temp = array[index];
                    array[index] = array[index + 1];
                    array[index + 1] = temp;
                    swapflag = true;
                }
            }
        } while (swapflag);
    }
}

template <class T> void PBubble_Sort(T* array, int count)
{
    if (array != NULL && count > 1) {
        bool swapflag;

        do {
            swapflag = false;
            for (int index = 0; index < count - 1; index++) {
                if (*array[index] > *array[index + 1]) {
                    T temp = array[index];
                    array[index] = array[index + 1];
                    array[index + 1] = temp;
                    swapflag = true;
                }
            }
        } while (swapflag);
    }
}

template <class T> void PNBubble_Sort(T* array, int count)
{
    if (array != NULL && count > 1) {
        bool swapflag;

        do {
            swapflag = false;
            for (int index = 0; index < count - 1; index++) {
                if (stricmp(array[index]->Name(), array[index + 1]->Name()) > 0) {
                    T temp = array[index];
                    array[index] = array[index + 1];
                    array[index + 1] = temp;
                    swapflag = true;
                }
            }
        } while (swapflag);
    }
}

template <class T> class SmartPtr
{
public:
    SmartPtr(NoInitClass const&)
    {
    }
    SmartPtr(T* realptr = 0)
        : Pointer(realptr)
    {
    }
    SmartPtr(SmartPtr const& rvalue)
        : Pointer(rvalue.Pointer)
    {
    }
    ~SmartPtr(void)
    {
        Pointer = 0;
    }

    operator T*(void) const
    {
        return (Pointer);
    }

    operator long(void) const
    {
        return ((long)Pointer);
    }

    SmartPtr<T> operator++(int)
    {
        assert(Pointer != 0);
        SmartPtr<T> temp = *this;
        ++Pointer;
        return (temp);
    }
    SmartPtr<T>& operator++(void)
    {
        assert(Pointer != 0);
        ++Pointer;
        return (*this);
    }
    SmartPtr<T> operator--(int)
    {
        assert(Pointer != 0);
        SmartPtr<T> temp = *this;
        --Pointer;
        return (temp);
    }
    SmartPtr<T>& operator--(void)
    {
        assert(Pointer != 0);
        --Pointer;
        return (*this);
    }

    SmartPtr& operator=(SmartPtr const& rvalue)
    {
        Pointer = rvalue.Pointer;
        return (*this);
    }
    T* operator->(void) const
    {
        assert(Pointer != 0);
        return (Pointer);
    }
    T& operator*(void) const
    {
        assert(Pointer != 0);
        return (*Pointer);
    }

private:
    T* Pointer;
};

#endif
