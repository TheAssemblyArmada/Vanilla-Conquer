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

void __cdecl Set_Bit(void* array, int bit, int value)
{
    // Don't try and handle a negative value
    if (bit < 0) {
        return;
    }

    uint8_t *byte_array = (uint8_t *)array;

    if (value) {
        byte_array[bit / 8] |= 1 << (bit % 8);
    } else {
        byte_array[bit / 8] &= ~(1 << (bit % 8));
    }
}

int __cdecl Get_Bit(void const* array, int bit)
{
    // If negative it is out of range so can't be set
    if (bit < 0) {
        return 0;
    }

    const uint8_t *byte_array = (const uint8_t *)array;

    return byte_array[bit / 8] & (1 << (bit % 8)) ? 1 : 0;
}

int __cdecl First_True_Bit(void const* array)
{
    const uint8_t *byte_array = (const uint8_t *)array;

    int bytenum = 0;
    int bitnum = 0;

    // Find the first none zero byte as it must contain the first bit.
    for (bytenum = 0; /*bytenum < size*/; ++bytenum) {
        if (byte_array[bytenum] != 0) {
            break;
        }
    }

#if 0
    if (bytenum >= size) {
        return 8 * bytenum;
    }
#endif

    // Scan through the bits of the byte until we find the first set bit.
    for (bitnum = 0; bitnum < 8; ++bitnum) {
        if (Get_Bit(&byte_array[bytenum], bitnum)) {
            break;
        }
    }

    return 8 * bytenum + bitnum;
}

int __cdecl First_False_Bit(void const* array)
{
    const uint8_t *byte_array = (const uint8_t *)array;

    int bytenum = 0;
    int bitnum = 0;

    // Find the first byte with an unset bit as it must contain the first bit.
    for (bytenum = 0; /*bytenum < size*/; ++bytenum) {
        if (byte_array[bytenum] != 0xFF) {
            break;
        }
    }

#if 0
    if (bytenum >= size) {
        return 8 * bytenum;
    }
#endif

    // Scan through the bits until we find one that isn't set.
    for (bitnum = 0; bitnum < 8; ++bitnum) {
        if (!Get_Bit(&byte_array[bytenum], bitnum)) {
            break;
        }
    }

    return 8 * bytenum + bitnum;
}

int __cdecl _Bound(int original, int min, int max)
{
    if (original < min) {
        return min;
    }

    if (original > max) {
        return max;
    }

    return original;
}

#endif
