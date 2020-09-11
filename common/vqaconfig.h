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
#ifndef VQACONFIG_H
#define VQACONFIG_H

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <dsound.h>
#endif

enum VQADrawEnum
{
    VQACFGF_TOPLEFT = 0,
    VQACFGF_BUFFER = 1 << 0,
    VQACFGF_NOSKIP = 1 << 2,
};

enum VQAOptionEnum
{
    VQAOPTF_AUDIO = 1 << 0,
    VQAOPTF_SINGLESTEP = 1 << 1,
    VQAOPTF_MONO = 1 << 2,
    VQA_OPTION_8 = 1 << 3,
    VQAOPTF_SLOWPAL = 1 << 4,
    VQAOPTF_HMIINIT = 1 << 5,
    VQAOPTF_ALTAUDIO = 1 << 6,
    VQAOPTF_CAPTIONS = 1 << 7,
    VQAOPTF_EVA = 1 << 8,
    VQA_OPTION_512 = 1 << 9,
};

enum VQALanguageType
{
    VQA_LANG_ENGLISH = 0
};

// Need to confirm it does return,
typedef long (*DrawerCallbackFuncPtr)(unsigned char* buffer, long frame_number);
typedef int (*EventHandlerFuncPtr)(int mode, uint8_t* pal, int pal_size);

typedef struct _VQAConfig
{
    DrawerCallbackFuncPtr DrawerCallback;
    EventHandlerFuncPtr EventHandler;
    int NotifyFlags;
    int Vmode;
    int VBIBit;
    uint8_t* ImageBuf;
    int ImageWidth;
    int ImageHeight;
    int X1;
    int Y1;
    int FrameRate;
    int DrawRate;
    int TimerMethod;
    int DrawFlags;
    int OptionFlags; // VQAOptionEnum
    int NumFrameBufs;
    int NumCBBufs;
#ifdef _WIN32
    LPDIRECTSOUND SoundObject;
    LPDIRECTSOUNDBUFFER PrimaryBufferPtr;
#endif
    void* VocFile;
    uint8_t* AudioBuf;
    unsigned AudioBufSize;
    int AudioRate;
    int Volume;
    int HMIBufSize;
    int DigiHandle;
    int DigiCard;
    int DigiPort;
    int DigiIRQ;
    int DigiDMA;
    int Language;
    void* CapFont;
    void* EVAFont;
} VQAConfig;

#ifdef __cplusplus
extern "C" {
#endif
void VQA_INIConfig(VQAConfig* config);
void VQA_DefaultConfig(VQAConfig* dest);
#ifdef __cplusplus
}
#endif

#endif
