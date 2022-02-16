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
 *   GBC::Scale_Rotate -- Bilinear scale and rotate a surface              *
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
void GraphicBufferClass::Init(int w, int h, void* buffer, int size, GBC_Enum flags)
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
 * INPUT:      int size     - size of the buffer to create                *
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
GraphicBufferClass::GraphicBufferClass(int w, int h, void* buffer, int size)
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

/***************************************************************************
 * GBC::Scale_Rotate                                                       *
 *                                                                         *
 * Using Bi-Linear Interpolation, draws a scaled and rotated               *
 * bitmap onto the buffer.  No clipping is performed so beware.            *
 *                                                                         *
 * INPUT:      bmp    - bitmap to draw                                     *
 *             pt     - desired position of the center                     *
 *             scale  - 24.8 fixed point scale factor                      *
 *             angle  - 8bit angle (0=0deg, 255=360deg)                    *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 **************************************************************************/
void GraphicBufferClass::Scale_Rotate(BitmapClass& bmp, TPoint2D const& pt, int scale, unsigned char angle)
{
    static int _SineTab[256] = {
        0,    6,    12,   18,   25,   31,   37,   43,   49,   56,   62,   68,   74,   80,   86,   92,   97,   103,
        109,  115,  120,  126,  131,  136,  142,  147,  152,  157,  162,  167,  171,  176,  180,  185,  189,  193,
        197,  201,  205,  209,  212,  216,  219,  222,  225,  228,  231,  233,  236,  238,  240,  243,  244,  246,
        248,  249,  251,  252,  253,  254,  254,  255,  255,  255,  255,  255,  255,  255,  254,  254,  253,  252,
        251,  249,  248,  246,  245,  243,  241,  238,  236,  234,  231,  228,  225,  222,  219,  216,  213,  209,
        205,  201,  198,  194,  189,  185,  181,  176,  172,  167,  162,  157,  152,  147,  142,  137,  131,  126,
        120,  115,  109,  104,  98,   92,   86,   80,   74,   68,   62,   56,   50,   44,   37,   31,   25,   19,
        12,   6,    0,    -5,   -12,  -18,  -24,  -30,  -37,  -43,  -49,  -55,  -61,  -67,  -73,  -79,  -85,  -91,
        -97,  -103, -109, -114, -120, -125, -131, -136, -141, -147, -152, -157, -162, -166, -171, -176, -180, -185,
        -189, -193, -197, -201, -205, -208, -212, -215, -219, -222, -225, -228, -231, -233, -236, -238, -240, -242,
        -244, -246, -248, -249, -250, -252, -253, -254, -254, -255, -255, -255, -255, -255, -255, -255, -254, -254,
        -253, -252, -251, -249, -248, -246, -245, -243, -241, -239, -236, -234, -231, -228, -226, -223, -219, -216,
        -213, -209, -206, -202, -198, -194, -190, -185, -181, -177, -172, -167, -162, -158, -153, -148, -142, -137,
        -132, -126, -121, -115, -110, -104, -98,  -92,  -86,  -81,  -75,  -69,  -62,  -56,  -50,  -44,  -38,  -32,
        -25,  -19,  -13,  -7,
    };

    static int _CosineTab[256] = {
        256,  255,  255,  255,  254,  254,  253,  252,  251,  249,  248,  246,  244,  243,  241,  238,  236,  234,
        231,  228,  225,  222,  219,  216,  212,  209,  205,  201,  197,  193,  189,  185,  181,  176,  171,  167,
        162,  157,  152,  147,  142,  137,  131,  126,  120,  115,  109,  103,  98,   92,   86,   80,   74,   68,
        62,   56,   50,   43,   37,   31,   25,   19,   12,   6,    0,    -6,   -12,  -18,  -24,  -31,  -37,  -43,
        -49,  -55,  -61,  -68,  -74,  -80,  -86,  -91,  -97,  -103, -109, -114, -120, -125, -131, -136, -141, -147,
        -152, -157, -162, -166, -171, -176, -180, -185, -189, -193, -197, -201, -205, -209, -212, -216, -219, -222,
        -225, -228, -231, -233, -236, -238, -240, -242, -244, -246, -248, -249, -251, -252, -253, -254, -254, -255,
        -255, -255, -255, -255, -255, -255, -254, -254, -253, -252, -251, -249, -248, -246, -245, -243, -241, -239,
        -236, -234, -231, -228, -225, -222, -219, -216, -213, -209, -205, -202, -198, -194, -190, -185, -181, -176,
        -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121, -115, -109, -104, -98,  -92,  -86,  -80,
        -74,  -68,  -62,  -56,  -50,  -44,  -38,  -31,  -25,  -19,  -13,  -6,   0,    5,    11,   18,   24,   30,
        36,   43,   49,   55,   61,   67,   73,   79,   85,   91,   97,   103,  108,  114,  120,  125,  131,  136,
        141,  146,  151,  156,  161,  166,  171,  176,  180,  184,  189,  193,  197,  201,  205,  208,  212,  215,
        219,  222,  225,  228,  231,  233,  236,  238,  240,  242,  244,  246,  248,  249,  250,  252,  253,  253,
        254,  255,  255,  255,
    };

    unsigned int scrpos;
    unsigned int temp;

    int i, j;        // counter vars
    int pxerror = 0; // these three vars will be used in an
    int pyerror = 0; // integer difference alg to keep track
    int pixpos = 0;  // of what pixel to draw.
    unsigned char pixel;

    TPoint2D p0; // "upper left" corner of the rectangle
    TPoint2D p1; // "upper right" corner of the rectangle
    TPoint2D p2; // "lower left" corner of the rectangle

    /*-------------------------------------------------
        Compute three corner points of the rectangle
    -------------------------------------------------*/
    {
        angle &= 0x0FF;
        int c = _CosineTab[angle];
        int s = _SineTab[angle];
        int W = (scale * bmp.Width) >> 1;
        int L = (scale * bmp.Height) >> 1;

        p0.x = (pt.x + ((((L * c) >> 8) - ((W * s) >> 8)) >> 8));
        p0.y = (pt.y + (((-(L * s) >> 8) - ((W * c) >> 8)) >> 8));
        p1.x = (pt.x + ((((L * c) >> 8) + ((W * s) >> 8)) >> 8));
        p1.y = (pt.y + (((-(L * s) >> 8) + ((W * c) >> 8)) >> 8));
        p2.x = (pt.x + (((-(L * c) >> 8) - ((W * s) >> 8)) >> 8));
        p2.y = (pt.y + ((((L * s) >> 8) - ((W * c) >> 8)) >> 8));
    }

    /*-----------------------------------
        Initialize Breshnam constants
    -----------------------------------*/

    // This breshnam line goes across the FRONT of the rectangle
    // In the bitmap, this will step from left to right

    int f_deltax = (p1.x - p0.x);
    int f_deltay = (p1.y - p0.y);
    int f_error = 0;
    int f_xstep = 1;
    int f_ystep = Width;

    // This breshnam line goes down the SIDE of the rectangle
    // In the bitmap, this line will step from top to bottom

    int s_deltax = (p2.x - p0.x);
    int s_deltay = (p2.y - p0.y);
    int s_error = 0;
    int s_xstep = 1;
    int s_ystep = Width;

    /*--------------------------------
        fixup deltas and step values
    --------------------------------*/

    if (f_deltay < 0) {
        f_deltay = -f_deltay;
        f_ystep = -Width;
    };

    if (f_deltax < 0) {
        f_deltax = -f_deltax;
        f_xstep = -1;
    };

    if (s_deltay < 0) {
        s_deltay = -s_deltay;
        s_ystep = -Width;
    };

    if (s_deltax < 0) {
        s_deltax = -s_deltax;
        s_xstep = -1;
    };

    scrpos = p0.x + Width * p0.y; // address of initial screen pos.
    temp = scrpos;

    /*---------------------------------------------------------------------
        Now all of the differences, errors, and steps are set up so we can
        begin drawing the bitmap...

        There are two cases here,
        1 - the "Front" line has a slope of <  1.0 (45 degrees)
        2 - the "Front" line has a slope of >= 1.0

        For case 1, we step along the X direction, for case 2, step in y
    ---------------------------------------------------------------------*/

    if (f_deltax > f_deltay) { // CASE 1, step front in X, side in Y

        // outer loop steps from top to bottom of the rectangle
        for (j = 0; j < s_deltay; j++) {
            temp = scrpos;

            // The inner loop steps across the rectangle
            for (i = 0; i < f_deltax; i++) {
                pixel = bmp.Data[pixpos]; // read pixel
                if (pixel)
                    ((unsigned char*)Get_Buffer())[scrpos] = pixel; // draw if not transparent
                //				if (pixel) Data[scrpos]=pixel;	//draw if not transparent
                pxerror += bmp.Width; // update position in bitmap
                while (pxerror > f_deltax) {
                    pixpos++;
                    pxerror -= f_deltax;
                };
                scrpos += f_xstep; // step to next screen pos
                f_error += f_deltay;
                if (f_error > f_deltax) {
                    if (pixel)
                        ((unsigned char*)Get_Buffer())[scrpos] = pixel;
                    f_error -= f_deltax;
                    scrpos += f_ystep;
                };
            };
            pxerror = 0;
            pixpos -= bmp.Width - 1;
            pyerror += bmp.Height;

            while (pyerror > s_deltay) {
                pixpos += bmp.Width;
                pyerror -= s_deltay;
            };

            f_error = 0;
            scrpos = temp;
            scrpos += s_ystep;
            s_error += s_deltax;

            if (s_error > s_deltay) {
                s_error -= s_deltay;
                scrpos += s_xstep;
            };
        }

    } else { // CASE 2, Step front line in X, side line in Y

        // outer loop steps from top to bottom of the rectangle
        for (j = 0; j < s_deltax; j++) {
            temp = scrpos;

            // The inner loop steps across the rectangle
            for (i = 0; i < f_deltay; i++) {
                pixel = bmp.Data[pixpos]; // read pixel
                if (pixel)
                    ((unsigned char*)Get_Buffer())[scrpos] = pixel; // draw if not transparent
                pxerror += bmp.Width;                               // update position in bitmap
                while (pxerror > f_deltay) {
                    pixpos++;
                    pxerror -= f_deltay;
                };

                scrpos += f_ystep; // step to next screen pos
                f_error += f_deltax;
                if (f_error > f_deltay) {
                    if (pixel)
                        ((unsigned char*)Get_Buffer())[scrpos] = pixel;
                    f_error -= f_deltay;
                    scrpos += f_xstep;
                };
            };

            pxerror = 0;
            pixpos -= bmp.Width - 1;
            pyerror += bmp.Height;
            while (pyerror > s_deltax) {
                pixpos += bmp.Width;
                pyerror -= s_deltax;
            };

            scrpos = temp;
            scrpos += s_xstep;
            s_error += s_deltay;
            f_error = 0;
            if (s_error > s_deltax) {
                s_error -= s_deltax;
                scrpos += s_ystep;
            };
        }
    }
}
