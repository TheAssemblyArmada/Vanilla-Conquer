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
 *                 Project Name : Westwood Library                         *
 *                                                                         *
 *                    File Name : GETSHAPE.CPP                             *
 *                                                                         *
 *                   Programmer : Joe L. Bostic                            *
 *                                                                         *
 *                   Start Date : April 5, 1992                            *
 *                                                                         *
 *                  Last Update : May 25, 1994   [BR]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Get_Shape_Size -- Fetch the size of the shape in memory.              *
 *   Get_Shape_Uncomp_Size -- gets shape's uncompressed size in bytes		*
 *   Get_Shape_Data -- retrieves a shape's special prefix data					*
 *   Extract_Shape_Count -- returns # of shapes in the given shape block	*
 *   Extract_Shape -- Gets pointer to shape in given shape block				*
 *   Get_Shape_Width -- gets shape width in pixels									*
 *   Get_Shape_Height -- gets shape height in pixels								*
 *   Set_Shape_Height -- modifies shape's height									*
 *   Restore_Shape_Height -- restores a shape to its original height			*
 *   Get_Shape_Original_Height -- gets shape's unmodified height				*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
********************************* Includes **********************************
*/

#include <cstddef>

#include "wwstd.h"
#include "shape.h"
#include "endianness.h"

/***************************************************************************
 * Get_Shape_Size -- Fetch the size of the shape in memory.                *
 *                                                                         *
 * The shape size returned includes both the shape header & its data.		*
 *																									*
 * INPUT:                                                                  *
 *	shape		pointer to shape																*
 *                                                                         *
 * OUTPUT:                                                                 *
 * shape's size in memory																	*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Get_Shape_Size(void const* shape)
{
    Shape_Type* shp = (Shape_Type*)shape;

    /*
    ------------------------- Return if NULL pointer -------------------------
    */
    if (!shape)
        return (0);

    /*
    -------------------------- Returns shape's size --------------------------
    */
    return le16toh(shp->ShapeSize);

} /* end of Get_Shape_Size */

/***************************************************************************
 * Get_Shape_Uncomp_Size -- gets shape's uncompressed size in bytes			*
 *                                                                         *
 * INPUT:                                                                  *
 *	shape		pointer to shape																*
 *                                                                         *
 * OUTPUT:                                                                 *
 *	shape's size in bytes when uncompressed											*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Get_Shape_Uncomp_Size(void const* shape)
{
    Shape_Type* shp = (Shape_Type*)shape;

    return le16toh(shp->DataLength);

} /* end of Get_Shape_Uncomp_Size */

/***************************************************************************
 * Get_Shape_Data -- retrieves a shape's special prefix data					*
 *																									*
 * MAKESHPS.EXE can store special data values along with a shape.  These	*
 * values are inserted in the shape table >before< the shape's header.		*
 * So, this routine uses the 'data' parameter as a negative index from		*
 * the given shape pointer.																*
 *                                                                         *
 * INPUT:                                                                  *
 *	shape		pointer to shape																*
 * data		index of unsigned short data value to get											*
 *                                                                         *
 * OUTPUT:                                                                 *
 * data value																					*
 *                                                                         *
 * WARNINGS:                                                               *
 *	The shape pointer must be a pointer into a shape table created by			*
 * MAKESHPS.EXE; it >cannot< be a pointer to shape returned by Make_Shape!	*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
unsigned short Get_Shape_Data(void const* shape, unsigned short data)
{
    unsigned short* word_ptr = (unsigned short*)shape;
    unsigned short retval;

    retval = *(word_ptr - (data + 1));

    return (retval);

} /* end of Get_Shape_Data */

/***************************************************************************
 * Extract_Shape_Count -- returns # of shapes in the given shape block		*
 *                                                                         *
 * The # of shapes in a shape block is the first unsigned short in the block, so		*
 * this is the value returned.															*
 *																									*
 * INPUT:                                                                  *
 * buffer	pointer to shape block, created with MAKESHPS.EXE					*
 *                                                                         *
 * OUTPUT:                                                                 *
 * # shapes in the block																	*
 *                                                                         *
 * WARNINGS:                                                               *
 *	none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Extract_Shape_Count(void const* buffer)
{
    char const* bbuffer = (char const*)buffer;
    uint16_t numshapes;
    memcpy(&numshapes, bbuffer + offsetof(ShapeBlock_Type, NumShapes), sizeof(numshapes));
    return (int)le16toh(numshapes);
} /* end of Extract_Shape_Count */

/***************************************************************************
 * Extract_Shape -- Gets pointer to shape in given shape block					*
 *                                                                         *
 * INPUT:                                                                  *
 * buffer	pointer to shape block, created with MAKESHPS.EXE					*
 * shape		index of shape to get														*
 *                                                                         *
 * OUTPUT:                                                                 *
 * pointer to shape in the shape block													*
 *                                                                         *
 * WARNINGS:                                                               *
 *	none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
void* Extract_Shape(void const* buffer, int shape)
{
    ShapeBlock_Type* block = (ShapeBlock_Type*)buffer;
    // PG	int numshapes;		// Number of shapes
    uint32_t offset; // Offset of shape data, from start of block
    char* bytebuf = (char*)buffer;

    /*
    ----------------------- Return if invalid argument -----------------------
    */
    if (!buffer || shape < 0)
        return (NULL);

    uint16_t numshapes;
    memcpy(&numshapes, bytebuf + offsetof(ShapeBlock_Type, NumShapes), sizeof(short));

    if (shape >= numshapes) {
        return NULL;
    }

    /*  Same as offset = block->Offsets[shape]; on arch that unaligned access
        behaves well.  */
    memcpy(&offset, bytebuf + offsetof(ShapeBlock_Type, Offsets) + shape * sizeof(uint32_t), sizeof(uint32_t));
    offset = le32toh(offset);

    return (bytebuf + 2 + offset);

} /* end of Extract_Shape */

/***************************************************************************
 * Get_Shape_Width -- gets shape width in pixels									*
 *                                                                         *
 * INPUT:                                                                  *
 * shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *	shape width in pixels																	*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Get_Shape_Width(void const* shape)
{
    Shape_Type* shp = (Shape_Type*)shape;

    return le16toh(shp->Width);

} /* end of Get_Shape_Width */

/***************************************************************************
 * Get_Shape_Height -- gets shape height in pixels									*
 *                                                                         *
 * INPUT:                                                                  *
 * shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * shape height in pixels																	*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Get_Shape_Height(void const* shape)
{
    Shape_Type* shp = (Shape_Type*)shape;

    return (shp->Height);

} /* end of Get_Shape_Height */

/***************************************************************************
 * Set_Shape_Height -- modifies shape's height										*
 *                                                                         *
 * The new height must be shorter than the original height.  This effect	*
 * chops off the lower portion of the shape, like it's sinking into the		*
 * ground.																						*
 *																									*
 * INPUT:                                                                  *
 * shape			pointer to a shape														*
 * newheight	new shape height															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * old shape height																			*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Set_Shape_Height(void const* shape, unsigned short newheight)
{
    Shape_Type* shp = (Shape_Type*)shape;
    unsigned short oldheight;

    oldheight = shp->Height;
    shp->Height = (unsigned char)newheight;

    return (oldheight);

} /* end of Set_Shape_Height */

/***************************************************************************
 * Restore_Shape_Height -- restores a shape to its original height			*
 *                                                                         *
 * INPUT:                                                                  *
 * shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * old shape height																			*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Restore_Shape_Height(void* shape)
{
    Shape_Type* shp = (Shape_Type*)shape;
    unsigned short oldheight;

    oldheight = shp->Height;
    shp->Height = shp->OriginalHeight;

    return (oldheight);

} /* end of Restore_Shape_Height */

/***************************************************************************
 * Get_Shape_Original_Height -- gets shape's unmodified height					*
 *                                                                         *
 * INPUT:                                                                  *
 *	shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * shape's unmodified height																*
 *                                                                         *
 * WARNINGS:                                                               *
 *	none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int Get_Shape_Original_Height(void const* shape)
{
    Shape_Type* shp = (Shape_Type*)shape;

    return (shp->OriginalHeight);

} /* end of Get_Shape_Original_Height */

/************************* end of getshape.cpp *****************************/
