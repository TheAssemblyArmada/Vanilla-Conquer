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

/* $Header: /CounterStrike/MIXFILE.H 1     3/03/97 10:25a Joe_bostic $ */

#ifndef MIXFILE_H
#define MIXFILE_H

#include <stdlib.h>
#include "listnode.h"
#include "pk.h"
#include "buff.h"
#include "crc.h"
#include "xstraw.h"
#include "pkstraw.h"
#include "shastraw.h"
#include "rndstraw.h"

#ifndef _WIN32
#include <libgen.h> // For basename()
#endif

void Prog_End(const char*, bool);
bool Force_CD_Available(int);
void Emergency_Exit(int);
extern int RequiredCD;
extern bool RunningAsDLL;

template <class T> class MixFileClass : public Node<MixFileClass<T>>
{
public:
    char const* Filename; // Filename of mixfile.

    MixFileClass(char const* filename, PKey const* key);
    ~MixFileClass(void);

    static bool Free(char const* filename);
    void Free(void);
    static void Free_All(void); // ST - 12/18/2019 11:35AM
    bool Cache(Buffer const* buffer = NULL);
    static bool Cache(char const* filename, Buffer const* buffer = NULL);
    static bool
    Offset(char const* filename, void** realptr = 0, MixFileClass** mixfile = 0, long* offset = 0, long* size = 0);
    static void const* Retrieve(char const* filename);

    struct SubBlock
    {
        long CRC;    // CRC code for embedded file.
        long Offset; // Offset from start of data section.
        long Size;   // Size of data subfile.

        int operator<(SubBlock& two) const
        {
            return (CRC < two.CRC);
        };
        int operator>(SubBlock& two) const
        {
            return (CRC > two.CRC);
        };
        int operator==(SubBlock& two) const
        {
            return (CRC == two.CRC);
        };
    };

private:
    static MixFileClass* Finder(char const* filename);
    // long Offset(long crc, long * size = 0) const;	// ST - 5/10/2019

    /*
    **	If this mixfile has an attached message digest, then this flag
    **	will be true. The digest is checked only when the mixfile is
    **	cached.
    */
    unsigned IsDigest : 1;

    /*
    **	If the header to this mixfile has been encrypted, then this flag
    **	will be true. Although the header of the mixfile may be encrypted,
    **	the attached data files are not.
    */
    unsigned IsEncrypted : 1;

    /*
    **	If the cached memory block was allocated by this routine, then this
    **	flag will be true.
    */
    unsigned IsAllocated : 1;

    /*
    **	This is the initial file header. It tells how many files are embedded
    **	within this mixfile and the total size of all embedded files.
    */
    typedef struct
    {
        short count;
        long size;
    } FileHeader;

    /*
    **	The number of files within the mixfile.
    */
    int Count;

    /*
    **	This is the total size of all the data file embedded within the mixfile.
    **	It does not include the header or digest bytes.
    */
    long DataSize;

    /*
    **	Start of raw data in within the mixfile.
    */
    long DataStart;

    /*
    **	Points to the file header control block array. Each file in the mixfile will
    **	have an entry in this table. The entries are sorted by their (signed) CRC value.
    */
    SubBlock* HeaderBuffer;

    /*
    **	If the mixfile has been cached, then this points to the cached data.
    */
    void* Data; // Pointer to raw data.

    static List<MixFileClass> List;
};

/*
**	This is the pointer to the first mixfile in the list of mixfiles registered
**	with the mixfile system.
*/
template <class T> List<MixFileClass<T>> MixFileClass<T>::List;

/***********************************************************************************************
 * MixFileClass::Free -- Uncaches a cached mixfile.                                            *
 *                                                                                             *
 *    Use this routine to uncache a mixfile that has been cached.                              *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename of the mixfile that is to be uncached.         *
 *                                                                                             *
 * OUTPUT:  bool; Was the mixfile found and freed?                                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/23/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
template <class T> bool MixFileClass<T>::Free(char const* filename)
{
    MixFileClass* ptr = Finder(filename);

    if (ptr) {
        ptr->Free();
        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * MixFileClass::~MixFileClass -- Destructor for the mixfile object.                           *
 *                                                                                             *
 *    This destructor will free all memory allocated by this mixfile and will remove it from   *
 *    the system. A mixfile removed in this fashion must be created anew in order to be        *
 *    subsequent used.                                                                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1994 JLB : Created.                                                                 *
 *   01/06/1995 JLB : Puts mixfile header table into EMS.                                      *
 *=============================================================================================*/
template <class T> MixFileClass<T>::~MixFileClass(void)
{
    /*
    **	Deallocate any allocated memory.
    */
    if (Filename) {
        free((char*)Filename);
    }
    if (Data != NULL && IsAllocated) {
        delete[] Data;
        IsAllocated = false;
    }
    Data = NULL;

    if (HeaderBuffer != NULL) {
        delete[] HeaderBuffer;
        HeaderBuffer = NULL;
    }

    /*
    **	Unlink this mixfile object from the chain.
    */
    Unlink();
}

/***********************************************************************************************
 * MixFileClass::MixFileClass -- Constructor for mixfile object.                               *
 *                                                                                             *
 *    This is the constructor for the mixfile object. It takes a filename and a memory         *
 *    handler object and registers the mixfile object with the system. The index block is      *
 *    allocated and loaded from disk by this routine.                                          *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename of the mixfile object.                         *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1994 JLB : Created.                                                                 *
 *   07/12/1996 JLB : Handles compressed file header.                                          *
 *=============================================================================================*/
template <class T>
MixFileClass<T>::MixFileClass(char const* filename, PKey const* key)
    : IsDigest(false)
    , IsEncrypted(false)
    , IsAllocated(false)
    , Filename(0)
    , Count(0)
    , DataSize(0)
    , DataStart(0)
    , HeaderBuffer(0)
    , Data(0)
{
    if (filename == NULL)
        return; // ST - 5/9/2019

    /*
    **	Check to see if the file is available. If it isn't, then
    **	no further processing is needed or possible.
    */
    if (!Force_CD_Available(RequiredCD)) {
        Prog_End("MixFileClass Force_CD_Available failed", true);
        if (!RunningAsDLL) { // PG
            Emergency_Exit(EXIT_FAILURE);
        }
    }

    T file(filename); // Working file object.
    Filename = strdup(file.File_Name());
    FileStraw fstraw(file);
    // Removes dependency on redalert global CryptRandom as it is only used to make mix files, not decrypt them.
    RandomStraw fakernd;
    PKStraw pstraw(PKStraw::DECRYPT, fakernd);
    Straw* straw = &fstraw;

    if (!file.Is_Available())
        return;

    /*
    **	Stuctures used to hold the various file headers.
    */
    FileHeader fileheader;
    struct
    {
        short First;  // Always zero for extended mixfile format.
        short Second; // Bitfield of extensions to this mixfile.
    } alternate;

    /*
    **	Fetch the first bit of the file. From this bit, it is possible to detect
    **	whether this is an extended mixfile format or the plain format. An
    **	extended format may have extra options or data layout.
    */
    int got = straw->Get(&alternate, sizeof(alternate));

    /*
    **	Detect if this is an extended mixfile. If so, then see if it is encrypted
    **	and/or has a message digest attached. Otherwise, just retrieve the
    **	plain mixfile header.
    */
    if (alternate.First == 0) {
        IsDigest = ((alternate.Second & 0x01) != 0);
        IsEncrypted = ((alternate.Second & 0x02) != 0);

        if (IsEncrypted) {
            pstraw.Key(key);
            pstraw.Get_From(&fstraw);
            straw = &pstraw;
        }
        straw->Get(&fileheader, sizeof(fileheader));

    } else {
        memmove(&fileheader, &alternate, sizeof(alternate));
        straw->Get(((char*)&fileheader) + sizeof(alternate), sizeof(fileheader) - sizeof(alternate));
    }

    Count = fileheader.count;
    DataSize = fileheader.size;
    // BGMono_Printf("Mixfileclass %s DataSize: %08x   \n",filename,DataSize);Get_Key();
    /*
    **	Load up the offset control array. If RAM is exhausted, then the mixfile is invalid.
    */
    HeaderBuffer = new SubBlock[Count];
    if (HeaderBuffer == NULL)
        return;
    straw->Get(HeaderBuffer, Count * sizeof(SubBlock));

    /*
    **	The start of the embedded mixfile data will be at the current file offset.
    **	This should be true even if the file header has been encrypted because the file
    **	header was cleverly written with just the sufficient number of padding bytes so
    **	that this condition would be true.
    */
    DataStart = file.Seek(0, SEEK_CUR) + file.BiasStart;
    //	DataStart = file.Seek(0, SEEK_CUR);

    /*
    **	Attach to list of mixfiles.
    */
    List.Add_Tail(this);
}

/***********************************************************************************************
 * MixFileClass::Retrieve -- Retrieves a pointer to the specified data file.                   *
 *                                                                                             *
 *    This routine will return with a pointer to the specified data file if the file resides   *
 *    in memory. Otherwise, this routine returns NULL. Use this routine to access a resident   *
 *    file directly rather than going through the process of pseudo disk access. This will     *
 *    save both time and RAM.                                                                  *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename of the data file to retrieve a pointer to.     *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the data file's data. If the file is not in RAM, then    *
 *          NULL is returned.                                                                  *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/23/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
template <class T> void const* MixFileClass<T>::Retrieve(char const* filename)
{
    void* ptr = 0;
    Offset(filename, &ptr);
    return (ptr);
};

/***********************************************************************************************
 * MixFileClass::Finder -- Finds the mixfile object that matches the name specified.           *
 *                                                                                             *
 *    This routine will scan through all registered mixfiles and return with a pointer to      *
 *    the matching mixfile. If no mixfile could be found that matches the name specified,      *
 *    then NULL is returned.                                                                   *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename to search for.                                 *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the matching mixfile -- if found.                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1994 JLB : Created.                                                                 *
 *   06/08/1996 JLB : Only compares filename and extension.                                    *
 *=============================================================================================*/
template <class T> MixFileClass<T>* MixFileClass<T>::Finder(char const* filename)
{
    MixFileClass<T>* ptr = List.First();
    while (ptr->Is_Valid()) {
#ifdef _WIN32
        char path[_MAX_PATH];
        char name[_MAX_FNAME];
        char ext[_MAX_EXT];

        /*
        **	Strip the drive and path (if present) off of the filename
        **	in the mixfile list. This enables a simple comparison to the
        **	filename specified. The filename specified won't have a path attached and
        **	the full pathname in the mixfile list WILL have a path attached. Hence, this
        ** stripping of the path is necessary.
        */
        _splitpath(ptr->Filename, NULL, NULL, name, ext);
        _makepath(path, NULL, NULL, name, ext);
#else
        char buff[PATH_MAX];
        char* path = nullptr;
        strncpy(buff, ptr->FileName, PATH_MAX);
        buff[PATH_MAX - 1] = '\0';
        path = basename(buff);
#endif
        if (stricmp(path, filename) == 0) {
            return (ptr);
        }
        ptr = ptr->Next();
    }
    return (0);
}

/***********************************************************************************************
 * MixFileClass::Cache -- Caches the named mixfile into RAM.                                   *
 *                                                                                             *
 *    This routine will cache the mixfile, specified by name, into RAM.                        *
 *                                                                                             *
 * INPUT:   filename -- The name of the mixfile that should be cached.                         *
 *                                                                                             *
 * OUTPUT:  bool; Was the cache successful?                                                    *
 *                                                                                             *
 * WARNINGS:   This routine could go to disk for a very long time.                             *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
template <class T> bool MixFileClass<T>::Cache(char const* filename, Buffer const* buffer)
{
    MixFileClass<T>* mixer = Finder(filename);

    if (mixer != NULL) {
        return (mixer->Cache(buffer));
    }
    return (false);
}

/***********************************************************************************************
 * MixFileClass::Cache -- Loads this particular mixfile's data into RAM.                       *
 *                                                                                             *
 *    This load the mixfile data into ram for this mixfile object. This is the counterpart     *
 *    to the Free() function.                                                                  *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Was the file load successful?  It could fail if there wasn't enough room     *
 *                to allocate the raw data block.                                              *
 *                                                                                             *
 * WARNINGS:   This routine goes to disk for a potentially very long time.                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1994 JLB : Created.                                                                 *
 *   07/12/1996 JLB : Handles attached message digest.                                         *
 *=============================================================================================*/
template <class T> bool MixFileClass<T>::Cache(Buffer const* buffer)
{
    /*
    **	If the mixfile is already cached, then no action needs to be performed.
    */
    if (Data != NULL)
        return (true);

    /*
    **	If a buffer was supplied (and it is big enough), then use it as the data block
    **	pointer. Otherwise, the data block must be allocated.
    */
    if (buffer != NULL) {
        if (buffer->Get_Size() == 0 || buffer->Get_Size() >= DataSize) {
            Data = buffer->Get_Buffer();
        }
    } else {
        Data = new char[DataSize];
        IsAllocated = true;
    }

    /*
    **	If there is a data buffer to fill, then fill it now.
    */
    if (Data != NULL) {
        T file(Filename);

        FileStraw fstraw(file);
        Straw* straw = &fstraw;

        /*
        **	If a message digest is attached, then link a SHA straw segment to the data
        **	stream so that the actual SHA can be compared with the attached one.
        */
        SHAStraw sha;
        if (IsDigest) {
            sha.Get_From(fstraw);
            straw = &sha;
        }

        /*
        **	Bias the file to the actual start of the data. This is necessary because the
        **	real data starts some distance (not so easily determined) from the beginning of
        **	the real file.
        */
        file.Open(READ);
        file.Bias(0);
        file.Bias(DataStart);

        /*
        **	Fetch the whole mixfile data in one step. If the number of bytes retrieved
        **	does not equal that requested, then this indicates a serious error.
        */
        long actual = straw->Get(Data, DataSize);
        if (actual != DataSize) {
            delete[] Data;
            Data = NULL;
            file.Error(EIO);
            return (false);
        }

        /*
        **	If there is a digest attached to this mixfile, then read it in and
        **	compare it to the generated digest. If they don't match, then
        **	return with the "failure to cache" error code.
        */
        if (IsDigest) {
            char digest1[20];
            char digest2[20];
            sha.Result(digest2);
            fstraw.Get(digest1, sizeof(digest1));
            if (memcmp(digest1, digest2, sizeof(digest1)) != 0) {
                delete[] Data;
                Data = NULL;
                return (false);
            }
        }

        return (true);
    }
    IsAllocated = false;
    return (false);
}

/***********************************************************************************************
 * MixFileClass::Free -- Frees the allocated raw data block (not the index block).             *
 *                                                                                             *
 *    This routine will free the (presumably large) raw data block, but leave the index        *
 *    block intact. By using this in conjunction with the Cache() function, one can maintain   *
 *    tight control of memory usage. If the index block is desired to be freed, then the       *
 *    mixfile object must be deleted.                                                          *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/08/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
template <class T> void MixFileClass<T>::Free(void)
{
    if (Data != NULL && IsAllocated) {
        delete[] Data;
    }
    Data = NULL;
    IsAllocated = false;
}

inline int compfunc(void const* ptr1, void const* ptr2)
{
    if (*(long const*)ptr1 < *(long const*)ptr2)
        return (-1);
    if (*(long const*)ptr1 > *(long const*)ptr2)
        return (1);
    return (0);
}

/***********************************************************************************************
 * MixFileClass::Offset -- Determines the offset of the requested file from the mixfile system.*
 *                                                                                             *
 *    This routine will take the filename specified and search through the mixfile system      *
 *    looking for it. If the file was found, then the mixfile it was found in, the offset      *
 *    from the start of the mixfile, and the size of the embedded file will be returned.       *
 *    Using this method it is possible for the CCFileClass system to process it as a normal    *
 *    file.                                                                                    *
 *                                                                                             *
 * INPUT:   filename    -- The filename to search for.                                         *
 *                                                                                             *
 *          realptr     -- Stores a pointer to the start of the file in memory here. If the    *
 *                         file is not in memory, then NULL is stored here.                    *
 *                                                                                             *
 *          mixfile     -- The pointer to the corresponding mixfile is placed here. If no      *
 *                         mixfile was found that contains the file, then NULL is stored here. *
 *                                                                                             *
 *          offset      -- The starting offset from the beginning of the parent mixfile is     *
 *                         stored here.                                                        *
 *                                                                                             *
 *          size        -- The size of the embedded file is stored here.                       *
 *                                                                                             *
 * OUTPUT:  bool; Was the file found? The file may or may not be resident, but it does exist   *
 *                 and can be opened.                                                           *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
template <class T>
bool MixFileClass<T>::Offset(char const* filename, void** realptr, MixFileClass** mixfile, long* offset, long* size)
{
    MixFileClass<T>* ptr;

    if (filename == NULL) {
        assert(filename != NULL); // BG
        return (false);
    }

    /*
    **	Create the key block that will be used to binary search for the file.
    */
    // Can't call strupr on a const string. ST - 5/20/2019
    // long crc = Calculate_CRC(strupr((char *)filename), strlen(filename));
    char filename_upper[_MAX_PATH];
    strcpy(filename_upper, filename);
    strupr(filename_upper);
    long crc = Calculate_CRC(strupr(filename_upper), strlen(filename_upper));
    SubBlock key;
    key.CRC = crc;

    /*
    **	Sweep through all registered mixfiles, trying to find the file in question.
    */
    ptr = List.First();
    while (ptr->Is_Valid()) {
        SubBlock* block;

        /*
        **	Binary search for the file in this mixfile. If it is found, then extract the
        **	appropriate information and store it in the locations provided and then return.
        */
        block = (SubBlock*)bsearch(&key, ptr->HeaderBuffer, ptr->Count, sizeof(SubBlock), compfunc);
        if (block != NULL) {
            if (mixfile != NULL)
                *mixfile = ptr;
            if (size != NULL)
                *size = block->Size;
            if (realptr != NULL)
                *realptr = NULL;
            if (offset != NULL)
                *offset = block->Offset;
            if (realptr != NULL && ptr->Data != NULL) {
                *realptr = (char*)ptr->Data + block->Offset;
            }
            if (ptr->Data == NULL && offset != NULL) {
                *offset += ptr->DataStart;
            }
            return (true);
        }

        /*
        **	Advance to next mixfile.
        */
        ptr = ptr->Next();
    }

    /*
    **	All the mixfiles have been examined but no match was found. Return with the non success flag.
    */
    assert(1); // BG
    return (false);
}

// ST - 12/18/2019 11:36AM
template <class T> void MixFileClass<T>::Free_All(void)
{
    MixFileClass<T>* ptr = List.First();
    while (ptr->Is_Valid()) {
        delete ptr;
        ptr = List.First();
    }
}

#endif
