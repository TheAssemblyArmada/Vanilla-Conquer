#include "audio.h"
#include "memflag.h"
#include <nds.h>
#include <cstdio>
#include <limits.h>

#define CALLED printf("%s called\n", __func__)

void pause(const char*, ...);

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

/*
** Define the different type of sound compression avaliable to the westwood
** library.
*/
typedef enum
{
    SCOMP_NONE = 0,     // No compression -- raw data.
    SCOMP_WESTWOOD = 1, // Special sliding window delta compression.
    SCOMP_SOS = 99      // SOS frame compression.
} SCompressType;

static unsigned int SoundTicks = 0;

// Tracker class. Represent a track in game.
class SoundTracker
{
public:
    SoundTracker()
    {
        memset(this, 0, sizeof(*this));
        SoundHandle = -1;
    }

    static inline bool Can_Be_Hardware_Decompressed(SCompressType c, unsigned char bits)
    {
        switch (c) {
        case SCOMP_NONE:
        case SCOMP_SOS:
            return true;
            break;

        case SCOMP_WESTWOOD:
            return false;
            break;
        }

        // Should be unreachable, but in any case...
        return false;
    }

    static SoundFormat DS_Sound_Format(SCompressType type, unsigned char bits)
    {
        if (type == SCOMP_SOS) {
            return SoundFormat_ADPCM;
        } else if (bits == 16) {
            return SoundFormat_16Bit;
        }
        return SoundFormat_8Bit;
    }

    inline void* Get_Sample()
    {
        return Sample;
    }

    inline bool Is_Sample_Playing()
    {
        // The Nintendo DS has no means of verifying if the sample is playing
        // on the ARM9 chip.  If we want to do that, we have to move this code
        // to the ARM7 chip, which means future work.  So we always return yes.

        if (SoundTicks - Ticks > 3 * 1000 * 1000)
            return false;

        return true;
    }

    inline void Stop_Sample()
    {
        if (SoundHandle >= 0)
            soundKill(SoundHandle);
    }

    inline unsigned char Get_Priority()
    {
        return Priority;
    }

    inline int Play(bool hwuncompress)
    {
        // Play sound in system.
        void* sample = Sample;
        SoundFormat format = DS_Sound_Format((hwuncompress) ? Compression : SCOMP_NONE, Bits);
        unsigned short freq = Frequency;
        int size = SampleSize;
        unsigned char volume = Volume / 2;
        unsigned char panloc = ((int)panloc + 32767) / 517;
        SoundTicks += timerTick(0);
        Ticks = SoundTicks;

        SoundHandle = soundPlaySample(sample, format, size, freq, volume, panloc, false, 0);

        return SoundHandle;
    }

    int Play_Sample(const void* sample, int priority, int volume, signed short panloc)
    {
        // Set attributes given by call.
        Priority = (unsigned char)priority;
        Volume = volume;
        Panloc = panloc;

        // Load the AUD header;
        AUDHeaderType raw_header;
        memcpy(&raw_header, sample, sizeof(raw_header));

        // We don't support anything lower than 20000 hz.
        if (raw_header.Rate < 24000 && raw_header.Rate > 20000) {
            raw_header.Rate = 22050;
        }

        // Get number of bits in sample.
        Bits = (raw_header.Flags & 2) ? 16 : 8;

        // Update Frequency according to header/
        Frequency = raw_header.Rate;

        // Check the Compression.
        Compression = SCompressType(raw_header.Compression);
        if (Can_Be_Hardware_Decompressed(Compression, Bits)) {
            // Yay! just throw this sample to the hardware.

            // Size of sample is the same size reported by the header.
            SampleSize = raw_header.Size;
            Sample = Add_Long_To_Pointer(sample, sizeof(AUDHeaderType));

            return Play(true);
        }
        return 0;
    }

private:
    unsigned char Priority;    // The priority of current sound.
    unsigned char Bits;        // 8 or 16 bits.
    signed short Panloc;       // Directional Audio.
    int Volume;                // Volume of current sound.
    SCompressType Compression; // Compression that this sound data is using.
    int Frequency;             // Frequency of the sample.
    int SoundHandle;           // The Nintendo DS sound handle.
    void* Sample;              // Playable sample. May be compressed or not.
    int SampleSize;            // Size of the Sample.
    unsigned int Ticks;        // Tick in which the current sample was introduced.
};

class SoundTrackers
{
public:
    SoundTrackers()
    {
        DigiHandle = INVALID_FILE_HANDLE;
        AudioInitialized = false;
    }

    void Initialize_Trackers()
    {
        AudioInitialized = true;
        DigiHandle = 1;
    }

    bool Is_Initialized()
    {
        return AudioInitialized;
    }

    inline int Get_Digi_Handle()
    {
        return DigiHandle;
    }

    inline SoundTracker* Get_Sample_Tracker(int i)
    {
        return &Trackers[i];
    }

    inline bool Is_Sample_Playing(const void* sample)
    {
        SoundTracker* st;

        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            st = Get_Sample_Tracker(i);

            if (st->Get_Sample() == sample && st->Is_Sample_Playing())
                return true;
        }

        return false;
    }

    inline int Get_Free_Sound_Handle(int priority)
    {
        int i;
        // Not found. Get with least priority.
        unsigned int min_priority = 255;
        int min_handle = 0;

        // Look in all trackers for a free slot.
        for (i = MAX_SAMPLE_TRACKERS - 1; i >= 0; --i) {
            SoundTracker* st = Get_Sample_Tracker(i);
            unsigned char current_priority = st->Get_Priority();

            if (!st->Is_Sample_Playing()) {
                return i;
            }

            if (priority > current_priority) {
                return i;
            }
            /*
            if (current_priority < min_priority) {
                min_handle = i;
                min_priority = current_priority;
            }
*/
        }

        // Now that the lowest priority tracker have been found, return it.
        return min_handle;
    }

    int Play_Sample(void const* sample, int priority, int volume, signed short panloc)
    {
        // Since Nintendo DS has no means to check if a sound is playing, we do a
        // simple round-robin on the trackers.
        int free_tracker = Get_Free_Sound_Handle(priority);
        SoundTracker* st = Get_Sample_Tracker(free_tracker);

        // Stop sound if currently playing
        st->Stop_Sample();
        return st->Play_Sample(sample, priority, volume, panloc);
    }

    void Print_Priorities()
    {
        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            SoundTracker* st = Get_Sample_Tracker(i);

            printf("%d ", (int)st->Get_Priority());
        }
        printf("\n");
    }

private:
    SoundTracker Trackers[MAX_SAMPLE_TRACKERS];
    bool AudioInitialized;
    int DigiHandle;

public:
    // Timer hack to determine if sample spent too much time in queue.
    static unsigned int Ticks;
};

static SoundTrackers Trackers;

// Everything down there is present to conform to the game's API.

void (*Audio_Focus_Loss_Function)(void) = nullptr;
bool StreamLowImpact = false;

SFX_Type SoundType;
Sample_Type SampleType;

int File_Stream_Sample(char const* filename, bool real_time_start)
{
    CALLED;
    return 1;
};
int File_Stream_Sample_Vol(char const* filename, int volume, bool real_time_start)
{
    CALLED;
    return 1;
};
void Sound_Callback(void){};
void maintenance_callback(void){};
void* Load_Sample(char const* filename)
{
    CALLED;
    return nullptr;
};
long Load_Sample_Into_Buffer(char const* filename, void* buffer, long size)
{
    CALLED;
    return 0;
}
long Sample_Read(int fh, void* buffer, long size)
{
    CALLED;
    return 0;
};
void Free_Sample(void const* sample){};
bool Audio_Init(int bits_per_sample, bool stereo, int rate, bool reverse_channels)
{
    CALLED;

    // Initialize Nintendo DS sound system.
    soundEnable();

    // Set Global structures required by game's API.
    SoundType = SFX_ALFX;
    SampleType = SAMPLE_SB;
    Trackers.Initialize_Trackers();

    return true;
};
void Sound_End(void){};
void Stop_Sample(int handle){};
bool Sample_Status(int handle)
{
    CALLED;
    return 0;
};
bool Is_Sample_Playing(void const* sample)
{
    return false;
    //return Trackers.Is_Sample_Playing(sample);
};
void Stop_Sample_Playing(void const* sample){};
int Play_Sample(void const* sample, int priority, int volume, signed short panloc)
{
    if (Trackers.Is_Initialized())
        return Trackers.Play_Sample(sample, priority, volume, panloc);
    return -1;
}

int Play_Sample_Handle(void const* sample, int priority, int volume, signed short panloc, int id)
{
    CALLED;
    if (Trackers.Is_Initialized()) {
        SoundTracker* st = Trackers.Get_Sample_Tracker(id);
        return st->Play_Sample(sample, priority, volume, panloc);
    }
    return -1;
};
int Set_Sound_Vol(int volume)
{
    CALLED;
    return 127;
};
int Set_Score_Vol(int volume)
{
    CALLED;
    return 127;
};
void Fade_Sample(int handle, int ticks){};
int Get_Free_Sample_Handle(int priority)
{
    CALLED;
    return 1;
};
int Get_Digi_Handle(void)
{
    CALLED;
    return Trackers.Get_Digi_Handle();
}
long Sample_Length(void const* sample)
{
    CALLED;
    return 0;
};
void Restore_Sound_Buffers(void){};
bool Set_Primary_Buffer_Format(void)
{
    CALLED;
    return 0;
};
bool Start_Primary_Sound_Buffer(bool forced)
{
    CALLED;
    return 0;
};
void Stop_Primary_Sound_Buffer(void){};
