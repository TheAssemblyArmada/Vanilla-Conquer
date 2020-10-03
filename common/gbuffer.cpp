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
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : GBUFFER.CPP                              *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 3, 1994                              *
 *                                                                         *
 *                  Last Update : October 9, 1995   []                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   VVPC::VirtualViewPort -- Default constructor for a virtual viewport   *
 *   VVPC:~VirtualViewPortClass -- Destructor for a virtual viewport       *
 *   VVPC::Clear -- Clears a graphic page to correct color                 *
 *   VBC::VideoBufferClass -- Lowlevel constructor for video buffer class  *
 *   GVPC::Change -- Changes position and size of a Graphic View Port      *
 *   VVPC::Change -- Changes position and size of a Video View Port        *
 *   Set_Logic_Page -- Sets LogicPage to new buffer                        *
 *   GBC::DD_Init -- Inits a direct draw surface for a GBC                 *
 *   GBC::Init -- Core function responsible for initing a GBC              *
 *   GBC::Lock -- Locks a Direct Draw Surface                              *
 *   GBC::Unlock -- Unlocks a direct draw surface                          *
 *   GBC::GraphicBufferClass -- Default constructor (requires explicit init)*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "gbuffer.h"

int TotalLocks = 0;
extern bool GameInFocus;

extern void Block_Mouse(GraphicBufferClass* buffer);
extern void Unblock_Mouse(GraphicBufferClass* buffer);

/***************************************************************************
 * GBC::DD_INIT -- Inits a direct draw surface for a GBC                   *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *=========================================================================*/
void GraphicBufferClass::DD_Init(GBC_Enum flags)
{
    VideoSurfacePtr = Video::Shared().CreateSurface(Width, Height, flags);

    Allocated = false;   // even if system alloced, dont flag it cuz
                         //   we dont want it freed.
    IsHardware = true;   // flag it as a video surface
    Offset = NOT_LOCKED; // flag it as unavailable for reading or writing
    LockCount = 0;       //  surface is not locked
}

void GraphicBufferClass::Attach_DD_Surface(GraphicBufferClass* attach_buffer)
{
    VideoSurfacePtr->AddAttachedSurface(attach_buffer->Get_DD_Surface());
}

/***************************************************************************
 * GBC::INIT -- Core function responsible for initing a GBC                *
 *                                                                         *
 * INPUT:      int      - the width in pixels of the GraphicBufferClass    *
 *             int      - the heigh in pixels of the GraphicBufferClass    *
 *             void *   - pointer to user supplied buffer (system will     *
 *                        allocate space if buffer is NULL)                *
 *             long     - size of the user provided buffer                 *
 *             GBC_Enum - flags if this is defined as a direct draw        *
 *                        surface                                          *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *=========================================================================*/
void GraphicBufferClass::Init(int w, int h, void* buffer, long size, GBC_Enum flags)
{
    Size = size; // find size of physical buffer
    Width = w;   // Record width of Buffer
    Height = h;  // Record height of Buffer

    //
    // If the surface we are creating is a direct draw object then
    //   we need to do a direct draw init.  Otherwise we will do
    //   a normal alloc.
    //
    if (flags & (GBC_VIDEOMEM | GBC_VISIBLE)) {
        DD_Init(flags);
    } else {
        if (buffer) {          // if buffer is specified
            Buffer = buffer;   //      point to it and mark
            Allocated = false; //      it as user allocated
        } else {
            if (!Size)
                Size = w * h;
            Buffer = new char[Size]; // otherwise allocate it and
            Allocated = true;        //     mark it system alloced
        }
        Offset = (intptr_t)Buffer; // Get offset to the buffer
        IsHardware = false;
    }

    Pitch = 0;          // Record width of Buffer
    XAdd = 0;           // Record XAdd of Buffer
    XPos = 0;           // Record XPos of Buffer
    YPos = 0;           // Record YPos of Buffer
    GraphicBuff = this; // Get a pointer to our self
}

/***********************************************************************************************
 * GBC::Un_Init -- releases the video surface belonging to this gbuffer                        *
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
 *    6/6/96 12:44PM ST : Created                                                              *
 *=============================================================================================*/

void GraphicBufferClass::Un_Init()
{
    if (IsHardware) {
        if (VideoSurfacePtr != nullptr) {
            while (LockCount) {
                Unlock();
            }

            delete VideoSurfacePtr;
            VideoSurfacePtr = nullptr;
        }
    }
}

/***************************************************************************
 * GBC::GRAPHICBUFFERCLASS -- Default constructor (requires explicit init) *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::GraphicBufferClass()
    : VideoSurfacePtr(nullptr)
{
    GraphicBuff = this; // Get a pointer to our self
}

/***************************************************************************
 * GBC::GRAPHICBUFFERCLASS -- Constructor for fixed size buffers           *
 *                                                                         *
 * INPUT:      long size     - size of the buffer to create                *
 *             int w         - width of buffer in pixels (default = 320)   *
 *             int h         - height of buffer in pixels (default = 200)  *
 *             void *buffer  - a pointer to the buffer if any (optional)   *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/13/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::GraphicBufferClass(int w, int h, void* buffer, long size)
    : VideoSurfacePtr(nullptr)
{
    Init(w, h, buffer, size, GBC_NONE);
}
/*=========================================================================*
 * GBC::GRAPHICBUFFERCLASS -- inline constructor for GraphicBufferClass    *
 *                                                                         *
 * INPUT:      int w         - width of buffer in pixels (default = 320)   *
 *             int h         - height of buffer in pixels (default = 200)  *
 *             void *buffer  - a pointer to the buffer if any (optional)   *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::GraphicBufferClass(int w, int h, void* buffer)
    : VideoSurfacePtr(nullptr)
{
    Init(w, h, buffer, w * h, GBC_NONE);
}

/*====================================================================================*
 * GBC::GRAPHICBUFFERCLASS -- contructor for GraphicsBufferClass with special flags   *
 *                                                                                    *
 * INPUT:        int w          - width of buffer in pixels (default = 320)           *
 *               int h          - height of buffer in pixels (default = 200)          *
 *               void *buffer   - unused                                              *
 *               unsigned flags - flags for creation of special buffer types          *
 *                                GBC_VISIBLE - buffer is a visible screen surface    *
 *                                GBC_VIDEOMEM - buffer resides in video memory       *
 *                                                                                    *
 * OUTPUT:     none                                                                   *
 *                                                                                    *
 * HISTORY:                                                                           *
 *   09-21-95 04:19pm ST : Created                                                    *
 *====================================================================================*/
GraphicBufferClass::GraphicBufferClass(int w, int h, GBC_Enum flags)
    : VideoSurfacePtr(nullptr)
{
    Init(w, h, NULL, w * h, flags);
}

/*=========================================================================*
 * GBC::~GRAPHICBUFFERCLASS -- Destructor for the graphic buffer class     *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::~GraphicBufferClass()
{

    //
    // Release the direct draw surface if it exists
    //
    Un_Init();
}

bool GraphicBufferClass::Lock()
{
    //
    // If its not a direct draw surface then the lock is always sucessful.
    //
    if (!IsHardware) {
        return (Offset != 0);
    }

    /*
    ** If the video surface pointer is null then return
    */
    if (VideoSurfacePtr == nullptr) {
        return false;
    }

    /*
    ** If we dont have focus then return failure
    */
    if (!GameInFocus) {
        return false;
    }

    //
    // If surface is already locked then inc the lock count and return true
    //
    if (LockCount) {
        LockCount++;
        return true;
    }

    //
    // If it isn't locked at all then we will have to request that Direct
    // Draw actually lock the surface.
    //

    if (VideoSurfacePtr == nullptr) {
        return false;
    }

    Block_Mouse(this);

    bool result = false;

    if (VideoSurfacePtr->LockWait()) {
        Offset = (intptr_t)VideoSurfacePtr->GetData();
        Pitch = VideoSurfacePtr->GetPitch();
        Pitch -= Width;
        LockCount++;  // increment count so we can track if
        TotalLocks++; // Total number of times we have locked (for debugging)
        result = true;
    }

    Unblock_Mouse(this);

    return result; // Return false because we couldnt lock or restore the surface
}

/***************************************************************************
 * GBC::UNLOCK -- Unlocks a direct draw surface                            *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *   10/09/1995     : Code stolen from Steve Tall                          *
 *=========================================================================*/

bool GraphicBufferClass::Unlock()
{
    //
    // If there is no lock count or this is not a direct draw surface
    // then just return true as there is no harm done.
    //
    if (!(LockCount && IsHardware)) {
        return true;
    }

    //
    // If lock count is directly equal to one then we actually need to
    // unlock so just give it a shot.
    //
    if (LockCount == 1 && VideoSurfacePtr) {
        Block_Mouse(this);
        if (VideoSurfacePtr->Unlock()) {
            Offset = NOT_LOCKED;
            LockCount--;
            Unblock_Mouse(this);
            return true;
        } else {
            Unblock_Mouse(this);
            return false;
        }
    }
    // Colour_Debug (0);
    LockCount--;
    return true;
}

bool GraphicBufferClass::IsAllocated() const
{
    if (VideoSurfacePtr == nullptr) {
        return false;
    }

    return VideoSurfacePtr->IsAllocated();
}
