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

#include "FUNCTION.H"

extern "C" void __cdecl Mem_Copy(void const* source, void* dest, unsigned long bytes_to_copy)
{
    memcpy(dest, source, bytes_to_copy);
}

/*

CELL __cdecl Coord_Cell(COORDINATE coord)
{
    __asm {
        mov	eax, coord
        mov	ebx,eax
        shr	eax,010h
        xor	al,al
        shr	eax,2
        or		al,bh
    }

}



*/

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
void __cdecl Shake_Screen(int shakes)
{
    // PG_TO_FIX
    // Need a different solution for shaking the screen
    shakes;
}

/*
GLOBAL C Shake_Screen : NEAR

                            CODESEG

                        ;
***********************************************************;
SHAKE_SCREEN;
;
VOID Shake_Screen(int shakes);
;
;
This routine shakes the screen the number of times indicated.;
;
Bounds Checking : None;
;
*PROC Shake_Screen C near USES ecx,
    edx

        ARG shakes : DWORD ret

                         mov ecx,
                     [shakes]

    ;
;
;
push es;
;
;
mov ax, 40h;
;
;
mov es, ax;
;
;
mov dx, [es:63h];
;
;
pop es mov eax, [0463h];
get CRTC I / O port mov dx, ax add dl, 6;
video status port

    ? ? top_loop :

      ? ? start_retrace : in al,
    dx test al,
    8 jz ? ? start_retrace

    ? ? end_retrace : in al,
    dx test al,
    8 jnz ? ? end_retrace

        cli sub dl,
    6;
dx = 3B4H
     or 3D4H

     mov ah,
    01;
top word of start address mov al, 0Ch out dx, al xchg ah, al inc dx out dx, al xchg ah,
    al dec dx

        mov ah,
    040h;
bottom word = 40(140h)inc al out dx, al xchg ah, al inc dx out dx, al xchg ah,
       al

           sti add dl,
       5

    ? ? start_retrace2 : in al,
       dx test al,
       8 jz ? ? start_retrace2

    ? ? end_retrace2 : in al,
       dx test al,
       8 jnz ? ? end_retrace2

    ? ? start_retrace3 : in al,
       dx test al,
       8 jz ? ? start_retrace3

    ? ? end_retrace3 : in al,
       dx test al,
       8 jnz ? ? end_retrace3

           cli sub dl,
       6;
dx = 3B4H
     or 3D4H

     mov ah,
    0 mov al, 0Ch out dx, al xchg ah, al inc dx out dx, al xchg ah,
    al dec dx

        mov ah,
    0 inc al out dx, al xchg ah, al inc dx out dx, al xchg ah,
    al

        sti add dl,
    5

    loop
    ? ? top_loop

        ret

            ENDP Shake_Screen

    ;
***********************************************************

                                                          END
*/

extern "C" void __cdecl Set_Palette_Range(void* palette)
{
    if (palette == NULL) {
        return;
    }

    memcpy(CurrentPalette, palette, 768);
    Set_DD_Palette(palette);
}