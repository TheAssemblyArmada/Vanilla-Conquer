//
// Copyright 2020 Electronic Arts Inc.
//
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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Part of the FILEIO Library               *
 *                                                                         *
 *                    File Name : IFF.H                                    *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : April 20, 1994                           *
 *                                                                         *
 *                  Last Update : April 20, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef IFF_H
#define IFF_H

#include "gbuffer.h"
#include "misc.h"    // This is needed fro Reverse_WORD and _LONG
#include "memflag.h" // This is needed for MemoryFlagType.
#include "ccfile.h"

#define LZW_SUPPORTED FALSE

/*=========================================================================*/
/* Iff and Load Picture system defines and enumerations							*/
/*=========================================================================*/

#define MAKE_ID(a, b, c, d) ((int)((int)d << 24) | ((int)c << 16) | ((int)b << 8) | (int)(a))
#define IFFize_WORD(a)      Reverse_Word(a)
#define IFFize_LONG(a)      Reverse_Long(a)

// lint -strong(AJX,PicturePlaneType)
typedef enum
{
    BM_AMIGA, // Bit plane format (8K per bitplane).
    BM_MCGA,  // Byte per pixel format (64K).

    BM_DEFAULT = BM_MCGA // Default picture format.
} PicturePlaneType;

/*
**	This is the compression type code.  This value is used in the compressed
**	file header to indicate the method of compression used.  Note that the
**	LZW method may not be supported.
*/
// lint -strong(AJX,CompressionType)
typedef enum
{
    NOCOMPRESS, // No compression (raw data).
    LZW12,      // LZW 12 bit codes.
    LZW14,      // LZW 14 bit codes.
    HORIZONTAL, // Run length encoding (RLE).
    LCW         // Westwood proprietary compression.
} CompressionType;

/*
**	Compressed blocks of data must start with this header structure.
**	Note that disk based compressed files have an additional two
**	leading bytes that indicate the size of the entire file.
*/
// lint -strong(AJX,CompHeaderType)
#pragma pack(push, 1)
typedef struct
{
    char Method;  // Compression method (CompressionType).
    char pad;     // Reserved pad byte (always 0).
    int32_t Size; // Size of the uncompressed data.
    short Skip;   // Number of bytes to skip before data.
} CompHeaderType;
#pragma pack(pop)

/*=========================================================================*/
/* The following prototypes are for the file: IFF.CPP								*/
/*=========================================================================*/

int Open_Iff_File(char const* filename);
void Close_Iff_File(int fh);
unsigned int Get_Iff_Chunk_Size(int fh, int id);
unsigned int Read_Iff_Chunk(int fh, int id, void* buffer, unsigned int maxsize);
void Write_Iff_Chunk(int file, int id, void* buffer, int length);

/*=========================================================================*/
/* The following prototypes are for the file: LOADPICT.CPP						*/
/*=========================================================================*/

int Load_Picture(char const* filename,
                 BufferClass& scratchbuf,
                 BufferClass& destbuf,
                 unsigned char* palette = NULL,
                 PicturePlaneType format = BM_DEFAULT);

/*=========================================================================*/
/* The following prototypes are for the file: LOAD.CPP							*/
/*=========================================================================*/

unsigned int Load_Data(char const* name, void* ptr, unsigned int size);
unsigned int Write_Data(char const* name, void* ptr, unsigned int size);
void* Load_Alloc_Data(char const* name, MemoryFlagType flags);
void* Load_Alloc_Data(const FileClass& file);
void* Load_Alloc_Data(const FileClass* file);
unsigned int
Load_Uncompress(char const* file, BufferClass& uncomp_buff, BufferClass& dest_buff, void* reserved_data = NULL);
unsigned int Uncompress_Data(void const* src, void* dst);
void Set_Uncomp_Buffer(int buffer_segment, int size_of_buffer);

/*=========================================================================*/
/* The following prototypes are for the file: WRITELBM.CPP						*/
/*=========================================================================*/

bool Write_LBM_File(int lbmhandle, BufferClass& buff, int bitplanes, unsigned char* palette);

/*========================= Assembly Functions ============================*/

/*=========================================================================*/
/* The following prototypes are for the file: PACK2PLN.ASM						*/
/*=========================================================================*/

extern void Pack_2_Plane(void* buffer, void* pageptr, int planebit);

/*=========================================================================*/

#endif // IFF_H
