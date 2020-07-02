#include "compat.h"
#include "soscomp.h"
#include <string.h>

// index table for stepping into step table.
static const short wCODECIndexTab[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

static const short wCODECStepTab[89] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

#ifndef clamp
#define clamp(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

void __cdecl sosCODECInitStream(_SOS_COMPRESS_INFO* stream)
{
    stream->wCode = 0;
    stream->wCodeBuf = 0;
    stream->wIndex = 0;
    stream->wStep = wCODECStepTab[stream->wIndex];
    stream->dwPredicted = 0;
    stream->dwSampleIndex = 0;

    stream->wCode2 = 0;
    stream->wCodeBuf2 = 0;
    stream->wIndex2 = 0;
    stream->wStep2 = wCODECStepTab[stream->wIndex2];
    stream->dwPredicted2 = 0;
    stream->dwSampleIndex2 = 0;
}

//
// decompress data from a 4:1 ADPCM compressed file.  the number of
// bytes decompressed is returned.
//
unsigned long __cdecl sosCODECDecompressData(_SOS_COMPRESS_INFO* stream, unsigned long bytes)
{
    short current_nybble;
    unsigned step;
    int sample;
    unsigned full_length;

    full_length = bytes;
    stream->dwSampleIndex = 0;
    stream->dwSampleIndex2 = 0;

    if (stream->wBitSize == 16) {
        bytes /= 2;
    }

    char* src = stream->lpSource;
    short* dst = (short*)(stream->lpDest);

    // Handle stereo.
    if (stream->wChannels == 2) {
        current_nybble = 0;
        for (int i = bytes; i > 0; i -= 2) {
            if ((stream->dwSampleIndex & 1) != 0) {
                current_nybble = stream->wCodeBuf >> 4;
                stream->wCode = current_nybble;
            } else {
                stream->wCodeBuf = *src;
                // Stereo is interleaved so skip a byte for this channel.
                src += 2;
                current_nybble = stream->wCodeBuf & 0xF;
                stream->wCode = current_nybble;
            }

            step = stream->wStep;
            stream->dwDifference = step >> 3;

            if ((current_nybble & 4) != 0) {
                stream->dwDifference += step;
            }

            if ((current_nybble & 2) != 0) {
                stream->dwDifference += step >> 1;
            }

            if ((current_nybble & 1) != 0) {
                stream->dwDifference += step >> 2;
            }

            if ((current_nybble & 8) != 0) {
                stream->dwDifference = -stream->dwDifference;
            }

            sample = clamp(stream->dwDifference + stream->dwPredicted, -32768, 32767);
            stream->dwPredicted = sample;

            if (stream->wBitSize == 16) {
                *dst = sample;
                // Stereo is interleaved so skip a sample for this channel.
                dst += 2;
            } else {
                *dst++ = ((sample & 0xFF00) >> 8) ^ 0x80;
            }

            stream->wIndex += wCODECIndexTab[stream->wCode & 0x7];
            stream->wIndex = clamp(stream->wIndex, 0, 88);
            ++stream->dwSampleIndex;
            stream->wStep = wCODECStepTab[stream->wIndex];
        }

        src = stream->lpSource + 1;
        dst = (short*)(stream->lpDest + 1);

        if (stream->wBitSize == 16) {
            dst = (short*)(stream->lpDest) + 1;
        }

        for (int i = bytes; i > 0; i -= 2) {
            if ((stream->dwSampleIndex2 & 1) != 0) {
                current_nybble = stream->wCodeBuf2 >> 4;
                stream->wCode2 = current_nybble;
            } else {
                stream->wCodeBuf2 = *src;
                // Stereo is interleaved so skip a byte for this channel.
                src += 2;
                current_nybble = stream->wCodeBuf2 & 0xF;
                stream->wCode2 = current_nybble;
            }

            step = stream->wStep2;
            stream->dwDifference2 = step >> 3;

            if ((current_nybble & 4) != 0) {
                stream->dwDifference2 += step;
            }

            if ((current_nybble & 2) != 0) {
                stream->dwDifference2 += step >> 1;
            }

            if ((current_nybble & 1) != 0) {
                stream->dwDifference2 += step >> 2;
            }

            if ((current_nybble & 8) != 0) {
                stream->dwDifference2 = -stream->dwDifference2;
            }

            sample = clamp(stream->dwDifference2 + stream->dwPredicted2, -32768, 32767);
            stream->dwPredicted2 = sample;

            if (stream->wBitSize == 16) {
                *dst = sample;
                // Stereo is interleaved so skip a sample for this channel.
                dst += 2;
            } else {
                *dst++ = ((sample & 0xFF00) >> 8) ^ 0x80;
            }

            stream->wIndex2 += wCODECIndexTab[stream->wCode2 & 0x7];
            stream->wIndex2 = clamp(stream->wIndex2, 0, 88);
            ++stream->dwSampleIndex2;
            stream->wStep2 = wCODECStepTab[stream->wIndex2];
        }
    } else {
        for (int i = bytes; i > 0; --i) {
            if ((stream->dwSampleIndex & 1) != 0) {
                current_nybble = stream->wCodeBuf >> 4;
                stream->wCode = current_nybble;
            } else {
                stream->wCodeBuf = *src++;
                current_nybble = stream->wCodeBuf & 0xF;
                stream->wCode = current_nybble;
            }

            step = stream->wStep;
            stream->dwDifference = step >> 3;

            if ((current_nybble & 4) != 0) {
                stream->dwDifference += step;
            }

            if ((current_nybble & 2) != 0) {
                stream->dwDifference += step >> 1;
            }

            if ((current_nybble & 1) != 0) {
                stream->dwDifference += step >> 2;
            }

            if ((current_nybble & 8) != 0) {
                stream->dwDifference = -stream->dwDifference;
            }

            sample = clamp(stream->dwDifference + stream->dwPredicted, -32768, 32767);
            stream->dwPredicted = sample;

            if (stream->wBitSize == 16) {
                *dst++ = sample;
            } else {
                *dst = ((sample & 0xFF00) >> 8) ^ 0x80;
                dst = (short*)((char*)(dst) + 1);
            }

            stream->wIndex += wCODECIndexTab[stream->wCode & 0x7];
            stream->wIndex = clamp(stream->wIndex, 0, 88);
            ++stream->dwSampleIndex;
            stream->wStep = wCODECStepTab[stream->wIndex];
        };
    }

    return full_length;
}

//
// Compresses a data stream into 4:1 ADPCM.  16 bit data is compressed 4:1
// 8 bit data is compressed 2:1.
//
unsigned long __cdecl sosCODECCompressData(_SOS_COMPRESS_INFO* stream, unsigned long bytes)
{
    int delta;
    int tmp_step;
    short code;
    unsigned tmp;
    unsigned step;
    short current_samp;

    int samples = stream->wBitSize == 16 ? bytes >> 1 : bytes;
    stream->dwSampleIndex = 0;
    stream->dwSampleIndex2 = 0;
    short* src = (short*)(stream->lpSource);
    char* dst = stream->lpDest;

    if (stream->wChannels == 2) {
        // Compress a stereo data stream.
        for (int i = samples; i > 0; i -= 2) {
            if (stream->wBitSize == 16) {
                current_samp = *src;
                src += 2;
            } else {
                current_samp = *src;
                ++src;
                current_samp = (current_samp & 0xFF00) ^ 0x8000;
            }

            delta = current_samp - (stream->dwPredicted);
            code = 0;

            if (delta < 0) {
                delta = -delta;
                code = 8;
            }

            stream->wCode = code;
            tmp_step = stream->wStep;
            tmp = 4;

            for (int i = 3; i > 0; --i) {
                if (delta >= tmp_step) {
                    stream->wCode |= tmp;
                    delta -= tmp_step;
                }

                tmp_step = tmp_step >> 1;
                tmp >>= 1;
            };

            stream->dwDifference = delta;

            if (stream->dwSampleIndex & 1) {
                *dst = stream->wCodeBuf | (stream->wCode << 4);
                dst += 2;
            } else {
                stream->wCodeBuf = stream->wCode & 0xF;
            }

            step = stream->wStep;
            code = stream->wCode;
            stream->dwDifference = step >> 3;

            if (code & 4) {
                stream->dwDifference += step;
            }

            if (code & 2) {
                stream->dwDifference += step >> 1;
            }

            if (code & 1) {
                stream->dwDifference += step >> 2;
            }

            if (code & 8) {
                stream->dwDifference = -stream->dwDifference;
            }

            stream->dwPredicted = clamp(stream->dwDifference + stream->dwPredicted, -32768, 32767);
            stream->wIndex += wCODECIndexTab[stream->wCode];
            stream->wIndex = clamp(stream->wIndex, 0, 88);
            ++stream->dwSampleIndex;
            stream->wStep = wCODECStepTab[stream->wIndex];
        }

        src = (short*)(stream->lpSource + 1);
        dst = stream->lpDest + 1;

        if (stream->wBitSize == 16) {
            src = (short*)(stream->lpSource) + 1;
        }

        for (int i = samples; i > 0; i -= 2) {
            if (stream->wBitSize == 16) {
                current_samp = *src;
                src += 2;
            } else {
                current_samp = *src;
                ++src;
                current_samp = (current_samp & 0xFF00) ^ 0x8000;
            }

            delta = current_samp - (stream->dwPredicted2);
            code = 0;

            if (delta < 0) {
                delta = -delta;
                code = 8;
            }

            stream->wCode2 = code;
            tmp_step = stream->wStep2;
            tmp = 4;

            for (int i = 3; i > 0; --i) {
                if (delta >= tmp_step) {
                    stream->wCode2 |= tmp;
                    delta -= tmp_step;
                }

                tmp_step >>= 1;
                tmp >>= 1;
            };

            stream->dwDifference2 = delta;

            if (stream->dwSampleIndex2 & 1) {
                *dst = stream->wCodeBuf2 | (stream->wCode2 << 4);
                dst += 2;
            } else {
                stream->wCodeBuf2 = stream->wCode2 & 0xF;
            }

            step = stream->wStep2;
            code = stream->wCode2;
            stream->dwDifference2 = step >> 3;

            if (code & 4) {
                stream->dwDifference2 += step;
            }

            if (code & 2) {
                stream->dwDifference2 += step >> 1;
            }

            if (code & 1) {
                stream->dwDifference2 += step >> 2;
            }

            if (code & 8) {
                stream->dwDifference2 = -stream->dwDifference2;
            }

            stream->dwPredicted2 = clamp(stream->dwDifference2 + stream->dwPredicted2, -32768, 32767);
            stream->wIndex2 += wCODECIndexTab[stream->wCode2];
            stream->wIndex2 = clamp(stream->wIndex2, 0, 88);
            ++stream->dwSampleIndex2;
            stream->wStep2 = wCODECStepTab[stream->wIndex2];
        }
    } else {
        // Compress a mono data stream.
        for (int i = samples; i > 0; --i) {
            if (stream->wBitSize == 16) {
                current_samp = *src;
                ++src;
            } else {
                current_samp = *src;
                src = (short*)((char*)(src) + 1);
                current_samp = (current_samp & 0xFF00) ^ 0x8000;
            }

            delta = current_samp - (stream->dwPredicted);
            code = 0;

            if (delta < 0) {
                delta = -delta;
                code = 8;
            }

            stream->wCode = code;
            tmp_step = stream->wStep;
            tmp = 4;

            for (int i = 3; i > 0; --i) {
                if (delta >= tmp_step) {
                    stream->wCode |= tmp;
                    delta -= tmp_step;
                }

                tmp_step >>= 1;
                tmp >>= 1;
            }

            stream->dwDifference = delta;

            if (stream->dwSampleIndex & 1) {
                *dst++ = stream->wCodeBuf | (stream->wCode << 4);
            } else {
                stream->wCodeBuf = stream->wCode & 0xF;
            }

            step = stream->wStep;
            code = stream->wCode;
            stream->dwDifference = step >> 3;

            if (code & 4) {
                stream->dwDifference += step;
            }

            if (code & 2) {
                stream->dwDifference += step >> 1;
            }

            if (code & 1) {
                stream->dwDifference += step >> 2;
            }

            if (code & 8) {
                stream->dwDifference = -stream->dwDifference;
            }

            stream->dwPredicted = clamp(stream->dwDifference + stream->dwPredicted, -32768, 32767);
            stream->wIndex += wCODECIndexTab[stream->wCode];
            stream->wIndex = clamp(stream->wIndex, 0, 88);
            ++stream->dwSampleIndex;
            stream->wStep = wCODECStepTab[stream->wIndex];
        }
    }

    return stream->wBitSize == 16 ? bytes >> 2 : bytes >> 1;
}
