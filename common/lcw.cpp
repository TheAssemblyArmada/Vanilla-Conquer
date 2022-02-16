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

/* $Header: /CounterStrike/LCW.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **    C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *               Project Name : WESTWOOD LIBRARY (PSX)                     *
 *                                                                         *
 *                 File Name : LCW.CPP                                *
 *                                                                         *
 *                Programmer : Ian M. Leslie                               *
 *                                                                         *
 *                Start Date : May 17, 1995                                *
 *                                                                         *
 *               Last Update : May 17, 1995    [IML]                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "lcw.h"
#include <string.h>

/***************************************************************************
 * LCW_Uncompress -- Decompress an LCW encoded data block.                 *
 *                                                                         *
 * Uncompress data to the following codes in the format b = byte, w = word *
 * n = byte code pulled from compressed data.                              *
 *                                                                         *
 *   Command code, n        |Description                                   *
 * ------------------------------------------------------------------------*
 * n=0xxxyyyy,yyyyyyyy      |short copy back y bytes and run x+3 from dest *
 * n=10xxxxxx,n1,n2,...,nx+1|med length copy the next x+1 bytes from source*
 * n=11xxxxxx,w1            |med copy from dest x+3 bytes from offset w1   *
 * n=11111111,w1,w2         |long copy from dest w1 bytes from offset w2   *
 * n=11111110,w1,b1         |long run of byte b1 for w1 bytes              *
 * n=10000000               |end of data reached                           *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      void * source ptr                                                  *
 *      void * destination ptr                                             *
 *      unsigned int length of uncompressed data                          *
 *                                                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *     unsigned int # of destination bytes written                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *     3rd argument is dummy. It exists to provide cross-platform          *
 *      compatibility. Note therefore that this implementation does not    *
 *      check for corrupt source data by testing the uncompressed length.  *
 *                                                                         *
 * HISTORY:                                                                *
 *    03/20/1995 IML : Created.                                            *
 *=========================================================================*/
int LCW_Uncompress(void const* source, void* dest, unsigned length)
{
    unsigned char *source_ptr, *dest_ptr, *copy_ptr, *dest_end, op_code;
    unsigned count;

    /* Copy the source and destination ptrs. */
    source_ptr = (unsigned char*)source;
    dest_ptr = (unsigned char*)dest;
    dest_end = dest_ptr + length;

    while (dest_ptr < dest_end) {

        /* Read in the operation code. */
        op_code = *source_ptr++;

        if (!(op_code & 0x80)) {

            /* Do a short copy from destination. */
            count = (op_code >> 4) + 3;
            copy_ptr = dest_ptr - ((unsigned)*source_ptr++ + (((unsigned)op_code & 0x0f) << 8));

            /* Check we aren't going to write past the end of the destination buffer */
            if (count > (unsigned)(dest_end - dest_ptr)) {
                count = dest_end - dest_ptr;
            }

            while (count--)
                *dest_ptr++ = *copy_ptr++;

        } else {

            if (!(op_code & 0x40)) {

                if (op_code == 0x80) {

                    /* Return # of destination bytes written. */
                    return (int)(dest_ptr - (unsigned char*)dest);

                } else {

                    /* Do a medium copy from source. */
                    count = op_code & 0x3f;

                    /* Check we aren't going to write past the end of the destination buffer */
                    if (count > (unsigned)(dest_end - dest_ptr)) {
                        count = dest_end - dest_ptr;
                    }

                    while (count--)
                        *dest_ptr++ = *source_ptr++;
                }

            } else {

                if (op_code == 0xfe) {

                    /* Do a long run. */
                    count = *source_ptr++;
                    count += (*source_ptr++) << 8;

                    if (count > (unsigned)(dest_end - dest_ptr)) {
                        count = dest_end - dest_ptr;
                    }

                    memset(dest_ptr, (*source_ptr++), count);
                    dest_ptr += count;

                } else {

                    if (op_code == 0xff) {

                        /* Do a long copy from destination. */
                        count = *source_ptr++;
                        count += (*source_ptr++) << 8;
                        copy_ptr = (unsigned char*)dest + *source_ptr++;
                        copy_ptr += (*source_ptr++) << 8;

                        if (count > (unsigned)(dest_end - dest_ptr)) {
                            count = dest_end - dest_ptr;
                        }

                        while (count--)
                            *dest_ptr++ = *copy_ptr++;

                    } else {

                        /* Do a medium copy from destination. */
                        count = (op_code & 0x3f) + 3;
                        copy_ptr = (unsigned char*)dest + *source_ptr + ((unsigned)*(source_ptr + 1) << 8);
                        source_ptr += 2;

                        if (count > (unsigned)(dest_end - dest_ptr)) {
                            count = dest_end - dest_ptr;
                        }

                        while (count--)
                            *dest_ptr++ = *copy_ptr++;
                    }
                }
            }
        }
    }

    return (int)(dest_ptr - (unsigned char*)dest);
}

int LCW_Comp(const void* src, void* dst, unsigned int bytes)
{
    if (!bytes) {
        return 0;
    }

    const unsigned char* getp = (const unsigned char*)(src);
    unsigned char* putp = (unsigned char*)(dst);
    const unsigned char* getstart = getp;
    const unsigned char* getend = getp + bytes;
    unsigned char* putstart = putp;
    bool cmd_one;
    // Write a starting cmd1 and set bool to have cmd1 in progress
    unsigned char* cmd_onep = putp;
    *putp++ = 0x81;
    *putp++ = *getp++;
    cmd_one = true;

    // Compress data
    while (getp < getend) {
        // Is RLE encode (4bytes) worth evaluating?
        if (getend - getp > 64 && *getp == *(getp + 64)) {
            // RLE run length is encoded as a short so max is UINT16_MAX
            const unsigned char* rlemax = (getend - getp) < 0xFFFF ? getend : getp + 0xFFFF;
            const unsigned char* rlep;

            for (rlep = getp + 1; *rlep == *getp && rlep < rlemax; ++rlep)
                ;

            unsigned short run_length = rlep - getp;

            // If run length is long enough, write the command and start loop again
            if (run_length >= 0x41) {
                cmd_one = false;
                *putp++ = 0xFE;
                *putp++ = (unsigned char)run_length;
                *putp++ = run_length >> 8;
                *putp++ = *getp;
                getp = rlep;
                continue;
            }
        }

        // current block size for an offset copy
        int block_size = 0;
        const unsigned char* offstart;

        // Set where we start looking for matching runs.
        offstart = getstart;

        // Look for matching runs
        const unsigned char* offchk = offstart;
        const unsigned char* offsetp = getp;
        while (offchk < getp) {
            // Move offchk to next matching position
            while (offchk < getp && *offchk != *getp) {
                ++offchk;
            }

            // If the checking pointer has reached current pos, break
            if (offchk >= getp) {
                break;
            }

            // find out how long the run of matches goes for
            //<= because it can consider the current pixel as part of a run
            int i;
            for (i = 1; &getp[i] < getend; ++i) {
                if (offchk[i] != getp[i]) {
                    break;
                }
            }

            if (i >= block_size) {
                block_size = i;
                offsetp = offchk;
            }

            ++offchk;
        }

        // decide what encoding to use for current run
        if (block_size <= 2) {
            // short copy 0b10??????
            // check we have an existing 1 byte command and if its value is still
            // small enough to handle additional bytes
            // start a new command if current one doesn't have space or we don't
            // have one to continue
            if (cmd_one && *cmd_onep < 0xBF) {
                // increment command value
                ++*cmd_onep;
                *putp++ = *getp++;
            } else {
                cmd_onep = putp;
                *putp++ = 0x81;
                *putp++ = *getp++;
                cmd_one = true;
            }
        } else {
            unsigned short offset;
            unsigned short rel_offset = getp - offsetp;
            if (block_size > 0xA || (rel_offset > 0xFFF)) {
                // write 5 byte command 0b11111111
                if (block_size > 0x40) {
                    *putp++ = 0xFF;
                    *putp++ = block_size;
                    *putp++ = block_size >> 8;
                    // write 3 byte command 0b11??????
                } else {
                    *putp++ = (block_size - 3) | 0xC0;
                }

                offset = offsetp - getstart;
                // write 2 byte command? 0b0???????
            } else {
                offset = rel_offset << 8 | (16 * (block_size - 3) + (rel_offset >> 8));
            }
            *putp++ = (unsigned char)offset;
            *putp++ = offset >> 8;
            getp += block_size;
            cmd_one = false;
        }
    }

    // write final 0x80, this is why its also known as format80 compression
    *putp++ = 0x80;
    return putp - putstart;
}
