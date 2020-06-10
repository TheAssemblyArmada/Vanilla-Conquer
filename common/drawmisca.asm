;
; Copyright 2020 Electronic Arts Inc.
;
; TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
; software: you can redistribute it and/or modify it under the terms of
; the GNU General Public License as published by the Free Software Foundation,
; either version 3 of the License, or (at your option) any later version.
;
; TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
; in the hope that it will be useful, but with permitted additional restrictions
; under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
; distributed with this program. You should have received a copy of the
; GNU General Public License along with permitted additional restrictions
; with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
;
;  Misc asm functions from ww lib
;

.model flat

externdef C Buffer_Draw_Line:near

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

;***************************************************************************
;* VVC::DRAW_LINE -- Scales a virtual viewport to another virtual viewport *
;*                                                                         *
;* INPUT:	WORD sx_pixel 	- the starting x pixel position		   *
;*		WORD sy_pixel	- the starting y pixel position		   *
;*		WORD dx_pixel	- the destination x pixel position	   *
;*		WORD dy_pixel   - the destination y pixel position	   *
;*		WORD color      - the color of the line to draw		   *
;*                                                                         *
;* Bounds Checking: Compares sx_pixel, sy_pixel, dx_pixel and dy_pixel	   *
;*       with the graphic viewport it has been assigned to.		   *
;*                                                                         *
;* HISTORY:                                                                *
;*   06/16/1994 PWG : Created.                                             *
;*   08/30/1994 IML : Fixed clipping bug.				   *
;*=========================================================================*
;void __cdecl Buffer_Draw_Line(void* this_object, int sx, int sy, int dx, int dy, unsigned char color)
Buffer_Draw_Line proc C this_object:dword, x1_pixel:dword, y1_pixel:dword, x2_pixel:dword, y2_pixel:dword, color:dword
    ;*==================================================================
    ;* Define the local variables that we will use on the stack
    ;*==================================================================
    LOCAL    clip_min_x:DWORD
    LOCAL    clip_max_x:DWORD
    LOCAL    clip_min_y:DWORD
    LOCAL    clip_max_y:DWORD
    LOCAL    clip_var:DWORD
    LOCAL    accum:DWORD
    LOCAL    bpr:DWORD

    push ebx
    push edx
    push esi
    push edi

    ;*==================================================================
    ;* Take care of find the clip minimum and maximums
    ;*==================================================================
    mov    ebx,[this_object]
    xor    eax,eax
    mov    [clip_min_x],eax
    mov    [clip_min_y],eax
    mov    eax,(GraphicViewPort ptr [ebx]).GVPWidth
    mov    [clip_max_x],eax
    add    eax,(GraphicViewPort ptr [ebx]).GVPXAdd
    add    eax,(GraphicViewPort ptr [ebx]).GVPPitch
    mov    [bpr],eax
    mov    eax,(GraphicViewPort ptr [ebx]).GVPHeight
    mov    [clip_max_y],eax
    ;*==================================================================
    ;* Adjust max pixels as they are tested inclusively.
    ;*==================================================================
    dec    [clip_max_x]
    dec    [clip_max_y]
    ;*==================================================================
    ;* Set the registers with the data for drawing the line
    ;*==================================================================
    mov    eax,[x1_pixel]        ; eax = start x pixel position
    mov    ebx,[y1_pixel]        ; ebx = start y pixel position
    mov    ecx,[x2_pixel]        ; ecx = dest x pixel position
    mov    edx,[y2_pixel]        ; edx = dest y pixel position
    ;*==================================================================
    ;* This is the section that "pushes" the line into bounds.
    ;* I have marked the section with PORTABLE start and end to signify
    ;* how much of this routine is 100% portable between graphics modes.
    ;* It was just as easy to have variables as it would be for constants
    ;* so the global vars ClipMaxX,ClipMinY,ClipMaxX,ClipMinY are used
    ;* to clip the line (default is the screen)
    ;* PORTABLE start
    ;*==================================================================
    cmp    eax,[clip_min_x]
    jl    short ??clip_it
    cmp    eax,[clip_max_x]
    jg    short ??clip_it
    cmp    ebx,[clip_min_y]
    jl    short ??clip_it
    cmp    ebx,[clip_max_y]
    jg    short ??clip_it
    cmp    ecx,[clip_min_x]
    jl    short ??clip_it
    cmp    ecx,[clip_max_x]
    jg    short ??clip_it
    cmp    edx,[clip_min_y]
    jl    short ??clip_it
    cmp    edx,[clip_max_y]
    jle    short ??on_screen
    ;*==================================================================
    ;* Takes care off clipping the line.
    ;*==================================================================
??clip_it:
    call    NEAR PTR ??set_bits
    xchg    eax,ecx
    xchg    ebx,edx
    mov    edi,esi
    call    NEAR PTR ??set_bits
    mov    [clip_var],edi
    or    [clip_var],esi
    jz    short ??on_screen
    test    edi,esi
    jne    short ??off_screen
    shl    esi,2
    call    [DWORD PTR cs:??clip_tbl+esi]
    jc    ??clip_it
    xchg    eax,ecx
    xchg    ebx,edx
    shl    edi,2
    call    [DWORD PTR cs:??clip_tbl+edi]
    jmp    ??clip_it
??on_screen:
    jmp    ??draw_it
??off_screen:
    jmp    ??out
    ;*==================================================================
    ;* Jump table for clipping conditions
    ;*==================================================================
??clip_tbl    DD    ??nada,??a_up,??a_dwn,??nada
        DD    ??a_lft,??a_lft,??a_dwn,??nada
        DD    ??a_rgt,??a_up,??a_rgt,??nada
        DD    ??nada,??nada,??nada,??nada
??nada:
    clc
    retn
??a_up:
    mov    esi,[clip_min_y]
    call    NEAR PTR ??clip_vert
    stc
    retn
??a_dwn:
    mov    esi,[clip_max_y]
    neg    esi
    neg    ebx
    neg    edx
    call    NEAR PTR ??clip_vert
    neg    ebx
    neg    edx
    stc
    retn
    ;*==================================================================
    ;* xa'=xa+[(miny-ya)(xb-xa)/(yb-ya)]
    ;*==================================================================
??clip_vert:
    push    edx
    push    eax
    mov    [clip_var],edx        ; clip_var = yb
    sub    [clip_var],ebx        ; clip_var = (yb-ya)
    neg    eax            ; eax=-xa
    add    eax,ecx            ; (ebx-xa)
    mov    edx,esi            ; edx=miny
    sub    edx,ebx            ; edx=(miny-ya)
    imul    edx
    idiv    [clip_var]
    pop    edx
    add    eax,edx
    pop    edx
    mov    ebx,esi
    retn
??a_lft:
    mov    esi,[clip_min_x]
    call    NEAR PTR ??clip_horiz
    stc
    retn
??a_rgt:
    mov    esi,[clip_max_x]
    neg    eax
    neg    ecx
    neg    esi
    call    NEAR PTR ??clip_horiz
    neg    eax
    neg    ecx
    stc
    retn
    ;*==================================================================
    ;* ya'=ya+[(minx-xa)(yb-ya)/(xb-xa)]
    ;*==================================================================
??clip_horiz:
    push    edx
    mov    [clip_var],ecx        ; clip_var = xb
    sub    [clip_var],eax        ; clip_var = (xb-xa)
    sub    edx,ebx            ; edx = (yb-ya)
    neg    eax            ; eax = -xa
    add    eax,esi            ; eax = (minx-xa)
    imul    edx            ; eax = (minx-xa)(yb-ya)
    idiv    [clip_var]        ; eax = (minx-xa)(yb-ya)/(xb-xa)
    add    ebx,eax            ; ebx = xa+[(minx-xa)(yb-ya)/(xb-xa)]
    pop    edx
    mov    eax,esi
    retn
    ;*==================================================================
    ;* Sets the condition bits
    ;*==================================================================
??set_bits:
    xor    esi,esi
    cmp    ebx,[clip_min_y]    ; if y >= top its not up
    jge    short ??a_not_up
    or    esi,1
??a_not_up:
    cmp    ebx,[clip_max_y]    ; if y <= bottom its not down
    jle    short ??a_not_down
    or    esi,2
??a_not_down:
    cmp    eax,[clip_min_x]       ; if x >= left its not left
    jge    short ??a_not_left
    or    esi,4
??a_not_left:
    cmp    eax,[clip_max_x]    ; if x <= right its not right
    jle    short ??a_not_right
    or    esi,8
??a_not_right:
    retn
    ;*==================================================================
    ;* Draw the line to the screen.
    ;* PORTABLE end
    ;*==================================================================
??draw_it:
    sub    edx,ebx            ; see if line is being draw down
    jnz    short ??not_hline    ; if not then its not a hline
    jmp    short ??hline        ; do special case h line
??not_hline:
    jg    short ??down        ; if so there is no need to rev it
    neg    edx            ; negate for actual pixel length
    xchg    eax,ecx            ; swap x's to rev line draw
    sub    ebx,edx            ; get old edx
??down:
    push    edx
    push    eax
    mov    eax,[bpr]
    mul    ebx
    mov    ebx,eax
    mov    eax,[this_object]
    add    ebx,(GraphicViewPort ptr [eax]).GVPOffset
    pop    eax
    pop    edx
    mov    esi,1            ; assume a right mover
    sub    ecx,eax            ; see if line is right
    jnz    short ??not_vline    ; see if its a vertical line
    jmp    ??vline
??not_vline:
    jg    short ??right        ; if so, the difference = length
??left:
    neg    ecx            ; else negate for actual pixel length
    neg    esi            ; negate counter to move left
??right:
    cmp    ecx,edx            ; is it a horiz or vert line
    jge    short ??horiz        ; if ecx > edx then |x|>|y| or horiz
??vert:
    xchg    ecx,edx            ; make ecx greater and edx lesser
    mov    edi,ecx            ; set greater
    mov    [accum],ecx        ; set accumulator to 1/2 greater
    shr    [accum],1
    ;*==================================================================
    ;* at this point ...
    ;* eax=xpos ; ebx=page line offset; ecx=counter; edx=lesser; edi=greater;
    ;* esi=adder; accum=accumulator
    ;* in a vertical loop the adder is conditional and the inc constant
    ;*==================================================================
??vert_loop:
    add    ebx,eax
    mov    eax,[color]
??v_midloop:
    mov    [ebx],al
    dec    ecx
    jl    ??out
    add    ebx,[bpr]
    sub    [accum],edx        ; sub the lesser
    jge    ??v_midloop        ; any line could be new
    add    [accum],edi        ; add greater for new accum
    add    ebx,esi            ; next pixel over
    jmp    ??v_midloop
??horiz:
    mov    edi,ecx            ; set greater
    mov    [accum],ecx        ; set accumulator to 1/2 greater
    shr    [accum],1
    ;*==================================================================
    ;* at this point ...
    ;* eax=xpos ; ebx=page line offset; ecx=counter; edx=lesser; edi=greater;
    ;* esi=adder; accum=accumulator
    ;* in a vertical loop the adder is conditional and the inc constant
    ;*==================================================================
??horiz_loop:
    add    ebx,eax
    mov    eax,[color]
??h_midloop:
    mov    [ebx],al
    dec    ecx                ; dec counter
    jl    ??out                ; end of line
    add    ebx,esi
    sub     [accum],edx            ; sub the lesser
    jge    ??h_midloop
    add    [accum],edi            ; add greater for new accum
    add    ebx,[bpr]            ; goto next line
    jmp    ??h_midloop
    ;*==================================================================
    ;* Special case routine for horizontal line draws
    ;*==================================================================
??hline:
    cmp    eax,ecx            ; make eax < ecx
    jl    short ??hl_ac
    xchg    eax,ecx
??hl_ac:
    sub    ecx,eax            ; get len
    inc    ecx
    push    edx
    push    eax
    mov    eax,[bpr]
    mul    ebx
    mov    ebx,eax
    mov    eax,[this_object]
    add    ebx,(GraphicViewPort ptr [eax]).GVPOffset
    pop    eax
    pop    edx
    add    ebx,eax
    mov    edi,ebx
    cmp    ecx,15
    jg    ??big_line
    mov    eax,[color]
    rep    stosb            ; write as many words as possible
    jmp    short ??out        ; get outt
??big_line:
    mov    eax,[color]
    mov    ah,al
    mov     ebx,eax
    shl    eax,16
    mov    ax,bx
    test    edi,3
    jz    ??aligned
    mov    [edi],al
    inc    edi
    dec    ecx
    test    edi,3
    jz    ??aligned
    mov    [edi],al
    inc    edi
    dec    ecx
    test    edi,3
    jz    ??aligned
    mov    [edi],al
    inc    edi
    dec    ecx
??aligned:
    mov    ebx,ecx
    shr    ecx,2
    rep    stosd
    mov    ecx,ebx
    and    ecx,3
    rep    stosb
    jmp    ??out
    ;*==================================================================
    ;* a special case routine for vertical line draws
    ;*==================================================================
??vline:
    mov    ecx,edx            ; get length of line to draw
    inc    ecx
    add    ebx,eax
    mov    eax,[color]
??vl_loop:
    mov    [ebx],al        ; store bit
    add    ebx,[bpr]
    dec    ecx
    jnz    ??vl_loop
??out:
    pop edi
    pop esi
    pop edx
    pop ebx
    ret
Buffer_Draw_Line endp

end
