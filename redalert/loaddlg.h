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

/* $Header: /CounterStrike/LOADDLG.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : LOADDLG.H                                                    *
 *                                                                                             *
 *                   Programmer : Maria Legg, Joe Bostic, Bill Randolph                        *
 *                                                                                             *
 *                   Start Date : March 19, 1995                                               *
 *                                                                                             *
 *                  Last Update : March 19, 1995                                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef LOADDLG_H
#define LOADDLG_H

#include "defines.h"

class FileEntryClass
{
public:
    char Descr[80];         // save-game description
    unsigned Scenario;      // scenario #
    HousesType House;       // house
    int Num;                // save file number (from the extension)
    unsigned long DateTime; // date/time stamp of file
    bool Valid;             // Is the scenario valid?
};

class LoadOptionsClass
{
public:
    /*
    ** This defines the style of the dialog
    */
    typedef enum OperationModeEnum
    {
        NONE = 0,
        LOAD,
        SAVE,
        WWDELETE
    } LoadStyleType;

    LoadOptionsClass(LoadStyleType style = LoadOptionsClass::NONE);
    ~LoadOptionsClass();
    int Process(void);

protected:
    /*
    ** Internal routines
    */
    void Clear_List(ListClass* list);                   // clears the list & game # array
    void Fill_List(ListClass* list);                    // fills the list & game # array
    int Num_From_Ext(const char* fname);                // translates filename to file #
    static int Compare(const void* p1, const void* p2); // for qsort()

    /*
    ** This is the requested style of the dialog
    */
    LoadStyleType Style;

    /*
    ** This is an array of pointers to FileEntryClass objects.  These objects
    ** are allocated on the fly as files are found, and pointers to them are
    ** added to the vector list.  Thus, all the objects must be free'd before
    ** the vector list is cleared.  This list is used for sorting the files
    ** by date/time.
    */
    DynamicVectorClass<FileEntryClass*> Files;
};

#endif
