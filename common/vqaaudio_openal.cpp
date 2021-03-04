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
#include "audio.h"
#include "vqafile.h"
#include "vqaloader.h"
#include "vqatask.h"
#include <al.h>
#include <alc.h>
#include <algorithm>
#include <chrono>

int AudioFlags;
int TimerIntCount;
int TimerMethod;
int VQATickCount;
int TickOffset;
unsigned VQAAudioPaused;
VQAHandle* AudioVQAHandle;

// 8192 has some chopping issues, like its not overlapping correctlying between each chunk?
// 8192 * 4 seems to fix the above for the short sample but there is still slight chopping when INTRO is played.
#define BUFFER_CHUNK_SIZE 8192 * 4 // was 8192 in RA 8192 is 186ms?
#define TIMER_DELAY       16
#define TIMER_RESOLUTION  1
#define TARGET_RESOLUTION 10 // 10-millisecond target resolution

static ALenum Get_OpenAL_Format(int bits, int channels)
{
    switch (bits) {
    case 16:
        if (channels > 1) {
            return AL_FORMAT_STEREO16;
        } else {
            return AL_FORMAT_MONO16;
        }
    case 8:
        if (channels > 1) {
            return AL_FORMAT_STEREO8;
        } else {
            return AL_FORMAT_MONO8;
        }
    default:
        return -1;
    }
}

static const char* Get_OpenAL_Error(ALenum error)
{
    switch (error) {
    case AL_NO_ERROR:
        return "No OpenAL error occured.";

    case AL_INVALID_NAME:
        return "OpenAL invalid device name.";

    case AL_INVALID_ENUM:
        return "OpenAL invalid Enum.";

    case AL_INVALID_VALUE:
        return "OpenAL invalid value.";

    case AL_INVALID_OPERATION:
        return "OpenAL invalid operation.";

    case AL_OUT_OF_MEMORY:
        return "OpenAL out of memory.";

    default:
        break;
    }

    return "Unknown OpenAL error.";
}

bool Queue_Audio(ALuint buffer)
{
    VQAConfig* config = &AudioVQAHandle->Config;
    VQAData* data = AudioVQAHandle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (!alIsSource(audio->OpenALSource)) {
        return false;
    }

    alBufferData(buffer,
                 Get_OpenAL_Format(audio->BitsPerSample, audio->Channels),
                 &audio->Buffer[audio->PlayPosition],
                 config->HMIBufSize,
                 audio->SampleRate);
    alSourceQueueBuffers(audio->OpenALSource, 1, &buffer);
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

void VQA_AudioCallback()
{
    if (!VQAAudioPaused && AudioVQAHandle) {
        VQAConfig* config = &AudioVQAHandle->Config;
        VQAData* data = AudioVQAHandle->VQABuf;
        VQAAudio* audio = &data->Audio;

        if (alIsSource(audio->OpenALSource)) {
            ALint processed_buffers;

            // Work out if we have any space to buffer more data right now.
            alGetSourcei(audio->OpenALSource, AL_BUFFERS_PROCESSED, &processed_buffers);
            if (processed_buffers > 0) {
                ALuint buffer;
                alSourceUnqueueBuffers(audio->OpenALSource, 1, &buffer);
                Queue_Audio(buffer);
            }
        }
    }
}

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

    // If its already open, close the OpenAL source buffer
    if (alIsSource(audio->OpenALSource)) {
        ALint queued = -1;
        alGetSourcei(audio->OpenALSource, AL_BUFFERS_QUEUED, &queued);
        alSourceStop(audio->OpenALSource);

        while (queued > 0) {
            ALuint tmp;
            alSourceUnqueueBuffers(audio->OpenALSource, 1, &tmp);
            --queued;
        }

        alDeleteSources(1, &audio->OpenALSource);
        alDeleteBuffers(OPENAL_BUFFER_COUNT, audio->AudioBuffers);
    }

    alGenBuffers(OPENAL_BUFFER_COUNT, audio->AudioBuffers);
    alGenSources(1, &audio->OpenALSource);

    audio->BuffBytes = config->HMIBufSize * 4;

    audio->field_B0 = 0;
    audio->field_B4 = 0;

    for (unsigned int AudioBuffer : audio->AudioBuffers) {
        Queue_Audio(AudioBuffer);
    }

    alSourcef(audio->OpenALSource, AL_GAIN, config->Volume / 256.0f);
    alSourcePlay(audio->OpenALSource);

    audio->Flags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    AudioFlags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    return 0;
}

void VQA_StopAudio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER && alIsSource(audio->OpenALSource)) {
        ALint queued = -1;
        alGetSourcei(audio->OpenALSource, AL_BUFFERS_QUEUED, &queued);
        alSourceStop(audio->OpenALSource);

        while (queued > 0) {
            ALuint tmp;
            alSourceUnqueueBuffers(audio->OpenALSource, 1, &tmp);
            --queued;
        }

        alDeleteSources(1, &audio->OpenALSource);
        alDeleteBuffers(OPENAL_BUFFER_COUNT, audio->AudioBuffers);

        audio->Flags &= ~VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
        AudioFlags &= ~VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    }

    AudioVQAHandle = nullptr;
}

void VQA_PauseAudio()
{
    if (AudioVQAHandle) {
        VQAData* data = AudioVQAHandle->VQABuf;

        if (data != nullptr) {
            ALuint source = data->Audio.OpenALSource;

            if (alIsSource(source)) {
                if (AudioFlags & 0x40) {
                    if (!VQAAudioPaused) {
                        alSourcePause(source);
                        VQAAudioPaused = VQA_GetTime(AudioVQAHandle);
                    }
                }
            }
        }
    }
}

void VQA_ResumeAudio()
{
    if (AudioVQAHandle) {
        VQAData* data = AudioVQAHandle->VQABuf;
        if (data) {
            ALuint source = data->Audio.OpenALSource;

            if (alIsSource(source)) {
                if (AudioFlags & 0x40) {
                    if (VQAAudioPaused) {
                        alSourcePlay(source);
                        TickOffset -= VQA_GetTime(AudioVQAHandle) - VQAAudioPaused;
                        VQAAudioPaused = 0;
                    }
                }
            }
        }
    }
}

int VQA_CopyAudio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

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
    if (method == -1) {
        if (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
            method = VQA_AUDIO_TIMER_METHOD_DMA;
        } else if (AudioFlags & (VQA_AUDIO_FLAG_UNKNOWN016 | VQA_AUDIO_FLAG_UNKNOWN032)) {
            method = VQA_AUDIO_TIMER_METHOD_INTERRUPT;
        } else {
            method = VQA_AUDIO_TIMER_METHOD_DOS;
        }
    } else {
        if (!(AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) && method == 3) {
            method = VQA_AUDIO_TIMER_METHOD_INTERRUPT;
        }

        if (!(AudioFlags & (VQA_AUDIO_FLAG_UNKNOWN016 | VQA_AUDIO_FLAG_UNKNOWN032))
            && method == VQA_AUDIO_TIMER_METHOD_INTERRUPT) {
            method = VQA_AUDIO_TIMER_METHOD_DOS;
        }
    }

    TimerMethod = method;
    TickOffset = 0;
    TickOffset = time - VQA_GetTime(handle);
}

unsigned VQA_GetTime(VQAHandle* handle)
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    unsigned result_time =
        unsigned(TickOffset + 60 * (std::chrono::duration_cast<std::chrono::milliseconds>(now).count()) / 1000);

    return result_time;
}

int VQA_TimerMethod()
{
    return TimerMethod;
}
