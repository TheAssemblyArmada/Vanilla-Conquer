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

/* $Header: /CounterStrike/RECT.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : RECT.H                                                       *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 07/21/96                                                     *
 *                                                                                             *
 *                  Last Update : July 21, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef RECT_H
#define RECT_H

#include "cliprect.h"

class Rect
{
public:
    Rect(int x = 0, int y = 0, int w = 0, int h = 0);

    Rect const Intersect(const Rect& rectangle, int* x = nullptr, int* y = nullptr) const;
    friend Rect const Union(const Rect& rect1, const Rect& rect2);

    bool Is_Valid(void) const;
    int Size(void) const
    {
        return (Width * Height);
    }

    //	private:
    int X;
    int Y;
    int Width;
    int Height;

    friend inline bool operator==(const Rect& lhs, const Rect& rhs);
    friend inline bool operator!=(const Rect& lhs, const Rect& rhs);
};

inline bool operator==(const Rect& lhs, const Rect& rhs)
{
    return (lhs.X == rhs.X) && (lhs.Y == rhs.Y) && (lhs.Width == rhs.Width) && (lhs.Height == rhs.Height);
}

inline bool operator!=(const Rect& lhs, const Rect& rhs)
{
    return !(lhs == rhs);
}

#endif
