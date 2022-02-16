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

/****************************************************************************
 *
 *  File              : soscomp.h
 *  Date Created      : 6/1/94
 *  Description       :
 *
 *  Programmer(s)     : Nick Skrepetos
 *  Last Modification : 10/1/94 - 11:37:9 AM
 *  Additional Notes  : Modified by Denzil E. Long, Jr.
 *
 *****************************************************************************
 *            Copyright (c) 1994,  HMI, Inc.  All Rights Reserved            *
 ****************************************************************************/

#ifndef _SOS_COMPRESS
#define _SOS_COMPRESS

/* compression types */
enum
{
    _ADPCM_TYPE_1,
};

/* define compression structure */
typedef struct _tagCOMPRESS_INFO
{
    char* lpSource;
    char* lpDest;
    unsigned int dwCompSize;
    unsigned int dwUnCompSize;
    unsigned int dwSampleIndex;
    int dwPredicted;
    int dwDifference;
    short wCodeBuf;
    short wCode;
    short wStep;
    short wIndex;

    unsigned int dwSampleIndex2; // added BP for channel 2
    int dwPredicted2;            // added BP for channel 2
    int dwDifference2;           // added BP for channel 2
    short wCodeBuf2;             // added BP for channel 2
    short wCode2;                // added BP for channel 2
    short wStep2;                // added BP for channel 2
    short wIndex2;               // added BP for channel 2
    short wBitSize;
    short wChannels; // added BP for # of channels
} _SOS_COMPRESS_INFO;

/* compressed file type header */
typedef struct _tagCOMPRESS_HEADER
{
    unsigned int dwType;             // type of compression
    unsigned int dwCompressedSize;   // compressed file size
    unsigned int dwUnCompressedSize; // uncompressed file size
    unsigned int dwSourceBitSize;    // original bit size
    char szName[16];                 // file type, for error checking
} _SOS_COMPRESS_HEADER;

/* Prototypes */
void sosCODECInitStream(_SOS_COMPRESS_INFO*);
unsigned int sosCODECCompressData(_SOS_COMPRESS_INFO*, unsigned int);
unsigned int sosCODECDecompressData(_SOS_COMPRESS_INFO*, unsigned int);

#endif
