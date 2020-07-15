#include "audio.h"

void (*Audio_Focus_Loss_Function)(void) = nullptr;
bool StreamLowImpact = false;

SFX_Type SoundType;
Sample_Type SampleType;

int File_Stream_Sample(char const* filename, BOOL real_time_start)
{
    return 1;
};
int File_Stream_Sample_Vol(char const* filename, int volume, BOOL real_time_start)
{
    return 1;
};
void __cdecl Sound_Callback(void){};
void __cdecl far maintenance_callback(void){};
void* Load_Sample(char const* filename)
{
    return NULL;
};
long Load_Sample_Into_Buffer(char const* filename, void* buffer, long size)
{
    return 0;
}
long Sample_Read(int fh, void* buffer, long size)
{
    return 0;
};
void Free_Sample(void const* sample){};
BOOL Audio_Init(HWND window, int bits_per_sample, BOOL stereo, int rate, int reverse_channels)
{
    return 0;
};
void Sound_End(void){};
void Stop_Sample(int handle){};
BOOL Sample_Status(int handle)
{
    return 0;
};
BOOL Is_Sample_Playing(void const* sample)
{
    return 0;
};
void Stop_Sample_Playing(void const* sample){};
int Play_Sample(void const* sample, int priority, int volume, signed short panloc)
{
    return 1;
};
int Play_Sample_Handle(void const* sample, int priority, int volume, signed short panloc, int id)
{
    return 1;
};
int Set_Sound_Vol(int volume)
{
    return 0;
};
int Set_Score_Vol(int volume)
{
    return 0;
};
void Fade_Sample(int handle, int ticks){};
int Get_Free_Sample_Handle(int priority)
{
    return 1;
};
int Get_Digi_Handle(void)
{
    return 1;
}
long Sample_Length(void const* sample)
{
    return 0;
};
void Restore_Sound_Buffers(void){};
BOOL Set_Primary_Buffer_Format(void)
{
    return 0;
};
BOOL Start_Primary_Sound_Buffer(BOOL forced)
{
    return 0;
};
void Stop_Primary_Sound_Buffer(void){};
