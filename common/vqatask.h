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
#ifndef VQATASK_H
#define VQATASK_H

#include "vqafile.h"

typedef struct _VQAInfo
{
    int NumFrames;
    int ImageWidth;
    int ImageHeight;
    uint8_t* ImageBuf; // New in newer code (RA onwards)
} VQAInfo;

typedef struct _VQAStatistics
{
    int StartTime;
    int EndTime;
    int FramesLoaded;
    int FramesDrawn;
    int FramesSkipped;
    int MaxFrameSize;
    unsigned SamplesPlayed;
    unsigned MemUsed;
} VQAStatistics;

typedef enum
{
    VQAMODE_RUN,
} VQAPlayMode;

extern bool VQAMovieDone;

VQAHandle* VQA_Alloc();
void VQA_Free(void* block);
void VQA_Init(VQAHandle* vqa, StreamHandlerFuncPtr streamhandler);
void VQA_Reset(VQAHandle* vqa);
VQAErrorType VQA_Play(VQAHandle* vqa, VQAPlayMode mode);
void VQA_SetStreamHandler(VQAHandle* vqa, StreamHandlerFuncPtr streamhandler);
int16_t VQA_SetStop(VQAHandle* vqa, int16_t frame);
int VQA_SetDrawFlags(VQAHandle* vqa, int flags);
int VQA_ClearDrawFlags(VQAHandle* vqa, int flags);
void VQA_GetInfo(VQAHandle* vqa, VQAInfo* info);
void VQA_GetStats(VQAHandle* vqa, VQAStatistics* stats);
VQAErrorType VQA_GetBlockWH_ColorMode(VQAHandle* vqa, unsigned* blockwidth, unsigned* blockheight, int* colormode);
VQAErrorType VQA_GetXYPos(VQAHandle* vqa, uint16_t* xpos, uint16_t* ypos);
VQAErrorType VQA_UserUpdate(VQAHandle* vqa);

#endif
