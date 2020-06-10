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

externdef C Init_Stamps:near
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

