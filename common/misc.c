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

struct RGB {
    uint8_t Red, Green, Blue;
};

static void RGB_Adjust(struct RGB *_this, int ratio, const struct RGB *rgb)
{
    /*
    **	Ratio conversion is limited to 0 through 100%. This is
    **	the range of 0 to 255.
    */
    ratio &= 0x00FF;

    /*
    **	Adjust the color guns by the ratio specified toward the
    **	destination color.
    */
    int value = (int)rgb->Red - (int)_this->Red;
    _this->Red = (int)_this->Red + (value * ratio) / 256;

    value = (int)rgb->Green - (int)_this->Green;
    _this->Green = (int)_this->Green + (value * ratio) / 256;

    value = (int)rgb->Blue - (int)_this->Blue;
    _this->Blue = (int)_this->Blue + (value * ratio) / 256;
}

static int RGB_Difference(struct RGB *_this, const struct RGB *rgb)
{
    int r = (int)_this->Red - (int)rgb->Red;
    if (r < 0)
        r = -r;

    int g = (int)_this->Green - (int)rgb->Green;
    if (g < 0)
        g = -g;

    int b = (int)_this->Blue - (int)rgb->Blue;
    if (b < 0)
        b = -b;

    return (r * r + g * g + b * b);
}

void* __cdecl Conquer_Build_Fading_Table(void const* palette, void* fade_table, int color, int frac)
{
    if (fade_table) {
        const struct RGB *rgb_palette = palette;
        uint8_t *dst = (uint8_t *)(fade_table);
        struct RGB const *target_col = &rgb_palette[color];

        for (int i = 0; i < 256; ++i) {
            if (i <= 240 && i) {
                struct RGB *tmp = &rgb_palette[i];
                RGB_Adjust(tmp, frac, target_col);

                int index = 0;
                int prevdiff = -1;

                for (int j = 240; j < 255; ++j) {
                    int difference = RGB_Difference(&rgb_palette[j], tmp);

                    if (prevdiff == -1 || difference < prevdiff) {
                        index = j;
                        prevdiff = difference;
                    }
                }

                dst[i] = index;
            } else {
                dst[i] = i;
            }
        }
    }

    return fade_table;
}

long __cdecl Reverse_Long(long number)
{
    return ((number >> 24) & 0xFF) |
           ((number >> 8)  & 0xFF00) |
           ((number << 8)  & 0xFF0000) |
           ((number << 24) & 0xFF000000);
}

short __cdecl Reverse_Short(short number)
{
    return (number >> 8) | (number << 8);
}

long __cdecl Swap_Long(long number)
{
    return (number >> 16) | (number << 16);
}

void __cdecl strtrim(char* buffer)
{
    char *start = buffer;
    char *end = start + strlen(start) - 1;

    if (end <= start)
        return;

    while (end > start && (*end == '\t' || *end == ' '))
        end--;

    while (start < end && (*start == '\t' || *start == ' '))
        start++;

    *++end = '\0';

    memmove(buffer, start, strlen(start) + 1);
}

#endif
