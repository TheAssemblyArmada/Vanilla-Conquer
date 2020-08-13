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

#include "wwmousedrawable.h"

#include "lcw.h"
#include "shape.h"

#include <string.h>

WWMouseClassDrawable::WWMouseClassDrawable(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
    : WWMouseClass(scr, mouse_max_width, mouse_max_height)
{
}

WWMouseClassDrawable::~WWMouseClassDrawable()
{
}

extern bool GameInFocus;

void WWMouseClassDrawable::Process_Mouse()
{
    //
    // If the mouse is currently hidden or it has not been installed, then we
    // have no need to redraw the mouse.
    //
    if (!Screen || State > 0 || MouseUpdate || EraseFlags || !GameInFocus)
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
    int x = 0;
    int y = 0;
    GetPosition(x, y);

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
}

void* WWMouseClassDrawable::Set_Cursor(int xhotspot, int yhotspot, void* cursor)
{
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
    void* retval = Set_Mouse_Cursor(xhotspot, yhotspot, cursor);
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
    // Asm_Set_Mouse_Cursor.
    //
    return retval;
}

void WWMouseClassDrawable::Low_Hide_Mouse()
{
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
    State++;
}

void WWMouseClassDrawable::Low_Show_Mouse(int x, int y)
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

    //
    // If the mouse is completely visible then draw it at its current
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
}

void* WWMouseClassDrawable::Set_Mouse_Cursor(int hotspotx, int hotspoty, void* cursor)
{
    Shape_Header* cursor_header = (Shape_Header*)(cursor);
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
        frame_flags = cursor_header->ShapeType;

        // Flag bit 2 is flag for no compression on frame, decompress to
        // intermediate buffer if flag is clear
        if (!(cursor_header->ShapeType & 2)) {
            decmp_buff = (unsigned char*)_ShapeBuffer;
            lcw_buff = (unsigned char*)(cursor_header);
            frame_flags = cursor_header->ShapeType | 2;

            memcpy(decmp_buff, lcw_buff, sizeof(Shape_Header));
            decmp_buff += sizeof(Shape_Header);
            lcw_buff += sizeof(Shape_Header);

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
            unsigned char* data_start = data_buff + sizeof(Shape_Header);
            unsigned char* image_start = data_buff + sizeof(Shape_Header) + 16;
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
            unsigned char* data_start = data_buff + sizeof(Shape_Header);
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
    PrevCursor = (char*)cursor;

    return result;
}

void WWMouseClassDrawable::Draw_Mouse(GraphicViewPortClass* scr)
{
    if (State != 0)
        return;
    MouseUpdate++;
    //
    //	Get the position that the mouse is currently located at
    //
    int x = 0;
    int y = 0;
    GetPosition(x, y);
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
}

void WWMouseClassDrawable::Erase_Mouse(GraphicViewPortClass* scr, int forced)
{
    //
    // If we are forcing the redraw of a mouse we already managed to
    // restore then just get outta here.
    //
    if (forced && EraseBuffX == -1 && EraseBuffY == -1)
        return;

    MouseUpdate++;

    //
    // If this is not a forced call, only update the mouse is we can legally
    // lock the buffer.
    //
    if (forced) {
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
}

void WWMouseClassDrawable::Mouse_Shadow_Buffer(GraphicViewPortClass* viewport,
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

    xend = MIN(xend, viewport->Get_Width() - 1);
    yend = MIN(yend, viewport->Get_Height() - 1);

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

void WWMouseClassDrawable::Draw_Mouse(GraphicViewPortClass* viewport, int x, int y)
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

    xend = MIN(xend, viewport->Get_Width() - 1);
    yend = MIN(yend, viewport->Get_Height() - 1);

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
