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
#include <algorithm>
#include <sys/timeb.h>

int AudioFlags;
int TimerIntCount;
MMRESULT VQATimer;
int TimerMethod;
int VQATickCount;
int TickOffset;
int SuspendAudioCallback;
int VQAAudioPaused;
VQAHandle* AudioVQAHandle;

// 8192 has some chopping issues, like its not overlapping correctlying between each chunk?
// 8192 * 4 seems to fix the above for the short sample but there is still slight chopping when INTRO is played.
#define BUFFER_CHUNK_SIZE 8192 * 4 // was 8192 in RA 8192 is 186ms?
#define TIMER_DELAY       16
#define TIMER_RESOLUTION  1
#define TARGET_RESOLUTION 10 // 10-millisecond target resolution

bool Move_HMI_Audio_Block_To_Direct_Sound_Buffer()
{
    VQAConfig* config = &AudioVQAHandle->Config;
    VQAData* data = AudioVQAHandle->VQABuf;
    VQAAudio* audio = &data->Audio;
    void* ptr1;
    DWORD bytes1;
    void* ptr2;
    DWORD bytes2;
    DWORD status;

    if (audio->PrimaryBufferPtr->Lock(audio->field_B0, config->HMIBufSize, &ptr1, &bytes1, &ptr2, &bytes2, 0) != DS_OK
        || config->PrimaryBufferPtr->GetStatus(&status) == DSERR_BUFFERLOST) {
        return false;
    }

    memcpy(ptr1, &audio->Buffer[audio->PlayPosition], config->HMIBufSize);
    audio->PrimaryBufferPtr->Unlock(ptr1, bytes1, ptr2, bytes2);
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

void __stdcall VQA_SoundTimerCallback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    ++VQATickCount;
}

void __stdcall VQA_AudioCallback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    if (!SuspendAudioCallback && !VQAAudioPaused) {
        VQAConfig* config = &AudioVQAHandle->Config;
        VQAData* data = AudioVQAHandle->VQABuf;
        VQAAudio* audio = &data->Audio;

        if (audio->PrimaryBufferPtr != nullptr) {
            BOOL buffer_restored = false;
            BOOL restart = false;
            DWORD playCursor;
            DWORD writeOffset;
            DWORD status;

            if (audio->PrimaryBufferPtr->GetCurrentPosition(&playCursor, &writeOffset) == DSERR_BUFFERLOST
                || config->PrimaryBufferPtr->GetStatus(&status) == DSERR_BUFFERLOST) {
                config->PrimaryBufferPtr->Restore();
                audio->PrimaryBufferPtr->Restore();
                audio->PrimaryBufferPtr->Stop();
                audio->field_B0 = 0;
                audio->field_B8 = 0;

                buffer_restored = true;
                restart = true;
            }

            if (playCursor >= audio->field_B0) {
                if (((audio->BuffBytes * 3) / 4) < playCursor && audio->field_B0 <= 0) {
                    restart = true;
                }
            } else if ((audio->field_B0 - playCursor) <= (audio->BuffBytes / 4)) {
                restart = true;
            }

            if (restart && Move_HMI_Audio_Block_To_Direct_Sound_Buffer() && buffer_restored) {
                audio->PrimaryBufferPtr->Play(0, 0, DSBPLAY_LOOPING);
            }
        }
    }
}

int VQA_StartTimerInt(VQAHandle* handle, int a2)
{
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (!(AudioFlags & VQA_AUDIO_FLAG_INTERRUPT_TIMER)) {
        VQATimer = timeSetEvent(TIMER_DELAY,
                                TIMER_RESOLUTION,
                                VQA_SoundTimerCallback,
                                (DWORD_PTR) nullptr,
                                TIME_CALLBACK_FUNCTION | TIME_PERIODIC);

        if (!VQATimer) {
            return -1;
        }

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

    if ((AudioFlags & VQA_AUDIO_FLAG_INTERRUPT_TIMER) == VQA_AUDIO_FLAG_UNKNOWN016 && TimerIntCount == 0) {
        timeKillEvent(VQATimer);
    }

    AudioFlags &= ~VQA_AUDIO_FLAG_INTERRUPT_TIMER;
}

int VQA_OpenAudio(VQAHandle* handle, void* hwnd)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAHeader* header = &handle->Header;
    VQAAudio* audio = &data->Audio;
    audio->field_10 = 0;
    HRESULT dsretval;

    if (config->SoundObject == nullptr) {
        if ((dsretval = DirectSoundCreate(nullptr, &config->SoundObject, nullptr)) != DS_OK) {
            return -1;
        }

        audio->field_BC = 1;

        if ((dsretval = config->SoundObject->SetCooperativeLevel((HWND)hwnd, DSSCL_PRIORITY)) != DS_OK) {
            config->SoundObject->Release();
            config->SoundObject = nullptr;
            return -1;
        }

        if (config->PrimaryBufferPtr == nullptr) {

            // Set up DSBUFFERDESC structure.
            memset(&audio->BufferDesc, 0, sizeof(audio->BufferDesc));
            audio->BufferDesc.dwSize = sizeof(audio->BufferDesc);
            audio->BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;

            // Set up buffer format structure.
            memset(&audio->BuffFormat, 0, sizeof(audio->BuffFormat));
            audio->BuffFormat.wFormatTag = WAVE_FORMAT_PCM; // Format code
            audio->BuffFormat.nChannels = 2;                // Number of interleaved channels

            // Sampling rate (blocks per second)
            if (config->AudioRate == -1) {
                int rate = 0;

                if (header->FPS == config->FrameRate) {
                    audio->BuffFormat.nSamplesPerSec = audio->SampleRate;
                    rate = audio->SampleRate;
                } else {
                    rate = config->FrameRate * audio->SampleRate / header->FPS;
                    audio->BuffFormat.nSamplesPerSec = rate;
                }

                config->AudioRate = rate;
            } else {
                audio->BuffFormat.nSamplesPerSec = config->AudioRate;
            }

            audio->BuffFormat.wBitsPerSample = 16; // Bits per sample
            audio->BuffFormat.nBlockAlign =
                audio->BuffFormat.nChannels * audio->BuffFormat.wBitsPerSample / 8; // Data block size (bytes)
            audio->BuffFormat.cbSize = 0;                                           // Size of the extension (0 or 22)
            audio->BuffFormat.nAvgBytesPerSec = audio->BuffFormat.nBlockAlign * audio->BuffFormat.nSamplesPerSec;

            if ((dsretval =
                     config->SoundObject->CreateSoundBuffer(&audio->BufferDesc, &config->PrimaryBufferPtr, nullptr))
                != DS_OK) {
                config->SoundObject->Release();
                config->SoundObject = nullptr;

                return -1;
            }

            audio->field_C0 = 1;
        }

        if (audio->field_BC) {
            config->SoundObject->Release();
            config->SoundObject = nullptr;
            audio->field_BC = 0;
        }
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
        config->PrimaryBufferPtr->Stop();
        config->PrimaryBufferPtr->Release();
        config->PrimaryBufferPtr = 0;
        audio->field_C0 = 0;
    }

    if (audio->field_BC) {
        config->SoundObject->Release();
        config->SoundObject = nullptr;
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

    // If the timer is already set up, abort.
    if (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
        return -1;
    }

    // If its already open, close the DSound buffer
    if (audio->PrimaryBufferPtr != nullptr) {
        audio->PrimaryBufferPtr->Stop();
        audio->PrimaryBufferPtr->Release();
        audio->PrimaryBufferPtr = nullptr;
    }

    audio->BuffBytes = config->HMIBufSize * 4;

    // Set up DSBUFFERDESC structure.
    memset(&audio->BufferDesc, 0, sizeof(audio->BufferDesc));
    audio->BufferDesc.dwSize = sizeof(audio->BufferDesc);
    audio->BufferDesc.dwFlags = DSBCAPS_CTRLVOLUME; // Buffer flags
    audio->BufferDesc.dwBufferBytes = audio->BuffBytes;
    audio->BufferDesc.lpwfxFormat = &audio->BuffFormat;

    // Set up buffer format structure.
    memset(&audio->BuffFormat, 0, sizeof(audio->BuffFormat));
    audio->BuffFormat.nSamplesPerSec = audio->SampleRate;
    audio->BuffFormat.nChannels = audio->Channels;
    audio->BuffFormat.wBitsPerSample = audio->BitsPerSample;
    audio->BuffFormat.wFormatTag = WAVE_FORMAT_PCM; // Format code
    audio->BuffFormat.nBlockAlign =
        audio->BuffFormat.nChannels * (audio->BuffFormat.wBitsPerSample / 8); // Data block size (bytes)
    audio->BuffFormat.nAvgBytesPerSec = audio->BuffFormat.nBlockAlign * audio->BuffFormat.nSamplesPerSec;
    config->SoundObject->CreateSoundBuffer(&audio->BufferDesc, &audio->PrimaryBufferPtr, nullptr);
    config->PrimaryBufferPtr->Stop();

    if (config->PrimaryBufferPtr->SetFormat(&audio->BuffFormat) != DS_OK) {
        if (audio->BitsPerSample == 16) {
            audio->BuffFormat.wBitsPerSample = 8;

            int tmp_bits = audio->BuffFormat.wBitsPerSample;
            int tmp_bytes = audio->BuffFormat.nAvgBytesPerSec;
            int tmp_align = audio->BuffFormat.nBlockAlign;

            audio->BuffFormat.nBlockAlign =
                audio->BuffFormat.nChannels * audio->BuffFormat.wBitsPerSample / 8; // Data block size (bytes)
            audio->BuffFormat.nAvgBytesPerSec = audio->BuffFormat.nBlockAlign * audio->BuffFormat.nSamplesPerSec;
            config->PrimaryBufferPtr->SetFormat(&audio->BuffFormat);
            audio->BuffFormat.nBlockAlign = tmp_align;
            audio->BuffFormat.wBitsPerSample = tmp_bits;
            audio->BuffFormat.nAvgBytesPerSec = tmp_bytes;
        }
    }

    if (config->PrimaryBufferPtr->Play(0, 0, DSBPLAY_LOOPING) != DS_OK) {
        return -1;
    }

    audio->field_B0 = 0;
    audio->field_B4 = 0;
    Move_HMI_Audio_Block_To_Direct_Sound_Buffer();
    Move_HMI_Audio_Block_To_Direct_Sound_Buffer();
    audio->PrimaryBufferPtr->SetCurrentPosition(0);

    if (audio->PrimaryBufferPtr->Play(0, 0, DSBPLAY_LOOPING) != DS_OK) {
        return -1;
    }

    audio->PrimaryBufferPtr->SetVolume(-(1000 * (0x8000 - (config->Volume * 128)) / 0x8000));
    timeBeginPeriod(TIMER_DELAY);
    audio->SoundTimerHandle = timeSetEvent(
        TIMER_DELAY, TIMER_RESOLUTION, VQA_AudioCallback, (DWORD_PTR) nullptr, TIME_CALLBACK_FUNCTION | TIME_PERIODIC);
    audio->Flags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    AudioFlags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    return 0;
}

void VQA_StopAudio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
        config->PrimaryBufferPtr->Stop();
        timeKillEvent(audio->SoundTimerHandle);
        timeEndPeriod(TIMER_DELAY);
        audio->SoundTimerHandle = 0;

        if (audio->PrimaryBufferPtr) {
            audio->PrimaryBufferPtr->Stop();
            audio->PrimaryBufferPtr->Release();
            audio->PrimaryBufferPtr = nullptr;
        }

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
            LPDIRECTSOUNDBUFFER buffer = data->Audio.PrimaryBufferPtr;

            if (buffer != nullptr) {
                if (AudioFlags & 0x40) {
                    if (!VQAAudioPaused) {
                        buffer->Stop();
                        VQAAudioPaused = 1;
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
            LPDIRECTSOUNDBUFFER buffer = data->Audio.PrimaryBufferPtr;
            VQAAudio* audio = &data->Audio;

            if (buffer) {
                if (AudioFlags & 0x40) {
                    if (VQAAudioPaused) {
                        buffer->SetCurrentPosition(0);
                        audio->field_B8 = 0;
                        audio->field_B0 = 0;
                        Move_HMI_Audio_Block_To_Direct_Sound_Buffer();
                        buffer->Play(0, 0, DSBPLAY_LOOPING);
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
            method = 3;
        } else if (AudioFlags & (VQA_AUDIO_FLAG_UNKNOWN016 | VQA_AUDIO_FLAG_UNKNOWN032)) {
            method = 2;
        } else {
            method = 1;
        }
    } else {
        if (!(AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) && method == 3) {
            method = 2;
        }

        if (!(AudioFlags & (VQA_AUDIO_FLAG_UNKNOWN016 | VQA_AUDIO_FLAG_UNKNOWN032)) && method == 2) {
            method = 1;
        }
    }

    TimerMethod = method;
    TickOffset = 0;
    TickOffset = time - VQA_GetTime(handle);
}

unsigned VQA_GetTime(VQAHandle* handle)
{
    static unsigned last_chunksmovedtoaudiobuffer;
    static unsigned last_totalbytes;
    static unsigned last_ticks;
    static unsigned last_playcursor;
    static unsigned last_hmibufsize;
    static unsigned last_lastchunkposition;
    static unsigned this_chunksmovedtoaudiobuffer;
    static unsigned this_totalbytes;
    static unsigned this_ticks;
    static unsigned this_playcursor;
    static unsigned this_hmibufsize;
    static unsigned this_lastchunkposition;
    unsigned ticks = 0;
    unsigned bytes = 0;
    unsigned result_time = 0;
    DWORD play_cursor;
    DWORD write_offset;
    VQAData* data = handle->VQABuf;
    VQAConfig* config = &handle->Config;
    VQAAudio* audio = &handle->VQABuf->Audio;

    switch (VQA_TimerMethod()) {
    default:
    case VQA_AUDIO_TIMER_METHOD_DOS: {
        struct timeb mytime;
        ftime(&mytime);
        result_time = (TickOffset + 60 * (mytime.millitm + 1000 * mytime.time) / 1000);
        break;
    }

    case VQA_AUDIO_TIMER_METHOD_INTERRUPT: {
        result_time = TickOffset + VQATickCount;
        break;
    }

    case VQA_AUDIO_TIMER_METHOD_DMA: {
        this_chunksmovedtoaudiobuffer = audio->field_B4;
        this_hmibufsize = config->HMIBufSize;

        if (audio->PrimaryBufferPtr != nullptr) {
            if (audio->PrimaryBufferPtr->GetCurrentPosition(&play_cursor, &write_offset) != DS_OK) {
                bytes = config->HMIBufSize * (audio->field_B4 - 1);
            } else {
                int v5 = config->HMIBufSize * audio->field_B4;
                this_playcursor = play_cursor;
                this_lastchunkposition = audio->field_B8;

                if (this_lastchunkposition > 0) {
                    bytes = play_cursor - audio->field_B8 + v5;
                } else {
                    if ((3 * audio->BuffBytes) >> 2 >= play_cursor) {
                        bytes = play_cursor - audio->field_B8 + v5;
                    } else {
                        bytes = v5 - (audio->BuffBytes - play_cursor);
                    }
                }
            }
        } else {
            bytes = config->HMIBufSize * (audio->field_B4 - 1);
        }

        this_totalbytes = bytes;
        result_time =
            TickOffset
            + 60 * (bytes / audio->BuffFormat.nChannels / (audio->BuffFormat.wBitsPerSample / 8)) / audio->SampleRate;

        break;
    }
    };

    if (result_time <= 100 || (ticks = last_ticks, result_time - last_ticks <= 20)) {
        last_chunksmovedtoaudiobuffer = this_chunksmovedtoaudiobuffer;
        last_totalbytes = this_totalbytes;
        last_ticks = this_ticks;
        last_playcursor = this_playcursor;
        last_hmibufsize = this_hmibufsize;
        last_lastchunkposition = this_lastchunkposition;
    } else if (VQA_TimerMethod() == VQA_AUDIO_TIMER_METHOD_DMA) {
        ++last_ticks;
        result_time = ticks + 1;
    }

    return result_time;
}

int VQA_TimerMethod()
{
    return TimerMethod;
}
