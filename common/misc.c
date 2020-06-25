#include <windows.h> // for windows types, sigh
#include <stdint.h>
#include "miscasm.h"

#ifdef NOASM

int __cdecl calcx(signed short param1, short distance)
{
    return (param1 * distance) / 128;
}

int __cdecl calcy(signed short param1, short distance)
{
    return (param1 * distance) / 128;
}

unsigned int __cdecl Cardinal_To_Fixed(unsigned int base, unsigned int cardinal)
{
    return base ? (cardinal << 16) / base : -1;
}

unsigned int __cdecl Fixed_To_Cardinal(unsigned int base, unsigned int fixed)
{
    return (fixed * base + 32768) >> 16;
}

int __cdecl Desired_Facing256(LONG x1, LONG y1, LONG x2, LONG y2)
{
    int8_t unk1 = 0;

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

    return 0;
}

int __cdecl Desired_Facing8(long x1, long y1, long x2, long y2)
{
    int xdiff = 0;
    int ydiff = 0;
    char dirtype = 0;

    xdiff = x2 - x1;

    if (xdiff < 0) {
        dirtype = -64;
        xdiff = -xdiff;
    }

    ydiff = y1 - y2;

    if (ydiff < 0) {
        dirtype ^= 0x40;
        ydiff = -ydiff;
    }

    unsigned int lower_diff;

    if (xdiff >= ydiff) {
        lower_diff = ydiff;
        ydiff = xdiff;
    } else {
        lower_diff = xdiff;
    }

    char ranged_dir;

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
