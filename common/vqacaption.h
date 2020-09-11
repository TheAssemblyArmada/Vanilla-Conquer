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
#ifndef VQACAPTION_H
#define VQACAPTION_H

#include <stdint.h>

typedef struct _CaptionText
{
    int16_t field_0;
    int16_t field_2;
    int16_t field_4; // bitfield options - 0x80 = black background? could be TextPrintType?
    int16_t field_6;
    int field_8;
    int16_t field_C;
    int field_E;      // sometype of char or text buffer?
    int16_t field_12; // text width?
    int16_t field_14; // text height?
} CaptionText;

typedef struct _CaptionInfo
{
    CaptionText* field_0;
    int field_4;
    int field_8;
    int field_C;
    void* Font;
    int16_t XPos;
    int16_t YPos;
    int16_t Width;
    int16_t Height;
    int field_1C;
    CaptionText field_20[3];
} CaptionInfo;

typedef struct _CaptionNode
{
    struct _CaptionNode* Prev;
    struct _CaptionNode* Next;
    int16_t field_8;
} CaptionNode;

typedef struct _CaptionList
{
    int field_0;
    int field_4;
    CaptionNode* Next;
} CaptionList;

// TODO: Caption handling is not present in RA, to research and implement later if possible.

#endif // VQACAPTION_H
