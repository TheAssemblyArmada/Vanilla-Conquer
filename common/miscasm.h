#ifndef COMMON_MISCASM_H
#define COMMON_MISCASM_H

extern "C" int __cdecl calcx(signed short param1, short distance);
extern "C" int __cdecl calcy(signed short param1, short distance);
extern "C" unsigned int __cdecl Cardinal_To_Fixed(unsigned base, unsigned cardinal);
extern "C" unsigned int __cdecl Fixed_To_Cardinal(unsigned base, unsigned fixed);
extern "C" int __cdecl Desired_Facing256(LONG srcx, LONG srcy, LONG dstx, LONG dsty);
extern "C" int __cdecl Desired_Facing8(long x1, long y1, long x2, long y2);

#endif
