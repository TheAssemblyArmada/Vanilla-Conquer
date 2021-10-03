/** Sound engine for Vanilla-Conquer to the Nintendo DSi ARM7 chip.
  *
  * Author: mrparrot (aka giulianob)
  *
  * This engine is asynchronous and it was designed to not lag behind the main
  * console CPU. Every sound or music is sent either using a message or a
  * command, which is then processed from the ARM7 side. No decompression is
  * done here, so its overhead is minimal.
  **/

#include <nds/debug.h>
#include <nds/arm7/audio.h>
#include <nds/system.h>
#include <nds/ipc.h>
#include <nds/fifocommon.h>
#include <nds/system.h>
#include <nds/timers.h>

#include "../common/memflag.h"
#include "../common/audio.h"
#include "../common/audio_fifocommon.h"

#include "printf.h"

#define ARM7

// Not good practice, but who cares.
#include "../common/soscodec.cpp"
#include "../common/auduncmp.cpp"

#define IS_CHANNEL_FREE(i) (!(SCHANNEL_CR(i) & SCHANNEL_ENABLE))
#define VQA_CHANNEL        7

// Calculate the ceil of division x / y.
#define CEIL_DIV(x, y) (1 + (((x) - 1) / (y)))

// Provide an implementation of timerElapsed. Stolen from libnds.

//---------------------------------------------------------------------------------
static u16 timerElapsedARM7(void)
{
    //---------------------------------------------------------------------------------
    static u16 elapsed = 0;
    u16 time = TIMER_DATA(1);

    s32 result = (s32)time - (s32)elapsed;

    //overflow...this will only be accurate if it has overflowed no more than once.
    if (result < 0) {
        result = time + (0x10000 - elapsed);
    }

    elapsed = time;

    return (u16)result;
}

// Nintendo DS supported sounds format.
typedef enum
{
    SoundFormat_16Bit = 1, /*!<  16-bit PCM */
    SoundFormat_8Bit = 0,  /*!<  8-bit PCM */
    SoundFormat_PSG = 3,   /*!<  PSG (programmable sound generator?) */
    SoundFormat_ADPCM = 2  /*!<  IMA ADPCM compressed audio  */
} SoundFormat;

// Global values used in the engine.
enum
{
    AUD_CHUNK_MAGIC_ID = 0x0000DEAF,
    VOLUME_MIN = 0,
    VOLUME_MAX = 255,
    PRIORITY_MIN = 0,
    PRIORITY_MAX = 255,
    MAX_SAMPLE_TRACKERS = 5,
    DECOMP_BUFFER_COUNT = 2,
    BUFFER_CHUNK_SIZE = 4096,
    UNCOMP_BUFFER_SIZE = 2098,
    BUFFER_TOTAL_BYTES = BUFFER_CHUNK_SIZE * DECOMP_BUFFER_COUNT,
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

class SoundTracker;
static inline int __attribute__((pure)) Get_Channel_Index(SoundTracker*);

// Functions used in decompressing westwood audio.
int Simple_Copy(void**, int*, void**, int*, void*);
int Sample_Copy(SoundTracker*, void**, int*, void**, int*, void*, int, SCompressType, void*, int16_t*);

/* Speed in which the timer used to track where the playing cursor is.
   Unfortunately the DS doesn't give an interface to query in which 

*/
#define TIMER_SPEED (BUS_CLOCK / 256)

/* Tracker class. Represent a track in game.  This works as follows: 

   When the game wants to play a sound, it queries a a free tracker for it to
   play (see Get_Free_Sound_Tracker). If it can't find a free tracker, then it
   stops the tracker which has the lowest priority.

   Then it set the sound to be played. However, the sound is compressed thus we
   need to decompress it as we play, and we only have a very limited memory for
   that. So we use a BUFFER_TOTAL_BYTES buffer divided into 2 BUFFER_CHUNK_SIZE,
   in order to double buffer, and we keep track how much of the original
   compressed audio we could decompress. We then send the small BUFFER_TOTAL_BYTES
   into the hardware and set it to loop around it, so we have to keep the buffer
   updated in order for it to not get stuck as a scratched recording.

   Later on, the CPU picks up the tracker for Update, and it decompresses the
   part of the buffer which is not currently playing.  If it detects that
   there is no more to decompress, it set the tracker as a ONE_SHOT to avoid it
   to wrap around again when finished.

   To detect when to update the buffer, we use a timer and the Active boolean.
   The timer is set to be preicise and we use it to keep track the position
   in where the hardware is when playing the small BUFFER_TOTAL_BYTES buffer,
   as there is no way to directly query that. The Active flag is used to signalize
   that the tracker is active.
*/
class SoundTracker
{
public:
    // Initialize Tracker.
    SoundTracker()
    {
        memset(this, 0, sizeof(*this));
    }

    // Get the tracker's number. Used to access the sound hardware.
    inline int Get_Channel_Index();

    static SoundFormat DS_Sound_Format(SCompressType type, unsigned char bits)
    {
        if (type == SCOMP_SOS) {
            return SoundFormat_ADPCM;
        } else if (bits == 16) {
            return SoundFormat_16Bit;
        }
        return SoundFormat_8Bit;
    }

    // How many ticks has elapsed since StartTime?
    unsigned long long Get_Elapsed_Ticks()
    {
        SoundTracker::Now += timerElapsedARM7();
        return SoundTracker::Now - StartedTime;
    }

    // Set the tracker StartTime.
    void Set_Tracker_Timer(unsigned bias = 0)
    {
        SoundTracker::Now += timerElapsedARM7();
        StartedTime = Now - bias;
    }

    inline void* Get_Sample()
    {
        return Sample;
    }

    inline bool Is_Sample_Playing()
    {
        return Active;
    }

    // Stop this tracker from playing a sample.
    void Stop_Sample()
    {
        int channel = Get_Channel_Index();
        SCHANNEL_CR(channel) = 0;
        Active = false;
        MoreSource = false;
        OneShot = false;
        QueueBuffer = NULL;
        QueueSize = false;
        Remainder = 0;
        Decomp_Buffer_Index = 0;
        Size = 0;
        Sample = NULL;
        OriginalSample = NULL;
        SampleSize = 0;
        IsMusic = false;
        MusicStreamIndex = 0;
        StartedTime = 0;
    }

    inline unsigned char Get_Priority()
    {
        return Priority;
    }

    inline unsigned Get_Handle()
    {
        return SoundHandle;
    }

    // Play watever is in this track.
    int Play();

    // Play a sample.
    int Play_Sample(const void* sample,
                    unsigned char priority,
                    unsigned char volume,
                    unsigned char panloc,
                    u16 handle,
                    bool is_music)
    {
        // Set attributes given by call.
        Priority = priority;
        Volume = volume;
        Panloc = panloc;
        SoundHandle = handle;
        IsMusic = is_music;

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
        OriginalSample = (void*)sample;
        Sample = Add_Long_To_Pointer(sample, sizeof(AUDHeaderType));

        if (IsMusic) {
            QueueBuffer = Add_Long_To_Pointer(sample, MUSIC_CHUNK_SIZE);
            QueueSize = MUSIC_CHUNK_SIZE;
            Remainder = MUSIC_CHUNK_SIZE - sizeof(AUDHeaderType);
        } else {
            Remainder = raw_header.Size;
        }

        // Compression is ADPCM so we need to init it's stream info.
        if (Compression == SCOMP_SOS) {
            sosinfo.wChannels = (raw_header.Flags & 1) + 1;
            sosinfo.wBitSize = raw_header.Flags & 2 ? 16 : 8;
            sosinfo.dwCompSize = raw_header.Size;
            sosinfo.dwUnCompSize = raw_header.Size * (sosinfo.wBitSize / 4);
            sosCODECInitStream(&sosinfo);
        } else if (Compression == SCOMP_WESTWOOD) {
            /* There is a bug in SCOMP_WESTWOOD on DS in which sounds compressed
               by it get wavely loud in some audios.  So we lower their volume
               so that it doesn't bother the user too much.  */
            Volume = Volume / 6;
            Bits = 8;
        }

        // Decompress two chunks of data from the compressed audio.
        // This function Sample_Copy code is completely awful and even I don't
        // understand it fully. Please do not ask me how it works.
        int bytes_read = Sample_Copy(this,
                                     &Sample,
                                     &Remainder,
                                     &QueueBuffer,
                                     &QueueSize,
                                     Decomp_Buff[0],
                                     BUFFER_TOTAL_BYTES,
                                     Compression,
                                     nullptr,
                                     nullptr);

        Size = bytes_read; // Keep track of size
        if (bytes_read == BUFFER_TOTAL_BYTES) {
            // We need to flag that we have more data to decompress.
            MoreSource = true;
            OneShot = false;
        } else {
            // We already decompressed everything.
            MoreSource = false;
            OneShot = true;
            Remainder = 0;
            QueueBuffer = 0;
        }

        Decomp_Buffer_Index = 0;

        // Set the tacker to be played.
        return Play();
    }

    // Hash the sample pointer to 16-bit pointers.
    inline u16 Get_Sample_16()
    {
        return (u16)((u32)OriginalSample & 0xFFFF);
    }

    // Request music data to the ARM9 chip.
    void Request_Music_Data()
    {
        fifoSendValue32(FIFO_USER_02, USR2::MUSIC_REQUEST_CHUNK);
        QueueBuffer = (char*)OriginalSample + MusicStreamIndex * MUSIC_CHUNK_SIZE;
        MusicStreamIndex = (MusicStreamIndex + 1) % 2;
        QueueSize = MUSIC_CHUNK_SIZE;

        // Do not wait for confirmation.
        // while(!fifoCheckValue32(FIFO_USER_01));
    }

    // Update the tracker. This is the core routine of this sound engine.
    inline void Update()
    {
        // If tracker is not playing, return and go to next.
        if (!Active)
            return;

        // Compute the time (in ticks) of this chunk.
        unsigned length_in_ticks = CEIL_DIV(Size * TIMER_SPEED, (Bits >> 3) * Frequency);

        // If it is not time to update the timer then goto next.
        if (Get_Elapsed_Ticks() < length_in_ticks)
            return;

        // It is time to update the tracker. Set the timer to where should be
        // the position when the chunk started playing.
        Set_Tracker_Timer(Get_Elapsed_Ticks() - length_in_ticks);

        // If it is OneShot, set it to false to signal it was processed.
        if (OneShot) {
            OneShot = false;
        }

        // We are doing a double buffering.  The previous buffer was already
        // decompressed on Play_Sample, or a previous iteration of Update.
        // But the earlier buffer need update to hold the next uncompressed
        // data

        char to_update = Decomp_Buffer_Index;
        Decomp_Buffer_Index = (Decomp_Buffer_Index + 1) % DECOMP_BUFFER_COUNT;

        if (MoreSource) {
            // Update stopped buffer with new data
            int bytes_read = Sample_Copy(this,
                                         &Sample,
                                         &Remainder,
                                         &QueueBuffer,
                                         &QueueSize,
                                         Decomp_Buff[to_update],
                                         BUFFER_CHUNK_SIZE,
                                         Compression,
                                         nullptr,
                                         nullptr);

            // Fill remaining buffer with 0 in case it case not enough data.
            if (bytes_read < BUFFER_CHUNK_SIZE) {
                memset(Decomp_Buff[to_update] + bytes_read, 0, BUFFER_CHUNK_SIZE - bytes_read);
            }

            if (IsMusic && !QueueBuffer) {
                Request_Music_Data();
            }

            if (bytes_read == 0) {
                // The entire sample have been decompressed, no more precessing is
                // necessary.
                MoreSource = false;

                int current_channel = Get_Channel_Index();
                SCHANNEL_CR(current_channel) = (SCHANNEL_CR(current_channel) & (~SOUND_REPEAT)) | SOUND_ONE_SHOT;
            }
        } else {
            if (IsMusic) {
                // Request more data so that the ARM9 chip realize that the
                // stream ended, so it can queue the next music.
                Request_Music_Data();
            }

            // If we are here, it means that the audio has ended.  Deinitialize the
            // tracker.
            Stop_Sample();
        }
    }

    bool Is_Music()
    {
        return Active && IsMusic;
    }

    void Set_Volume(unsigned char volume)
    {
        SCHANNEL_VOL(Get_Channel_Index()) = (volume & 0x7F);
    }

    // Used to Debug.
    void Print_IsMusic()
    {
        if (IsMusic)
            nocashPrintf("1 ");
        else
            nocashPrintf("0 ");
    }

private:
    // Total size is BUFFER_TOTAL_BYTES
    unsigned char Decomp_Buff[DECOMP_BUFFER_COUNT][BUFFER_CHUNK_SIZE];

    unsigned char Priority;    // The priority of current sound.
    unsigned char Bits;        // 8 or 16 bits.
    unsigned char Panloc;      // Directional Audio.
    unsigned char Volume;      // Volume of current sound.
    SCompressType Compression; // Compression that this sound data is using.
    int Frequency;             // Frequency of the sample.
    u16 SoundHandle;           // The Nintendo DS sound handle.
    void* Sample;              // Playable sample. May be compressed or not.
    void* OriginalSample;
    int SampleSize; // Size of the Sample.

    int Remainder;     // Number of bytes remaining in the source data
                       // as pointed by the "Source" element.
    void* QueueBuffer; // Pointer to continued sample data.
    int QueueSize;     // Size of queue buffer attached.
    bool MoreSource;   // Indicate that we have more stuff to decompress.
    bool OneShot;

    short Size;
    char Decomp_Buffer_Index;

    bool Active;
    bool IsMusic;

    char MusicStreamIndex;

    unsigned long long StartedTime; // Tick number which this track started to play.
    static unsigned long long Now;  // Track the current time in ticks.
public:
    _SOS_COMPRESS_INFO sosinfo;
};

// Keep track of the number of ticks representing the current time.
unsigned long long SoundTracker::Now;

// This is an interface to a bunch of `Tracker`.
class SoundTrackers
{
public:
    inline SoundTracker* Get_Sample_Tracker(int i)
    {
        return &Trackers[i];
    }

    SoundTracker* Get_Tracker_By_Handle(int handle)
    {
        SoundTracker* st;
        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            st = Get_Sample_Tracker(i);

            if (st->Is_Sample_Playing() && st->Get_Handle() == handle)
                return st;
        }

        return NULL;
    }

    bool Is_Sample_Playing(const void* sample)
    {
        SoundTracker* st;

        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            st = Get_Sample_Tracker(i);

            if (st->Get_Sample() == sample && st->Is_Sample_Playing())
                return true;
        }

        return false;
    }

    void Stop_Sample_Handle(u16 handle)
    {
        SoundTracker* st;

        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            st = Get_Sample_Tracker(i);

            if (st->Get_Handle() == handle && st->Is_Sample_Playing())
                st->Stop_Sample();
        }
    }

    void Stop_Sample(u16 sample)
    {
        SoundTracker* st;

        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            st = Get_Sample_Tracker(i);

            if (st->Get_Sample_16() == sample && st->Is_Sample_Playing())
                st->Stop_Sample();
        }
    }

    // Returns a free tracker index or the tracker with had the smallest
    // priority.
    int Get_Free_Sound_Tracker(int priority)
    {
        int i;
        unsigned int min_priority = 255;
        unsigned int min_priority_pos = -1;

        // Look in all trackers for a free slot.
        for (i = MAX_SAMPLE_TRACKERS - 1; i >= 0; --i) {
            SoundTracker* st = Get_Sample_Tracker(i);
            unsigned char current_priority = st->Get_Priority();

            if (!st->Is_Sample_Playing()) {
                return i;
            }

            if (current_priority < min_priority) {
                min_priority = current_priority;
                min_priority_pos = i;
            }
        }

        if (priority > min_priority) {
            return min_priority_pos;
        }

        return 0;
    }

    // Play sample in one of the trackers.
    int Play_Sample(void const* sample, int priority, int volume, signed short panloc, u16 handle, bool is_music)
    {
        int free_tracker = Get_Free_Sound_Tracker(priority);
        SoundTracker* st = Get_Sample_Tracker(free_tracker);

        // Stop sound if currently playing
        st->Stop_Sample();
        return st->Play_Sample(sample, priority, volume, panloc, handle, is_music);
    }

    // Used to debug.
    void Print_Priorities()
    {

        nocashPrintf("Prio: ");
        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            SoundTracker* st = Get_Sample_Tracker(i);

            nocashPrintf("%d ", (int)st->Get_Priority());
        }
        nocashPrintf("\n");
    }

    // Used to debug.
    void Print_Active()
    {

        nocashPrintf("Actv: ");
        for (int i = 0; i < MAX_SAMPLE_TRACKERS; i++) {
            SoundTracker* st = Get_Sample_Tracker(i);
            int active = (st->Is_Sample_Playing()) ? 1 : 0;
            nocashPrintf("%d ", active);
        }
        nocashPrintf("\n");
    }

    // Update all trackers.
    void Update_Trackers()
    {
        for (int i = MAX_SAMPLE_TRACKERS - 1; i >= 0; i--) {
            SoundTracker* st = Get_Sample_Tracker(i);
            st->Update();
        }
    }

    void Stop_Trackers()
    {
        for (int i = MAX_SAMPLE_TRACKERS - 1; i >= 0; i--) {
            SoundTracker* st = Get_Sample_Tracker(i);
            st->Stop_Sample();
        }
    }

    // Set the music volume.
    void Set_Music_Vol(int volume)
    {
        for (int i = MAX_SAMPLE_TRACKERS - 1; i >= 0; i--) {
            SoundTracker* st = Get_Sample_Tracker(i);

            if (st->Is_Music()) {
                st->Set_Volume(volume);
                return;
            }
        }
    }

private:
    SoundTracker Trackers[MAX_SAMPLE_TRACKERS];
};

static SoundTrackers Trackers;

// On a Tracker, get which index corresponds to the hardware.
int SoundTracker::Get_Channel_Index()
{
    return ((unsigned long)this - (unsigned long)Trackers.Get_Sample_Tracker(0)) / sizeof(SoundTracker);
}

int SoundTracker::Play()
{
    // Play sound in system.
    void* sample;
    SoundFormat format;
    int size;

    unsigned short freq = Frequency;
    unsigned char volume = Volume;
    unsigned char panloc = Panloc;
    int channel = Get_Channel_Index();

    format = DS_Sound_Format(SCOMP_NONE, Bits);
    sample = Decomp_Buff[0]; //Decomp_Buff[Decomp_Buffer_Index];
    size = Size;
    freq = Frequency;
    if (size == 0) {
        if (IsMusic && Active) {
            // Request more data so that the ARM9 chip realize that the
            // stream ended, so it can queue the next music.
            Request_Music_Data();
        }
    } else {
        Active = true;
        SCHANNEL_SOURCE(channel) = (u32)sample;
        SCHANNEL_REPEAT_POINT(channel) = 0;
        SCHANNEL_LENGTH(channel) = size >> 2;
        SCHANNEL_TIMER(channel) = SOUND_FREQ(freq);
        SCHANNEL_CR(channel) = SCHANNEL_ENABLE | SOUND_VOL(volume) | SOUND_PAN(panloc) | (format << 29);
        if (OneShot) {
            SCHANNEL_CR(channel) |= SOUND_ONE_SHOT;
        } else {
            SCHANNEL_CR(channel) |= SOUND_REPEAT;
            // Set size to BUFFER_CHUNK_SIZE so that the buffer gets correctly
            // updated when the track position has passed half of the audio buffer.
            Size = BUFFER_CHUNK_SIZE;
        }
        Set_Tracker_Timer();
    }

    return SoundHandle;
}

// Software audio decompression.  The code is crap as hell...
// ----------------------------------------------------------------------------
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

int Sample_Copy(SoundTracker* st,
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
    unsigned char uncomp_buffer[UNCOMP_BUFFER_SIZE];
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
            || magic != AUD_CHUNK_MAGIC_ID) {
            break;
        }

        if (fsize == dsize) {
            // File size matches size to decompress, so there's nothing to do other than copy the buffer over.
            if (Simple_Copy(source, ssize, alternate, altsize, &dest, fsize) < dsize) {
                return datasize;
            }
        } else {
            // Else we need to decompress it.
            void* uptr = uncomp_buffer; //LockedData.UncompBuffer;
            memset(uncomp_buffer, 0, sizeof(uncomp_buffer));

            if (Simple_Copy(source, ssize, alternate, altsize, &uptr, fsize) < fsize) {
                return datasize;
            }

            if (scomp == SCOMP_WESTWOOD) {
                Audio_Unzap(uncomp_buffer, dest, dsize);
            } else {
                s->lpSource = (char*)uncomp_buffer;
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

void Sound_Update()
{
    Trackers.Update_Trackers();

    // Used to debug priorities on emulator.
    //Trackers.Print_Priorities();
    //Trackers.Print_Active();
}

// End software decompression code.
//-----------------------------------------------------------------------------

/* Queue used to serialize messages. 
   
   When the ARM7 receives a message, is generates an interruption to process
   it.  That is not a good idea, as it may create race conditions, so we use
   this queue to serialize it.  */
// MESSAGES_MAX must be a power of two.
template <int MESSAGES_MAX> class MessageQueue
{
public:
    MessageQueue()
    {
        memset(this, 0, sizeof(*this));
    }

    int Pop_Message(USR1::FifoMessage* msg)
    {
        if (Is_Empty())
            return 0;

        memcpy(msg, &Messages[Tail], sizeof(*msg));
        Tail = (Tail + 1) % MESSAGES_MAX;
        return 1;
    }

    int Push_Message(USR1::FifoMessage* msg)
    {
        if (Is_Full()) {
            return 0;
        }

        memcpy(&Messages[Head], msg, sizeof(*msg));
        Head = (Head + 1) % MESSAGES_MAX;
        return 1;
    }

private:
    inline bool Is_Full(void)
    {
        return (Tail + 1) % MESSAGES_MAX == Head;
    }

    inline bool Is_Empty(void)
    {
        return Head == Tail;
    }

    USR1::FifoMessage Messages[MESSAGES_MAX];
    unsigned Head, Tail;
};

static MessageQueue<32> MQueue;

//---------------------------------------------------------------------------------
void user01CommandHandler(u32 command, void* userdata)
{
    //---------------------------------------------------------------------------------

    int cmd = (command)&0x00F00000;
    int data = command & 0xFFFF;
    int channel = (command >> 16) & 0xF;

    switch (cmd) {

    case USR1::SOUND_KILL:
        Trackers.Stop_Trackers();
        SCHANNEL_CR(VQA_CHANNEL) &= ~SCHANNEL_ENABLE;
        break;

    case USR1::MUSIC_CHUNK_UPDATED:
        // Ignore messages that the MUSIC_CHUNK was updated.
        break;

    case USR1::STOP_SAMPLE_HANDLE:
        Trackers.Stop_Sample_Handle(data);

    case USR1::STOP_SAMPLE:
        Trackers.Stop_Sample(data);

    case USR1::SET_MUSIC_VOL:
        Trackers.Set_Music_Vol(data);

    default:
        break;
    }
}

// Process one element of the Sound Queue.
void Process_Queue()
{
    USR1::FifoMessage msg;
    if (MQueue.Pop_Message(&msg) == 0)
        return;

    if (msg.type == USR1::SOUND_PLAY_MESSAGE) {
        const void* sample = msg.SoundPlay.data;
        u16 handle = msg.SoundPlay.handle;
        u8 priority = msg.SoundPlay.priority;
        u8 volume = msg.SoundPlay.volume;
        u8 panloc = msg.SoundPlay.pan;
        bool is_music = msg.SoundPlay.is_music;

        Trackers.Play_Sample(sample, priority, volume, panloc, handle, is_music);
    } else if (msg.type == USR1::SOUND_VQA_MESSAGE) {
        const void* sample = msg.SoundVQAChunk.data;
        u16 freq = msg.SoundVQAChunk.freq;
        u32 size = msg.SoundVQAChunk.size;
        u8 volume = msg.SoundVQAChunk.volume;
        u8 bits = msg.SoundVQAChunk.bits;
        unsigned format = SoundTracker::DS_Sound_Format(SCOMP_NONE, bits);

        SCHANNEL_SOURCE(VQA_CHANNEL) = (u32)sample;
        SCHANNEL_REPEAT_POINT(VQA_CHANNEL) = 0;
        SCHANNEL_LENGTH(VQA_CHANNEL) = size >> 2;
        SCHANNEL_TIMER(VQA_CHANNEL) = SOUND_FREQ(freq);
        SCHANNEL_CR(VQA_CHANNEL) =
            SCHANNEL_ENABLE | SOUND_VOL(volume) | SOUND_PAN(64) | (format << 29) | (SOUND_REPEAT);

        SCHANNEL_REPEAT_POINT(VQA_CHANNEL) = 0;
    }
}

void user01DataHandler(int bytes, void* user_data)
{
    USR1::FifoMessage msg;
    fifoGetDatamsg(FIFO_USER_01, bytes, (u8*)&msg);
    MQueue.Push_Message(&msg);

    // Don't send confirmation -- This engine is asynchronous.
    //fifoSendValue32(FIFO_USER_01, (u32)channel);
}

//---------------------------------------------------------------------------------
void installUser01FIFO(void)
{
    //---------------------------------------------------------------------------------

    fifoSetDatamsgHandler(FIFO_USER_01, user01DataHandler, 0);
    fifoSetValue32Handler(FIFO_USER_01, user01CommandHandler, 0);

    // Setup timer which will tell when to update the circular queue.
    // Use the macro version, as the C function version fails to link.
    TIMER_DATA(1) = 0;
    TIMER_CR(1) = ClockDivider_256 | TIMER_ENABLE;
}
