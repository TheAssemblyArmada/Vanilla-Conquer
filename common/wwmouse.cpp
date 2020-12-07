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
 *   WWMouseClass::WWMouseClass -- Constructor for the Mouse Class                                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "wwmouse.h"
#include "lcw.h"
#include <string.h>
#ifdef SDL2_BUILD
#include <SDL.h>
extern SDL_Window* window;
extern SDL_Renderer* renderer;
#endif

#ifndef min
#define min(x, y) (x <= y ? x : y)
#endif

#if defined(_WIN32) && !defined(SDL2_BUILD)
#include <mmsystem.h>
void CALLBACK Process_Mouse(UINT event_id, UINT res1, DWORD_PTR user, DWORD_PTR res2, DWORD_PTR res3);
#endif

static WWMouseClass* _Mouse = nullptr;
extern bool GameInFocus;

/***********************************************************************************************
 * MOUSECLASS::MOUSECLASS -- Constructor for the Mouse Class                                   *
 *                                                                                             *
 * INPUT:		GraphicViewPortClass * screen - pointer to screen mouse is created for				 *
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
#if defined(_WIN32) && !defined(SDL2_BUILD)
    timeBeginPeriod(1000 / 60);

    InitializeCriticalSection(&MouseCriticalSection);
#endif
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

    // Add TIME_KILL_SYNCHRONOUS flag. ST - 2/13/2019 5:07PM
    // TimerHandle = timeSetEvent( 1000/60 , 1 , ::Process_Mouse, 0 , TIME_PERIODIC);
#if defined(_WIN32) && !defined(REMASTER_BUILD) && !defined(SDL2_BUILD)
    TimerHandle = timeSetEvent(1000 / 60, 1, ::Process_Mouse, 0, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
#endif
    // Removed. ST - 2/13/2019 5:12PM

    /*
    ** Force the windows mouse pointer to stay withing the graphic view port region
    */
    Set_Cursor_Clip();

#ifdef SDL2_BUILD
    if (window) {
        int w, h;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        MouseXScale = scr->Get_Width() / (float)w;
        MouseYScale = scr->Get_Height() / (float)h;
    }
#else
    MouseXScale = 1.0f;
    MouseYScale = 1.0f;
#endif
}

WWMouseClass::~WWMouseClass()
{
    MouseUpdate++;

    if (MouseCursor)
        delete[] MouseCursor;
    if (MouseBuffer)
        delete[] MouseBuffer;
#if defined(_WIN32) && !defined(SDL2_BUILD)
    if (TimerHandle) {
        timeKillEvent(TimerHandle);
        TimerHandle = 0; // ST - 2/13/2019 5:12PM
    }
    timeEndPeriod(1000 / 60);
    DeleteCriticalSection(&MouseCriticalSection);
#endif

    /*
    ** Free up the windows mouse pointer movement
    */
    Clear_Cursor_Clip();
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
#if defined(_WIN32) && !defined(SDL2_BUILD)
        EnterCriticalSection(&MouseCriticalSection);
#endif
    }
}

void WWMouseClass::Unblock_Mouse(GraphicBufferClass* buffer)
{
    if (buffer == Screen->Get_Graphic_Buffer()) {
#if defined(_WIN32) && !defined(SDL2_BUILD)
        LeaveCriticalSection(&MouseCriticalSection);
#endif
    }
}

void WWMouseClass::Set_Cursor_Clip(void)
{
#if !defined(REMASTER_BUILD)
    if (Screen) {
#if defined(SDL2_BUILD)
        SDL_SetWindowGrab(window, SDL_TRUE);
#elif defined(_WIN32)
        RECT region;

        region.left = 0;
        region.top = 0;
        region.right = Screen->Get_Width();
        region.bottom = Screen->Get_Height();

        ClipCursor(&region);
#endif
    }
#endif // !REMASTER_BUILD
}

void WWMouseClass::Clear_Cursor_Clip(void)
{
#if !defined(REMASTER_BUILD)
#if defined(SDL2_BUILD)
    SDL_SetWindowGrab(window, SDL_FALSE);
#elif defined(_WIN32)
    ClipCursor(nullptr);
#endif
#endif // !REMASTER_BUILD
}

void WWMouseClass::Process_Mouse(void)
{
    // ST - 1/3/2019 10:50AM
#if !defined(REMASTER_BUILD)
    int x, y;

    //
    // If the mouse is currently hidden or it has not been installed, then we
    // have no need to redraw the mouse.
    //
    if (!Screen || !_Mouse || State > 0 || MouseUpdate || EraseFlags || !GameInFocus)
        return;

    //
    // Make sure there are no conflicts with other
    // threads that may try and lock the screen buffer
    //
    Block_Mouse(Screen->Get_Graphic_Buffer());

    //
    // If the screen is already locked by another thread then just exit
    //
    if (Screen->Get_LockCount() != 0) {
        // Unblock_Mouse(Screen->Get_Graphic_Buffer());
        return;
    }

    //
    // Get the mouse's current real cursor position
    //
    Get_Mouse_XY(x, y);

    //
    // If the mouse has moved then we are responsible to redraw the mouse
    //
    if (x != MouseBuffX || y != MouseBuffY) {
        //
        // If we can't lock the surface we need to draw to, we cannot update
        // the mouse.
        //
        if (Screen->Lock()) {
            //
            // Erase the old mouse by dumping the mouses shadow buff
            //   to the screen (if its position had ever been recorded).
            //
            Low_Hide_Mouse();

            //
            // Verify that the mouse has not gone into a conditional hiden area
            // If it has, mark it as being in one.
            //
            if (MCFlags & CONDHIDE && x >= MouseCXLeft && x <= MouseCXRight && y >= MouseCYUpper && y <= MouseCYLower) {
                MCFlags |= CONDHIDDEN;
            }

            //
            // Show the mouse if we are allowed to.
            //
            if (!(MCFlags & CONDHIDDEN)) {
                Low_Show_Mouse(x, y);
            }
            //
            // Finally unlock the destination surface as we have sucessfully
            // updated the mouse.
            //
            Screen->Unlock();
        }
    }

    //
    // Allow other threads to lock the screen again
    //
    Unblock_Mouse(Screen->Get_Graphic_Buffer());
#endif
}

void* WWMouseClass::Set_Cursor(int xhotspot, int yhotspot, void* cursor)
{
#if defined(REMASTER_BUILD)
    // ST - 1/3/2019 10:50AM
    xhotspot;
    yhotspot;
    cursor;
    return cursor;

#else

    //
    // If the pointer to the cursor we got is invalid, or its the same as the
    // currently set cursor then just return.
    if (!cursor || cursor == PrevCursor)
        return (cursor);

    //
    // Wait until we have exclusive access to our data
    //
    MouseUpdate++;
    //
    // Since we are updating the mouse we need to hide the cursor so we
    // do not get some sort of weird transformation.
    //
    Hide_Mouse();
    //
    // Now convert the shape to a mouse cursor with the given hotspots and
    // set it as our current mouse.
    //
    void* retval = Set_Mouse_Cursor(xhotspot, yhotspot, (Cursor*)cursor);
    //
    // Show the mouse which will force it to appear with the new shape we
    // have assigned.
    //
    Show_Mouse();
    //
    // We are done updating the mouse cursor so on to bigger and better things.
    //
    MouseUpdate--;
    //
    // Return the previous mouse cursor which as conveniantly passed back by
    // Set_Mouse_Cursor.
    //
    return (retval);
#endif
}

void WWMouseClass::Low_Hide_Mouse()
{
// ST - 1/3/2019 10:50AM
#ifndef REMASTER_BUILD
    if (!State) {
        if (MouseBuffX != -1 || MouseBuffY != -1) {
            if (Screen->Lock()) {
                Mouse_Shadow_Buffer(Screen, MouseBuffer, MouseBuffX, MouseBuffY, MouseXHot, MouseYHot, 0);
                Screen->Unlock();
            }
        }
        MouseBuffX = -1;
        MouseBuffY = -1;
    }
#endif
    State++;
}
void WWMouseClass::Hide_Mouse()
{
    MouseUpdate++;
    Low_Hide_Mouse();
    MouseUpdate--;
}

void WWMouseClass::Low_Show_Mouse(int x, int y)
{
    //
    // If the mouse is already visible then just ignore the problem.
    //
    if (State == 0)
        return;
    //
    // Make the mouse a little bit more visible
    //
    State--;

// ST - 1/3/2019 10:50AM
#ifndef REMASTER_BUILD

    //
    //	If the mouse is completely visible then draw it at its current
    // position.
    //
    if (!State) {
        //
        // Try to lock the screen til we sucessfully get a lock.
        //
        if (Screen->Lock()) {
            //
            // Save off the area behind the mouse.
            //
            Mouse_Shadow_Buffer(Screen, MouseBuffer, x, y, MouseXHot, MouseYHot, 1);
            //
            // Draw the mouse in its new location
            //
            Draw_Mouse(Screen, x, y);
            //
            // Save off the positions that we saved the buffer from
            //
            MouseBuffX = x;
            MouseBuffY = y;
            //
            // Unlock the screen and lets get moving.
            //
            Screen->Unlock();
        }
    }
#endif
}

void WWMouseClass::Show_Mouse()
{
    int x, y;
    Get_Mouse_XY(x, y);

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
    // 	(perform normal region checking with possible hide)
    // 2) The mouse is hidden and no conditional hide as been specified.
    // 	(record region and do nothing)
    // 3) The mouse is visible and a conditional region has been specified
    // 	(expand region and perform check with possible hide).
    // 4) The mouse is already hidden by a previous conditional.
    // 	(expand region and do nothing)
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

void WWMouseClass::Draw_Mouse(GraphicViewPortClass* scr)
{
#if defined(REMASTER_BUILD)
    scr;
    return;
// ST - 1/3/2019 10:50AM
#else

    int x, y;

    if (State != 0)
        return;
    MouseUpdate++;
    //
    //	Get the position that the mouse is currently located at
    //
    Get_Mouse_XY(x, y);
    if (MCFlags & CONDHIDE && x >= MouseCXLeft && x <= MouseCXRight && y >= MouseCYUpper && y <= MouseCYLower) {
        Hide_Mouse();
        MCFlags |= CONDHIDDEN;
    } else {
        //
        // If the mouse is already visible then just ignore the problem.
        //
        EraseFlags = true;

        //
        // Try to lock the screen  - dont do video stuff if we cant.
        //
        if (scr->Lock()) {
            //
            // Save off the area behind the mouse into two different buffers, one
            // which will be used to restore the mouse and the other which will
            // be used to restore the hidden surface when we get a chance.
            //
            Mouse_Shadow_Buffer(scr, EraseBuffer, x, y, MouseXHot, MouseYHot, 1);
            memcpy(MouseBuffer, EraseBuffer, MaxWidth * MaxHeight);
            //
            // Draw the mouse in its new location
            //
            Draw_Mouse(scr, x, y);
            //
            // Save off the positions that we saved the buffer from
            //
            EraseBuffX = x;
            MouseBuffX = x;
            EraseBuffY = y;
            MouseBuffY = y;
            EraseBuffHotX = MouseXHot;
            EraseBuffHotY = MouseYHot;
            //
            // Unlock the screen and lets get moving.
            //
            scr->Unlock();
        }
    }

    MouseUpdate--;
#endif
}

void WWMouseClass::Erase_Mouse(GraphicViewPortClass* scr, int forced)
{
#if defined(REMASTER_BUILD)
    // ST - 1/3/2019 10:50AM
    scr;
    forced;
    return;
#else
    //
    // If we are forcing the redraw of a mouse we already managed to
    // restore then just get outta here.
    //
    if (forced && EraseBuffX == -1 && EraseBuffY == -1)
        return;

    MouseUpdate++;

    //
    // If this is not a forced call, only update the mouse is we can legally
    //	lock the buffer.
    //
    if (!forced) {
#if (0)
        if (scr->Lock()) {
            //
            // If the surface has not already been restore then restore it and erase the
            // restoration coordinates so we don't accidentally do it twice.
            //
            if (EraseBuffX != -1 || EraseBuffY != -1) {
                Mouse_Shadow_Buffer(scr, EraseBuffer, EraseBuffX, EraseBuffY, EraseBuffHotX, EraseBuffHotY, 0);
                EraseBuffX = -1;
                EraseBuffY = -1;
            }
            //
            // We are done writing to the buffer so unlock it.
            //
            scr->Unlock();
        }
#endif
    } else {
        //
        // If the surface has not already been restore then restore it and erase the
        // restoration coordinates so we don't accidentally do it twice.
        //
        if (EraseBuffX != -1 || EraseBuffY != -1) {
            if (scr->Lock()) {
                Mouse_Shadow_Buffer(scr, EraseBuffer, EraseBuffX, EraseBuffY, EraseBuffHotX, EraseBuffHotY, 0);
                scr->Unlock();
            }
            EraseBuffX = -1;
            EraseBuffY = -1;
        }
    }
    MouseUpdate--;
    EraseFlags = false;
#endif
}

int WWMouseClass::Get_Mouse_State(void)
{
    return (State);
}
/***********************************************************************************************
 * WWKeyboardClass::Get_Mouse_X -- Returns the mouses current x position in pixels             *
 *                                                                                             *
 * INPUT:		none                                                                            *
 *                                                                                             *
 * OUTPUT:     int		- returns the mouses current x position in pixels                      *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int WWMouseClass::Get_Mouse_X(void)
{
    int x, y;
    Get_Mouse_XY(x, y);
    return x;
}

/***********************************************************************************************
 * WWKeyboardClass::Get_Mouse_Y -- returns the mouses current y position in pixels             *
 *                                                                                             *
 * INPUT:		none                                                                            *
 *                                                                                             *
 * OUTPUT:     int		- returns the mouses current y position in pixels                      *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int WWMouseClass::Get_Mouse_Y(void)
{
    int x, y;
    Get_Mouse_XY(x, y);
    return y;
}

/***********************************************************************************************
 * WWKeyboardClass::Get_Mouse_XY -- Returns the mouses x,y position via reference vars         *
 *                                                                                             *
 * INPUT:		int &x		- variable to return the mouses x position in pixels                *
 *					int &y		- variable to return the mouses y position in pixels					  *
 *                                                                                             *
 * OUTPUT:     none - output is via reference variables                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void WWMouseClass::Get_Mouse_XY(int& x, int& y)
{
#if defined(SDL2_BUILD)
    SDL_GetMouseState(&x, &y);
    x *= MouseXScale;
    y *= MouseYScale;
#elif defined(_WIN32)
    POINT pt;

    GetCursorPos(&pt);
    x = pt.x;
    y = pt.y;
#endif
}

void WWMouseClass::Get_Mouse_Scale_XY(float& x, float& y)
{
    x = MouseXScale;
    y = MouseYScale;
}

void WWMouseClass::Mouse_Shadow_Buffer(GraphicViewPortClass* viewport,
                                       void* buffer,
                                       int x,
                                       int y,
                                       int hotx,
                                       int hoty,
                                       int store)
{
    int xstart = x - hotx;
    int ystart = y - hoty;
    int yend = CursorHeight + ystart - 1;
    int xend = CursorWidth + xstart - 1;
    int ms_img_offset = 0;
    unsigned char* mouse_img = (unsigned char*)(buffer);

    // If we aren't drawing within the viewport, return
    if (xstart >= viewport->Get_Width() || ystart >= viewport->Get_Height() || xend <= 0 || yend <= 0) {
        return;
    }

    if (xstart < 0) {
        ms_img_offset = -xstart;
        xstart = 0;
    }

    if (ystart < 0) {
        mouse_img += CursorWidth * (-ystart);
        ystart = 0;
    }

    xend = min(xend, viewport->Get_Width() - 1);
    yend = min(yend, viewport->Get_Height() - 1);

    int blit_height = yend - ystart + 1;
    int blit_width = xend - xstart + 1;
    int src_pitch;
    int dst_pitch;
    unsigned char* src;
    unsigned char* dst;

    // Bool will control if we are copying to or from shadow buffer, set vars here
    if (store) {
        src_pitch = (viewport->Get_Pitch() + viewport->Get_XAdd() + viewport->Get_Width());
        dst_pitch = CursorWidth;
        src = (unsigned char*)(viewport->Get_Offset()) + src_pitch * ystart + xstart;
        dst = mouse_img + ms_img_offset;
    } else {
        src_pitch = CursorWidth;
        dst_pitch = (viewport->Get_Pitch() + viewport->Get_XAdd() + viewport->Get_Width());
        src = mouse_img + ms_img_offset;
        dst = (unsigned char*)(viewport->Get_Offset()) + dst_pitch * ystart + xstart;
    }

    while (blit_height--) {
        memcpy(dst, src, blit_width);
        src += src_pitch;
        dst += dst_pitch;
    }
}

void WWMouseClass::Draw_Mouse(GraphicViewPortClass* viewport, int x, int y)
{
    int xstart = x - MouseXHot;
    int ystart = y - MouseYHot;
    int yend = CursorHeight + ystart - 1;
    int xend = CursorWidth + xstart - 1;
    int ms_img_offset = 0;
    unsigned char* mouse_img = (unsigned char*)MouseCursor;

    // If we aren't drawing within the viewport, return
    if (xstart >= viewport->Get_Width() || ystart >= viewport->Get_Height() || xend <= 0 || yend <= 0) {
        return;
    }

    if (xstart < 0) {
        ms_img_offset = -xstart;
        xstart = 0;
    }

    if (ystart < 0) {
        mouse_img += CursorWidth * (-ystart);
        ystart = 0;
    }

    xend = min(xend, viewport->Get_Width() - 1);
    yend = min(yend, viewport->Get_Height() - 1);

    int pitch = viewport->Get_Pitch() + viewport->Get_XAdd() + viewport->Get_Width();
    unsigned char* dst = xstart + pitch * ystart + (unsigned char*)(viewport->Get_Offset());
    unsigned char* src = ms_img_offset + mouse_img;
    int blit_pitch = xend - xstart + 1;

    if ((xend > xstart) && (yend > ystart)) {
        int blit_height = yend - ystart + 1;
        int dst_pitch = pitch - blit_pitch;
        int src_pitch = CursorWidth - blit_pitch;

        while (blit_height--) {
            int blit_width = blit_pitch;
            while (blit_width--) {
                unsigned char current = *src++;
                if (current) {
                    *dst = current;
                }

                ++dst;
            }

            src += src_pitch;
            dst += dst_pitch;
        }
    }
}

void* WWMouseClass::Set_Mouse_Cursor(int hotspotx, int hotspoty, Cursor* cursor)
{
    void* result = NULL;
    unsigned char* cursor_buff;
    unsigned char* data_buff;
    unsigned char* decmp_buff;
    unsigned char* lcw_buff;
    short frame_flags;
    int height;
    int width;
    int uncompsz;

    // Get the dimensions of our frame, mouse shp format can have variable
    // dimensions for each frame.
    uncompsz = Get_Shape_Uncomp_Size(cursor);
    width = Get_Shape_Width(cursor);
    height = Get_Shape_Original_Height(cursor);

    if (width <= MaxWidth && height <= MaxHeight) {
        cursor_buff = (unsigned char*)MouseCursor;
        data_buff = (unsigned char*)(cursor);
        frame_flags = cursor->ShapeType;

        // Flag bit 2 is flag for no compression on frame, decompress to
        // intermediate buffer if flag is clear
        if (!(cursor->ShapeType & 2)) {
            decmp_buff = (unsigned char*)_ShapeBuffer;
            lcw_buff = (unsigned char*)(cursor);
            frame_flags = cursor->ShapeType | 2;

            memcpy(decmp_buff, lcw_buff, sizeof(Cursor));
            decmp_buff += sizeof(Cursor);
            lcw_buff += sizeof(Cursor);

            // Copies a small lookup table if it exists, probably not in RA.
            if (frame_flags & 1) {
                memcpy(decmp_buff, lcw_buff, 16);
                decmp_buff += 16;
                lcw_buff += 16;
            }

            LCW_Uncompress(lcw_buff, decmp_buff, uncompsz);
            data_buff = (unsigned char*)_ShapeBuffer;
        }

        if (frame_flags & 1) {
            unsigned char* data_start = data_buff + sizeof(Cursor);
            unsigned char* image_start = data_buff + sizeof(Cursor) + 16;
            int image_size = height * width;
            int run_len = 0;

            while (image_size) {
                unsigned char current_byte = *image_start++;

                if (current_byte) {
                    *cursor_buff++ = data_start[current_byte];
                    --image_size;
                    continue;
                }

                if (!image_size) {
                    break;
                }

                run_len = *image_start;
                image_size -= run_len;
                ++image_start;

                while (run_len--) {
                    *cursor_buff++ = 0;
                }
            }
        } else {
            unsigned char* data_start = data_buff + sizeof(Cursor);
            int image_size = height * width;
            int run_len = 0;

            while (image_size) {
                unsigned char current_byte = *data_start++;

                if (current_byte) {
                    *cursor_buff++ = current_byte;
                    --image_size;
                    continue;
                }

                if (!image_size) {
                    break;
                }

                run_len = *data_start;
                image_size -= run_len;
                ++data_start;

                while (run_len--) {
                    *cursor_buff++ = 0;
                }
            }
        }

        MouseXHot = hotspotx;
        MouseYHot = hotspoty;
        CursorHeight = height;
        CursorWidth = width;
    }

    result = PrevCursor;
    PrevCursor = cursor;

    return result;
}

#ifdef _WIN32
void CALLBACK Process_Mouse(UINT event_id, UINT res1, DWORD user, DWORD res2, DWORD res3)
{
    static BOOL in_mouse_callback = false;

    if (_Mouse && !in_mouse_callback) {
        in_mouse_callback = true;
        _Mouse->Process_Mouse();
        in_mouse_callback = false;
    }
}
#endif

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

extern int DLLForceMouseX;
extern int DLLForceMouseY;

int Get_Mouse_X(void)
{
#ifdef REMASTER_BUILD
    if (DLLForceMouseX >= 0) {
        return DLLForceMouseX;
    }
#endif
    if (!_Mouse)
        return (0);
    return (_Mouse->Get_Mouse_X());
}

int Get_Mouse_Y(void)
{
#ifdef REMASTER_BUILD
    if (DLLForceMouseY >= 0) {
        return DLLForceMouseY;
    }
#endif
    if (!_Mouse)
        return (0);
    return (_Mouse->Get_Mouse_Y());
}

void Get_Mouse_Scale_XY(float& x, float& y)
{
    if (!_Mouse)
        return;

    _Mouse->Get_Mouse_Scale_XY(x, y);
}