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

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood 32 Bit Library                                      *
 *                                                                                             *
 *                    File Name : MOUSE.H                                                      *
 *                                                                                             *
 *                   Programmer : Philip W. Gorrow                                             *
 *                                                                                             *
 *                   Start Date : 12/12/95                                                     *
 *                                                                                             *
 *                  Last Update : December 12, 1995 [PWG]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WW_MOUSE_H
#define WW_MOUSE_H

#include "gbuffer.h"
#include "shape.h"

#pragma pack(push, 1)
typedef struct
{
    uint16_t ShapeType;     // 0 = normal, 1 = 16 colors,
                            //  2 = uncompressed, 4 = 	<16 colors
    uint8_t Height;         // Height of the shape in scan lines
    uint16_t Width;         // Width of the shape in bytes
    uint8_t OriginalHeight; // Original height of shape in scan lines
    uint16_t ShapeSize;     // Size of the shape, including header
    uint16_t DataLength;    // Size of the uncompressed shape (just data)
    uint8_t Data[];         // Cursor data
} Cursor;
#pragma pack(pop)

class WWMouseClass
{
public:
    WWMouseClass(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height);
    ~WWMouseClass();
    void* Set_Cursor(int xhotspot, int yhotspot, void* cursor);
    void Process_Mouse(void);
    void Hide_Mouse(void);
    void Show_Mouse(void);
    void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2);
    void Conditional_Show_Mouse(void);
    int Get_Mouse_State(void);
    int Get_Mouse_X(void);
    int Get_Mouse_Y(void);
    void Get_Mouse_XY(int& x, int& y);
    //
    // The following two routines can be used to render the mouse onto a graphicbuffer
    // other than the hidpage.
    //
    void Draw_Mouse(GraphicViewPortClass* scr);
    void Draw_Mouse(GraphicViewPortClass* viewport, int x, int y);
    void Erase_Mouse(GraphicViewPortClass* scr, int forced = 0);
    void Mouse_Shadow_Buffer(GraphicViewPortClass* viewport, void* buffer, int x, int y, int hotx, int hoty, int store);
    void* Set_Mouse_Cursor(int hotspotx, int hotspoty, Cursor* cursor);

    void Block_Mouse(GraphicBufferClass* buffer);
    void Unblock_Mouse(GraphicBufferClass* buffer);
    void Set_Cursor_Clip(void);
    void Clear_Cursor_Clip(void);

private:
    enum
    {
        CONDHIDE = 1,
        CONDHIDDEN = 2,
    };
    void Low_Hide_Mouse(void);
    void Low_Show_Mouse(int x, int y);

    char* MouseCursor; // pointer to the mouse cursor in memory
    int MouseXHot;     // X hot spot of the current mouse cursor
    int MouseYHot;     // Y hot spot of the current mouse cursor
    int CursorWidth;   // width of the mouse cursor in pixels
    int CursorHeight;  // height of the mouse cursor in pixels

    char* MouseBuffer; // pointer to background buffer in memory
    int MouseBuffX;    // pixel x mouse buffer was preserved at
    int MouseBuffY;    // pixel y mouse buffer was preserved at
    int MaxWidth;      // maximum width of mouse background buffer
    int MaxHeight;     // maximum height of mouse background buffer

    int MouseCXLeft;  // left x pos if conditional hide mouse in effect
    int MouseCYUpper; // upper y pos if conditional hide mouse in effect
    int MouseCXRight; // right x pos if conditional hide mouse in effect
    int MouseCYLower; // lower y pos if conditional hide mouse in effect
    char MCFlags;     // conditional hide mouse flags
    char MCCount;     // nesting count for conditional hide mouse

    GraphicViewPortClass* Screen; // pointer to the surface mouse was init'd with
    Cursor* PrevCursor;           // pointer to previous cursor shape
    int MouseUpdate;
    int State;

    char* EraseBuffer; // Buffer which holds background to restore to hidden page
    int EraseBuffX;    // X position of the hidden page background
    int EraseBuffY;    // Y position of the hidden page background
    int EraseBuffHotX; // X position of the hidden page background
    int EraseBuffHotY; // Y position of the hidden page background

    int EraseFlags; // Records whether mutex has been released

#ifdef _WIN32
    CRITICAL_SECTION MouseCriticalSection; // Control for mouse re-enterancy
    unsigned TimerHandle;
#endif
};

void Hide_Mouse(void);
void Show_Mouse(void);
void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2);
void Conditional_Show_Mouse(void);
int Get_Mouse_State(void);
void* Set_Mouse_Cursor(int hotx, int hoty, void* cursor);
int Get_Mouse_X(void);
int Get_Mouse_Y(void);

#endif
