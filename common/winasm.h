#ifndef WWMOUSE_H
#define WWMOUSE_H

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl Asm_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch);
void __cdecl Asm_Interpolate_Line_Double(void* src, void* dst, int src_height, int src_width, int dst_pitch);
void __cdecl Asm_Interpolate_Line_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch);

#ifdef __cplusplus
}
#endif

#endif