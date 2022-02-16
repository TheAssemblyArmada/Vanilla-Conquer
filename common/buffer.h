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
 *                 Project Name : Westwood 32 Bit Library						*
 *                                                                         *
 *                    File Name : GBUFFER.H                                *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 26, 1994                             *
 *                                                                         *
 *                  Last Update : July 5, 1994   [PWG]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   BC::Get_Size -- Returns the buffer size of the BufferClass instance   *
 *   BC::Get_Buffer -- Returns pointer to buffer inherent to BufferClass 	*
 *   BC::BufferClass -- inline constructor for BufferClass with size only  *
 *   BC::To_Page -- Copys a buffer class to a page with definable x, y, w, h*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef BUFFER_H
#define BUFFER_H

/*=========================================================================*/
/* If we have not already loaded the standard library header, than we can	*/
/*		load it.																					*/
/*=========================================================================*/
#include "wwstd.h"

class GraphicViewPortClass;

/*=========================================================================*/
/* BufferClass - A base class which holds buffer information including a	*/
/*		pointer and the size of the buffer.												*/
/*=========================================================================*/
class BufferClass
{
public:
    /*===================================================================*/
    /* Define the base constructor and destructors for the class			*/
    /*===================================================================*/
    BufferClass(void* ptr, int size);
    BufferClass(int size);
    BufferClass();
    ~BufferClass();
    /*===================================================================*/
    /* Define functions which work with the buffer class.						*/
    /*===================================================================*/
    int To_Page(GraphicViewPortClass& view);
    int To_Page(int w, int h, GraphicViewPortClass& view);
    int To_Page(int x, int y, int w, int h, GraphicViewPortClass& view);

    /*===================================================================*/
    /* define functions to get at the protected data members					*/
    /*===================================================================*/
    void* Get_Buffer(void);
    int Get_Size(void);

private:
    /*===================================================================*/
    /* Define the operators we do not want to happen which are the copy	*/
    /* and equal constructors.  These are bad because the Allocated flag	*/
    /*	could be copied and the associated buffer freed.  If this were to	*/
    /*	gappen it could cause weird general protection fault.					*/
    /*===================================================================*/
    BufferClass(BufferClass const&);
    BufferClass& operator=(BufferClass const&);

protected:
    void* Buffer;
    int Size;
    bool Allocated;
};
/***************************************************************************
 * BC::GET_SIZE -- Returns the buffer size of the BufferClass instance     *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     int the size of the buffer                                 *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int BufferClass::Get_Size(void)
{
    return (Size);
}
/***************************************************************************
 * BC::GET_BUFFER -- Returns pointer to buffer inherent to BufferClass 		*
 *                                                                         *
 * INPUT:			none                                                     *
 *                                                                         *
 * OUTPUT:        void * to the inherent buffer.                           *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline void* BufferClass::Get_Buffer(void)
{
    return (Buffer);
}
#endif
