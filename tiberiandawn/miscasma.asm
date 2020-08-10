;
; Copyright 2020 Electronic Arts Inc.
;
; TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
; software: you can redistribute it and/or modify it under the terms of
; the GNU General Public License as published by the Free Software Foundation,
; either version 3 of the License, or (at your option) any later version.

; TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
; in the hope that it will be useful, but with permitted additional restrictions
; under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
; distributed with this program. You should have received a copy of the
; GNU General Public License along with permitted additional restrictions
; with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

; Misc. assembly code moved from headers

.model flat

IFNDEF NOASM
externdef C Distance_Coord:near
externdef C Coord_Cell:near
ENDIF
externdef C Fat_Put_Pixel:near

GraphicViewPort struct
    GVPOffset   DD ? ; offset to virtual viewport
    GVPWidth    DD ? ; width of virtual viewport
    GVPHeight   DD ? ; height of virtual viewport
    GVPXAdd     DD ? ; x mod to get to next line
    GVPXPos     DD ? ; x pos relative to Graphic Buff
    GVPYPos     DD ? ; y pos relative to Graphic Buff
    GVPPitch    DD ? ; modulo of graphic view port
    GVPBuffPtr  DD ? ; ptr to associated Graphic Buff
GraphicViewPort ends

.code

IFNDEF NOASM
;***********************************************************************************************
;* Distance -- Determines the lepton distance between two coordinates.                         *
;*                                                                                             *
;*    This routine is used to determine the distance between two coordinates. It uses the      *
;*    Dragon Strike method of distance determination and thus it is very fast.                 *
;*                                                                                             *
;* INPUT:   coord1   -- First coordinate.                                                      *
;*                                                                                             *
;*          coord2   -- Second coordinate.                                                     *
;*                                                                                             *
;* OUTPUT:  Returns the lepton distance between the two coordinates.                           *
;*                                                                                             *
;* WARNINGS:   none                                                                            *
;*                                                                                             *
;* HISTORY:                                                                                    *
;*   05/27/1994 JLB : Created.                                                                 *
;*=============================================================================================*/

;int Distance_Coord(COORDINATE coord1, COORDINATE coord2)
Distance_Coord proc C coord1:dword, coord2:dword
    push    ebx
    mov     eax,[coord1]
    mov     ebx,[coord2]
    mov     dx,ax
    sub     dx,bx
    jg      okx
    neg     dx
okx:
    shr     eax,16
    shr     ebx,16
    sub     ax,bx
    jg      oky
    neg     ax
oky:
    cmp     ax,dx
    jg      ok
    xchg    ax,dx
ok:
    shr     dx,1
    add     ax,dx
    pop     ebx
    ret
Distance_Coord endp

; CELL __cdecl Coord_Cell(COORDINATE coord)
Coord_Cell proc C coord:dword
    push ebx
    mov eax,coord
    mov ebx,eax
    shr eax,010h
    xor al,al
    shr eax,2
    or  al,bh
    pop ebx
    ret
Coord_Cell endp

ENDIF

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
;void __cdecl Fat_Put_Pixel(int x, int y, int color, int siz, GraphicViewPortClass& gpage)
Fat_Put_Pixel proc C x:dword, y:dword, color:dword, siz:dword, gpage:dword
    push    ebx
    push    edi

    cmp     [siz],0
    je      short exit_label

    ; Set EDI to point to start of logical page memory.
    ;*===================================================================
    ; Get the viewport information and put bytes per row in ecx
    ;*===================================================================
    mov     ebx,[gpage]                               ; get a pointer to viewport
    mov     edi,(GraphicViewPort ptr [ebx]).GVPOffset ; get the correct offset

    ; Verify the the Y pixel offset is legal.
    mov     eax,[y]
    cmp     eax,(GraphicViewPort ptr [ebx]).GVPHeight ; YPIXEL_MAX
    jae     short exit_label
    mov     ecx,(GraphicViewPort ptr [ebx]).GVPWidth
    add     ecx,(GraphicViewPort ptr [ebx]).GVPXAdd
    add     ecx,(GraphicViewPort ptr [ebx]).GVPPitch
    mul     ecx
    add     edi,eax

    ; Verify the the X pixel offset is legal.
    mov     edx,(GraphicViewPort ptr [ebx]).GVPWidth
    cmp     edx,[x]
    mov     edx,ecx
    jbe     short exit_label
    add     edi,[x]

    ; Write the pixel to the screen.
    mov     ebx,[siz]   ; Copy of pixel size.
    sub     edx,ebx     ; Modulo to reach start of next row.
    mov     eax,[color]
again:
    mov     ecx,ebx
    rep stosb
    add     edi,edx     ; EDI points to start of next row.
    dec     [siz]
    jnz     short again

exit_label:
    pop     edi
    pop     ebx
    ret
Fat_Put_Pixel endp

end
