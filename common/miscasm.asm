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

end
