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
#include "keyframe.h"
#include "shape.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef _NDS
#pragma GCC optimize("Ofast,unroll-loops")
#pragma GCC target("arm")
#endif

#define SHP_HAS_PAL            0x0001
#define SHP_LCW_FRAME          0x80
#define SHP_XOR_FAR_FRAME      0x40
#define SHP_XOR_PREV_FRAME     0x20
#define BIGSHP_BUFFER_MIN_FREE 128000
#define BIGSHP_BUFFER_GROW     2000000

extern bool OriginalUseBigShapeBuffer;

// The predator effect basically just takes a destination pixel and replaces it
// with the value PredTable[PredFrame] pixels away if PartialCount
// is greater than or equal to 256. After every pixel, it is increased by
// PartialPred and reset to % 256 after reaching 256 or greater.
static const short PredTable[8] = {1, 3, 2, 5, 2, 3, 4, 1};
static unsigned PredFrame;
static unsigned PartialCount;
static unsigned PartialPred;
static unsigned char* PredatorLimit = nullptr;

// Buffer Frame to Page function pointer type defs
typedef void (*BF_Function)(int,
                            int,
                            unsigned char*,
                            unsigned char*,
                            int,
                            int,
                            unsigned char*,
                            unsigned char*,
                            unsigned char*,
                            int);
typedef void (
    *Single_Line_Function)(int, unsigned char*, unsigned char*, unsigned char*, unsigned char*, unsigned char*, int);

// Just copy source to dest as is.
void BF_Copy(int width,
             int height,
             unsigned char* dst,
             unsigned char* src,
             int dst_pitch,
             int src_pitch,
             unsigned char* ghost_lookup,
             unsigned char* ghost_tab,
             unsigned char* fade_tab,
             int count)
{
    while (height--) {
        memcpy(dst, src, width);
        dst = dst + dst_pitch + width;
        src = src + src_pitch + width;
    }
}

// Index 0 transparency
void BF_Trans(int width,
              int height,
              unsigned char* dst,
              unsigned char* src,
              int dst_pitch,
              int src_pitch,
              unsigned char* ghost_lookup,
              unsigned char* ghost_tab,
              unsigned char* fade_tab,
              int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;

            if (sbyte) {
                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

// Fading table based shadow and transparency
void BF_Ghost(int width,
              int height,
              unsigned char* dst,
              unsigned char* src,
              int dst_pitch,
              int src_pitch,
              unsigned char* ghost_lookup,
              unsigned char* ghost_tab,
              unsigned char* fade_tab,
              int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;
            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            *dst++ = sbyte;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

// Fading table based shadow and transparency with index 0 ignored
void BF_Ghost_Trans(int width,
                    int height,
                    unsigned char* dst,
                    unsigned char* src,
                    int dst_pitch,
                    int src_pitch,
                    unsigned char* ghost_lookup,
                    unsigned char* ghost_tab,
                    unsigned char* fade_tab,
                    int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;

            if (sbyte) {
                unsigned char fbyte = ghost_lookup[sbyte];

                if (fbyte != 0xFF) {
                    sbyte = ghost_tab[*dst + fbyte * 256];
                }

                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Fading(int width,
               int height,
               unsigned char* dst,
               unsigned char* src,
               int dst_pitch,
               int src_pitch,
               unsigned char* ghost_lookup,
               unsigned char* ghost_tab,
               unsigned char* fade_tab,
               int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src;

            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst++ = sbyte;
        }
    }
}

void BF_Fading_Trans(int width,
                     int height,
                     unsigned char* dst,
                     unsigned char* src,
                     int dst_pitch,
                     int src_pitch,
                     unsigned char* ghost_lookup,
                     unsigned char* ghost_tab,
                     unsigned char* fade_tab,
                     int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;

            if (sbyte) {
                for (int i = 0; i < count; ++i) {
                    sbyte = fade_tab[sbyte];
                }

                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Ghost_Fading(int width,
                     int height,
                     unsigned char* dst,
                     unsigned char* src,
                     int dst_pitch,
                     int src_pitch,
                     unsigned char* ghost_lookup,
                     unsigned char* ghost_tab,
                     unsigned char* fade_tab,
                     int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;
            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst++ = sbyte;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Ghost_Fading_Trans(int width,
                           int height,
                           unsigned char* dst,
                           unsigned char* src,
                           int dst_pitch,
                           int src_pitch,
                           unsigned char* ghost_lookup,
                           unsigned char* ghost_tab,
                           unsigned char* fade_tab,
                           int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;

            if (sbyte) {
                unsigned char fbyte = ghost_lookup[sbyte];

                if (fbyte != 0xFF) {
                    sbyte = ghost_tab[*dst + fbyte * 256];
                }

                for (int i = 0; i < count; ++i) {
                    sbyte = fade_tab[sbyte];
                }

                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Predator(int width,
                 int height,
                 unsigned char* dst,
                 unsigned char* src,
                 int dst_pitch,
                 int src_pitch,
                 unsigned char* ghost_lookup,
                 unsigned char* ghost_tab,
                 unsigned char* fade_tab,
                 int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            PartialCount += PartialPred;

            // if ( PartialCount & 0xFF00 ) {
            //    PartialCount &= 0xFFFF00FF;
            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    *dst = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            ++dst;
        }

        src += src_pitch + width;
        dst += dst_pitch;
    }
}

void BF_Predator_Trans(int width,
                       int height,
                       unsigned char* dst,
                       unsigned char* src,
                       int dst_pitch,
                       int src_pitch,
                       unsigned char* ghost_lookup,
                       unsigned char* ghost_tab,
                       unsigned char* fade_tab,
                       int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;
            if (sbyte) {
                PartialCount += PartialPred;

                if (PartialCount >= 256) {
                    PartialCount %= 256;

                    if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                        sbyte = dst[PredTable[PredFrame]];
                    }

                    PredFrame = (PredFrame + 2) % 8;
                }

                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Predator_Ghost(int width,
                       int height,
                       unsigned char* dst,
                       unsigned char* src,
                       int dst_pitch,
                       int src_pitch,
                       unsigned char* ghost_lookup,
                       unsigned char* ghost_tab,
                       unsigned char* fade_tab,
                       int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;
            PartialCount += PartialPred;

            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    sbyte = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            *dst++ = sbyte;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Predator_Ghost_Trans(int width,
                             int height,
                             unsigned char* dst,
                             unsigned char* src,
                             int dst_pitch,
                             int src_pitch,
                             unsigned char* ghost_lookup,
                             unsigned char* ghost_tab,
                             unsigned char* fade_tab,
                             int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;
            if (sbyte) {
                PartialCount += PartialPred;

                if (PartialCount >= 256) {
                    PartialCount %= 256;

                    if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                        sbyte = dst[PredTable[PredFrame]];
                    }

                    PredFrame = (PredFrame + 2) % 8;
                }

                unsigned char fbyte = ghost_lookup[sbyte];

                if (fbyte != 0xFF) {
                    sbyte = ghost_tab[*dst + fbyte * 256];
                }

                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Predator_Fading(int width,
                        int height,
                        unsigned char* dst,
                        unsigned char* src,
                        int dst_pitch,
                        int src_pitch,
                        unsigned char* ghost_lookup,
                        unsigned char* ghost_tab,
                        unsigned char* fade_tab,
                        int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;
            PartialCount += PartialPred;

            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    sbyte = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst++ = sbyte;
        }
    }
}

void BF_Predator_Fading_Trans(int width,
                              int height,
                              unsigned char* dst,
                              unsigned char* src,
                              int dst_pitch,
                              int src_pitch,
                              unsigned char* ghost_lookup,
                              unsigned char* ghost_tab,
                              unsigned char* fade_tab,
                              int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;
            if (sbyte) {
                PartialCount += PartialPred;

                if (PartialCount >= 256) {
                    PartialCount %= 256;

                    if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                        sbyte = dst[PredTable[PredFrame]];
                    }

                    PredFrame = (PredFrame + 2) % 8;
                }

                for (int i = 0; i < count; ++i) {
                    sbyte = fade_tab[sbyte];
                }

                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Predator_Ghost_Fading(int width,
                              int height,
                              unsigned char* dst,
                              unsigned char* src,
                              int dst_pitch,
                              int src_pitch,
                              unsigned char* ghost_lookup,
                              unsigned char* ghost_tab,
                              unsigned char* fade_tab,
                              int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;

            PartialCount += PartialPred;

            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    sbyte = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst++ = sbyte;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

void BF_Predator_Ghost_Fading_Trans(int width,
                                    int height,
                                    unsigned char* dst,
                                    unsigned char* src,
                                    int dst_pitch,
                                    int src_pitch,
                                    unsigned char* ghost_lookup,
                                    unsigned char* ghost_tab,
                                    unsigned char* fade_tab,
                                    int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            unsigned char sbyte = *src++;

            if (sbyte) {
                PartialCount += PartialPred;

                if (PartialCount >= 256) {
                    PartialCount %= 256;

                    if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                        sbyte = dst[PredTable[PredFrame]];
                    }

                    PredFrame = (PredFrame + 2) % 8;
                }

                unsigned char fbyte = ghost_lookup[sbyte];

                if (fbyte != 0xFF) {
                    sbyte = ghost_tab[*dst + fbyte * 256];
                }

                for (int i = 0; i < count; ++i) {
                    sbyte = fade_tab[sbyte];
                }

                *dst = sbyte;
            }

            ++dst;
        }

        src += src_pitch;
        dst += dst_pitch;
    }
}

// Jump table for BF_* functions
static const BF_Function OldShapeJumpTable[16] = {BF_Copy,
                                                  BF_Trans,
                                                  BF_Ghost,
                                                  BF_Ghost_Trans,
                                                  BF_Fading,
                                                  BF_Fading_Trans,
                                                  BF_Ghost_Fading,
                                                  BF_Ghost_Fading_Trans,
                                                  BF_Predator,
                                                  BF_Predator_Trans,
                                                  BF_Predator_Ghost,
                                                  BF_Predator_Ghost_Trans,
                                                  BF_Predator_Fading,
                                                  BF_Predator_Fading_Trans,
                                                  BF_Predator_Ghost_Fading,
                                                  BF_Predator_Ghost_Fading_Trans};

// Single line versions
void Single_Line_Skip(int width,
                      unsigned char* dst,
                      unsigned char* src,
                      unsigned char* ghost_lookup,
                      unsigned char* ghost_tab,
                      unsigned char* fade_tab,
                      int count)
{
}

// This was Short_Single_Line_Copy, renamed for consistency
void Single_Line_Copy(int width,
                      unsigned char* dst,
                      unsigned char* src,
                      unsigned char* ghost_lookup,
                      unsigned char* ghost_tab,
                      unsigned char* fade_tab,
                      int count)
{
    memcpy(dst, src, width);
}

void Single_Line_Trans(int width,
                       unsigned char* dst,
                       unsigned char* src,
                       unsigned char* ghost_lookup,
                       unsigned char* ghost_tab,
                       unsigned char* fade_tab,
                       int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        if (sbyte) {
            *dst = sbyte;
        }

        ++dst;
    }
}

void Single_Line_Ghost(int width,
                       unsigned char* dst,
                       unsigned char* src,
                       unsigned char* ghost_lookup,
                       unsigned char* ghost_tab,
                       unsigned char* fade_tab,
                       int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        unsigned char fbyte = ghost_lookup[sbyte];

        if (fbyte != 0xFF) {
            sbyte = ghost_tab[*dst + fbyte * 256];
        }

        *dst++ = sbyte;
    }
}

void Single_Line_Ghost_Trans(int width,
                             unsigned char* dst,
                             unsigned char* src,
                             unsigned char* ghost_lookup,
                             unsigned char* ghost_tab,
                             unsigned char* fade_tab,
                             int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        if (sbyte) {
            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            *dst = sbyte;
        }

        ++dst;
    }
}

void Single_Line_Fading(int width,
                        unsigned char* dst,
                        unsigned char* src,
                        unsigned char* ghost_lookup,
                        unsigned char* ghost_tab,
                        unsigned char* fade_tab,
                        int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        for (int i = 0; i < count; ++i) {
            sbyte = fade_tab[sbyte];
        }

        *dst++ = sbyte;
    }
}

void Single_Line_Fading_Trans(int width,
                              unsigned char* dst,
                              unsigned char* src,
                              unsigned char* ghost_lookup,
                              unsigned char* ghost_tab,
                              unsigned char* fade_tab,
                              int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;

        if (sbyte) {
            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst = sbyte;
        }

        ++dst;
    }
}

void Single_Line_Single_Fade(int width,
                             unsigned char* dst,
                             unsigned char* src,
                             unsigned char* ghost_lookup,
                             unsigned char* ghost_tab,
                             unsigned char* fade_tab,
                             int count)
{
    for (int i = width; i > 0; --i) {
        *dst++ = fade_tab[*src++];
    }
}

void Single_Line_Single_Fade_Trans(int width,
                                   unsigned char* dst,
                                   unsigned char* src,
                                   unsigned char* ghost_lookup,
                                   unsigned char* ghost_tab,
                                   unsigned char* fade_tab,
                                   int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;

        if (sbyte) {
            *dst = fade_tab[sbyte];
        }

        ++dst;
    }
}

void Single_Line_Ghost_Fading(int width,
                              unsigned char* dst,
                              unsigned char* src,
                              unsigned char* ghost_lookup,
                              unsigned char* ghost_tab,
                              unsigned char* fade_tab,
                              int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        unsigned char fbyte = ghost_lookup[sbyte];

        if (fbyte != 0xFF) {
            sbyte = ghost_tab[*dst + fbyte * 256];
        }

        for (int i = 0; i < count; ++i) {
            sbyte = fade_tab[sbyte];
        }

        *dst++ = sbyte;
    }
}

void Single_Line_Ghost_Fading_Trans(int width,
                                    unsigned char* dst,
                                    unsigned char* src,
                                    unsigned char* ghost_lookup,
                                    unsigned char* ghost_tab,
                                    unsigned char* fade_tab,
                                    int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;

        if (sbyte) {
            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst = sbyte;
        }

        ++dst;
    }
}

void Single_Line_Predator(int width,
                          unsigned char* dst,
                          unsigned char* src,
                          unsigned char* ghost_lookup,
                          unsigned char* ghost_tab,
                          unsigned char* fade_tab,
                          int count)
{
    for (int i = width; i > 0; --i) {
        PartialCount += PartialPred;

        if (PartialCount >= 256) {
            PartialCount %= 256;

            if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                *dst = dst[PredTable[PredFrame]];
            }

            PredFrame = (PredFrame + 2) % 8;
        }

        ++dst;
    }
}

void Single_Line_Predator_Trans(int width,
                                unsigned char* dst,
                                unsigned char* src,
                                unsigned char* ghost_lookup,
                                unsigned char* ghost_tab,
                                unsigned char* fade_tab,
                                int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        if (sbyte) {
            PartialCount += PartialPred;

            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    sbyte = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            *dst = sbyte;
        }

        ++dst;
    }
}

void Single_Line_Predator_Ghost(int width,
                                unsigned char* dst,
                                unsigned char* src,
                                unsigned char* ghost_lookup,
                                unsigned char* ghost_tab,
                                unsigned char* fade_tab,
                                int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        PartialCount += PartialPred;

        if (PartialCount >= 256) {
            PartialCount %= 256;

            if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                sbyte = dst[PredTable[PredFrame]];
            }

            PredFrame = (PredFrame + 2) % 8;
        }

        unsigned char fbyte = ghost_lookup[sbyte];

        if (fbyte != 0xFF) {
            sbyte = ghost_tab[*dst + fbyte * 256];
        }

        *dst++ = sbyte;
    }
}

void Single_Line_Predator_Ghost_Trans(int width,
                                      unsigned char* dst,
                                      unsigned char* src,
                                      unsigned char* ghost_lookup,
                                      unsigned char* ghost_tab,
                                      unsigned char* fade_tab,
                                      int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        if (sbyte) {
            PartialCount += PartialPred;

            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    sbyte = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            *dst = sbyte;
        }

        ++dst;
    }
}

void Single_Line_Predator_Fading(int width,
                                 unsigned char* dst,
                                 unsigned char* src,
                                 unsigned char* ghost_lookup,
                                 unsigned char* ghost_tab,
                                 unsigned char* fade_tab,
                                 int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        PartialCount += PartialPred;

        if (PartialCount >= 256) {
            PartialCount %= 256;

            if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                sbyte = dst[PredTable[PredFrame]];
            }

            PredFrame = (PredFrame + 2) % 8;
        }

        for (int i = 0; i < count; ++i) {
            sbyte = fade_tab[sbyte];
        }

        *dst++ = sbyte;
    }
}

void Single_Line_Predator_Fading_Trans(int width,
                                       unsigned char* dst,
                                       unsigned char* src,
                                       unsigned char* ghost_lookup,
                                       unsigned char* ghost_tab,
                                       unsigned char* fade_tab,
                                       int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;
        if (sbyte) {
            PartialCount += PartialPred;

            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    sbyte = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst = sbyte;
        }

        ++dst;
    }
}

void Single_Line_Predator_Ghost_Fading(int width,
                                       unsigned char* dst,
                                       unsigned char* src,
                                       unsigned char* ghost_lookup,
                                       unsigned char* ghost_tab,
                                       unsigned char* fade_tab,
                                       int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;

        PartialCount += PartialPred;

        if (PartialCount >= 256) {
            PartialCount %= 256;

            if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                sbyte = dst[PredTable[PredFrame]];
            }

            PredFrame = (PredFrame + 2) % 8;
        }

        unsigned char fbyte = ghost_lookup[sbyte];

        if (fbyte != 0xFF) {
            sbyte = ghost_tab[*dst + fbyte * 256];
        }

        for (int i = 0; i < count; ++i) {
            sbyte = fade_tab[sbyte];
        }

        *dst++ = sbyte;
    }
}

void Single_Line_Predator_Ghost_Fading_Trans(int width,
                                             unsigned char* dst,
                                             unsigned char* src,
                                             unsigned char* ghost_lookup,
                                             unsigned char* ghost_tab,
                                             unsigned char* fade_tab,
                                             int count)
{
    for (int i = width; i > 0; --i) {
        unsigned char sbyte = *src++;

        if (sbyte) {
            PartialCount += PartialPred;

            if (PartialCount >= 256) {
                PartialCount %= 256;

                if (&dst[PredTable[PredFrame]] < PredatorLimit) {
                    sbyte = dst[PredTable[PredFrame]];
                }

                PredFrame = (PredFrame + 2) % 8;
            }

            unsigned char fbyte = ghost_lookup[sbyte];

            if (fbyte != 0xFF) {
                sbyte = ghost_tab[*dst + fbyte * 256];
            }

            for (int i = 0; i < count; ++i) {
                sbyte = fade_tab[sbyte];
            }

            *dst = sbyte;
        }

        ++dst;
    }
}

// Jump table for Single_Line_* functions
static Single_Line_Function NewShapeJumpTable[32] = {Single_Line_Copy,
                                                     Single_Line_Trans,
                                                     Single_Line_Ghost,
                                                     Single_Line_Ghost_Trans,
                                                     Single_Line_Fading,
                                                     Single_Line_Fading_Trans,
                                                     Single_Line_Ghost_Fading,
                                                     Single_Line_Ghost_Fading_Trans,
                                                     Single_Line_Predator,
                                                     Single_Line_Predator_Trans,
                                                     Single_Line_Predator_Ghost,
                                                     Single_Line_Predator_Ghost_Trans,
                                                     Single_Line_Predator_Fading,
                                                     Single_Line_Predator_Fading_Trans,
                                                     Single_Line_Predator_Ghost_Fading,
                                                     Single_Line_Predator_Ghost_Fading_Trans,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip,
                                                     Single_Line_Skip};

// Definition copied from 2keyfram.cpp
typedef struct tShapeHeaderType
{
    unsigned draw_flags;
    char* shape_data;
    int shape_buffer; // 1 if shape is in theater buffer
} ShapeHeaderType;

// Copied from conquer.cpp
#define SHAPE_TRANS 0x40

// Used by Buffer_Frame_To_Page to flag which blit function to use for each line
// Results are cached for subsequent draw calls.
static void Setup_Shape_Header(int width,
                               int height,
                               void* frame,
                               void* draw_header,
                               int flags,
                               void* ghost_tab,
                               void* ghost_lookup)
{
    unsigned char* shape_src;
    unsigned char* flag_dst;
    unsigned char tmp_flags;
    int current_byte;
    unsigned char has_skipped;
    unsigned char row_flags;
    int skipped;

    shape_src = static_cast<unsigned char*>(frame);
    ShapeHeaderType* header = static_cast<ShapeHeaderType*>(draw_header);
    header->draw_flags = flags & 0x1340;
    flag_dst = static_cast<unsigned char*>(draw_header) + sizeof(*header);
    unsigned char* ghost = static_cast<unsigned char*>(ghost_lookup);

    for (int i = height; i > 0; --i) {
        tmp_flags = 0;
        skipped = 0;
        for (int j = width; j > 0; --j) {
            current_byte = *shape_src++;

            if (current_byte || !(flags & SHAPE_TRANS)) {
                if (flags & SHAPE_PREDATOR) {
                    tmp_flags |= 8;
                }

                if (flags & SHAPE_GHOST && ghost[current_byte] != 0xFF) {
                    tmp_flags |= 2;
                }

                if (flags & SHAPE_FADING) {
                    tmp_flags |= 4;
                }
            } else {
                tmp_flags |= 1;
                ++skipped;
            }
        }

        has_skipped = 0;

        if (tmp_flags & 1 && (has_skipped = 1, width == skipped)) {
            row_flags = 0x10;
        } else {
            row_flags = (tmp_flags & 4) | (tmp_flags & 2) | (tmp_flags & 8) | has_skipped;
        }

        *flag_dst++ = row_flags;
    }
}

void Buffer_Frame_To_Page(int x,
                          int y,
                          int width,
                          int height,
                          void* shape,
                          GraphicViewPortClass& viewport,
                          int flags,
                          ...)
{
    bool use_old_drawer = false; //true; // false; New draw system not supported in TD.
    int fade_count = 0;
    ShapeHeaderType* draw_header = nullptr;
    unsigned char* fade_table = nullptr;
    unsigned char* ghost_table = nullptr;
    unsigned char* ghost_lookup = nullptr;

    if (!shape) {
        return;
    }

    unsigned char* frame_data = static_cast<unsigned char*>(shape);

    /*if (UseOldShapeDraw) {
        use_old_drawer = true;
    } else*/
    if (UseBigShapeBuffer) {
        draw_header = static_cast<ShapeHeaderType*>(shape);
        unsigned char* shape_buff = reinterpret_cast<unsigned char*>(BigShapeBufferStart);

        if (draw_header->shape_buffer) {
            shape_buff = reinterpret_cast<unsigned char*>(TheaterShapeBufferStart);
        }

        frame_data = shape_buff + (uintptr_t)draw_header->shape_data;
        // use_old_drawer = false;
    }

    va_list ap;
    va_start(ap, flags);

    int blit_style = 0;

    if (flags & SHAPE_CENTER) {
        x -= width / 2;
        y -= height / 2;
    }

    // Sets for BF_Trans functions
    if (flags & SHAPE_TRANS) {
        blit_style |= 1;
    }

    // Sets for BF_Ghost functions
    if (flags & SHAPE_GHOST) {
        blit_style |= 2;
        ghost_lookup = va_arg(ap, unsigned char*);
        ghost_table = ghost_lookup + 256;
    }

    if (!UseBigShapeBuffer || UseOldShapeDraw) {
        use_old_drawer = true;
    }

    if (!use_old_drawer && (draw_header->draw_flags == 0xFFFFFFFF || draw_header->draw_flags != (flags & 0x1340))) {
        Setup_Shape_Header(width, height, frame_data, draw_header, flags, ghost_table, ghost_lookup);
    }

    // Sets for BF_Fading functions
    if (flags & SHAPE_FADING) {
        fade_table = va_arg(ap, unsigned char*);
        fade_count = va_arg(ap, int) & 0x3F;
        blit_style |= 4;

        if (!fade_count) {
            flags &= ~SHAPE_FADING;
        }

        // s_Special blitters for if fade step count is only 1
        NewShapeJumpTable[4] = Single_Line_Single_Fade;
        NewShapeJumpTable[5] = Single_Line_Single_Fade_Trans;

        if (fade_count != 1) {
            NewShapeJumpTable[4] = Single_Line_Fading;
            NewShapeJumpTable[5] = Single_Line_Fading_Trans;
        }
    }

    // Sets for BF_Predator functions
    if (flags & SHAPE_PREDATOR) {
        int current_frame = va_arg(ap, unsigned);
        blit_style |= 8;

        PredFrame = ((unsigned)current_frame) % 8;
        PartialCount = 0;
        PartialPred = 256;

        // Calculates the end of the visible display buffer, hopefully prevent crashes from predator effect.
        // Unused by default in RA, but would be nice on the phase tank in Aftermath.
        PredatorLimit = reinterpret_cast<unsigned char*>(viewport.Get_Offset())
                        + (viewport.Get_XAdd() + viewport.Get_Width() + viewport.Get_Pitch()) * viewport.Get_Height();
    }

    if (flags & SHAPE_PARTIAL) {
        PartialPred = va_arg(ap, int) & 0xFF;
    }

    va_end(ap);

    int xstart = x;
    int ystart = y;
    int yend = y + height - 1;
    int xend = x + width - 1;
    int ms_img_offset = 0;

    // If we aren't drawing within the viewport, return.
    if (xstart >= viewport.Get_Width() || ystart >= viewport.Get_Height() || xend <= 0 || yend <= 0) {
        return;
    }

    // Do any needed clipping.
    if (xstart < 0) {
        ms_img_offset = -xstart;
        xstart = 0;
        use_old_drawer = true;
    }

    if (ystart < 0) {
        frame_data += width * (-ystart);
        ystart = 0;
        use_old_drawer = true;
    }

    if (xend >= viewport.Get_Width() - 1) {
        xend = viewport.Get_Width() - 1;
        use_old_drawer = true;
    }

    if (yend >= viewport.Get_Height() - 1) {
        yend = viewport.Get_Height() - 1;
        use_old_drawer = true;
    }

    int blit_width = xend - xstart + 1;
    int blit_height = yend - ystart + 1;

    int pitch = viewport.Get_XAdd() + viewport.Get_Width() + viewport.Get_Pitch();
    unsigned char* dst = ystart * pitch + xstart + reinterpret_cast<unsigned char*>(viewport.Get_Offset());
    unsigned char* src = frame_data + ms_img_offset;
    int dst_pitch = pitch - blit_width;
    int src_pitch = width - blit_width;

    // Use "new" line drawing routines that appear to have been added during the windows port.
    if (!use_old_drawer) {
        // Here we can use the individual line drawing routines
        // Means we can skip drawing some lines all together or avoid using
        // more expensive routines on some lines.
        unsigned char* line_flags = reinterpret_cast<unsigned char*>(draw_header + 1);

        for (int i = 0; i < blit_height; ++i) {
            NewShapeJumpTable[line_flags[i] & 0x1F](
                blit_width, dst, src, ghost_lookup, ghost_table, fade_table, fade_count);
            src += width;
            dst += pitch;
        }

        return;
    }

    // Here we just use the function that will blit the entire frame
    // using the appropriate effects.
    if (blit_height > 0 && blit_width > 0) {
        OldShapeJumpTable[blit_style & 0xF](
            blit_width, blit_height, dst, src, dst_pitch, src_pitch, ghost_lookup, ghost_table, fade_table, fade_count);
    }
}
