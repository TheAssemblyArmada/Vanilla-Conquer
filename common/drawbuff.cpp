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
#include "drawbuff.h"
#include "graphicsviewport.h"
#include <string.h>
#include <algorithm>

#ifdef NOASM

int Buffer_Get_Pixel(void* thisptr, int x, int y)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);

    if ((unsigned)x < (unsigned)vp.Get_Width() && (unsigned)y < (unsigned)vp.Get_Height()) {
        return *(reinterpret_cast<unsigned char*>(vp.Get_Offset()) + x
                 + y * (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width()));
    }

    return 0;
}

void Buffer_Put_Pixel(void* thisptr, int x, int y, unsigned char color)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);

    if ((unsigned)x < (unsigned)vp.Get_Width() && (unsigned)y < (unsigned)vp.Get_Height()) {
        *(reinterpret_cast<unsigned char*>(vp.Get_Offset()) + x
          + y * (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width())) = color;
    }
}

void Buffer_Fill_Rect(void* thisptr, int sx, int sy, int dx, int dy, unsigned char color)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);

    // If we aren't drawing within the viewport, return
    if (sx >= vp.Get_Width() || sy >= vp.Get_Height() || dx < 0 || dy < 0) {
        return;
    }

    // Clipping
    if (sx < 0) {
        sx = 0;
    }

    if (sy < 0) {
        sy = 0;
    }

    dx = std::min(dx, vp.Get_Width() - 1);
    dy = std::min(dy, vp.Get_Height() - 1);

    unsigned char* offset =
        sy * (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width()) + sx + reinterpret_cast<unsigned char*>(vp.Get_Offset());
    int height = dy - sy + 1;
    int width = dx - sx + 1;

    for (int i = 0; i < height; ++i) {
        memset(offset, color, width);
        offset += (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width());
    }
}

void Buffer_Clear(void* thisptr, unsigned char color)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);
    unsigned char* offset = reinterpret_cast<unsigned char*>(vp.Get_Offset());

    for (int h = 0; h < vp.Get_Height(); ++h) {
        memset(offset, color, vp.Get_Width());
        offset += (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width());
    }
}

#endif
