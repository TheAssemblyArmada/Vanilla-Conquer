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

#include "cliprect.h"

#ifdef NOASM

/**************************************************************************
* Clip_Rect -- clip a given rectangle against a given window              *
*                                                                         *
* INPUT:   &x , &y , &w , &h  -> Pointer to rectangle being clipped       *
*          width , height     -> dimension of clipping window             *
*                                                                         *
* OUTPUT: a) Zero if the rectangle is totally contained by the            *
*             clipping window.                                            *
*         b) A negative value if the rectangle is totally outside the     *
*             the clipping window                                         *
*         c) A positive value if the rectangle was clipped against the    *
*             clipping window, also the values pointed by x, y, w, h will *
*             be modified to new clipped values                           *
*                                                                         *
*   05/03/1995 JRJ : added comment                                        *
*========================================================================*/
int Clip_Rect(int* x, int* y, int* w, int* h, int width, int height)
{
    int xstart = *x;
    int ystart = *y;
    int yend = *y + *h - 1;
    int xend = *x + *w - 1;
    int result = 0;

    // If we aren't drawing within bounds, return -1
    if (xstart >= width || ystart >= height || xend < 0 || yend < 0) {
        return -1;
    }

    if (xstart < 0) {
        xstart = 0;
        result = 1;
    }

    if (ystart < 0) {
        ystart = 0;
        result = 1;
    }

    if (xend >= width) {
        xend = width - 1;
        result = 1;
    }

    if (yend >= height) {
        yend = height - 1;
        result = 1;
    }

    *x = xstart;
    *y = ystart;
    *w = xend - xstart + 1;
    *h = yend - ystart + 1;

    return result;
}

/**************************************************************************
* Confine_Rect -- clip a given rectangle against a given window           *
*                                                                         *
* INPUT:   &x,&y,w,h        -> Pointer to rectangle being clipped         *
*          width,height     -> dimension of clipping window               *
*                                                                         *
* OUTPUT: a) Zero if the rectangle is totally contained by the            *
*            clipping window.                                             *
*         c) A positive value if the rectangle was shifted in position    *
*            to fix inside the clipping window, also the values pointed   *
*            by x, y, will adjusted to a new values                       *
*                                                                         *
*  NOTE:  this function make not attempt to verify if the rectangle is    *
*       bigger than the clipping window and at the same time wrap around  *
*       it. If that is the case the result is meaningless                 *
*=========================================================================*/
int Confine_Rect(int* x, int* y, int w, int h, int width, int height)
{
    int confined = 0;

    if ((-*x & (*x + w - width - 1)) >= 0) {
        if (*x > 0) {
            *x -= *x + w - width;
        } else {
            *x = 0;
        }

        confined = 1;
    }

    if ((-*y & (*y + h - height - 1)) >= 0) {
        if (*y > 0) {
            *y -= *y + h - height;
        } else {
            *y = 0;
        }

        confined = 1;
    }

    return confined;
}

#endif  // NOASM
