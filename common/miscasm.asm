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
externdef C calcx:near
externdef C calcy:near
externdef C Cardinal_To_Fixed:near
externdef C Fixed_To_Cardinal:near
externdef C Desired_Facing256:near
externdef C Desired_Facing8:near
ENDIF
externdef C Set_Bit:near
externdef C Get_Bit:near
externdef C First_True_Bit:near
externdef C First_False_Bit:near
externdef C _Bound:near
externdef C Conquer_Build_Fading_Table:near
externdef C Reverse_Long:near
externdef C Reverse_Short:near
externdef C Swap_Long:near
externdef C strtrim:near

.data

_new_facing8 db 1, 2, 1, 0, 7, 6, 7, 0, 3, 2, 3, 4, 5, 6, 5, 4

.code
IFNDEF NOASM
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

;***************************************************************************
;* DESIRED_FACING8 -- Determines facing to reach a position.               *
;*                                                                         *
;*    This routine will return with the most desirable facing to reach     *
;*    one position from another.  It is accurate to a resolution of 0 to   *
;*    7.                                                                   *
;*                                                                         *
;* INPUT:       x1,y1   -- Position of origin point.                       *
;*                                                                         *
;*              x2,y2   -- Position of target.                             *
;*                                                                         *
;* OUTPUT:      Returns desired facing as a number from 0..255 with an     *
;*              accuracy of 32 degree increments.                          *
;*                                                                         *
;* WARNINGS:    If the two coordinates are the same, then -1 will be       *
;*              returned.  It is up to you to handle this case.            *
;*                                                                         *
;* HISTORY:                                                                *
;*   07/15/1991 JLB : Documented.                                          *
;*   08/08/1991 JLB : Same position check.                                 *
;*   08/14/1991 JLB : New algorithm                                        *
;*   02/06/1995 BWG : Convert to 32-bit                                    *
;*=========================================================================*
;int __cdecl Desired_Facing8(long x1, long y1, long x2, long y2)
Desired_Facing8 proc C x1:dword, y1:dword, x2:dword, y2:dword
    push    ebx
    xor     ebx,ebx ; Index byte (built).

    ; Determine Y axis difference.
    mov     edx,[y1]
    mov     ecx,[y2]
    sub     edx,ecx ; DX = Y axis (signed).
    jns     short absy
    inc     ebx ; Set the signed bit.
    neg     edx ; ABS(y)
absy:

    ; Determine X axis difference.
    shl     ebx,1
    mov     eax,[x1]
    mov     ecx,[x2]
    sub     ecx,eax ; CX = X axis (signed).
    jns     short absx
    inc     ebx ; Set the signed bit.
    neg     ecx ; ABS(x)
absx:

    ; Determine the greater axis.
    cmp     ecx,edx
    jb      short dxisbig
    xchg    ecx,edx
dxisbig:
    rcl     ebx,1 ; Y > X flag bit.

    ; Determine the closeness or farness of lesser axis.
    mov     eax,edx
    inc     eax ; Round up.
    shr     eax,1

    cmp     ecx,eax
    rcl     ebx,1 ; Close to major axis bit.

    xor     eax,eax
    mov     al,[_new_facing8+ebx]

    ; Normalize to 0..FF range.
    shl     eax,5
    pop     ebx
    ret
Desired_Facing8 endp

ENDIF

;void __cdecl Set_Bit(void* array, int bit, int value)
Set_Bit proc C array:dword, bit:dword, value:dword
    push    esi
    push    ebx
    mov     ecx, [bit]
    mov     eax, [value]
    mov     esi, [array]
    mov     ebx,ecx
    shr     ebx,5
    and     ecx,01Fh
    btr     [esi+ebx*4],ecx
    or      eax,eax
    jz      set_bit_ok
    bts     [esi+ebx*4],ecx
set_bit_ok:
    pop     ebx
    pop     esi
    ret
Set_Bit endp

;int __cdecl Get_Bit(void const* array, int bit)
Get_Bit proc C array:dword, bit:dword
    push    esi
    push    ebx
    mov     eax, [bit]
    mov     esi, [array]
    mov     ebx,eax
    shr     ebx,5
    and     eax,01Fh
    bt      [esi+ebx*4],eax
    setc    al
    pop     ebx
    pop     esi
    ret
Get_Bit endp

;int __cdecl First_True_Bit(void const* array)
First_True_Bit proc C array:dword
    push    esi
    push    ebx
    mov     esi, [array]
    mov     eax,-32
first_true_bit_again:
    add     eax,32
    mov     ebx,[esi]
    add     esi,4
    bsf     ebx,ebx
    jz      first_true_bit_again
    add     eax,ebx
    pop     ebx
    pop     esi
    ret
First_True_Bit endp

;int __cdecl First_False_Bit(void const* array)
First_False_Bit proc C array:dword
    push    esi
    push    ebx
    mov     esi, [array]
    mov     eax,-32
first_false_bit_again:
    add     eax,32
    mov     ebx,[esi]
    not     ebx
    add     esi,4
    bsf     ebx,ebx
    jz      first_false_bit_again
    add     eax,ebx
    pop     ebx
    pop     esi
    ret
First_False_Bit endp

;int __cdecl Bound(int original, int min, int max)
_Bound proc C original:dword, min:dword, max:dword
    push    ebx
    mov     eax,[original]
    mov     ebx,[min]
    mov     ecx,[max]
    cmp     ebx,ecx
    jl      bound_okorder
    xchg    ebx,ecx
bound_okorder:
    cmp     eax,ebx
    jg      bound_okmin
    mov     eax,ebx
bound_okmin:
    cmp     eax,ecx
    jl      bound_okmax
    mov     eax,ecx
bound_okmax:
    pop     ebx
    ret
_Bound endp

;***************************************************************************
;* Conquer_Build_Fading_Table -- Builds custom shadow/light fading table.  *
;*                                                                         *
;*    This routine is used to build a special fading table for C&C.  There *
;*    are certain colors that get faded to and cannot be faded again.      *
;*    With this rule, it is possible to draw a shadow multiple times and   *
;*    not have it get any lighter or darker.                               *
;*                                                                         *
;* INPUT:   palette  -- Pointer to the 768 byte IBM palette to build from. *
;*                                                                         *
;*          dest     -- Pointer to the 256 byte remap table.               *
;*                                                                         *
;*          color    -- Color index of the color to "fade to".             *
;*                                                                         *
;*          frac     -- The fraction to fade to the specified color        *
;*                                                                         *
;* OUTPUT:  Returns with pointer to the remap table.                       *
;*                                                                         *
;* WARNINGS:   none                                                        *
;*                                                                         *
;* HISTORY:                                                                *
;*   10/07/1992 JLB : Created.                                             *
;*=========================================================================*

;void* __cdecl Conquer_Build_Fading_Table(void const* palette, void* dest, int color, int frac)
Conquer_Build_Fading_Table proc C palette:dword, dest:dword, color:dword, frac:dword
    LOCAL   matchvalue:DWORD ; Last recorded match value.
    LOCAL   targetred:BYTE   ; Target gun red.
    LOCAL   targetgreen:BYTE ; Target gun green.
    LOCAL   targetblue:BYTE  ; Target gun blue.
    LOCAL   idealred:BYTE
    LOCAL   idealgreen:BYTE
    LOCAL   idealblue:BYTE
    LOCAL   matchcolor:BYTE  ; Tentative match color.

    ALLOWED_COUNT   EQU 16
    ALLOWED_START   EQU 256-ALLOWED_COUNT

    push    esi
    push    ebx
    push    edi
    cld

    ; If the source palette is NULL, then just return with current fading table pointer.
    cmp     [palette],0
    je      fini1
    cmp     [dest],0
    je      fini1

    ; Fractions above 255 become 255.
    mov     eax,[frac]
    cmp     eax,0100h
    jb      short conquer_build_fading_table_ok
    mov     [frac],0FFh
conquer_build_fading_table_ok:

    ; Record the target gun values.
    mov     esi,[palette]
    mov     ebx,[color]
    add     esi,ebx
    add     esi,ebx
    add     esi,ebx
    lodsb
    mov     [targetred],al
    lodsb
    mov     [targetgreen],al
    lodsb
    mov     [targetblue],al

    ; Main loop.
    xor     ebx,ebx            ; Remap table index.

    ; Transparent black never gets remapped.
    mov     edi,[dest]
    mov     [edi],bl
    inc     edi

    ; EBX = source palette logical number (1..255).
    ; EDI = running pointer into dest remap table.
mainloop:
    inc     ebx
    mov     esi,[palette]
    add     esi,ebx
    add     esi,ebx
    add     esi,ebx

    mov     edx,[frac]
    shr     edx,1
    ; new = orig - ((orig-target) * fraction);

    lodsb                    ; orig
    mov     dh,al            ; preserve it for later.
    sub     al,[targetred]   ; al = (orig-target)
    imul    dl               ; ax = (orig-target)*fraction
    shl     eax,1
    sub     dh,ah            ; dh = orig - ((orig-target) * fraction)
    mov     [idealred],dh    ; preserve ideal color gun value.

    lodsb                    ; orig
    mov     dh,al            ; preserve it for later.
    sub     al,[targetgreen] ; al = (orig-target)
    imul    dl               ; ax = (orig-target)*fraction
    shl     eax,1
    sub     dh,ah            ; dh = orig - ((orig-target) * fraction)
    mov     [idealgreen],dh  ; preserve ideal color gun value.

    lodsb                    ; orig
    mov     dh,al            ; preserve it for later.
    sub     al,[targetblue]  ; al = (orig-target)
    imul    dl               ; ax = (orig-target)*fraction
    shl     eax,1
    sub     dh,ah            ; dh = orig - ((orig-target) * fraction)
    mov     [idealblue],dh   ; preserve ideal color gun value.

    ; Sweep through a limited set of existing colors to find the closest
    ; matching color.

    mov     eax,[color]
    mov     [matchcolor],al  ; Default color (self).
    mov     [matchvalue],-1  ; Ridiculous match value init.
    mov     ecx,ALLOWED_COUNT

    mov     esi,[palette]    ; Pointer to original palette.
    add     esi,(ALLOWED_START)*3

    ; BH = color index.
    mov     bh,ALLOWED_START
innerloop:

    xor     edx,edx          ; Comparison value starts null.

    ; Build the comparison value based on the sum of the differences of the color
    ; guns squared.
    lodsb
    sub     al,[idealred]
    mov     ah,al
    imul    ah
    add     edx,eax

    lodsb
    sub     al,[idealgreen]
    mov     ah,al
    imul    ah
    add     edx,eax

    lodsb
    sub     al,[idealblue]
    mov     ah,al
    imul    ah
    add     edx,eax
    jz      short perfect    ; If perfect match found then quit early.

    cmp     edx,[matchvalue]
    jae     short notclose
    mov     [matchvalue],edx ; Record new possible color.
    mov     [matchcolor],bh
notclose:
    inc     bh               ; Checking color index.
    loop    innerloop
    mov     bh,[matchcolor]
perfect:
    mov     [matchcolor],bh
    xor     bh,bh            ; Make BX valid main index again.

    ; When the loop exits, we have found the closest match.
    mov     al,[matchcolor]
    stosb
    cmp     ebx,ALLOWED_START-1
    jne     mainloop

    ; Fill the remainder of the remap table with values
    ; that will remap the color to itself.
    mov     ecx,ALLOWED_COUNT
fillerloop:
    inc     bl
    mov     al,bl
    stosb
    loop    fillerloop

fini1:
    mov     esi,[dest]
    mov     eax,esi

    pop     edi
    pop     ebx
    pop     esi
    ret
Conquer_Build_Fading_Table endp

;extern "C" long __cdecl Reverse_Long(long number)
Reverse_Long proc C number:dword
    mov     eax,dword ptr [number]
    xchg    al,ah
    ror     eax,16
    xchg    al,ah
    ret
Reverse_Long endp

;extern "C" short __cdecl Reverse_Short(short number)
Reverse_Short proc C number:word
    mov     ax,[number]
    xchg    ah,al
    ret
Reverse_Short endp

;extern "C" long __cdecl Swap_Long(long number)
Swap_Long proc C number:dword
    mov     eax,dword ptr [number]
    ror     eax,16
    ret
Swap_Long endp

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
;void __cdecl strtrim(char* buffer)
strtrim proc C buffer:dword
    push    esi
    push    edi
    cmp     [buffer],0
    je      short fini

    ; Prepare for string scanning by loading pointers.
    cld
    mov     esi,[buffer]
    mov     edi,esi

    ; Strip white space from the start of the string.
looper:
    lodsb
    cmp     al,20h            ; Space
    je      short looper
    cmp     al,9            ; TAB
    je      short looper
    stosb

    ; Copy the rest of the string.
gruntloop:
    lodsb
    stosb
    or      al,al
    jnz     short gruntloop
    dec     edi
    ; Strip the white space from the end of the string.
looper2:
    mov     [edi],al
    dec     edi
    mov     ah,[edi]
    cmp     ah,20h
    je      short looper2
    cmp     ah,9
    je      short looper2

fini:
    pop     edi
    pop     esi
    ret
strtrim endp

end
