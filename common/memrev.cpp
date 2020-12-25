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
#include "memrev.h"

/***********************************************************************************************
 * memrev -- Reverse the byte order of the buffer specified.                                   *
 *                                                                                             *
 *    This routine will reverse the byte order in the buffer specified.                        *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer that will be reversed.                           *
 *                                                                                             *
 *          length   -- The length of the buffer.                                              *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void memrev(char* buffer, size_t length)
{
    char* r2 = &(buffer[length - 1]);
    while (buffer < r2) {
        char b = *buffer;
        *buffer++ = *r2;
        *r2-- = b;
    }
}
