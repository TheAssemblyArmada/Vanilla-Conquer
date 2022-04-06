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

/* $Header:   F:\projects\c&c\vcs\code\compat.h_v   2.19   16 Oct 1995 16:46:02   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : COMPAT.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 03/02/95                                                     *
 *                                                                                             *
 *                  Last Update : March 2, 1995 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef COMPAT_H
#define COMPAT_H

#define movmem(a, b, c) memmove(b, a, c)
extern int ShapeBufferSize;
extern char* ShapeBuffer;

/*=========================================================================*/
/* Define some Graphic Routines which will only be fixed by these defines	*/
/*=========================================================================*/
#define Set_Font_Palette(a) Set_Font_Palette_Range(a, 0, 15)

extern unsigned char* Palette;

/*
**	This is the menu control structures.
*/
typedef enum MenuIndexType
{
    MENUX,
    MENUY,
    ITEMWIDTH,
    ITEMSHIGH,
    MSELECTED,
    NORMCOL,
    HILITE,
    MENUPADDING = 0x1000
} MenuIndexType;

/* These defines handle the various names given to the same color. */
#define DKGREEN GREEN
#define DKBLUE  BLUE
#define GRAY    GREY
#define DKGREY  GREY
#define DKGRAY  GREY
#define LTGRAY  LTGREY

#endif
