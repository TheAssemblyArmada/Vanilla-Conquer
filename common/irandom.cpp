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
 *                 Project Name : LIBRARY                                  *
 *                                                                         *
 *                    File Name : IRANDOM.C                                *
 *                                                                         *
 *                   Programmer : Barry W. Green                           *
 *                                                                         *
 *                  Last Update : 10 Feb, 1995     [BWG]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <stdlib.h>
#include <time.h>
#include "irandom.h"

unsigned long RandNumb = 0x12349876;

static bool bsr(unsigned int* index, unsigned mask)
{
    if (mask == 0 || index == 0)
        return false;

    int i = 0;
    for (i = 31; i >= 0; --i) {
        unsigned tmp = 1 << i;

        if ((mask & tmp) != 0) {
            *index = i;
            break;
        }
    }

    return i >= 0;
}

unsigned char Random()
{
    /*
    ** This treats the number as bytes so setting it on bit endian machines needs to byte swap.
    */
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&RandNumb);
    unsigned char cf1 = (bytes[0] >> 1) & 1;
    unsigned char tmp_a = bytes[0] >> 2;
    unsigned char cf2 = (bytes[2] >> 7) & 1;
    bytes[2] = (bytes[2] << 1) | cf1;
    cf1 = (bytes[1] >> 7) & 1;
    bytes[1] = (bytes[1] << 1) | cf2;
    cf2 = (tmp_a - (RandNumb + (cf1 != 1))) & 1;
    bytes[0] = (bytes[0] >> 1) | (cf2 << 7);

    return bytes[1] ^ bytes[0];
}

int Get_Random_Mask(int maxval)
{
    unsigned int index;
    unsigned int mask = 1;

    if (bsr(&index, maxval)) {
        mask = (1 << (index + 1)) - 1;
    }

    return mask;
}

/* IRANDOM ----------------------------------------------------------

   IRandom returns a random value between min and max inclusive.

   INPUTS:	int min and int max

   RETURNS:	int random number
*/

int IRandom(int minval, int maxval)
{
    int num, mask;

    // Keep minval and maxval straight.
    if (minval > maxval) {
        minval ^= maxval;
        maxval ^= minval;
        minval ^= maxval;
    }

    mask = Get_Random_Mask(maxval - minval);

    while ((num = (rand() & mask) + minval) > maxval)
        ;
    return (num);
}
