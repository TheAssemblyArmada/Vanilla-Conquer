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
#ifndef REMAPCONTROL_H
#define REMAPCONTROL_H

// Moved from RA defines.h to be used in code moved to common.

/****************************************************************************
**	This entry defines a complete color scheme, with the player's remap table,
** the font remap table, and a color scheme for dialog boxes and buttons.
*/
typedef struct RemapControlType
{
    unsigned char BrightColor;     // Highlight (bright) color index.
    unsigned char Color;           // Normal color index.
    unsigned char RemapTable[256]; // Actual remap table.
    unsigned char FontRemap[16];   // Remap table for gradient font.
    unsigned char Shadow;          // Color of shadowed edge of a raised button.
    unsigned char Background;      // Background fill color for buttons.
    unsigned char Corners;         // Transition color between shadow and highlight.
    unsigned char Highlight;       // Bright edge of raised button.
    unsigned char Box;             // Color for dialog box border.
    unsigned char Bright;          // Color used for highlighted text.
    unsigned char Underline;       // Color for underlining dialog box titles.
    unsigned char Bar;             // Selected entry list box background color.
} RemapControlType;

#endif
