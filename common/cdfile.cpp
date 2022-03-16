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

/* $Header: /CounterStrike/CDFILE.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : CDFILE.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : October 18, 1994                                             *
 *                                                                                             *
 *                  Last Update : September 22, 1995 [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   CDFileClass::Clear_Search_Drives -- Removes all record of a search path.                  *
 *   CDFileClass::Open -- Opens the file object -- with path search.                           *
 *   CDFileClass::Open -- Opens the file wherever it can be found.                             *
 *   CDFileClass::Set_Name -- Performs a multiple directory scan to set the filename.          *
 *   CDFileClass::Set_Search_Drives -- Sets a list of search paths for file access.            *
 *   Is_Disk_Inserted -- Checks to see if a disk is inserted in specified drive.               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "cdfile.h"
#include "paths.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h> // for MAX_PATH
#else
#ifndef MAX_PATH
#include <limits.h>
#define MAX_PATH  PATH_MAX
#define _MAX_PATH PATH_MAX
#endif
#endif

/*
**	Pointer to the first search path record.
*/
CDFileClass::SearchDriveType* CDFileClass::First = 0;

int CDFileClass::CurrentCDDrive = 0;
int CDFileClass::LastCDDrive = 0;
char CDFileClass::RawPath[512] = {0};

CDFileClass::CDFileClass(char const* filename)
    : IsDisabled(false)
{
    CDFileClass::Set_Name(filename);
    //	memset (RawPath, 0, sizeof(RawPath));
}

CDFileClass::CDFileClass(void)
    : IsDisabled(false)
{
}

extern int Get_CD_Index(int cd_drive, int timeout);

/***********************************************************************************************
 * Is_Disk_Inserted -- Checks to see if a disk is inserted in specified drive.                 *
 *                                                                                             *
 *    This routine will examine the drive specified to see if there is a disk inserted. It     *
 *    can be used for floppy drives as well as for the CD-ROM.                                 *
 *                                                                                             *
 * INPUT:   disk  -- The drive number to examine. 0=A, 1=B, etc.                               *
 *                                                                                             *
 * OUTPUT:  bool; Is a disk inserted into the specified drive?                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
int Is_Disk_Inserted(int disk)
{
    disk;
    return true;
}

/***********************************************************************************************
 * CDFileClass::Open -- Opens the file object -- with path search.                             *
 *                                                                                             *
 *    This will open the file object, but since the file object could have been constructed    *
 *    with a pathname, this routine will try to find the file first. For files opened for      *
 *    writing, then use the existing filename without performing a path search.                *
 *                                                                                             *
 * INPUT:   rights   -- The access rights to use when opening the file                         *
 *                                                                                             *
 * OUTPUT:  bool; Was the open successful?                                                     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int CDFileClass::Open(int rights)
{
    std::string path = File_Name();
    /*
    ** If we are wanting a writeable file and the path is not based off of the User_Path
    ** then we might have a problem. If the filename is relative then just append to User_Path.
    ** Otherwise it will try and write it to the working directory, probably the binary dir.
    */
    if ((rights & WRITE) && !PathsClass::Is_Absolute(File_Name())) {
        path = Paths.User_Path();
        path += PathsClass::SEP;
        path += File_Name();
        BufferIOFileClass::Set_Name(path.c_str());
    }

    return (BufferIOFileClass::Open(rights));
}
/***********************************************************************************************
 * CDFC::Refresh_Search_Drives -- Updates the search path when a CD changes or is added        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/22/96 9:01AM ST : Created                                                              *
 *=============================================================================================*/
void CDFileClass::Refresh_Search_Drives(void)
{
    Clear_Search_Drives();
    // User_Path always first so the game always looks here first.
    Add_Search_Drive(Paths.User_Path());
    Set_Search_Drives(RawPath);
    Add_Search_Drive(Paths.Data_Path());
}

/***********************************************************************************************
 * CDFileClass::Set_Search_Drives -- Sets a list of search paths for file access.              *
 *                                                                                             *
 *    This routine sets up a list of search paths to use when accessing files. The path list   *
 *    is scanned if the file could not be found in the current directory. This is the primary  *
 *    method of supporting CD-ROM drives, but is also useful for scanning network and other    *
 *    directories. The pathlist as passed to this routine is of the same format as the path    *
 *    list used by DOS -- paths are separated by semicolons and need not end in an antivirgule.*
 *                                                                                             *
 *    If a path entry begins with "?:" then the question mark will be replaced with the first  *
 *    CD-ROM drive letter available. If there is no CD-ROM driver detected, then this path     *
 *    entry will be ignored. By using this feature, you can always pass the CD-ROM path        *
 *    specification to this routine and it will not break if the CD-ROM is not loaded (as in   *
 *    the case during development).                                                            *
 *                                                                                             *
 *    Here is an example path specification:                                                   *
 *                                                                                             *
 *       Set_Search_Drives("DATA;?:\DATA;F:\PROJECT\DATA");                                    *
 *                                                                                             *
 *    In this example, the current directory will be searched first, followed by a the         *
 *    subdirectory "DATA" located off of the current directory. If not found, then the CD-ROM  *
 *    will be searched in a directory called "\DATA". If not found or the CD-ROM is not        *
 *    present, then it will look to the hard coded path of "F:\PROJECTS\DATA" (maybe a         *
 *    network?). If all of these searches fail, the file system will default to the current    *
 *    directory and let the normal file error system take over.                                *
 *                                                                                             *
 * INPUT:   pathlist -- Pointer to string of path specifications (separated by semicolons)     *
 *                      that will be used to search for files.                                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *   05/21/1996 ST  : Modified to recognise multiple CD drives                                 *
 *=============================================================================================*/
int CDFileClass::Set_Search_Drives(char* pathlist)
{
    bool found = false;
    bool empty = false;

    /*
    **	If there is no pathlist to add, then just return.
    */
    if (!pathlist)
        return (0);

    /*
    ** Save the path as it was passed in so we can parse it again later.
    ** Check for the case where RawPath was passed in.
    */
    if (pathlist != RawPath) {
        strcat(RawPath, ";");
        strcat(RawPath, pathlist);
    }

    char const* ptr = strtok(pathlist, ";");
    while (ptr) {
        if (strlen(ptr)) {
            /*
            **	If the path is relative, append it to user and data paths for searching, otherwise just append as is.
            */
            if (PathsClass::Is_Absolute(ptr)) {
                Add_Search_Drive(ptr);
            } else {
                std::string fullpath = Paths.User_Path();
                fullpath += ptr;
                Add_Search_Drive(fullpath.c_str());

                /*
                ** If the Data path is not the same as the user path, append that too.
                */
                if (Paths.User_Path() != Paths.Data_Path()) {
                    fullpath = Paths.Data_Path();
                    fullpath += ptr;
                    Add_Search_Drive(fullpath.c_str());
                }

                /*
                ** Regardless of anything else, add the relative path so it works if its in the working dir.
                */
                Add_Search_Drive(ptr);
            }
        }

        /*
        **	Find the next path string and resubmit.
        */
        ptr = strtok(NULL, ";");
    }
    if (!found)
        return (1);
    if (empty)
        return (2);
    return (0);
}

/***********************************************************************************************
 * CDFC::Add_Search_Drive -- Add a new path to the search path list                            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    path                                                                              *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/22/96 10:12AM ST : Created                                                             *
 *=============================================================================================*/
void CDFileClass::Add_Search_Drive(const char* path)
{
    SearchDriveType* srch; // Working pointer to path object.
    /*
    **	Allocate a record structure.
    */
    srch = new SearchDriveType;

    /*
    **	Attach the path to this structure.
    */
    srch->Path = strdup(path);
    srch->Next = NULL;

    /*
    **	Attach this path record to the end of the path chain.
    */
    if (!First) {
        First = srch;
    } else {
        SearchDriveType* chain = First;

        while (chain->Next) {
            chain = (SearchDriveType*)chain->Next;
        }
        chain->Next = srch;
    }
}

/***********************************************************************************************
 * CDFC::Set_CD_Drive -- sets the current CD drive letter                                      *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/22/96 9:39AM ST : Created                                                              *
 *=============================================================================================*/
void CDFileClass::Set_CD_Drive(int drive)
{
    LastCDDrive = CurrentCDDrive;
    CurrentCDDrive = drive;
}

/***********************************************************************************************
 * CDFileClass::Clear_Search_Drives -- Removes all record of a search path.                    *
 *                                                                                             *
 *    Use this routine to clear out any previous path(s) set with Set_Search_Drives()          *
 *    function.                                                                                *
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
void CDFileClass::Clear_Search_Drives(void)
{
    SearchDriveType* chain; // Working pointer to path chain.

    chain = First;
    while (chain) {
        SearchDriveType* next;

        next = (SearchDriveType*)chain->Next;
        if (chain->Path) {
            free((char*)chain->Path);
        }
        delete chain;

        chain = next;
    }
    First = 0;
}

/***********************************************************************************************
 * CDFileClass::Set_Name -- Performs a multiple directory scan to set the filename.            *
 *                                                                                             *
 *    This routine will scan all the directories specified in the path list and if the file    *
 *    was found in one of the directories, it will set the filename to a composite of the      *
 *    correct directory and the filename. It is used to allow path searching when searching    *
 *    for files. Typical use is to support CD-ROM drives. This routine examines the current    *
 *    directory first before scanning through the path list. If after scanning the entire      *
 *    path list, the file still could not be found, then the file object's name is set with    *
 *    just the raw filename as passed to this routine.                                         *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename to set as the name of this file object.        *
 *                                                                                             *
 * OUTPUT:  Returns a pointer to the final and complete filename of this file object. This     *
 *          may have a path attached to the file.                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
char const* CDFileClass::Set_Name(char const* filename)
{
    /*
    **	Just return with the normal file name setting process if there is
    **	no multi-drive search path or searching is disabled.
    */
    if (IsDisabled || !First || PathsClass::Is_Absolute(filename)) {
        BufferIOFileClass::Set_Name(filename);
        return (File_Name());
    }

    /*
    **	Attempt to find the file first. Check the current directory. If not found there, then
    **	search all the path specifications available. If it still can't be found, then just
    **	fall into the normal raw file filename setting system.
    */
    SearchDriveType* srch = First;

    while (srch) {
        /*
        **	Build a pathname to search for.
        */
        std::string path = srch->Path;
        path += PathsClass::SEP;
        path += filename;

        /*
        **	Check to see if the file could be found. The low level Is_Available logic will
        **	prompt if necessary when the CD-ROM drive has been removed. In all other cases,
        **	it will return false and the search process will continue.
        */
        BufferIOFileClass::Set_Name(path.c_str());
        if (BufferIOFileClass::Is_Available()) {
            return (File_Name());
        }

        /*
        **	It wasn't found, so try the next path entry.
        */
        srch = (SearchDriveType*)srch->Next;
    }

    /*
    **	At this point, all path searching has failed. Just set the file name to the
    **	plain text passed to this routine and be done with it.
    */
    BufferIOFileClass::Set_Name(filename);
    return (File_Name());
}

int CDFileClass::Is_Available(int forced)
{
    std::string filename = RawFileClass::File_Name();

    if (IsDisabled || !First || PathsClass::Is_Absolute(filename.c_str())) {
        return BufferIOFileClass::Is_Available(forced);
    }

    /*
    **	Attempt to find the file first. Check the current directory. If not found there, then
    **	search all the path specifications available. If it still can't be found, then just
    **	fall into the normal raw file filename setting system.
    */
    SearchDriveType* srch = First;

    while (srch) {
        /*
        **	Build a pathname to search for.
        */
        std::string path = srch->Path;
        path += PathsClass::SEP;
        path += filename;

        /*
        **	Check to see if the file could be found. The low level Is_Available logic will
        **	prompt if necessary when the CD-ROM drive has been removed. In all other cases,
        **	it will return false and the search process will continue.
        */
        if (RawFileClass(path.c_str()).Is_Available()) {
            return true;
        }

        /*
        **	It wasn't found, so try the next path entry.
        */
        srch = (SearchDriveType*)srch->Next;
    }

    /*
    **	At this point, all path searching has failed. Just set the file name to the
    **	original and return its availability.
    */

    return BufferIOFileClass::Is_Available(forced);
}

/***********************************************************************************************
 * CDFileClass::Open -- Opens the file wherever it can be found.                               *
 *                                                                                             *
 *    This routine is similar to the RawFileClass open except that if the file is being        *
 *    opened only for READ access, it will search all specified directories looking for the    *
 *    file. If after a complete search the file still couldn't be found, then it is opened     *
 *    using the normal BufferIOFileClass system -- resulting in normal error procedures.       *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the override filename to supply for this file object. It    *
 *                      would be the base filename (sans any directory specification).         *
 *                                                                                             *
 *          rights   -- The access rights to use when opening the file.                        *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened successfully? If so then the filename may be different   *
 *                than requested. The location of the file can be determined by examining the  *
 *                filename of this file object. The filename will contain the complete         *
 *                pathname used to open the file.                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int CDFileClass::Open(char const* filename, int rights)
{
    CDFileClass::Close();

    /*
    **	Verify that there is a filename associated with this file object. If not, then this is a
    **	big error condition.
    */
    if (!filename) {
        Error(ENOENT, false);
    }

    /*
    **	If writing is requested, then multiple drive searching is not performed.
    */
    if (IsDisabled || PathsClass::Is_Absolute(filename)) {
        BufferIOFileClass::Set_Name(filename);
        return (BufferIOFileClass::Open(rights));
    }

    if ((rights | WRITE)) {
        std::string write_path = Paths.User_Path();
        write_path += PathsClass::SEP;
        write_path += filename;
        BufferIOFileClass::Set_Name(filename);
        return (BufferIOFileClass::Open(rights));
    }

    /*
    **	Perform normal multiple drive searching for the filename and open
    **	using the normal procedure.
    */
    Set_Name(filename);
    return (BufferIOFileClass::Open(rights));
}

const char* CDFileClass::Get_Search_Path(int index)
{
    if (First == NULL) {
        return NULL;
    }

    SearchDriveType* sd = First;

    for (int i = 0; i <= index; i++) { // We want to loop once, even if index==0

        if (i == index) {
            return sd->Path;
        }

        sd = (SearchDriveType*)sd->Next;
        if (sd == NULL) {
            return NULL;
        }
    }

    return NULL;
}
