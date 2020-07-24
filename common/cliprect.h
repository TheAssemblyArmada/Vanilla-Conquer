#ifndef CLIPRECT_H
#define CLIPRECT_H

#ifdef __cplusplus
extern "C" {
#endif

int _cdecl Clip_Rect(int* x, int* y, int* dw, int* dh, int width, int height);
int _cdecl Confine_Rect(int* x, int* y, int dw, int dh, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
