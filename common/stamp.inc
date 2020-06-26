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

;
; Note: this holds the common code for stamps, IControl_Type is game specific and this is an include file
;

.model flat

include graphicviewport.inc

externdef C Init_Stamps:near
externdef C Buffer_Draw_Stamp_Clip:near
externdef C LastIconset:dword
externdef C StampPtr:dword
externdef C IsTrans:dword
externdef C MapPtr:dword
externdef C IconWidth:dword
externdef C IconHeight:dword
externdef C IconSize:dword
externdef C IconCount:dword

.data

LastIconset dd 0
StampPtr    dd 0 ; Pointer to icon data.
IsTrans     dd 0 ; Pointer to transparent icon flag table.
MapPtr      dd 0 ; Pointer to icon map.
IconWidth   dd 0 ; Width of icon in pixels.
IconHeight  dd 0 ; Height of icon in pixels.
IconSize    dd 0 ; Number of bytes for each icon data.
IconCount   dd 0 ; Number of icons in the set.

.code

;***********************************************************
; INIT_STAMPS
;
; VOID cdecl Init_Stamps(VOID *icondata);
;
; This routine initializes the stamp data.
; Bounds Checking: NONE
;
;*
;extern "C" void __cdecl Init_Stamps(unsigned int icondata)
Init_Stamps proc C icondata:dword
    push edi
    push ebx

    ; Verify legality of parameter.
    cmp    [icondata],0
    je    short init_stamps_fini

    ; Don't initialize if already initialized to this set (speed reasons).
    mov    edi,[icondata]
    cmp    [LastIconset],edi
    je    short init_stamps_fini
    mov    [LastIconset],edi

    ; Record number of icons in set.
    movzx    eax,(IControl_Type ptr [edi]).Count
    mov    [IconCount],eax

    ; Record width of icon.
    movzx    eax,(IControl_Type ptr [edi])._Width
    mov    [IconWidth],eax

    ; Record height of icon.
    movzx    ebx,(IControl_Type ptr [edi]).Height
    mov    [IconHeight],ebx

    ; Record size of icon (in bytes).
    mul    ebx
    mov    [IconSize],eax

    ; Record hard pointer to icon map data.
    mov    eax,(IControl_Type ptr [edi]).Map
    add    eax,edi
    mov    [MapPtr],eax

    ;nomap:
    ; Record hard pointer to icon data.
    mov    eax,edi
    add    eax,(IControl_Type ptr [edi]).Icons
    mov    [StampPtr],eax

    ; Record the transparent table.
    mov    eax,edi
    add    eax,(IControl_Type ptr [edi]).TransFlag
    mov    [IsTrans],eax

init_stamps_fini:
    pop edi
    pop ebx
    ret
Init_Stamps endp

;***********************************************************
; DRAW_STAMP_CLIP
;
; VOID cdecl MCGA_Draw_Stamp_Clip(VOID *icondata, WORD icon, WORD x_pixel, WORD y_pixel, VOID *remap, LONG min_x, LONG min_y, LONG max_x, LONG max_y);
;
; This routine renders the icon at the given coordinate.
;
; The remap table is a 256 byte simple pixel translation table to use when
; drawing the icon.  Transparency check is performed AFTER the remap so it is possible to
; remap valid colors to be invisible (for special effect reasons).
; This routine is fastest when no remap table is passed in.
;*
;void __cdecl Buffer_Draw_Stamp_Clip(void const* this_object, void const* icondata, int icon, int x_pixel, int y_pixel, void const* remap, int min_x, int min_y, int max_x, int max_y)
Buffer_Draw_Stamp_Clip proc C this_object:dword, icondata:dword, icon:dword, x_pixel:dword, y_pixel:dword, remap:dword, min_x:dword, min_y:dword, max_x:dword, max_y:dword
        LOCAL    modulo:DWORD        ; Modulo to get to next row.
        LOCAL    iwidth:DWORD        ; Icon width (here for speedy access).
        LOCAL    skip:DWORD        ; amount to skip per row of icon data
        LOCAL    doremap:BYTE        ; Should remapping occur?

            pushad
            cmp    [icondata],0
            je    proc_out

            ; Initialize the stamp data if necessary.
            mov    eax,[icondata]
            cmp    [LastIconset],eax
            je        short noreset2
            push    eax
            call    Init_Stamps
            pop    eax                         ; Clean up stack. ST - 12/20/2018 10:42AM
noreset2:

            ; Determine if the icon number requested is actually in the set.
            ; Perform the logical icon to actual icon number remap if necessary.
            mov    ebx,[icon]
            cmp    [MapPtr],0
            je    short notmap2
            mov    edi,[MapPtr]
            mov    bl,[edi+ebx]
notmap2:
            cmp    ebx,[IconCount]
            jae    proc_out
            mov    [icon],ebx        ; Updated icon number.

            ; Setup some working variables.
            mov    ecx,[IconHeight]    ; Row counter.
            mov    eax,[IconWidth]
            mov    [iwidth],eax        ; Stack copy of byte width for easy BP access.

            ; Fetch pointer to start of icon's data.  ESI = ptr to icon data.
            mov    eax,[icon]
            mul    [IconSize]
            mov    esi,[StampPtr]
            add    esi,eax

            ; Update the clipping window coordinates to be valid maxes instead of width & height
            ; , and change the coordinates to be window-relative
            mov    ebx,[min_x]
            add    [max_x],ebx
            add    [x_pixel],ebx        ; make it window-relative
            mov    ebx,[min_y]
            add    [max_y],ebx
            add    [y_pixel],ebx        ; make it window-relative

            ; See if the icon is within the clipping window
            ; First, verify that the icon position is less than the maximums
            mov    ebx,[x_pixel]
            cmp    ebx,[max_x]
            jge    proc_out
            mov    ebx,[y_pixel]
            cmp    ebx,[max_y]
            jge    proc_out
            ; Now verify that the icon position is >= the minimums
            add    ebx,[IconHeight]
            cmp    ebx,[min_y]
            jle    proc_out
            mov    ebx,[x_pixel]
            add    ebx,[IconWidth]
            cmp    ebx,[min_x]
            jle    proc_out

            ; Now, clip the x, y, width, and height variables to be within the
            ; clipping rectangle
            mov    ebx,[x_pixel]
            cmp    ebx,[min_x]
            jge    nominxclip
            ; x < minx, so must clip
            mov    ebx,[min_x]
            sub    ebx,[x_pixel]
            add    esi,ebx        ; source ptr += (minx - x)
            sub    [iwidth],ebx    ; icon width -= (minx - x)
            mov    ebx,[min_x]
            mov    [x_pixel],ebx

nominxclip:
            mov    eax,[IconWidth]
            sub    eax,[iwidth]
            mov    [skip],eax

            ; Check for x+width > max_x
            mov    eax,[x_pixel]
            add    eax,[iwidth]
            cmp    eax,[max_x]
            jle    nomaxxclip
            ; x+width is greater than max_x, so must clip width down
            mov    eax,[iwidth]    ; eax = old width
            mov    ebx,[max_x]
            sub    ebx,[x_pixel]
            mov    [iwidth],ebx    ; iwidth = max_x - xpixel
            sub    eax,ebx
            add    [skip],eax    ; skip += (old width - iwidth)
nomaxxclip:
            ; check if y < miny
            mov    eax,[min_y]
            cmp    eax,[y_pixel]    ; if(miny <= y_pixel), no clip needed
            jle    nominyclip
            sub    eax,[y_pixel]
            sub    ecx,eax        ; height -= (miny - y)
            mul    [IconWidth]
            add    esi,eax        ; icon source ptr += (width * (miny - y))
            mov    eax,[min_y]
            mov    [y_pixel],eax    ; y = miny
nominyclip:
            ; check if (y+height) > max y
            mov    eax,[y_pixel]
            add    eax,ecx
            cmp    eax,[max_y]    ; if (y + height <= max_y), no clip needed
            jle    nomaxyclip
            mov    ecx,[max_y]    ; height = max_y - y_pixel
            sub    ecx,[y_pixel]
nomaxyclip:

            ; If the remap table pointer passed in is NULL, then flag this condition
            ; so that the faster (non-remapping) icon draw loop will be used.
            cmp    [remap],0
            setne    [doremap]

            ; Get pointer to position to render icon. EDI = ptr to destination page.
            mov    ebx,[this_object]
            mov    edi,(GraphicViewPort ptr [ebx]).GVPOffset
            mov    eax,(GraphicViewPort ptr [ebx]).GVPWidth
            add    eax,(GraphicViewPort ptr [ebx]).GVPXAdd
            add    eax,(GraphicViewPort ptr [ebx]).GVPPitch
            push    eax            ; save viewport full width for lower
            mul    [y_pixel]
            add    edi,eax
            add    edi,[x_pixel]

            ; Determine row modulo for advancing to next line.
            pop    eax            ; retrieve viewport width
            sub    eax,[iwidth]
            mov    [modulo],eax

            ; Determine whether simple icon draw is sufficient or whether the
            ; extra remapping icon draw is needed.
            cmp    byte ptr [doremap],0
            je    short istranscheck2

            ;************************************************************
            ; Complex icon draw -- extended remap.
            ; EBX = Palette pointer (ready for XLAT instruction).
            ; EDI = Pointer to icon destination in page.
            ; ESI = Pointer to icon data.
            ; ECX = Number of pixel rows.
            mov    ebx,[remap]
            xor    eax,eax
xrowloopc:
            push    ecx
            mov    ecx,[iwidth]

xcolumnloopc:
            lodsb
            xlatb
            or    al,al
            jz    short xskip1c        ; Transparency skip check.
            mov    [edi],al
xskip1c:
            inc    edi
            loop    xcolumnloopc

            pop    ecx
            add    edi,[modulo]
            add esi,[skip]
            loop    xrowloopc
            jmp    short proc_out


            ;************************************************************
            ; Check to see if transparent or generic draw is necessary.
istranscheck2:
            mov    ebx,[IsTrans]
            add    ebx,[icon]
            cmp    byte ptr [ebx],0
            jne    short rowloopc

            ;************************************************************
            ; Fast non-transparent icon draw routine.
            ; ES:DI = Pointer to icon destination in page.
            ; DS:SI = Pointer to icon data.
            ; CX = Number of pixel rows.
            mov    ebx,ecx
            mov    edx,[modulo]
            mov    eax,[iwidth]

            ;
            ; Optimise copy by dword aligning the destination
            ;
loop1c:
            push    eax
            rept 3
            test    edi,3
            jz    aligned
            movsb
            dec    eax
            jz    finishedit
            endm
aligned:
            mov    ecx,eax
            shr    ecx,2
            rep    movsd
            mov    ecx,eax
            and    ecx,3
            rep    movsb

finishedit:
            add    edi,edx
            add    esi,[skip]
            pop    eax

            dec    ebx
            jnz    loop1c
            jmp    short proc_out

            ;************************************************************
            ; Transparent icon draw routine -- no extended remap.
            ; ES:DI = Pointer to icon destination in page.
            ; DS:SI = Pointer to icon data.
            ; CX = Number of pixel rows.
rowloopc:
            push    ecx
            mov    ecx,[iwidth]

columnloopc:
            lodsb
            or    al,al
            jz    short skip1c        ; Transparency check.
            mov    [edi],al
skip1c:
            inc    edi
            loop    columnloopc

            pop    ecx
            add    edi,[modulo]
            add esi,[skip]
            loop    rowloopc

            ; Cleanup and exit icon drawing routine.
proc_out:
            popad
            ret

Buffer_Draw_Stamp_Clip endp
