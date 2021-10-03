#include "audio.h"
#include "memflag.h"
#include "file.h"
#include <cstdio>
#include <limits.h>
#include <cstdlib>
#include <nds/arm9/sound.h>
#include <nds/fifocommon.h>
#include <nds/arm9/cache.h>
#include "audio_fifocommon.h"

/** Sound interface between C&C and the ARM7 chip.
  *
  * Author: mrparrot (aka giulianob)
  *
  * This engine is asynchronous and it was designed to not lag behind the main
  * console CPU. Every sound or music is sent either using a message or a
  * command, which is then processed from the ARM7 side. No decompression is
  * done here, so its overhead is minimal.
  **/

/* Converts C&C volume unit to value which the DS supports. DS volume ranges
   from 0 (min) to 127 (max), where the game ranges from 0 to 255.  This
   conversion is not 100% precise but is good enough.  */
#define VOL_CNC2DS(x) ((x) / 2)
#define VOL_DS2CNC(x) ((x)*2)

/* Some enumerations defining global constants.*/
enum
{
    VOLUME_MIN = 0,
    VOLUME_MAX = 255,
    DS_VOLUME_MAX = VOL_CNC2DS(VOLUME_MAX), // Max volume supported by DS.
    INVALID_AUDIO_HANDLE = -1,
    INVALID_FILE_HANDLE = -1,
};

/* Define a music handle as being a 16-bit unsigned number ranging from
   0 to (2**16-1). We do this way because we communicate the handle using
   a command rather than a message, and we can only use a 16-bit integer
   as an argument to command.  This is good enough, as it will take a while
   to wrap and hopefully the previous sound that got the same handle was
   already stopped.  */
static inline u16 Get_Next_Handle()
{
    static u16 handle = 0;
    return ++handle;
}

// Everything down there is present to conform to the game's API.
bool StreamLowImpact = false; // Unused, required by game engine.

SFX_Type SoundType;                           // Required by game engine.
Sample_Type SampleType;                       // Required by game engine
static int DigiHandle = INVALID_AUDIO_HANDLE; // Required by game engine.
static bool AudioInitialized = false;         // Flags that the audio was already initialized.

/* Defines the current volume that the user set in the game settings.  This has
   nothing to do with the DS hardware volume which you setup by pressing some
   buttons on its case.  */
static int SampleVolume = DS_VOLUME_MAX;

/* This class represents a music buffer.  It is used to stream music data to the
   ARM7 chip, as it doesn't have enough memory to load the entire music there.  

   This works as following: The game calls Set_File_Stream, which will open
   the music file, read a chunk of data and send it to the ARM7 CPU through
   a message.  When the ARM7 realizes that it has already processed a good
   amount of this chunk, it then sends a USR2::REQUEST_MUSIC_DATA to the
   ARM9 chip, which ends up calling Mark_For_Update, and then we update it
   on Update_File_Stream.

   We keep track of its volume and if it is playing on this chip because we
   need to constantly communicate with the game that it is playing.
*/
class MusicBuffer
{
public:
    // Basic constructor.
    MusicBuffer()
    {
        memset(this, 0, sizeof(*this));
        Handle = -1;
        Volume = DS_VOLUME_MAX;
    }

    // Set muisic volume.
    inline int Set_Volume(int volume)
    {
        int oldvol = Volume;
        unsigned command = USR1::SET_MUSIC_VOL | (volume & 0xFFFF);
        Volume = volume;
        return oldvol;
    }

    // Checks if given handle is a music handle.
    inline bool Is_Music_Handle(u16 handle)
    {
        if (handle == Handle)
            return true;

        return false;
    }

    // Check if the music is playing.
    inline bool Sample_Status()
    {
        return IsPlaying;
    }

    // Set music streaming from filename.
    inline int Set_File_Stream(const char* filename, unsigned char volume)
    {
        int bytes;

        // If volume is too low then disable music to save resources
        if (Volume <= 0) {
            return INVALID_AUDIO_HANDLE;
        }

        FileHandle = Open_File(filename, 1);
        if (FileHandle == INVALID_FILE_HANDLE) {
            return INVALID_AUDIO_HANDLE;
        }

        if (Buffer == NULL) {
            Buffer = (char*)malloc(2 * MUSIC_CHUNK_SIZE);
        }

        if (Buffer == NULL) {
            printf("Failure allocating music buffer\n");
            while (1)
                ;
        }

        memset(Buffer, 0, 2 * MUSIC_CHUNK_SIZE);

        bytes = Read_File(FileHandle, Buffer, 2 * MUSIC_CHUNK_SIZE);
        if (bytes == 0) {
            IsPlaying = false;
            return INVALID_AUDIO_HANDLE;
        }

        ToUpdate = 0;
        IsPlaying = true;
        HasMoreSource = true;
        Handle = Get_Next_Handle();
        Send_Chunk();
        return Handle;
    }

    // Forcely stops the file stream.
    inline int Stop_File_Stream()
    {
        ShouldBeUpdated = 0;
        IsPlaying = false;
        HasMoreSource = false;
        if (FileHandle >= 0)
            Close_File(FileHandle);
        FileHandle = -1;
        return 0;
    }

    // Mark file stream to be updated on the next maintenance call.
    inline void Mark_For_Update()
    {
        ShouldBeUpdated++;
    }

    // Update the music chunk with more data.
    inline void Update_File_Stream()
    {
        if (ShouldBeUpdated == 0)
            return;

        ShouldBeUpdated--;

        if (!Buffer || !IsPlaying)
            return;

        // We use a double-buffer so that we always update the part which is not
        // yet playing.
        unsigned char* to_update = (unsigned char*)Buffer + ToUpdate * MUSIC_CHUNK_SIZE;
        if (HasMoreSource) {
            int bytes = Read_File(FileHandle, to_update, MUSIC_CHUNK_SIZE);
            if (bytes == 0) {
                HasMoreSource = false;
                Close_File(FileHandle);
                FileHandle = -1;
            }
        } else {
            IsPlaying = false;
        }

        ToUpdate = (ToUpdate + 1) % 2;
    }

    // Send the music chunk to the ARM7.
    inline void Send_Chunk()
    {
        if (!Buffer)
            return;

        USR1::FifoMessage msg;

        msg.type = USR1::SOUND_PLAY_MESSAGE;
        msg.SoundPlay.priority = 255;
        msg.SoundPlay.handle = Handle;
        msg.SoundPlay.data = Buffer;
        msg.SoundPlay.volume = Volume;
        msg.SoundPlay.pan = 64;
        msg.SoundPlay.is_music = true;

        // Asynchronous send sound play command
        fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
    }

private:
    char* Buffer;         // Music chunk data loaded.
    int FileHandle;       // Music file descriptor.
    bool IsPlaying;       // Is it playing?
    int ToUpdate;         // Which side of the buffer should be updated?
    int ShouldBeUpdated;  // Should this buffer be updated?
    u16 Handle;           // Sound Handle.
    unsigned char Volume; // Volume
    bool HasMoreSource;   // Is there more stuff in the file?
};

// Declare the Music Buffer instance.
static MusicBuffer MBuffer;

// Declare the command handler which will decompress messages from ARM9 related
// to this sound engine.
void user02CommandHandler(u32 command, void* userdata)
{
    int cmd = (command)&0x00F00000;
    int data = command & 0xFFFF;
    int channel = (command >> 16) & 0xF;

    switch (cmd) {

    case USR2::MUSIC_REQUEST_CHUNK:
        MBuffer.Mark_For_Update();
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------
 *      Everything down here is an interface to the game's engine.
 * --------------------------------------------------------------------
 */

int File_Stream_Sample(char const* filename, bool real_time_start)
{
    return File_Stream_Sample_Vol(filename, 255, real_time_start);
};

// Open a audio file for streaming.  Assume this is music.
int File_Stream_Sample_Vol(char const* filename, int volume, bool real_time_start)
{
    return MBuffer.Set_File_Stream(filename, VOL_CNC2DS(volume));
};

// Sound maintenance callback.  This is called at most every frame to update
// audio-related stuff.  In our case we only have to be concerned about the
// music stream.
void Sound_Callback(void)
{
    MBuffer.Update_File_Stream();
}

// Unused but required by game engine.
void maintenance_callback(void){};

// Unsused but required by game engine.
void* Load_Sample(char const* filename)
{
    return nullptr;
};

// Unused but required by game engine.
void Free_Sample(void const* sample){};

// Initialize audio-related structures.
bool Audio_Init(int bits_per_sample, bool stereo, int rate, bool reverse_channels)
{
    // Initialize Nintendo DS sound system.
    soundEnable();

    // Install ARM7 to ARM9 Queue, used to request music data.
    fifoSetValue32Handler(FIFO_USER_02, user02CommandHandler, 0);

    // Set Global structures required by game's API.
    SoundType = SFX_ALFX;
    SampleType = SAMPLE_SB;
    DigiHandle = 1;
    AudioInitialized = true;

    return true;
};

// Unused, but requited by game engine.
void Sound_End(void)
{
}

// Stop a sample by its handle.
void Stop_Sample(int handle)
{
    if (MBuffer.Is_Music_Handle(handle)) {
        // In case it is music, we need to stop our file stream.
        MBuffer.Stop_File_Stream();
    }

    // Send command to the ARM7 to stop the stuff that is going there.
    unsigned command = USR1::STOP_SAMPLE_HANDLE | ((u32)handle & 0xFFFF);
    fifoSendValue32(FIFO_USER_01, command);
}

bool Sample_Status(int handle)
{
    // It seems we only need to implement this for music?
    if (MBuffer.Is_Music_Handle(handle))
        return MBuffer.Sample_Status();

    return false;
};

bool Is_Sample_Playing(void const* sample)
{
    /* Don't implement that.  It is called constantly and may flood the ARM7
       with messages more than we already is.  */
    return false;
};

/* Stop a sample that is playing.  We just pass that to the ARM7...  */
void Stop_Sample_Playing(void const* sample)
{
    unsigned command = USR1::STOP_SAMPLE | ((u32)sample & 0xFFFF);
    fifoSendValue32(FIFO_USER_01, command);
};

/* Play a sample.  We just pass that to the ARM7.  */
int Play_Sample(void const* sample, int priority, int volume, signed short panloc)
{
    u16 handle = Get_Next_Handle();
    USR1::FifoMessage msg;

    // We set the samples volume by actually lowering the volume argument
    // passed to the ARM7.
    volume = (SampleVolume * VOL_CNC2DS(volume)) / DS_VOLUME_MAX;

    msg.type = USR1::SOUND_PLAY_MESSAGE;
    msg.SoundPlay.priority = priority;
    msg.SoundPlay.handle = handle;
    msg.SoundPlay.data = sample;
    msg.SoundPlay.volume = volume;

    // Convert the panloc to a value the DS understand.
    msg.SoundPlay.pan = ((int)panloc + 32767) / 517;
    msg.SoundPlay.is_music = false;

    // Asynchronous send sound play command
    fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);

    return handle;
}

// Set global sound volume.
int Set_Sound_Vol(int volume)
{
    int oldval = VOL_DS2CNC(SampleVolume);
    SampleVolume = VOL_CNC2DS(volume);
    return oldval;
};

// Set music volume.
int Set_Score_Vol(int volume)
{
    return VOL_DS2CNC(MBuffer.Set_Volume(VOL_CNC2DS(volume)));
};

// This actually has to fade the sample slowly until it stops playing. But we
// don't care too much about that so we just stop the sample right away.
void Fade_Sample(int handle, int ticks)
{
    Stop_Sample(handle);
}

// Return the DigiHandle, necessary to the game engine.
int Get_Digi_Handle(void)
{
    return DigiHandle;
}

// Unused, but required by the game engine.
bool Set_Primary_Buffer_Format(void)
{
    return 0;
}

// Unused, but required by the game engine.
bool Start_Primary_Sound_Buffer(bool forced)
{
    return 0;
}
