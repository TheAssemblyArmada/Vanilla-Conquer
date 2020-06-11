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

include graphicviewport.inc

externdef C Buffer_Draw_Line:near
externdef C Buffer_Fill_Rect:near
externdef C Buffer_Clear:near
externdef C Linear_Blit_To_Linear:near
externdef C Linear_Scale_To_Linear:near
externdef C Buffer_Remap:near
externdef C Build_Fading_Table:near
externdef C Buffer_Put_Pixel:near
externdef C Buffer_Get_Pixel:near
externdef C Clip_Rect:near
externdef C Confine_Rect:near

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

;***************************************************************************
;* GVPC::FILL_RECT -- Fills a rectangular region of a graphic view port	   *
;*                                                                         *
;* INPUT:	WORD the left hand x pixel position of region		   *
;*		WORD the upper x pixel position of region		   *
;*		WORD the right hand x pixel position of region		   *
;*		WORD the lower x pixel position of region		   *
;*		UBYTE the color (optional) to clear the view port to	   *
;*                                                                         *
;* OUTPUT:      none                                                       *
;*                                                                         *
;* NOTE:	This function is optimized to handle viewport with no XAdd *
;*		value.  It also handles DWORD aligning the destination	   *
;*		when speed can be gained by doing it.			   *
;* HISTORY:                                                                *
;*   06/07/1994 PWG : Created.                                             *
;*=========================================================================*

;******************************************************************************
; Much testing was done to determine that only when there are 14 or more bytes
; being copied does it speed the time it takes to do copies in this algorithm.
; For this reason and because 1 and 2 byte copies crash, is the special case
; used.  SKB 4/21/94.  Tested on 486 66mhz.  Copied by PWG 6/7/04.

OPTIMAL_BYTE_COPY equ 14

;void __cdecl Buffer_Fill_Rect(void* thisptr, int sx, int sy, int dx, int dy, unsigned char color)
Buffer_Fill_Rect proc C this_object:dword, x1_pixel:dword, y1_pixel:dword, x2_pixel:dword, y2_pixel:dword, color:byte

    ;*===================================================================
    ; Define some locals so that we can handle things quickly
    ;*===================================================================
    LOCAL    VPwidth:DWORD        ; the width of the viewport
    LOCAL    VPheight:DWORD        ; the height of the viewport
    LOCAL    VPxadd:DWORD        ; the additional x offset of viewport
    LOCAL    VPbpr:DWORD        ; the number of bytes per row of viewport


    local local_ebp:dword ; Can't use ebp

    push ebx
    push esi
    push edi

        ;*===================================================================
        ;* save off the viewport characteristics on the stack
        ;*===================================================================
        mov    ebx,[this_object]                ; get a pointer to viewport
        mov    eax,(GraphicViewPort ptr [ebx]).GVPWidth        ; get width from viewport
        mov    ecx,(GraphicViewPort ptr [ebx]).GVPHeight    ; get height from viewport
        mov    edx,(GraphicViewPort ptr [ebx]).GVPXAdd        ; get xadd from viewport
        add    edx,(GraphicViewPort ptr [ebx]).GVPPitch        ; extra pitch of direct draw surface
        mov    [VPwidth],eax                ; store the width of locally
        mov    [VPheight],ecx
        mov    [VPxadd],edx
        add    eax,edx
        mov    [VPbpr],eax

        ;*===================================================================
        ;* move the important parameters into local registers
        ;*===================================================================
        mov    eax,[x1_pixel]
        mov    ebx,[y1_pixel]
        mov    ecx,[x2_pixel]
        mov    edx,[y2_pixel]

        ;*===================================================================
        ;* Convert the x2 and y2 pixel to a width and height
        ;*===================================================================
        cmp    eax,ecx
        jl    no_swap_x
        xchg    eax,ecx

    no_swap_x:
        sub    ecx,eax
        cmp    ebx,edx
        jl    no_swap_y
        xchg    ebx,edx
    no_swap_y:
        sub    edx,ebx
        inc    ecx
        inc    edx

        ;*===================================================================
        ;* Bounds check source X.
        ;*===================================================================
        cmp    eax, [VPwidth]            ; compare with the max
        jge    done                ; starts off screen, then later
        jb    short sx_done            ; if it's not negative, it's ok

        ;------ Clip source X to left edge of screen.
        add    ecx, eax            ; Reduce width (add in negative src X).
        xor    eax, eax            ; Clip to left of screen.
    sx_done:

        ;*===================================================================
        ;* Bounds check source Y.
        ;*===================================================================
        cmp    ebx, [VPheight]            ; compare with the max
        jge    done                ; starts off screen, then later
        jb    short sy_done            ; if it's not negative, it's ok

        ;------ Clip source Y to top edge of screen.
        add    edx, ebx            ; Reduce height (add in negative src Y).
        xor    ebx, ebx            ; Clip to top of screen.

    sy_done:
        ;*===================================================================
        ;* Bounds check width versus width of source and dest view ports
        ;*===================================================================
        push    ebx                ; save off ebx for later use
        mov    ebx,[VPwidth]            ; get the source width
        sub    ebx, eax            ; Maximum allowed pixel width (given coordinates).
        sub    ebx, ecx            ; Pixel width undershoot.
        jns    short width_ok        ; if not signed no adjustment necessary
        add    ecx, ebx            ; Reduce width to screen limits.

    width_ok:
        pop    ebx                ; restore ebx to old value

        ;*===================================================================
        ;* Bounds check height versus height of source view port
        ;*===================================================================
        push    eax                ; save of eax for later use
        mov    eax, [VPheight]            ; get the source height
        sub    eax, ebx            ; Maximum allowed pixel height (given coordinates).
        sub    eax, edx            ; Pixel height undershoot.
        jns    short height_ok        ; if not signed no adjustment necessary
        add    edx, eax            ; Reduce height to screen limits.
    height_ok:
        pop    eax                ; restore eax to old value

        ;*===================================================================
        ;* Perform the last minute checks on the width and height
        ;*===================================================================
        or    ecx,ecx
        jz    done

        or    edx,edx
        jz    done

        cmp    ecx,[VPwidth]
        ja    done
        cmp    edx,[VPheight]
        ja    done

        ;*===================================================================
        ;* Get the offset into the virtual viewport.
        ;*===================================================================
        xchg    edi,eax            ; save off the contents of eax
        xchg    esi,edx            ;   and edx for size test
        mov    eax,ebx            ; move the y pixel into eax
        mul    [VPbpr]            ; multiply by bytes per row
        add    edi,eax            ; add the result into the x position
        mov    ebx,[this_object]
        add    edi,(GraphicViewPort ptr [ebx]).GVPOffset

        mov    edx,esi            ; restore edx back to real value
        mov    eax,ecx            ; store total width in ecx
        sub    eax,[VPwidth]        ; modify xadd value to include clipped
        sub    [VPxadd],eax        ;   width bytes (subtract a negative number)

        ;*===================================================================
        ; Convert the color byte to a DWORD for fast storing
        ;*===================================================================
        mov    al,[color]                ; get color to clear to
        mov    ah,al                    ; extend across WORD
        mov    ebx,eax                    ; extend across DWORD in
        shl    eax,16                    ;   several steps
        mov    ax,bx

        ;*===================================================================
        ; If there is no row offset then adjust the width to be the size of
        ;   the entire viewport and adjust the height to be 1
        ;*===================================================================
        mov    esi,[VPxadd]
        or    esi,esi                    ; set the flags for esi
        jnz    row_by_row_aligned            ;   and act on them

        xchg    eax,ecx                    ; switch bit pattern and width
        mul    edx                    ; multiply by edx to get size
        xchg    eax,ecx                    ; switch size and bit pattern
        mov    edx,1                    ; only 1 line off view port size to do

        ;*===================================================================
        ; Find out if we should bother to align the row.
        ;*===================================================================
    row_by_row_aligned:
        mov    [local_ebp],ecx                    ; width saved in ebp
        cmp    ecx,OPTIMAL_BYTE_COPY            ; is it worth aligning them?
        jl    row_by_row                ;   if not then skip

        ;*===================================================================
        ; Figure out the alignment offset if there is any
        ;*===================================================================
        mov    ebx,edi                    ; get output position
        and    ebx,3                    ;   is there a remainder?
        jz    aligned_loop                ;   if not we are aligned
        xor    ebx,3                    ; find number of align bytes
        inc    ebx                    ; this number is off by one
        sub    [local_ebp],ebx                    ; subtract from width

        ;*===================================================================
        ; Now that we have the alignment offset copy each row
        ;*===================================================================
    aligned_loop:
        mov    ecx,ebx                    ; get number of bytes to align
        rep    stosb                    ;   and move them over
        mov    ecx,[local_ebp]                    ; get number of aligned bytes
        shr    ecx,2                    ;   convert to DWORDS
        rep    stosd                    ;   and move them over
        mov    ecx,[local_ebp]                    ; get number of aligned bytes
        and    ecx,3                    ;   find the remainder
        rep    stosb                    ;   and move it over
        add    edi,esi                    ; fix the line offset
        dec    edx                    ; decrement the height
        jnz    aligned_loop                ; if more to do than do it
        jmp    done                    ; we are all done

        ;*===================================================================
        ; If not enough bytes to bother aligning copy each line across a byte
        ;    at a time.
        ;*===================================================================
    row_by_row:
        mov    ecx,[local_ebp]                    ; get total width in bytes
        rep    stosb                    ; store the width
        add    edi,esi                    ; handle the xadd
        dec    edx                    ; decrement the height
        jnz    row_by_row                ; if any left then next line
    done:
        pop edi
        pop esi
        pop ebx
        ret
Buffer_Fill_Rect endp

;***************************************************************************
;* VVPC::CLEAR -- Clears a virtual viewport instance                       *
;*                                                                         *
;* INPUT:	UBYTE the color (optional) to clear the view port to	   *
;*                                                                         *
;* OUTPUT:      none                                                       *
;*                                                                         *
;* NOTE:	This function is optimized to handle viewport with no XAdd *
;*		value.  It also handles DWORD aligning the destination	   *
;*		when speed can be gained by doing it.			   *
;* HISTORY:                                                                *
;*   06/07/1994 PWG : Created.                                             *
;*   08/23/1994 SKB : Clear the direction flag to always go forward.       *
;*=========================================================================*
;void __cdecl Buffer_Clear(void* this_object, unsigned char color)
Buffer_Clear proc C this_object:dword, color:byte
        push ebx
        push esi
        push edi

        cld                          ; always go forward

        mov    ebx,[this_object]            ; get a pointer to viewport
        mov    edi,(GraphicViewPort ptr [ebx]).GVPOffset    ; get the correct offset
        mov    edx,(GraphicViewPort ptr [ebx]).GVPHeight    ; get height from viewport
        mov    esi,(GraphicViewPort ptr [ebx]).GVPWidth        ; get width from viewport
        ;push    [dword (GraphicViewPort ebx).GVPPitch]    ; extra pitch of direct draw surface
        push    (GraphicViewPort ptr [ebx]).GVPPitch

        mov    ebx,(GraphicViewPort ptr [ebx]).GVPXAdd        ; esi = add for each line
        add    ebx,[esp]                ; Yes, I know its nasty but
        add    esp,4                    ;      it works!

        ;*===================================================================
        ; Convert the color byte to a DWORD for fast storing
        ;*===================================================================
        mov    al,[color]                ; get color to clear to
        mov    ah,al                    ; extend across WORD
        mov    ecx,eax                    ; extend across DWORD in
        shl    eax,16                    ;   several steps
        mov    ax,cx

        ;*===================================================================
        ; Find out if we should bother to align the row.
        ;*===================================================================

        cmp    esi , OPTIMAL_BYTE_COPY            ; is it worth aligning them?
        jl    byte_by_byte                ;   if not then skip

        ;*===================================================================
        ; Figure out the alignment offset if there is any
        ;*===================================================================
        push    ebx
    
    dword_aligned_loop:
            mov    ecx , edi
            mov    ebx , esi
            neg    ecx
            and    ecx , 3
            sub    ebx , ecx
            rep    stosb
            mov    ecx , ebx
            shr    ecx , 2
            rep    stosd
            mov    ecx , ebx
            and    ecx , 3
            rep    stosb
            add    edi , [ esp ]
            dec    edx                    ; decrement the height
            jnz    dword_aligned_loop                ; if more to do than do it
            pop    eax
            jmp buffer_clear_cleanup

        ;*===================================================================
        ; If not enough bytes to bother aligning copy each line across a byte
        ;    at a time.
        ;*===================================================================
    byte_by_byte:
        mov    ecx,esi                    ; get total width in bytes
        rep    stosb                    ; store the width
        add    edi,ebx                    ; handle the xadd
        dec    edx                    ; decrement the height
        jnz    byte_by_byte                ; if any left then next line
    buffer_clear_cleanup:
        pop edi
        pop esi
        pop ebx
        ret
Buffer_Clear endp

;BOOL __cdecl Linear_Blit_To_Linear(void* this_object, void* dest, int x_pixel, int y_pixel, int dest_x0, int dest_y0, int pixel_width, int pixel_height, BOOL trans)
Linear_Blit_To_Linear proc C this_object:dword, dest:dword, x_pixel:dword, y_pixel:dword, dest_x0:dword, dest_y0:dword, pixel_width:dword, pixel_height:dword, trans:dword
        ;*===================================================================
        ; Define some locals so that we can handle things quickly
        ;*===================================================================
        LOCAL     x1_pixel :dword
        LOCAL    y1_pixel :dword
        LOCAL    dest_x1 : dword
        LOCAL    dest_y1 : dword
        LOCAL    scr_adjust_width:DWORD
        LOCAL    dest_adjust_width:DWORD
        LOCAL    source_area :  dword
        LOCAL    dest_area :  dword

        push ebx
        push esi
        push edi
    
        ;This Clipping algorithm is a derivation of the very well known
        ;Cohen-Sutherland Line-Clipping test. Due to its simplicity and efficiency
        ;it is probably the most commontly implemented algorithm both in software
        ;and hardware for clipping lines, rectangles, and convex polygons against
        ;a rectagular clipping window. For reference see
        ;"COMPUTER GRAPHICS principles and practice by Foley, Vandam, Feiner, Hughes
        ; pages 113 to 177".
        ; Briefly consist in computing the Sutherland code for both end point of
        ; the rectangle to find out if the rectangle is:
        ; - trivially accepted (no further clipping test, display rectangle)
        ; - trivially rejected (return with no action)
        ; - retangle must be iteratively clipped again edges of the clipping window
        ;   and the remaining retangle is display.

        ; Clip Source Rectangle against source Window boundaries.
            mov      esi,[this_object]    ; get ptr to src
            xor     ecx,ecx            ; Set sutherland code to zero
            xor     edx,edx            ; Set sutherland code to zero

           ; compute the difference in the X axis and get the bit signs into ecx , edx
            mov    edi,(GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
            mov    ebx,[x_pixel]        ; Get first end point x_pixel into register
            mov    eax,[x_pixel]        ; Get second end point x_pixel into register
            add    ebx,[pixel_width]   ; second point x1_pixel = x + width
            shld    ecx, eax,1        ; the sign bit of x_pixel is sutherland code0 bit4
            mov    [x1_pixel],ebx        ; save second for future use
            inc    edi            ; move the right edge by one unit
            shld    edx,ebx,1        ; the sign bit of x1_pixel is sutherland code0 bit4
            sub    eax,edi            ; compute the difference x0_pixel - width
            sub    ebx,edi            ; compute the difference x1_pixel - width
            shld    ecx,eax,1        ; the sign bit of the difference is sutherland code0 bit3
            shld    edx,ebx,1        ; the sign bit of the difference is sutherland code0 bit3

           ; the following code is just a repeticion of the above code
           ; in the Y axis.
            mov    edi,(GraphicViewPort ptr [esi]).GVPHeight ; get height into register
            mov    ebx,[y_pixel]
            mov    eax,[y_pixel]
            add    ebx,[pixel_height]
            shld    ecx,eax,1
            mov    [y1_pixel ],ebx
            inc    edi
            shld    edx,ebx,1
            sub    eax,edi
            sub    ebx,edi
            shld    ecx,eax,1
            shld    edx,ebx,1

            ; Here we have the to Sutherland code into cl and dl
            xor    cl,5               ; bit 2 and 0 are complented, reverse then
            xor    dl,5               ; bit 2 and 0 are complented, reverse then
            mov    al,cl               ; save code1 in case we have to clip iteratively
            test    dl,cl               ; if any bit in code0 and its counter bit
            jnz    real_out           ; in code1 is set then the rectangle in outside
            or    al,dl               ; if all bit of code0 the counter bit in
            jz    clip_against_dest    ; in code1 is set to zero, then all
                               ; end points of the rectangle are
                               ; inside the clipping window

             ; if we are here the polygon have to be clip iteratively
            test    cl,1000b           ; if bit 4 in code0 is set then
            jz    scr_left_ok           ; x_pixel is smaller than zero
            mov    [x_pixel],0           ; set x_pixel to cero.

        scr_left_ok:
            test    cl,0010b           ; if bit 2 in code0 is set then
            jz    scr_bottom_ok           ; y_pixel is smaller than zero
            mov    [ y_pixel ],0           ; set y_pixel to cero.

        scr_bottom_ok:
            test    dl,0100b           ; if bit 3 in code1 is set then
            jz    scr_right_ok           ; x1_pixel is greater than the width
            mov    eax,(GraphicViewPort ptr [esi]).GVPWidth ; get width into register
            mov    [ x1_pixel ],eax       ; set x1_pixel to width.
        scr_right_ok:
            test    dl,0001b           ; if bit 0 in code1 is set then
            jz    clip_against_dest    ; y1_pixel is greater than the width
            mov    eax,(GraphicViewPort ptr [esi]).GVPHeight  ; get height into register
            mov    [ y1_pixel ],eax       ; set y1_pixel to height.

        ; Clip Source Rectangle against destination Window boundaries.
        clip_against_dest:

           ; build the destination rectangle before clipping
           ; dest_x1 = dest_x0 + ( x1_pixel - x_pixel )
           ; dest_y1 = dest_y0 + ( y1_pixel - y_pixel )
            mov    eax,[dest_x0]         ; get dest_x0 into eax
            mov    ebx,[dest_y0]         ; get dest_y0 into ebx
            sub    eax,[x_pixel]         ; subtract x_pixel from eax
            sub    ebx,[y_pixel]         ; subtract y_pixel from ebx
            add    eax,[x1_pixel]         ; add x1_pixel to eax
            add    ebx,[y1_pixel]         ; add y1_pixel to ebx
            mov    [dest_x1],eax         ; save eax into dest_x1
            mov    [dest_y1],ebx         ; save eax into dest_y1


          ; The followin code is a repeticion of the Sutherland clipping
          ; descrived above.
            mov      esi,[dest]        ; get ptr to src
            xor     ecx,ecx
            xor     edx,edx
            mov    edi,(GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
            mov    eax,[dest_x0]
            mov    ebx,[dest_x1]
            shld    ecx,eax,1
            inc    edi
            shld    edx,ebx,1
            sub    eax,edi
            sub    ebx,edi
            shld    ecx,eax,1
            shld    edx,ebx,1

            mov    edi,(GraphicViewPort ptr [esi]).GVPHeight ; get height into register
            mov    eax,[dest_y0]
            mov    ebx,[dest_y1]
            shld    ecx,eax,1
            inc    edi
            shld    edx,ebx,1
            sub    eax,edi
            sub    ebx,edi
            shld    ecx,eax,1
            shld    edx,ebx,1

            xor    cl,5
            xor    dl,5
            mov    al,cl
            test    dl,cl
            jnz    real_out
            or    al,dl
            jz    do_blit

            test    cl,1000b
            jz    dest_left_ok
            mov    eax,[ dest_x0 ]
            mov    [ dest_x0 ],0
            sub    [ x_pixel ],eax

        dest_left_ok:
            test    cl,0010b
            jz    dest_bottom_ok
            mov    eax,[ dest_y0 ]
            mov    [ dest_y0 ],0
            sub    [ y_pixel ],eax


        dest_bottom_ok:
            test    dl,0100b
            jz    dest_right_ok
            mov    ebx,(GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
            mov    eax,[ dest_x1 ]
            mov    [ dest_x1 ],ebx
            sub    eax,ebx
            sub    [ x1_pixel ],eax

        dest_right_ok:
            test    dl,0001b
            jz    do_blit
            mov    ebx,(GraphicViewPort ptr [esi]).GVPHeight  ; get width into register
            mov    eax,[ dest_y1 ]
            mov    [ dest_y1 ],ebx
            sub    eax,ebx
            sub    [ y1_pixel ],eax


        ; Here is where    we do the actual blit
        do_blit:
               cld
               mov    ebx,[this_object]
               mov    esi,(GraphicViewPort ptr [ebx]).GVPOffset
               mov    eax,(GraphicViewPort ptr [ebx]).GVPXAdd
               add    eax,(GraphicViewPort ptr [ebx]).GVPWidth
               add    eax,(GraphicViewPort ptr [ebx]).GVPPitch
               mov    ecx,eax
               mul    [y_pixel]
               add    esi,[x_pixel]
               mov    [source_area],ecx
               add    esi,eax

               add    ecx,[x_pixel ]
               sub    ecx,[x1_pixel ]
               mov    [scr_adjust_width ],ecx

               mov    ebx,[dest]
               mov    edi,(GraphicViewPort ptr [ebx]).GVPOffset
               mov    eax,(GraphicViewPort ptr [ebx]).GVPXAdd
               add    eax,(GraphicViewPort ptr [ebx]).GVPWidth
               add    eax,(GraphicViewPort ptr [ebx]).GVPPitch
               mov    ecx,eax
               mul    [ dest_y0 ]
               add    edi,[ dest_x0 ]
               mov    [ dest_area ],ecx
               add    edi,eax

               mov    eax,[ dest_x1 ]
               sub    eax,[ dest_x0 ]
               jle    real_out
               sub    ecx,eax
               mov    [ dest_adjust_width ],ecx

               mov    edx,[ dest_y1 ]
               sub    edx,[ dest_y0 ]
               jle    real_out

               cmp    esi,edi
               jz    real_out
               jl    backupward_blit

        ; ********************************************************************
        ; Forward bitblit

               test    [ trans ],1
               jnz    forward_Blit_trans


        ; the inner loop is so efficient that
        ; the optimal consept no longer apply because
        ; the optimal byte have to by a number greather than 9 bytes
               cmp    eax,10
               jl    forward_loop_bytes

        forward_loop_dword:
               mov    ecx,edi
               mov    ebx,eax
               neg    ecx
               and    ecx,3
               sub    ebx,ecx
               rep    movsb
               mov    ecx,ebx
               shr    ecx,2
               rep    movsd
               mov    ecx,ebx
               and    ecx,3
               rep    movsb
               add    esi,[ scr_adjust_width ]
               add    edi,[ dest_adjust_width ]
               dec    edx
               jnz    forward_loop_dword
               jmp    real_out    ;ret

        forward_loop_bytes:
               mov    ecx,eax
               rep    movsb
               add    esi,[ scr_adjust_width ]
               add    edi,[ dest_adjust_width ]
               dec    edx
               jnz    forward_loop_bytes
               jmp    real_out

        forward_Blit_trans:
               mov    ecx,eax
               and    ecx,01fh
               lea    ecx,[ ecx + ecx * 4 ]
               neg    ecx
               shr    eax,5
               lea    ecx,[ transp_reference + ecx * 2 ]
               mov    [ y1_pixel ],ecx

        forward_loop_trans:
               mov    ecx,eax
               jmp    [ y1_pixel ]
        forward_trans_line:
               ;REPT    32
               ;local    transp_pixel
                 ;No REPT in msvc inline assembly.
                 ; Save ECX and use as counter instead. ST - 12/19/2018 5:41PM
                 push    ecx
                 mov    ecx, 32

        rept_loop:
                mov    bl,[ esi ]
                test    bl,bl
                jz        transp_pixel
                mov    [ edi ],bl
        transp_pixel:
                inc    esi
                inc    edi

                dec    ecx            ;ST - 12/19/2018 5:44PM
                jnz    rept_loop    ;ST - 12/19/2018 5:44PM

                pop    ecx            ;ST - 12/19/2018 5:44PM

            ;ENDM
            transp_reference:
               dec    ecx
               jge    forward_trans_line
               add    esi,[ scr_adjust_width ]
               add    edi,[ dest_adjust_width ]
               dec    edx
               jnz    forward_loop_trans
               jmp    real_out        ;ret


        ; ************************************************************************
        ; backward bitblit

        backupward_blit:

            mov    ebx,[ source_area ]
            dec    edx
            add    esi,eax
            imul    ebx,edx
            std
            lea    esi,[ esi + ebx - 1 ]

            mov    ebx,[ dest_area ]
            add    edi,eax
            imul    ebx,edx
            lea    edi,[ edi + ebx - 1]

               test    [ trans ],1
               jnz    backward_Blit_trans

                cmp    eax,15
                jl    backward_loop_bytes

        backward_loop_dword:
            push    edi
            push    esi
            lea    ecx,[edi+1]
            mov    ebx,eax
            and    ecx,3        ; Get non aligned bytes.
            sub    ebx,ecx        ; remove that from the total size to be copied later.
            rep    movsb        ; do the copy.
            sub    esi,3
            mov    ecx,ebx        ; Get number of bytes left.
             sub    edi,3
            shr    ecx,2        ; Do 4 bytes at a time.
            rep    movsd        ; do the dword copy.
            mov    ecx,ebx
            add    esi,3
            add    edi,3
            and    ecx,03h
            rep    movsb        ; finnish the remaining bytes.
            pop    esi
            pop    edi
                sub    esi,[ source_area ]
                sub    edi,[ dest_area ]
            dec    edx
            jge    backward_loop_dword
            cld
            jmp    real_out        ;ret

        backward_loop_bytes:
            push    edi
            mov    ecx,eax        ; remove that from the total size to be copied later.
            push    esi
            rep    movsb        ; do the copy.
            pop    esi
            pop    edi
                sub    esi,[ source_area ]
                sub    edi,[ dest_area ]
            dec    edx
            jge    backward_loop_bytes
            cld
            ret

        backward_Blit_trans:
               mov    ecx,eax
               and    ecx,01fh
               lea    ecx,[ ecx + ecx * 4 ]
               neg    ecx
               shr    eax,5
               lea    ecx,[ back_transp_reference + ecx * 2 ]
               mov    [ y1_pixel ],ecx

        backward_loop_trans:
               mov    ecx,eax
               push    edi
               push    esi
               jmp    [ y1_pixel ]
        backward_trans_line:
               ;REPT    32
               ;local    transp_pixel2
                 ;No REPT in msvc inline assembly.
                 ; Save ECX and use as counter instead. ST - 12/19/2018 5:41PM
                 push    ecx
                 mov    ecx, 32
        rept_loop2:
                 mov    bl,[ esi ]
                 test    bl,bl
                 jz    transp_pixel2
                 mov    [ edi ],bl
        transp_pixel2:
                 dec    esi
                 dec    edi

                 dec    ecx                ;ST - 12/19/2018 5:44PM
                 jnz    rept_loop2        ;ST - 12/19/2018 5:44PM

                 pop    ecx                ;ST - 12/19/2018 5:44PM
            
            ;ENDM
            
             back_transp_reference:
               dec    ecx
               jge    backward_trans_line
               pop    esi
               pop    edi
               sub    esi,[ source_area ]
               sub    edi,[ dest_area ]
               dec    edx
               jge    backward_loop_trans
               cld

        real_out:
            pop edi
            pop esi
            pop ebx
            ret

Linear_Blit_To_Linear endp

;***************************************************************************
;* VVC::SCALE -- Scales a virtual viewport to another virtual viewport     *
;*                                                                         *
;* INPUT:                                                                  *
;*                                                                         *
;* OUTPUT:                                                                 *
;*                                                                         *
;* WARNINGS:                                                               *
;*                                                                         *
;* HISTORY:                                                                *
;*   06/16/1994 PWG : Created.                                             *
;*=========================================================================*

;BOOL __cdecl Linear_Scale_To_Linear(void* this_object, void* dest, int src_x, int src_y, int dst_x, int dst_y, int src_width, int src_height, int dst_width, int dst_height, BOOL trans, char* remap)

Linear_Scale_To_Linear proc C this_object:dword, dest:dword, src_x:dword, src_y:dword, dst_x:dword, dst_y:dword, src_width:dword, src_height:dword, dst_width:dword, dst_height:dword, trans:dword, remap:dword
        ;*===================================================================
        ;* Define local variables to hold the viewport characteristics
        ;*===================================================================
        local	src_x0 : dword
        local	src_y0 : dword
        local	src_x1 : dword
        local	src_y1 : dword

        local	dst_x0 : dword
        local	dst_y0 : dword
        local	dst_x1 : dword
        local	dst_y1 : dword

        local	src_win_width : dword
        local	dst_win_width : dword
        local	dy_intr : dword
        local	dy_frac : dword
        local	dy_acc  : dword
        local	dx_frac : dword

        local	counter_x     : dword
        local	counter_y     : dword
        local	remap_counter :dword
        local	entry : dword

        push ebx
        push esi
        push edi
        
        ;*===================================================================
        ;* Check for scale error when to or from size 0,0
        ;*===================================================================
        cmp    [dst_width],0
        je    all_done
        cmp    [dst_height],0
        je    all_done
        cmp    [src_width],0
        je    all_done
        cmp    [src_height],0
        je    all_done

        mov    eax , [ src_x ]
        mov    ebx , [ src_y ]
        mov    [ src_x0 ] , eax
        mov    [ src_y0 ] , ebx
        add    eax , [ src_width ]
        add    ebx , [ src_height ]
        mov    [ src_x1 ] , eax
        mov    [ src_y1 ] , ebx

        mov    eax , [ dst_x ]
        mov    ebx , [ dst_y ]
        mov    [ dst_x0 ] , eax
        mov    [ dst_y0 ] , ebx
        add    eax , [ dst_width ]
        add    ebx , [ dst_height ]
        mov    [ dst_x1 ] , eax
        mov    [ dst_y1 ] , ebx

    ; Clip Source Rectangle against source Window boundaries.
        mov      esi , [ this_object ]        ; get ptr to src
        xor     ecx , ecx
        xor     edx , edx
        mov    edi , (GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
        mov    eax , [ src_x0 ]
        mov    ebx , [ src_x1 ]
        shld    ecx , eax , 1
        inc    edi
        shld    edx , ebx , 1
        sub    eax , edi
        sub    ebx , edi
        shld    ecx , eax , 1
        shld    edx , ebx , 1

        mov    edi,(GraphicViewPort ptr [esi]).GVPHeight ; get height into register
        mov    eax , [ src_y0 ]
        mov    ebx , [ src_y1 ]
        shld    ecx , eax , 1
        inc    edi
        shld    edx , ebx , 1
        sub    eax , edi
        sub    ebx , edi
        shld    ecx , eax , 1
        shld    edx , ebx , 1

        xor    cl , 5
        xor    dl , 5
        mov    al , cl
        test    dl , cl
        jnz    all_done
        or    al , dl
        jz    clip_against_dest2
        mov    bl , dl
        test    cl , 1000b
        jz    src_left_ok
        xor    eax , eax
        mov    [ src_x0 ] , eax
        sub    eax , [ src_x ]
        imul    [ dst_width ]
        idiv    [ src_width ]
        add    eax , [ dst_x ]
        mov    [ dst_x0 ] , eax

    src_left_ok:
        test    cl , 0010b
        jz    src_bottom_ok
        xor    eax , eax
        mov    [ src_y0 ] , eax
        sub    eax , [ src_y ]
        imul    [ dst_height ]
        idiv    [ src_height ]
        add    eax , [ dst_y ]
        mov    [ dst_y0 ] , eax

    src_bottom_ok:
        test    bl , 0100b
        jz    src_right_ok
        mov    eax , (GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
        mov    [ src_x1 ] , eax
        sub    eax , [ src_x ]
        imul    [ dst_width ]
        idiv    [ src_width ]
        add    eax , [ dst_x ]
        mov    [ dst_x1 ] , eax

    src_right_ok:
        test    bl , 0001b
        jz    clip_against_dest2
        mov    eax , (GraphicViewPort ptr [esi]).GVPHeight  ; get width into register
        mov    [ src_y1 ] , eax
        sub    eax , [ src_y ]
        imul    [ dst_height ]
        idiv    [ src_height ]
        add    eax , [ dst_y ]
        mov    [ dst_y1 ] , eax

    ; Clip destination Rectangle against source Window boundaries.
    clip_against_dest2:
        mov      esi , [ dest ]        ; get ptr to src
        xor     ecx , ecx
        xor     edx , edx
        mov    edi , (GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
        mov    eax , [ dst_x0 ]
        mov    ebx , [ dst_x1 ]
        shld    ecx , eax , 1
        inc    edi
        shld    edx , ebx , 1
        sub    eax , edi
        sub    ebx , edi
        shld    ecx , eax , 1
        shld    edx , ebx , 1

        mov    edi,(GraphicViewPort ptr [esi]).GVPHeight ; get height into register
        mov    eax , [ dst_y0 ]
        mov    ebx , [ dst_y1 ]
        shld    ecx , eax , 1
        inc    edi
        shld    edx , ebx , 1
        sub    eax , edi
        sub    ebx , edi
        shld    ecx , eax , 1
        shld    edx , ebx , 1

        xor    cl , 5
        xor    dl , 5
        mov    al , cl
        test    dl , cl
        jnz    all_done
        or    al , dl
        jz    do_scaling
        mov    bl , dl
        test    cl , 1000b
        jz    dst_left_ok
        xor    eax , eax
        mov    [ dst_x0 ] , eax
        sub    eax , [ dst_x ]
        imul    [ src_width ]
        idiv    [ dst_width ]
        add    eax , [ src_x ]
        mov    [ src_x0 ] , eax

    dst_left_ok:
        test    cl , 0010b
        jz    dst_bottom_ok
        xor    eax , eax
        mov    [ dst_y0 ] , eax
        sub    eax , [ dst_y ]
        imul    [ src_height ]
        idiv    [ dst_height ]
        add    eax , [ src_y ]
        mov    [ src_y0 ] , eax

    dst_bottom_ok:
        test    bl , 0100b
        jz    dst_right_ok
        mov    eax , (GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
        mov    [ dst_x1 ] , eax
        sub    eax , [ dst_x ]
        imul    [ src_width ]
        idiv    [ dst_width ]
        add    eax , [ src_x ]
        mov    [ src_x1 ] , eax

    dst_right_ok:
        test    bl , 0001b
        jz    do_scaling

        mov    eax , (GraphicViewPort ptr [esi]).GVPHeight  ; get width into register
        mov    [ dst_y1 ] , eax
        sub    eax , [ dst_y ]
        imul    [ src_height ]
        idiv    [ dst_height ]
        add    eax , [ src_y ]
        mov    [ src_y1 ] , eax

    do_scaling:

           cld
           mov    ebx , [ this_object ]
           mov    esi , (GraphicViewPort ptr [ebx]).GVPOffset
           mov    eax , (GraphicViewPort ptr [ebx]).GVPXAdd
           add    eax , (GraphicViewPort ptr [ebx]).GVPWidth
           add    eax , (GraphicViewPort ptr [ebx]).GVPPitch
           mov    [ src_win_width ] , eax
           mul    [ src_y0 ]
           add    esi , [ src_x0 ]
           add    esi , eax

           mov    ebx , [ dest ]
           mov    edi , (GraphicViewPort ptr [ebx]).GVPOffset
           mov    eax , (GraphicViewPort ptr [ebx]).GVPXAdd
           add    eax , (GraphicViewPort ptr [ebx]).GVPWidth
           add    eax , (GraphicViewPort ptr [ebx]).GVPPitch
           mov    [ dst_win_width ] , eax
           mul    [ dst_y0 ]
           add    edi , [ dst_x0 ]
           add    edi , eax

           mov    eax , [ src_height ]
           xor    edx , edx
           mov    ebx , [ dst_height ]
           idiv    [ dst_height ]
           imul    eax , [ src_win_width ]
           neg    ebx
           mov    [ dy_intr ] , eax
           mov    [ dy_frac ] , edx
           mov    [ dy_acc ]  , ebx

           mov    eax , [ src_width ]
           xor    edx , edx
           shl    eax , 16
           idiv    [ dst_width ]
           xor    edx , edx
           shld    edx , eax , 16
           shl    eax , 16

           mov    ecx , [ dst_y1 ]
           mov    ebx , [ dst_x1 ]
           sub    ecx , [ dst_y0 ]
           jle    all_done
           sub    ebx , [ dst_x0 ]
           jle    all_done

           mov    [ counter_y ] , ecx

           cmp    [ trans ] , 0
           jnz    transparency

           cmp    [ remap ] , 0
           jnz    normal_remap

    ; *************************************************************************
    ; normal scale
           mov    ecx , ebx
           and    ecx , 01fh
           lea    ecx , [ ecx + ecx * 2 ]
           shr    ebx , 5
           neg    ecx
           mov    [ counter_x ] , ebx
           lea    ecx , [ ref_point + ecx + ecx * 2 ]
           mov    [ entry ] , ecx

     outter_loop:
           push    esi
           push    edi
           xor    ecx , ecx
           mov    ebx , [ counter_x ]
           jmp    [ entry ]
     inner_loop:
           ;/ REPT not supported for inline asm. ST - 12/19/2018 6:11PM
             ;/REPT    32
                 push ebx        ;/ST - 12/19/2018 6:11PM
                mov ebx,32    ;/ST - 12/19/2018 6:11PM
rept_loop3:
               mov    cl , [ esi ]
               add    ecx , eax
               adc    esi , edx
               mov    [ edi ] , cl
               inc    edi

                dec ebx                ;/ST - 12/19/2018 6:11PM
                jnz rept_loop3       ;/ST - 12/19/2018 6:11PM
                pop ebx                ;/ST - 12/19/2018 6:11PM
           ;/ENDM
     ref_point:
           dec    ebx
           jge    inner_loop

           pop    edi
           pop    esi
           add    edi , [ dst_win_width ]
           add    esi , [ dy_intr ]

           mov    ebx , [ dy_acc ]
           add    ebx , [ dy_frac ]
           jle    skip_line
           add    esi , [ src_win_width ]
           sub    ebx , [ dst_height ]
    skip_line:
        dec    [ counter_y ]
        mov    [ dy_acc ] , ebx
        jnz    outter_loop
        jmp    all_done    ;ret


    ; *************************************************************************
    ; normal scale with remap

    normal_remap:
           mov    ecx , ebx
           mov    [ dx_frac ], eax
           and    ecx , 01fh
           mov    eax , [ remap ]
           shr    ebx , 5
           imul    ecx , - 13
           mov    [ counter_x ] , ebx
           lea    ecx , [ remapref_point + ecx ]
           mov    [ entry ] , ecx

     remapoutter_loop:
           mov    ebx , [ counter_x ]
           push    esi
           mov    [ remap_counter ] , ebx
           push    edi
           xor    ecx , ecx
           xor    ebx , ebx
           jmp    [ entry ]
     remapinner_loop:
           ;/ REPT not supported for inline asm. ST - 12/19/2018 6:11PM
             ;/REPT    32
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi
               mov    bl , [ esi ]
               add    ecx , [ dx_frac ]
               adc    esi , edx
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
               inc    edi

           ;ENDM
     remapref_point:
           dec    [ remap_counter ]
           jge    remapinner_loop

           pop    edi
           pop    esi
           add    edi , [ dst_win_width ]
           add    esi , [ dy_intr ]

           mov    ebx , [ dy_acc ]
           add    ebx , [ dy_frac ]
           jle    remapskip_line
           add    esi , [ src_win_width ]
           sub    ebx , [ dst_height ]
    remapskip_line:
        dec    [ counter_y ]
        mov    [ dy_acc ] , ebx
        jnz    remapoutter_loop
        jmp    all_done    ;ret


    ;****************************************************************************
    ; scale with trnsparency

    transparency:
           cmp    [ remap ] , 0
           jnz    trans_remap

    ; *************************************************************************
    ; normal scale with transparency
           mov    ecx , ebx
           and    ecx , 01fh
           imul    ecx , -13
           shr    ebx , 5
           mov    [ counter_x ] , ebx
           lea    ecx , [ trans_ref_point + ecx ]
           mov    [ entry ] , ecx

     trans_outter_loop:
           xor    ecx , ecx
           push    esi
           push    edi
           mov    ebx , [ counter_x ]
           jmp    [ entry ]
     trans_inner_loop:
           
           ;/ REPT not supported for inline asm. ST - 12/19/2018 6:11PM
             ;/REPT    32
                 push ebx        ;/ST - 12/19/2018 6:11PM
                mov ebx,32    ;/ST - 12/19/2018 6:11PM
rept_loop4:
             
               mov    cl , [ esi ]
               test    cl , cl
               jz    trans_pixel
               mov    [ edi ] , cl
           trans_pixel:
               add    ecx , eax
               adc    esi , edx
               inc    edi
                
                dec ebx                ;/ST - 12/19/2018 6:11PM
                jnz rept_loop4        ;/ST - 12/19/2018 6:11PM
                pop ebx               ;//ST - 12/19/2018 6:11PM
           
             ;/ENDM
     trans_ref_point:
           dec    ebx
           jge    trans_inner_loop

           pop    edi
           pop    esi
           add    edi , [ dst_win_width ]
           add    esi , [ dy_intr ]

           mov    ebx , [ dy_acc ]
           add    ebx , [ dy_frac ]
           jle    trans_skip_line
           add    esi , [ src_win_width ]
           sub    ebx , [ dst_height ]
    trans_skip_line:
        dec    [ counter_y ]
        mov    [ dy_acc ] , ebx
        jnz    trans_outter_loop
        jmp    all_done    ;/ret


    ; *************************************************************************
    ; normal scale with remap

    trans_remap:
           mov    ecx , ebx
           mov    [ dx_frac ], eax
           and    ecx , 01fh
           mov    eax , [ remap ]
           shr    ebx , 5
           imul    ecx , - 17
           mov    [ counter_x ] , ebx
           lea    ecx , [ trans_remapref_point + ecx ]
           mov    [ entry ] , ecx

     trans_remapoutter_loop:
           mov    ebx , [ counter_x ]
           push    esi
           mov    [ remap_counter ] , ebx
           push    edi
           xor    ecx , ecx
           xor    ebx , ebx
           jmp    [ entry ]
     
    
    trans_remapinner_loop:
           ;/ REPT not supported for inline asm. ST - 12/19/2018 6:11PM
             ;/REPT    32
             ;/ Run out of registers so use ebp
                 push ebp        ;/ST - 12/19/2018 6:11PM
                mov ebp,32    ;/ST - 12/19/2018 6:11PM
rept_loop5:
               mov    bl , [ esi ]
               test    bl , bl
               jz    trans_pixel2
               mov    cl , [ eax + ebx ]
               mov    [ edi ] , cl
          trans_pixel2:
               add    ecx , [ dx_frac ]
               adc    esi , edx
               inc    edi
                
                dec ebp                ;/ST - 12/19/2018 6:11PM
                jnz rept_loop5        ;/ST - 12/19/2018 6:11PM
                pop ebp                ;/ST - 12/19/2018 6:11PM

           ;ENDM

     trans_remapref_point:
           dec    [ remap_counter ]
           jge    trans_remapinner_loop

           pop    edi
           pop    esi
           add    edi , [ dst_win_width ]
           add    esi , [ dy_intr ]

           mov    ebx , [ dy_acc ]
           add    ebx , [ dy_frac ]
           jle    trans_remapskip_line
           add    esi , [ src_win_width ]
           sub    ebx , [ dst_height ]
    trans_remapskip_line:
        dec    [ counter_y ]
        mov    [ dy_acc ] , ebx
        jnz    trans_remapoutter_loop
        ;ret


    all_done:
        pop edi
        pop esi
        pop ebx
        ret
Linear_Scale_To_Linear endp

;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Westwood 32 bit Library                  *
;*                                                                         *
;*                    File Name : REMAP.ASM                                *
;*                                                                         *
;*                   Programmer : Phil W. Gorrow                           *
;*                                                                         *
;*                   Start Date : July 1, 1994                             *
;*                                                                         *
;*                  Last Update : July 1, 1994   [PWG]                     *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
;VOID __cdecl Buffer_Remap(void* this_object, int sx, int sy, int width, int height, void* remap)
Buffer_Remap proc C this_object:dword, x0_pixel:dword, y0_pixel:dword, region_width:dword, region_height:dword, remap:dword
        ;*===================================================================
        ; Define some locals so that we can handle things quickly
        ;*===================================================================
        local    x1_pixel  : DWORD
        local    y1_pixel  : DWORD
        local    win_width : dword
        local    counter_x : dword

        push ebx
        push edi
        push esi

        cmp    [ remap ] , 0
        jz        real_out2

    ; Clip Source Rectangle against source Window boundaries.
        mov      esi , [ this_object ]        ; get ptr to src
        xor     ecx , ecx
        xor     edx , edx
        mov    edi , (GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
        mov    ebx , [ x0_pixel ]
        mov    eax , [ x0_pixel ]
        add    ebx , [ region_width ]
        shld    ecx , eax , 1
        mov    [ x1_pixel ] , ebx
        inc    edi
        shld    edx , ebx , 1
        sub    eax , edi
        sub    ebx , edi
        shld    ecx , eax , 1
        shld    edx , ebx , 1

        mov    edi,(GraphicViewPort ptr [esi]).GVPHeight ; get height into register
        mov    ebx , [ y0_pixel ]
        mov    eax , [ y0_pixel ]
        add    ebx , [ region_height ]
        shld    ecx , eax , 1
        mov    [ y1_pixel ] , ebx
        inc    edi
        shld    edx , ebx , 1
        sub    eax , edi
        sub    ebx , edi
        shld    ecx , eax , 1
        shld    edx , ebx , 1

        xor    cl , 5
        xor    dl , 5
        mov    al , cl
        test    dl , cl
        jnz    real_out2
        or    al , dl
        jz        do_remap

        test    cl , 1000b
        jz        scr_left_ok2
        mov    [ x0_pixel ] , 0

scr_left_ok2:
        test    cl , 0010b
        jz        scr_bottom_ok2
        mov    [ y0_pixel ] , 0

scr_bottom_ok2:
        test    dl , 0100b
        jz        scr_right_ok2
        mov    eax , (GraphicViewPort ptr [esi]).GVPWidth  ; get width into register
        mov    [ x1_pixel ] , eax
scr_right_ok2:
        test    dl , 0001b
        jz        do_remap
        mov    eax , (GraphicViewPort ptr [esi]).GVPHeight  ; get width into register
        mov    [ y1_pixel ] , eax


do_remap:
           cld
           mov    edi , (GraphicViewPort ptr [esi]).GVPOffset
           mov    eax , (GraphicViewPort ptr [esi]).GVPXAdd
           mov    ebx , [ x1_pixel ]
           add    eax , (GraphicViewPort ptr [esi]).GVPWidth
           add    eax , (GraphicViewPort ptr [esi]).GVPPitch
           mov    esi , eax
           mul    [ y0_pixel ]
           add    edi , [ x0_pixel ]
           sub    ebx , [ x0_pixel ]
           jle    real_out2
           add    edi , eax
           sub    esi , ebx

           mov    ecx , [ y1_pixel ]
           sub    ecx , [ y0_pixel ]
           jle    real_out2
           mov    eax , [ remap ]
           mov    [ counter_x ] , ebx
           xor    edx , edx

outer_loop2:
           mov    ebx , [ counter_x ]
inner_loop2:
           mov    dl , [ edi ]
           mov    dl , [ eax + edx ]
           mov    [ edi ] , dl
           inc    edi
           dec    ebx
           jnz    inner_loop2
           add    edi , esi
           dec    ecx
           jnz    outer_loop2
real_out2:
           pop esi
           pop edi
           pop ebx
           ret
Buffer_Remap endp

;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Westwood Library                         *
;*                                                                         *
;*                    File Name : FADING.ASM                               *
;*                                                                         *
;*                   Programmer : Joe L. Bostic                            *
;*                                                                         *
;*                   Start Date : August 20, 1993                          *
;*                                                                         *
;*                  Last Update : August 20, 1993   [JLB]                  *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *

;***********************************************************
; BUILD_FADING_TABLE
;
; void *Build_Fading_Table(void *palette, void *dest, long int color, long int frac);
;
; This routine will create the fading effect table used to coerce colors
; from toward a common value.  This table is used when Fading_Effect is
; active.
;
; Bounds Checking: None
;*
;void* __cdecl Build_Fading_Table(void const* palette, void const* dest, long int color, long int frac)
Build_Fading_Table proc C palette:dword, dest:dword, color:dword, frac:dword
    LOCAL	matchvalue:DWORD	; Last recorded match value.
    LOCAL	targetred:BYTE		; Target gun red.
    LOCAL	targetgreen:BYTE	; Target gun green.
    LOCAL	targetblue:BYTE		; Target gun blue.
    LOCAL	idealred:BYTE
    LOCAL	idealgreen:BYTE
    LOCAL	idealblue:BYTE
    LOCAL	matchcolor:BYTE		; Tentative match color.

    push ebx
    push edi
    push esi

        cld

        ; If the source palette is NULL, then just return with current fading table pointer.
        cmp    [palette],0
        je    fini
        cmp    [dest],0
        je    fini

        ; Fractions above 255 become 255.
        mov    eax,[frac]
        cmp    eax,0100h
        jb    short ok
        mov    [frac],0FFh
    ok:

        ; Record the target gun values.
        mov    esi,[palette]
        mov    ebx,[color]
        add    esi,ebx
        add    esi,ebx
        add    esi,ebx
        lodsb
        mov    [targetred],al
        lodsb
        mov    [targetgreen],al
        lodsb
        mov    [targetblue],al

        ; Main loop.
        xor    ebx,ebx            ; Remap table index.

        ; Transparent black never gets remapped.
        mov    edi,[dest]
        mov    [edi],bl
        inc    edi

        ; EBX = source palette logical number (1..255).
        ; EDI = running pointer into dest remap table.
    mainloop:
        inc    ebx
        mov    esi,[palette]
        add    esi,ebx
        add    esi,ebx
        add    esi,ebx

        mov    edx,[frac]
        shr    edx,1
        ; new = orig - ((orig-target) * fraction);

        lodsb                ; orig
        mov    dh,al            ; preserve it for later.
        sub    al,[targetred]        ; al = (orig-target)
        imul    dl            ; ax = (orig-target)*fraction
        shl    ax,1
        sub    dh,ah            ; dh = orig - ((orig-target) * fraction)
        mov    [idealred],dh        ; preserve ideal color gun value.

        lodsb                ; orig
        mov    dh,al            ; preserve it for later.
        sub    al,[targetgreen]    ; al = (orig-target)
        imul    dl            ; ax = (orig-target)*fraction
        shl    ax,1
        sub    dh,ah            ; dh = orig - ((orig-target) * fraction)
        mov    [idealgreen],dh        ; preserve ideal color gun value.

        lodsb                ; orig
        mov    dh,al            ; preserve it for later.
        sub    al,[targetblue]        ; al = (orig-target)
        imul    dl            ; ax = (orig-target)*fraction
        shl    ax,1
        sub    dh,ah            ; dh = orig - ((orig-target) * fraction)
        mov    [idealblue],dh        ; preserve ideal color gun value.

        ; Sweep through the entire existing palette to find the closest
        ; matching color.  Never matches with color 0.

        mov    eax,[color]
        mov    [matchcolor],al        ; Default color (self).
        mov    [matchvalue],-1        ; Ridiculous match value init.
        mov    ecx,255

        mov    esi,[palette]        ; Pointer to original palette.
        add    esi,3

        ; BH = color index.
        mov    bh,1
    innerloop:

        ; Recursion through the fading table won't work if a color is allowed
        ; to remap to itself.  Prevent this from occuring.
        add    esi,3
        cmp    bh,bl
        je    short notclose
        sub    esi,3

        xor    edx,edx            ; Comparison value starts null.
        mov    eax,edx
        ; Build the comparison value based on the sum of the differences of the color
        ; guns squared.
        lodsb
        sub    al,[idealred]
        mov    ah,al
        imul    ah
        add    edx,eax

        lodsb
        sub    al,[idealgreen]
        mov    ah,al
        imul    ah
        add    edx,eax

        lodsb
        sub    al,[idealblue]
        mov    ah,al
        imul    ah
        add    edx,eax
        jz    short perfect        ; If perfect match found then quit early.

        cmp    edx,[matchvalue]
        ja    short notclose
        mov    [matchvalue],edx    ; Record new possible color.
        mov    [matchcolor],bh
    notclose:
        inc    bh            ; Checking color index.
        loop    innerloop
        mov    bh,[matchcolor]
    perfect:
        mov    [matchcolor],bh
        xor    bh,bh            ; Make BX valid main index again.

        ; When the loop exits, we have found the closest match.
        mov    al,[matchcolor]
        stosb
        cmp    ebx,255
        jne    mainloop

    fini:
        mov    eax,[dest]
        pop esi
        pop edi
        pop ebx
        ret
Build_Fading_Table endp

;***************************************************************************
;**     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
;***************************************************************************
;*                                                                         *
;*                 Project Name : GraphicViewPortClass			   *
;*                                                                         *
;*                    File Name : PUTPIXEL.ASM                             *
;*                                                                         *
;*                   Programmer : Phil Gorrow				   *
;*                                                                         *
;*                   Start Date : June 7, 1994				   *
;*                                                                         *
;*                  Last Update : June 8, 1994   [PWG]                     *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;*   VVPC::Put_Pixel -- Puts a pixel on a virtual viewport                 *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *

;***************************************************************************
;* VVPC::PUT_PIXEL -- Puts a pixel on a virtual viewport                   *
;*                                                                         *
;* INPUT:	WORD the x position for the pixel relative to the upper    *
;*			left corner of the viewport			   *
;*		WORD the y pos for the pixel relative to the upper left	   *
;*			corner of the viewport				   *
;*		UBYTE the color of the pixel to write			   *
;*                                                                         *
;* OUTPUT:      none                                                       *
;*                                                                         *
;* WARNING:	If pixel is to be placed outside of the viewport then	   *
;*		this routine will abort.				   *
;*									   *
;* HISTORY:                                                                *
;*   06/08/1994 PWG : Created.                                             *
;*=========================================================================*
;void __cdecl Buffer_Put_Pixel(void* this_object, int x_pixel, int y_pixel, unsigned char color)
Buffer_Put_Pixel proc C this_object:dword, x_pixel:dword, y_pixel:dword, color:byte
            push ebx
            push edi

            ;*===================================================================
            ; Get the viewport information and put bytes per row in ecx
            ;*===================================================================
            mov    ebx,[this_object]                ; get a pointer to viewport
            xor    eax,eax
            mov    edi,(GraphicViewPort ptr [ebx]).GVPOffset    ; get the correct offset
            mov    ecx,(GraphicViewPort ptr [ebx]).GVPHeight    ; edx = height of viewport
            mov    edx,(GraphicViewPort ptr [ebx]).GVPWidth    ; ecx = width of viewport

            ;*===================================================================
            ; Verify that the X pixel offset if legal
            ;*===================================================================
            mov    eax,[x_pixel]                ; find the x position
            cmp    eax,edx                    ;   is it out of bounds
            jae    short putpix_done                ; if so then get out
            add    edi,eax                    ; otherwise add in offset

            ;*===================================================================
            ; Verify that the Y pixel offset if legal
            ;*===================================================================
            mov    eax,[y_pixel]                ; get the y position
            cmp    eax,ecx                    ;  is it out of bounds
            jae    putpix_done                    ; if so then get out
            add    edx,(GraphicViewPort ptr [ebx]).GVPXAdd    ; otherwise find bytes per row
            add    edx,(GraphicViewPort ptr [ebx]).GVPPitch    ; add in direct draw pitch
            mul    edx                    ; offset = bytes per row * y
            add    edi,eax                    ; add it into the offset

            ;*===================================================================
            ; Write the pixel to the screen
            ;*===================================================================
            mov    al,[color]                ; read in color value
            mov    [edi],al                ; write it to the screen
        putpix_done:
            pop edi
            pop ebx
            ret
Buffer_Put_Pixel endp

;***************************************************************************
;* VVPC::GET_PIXEL -- Gets a pixel from the current view port           *
;*                                                                         *
;* INPUT:    WORD the x pixel on the screen.                   *
;*        WORD the y pixel on the screen.                   *
;*                                                                         *
;* OUTPUT:      UBYTE the pixel at the specified location           *
;*                                                                         *
;* WARNING:    If pixel is to be placed outside of the viewport then       *
;*        this routine will abort.                   *
;*                                                                         *
;* HISTORY:                                                                *
;*   06/07/1994 PWG : Created.                                             *
;*=========================================================================*

;extern "C" int __cdecl Buffer_Get_Pixel(void* this_object, int x_pixel, int y_pixel)
Buffer_Get_Pixel proc C this_object:dword, x_pixel:dword, y_pixel:dword
        push ebx
        push edi

        ;*===================================================================
        ; Get the viewport information and put bytes per row in ecx
        ;*===================================================================
        mov    ebx,[this_object]                ; get a pointer to viewport
        xor    eax,eax
        mov    edi,(GraphicViewPort ptr [ebx]).GVPOffset    ; get the correct offset
        mov    ecx,(GraphicViewPort ptr [ebx]).GVPHeight    ; edx = height of viewport
        mov    edx,(GraphicViewPort ptr [ebx]).GVPWidth    ; ecx = width of viewport

        ;*===================================================================
        ; Verify that the X pixel offset if legal
        ;*===================================================================
        mov    eax,[x_pixel]                ; find the x position
        cmp    eax,edx                    ;   is it out of bounds
        jae    short exit_label                ; if so then get out
        add    edi,eax                    ; otherwise add in offset

        ;*===================================================================
        ; Verify that the Y pixel offset if legal
        ;*===================================================================
        mov    eax,[y_pixel]                ; get the y position
        cmp    eax,ecx                    ;  is it out of bounds
        jae    exit_label                    ; if so then get out
        add    edx,(GraphicViewPort ptr [ebx]).GVPXAdd    ; otherwise find bytes per row
        add    edx,(GraphicViewPort ptr [ebx]).GVPPitch    ; otherwise find bytes per row
        mul    edx                    ; offset = bytes per row * y
        add    edi,eax                    ; add it into the offset

        ;*===================================================================
        ; Write the pixel to the screen
        ;*===================================================================
        xor    eax,eax                    ; clear the word
        mov    al,[edi]                ; read in the pixel
    exit_label:
        pop edi
        pop ebx
        ret
Buffer_Get_Pixel endp

;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Support Library                          *
;*                                                                         *
;*                    File Name : cliprect.asm                             *
;*                                                                         *
;*                   Programmer : Julio R Jerez                            *
;*                                                                         *
;*                   Start Date : Mar, 2 1995                              *
;*                                                                         *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;* int Clip_Rect ( int * x , int * y , int * dw , int * dh , 		   *
;*	       	   int width , int height ) ;          			   *
;* int Confine_Rect ( int * x , int * y , int * dw , int * dh , 	   *
;*	       	      int width , int height ) ;          		   *
;*									   *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *

;***************************************************************************
;* Clip_Rect -- clip a given rectangle against a given window		   *
;*                                                                         *
;* INPUT:   &x , &y , &w , &h  -> Pointer to rectangle being clipped       *
;*          width , height     -> dimension of clipping window             *
;*                                                                         *
;* OUTPUT: a) Zero if the rectangle is totally contained by the 	   *
;*	      clipping window.						   *
;*	   b) A negative value if the rectangle is totally outside the     *
;*            the clipping window					   *
;*	   c) A positive value if the rectangle	was clipped against the	   *
;*	      clipping window, also the values pointed by x, y, w, h will  *
;*	      be modified to new clipped values	 			   *
;*									   *
;*   05/03/1995 JRJ : added comment                                        *
;*=========================================================================*
; int Clip_Rect (int* x, int* y, int* dw, int* dh, int width, int height);          			   *

;extern "C" int __cdecl Clip_Rect(int* x, int* y, int* w, int* h, int width, int height)
Clip_Rect proc C x:dword, y:dword, w:dword, h:dword, widt:dword, height:dword
        push esi
        push edi
        push ebx

        ;This Clipping algorithm is a derivation of the very well known
        ;Cohen-Sutherland Line-Clipping test. Due to its simplicity and efficiency
        ;it is probably the most commontly implemented algorithm both in software
        ;and hardware for clipping lines, rectangles, and convex polygons against
        ;a rectagular clipping window. For reference see
        ;"COMPUTER GRAPHICS principles and practice by Foley, Vandam, Feiner, Hughes
        ; pages 113 to 177".
        ; Briefly consist in computing the Sutherland code for both end point of
        ; the rectangle to find out if the rectangle is:
        ; - trivially accepted (no further clipping test, return the oroginal data)
        ; - trivially rejected (return with no action, return error code)
        ; - retangle must be iteratively clipped again edges of the clipping window
        ;   and return the clipped rectangle

            ; get all four pointer into regisnters
            mov    esi,[x]        ; esi = pointer to x
            mov    edi,[y]        ; edi = pointer to x
            mov    eax,[w]        ; eax = pointer to dw
            mov    ebx,[h]        ; ebx = pointer to dh

            ; load the actual data into reg
            mov    esi,[esi]    ; esi = x0
            mov    edi,[edi]    ; edi = y0
            mov    eax,[eax]    ; eax = dw
            mov    ebx,[ebx]    ; ebx = dh

            ; create a wire frame of the type [x0,y0] , [x1,y1]
            add    eax,esi        ; eax = x1 = x0 + dw
            add    ebx,edi        ; ebx = y1 = y0 + dh

            ; we start we suthenland code0 and code1 set to zero
            xor     ecx,ecx        ; cl = sutherland boolean code0
            xor     edx,edx        ; dl = sutherland boolean code0

            ; now we start computing the to suthenland boolean code for x0 , x1
            shld    ecx,esi,1    ; bit3 of code0 = sign bit of (x0 - 0)
            shld    edx,eax,1     ; bit3 of code1 = sign bit of (x1 - 0)
            sub    esi,[widt]    ; get the difference (x0 - (width + 1))
            sub    eax,[widt]    ; get the difference (x1 - (width + 1))
            dec    esi
            dec    eax
            shld    ecx,esi,1    ; bit2 of code0 = sign bit of (x0 - (width + 1))
            shld    edx,eax,1    ; bit2 of code1 = sign bit of (x0 - (width + 1))

            ; now we start computing the to suthenland boolean code for y0 , y1
            shld    ecx,edi,1       ; bit1 of code0 = sign bit of (y0 - 0)
            shld    edx,ebx,1    ; bit1 of code1 = sign bit of (y0 - 0)
            sub    edi,[height]    ; get the difference (y0 - (height + 1))
            sub    ebx,[height]    ; get the difference (y1 - (height + 1))
            dec    edi
            dec    ebx
            shld    ecx,edi,1    ; bit0 of code0 = sign bit of (y0 - (height + 1))
            shld    edx,ebx,1    ; bit0 of code1 = sign bit of (y1 - (height + 1))

            ; Bit 2 and 0 of cl and bl are complemented
            xor    cl,5        ; reverse bit2 and bit0 in code0
            xor    dl,5         ; reverse bit2 and bit0 in code1

            ; now perform the rejection test
            mov    eax,-1        ; set return code to false
            mov    bl,cl         ; save code0 for future use
            test    dl,cl          ; if any two pair of bit in code0 and code1 is set
            jnz    clip_out    ; then rectangle is outside the window

            ; now perform the aceptance test
            xor    eax,eax        ; set return code to true
            or    bl,dl        ; if all pair of bits in code0 and code1 are reset
            jz    clip_out    ; then rectangle is insize the window.                                      '

            ; we need to clip the rectangle iteratively
            mov    eax,-1        ; set return code to false
            test    cl,1000b    ; if bit3 of code0 is set then the rectangle
            jz    left_ok    ; spill out the left edge of the window
            mov    edi,[x]        ; edi = a pointer to x0
            mov    ebx,[w]        ; ebx = a pointer to dw
            mov    esi,[edi]    ; esi = x0
            mov    [dword ptr edi],0 ; set x0 to 0 "this the left edge value"
            add    [ebx],esi    ; adjust dw by x0, since x0 must be negative

        left_ok:
            test    cl,0010b    ; if bit1 of code0 is set then the rectangle
            jz    bottom_ok    ; spill out the bottom edge of the window
            mov    edi,[y]        ; edi = a pointer to y0
            mov    ebx,[h]        ; ebx = a pointer to dh
            mov    esi,[edi]    ; esi = y0
            mov    [dword ptr edi],0 ; set y0 to 0 "this the bottom edge value"
            add    [ebx],esi    ; adjust dh by y0, since y0 must be negative

        bottom_ok:
            test    dl,0100b    ; if bit2 of code1 is set then the rectangle
            jz    right_ok    ; spill out the right edge of the window
            mov    edi,[w]     ; edi = a pointer to dw
            mov    esi,[x]        ; esi = a pointer to x
            mov    ebx,[widt]    ; ebx = the width of the window
            sub    ebx,[esi]     ; the new dw is the difference (width-x0)
            mov    [edi],ebx    ; adjust dw to (width - x0)
            jle    clip_out    ; if (width-x0) = 0 then the clipped retangle
                        ; has no width we are done
        right_ok:
            test    dl,0001b    ; if bit0 of code1 is set then the rectangle
            jz    clip_ok    ; spill out the top edge of the window
            mov    edi,[h]     ; edi = a pointer to dh
            mov    esi,[y]        ; esi = a pointer to y0
            mov    ebx,[height]    ; ebx = the height of the window
            sub    ebx,[esi]     ; the new dh is the difference (height-y0)
            mov    [edi],ebx    ; adjust dh to (height-y0)
            jle    clip_out    ; if (width-x0) = 0 then the clipped retangle
                        ; has no width we are done
        clip_ok:
            mov    eax,1      ; signal the calling program that the rectangle was modify
        clip_out:
            pop ebx
            pop edi
            pop esi
            ret
Clip_Rect endp

;***************************************************************************
;* Confine_Rect -- clip a given rectangle against a given window       *
;*                                                                         *
;* INPUT:   &x,&y,w,h    -> Pointer to rectangle being clipped       *
;*          width,height     -> dimension of clipping window             *
;*                                                                         *
;* OUTPUT: a) Zero if the rectangle is totally contained by the        *
;*          clipping window.                           *
;*       c) A positive value if the rectangle    was shifted in position    *
;*          to fix inside the clipping window, also the values pointed   *
;*          by x, y, will adjusted to a new values                *
;*                                       *
;*  NOTE:  this function make not attempt to verify if the rectangle is       *
;*       bigger than the clipping window and at the same time wrap around*
;*       it. If that is the case the result is meaningless           *
;*=========================================================================*
; int Confine_Rect (int* x, int* y, int dw, int dh, int width, int height);                         *
;extern "C" int __cdecl Confine_Rect(int* x, int* y, int w, int h, int width, int height)
Confine_Rect proc C x:dword, y:dword, w:dword, h:dword, widt:dword, height:dword
            push ebx
            push edi
            push esi
    
            xor    eax,eax
            mov    ebx,[x]
            mov    edi,[w]

            mov    esi,[ebx]
            add    edi,[ebx]

            sub    edi,[widt]
            neg    esi
            dec    edi

            test    esi,edi
            jl    x_axix_ok
            mov    eax,1

            test    esi,esi
            jl    shift_right
            mov    [dword ptr ebx],0
            jmp    x_axix_ok
        shift_right:
            inc    edi
            sub    [ebx],edi
        x_axix_ok:
            mov    ebx,[y]
            mov    edi,[h]

            mov    esi,[ebx]
            add    edi,[ebx]

            sub    edi,[height]
            neg    esi
            dec    edi

            test    esi,edi
            jl    confi_out
            mov    eax,1

            test    esi,esi
            jl    shift_top
            mov    [dword ptr ebx],0

            jmp confi_out
        shift_top:
            inc    edi
            sub    [ebx],edi
        confi_out:
            pop esi
            pop edi
            pop ebx
            ret
Confine_Rect endp

end
