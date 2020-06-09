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

#endif
