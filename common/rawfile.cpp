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

/* $Header: /CounterStrike/RAWFILE.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : RAWFILE.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : August 4, 1996 [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   RawFileClass::Bias -- Bias a file with a specific starting position and length.           *
 *   RawFileClass::Close -- Perform a closure of the file.                                     *
 *   RawFileClass::Create -- Creates an empty file.                                            *
 *   RawFileClass::Delete -- Deletes the file object from the disk.                            *
 *   RawFileClass::Error -- Handles displaying a file error message.                           *
 *   RawFileClass::Is_Available -- Checks to see if the specified file is available to open.   *
 *   RawFileClass::Is_Directory -- Checks to see if the specified file is a directory.         *
 *   RawFileClass::Open -- Assigns name and opens file in one operation.                       *
 *   RawFileClass::Open -- Opens the file object with the rights specified.                    *
 *   RawFileClass::RawFileClass -- Simple constructor for a file object.                       *
 *   RawFileClass::Raw_Seek -- Performs a seek on the unbiased file                            *
 *   RawFileClass::Read -- Reads the specified number of bytes into a memory buffer.           *
 *   RawFileClass::Seek -- Reposition the file pointer as indicated.                           *
 *   RawFileClass::Set_Name -- Manually sets the name for a file object.                       *
 *   RawFileClass::Size -- Determines size of file (in bytes).                                 *
 *   RawFileClass::Write -- Writes the specified data to the buffer specified.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "rawfile.h"
#include "file.h"
#include "wwstd.h"

#ifndef _WIN32
#include <unistd.h>
#define _unlink         unlink
#define raw_fopen(x, y) fopen(x, y)
#else
#include "utf.h"
#define raw_fopen(x, y) _wfopen(UTF8To16(x), UTF8To16(y))
#endif

#include <sys/stat.h>

/***********************************************************************************************
 * RawFileClass::Error -- Handles displaying a file error message.                             *
 *                                                                                             *
 *    Display an error message as indicated. If it is allowed to retry, then pressing a key    *
 *    will return from this function. Otherwise, it will exit the program with "exit()".       *
 *                                                                                             *
 * INPUT:   error    -- The error number (same as the DOSERR.H error numbers).                 *
 *                                                                                             *
 *          canretry -- Can this routine exit normally so that retrying can occur? If this is  *
 *                      false, then the program WILL exit in this routine.                     *
 *                                                                                             *
 *          filename -- Optional filename to report with this error. If no filename is         *
 *                      supplied, then no filename is listed in the error message.             *
 *                                                                                             *
 * OUTPUT:  none, but this routine might not return at all if the "canretry" parameter is      *
 *          false or the player pressed ESC.                                                   *
 *                                                                                             *
 * WARNINGS:   This routine may not return at all. It handles being in text mode as well as    *
 *             if in a graphic mode.                                                           *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void RawFileClass::Error(int error, int canretry, char const* filename)
{
#ifndef _WIN32
    perror(filename);
#endif
    exit(1);
}

/***********************************************************************************************
 * RawFileClass::RawFileClass -- Simple constructor for a file object.                         *
 *                                                                                             *
 *    This constructor is called when a file object is created with a supplied filename, but   *
 *    not opened at the same time. In this case, an assumption is made that the supplied       *
 *    filename is a constant string. A duplicate of the filename string is not created since   *
 *    it would be wasteful in that case.                                                       *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object.                            *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
RawFileClass::RawFileClass(char const* filename)
    : Rights(0)
    , BiasStart(0)
    , BiasLength(-1)
    , Handle(nullptr)
    , Filename(nullptr)
{
    Set_Name(filename);
}

/***********************************************************************************************
 * RawFileClass::Set_Name -- Manually sets the name for a file object.                         *
 *                                                                                             *
 *    This routine will set the name for the file object to the name specified. This name is   *
 *    duplicated in free store. This allows the supplied name to be a temporarily constructed  *
 *    text string. Setting the name in this fashion doesn't affect the closed or opened state  *
 *    of the file.                                                                             *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object.                            *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated copy of this filename. This pointer is     *
 *          guaranteed to remain valid for the duration of this file object or until the name  *
 *          is changed -- whichever is sooner.                                                 *
 *                                                                                             *
 * WARNINGS:   Because of the allocation this routine must perform, memory could become        *
 *             fragmented.                                                                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
char const* RawFileClass::Set_Name(char const* filename)
{
    if (Filename != NULL) {
        free(Filename);
        Filename = nullptr;
    }

    if (filename == NULL)
        return (NULL);

    Bias(0);

    Filename = strdup(filename);
    if (Filename == NULL) {
        Error(ENOMEM, false, filename);
    }

    /*
    ** If we ever save this file, make sure we save it in lowercase but
    ** if Resolve_File finds an actual file on-disk we use the real name
    ** instead.
    */
    _strlwr(Filename);

    /*
    ** Try to locate an existing file ignoring case, updates Filename
    */
    Resolve_File(Filename);

    return (Filename);
}

/***********************************************************************************************
 * RawFileClass::Open -- Assigns name and opens file in one operation.                         *
 *                                                                                             *
 *    This routine will assign the specified filename to the file object and open it at the    *
 *    same time. If the file object was already open, then it will be closed first. If the     *
 *    file object was previously assigned a filename, then it will be replaced with the new    *
 *    name. Typically, this routine is used when an anonymous file object has been crated and  *
 *    now it needs to be assigned a name and opened.                                           *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object.                            *
 *                                                                                             *
 *          rights   -- The open file access rights to use.                                    *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened? The return value of this is moot, since the open file   *
 *          is designed to never return unless it succeeded.                                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int RawFileClass::Open(char const* filename, int rights)
{
    Set_Name(filename);
    return (Open(rights));
}

/***********************************************************************************************
 * RawFileClass::Open -- Opens the file object with the rights specified.                      *
 *                                                                                             *
 *    This routine is used to open the specified file object with the access rights indicated. *
 *    This only works if the file has already been assigned a filename. It is guaranteed, by   *
 *    the error handler, that this routine will always return with success.                    *
 *                                                                                             *
 * INPUT:   rights   -- The file access rights to use when opening this file. This is a        *
 *                      combination of READ and/or WRITE bit flags.                            *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened successfully? This will always return true by reason of  *
 *          the error handler.                                                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int RawFileClass::Open(int rights)
{
    Close();

    /*
    **	Verify that there is a filename associated with this file object. If not, then this is a
    **	big error condition.
    */
    if (Filename == NULL) {
        Error(ENOENT, false);
    }

    /*
    **	Record the access rights used for this open call. These rights will be used if the
    **	file object is duplicated.
    */
    Rights = rights;

    /*
    **	Repetitively try to open the file. Abort if a fatal error condition occurs.
    */
    for (;;) {

        /*
        **	Try to open the file according to the access rights specified.
        */
        switch (rights) {

        /*
        **	If the access rights are not recognized, then report this as
        **	an invalid access code.
        */
        default:
            errno = EINVAL;
            break;

        case READ:
            Handle = raw_fopen(Filename, "rb");
            break;

        case WRITE:
            Handle = raw_fopen(Filename, "wb");
            break;

        case READ | WRITE:
            Handle = raw_fopen(Filename, "rwb");
            break;
        }

        /*
        **	Biased files must be positioned past the bias start position.
        */
        if (BiasStart != 0 || BiasLength != -1) {
            Seek(0, SEEK_SET);
        }

        /*
        **	If the handle indicates the file is not open, then this is an error condition.
        **	For the case of the file cannot be found, then allow a retry. All other cases
        **	are fatal.
        */
        if (Handle == nullptr) {
            Error(errno, false, Filename);
        }
        break;
    }

    return (true);
}

/***********************************************************************************************
 * RawFileClass::Is_Available -- Checks to see if the specified file is available to open.     *
 *                                                                                             *
 *    This routine will examine the disk system to see if the specified file can be opened     *
 *    or not. Use this routine before opening a file in order to make sure that is available   *
 *    or to perform other necessary actions.                                                   *
 *                                                                                             *
 * INPUT:   force -- Should this routine keep retrying until the file becomes available? If    *
 *                   in this case it doesn't become available, then the program will abort.    *
 *                                                                                             *
 * OUTPUT:  bool; Is the file available to be opened?                                          *
 *                                                                                             *
 * WARNINGS:   Depending on the parameter passed in, this routine may never return.            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int RawFileClass::Is_Available(int forced)
{
    if (Filename == NULL)
        return (false);

    /*
    **	If the file is already open, then is must have already passed the availability check.
    **	Return true in this case.
    */
    if (Is_Open())
        return (true);

    /*
    **	If this is a forced check, then go through the normal open channels, since those
    **	channels ensure that the file must exist.
    */
    if (forced) {
        RawFileClass::Open(READ);
        RawFileClass::Close();
        return (true);
    }

    /*
    **	Perform a raw open of the file. If this open fails for ANY REASON, including a missing
    **	CD-ROM, this routine will return a failure condition. In all but the missing file
    **	condition, go through the normal error recover channels.
    */
    Handle = raw_fopen(Filename, "r");
    if (Handle == nullptr) {
        return (false);
    }

    /*
    **	Since the file could be opened, then close it and return that the file exists.
    */
    if (fclose(Handle) != 0) {
        Error(errno, false, Filename);
    }
    Handle = nullptr;

    return (true);
}

/***********************************************************************************************
 * RawFileClass::Is_Directory -- Checks to see if the specified file is a directory.           *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the file a directory?                                                     *
 *                                                                                             *
 *=============================================================================================*/
bool RawFileClass::Is_Directory()
{
    if (Filename == NULL) {
        return false;
    }

    struct stat st;
    return stat(Filename, &st) == 0 && (st.st_mode & S_IFDIR) == S_IFDIR;
}

/***********************************************************************************************
 * RawFileClass::Close -- Perform a closure of the file.                                       *
 *                                                                                             *
 *    Close the file object. In the rare case of an error, handle it as appropriate.           *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Some rare error conditions may cause this routine to abort the program.         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void RawFileClass::Close(void)
{
    /*
    **	If the file is open, then close it. If the file is already closed, then just return. This
    **	isn't considered an error condition.
    */
    if (Is_Open()) {
        /*
        **	Try to close the file. If there was an error (who knows what that could be), then
        **	call the error routine.
        */
        if (fclose(Handle) != 0) {
            Error(errno, false, Filename);
        }

        /*
        **	At this point the file must have been closed. Mark the file as empty and return.
        */
        Handle = nullptr;

        /*
        **	Clear any positioning information incase class is reused to open another file.
        */
        BiasStart = 0;
        BiasLength = -1;
    }
}

/***********************************************************************************************
 * RawFileClass::Read -- Reads the specified number of bytes into a memory buffer.             *
 *                                                                                             *
 *    This routine will read the specified number of bytes and place the data into the buffer  *
 *    indicated. It is legal to call this routine with a request for more bytes than are in    *
 *    the file. This condition can result in fewer bytes being read than requested. Determine  *
 *    this by examining the return value.                                                      *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer to read data into. If NULL is passed, no read    *
 *                      is performed.                                                          *
 *                                                                                             *
 *          size     -- The number of bytes to read. If NULL is passed, then no read is        *
 *                      performed.                                                             *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes read into the buffer. If this number is less      *
 *          than requested, it indicates that the file has been exhausted.                     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long RawFileClass::Read(void* buffer, long size)
{
    long bytesread = 0; // Running count of the number of bytes read into the buffer.
    int opened = false; // Was the file opened by this routine?

    /*
    **	If the file isn't opened, open it. This serves as a convenience
    **	for the programmer.
    */
    if (!Is_Open()) {

        /*
        **	The error check here is moot. Open will never return unless it succeeded.
        */
        if (!Open(READ)) {
            return (0);
        }
        opened = true;
    }

    /*
    **	A biased file has the requested read length limited to the bias length of
    **	the file.
    */
    if (BiasLength != -1) {
        int remainder = BiasLength - Seek(0);
        size = size < remainder ? size : remainder;
    }

    long total = 0;
    while (size > 0) {
        clearerr(Handle);
        bytesread = fread(buffer, 1, size, Handle);
        if (ferror(Handle)) {
            size -= bytesread;
            total += bytesread;
            Error(errno, true, Filename);
            continue;
        }
        size -= bytesread;
        total += bytesread;
        if (bytesread == 0)
            break;
    }
    bytesread = total;

    /*
    **	Close the file if it was opened by this routine and return
    **	the actual number of bytes read into the buffer.
    */
    if (opened)
        Close();
    return (bytesread);
}

/***********************************************************************************************
 * RawFileClass::Write -- Writes the specified data to the buffer specified.                   *
 *                                                                                             *
 *    This routine will write the data specified to the file.                                  *
 *                                                                                             *
 * INPUT:   buffer   -- The buffer that holds the data to write.                               *
 *                                                                                             *
 *          size     -- The number of bytes to write to the file.                              *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes written to the file. This routine catches the     *
 *          case of a disk full condition, so this routine will always return with the number  *
 *          matching the size request.                                                         *
 *                                                                                             *
 * WARNINGS:   A fatal file condition could cause this routine to never return.                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long RawFileClass::Write(void const* buffer, long size)
{
    long byteswritten = 0;
    int opened = false; // Was the file manually opened?

    /*
    **	Check to open status of the file. If the file is open, then merely write to
    **	it. Otherwise, open the file for writing and then close the file when the
    **	output is finished.
    */
    if (!Is_Open()) {
        if (!Open(WRITE)) {
            return (0);
        }
        opened = true;
    }

    clearerr(Handle);
    byteswritten = fwrite(buffer, 1, size, Handle);
    if (ferror(Handle)) {
        Error(errno, false, Filename);
    }

    /*
    **	Fixup the bias length if necessary.
    */
    if (BiasLength != -1) {
        if (Raw_Seek(0) > BiasStart + BiasLength) {
            BiasLength = Raw_Seek(0) - BiasStart;
        }
    }

    /*
    **	If this routine had to open the file, then close it before returning.
    */
    if (opened) {
        Close();
    }

    /*
    **	Return with the number of bytes written. This will always be the number of bytes
    **	requested, since the case of the disk being full is caught by this routine.
    */
    return (byteswritten);
}

/***********************************************************************************************
 * RawFileClass::Seek -- Reposition the file pointer as indicated.                             *
 *                                                                                             *
 *    Use this routine to move the filepointer to the position indicated. It can move either   *
 *    relative to current position or absolute from the beginning or ending of the file. This  *
 *    routine will only return if it successfully performed the seek.                          *
 *                                                                                             *
 * INPUT:   pos   -- The position to seek to. This is interpreted as relative to the position  *
 *                   indicated by the "dir" parameter.                                         *
 *                                                                                             *
 *          dir   -- The relative position to relate the seek to. This can be either SEEK_SET  *
 *                   for the beginning of the file, SEEK_CUR for the current position, or      *
 *                   SEEK_END for the end of the file.                                         *
 *                                                                                             *
 * OUTPUT:  This routine returns the position that the seek ended up at.                       *
 *                                                                                             *
 * WARNINGS:   If there was a file error, then this routine might never return.                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long RawFileClass::Seek(long pos, int dir)
{

    /*
    **	A file that is biased will have a seek operation modified so that the file appears to
    **	exist only within the bias range. All bytes outside of this range appear to be
    **	non-existant.
    */
    if (BiasLength != -1) {
        switch (dir) {
        case SEEK_SET:
            if (pos > BiasLength) {
                pos = BiasLength;
            }
            pos += BiasStart;
            break;

        case SEEK_CUR:
            break;

        case SEEK_END:
            dir = SEEK_SET;
            pos += BiasStart + BiasLength;
            //				pos = (pos <= BiasStart+BiasLength) ? pos : BiasStart+BiasLength;
            //				pos = (pos >= BiasStart) ? pos : BiasStart;
            break;
        }

        /*
        **	Perform the modified raw seek into the file.
        */
        long newpos = Raw_Seek(pos, dir) - BiasStart;

        /*
        **	Perform a final double check to make sure the file position fits with the bias range.
        */
        if (newpos < 0) {
            newpos = Raw_Seek(BiasStart, SEEK_SET) - BiasStart;
        }
        if (newpos > BiasLength) {
            newpos = Raw_Seek(BiasStart + BiasLength, SEEK_SET) - BiasStart;
        }
        return (newpos);
    }

    /*
    **	If the file is not biased in any fashion, then the normal seek logic will
    **	work just fine.
    */
    return (Raw_Seek(pos, dir));
}

/***********************************************************************************************
 * RawFileClass::Size -- Determines size of file (in bytes).                                   *
 *                                                                                             *
 *    Use this routine to determine the size of the file. The file must exist or this is an    *
 *    error condition.                                                                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes in the file.                                      *
 *                                                                                             *
 * WARNINGS:   This routine handles error conditions and will not return unless the file       *
 *             exists and can successfully be queried for file length.                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long RawFileClass::Size(void)
{
    long size = 0;

    /*
    **	A biased file already has its length determined.
    */
    if (BiasLength != -1) {
        return (BiasLength);
    }

    /*
    **	If the file is open, then proceed normally.
    */
    if (Is_Open()) {

        /*
        ** With stdio we seek to end to obtain the length, then reset the position back.
        */
        clearerr(Handle);

        long position = ftell(Handle);
        if (position < 0) {
            Error(errno, false, Filename);
            return 0;
        }

        if (fseek(Handle, 0, SEEK_END) < 0) {
            Error(errno, false, Filename);
            return 0;
        }

        size = ftell(Handle);

        if (fseek(Handle, position, SEEK_SET) < 0) {
            Error(errno, false, Filename);
            return 0;
        }

    } else {

        /*
        **	If the file wasn't open, then open the file and call this routine again. Count on
        **	the fact that the open function must succeed.
        */
        if (Open()) {
            size = Size();

            /*
            **	Since we needed to open the file we must remember to close the file when the
            **	size has been determined.
            */
            Close();
        }
    }

    BiasLength = size - BiasStart;
    return (BiasLength);
}

/***********************************************************************************************
 * RawFileClass::Create -- Creates an empty file.                                              *
 *                                                                                             *
 *    This routine will create an empty file from the file object. The file object's filename  *
 *    must already have been assigned before this routine will function.                       *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Was the file successfully created? This routine will always return true.     *
 *                                                                                             *
 * WARNINGS:   A fatal error condition could occur with this routine. Especially if the disk   *
 *             is full or a read-only media was selected.                                      *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int RawFileClass::Create(void)
{
    Close();
    if (Open(WRITE)) {

        /*
        **	A biased file must be at least as long as the bias offset. Seeking to the
        **	appropriate start offset has the effect of lengthening the file to the
        **	correct length.
        */
        if (BiasLength != -1) {
            Seek(0, SEEK_SET);
        }

        Close();
        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * RawFileClass::Delete -- Deletes the file object from the disk.                              *
 *                                                                                             *
 *    This routine will delete the file object from the disk. If the file object doesn't       *
 *    exist, then this routine will return as if it had succeeded (since the effect is the     *
 *    same).                                                                                   *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Was the file deleted? If the file was already missing, the this value will   *
 *                be false.                                                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int RawFileClass::Delete(void)
{
    /*
    **	If the file was open, then it must be closed first.
    */
    Close();

    /*
    **	If there is no filename associated with this object, then this indicates a fatal error
    **	condition. Report this and abort.
    */
    if (!Filename) {
        Error(ENOENT, false);
    }

    /*
    **	Repetitively try to delete the file if possible. Either return with success, or
    **	abort the program with an error.
    */
    for (;;) {

        /*
        **	If the file is already missing, then return with this fact. No action is necessary.
        **	This can occur as this section loops if the file exists on a floppy and the floppy
        **	was removed, the file deleted on another machine, and then the floppy was
        **	reinserted. Admittedly, this is a rare case, but is handled here.
        */
        if (!Is_Available()) {
            return (false);
        }

        if (_unlink(Filename) < 0) {
            Error(errno, false, Filename);
            return (false);
        }
        break;
    }

    /*
    **	DOS reports that the file was successfully deleted. Return with this fact.
    */
    return (true);
}

/***********************************************************************************************
 * RawFileClass::Bias -- Bias a file with a specific starting position and length.             *
 *                                                                                             *
 *    This will bias a file by giving it an artificial starting position and length. By        *
 *    using this routine, it is possible to 'fool' the file into ignoring a header and         *
 *    trailing extra data. An example of this would be a file inside of a mixfile.             *
 *                                                                                             *
 * INPUT:   start    -- The starting offset that will now be considered the start of the       *
 *                      file.                                                                  *
 *                                                                                             *
 *          length   -- The forced length of the file. For files that are opened for write,    *
 *                      this serves as the artificial constraint on the file's length. For     *
 *                      files opened for read, this limits the usable file size.               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void RawFileClass::Bias(int start, int length)
{
    if (start == 0) {
        BiasStart = 0;
        BiasLength = -1;
        return;
    }

    BiasLength = RawFileClass::Size();
    BiasStart += start;
    if (length != -1) {
        BiasLength = BiasLength < length ? BiasLength : length;
    }
    BiasLength = BiasLength > 0 ? BiasLength : 0;

    /*
    **	Move the current file offset to a legal position if necessary and the
    **	file was open.
    */
    if (Is_Open()) {
        RawFileClass::Seek(0, SEEK_SET);
    }
}

/***********************************************************************************************
 * RawFileClass::Raw_Seek -- Performs a seek on the unbiased file                              *
 *                                                                                             *
 *    This will perform a seek on the file as if it were unbiased. This is in spite of any     *
 *    bias setting the file may have. The ability to perform a raw seek in this fasion is      *
 *    necessary to maintain the bias ability.                                                  *
 *                                                                                             *
 * INPUT:   pos   -- The position to seek the file relative to the "dir" parameter.            *
 *                                                                                             *
 *          dir   -- The origin of the seek operation.                                         *
 *                                                                                             *
 * OUTPUT:  Returns with the new position of the seek operation.                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/04/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
long RawFileClass::Raw_Seek(long pos, int dir)
{
    /*
    **	If the file isn't opened, then this is a fatal error condition.
    */
    if (!Is_Open()) {
        Error(EBADF, false, Filename);
    }

    clearerr(Handle);
    if (fseek(Handle, pos, dir) < 0) {
        Error(errno, false, Filename);
    }

    pos = ftell(Handle);

    /*
    **	Return with the new position of the file. This will range between zero and the number of
    **	bytes the file contains.
    */
    return (pos);
}
