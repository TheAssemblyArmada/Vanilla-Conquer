#include "miscasm.h"
#include <stdbool.h>

unsigned short __rotl16(unsigned short a, unsigned b)
{
    b &= 15;
    return (a << b) | (a >> (16 - b));
}

#ifdef NOASM

int calcx(signed short param1, short distance)
{
    int tmp = (int)param1 * distance;
    return (((unsigned short)tmp / 128) & 0xFF) | ((__rotl16(tmp >> 16, 1) << 8) & 0xFF00);
}

int calcy(signed short param1, short distance)
{
    int tmp = (int)param1 * distance;
    return -((((unsigned short)tmp / 128) & 0xFF) | ((__rotl16(tmp >> 16, 1) << 8) & 0xFF00));
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

#endif
