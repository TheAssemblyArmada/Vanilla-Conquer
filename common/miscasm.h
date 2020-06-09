#ifndef COMMON_MISCASM_H
#define COMMON_MISCASM_H

extern "C" int __cdecl calcx(signed short param1, short distance);
extern "C" int __cdecl calcy(signed short param1, short distance);
extern "C" unsigned int __cdecl Cardinal_To_Fixed(unsigned base, unsigned cardinal);
extern "C" unsigned int __cdecl Fixed_To_Cardinal(unsigned base, unsigned fixed);
extern "C" int __cdecl Desired_Facing256(LONG srcx, LONG srcy, LONG dstx, LONG dsty);
extern "C" int __cdecl Desired_Facing8(long x1, long y1, long x2, long y2);
extern "C" void __cdecl Set_Bit(void* array, int bit, int value);
extern "C" int __cdecl Get_Bit(void const* array, int bit);
extern "C" int __cdecl First_True_Bit(void const* array);
extern "C" int __cdecl First_False_Bit(void const* array);
extern "C" int __cdecl _Bound(int original, int min, int max);
#define Bound _Bound
extern "C" void* __cdecl Conquer_Build_Fading_Table(void const* palette, void* dest, int color, int frac);
extern "C" long __cdecl Reverse_Long(long number);
extern "C" short __cdecl Reverse_Short(short number);
extern "C" long __cdecl Swap_Long(long number);
extern "C" void __cdecl strtrim(char* buffer);

#endif
