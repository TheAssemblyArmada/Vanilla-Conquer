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

/* $Header: /CounterStrike/RAWFILE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : RAWFILE.H                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : October 18, 1994   [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   RawFileClass::File_Name -- Returns with the filename associate with the file object.      *
 *   RawFileClass::RawFileClass -- Default constructor for a file object.                      *
 *   RawFileClass::~RawFileClass -- Default deconstructor for a file object.                   *
 *   RawFileClass::Is_Open -- Checks to see if the file is open or not.                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef RAWFILE_H
#define RAWFILE_H

#include <limits.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "wwfile.h"

#ifndef WWERROR
#define WWERROR -1
#endif

/*
**	This is the definition of the raw file class. It is derived from the abstract base FileClass
**	and handles the interface to the low level DOS routines. This is the first class in the
**	chain of derived file classes that actually performs a useful function. With this class,
**	I/O is possible. More sophisticated features, such as packed files, CD-ROM support,
**	file caching, and XMS/EMS memory support, are handled by derived classes.
**
**	Of particular importance is the need to override the error routine if more sophisticated
**	error handling is required. This is more than likely if greater functionality is derived
**	from this base class.
*/
class RawFileClass : public FileClass
{
public:
    /*
    **	This is a record of the access rights used to open the file. These rights are
    **	used if the file object is duplicated.
    */
    int Rights;

    RawFileClass(char const* filename);
    RawFileClass(void);
    RawFileClass(RawFileClass const& f);
    RawFileClass& operator=(RawFileClass const& f);
    virtual ~RawFileClass(void);

    virtual char const* File_Name(void) const;
    virtual char const* Set_Name(char const* filename);
    virtual int Create(void);
    virtual int Delete(void);
    virtual int Is_Available(int forced = false);
    virtual bool Is_Directory(void);
    virtual int Is_Open(void) const;
    virtual int Open(char const* filename, int rights = READ);
    virtual int Open(int rights = READ);
    virtual int Read(void* buffer, int size);
    virtual int Seek(int pos, int dir = SEEK_CUR);
    virtual int Size(void);
    virtual int Write(void const* buffer, int size);
    virtual void Close(void);
    virtual void Error(int error, int canretry = false, char const* filename = NULL);
    void Bias(int start, int length = -1);

    FILE* Get_File_Handle(void)
    {
        return (Handle);
    };

    /*
    **	These bias values enable a sub-portion of a file to appear as if it
    **	were the whole file. This comes in very handy for multi-part files such as
    **	mixfiles.
    */
    int BiasStart;
    int BiasLength;

protected:
    /*
    **	This function returns the largest size a low level DOS read or write may
    **	perform. Larger file transfers are performed in chunks of this size or less.
    */
    int Transfer_Block_Size(void)
    {
        return (int)((unsigned)UINT_MAX) - 16L;
    };

    int Raw_Seek(int pos, int dir = SEEK_CUR);

private:
    /*
    **	This is the low level DOS handle. A -1 indicates an empty condition.
    */
    FILE* Handle;

    /*
    **	This points to a copy of the filename as a NULL terminated string.
    */
    char* Filename;
};

/***********************************************************************************************
 * RawFileClass::File_Name -- Returns with the filename associate with the file object.        *
 *                                                                                             *
 *    Use this routine to determine what filename is associated with this file object. If no   *
 *    filename has yet been assigned, then this routing will return NULL.                      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the file name associated with this file object or NULL   *
 *          if one doesn't exist.                                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline char const* RawFileClass::File_Name(void) const
{
    return Filename;
}

/***********************************************************************************************
 * RawFileClass::RawFileClass -- Default constructor for a file object.                        *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle or filename    *
 *    associated with it. In order to use a file object created in this fashion it must be     *
 *    assigned a name and then opened.                                                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline RawFileClass::RawFileClass(void)
    : Rights(READ)
    , BiasStart(0)
    , BiasLength(-1)
    , Handle(nullptr)
    , Filename(0)
{
}

/***********************************************************************************************
 * RawFileClass::~RawFileClass -- Default deconstructor for a file object.                     *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle or filename    *
 *    associated with it. In order to use a file object created in this fashion it must be     *
 *    assigned a name and then opened.                                                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline RawFileClass::~RawFileClass(void)
{
    Close();
    if (Filename) {
        free((char*)Filename);
        ((char*&)Filename) = 0;
    }
}

/***********************************************************************************************
 * RawFileClass::Is_Open -- Checks to see if the file is open or not.                          *
 *                                                                                             *
 *    Use this routine to determine if the file is open. It returns true if it is.             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open?                                                            *
 *                                                                                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline int RawFileClass::Is_Open(void) const
{
    return (Handle != nullptr);
}

#endif
