#include "miscasm.h"
#include <stdbool.h>

#ifdef NOASM

int calcx(signed short param1, short distance)
{
    int tmp = (int)param1 * distance;
    return (unsigned short)(tmp >> 7);
}

int calcy(signed short param1, short distance)
{
    int tmp = (int)param1 * distance;
    return -((unsigned short)(tmp >> 7));
}

unsigned int Cardinal_To_Fixed(unsigned int base, unsigned int cardinal)
{
    return base ? (cardinal << 8) / base : 0xFFFF;
}

unsigned int Fixed_To_Cardinal(unsigned int base, unsigned int fixed)
{
    int tmp = (fixed * base + 128);
    return tmp & 0xFF000000 ? 0xFFFF : tmp >> 8;
}

void Set_Bit(void* array, int bit, int value)
{
    // Don't try and handle a negative value
    if (bit < 0) {
        return;
    }

    unsigned char* byte_array = (unsigned char*)array;

    if (value) {
        byte_array[bit / 8] |= 1 << (bit % 8);
    } else {
        byte_array[bit / 8] &= ~(1 << (bit % 8));
    }
}

int Get_Bit(const void* array, int bit)
{
    // If negative it is out of range so can't be set
    if (bit < 0) {
        return false;
    }

    const unsigned char* byte_array = (const unsigned char*)array;

    return byte_array[bit / 8] & (1 << (bit % 8)) ? true : false;
}

int First_True_Bit(const void* array)
{
    const unsigned char* byte_array = (const unsigned char*)array;

    int bytenum = 0;
    int bitnum = 0;

    // Find the first none zero byte as it must contain the first bit.
    while (byte_array[bytenum] == 0) {
        ++bytenum;
    }

    // Scan through the bits of the byte until we find the first set bit.
    for (bitnum = 0; bitnum < 8; ++bitnum) {
        if (Get_Bit(&byte_array[bytenum], bitnum)) {
            break;
        }
    }

    return 8 * bytenum + bitnum;
}

int First_False_Bit(const void* array)
{
    const unsigned char* byte_array = (const unsigned char*)array;

    int bytenum = 0;
    int bitnum = 0;

    // Find the first byte with an unset bit as it must contain the first bit.
    while (byte_array[bytenum] == 0xFF) {
        ++bytenum;
    }

    // Scan through the bits until we find one that isn't set.
    for (bitnum = 0; bitnum < 8; ++bitnum) {
        if (!Get_Bit(&byte_array[bytenum], bitnum)) {
            break;
        }
    }

    return 8 * bytenum + bitnum;
}

int _Bound(int original, int min, int max)
{
    if (original < min) {
        return min;
    }
    
    if (original > max) {
        return max;
    }

    return original;
}

void* Conquer_Build_Fading_Table(const void* palette, void* dest, int color, int frac)
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
    for (int i = 1; i < ALLOWED_START; ++i) {
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

        const unsigned char* fade = pal + ALLOWED_START * 3;
        unsigned matchcolor = color;
        unsigned matchvalue = (unsigned)(-1);
        unsigned matchindex = ALLOWED_START;

        // Find the closest match that actually exists in the allowed palette range for the adjusted color.
        for (int j = 0; j < ALLOWED_COUNT; ++j) {
            // Build a distance of our current color to the ideal faded color.
            signed char diff = *fade++ - idealred;
            unsigned value = diff * diff;
            diff = *fade++ - idealgreen;
            value += diff * diff;
            diff = *fade++ - idealblue;
            value += diff * diff;

            if (value < matchvalue) {
                matchvalue = value;
                matchcolor = matchindex;
            }

            // Perfect match, use it.
            if (value == 0) {
                break;
            }

            ++matchindex;
        }

        dst[i] = matchcolor;
    }

    // Make sure last 16 values just remap to themselves.
    for (int i = ALLOWED_START; i < 256; ++i) {
        dst[i] = i;
    }

    return dest;
}

#endif
