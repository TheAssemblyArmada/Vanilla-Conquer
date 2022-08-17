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

/* $Header: g:/library/wwlib32/file/rcs/load.cpp 1.4 1994/04/22 12:42:21 scott_bowen Exp $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : LIBRARY                                  *
 *                                                                         *
 *                    File Name : LOAD.C                                   *
 *                                                                         *
 *                   Programmer : Christopher Yates                        *
 *                                                                         *
 *                  Last Update : September 17, 1993   [JLB]               *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Load_Uncompress -- Load and uncompress the given file.                *
 *   Uncompress_Data -- Uncompress standard CPS buffer.                    *
 *   Load_Data -- Loads a data file from disk.                             *
 *   Load_Alloc_Data -- Loads and allocates buffer for a file.             *
 *   Write_Data -- Writes a block of data as a file to disk.               *
 *   Uncompress_Data -- Uncompresses data from one buffer to another.      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "iff.h"
#include "lcw.h"
#include "file.h"
#include "misc.h"
#include "wwstd.h"
#include "wwmem.h"

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/***************************************************************************
 * LOAD_DATA -- Loads a data file from disk.                               *
 *                                                                         *
 *    This routine will load a data file from disk.  It does no translation*
 *    on the data.                                                         *
 *                                                                         *
 * INPUT:   name  -- Pointer to ASCII filename of the data file.           *
 *                                                                         *
 *          ptr   -- Buffer to load the data file into.                    *
 *                                                                         *
 *          size  -- Maximum size of the buffer (in bytes).                *
 *                                                                         *
 * OUTPUT:  Returns with the number of bytes read.                         *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/24/1991 JLB : Created.                                             *
 *=========================================================================*/
unsigned int Load_Data(char const* name, void* ptr, unsigned int size)
{
    int fd;

    fd = Open_File(name, READ);
    size = Read_File(fd, ptr, size);
    Close_File(fd);
    return (size);
}

/***************************************************************************
 * WRITE_DATA -- Writes a block of data as a file to disk.                 *
 *                                                                         *
 *    This routine will write a block of data as a file to the disk.  It   *
 *    is the compliment of Load_Data.                                      *
 *                                                                         *
 * INPUT:   name     -- Name of the file to create.                        *
 *                                                                         *
 *          ptr      -- Pointer to the block of data to write.             *
 *                                                                         *
 *          size     -- Size of the data block to be written.              *
 *                                                                         *
 * OUTPUT:  Returns with the number of bytes actually written.             *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1992 JLB : Created.                                             *
 *=========================================================================*/
unsigned int Write_Data(char const* name, void* ptr, unsigned int size)
{
    int fd;

    fd = Open_File(name, WRITE);
    size = Write_File(fd, ptr, size);
    Close_File(fd);
    return (size);
}

/***************************************************************************
 * LOAD_ALLOC_DATA -- Loads and allocates buffer for a file.               *
 *                                                                         *
 *    The routine will allocate a buffer and load the specified file into  *
 *    it.  The kind of memory used for the buffer is determined by the     *
 *    memory allocation flags passed in.                                   *
 *                                                                         *
 * INPUT:   name  -- Name of the file to load.                             *
 *                                                                         *
 *          flags -- Memory allocation flags to use when allocating.       *
 *                                                                         *
 * OUTPUT:  Returns with a pointer to the buffer that contains the file's  *
 *          data.                                                          *
 *                                                                         *
 * WARNINGS:   A memory error could occur if regular memory flags are      *
 *             specified.  If XMS memory is specified, then this routine   *
 *             could likely return nullptr.                                   *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/28/1992 JLB : Created.                                             *
 *=========================================================================*/
void* Load_Alloc_Data(char const* name, MemoryFlagType flags)
{
    int fd;            // Working file handle.
    unsigned int size; // Size of the file to load.
    void* buffer;      // Buffer to hold the file.

    fd = Open_File(name, READ);
    size = File_Size(fd);
    buffer = Alloc(size, flags);
    if (buffer) {
        Read_File(fd, buffer, size);
    }
    Close_File(fd);
    return (buffer);
}

/***********************************************************************************************
 * Load_Alloc_Data -- Allocates a buffer and loads the file into it.                           *
 *                                                                                             *
 *    This is the C++ replacement for the Load_Alloc_Data function. It will allocate the       *
 *    memory big enough to hold the file and then read the file into it.                       *
 *                                                                                             *
 * INPUT:   file  -- The file to read.                                                         *
 *                                                                                             *
 *          mem   -- The memory system to use for allocation.                                  *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated and filled memory block.                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void* Load_Alloc_Data(const FileClass& file)
{
    void* ptr = 0;
    int size = const_cast<FileClass&>(file).Size();

    ptr = malloc(size);
    if (ptr) {
        const_cast<FileClass&>(file).Read(ptr, size);
    }
    return (ptr);
}

/***********************************************************************************************
 * Load_Alloc_Data -- Allocates a buffer and loads the file into it.                           *
 *                                                                                             *
 *    This is the C++ replacement for the Load_Alloc_Data function. It will allocate the       *
 *    memory big enough to hold the file and then read the file into it.                       *
 *                                                                                             *
 * INPUT:   file  -- The file to read.                                                         *
 *                                                                                             *
 *          mem   -- The memory system to use for allocation.                                  *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated and filled memory block.                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void* Load_Alloc_Data(const FileClass* file)
{
    return Load_Alloc_Data(*file);
}

/***************************************************************************
 * LOAD_UNCOMPRESS -- Load and uncompress the given file.                  *
 *                                                                         *
 * INPUT:      char *					- file name to uncompress 					*
 *					GraphicBufferClass&	- to load the source data into			*
 *					GraphicBufferClass&	- for the picture								*
 *             void *					- ptr for header uncompressed data     *
 *                                                                         *
 * OUTPUT:     unsigned int size of uncompressed data                             *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/28/1991  CY : Created.                                             *
 *   06/26/1991 JLB : Handles load & uncompress to same buffer.            *
 *=========================================================================*/
unsigned int Load_Uncompress(char const* file, BufferClass& uncomp_buff, BufferClass& dest_buff, void* reserved_data)
{
    int fd;                 // Source file handle.
    unsigned int isize = 0; // Size of the file.
    unsigned int skipsize;  // Size of the skip data bytes.
    void* uncomp_ptr;       //	Source buffer pointer.
    char* newuncomp_ptr;    // Adjusted source pointer.

    uncomp_ptr = uncomp_buff.Get_Buffer(); // get a pointer to buffer

    /*======================================================================*/
    /* Read the file into the uncompression buffer.									*/
    /*======================================================================*/

    fd = Open_File(file, READ);       // Open up the file to read from
    Read_File(fd, (char*)&isize, 2L); // Read the file size
    isize = le32toh(isize);           // Fix size on Big Endian machines.
    Read_File(fd, uncomp_ptr, 8L);    // Read the header bytes in.
    isize -= 8;                       // Remaining data in file.

    /*======================================================================*/
    /* Check for and read in the skip data block.									*/
    /*======================================================================*/

    skipsize = le16toh(*(((short*)uncomp_ptr) + 3));

    if (reserved_data && skipsize) {
        Read_File(fd, reserved_data, (unsigned int)skipsize);
    } else {
        Seek_File(fd, skipsize, SEEK_CUR);
    }

    *(((short*)uncomp_ptr + 3)) = 0; // K/O any skip value.
    isize -= skipsize;

    /*======================================================================*/
    /*	If the source and dest buffer are the same, we adjust the pointer so */
    /* that the compressed data is loaded into the end of the buffer.  In 	*/
    /* this way the uncompress code can write to the same buffer.				*/
    /*======================================================================*/
    newuncomp_ptr = (char*)Add_Long_To_Pointer(uncomp_buff.Get_Buffer(), uncomp_buff.Get_Size() - (isize + 8L));

    /*======================================================================*/
    /*	Duplicate the header bytes.														*/
    /*======================================================================*/
    Mem_Copy(uncomp_ptr, newuncomp_ptr, 8);

    /*======================================================================*/
    /*	Read in the main compressed part of the file.								*/
    /*======================================================================*/
    Read_File(fd, newuncomp_ptr + 8, (unsigned int)isize);
    Close_File(fd);

    /*======================================================================*/
    /* Uncompress the file into the destination buffer (which may very well	*/
    /*		be the source buffer).															*/
    /*======================================================================*/
    return (Uncompress_Data(newuncomp_ptr, dest_buff.Get_Buffer()));
}

/***************************************************************************
 * Uncompress_Data -- Uncompresses data from one buffer to another.        *
 *                                                                         *
 *    This routine takes data from a compressed file (sans the first two   *
 *    size bytes) and uncompresses it to a destination buffer.  The source *
 *    data MUST have the CompHeaderType at its start.                      *
 *                                                                         *
 * INPUT:   src   -- Source compressed data pointer.                       *
 *                                                                         *
 *          dst   -- Destination (paragraph aligned) pointer.              *
 *                                                                         *
 * OUTPUT:  Returns with the size of the uncompressed data.                *
 *                                                                         *
 * WARNINGS:   If LCW compression is used, the destination buffer must     *
 *             be paragraph aligned.                                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/17/1993 JLB : Created.                                             *
 *=========================================================================*/
unsigned int Uncompress_Data(void const* src, void* dst)
{
    unsigned int skip;      // Number of leading data to skip.
    CompressionType method; // Compression method used.
    unsigned int uncomp_size = 0;

    if (!src || !dst)
        return 0;

    /*
    **	Interpret the data block header structure to determine
    **	compression method, size, and skip data amount.
    */
    uncomp_size = ((CompHeaderType*)src)->Size;
#if (__BIG_ENDIAN__)
    uncomp_size = bswap32(uncomp_size);
#endif
    skip = ((CompHeaderType*)src)->Skip;
#if (__BIG_ENDIAN__)
    skip = bswap16(skip);
#endif
    method = (CompressionType)((CompHeaderType*)src)->Method;
    src = Add_Long_To_Pointer((void*)src, (int)sizeof(CompHeaderType) + (int)skip);

    switch (method) {

    default:
    case NOCOMPRESS:
        Mem_Copy((void*)src, dst, uncomp_size);
        break;

    case HORIZONTAL:
#if LIB_EXTERNS_RESOLVED
        RLE_Uncompress((void*)src, dst, uncomp_size);
#endif
        break;

    case LCW:
        LCW_Uncompress((void*)src, (void*)dst, (unsigned int)uncomp_size);
        break;
    }

    return (uncomp_size);
}
