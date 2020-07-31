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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : LIBRARY                                  *
 *                                                                         *
 *                    File Name : FONT.C                                   *
 *                                                                         *
 *                   Programmer : David Dettmer                            *
 *                                                                         *
 *                  Last Update : July 20, 1994   [SKB]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Char_Pixel_Width -- Return pixel width of a character.                *
 *   String_Pixel_Width -- Return pixel width of a string of characters.   *
 *   Get_Next_Text_Print_XY -- Calculates X and Y given ret value from Text_P*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "font.h"

#include "gbuffer.h"
#include "file.h"
#include "memflag.h"

int FontXSpacing = 0;
int FontYSpacing = 0;
void const* FontPtr = nullptr;
char FontWidth = 8;
char FontHeight = 8;

// only font.c and set_font.c use the following
char* FontWidthBlockPtr = nullptr;

/***************************************************************************
 * SET_FONT -- Changes the default text printing font.                     *
 *                                                                         *
 *    This routine will change the default text printing font for all      *
 *    text output.  It handles updating the system where necessary.        *
 *                                                                         *
 * INPUT:   fontptr  -- Pointer to the font to change to.                  *
 *                                                                         *
 * OUTPUT:  Returns with a pointer to the previous font.                   *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/06/1991 JLB : Created.                                             *
 *   09/17/1991 JLB : Fixed return value bug.                              *
 *   01/31/1992 DRD : Modified to use new font format.                     *
 *   06/29/1994 SKB : modified for 32 bit library                          *
 *=========================================================================*/
void* Set_Font(void const* fontptr)
{
    void* oldfont = (void*)FontPtr;

    if (fontptr) {
        FontPtr = (void*)fontptr;

        /*
        **	Inform the system about the new font.
        */

        FontWidthBlockPtr = (char*)fontptr + *(unsigned short*)((char*)fontptr + FONTWIDTHBLOCK);
        char const* blockptr = (char*)fontptr + *(unsigned short*)((char*)fontptr + FONTINFOBLOCK);
        FontHeight = *(blockptr + FONTINFOMAXHEIGHT);
        FontWidth = *(blockptr + FONTINFOMAXWIDTH);
    }

    return oldfont;
}

/***************************************************************************
 * CHAR_PIXEL_WIDTH -- Return pixel width of a character.                  *
 *                                                                         *
 *    Retreives the pixel width of a character from the font width block.  *
 *                                                                         *
 * INPUT:      Character.                                                  *
 *                                                                         *
 * OUTPUT:     Pixel width of a string of characters.                      *
 *                                                                         *
 * WARNINGS:   Set_Font must have been called first.                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/31/1992 DRD : Created.                                             *
 *   06/30/1994 SKB : Converted to 32 bit library.                         *
 *=========================================================================*/
int Char_Pixel_Width(char chr)
{
    int width = (unsigned char)*(FontWidthBlockPtr + (unsigned char)chr) + FontXSpacing;

    return (width);
}

/***************************************************************************
 * STRING_PIXEL_WIDTH -- Return pixel width of a string of characters.     *
 *                                                                         *
 *    Calculates the pixel width of a string of characters.  This uses     *
 *      the font width block for the widths.                               *
 *                                                                         *
 * INPUT:      Pointer to string of characters.                            *
 *                                                                         *
 * OUTPUT:     Pixel width of a string of characters.                      *
 *                                                                         *
 * WARNINGS:   Set_Font must have been called first.                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/30/1992 DRD : Created.                                             *
 *   01/31/1992 DRD : Use Char_Pixel_Width.                                *
 *   06/30/1994 SKB : Converted to 32 bit library.                         *
 *=========================================================================*/
unsigned int String_Pixel_Width(char const* string)
{
    if (!string) {
        return 0;
    }

    unsigned short largest = 0; // Largest recorded width of the string.
    unsigned short width = 0;   // Working accumulator of string width.
    while (*string) {
        if (*string == '\r') {
            string++;
            largest = MAX(largest, width);
            width = 0;
        } else {
            width += Char_Pixel_Width(*string++); // add each char's width
        }
    }
    largest = MAX(largest, width);
    return largest;
}

/***************************************************************************
 * GET_NEXT_TEXT_PRINT_XY -- Calculates X and Y given ret value from Text_P*
 *                                                                         *
 *                                                                         *
 * INPUT:   VVPC& vp - viewport that was printed to.                       *
 *          unsigned long offset - offset that Text_Print returned.        *
 *          INT *x - x return value.                                       *
 *          INT *y - y return value.                                       *
 *                                                                         *
 * OUTPUT:  x and y are set.                                               *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/20/1994 SKB : Created.                                             *
 *=========================================================================*/
void Get_Next_Text_Print_XY(GraphicViewPortClass& gp, unsigned long offset, int* x, int* y)
{
    if (offset) {
        int buffwidth = gp.Get_Width() + gp.Get_XAdd();
        offset -= gp.Get_Offset();
        *x = offset % buffwidth;
        *y = offset / buffwidth;
    } else {
        *x = *y = 0;
    }
}

extern "C" unsigned char ColorXlat[16][16];

void Set_Font_Palette_Range(void const* palette, int start_idx, int end_idx)
{
    start_idx &= 15;
    end_idx &= 15;
    const unsigned char* colors = (const unsigned char*)palette;

    for (int i = start_idx; i < end_idx + 1; ++i) {
        ColorXlat[0][i] = *colors;
        ColorXlat[i][0] = *colors++;
    }
}

/***************************************************************************
 * LOAD_FONT -- Loads a font from disk.                                    *
 *                                                                         *
 *    This loads a font from disk.  This function must be called as a      *
 *    precursor to calling Set_Font().  You need only call this function   *
 *    once per desired font at the beginning of your code, but AFTER       *
 *    Prog_Init() is called.                                               *
 *                                                                         *
 * INPUT:      name  - Pointer to font name to use (eg. "topaz.font")      *
 *                                                                         *
 *             fontsize - Size in points of the font loaded.               *
 *                                                                         *
 * OUTPUT:     Pointer to font data or NULL if unable to load.             *
 *                                                                         *
 * WARNINGS:   Some system memory is grabbed by this routine.              *
 *                                                                         *
 * HISTORY:                                                                *
 *   4/10/91    BS  : 2.0 compatibily                                      *
 *   6/09/91    JLB : IBM and Amiga compatability.                         *
 *   11/27/1991 JLB : Uses file I/O routines for disk access.              *
 *   01/29/1992 DRD : Modified to use new font format.                     *
 *   02/01/1992 DRD : Added font file verification.                        *
 *   06/29/1994 SKB : modified for 32 bit library                          *
 *=========================================================================*/
void* Load_Font(char const* name)
{
    char valid;
    int fh;              // DOS file handle for font file.
    unsigned short size; // Size of the data in the file (-2);
    char* ptr = NULL;    // Pointer to newly loaded font.

    fh = Open_File(name, READ);
    if (fh >= 0) {
        if (Read_File(fh, (char*)&size, 2) != 2)
            return (NULL);

        ptr = (char*)Alloc(size, MEM_NORMAL);
        *(short*)ptr = size;
        Read_File(fh, ptr + 2, size - 2);
        Close_File(fh);
    } else {
        return ((void*)errno);
    }

#ifdef cuts
    if (Find_File(name)) {
        fh = Open_File(name, READ);
        if (Read_File(fh, (char*)&size, 2) != 2)
            return (NULL);

        ptr = (char*)Alloc(size, MEM_NORMAL);
        *(short*)ptr = size;
        Read_File(fh, ptr + 2, size - 2);
        Close_File(fh);
    } else {
        return (NULL);
    }
#endif

    //
    // verify that the file loaded is a valid font file.
    //

    valid = FALSE;
    if (*(ptr + 2) == 0) {     // no compression
        if (*(ptr + 3) == 5) { // currently only 5 data blocks are used.
            valid = TRUE;
        }
    }

    if (!valid) {
        return (NULL);
    }

    return (ptr);
}
