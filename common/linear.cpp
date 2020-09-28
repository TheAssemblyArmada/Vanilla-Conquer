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
#include "linear.h"
#include "graphicsviewport.h"
#include <algorithm>
#include <string.h>

int Linear_Blit_To_Linear(void* thisptr,
                                  void* dest,
                                   int src_x,
                                   int src_y,
                                   int dst_x,
                                   int dst_y,
                                   int w,
                                   int h,
                                   int use_key)
{
    GraphicViewPortClass& src_vp = *static_cast<GraphicViewPortClass*>(thisptr);
    GraphicViewPortClass& dst_vp = *static_cast<GraphicViewPortClass*>(dest);
    unsigned char* src = reinterpret_cast<unsigned char *>(src_vp.Get_Offset());
    unsigned char* dst = reinterpret_cast<unsigned char *>(dst_vp.Get_Offset());
    int src_pitch = (src_vp.Get_Pitch() + src_vp.Get_XAdd() + src_vp.Get_Width());
    int dst_pitch = (dst_vp.Get_Pitch() + dst_vp.Get_XAdd() + dst_vp.Get_Width());

    if (src_x >= src_vp.Get_Width() || src_y >= src_vp.Get_Height() || dst_x >= dst_vp.Get_Width()
        || dst_y >= dst_vp.Get_Height() || h < 0 || w < 1) {
        return 0;
    }

    src_x = std::max(0, src_x);
    src_y = std::max(0, src_y);
    dst_x = std::max(0, dst_x);
    dst_y = std::max(0, dst_y);

    h = (dst_y + h) > dst_vp.Get_Height() ? dst_vp.Get_Height() - 1 - dst_y : h;
    w = (dst_x + w) > dst_vp.Get_Width() ? dst_vp.Get_Width() - 1 - dst_x : w;

    // move our pointers to the start locations
    src += src_x + src_y * src_pitch;
    dst += dst_x + dst_y * dst_pitch;

    // If src is before dst, we run the risk of overlapping memory regions so we
    // need to move src and dst to the last line and work backwards
    if (src < dst) {
        unsigned char* esrc = src + (h - 1) * src_pitch;
        unsigned char* edst = dst + (h - 1) * dst_pitch;
        if (use_key) {
            char key_colour = 0;
            while (h-- != 0) {
                // Can't optimize as we need to check every pixel against key colour :(
                for (int i = w - 1; i >= 0; --i) {
                    if (esrc[i] != key_colour) {
                        edst[i] = esrc[i];
                    }
                }

                edst -= dst_pitch;
                esrc -= src_pitch;
            }
        } else {
            while (h-- != 0) {
                memmove(edst, esrc, w);
                edst -= dst_pitch;
                esrc -= src_pitch;
            }
        }
    } else {
        if (use_key) {
            unsigned char key_colour = 0;
            while (h-- != 0) {
                // Can't optimize as we need to check every pixel against key colour :(
                for (int i = 0; i < w; ++i) {
                    if (src[i] != key_colour) {
                        dst[i] = src[i];
                    }
                }

                dst += dst_pitch;
                src += src_pitch;
            }
        } else {
            while (h-- != 0) {
                memmove(dst, src, w);
                dst += dst_pitch;
                src += src_pitch;
            }
        }
    }

    // Return supposed to match DDraw to 0 is DD_OK.
    return 0;
}
