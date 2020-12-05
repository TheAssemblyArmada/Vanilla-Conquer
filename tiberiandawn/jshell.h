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

/* $Header:   F:\projects\c&c\vcs\code\jshell.h_v   2.16   16 Oct 1995 16:45:06   JOE_BOSTIC  $ */
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

#ifdef NEVER
inline void* operator delete(void* data)
{
    Free(data);
}

inline void* operator delete[](void* data)
{
    Free(data);
}
#endif

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

#endif
