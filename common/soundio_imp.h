#include <stddef.h>

struct SampleTrackerTypeImp;

void SoundImp_Buffer_Sample_Data(SampleTrackerTypeImp* st, const void* data, size_t datalen);
int SoundImp_Get_Sample_Free_Buffer_Count(SampleTrackerTypeImp* st);
bool SoundImp_Init(int bits_per_sample, bool stereo, int rate, bool reverse_channels);
void SoundImp_PauseSound();
bool SoundImp_ResumeSound();
SampleTrackerTypeImp* SoundImp_Init_Sample(int bits_per_sample, bool stereo, int rate);
bool SoundImp_Sample_Status(SampleTrackerTypeImp* st);
void SoundImp_Set_Sample_Attributes(SampleTrackerTypeImp* st, int bits_per_sample, bool stereo, int rate);
void SoundImp_Set_Sample_Volume(SampleTrackerTypeImp* st, unsigned int volume);
void SoundImp_Shutdown();
void SoundImp_Shutdown_Sample(SampleTrackerTypeImp* st);
void SoundImp_Start_Sample(SampleTrackerTypeImp* st);
void SoundImp_Stop_Sample(SampleTrackerTypeImp* st);
