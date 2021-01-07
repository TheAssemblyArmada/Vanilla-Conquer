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

void Buffer_Remap(void* thisptr, int sx, int sy, int width, int height, void* remap)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);

    if (remap == nullptr) {
        return;
    }

    int xstart = sx;
    int ystart = sy;
    int xend = sx + width - 1;
    int yend = sy + height - 1;

    // If we aren't drawing within the viewport, return
    if (xstart >= vp.Get_Width() || ystart >= vp.Get_Height() || xend < 0 || yend < 0) {
        return;
    }

    // Clipping
    if (xstart < 0) {
        xstart = 0;
    }

    if (ystart < 0) {
        ystart = 0;
    }

    xend = std::min(xend, vp.Get_Width() - 1);
    yend = std::min(yend, vp.Get_Height() - 1);

    // Setup parameters for blit
    unsigned char* offset = ystart * (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width()) + xstart
                            + reinterpret_cast<unsigned char*>(vp.Get_Offset());
    int lines = yend - ystart + 1;
    int blit_width = xend - xstart + 1;
    unsigned char* fading_table = static_cast<unsigned char*>(remap);

    // remap blit
    while (lines--) {
        for (int i = 0; i < blit_width; ++i) {
            offset[i] = fading_table[offset[i]];
        }

        offset += (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width());
    }
}

long Buffer_To_Page(int x, int y, int w, int h, void* buffer, void* view)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(view);

    int xstart = x;
    int ystart = y;
    int xend = x + w - 1;
    int yend = y + h - 1;

    int xoffset = 0;
    int yoffset = 0;

    if (buffer == nullptr) {
        return 0;
    }

    // If we aren't drawing within the viewport, return
    if (xstart >= vp.Get_Width() || ystart >= vp.Get_Height() || xend < 0 || yend < 0) {
        return 0;
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

    xend = std::min(xend, vp.Get_Width() - 1);
    yend = std::min(yend, vp.Get_Height() - 1);

    int pitch = vp.Get_Pitch() + vp.Get_Width() + vp.Get_XAdd();
    unsigned char* dst = y * pitch + x + reinterpret_cast<unsigned char*>(vp.Get_Offset());
    unsigned char* src = xoffset + w * yoffset + static_cast<unsigned char*>(buffer);
    int lines = yend - ystart + 1;
    int blit_width = xend - xstart + 1;

    // blit
    while (lines--) {
        memcpy(dst, src, blit_width);
        src += w;
        dst += pitch;
    }

    return 0;
}
