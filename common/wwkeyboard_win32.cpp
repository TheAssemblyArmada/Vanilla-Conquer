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

#include "wwkeyboard_win32.h"
#include "video.h"
#include "settings.h"

void Focus_Loss();
void Focus_Restore();
void Process_Network();

WWKeyboardClassWin32::~WWKeyboardClassWin32()
{
    memset(KeyState, '\0', sizeof(KeyState));
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

/***********************************************************************************************
 * WWKeyboardClassWin32::To_ASCII -- Convert the key value into an ASCII representation.       *
 *                                                                                             *
 *    This routine will convert the key code specified into an ASCII value. This takes into    *
 *    consideration the language and keyboard mapping of the host Windows system.              *
 *                                                                                             *
 * INPUT:   key   -- The key code to convert into ASCII.                                       *
 *                                                                                             *
 * OUTPUT:  Returns with the key converted into ASCII. If the key has no ASCII equivalent,     *
 *          then '\0' is returned.                                                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
KeyASCIIType WWKeyboardClassWin32::To_ASCII(unsigned short key)
{
    /*
    **	Released keys never translate into an ASCII value.
    */
    if (key & WWKEY_RLS_BIT) {
        return KA_NONE;
    }

    /*
    **	Ask windows to translate the key into an ASCII equivalent.
    */
    char buffer[10];
    int result = 1;
    int scancode = 0;

    /*
    **	Set the KeyState buffer to reflect the shift bits stored in the key value.
    */
    if (key & WWKEY_SHIFT_BIT) {
        KeyState[VK_SHIFT] = 0x80;
    }
    if (key & WWKEY_CTRL_BIT) {
        KeyState[VK_CONTROL] = 0x80;
    }
    if (key & WWKEY_ALT_BIT) {
        KeyState[VK_MENU] = 0x80;
    }

    scancode = MapVirtualKeyA(key & 0xFF, 0);
    result = ToAscii((UINT)(key & 0xFF), (UINT)scancode, (PBYTE)KeyState, (LPWORD)buffer, (UINT)0);

    /*
    **	Restore the KeyState buffer back to pristine condition.
    */
    if (key & WWKEY_SHIFT_BIT) {
        KeyState[VK_SHIFT] = 0;
    }
    if (key & WWKEY_CTRL_BIT) {
        KeyState[VK_CONTROL] = 0;
    }
    if (key & WWKEY_ALT_BIT) {
        KeyState[VK_MENU] = 0;
    }

    /*
    **	If Windows could not perform the translation as expected, then
    **	return with a null ASCII value.
    */
    if (result != 1) {
        return KA_NONE;
    }

    return (KeyASCIIType)(buffer[0]);
}

WWKeyboardClass* CreateWWKeyboardClass(void)
{
    return new WWKeyboardClassWin32;
}
