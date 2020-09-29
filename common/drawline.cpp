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
#include "drawline.h"
#include "graphicsviewport.h"
#include <string.h>

#ifdef NOASM

// Clipping functions for line drawing.
static unsigned Line_Get_Clipping(GraphicViewPortClass& view, int x, int y)
{
    unsigned flags = 0;

    if (y < 0) {
        flags |= 0x1;
    }

    if (y > view.Get_Height() - 1) {
        flags |= 0x2;
    }

    if (x < 0) {
        flags |= 0x4;
    }

    if (x > view.Get_Width() - 1) {
        flags |= 0x8;
    }

    return flags;
}

static void Line_Clip_Top(GraphicViewPortClass& view, int& x1, int& y1, int x2, int y2)
{
    x1 += (x2 - x1) * (0 - y1) / (y2 - y1);
    y1 = 0;
}

static void Line_Clip_Bottom(GraphicViewPortClass& view, int& x1, int& y1, int x2, int y2)
{
    x1 += (x2 - x1) * (y1 - view.Get_Height() - 1) / (y1 - y2);
    y1 = view.Get_Height() - 1;
}

static void Line_Clip_Left(GraphicViewPortClass& view, int& x1, int& y1, int x2, int y2)
{
    y1 += (y2 - y1) * (0 - x1) / (x2 - x1);
    x1 = 0;
}

static void Line_Clip_Right(GraphicViewPortClass& view, int& x1, int& y1, int x2, int y2)
{
    y1 += (y2 - y1) * (x1 - view.Get_Width() - 1) / (x1 - x2);
    x1 = view.Get_Width() - 1;
}

void Buffer_Draw_Line(void* this_object, int sx, int sy, int dx, int dy, unsigned char color)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(this_object);
    uint8_t* screen = reinterpret_cast<uint8_t*>(vp.Get_Offset());
    int increment = 1;

    // Do we need to do any clipping
    if (unsigned(sx) >= unsigned(vp.Get_Width()) || unsigned(sy) >= unsigned(vp.Get_Height())
        || unsigned(dx) >= unsigned(vp.Get_Width()) || unsigned(dy) >= unsigned(vp.Get_Height())) {
        while (true) {
            unsigned clip1 = Line_Get_Clipping(vp, sx, sy);
            unsigned clip2 = Line_Get_Clipping(vp, dx, dy);

            // no more clipping to be done
            if (clip1 == 0 && clip2 == 0) {
                break;
            }

            // if both are flagged to clip the same, no need to draw the line
            if ((clip1 & clip2) != 0) {
                return;
            }

            switch (clip1) {
            case 1:
            case 9:
                Line_Clip_Top(vp, sx, sy, dx, dy);
                break;

            case 2:
            case 6:
                Line_Clip_Bottom(vp, sx, sy, dx, dy);
                break;

            case 4:
            case 5:
                Line_Clip_Left(vp, sx, sy, dx, dy);
                break;

            case 8:
            case 10:
                Line_Clip_Right(vp, sx, sy, dx, dy);
                break;

            default:
                switch (clip2) {
                case 1:
                case 9:
                    Line_Clip_Top(vp, dx, dy, sx, sy);
                    break;

                case 2:
                case 6:
                    Line_Clip_Bottom(vp, dx, dy, sx, sy);
                    break;

                case 4:
                case 5:
                    Line_Clip_Left(vp, dx, dy, sx, sy);
                    break;

                case 8:
                case 10:
                    Line_Clip_Right(vp, dx, dy, sx, sy);
                    break;

                default:
                    break;
                }
            }
        }
    }

    dy -= sy;

    if (dy == 0) {
        if (sx >= dx) {
            int x = sx;
            sx = dx;
            dx = x;
        }

        dx -= sx - 1;
        screen += sy * (vp.Get_XAdd() + vp.Get_Width() + vp.Get_Pitch()) + sx;
        memset(screen, color, dx);

        return;
    }

    if (dy < 0) {
        int x = sx;
        sx = dx;
        dx = x;
        dy = -dy;
        sy -= dy;
    }

    screen += sy * (vp.Get_XAdd() + vp.Get_Width() + vp.Get_Pitch());
    dx -= sx;

    if (dx == 0) {
        screen += sx;

        while (dy-- >= 0) {
            *screen = color;
            screen += (vp.Get_XAdd() + vp.Get_Width() + vp.Get_Pitch());
        }

        return;
    }

    if (dx < 0) {
        dx = -dx;
        increment = -1;
    }

    if (dx < dy) {
        int full = dy;
        int half = dy / 2;
        screen += sx;

        while (true) {
            *screen = color;

            if (dy-- == 0) {
                return;
            }

            screen += (vp.Get_XAdd() + vp.Get_Width() + vp.Get_Pitch());
            half -= dx;

            if (half < 0) {
                half += full;
                screen += increment;
            }
        }
    } else {
        int full = dx;
        int half = dx / 2;
        screen += sx;

        while (true) {
            *screen = color;

            if (dx-- == 0) {
                return;
            }

            screen += increment;
            half -= dy;

            if (half < 0) {
                half += full;
                screen += (vp.Get_XAdd() + vp.Get_Width() + vp.Get_Pitch());
            }
        }
    }
}

#endif
