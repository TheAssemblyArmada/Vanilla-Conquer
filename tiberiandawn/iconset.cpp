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
 *                 Project Name : Library                                  *
 *                                                                         *
 *                    File Name : ICONSET.C                                *
 *                                                                         *
 *                   Programmer : Joe L. Bostic                            *
 *                                                                         *
 *                   Start Date : June 9, 1991                             *
 *                                                                         *
 *                  Last Update : September 15, 1993   [JLB]               *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Load_Icon_Set -- Loads an icons set and initializes it.               *
 *   Free_Icon_Set -- Frees allocations made by Load_Icon_Set().           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//#include	"function.h"

//#define		_WIN32
//#define		WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <stdio.h>
#include "common/wwstd.h"
#include "common/file.h"
#include "tile.h"
#include "common/iff.h"

// Misc? ST - 1/3/2019 10:40AM
// extern int Misc;
int Misc;

long Get_Icon_Set_Size(void const* iconset);
int Get_Icon_Set_Width(void const* iconset);
int Get_Icon_Set_Height(void const* iconset);
void* Get_Icon_Set_Icondata(void const* iconset);
void* Get_Icon_Set_Trans(void const* iconset);
void* Get_Icon_Set_Remapdata(void const* iconset);
void* Get_Icon_Set_Palettedata(void const* iconset);
int Get_Icon_Set_Count(void const* iconset);
void* Get_Icon_Set_Map(void const* iconset);

//#define	ICON_PALETTE_BYTES	16
//#define	ICON_MAX					256

/***************************************************************************
**	The terrain is rendered by using icons.  These are the buffers that hold
**	the icon data, remap tables, and remap index arrays.
*/
// PRIVATE char *IconPalette = NULL;		// MCGA only.
// PRIVATE char *IconRemap = NULL;			// MCGA only.

long Get_Icon_Set_Size(void const* iconset)
{
    IControl_Type* icontrol;
    int size = 0;

    icontrol = (IControl_Type*)iconset;
    if (icontrol) {
        size = icontrol->Size;
    }
    return (size);
}

int Get_Icon_Set_Width(void const* iconset)
{
    IControl_Type* icontrol;
    int width = 0;

    icontrol = (IControl_Type*)iconset;
    if (icontrol) {
        width = icontrol->Width;
    }
    return (width);
}

int Get_Icon_Set_Height(void const* iconset)
{
    IControl_Type* icontrol;
    int height = 0;

    icontrol = (IControl_Type*)iconset;
    if (icontrol) {
        height = icontrol->Height;
    }
    return (height);
}

void* Get_Icon_Set_Icondata(void const* iconset)
{
    IControl_Type* icontrol;
    icontrol = (IControl_Type*)iconset;
    if (icontrol)
        return (Add_Long_To_Pointer(iconset, (long)icontrol->Icons));
    return (NULL);
}

void* Get_Icon_Set_Trans(void const* iconset)
{
    IControl_Type* icontrol;
    void* ptr = NULL;

    icontrol = (IControl_Type*)iconset;
    if (icontrol) {
        ptr = Add_Long_To_Pointer((void*)iconset, icontrol->TransFlag);
    }
    return (ptr);
}

int Get_Icon_Set_Count(void const* iconset)
{
    IControl_Type* icontrol;
    int count;

    icontrol = (IControl_Type*)iconset;
    if (icontrol) {
        count = icontrol->Count;
    }
    return (count);
}

void* Get_Icon_Set_Map(void const* iconset)
{
    char* icontrol = (char*)iconset;
    if (icontrol) {
        uint32_t icontrol_map;
        memcpy(&icontrol_map, icontrol + offsetof(IControl_Type, Map), sizeof(uint32_t));
        return icontrol + icontrol_map;
    }
    return (NULL);
}