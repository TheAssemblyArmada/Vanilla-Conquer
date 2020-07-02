#include "compat.h"
#include "winasm.h"
#include <string.h>

#define SIZE_OF_PALETTE 256
extern unsigned char PaletteInterpolationTable[SIZE_OF_PALETTE][SIZE_OF_PALETTE];

static unsigned char LineBuffer[1600];
static unsigned char TopLine[1600];
static unsigned char BottomLine[1600];

/**
 * Interpolates in X axis, leaves additional lines blank in the Y axis.
 */
void __cdecl Asm_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch)
{
    unsigned char* dptr = (unsigned char*)(dst);
    unsigned char* sptr = (unsigned char*)(src);

    while (src_height--) {
        unsigned char* wptr = dptr;

        for (int i = 0; i < src_width - 1; ++i) {
            *wptr++ = *sptr;
            *wptr++ = PaletteInterpolationTable[sptr[0]][sptr[1]];
            ++sptr;
        }

        *wptr++ = *sptr++;
        *wptr = 0;
        dptr += dst_pitch;
    }
}

/**
 * Interpolates in X axis, duplicates lines in the Y axis.
 */
void __cdecl Asm_Interpolate_Line_Double(void* src, void* dst, int src_height, int src_width, int dst_pitch)
{
    unsigned char* dptr = (unsigned char*)(dst);
    unsigned char* sptr = (unsigned char*)(src);

    while (src_height--) {
        unsigned char* wptr = dptr;
        unsigned char* bptr = LineBuffer;

        for (int i = 0; i < src_width - 1; ++i) {
            *wptr++ = *sptr;
            *bptr++ = *sptr;
            *wptr++ = PaletteInterpolationTable[sptr[0]][sptr[1]];
            *bptr++ = PaletteInterpolationTable[sptr[0]][sptr[1]];
            ++sptr;
        }

        *wptr++ = *sptr;
        *bptr++ = *sptr++;
        *wptr = 0;
        *bptr = 0;
        dptr += dst_pitch / 2;
        memcpy(dptr, LineBuffer, src_width * 2);
        dptr += dst_pitch / 2;
    }
}

static void Interpolate_X_Axis(void* src, void* dst, int src_width)
{
    unsigned char* dptr = (unsigned char*)(dst);
    unsigned char* sptr = (unsigned char*)(src);

    unsigned char* wptr = dptr;

    for (int i = 0; i < src_width - 1; ++i) {
        *wptr++ = *sptr;
        *wptr++ = PaletteInterpolationTable[sptr[0]][sptr[1]];
        ++sptr;
    }

    *wptr++ = *sptr;
    *wptr = 0;
}

static void Interpolate_Y_Axis(void* top_line, void* bottom_line, void* middle_line, int src_width)
{
    unsigned char* tlp = (unsigned char*)(top_line);
    unsigned char* mlp = (unsigned char*)(middle_line);
    unsigned char* blp = (unsigned char*)(bottom_line);
    int dst_width = 2 * src_width;

    for (int i = 0; i < dst_width; ++i) {
        *mlp++ = PaletteInterpolationTable[*tlp][*blp];
        ++tlp;
        ++blp;
    }
}

/**
 * @brief Interpolates in both the X and Y axis.
 */
void __cdecl Asm_Interpolate_Line_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch)
{
    unsigned char* dptr = (unsigned char*)(dst);
    unsigned char* buff_offset1 = TopLine;
    unsigned char* buff_offset2 = BottomLine;

    int pitch = dst_pitch / 2;
    Interpolate_X_Axis(src, buff_offset1, src_width);

    unsigned char* tmp = buff_offset1;
    buff_offset1 = buff_offset2;
    buff_offset2 = tmp;

    unsigned char* current_line = (unsigned char*)(src) + src_width;
    int lines = src_height - 1;

    while (lines--) {
        Interpolate_X_Axis(current_line, buff_offset1, src_width);
        Interpolate_Y_Axis(buff_offset2, buff_offset1, LineBuffer, src_width);
        memcpy(dptr, buff_offset2, 2 * src_width);
        dptr += pitch;
        memcpy(dptr, LineBuffer, 2 * src_width);
        current_line += src_width;
        dptr += pitch;

        unsigned char* tmp = buff_offset1;
        buff_offset1 = buff_offset2;
        buff_offset2 = tmp;
    }

    memcpy(dptr, buff_offset2, 2 * src_width);
    dptr += pitch;
    // memcpy(dptr, buff_offset1, 2 * src_width);
}
