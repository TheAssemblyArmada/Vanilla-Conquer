#include "compat.h"
#include "setfpal.h"

extern unsigned char ColorXlat[16][16];

void __cdecl Set_Font_Palette_Range(void const* palette, int start_idx, int end_idx)
{
    start_idx &= 15;
    end_idx &= 15;
    const unsigned char* colors = (const unsigned char*)palette;

    for (int i = start_idx; i < end_idx + 1; ++i) {
        ColorXlat[0][i] = *colors;
        ColorXlat[i][0] = *colors++;
    }
}
