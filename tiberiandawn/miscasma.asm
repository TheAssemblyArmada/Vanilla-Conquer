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

externdef C Distance_Coord:near
externdef C Coord_Cell:near

.code

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


end
