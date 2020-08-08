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
#include "fading.h"

#ifdef NOASM

void* Build_Fading_Table(void const* palette, void* dest, int color, int frac)
{
    const int ALLOWED_COUNT = 16;
    const int ALLOWED_START = 256 - ALLOWED_COUNT;

    if (!palette || !dest) {
        return 0;
    }

    const unsigned char* pal = palette;
    unsigned char* dst = dest;

    if ((unsigned)frac >= 256) {
        frac = 255;
    }

    int fraction = frac >> 1;
    unsigned palindex = color * 3;
    unsigned char targetred = pal[palindex++];
    unsigned char targetgreen = pal[palindex++];
    unsigned char targetblue = pal[palindex];
    unsigned tableindex = 0;
    dst[0] = 0;

    // Remap most pal entries to the last 16 entries that are the most faded colours.
    for (int i = 1; i < 256; ++i) {
        // Decide what the "perfect" match would be for our adjusted color.
        palindex = i * 3;
        signed char original = pal[palindex++];
        signed short tmp = ((original - targetred) * fraction) << 1;
        unsigned char idealred = original - (tmp >> 8);
        original = pal[palindex++];
        tmp = ((original - targetgreen) * fraction) << 1;
        unsigned char idealgreen = original - (tmp >> 8);
        original = pal[palindex];
        tmp = ((original - targetblue) * fraction) << 1;
        unsigned char idealblue = original - (tmp >> 8);

        const unsigned char* fade = pal + 3; // Skip first entry.
        unsigned matchcolor = color;
        unsigned matchvalue = (unsigned)(-1);

        // Find the closest match that actually exists in the allowed palette range for the adjusted color.
        for (int j = 1; j < 256; ++j) {
            if (i == j) {
                fade += 3;
                continue;
            }

            // Build a distance of our current color to the ideal faded color.
            signed char diff = *fade++ - idealred;
            unsigned value = diff * diff;
            diff = *fade++ - idealgreen;
            value += diff * diff;
            diff = *fade++ - idealblue;
            value += diff * diff;

            // In the event of a tie, this will match the later color in the palette.
            if (value <= matchvalue) {
                matchvalue = value;
                matchcolor = j;
            }

            // Perfect match, use it.
            if (value == 0) {
                break;
            }
        }

        dst[i] = matchcolor;
    }

    return dest;
}

#endif
