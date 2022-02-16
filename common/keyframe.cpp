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

/* $Header:   F:\projects\c&c\vcs\code\keyframe.cpv   2.14   16 Oct 1995 16:48:54   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : KEYFRAME.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 06/25/95                                                     *
 *                                                                                             *
 *                  Last Update : June 25, 1995 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Get_Build_Frame_Count -- Fetches the number of frames in data block.                      *
 *   Get_Build_Frame_Width -- Fetches the width of the shape image.                            *
 *   Get_Build_Frame_Height -- Fetches the height of the shape image.                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "lcw.h"
#include "xordelta.h"
#include "memflag.h"
#include "keyframe.h"
#include "debugstring.h"

#include <string.h>

#define SUBFRAMEOFFS 7 // 3 1/2 frame offsets loaded (2 offsets/frame)

#define Apply_Delta(buffer, delta) Apply_XOR_Delta((char*)(buffer), (char*)(delta))

typedef struct
{
    unsigned short frames;
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
    unsigned short largest_frame_size;
    short flags;
} KeyFrameHeaderType;

#define INITIAL_BIG_SHAPE_BUFFER_SIZE 12000 * 1024
#define THEATER_BIG_SHAPE_BUFFER_SIZE 2000 * 1024
#define UNCOMPRESS_MAGIC_NUMBER       56789

static unsigned short CurrentUncompressMagicNum = UNCOMPRESS_MAGIC_NUMBER;
static int BigShapeBufferLength = INITIAL_BIG_SHAPE_BUFFER_SIZE;
static int TheaterShapeBufferLength = THEATER_BIG_SHAPE_BUFFER_SIZE;
char* BigShapeBufferStart = nullptr;
char* TheaterShapeBufferStart = nullptr;
unsigned int UseBigShapeBuffer = false;
unsigned int IsTheaterShape = false;
static char* BigShapeBufferPtr = nullptr;
static int TotalBigShapes = 0;
static bool ReallocShapeBufferFlag = false;
static bool OriginalUseBigShapeBuffer = false;
static const bool AllowBigShapeBufRealloc = false;

/* Get amount of free memory on bigshapebuffer.  */
#define BIGSHPBUF_AVAILABLE(x) (((ptrdiff_t)BigShapeBufferStart + BigShapeBufferLength) - (ptrdiff_t)(x))

/* Get amount of free memory on theatershapebuffer.  */
#define THEATERSHPBUF_AVAILABLE(x) (((intptr_t)TheaterShapeBufferStart + TheaterShapeBufferLength) - (intptr_t)(x))

char* TheaterShapeBufferPtr = nullptr;
int TotalTheaterShapes = 0;

#define MAX_SLOTS          1500
#define THEATER_SLOT_START 1000

char** KeyFrameSlots[MAX_SLOTS];
int TotalSlotsUsed = 0;
int TheaterSlotsUsed = THEATER_SLOT_START;

typedef struct tShapeHeaderType
{
    unsigned draw_flags;
    char* shape_data;
    int shape_buffer; // 1 if shape is in theater buffer
} ShapeHeaderType;

static int Length;

void* Get_Shape_Header_Data(void* ptr)
{
    if (UseBigShapeBuffer) {

        ShapeHeaderType* header = (ShapeHeaderType*)ptr;
        return ((void*)(header->shape_data
                        + (uintptr_t)(header->shape_buffer ? TheaterShapeBufferStart : BigShapeBufferStart)));

    } else {
        return (ptr);
    }
}

int Get_Last_Frame_Length(void)
{
    return (Length);
}

void Reset_Theater_Shapes(void)
{
    /*
    ** Delete any previously allocated slots
    */
    for (int i = THEATER_SLOT_START; i < TheaterSlotsUsed; i++) {
        delete[] KeyFrameSlots[i];
    }

    TheaterShapeBufferPtr = TheaterShapeBufferStart;
    TotalTheaterShapes = 0;
    TheaterSlotsUsed = THEATER_SLOT_START;
}

void Reset_BigShapeBuffer(void)
{
    for (int i = 0; i < TotalSlotsUsed; i++) {
        delete[] KeyFrameSlots[i];
        KeyFrameSlots[i] = NULL;
    }

    BigShapeBufferPtr = BigShapeBufferStart;
    TotalBigShapes = 0;
    TotalSlotsUsed = 0;
}

extern void Memory_Error_Handler();

void Reallocate_Big_Shape_Buffer()
{
    if (!ReallocShapeBufferFlag)
        return;

    if (AllowBigShapeBufRealloc) {
        BigShapeBufferLength += 2000 * 1024; // Extra 2 Mb of uncompressed shape space
        BigShapeBufferPtr -= (uintptr_t)BigShapeBufferStart;
        Memory_Error = nullptr;
        BigShapeBufferStart = (char*)Resize_Alloc(BigShapeBufferStart, BigShapeBufferLength);
        Memory_Error = &Memory_Error_Handler;
        /*
        ** If we have run out of memory then disable the uncompressed shapes
        ** It may still be possible to continue with compressed shapes
        */
        if (!BigShapeBufferStart) {
            UseBigShapeBuffer = false;
            return;
        }
        BigShapeBufferPtr += (uintptr_t)BigShapeBufferStart;
    } else {
        // mrparrot 2021-11-22: A better alternative to disabling bigshapebuffer
        // is flushing and refilling it in the hope of discarding shapes not
        // very often used, like enemy building animations, radar animations,
        // and so on.
        Reset_Theater_Shapes();
        Reset_BigShapeBuffer();
        CurrentUncompressMagicNum++;
        DBG_LOG("BigShpBuf: flushed. Rebuilding and re-enabling.");
    }
    ReallocShapeBufferFlag = false;
    UseBigShapeBuffer = true;
}

void Check_Use_Compressed_Shapes()
{
    // Uncompressed shapes enabled for performance reasons. We don't need to worry about memory.
    // Uncompressed shapes don't seem to work in RA for rotated/scaled objects so wherever scale/rotate is used,
    // we will need to disable it (like in Techno_Draw_Object). ST - 11/6/2019 2:09PM
    UseBigShapeBuffer = true;
    OriginalUseBigShapeBuffer = true;
}

/***********************************************************************************************
 * Disable_Uncompressed_Shapes -- Temporarily turns off shape decompression                    *
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
 *    11/19/96 2:37PM ST : Created                                                             *
 *=============================================================================================*/
void Disable_Uncompressed_Shapes()
{
    UseBigShapeBuffer = false;
}

/***********************************************************************************************
 * Enable_Uncompressed_Shapes -- Restores state of shape decompression before it was disabled  *
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
 *    11/19/96 2:37PM ST : Created                                                             *
 *=============================================================================================*/
void Enable_Uncompressed_Shapes()
{
    UseBigShapeBuffer = OriginalUseBigShapeBuffer;
}

#define FIXIT_SCORE_CRASH

uintptr_t Build_Frame(void const* dataptr, unsigned short framenumber, void* buffptr)
{
#ifdef FIXIT_SCORE_CRASH
    char* ptr;
    unsigned int offcurr, offdiff;
#else
    char *ptr, *lockptr;
    unsigned int offcurr, off16, offdiff;
#endif
    uint32_t offset[SUBFRAMEOFFS];
    KeyFrameHeaderType* keyfr;
    unsigned short buffsize, currframe, subframe;
    unsigned int length = 0;
    char frameflags;
    uintptr_t return_value;
    char* temp_shape_ptr;

    //
    // valid pointer??
    //
    Length = 0;
    if (!dataptr || !buffptr) {
        return (0);
    }

    //
    // look at header then check that frame to build is not greater
    // than total frames
    //
    keyfr = (KeyFrameHeaderType*)dataptr;

    if (framenumber >= keyfr->frames) {
        return (0);
    }

    if (UseBigShapeBuffer) {
        /*
        ** If we havnt yet allocated memory for uncompressed shapes then do so now.
        **
        */
        if (!BigShapeBufferStart) {
            BigShapeBufferStart = (char*)Alloc(BigShapeBufferLength, MEM_NORMAL);
            BigShapeBufferPtr = BigShapeBufferStart;

            /*
            ** Allocate memory for theater specific uncompressed shapes
            */
            TheaterShapeBufferStart = (char*)Alloc(TheaterShapeBufferLength, MEM_NORMAL);
            TheaterShapeBufferPtr = TheaterShapeBufferStart;
        }

        /*
        ** If this animation was not previously uncompressed then
        ** allocate memory to keep the pointers to the uncompressed data
        ** for these animation frames
        */
        if (keyfr->x != CurrentUncompressMagicNum) {
            keyfr->x = CurrentUncompressMagicNum;
            if (IsTheaterShape) {
                keyfr->y = TheaterSlotsUsed;
                TheaterSlotsUsed++;
            } else {
                keyfr->y = TotalSlotsUsed;
                TotalSlotsUsed++;
            }
            /*
            ** Allocate and clear the memory for the shape info
            */
            KeyFrameSlots[keyfr->y] = new char*[keyfr->frames];
            memset(KeyFrameSlots[keyfr->y], 0, keyfr->frames * sizeof(char*));
        }

        /*
        ** If this frame was previously uncompressed then just return
        ** a pointer to the raw data
        */
        if (*(KeyFrameSlots[keyfr->y] + framenumber)) {
            if (IsTheaterShape) {
                return ((uintptr_t)TheaterShapeBufferStart + (uintptr_t) * (KeyFrameSlots[keyfr->y] + framenumber));
            } else {
                return ((uintptr_t)BigShapeBufferStart + (uintptr_t) * (KeyFrameSlots[keyfr->y] + framenumber));
            }
        }
    }

    // calc buff size
    buffsize = keyfr->width * keyfr->height;

    // get offset into data
    ptr = (char*)Add_Long_To_Pointer(dataptr, (((unsigned int)framenumber << 3) + sizeof(KeyFrameHeaderType)));
    Mem_Copy(ptr, &offset[0], 12L);
    frameflags = (char)(offset[0] >> 24);

    if ((frameflags & KF_KEYFRAME)) {

        ptr = (char*)Add_Long_To_Pointer(dataptr, (offset[0] & 0x00FFFFFFL));

        if (keyfr->flags & 1) {
            ptr = (char*)Add_Long_To_Pointer(ptr, 768L);
        }
        length = LCW_Uncompress(ptr, buffptr, buffsize);
    } else { // key delta or delta

        if ((frameflags & KF_DELTA)) {
            currframe = (unsigned short)offset[1];

            ptr = (char*)Add_Long_To_Pointer(dataptr, (((unsigned int)currframe << 3) + sizeof(KeyFrameHeaderType)));
            Mem_Copy(ptr, &offset[0], (int)(SUBFRAMEOFFS * sizeof(uint32_t)));
        }

        // key frame
        offcurr = offset[1] & 0x00FFFFFFL;

        // key delta
        offdiff = (offset[0] & 0x00FFFFFFL) - offcurr;

        ptr = (char*)Add_Long_To_Pointer(dataptr, offcurr);

        if (keyfr->flags & 1) {
            ptr = (char*)Add_Long_To_Pointer(ptr, 768L);
        }

#ifndef FIXIT_SCORE_CRASH
        off16 = (unsigned int)lockptr & 0x00003FFFL;
#endif

        length = LCW_Uncompress(ptr, buffptr, buffsize);

        if (length > buffsize) {
            return (0);
        }

#ifndef FIXIT_SCORE_CRASH
        if (((offset[2] & 0x00FFFFFFL) - offcurr) >= (0x00010000L - off16)) {

            ptr = (char*)Add_Long_To_Pointer(ptr, offdiff);
            off16 = (unsigned int)ptr & 0x00003FFFL;

            offcurr += offdiff;
            offdiff = 0;
        }
#endif

        length = buffsize;
        Apply_Delta(buffptr, Add_Long_To_Pointer(ptr, offdiff));

        if ((frameflags & KF_DELTA)) {
            // adjust to delta after the keydelta

            currframe++;
            subframe = 2;

            while (currframe <= framenumber) {
                offdiff = (offset[subframe] & 0x00FFFFFFL) - offcurr;

#ifndef FIXIT_SCORE_CRASH
                if (((offset[subframe + 2] & 0x00FFFFFFL) - offcurr) >= (0x00010000L - off16)) {

                    ptr = (char*)Add_Long_To_Pointer(ptr, offdiff);
                    off16 = (unsigned int)lockptr & 0x00003FFFL;

                    offcurr += offdiff;
                    offdiff = 0;
                }
#endif

                length = buffsize;
                Apply_Delta(buffptr, Add_Long_To_Pointer(ptr, offdiff));

                currframe++;
                subframe += 2;

                if (subframe >= (SUBFRAMEOFFS - 1) && currframe <= framenumber) {
                    Mem_Copy(
                        Add_Long_To_Pointer(dataptr, (((unsigned int)currframe << 3) + sizeof(KeyFrameHeaderType))),
                        &offset[0],
                        (int)(SUBFRAMEOFFS * sizeof(uint32_t)));
                    subframe = 0;
                }
            }
        }
    }

    if (UseBigShapeBuffer) {
        /*
        ** Save the uncompressed shape data so we dont have to uncompress it
        ** again next time its drawn.
        ** We keep a space free before the raw shape data so we can add line
        ** header info before the shape is drawn for the first time
        */

        if (IsTheaterShape) {
            /*
            ** Shape is a theater specific shape
            */
            return_value = (uintptr_t)TheaterShapeBufferPtr;
            temp_shape_ptr = TheaterShapeBufferPtr + keyfr->height + sizeof(ShapeHeaderType);
            /*
            ** align the actual shape data
            */
            if (3 & (uintptr_t)temp_shape_ptr) {
                temp_shape_ptr = (char*)((uintptr_t)(temp_shape_ptr + 3) & ~3);
            }

            /* Check if it will fit. Else, disable TheaterShpBuffer temporarly.  */
            if (THEATERSHPBUF_AVAILABLE(temp_shape_ptr) < (int)length && !AllowBigShapeBufRealloc) {
                DBG_LOG("TheaterShpBuf: shape won't fit. Disabling it temporarly...");
                UseBigShapeBuffer = false;
                ReallocShapeBufferFlag = true;

                return (uintptr_t)buffptr;
            }

            memcpy(temp_shape_ptr, buffptr, length);
            ((ShapeHeaderType*)TheaterShapeBufferPtr)->draw_flags = -1; // Flag that headers need to be generated
            ((ShapeHeaderType*)TheaterShapeBufferPtr)->shape_data =
                temp_shape_ptr - (uintptr_t)TheaterShapeBufferStart;     // pointer to old raw shape data
            ((ShapeHeaderType*)TheaterShapeBufferPtr)->shape_buffer = 1; // Theater buffer
            *(KeyFrameSlots[keyfr->y] + framenumber) = TheaterShapeBufferPtr - (uintptr_t)TheaterShapeBufferStart;
            TheaterShapeBufferPtr = (char*)(length + (uintptr_t)temp_shape_ptr);
            /*
            ** Align the next shape
            */
            if (3 & (uintptr_t)TheaterShapeBufferPtr) {
                TheaterShapeBufferPtr = (char*)((uintptr_t)(TheaterShapeBufferPtr + 3) & ~3);
            }
            Length = length;
            return (return_value);

        } else {
            return_value = (uintptr_t)BigShapeBufferPtr;
            temp_shape_ptr = BigShapeBufferPtr + keyfr->height + sizeof(ShapeHeaderType);
            /*
            ** align the actual shape data
            */
            if (3 & (uintptr_t)temp_shape_ptr) {
                temp_shape_ptr = (char*)((uintptr_t)(temp_shape_ptr + 3) & ~3);
            }

            /* Check if it will fit. Else, disable BigShpBuffer temporarly.  */
            if (BIGSHPBUF_AVAILABLE(temp_shape_ptr) <= (int)length) {
                DBG_LOG("BigShpBuf: shape won't fit. Disabling it temporarly...");
                UseBigShapeBuffer = false;
                ReallocShapeBufferFlag = true;

                return (uintptr_t)buffptr;
            }

            memcpy(temp_shape_ptr, buffptr, length);
            ((ShapeHeaderType*)BigShapeBufferPtr)->draw_flags = -1; // Flag that headers need to be generated
            ((ShapeHeaderType*)BigShapeBufferPtr)->shape_data =
                temp_shape_ptr - (uintptr_t)BigShapeBufferStart;     // pointer to old raw shape data
            ((ShapeHeaderType*)BigShapeBufferPtr)->shape_buffer = 0; // Normal Big Shape Buffer
            *(KeyFrameSlots[keyfr->y] + framenumber) = BigShapeBufferPtr - (uintptr_t)BigShapeBufferStart;
            BigShapeBufferPtr = (char*)(length + (uintptr_t)temp_shape_ptr);
            // Align the next shape
            if (3 & (uintptr_t)BigShapeBufferPtr) {
                BigShapeBufferPtr = (char*)((uintptr_t)(BigShapeBufferPtr + 3) & ~3);
            }
            Length = length;
            return (return_value);
        }

    } else {
        return ((uintptr_t)buffptr);
    }
}

/***********************************************************************************************
 * Get_Build_Frame_Count -- Fetches the number of frames in data block.                        *
 *                                                                                             *
 *    Use this routine to determine the number of shapes within the data block.                *
 *                                                                                             *
 * INPUT:   dataptr  -- Pointer to the keyframe shape data block.                              *
 *                                                                                             *
 * OUTPUT:  Returns with the number of shapes in the data block.                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1995 JLB : Commented.                                                               *
 *=============================================================================================*/
unsigned short Get_Build_Frame_Count(void const* dataptr)
{
    if (dataptr) {
        return (((KeyFrameHeaderType const*)dataptr)->frames);
    }
    return (0);
}

unsigned short Get_Build_Frame_X(void const* dataptr)
{
    if (dataptr) {
        return (((KeyFrameHeaderType const*)dataptr)->x);
    }
    return (0);
}

unsigned short Get_Build_Frame_Y(void const* dataptr)
{
    if (dataptr) {
        return (((KeyFrameHeaderType const*)dataptr)->y);
    }
    return (0);
}

/***********************************************************************************************
 * Get_Build_Frame_Width -- Fetches the width of the shape image.                              *
 *                                                                                             *
 *    Use this routine to fetch the width of the shapes within the keyframe shape data block.  *
 *    All shapes within the block have the same width.                                         *
 *                                                                                             *
 * INPUT:   dataptr  -- Pointer to the keyframe shape data block.                              *
 *                                                                                             *
 * OUTPUT:  Returns with the width of the shapes in the block -- expressed in pixels.          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1995 JLB : Commented                                                                *
 *=============================================================================================*/
unsigned short Get_Build_Frame_Width(void const* dataptr)
{
    if (dataptr) {
        return (((KeyFrameHeaderType const*)dataptr)->width);
    }
    return (0);
}

/***********************************************************************************************
 * Get_Build_Frame_Height -- Fetches the height of the shape image.                            *
 *                                                                                             *
 *    Use this routine to fetch the height of the shapes within the keyframe shape data block. *
 *    All shapes within the block have the same height.                                        *
 *                                                                                             *
 * INPUT:   dataptr  -- Pointer to the keyframe shape data block.                              *
 *                                                                                             *
 * OUTPUT:  Returns with the height of the shapes in the block -- expressed in pixels.         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1995 JLB : Commented                                                                *
 *=============================================================================================*/
unsigned short Get_Build_Frame_Height(void const* dataptr)
{
    if (dataptr) {
        return (((KeyFrameHeaderType const*)dataptr)->height);
    }
    return (0);
}

bool Get_Build_Frame_Palette(void const* dataptr, void* palette)
{
    if (dataptr && (((KeyFrameHeaderType const*)dataptr)->flags & 1)) {
        char const* ptr = (char const*)Add_Long_To_Pointer(
            dataptr,
            ((((int)sizeof(unsigned int) << 1) * ((KeyFrameHeaderType*)dataptr)->frames) + 16
             + sizeof(KeyFrameHeaderType)));

        memcpy(palette, ptr, 768L);
        return (true);
    }
    return (false);
}
