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

/* $Header: /CounterStrike/WWFILE.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : WWFILE.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : August 8, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WWFILE_Hxx
#define WWFILE_Hxx

#include <stddef.h>
#include <stdio.h>

#ifndef _READ
#define READ  1
#define WRITE 2
#endif

#ifndef READ
#define READ _READ
#endif
#ifndef WRITE
#define WRITE _WRITE
#endif

class FileClass
{
public:
    virtual ~FileClass(void){};
    virtual char const* File_Name(void) const = 0;
    virtual char const* Set_Name(char const* filename) = 0;
    virtual int Create(void) = 0;
    virtual int Delete(void) = 0;
    virtual int Is_Available(int forced = false) = 0;
    virtual int Is_Open(void) const = 0;
    virtual int Open(char const* filename, int rights = READ) = 0;
    virtual int Open(int rights = READ) = 0;
    virtual int Read(void* buffer, int size) = 0;
    virtual int Seek(int pos, int dir = SEEK_CUR) = 0;
    virtual int Size(void) = 0;
    virtual int Write(void const* buffer, int size) = 0;
    virtual void Close(void) = 0;
    virtual unsigned int Get_Date_Time(void)
    {
        return (0);
    }
    virtual bool Set_Date_Time(unsigned int)
    {
        return (false);
    }
    virtual void Error(int error, int canretry = false, char const* filename = NULL) = 0;

    operator char const*()
    {
        return File_Name();
    };
};

#endif
