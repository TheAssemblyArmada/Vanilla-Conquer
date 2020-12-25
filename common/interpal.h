//
// Copyright 2020 Electronic Arts Inc.
//
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
#ifndef COMMON_INTERPAL_H
#define COMMON_INTERPAL_H

class GraphicBufferClass;
class GraphicViewPortClass;

#define SIZE_OF_PALETTE 256
extern unsigned char* InterpolationPalette;
extern bool InterpolationPaletteChanged;
extern unsigned char PaletteInterpolationTable[SIZE_OF_PALETTE][SIZE_OF_PALETTE];

void Interpolate_2X_Scale(GraphicBufferClass* source, GraphicViewPortClass* dest, char const* palette_file_name);
void Read_Interpolation_Palette(char const* palette_file_name);
void Write_Interpolation_Palette(char const* palette_file_name);
void Increase_Palette_Luminance(unsigned char* InterpolationPalette,
                                int RedPercentage,
                                int GreenPercentage,
                                int BluePercentage,
                                int cap);

#endif
