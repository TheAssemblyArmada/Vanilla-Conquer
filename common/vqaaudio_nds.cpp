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
#include "vqaaudio.h"
#include "vqafile.h"
#include "vqaloader.h"
#include "vqatask.h"
#include "timer.h"
#include <algorithm>
#include <sys/timeb.h>
#include <cstdio>
#include <cstring>
#include <nds/fifocommon.h>
#include "audio_fifocommon.h"

#define CALLED printf("%s called\n", __func__)

void Start_Primary_Sound_Buffer(bool);

int AudioFlags;
int TimerIntCount;
int TimerMethod;
int VQATickCount;
int TickOffset;
unsigned VQAAudioPaused;
VQAHandle* AudioVQAHandle;

static TimerClass Timer;

int VQA_StartTimerInt(VQAHandle* handle, int a2)
{
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (!(AudioFlags & VQA_AUDIO_FLAG_INTERRUPT_TIMER)) {
        AudioFlags |= VQA_AUDIO_FLAG_UNKNOWN016;
    }

    audio->Flags |= VQA_AUDIO_FLAG_UNKNOWN016;
    ++TimerIntCount;
    return 0;
}

void VQA_StopTimerInt(VQAHandle* handle)
{
    if (TimerIntCount > 0) {
        --TimerIntCount;
    }

    AudioFlags &= ~VQA_AUDIO_FLAG_INTERRUPT_TIMER;
}

int VQA_OpenAudio(VQAHandle* handle, void* hwnd)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAHeader* header = &handle->Header;
    VQAAudio* audio = &data->Audio;

    // We are assuming here that the main game already created an OpenAL device context.
    Start_Primary_Sound_Buffer(true);

    audio->field_10 = 0;
    audio->field_BC = 1;

    // Sampling rate (blocks per second)
    if (config->AudioRate == -1) {
        int rate = 0;

        if (header->FPS == config->FrameRate) {
            rate = audio->SampleRate;
        } else {
            rate = config->FrameRate * audio->SampleRate / header->FPS;
        }

        config->AudioRate = rate;
    }

    audio->field_C0 = 1;

    if (audio->field_BC) {
        audio->field_BC = 0;
    }

    audio->Flags |= VQA_AUDIO_FLAG_UNKNOWN001;
    AudioFlags |= VQA_AUDIO_FLAG_UNKNOWN001;
    return 0;
}

bool Queue_Audio(unsigned buffer)
{
    VQAConfig* config = &AudioVQAHandle->Config;
    VQAData* data = AudioVQAHandle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (!data)
        return false;

    audio->field_B8 = audio->field_B0;
    audio->field_B0 += config->HMIBufSize;

    if (audio->field_B0 >= audio->BuffBytes) {
        audio->field_B0 = 0;
    }

    audio->field_14 = audio->field_10 + 1;

    if (audio->field_14 >= audio->NumAudBlocks) {
        audio->field_14 = 0;
    }

    if (audio->IsLoaded[audio->field_14] != 1) {
        if (VQAMovieDone) {
            ++audio->field_B4;
        }

        ++audio->NumSkipped;
        config->DrawFlags &= 0xFB;

        return false;
    }

    audio->IsLoaded[audio->field_10] = 0;
    audio->PlayPosition += config->HMIBufSize;
    ++audio->field_10;

    if (audio->PlayPosition >= config->AudioBufSize) {
        audio->PlayPosition = 0;
        audio->field_10 = 0;
    }

    ++audio->field_B4;
    return true;
}

void VQA_CloseAudio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    VQA_StopAudio(handle);
    AudioFlags &= ~(VQA_AUDIO_FLAG_UNKNOWN004 | VQA_AUDIO_FLAG_UNKNOWN008);
    audio->Flags &= ~(VQA_AUDIO_FLAG_UNKNOWN004 | VQA_AUDIO_FLAG_UNKNOWN008);

    if (audio->field_C0) {
        audio->field_C0 = 0;
    }

    if (audio->field_BC) {
        audio->field_BC = 0;
    }

    audio->Flags &= ~(VQA_AUDIO_FLAG_UNKNOWN001 | VQA_AUDIO_FLAG_UNKNOWN002);
    AudioFlags &= ~(VQA_AUDIO_FLAG_UNKNOWN001 | VQA_AUDIO_FLAG_UNKNOWN002 | VQA_AUDIO_FLAG_AUDIO_DMA_TIMER);
}

int VQA_StartAudio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    AudioVQAHandle = handle;

    // Audio already started, abort.
    if (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
        return -1;
    }

    audio->BuffBytes = config->HMIBufSize * 4;

    audio->field_B0 = 0;
    audio->field_B4 = 0;

    audio->Flags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    AudioFlags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;

    USR1::FifoMessage msg;

    msg.type = USR1::SOUND_VQA_MESSAGE;
    msg.SoundVQAChunk.data = audio->Buffer;
    msg.SoundVQAChunk.freq = audio->SampleRate;
    msg.SoundVQAChunk.size = 65536;
    msg.SoundVQAChunk.volume = config->Volume;
    msg.SoundVQAChunk.bits = audio->BitsPerSample;

    // Asynchronous send sound play command
    fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);

    return 0;
}

void VQA_StopAudio(VQAHandle* handle)
{
    fifoSendValue32(FIFO_USER_01, USR1::SOUND_KILL);
    AudioVQAHandle = nullptr;
}

void VQA_PauseAudio()
{
    Timer.Stop();
}

void VQA_ResumeAudio()
{
    Timer.Start();
}

void VQA_AudioCallback()
{
    if (!VQAAudioPaused && AudioVQAHandle) {
        Queue_Audio(0);
    }
}

int VQA_CopyAudio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (!audio)
        return 0;

    VQA_AudioCallback();

    if (config->OptionFlags & 1) {
        if (audio->Buffer != nullptr) {
            if (audio->TempBufSize > 0) {
                int current_block = audio->AudBufPos / config->HMIBufSize;
                int next_block = (audio->TempBufSize + audio->AudBufPos) / config->HMIBufSize;

                if ((unsigned)next_block >= audio->NumAudBlocks) {
                    next_block -= audio->NumAudBlocks;
                }

                if (audio->IsLoaded[next_block] == 1) {
                    return -10;
                }

                // Need to loop back and treat like circular buffer?
                if (next_block < current_block) {
                    int end_space = config->AudioBufSize - audio->AudBufPos;
                    int remaining = audio->TempBufSize - end_space;
                    memcpy(&audio->Buffer[audio->AudBufPos], audio->TempBuf, end_space);
                    memcpy(audio->Buffer, &audio->TempBuf[end_space], remaining);

                    audio->AudBufPos += audio->TempBufSize;
                    audio->TempBufSize = 0;

                    audio->AudBufPos = remaining;
                    audio->TempBufSize = 0;

                    for (unsigned i = current_block; i < audio->NumAudBlocks; ++i) {
                        audio->IsLoaded[i] = 1;
                    }

                    for (int i = 0; i < next_block; ++i) {
                        audio->IsLoaded[i] = 1;
                    }
                } else {
                    memcpy(&audio->Buffer[audio->AudBufPos], audio->TempBuf, audio->TempBufSize);

                    audio->AudBufPos += audio->TempBufSize;
                    audio->TempBufSize = 0;

                    for (int i = current_block; i < next_block; ++i) {
                        audio->IsLoaded[i] = 1;
                    }
                }
            }
        }
    }
    return 0;
}

void VQA_SetTimer(VQAHandle* handle, int time, int method)
{
    Timer.Set(0);
}

unsigned VQA_GetTime(VQAHandle* handle)
{
    return Timer.Time();
}

int VQA_TimerMethod()
{
    return 0;
}
