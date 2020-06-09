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
;* strtrim -- Remove the trailing white space from a string.               *
;*                                                                         *
;*    Use this routine to remove white space characters from the beginning *
;*    and end of the string.        The string is modified in place by     *
;*    this routine.                                                        *
;*                                                                         *
;* INPUT:   buffer   -- Pointer to the string to modify.                   *
;*                                                                         *
;* OUTPUT:     none                                                        *
;*                                                                         *
;* WARNINGS:   none                                                        *
;*                                                                         *
;* HISTORY:                                                                *
;*   10/07/1992 JLB : Created.                                             *
;*=========================================================================*
; VOID cdecl strtrim(BYTE *buffer);
    global C	strtrim :NEAR
    PROC	strtrim C near
    USES	ax, edi, esi

    ARG	buffer:DWORD		; Pointer to string to modify.
*/
void __cdecl strtrim(char* buffer)
{
    __asm {		  
			cmp	[buffer],0
			je	short fini

			; Prepare for string scanning by loading pointers.
			cld
			mov	esi,[buffer]
			mov	edi,esi

			; Strip white space from the start of the string.
		looper:
			lodsb
			cmp	al,20h			; Space
			je	short looper
			cmp	al,9			; TAB
			je	short looper
			stosb

			; Copy the rest of the string.
		gruntloop:
			lodsb
			stosb
			or	al,al
			jnz	short gruntloop
			dec	edi
			; Strip the white space from the end of the string.
		looper2:
			mov	[edi],al
			dec	edi
			mov	ah,[edi]
			cmp	ah,20h
			je	short looper2
			cmp	ah,9
			je	short looper2

		fini:
			//ret
	}
}

/*
;***************************************************************************
;* Fat_Put_Pixel -- Draws a fat pixel.                                     *
;*                                                                         *
;*    Use this routine to draw a "pixel" that is bigger than 1 pixel       *
;*    across.  This routine is faster than drawing a similar small shape   *
;*    and faster than calling Fill_Rect.                                   *
;*                                                                         *
;* INPUT:   x,y       -- Screen coordinates to draw the pixel's upper      *
;*                       left corner.                                      *
;*                                                                         *
;*          color     -- The color to render the pixel in.                 *
;*                                                                         *
;*          size      -- The number of pixels width of the big "pixel".    *
;*                                                                         *
;*          page      -- The pointer to a GraphicBuffer class or something *
;*                                                                         *
;* OUTPUT:  none                                                           *
;*                                                                         *
;* WARNINGS:   none                                                        *
;*                                                                         *
;* HISTORY:                                                                *
;*   03/17/1994 JLB : Created.                                             *
;*=========================================================================*
; VOID cdecl Fat_Put_Pixel(long x, long y, long color, long size, void *page)
    global C	Fat_Put_Pixel:NEAR
    PROC	Fat_Put_Pixel C near
    USES	eax, ebx, ecx, edx, edi, esi

    ARG	x:DWORD		; X coordinate of upper left pixel corner.
    ARG	y:DWORD		; Y coordinate of upper left pixel corner.
    ARG	color:DWORD	; Color to use for the "pixel".
    ARG	siz:DWORD	; Size of "pixel" to plot (square).
    ARG	gpage:DWORD	; graphic page address to plot onto
*/

void __cdecl Fat_Put_Pixel(int x, int y, int color, int siz, GraphicViewPortClass& gpage)
{
    __asm {
				  
			cmp	[siz],0
			je	short exit_label

			; Set EDI to point to start of logical page memory.
			;*===================================================================
			; Get the viewport information and put bytes per row in ecx
			;*===================================================================
			mov	ebx,[gpage]				; get a pointer to viewport
			mov	edi,[ebx]GraphicViewPortClass.Offset	; get the correct offset

			; Verify the the Y pixel offset is legal.
			mov	eax,[y]
			cmp	eax,[ebx]GraphicViewPortClass.Height	;YPIXEL_MAX
			jae	short exit_label
			mov	ecx,[ebx]GraphicViewPortClass.Width
			add	ecx,[ebx]GraphicViewPortClass.XAdd
			add	ecx,[ebx]GraphicViewPortClass.Pitch
			mul	ecx
			add	edi,eax

			; Verify the the X pixel offset is legal.
	
			mov	edx,[ebx]GraphicViewPortClass.Width
			cmp	edx,[x]
			mov	edx,ecx
			jbe	short exit_label
			add	edi,[x]

			; Write the pixel to the screen.
			mov	ebx,[siz]		; Copy of pixel size.
			sub	edx,ebx			; Modulo to reach start of next row.
			mov	eax,[color]
		again:
			mov	ecx,ebx
			rep stosb
			add	edi,edx			; EDI points to start of next row.
			dec	[siz]
			jnz	short again

		exit_label:
            // ret
    }
}

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
/*
extern "C" long __cdecl Calculate_CRC(void *buffer, long length)
{
    unsigned long crc;

    unsigned long local_length = (unsigned long) length;

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
            //ret
    }
}


*/

extern "C" void __cdecl Set_Palette_Range(void* palette)
{
    if (palette == NULL) {
        return;
    }

    memcpy(CurrentPalette, palette, 768);
    Set_DD_Palette(palette);
}