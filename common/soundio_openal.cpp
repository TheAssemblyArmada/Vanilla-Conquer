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
#include "soundio_imp.h"
#include <al.h>
#include <alc.h>
#include <stdlib.h>

enum
{
    OPENAL_BUFFER_COUNT = 2,
};

struct SampleTrackerTypeImp
{
    // A point in space that is the source of this sound.
    ALuint OpenALSource;

    // The format of the audio stream.
    ALenum Format;

    // The Frequency of the audio stream
    int Frequency;

    // A set of buffers
    ALuint AudioBuffers[OPENAL_BUFFER_COUNT];

    int UnusedBufferCount;
};

static ALCcontext* OpenALContext = nullptr;

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

void SoundImp_Buffer_Sample_Data(SampleTrackerTypeImp* st, const void* data, size_t datalen)
{
    ALuint buffer;

    if (1 && st->UnusedBufferCount) {
        buffer = st->AudioBuffers[OPENAL_BUFFER_COUNT - st->UnusedBufferCount];
        st->UnusedBufferCount--;
    } else {
        alSourceUnqueueBuffers(st->OpenALSource, 1, &buffer);
    }

    alBufferData(buffer, st->Format, data, datalen, st->Frequency);
    alSourceQueueBuffers(st->OpenALSource, 1, &buffer);
}

int SoundImp_Get_Sample_Free_Buffer_Count(SampleTrackerTypeImp* st)
{
    ALint processed_buffers;
    ALint state;

    alGetSourcei(st->OpenALSource, AL_SOURCE_STATE, &state);
    if (state == AL_PLAYING) {
        alGetSourcei(st->OpenALSource, AL_BUFFERS_PROCESSED, &processed_buffers);
    } else {
        processed_buffers = 0;
    }

    return st->UnusedBufferCount + processed_buffers;
}

bool SoundImp_Init(int bits_per_sample, bool stereo, int rate, bool reverse_channels)
{
    ALCdevice* device = alcOpenDevice(nullptr);

    if (device == nullptr) {

        //CCDebugString("Error occured getting OpenAL device.\n");

        return false;
    }

    OpenALContext = alcCreateContext(device, nullptr);
    if (OpenALContext == nullptr || !alcMakeContextCurrent(OpenALContext)) {
        //CCDebugString("OpenAL failed to make audio context current.\n");
        alcCloseDevice(device);
        OpenALContext = nullptr;
        return false;
    }

    return true;
}

void SoundImp_PauseSound()
{
    if (OpenALContext != nullptr) {
        alcSuspendContext(OpenALContext);
    }
}

bool SoundImp_ResumeSound()
{
    if (OpenALContext == nullptr) {
        return false;
    }

    alcProcessContext(OpenALContext);

    return true;
}

SampleTrackerTypeImp* SoundImp_Init_Sample(int bits_per_sample, bool stereo, int rate)
{
    SampleTrackerTypeImp* st;
    ALCenum error;

    st = (SampleTrackerTypeImp*)malloc(sizeof(*st));
    if (st) {
        if ((error = alGetError()) != AL_NO_ERROR) {
            //CCDebugString(Get_OpenAL_Error(error));
        } else {
            alGenSources(1, &st->OpenALSource);
            alGenBuffers(OPENAL_BUFFER_COUNT, st->AudioBuffers);

            if ((error = alGetError()) != AL_NO_ERROR) {
                //CCDebugString(Get_OpenAL_Error(error));
            } else {
                st->Frequency = rate;
                st->Format = Get_OpenAL_Format(bits_per_sample, stereo ? 2 : 1);
                st->UnusedBufferCount = OPENAL_BUFFER_COUNT;

                return st;
            }
        }

        free(st);
    }

    return NULL;
}

void SoundImp_Set_Sample_Attributes(SampleTrackerTypeImp* st, int bits_per_sample, bool stereo, int rate)
{
    ALenum new_format;

    new_format = Get_OpenAL_Format(bits_per_sample, stereo ? 2 : 1);

    if (rate != st->Frequency || new_format != st->Format) {
        st->Frequency = rate;
        st->Format = new_format;
    }
}

void SoundImp_Set_Sample_Volume(SampleTrackerTypeImp* st, unsigned int volume)
{
    alSourcef(st->OpenALSource, AL_GAIN, volume / 65536.0);
}

void SoundImp_Shutdown()
{
    ALCdevice* device = alcGetContextsDevice(OpenALContext);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(OpenALContext);
    alcCloseDevice(device);
}

void SoundImp_Shutdown_Sample(SampleTrackerTypeImp* st)
{
    alDeleteBuffers(OPENAL_BUFFER_COUNT, st->AudioBuffers);
    alDeleteSources(1, &st->OpenALSource);
    free(st);
}

void SoundImp_Start_Sample(SampleTrackerTypeImp* st)
{
    alSourcePlay(st->OpenALSource);
}

void SoundImp_Stop_Sample(SampleTrackerTypeImp* st)
{
    ALint processed_count = -1;
    alSourceStop(st->OpenALSource);
    alGetSourcei(st->OpenALSource, AL_BUFFERS_PROCESSED, &processed_count);

    while (processed_count-- > 0) {
        ALuint tmp;
        alSourceUnqueueBuffers(st->OpenALSource, 1, &tmp);
    }

    st->UnusedBufferCount = OPENAL_BUFFER_COUNT;
}

bool SoundImp_Sample_Status(SampleTrackerTypeImp* st)
{
    ALint val;
    alGetSourcei(st->OpenALSource, AL_SOURCE_STATE, &val);

    return val == AL_PLAYING;
}
