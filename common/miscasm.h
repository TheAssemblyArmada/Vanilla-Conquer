#ifndef COMMON_MISCASM_H
#define COMMON_MISCASM_H

#ifdef __cplusplus
extern "C" {
#endif

int calcx(signed short param1, short distance);
int calcy(signed short param1, short distance);
unsigned int __cdecl Cardinal_To_Fixed(unsigned base, unsigned cardinal);
unsigned int __cdecl Fixed_To_Cardinal(unsigned base, unsigned fixed);
int __cdecl Desired_Facing256(LONG srcx, LONG srcy, LONG dstx, LONG dsty);
int __cdecl Desired_Facing8(long x1, long y1, long x2, long y2);
void __cdecl Set_Bit(void* array, int bit, int value);
int __cdecl Get_Bit(void const* array, int bit);
int __cdecl First_True_Bit(void const* array);
int __cdecl First_False_Bit(void const* array);
int __cdecl _Bound(int original, int min, int max);
#define Bound _Bound
void* __cdecl Conquer_Build_Fading_Table(void const* palette, void* dest, int color, int frac);
long __cdecl Reverse_Long(long number);
short __cdecl Reverse_Short(short number);
long __cdecl Swap_Long(long number);
void __cdecl strtrim(char* buffer);

#ifdef __cplusplus
}
#endif

#endif
