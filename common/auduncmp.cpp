#include "auduncmp.h"
#include <string.h>

static const signed char ZapTabTwo[4] = {-2, -1, 0, 1};

static const signed char ZapTabFour[16] = {-9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8};

#ifndef clamp
static int clamp(int x, int low, int high)
{
    return ((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x));
}
#endif

#ifdef _NDS
/* Nintendo DS only supports signed 8-bit audio.  */
#define DEST_TYPE signed char
static inline DEST_TYPE maybe_signify(int x)
{
    return x ^ 0x80;
}
#else
#define DEST_TYPE unsigned char
static inline DEST_TYPE maybe_signify(int x)
{
    return x;
}
#endif

short Audio_Unzap(void* source, void* dest, short size)
{
    short sample;
    unsigned char code;
    signed char count;
    unsigned short shifted;

    sample = 0x80; //-128
    unsigned char* src = (unsigned char*)(source);
    DEST_TYPE* dst = (DEST_TYPE*)(dest);
    unsigned short remaining = size;

    while (remaining > 0) { // expecting more output
        shifted = *src++;
        shifted <<= 2;
        code = (shifted & 0xFF00) >> 8;
        count = (shifted & 0x00FF) >> 2;

        switch (code) {
        case 2: // no compression...
            if (count & 0x20) {
                count <<= 3;          // here it's significant that (count) is signed:
                sample += count >> 3; // the sign bit will be copied by these shifts!
                *dst++ = maybe_signify(clamp(sample, 0, 255));
                remaining--; // one byte added to output
            } else {
                for (++count; count > 0; --count) {
                    --remaining;
                    *dst++ = maybe_signify(*src++);
                }

                sample = *(src - 1); // set (sample) to the last byte sent to output
            }
            break;

        case 1:                                 // ADPCM 8-bit -> 4-bit
            for (++count; count > 0; --count) { // decode (count+1) bytes
                code = *src++;
                sample += ZapTabFour[(code & 0x0F)]; // lower nibble
                *dst++ = maybe_signify(clamp(sample, 0, 255));
                sample += ZapTabFour[(code >> 4)]; // higher nibble
                *dst++ = maybe_signify(clamp(sample, 0, 255));
                remaining -= 2; // two bytes added to output
            }
            break;

        case 0:                                 // ADPCM 8-bit -> 2-bit
            for (++count; count > 0; --count) { // decode (count+1) bytes
                code = *src++;
                sample += ZapTabTwo[(code & 0x03)]; // lower 2 bits
                *dst++ = maybe_signify(clamp(sample, 0, 255));
                sample += ZapTabTwo[((code >> 2) & 0x03)]; // lower middle 2 bits
                *dst++ = maybe_signify(clamp(sample, 0, 255));
                sample += ZapTabTwo[((code >> 4) & 0x03)]; // higher middle 2 bits
                *dst++ = maybe_signify(clamp(sample, 0, 255));
                sample += ZapTabTwo[((code >> 6) & 0x03)]; // higher 2 bits
                *dst++ = maybe_signify(clamp(sample, 0, 255));
                remaining -= 4; // 4 bytes sent to output
            }
            break;

        default: // just copy (sample) (count+1) times to output
            memset(dst, maybe_signify(clamp(sample, 0, 255)), ++count);
            remaining -= count;
            dst += count;
            break;
        }
    }

    return size - remaining;
}
