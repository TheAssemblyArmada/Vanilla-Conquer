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

#if (0)
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

#endif

/*
;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Westwood Library                         *
;*                                                                         *
;*                    File Name : CRC.ASM                                  *
;*                                                                         *
;*                   Programmer : Joe L. Bostic                            *
;*                                                                         *
;*                   Start Date : June 12, 1992                            *
;*                                                                         *
;*                  Last Update : February 10, 1995 [BWG]                  *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *

IDEAL
P386
MODEL USE32 FLAT

GLOBAL	C Calculate_CRC	:NEAR

    CODESEG
*/

extern "C" long __cdecl Calculate_CRC(void* buffer, long length)
{
    unsigned long crc;

    unsigned long local_length = (unsigned long)length;

    __asm {
			; Load pointer to data block.
			mov	[crc],0
			pushad
			mov	esi,[buffer]
			cld

			; Clear CRC to default (NULL) value.
			xor	ebx,ebx

            //; Fetch the length of the data block to CRC.
			
			mov	ecx,[local_length]

			jecxz	short fini

			; Prepare the length counters.
			mov	edx,ecx
			and	dl,011b
			shr	ecx,2

			; Perform the bulk of the CRC scanning.
			jecxz	short remainder2
		accumloop:
			lodsd
			rol	ebx,1
			add	ebx,eax
			loop	accumloop

			; Handle the remainder bytes.
		remainder2:
			or	dl,dl
			jz	short fini
			mov	ecx,edx
			xor	eax,eax

			and 	ecx,0FFFFh
			push	ecx
		nextbyte:
			lodsb
			ror	eax,8
			loop	nextbyte
			pop	ecx
			neg	ecx
			add	ecx,4
			shl	ecx,3
			ror	eax,cl

		;nextbyte:
		;	shl	eax,8
		;	lodsb
		;	loop	nextbyte
			rol	ebx,1
			add	ebx,eax

		fini:
			mov	[crc],ebx
			popad
			mov	eax,[crc]
        // ret
    }
}

extern "C" void __cdecl Set_Palette_Range(void* palette)
{
    if (palette == NULL) {
        return;
    }

    memcpy(CurrentPalette, palette, 768);
    Set_DD_Palette(palette);
}