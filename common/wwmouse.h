#ifndef WWMOUSE_H
#define WWMOUSE_H

#ifdef __cplusplus
extern "C" {
#endif

void Mouse_Shadow_Buffer(void* thisptr, void* srcdst, void* buffer, int x, int y, int hotx, int hoty, int store);
void Draw_Mouse(void* thisptr, void* srcdst, int x, int y);
void* ASM_Set_Mouse_Cursor(void* thisptr, int hotspotx, int hotspoty, void* cursor);

#ifdef __cplusplus
}
#endif

#endif
