#ifndef COMMON_MISCASM_H
#define COMMON_MISCASM_H

#ifdef __cplusplus
extern "C" {
#endif

int calcx(signed short param1, short distance);
int calcy(signed short param1, short distance);
unsigned int Cardinal_To_Fixed(unsigned base, unsigned cardinal);
unsigned int Fixed_To_Cardinal(unsigned base, unsigned fixed);
void Set_Bit(void* array, int bit, int value);
int Get_Bit(void const* array, int bit);
int First_True_Bit(void const* array);
int First_False_Bit(void const* array);
int _Bound(int original, int min, int max);
#define Bound _Bound
void* Conquer_Build_Fading_Table(void const* palette, void* dest, int color, int frac);
long Reverse_Long(long number);
short Reverse_Short(short number);
long Swap_Long(long number);
void strtrim(char* buffer);

#ifdef __cplusplus
}
#endif

#endif
