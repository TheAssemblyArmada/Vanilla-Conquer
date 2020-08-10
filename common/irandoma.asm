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

;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : LIBRARY                                  *
;*                                                                         *
;*                    File Name : IRANDOM.C                                *
;*                                                                         *
;*                   Programmer : Barry W. Green                           *
;*                                                                         *
;*                  Last Update : 10 Feb, 1995     [BWG]                   *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

.model flat

IFNDEF NOASM
externdef C Random:near
externdef C Get_Random_Mask:near
extern C RandNumb:near
ENDIF

.code

IFNDEF NOASM
;unsigned char Random(void)
Random proc C
    push    esi
    lea     esi, [RandNumb]    ; get offset in segment of RandNumb
    xor     eax,eax
    mov     al,[esi]
    shr     al,1               ; shift right 1 bit (bit0 in carry)
    shr     al,1
    rcl     byte ptr [esi+2],1 ; rcl byte 3 of RandNumb
    rcl     byte ptr [esi+1],1 ; rcl byte 2 of RandNumb
    cmc                        ; complement carry
    sbb     al,[esi]           ; sbb byte 1 of RandNumb
    shr     al,1               ; sets carry
    rcr     byte ptr [esi],1   ; rcr byte 1 of RandNumb
    mov     al,[esi]           ; reload byte 1 of RandNumb
    xor     al,[esi+1]         ; xor with byte 2 of RandNumb
    pop     esi
    ret
Random endp

;int Get_Random_Mask(int maxval)
Get_Random_Mask proc C maxval:dword
    bsr     ecx,[maxval] ; put bit position of highest bit in ecx
    mov     eax, 1       ; set one bit on in eax
    jz      invalid      ; if BSR shows maxval==0, return eax=1
    inc     ecx          ; get one bit higher than count showed
    shl     eax,cl       ; move our EAX bit into position
    dec     eax          ; dec it to create the mask.
invalid:
    ret
Get_Random_Mask endp
ENDIF

end