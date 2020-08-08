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
 * overide the the necessary stub functions automatically.                 *
 *                                                                         *
 * In addition, there are helpful inline function calls for parameter      *
 * ellimination.  This header file gives the defintion for all             *
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

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <stdint.h>

/*
** Pointer to function to call if we detect a focus loss
*/
extern void (*Gbuffer_Focus_Loss_Function)(void);

class BitmapClass
{
public:
    BitmapClass(int w, int h, unsigned char* data)
        : Width(w)
        , Height(h)
        , Data(data){};

    int Width;
    int Height;
    unsigned char* Data;
};

class TPoint2D
{
public:
    TPoint2D(int xx, int yy)
        : x(xx)
        , y(yy){};
    TPoint2D(void)
        : x(0)
        , y(0){};

    int x;
    int y;
};

#define NOT_LOCKED 0

/*=========================================================================*/
/* Define the screen width and height to make portability to other modules */
/*      easier.                                                            */
/*=========================================================================*/
#define DEFAULT_SCREEN_WIDTH  768
#define DEFAULT_SCREEN_HEIGHT 768

/*=========================================================================*/
/* Let the compiler know that a GraphicBufferClass exists so that it can   */
/*      keep a pointer to it in a VideoViewPortClass.                      */
/*=========================================================================*/
class GraphicViewPortClass;
class GraphicBufferClass;
class VideoViewPortClass;
class VideoBufferClass;
class BufferClass;

GraphicViewPortClass* Set_Logic_Page(GraphicViewPortClass* ptr);
GraphicViewPortClass* Set_Logic_Page(GraphicViewPortClass& ptr);

/*=========================================================================*/
/* GraphicViewPortClass - Holds viewport information on a viewport which   */
/*    has been attached to a GraphicBuffer.  A viewport is effectively a   */
/*    rectangular subset of the full buffer which is used for clipping and */
/*    the like.                                                            */
/*                                                                         */
/*          char    *Buffer -  is the offset to view port buffer           */
/*          int     Width   -  is the width of view port                   */
/*          int     Height  -  is the height of view port                  */
/*          int     XAdd    -  is add value to go from the end of a line   */
/*                             to the beginning of the next line           */
/*          int     XPos;   -  x offset into its associated VideoBuffer    */
/*          int     YPos;   -  y offset into its associated VideoBuffer    */
/*=========================================================================*/
class GraphicViewPortClass
{
public:
    /*===================================================================*/
    /* Define the base constructor and destructors for the class         */
    /*===================================================================*/
    GraphicViewPortClass(GraphicBufferClass* graphic_buff, int x, int y, int w, int h);
    GraphicViewPortClass();
    ~GraphicViewPortClass();

    /*===================================================================*/
    /* define functions to get at the private data members               */
    /*===================================================================*/
    long Get_Offset()
    {
        return Offset;
    }
    int Get_Height()
    {
        return Height;
    }
    int Get_Width()
    {
        return Width;
    }
    int Get_XAdd()
    {
        return XAdd;
    }
    int Get_XPos()
    {
        return XPos;
    }
    int Get_YPos()
    {
        return YPos;
    }
    int Get_Pitch()
    {
        return Pitch;
    }
    bool Get_IsDirectDraw()
    {
        return IsHardware;
    }
    GraphicBufferClass* Get_Graphic_Buffer()
    {
        return GraphicBuff;
    }

    /*===================================================================*/
    /* Define a function which allows us to change a video viewport on   */
    /*      the fly.                                                     */
    /*===================================================================*/
    bool Change(int x, int y, int w, int h);

    /*===================================================================*/
    /* Define the set of common graphic functions that are supported by  */
    /*      both Graphic ViewPorts and VideoViewPorts.                   */
    /*===================================================================*/
    void Put_Pixel(int x, int y, unsigned char color);
    int Get_Pixel(int x, int y);
    void Clear(unsigned char color = 0);
    long To_Buffer(int x, int y, int w, int h, void* buff, long size);
    long To_Buffer(int x, int y, int w, int h, BufferClass* buff);
    long To_Buffer(BufferClass* buff);
    bool Blit(GraphicViewPortClass& dest,
              int x_pixel,
              int y_pixel,
              int dx_pixel,
              int dy_pixel,
              int pixel_width,
              int pixel_height,
              bool trans = false);
    bool Blit(GraphicViewPortClass& dest, int dx, int dy, bool trans = false);
    bool Blit(GraphicViewPortClass& dest, bool trans = false);
    void Blit(void* buffer, int x, int y, int w, int h);
    bool Scale(GraphicViewPortClass& dest,
               int src_x,
               int src_y,
               int dst_x,
               int dst_y,
               int src_w,
               int src_h,
               int dst_w,
               int dst_h,
               bool trans = false,
               char* remap = nullptr);
    bool Scale(GraphicViewPortClass& dest,
               int src_x,
               int src_y,
               int dst_x,
               int dst_y,
               int src_w,
               int src_h,
               int dst_w,
               int dst_h,
               char* remap);
    bool Scale(GraphicViewPortClass& dest, bool trans = false, char* remap = nullptr);
    bool Scale(GraphicViewPortClass& dest, char* remap);
    unsigned long Print(char const* string, int x_pixel, int y_pixel, int fcolor, int bcolor);
    unsigned long Print(short num, int x_pixel, int y_pixel, int fcol, int bcol);
    unsigned long Print(int num, int x_pixel, int y_pixel, int fcol, int bcol);
    unsigned long Print(long num, int x_pixel, int y_pixel, int fcol, int bcol);

    /*===================================================================*/
    /* Define the list of graphic functions which work only with a       */
    /*      graphic buffer.                                              */
    /*===================================================================*/
    void Draw_Line(int sx, int sy, int dx, int dy, unsigned char color);
    void Draw_Rect(int sx, int sy, int dx, int dy, unsigned char color);
    void Fill_Rect(int sx, int sy, int dx, int dy, unsigned char color);
    void Remap(int sx, int sy, int width, int height, void* remap);
    void Remap(void* remap);
    void Draw_Stamp(void const* icondata, int icon, int x_pixel, int y_pixel, void const* remap, int clip_window);

    // PG_TO_FIX
    // This seems to be missing. Might not be needed anyway since it looks like it's only used for UI drawing. ST -
    // 12/17/2018 6:11PM
    void Texture_Fill_Rect(int xpos,
                           int ypos,
                           int width,
                           int height,
                           void const* shape_pointer,
                           int source_width,
                           int source_height)
    {
        return;
    }

    // This doesnt seem to exist anywhere?? - Steve T 9/26/95 6:05PM
    //      VOID Grey_Out_Region(int x, int y, int width, int height, int color);

    //
    // New members to lock and unlock the direct draw video memory
    //
    bool Lock();
    bool Unlock();
    int Get_LockCount()
    {
        return LockCount;
    }

    // Member to blit using direct draw access to hardware blitter
    bool DD_Linear_Blit_To_Linear(GraphicViewPortClass& dest,
                                  int source_x,
                                  int source_y,
                                  int dest_x,
                                  int dest_y,
                                  int width,
                                  int height,
                                  bool mask);

    /*===================================================================*/
    /* Define functions to attach the viewport to a graphicbuffer        */
    /*===================================================================*/
    void Attach(GraphicBufferClass* graphic_buff, int x, int y, int w, int h);

protected:
    /*===================================================================*/
    /* Define the data used by a GraphicViewPortClass                    */
    /*===================================================================*/
    intptr_t Offset;                 // offset to graphic page
    int Width;                       // width of graphic page
    int Height;                      // height of graphic page
    int XAdd;                        // xadd for graphic page (0)
    int XPos;                        // x offset in relation to graphicbuff
    int YPos;                        // y offset in relation to graphicbuff
    long Pitch;                      // Distance from one line to the next
    GraphicBufferClass* GraphicBuff; // related graphic buff
    bool IsHardware;                 // Flag to let us know if it is a direct draw surface
    int LockCount;                   // Count for stacking locks if non-zero the buffer
};                                   //   is a locked DD surface

extern GraphicViewPortClass* LogicPage;

/*=========================================================================*/
/* The following BufferClass functions are defined here because they act   */
/*      on graphic viewports.                                              */
/*=========================================================================*/

long Buffer_To_Page(int x, int y, int w, int h, void* Buffer, GraphicViewPortClass& view);

#endif // VIEWPORT_H
