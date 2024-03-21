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

#include "wwkeyboard_sdl1.h"
#include "video.h"
#include "settings.h"
#include <SDL.h>

void Focus_Loss();
void Focus_Restore();
void Process_Network();

WWKeyboardClassWin32::~WWKeyboardClassWin32()
{
}

void WWKeyboardClassWin32::Fill_Buffer_From_System(void)
{
    if (!Is_Buffer_Full()) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (!GetMessageA(&msg, NULL, 0, 0)) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

WWKeyboardClass* CreateWWKeyboardClass(void)
{
    return new WWKeyboardClassWin32;
}
