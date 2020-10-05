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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Part of the TILE Library                 *
 *                                                                         *
 *                    File Name : TILE.H                                   *
 *                                                                         *
 *                   Programmer : Barry W. Green                           *
 *                                                                         *
 *                   Start Date : February 2, 1995                         *
 *                                                                         *
 *                  Last Update : February 2, 1995 [BWG]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef TILE_H
#define TILE_H

/*=========================================================================*/
/* The following prototypes are for the file: ICONSET.CPP						*/
/*=========================================================================*/
void* Load_Icon_Set(char const* filename, void* iconsetptr, long buffsize);
void Free_Icon_Set(void const* iconset);
long Get_Icon_Set_Size(void const* iconset);
int Get_Icon_Set_Width(void const* iconset);
int Get_Icon_Set_Height(void const* iconset);
void* Get_Icon_Set_Icondata(void const* iconset);
void* Get_Icon_Set_Trans(void const* iconset);
void* Get_Icon_Set_Remapdata(void const* iconset);
void* Get_Icon_Set_Palettedata(void const* iconset);
int Get_Icon_Set_Count(void const* iconset);
void* Get_Icon_Set_Map(void const* iconset);

#if (1)
/*
** This is the control structure at the start of a loaded icon set.  It must match
** the structure in WWLIB.I!  This structure MUST be a multiple of 16 bytes long.
*/

// C&C version of struct
#pragma pack(push, 2)
typedef struct
{
    int16_t Width;     // Width of icons (pixels).
    int16_t Height;    // Height of icons (pixels).
    int16_t Count;     // Number of (logical) icons in this set.
    int16_t Allocated; // Was this iconset allocated?
    int32_t Size;      // Size of entire iconset memory block.
    int32_t Icons;     // Offset from buffer start to icon data.
    int32_t Palettes;  // Offset from buffer start to palette data.
    int32_t Remaps;    // Offset from buffer start to remap index data.
    int32_t TransFlag; // Offset for transparency flag table.
    int32_t Map;       // Icon map offset (if present).
} IControl_Type;
#pragma pack(pop)

#else

// RA version of struct
typedef struct
{
    short Width;     // Width of icons (pixels).
    short Height;    // Height of icons (pixels).
    short Count;     // Number of (logical) icons in this set.
    short Allocated; // Was this iconset allocated?
    short MapWidth;  // Width of map (in icons).
    short MapHeight; // Height of map (in icons).
    long Size;       // Size of entire iconset memory block.
    long Icons;      // Offset from buffer start to icon data.
    long Palettes;   // Offset from buffer start to palette data.
    long Remaps;     // Offset from buffer start to remap index data.
    long TransFlag;  // Offset for transparency flag table.
    long ColorMap;   // Offset for color control value table.
    long Map;        // Icon map offset (if present).
} IControl_Type;

#endif

#endif // TILE_H
