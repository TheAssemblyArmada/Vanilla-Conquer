#ifndef DRAWMISC_H
#define DRAWMISC_H

extern "C" void __cdecl Buffer_Draw_Line(void* this_object, int sx, int sy, int dx, int dy, unsigned char color);
extern "C" void __cdecl Buffer_Fill_Rect(void* thisptr, int sx, int sy, int dx, int dy, unsigned char color);

#endif
