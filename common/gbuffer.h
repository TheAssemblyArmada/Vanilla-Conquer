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
 *                    File Name : GBUFFER.H                                *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 26, 1994                             *
 *                                                                         *
 *                  Last Update : October 9, 1995   []                     *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *  This module contains the definition for the graphic buffer class.  The *
 * primary functionality of the graphic buffer class is handled by inline  *
 * functions that make a call through function pointers to the correct     *
 * routine.  This has two benefits:                                        *
 *                                                                         *
 *                                                                         *
 *      1) C++ name mangling is not a big deal since the function pointers *
 *         point to functions in standard C format.                        *
 *      2) The function pointers can be changed when we set a different    *
 *         graphic mode.  This allows us to have both supervga and mcga    *
 *         routines present in memory at once.                             *
 *                                                                         *
 * In the basic library, these functions point to stub routines which just *
 * return.  This makes a product that just uses a graphic buffer take the  *
 * minimum amount of code space.  For programs that require MCGA or VESA   *
 * support, all that is necessary to do is link either the MCGA or VESA    *
 * specific libraries in, previous to WWLIB32.  The linker will then       *
 * override the necessary stub functions automatically.                    *
 *                                                                         *
 * In addition, there are helpful inline function calls for parameter      *
 * elimination.  This header file gives the definition for all             *
 * GraphicViewPort and GraphicBuffer classes.                              *
 *                                                                         *
 * Terminology:                                                            *
 *                                                                         *
 *  Buffer Class - A class which consists of a pointer to an allocated     *
 *      buffer and the size of the buffer that was allocated.              *
 *                                                                         *
 *  Graphic ViewPort - The Graphic ViewPort defines a window into a        *
 *      Graphic Buffer.  This means that although a Graphic Buffer         *
 *      represents linear memory, this may not be true with a Graphic      *
 *      Viewport.  All low level functions that act directly on a graphic  *
 *      viewport are included within this class.  This includes but is not *
 *      limited to most of the functions which can act on a Video Viewport *
 *      Video Buffer.                                                      *
 *                                                                         *
 * Graphic Buffer - A Graphic Buffer is an instance of an allocated buffer *
 *    used to represent a rectangular region of graphics memory.           *
 *    The HidBuff and BackBuff are excellent examples of a Graphic Buffer. *
 *                                                                         *
 * Below is a tree which shows the relationship of the VideoBuffer and     *
 * Buffer classes to the GraphicBuffer class:                              *
 *                                                                         *
 *   BUFFER.H           GBUFFER.H           BUFFER.H            VBUFFER.H  *
 *  ----------          ----------         ----------          ----------  *
 * |  Buffer  |        | Graphic  |       |  Buffer  |        |  Video   | *
 * |  Class   |        | ViewPort |       |  Class   |        | ViewPort | *
 *  ----------          ----------         ----------          ----------  *
 *            \        /                             \        /            *
 *             \      /                               \      /             *
 *            ----------                             ----------            *
 *           |  Graphic |                           |  Video   |           *
 *           |  Buffer  |                           |  Buffer  |           *
 *            ----------                             ----------            *
 *             GBUFFER.H                              VBUFFER.H            *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   GBC::GraphicBufferClass -- inline constructor for GraphicBufferClass  *
 *   GVPC::Remap -- Short form to remap an entire graphic view port        *
 *   GVPC::Get_XPos -- Returns x offset for a graphic viewport class       *
 *   GVPC::Get_Ypos -- Return y offset in a GraphicViewPortClass           *
 *   VVPC::Get_XPos -- Get the x pos of the VP on the Video                *
 *   VVPC::Get_YPos -- Get the y pos of the VP on the video                *
 *   GBC::Get_Graphic_Buffer -- Get the graphic buffer of the VP.          *
 *   GVPC::Draw_Line -- Stub function to draw line in Graphic Viewport Class*
 *   GVPC::Fill_Rect -- Stub function to fill rectangle in a GVPC          *
 *   GVPC::Remap -- Stub function to remap a GVPC                          *
 *   GVPC::Print -- stub func to print a text string                       *
 *   GVPC::Print -- Stub function to print an integer                      *
 *   GVPC::Print -- Stub function to print a short to a graphic viewport   *
 *   GVPC::Print -- stub function to print a long on a graphic view port   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef GBUFFER_H
#define GBUFFER_H

/*=========================================================================*/
/* If we have not already loaded the standard library header, than we can  */
/*      load it.                                                           */
/*=========================================================================*/

#include "buffer.h"
#include "video.h"
#include "drawbuff.h"
#include "font.h"
#include "graphicsviewport.h"

#include <stdint.h>
#include <stdio.h>

class VideoSurface;

/*=========================================================================*/
/* GraphicBufferClass - A GraphicBuffer refers to an actual instance of an */
/*      allocated buffer.  The GraphicBuffer may be drawn to directly      */
/*      because it inherits a ViewPort which represents its physical size. */
/*                                                                         */
/*          BYTE    *Buffer  -  is the offset to graphic buffer            */
/*          int     Width    -  is the width of graphic buffer             */
/*          int     Height   -  is the height of graphic buffer            */
/*          int     XAdd     -  is the xadd of graphic buffer              */
/*          int     XPos;    -  will be 0 because it is graphicbuff        */
/*          int     YPos;    -  will be 0 because it is graphicbuff        */
/*          long    Pitch    -  modulo of buffer for reading and writing   */
/*          BOOL    IsDirectDraw -  flag if its a direct draw surface      */
/*=========================================================================*/
class GraphicBufferClass : public GraphicViewPortClass, public BufferClass
{

public:
    GraphicBufferClass(int w, int h, GBC_Enum flags);
    GraphicBufferClass(int w, int h, void* buffer, long size);
    GraphicBufferClass(int w, int h, void* buffer = 0);
    GraphicBufferClass();
    ~GraphicBufferClass();

    void DD_Init(GBC_Enum flags);
    void Init(int w, int h, void* buffer, long size, GBC_Enum flags);
    void Un_Init();
    void Attach_DD_Surface(GraphicBufferClass* attach_buffer);
    bool Lock();
    bool Unlock();
    bool IsAllocated() const;

    void Scale_Rotate(BitmapClass& bmp, TPoint2D const& pt, long scale, unsigned char angle);

    // Member to get a pointer to a direct draw surface
    VideoSurface* Get_DD_Surface()
    {
        return VideoSurfacePtr;
    }

protected:
    VideoSurface* VideoSurfacePtr; // Pointer to the related direct draw surface
};

#endif
