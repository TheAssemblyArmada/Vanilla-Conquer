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
    unsigned char* src = reinterpret_cast<unsigned char*>(src_vp.Get_Offset());
    unsigned char* dst = reinterpret_cast<unsigned char*>(dst_vp.Get_Offset());
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

// Note that this function does not generate pixel perfect results when compared to the ASM implementation
// It should not generate anything that is visibly different without doing side byt side comparisons however.
bool Linear_Scale_To_Linear(void* thisptr,
                            void* dest,
                            int src_x,
                            int src_y,
                            int dst_x,
                            int dst_y,
                            int src_width,
                            int src_height,
                            int dst_width,
                            int dst_height,
                            bool trans,
                            char* remap)
{
    GraphicViewPortClass& src_vp = *static_cast<GraphicViewPortClass*>(thisptr);
    GraphicViewPortClass& dst_vp = *static_cast<GraphicViewPortClass*>(dest);
    // If there is nothing to scale, just return.
    if (src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
        return true;
    }

    int src_x0 = src_x;
    int src_y0 = src_y;
    int dst_x0 = dst_x;
    int dst_y0 = dst_y;
    int dst_x1 = dst_width + dst_x;
    int dst_y1 = dst_height + dst_y;

    // These ifs are all for clipping purposes in case coords are outside
    // the expected area.
    if (src_x < 0) {
        src_x0 = 0;
        dst_x0 = dst_x + ((dst_width * -src_x) / src_width);
    }

    if (src_y < 0) {
        src_y0 = 0;
        dst_y0 = dst_y + ((dst_height * -src_y) / src_height);
    }

    if (src_x + src_width > src_vp.Get_Width() + 1) {
        dst_x1 = dst_x + (dst_width * (src_vp.Get_Width() - src_x) / src_width);
    }

    if (src_y + src_height > src_vp.Get_Height() + 1) {
        dst_y1 = dst_y + (dst_height * (src_vp.Get_Height() - src_y) / src_height);
    }

    if (dst_x0 < 0) {
        dst_x0 = 0;
        src_x0 = src_x + ((src_width * -dst_x) / dst_width);
    }

    if (dst_y0 < 0) {
        dst_y0 = 0;
        src_y0 = src_y + ((src_height * -dst_y) / dst_height);
    }

    if (dst_x1 > dst_vp.Get_Width() + 1) {
        dst_x1 = dst_vp.Get_Width();
    }

    if (dst_y1 > dst_vp.Get_Height() + 1) {
        dst_y1 = dst_vp.Get_Height();
    }

    if (dst_y0 > dst_y1 || dst_x0 > dst_x1) {
        return true;
    }

    char* src = src_y0 * (src_vp.Get_Pitch() + src_vp.Get_XAdd() + src_vp.Get_Width()) + src_x0
                + reinterpret_cast<char*>(src_vp.Get_Offset());
    char* dst = dst_y0 * (dst_vp.Get_Pitch() + dst_vp.Get_XAdd() + dst_vp.Get_Width()) + dst_x0
                + reinterpret_cast<char*>(dst_vp.Get_Offset());
    dst_x1 -= dst_x0;
    dst_y1 -= dst_y0;
    int x_ratio = ((src_width << 16) / dst_x1) + 1;
    int y_ratio = ((src_height << 16) / dst_y1) + 1;

    // trans basically means do we skip index 0 entries, thus treating them as
    // transparent?
    if (trans) {
        if (remap != nullptr) {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp.Get_Pitch() + dst_vp.Get_XAdd() + dst_vp.Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (src_vp.Get_Pitch() + src_vp.Get_XAdd() + src_vp.Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    unsigned char tmp = s[xrat >> 16];

                    if (tmp != 0) {
                        *d = (remap)[tmp];
                    }

                    ++d;
                    xrat += x_ratio;
                }
            }
        } else {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp.Get_Pitch() + dst_vp.Get_XAdd() + dst_vp.Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (src_vp.Get_Pitch() + src_vp.Get_XAdd() + src_vp.Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    unsigned char tmp = s[xrat >> 16];

                    if (tmp != 0) {
                        *d = tmp;
                    }

                    ++d;
                    xrat += x_ratio;
                }
            }
        }
    } else {
        if (remap != nullptr) {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp.Get_Pitch() + dst_vp.Get_XAdd() + dst_vp.Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (src_vp.Get_Pitch() + src_vp.Get_XAdd() + src_vp.Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    *d++ = (remap)[s[xrat >> 16]];
                    xrat += x_ratio;
                }
            }
        } else {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp.Get_Pitch() + dst_vp.Get_XAdd() + dst_vp.Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (src_vp.Get_Pitch() + src_vp.Get_XAdd() + src_vp.Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    *d++ = s[xrat >> 16];
                    xrat += x_ratio;
                }
            }
        }
    }

    return true;
}
