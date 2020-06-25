#include <windows.h>
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

#endif
