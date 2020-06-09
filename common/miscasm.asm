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

externdef C calcx:near
externdef C calcy:near
externdef C Cardinal_To_Fixed:near
externdef C Fixed_To_Cardinal:near
externdef C Desired_Facing256:near

.code

; int __cdecl calcx(signed short param1, short distance)
calcx proc C param1:word, distance:word
    push    ebx
    movzx   eax, [param1]
    mov     bx, [distance]
    imul    bx
    shl     ax,1
    rcl     dx,1
    mov     al,ah
    mov     ah,dl
    cwd
    pop     ebx
    ret
calcx endp

; int __cdecl calcy(signed short param1, short distance)
calcy proc C param1:word, distance:word
    push    ebx
    movzx   eax, [param1]
    mov     bx, [distance]
    imul    bx
    shl     ax,1
    rcl     dx,1
    mov     al,ah
    mov     ah,dl
    cwd
    neg     eax
    pop     ebx
    ret
calcy endp

;***********************************************************************************************
;* Cardinal_To_Fixed -- Converts cardinal numbers into a fixed point number.                   *
;*                                                                                             *
;*    This utility function will convert cardinal numbers into a fixed point fraction. The     *
;*    use of fixed point numbers occurs throughout the product -- since it is a convenient     *
;*    tool. The fixed point number is based on the formula:                                    *
;*                                                                                             *
;*       result = cardinal / base                                                              *
;*                                                                                             *
;*    The accuracy of the fixed point number is limited to 1/256 as the lowest and up to       *
;*    256 as the largest.                                                                      *
;*                                                                                             *
;* INPUT:   base     -- The key number to base the fraction about.                             *
;*                                                                                             *
;*          cardinal -- The other number (hey -- what do you call it?)                         *
;*                                                                                             *
;* OUTPUT:  Returns with the fixed point number of the "cardinal" parameter as it relates      *
;*          to the "base" parameter.                                                           *
;*                                                                                             *
;* WARNINGS:   none                                                                            *
;*                                                                                             *
;* HISTORY:                                                                                    *
;*   02/17/1995 BWG : Created.                                                                 *
;*=============================================================================================*/

; unsigned int __cdecl Cardinal_To_Fixed(unsigned base, unsigned cardinal)
Cardinal_To_Fixed proc C base:dword, cardinal:dword
    push    ebx
    mov     eax,0FFFFh      ; establish default return value
    mov     ebx,[base]
    or      ebx, ebx
    jz      retneg1         ; if base==0, return 65535
    mov     eax,[cardinal]  ; otherwise, return (cardinal*256)/base
    shl     eax,8
    xor     edx,edx
    div     ebx
retneg1:
    pop     ebx
    ret
Cardinal_To_Fixed endp


;***********************************************************************************************
;* Fixed_To_Cardinal -- Converts a fixed point number into a cardinal number.                  *
;*                                                                                             *
;*    Use this routine to convert a fixed point number into a cardinal number.                 *
;*                                                                                             *
;* INPUT:   base     -- The base number that the original fixed point number was created from. *
;*                                                                                             *
;*          fixed    -- The fixed point number to convert.                                     *
;*                                                                                             *
;* OUTPUT:  Returns with the reconverted number.                                               *
;*                                                                                             *
;* WARNINGS:   none                                                                            *
;*                                                                                             *
;* HISTORY:                                                                                    *
;*   02/17/1995 BWG : Created.                                                                 *
;*=============================================================================================*/

; unsigned int __cdecl Fixed_To_Cardinal(unsigned base, unsigned fixed)
Fixed_To_Cardinal proc C base:dword, fixed:dword
    push    ebx
    mov     eax,[base]
    mul     [fixed]
    add     eax,080h        ; eax = (base * fixed) + 0x80

    test    eax,0FF000000h  ; if high byte set, return FFFF
    jnz     rneg1
    shr     eax,8           ; else, return eax/256
    jmp     all_done
rneg1:
    mov     eax,0FFFFh      ; establish default return value
all_done:
    pop     ebx
    ret
Fixed_To_Cardinal endp

;***************************************************************************
;* Desired_Facing256 -- Desired facing algorithm 0..255 resolution.        *
;*                                                                         *
;*    This is a desired facing algorithm that has a resolution of 0        *
;*    through 255.                                                         *
;*                                                                         *
;* INPUT:   srcx,srcy   -- Source coordinate.                              *
;*                                                                         *
;*          dstx,dsty   -- Destination coordinate.                         *
;*                                                                         *
;* OUTPUT:  Returns with the desired facing to face the destination        *
;*          coordinate from the position of the source coordinate.  North  *
;*          is 0, East is 64, etc.                                         *
;*                                                                         *
;* WARNINGS:   This routine is slower than the other forms of desired      *
;*             facing calculation.  Use this routine when accuracy is      *
;*             required.                                                   *
;*                                                                         *
;* HISTORY:                                                                *
;*   12/24/1991 JLB : Adapted.                                             *
;*=========================================================================*/

;int __cdecl Desired_Facing256(LONG srcx, LONG srcy, LONG dstx, LONG dsty)
Desired_Facing256 proc C srcx:dword, srcy:dword, dstx:dword, dsty:dword
    push    ebx
    xor     ebx,ebx ; Facing number.

    ; Determine absolute X delta and left/right direction.
    mov     ecx,[dstx]
    sub     ecx,[srcx]
    jge     short xnotneg
    neg     ecx
    mov     ebx,11000000b ; Set bit 7 and 6 for leftward.
xnotneg:

    ; Determine absolute Y delta and top/bottom direction.
    mov     eax,[srcy]
    sub     eax,[dsty]
    jge     short ynotneg
    xor     ebx,01000000b ; Complement bit 6 for downward.
    neg     eax
ynotneg:

    ; Set DX=64 for quadrants 0 and 2.
    mov     edx,ebx
    and     edx,01000000b
    xor     edx,01000000b

    ; Determine if the direction is closer to the Y axis and make sure that
    ; CX holds the larger of the two deltas.  This is in preparation for the
    ; divide.
    cmp     eax,ecx
    jb      short gotaxis
    xchg    eax,ecx
    xor     edx,01000000b ; Closer to Y axis so make DX=64 for quad 0 and 2.
gotaxis:

    ; If closer to the X axis then add 64 for quadrants 0 and 2.  If
    ; closer to the Y axis then add 64 for quadrants 1 and 3.  Determined
    ; add value is in DX and save on stack.
    push    edx

    ; Make sure that the division won't overflow.  Reduce precision until
    ; the larger number is less than 256 if it appears that an overflow
    ; will occur.  If the high byte of the divisor is not zero, then this
    ; guarantees no overflow, so just abort shift operation.
    test    eax,0FFFFFF00h
    jnz     short nooverflow
again:
    test    ecx,0FFFFFF00h
    jz      short nooverflow
    shr     ecx,1
    shr     eax,1
    jmp     short again
nooverflow:

    ; Make sure that the division won't underflow (divide by zero).  If
    ; this would occur, then set the quotient to $FF and skip divide.
    or     ecx,ecx
    jnz    short nounderflow
    mov    eax,0FFFFFFFFh
    jmp    short divcomplete

    ; Derive a pseudo angle number for the octant.  The angle is based
    ; on $00 = angle matches long axis, $00 = angle matches $FF degrees.
nounderflow:
    xor    edx,edx
    shld   edx,eax,8 ; shift high byte of eax into dl
    shl    eax,8
    div    ecx
divcomplete:

    ; Integrate the 5 most significant bits into the angle index.  If DX
    ; is not zero, then it is 64.  This means that the dividend must be negated
    ; before it is added into the final angle value.
    shr    eax,3
    pop    edx
    or     edx,edx
    je     short noneg
    dec    edx
    neg    eax
noneg:
    add    eax,edx
    add    eax,ebx
    and    eax,0FFH
    pop    ebx
    ret
Desired_Facing256 endp

end
