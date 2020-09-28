#include "drawmisc.h"
#include "gbuffer.h"
#include <string.h>

#ifdef _WIN32
#include <windows.h>
HWND MainWindow; // Handle to programs main window
#endif

extern "C" unsigned char CurrentPalette[768] = {255};
extern "C" unsigned char PaletteTable[1024] = {0};

bool SystemToVideoBlits = false;  // Does hardware support system mem to video mem blits?
bool VideoToSystemBlits = false;  // Does hardware support video mem to system mem blits?
bool SystemToSystemBlits = false; // Does hardware support system mem to system mem blits?
bool OverlappedVideoBlits = true; // Can video driver blit overlapped regions?
bool AllowHardwareBlitFills = true;

/*
** Function to call if we detect focus loss
*/
void (*Misc_Focus_Loss_Function)(void) = nullptr;
void (*Misc_Focus_Restore_Function)(void) = nullptr;

#ifdef NOASM

void Fat_Put_Pixel(int x, int y, int value, int size, GraphicViewPortClass& gvp)
{
    char* buf;
    int w;
    int pitch;

    if (size == 0) {
        return;
    }

    if ((unsigned)y >= (unsigned)gvp.Get_Height() || (unsigned)x >= (unsigned)gvp.Get_Width()) {
        return;
    }

    buf = reinterpret_cast<char*>(x + (gvp.Get_Pitch() + gvp.Get_XAdd() + gvp.Get_Width()) * y + gvp.Get_Offset());
    w = size;
    pitch = gvp.Get_Pitch() + gvp.Get_XAdd() + gvp.Get_Width();

    while (size--) {
        memset(buf, value, w);
        buf += pitch;
    }
}

#endif
