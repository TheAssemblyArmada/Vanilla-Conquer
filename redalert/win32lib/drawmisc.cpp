//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

/*
**
**
**  Misc asm functions from ww lib
**  ST - 12/19/2018 1:20PM
**
**
**
**
**
**
**
**
**
**
**
*/

#include "gbuffer.h"
#include <tile.h>
#include "MISC.H"
#include "WSA.H"
#include "common/drawmisc.h"

extern "C" void __cdecl Set_Font_Palette_Range(void const* palette, INT start_idx, INT end_idx)
{
}

/*
;***********************************************************

;***********************************************************
; DRAW_STAMP
;
; VOID cdecl Buffer_Draw_Stamp(VOID *icondata, WORD icon, WORD x_pixel, WORD y_pixel, VOID *remap);
;
; This routine renders the icon at the given coordinate.
;
; The remap table is a 256 byte simple pixel translation table to use when
; drawing the icon.  Transparency check is performed AFTER the remap so it is possible to
; remap valid colors to be invisible (for special effect reasons).
; This routine is fastest when no remap table is passed in.
;*
*/

void __cdecl Buffer_Draw_Stamp(void const* this_object,
                               void const* icondata,
                               int icon,
                               int x_pixel,
                               int y_pixel,
                               void const* remap)
{
    unsigned int modulo = 0;
    unsigned int iwidth = 0;
    unsigned char doremap = 0;

    /*
            PROC	Buffer_Draw_Stamp C near

            ARG	this_object:DWORD		; this is a member function
            ARG	icondata:DWORD		; Pointer to icondata.
            ARG	icon:DWORD		; Icon number to draw.
            ARG	x_pixel:DWORD		; X coordinate of icon.
            ARG	y_pixel:DWORD		; Y coordinate of icon.
            ARG	remap:DWORD 		; Remap table.

            LOCAL	modulo:DWORD		; Modulo to get to next row.
            LOCAL	iwidth:DWORD		; Icon width (here for speedy access).
            LOCAL	doremap:BYTE		; Should remapping occur?
    */

    __asm {

			pushad
			cmp	[icondata],0
			je	proc_out

			; Initialize the stamp data if necessary.
			mov	eax,[icondata]
			cmp	[LastIconset],eax
			je		short noreset
			push	eax
			call	Init_Stamps
			pop	eax			             // Clean up stack. ST - 12/20/2018 10:42AM
noreset:

			; Determine if the icon number requested is actually in the set.
			; Perform the logical icon to actual icon number remap if necessary.
			mov	ebx,[icon]
			cmp	[MapPtr],0
			je	short notmap
			mov	edi,[MapPtr]
			mov	bl,[edi+ebx]
notmap:
			cmp	ebx,[IconCount]
			jae	proc_out
			mov	[icon],ebx		; Updated icon number.

			; If the remap table pointer passed in is NULL, then flag this condition
			; so that the faster (non-remapping) icon draw loop will be used.
			cmp	[remap],0
			setne	[doremap]

			; Get pointer to position to render icon. EDI = ptr to destination page.
			mov	ebx,[this_object]
			mov	edi,[ebx]GraphicViewPortClass.Offset
			mov	eax,[ebx]GraphicViewPortClass.Width
			add	eax,[ebx]GraphicViewPortClass.XAdd
			add	eax,[ebx]GraphicViewPortClass.Pitch
			push	eax			; save viewport full width for lower
			mul	[y_pixel]
			add	edi,eax
			add	edi,[x_pixel]

			; Determine row modulo for advancing to next line.
			pop	eax			; retrieve viewport width
			sub	eax,[IconWidth]
			mov	[modulo],eax

			; Setup some working variables.
			mov	ecx,[IconHeight]	; Row counter.
			mov	eax,[IconWidth]
			mov	[iwidth],eax		; Stack copy of byte width for easy BP access.

			; Fetch pointer to start of icon's data.  ESI = ptr to icon data.
			mov	eax,[icon]
			mul	[IconSize]
			mov	esi,[StampPtr]
			add	esi,eax

			; Determine whether simple icon draw is sufficient or whether the
			; extra remapping icon draw is needed.
			cmp	[BYTE PTR doremap],0
			je	short istranscheck

			;************************************************************
			; Complex icon draw -- extended remap.
			; EBX = Palette pointer (ready for XLAT instruction).
			; EDI = Pointer to icon destination in page.
			; ESI = Pointer to icon data.
			; ECX = Number of pixel rows.
		;;;	mov	edx,[remap]
		 mov ebx,[remap]
			xor	eax,eax
xrowloop:
			push	ecx
			mov	ecx,[iwidth]

xcolumnloop:
			lodsb
		;;;	mov	ebx,edx
		;;;	add	ebx,eax
		;;;	mov	al,[ebx]		; New real color to draw.
		 xlatb
			or	al,al
			jz	short xskip1		; Transparency skip check.
			mov	[edi],al
xskip1:
			inc	edi
			loop	xcolumnloop

			pop	ecx
			add	edi,[modulo]
			loop	xrowloop
			jmp	short proc_out


			;************************************************************
			; Check to see if transparent or generic draw is necessary.
istranscheck:
			mov	ebx,[IsTrans]
			add	ebx,[icon]
			cmp	[BYTE PTR ebx],0
			jne	short rowloop

			;************************************************************
			; Fast non-transparent icon draw routine.
			; ES:DI = Pointer to icon destination in page.
			; DS:SI = Pointer to icon data.
			; CX = Number of pixel rows.
			mov	ebx,ecx
			shr	ebx,2
			mov	edx,[modulo]
			mov	eax,[iwidth]
			shr	eax,2
loop1:
			mov	ecx,eax
			rep movsd
			add	edi,edx

			mov	ecx,eax
			rep movsd
			add	edi,edx

			mov	ecx,eax
			rep movsd
			add	edi,edx

			mov	ecx,eax
			rep movsd
			add	edi,edx

			dec	ebx
			jnz	loop1
			jmp	short proc_out

			;************************************************************
			; Transparent icon draw routine -- no extended remap.
			; ES:DI = Pointer to icon destination in page.
			; DS:SI = Pointer to icon data.
			; CX = Number of pixel rows.
rowloop:
			push	ecx
			mov	ecx,[iwidth]

columnloop:
			lodsb
			or	al,al
			jz	short skip1		; Transparency check.
			mov	[edi],al
skip1:
			inc	edi
			loop	columnloop

			pop	ecx
			add	edi,[modulo]
			loop	rowloop

			; Cleanup and exit icon drawing routine.
proc_out:
			popad
			//ret
	}
}

VOID __cdecl Buffer_Draw_Line(void* thisptr, int sx, int sy, int dx, int dy, unsigned char color);
VOID __cdecl Buffer_Fill_Rect(void* thisptr, int sx, int sy, int dx, int dy, unsigned char color);
VOID __cdecl Buffer_Remap(void* thisptr, int sx, int sy, int width, int height, void* remap);
VOID __cdecl Buffer_Fill_Quad(void* thisptr,
                              VOID* span_buff,
                              int x0,
                              int y0,
                              int x1,
                              int y1,
                              int x2,
                              int y2,
                              int x3,
                              int y3,
                              int color);
void __cdecl Buffer_Draw_Stamp(void const* thisptr,
                               void const* icondata,
                               int icon,
                               int x_pixel,
                               int y_pixel,
                               void const* remap);
void __cdecl Buffer_Draw_Stamp_Clip(void const* thisptr,
                                    void const* icondata,
                                    int icon,
                                    int x_pixel,
                                    int y_pixel,
                                    void const* remap,
                                    int,
                                    int,
                                    int,
                                    int);
void* __cdecl Get_Font_Palette_Ptr(void);

/*
; **************************************************************************
; **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   *
; **************************************************************************
; *                                                                        *
; *                 Project Name : WSA Support routines			   *
; *                                                                        *
; *                    File Name : XORDELTA.ASM                            *
; *                                                                        *
; *                   Programmer : Scott K. Bowen			   *
; *                                                                        *
; *                  Last Update :May 23, 1994   [SKB]                     *
; *                                                                        *
; *------------------------------------------------------------------------*
; * Functions:                                                             *
;*   Apply_XOR_Delta -- Apply XOR delta data to a buffer.                  *
;*   Apply_XOR_Delta_To_Page_Or_Viewport -- Calls the copy or the XOR funti*
;*   Copy_Delta_buffer -- Copies XOR Delta Data to a section of a page.    *
;*   XOR_Delta_Buffer -- Xor's the data in a XOR Delta format to a page.   *
; * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*

IDEAL
P386
MODEL USE32 FLAT
*/

/*
LOCALS ??

; These are used to call Apply_XOR_Delta_To_Page_Or_Viewport() to setup flags parameter.  If
; These change, make sure and change their values in wsa.cpp.
DO_XOR		equ	0
DO_COPY		equ	1
TO_VIEWPORT	equ	0
TO_PAGE		equ	2

;
; Routines defined in this module
;
;
; UWORD Apply_XOR_Delta(UWORD page_seg, BYTE *delta_ptr);
; PUBLIC Apply_XOR_Delta_To_Page_Or_Viewport(UWORD page_seg, BYTE *delta_ptr, WORD width, WORD copy)
;
;	PROC	C XOR_Delta_Buffer
;	PROC	C Copy_Delta_Buffer
;

GLOBAL 	C Apply_XOR_Delta:NEAR
GLOBAL 	C Apply_XOR_Delta_To_Page_Or_Viewport:NEAR
*/

#define DO_XOR      0
#define DO_COPY     1
#define TO_VIEWPORT 0
#define TO_PAGE     2

void __cdecl XOR_Delta_Buffer(int nextrow);
void __cdecl Copy_Delta_Buffer(int nextrow);

/*
;***************************************************************************
;* APPLY_XOR_DELTA -- Apply XOR delta data to a linear buffer.             *
;*   AN example of this in C is at the botton of the file commented out.   *
;*                                                                         *
;* INPUT:  BYTE *target - destination buffer.                              *
;*         BYTE *delta - xor data to be delta uncompress.                  *
;*                                                                         *
;* OUTPUT:                                                                 *
;*                                                                         *
;* WARNINGS:                                                               *
;*                                                                         *
;* HISTORY:                                                                *
;*   05/23/1994 SKB : Created.                                             *
;*=========================================================================*
*/
unsigned int __cdecl Apply_XOR_Delta(char* target, char* delta)
{
    /*
    PROC	Apply_XOR_Delta C near
        USES 	ebx,ecx,edx,edi,esi
        ARG	target:DWORD 		; pointers.
        ARG	delta:DWORD		; pointers.
    */

    __asm {
		
			; Optimized for 486/pentium by rearanging instructions.
			mov	edi,[target]		; get our pointers into offset registers.
			mov	esi,[delta]

			cld				; make sure we go forward
			xor	ecx,ecx			; use cx for loop

		top_loop:
			xor	eax,eax			; clear out eax.
			mov	al,[esi]		; get delta source byte
			inc	esi

			test	al,al			; check for a SHORTDUMP ; check al incase of sign value.
			je	short_run
			js	check_others

		;
		; SHORTDUMP
		;
			mov	ecx,eax			; stick count in cx

		dump_loop:
			mov	al,[esi]		;get delta XOR byte
			xor	[edi],al		; xor that byte on the dest
			inc	esi
			inc	edi
			dec	ecx
			jnz	dump_loop
			jmp	top_loop

		;
		; SHORTRUN
		;

		short_run:
			mov	cl,[esi]		; get count
			inc	esi			; inc delta source

		do_run:
			mov	al,[esi]		; get XOR byte
			inc	esi

		run_loop:
			xor	[edi],al		; xor that byte.

			inc	edi			; go to next dest pixel
			dec	ecx			; one less to go.
			jnz	run_loop
			jmp	top_loop

		;
		; By now, we know it must be a LONGDUMP, SHORTSKIP, LONGRUN, or LONGSKIP
		;

		check_others:
			sub	eax,080h		; opcode -= 0x80
			jnz	do_skip		; if zero then get next word, otherwise use remainder.

			mov	ax,[esi]
			lea	esi,[esi+2]		; get word code in ax
			test	ax,ax			; set flags. (not 32bit register so neg flag works)
			jle	not_long_skip

		;
		; SHORTSKIP AND LONGSKIP
		;
		do_skip:
			add	edi,eax			; do the skip.
			jmp	top_loop


		not_long_skip:
			jz	stop			; long count of zero means stop
			sub	eax,08000h     		; opcode -= 0x8000
			test	eax,04000h		; is it a LONGRUN (code & 0x4000)?
			je	long_dump

		;
		; LONGRUN
		;
			sub	eax,04000h		; opcode -= 0x4000
			mov	ecx,eax			; use cx as loop count
			jmp	do_run		; jump to run code.


		;
		; LONGDUMP
		;

		long_dump:
			mov	ecx,eax			; use cx as loop count
			jmp	dump_loop		; go to the dump loop.

		stop:
	}
}

/*
;----------------------------------------------------------------------------

;***************************************************************************
;* APPLY_XOR_DELTA_To_Page_Or_Viewport -- Calls the copy or the XOR funtion.           *
;*                                                                         *
;*									   *
;* 	This funtion is call to either xor or copy XOR_Delta data onto a   *
;*	page instead of a buffer.  The routine will set up the registers   *
;*	need for the actual routines that will perform the copy or xor.	   *
;*									   *
;*	The registers are setup as follows :				   *
;*		es:edi - destination segment:offset onto page.		   *
;*		ds:esi - source buffer segment:offset of delta data.	   *
;*		dx,cx,ax - are all zeroed out before entry.		   *
;*                                                                         *
;* INPUT:                                                                  *
;*                                                                         *
;* OUTPUT:                                                                 *
;*                                                                         *
;* WARNINGS:                                                               *
;*                                                                         *
;* HISTORY:                                                                *
;*   03/09/1992  SB : Created.                                             *
;*=========================================================================*
*/

void __cdecl Apply_XOR_Delta_To_Page_Or_Viewport(void* target, void* delta, int width, int nextrow, int copy)
{
    /*
    USES 	ebx,ecx,edx,edi,esi
    ARG	target:DWORD		; pointer to the destination buffer.
    ARG	delta:DWORD		; pointer to the delta buffer.
    ARG	width:DWORD		; width of animation.
    ARG	nextrow:DWORD		; Page/Buffer width - anim width.
    ARG	copy:DWORD		; should it be copied or xor'd?
    */

    __asm {

		mov	edi,[target]		; Get the target pointer.
		mov	esi,[delta]		; Get the destination pointer.

		xor	eax,eax			; clear eax, later put them into ecx and edx.

		cld				; make sure we go forward

		mov	ebx,[nextrow]		; get the amount to add to get to next row from end.  push it later...

		mov	ecx,eax			; use cx for loop
		mov	edx,eax			; use dx to count the relative column.

		push	ebx			; push nextrow onto the stack for Copy/XOR_Delta_Buffer.
		mov	ebx,[width]		; bx will hold the max column for speed compares

	; At this point, all the registers have been set up.  Now call the correct function
	; to either copy or xor the data.

		cmp	[copy],DO_XOR		; Do we want to copy or XOR
		je	xorfunct		; Jump to XOR if not copy
		call	Copy_Delta_Buffer	; Call the function to copy the delta buffer.
		jmp	didcopy		; jump past XOR
	xorfunct:
		call	XOR_Delta_Buffer	; Call funtion to XOR the deltat buffer.
	didcopy:
		pop	ebx			; remove the push done to pass a value.
	}
}

/*
;----------------------------------------------------------------------------


;***************************************************************************
;* XOR_DELTA_BUFFER -- Xor's the data in a XOR Delta format to a page.     *
;*	This will only work right if the page has the previous data on it. *
;*	This function should only be called by XOR_Delta_Buffer_To_Page_Or_Viewport.   *
;*      The registers must be setup as follows :                           *
;*                                                                         *
;* INPUT:                                                                  *
;*	es:edi - destination segment:offset onto page.		 	   *
;*	ds:esi - source buffer segment:offset of delta data.	 	   *
;*	edx,ecx,eax - are all zeroed out before entry.		 	   *
;*                                                                         *
;* OUTPUT:                                                                 *
;*                                                                         *
;* WARNINGS:                                                               *
;*                                                                         *
;* HISTORY:                                                                *
;*   03/09/1992  SB : Created.                                             *
;*=========================================================================*
*/
void __cdecl XOR_Delta_Buffer(int nextrow)
{
    /*
    ARG	nextrow:DWORD
    */

    __asm {

		top_loop:
			xor	eax,eax			; clear out eax.
			mov	al,[esi]		; get delta source byte
			inc	esi

			test	al,al			; check for a SHORTDUMP ; check al incase of sign value.
			je	short_run
			js	check_others

		;
		; SHORTDUMP
		;
			mov	ecx,eax			; stick count in cx

		dump_loop:
			mov	al,[esi]		; get delta XOR byte
			xor	[edi],al		; xor that byte on the dest
			inc	esi
			inc	edx			; increment our count on current column
			inc	edi
			cmp	edx,ebx			; are we at the final column
			jne	end_col1		; if not the jmp over the code

			sub	edi,edx			; get our column back to the beginning.
			xor	edx,edx			; zero out our column counter
			add	edi,[nextrow]		; jump to start of next row
		end_col1:

			dec	ecx
			jnz	dump_loop
			jmp	top_loop

		;
		; SHORTRUN
		;

		short_run:
			mov	cl,[esi]		; get count
			inc	esi			; inc delta source

		do_run:
			mov	al,[esi]		; get XOR byte
			inc	esi

		run_loop:
			xor	[edi],al		; xor that byte.

			inc	edx			; increment our count on current column
			inc	edi			; go to next dest pixel
			cmp	edx,ebx			; are we at the final column
			jne	end_col2		; if not the jmp over the code

			sub	edi,ebx			; get our column back to the beginning.
			xor	edx,edx			; zero out our column counter
			add	edi,[nextrow]		; jump to start of next row
		end_col2:


			dec	ecx
			jnz	run_loop
			jmp	top_loop

		;
		; By now, we know it must be a LONGDUMP, SHORTSKIP, LONGRUN, or LONGSKIP
		;

		check_others:
			sub	eax,080h		; opcode -= 0x80
			jnz	do_skip		; if zero then get next word, otherwise use remainder.

			mov	ax,[esi]		; get word code in ax
			lea	esi,[esi+2]
			test	ax,ax			; set flags. (not 32bit register so neg flag works)
			jle	not_long_skip

		;
		; SHORTSKIP AND LONGSKIP
		;
		do_skip:
			sub	edi,edx			; go back to beginning or row.
			add	edx,eax			; incriment our count on current row
		recheck3:
			cmp	edx,ebx			; are we past the end of the row
			jb	end_col3  		; if not the jmp over the code

			sub	edx,ebx			; Subtract width from the col counter
			add	edi,[nextrow]  		; jump to start of next row
			jmp	recheck3		; jump up to see if we are at the right row
		end_col3:
			add	edi,edx			; get to correct position in row.
			jmp	top_loop


		not_long_skip:
			jz	stop			; long count of zero means stop
			sub	eax,08000h     		; opcode -= 0x8000
			test	eax,04000h		; is it a LONGRUN (code & 0x4000)?
			je	long_dump

		;
		; LONGRUN
		;
			sub	eax,04000h		; opcode -= 0x4000
			mov	ecx,eax			; use cx as loop count
			jmp	do_run		; jump to run code.


		;
		; LONGDUMP
		;

		long_dump:
			mov	ecx,eax			; use cx as loop count
			jmp	dump_loop		; go to the dump loop.

		stop:

	}
}

/*
;----------------------------------------------------------------------------


;***************************************************************************
;* COPY_DELTA_BUFFER -- Copies XOR Delta Data to a section of a page.      *
;*	This function should only be called by XOR_Delta_Buffer_To_Page_Or_Viewport.   *
;*      The registers must be setup as follows :                           *
;*                                                                         *
;* INPUT:                                                                  *
;*	es:edi - destination segment:offset onto page.		 	   *
;*	ds:esi - source buffer segment:offset of delta data.	 	   *
;*	dx,cx,ax - are all zeroed out before entry.		 	   *
;*                                                                         *
;* OUTPUT:                                                                 *
;*                                                                         *
;* WARNINGS:                                                               *
;*                                                                         *
;* HISTORY:                                                                *
;*   03/09/1992  SB : Created.                                             *
;*=========================================================================*
*/
void __cdecl Copy_Delta_Buffer(int nextrow)
{
    /*
    ARG	nextrow:DWORD
    */

    __asm {
		
		top_loop:
			xor	eax,eax			; clear out eax.
			mov	al,[esi]		; get delta source byte
			inc	esi

			test	al,al			; check for a SHORTDUMP ; check al incase of sign value.
			je	short_run
			js	check_others

		;
		; SHORTDUMP
		;
			mov	ecx,eax			; stick count in cx

		dump_loop:
			mov	al,[esi]		; get delta XOR byte

			mov	[edi],al		; store that byte on the dest

			inc	edx			; increment our count on current column
			inc	esi
			inc	edi
			cmp	edx,ebx			; are we at the final column
			jne	end_col1		; if not the jmp over the code

			sub	edi,edx			; get our column back to the beginning.
			xor	edx,edx			; zero out our column counter
			add	edi,[nextrow]		; jump to start of next row
		end_col1:

			dec	ecx
			jnz	dump_loop
			jmp	top_loop

		;
		; SHORTRUN
		;

		short_run:
			mov	cl,[esi]		; get count
			inc	esi			; inc delta source

		do_run:
			mov	al,[esi]		; get XOR byte
			inc	esi

		run_loop:
			mov	[edi],al		; store the byte (instead of XOR against current color)

			inc	edx			; increment our count on current column
			inc	edi			; go to next dest pixel
			cmp	edx,ebx			; are we at the final column
			jne	end_col2		; if not the jmp over the code

			sub	edi,ebx			; get our column back to the beginning.
			xor	edx,edx			; zero out our column counter
			add	edi,[nextrow]		; jump to start of next row
		end_col2:


			dec	ecx
			jnz	run_loop
			jmp	top_loop

		;
		; By now, we know it must be a LONGDUMP, SHORTSKIP, LONGRUN, or LONGSKIP
		;

		check_others:
			sub	eax,080h		; opcode -= 0x80
			jnz	do_skip		; if zero then get next word, otherwise use remainder.

			mov	ax,[esi]		; get word code in ax
			lea	esi,[esi+2]
			test	ax,ax			; set flags. (not 32bit register so neg flag works)
			jle	not_long_skip

		;
		; SHORTSKIP AND LONGSKIP
		;
		do_skip:
			sub	edi,edx			; go back to beginning or row.
			add	edx,eax			; incriment our count on current row
		recheck3:
			cmp	edx,ebx			; are we past the end of the row
			jb	end_col3  		; if not the jmp over the code

			sub	edx,ebx			; Subtract width from the col counter
			add	edi,[nextrow]  		; jump to start of next row
			jmp	recheck3		; jump up to see if we are at the right row
		end_col3:
			add	edi,edx			; get to correct position in row.
			jmp	top_loop


		not_long_skip:
			jz	stop			; long count of zero means stop
			sub	eax,08000h     		; opcode -= 0x8000
			test	eax,04000h		; is it a LONGRUN (code & 0x4000)?
			je	long_dump

		;
		; LONGRUN
		;
			sub	eax,04000h		; opcode -= 0x4000
			mov	ecx,eax			; use cx as loop count
			jmp	do_run		; jump to run code.


		;
		; LONGDUMP
		;

		long_dump:
			mov	ecx,eax			; use cx as loop count
			jmp	dump_loop		; go to the dump loop.

		stop:

	}
}
/*
;----------------------------------------------------------------------------
*/

/*
;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Westwood Library                         *
;*                                                                         *
;*                    File Name : PAL.ASM                                  *
;*                                                                         *
;*                   Programmer : Joe L. Bostic                            *
;*                                                                         *
;*                   Start Date : May 30, 1992                             *
;*                                                                         *
;*                  Last Update : April 27, 1994   [BR]                    *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;*   Set_Palette_Range -- Sets changed values in the palette.              *
;*   Bump_Color -- adjusts specified color in specified palette            *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
;********************** Model & Processor Directives ************************
IDEAL
P386
MODEL USE32 FLAT


;include "keyboard.inc"
FALSE = 0
TRUE  = 1

;****************************** Declarations ********************************
GLOBAL 		C Set_Palette_Range:NEAR
GLOBAL 		C Bump_Color:NEAR
GLOBAL  	C CurrentPalette:BYTE:768
GLOBAL		C PaletteTable:byte:1024


;********************************** Data ************************************
LOCALS ??

    DATASEG

CurrentPalette	DB	768 DUP(255)	; copy of current values of DAC regs
PaletteTable	DB	1024 DUP(0)

IFNDEF LIB_EXTERNS_RESOLVED
VertBlank	DW	0		; !!!! this should go away
ENDIF


;********************************** Code ************************************
    CODESEG
*/

extern "C" unsigned char CurrentPalette[768] = {255}; //	DB	768 DUP(255)	; copy of current values of DAC regs
extern "C" unsigned char PaletteTable[1024] = {0};    //	DB	1024 DUP(0)

/*
;***************************************************************************
;* SET_PALETTE_RANGE -- Sets a palette range to the new pal                *
;*                                                                         *
;* INPUT:                                                                  *
;*                                                                         *
;* OUTPUT:                                                                 *
;*                                                                         *
;* PROTO:                                                                  *
;*                                                                         *
;* WARNINGS:	This routine is optimized for changing a small number of   *
;*		colors in the palette.
;*                                                                         *
;* HISTORY:                                                                *
;*   03/07/1995 PWG : Created.                                             *
;*=========================================================================*
*/
void __cdecl Set_Palette_Range(void* palette)
{
    memcpy(CurrentPalette, palette, 768);
    Set_DD_Palette(palette);

    /*
    PROC	Set_Palette_Range C NEAR
    ARG	palette:DWORD

    GLOBAL	Set_DD_Palette_:near
    GLOBAL	Wait_Vert_Blank_:near

    pushad
    mov	esi,[palette]
    mov	ecx,768/4
    mov	edi,offset CurrentPalette
    cld
    rep	movsd
    ;call	Wait_Vert_Blank_
    mov	eax,[palette]
    push	eax
    call	Set_DD_Palette_
    pop	eax
    popad
    ret
    */
}

/*
;***************************************************************************
;* Bump_Color -- adjusts specified color in specified palette              *
;*                                                                         *
;* INPUT:                                                                  *
;*	VOID *palette	- palette to modify				   *
;*	WORD changable	- color # to change				   *
;*	WORD target	- color to bend toward				   *
;*                                                                         *
;* OUTPUT:                                                                 *
;*                                                                         *
;* WARNINGS:                                                               *
;*                                                                         *
;* HISTORY:                                                                *
;*   04/27/1994 BR : Converted to 32-bit.                                  *
;*=========================================================================*
; BOOL cdecl Bump_Color(VOID *palette, WORD changable, WORD target);
*/
BOOL __cdecl Bump_Color(void* pal, int color, int desired)
{
    /*
PROC Bump_Color C NEAR
    USES ebx,ecx,edi,esi
    ARG	pal:DWORD, color:WORD, desired:WORD
    LOCAL	changed:WORD		; Has palette changed?
     */

    short short_color = (short)color;
    short short_desired = (short)desired;
    bool changed = false;

    __asm {
		mov	edi,[pal]		; Original palette pointer.
		mov	esi,edi
		mov	eax,0
		mov	ax,[short_color]
		add	edi,eax
		add	edi,eax
		add	edi,eax			; Offset to changable color.
		mov	ax,[short_desired]
		add	esi,eax
		add	esi,eax
		add	esi,eax			; Offset to target color.

		mov	[changed],FALSE		; Presume no change.
		mov	ecx,3			; Three color guns.

		; Check the color gun.
	colorloop:
		mov	al,[BYTE PTR esi]
		sub	al,[BYTE PTR edi]	; Carry flag is set if subtraction needed.
		jz	short gotit
		mov	[changed],TRUE
		inc	[BYTE PTR edi]		; Presume addition.
		jnc	short gotit		; oops, subtraction needed so dec twice.
		dec	[BYTE PTR edi]
		dec	[BYTE PTR edi]
	gotit:
		inc	edi
		inc	esi
		loop	colorloop

		movzx	eax,[changed]
    }
}

/*
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

IDEAL
P386
MODEL USE32 FLAT

INCLUDE ".\drawbuff.inc"
INCLUDE ".\gbuffer.inc"


CODESEG
*/

/*
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
    PROC	Buffer_Put_Pixel C near
    USES	eax,ebx,ecx,edx,edi
*/

void __cdecl Buffer_Put_Pixel(void* this_object, int x_pixel, int y_pixel, unsigned char color)
{

    /*
    ARG    	this_object:DWORD				; this is a member function
    ARG	x_pixel:DWORD				; x position of pixel to set
    ARG	y_pixel:DWORD				; y position of pixel to set
    ARG    	color:BYTE				; what color should we clear to
    */

    __asm {
		
	
			;*===================================================================
			; Get the viewport information and put bytes per row in ecx
			;*===================================================================
			mov	ebx,[this_object]				; get a pointer to viewport
			xor	eax,eax
			mov	edi,[ebx]GraphicViewPortClass.Offset	; get the correct offset
			mov	ecx,[ebx]GraphicViewPortClass.Height	; edx = height of viewport
			mov	edx,[ebx]GraphicViewPortClass.Width	; ecx = width of viewport

			;*===================================================================
			; Verify that the X pixel offset if legal
			;*===================================================================
			mov	eax,[x_pixel]				; find the x position
			cmp	eax,edx					;   is it out of bounds
			jae	short done				; if so then get out
			add	edi,eax					; otherwise add in offset

			;*===================================================================
			; Verify that the Y pixel offset if legal
			;*===================================================================
			mov	eax,[y_pixel]				; get the y position
			cmp	eax,ecx					;  is it out of bounds
			jae	done					; if so then get out
			add	edx,[ebx]GraphicViewPortClass.XAdd	; otherwise find bytes per row
			add	edx,[ebx]GraphicViewPortClass.Pitch	; add in direct draw pitch
			mul	edx					; offset = bytes per row * y
			add	edi,eax					; add it into the offset

			;*===================================================================
			; Write the pixel to the screen
			;*===================================================================
			mov	al,[color]				; read in color value
			mov	[edi],al				; write it to the screen
		done:
    }
}

/*
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


IDEAL
P386
MODEL USE32 FLAT

GLOBAL	 C Clip_Rect	:NEAR
GLOBAL	 C Confine_Rect	:NEAR

CODESEG

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
*/

extern "C" int __cdecl Clip_Rect(int* x, int* y, int* w, int* h, int width, int height)
{

    /*
        PROC	Clip_Rect C near
        uses	ebx,ecx,edx,esi,edi
        arg	x:dword
        arg	y:dword
        arg	w:dword
        arg	h:dword
        arg	width:dword
        arg	height:dword
    */

    __asm {

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
			mov	esi,[x]		; esi = pointer to x
			mov	edi,[y]		; edi = pointer to x
			mov	eax,[w]		; eax = pointer to dw
			mov	ebx,[h]		; ebx = pointer to dh

			; load the actual data into reg
			mov	esi,[esi]	; esi = x0
			mov	edi,[edi]	; edi = y0
			mov	eax,[eax]	; eax = dw
			mov	ebx,[ebx]	; ebx = dh

			; create a wire frame of the type [x0,y0] , [x1,y1]
			add	eax,esi		; eax = x1 = x0 + dw
			add	ebx,edi		; ebx = y1 = y0 + dh

			; we start we suthenland code0 and code1 set to zero
			xor 	ecx,ecx		; cl = sutherland boolean code0
			xor 	edx,edx		; dl = sutherland boolean code0

			; now we start computing the to suthenland boolean code for x0 , x1
			shld	ecx,esi,1	; bit3 of code0 = sign bit of (x0 - 0)
			shld	edx,eax,1 	; bit3 of code1 = sign bit of (x1 - 0)
			sub	esi,[width]	; get the difference (x0 - (width + 1))
			sub	eax,[width]	; get the difference (x1 - (width + 1))
			dec	esi
			dec	eax
			shld	ecx,esi,1	; bit2 of code0 = sign bit of (x0 - (width + 1))
			shld	edx,eax,1	; bit2 of code1 = sign bit of (x0 - (width + 1))

			; now we start computing the to suthenland boolean code for y0 , y1
			shld	ecx,edi,1   	; bit1 of code0 = sign bit of (y0 - 0)
			shld	edx,ebx,1	; bit1 of code1 = sign bit of (y0 - 0)
			sub	edi,[height]	; get the difference (y0 - (height + 1))
			sub	ebx,[height]	; get the difference (y1 - (height + 1))
			dec	edi
			dec	ebx
			shld	ecx,edi,1	; bit0 of code0 = sign bit of (y0 - (height + 1))
			shld	edx,ebx,1	; bit0 of code1 = sign bit of (y1 - (height + 1))

			; Bit 2 and 0 of cl and bl are complemented
			xor	cl,5		; reverse bit2 and bit0 in code0
			xor	dl,5 		; reverse bit2 and bit0 in code1

			; now perform the rejection test
			mov	eax,-1		; set return code to false
			mov	bl,cl 		; save code0 for future use
			test	dl,cl  		; if any two pair of bit in code0 and code1 is set
			jnz	clip_out	; then rectangle is outside the window

			; now perform the aceptance test
			xor	eax,eax		; set return code to true
			or	bl,dl		; if all pair of bits in code0 and code1 are reset
			jz	clip_out	; then rectangle is insize the window.								      '

			; we need to clip the rectangle iteratively
			mov	eax,-1		; set return code to false
			test	cl,1000b	; if bit3 of code0 is set then the rectangle
			jz	left_ok	; spill out the left edge of the window
			mov	edi,[x]		; edi = a pointer to x0
			mov	ebx,[w]		; ebx = a pointer to dw
			mov	esi,[edi]	; esi = x0
			mov	[dword ptr edi],0 ; set x0 to 0 "this the left edge value"
			add	[ebx],esi	; adjust dw by x0, since x0 must be negative

		left_ok:
			test	cl,0010b	; if bit1 of code0 is set then the rectangle
			jz	bottom_ok	; spill out the bottom edge of the window
			mov	edi,[y]		; edi = a pointer to y0
			mov	ebx,[h]		; ebx = a pointer to dh
			mov	esi,[edi]	; esi = y0
			mov	[dword ptr edi],0 ; set y0 to 0 "this the bottom edge value"
			add	[ebx],esi	; adjust dh by y0, since y0 must be negative

		bottom_ok:
			test	dl,0100b	; if bit2 of code1 is set then the rectangle
			jz	right_ok	; spill out the right edge of the window
			mov	edi,[w] 	; edi = a pointer to dw
			mov	esi,[x]		; esi = a pointer to x
			mov	ebx,[width]	; ebx = the width of the window
			sub	ebx,[esi] 	; the new dw is the difference (width-x0)
			mov	[edi],ebx	; adjust dw to (width - x0)
			jle	clip_out	; if (width-x0) = 0 then the clipped retangle
						; has no width we are done
		right_ok:
			test	dl,0001b	; if bit0 of code1 is set then the rectangle
			jz	clip_ok	; spill out the top edge of the window
			mov	edi,[h] 	; edi = a pointer to dh
			mov	esi,[y]		; esi = a pointer to y0
			mov	ebx,[height]	; ebx = the height of the window
			sub	ebx,[esi] 	; the new dh is the difference (height-y0)
			mov	[edi],ebx	; adjust dh to (height-y0)
			jle	clip_out	; if (width-x0) = 0 then the clipped retangle
						; has no width we are done
		clip_ok:
			mov	eax,1  	; signal the calling program that the rectangle was modify
		clip_out:
		//ret
	}

    // ENDP	Clip_Rect
}

/*
;***************************************************************************
;* Confine_Rect -- clip a given rectangle against a given window	   *
;*                                                                         *
;* INPUT:   &x,&y,w,h    -> Pointer to rectangle being clipped       *
;*          width,height     -> dimension of clipping window             *
;*                                                                         *
;* OUTPUT: a) Zero if the rectangle is totally contained by the 	   *
;*	      clipping window.						   *
;*	   c) A positive value if the rectangle	was shifted in position    *
;*	      to fix inside the clipping window, also the values pointed   *
;*	      by x, y, will adjusted to a new values	 		   *
;*									   *
;*  NOTE:  this function make not attempt to verify if the rectangle is	   *
;*	   bigger than the clipping window and at the same time wrap around*
;*	   it. If that is the case the result is meaningless		   *
;*=========================================================================*
; int Confine_Rect (int* x, int* y, int dw, int dh, int width, int height);          			   *
*/

extern "C" int __cdecl Confine_Rect(int* x, int* y, int w, int h, int width, int height)
{

    /*
        PROC	Confine_Rect C near
        uses	ebx, esi,edi
        arg	x:dword
        arg	y:dword
        arg	w:dword
        arg	h:dword
        arg	width :dword
        arg	height:dword
    */
    __asm {		  
	
			xor	eax,eax
			mov	ebx,[x]
			mov	edi,[w]

			mov	esi,[ebx]
			add	edi,[ebx]

			sub	edi,[width]
			neg	esi
			dec	edi

			test	esi,edi
			jl	x_axix_ok
			mov	eax,1

			test	esi,esi
			jl	shift_right
			mov	[dword ptr ebx],0
			jmp	x_axix_ok
		shift_right:
			inc	edi
			sub	[ebx],edi
		x_axix_ok:
			mov	ebx,[y]
			mov	edi,[h]

			mov	esi,[ebx]
			add	edi,[ebx]

			sub	edi,[height]
			neg	esi
			dec	edi

			test	esi,edi
			jl	confi_out
			mov	eax,1

			test	esi,esi
			jl	shift_top
			mov	[dword ptr ebx],0

        // ret
			jmp confi_out
		shift_top:
			inc	edi
			sub	[ebx],edi
		confi_out:
            // ret

            // ENDP	Confine_Rect
    }
}

/*
; $Header: //depot/Projects/Mobius/QA/Project/Run/SOURCECODE/REDALERT/WIN32LIB/DrawMisc.cpp#33 $
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

IDEAL
P386
MODEL USE32 FLAT

GLOBAL            C LCW_Uncompress          :NEAR

CODESEG

; ----------------------------------------------------------------
;
; Here are prototypes for the routines defined within this module:
;
; ULONG LCW_Uncompress(BYTE *source, BYTE *dest, ULONG length);
;
; ----------------------------------------------------------------
*/
#if (0) // ST 5/10/2019
extern "C" unsigned long __cdecl LCW_Uncompress(void* source, void* dest, unsigned long length_)
{
    // PROC	LCW_Uncompress C near
    //
    //	USES ebx,ecx,edx,edi,esi
    //
    //	ARG	source:DWORD
    //	ARG	dest:DWORD
    //	ARG	length:DWORD
    //;LOCALS
    //	LOCAL a1stdest:DWORD
    //	LOCAL maxlen:DWORD
    //	LOCAL lastbyte:DWORD
    //	LOCAL lastcom:DWORD
    //	LOCAL lastcom1:DWORD

    unsigned long a1stdest;
    unsigned long maxlen;
    unsigned long lastbyte;
    // unsigned long lastcom;
    // unsigned long lastcom1;

    __asm {


		mov	edi,[dest]
		mov	esi,[source]
		mov	edx,[length_]

	;
	;
	; uncompress data to the following codes in the format b = byte, w = word
	; n = byte code pulled from compressed data
	;   Bit field of n		command		description
	; n=0xxxyyyy,yyyyyyyy		short run	back y bytes and run x+3
	; n=10xxxxxx,n1,n2,...,nx+1	med length	copy the next x+1 bytes
	; n=11xxxxxx,w1			med run		run x+3 bytes from offset w1
	; n=11111111,w1,w2		long copy	copy w1 bytes from offset w2
	; n=11111110,w1,b1		long run	run byte b1 for w1 bytes
	; n=10000000			end		end of data reached
	;

		mov	[a1stdest],edi
		add	edx,edi
		mov	[lastbyte],edx
		cld			; make sure all lod and sto are forward
		mov	ebx,esi		; save the source offset

	loop_label:
		mov	eax,[lastbyte]
		sub	eax,edi		; get the remaining byte to uncomp
		jz	short out_label		; were done

		mov	[maxlen],eax	; save for string commands
		mov	esi,ebx		; mov in the source index

		xor	eax,eax
		mov	al,[esi]
		inc	esi
		test	al,al		; see if its a short run
		js	short notshort

		mov	ecx,eax		;put count nibble in cl

		mov	ah,al		; put rel offset high nibble in ah
		and	ah,0Fh		; only 4 bits count

		shr	cl,4		; get run -3
		add	ecx,3		; get actual run length

		cmp	ecx,[maxlen]	; is it too big to fit?
		jbe	short rsok		; if not, its ok

		mov	ecx,[maxlen]	; if so, max it out so it dosen't overrun

	rsok:
		mov	al,[esi]	; get rel offset low byte
		lea	ebx,[esi+1]	; save the source offset
		mov	esi,edi		; get the current dest
		sub	esi,eax		; get relative offset

		rep	movsb

		jmp	loop_label

	notshort:
		test	al,40h		; is it a length?
		jne	short notlength	; if not it could be med or long run

		cmp	al,80h		; is it the end?
		je	short out_label		; if so its over

		mov	cl,al		; put the byte in count register
		and	ecx,3Fh		; and off the extra bits

		cmp	ecx,[maxlen]	; is it too big to fit?
		jbe	short lenok		; if not, its ok

		mov	ecx,[maxlen]	; if so, max it out so it dosen't overrun

	lenok:
		rep movsb

		mov	ebx,esi		; save the source offset
		jmp	loop_label

	out_label:
	      	mov	eax,edi
		sub	eax,[a1stdest]
		jmp	label_exit

	notlength:
		mov	cl,al		; get the entire code
		and	ecx,3Fh		; and off all but the size -3
		add	ecx,3		; add 3 for byte count

		cmp	al,0FEh
		jne	short notrunlength

		xor	ecx,ecx
		mov	cx,[esi]

		xor	eax,eax
		mov	al,[esi+2]
		lea	ebx,[esi+3]	;save the source offset

		cmp	ecx,[maxlen]	; is it too big to fit?
		jbe	short runlenok		; if not, its ok

		mov	ecx,[maxlen]	; if so, max it out so it dosen't overrun

	runlenok:
		test	ecx,0ffe0h
		jnz	dont_use_stosb
		rep	stosb
		jmp	loop_label


	dont_use_stosb:
		mov	ah,al
		mov	edx,eax
		shl	eax,16
		or	eax,edx

		test	edi,3
		jz	aligned

		mov	[edi],eax
		mov	edx,edi
		and	edi,0fffffffch
		lea	edi,[edi+4]
		and	edx,3
		dec	dl
		xor	dl,3
		sub	ecx,edx

	aligned:
		mov	edx,ecx
		shr	ecx,2
		rep	stosd

		and	edx,3
		jz	loop_label
		mov	ecx,edx
		rep	stosb
		jmp	loop_label






	notrunlength:
		cmp	al,0FFh		; is it a long run?
		jne	short notlong	; if not use the code as the size

		xor     ecx,ecx
		xor	eax,eax
		mov	cx,[esi]	; if so, get the size
		lea	esi,[esi+2]

	notlong:
		mov	ax,[esi]	;get the real index
		add	eax,[a1stdest]	;add in the 1st index
		lea	ebx,[esi+2]	;save the source offset
		cmp	ecx,[maxlen]	;compare for overrun
		mov	esi,eax		;use eax as new source
		jbe	short runok	; if not, its ok

		mov	ecx,[maxlen]	; if so, max it out so it dosen't overrun

	runok:
		test	ecx,0ffe0h
		jnz	dont_use_movsb
		rep	movsb
		jmp	loop_label




	dont_use_movsb:
		lea	edx,[edi+0fffffffch]
		cmp	esi,edx
		ja	use_movsb

		test	edi,3
		jz	aligned2

		mov	eax,[esi]
		mov	[edi],eax
		mov	edx,edi
		and	edi,0fffffffch
		lea	edi,[edi+4]
		and	edx,3
		dec	dl
		xor	dl,3
		sub	ecx,edx
		add	esi,edx

	aligned2:
		mov	edx,ecx
		shr	ecx,2
		and	edx,3
		rep	movsd
		mov	ecx,edx
	use_movsb:
		rep	movsb
		jmp	loop_label




	label_exit:
		mov	eax,edi
		mov	ebx,[dest]
		sub	eax,ebx

		//ret

	}
}
#endif

/*
;***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Westwood 32 bit Library                  *
;*                                                                         *
;*                    File Name : TOPAGE.ASM                               *
;*                                                                         *
;*                   Programmer : Phil W. Gorrow                           *
;*                                                                         *
;*                   Start Date : June 8, 1994                             *
;*                                                                         *
;*                  Last Update : June 15, 1994   [PWG]                    *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;*   Buffer_To_Page -- Copies a linear buffer to a virtual viewport	   *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *

IDEAL
P386
MODEL USE32 FLAT

TRANSP	equ  0


INCLUDE ".\drawbuff.inc"
INCLUDE ".\gbuffer.inc"

CODESEG

;***************************************************************************
;* VVC::TOPAGE -- Copies a linear buffer to a virtual viewport		   *
;*                                                                         *
;* INPUT:	WORD	x_pixel		- x pixel on viewport to copy from *
;*		WORD	y_pixel 	- y pixel on viewport to copy from *
;*		WORD	pixel_width	- the width of copy region	   *
;*		WORD	pixel_height	- the height of copy region	   *
;*		BYTE *	src		- buffer to copy from		   *
;*		VVPC *  dest		- virtual viewport to copy to	   *
;*                                                                         *
;* OUTPUT:      none                                                       *
;*                                                                         *
;* WARNINGS:    Coordinates and dimensions will be adjusted if they exceed *
;*	        the boundaries.  In the event that no adjustment is 	   *
;*	        possible this routine will abort.  If the size of the 	   *
;*		region to copy exceeds the size passed in for the buffer   *
;*		the routine will automatically abort.			   *
;*									   *
;* HISTORY:                                                                *
;*   06/15/1994 PWG : Created.                                             *
;*=========================================================================*
 */

extern "C" long __cdecl Buffer_To_Page(int x_pixel,
                                       int y_pixel,
                                       int pixel_width,
                                       int pixel_height,
                                       void* src,
                                       void* dest)
{

    /*
        PROC	Buffer_To_Page C near
        USES	eax,ebx,ecx,edx,esi,edi

        ;*===================================================================
        ;* define the arguements that our function takes.
        ;*===================================================================
        ARG	x_pixel     :DWORD		; x pixel position in source
        ARG	y_pixel     :DWORD		; y pixel position in source
        ARG	pixel_width :DWORD		; width of rectangle to blit
        ARG	pixel_height:DWORD		; height of rectangle to blit
        ARG    	src         :DWORD		; this is a member function
        ARG	dest        :DWORD		; what are we blitting to

    ;	ARG	trans       :DWORD			; do we deal with transparents?

        ;*===================================================================
        ; Define some locals so that we can handle things quickly
        ;*===================================================================
        LOCAL 	x1_pixel :dword
        LOCAL	y1_pixel :dword
        local	scr_x 	: dword
        local	scr_y 	: dword
        LOCAL	dest_ajust_width:DWORD
        LOCAL	scr_ajust_width:DWORD
        LOCAL	dest_area   :  dword
    */

    unsigned long x1_pixel;
    unsigned long y1_pixel;
    unsigned long scr_x;
    unsigned long scr_y;
    unsigned long dest_ajust_width;
    unsigned long scr_ajust_width;
    // unsigned long dest_area;

    __asm {
	
			cmp	[ src ] , 0
			jz	real_out


		; Clip dest Rectangle against source Window boundaries.

			mov	[ scr_x ] , 0
			mov	[ scr_y ] , 0
			mov  	esi , [ dest ]	    ; get ptr to dest
			xor 	ecx , ecx
			xor 	edx , edx
			mov	edi , [esi]GraphicViewPortClass.Width  ; get width into register
			mov	ebx , [ x_pixel ]
			mov	eax , [ x_pixel ]
			add	ebx , [ pixel_width ]
			shld	ecx , eax , 1
			mov	[ x1_pixel ] , ebx
			inc	edi
			shld	edx , ebx , 1
			sub	eax , edi
			sub	ebx , edi
			shld	ecx , eax , 1
			shld	edx , ebx , 1

			mov	edi, [esi]GraphicViewPortClass.Height ; get height into register
			mov	ebx , [ y_pixel ]
			mov	eax , [ y_pixel ]
			add	ebx , [ pixel_height ]
			shld	ecx , eax , 1
			mov	[ y1_pixel ] , ebx
			inc	edi
			shld	edx , ebx , 1
			sub	eax , edi
			sub	ebx , edi
			shld	ecx , eax , 1
			shld	edx , ebx , 1

			xor	cl , 5
			xor	dl , 5
			mov	al , cl
			test	dl , cl
			jnz	real_out
			or	al , dl
			jz	do_blit

			test	cl , 1000b
			jz	dest_left_ok
			mov	eax , [ x_pixel ]
			neg	eax
			mov	[ x_pixel ] , 0
			mov	[ scr_x ] , eax

		dest_left_ok:
			test	cl , 0010b
			jz	dest_bottom_ok
			mov	eax , [ y_pixel ]
			neg	eax
			mov	[ y_pixel ] , 0
			mov	[ scr_y ] , eax

		dest_bottom_ok:
			test	dl , 0100b
			jz	dest_right_ok
			mov	eax , [esi]GraphicViewPortClass.Width  ; get width into register
			mov	[ x1_pixel ] , eax
		dest_right_ok:
			test	dl , 0001b
			jz	do_blit
			mov	eax , [esi]GraphicViewPortClass.Height  ; get width into register
			mov	[ y1_pixel ] , eax

		do_blit:

   		    cld

   		    mov	eax , [esi]GraphicViewPortClass.XAdd
   		    add	eax , [esi]GraphicViewPortClass.Width
   		    add	eax , [esi]GraphicViewPortClass.Pitch
   		    mov	edi , [esi]GraphicViewPortClass.Offset

   		    mov	ecx , eax
   		    mul	[ y_pixel ]
   		    add	edi , [ x_pixel ]
   		    add	edi , eax

   		    add	ecx , [ x_pixel ]
   		    sub	ecx , [ x1_pixel ]
   		    mov	[ dest_ajust_width ] , ecx


   		    mov	esi , [ src ]
   		    mov	eax , [ pixel_width ]
   		    sub	eax , [ x1_pixel ]
   		    add	eax , [ x_pixel ]
   		    mov	[ scr_ajust_width ] , eax

   		    mov	eax , [ scr_y ]
   		    mul 	[ pixel_width ]
   		    add	eax , [ scr_x ]
   		    add	esi , eax

   		    mov	edx , [ y1_pixel ]
   		    mov	eax , [ x1_pixel ]

   		    sub	edx , [ y_pixel ]
   		    jle	real_out
   		    sub	eax , [ x_pixel ]
   		    jle	real_out


		; ********************************************************************
		; Forward bitblit only

        // IF TRANSP
        //	    test	[ trans ] , 1
        //	    jnz	forward_Blit_trans
        // ENDIF


		; the inner loop is so efficient that
		; the optimal consept no longer apply because
		; the optimal byte have to by a number greather than 9 bytes
   		    cmp	eax , 10
   		    jl	forward_loop_bytes

		forward_loop_dword:
   		    mov	ecx , edi
   		    mov	ebx , eax
   		    neg	ecx
   		    and	ecx , 3
   		    sub	ebx , ecx
   		    rep	movsb
   		    mov	ecx , ebx
   		    shr	ecx , 2
   		    rep	movsd
   		    mov	ecx , ebx
   		    and	ecx , 3
   		    rep	movsb
   		    add	esi , [ scr_ajust_width ]
   		    add	edi , [ dest_ajust_width ]
   		    dec	edx
   		    jnz	forward_loop_dword
   		    jmp	real_out // ret

		forward_loop_bytes:
   		    mov	ecx , eax
   		    rep	movsb
   		    add	esi , [ scr_ajust_width ]
   		    add	edi , [ dest_ajust_width ]
   		    dec	edx					; decrement the height
   		    jnz	forward_loop_bytes
                                                                       //  ret

            // IF  TRANSP
            //
            //
            // forward_Blit_trans:
            //
            //
            //	    mov	ecx , eax
            //	    and	ecx , 01fh
            //	    lea	ecx , [ ecx + ecx * 4 ]
            //	    neg	ecx
            //	    shr	eax , 5
            //	    lea	ecx , [ transp_reference + ecx * 2 ]
            //	    mov	[ y1_pixel ] , ecx
            //
            //
            // forward_loop_trans:
            //	    mov	ecx , eax
            //	    jmp	[ y1_pixel ]
            // forward_trans_line:
            //	    REPT	32
            //	    local	transp_pixel
            //	    		mov	bl , [ esi ]
            //	    		inc	esi
            //	    		test	bl , bl
            //	    		jz	transp_pixel
            //	    		mov	[ edi ] , bl
            //	 	    transp_pixel:
            //	    		inc	edi
            //	ENDM
            //	 transp_reference:
            //	    dec	ecx
            //	    jge	forward_trans_line
            //	    add	esi , [ scr_ajust_width ]
            //	    add	edi , [ dest_ajust_width ]
            //	    dec	edx
            //	    jnz	forward_loop_trans
            //	    ret
            // ENDIF

		real_out:
            // ret
    }
}

// ENDP	Buffer_To_Page
// END

/*

;***************************************************************************
;* VVPC::GET_PIXEL -- Gets a pixel from the current view port		   *
;*                                                                         *
;* INPUT:	WORD the x pixel on the screen.				   *
;*		WORD the y pixel on the screen.				   *
;*                                                                         *
;* OUTPUT:      UBYTE the pixel at the specified location		   *
;*                                                                         *
;* WARNING:	If pixel is to be placed outside of the viewport then	   *
;*		this routine will abort.				   *
;*                                                                         *
;* HISTORY:                                                                *
;*   06/07/1994 PWG : Created.                                             *
;*=========================================================================*
    PROC	Buffer_Get_Pixel C near
    USES	ebx,ecx,edx,edi

    ARG    	this_object:DWORD				; this is a member function
    ARG	x_pixel:DWORD				; x position of pixel to set
    ARG	y_pixel:DWORD				; y position of pixel to set
*/

extern "C" int __cdecl Buffer_Get_Pixel(void* this_object, int x_pixel, int y_pixel)
{
    __asm {		  

		;*===================================================================
		; Get the viewport information and put bytes per row in ecx
		;*===================================================================
		mov	ebx,[this_object]				; get a pointer to viewport
		xor	eax,eax
		mov	edi,[ebx]GraphicViewPortClass.Offset	; get the correct offset
		mov	ecx,[ebx]GraphicViewPortClass.Height	; edx = height of viewport
		mov	edx,[ebx]GraphicViewPortClass.Width	; ecx = width of viewport

		;*===================================================================
		; Verify that the X pixel offset if legal
		;*===================================================================
		mov	eax,[x_pixel]				; find the x position
		cmp	eax,edx					;   is it out of bounds
		jae	short exit_label				; if so then get out
		add	edi,eax					; otherwise add in offset

		;*===================================================================
		; Verify that the Y pixel offset if legal
		;*===================================================================
		mov	eax,[y_pixel]				; get the y position
		cmp	eax,ecx					;  is it out of bounds
		jae	exit_label					; if so then get out
		add	edx,[ebx]GraphicViewPortClass.XAdd	; otherwise find bytes per row
		add	edx,[ebx]GraphicViewPortClass.Pitch	; otherwise find bytes per row
		mul	edx					; offset = bytes per row * y
		add	edi,eax					; add it into the offset

		;*===================================================================
		; Write the pixel to the screen
		;*===================================================================
		xor	eax,eax					; clear the word
		mov	al,[edi]				; read in the pixel
	exit_label:
            // ret
            // ENDP	Buffer_Get_Pixel

    }
}
