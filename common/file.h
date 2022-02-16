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
 *                 Project Name : Library - Filio header stuff.            *
 *                                                                         *
 *                    File Name : FILE.H                                 	*
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : September 13, 1993                       *
 *                                                                         *
 *                  Last Update : April 11, 1994									*
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef FILE_H
#define FILE_H

#ifndef FILETEMP_H
// This should be removed once the library is all intacked.
#include "filetemp.h"
#endif

/*=========================================================================*/
/* File IO system defines and enumerations											*/
/*=========================================================================*/

#define XMAXPATH 80

/*
**	These are the Open_File, Read_File, and Seek_File constants.
*/
#ifndef READ
#define READ 1 // Read access.
#endif
#ifndef WRITE
#define WRITE 2 // Write access.
#endif
#ifndef SEEK_SET
#define SEEK_SET 0 // Seek from start of file.
#define SEEK_CUR 1 // Seek relative from current location.
#define SEEK_END 2 // Seek from end of file.
#endif

typedef enum
{
    FILEB_PROCESSED = 8, // Was the packed file header of this file processed?
    FILEB_PRELOAD,       // Scan for and make file resident at WWDOS_Init time?
    FILEB_RESIDENT,      // Make resident at Open_File time?
    FILEB_FLUSH,         // Un-resident at Close_File time?
    FILEB_PACKED,        // Is this file packed?
    FILEB_KEEP,          // Don't ever flush this resident file?
    FILEB_PRIORITY,      // Flush this file last?

    FILEB_LAST
} FileFlags_Type;

#define FILEF_NONE      0
#define FILEF_PROCESSED (1 << FILEB_PROCESSED)
#define FILEF_PRELOAD   (1 << FILEB_PRELOAD)
#define FILEF_RESIDENT  (1 << FILEB_RESIDENT)
#define FILEF_FLUSH     (1 << FILEB_FLUSH)
#define FILEF_PACKED    (1 << FILEB_PACKED)
#define FILEF_KEEP      (1 << FILEB_KEEP)
#define FILEF_PRIORITY  (1 << FILEB_PRIORITY)

/*
** These errors are returned by WWDOS_Init().  All errors encountered are
** or'd together so there may be more then one error returned.  Not all
** errors are fatal, such as the cache errors.
*/
typedef enum
{
    FI_SUCCESS = 0x00,
    FI_CACHE_TOO_BIG = 0x01,
    FI_CACHE_ALREADY_INIT = 0x02,
    FI_FILEDATA_FILE_NOT_FOUND = 0x04,
    FI_FILEDATA_TOO_BIG = 0x08,
    FI_SEARCH_PATH_NOT_FOUND = 0x10,
    FI_STARTUP_PATH_NOT_FOUND = 0x20,
    FI_NO_CACHE_FOR_PRELOAD = 0x40,
    FI_FILETABLE_NOT_INIT = 0x80,
} FileInitErrorType;

/*
**	These are the errors that are detected by the File I/O system and
**	passed to the io error routine.
*/
// lint -strong(AJX,FileErrorType)
typedef enum
{
    CANT_CREATE_FILE,
    BAD_OPEN_MODE,
    COULD_NOT_OPEN,
    TOO_MANY_FILES,
    CLOSING_NON_HANDLE,
    READING_NON_HANDLE,
    WRITING_NON_HANDLE,
    SEEKING_NON_HANDLE,
    SEEKING_BAD_OFFSET,
    WRITING_RESIDENT,
    UNKNOWN_INDEX,
    DID_NOT_CLOSE,
    FATAL_ERROR,
    FILE_NOT_LISTED,
    FILE_LENGTH_MISMATCH,
    INTERNAL_ERROR,
    MAKE_RESIDENT_ZERO_SIZE,
    RESIDENT_SORT_FAILURE,

    NUMBER_OF_ERRORS /* MAKE SURE THIS IS THE LAST ENTRY */
} FileErrorType;

/*=========================================================================*/
/* File IO system structures																*/
/*=========================================================================*/

// lint -strong(AJX,FileDataType)
typedef struct
{
    char* Name;              // File name (include sub-directory but not volume).
    int Size;                // File size (0=indeterminate).
    void* Ptr;               // Resident file pointer.
    int Start;               // Starting offset in DOS handle file.
    unsigned char Disk;      // Disk number location.
    unsigned char OpenCount; // Count of open locks on resident file.
    unsigned short Flag;     // File control flags.
} FileDataType;

/*=========================================================================*/
/* FIle IO system globals.																	*/
/*=========================================================================*/

// These are cpp errors in funtions declarations	JULIO JEREZ

// extern FileDataType FileData[];
// extern BYTE ExecPath[XMAXPATH + 1];
// extern BYTE DataPath[XMAXPATH + 1];
// extern BYTE StartPath[XMAXPATH + 1];
// extern BOOL UseCD;

// The correct syntax is  NO TYPE MODIFIER APPLY TO DATA DECLARATIONS
extern FileDataType FileData[];
extern char ExecPath[XMAXPATH + 1];
extern char DataPath[XMAXPATH + 1];
extern char StartPath[XMAXPATH + 1];

/*=========================================================================*/
/* The following prototypes are for the file: FILEINIT.CPP						*/
/*=========================================================================*/

void WWDOS_Shutdown(void);
FileInitErrorType WWDOS_Init(unsigned int cachesize, char* filedata, char* cdpath);

/*=========================================================================*/
/* The following prototypes are for the file: FILE.CPP							*/
/*=========================================================================*/

int Open_File(char const* file_name, int mode);
void Close_File(int handle);
int Read_File(int handle, void* buf, unsigned int bytes);
int Load_File(const char* file_name, void* load_addr);
int Write_File(int handle, void const* buf, unsigned int bytes);
unsigned int Seek_File(int handle, int offset, int starting);
int File_Exists(char const* file_name);
unsigned int File_Size(int handle);
int Open_File_With_Recovery(char const* file_name, unsigned int mode);

/*=========================================================================*/
/* The following prototypes are for the file: FILECACH.CPP						*/
/*=========================================================================*/

void Unfragment_File_Cache(void);
int Flush_Unused_File_Cache(int flush_keeps);

/*=========================================================================*/
/* The following prototypes are for the file: FILECHNG.CPP						*/
/*=========================================================================*/

int Create_File(char const* file_name);
extern int Delete_File(char const* file_name);

/*=========================================================================*/
/* The following prototypes are for the file: FILEINFO.CPP						*/
/*=========================================================================*/

int Get_DOS_Handle(int fh);
int Free_Handles(void);
int Find_Disk_Number(char const* file_name);
int Set_File_Flags(char const* filename, int flags);
int Clear_File_Flags(char const* filename, int flags);
int Get_File_Flags(char const* filename);

/*=========================================================================*/
/* The following prototypes are for the file: FINDFILE.CPP						*/
/*=========================================================================*/

int Find_File(char const* file_name);
int Find_File_Index(char const* filename);

/*=========================================================================*/
/* The following prototypes are for the file: file.cpp                     */
/*=========================================================================*/

void Resolve_File(char* fname);

class Find_File_Data
{
public:
    static Find_File_Data* CreateFindData();

    virtual ~Find_File_Data()
    {
    }
    virtual const char* GetName() const = 0;
    virtual unsigned int GetTime() const = 0;

    virtual bool FindFirst(const char* fname) = 0;
    virtual bool FindNext() = 0;
    virtual void Close() = 0;
};

extern bool Find_First(const char* fname, unsigned int mode, Find_File_Data** ffblk);
extern bool Find_Next(Find_File_Data* ffblk);
extern void Find_Close(Find_File_Data* ffblk);

#endif
