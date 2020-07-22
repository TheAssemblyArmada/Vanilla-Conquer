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
 *                 Project Name : Westwood Win32 Library                   *
 *                                                                         *
 *                    File Name : DDRAW.CPP                                *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : October 10, 1995                         *
 *                                                                         *
 *                  Last Update : October 10, 1995   []                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

#include "misc.h"
#include "gbuffer.h"
#include "palette.h"
#include <stdio.h>
#include <ddraw.h>
#include "ddraw.h"

extern void Check_Overlapped_Blit_Capability();
extern PALETTEENTRY PaletteEntries[256]; // 256 windows palette entries
extern LPDIRECTDRAWPALETTE PalettePtr;   // Pointer to direct draw palette object

/***********************************************************************************************
 * Set_Video_Mode -- Initializes Direct Draw and sets the required Video Mode                  *
 *                                                                                             *
 * INPUT:           int width           - the width of the video mode in pixels                *
 *                  int height          - the height of the video mode in pixels               *
 *                  int bits_per_pixel  - the number of bits per pixel the video mode supports *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
BOOL Set_Video_Mode(HWND hwnd, int w, int h, int bits_per_pixel)
{
    HRESULT result;
    //
    // If there is not currently a direct draw object then we need to define one.
    //
    if (DirectDrawObject == NULL) {
        result = DirectDrawCreate(NULL, &DirectDrawObject, NULL);
        Process_DD_Result(result, FALSE);
        if (result == DD_OK) {
            if (w == 320) {
                result =
                    DirectDrawObject->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX);
            } else {
                result = DirectDrawObject->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
            }
            Process_DD_Result(result, FALSE);
        } else {
            return (FALSE);
        }
    }

    //
    // Set the required display mode with 8 bits per pixel
    //
    result = DirectDrawObject->SetDisplayMode(w, h, bits_per_pixel);
    if (result != DD_OK) {
        DirectDrawObject->Release();
        DirectDrawObject = NULL;
        return (FALSE);
    }

    //
    // Create a direct draw palette object
    //
    result = DirectDrawObject->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, &PaletteEntries[0], &PalettePtr, NULL);
    Process_DD_Result(result, FALSE);
    if (result != DD_OK) {
        return (FALSE);
    }

    Check_Overlapped_Blit_Capability();

    /*
    ** Find out if DirectX 2 extensions are available
    */
    result = DirectDrawObject->QueryInterface(IID_IDirectDraw2, (LPVOID*)&DirectDraw2Interface);
    SystemToVideoBlits = FALSE;
    VideoToSystemBlits = FALSE;
    SystemToSystemBlits = FALSE;
    if (result != DD_OK) {
        DirectDraw2Interface = NULL;
    } else {
        DDCAPS capabilities;
        DDCAPS emulated_capabilities;

        memset((char*)&capabilities, 0, sizeof(capabilities));
        memset((char*)&emulated_capabilities, 0, sizeof(emulated_capabilities));
        capabilities.dwSize = sizeof(capabilities);
        emulated_capabilities.dwSize = sizeof(emulated_capabilities);

        DirectDrawObject->GetCaps(&capabilities, &emulated_capabilities);

        if (capabilities.dwCaps & DDCAPS_CANBLTSYSMEM) {
            SystemToVideoBlits = (capabilities.dwSVBCaps & DDCAPS_BLT) ? TRUE : FALSE;
            VideoToSystemBlits = (capabilities.dwVSBCaps & DDCAPS_BLT) ? TRUE : FALSE;
            SystemToSystemBlits = (capabilities.dwSSBCaps & DDCAPS_BLT) ? TRUE : FALSE;
        }
    }

    return (TRUE);
}
