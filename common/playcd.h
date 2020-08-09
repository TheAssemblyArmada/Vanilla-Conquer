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
 *                 Project Name : WWLIB	  											*
 *                                                                         *
 *                    File Name : PLAYCD.H											*
 *                                                                         *
 *                   Programmer : STEVE WETHERILL									*
 *                                                                         *
 *                   Start Date : 5/13/94												*
 *                                                                         *
 *                  Last Update : June 4, 1994   [SW]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*/

#ifndef PLAYCD_H
#define PLAYCD_H

#include <string.h>

/***************************************************************************
 * GetCDClass -- object which will return logical CD drive						*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/04/1994 SW : Created.																*
 *=========================================================================*/

#define MAX_CD_DRIVES 26
#define NO_CD_DRIVE   -1

class GetCDClass
{
protected:
    int CDDrives[MAX_CD_DRIVES]; // Array containing CD drive letters
    int CDCount;                 // Number of available CD drives
    int CDIndex;

public:
    GetCDClass()
    {
#ifdef _WIN32
        char* _path = "a:\\";
        char drive[4];

        memset(CDDrives, NO_CD_DRIVE, sizeof(CDDrives));

        strcpy(drive, _path);

        for (char i = 'A'; i <= 'Z'; ++i) {
            drive[0] = i;

            int drive_type = GetDriveTypeA(drive);
            if (drive_type == DRIVE_CDROM) {
                CDDrives[CDCount++] = i - 'A';
            }
        }
#endif
    }

    ~GetCDClass()
    {
    }
    inline int Get_First_CD_Drive(void);
    inline int Get_Next_CD_Drive(void);
    inline int Get_Number_Of_Drives(void)
    {
        return (CDCount);
    };
};

/***********************************************************************************************
 * GCDC::Get_Next_CD_Drive -- return the logical drive number of the next CD drive             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Logical drive number of a cd drive or -1 if none                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/21/96 3:50PM ST : Created                                                              *
 *=============================================================================================*/
inline int GetCDClass::Get_Next_CD_Drive(void)
{
    if (CDCount) {
        if (CDIndex == CDCount)
            CDIndex = 0;
        return (CDDrives[CDIndex++]);
    } else {
        return (-1);
    }
}

/***************************************************************************
 * GCDC::Get_First_CD_Drive -- return the number of the first CD drive     *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *			none																				  *
 * OUTPUT:                                                                 *
 *			logical drive number 														  *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/26/1994 SW : Created.                                              *
 *   12/4/95    ST : fixed for Win95                                       *
 *=========================================================================*/
inline int GetCDClass::Get_First_CD_Drive(void)
{
    CDIndex = 0;
    return (Get_Next_CD_Drive());
}

#endif // PLAYCD_H
