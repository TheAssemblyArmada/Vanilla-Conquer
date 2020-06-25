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

#endif
