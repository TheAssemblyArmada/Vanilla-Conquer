#include "winasm.h"
#include <string.h>

struct InterpolationTable* InterpolationTable = NULL;

/**
 * Interpolates in X axis, leaves additional lines blank in the Y axis.
 */
void Asm_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch)
{
    unsigned char* dptr = (unsigned char*)(dst);
    unsigned char* sptr = (unsigned char*)(src);

    while (src_height--) {
        unsigned char* wptr = dptr;

        for (int i = 0; i < src_width - 1; ++i) {
            *wptr++ = *sptr;
            *wptr++ = InterpolationTable->PaletteInterpolationTable[sptr[0]][sptr[1]];
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
void Asm_Interpolate_Line_Double(void* src, void* dst, int src_height, int src_width, int dst_pitch)
{
    unsigned char* dptr = (unsigned char*)(dst);
    unsigned char* sptr = (unsigned char*)(src);

    while (src_height--) {
        unsigned char* wptr = dptr;
        unsigned char* bptr = InterpolationTable->LineBuffer;

        for (int i = 0; i < src_width - 1; ++i) {
            *wptr++ = *sptr;
            *bptr++ = *sptr;
            *wptr++ = InterpolationTable->PaletteInterpolationTable[sptr[0]][sptr[1]];
            *bptr++ = InterpolationTable->PaletteInterpolationTable[sptr[0]][sptr[1]];
            ++sptr;
        }

        *wptr++ = *sptr;
        *bptr++ = *sptr++;
        *wptr = 0;
        *bptr = 0;
        dptr += dst_pitch / 2;
        memcpy(dptr, InterpolationTable->LineBuffer, src_width * 2);
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
        *wptr++ = InterpolationTable->PaletteInterpolationTable[sptr[0]][sptr[1]];
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
        *mlp++ = InterpolationTable->PaletteInterpolationTable[*tlp][*blp];
        ++tlp;
        ++blp;
    }
}

/**
 * @brief Interpolates in both the X and Y axis.
 */
void Asm_Interpolate_Line_Interpolate(void* src, void* dst, int src_height, int src_width, int dst_pitch)
{
    unsigned char* dptr = (unsigned char*)(dst);
    unsigned char* buff_offset1 = InterpolationTable->TopLine;
    unsigned char* buff_offset2 = InterpolationTable->BottomLine;

    int pitch = dst_pitch / 2;
    Interpolate_X_Axis(src, buff_offset1, src_width);

    unsigned char* tmp = buff_offset1;
    buff_offset1 = buff_offset2;
    buff_offset2 = tmp;

    unsigned char* current_line = (unsigned char*)(src) + src_width;
    int lines = src_height - 1;

    while (lines--) {
        Interpolate_X_Axis(current_line, buff_offset1, src_width);
        Interpolate_Y_Axis(buff_offset2, buff_offset1, InterpolationTable->LineBuffer, src_width);
        memcpy(dptr, buff_offset2, 2 * src_width);
        dptr += pitch;
        memcpy(dptr, InterpolationTable->LineBuffer, 2 * src_width);
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
