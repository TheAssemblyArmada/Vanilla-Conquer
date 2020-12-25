#ifndef CLIPRECT_H
#define CLIPRECT_H

int Clip_Rect(int* x, int* y, int* dw, int* dh, int width, int height);
int Confine_Rect(int* x, int* y, int dw, int dh, int width, int height);

#endif
