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
#ifndef VQAAUDIO_H
#define VQAAUDIO_H

#include "soscomp.h"
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <dsound.h>
#endif

typedef struct _VQAHandle VQAHandle;

typedef enum
{
    VQA_AUDIO_TIMER_METHOD_M1 = -1,
    VQA_AUDIO_TIMER_METHOD_NONE = 0, // TODO: Needs confirming
    VQA_AUDIO_TIMER_METHOD_DOS = 1,
    VQA_AUDIO_TIMER_METHOD_INTERRUPT = 2,
    VQA_AUDIO_TIMER_METHOD_DMA = 3, // DMA - Direct memory access
} VQATimerMethod;

typedef enum
{
    // VQA_AUDIO_FLAG_DOS_TIMER = unknown value,//Method 1
    VQA_AUDIO_FLAG_UNKNOWN001 = 0x1,
    VQA_AUDIO_FLAG_UNKNOWN002 = 0x2,
    VQA_AUDIO_FLAG_UNKNOWN004 = 0x4,
    VQA_AUDIO_FLAG_UNKNOWN008 = 0x8,
    VQA_AUDIO_FLAG_UNKNOWN016 = 0x10,
    VQA_AUDIO_FLAG_UNKNOWN032 = 0x20,
    VQA_AUDIO_FLAG_INTERRUPT_TIMER = 0x30, // VQA_AUDIO_FLAG_UNKNOWN016 | VQA_AUDIO_FLAG_UNKNOWN032, // bit 48//Method 2
    VQA_AUDIO_FLAG_AUDIO_DMA_TIMER = 0x40, // Method 3
    VQA_AUDIO_FLAG_UNKNOWN128 = 0x80,
} VQAAudioFlags;

extern int AudioFlags;

typedef struct
{
    uint8_t* Buffer;
    unsigned AudBufPos;
    int16_t* IsLoaded;
    unsigned NumAudBlocks;
    unsigned field_10; // block position?
    unsigned field_14; // block position for IsLoaded?
    uint8_t* TempBuf;
    unsigned TempBufSize;
    unsigned TempBufLen;
    unsigned Flags;
    unsigned PlayPosition;
    unsigned SamplesPlayed;
    unsigned NumSkipped;
    uint16_t SampleRate;  // 22050, 44100, etc.
    int8_t Channels;      // Mono = 1, Stereo = 2, etc.
    int8_t BitsPerSample; // 8 bits = 8, 16 bits = 16, etc.
    int BytesPerSecond;   // Bytes or samples per second?
    _SOS_COMPRESS_INFO AdcpmInfo;
#ifdef _WIN32
    MMRESULT SoundTimerHandle;
#endif
    int field_7C;
#ifdef _WIN32
    LPDIRECTSOUNDBUFFER PrimaryBufferPtr;
    WAVEFORMATEX BuffFormat;
    DSBUFFERDESC BufferDesc;
#endif
    int16_t field_AA;
    unsigned BuffBytes; // HMIBufSize * 4 set field_AC, and field_AC sets dwBufferBytes.
    unsigned field_B0;  // current chunk position or offset?
    int field_B4;       // use to set ChunksMovedToAudioBuffer in VQA_GetTime
    int field_B8;       // use to set LastChunkPosition in VQA_GetTime
    int field_BC;       // set when the sound object is created.
    int field_C0;       // set when the sound buffer is created.
} VQAAudio;

#ifdef __cplusplus
extern "C" {
#endif

int VQA_StartTimerInt(VQAHandle* handle, int a2);
void VQA_StopTimerInt(VQAHandle* handle);

int VQA_OpenAudio(VQAHandle* handle, void* hwnd);
void VQA_CloseAudio(VQAHandle* handle);

int VQA_StartAudio(VQAHandle* handle);
void VQA_StopAudio(VQAHandle* handle);
void VQA_PauseAudio();
void VQA_ResumeAudio();

int VQA_CopyAudio(VQAHandle* handle);

void VQA_SetTimer(VQAHandle* handle, int time, int method);
unsigned VQA_GetTime(VQAHandle* handle);
int VQA_TimerMethod();

#ifdef __cplusplus
}
#endif

#endif // VQAAUDIO_H
