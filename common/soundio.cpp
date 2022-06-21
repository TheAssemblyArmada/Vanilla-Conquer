#include "soundint.h"
#include "auduncmp.h"
#include "soscomp.h"
#include "memflag.h"
#include "file.h"
#include "audio.h"
#include "misc.h"
#include <algorithm>
#include <math.h>
#include <stdint.h>

extern bool GameInFocus;

void (*Audio_Focus_Loss_Function)(void) = nullptr;
bool StreamLowImpact = false;

// These includes must not be resorted.
#include <mmsystem.h>
#include <dsound.h>

enum
{
    AUD_CHUNK_MAGIC_ID = 0x0000DEAF,
    VOLUME_MIN = 0,
    VOLUME_MAX = 255,
    PRIORITY_MIN = 0,
    PRIORITY_MAX = 255,
    MAX_SAMPLE_TRACKERS = 5, // C&C issue where sounds get cut off is because of the small number of trackers.
    STREAM_BUFFER_COUNT = 16,
    BUFFER_CHUNK_SIZE = 8192, // 256 * 32,
    UNCOMP_BUFFER_SIZE = 2098,
    BUFFER_TOTAL_BYTES = BUFFER_CHUNK_SIZE * 4, // 32 kb
    TIMER_DELAY = 25,
    TIMER_RESOLUTION = 1,
    TIMER_TARGET_RESOLUTION = 10, // 10-millisecond target resolution
    INVALID_AUDIO_HANDLE = -1,
    INVALID_FILE_HANDLE = -1,
};

bool ReverseChannels;
LockedDataType LockedData;
LPDIRECTSOUND SoundObject;
LPDIRECTSOUNDBUFFER DumpBuffer;
LPDIRECTSOUNDBUFFER PrimaryBufferPtr;
WAVEFORMATEX DsBuffFormat;
WAVEFORMATEX PrimaryBuffFormat;
DSBUFFERDESC BufferDesc;
DSBUFFERDESC PrimaryBufferDesc;
CRITICAL_SECTION GlobalAudioCriticalSection;
void* SoundThreadHandle;
bool SoundThreadActive;
bool StartingFileStream;
MemoryFlagType StreamBufferFlag;
//int Misc;
UINT SoundTimerHandle;
void* FileStreamBuffer;
bool volatile AudioDone;
SFX_Type SoundType;
Sample_Type SampleType;

// Forward declare some internal functions.
bool Attempt_Audio_Restore(LPDIRECTSOUNDBUFFER sound_buffer);
void CALLBACK
Sound_Timer_Callback(UINT uID = 0, UINT uMsg = 0, DWORD_PTR dwUser = 0, DWORD_PTR dw1 = 0, DWORD_PTR dw2 = 0);
int Simple_Copy(void** source, int* ssize, void** alternate, int* altsize, void** dest, int size);
int Sample_Copy(SampleTrackerType* st,
                void** source,
                int* ssize,
                void** alternate,
                int* altsize,
                void* dest,
                int size,
                SCompressType sound_comp,
                void* trailer,
                int16_t* trailersize);

int Convert_HMI_To_Direct_Sound_Volume(int vol)
{
    // Complete silence.
    if (vol <= 0) {
        return DSBVOLUME_MIN;
    }

    // Max volume.
    if (vol >= 255) {
        return DSBVOLUME_MAX;
    }

    // Dark magic.
    double v = exp((255.0 - vol) * log(double(10000 + 1)) / 255.0);

    // Simple clamping. 10000.99 would clamp to 9999.99.
    // Flip the value as we need a inverted value for DirectSound.
    return int(-(v + -1.0));
}

void Maintenance_Callback()
{
    SampleTrackerType* st = LockedData.SampleTracker;
    HRESULT ret;
    DWORD play_cursor;
    DWORD write_cursor;

    for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
        if (st->Active) { // If this tracker needs processing and isn't already marked as being processed, then process it.
            if (st->Service && !st->DontTouch) {
                st->DontTouch = true;

                ret = st->PlayBuffer->GetCurrentPosition(&play_cursor, &write_cursor);

                if (ret != DS_OK) {
                    //CCDebugString("%s - Failed to get Buffer play position!", __CURRENT_FUNCTION__);
                    if (ret == DSERR_BUFFERLOST && Audio_Focus_Loss_Function != nullptr) {
                        Audio_Focus_Loss_Function();
                    }

                    return;
                }

                // Do we have more data in this tracker to play?
                if (st->MoreSource) {
                    bool write_more = false;

                    // Work out where we are relative to where we copied up to? Code suggests DestPtr was a pointer in
                    // original and was abused to hold an integer value for the DSound implementation.
                    if (play_cursor >= (unsigned)st->DestPtr) {
                        if (play_cursor > BUFFER_TOTAL_BYTES - BUFFER_CHUNK_SIZE && st->DestPtr == 0) {
                            write_more = true;
                        }
                    } else if ((unsigned)st->DestPtr - play_cursor <= BUFFER_CHUNK_SIZE) {
                        write_more = true;
                    }

                    void* play_buffer_ptr;
                    void* wrapped_buffer_ptr;
                    DWORD play_lock_len;
                    DWORD wrapped_lock_len;

                    // Lock the buffer object and copy uncompressed PCM samples to it.
                    if (write_more
                        && st->PlayBuffer->Lock((DWORD)st->DestPtr,
                                                2 * BUFFER_CHUNK_SIZE,
                                                &play_buffer_ptr,
                                                &play_lock_len,
                                                &wrapped_buffer_ptr,
                                                &wrapped_lock_len,
                                                0)
                               == DS_OK) {
                        int bytes_copied = Sample_Copy(st,
                                                       &st->Source,
                                                       (int*)&st->Remainder,
                                                       &st->QueueBuffer,
                                                       (int*)&st->QueueSize,
                                                       play_buffer_ptr,
                                                       BUFFER_CHUNK_SIZE,
                                                       st->Compression,
                                                       st->Trailer,
                                                       &st->TrailerLen);

                        // We didn't have enough data left to finish filling the chunk so fill with silence.
                        if (bytes_copied != BUFFER_CHUNK_SIZE) {
                            st->MoreSource = false;

                            if (st->BitSize != 0) {
                                memset(static_cast<char*>(play_buffer_ptr) + bytes_copied,
                                       0,
                                       BUFFER_CHUNK_SIZE - bytes_copied);
                            } else {
                                // Sole has these memsets for when bitsize is 0, possibly for 8bit unsigned audio?
                                memset(static_cast<char*>(play_buffer_ptr) + bytes_copied,
                                       0x80,
                                       BUFFER_CHUNK_SIZE - bytes_copied);
                            }
                        }

                        // This block silences the next block, ensures we don't play random garbage with buffer underruns.
                        if ((unsigned)st->DestPtr == BUFFER_TOTAL_BYTES - BUFFER_CHUNK_SIZE) {
                            if (wrapped_buffer_ptr != nullptr && wrapped_lock_len != 0) {
                                if (st->BitSize != 0) {
                                    memset(wrapped_buffer_ptr, 0, wrapped_lock_len);
                                } else {
                                    // Sole has these memsets for when bitsize is 0, possibly for 8bit unsigned audio?
                                    memset(wrapped_buffer_ptr, 0x80, wrapped_lock_len);
                                }
                            }
                        } else {
                            if (st->BitSize != 0) {
                                memset(static_cast<char*>(play_buffer_ptr) + BUFFER_CHUNK_SIZE, 0, BUFFER_CHUNK_SIZE);
                            } else {
                                // Sole has these memsets for when bitsize is 0, possibly for 8bit unsigned audio?
                                memset(
                                    static_cast<char*>(play_buffer_ptr) + BUFFER_CHUNK_SIZE, 0x80, BUFFER_CHUNK_SIZE);
                            }
                        }

                        st->DestPtr = Add_Long_To_Pointer(st->DestPtr, bytes_copied);

                        // If we reached the end of the buffer, loop back around.
                        if ((unsigned)st->DestPtr >= BUFFER_TOTAL_BYTES) {
                            st->DestPtr = Add_Long_To_Pointer(st->DestPtr, -BUFFER_TOTAL_BYTES);
                        }

                        st->PlayBuffer->Unlock(play_buffer_ptr, play_lock_len, wrapped_buffer_ptr, wrapped_lock_len);
                    }
                } else if (play_cursor >= (unsigned)st->DestPtr
                               && play_cursor - (unsigned)st->DestPtr < BUFFER_CHUNK_SIZE
                           || !st->OneShot && play_cursor < (unsigned)st->DestPtr
                                  && (unsigned)st->DestPtr - play_cursor > BUFFER_TOTAL_BYTES - BUFFER_CHUNK_SIZE) {
                    st->PlayBuffer->Stop();
                    st->Service = 0;
                    Stop_Sample(i);
                }

                st->DontTouch = false;
            }

            if (!st->DontTouch && !st->QueueBuffer && st->FilePending != 0) {
                st->QueueBuffer = static_cast<char*>(st->FileBuffer)
                                  + LockedData.StreamBufferSize * (st->Odd % LockedData.StreamBufferCount);
                --st->FilePending;
                ++st->Odd;

                if (st->FilePending != 0) {
                    st->QueueSize = LockedData.StreamBufferSize;
                } else {
                    st->QueueSize = st->FilePendingSize;
                }
            }
        }

        ++st;
    }

    // Perform any volume modifications that need to be made.
    if (LockedData._int == 0) {
        ++LockedData._int;
        st = LockedData.SampleTracker;

        for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
            if (st->Active && st->Reducer > 0 && st->Volume > 0) {
                if (st->Reducer >= st->Volume) {
                    st->Volume = VOLUME_MIN;
                } else {
                    st->Volume -= st->Reducer;
                }

                if (!st->IsScore) {
                    st->PlayBuffer->SetVolume(
                        Convert_HMI_To_Direct_Sound_Volume(LockedData.SoundVolume * (st->Volume / 128)) / 256);
                } else {
                    st->PlayBuffer->SetVolume(
                        Convert_HMI_To_Direct_Sound_Volume(LockedData.ScoreVolume * (st->Volume / 128)) / 256);
                }
            }

            ++st;
        }

        --LockedData._int;
    }
}

void Init_Locked_Data()
{
    LockedData.DigiHandle = INVALID_AUDIO_HANDLE;
    LockedData.ServiceSomething = 0;
    LockedData.MagicNumber = AUD_CHUNK_MAGIC_ID;
    LockedData.UncompBuffer = 0;
    LockedData.StreamBufferSize = BUFFER_CHUNK_SIZE + 128;
    LockedData.StreamBufferCount = STREAM_BUFFER_COUNT;
    LockedData.SoundVolume = VOLUME_MAX;
    LockedData.ScoreVolume = VOLUME_MAX;
    LockedData._int = 0;
}

bool File_Callback(short id, short* odd, void** buffer, int* size)
{
    if (id == INVALID_AUDIO_HANDLE) {
        return false;
    }

    SampleTrackerType* st = &LockedData.SampleTracker[id];

    if (st->FileBuffer == nullptr) {
        return false;
    }

    if (!AudioDone) {
        EnterCriticalSection(&GlobalAudioCriticalSection);
    }

    st->DontTouch = true;

    if (!AudioDone) {
        LeaveCriticalSection(&GlobalAudioCriticalSection);
    }

    if (*buffer == nullptr && st->FilePending) {
        *buffer =
            static_cast<char*>(st->FileBuffer) + LockedData.StreamBufferSize * (*odd % LockedData.StreamBufferCount);
        --st->FilePending;
        ++*odd;
        *size = st->FilePending == 0 ? st->FilePendingSize : LockedData.StreamBufferSize;
    }

    if (!AudioDone) {
        EnterCriticalSection(&GlobalAudioCriticalSection);
    }

    st->DontTouch = false;

    if (!AudioDone) {
        LeaveCriticalSection(&GlobalAudioCriticalSection);
    }

    Sound_Timer_Callback();

    int count = StreamLowImpact ? LockedData.StreamBufferCount / 2 : LockedData.StreamBufferCount - 3;

    if (count > st->FilePending && st->FileHandle != INVALID_FILE_HANDLE) {
        if (LockedData.StreamBufferCount - 2 != st->FilePending) {
            // Fill empty buffers.
            for (int num_empty_buffers = LockedData.StreamBufferCount - 2 - st->FilePending;
                 num_empty_buffers && st->FileHandle != INVALID_FILE_HANDLE;
                 --num_empty_buffers) {
                // Buffer to fill with data.
                void* tofill =
                    static_cast<char*>(st->FileBuffer)
                    + LockedData.StreamBufferSize * ((st->FilePending + *odd) % LockedData.StreamBufferCount);

                int psize = Read_File(st->FileHandle, tofill, LockedData.StreamBufferSize);

                if (psize != LockedData.StreamBufferSize) {
                    Close_File(st->FileHandle);
                    st->FileHandle = INVALID_FILE_HANDLE;
                }

                if (psize > 0) {
                    if (!AudioDone) {
                        EnterCriticalSection(&GlobalAudioCriticalSection);
                    }

                    st->DontTouch = true;
                    st->FilePendingSize = psize;
                    ++st->FilePending;
                    st->DontTouch = false;

                    if (!AudioDone) {
                        LeaveCriticalSection(&GlobalAudioCriticalSection);
                    }

                    Sound_Timer_Callback();
                }
            }
        }

        if (!AudioDone) {
            EnterCriticalSection(&GlobalAudioCriticalSection);
        }

        st->DontTouch = true;

        if (st->QueueBuffer == nullptr && st->FilePending) {
            st->QueueBuffer = static_cast<char*>(st->FileBuffer)
                              + LockedData.StreamBufferSize * (st->Odd % LockedData.StreamBufferCount);
            --st->FilePending;
            ++st->Odd;
            st->QueueSize = st->FilePending > 0 ? LockedData.StreamBufferSize : st->FilePendingSize;
        }

        st->DontTouch = false;

        if (!AudioDone) {
            LeaveCriticalSection(&GlobalAudioCriticalSection);
        }

        Sound_Timer_Callback();
    }

    if (st->FilePending) {
        return true;
    }

    return false;
}

int __cdecl Stream_Sample_Vol(void* buffer,
                              int size,
                              bool (*callback)(short int, short int*, void**, int*),
                              int volume,
                              int handle)
{
    if (AudioDone || buffer == nullptr || size == 0 || LockedData.DigiHandle == INVALID_AUDIO_HANDLE) {
        return INVALID_AUDIO_HANDLE;
    }

    AUDHeaderType header;
    memcpy(&header, buffer, sizeof(header));
    int oldsize = header.Size;
    header.Size = size - sizeof(header);
    memcpy(buffer, &header, sizeof(header));
    int playid = Play_Sample_Handle(buffer, PRIORITY_MAX, volume, 0, handle);
    header.Size = oldsize;
    memcpy(buffer, &header, sizeof(header));

    if (playid == INVALID_AUDIO_HANDLE) {
        return INVALID_AUDIO_HANDLE;
    }

    SampleTrackerType* st = &LockedData.SampleTracker[playid];
    st->Callback = callback;
    st->Odd = 0;

    return playid;
}

int File_Stream_Sample(const char* filename, bool real_time_start)
{
    return File_Stream_Sample_Vol(filename, VOLUME_MAX, real_time_start);
}

void File_Stream_Preload(int index)
{
    SampleTrackerType* st = &LockedData.SampleTracker[index];
    int maxnum = (LockedData.StreamBufferCount / 2) + 4;
    int num = st->Loading ? std::min<int>(st->FilePending + 2, maxnum) : maxnum;

    int i = 0;

    for (i = st->FilePending; i < num; ++i) {
        int size = Read_File(st->FileHandle,
                             static_cast<char*>(st->FileBuffer) + i * LockedData.StreamBufferSize,
                             LockedData.StreamBufferSize);

        if (size > 0) {
            st->FilePendingSize = size;
            ++st->FilePending;
        }

        if (size < LockedData.StreamBufferSize) {
            break;
        }
    }

    Sound_Timer_Callback();

    if (LockedData.StreamBufferSize > st->FilePendingSize || i == maxnum) {
        int old_vol = LockedData.SoundVolume;

        int stream_size = st->FilePending == 1 ? st->FilePendingSize : LockedData.StreamBufferSize;

        LockedData.SoundVolume = LockedData.ScoreVolume;
        StartingFileStream = true;
        Stream_Sample_Vol(st->FileBuffer, stream_size, File_Callback, st->Volume, index);
        StartingFileStream = false;

        LockedData.SoundVolume = old_vol;

        st->Loading = false;
        --st->FilePending;

        if (st->FilePending == 0) {
            st->Odd = 0;
            st->QueueBuffer = 0;
            st->QueueSize = 0;
            st->FilePendingSize = 0;
            st->Callback = nullptr;
            Close_File(st->FileHandle);
        } else {
            st->Odd = 2;
            --st->FilePending;

            if (st->FilePendingSize != LockedData.StreamBufferSize) {
                Close_File(st->FileHandle);
                st->FileHandle = INVALID_FILE_HANDLE;
            }

            st->QueueBuffer = static_cast<char*>(st->FileBuffer) + LockedData.StreamBufferSize;
            st->QueueSize = st->FilePending == 0 ? st->FilePendingSize : LockedData.StreamBufferSize;
        }
    }
}

int File_Stream_Sample_Vol(const char* filename, int volume, bool real_time_start)
{
    if (LockedData.DigiHandle == INVALID_AUDIO_HANDLE || filename == nullptr || !Find_File(filename)) {
        return INVALID_AUDIO_HANDLE;
    }

    if (FileStreamBuffer == nullptr) {
        FileStreamBuffer = malloc((unsigned long)(LockedData.StreamBufferSize * LockedData.StreamBufferCount));

        for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
            LockedData.SampleTracker[i].FileBuffer = FileStreamBuffer;
        }
    }

    if (FileStreamBuffer == nullptr) {
        return INVALID_AUDIO_HANDLE;
    }

    int fh = Open_File(filename, 1);

    if (fh == INVALID_FILE_HANDLE) {
        return INVALID_AUDIO_HANDLE;
    }

    int handle = Get_Free_Sample_Handle(PRIORITY_MAX);

    if (handle < MAX_SAMPLE_TRACKERS) {
        SampleTrackerType* st = &LockedData.SampleTracker[handle];
        st->IsScore = true;
        st->FilePending = 0;
        st->FilePendingSize = 0;
        st->Loading = real_time_start;
        st->Volume = volume;
        st->FileHandle = fh;
        File_Stream_Preload(handle);
        return handle;
    }

    return INVALID_AUDIO_HANDLE;
}

void Sound_Callback()
{
    if (!AudioDone && LockedData.DigiHandle != INVALID_AUDIO_HANDLE) {
        Sound_Timer_Callback();

        for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
            SampleTrackerType* st = &LockedData.SampleTracker[i];

            // Is a load pending?
            if (st->Loading) {
                File_Stream_Preload(i);
                // We are done with this sample.
                continue;
            }

            // Is this sample inactive?
            if (!st->Active) {
                // If so, we close the handle.
                if (st->FileHandle != INVALID_FILE_HANDLE) {
                    Close_File(st->FileHandle);
                    st->FileHandle = INVALID_FILE_HANDLE;
                }
                // We are done with this sample.
                continue;
            }

            // Has it been faded Is the volume 0?
            if (st->Reducer && !st->Volume) {
                // If so stop it.
                Stop_Sample(i);

                // We are done with this sample.
                continue;
            }

            // Process pending files.
            if (st->QueueBuffer == nullptr
                || st->FileHandle != INVALID_FILE_HANDLE && LockedData.StreamBufferCount - 3 > st->FilePending) {
                if (st->Callback != nullptr) {
                    if (!st->Callback(i, &st->Odd, &st->QueueBuffer, &st->QueueSize)) {
                        // No files are pending so pending file callback not needed anymore.
                        st->Callback = nullptr;
                    }
                }

                // We are done with this sample.
                continue;
            }
        }
    }
}

void* Load_Sample(char const* filename)
{
    if (LockedData.DigiHandle == INVALID_AUDIO_HANDLE || filename == nullptr || !Find_File(filename)) {
        return nullptr;
    }

    void* data = nullptr;
    int handle = Open_File(filename, 1);

    if (handle != INVALID_FILE_HANDLE) {
        int data_size = File_Size(handle) + sizeof(AUDHeaderType);
        data = malloc(data_size);

        if (data != nullptr) {
            Sample_Read(handle, data, data_size);
        }

        Close_File(handle);
        Misc = data_size;
    }

    return data;
}

long Load_Sample_Into_Buffer(char const* filename, void* buffer, long size)
{
    if (buffer == nullptr || size == 0 || LockedData.DigiHandle == INVALID_AUDIO_HANDLE || !filename
        || !Find_File(filename)) {
        return 0;
    }

    int handle = Open_File(filename, 1);

    if (handle == INVALID_FILE_HANDLE) {
        return 0;
    }

    int sample_size = Sample_Read(handle, buffer, size);
    Close_File(handle);
    return sample_size;
}

int Sample_Read(int fh, void* buffer, int size)
{
    if (buffer == nullptr || fh == INVALID_AUDIO_HANDLE || size <= sizeof(AUDHeaderType)) {
        return 0;
    }

    AUDHeaderType header;
    int actual_bytes_read = Read_File(fh, &header, sizeof(AUDHeaderType));
    int to_read = std::min<unsigned>(size - sizeof(AUDHeaderType), header.Size);

    actual_bytes_read += Read_File(fh, static_cast<char*>(buffer) + sizeof(AUDHeaderType), to_read);

    memcpy(buffer, &header, sizeof(AUDHeaderType));

    return actual_bytes_read;
}

void Free_Sample(void const* sample)
{
    if (sample != nullptr) {
        //Free(sample);
        free((void*)sample);
    }
}

void CALLBACK Sound_Timer_Callback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    if (!AudioDone) {
        EnterCriticalSection(&GlobalAudioCriticalSection);
        Maintenance_Callback();
        LeaveCriticalSection(&GlobalAudioCriticalSection);
    }
}

void Sound_Thread(void* a1)
{
    // TODO : Find a alternative solution, this is the original code, and likely causes lockups on modern systems.
    DuplicateHandle(
        GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &SoundThreadHandle, THREAD_ALL_ACCESS, true, 0);
    SetThreadPriority(SoundThreadHandle, 15);
    SoundThreadActive = true;

    while (!AudioDone) {
        EnterCriticalSection(&GlobalAudioCriticalSection);
        Maintenance_Callback();
        LeaveCriticalSection(&GlobalAudioCriticalSection);
        Sleep(TIMER_DELAY);
    }

    SoundThreadActive = false;
}

bool Set_Primary_Buffer_Format()
{
    if (SoundObject != nullptr && PrimaryBufferPtr != nullptr) {
        return PrimaryBufferPtr->SetFormat(&PrimaryBuffFormat) == DS_OK;
    }

    return false;
}

int Print_Sound_Error(char* sound_error, void* window)
{
    return MessageBoxA((HWND)window, sound_error, "DirectSound Audio Error", MB_OK | MB_ICONWARNING);
}

bool Audio_Init(int bits_per_sample, bool stereo, int rate, bool reverse_channels)
{
    Init_Locked_Data();
    FileStreamBuffer = nullptr;
    memset(LockedData.SampleTracker, 0, sizeof(LockedData.SampleTracker));

    // Audio already init'ed.
    if (SoundObject != nullptr) {
        return true;
    }

    HRESULT return_code = DirectSoundCreate(NULL, &SoundObject, NULL);

    if (return_code != DS_OK) {
        //CCDebugString("Audio_Init - Failed to create Direct Sound Object. Error code %d.", return_code);
        return false;
    }

    return_code = SoundObject->SetCooperativeLevel(MainWindow, DSSCL_PRIORITY);

    if (return_code != DS_OK) {
        //CCDebugString("Audio_Init - Unable to set Direct Sound cooperative level. Error code %d.", return_code);
        SoundObject->Release();
        SoundObject = nullptr;

        return false;
    }

    // Set up DSBUFFERDESC structure.
    memset(&BufferDesc, 0, sizeof(BufferDesc));
    BufferDesc.dwSize = sizeof(DSBUFFERDESC);
    BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;

    // Set up wave format structure.
    memset(&DsBuffFormat, 0, sizeof(DsBuffFormat));
    DsBuffFormat.wFormatTag = WAVE_FORMAT_PCM;
    DsBuffFormat.nChannels = stereo + 1;
    DsBuffFormat.nSamplesPerSec = rate;
    DsBuffFormat.wBitsPerSample = bits_per_sample;
    DsBuffFormat.nBlockAlign = DsBuffFormat.nChannels * DsBuffFormat.wBitsPerSample / 8; // todo confirm the math
    DsBuffFormat.nAvgBytesPerSec = DsBuffFormat.nBlockAlign * DsBuffFormat.nSamplesPerSec;
    DsBuffFormat.cbSize = 0;

    memcpy(&PrimaryBufferDesc, &BufferDesc, sizeof(PrimaryBufferDesc));
    memcpy(&PrimaryBuffFormat, &DsBuffFormat, sizeof(PrimaryBuffFormat));

    return_code = SoundObject->CreateSoundBuffer(&PrimaryBufferDesc, &PrimaryBufferPtr, NULL);

    if (return_code != DS_OK) {
        //CCDebugString("Audio_Init - Failed to create the primary sound buffer. Error code %d.");
        SoundObject->Release();
        SoundObject = nullptr;

        return false;
    }

    // Attempt to allocate buffer.
    if (!Set_Primary_Buffer_Format()) {
        //CCDebugString("Audio_Init - Failed to set primary buffer format.");

        int old_bits_per_sample = DsBuffFormat.wBitsPerSample;
        int old_block_align = DsBuffFormat.nBlockAlign;
        int old_bytes_per_sec = DsBuffFormat.nAvgBytesPerSec;

        if (DsBuffFormat.wBitsPerSample == 16) {
            //CCDebugString("Audio_Init - Trying a 8-bit primary buffer format.");
            DsBuffFormat.wBitsPerSample = 8;
            DsBuffFormat.nBlockAlign = DsBuffFormat.nChannels;
            DsBuffFormat.nAvgBytesPerSec = DsBuffFormat.nChannels * DsBuffFormat.nSamplesPerSec;

            memcpy(&PrimaryBufferDesc, &BufferDesc, sizeof(PrimaryBufferDesc));
            memcpy(&PrimaryBuffFormat, &DsBuffFormat, sizeof(PrimaryBuffFormat));
        }

        // Attempt to allocate 8 bit buffer.
        if (!Set_Primary_Buffer_Format()) {
            PrimaryBufferPtr->Release();
            PrimaryBufferPtr = nullptr;

            SoundObject->Release();
            SoundObject = nullptr;

            //CCDebugString("Audio_Init - Failed to set any primary buffer format. Disabling audio.");

            return false;
        }

        // Restore original format settings.
        DsBuffFormat.wBitsPerSample = old_bits_per_sample;
        DsBuffFormat.nBlockAlign = old_block_align;
        DsBuffFormat.nAvgBytesPerSec = old_bytes_per_sec;
    }

    // Attempt to start playback.
    return_code = PrimaryBufferPtr->Play(0, 0, DSBPLAY_LOOPING);
    if (return_code != DS_OK) {
        //CCDebugString("Audio_Init - Failed to start primary sound buffer. Disabling audio. Error code %d.");

        PrimaryBufferPtr->Release();
        PrimaryBufferPtr = nullptr;

        SoundObject->Release();
        SoundObject = nullptr;

        return false;
    }

    LockedData.DigiHandle = 1;

    InitializeCriticalSection(&GlobalAudioCriticalSection);

    SoundTimerHandle = timeSetEvent(TIMER_DELAY, TIMER_RESOLUTION, Sound_Timer_Callback, 0, 1);

    BufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
    BufferDesc.dwBufferBytes = BUFFER_TOTAL_BYTES;

    BufferDesc.lpwfxFormat = &DsBuffFormat;

    LockedData.UncompBuffer = malloc(UNCOMP_BUFFER_SIZE);

    if (LockedData.UncompBuffer == nullptr) {
        //CCDebugString("Audio_Init - Failed to allocate UncompBuffer.");
        return false;
    }

    // Create placback buffers for all trackers.
    for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
        SampleTrackerType* st = &LockedData.SampleTracker[i];

        return_code = SoundObject->CreateSoundBuffer(&BufferDesc, &st->PlayBuffer, NULL);

        if (return_code != DS_OK) {
            //CCDebugString("Audio_Init - Failed to allocate Play Buffer for tracker %d. Error code %d.", i, return_code);
        }

        st->PlaybackRate = rate;
        st->Stereo = stereo;
        st->BitSize = (bits_per_sample == 16 ? 2 : 0);
        st->FileHandle = INVALID_FILE_HANDLE;
        st->QueueBuffer = nullptr;
        InitializeCriticalSection(&st->AudioCriticalSection);
    }

    SoundType = SFX_ALFX;
    SampleType = SAMPLE_SB;
    ReverseChannels = reverse_channels;
    AudioDone = false;

    return true;
}

void Sound_End()
{
    if (SoundObject != nullptr && PrimaryBufferPtr != nullptr) {
        for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
            if (LockedData.SampleTracker[i].PlayBuffer != nullptr) {
                Stop_Sample(i);
                LockedData.SampleTracker[i].PlayBuffer->Stop();
                LockedData.SampleTracker[i].PlayBuffer->Release();
                LockedData.SampleTracker[i].PlayBuffer = nullptr;
                DeleteCriticalSection(&LockedData.SampleTracker[i].AudioCriticalSection);
            }
        }
    }

    if (FileStreamBuffer != nullptr) {
        //Free(FileStreamBuffer);
        free((void*)FileStreamBuffer);
        FileStreamBuffer = nullptr;
    }

    if (PrimaryBufferPtr != nullptr) {
        PrimaryBufferPtr->Stop();
        PrimaryBufferPtr->Release();
        PrimaryBufferPtr = nullptr;
    }

    if (SoundObject != nullptr) {
        SoundObject->Release();
        SoundObject = nullptr;
    }

    if (LockedData.UncompBuffer != nullptr) {
        //Free(LockedData.UncompBuffer);
        free((void*)LockedData.UncompBuffer);
        LockedData.UncompBuffer = nullptr;
    }

    if (SoundTimerHandle != 0) {
        timeKillEvent(SoundTimerHandle);
        SoundTimerHandle = 0;
    }

    DeleteCriticalSection(&GlobalAudioCriticalSection);

    AudioDone = true;
}

void Stop_Sample(int index)
{
    if (LockedData.DigiHandle != INVALID_AUDIO_HANDLE && index < MAX_SAMPLE_TRACKERS && !AudioDone) {
        EnterCriticalSection(&GlobalAudioCriticalSection);
        SampleTrackerType* st = &LockedData.SampleTracker[index];

        if (st->Active || st->Loading) {
            st->Active = false;

            if (!st->IsScore) {
                st->Original = nullptr;
            }

            st->Priority = 0;

            if (!st->Loading) {
                st->PlayBuffer->Stop();
            }

            st->Loading = false;

            if (st->FileHandle != INVALID_FILE_HANDLE) {
                Close_File(st->FileHandle);
                st->FileHandle = INVALID_FILE_HANDLE;
            }

            st->QueueBuffer = nullptr;
        }

        LeaveCriticalSection(&GlobalAudioCriticalSection);
    }
}

bool Sample_Status(int index)
{
    if (index < 0) {
        return false;
    }

    if (AudioDone) {
        return false;
    }

    if (LockedData.DigiHandle == INVALID_AUDIO_HANDLE || index >= MAX_SAMPLE_TRACKERS) {
        return false;
    }

    SampleTrackerType* st = &LockedData.SampleTracker[index];

    if (st->Loading) {
        return true;
    }

    if (!st->Active) {
        return false;
    }

    DumpBuffer = st->PlayBuffer;
    DWORD status = 0;

    if (DumpBuffer->GetStatus(&status) != DS_OK) {
        //CCDebugString("Sample_Status - GetStatus failed");
        return true;
    }

    // original check, possible typo.
    // return (status & DSBSTATUS_PLAYING) || (status & DSBSTATUS_LOOPING);

    // Buffer has to be set as looping to qualify as playing.
    return (status & (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING)) != 0;
}

bool Is_Sample_Playing(void const* sample)
{
    if (AudioDone || sample == nullptr) {
        return false;
    }

    for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
        if (sample == LockedData.SampleTracker[i].Original && Sample_Status(i)) {
            return true;
        }
    }

    return false;
}

void Stop_Sample_Playing(void const* sample)
{
    if (sample != nullptr) {
        for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
            if (LockedData.SampleTracker[i].Original == sample) {
                Stop_Sample(i);
                break;
            }
        }
    }
}

int Get_Free_Sample_Handle(int priority)
{
    int index = 0;

    for (index = MAX_SAMPLE_TRACKERS - 1; index >= 0; --index) {
        if (!LockedData.SampleTracker[index].Active && !LockedData.SampleTracker[index].Loading) {
            if (StartingFileStream || !LockedData.SampleTracker[index].IsScore) {
                break;
            }

            StartingFileStream = true;
        }
    }

    if (index < 0) {
        for (index = 0; index < MAX_SAMPLE_TRACKERS && LockedData.SampleTracker[index].Priority > priority; ++index) {
            ;
        }

        if (index == MAX_SAMPLE_TRACKERS) {
            return INVALID_AUDIO_HANDLE;
        }

        Stop_Sample(index);
    }

    if (index == INVALID_AUDIO_HANDLE) {
        return INVALID_AUDIO_HANDLE;
    }

    if (LockedData.SampleTracker[index].FileHandle != INVALID_FILE_HANDLE) {
        Close_File(LockedData.SampleTracker[index].FileHandle);
        LockedData.SampleTracker[index].FileHandle = INVALID_FILE_HANDLE;
    }

    if (LockedData.SampleTracker[index].Original) {
        if (!LockedData.SampleTracker[index].IsScore) {
            LockedData.SampleTracker[index].Original = 0;
        }
    }

    LockedData.SampleTracker[index].IsScore = 0;
    return index;
}

int Play_Sample(void const* sample, int priority, int volume, signed short panloc)
{
    return Play_Sample_Handle(sample, priority, volume, panloc, Get_Free_Sample_Handle(priority));
}

bool Attempt_Audio_Restore(LPDIRECTSOUNDBUFFER sound_buffer)
{
    HRESULT return_code = 0;
    DWORD play_status = 0;

    if (AudioDone || sound_buffer == nullptr) {
        return false;
    }

    if (Audio_Focus_Loss_Function != nullptr) {
        Audio_Focus_Loss_Function();
    }

    for (int restore_attempts = 0; restore_attempts < 2 && return_code == DSERR_BUFFERLOST; ++restore_attempts) {
        Restore_Sound_Buffers();
        HRESULT return_code = sound_buffer->GetStatus(&play_status);
    }

    return return_code != DSERR_BUFFERLOST;
}

int Attempt_To_Play_Buffer(int id)
{
    HRESULT return_code;
    SampleTrackerType* st = &LockedData.SampleTracker[id];

    // Attempts to play the current sample's buffer.
    do {
        return_code = st->PlayBuffer->Play(0, 0, 1);

        // Playback was started so we set some needed sample tracker values.
        if (return_code == DS_OK) {
            if (!AudioDone) {
                EnterCriticalSection(&GlobalAudioCriticalSection);
            }

            st->Active = true;
            st->DontTouch = false;
            st->Handle = id;

            if (!AudioDone) {
                LeaveCriticalSection(&GlobalAudioCriticalSection);
            }

            return st->Handle;
        }

        // A error we can't handle here occured.
        if (return_code != DSERR_BUFFERLOST) {
            // Flag this sample as not active.
            st->Active = false;
            break;
        }

        // We got a DSERR_BUFFERLOST so attempting to restore and if able to trying again.
        if (!Attempt_Audio_Restore(st->PlayBuffer)) {
            break;
        }
    } while (return_code == DSERR_BUFFERLOST);

    return INVALID_AUDIO_HANDLE;
}

extern bool Any_Locked();
int Play_Sample_Handle(void const* sample, int priority, int volume, signed short panloc, int id)
{
    HRESULT return_code;
    DWORD status;

    //if (SeenBuff.Get_LockCount() != 0 || HidPage.Get_LockCount() != 0) {
    if (Any_Locked()) {
        // BUGFIX: Original returned 0 which im pretty sure could be a valid handle.
        return INVALID_AUDIO_HANDLE;
    }

    if (!AudioDone) {
        if (sample == nullptr || LockedData.DigiHandle == INVALID_AUDIO_HANDLE) {
            return INVALID_AUDIO_HANDLE;
        }

        if (id == INVALID_AUDIO_HANDLE) {
            return INVALID_AUDIO_HANDLE;
        }

        SampleTrackerType* st = &LockedData.SampleTracker[id];

        // Read in the sample's header.
        AUDHeaderType raw_header;
        memcpy(&raw_header, sample, sizeof(raw_header));

        // We don't support anything lower than 20000 hz.
        if (raw_header.Rate < 24000 && raw_header.Rate > 20000) {
            raw_header.Rate = 22050;
        }

        if (!AudioDone) {
            EnterCriticalSection(&GlobalAudioCriticalSection);
        }

        // Set up basic sample tracker info.
        st->Compression = SCompressType(raw_header.Compression);
        st->Original = sample;
        st->DontTouch = true;
        st->Odd = 0;
        st->Reducer = 0;
        st->Restart = 0;
        st->QueueBuffer = 0;
        st->QueueSize = 0;
        st->TrailerLen = 0;
        st->OriginalSize = raw_header.Size + sizeof(AUDHeaderType);
        st->Priority = priority;
        st->Service = 0;
        st->Remainder = raw_header.Size;
        st->Source = Add_Long_To_Pointer(sample, sizeof(AUDHeaderType));

        if (!AudioDone) {
            LeaveCriticalSection(&GlobalAudioCriticalSection);
        }

        // Compression is ADPCM so we need to init it's stream info.
        if (st->Compression == SCOMP_SOS) {
            st->sosinfo.wChannels = (raw_header.Flags & 1) + 1;
            st->sosinfo.wBitSize = raw_header.Flags & 2 ? 16 : 8;
            st->sosinfo.dwCompSize = raw_header.Size;
            st->sosinfo.dwUnCompSize = raw_header.Size * (st->sosinfo.wBitSize / 4);
            sosCODECInitStream(&st->sosinfo);
        }

        // If the loaded sample doesn't match the sample tracker we need to adjust the tracker.
        if (raw_header.Rate != st->PlaybackRate || (raw_header.Flags & 2) != (st->BitSize & 2)
            || (raw_header.Flags & 1) != (st->Stereo & 1)) {
            //CCDebugString("Play_Sample_Handle - Changing sample format.");
            st->Active = false;
            st->Service = 0;
            st->MoreSource = false;
            DumpBuffer = st->PlayBuffer;

            // Querry the playback status.
            do {
                return_code = st->PlayBuffer->GetStatus(&status);
                if (return_code == DSERR_BUFFERLOST && !Attempt_Audio_Restore(st->PlayBuffer)) {
                    //CCDebugString("Play_Sample_Handle - Unable to get PlaybBuffer status!");
                    return INVALID_AUDIO_HANDLE;
                }
            } while (return_code == DSERR_BUFFERLOST);

            // Stop the sample if its already playing.
            // TODO: Investigate this, logics here are possibly wrong.
            // - What happens when we call Restore when we have stopped the the buffer?
            // - Stop return isn't checked, in TS it checks for DSERR_BUFFERLOST, but thats not a valid Stop return.
            if (status & (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING)
                && (st->PlayBuffer->Stop(), return_code == DSERR_BUFFERLOST)
                && !Attempt_Audio_Restore(st->PlayBuffer)) {
                //CCDebugString("Play_Sample_Handle - Unable to stop buffer!");
                return INVALID_AUDIO_HANDLE;
            }

            st->PlayBuffer->Release();
            st->PlayBuffer = nullptr;
            DsBuffFormat.nSamplesPerSec = raw_header.Rate;
            DsBuffFormat.nChannels = (raw_header.Flags & 1) + 1;
            DsBuffFormat.wBitsPerSample = raw_header.Flags & 2 ? 16 : 8;
            DsBuffFormat.nBlockAlign = DsBuffFormat.nChannels * DsBuffFormat.wBitsPerSample / 8;
            DsBuffFormat.nAvgBytesPerSec = DsBuffFormat.nBlockAlign * DsBuffFormat.nSamplesPerSec;

            // Attempt to create a new buffer for this new sample.
            return_code = SoundObject->CreateSoundBuffer(&BufferDesc, &st->PlayBuffer, 0);

            if (return_code == DSERR_BUFFERLOST && !Attempt_Audio_Restore(st->PlayBuffer)) {
                return INVALID_AUDIO_HANDLE;
            }

            // We failed to create the buffer, bail!
            if (return_code != DS_OK && return_code != DSERR_BUFFERLOST) {
                st->PlaybackRate = 0;
                st->Stereo = false;
                st->BitSize = 0;
                //CCDebugString("Play_Sample_Handle - Bad sample format!");
                return INVALID_AUDIO_HANDLE;
            }

            // Set the new sample info.
            st->PlaybackRate = raw_header.Rate;
            st->Stereo = raw_header.Flags & 1;
            st->BitSize = raw_header.Flags & 2;
        }

        // If the sample tracker matches the loaded file we load the samples.
        do {
            DumpBuffer = st->PlayBuffer;
            return_code = DumpBuffer->GetStatus(&status);

            if (return_code == DSERR_BUFFERLOST && !Attempt_Audio_Restore(st->PlayBuffer)) {
                //CCDebugString("Play_Sample_Handle - Can't get DumpBuffer status.");
                return INVALID_AUDIO_HANDLE;
            }
        } while (return_code == DSERR_BUFFERLOST);

        // If the sample is already playing stop it.
        if (status & (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING)) {
            st->Active = false;
            st->Service = 0;
            st->MoreSource = false;
            st->PlayBuffer->Stop();
        }

        LPVOID play_buffer_ptr;
        DWORD lock_length1;
        LPVOID dummy_buffer_ptr;
        DWORD lock_length2;

        do {
            return_code = st->PlayBuffer->Lock(
                0, BUFFER_TOTAL_BYTES, &play_buffer_ptr, &lock_length1, &dummy_buffer_ptr, &lock_length2, 0);

            if (return_code == DSERR_BUFFERLOST && !Attempt_Audio_Restore(st->PlayBuffer)) {
                //CCDebugString("Play_Sample_Handle - Unable to lock buffer! Buffer lost.");
                return INVALID_AUDIO_HANDLE;
            }
        } while (return_code == DSERR_BUFFERLOST);

        if (return_code != DS_OK) {
            //CCDebugString("Play_Sample_Handle - Unable to lock buffer! Unknown error.");
            return INVALID_AUDIO_HANDLE;
        }

        st->DestPtr = (void*)Sample_Copy(st,
                                         &st->Source,
                                         (int*)&st->Remainder,
                                         &st->QueueBuffer,
                                         (int*)&st->QueueSize,
                                         play_buffer_ptr,
                                         BUFFER_CHUNK_SIZE,
                                         st->Compression,
                                         st->Trailer,
                                         &st->TrailerLen);

        if (st->DestPtr == (char*)BUFFER_CHUNK_SIZE) {
            st->MoreSource = true;
            st->Service = 1;
            st->OneShot = false;
        } else {
            st->MoreSource = false;
            st->OneShot = true;
            st->Service = 1;
            memset(static_cast<char*>(play_buffer_ptr) + (unsigned)st->DestPtr, 0, BUFFER_CHUNK_SIZE);
        }

        st->PlayBuffer->Unlock(play_buffer_ptr, lock_length1, dummy_buffer_ptr, lock_length2);
        st->Volume = volume * 128;

        do {
            st->PlayBuffer->SetVolume(Convert_HMI_To_Direct_Sound_Volume((volume * LockedData.SoundVolume) / 256));

            if (return_code == DSERR_BUFFERLOST && !Attempt_Audio_Restore(st->PlayBuffer)) {
                //CCDebugString("Play_Sample_Handle - Can't set volume!");
                return INVALID_AUDIO_HANDLE;
            }
        } while (return_code == DSERR_BUFFERLOST);

        if (!Start_Primary_Sound_Buffer(false)) {
            //CCDebugString("Play_Sample_Handle - Can't start primary buffer!");
            return INVALID_AUDIO_HANDLE;
        }

        // Reset buffer playback position.
        do {
            return_code = st->PlayBuffer->SetCurrentPosition(0);

            if (return_code == DSERR_BUFFERLOST && !Attempt_Audio_Restore(st->PlayBuffer)) {
                //CCDebugString("Play_Sample_Handle - Can't set current position!");
                return INVALID_AUDIO_HANDLE;
            }
        } while (return_code == DSERR_BUFFERLOST);

        return Attempt_To_Play_Buffer(id);
    }

    return INVALID_AUDIO_HANDLE;
}

void Restore_Sound_Buffers()
{
    if (PrimaryBufferPtr != nullptr) {
        PrimaryBufferPtr->Restore();
    }

    for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
        if (LockedData.SampleTracker[i].PlayBuffer != nullptr) {
            LockedData.SampleTracker[i].PlayBuffer->Restore();
        }
    }
}

int Set_Sound_Vol(int vol)
{
    int oldvol = LockedData.SoundVolume;
    LockedData.SoundVolume = vol;
    return oldvol;
}

int Set_Score_Vol(int volume)
{
    int old = LockedData.ScoreVolume;
    LockedData.ScoreVolume = volume;

    for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
        SampleTrackerType* st = &LockedData.SampleTracker[i];

        if (st->IsScore & st->Active) {
            st->PlayBuffer->SetVolume(
                Convert_HMI_To_Direct_Sound_Volume((LockedData.ScoreVolume * (st->Volume / 128)) / 256));
        }
    }

    return old;
}

void Fade_Sample(int index, int ticks)
{
    if (Sample_Status(index)) {
        SampleTrackerType* st = &LockedData.SampleTracker[index];

        if (ticks > 0 && !st->Loading) {
            st->Reducer = ((st->Volume / ticks) + 1);
        } else {
            Stop_Sample(index);
        }
    }
}

void Unfade_Sample(int index, int ticks)
{
    if (Sample_Status(index)) {
        SampleTrackerType* st = &LockedData.SampleTracker[index];

        if (ticks > 0 && !st->Loading) {
            st->Reducer -= ((st->Volume / ticks) + 1);
        } else {
            st->Reducer = 0;
        }
    }
}

int Get_Digi_Handle()
{
    return LockedData.DigiHandle;
}

unsigned Sample_Length(void* sample)
{
    if (sample == nullptr) {
        return 0;
    }

    AUDHeaderType header;
    memcpy(&header, sample, sizeof(header));
    unsigned time = header.UncompSize;

    if (header.Flags & 2) {
        time /= 2;
    }

    if (header.Flags & 1) {
        time /= 2;
    }

    if (header.Rate / 60 > 0) {
        time /= header.Rate / 60;
    }

    return time;
}

bool Start_Primary_Sound_Buffer(bool forced)
{
    if (PrimaryBufferPtr == nullptr || !GameInFocus) {
        return false;
    }

    if (forced) {
        PrimaryBufferPtr->Play(0, 0, DSBPLAY_LOOPING);
        return true;
    }

    DWORD status = 0;

    if (PrimaryBufferPtr->GetStatus(&status) != DS_OK) {
        return false;
    }

    // Primary buffer has to be set as looping to qualify as playing.
    if (status & DSBSTATUS_PLAYING | DSBSTATUS_LOOPING) {
        return true;
    }

    PrimaryBufferPtr->Play(0, 0, DSBPLAY_LOOPING);
    return true;
}

void Stop_Primary_Sound_Buffer()
{
    for (int i = 0; i < MAX_SAMPLE_TRACKERS; ++i) {
        Stop_Sample(i);
    }

    if (PrimaryBufferPtr != nullptr) {
        PrimaryBufferPtr->Stop();
    }
}

void Suspend_Audio_Thread()
{
    if (SoundThreadActive) {
        timeKillEvent(SoundTimerHandle);
        SoundTimerHandle = 0;
        SoundThreadActive = false;
    }
}

void Resume_Audio_Thread()
{
    if (!SoundThreadActive) {
        SoundTimerHandle = timeSetEvent(TIMER_DELAY, TIMER_RESOLUTION, Sound_Timer_Callback, SoundThreadActive, 1);
        SoundThreadActive = true;
    }
}

int Simple_Copy(void** source, int* ssize, void** alternate, int* altsize, void** dest, int size)
{
    int out = 0;

    if (*ssize == 0) {
        *source = *alternate;
        *ssize = *altsize;
        *alternate = nullptr;
        *altsize = 0;
    }

    if (*source == nullptr || *ssize == 0) {
        return out;
    }

    int s = size;

    if (*ssize < size) {
        s = *ssize;
    }

    memcpy(*dest, *source, s);
    *source = static_cast<char*>(*source) + s;
    *ssize -= s;
    *dest = static_cast<char*>(*dest) + s;
    out = s;

    if ((size - s) == 0) {
        return out;
    }

    *source = *alternate;
    *ssize = *altsize;
    *alternate = nullptr;
    *altsize = 0;

    out = Simple_Copy(source, ssize, alternate, altsize, dest, (size - s)) + s;

    return out;
}

int Sample_Copy(SampleTrackerType* st,
                void** source,
                int* ssize,
                void** alternate,
                int* altsize,
                void* dest,
                int size,
                SCompressType scomp,
                void* trailer,
                int16_t* trailersize)
{
    int datasize = 0;

    // There is no compression or it doesn't match any of the supported compressions so we just copy the data over.
    if (scomp == SCOMP_NONE || (scomp != SCOMP_WESTWOOD && scomp != SCOMP_SOS)) {
        return Simple_Copy(source, ssize, alternate, altsize, &dest, size);
    }

    _SOS_COMPRESS_INFO* s = &st->sosinfo;

    while (size > 0) {
        uint16_t fsize;
        uint16_t dsize;
        unsigned magic;

        void* fptr = &fsize;
        void* dptr = &dsize;
        void* mptr = &magic;

        // Verify and seek over the chunk header.
        if (Simple_Copy(source, ssize, alternate, altsize, &fptr, sizeof(fsize)) < sizeof(fsize)) {
            break;
        }

        if (Simple_Copy(source, ssize, alternate, altsize, &dptr, sizeof(dsize)) < sizeof(dsize) || dsize > size) {
            break;
        }

        if (Simple_Copy(source, ssize, alternate, altsize, &mptr, sizeof(magic)) < sizeof(magic)
            || magic != LockedData.MagicNumber) {
            break;
        }

        if (fsize == dsize) {
            // File size matches size to decompress, so there's nothing to do other than copy the buffer over.
            if (Simple_Copy(source, ssize, alternate, altsize, &dest, fsize) < dsize) {
                return datasize;
            }
        } else {
            // Else we need to decompress it.
            void* uptr = LockedData.UncompBuffer;

            if (Simple_Copy(source, ssize, alternate, altsize, &uptr, fsize) < fsize) {
                return datasize;
            }

            if (scomp == SCOMP_WESTWOOD) {
                Audio_Unzap(LockedData.UncompBuffer, dest, dsize);
            } else {
                s->lpSource = (char*)LockedData.UncompBuffer;
                s->lpDest = (char*)dest;

                sosCODECDecompressData(s, dsize);
            }

            dest = reinterpret_cast<char*>(dest) + dsize;
        }

        datasize += dsize;
        size -= dsize;
    }

    return datasize;
}
