#include "bitblit.h"
#include <string.h>

#ifndef max
#define max(x, y) (x > y ? x : y)
#endif

typedef struct
{
    long GVPOffset;   // offset to graphic page
    int GVPWidth;     // width of graphic page
    int GVPHeight;    // height of graphic page
    int GVPXAdd;      // xadd for graphic page (0)
    int GVPXPos;      // x offset in relation to graphicbuff
    int GVPYPos;      // y offset in relation to graphicbuff
    long GVPPitch;    // Distance from one line to the next
    void* GVPBuffPtr; // related graphic buff
} GraphicViewPort;

int __cdecl Linear_Blit_To_Linear(void* thisptr,
                                  void* dest,
                                   int src_x,
                                   int src_y,
                                   int dst_x,
                                   int dst_y,
                                   int w,
                                   int h,
                                   int use_key)
{
    GraphicViewPort* src_vp = (GraphicViewPort*)thisptr;
    GraphicViewPort* dst_vp = (GraphicViewPort*)dest;
    unsigned char* src = (unsigned char *)(src_vp->GVPOffset);
    unsigned char* dst = (unsigned char *)(dst_vp->GVPOffset);
    int src_pitch = (src_vp->GVPPitch + src_vp->GVPXAdd + src_vp->GVPWidth);
    int dst_pitch = (dst_vp->GVPPitch + dst_vp->GVPXAdd + dst_vp->GVPWidth);

    if (src_x >= src_vp->GVPWidth || src_y >= src_vp->GVPHeight || dst_x >= dst_vp->GVPWidth
        || dst_y >= dst_vp->GVPHeight || h < 0 || w < 1) {
        return 0;
    }

    src_x = max(0, src_x);
    src_y = max(0, src_y);
    dst_x = max(0, dst_x);
    dst_y = max(0, dst_y);

    h = (dst_y + h) > dst_vp->GVPHeight ? dst_vp->GVPHeight - 1 - dst_y : h;
    w = (dst_x + w) > dst_vp->GVPWidth ? dst_vp->GVPWidth - 1 - dst_x : w;

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
