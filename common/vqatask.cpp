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
#include "vqatask.h"
#include "vqaaudio.h"
#include "vqaconfig.h"
#include "vqadrawer.h"
#include "vqafile.h"
#include "vqaloader.h"
#include <string.h>

bool VQAMovieDone = false;

VQAHandle* VQA_Alloc(void)
{
    VQAHandle* handle = (VQAHandle*)malloc(sizeof(VQAHandle));

    if (handle) {
        memset(handle, 0, sizeof(VQAHandle));
    }

    return handle;
}

void VQA_Free(void* block)
{
    if (block) {
        free(block);
    }
}

void VQA_SetStreamHandler(VQAHandle* handle, StreamHandlerFuncPtr streamhandler)
{
    handle->StreamHandler = (StreamHandlerFuncPtr)streamhandler;
}

// Sets Stream Handler function, Poly has 2, Direct Disk to stream and a disk to memory to stream function
void VQA_Init(VQAHandle* handle, StreamHandlerFuncPtr streamhandler)
{
    VQA_SetStreamHandler(handle, streamhandler);
}

// From RA
void VQA_Reset(VQAHandle* handle)
{
    VQAData* data = handle->VQABuf;
    data->Flags = VQA_DATA_FLAG_ZERO;
    data->LoadedFrames = 0;
    data->DrawnFrames = 0;
    data->StartTime = 0;
    data->EndTime = 0;
}

// VQAPlayMode
// 0 = play
// 1 =
// 2 =
// 3 =
VQAErrorType VQA_Play(VQAHandle* handle, VQAPlayMode mode)
{
    VQAErrorType rc = VQAERR_NONE;

#ifdef _WIN32
    // RA code
    DWORD priority_class = GetPriorityClass(GetCurrentProcess());
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
    VQAData* data = handle->VQABuf;
    VQAConfig* config = &handle->Config;
    VQADrawer* drawer = &data->Drawer;

    if (!(data->Flags & VQA_DATA_FLAG_32)) {
        VQA_ConfigureDrawer(handle);

        // RA code
        if (config->OptionFlags & VQAOPTF_AUDIO) {
            if (data->Audio.IsLoaded) {
                VQA_StartAudio(handle);
            }
        }

        VQA_SetTimer(handle, 60 * data->Drawer.CurFrame->FrameNum / config->DrawRate, config->TimerMethod);

        data->StartTime = VQA_GetTime(handle);

        if (config->OptionFlags & VQAOPTF_MONO) {
            // VQA_InitMono(handle);
        }

        data->Flags |= VQA_DATA_FLAG_32;
    }

    if (mode <= 1 || mode != 2) {
        if (data->Flags & VQA_DATA_FLAG_64) {
            data->Flags &= ~VQA_DATA_FLAG_64;

            // RA code
            if (config->OptionFlags & VQAOPTF_AUDIO && VQA_StartAudio(handle) != VQAERR_NONE) {
                VQA_StopAudio(handle);
                return VQAERR_ERROR;
            }

            VQA_SetTimer(handle, data->EndTime, config->TimerMethod);
        }

        while (mode != 1) {
            if (data->Flags & (VQA_DATA_FLAG_VIDEO_MEMORY_SET | VQA_DATA_FLAG_8)) {
                break;
            }

            if (data->Flags & VQA_DATA_FLAG_VIDEO_MEMORY_SET) {
                VQAMovieDone = true;
            } else {
                rc = (VQAErrorType)VQA_LoadFrame(handle);

                if (rc != VQAERR_NONE) {
                    if (rc != VQAERR_NOBUFFER && rc != VQAERR_SLEEPING) {
                        data->Flags |= VQA_DATA_FLAG_VIDEO_MEMORY_SET;
                        rc = VQAERR_NONE;
                    }

                } else {
                    ++data->LoadedFrames;
                }
            }

            if (config->DrawFlags & 2) {
                data->Flags |= VQA_DATA_FLAG_8;
                drawer->CurFrame->Flags = 0;
                drawer->CurFrame = drawer->CurFrame->Next;

            } else {
                rc = (VQAErrorType)data->Draw_Frame(handle);

                if (rc != VQAERR_NONE) {
                    if (rc == VQAERR_ERROR) {
                        break;
                    }

                    if (data->Flags & VQA_DATA_FLAG_VIDEO_MEMORY_SET && rc == VQAERR_NOBUFFER) {
                        data->Flags |= VQA_DATA_FLAG_8;
                    }
                } else {
                    ++data->DrawnFrames;

                    VQA_UserUpdate(handle);

                    if ((data->Page_Flip)(handle) != 0) {
                        data->Flags |= (VQA_DATA_FLAG_VIDEO_MEMORY_SET | VQA_DATA_FLAG_8);
                    }
                }
            }

            if (config->OptionFlags & VQAOPTF_MONO) {
                // VQA_UpdateMono(handle);
            }
        }
    } else {
        if (!(data->Flags & VQA_DATA_FLAG_64)) {
            data->Flags |= VQA_DATA_FLAG_64;
            data->EndTime = VQA_GetTime(handle);

            // RA code
            if (data->Audio.Flags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
                VQA_StopAudio(handle);
            }
        }

        rc = VQAERR_PAUSED;
    }

    if (data->Flags & (VQA_DATA_FLAG_VIDEO_MEMORY_SET | VQA_DATA_FLAG_8) || mode == 3) {
        data->EndTime = VQA_GetTime(handle);

        rc = VQAERR_ERROR;
    }

    if (mode != 1) {
        if (data->Audio.Flags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
            VQA_StopAudio(handle);
        }
    }
#ifdef PLATFORM_WINDOWS
    // RA code
    SetPriorityClass(GetCurrentProcess(), priority_class);
#endif
    return rc;
}

int16_t VQA_SetStop(VQAHandle* handle, int16_t frame)
{
    int16_t stopframe = -1;
    VQAHeader* header = &handle->Header;

    if (frame <= 0 || header->Frames < frame) {
        return stopframe;
    }

    stopframe = header->Frames;
    header->Frames = frame;
    return stopframe;
}

int VQA_SetDrawFlags(VQAHandle* handle, int flags)
{
    int oldflags = handle->Config.DrawFlags;
    handle->Config.DrawFlags |= flags;
    return oldflags;
}

int VQA_ClearDrawFlags(VQAHandle* handle, int flags)
{
    int oldflags = handle->Config.DrawFlags;
    handle->Config.DrawFlags &= ~flags;
    return oldflags;
}

void VQA_GetInfo(VQAHandle* handle, VQAInfo* info)
{
    info->NumFrames = handle->Header.Frames;
    info->ImageHeight = handle->Header.ImageHeight;
    info->ImageWidth = handle->Header.ImageWidth;
    info->ImageBuf = handle->VQABuf->Drawer.ImageBuf;
}

void VQA_GetStats(VQAHandle* handle, VQAStatistics* stats)
{
    VQAData* data = handle->VQABuf;

    stats->MemUsed = data->MemUsed;
    stats->StartTime = data->StartTime;
    stats->EndTime = data->EndTime;
    stats->FramesLoaded = data->LoadedFrames;
    stats->FramesDrawn = data->DrawnFrames;
    stats->FramesSkipped = data->Drawer.NumSkipped;
    stats->MaxFrameSize = data->Loader.MaxFrameSize;
    stats->SamplesPlayed = data->Audio.SamplesPlayed;
}

// Function found in BR, appears its only use there is to force BH,BW and CM to 0;
VQAErrorType
VQA_GetBlockWH_ColorMode(VQAHandle* handle, unsigned int* blockwidth, unsigned int* blockheight, int* colormode)
{
    *blockwidth = handle->Header.BlockWidth;
    *blockheight = handle->Header.BlockHeight;
    *colormode = handle->Header.ColorMode;

    return VQAERR_ERROR;
}

// Function found in BR;
VQAErrorType VQA_GetXYPos(VQAHandle* handle, uint16_t* xpos, uint16_t* ypos)
{
    *xpos = handle->Header.Xpos;
    *ypos = handle->Header.Ypos;

    return VQAERR_ERROR;
}

VQAErrorType VQA_UserUpdate(VQAHandle* handle)
{
    VQAErrorType rc = VQAERR_NONE;
    VQAData* data = handle->VQABuf;

    if (data->Flags & VQA_DATA_FLAG_REPEAT_SAME_TAG) {
        rc = (VQAErrorType)data->Page_Flip(handle);
        data->Flipper.LastFrameNum = data->Flipper.CurFrame->FrameNum;
        data->Flipper.CurFrame->Flags = 0;
        data->Flags &= ~VQA_DATA_FLAG_REPEAT_SAME_TAG;
    }

    return rc;
}
