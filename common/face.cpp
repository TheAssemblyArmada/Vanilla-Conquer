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
#include "face.h"

#ifdef NOASM
int Desired_Facing256(long x1, long y1, long x2, long y2)
{
    signed char unk1 = 0;

    int x_diff = x2 - x1;

    if (x_diff < 0) {
        x_diff = -x_diff;
        unk1 = -64;
    }

    int y_diff = y1 - y2;

    if (y_diff < 0) {
        unk1 ^= 64;
        y_diff = -y_diff;
    }

    int s_diff;
    unsigned l_diff;

    if (x_diff != 0 || y_diff != 0) {
        if (x_diff >= y_diff) {
            s_diff = y_diff;
            l_diff = x_diff;
        } else {
            s_diff = x_diff;
            l_diff = y_diff;
        }

        unsigned unk2 = 32 * s_diff / l_diff;
        int ranged_dir = unk1 & 64;

        if (x_diff > y_diff) {
            ranged_dir = ranged_dir ^ 64;
        }

        if (ranged_dir != 0) {
            unk2 = ranged_dir - unk2 - 1;
        }

        return (unk2 + unk1) & 255;
    }

    return 255;
}

int Desired_Facing8(long x1, long y1, long x2, long y2)
{
    int xdiff = 0;
    int ydiff = 0;
    unsigned char dirtype = 0;

    xdiff = x2 - x1;

    if (xdiff < 0) {
        dirtype = 192;
        xdiff = -xdiff;
    }

    ydiff = y1 - y2;

    if (ydiff < 0) {
        dirtype ^= 64;
        ydiff = -ydiff;
    }

    unsigned lower_diff;

    if (xdiff >= ydiff) {
        lower_diff = ydiff;
        ydiff = xdiff;
    } else {
        lower_diff = xdiff;
    }

    unsigned char ranged_dir;

    if (((unsigned)(ydiff + 1) >> 1) > lower_diff) {
        ranged_dir = ((unsigned)dirtype) & 64;

        if (xdiff == ydiff) {
            ranged_dir ^= 64;
        }

        return dirtype + ranged_dir;
    }

    return dirtype + 32;
}

#endif
