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
#ifndef VQAPALETTE_H
#define VQAPALETTE_H

#include <stdint.h>

extern uint8_t* VQPalette;
extern int VQNumBytes;
extern bool VQSlowpal;
extern bool VQPaletteChange;

void VQA_Flag_To_Set_Palette(uint8_t* palette, int numbytes, bool slowpal);
void VQA_SetPalette(uint8_t* palette, int numbytes, bool slowpal);
void Check_VQ_Palette_Set();

#endif // VQAPALETTE_H
