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

#include "common/wwstd.h"

/*
**	DIALOG.CPP
*/
void Draw_Caption(int text, int x, int y, int w);
void Draw_Caption(char const* text, int x, int y, int w);
int Format_Window_String(char* string, int maxlinelen, int& width, int& height);
extern void Dialog_Box(int x, int y, int w, int h);
void Conquer_Clip_Text_Print(char const*,
                             unsigned x,
                             unsigned y,
                             RemapControlType* fore,
                             unsigned back = (unsigned)TBLACK,
                             TextPrintType flag = TPF_8POINT | TPF_DROPSHADOW,
                             int width = -1,
                             int const* tabs = 0);
void Draw_Box(int x, int y, int w, int h, BoxStyleEnum up, bool filled);
int Dialog_Message(char* errormsg, ...);
void Window_Box(WindowNumberType window, BoxStyleEnum style);
void Fancy_Text_Print(char const* text,
                      unsigned x,
                      unsigned y,
                      RemapControlType* fore,
                      unsigned back,
                      TextPrintType flag,
                      ...);
void Fancy_Text_Print(int text, unsigned x, unsigned y, RemapControlType* fore, unsigned back, TextPrintType flag, ...);
void Simple_Text_Print(char const* text,
                       unsigned x,
                       unsigned y,
                       RemapControlType* fore,
                       unsigned back,
                       TextPrintType flag);
void Plain_Text_Print(int text, unsigned x, unsigned y, unsigned fore, unsigned back, TextPrintType flag, ...);
void Plain_Text_Print(char const* text, unsigned x, unsigned y, unsigned fore, unsigned back, TextPrintType flag, ...);
