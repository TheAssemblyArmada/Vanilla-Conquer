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
externdef C Fat_Put_Pixel:near
externdef C Calculate_CRC:near
externdef C LCW_Uncompress:near

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

;extern "C" long __cdecl Calculate_CRC(void* buffer, long length)
Calculate_CRC proc C buffer:dword, len:dword
    local crc:dword

    push    esi
    push    ebx

    ; Load pointer to data block.
    mov     [crc],0
    pushad
    mov     esi,[buffer]
    cld

    ; Clear CRC to default (NULL) value.
    xor     ebx,ebx

    ; Fetch the length of the data block to CRC.
    mov     ecx,[len]

    jecxz   short fini

    ; Prepare the length counters.
    mov     edx,ecx
    and     dl,011b
    shr     ecx,2

    ; Perform the bulk of the CRC scanning.
    jecxz   short remainder2
accumloop:
    lodsd
    rol     ebx,1
    add     ebx,eax
    loop    accumloop

    ; Handle the remainder bytes.
remainder2:
    or      dl,dl
    jz      short fini
    mov     ecx,edx
    xor     eax,eax

    and     ecx,0FFFFh
    push    ecx
nextbyte:
    lodsb
    ror     eax,8
    loop    nextbyte
    pop     ecx
    neg     ecx
    add     ecx,4
    shl     ecx,3
    ror     eax,cl

;nextbyte:
;   shl     eax,8
;   lodsb
;   loop    nextbyte
    rol     ebx,1
    add     ebx,eax

fini:
    mov     [crc],ebx
    popad
    mov     eax,[crc]
    pop     ebx
    pop     esi
    ret
Calculate_CRC endp

; $Header: //depot/Projects/Mobius/QA/Project/Run/SOURCECODE/TIBERIANDAWN/WIN32LIB/DrawMisc.cpp#33 $
;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Library routine                          *
;*                                                                         *
;*                    File Name : UNCOMP.ASM                               *
;*                                                                         *
;*                   Programmer : Christopher Yates                        *
;*                                                                         *
;*                  Last Update : 20 August, 1990   [CY]                   *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;*                                                                         *
; ULONG LCW_Uncompress(BYTE *source, BYTE *dest, ULONG length);		   *
;*                                                                         *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *

; ----------------------------------------------------------------
;
; Here are prototypes for the routines defined within this module:
;
; ULONG LCW_Uncompress(BYTE *source, BYTE *dest, ULONG length);
;
; ----------------------------------------------------------------

;extern "C" unsigned long __cdecl LCW_Uncompress(void* source, void* dest, unsigned long length_)
LCW_Uncompress proc C source:dword, dest:dword, length_:dword
        LOCAL a1stdest:DWORD
        LOCAL maxlen:DWORD
        LOCAL lastbyte:DWORD
    ;    LOCAL lastcom:DWORD
    ;    LOCAL lastcom1:DWORD

        push ebx
        push edi
        push esi

        mov    edi,[dest]
        mov    esi,[source]
        mov    edx,[length_]

    ;
    ;
    ; uncompress data to the following codes in the format b = byte, w = word
    ; n = byte code pulled from compressed data
    ;   Bit field of n        command        description
    ; n=0xxxyyyy,yyyyyyyy        short run    back y bytes and run x+3
    ; n=10xxxxxx,n1,n2,...,nx+1    med length    copy the next x+1 bytes
    ; n=11xxxxxx,w1            med run        run x+3 bytes from offset w1
    ; n=11111111,w1,w2        long copy    copy w1 bytes from offset w2
    ; n=11111110,w1,b1        long run    run byte b1 for w1 bytes
    ; n=10000000            end        end of data reached
    ;

        mov    [a1stdest],edi
        add    edx,edi
        mov    [lastbyte],edx
        cld            ; make sure all lod and sto are forward
        mov    ebx,esi        ; save the source offset

    loop_label:
        mov    eax,[lastbyte]
        sub    eax,edi        ; get the remaining byte to uncomp
        jz    short out_label        ; were done

        mov    [maxlen],eax    ; save for string commands
        mov    esi,ebx        ; mov in the source index

        xor    eax,eax
        mov    al,[esi]
        inc    esi
        test    al,al        ; see if its a short run
        js    short notshort

        mov    ecx,eax        ;put count nibble in cl

        mov    ah,al        ; put rel offset high nibble in ah
        and    ah,0Fh        ; only 4 bits count

        shr    cl,4        ; get run -3
        add    ecx,3        ; get actual run length

        cmp    ecx,[maxlen]    ; is it too big to fit?
        jbe    short rsok        ; if not, its ok

        mov    ecx,[maxlen]    ; if so, max it out so it dosen't overrun

    rsok:
        mov    al,[esi]    ; get rel offset low byte
        lea    ebx,[esi+1]    ; save the source offset
        mov    esi,edi        ; get the current dest
        sub    esi,eax        ; get relative offset

        rep    movsb

        jmp    loop_label

    notshort:
        test    al,40h        ; is it a length?
        jne    short notlength    ; if not it could be med or long run

        cmp    al,80h        ; is it the end?
        je    short out_label        ; if so its over

        mov    cl,al        ; put the byte in count register
        and    ecx,3Fh        ; and off the extra bits

        cmp    ecx,[maxlen]    ; is it too big to fit?
        jbe    short lenok        ; if not, its ok

        mov    ecx,[maxlen]    ; if so, max it out so it dosen't overrun

    lenok:
        rep movsb

        mov    ebx,esi        ; save the source offset
        jmp    loop_label

    out_label:
              mov    eax,edi
        sub    eax,[a1stdest]
        jmp    label_exit

    notlength:
        mov    cl,al        ; get the entire code
        and    ecx,3Fh        ; and off all but the size -3
        add    ecx,3        ; add 3 for byte count

        cmp    al,0FEh
        jne    short notrunlength

        xor    ecx,ecx
        mov    cx,[esi]

        xor    eax,eax
        mov    al,[esi+2]
        lea    ebx,[esi+3]    ;save the source offset

        cmp    ecx,[maxlen]    ; is it too big to fit?
        jbe    short runlenok        ; if not, its ok

        mov    ecx,[maxlen]    ; if so, max it out so it dosen't overrun

    runlenok:
        test    ecx,0ffe0h
        jnz    dont_use_stosb
        rep    stosb
        jmp    loop_label


    dont_use_stosb:
        mov    ah,al
        mov    edx,eax
        shl    eax,16
        or    eax,edx

        test    edi,3
        jz    aligned_

        mov    [edi],eax
        mov    edx,edi
        and    edi,0fffffffch
        lea    edi,[edi+4]
        and    edx,3
        dec    dl
        xor    dl,3
        sub    ecx,edx

    aligned_:
        mov    edx,ecx
        shr    ecx,2
        rep    stosd

        and    edx,3
        jz    loop_label
        mov    ecx,edx
        rep    stosb
        jmp    loop_label






    notrunlength:
        cmp    al,0FFh        ; is it a long run?
        jne    short notlong    ; if not use the code as the size

        xor     ecx,ecx
        xor    eax,eax
        mov    cx,[esi]    ; if so, get the size
        lea    esi,[esi+2]

    notlong:
        mov    ax,[esi]    ;get the real index
        add    eax,[a1stdest]    ;add in the 1st index
        lea    ebx,[esi+2]    ;save the source offset
        cmp    ecx,[maxlen]    ;compare for overrun
        mov    esi,eax        ;use eax as new source
        jbe    short runok    ; if not, its ok

        mov    ecx,[maxlen]    ; if so, max it out so it dosen't overrun

    runok:
        test    ecx,0ffe0h
        jnz    dont_use_movsb
        rep    movsb
        jmp    loop_label




    dont_use_movsb:
        lea    edx,[edi+0fffffffch]
        cmp    esi,edx
        ja    use_movsb

        test    edi,3
        jz    aligned2

        mov    eax,[esi]
        mov    [edi],eax
        mov    edx,edi
        and    edi,0fffffffch
        lea    edi,[edi+4]
        and    edx,3
        dec    dl
        xor    dl,3
        sub    ecx,edx
        add    esi,edx

    aligned2:
        mov    edx,ecx
        shr    ecx,2
        and    edx,3
        rep    movsd
        mov    ecx,edx
    use_movsb:
        rep    movsb
        jmp    loop_label




    label_exit:
        mov    eax,edi
        mov    ebx,[dest]
        sub    eax,ebx
        pop esi
        pop edi
        pop ebx
        ret
LCW_Uncompress endp


end
