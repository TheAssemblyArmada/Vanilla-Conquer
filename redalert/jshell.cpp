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

/* $Header: /CounterStrike/JSHELL.CPP 2     3/13/97 2:05p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : JSHELL.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 2, 1994                                                *
 *                                                                                             *
 *                  Last Update : May 11, 1995 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Build_Translucent_Table -- Creates a translucent control table.                           *
 *   Conquer_Build_Translucent_Table -- Builds fading table for shadow colors only.            *
 *   Fatal -- General purpose fatal error handler.                                             *
 *   Load_Uncompress -- Loads and uncompresses data to a buffer.                               *
 *   Set_Window -- Sets the window dimensions to that specified.                               *
 *   Small_Icon -- Create a small icon from a big one.                                         *
 *   Translucent_Table_Size -- Determines the size of a translucent table.                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "common/fading.h"
#include "common/wwfile.h"

/***********************************************************************************************
 * Small_Icon -- Create a small icon from a big one.                                           *
 *                                                                                             *
 *    This routine will extract the specified icon from the icon data file and convert that    *
 *    icon into a small (3x3) representation. Typical use of this mini-icon is for the radar   *
 *    map.                                                                                     *
 *                                                                                             *
 * INPUT:   iconptr  -- Pointer to the icon data file.                                         *
 *                                                                                             *
 *          iconnum  -- The embedded icon number to convert into a small image.                *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the small icon imagery. This is exactly 9 bytes long.    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/11/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void* Small_Icon(void const* iconptr, int iconnum)
{
    static unsigned char _icon[9];
    IControl_Type const* iptr = (IControl_Type const*)iconptr;
    unsigned char* data;

    if (iconptr) {
        iconnum = ((char*)((char*)iptr + iptr->Map))[iconnum];
        data = &((unsigned char*)((unsigned char*)iptr + iptr->Icons))[iconnum * (24 * 24)];
        //		data = &iptr->Icons[iconnum*(24*24)];

        for (int index = 0; index < 9; index++) {
            int _offsets[9] = {4 + 4 * 24,
                               12 + 4 * 24,
                               20 + 4 * 24,
                               4 + 12 * 24,
                               12 + 12 * 24,
                               20 + 12 * 24,
                               4 + 20 * 24,
                               12 + 20 * 24,
                               20 + 20 * 24};
            _icon[index] = data[_offsets[index]];
        }
    }

    return (_icon);
}

/***********************************************************************************************
 * Set_Window -- Sets the window dimensions to that specified.                                 *
 *                                                                                             *
 *    Use this routine to set the windows dimensions to the coordinates and dimensions         *
 *    specified.                                                                               *
 *                                                                                             *
 * INPUT:   x     -- Window X pixel position.                                                  *
 *                                                                                             *
 *          y     -- Window Y pixel position.                                                  *
 *                                                                                             *
 *          w     -- Window width in pixels.                                                   *
 *                                                                                             *
 *          h     -- Window height in pixels.                                                  *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   The X and width values are truncated to an even 8 pixel boundary. This is       *
 *             the same as stripping off the lower 3 bits.                                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/15/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void Set_Window(int window, int x, int y, int w, int h)
{
    WindowList[window][WINDOWWIDTH] = w;
    WindowList[window][WINDOWHEIGHT] = h;
    WindowList[window][WINDOWX] = x;
    WindowList[window][WINDOWY] = y;
}

/***********************************************************************************************
 * Fatal -- General purpose fatal error handler.                                               *
 *                                                                                             *
 *    This is a very simple general purpose fatal error handler. It goes directly to text      *
 *    mode, prints the error, and then aborts with a failure code.                             *
 *                                                                                             *
 * INPUT:   message  -- The text message to display.                                           *
 *                                                                                             *
 *          ...      -- Any optional parameters that are used in formatting the message.       *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine never returns. The game exits immediately.                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void Fatal(char const* message, ...)
{
    va_list va;

    va_start(va, message);
    Prog_End(message, true);
    vfprintf(stderr, message, va);
    Mono_Printf(message);
    if (!RunningAsDLL) { // PG
        Emergency_Exit(EXIT_FAILURE);
    }
}

#ifdef NEVER
void File_Fatal(char const* message)
{
    // Prog_End();
    perror(message);
    Emergency_Exit(EXIT_FAILURE);
}
#endif

/***********************************************************************************************
 * Load_Uncompress -- Loads and uncompresses data to a buffer.                                 *
 *                                                                                             *
 *    This is the C++ counterpart to the Load_Uncompress function. It will load the file       *
 *    specified into the graphic buffer indicated and uncompress it.                           *
 *                                                                                             *
 * INPUT:   file     -- The file to load and uncompress.                                       *
 *                                                                                             *
 *          uncomp_buff -- The graphic buffer that initial loading will use.                   *
 *                                                                                             *
 *          dest_buff   -- The buffer that will hold the uncompressed data.                    *
 *                                                                                             *
 *          reserved_data  -- This is an optional pointer to a buffer that will hold any       *
 *                            reserved data the compressed file may contain. This is           *
 *                            typically a palette.                                             *
 *                                                                                             *
 * OUTPUT:  Returns with the size of the uncompressed data in the destination buffer.          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long Load_Uncompress(FileClass& file, BuffType& uncomp_buff, BuffType& dest_buff, void* reserved_data)
{
    unsigned short size;
    void* sptr = uncomp_buff.Get_Buffer();
    void* dptr = dest_buff.Get_Buffer();
    int opened = false;
    CompHeaderType header;

    /*
    **	The file must be opened in order to be read from. If the file
    **	isn't opened, then open it. Record this fact so that it can be
    **	restored to its closed state at the end.
    */
    if (!file.Is_Open()) {
        if (!file.Open()) {
            return (0);
        }
        opened = true;
    }

    /*
    **	Read in the size of the file (supposedly).
    */
    file.Read(&size, sizeof(size));

    /*
    **	Read in the header block. This block contains the compression type
    **	and skip data (among other things).
    */
    file.Read(&header, sizeof(header));
    size -= sizeof(header);

    /*
    **	If there are skip bytes then they must be processed. Either read
    **	them into the buffer provided or skip past them. No check is made
    **	to ensure that the reserved data buffer is big enough (watch out!).
    */
    if (header.Skip) {
        size -= header.Skip;
        if (reserved_data) {
            file.Read(reserved_data, header.Skip);
        } else {
            file.Seek(header.Skip, SEEK_CUR);
        }
        header.Skip = 0;
    }

    /*
    **	Determine where is the proper place to load the data. If both buffers
    **	specified are identical, then the data should be loaded at the end of
    **	the buffer and decompressed at the beginning.
    */
    if (uncomp_buff.Get_Buffer() == dest_buff.Get_Buffer()) {
        sptr = (char*)sptr + uncomp_buff.Get_Size() - (size + sizeof(header));
    }

    /*
    **	Read in the bulk of the data.
    */
    Mem_Copy(&header, sptr, sizeof(header));
    file.Read((char*)sptr + sizeof(header), size);

    /*
    **	Decompress the data.
    */
    size = (unsigned int)Uncompress_Data(sptr, dptr);

    /*
    **	Close the file if necessary.
    */
    if (opened) {
        file.Close();
    }
    return ((long)size);
}

int Load_Picture(char const* filename,
                 BufferClass& scratchbuf,
                 BufferClass& destbuf,
                 unsigned char* palette,
                 PicturePlaneType)
{
    return (Load_Uncompress(CCFileClass(filename), scratchbuf, destbuf, palette) / 8000);
}

/***********************************************************************************************
 * Translucent_Table_Size -- Determines the size of a translucent table.                       *
 *                                                                                             *
 *    Use this routine to determine how big the translucent table needs                        *
 *    to be given the specified number of colors. This value is typically                      *
 *    used when allocating the buffer for the translucent table.                               *
 *                                                                                             *
 * INPUT:   count -- The number of colors that are translucent.                                *
 *                                                                                             *
 * OUTPUT:  Returns the size of the translucent table.                                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/02/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long Translucent_Table_Size(int count)
{
    return (256L + (256L * count));
}

/***********************************************************************************************
 * Build_Translucent_Table -- Creates a translucent control table.                             *
 *                                                                                             *
 *    The table created by this routine is used by Draw_Shape (GHOST) to                       *
 *    achieve a translucent affect. The original color of the shape will                       *
 *    show through. This differs from the fading effect, since that                            *
 *    affect only alters the background color toward a single destination                      *
 *    color.                                                                                   *
 *                                                                                             *
 * INPUT:   palette  -- Pointer to the control palette.                                        *
 *                                                                                             *
 *          control  -- Pointer to array of structures that control how                        *
 *                      the translucent table will be built.                                   *
 *                                                                                             *
 *          count    -- The number of entries in the control array.                            *
 *                                                                                             *
 *          buffer   -- Pointer to buffer to place the translucent table.                      *
 *                      If NULL is passed in, then the buffer will be                          *
 *                      allocated.                                                             *
 *                                                                                             *
 * OUTPUT:  Returns with pointer to the translucent table.                                     *
 *                                                                                             *
 * WARNINGS:   This routine is exceedingly slow. Use sparingly.                                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/02/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void* Build_Translucent_Table(PaletteClass const& palette, TLucentType const* control, int count, void* buffer)
{
    unsigned char const* table; // Remap table pointer.
    int index;                  // Working color index.

    if (count && control /* && palette*/) { // palette can't be NULL... ST - 5/9/2019
        if (!buffer) {
            buffer = new char[Translucent_Table_Size(count)];
        }

        if (buffer) {
            memset(buffer, -1, 256);
            table = (unsigned char*)buffer + 256;

            /*
            **	Build the individual remap tables for each translucent color.
            */
            for (index = 0; index < count; index++) {
                ((unsigned char*)buffer)[control[index].SourceColor] = index;
                Build_Fading_Table(palette.Get_Data(), (void*)table, control[index].DestColor, control[index].Fading);
                table = (unsigned char*)table + 256;
            }
        }
    }
    return (buffer);
}

/***********************************************************************************************
 * Conquer_Build_Translucent_Table -- Builds fading table for shadow colors only.              *
 *                                                                                             *
 *    This routine will build a translucent (fading) table to remap colors into the shadow     *
 *    color region of the palette. Shadow colors are not affected by this translucent table.   *
 *    This means that a shape can be overlapped any number of times and the imagery will       *
 *    remain deterministic (and constant).                                                     *
 *                                                                                             *
 * INPUT:   palette  -- Pointer to the palette to base the translucent process on.             *
 *                                                                                             *
 *          control  -- Pointer to special control structure that specifies the                *
 *                      target color, and percentage of fade.                                  *
 *                                                                                             *
 *          count    -- The number of colors to be remapped (entries in the control array).    *
 *                                                                                             *
 *          buffer   -- Pointer to the staging buffer that will hold the translucent table     *
 *                      data. If this parameter is NULL, then an appropriate sized table       *
 *                      will be allocated.                                                     *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the translucent table data.                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/27/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void* Conquer_Build_Translucent_Table(PaletteClass const& palette, TLucentType const* control, int count, void* buffer)
{
    unsigned char const* table; // Remap table pointer.

    if (count && control) {
        if (!buffer) {
            buffer = new char[Translucent_Table_Size(count)];
        }

        if (buffer) {
            memset(buffer, -1, 256);
            table = (unsigned char*)buffer + 256;

            /*
            **	Build the individual remap tables for each translucent color.
            */
            for (int index = 0; index < count; index++) {
                ((unsigned char*)buffer)[control[index].SourceColor] = index;
                Conquer_Build_Fading_Table(palette, (void*)table, control[index].DestColor, control[index].Fading);
                table = (unsigned char*)table + 256;
            }
        }
    }
    return (buffer);
}

void* Make_Fading_Table(PaletteClass const& palette, void* dest, int color, int frac)
{
    if (dest) {
        unsigned char* ptr = (unsigned char*)dest;

        /*
        **	Find an appropriate remap color index for every color in the palette.
        **	There are certain exceptions to this, but they are trapped within the
        **	loop.
        */
        for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {

            /*
            **	Find the color that, ideally, the working color should be remapped
            **	to in the special remap range.
            */
            RGBClass trycolor = palette[index];
            trycolor.Adjust(frac, palette[color]); // Try to match this color.

            /*
            **	Search through the remap range to find the color that should be remapped
            **	to. This special range is used for shadows or other effects that are
            **	not compounded if additively applied.
            */
            *ptr++ = palette.Closest_Color(trycolor);
        }
    }
    return (dest);
}

void* Conquer_Build_Fading_Table(PaletteClass const& palette, void* dest, int color, int frac)
{
    if (dest) {
        unsigned char* ptr = (unsigned char*)dest;
        //		HSVClass desthsv = palette[color];

        /*
        **	Find an appropriate remap color index for every color in the palette.
        **	There are certain exceptions to this, but they are trapped within the
        **	loop.
        */
        for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {

            /*
            **	If this color should not be remapped, then it will be stored as a remap
            **	to itself. This is effectively no remap.
            */
            if (index > PaletteClass::COLOR_COUNT - 16 || index == 0) {
                *ptr++ = index;
            } else {

                /*
                **	Find the color that, ideally, the working color should be remapped
                **	to in the special remap range.
                */
                RGBClass trycolor = palette[index];
                trycolor.Adjust(frac, palette[color]); // Try to match this color.

                /*
                **	Search through the remap range to find the color that should be remapped
                **	to. This special range is used for shadows or other effects that are
                **	not compounded if additively applied.
                */
                int best = -1;
                int bvalue = 0;
                for (int id = PaletteClass::COLOR_COUNT - 16; id < PaletteClass::COLOR_COUNT - 1; id++) {
                    int diff = palette[id].Difference(trycolor);
                    if (best == -1 || diff < bvalue) {
                        best = id;
                        bvalue = diff;
                    }
                }
                *ptr++ = best;
            }
        }
    }
    return (dest);
}

#ifdef OBSOLETE
// int Desired_Facing8(int x1, int y1, int x2, int y2)
DirType xDesired_Facing8(int x1, int y1, int x2, int y2)
{
    int index = 0; // Facing composite value.

    /*
    **	Figure the absolute X difference. This determines
    **	if the facing is leftward or not.
    */
    int xdiff = x2 - x1;
    if (xdiff < 0) {
        index |= 0x00C0;
        xdiff = -xdiff;
    }

    /*
    **	Figure the absolute Y difference. This determines
    **	if the facing is downward or not. This also clarifies
    **	exactly which quadrant the facing lies.
    */
    int ydiff = y1 - y2;
    if (ydiff < 0) {
        index ^= 0x0040;
        ydiff = -ydiff;
    }

    /*
    **	Determine which of the two direction offsets it bigger. The
    **	offset direction that is bigger (X or Y) will indicate which
    **	orthogonal direction the facing is closer to.
    */
    unsigned bigger;
    unsigned smaller;
    if (xdiff < ydiff) {
        smaller = xdiff;
        bigger = ydiff;
    } else {
        smaller = ydiff;
        bigger = xdiff;
    }

    /*
    **	If on the diagonal, then incorporate this into the facing
    **	and then bail. The facing is known.
    */
    if (((bigger + 1) / 2) <= smaller) {
        index += 0x0020;
        return (DirType(index));
    }

    /*
    **	Determine if the facing is closer to the Y axis or
    **	the X axis.
    */
    int adder = (index & 0x0040);
    if (xdiff == bigger) {
        adder ^= 0x0040;
    }
    index += adder;

    return (DirType(index));
}

// int Desired_Facing256(int srcx, int srcy, int dstx, int dsty)
DirType xDesired_Facing256(int srcx, int srcy, int dstx, int dsty)
{
    int composite = 0; // Facing built from intermediate calculations.

    /*
    **	Fetch the absolute X difference. This also gives a clue as
    **	to which hemisphere the direction lies.
    */
    int xdiff = dstx - srcx;
    if (xdiff < 0) {
        composite |= 0x00C0;
        xdiff = -xdiff;
    }

    /*
    **	Fetch the absolute Y difference. This clarifies the exact
    **	quadrant that the direction lies.
    */
    int ydiff = srcy - dsty;
    if (ydiff < 0) {
        composite ^= 0x0040;
        ydiff = -ydiff;
    }

    /*
    **	Bail early if the coordinates are the same. This check also
    **	has the added bonus of ensuring that checking for division
    **	by zero is not needed in the following section.
    */
    if (xdiff == 0 && ydiff == 0)
        return (DirType(0xFF));

    /*
    **	Determine which of the two direction offsets it bigger. The
    **	offset direction that is bigger (X or Y) will indicate which
    **	orthogonal direction the facing is closer to.
    */
    unsigned bigger;
    unsigned smaller;
    if (xdiff < ydiff) {
        smaller = xdiff;
        bigger = ydiff;
    } else {
        smaller = ydiff;
        bigger = xdiff;
    }

    /*
    **	Now that the quadrant is known, we need to determine how far
    **	from the orthogonal directions, the facing lies. This value
    **	is calculated as a ratio from 0 (matches orthogonal) to 31
    **	(matches diagonal).
    */
    int frac = (smaller * 32U) / bigger;

    /*
    **	Given the quadrant and knowing whether the facing is closer
    **	to the X or Y axis, we must make an adjustment toward the
    **	subsequent quadrant if necessary.
    */
    int adder = (composite & 0x0040);
    if (xdiff > ydiff) {
        adder ^= 0x0040;
    }
    if (adder) {
        frac = (adder - frac) - 1;
    }

    /*
    **	Integrate the fraction value into the quadrant.
    */
    composite += frac;

    /*
    **	Return with the final facing value.
    */
    return (DirType(composite & 0x00FF));
}
#endif
