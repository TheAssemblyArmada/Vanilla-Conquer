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
 *                 Project Name : Westwood Library                         *
 *                                                                         *
 *                    File Name : ALLOC.CPP                                *
 *                                                                         *
 *                   Programmer : Joe L. Bostic                            *
 *                                                                         *
 *                   Start Date : February 1, 1992                         *
 *                                                                         *
 *                  Last Update : March 9, 1995 [JLB]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Alloc -- Allocates system RAM.                                        *
 *   Ram_Free -- Determines the largest free chunk of RAM.                 *
 *   Free -- Free an Alloc'ed block of RAM.                                *
 *   Resize_Alloc -- Change the size of an allocated block.                *
 *   Heap_Size -- Size of the heap we have.                                *
 *   Total_Ram_Free -- Total amount of free RAM.                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <string.h>
#include <stdlib.h>

#include "wwmem.h"

size_t Largest_Mem_Block(void);

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

size_t MinRam = 0; // Record of least memory at worst case.
size_t MaxRam = 0; // Record of total allocated at worst case.
static size_t TotalRam = 0;
static unsigned int Memory_Calls = 0;

void (*Memory_Error)(void) = NULL;
void (*Memory_Error_Exit)(char* string) = NULL;

/***************************************************************************
 * Alloc -- Allocates system RAM.                                          *
 *                                                                         *
 *    This is the basic RAM allocation function.  It is used for all       *
 *    memory allocations needed by the system or the main program.         *
 *                                                                         *
 * INPUT:   bytes_to_alloc -- LONG value of the number of bytes to alloc.  *
 *                                                                         *
 *          flags          -- Memory allocation control flags.             *
 *             MEM_NORMAL: No special flags.                               *
 *             MEM_CLEAR:  Zero out memory block.                        	*
 *             MEM_NEW:		Called by a new.                                *
 *                                                                         *
 * OUTPUT:  Returns with pointer to allocated block.  If NULL was returned *
 *          it indicates a failure to allocate.  Note: NULL will never be  *
 *          returned if the standard library allocation error routine is   *
 *          used.                                                          *
 *                                                                         *
 * WARNINGS:   If you replace the standard memory allocation error routine *
 *             and make it so that Alloc CAN return with a NULL, be sure   *
 *             and check for this in your code.                            *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/03/1991 JLB : Documented.                                          *
 *   08/09/1993 JLB : Updated with EMS memory support.                     *
 *   04/28/1994 JAW : Updated to 32bit Protected mode.                     *
 *   03/09/1995 JLB : Fixed                                                *
 *   09/28/1995 ST  : Simplified for win95                                                                      *
 *=========================================================================*/
void* Alloc(size_t bytes_to_alloc, MemoryFlagType flags)
{
    void* mem_ptr;

#ifdef MEM_CHECK
    bytes_to_alloc += sizeof(uintptr_t) * 8;
#endif // MEM_CHECK

    mem_ptr = malloc(bytes_to_alloc);

    if (!mem_ptr && Memory_Error) {
        Memory_Error();
    }

    if (mem_ptr && (flags & MEM_CLEAR)) {
        memset(mem_ptr, 0, bytes_to_alloc);
    }

#ifdef MEM_CHECK
    static_assert(sizeof(uintptr_t) >= sizeof(size_t), "size type mismatch");
    mem_ptr = (void*)((char*)mem_ptr + sizeof(uintptr_t) * 4);
    uintptr_t* magic_ptr = (uintptr_t*)(((char*)mem_ptr) - sizeof(uintptr_t) * 4);
    *magic_ptr++ = (uintptr_t)mem_ptr;
    *magic_ptr++ = (uintptr_t)mem_ptr;
    *magic_ptr++ = (uintptr_t)mem_ptr;
    *magic_ptr = bytes_to_alloc - sizeof(uintptr_t) * 8;
    magic_ptr = (uintptr_t*)(((char*)mem_ptr) + bytes_to_alloc - sizeof(uintptr_t) * 8);
    *magic_ptr++ = (uintptr_t)mem_ptr;
    *magic_ptr++ = (uintptr_t)mem_ptr;
    *magic_ptr++ = (uintptr_t)mem_ptr;
    *magic_ptr = (uintptr_t)mem_ptr;
#endif // MEM_CHECK

    Memory_Calls++;
    return (mem_ptr);
}

/***************************************************************************
 * Free -- Free an Alloc'ed block of RAM.                                  *
 *                                                                         *
 * FUNCTION:                                                               *
 *                                                                         *
 * INPUT:       A pointer to a block of RAM from Alloc.                    *
 *                                                                         *
 * OUTPUT:      None.                                                      *
 *                                                                         *
 * WARNINGS:    Don't use this for an Alloc_Block'ed RAM block.            *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/25/1990     : Created.                                             *
 ***************************************************************************/

void Free(void const* pointer)
{

    if (pointer) {

#ifdef MEM_CHECK

        uintptr_t* magic_ptr = (uintptr_t*)(((char*)pointer) - sizeof(uintptr_t) * 4);

        if (*magic_ptr++ != (uintptr_t)pointer || *magic_ptr++ != (uintptr_t)pointer
            || *magic_ptr++ != (uintptr_t)pointer) {
            Int3();
        }

        magic_ptr = (uintptr_t*)(((char*)pointer) + *magic_ptr);

        if (*magic_ptr++ != (uintptr_t)pointer || *magic_ptr++ != (uintptr_t)pointer
            || *magic_ptr++ != (uintptr_t)pointer || *magic_ptr++ != (uintptr_t)pointer) {
            Int3();
        }

        pointer = (void*)(((char*)pointer) - 16);
#endif // MEM_CHECK

        free((void*)pointer);
        Memory_Calls--;
    }
}

/***************************************************************************
 * Resize_Alloc -- Change the size of an allocated block.                  *
 *                                                                         *
 *    This routine will take a previously allocated block and change its   *
 *    size without unnecessarily altering its contents.                    *
 *                                                                         *
 * INPUT:   pointer  -- Pointer to the original memory allocation.         *
 *                                                                         *
 *          new_size -- Size in bytes that it will be converted to.        *
 *                                                                         *
 * OUTPUT:  Returns with a pointer to the new allocation.                  *
 *                                                                         *
 * WARNINGS:   ???                                                         *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/01/1992 JLB : Commented.                                           *
 *=========================================================================*/
void* Resize_Alloc(void* original_ptr, size_t new_size_in_bytes)
{

    uintptr_t* temp;

    temp = (uintptr_t*)original_ptr;

    /* ReAlloc the space */
    temp = (uintptr_t*)realloc(temp, new_size_in_bytes);
    if (temp == NULL) {
        if (Memory_Error != NULL)
            Memory_Error();
        return NULL;
    }

    return (temp);
}

/***************************************************************************
 * Ram_Free -- Determines the largest free chunk of RAM.                   *
 *                                                                         *
 *    Use this routine to determine the largest free chunk of available    *
 *    RAM for allocation.  It also performs a check of the memory chain.   *
 *                                                                         *
 * INPUT:   none                                                           *
 *                                                                         *
 * OUTPUT:  Returns with the size of the largest free chunk of RAM.        *
 *                                                                         *
 * WARNINGS:   This does not return the TOTAL memory free, only the        *
 *             largest free chunk.                                         *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/03/1991 JLB : Commented.                                           *
 *=========================================================================*/
int Ram_Free(MemoryFlagType)
{
    return (64 * 1024 * 1024);
}

/***************************************************************************
 * Heap_Size -- Size of the heap we have.                                  *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/21/1994 SKB : Created.                                             *
 *=========================================================================*/
int Heap_Size(MemoryFlagType)
{
    if (!TotalRam) {
        TotalRam = Total_Ram_Free(MEM_NORMAL);
    }
    return (TotalRam);
}

/***************************************************************************
 * Total_Ram_Free -- Total amount of free RAM.                             *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/21/1994 SKB : Created.                                             *
 *   03/09/1995 JLB : Uses prerecorded heap size maximum.                  *
 *=========================================================================*/
int Total_Ram_Free(MemoryFlagType)
{
    return (64 * 1024 * 1024);
}
