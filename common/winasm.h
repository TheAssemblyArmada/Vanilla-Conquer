#ifndef WINASM_H
#define WINASM_H

void Asm_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch);
void Asm_Interpolate_Line_Double(void* src, void* dst, int src_height, int src_width, int dst_pitch);
void Asm_Interpolate_Line_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch);

#endif
