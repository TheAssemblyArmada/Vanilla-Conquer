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
#ifndef VQADRAWER_H
#define VQADRAWER_H

#include <stdint.h>

typedef struct _VQAData VQAData;
typedef struct _VQAFrameNode VQAFrameNode;
typedef struct _VQAHandle VQAHandle;

typedef struct
{
    unsigned Mode;
    unsigned XRes;
    unsigned YRes;
    unsigned VBIbit;
    unsigned Extended;
} DisplayInfo;

typedef struct
{
    VQAFrameNode* CurFrame;
    unsigned Flags;
    DisplayInfo* Display;
    uint8_t* ImageBuf;
    int ImageWidth;
    int ImageHeight;
    int X1;
    int Y1;
    int X2;
    int Y2;
    int ScreenOffset;
    int CurPalSize;
    uint8_t Palette[768];
    uint8_t Palette_15[512];
    int BlocksPerRow;
    int NumRows;
    int NumBlocks;
    int MaskStart;
    int MaskWidth;
    int MaskHeight;
    int LastTime;
    int LastFrame;
    int LastFrameNum;
    int DesiredFrame;
    int NumSkipped;
    int WaitsOnFlipper;
    int WaitsOnLoader;
} VQADrawer;

uint8_t* VQA_GetPalette(VQAHandle* handle);
int VQA_GetPaletteSize(VQAHandle* handle);
void VQA_SetDrawBuffer(VQAHandle* handle, uint8_t* buffer, unsigned width, unsigned height, int xpos, int ypos);
void VQA_ConfigureDrawer(VQAHandle* handle);
int VQA_SelectFrame(VQAHandle* handle);
void VQA_PrepareFrame(VQAData* data);

#endif
