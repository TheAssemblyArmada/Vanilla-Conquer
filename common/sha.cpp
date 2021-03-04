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

/* $Header: /CounterStrike/SHA.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SHA.CPP                                                      *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 07/03/96                                                     *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   SHAEngine::Result -- Fetch the current digest.                                            *
 *   SHAEngine::Hash -- Process an arbitrarily long data block.                                *
 *   SHAEngine::Process_Partial -- Helper routine to process any partially accumulated data blo*
 *   SHAEngine::Process_Block -- Process a full data block into the hash accumulator.          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <stdlib.h>
//#include	<iostream.h>
#include "sha.h"
#include "endianness.h"
#include "memrev.h"
#include "rotates.h"
#include <algorithm>

/***********************************************************************************************
 * SHAEngine::Process_Partial -- Helper routine to process any partially accumulated data bloc *
 *                                                                                             *
 *    This routine will see if there is a partial block already accumulated in the holding     *
 *    buffer. If so, then the data is fetched from the source such that a full buffer is       *
 *    accumulated and then processed. If there is insufficient data to fill the buffer, then   *
 *    it accumulates what data it can and then returns so that this routine can be called      *
 *    again later.                                                                             *
 *                                                                                             *
 * INPUT:   data  -- Reference to a pointer to the data. This pointer will be modified if      *
 *                   this routine consumes any of the data in the buffer.                      *
 *                                                                                             *
 *          length-- Reference to the length of the data available. If this routine consumes   *
 *                   any of the data, then this length value will be modified.                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void SHAEngine::Process_Partial(void const*& data, int32_t& length)
{
    if (length == 0 || data == NULL)
        return;

    /*
    **	If there is no partial buffer and the source is greater than
    **	a source block size, then partial processing is unnecessary.
    **	Bail out in this case.
    */
    if (PartialCount == 0 && length >= SRC_BLOCK_SIZE)
        return;

    /*
    **	Attach as many bytes as possible from the source data into
    **	the staging buffer.
    */
    int add_count = std::min((int)length, (int)SRC_BLOCK_SIZE - PartialCount);
    memcpy(&Partial[PartialCount], data, add_count);
    data = ((char const*&)data) + add_count;
    PartialCount += add_count;
    length -= add_count;

    /*
    **	If a full staging buffer has been accumulated, then process
    **	the staging buffer and then bail.
    */
    if (PartialCount == SRC_BLOCK_SIZE) {
        Process_Block(&Partial[0], Acc);
        Length += (int32_t)SRC_BLOCK_SIZE;
        PartialCount = 0;
    }
}

/***********************************************************************************************
 * SHAEngine::Hash -- Process an arbitrarily long data block.                                  *
 *                                                                                             *
 *    This is the main access routine to the SHA engine. It will take the arbitrarily long     *
 *    data block and process it. The hash value is accumulated with any previous calls to      *
 *    this routine.                                                                            *
 *                                                                                             *
 * INPUT:   data     -- Pointer to the data block to process.                                  *
 *                                                                                             *
 *          length   -- The number of bytes to process.                                        *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void SHAEngine::Hash(void const* data, int32_t length)
{
    IsCached = false;

    /*
    **	Check for and handle any smaller-than-512bit blocks. This can
    **	result in all of the source data submitted to this routine to be
    **	consumed at this point.
    */
    Process_Partial(data, length);

    /*
    **	If there is no more source data to process, then bail. Speed reasons.
    */
    if (length == 0)
        return;

    /*
    **	First process all the whole blocks available in the source data.
    */
    int32_t blocks = (length / SRC_BLOCK_SIZE);
    int32_t const* source = (int32_t const*)data;
    for (int bcount = 0; bcount < blocks; bcount++) {
        Process_Block(source, Acc);
        Length += (int32_t)SRC_BLOCK_SIZE;
        source += SRC_BLOCK_SIZE / sizeof(int32_t);
        length -= (int32_t)SRC_BLOCK_SIZE;
    }

    /*
    **	Process any remainder bytes. This data is stored in the source
    **	accumulator buffer for future processing.
    */
    data = source;
    Process_Partial(data, length);
}

/***********************************************************************************************
 * SHAEngine::Result -- Fetch the current digest.                                              *
 *                                                                                             *
 *    This routine will return the digest as it currently stands.                              *
 *                                                                                             *
 * INPUT:   pointer  -- Pointer to the buffer that will hold the digest -- 20 bytes.           *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes copied into the buffer. This will always be       *
 *          20.                                                                                *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int SHAEngine::Result(void* result) const
{
    /*
    **	If the final hash result has already been calculated for the
    **	current data state, then immediately return with the precalculated
    **	value.
    */
    if (IsCached) {
        memcpy(result, &FinalResult, sizeof(FinalResult));
    }

    int32_t length = Length + PartialCount;
    int partialcount = PartialCount;
    char partial[SRC_BLOCK_SIZE];
    memcpy(partial, Partial, sizeof(Partial));

    /*
    **	Cap the end of the source data stream with a 1 bit.
    */
    partial[partialcount] = (char)0x80;

    /*
    **	Determine if there is insufficient room to append the
    **	data length number to the hash source. If not, then
    **	fill out the rest of the accumulator and flush it to
    **	the hash so that there will be room for the final
    **	count value.
    */
    SHADigest acc = Acc;
    if ((SRC_BLOCK_SIZE - partialcount) < 9) {
        if (partialcount + 1 < SRC_BLOCK_SIZE) {
            memset(&partial[partialcount + 1], '\0', SRC_BLOCK_SIZE - (partialcount + 1));
        }
        Process_Block(&partial[0], acc);
        partialcount = 0;
    } else {
        partialcount++;
    }

    /*
    **	Put the length of the source data as a 64 bit integer in the
    **	last 8 bytes of the pseudo-source data.
    */
    memset(&partial[partialcount], '\0', SRC_BLOCK_SIZE - partialcount);
    *(int32_t*)(&partial[SRC_BLOCK_SIZE - 4]) = htobe32((length * 8));
    Process_Block(&partial[0], acc);

    memcpy((char*)&FinalResult, &acc, sizeof(acc));
    for (unsigned int index : FinalResult.Long) {
        //	for (int index = 0; index < SRC_BLOCK_SIZE/sizeof(int32_t); index++) {
        (int32_t&)index = htobe32(index);
    }
    (bool&)IsCached = true;
    memcpy(result, &FinalResult, sizeof(FinalResult));
    return (sizeof(FinalResult));
}

/***********************************************************************************************
 * SHAEngine::Process_Block -- Process a full data block into the hash accumulator.            *
 *                                                                                             *
 *    This helper routine is called when a full block of data is available for processing      *
 *    into the hash.                                                                           *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the block of data to process.                               *
 *                                                                                             *
 *          acc      -- Reference to the hash accumulator that this hash step will be          *
 *                      accumulated into.                                                      *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void SHAEngine::Process_Block(void const* source, SHADigest& acc) const
{
    /*
    **	The hash is generated by performing operations on a
    **	block of generated/seeded data.
    */
    int32_t block[PROC_BLOCK_SIZE / sizeof(int32_t)];

    /*
    **	Expand the source data into a large 80 * 32bit buffer. This is the working
    **	data that will be transformed by the secure hash algorithm.
    */
    int32_t const* data = (int32_t const*)source;
    int index;
    for (index = 0; index < SRC_BLOCK_SIZE / sizeof(int32_t); index++) {
        block[index] = htobe32(data[index]);
    }

    for (index = SRC_BLOCK_SIZE / sizeof(int32_t); index < PROC_BLOCK_SIZE / sizeof(int32_t); index++) {
        //		block[index] = rotl32(block[(index-3)&15] ^ block[(index-8)&15] ^ block[(index-14)&15] ^
        //block[(index-16)&15], 1);
        block[index] = rotl32(block[index - 3] ^ block[index - 8] ^ block[index - 14] ^ block[index - 16], 1);
    }

    /*
    **	This is the core algorithm of the Secure Hash Algorithm. It is a block
    **	transformation of 512 bit source data with a 2560 bit intermediate buffer.
    */
    SHADigest alt = acc;
    for (index = 0; index < PROC_BLOCK_SIZE / sizeof(int32_t); index++) {
        int32_t temp = rotl32(alt.Long[0], 5) + Do_Function(index, alt.Long[1], alt.Long[2], alt.Long[3]) + alt.Long[4]
                       + block[index] + Get_Constant(index);
        alt.Long[4] = alt.Long[3];
        alt.Long[3] = alt.Long[2];
        alt.Long[2] = rotl32(alt.Long[1], 30);
        alt.Long[1] = alt.Long[0];
        alt.Long[0] = temp;
    }
    acc.Long[0] += alt.Long[0];
    acc.Long[1] += alt.Long[1];
    acc.Long[2] += alt.Long[2];
    acc.Long[3] += alt.Long[3];
    acc.Long[4] += alt.Long[4];
}
