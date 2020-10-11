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
#include "xordelta.h"
#include <stdbool.h>

#define XOR_SMALL 127
#define XOR_MED   255
#define XOR_LARGE 16383
#define XOR_MAX   32767

#ifdef NOASM

void Apply_XOR_Delta(void* dst, const void* src)
{
    unsigned char* putp = (unsigned char*)(dst);
    const unsigned char* getp = (const unsigned char*)(src);
    unsigned short count = 0;
    unsigned char value = 0;
    unsigned char cmd = 0;

    while (true) {
        bool xorval = false;
        cmd = *getp++;
        count = cmd;

        if (!(cmd & 0x80)) {
            // 0b00000000
            if (cmd == 0) {
                count = *getp++ & 0xFF;
                value = *getp++;
                xorval = true;
                // 0b0???????
            }
        } else {
            // 0b1??????? remove most significant bit
            count &= 0x7F;
            if (count != 0) {
                putp += count;
                continue;
            }

            count = (*getp & 0xFF) + (*(getp + 1) << 8);
            getp += 2;

            // 0b10000000 0 0
            if (count == 0) {
                return;
            }

            // 0b100000000 0?
            if ((count & 0x8000) == 0) {
                putp += count;
                continue;
            } else {
                // 0b10000000 11
                if (count & 0x4000) {
                    count &= 0x3FFF;
                    value = *getp++;
                    xorval = true;
                    // 0b10000000 10
                } else {
                    count &= 0x3FFF;
                }
            }
        }

        if (xorval) {
            for (; count > 0; --count) {
                *putp++ ^= value;
            }
        } else {
            for (; count > 0; --count) {
                *putp++ ^= *getp++;
            }
        }
    }
}

void Copy_Delta_Buffer(int width, void* offset, const void* delta, int pitch)
{
    unsigned char* putp = (unsigned char*)(offset);
    const unsigned char* getp = (const unsigned char*)(delta);
    unsigned char value = 0;
    unsigned char cmd = 0;
    int length = 0;
    int count = 0;

    while (true) {
        bool xorval = false;
        cmd = *getp++;
        count = cmd;

        if (!(cmd & 0x80)) {
            // 0b00000000
            if (cmd == 0) {
                count = *getp++ & 0xFF;
                value = *getp++;
                xorval = true;
                // 0b0???????
            }
        } else {
            // 0b1??????? remove most significant bit
            count &= 0x7F;
            if (count != 0) {
                putp -= length;
                length += count;

                while (length >= width) {
                    length -= width;
                    putp += pitch;
                }

                putp += length;
                continue;
            }

            count = (*getp & 0xFF) + (*(getp + 1) << 8);
            getp += 2;

            // 0b10000000 0 0
            if (count == 0) {
                return;
            }

            // 0b100000000 0?
            if ((count & 0x8000) == 0) {
                putp -= length;
                length += count;

                while (length >= width) {
                    length -= width;
                    putp += pitch;
                }

                putp += length;
                continue;
            } else {
                // 0b10000000 11
                if (count & 0x4000) {
                    count &= 0x3FFF;
                    value = *getp++;
                    xorval = true;
                    // 0b10000000 10
                } else {
                    count &= 0x3FFF;
                }
            }
        }

        if (xorval) {
            for (; count > 0; --count) {
                *putp++ = value;
                ++length;

                if (length == width) {
                    length = 0;
                    putp += pitch - width;
                }
            }
        } else {
            for (; count > 0; --count) {
                *putp++ = *getp++;
                ++length;

                if (length == width) {
                    length = 0;
                    putp += pitch - width;
                }
            }
        }
    }
}

void XOR_Delta_Buffer(int width, void* offset, const void* delta, int pitch)
{
    unsigned char* putp = (unsigned char*)(offset);
    const unsigned char* getp = (unsigned char*)(delta);
    unsigned char value = 0;
    unsigned char cmd = 0;
    int length = 0;
    int count = 0;

    while (true) {
        bool xorval = false;
        cmd = *getp++;
        count = cmd;

        if (!(cmd & 0x80)) {
            // 0b00000000
            if (cmd == 0) {
                count = *getp++ & 0xFF;
                value = *getp++;
                xorval = true;
                // 0b0???????
            }
        } else {
            // 0b1??????? remove most significant bit
            count &= 0x7F;
            if (count != 0) {
                putp -= length;
                length += count;

                while (length >= width) {
                    length -= width;
                    putp += pitch;
                }

                putp += length;
                continue;
            }

            count = (*getp & 0xFF) + (*(getp + 1) << 8);
            getp += 2;

            // 0b10000000 0 0
            if (count == 0) {
                return;
            }

            // 0b100000000 0?
            if ((count & 0x8000) == 0) {
                putp -= length;
                length += count;

                while (length >= width) {
                    length -= width;
                    putp += pitch;
                }

                putp += length;
                continue;
            } else {
                // 0b10000000 11
                if (count & 0x4000) {
                    count &= 0x3FFF;
                    value = *getp++;
                    xorval = true;
                    // 0b10000000 10
                } else {
                    count &= 0x3FFF;
                }
            }
        }

        if (xorval) {
            for (; count > 0; --count) {
                *putp++ ^= value;
                ++length;

                if (length == width) {
                    length = 0;
                    putp += pitch - width;
                }
            }
        } else {
            for (; count > 0; --count) {
                *putp++ ^= *getp++;
                ++length;

                if (length == width) {
                    length = 0;
                    putp += pitch - width;
                }
            }
        }
    }
}

void Apply_XOR_Delta_To_Page_Or_Viewport(void* offset, const void* delta, int width, int pitch, int copy)
{
    if (copy) {
        Copy_Delta_Buffer(width, offset, delta, pitch);
    } else {
        XOR_Delta_Buffer(width, offset, delta, pitch);
    }
}

#endif

int Generate_XOR_Delta(void* dst, const void* src, const void* base, int size)
{
    unsigned char* putp = (unsigned char*)(dst);               // our delta
    const unsigned char* getsp = (const unsigned char*)(src);  // This is the image we go to
    const unsigned char* getbp = (const unsigned char*)(base); // This is image we come from
    const unsigned char* getsendp = getsp + size;

    // only check getsp. Both source and base should be same size and both pointers
    // should be incremented at the same time.
    while (getsp < getsendp) {
        unsigned fillcount = 0;
        unsigned xorcount = 0;
        unsigned skipcount = 0;
        unsigned char lastxor = *getsp ^ *getbp;
        const unsigned char* testsp = getsp;
        const unsigned char* testbp = getbp;

        // Only evaluate other options if we don't have a matched pair
        while (*testsp != *testbp && testsp < getsendp) {
            if ((*testsp ^ *testbp) == lastxor) {
                ++fillcount;
                ++xorcount;
            } else {
                if (fillcount > 3) {
                    break;
                } else {
                    lastxor = *testsp ^ *testbp;
                    fillcount = 1;
                    ++xorcount;
                }
            }
            ++testsp;
            ++testbp;
        }

        fillcount = fillcount > 3 ? fillcount : 0;

        // Okay, lets see if we have any xor bytes we need to handle
        xorcount -= fillcount;
        while (xorcount) {
            unsigned short count = 0;
            // cmd 0???????
            if (xorcount < XOR_MED) {
                count = xorcount <= XOR_SMALL ? xorcount : XOR_SMALL;
                *putp++ = (unsigned char)count;
                // cmd 10000000 10?????? ??????
            } else {
                count = xorcount <= XOR_LARGE ? xorcount : XOR_LARGE;
                *putp++ = 0x80;
                *putp++ = count & 0xFF;
                *putp++ = (count >> 8) | 0x80;
            }

            while (count) {
                *putp++ = *getsp++ ^ *getbp++;
                --count;
                --xorcount;
            }
        }

        // lets handle the bytes that are best done as xorfill
        while (fillcount) {
            unsigned short count = 0;
            // cmd 00000000 ????????
            if (fillcount <= XOR_MED) {
                count = fillcount;
                *putp++ = 0;
                *putp++ = (unsigned char)count;
                // cmd 10000000 11?????? ??????
            } else {
                count = fillcount <= XOR_LARGE ? fillcount : XOR_LARGE;
                *putp++ = 0x80;
                *putp++ = count & 0xFF;
                *putp++ = (count >> 8) | 0xC0;
            }
            *putp++ = *getsp ^ *getbp;
            fillcount -= count;
            getsp += count;
            getbp += count;
        }

        // Handle regions that match exactly
        while (*testsp == *testbp && testsp < getsendp) {
            ++skipcount;
            ++testsp;
            ++testbp;
        }

        while (skipcount) {
            unsigned short count = 0;
            if (skipcount < XOR_MED) {
                count = skipcount <= XOR_SMALL ? skipcount : XOR_SMALL;
                *putp++ = count | 0x80;
                // cmd 10000000 0??????? ????????
            } else {
                count = skipcount <= XOR_MAX ? skipcount : XOR_MAX;
                *putp++ = 0x80;
                *putp++ = count & 0xFF;
                *putp++ = count >> 8;
            }
            skipcount -= count;
            getsp += count;
            getbp += count;
        }
    }

    // final skip command of 0;
    *putp++ = 0x80;
    *putp++ = 0;
    *putp++ = 0;

    return putp - (unsigned char*)(dst);
}
