#include "miscasm.h"

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

#endif
