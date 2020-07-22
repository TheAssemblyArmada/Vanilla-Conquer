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
 *                 Project Name : 32 bit library                           *
 *                                                                         *
 *                    File Name : MISC.H                                   *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : August 3, 1994                           *
 *                                                                         *
 *                  Last Update : August 3, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef DDRAW_H
#define DDRAW_H

#include "video.h"

#include <windows.h>
#include <ddraw.h>

extern LPDIRECTDRAWSURFACE PaletteSurface;

/*
 * Definition of surface monitor class
 *
 * This class keeps track of all the graphic buffers we generate in video memory so they
 *  can be restored after a focus switch.
 */

#define MAX_SURFACES 20

class SurfaceMonitorClassDDraw : public SurfaceMonitorClass
{

public:
    SurfaceMonitorClassDDraw();

    void Add_DD_Surface(LPDIRECTDRAWSURFACE);
    void Remove_DD_Surface(LPDIRECTDRAWSURFACE);
    bool Got_Surface_Already(LPDIRECTDRAWSURFACE);

    virtual void Restore_Surfaces();
    virtual void Set_Surface_Focus(bool in_focus);
    virtual void Release();

private:
    LPDIRECTDRAWSURFACE Surface[MAX_SURFACES];
    BOOL InFocus;
};

extern SurfaceMonitorClassDDraw AllSurfacesDDraw; // List of all direct draw surfaces

/*=========================================================================*/
/* The following variables are declared in: DDRAW.CPP                      */
/*=========================================================================*/
extern LPDIRECTDRAW DirectDrawObject;
extern LPDIRECTDRAW2 DirectDraw2Interface;

/*=========================================================================*/
/* The following prototypes are for the file: DDRAW.CPP                    */
/*=========================================================================*/
void Process_DD_Result(HRESULT result, int display_ok_msg);

#endif // DDRAW_H
