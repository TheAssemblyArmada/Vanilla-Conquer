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
 *                 Project Name : Westwood 32 bit Library                                      *
 *                                                                                             *
 *                    File Name : MOUSE.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Philip W. Gorrow                                             *
 *                                                                                             *
 *                   Start Date : 12/12/95                                                     *
 *                                                                                             *
 *                  Last Update : December 12, 1995 [PWG]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WWMouseClass::WWMouseClass -- Constructor for the Mouse Class                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "wwmouse.h"

#include "graphicsviewport.h"
#include "rect.h"
#include "wwstd.h"

static WWMouseClass* _Mouse = nullptr;

/***********************************************************************************************
 * MOUSECLASS::MOUSECLASS -- Constructor for the Mouse Class                                   *
 *                                                                                             *
 * INPUT:      GraphicViewPortClass * screen - pointer to screen mouse is created for          *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/12/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
WWMouseClass::WWMouseClass(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
{
    MouseCursor = new char[mouse_max_width * mouse_max_height];
    MouseXHot = 0;
    MouseYHot = 0;
    CursorWidth = 0;
    CursorHeight = 0;

    MouseBuffer = new char[mouse_max_width * mouse_max_height];
    MouseBuffX = -1;
    MouseBuffY = -1;
    MaxWidth = mouse_max_width;
    MaxHeight = mouse_max_height;

    MouseCXLeft = 0;
    MouseCYUpper = 0;
    MouseCXRight = 0;
    MouseCYLower = 0;
    MCFlags = 0;
    MCCount = 0;

    Screen = scr;
    PrevCursor = nullptr;
    MouseUpdate = 0;
    State = 1;

    //
    // Install the timer callback event handler
    //

    EraseBuffer = new char[mouse_max_width * mouse_max_height];
    EraseBuffX = -1;
    EraseBuffY = -1;
    EraseBuffHotX = -1;
    EraseBuffHotY = -1;
    EraseFlags = false;

    _Mouse = this;
}

WWMouseClass::~WWMouseClass()
{
    _Mouse = nullptr;

    MouseUpdate++;

    if (MouseCursor)
        delete[] MouseCursor;
    if (MouseBuffer)
        delete[] MouseBuffer;
}

void Block_Mouse(GraphicBufferClass* buffer)
{
    if (_Mouse) {
        _Mouse->Block_Mouse(buffer);
    }
}

void Unblock_Mouse(GraphicBufferClass* buffer)
{
    if (_Mouse) {
        _Mouse->Unblock_Mouse(buffer);
    }
}

void WWMouseClass::Block_Mouse(GraphicBufferClass* buffer)
{
    if (buffer == Screen->Get_Graphic_Buffer()) {
        Block();
    }
}

void WWMouseClass::Unblock_Mouse(GraphicBufferClass* buffer)
{
    if (buffer == Screen->Get_Graphic_Buffer()) {
        Unblock();
    }
}

void WWMouseClass::Set_Cursor_Clip(void)
{
    if (Screen) {
        Rect rect(0, 0, Screen->Get_Width(), Screen->Get_Height());
        Clip(rect);
    }
}

void WWMouseClass::Clear_Cursor_Clip(void)
{
    Rect rect(0, 0, -1, -1);
    Clip(rect);
}

void WWMouseClass::Hide_Mouse()
{
    MouseUpdate++;
    Low_Hide_Mouse();
    MouseUpdate--;
}

void WWMouseClass::Show_Mouse()
{
    int x = 0;
    int y = 0;
    GetPosition(x, y);

    MouseUpdate++;
    Low_Show_Mouse(x, y);
    MouseUpdate--;
}

void WWMouseClass::Conditional_Hide_Mouse(int x1, int y1, int x2, int y2)
{
    MouseUpdate++;

    //
    // First of all, adjust all the coordinates so that they handle
    // the fact that the hotspot is not necessarily the upper left
    // corner of the mouse.
    //
    x1 -= (CursorWidth - MouseXHot);
    x1 = MAX(0, x1);
    y1 -= (CursorHeight - MouseYHot);
    y1 = MAX(0, y1);
    x2 += MouseXHot;
    x2 = MIN(x2, Screen->Get_Width());
    y2 += MouseYHot;
    y2 = MIN(y2, Screen->Get_Height());

    // The mouse could be in one of four conditions.
    // 1) The mouse is visible and no conditional hide has been specified.
    //    (perform normal region checking with possible hide)
    // 2) The mouse is hidden and no conditional hide as been specified.
    //    (record region and do nothing)
    // 3) The mouse is visible and a conditional region has been specified
    //    (expand region and perform check with possible hide).
    // 4) The mouse is already hidden by a previous conditional.
    //    (expand region and do nothing)
    //
    // First: Set or expand the region according to the specified parameters
    if (!MCCount) {
        MouseCXLeft = x1;
        MouseCYUpper = y1;
        MouseCXRight = x2;
        MouseCYLower = y2;
    } else {
        MouseCXLeft = MIN(x1, MouseCXLeft);
        MouseCYUpper = MIN(y1, MouseCYUpper);
        MouseCXRight = MAX(x2, MouseCXRight);
        MouseCYLower = MAX(y2, MouseCYLower);
    }
    //
    // If the mouse isn't already hidden, then check its location against
    // the hiding region and hide if necessary.
    //
    if (!(MCFlags & CONDHIDDEN)) {
        if (MouseBuffX >= MouseCXLeft && MouseBuffX <= MouseCXRight && MouseBuffY >= MouseCYUpper
            && MouseBuffY <= MouseCYLower) {
            Low_Hide_Mouse();
            MCFlags |= CONDHIDDEN;
        }
    }
    //
    // Record the fact that a conditional hide was called and then exit
    //
    //
    MCFlags |= CONDHIDE;
    MCCount++;
    MouseUpdate--;
}

void WWMouseClass::Conditional_Show_Mouse(void)
{
    MouseUpdate++;

    //
    // if there are any nested hides then dec the count
    //
    if (MCCount) {
        MCCount--;
        //
        // If the mouse is now not hidden and it had actually been
        // hidden before then display it.
        //
        if (!MCCount) {
            if (MCFlags & CONDHIDDEN) {
                Show_Mouse();
            }
            MCFlags = 0;
        }
    }

    MouseUpdate--;
}

int WWMouseClass::Get_Mouse_State()
{
    return State;
}

/***********************************************************************************************
 * WWKeyboardClass::Get_Mouse_X -- Returns the mouses current x position in pixels             *
 *                                                                                             *
 * INPUT:      none                                                                            *
 *                                                                                             *
 * OUTPUT:     int      - returns the mouses current x position in pixels                      *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int WWMouseClass::Get_Mouse_X(void)
{
    int x = 0;
    int y = 0;
    GetPosition(x, y);
    return x;
}

/***********************************************************************************************
 * WWKeyboardClass::Get_Mouse_Y -- returns the mouses current y position in pixels             *
 *                                                                                             *
 * INPUT:      none                                                                            *
 *                                                                                             *
 * OUTPUT:     int      - returns the mouses current y position in pixels                      *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int WWMouseClass::Get_Mouse_Y(void)
{
    int x = 0;
    int y = 0;
    GetPosition(x, y);
    return y;
}

/***********************************************************************************************
 * WWKeyboardClass::Get_Mouse_XY -- Returns the mouses x,y position via reference vars         *
 *                                                                                             *
 * INPUT:      int &x      - variable to return the mouses x position in pixels                *
 *             int &y      - variable to return the mouses y position in pixels                *
 *                                                                                             *
 * OUTPUT:     none - output is via reference variables                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void WWMouseClass::Get_Mouse_XY(int& x, int& y)
{
    GetPosition(x, y);
}

void Hide_Mouse(void)
{
    if (!_Mouse)
        return;
    _Mouse->Hide_Mouse();
}

void Show_Mouse(void)
{
    if (!_Mouse)
        return;
    _Mouse->Show_Mouse();
}

void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2)
{
    if (!_Mouse)
        return;
    _Mouse->Conditional_Hide_Mouse(x1, y1, x2, y2);
}

void Conditional_Show_Mouse(void)
{
    if (!_Mouse)
        return;
    _Mouse->Conditional_Show_Mouse();
}

int Get_Mouse_State(void)
{
    if (!_Mouse)
        return (0);
    return (_Mouse->Get_Mouse_State());
}

void* Set_Mouse_Cursor(int hotx, int hoty, void* cursor)
{
    if (!_Mouse)
        return (0);
    return (_Mouse->Set_Cursor(hotx, hoty, cursor));
}

int Get_Mouse_X(void)
{
    if (!_Mouse)
        return (0);
    return (_Mouse->Get_Mouse_X());
}

int Get_Mouse_Y(void)
{
    if (!_Mouse)
        return (0);
    return (_Mouse->Get_Mouse_Y());
}
