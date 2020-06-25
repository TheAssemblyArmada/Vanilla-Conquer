#include <windows.h> // for windows types, sigh
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

#endif
