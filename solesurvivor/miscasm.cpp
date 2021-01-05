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

/*
**
**   Misc. assembly code moved from headers
**
**
**
**
**
*/

#include "function.h"

void Mem_Copy(void const* source, void* dest, unsigned long bytes_to_copy)
{
    if (dest != source) {
        memcpy(dest, source, bytes_to_copy);
    }
}

/*
;***********************************************************
; SHAKE_SCREEN
;
; VOID Shake_Screen(int shakes);
;
; This routine shakes the screen the number of times indicated.
;
; Bounds Checking: None
;
;*
*/
void Shake_Screen(int shakes)
{
    // PG_TO_FIX
    // Need a different solution for shaking the screen
    shakes;
}

void Set_Palette_Range(void* palette)
{
    if (palette == NULL) {
        return;
    }

    memcpy(CurrentPalette, palette, 768);
    Set_DD_Palette(palette);
}