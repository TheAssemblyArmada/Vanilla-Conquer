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
#ifndef VQAFILE_H
#define VQAFILE_H

#include "vqaaudio.h"
#include "vqaconfig.h"
#include "vqadrawer.h"

typedef struct _CaptionInfo CaptionInfo;

typedef enum
{
    COLORMODE_256 = 0,
    COLORMODE_15BIT = 1,
    COLORMODE_UNKNOWN1 = 2,
    COLORMODE_UNKNOWN2 = 3,
    COLORMODE_16BIT = 4,
} VQAColorModeType;

typedef enum
{
    VQACB_CODEBOOK_0x1 = 0x1,        // Unset for FullCB after loading in Load_CBF0 Load_CBFZ, Load_CBP0 and Load_CBPZ
    VQACB_COMPRESSED_CODEBOOK = 0x2, // Sset by Load_CBPZ, processed by Prepare_Frame then unset
    VQACB_CODEBOOK_0x4 = 0x4,
    VQACB_COMPRESSED_PALETTE = 0x8,   // Load_CPLZ, processed by Prepare_Frame then unset
    VQACB_COMPRESSED_POINTERS = 0x10, // Load_VPTZ, processed by Prepare_Frame then unset
} VQACBNodeFlags;

typedef enum
{
    FRAMENODE_FRAME_LOADED = 0x1,          // Set at end of VQA_LoadFrame
    FRAMENODE_COMPRESSED_POINTERS_K = 0x2, // set after VPTK chunk read, calls Load_VPTZ
    FRAMENODE_PALETTE = 0x4,               // confirmed, set after Load_CPL0, Load_CPLZ call
    FRAMENODE_COMPRESSED_PALETTE = 0x8,    // confirmed, set by Load_CPLZ, processed by Select_Frame then unset
    FRAMENODE_COMPRESSED_POINTERS = 0x10,  // confirmed, set by Load_VPTZ
    FRAMENODE_VECTOR_POINTER_TABLE_R = 0x20,
    FRAMENODE_VPTR_BUFFERED = 0x40,
    FRAMENODE_0x80 = 0x80,
    FRAMENODE_LOOP_REPEATED = 0x100,
    FRAMENODE_LOOP_SWITCHED = 0x200,
    FRAMENODE_CUSTOM_DATA = 0x400,
    FRAMENODE_UNKNOWN1 = 0x800,
} VQAFrameNodeFlags;

typedef enum
{
    VQAERR_NONE = 0,
    VQAERR_ERROR = -1,
    VQAERR_OPEN = -2,
    VQAERR_READ = -3,
    VQAERR_WRITE = -4,
    VQAERR_SEEK = -5,
    VQAERR_NOTVQA = -6,
    VQAERR_NOMEM = -7,
    VQAERR_NOBUFFER = -8,
    VQAERR_NOT_TIME = -9,
    VQAERR_SLEEPING = -10,
    VQAERR_VIDEO = -11,
    VQAERR_AUDIO = -12,
    VQAERR_PAUSED = -13,
} VQAErrorType;

typedef enum
{
    VQACMD_INIT = 1,
    VQACMD_CLEANUP = 2,
    VQACMD_OPEN = 3,
    VQACMD_CLOSE = 4,
    VQACMD_READ = 5,
    VQACMD_WRITE = 6,
    VQACMD_SEEK = 7,
    VQACMD_EOF = 8,
    VQACMD_SIZE = 9,
} VQAStreamActionType;

typedef long (*StreamHandlerFuncPtr)(VQAHandle*, long, void*, long);

typedef struct _VQACBNode
{
    uint8_t* Buffer;
    struct _VQACBNode* Next;
    unsigned Flags;
    unsigned CBOffset;
} VQACBNode;

typedef struct _VQAFrameNode
{
    uint8_t* Pointers;
    VQACBNode* Codebook;
    uint8_t* Palette;
    struct _VQAFrameNode* Next;
    unsigned Flags;
    int FrameNum;
    int PtrOffset;
    int PalOffset;
    int PaletteSize;
} VQAFrameNode;

typedef struct _VQAFlipper
{
    VQAFrameNode* CurFrame;
    int LastFrameNum;
} VQAFlipper;

#pragma pack(push, 1)
typedef struct _VQAHeader
{
    uint16_t Version;
    //    Known Header flag info
    //    1 = HasSound
    //    2 = HasAlternativeAudio
    //    4 = Set on LOLG VQA's that have a transparent/to be keyed background, it makes it use a special UnVQ function.
    //        Always set in BR/TS VQA's
    //        Set on LOL3 intro VQA
    //    8 = Always set in TS VQA's
    //        Set on LOL3 intro VQA
    //        Set on some LOLG VQA's, can only be decoded with UnVQ_4x2 in LOLG which is nearly identical to RA's but RA
    //        has value 15 while this has -1.
    //   16 = Always set in TS VQA's
    //        Set on LOL3 intro VQA
    //        Set on most BR VQA's
    uint16_t Flags;
    uint16_t Frames;
    uint16_t ImageWidth;
    uint16_t ImageHeight;
    uint8_t BlockWidth;
    uint8_t BlockHeight;
    uint8_t FPS;
    uint8_t Groupsize;
    uint16_t Num1Colors;
    uint16_t CBentries;
    uint16_t Xpos;
    uint16_t Ypos;
    uint16_t MaxFramesize;
    uint16_t SampleRate;
    uint8_t Channels;
    uint8_t BitsPerSample;
    uint16_t AltSampleRate; // Streams in old header
    uint8_t AltChannels;    // char FutureUse[12] was here
    uint8_t AltBitsPerSample;
    uint8_t ColorMode;
    uint8_t field_21;
    unsigned int MaxCompressedCBSize;
    unsigned int field_26;
} VQAHeader;
#pragma pack(pop)

typedef struct _VQAHandle
{
    void* VQAio;
    StreamHandlerFuncPtr StreamHandler;
    VQAData* VQABuf;
    VQAConfig Config;
    VQAHeader Header;
    int VocFH;
    CaptionInfo* field_BE; // EVA info?
    CaptionInfo* field_C2; // Captions info?
} VQAHandle;

typedef struct _VQAChunkHeader
{
    uint32_t ID;   // ID of the chunk header - FourCC
    uint32_t Size; // Length of the chunk data, in bytes.
} VQAChunkHeader;

#endif
