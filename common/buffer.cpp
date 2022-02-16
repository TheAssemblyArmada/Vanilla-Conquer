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
 *                 Project Name : Westwood 32 Bit Library                  *
 *                                                                         *
 *                    File Name : BUFFER.CPP                               *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 18, 1994                             *
 *                                                                         *
 *                  Last Update : June 1, 1994   [PWG]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   BC::BufferClass -- The default (void) constructor for a buffer class  *
 *   BC::~BufferClass -- The destructor for the buffer class               *
 *   BC::BufferClass -- The standard constructor for a buffer class        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "buffer.h"

#include "graphicsviewport.h"
#include "drawbuff.h"

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/***************************************************************************
 * BC::BufferClass -- The standard constructor for a buffer class          *
 *                                                                         *
 * INPUT:      VOID *   buffer to which should be included in buffer class *
 *             LONG     size of the buffer which we included               *
 *                                                                         *
 * OUTPUT:     NONE                                                        *
 *                                                                         *
 * WARNINGS:   If the buffer passed to this function is equal to NULL,     *
 *                  the buffer will be allocated using new.                *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/01/1994 PWG : Created.                                             *
 *=========================================================================*/
BufferClass::BufferClass(void* buffer, int size)
{
    Size = size; // find size of physical buffer

    if (buffer) {                        // if buffer is specified
        Buffer = (unsigned char*)buffer; //		point to it and mark
        Allocated = false;               //		it as user allocated
    } else {
        Buffer = new unsigned char[Size]; // otherwise allocate it and
        Allocated = true;                 //		mark it system alloced
    }
}

/***************************************************************************
 * BC::BufferClass -- constructor for BufferClass with size only    			*
 *                                                                         *
 * INPUT:		LONG the size of the buffer that needs to be allocated		*
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/01/1994 PWG : Created.                                             *
 *=========================================================================*/
BufferClass::BufferClass(int size)
{
    Size = size;
    Buffer = new unsigned char[Size]; // otherwise allocate it and
    Allocated = true;                 //		mark it system alloced
}

/***************************************************************************
 * BC::BufferClass -- The default (void) constructor for a buffer class    *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * NOTES:   	The primary function of this class is to be called by a     *
 *					derived class which will fill in the values after the			*
 *					fact.																			*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/01/1994 PWG : Created.                                             *
 *=========================================================================*/
BufferClass::BufferClass()
{
    Buffer = nullptr;
    Size = 0;
    Allocated = false;
}

/***************************************************************************
 * BC::~BUFFERCLASS -- The destructor for the buffer class                 *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/01/1994 PWG : Created.                                             *
 *=========================================================================*/
BufferClass::~BufferClass()
{
    if (Allocated) {
        delete[] static_cast<unsigned char*>(Buffer);
    }
}

/***************************************************************************
 * BUFFER_TO_PAGE -- Generic 'c' callable form of Buffer_To_Page           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/12/1995 PWG : Created.                                             *
 *=========================================================================*/
int Buffer_To_Page(int x, int y, int w, int h, void* Buffer, GraphicViewPortClass& view)
{
    int return_code = 0;
    if (view.Lock()) {
        return_code = (Buffer_To_Page(x, y, w, h, Buffer, &view));
        view.Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable w, h       *
 *                                                                         *
 * INPUT:      int   width   - the width of copy region                    *
 *             int   height  - the height of copy region                   *
 *             GVPC& dest    - virtual viewport to copy to                 *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * WARNINGS:   x and y position are the upper left corner of the dest      *
 *                     viewport                                            *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
int BufferClass::To_Page(int w, int h, GraphicViewPortClass& view)
{
    int return_code = 0;
    if (view.Lock()) {
        return_code = (Buffer_To_Page(0, 0, w, h, Buffer, &view));
        view.Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable w, h       *
 *                                                                         *
 * INPUT:      GVPC&    dest  - virtual viewport to copy to                *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * WARNINGS:   x and y position are the upper left corner of the dest      *
 *                      viewport.  width and height are assumed to be the  *
 *                      viewport's width and height.                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
int BufferClass::To_Page(GraphicViewPortClass& view)
{
    int return_code = 0;
    if (view.Lock()) {
        return_code = (Buffer_To_Page(0, 0, view.Get_Width(), view.Get_Height(), Buffer, &view));
        view.Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable x, y, w, h *
 *                                                                         *
 * INPUT:   int    x        - x pixel on viewport to copy from             *
 *          int    y        - y pixel on viewport to copy from             *
 *          int    width    - the width of copy region                     *
 *          int    height   - the height of copy region                    *
 *          GVPC&  dest     - virtual viewport to copy to                  *
 *                                                                         *
 * OUTPUT:  none                                                           *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
int BufferClass::To_Page(int x, int y, int w, int h, GraphicViewPortClass& view)
{
    int return_code = 0;
    if (view.Lock()) {
        return_code = (Buffer_To_Page(x, y, w, h, Buffer, &view));
        view.Unlock();
    }
    return (return_code);
}
