// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#include "vqapalette.h"
#include "interpal.h"
#include "palette.h"
#include "winasm.h"
#include <string.h>

uint8_t* VQPalette;
int VQNumBytes;
bool VQSlowpal;
bool VQPaletteChange = false;

extern unsigned char* InterpolatedPalettes[100];
extern bool PalettesRead;
extern unsigned PaletteCounter;

/**
 * Flags a VQA Palette change.
 */
void VQA_Flag_To_Set_Palette(uint8_t* palette, int numbytes, bool slowpal)
{
    VQPalette = palette;
    VQNumBytes = numbytes;
    VQSlowpal = slowpal;
    VQPaletteChange = true;
}

/**
 * Changes the VQA Palette.
 */
void VQA_SetPalette(uint8_t* palette, int numbytes, bool slowpal)
{
    for (int i = 0; i < 768; ++i) {
        palette[i] &= 0x3Fu;
    }

    Increase_Palette_Luminance(palette, 15, 15, 15, 63);

    if (PalettesRead) {
        memcpy(PaletteInterpolationTable, InterpolatedPalettes[PaletteCounter++], sizeof(PaletteInterpolationTable));
    }

    Set_Palette(palette);
}

/**
 * Changes the VQA Palette after a call to VQA_Flag_To_Set_Palette.
 */
void Check_VQ_Palette_Set()
{
    if (VQPaletteChange) {
        VQA_SetPalette(VQPalette, VQNumBytes, VQSlowpal);
        VQPaletteChange = false;
    }
}
