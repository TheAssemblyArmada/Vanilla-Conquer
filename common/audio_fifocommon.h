#ifndef AUDIO_FIFOCOMMON
#define AUDIO_FIFOCOMMON

// FIFO interface with the ARM7 chip.

// Define the music chunk size. We can't load the entire music in RAM so we
// have to stream it from the SD Card.
#define MUSIC_CHUNK_SIZE 32768

// USR1: ARM9 to ARM7
namespace USR1
{

    // Define some commands which we implement from the ARM7 side. A command
    // is a full 32-bit integer in which we break down in two pieces: the 16
    // least significative bits are used for arguments (like passing the
    // SoundHandle in STOP_SAMPLE_HANDLE) and the most significative 16 bits
    // are used for commands indeed.
    typedef enum
    {
        // Stop all playing sounds.
        SOUND_KILL = 0 << 20,

        // Alert the ARM7 that a new chunk of music is available.
        MUSIC_CHUNK_UPDATED = 1 << 20,

        // Stop a sample by its Handle.
        STOP_SAMPLE_HANDLE = 2 << 20,

        // Stop a sample by its pointer (use least signficiative 16-bits as hash)
        STOP_SAMPLE = 3 << 20,

        // Set volume of music.
        SET_MUSIC_VOL = 4 << 20,
    } FifoSoundCommand;

    // Define message kinds. Used to distinguish packages one from another.
    typedef enum
    {
        // Ordinary sound message.
        SOUND_PLAY_MESSAGE = 0x1234,

        // Sound message comming from VQA Player.
        SOUND_VQA_MESSAGE,
    } FifoSoundMessageType;

    // Define what can be in a message. Message must have a maximum length of
    // 32 bytes IIRC.
    typedef struct FifoMessage
    {
        u16 type;

        union
        {
            struct
            {
                const void* data;
                u16 handle;
                u16 freq;
                u8 volume;
                u8 pan;
                u8 priority;
                u8 is_music : 1;
            } SoundPlay;

            struct
            {
                const void* data;
                u32 size;
                u16 freq;
                u8 volume;
                u8 bits;
            } SoundVQAChunk;
        };

    } ALIGN(4) FifoSoundMessage;
} // namespace USR1

// USR2: Used to communicate from ARM7 to ARM9.
namespace USR2
{
    //! Enum values for the fifo sound commands.
    typedef enum
    {
        // Ask the ARM9 for more music data.
        MUSIC_REQUEST_CHUNK = 1 << 20,
    } SoundMusicChunk;
} // namespace USR2

#endif //AUDIO_FIFOCOMMON
