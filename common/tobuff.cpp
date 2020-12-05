// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#include "graphicsviewport.h"
#include <algorithm>
#include <cstring>

extern "C" void Buffer_To_Buffer(void* thisptr, int x, int y, int w, int h, void* buff, int size)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);
    int xstart = x;
    int ystart = y;
    int xend = x + w - 1;
    int yend = y + h - 1;

    int xoffset = 0;
    int yoffset = 0;

    // If we aren't drawing within the viewport, return
    if (!buff || size <= 0 || xstart >= vp.Get_Width() || ystart >= vp.Get_Height() || xend < 0 || yend < 0) {
        return;
    }

    // Clipping
    if (xstart < 0) {
        xoffset = -xstart;
        xstart = 0;
    }

    if (ystart < 0) {
        yoffset += h * (-ystart);
        ystart = 0;
    }

    xend = std::min(xend, (int)vp.Get_Width() - 1);
    yend = std::min(yend, (int)vp.Get_Height() - 1);

    // Setup parameters for blit
    int pitch = vp.Get_Pitch() + vp.Get_Width() + vp.Get_XAdd();
    uint8_t* src = y * pitch + x + reinterpret_cast<uint8_t*>(vp.Get_Offset());
    uint8_t* dst = xoffset + w * yoffset + static_cast<uint8_t*>(buff);
    int lines = yend - ystart + 1;
    int blit_width = xend - xstart + 1;

    // Is our buffer large enough?
    if (lines * w <= size) {
        // blit
        while (lines--) {
            std::memcpy(dst, src, blit_width);
            src += pitch;
            dst += w;
        }
    }
}
