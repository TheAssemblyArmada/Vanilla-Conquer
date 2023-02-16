#include "audio.h"

void (*Audio_Focus_Loss_Function)(void) = nullptr;
bool StreamLowImpact = false;

SFX_Type SoundType;
Sample_Type SampleType;

int File_Stream_Sample_Vol(char const* filename, int volume, bool real_time_start)
{
    return 1;
}
void Sound_Callback(void)
{
}
void* Load_Sample(char const* filename)
{
    return nullptr;
}
void Free_Sample(void const* sample)
{
}
bool Audio_Init(int bits_per_sample, bool stereo, int rate, bool reverse_channels)
{
    return 0;
}
void Sound_End(void)
{
}
void Stop_Sample(int handle)
{
}
bool Sample_Status(int handle)
{
    return 0;
}
bool Is_Sample_Playing(void const* sample)
{
    return 0;
}
void Stop_Sample_Playing(void const* sample)
{
}
int Play_Sample(void const* sample, int priority, int volume, signed short panloc)
{
    return 1;
}
int Set_Score_Vol(int volume)
{
    return 0;
}
void Fade_Sample(int handle, int ticks)
{
}
int Get_Digi_Handle(void)
{
    return 1;
}
void Restore_Sound_Buffers(void)
{
}
bool Set_Primary_Buffer_Format(void)
{
    return 0;
}
bool Start_Primary_Sound_Buffer(bool forced)
{
    return 0;
}
void Stop_Primary_Sound_Buffer(void)
{
}
