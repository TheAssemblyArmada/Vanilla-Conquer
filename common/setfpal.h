#ifndef SETFPAL_H
#define SETFPAL_H

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl Set_Font_Palette_Range(void const* palette, int start_idx, int end_idx);

#ifdef __cplusplus
}
#endif

#endif
