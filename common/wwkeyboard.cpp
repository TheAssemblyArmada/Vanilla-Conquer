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

/* $Header: /CounterStrike/KEY.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Keyboard Library                                    *
 *                                                                                             *
 *                    File Name : KEYBOARD.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Philip W. Gorrow                                             *
 *                                                                                             *
 *                   Start Date : 10/16/95                                                     *
 *                                                                                             *
 *                  Last Update : November 2, 1996 [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WWKeyboardClass::Buff_Get -- Lowlevel function to get a key from key buffer               *
 *   WWKeyboardClass::Check -- Checks to see if a key is in the buffer                         *
 *   WWKeyboardClass::Clear -- Clears the keyboard buffer.                                     *
 *   WWKeyboardClass::Down -- Checks to see if the specified key is being held down.           *
 *   WWKeyboardClass::Fetch_Element -- Extract the next element in the keyboard buffer.        *
 *   WWKeyboardClass::Fill_Buffer_From_Syste -- Extract and process any queued windows messages*
 *   WWKeyboardClass::Get -- Logic to get a metakey from the buffer                            *
 *   WWKeyboardClass::Get_Mouse_X -- Returns the mouses current x position in pixels           *
 *   WWKeyboardClass::Get_Mouse_XY -- Returns the mouses x,y position via reference vars       *
 *   WWKeyboardClass::Get_Mouse_Y -- returns the mouses current y position in pixels           *
 *   WWKeyboardClass::Is_Buffer_Empty -- Checks to see if the keyboard buffer is empty.        *
 *   WWKeyboardClass::Is_Buffer_Full -- Determines if the keyboard buffer is full.             *
 *   WWKeyboardClass::Is_Mouse_Key -- Checks to see if specified key refers to the mouse.      *
 *   WWKeyboardClass::Message_Handler -- Process a windows message as it relates to the keyboar*
 *   WWKeyboardClass::Peek_Element -- Fetches the next element in the keyboard buffer.         *
 *   WWKeyboardClass::Put -- Logic to insert a key into the keybuffer]                         *
 *   WWKeyboardClass::Put_Element -- Put a keyboard data element into the buffer.              *
 *   WWKeyboardClass::Put_Key_Message -- Translates and inserts wParam into Keyboard Buffer    *
 *   WWKeyboardClass::To_ASCII -- Convert the key value into an ASCII representation.          *
 *   WWKeyboardClass::Available_Buffer_Room -- Fetch the quantity of free elements in the keybo*
 *   WWKeyboardClass::Put_Mouse_Message -- Stores a mouse type message into the keyboard buffer*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "wwkeyboard.h"
#include "video.h"
#include "miscasm.h"
#include <string.h>
#ifdef SDL2_BUILD
#include <SDL.h>
#include "sdl_keymap.h"
#endif
#include "settings.h"

#define ARRAY_SIZE(x) int(sizeof(x) / sizeof(x[0]))

/*
**  Focus handlers for both games.
*/
extern void Focus_Loss();
extern void Focus_Restore();

/***********************************************************************************************
 * WWKeyboardClass::WWKeyBoardClass -- Construction for Westwood Keyboard Class                *
 *                                                                                             *
 * INPUT:		none							                                                        *
 *                                                                                             *
 * OUTPUT:     none							                                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
WWKeyboardClass::WWKeyboardClass(void)
    : MouseQX(0)
    , MouseQY(0)
    , Head(0)
    , Tail(0)
    , DownSkip(0)
{
    memset(KeyState, '\0', sizeof(KeyState));
    memset(DownState, '\0', sizeof(DownState));
}

/***********************************************************************************************
 * WWKeyboardClass::Buff_Get -- Lowlevel function to get a key from key buffer                 *
 *                                                                                             *
 * INPUT:		none                                                        						  *
 *                                                                                             *
 * OUTPUT:     int		- the key value that was pulled from buffer (includes bits)				  * *
 *                                                                                             *
 * WARNINGS:   If the key was a mouse event MouseQX and MouseQY will be updated                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
unsigned short WWKeyboardClass::Buff_Get(void)
{
    while (!Check()) {
    } // wait for key in buffer

    unsigned short temp = Fetch_Element();
    if (Is_Mouse_Key(temp)) {
        MouseQX = Fetch_Element();
        MouseQY = Fetch_Element();
    }
    return (temp);
}

/***********************************************************************************************
 * WWKeyboardClass::Is_Mouse_Key -- Checks to see if specified key refers to the mouse.        *
 *                                                                                             *
 *    This checks the specified key code to see if it refers to the mouse buttons.             *
 *                                                                                             *
 * INPUT:   key   -- The key to check.                                                         *
 *                                                                                             *
 * OUTPUT:  bool; Is the key a mouse button key?                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Is_Mouse_Key(unsigned short key)
{
    key &= 0xFF;
    return (key == VK_LBUTTON || key == VK_MBUTTON || key == VK_RBUTTON);
}

/***********************************************************************************************
 * WWKeyboardClass::Check -- Checks to see if a key is in the buffer                           *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *   09/24/1996 JLB : Converted to new style keyboard system.                                  *
 *=============================================================================================*/
KeyNumType WWKeyboardClass::Check(void) const
{
    ((WWKeyboardClass*)this)->Fill_Buffer_From_System();
    if (Is_Buffer_Empty())
        return KN_NONE;
    return (KeyNumType)(Peek_Element());
}

/***********************************************************************************************
 * WWKeyboardClass::Get -- Logic to get a metakey from the buffer                              *
 *                                                                                             *
 * INPUT:		none                                                        						  *
 *                                                                                             *
 * OUTPUT:     int		- the meta key taken from the buffer.											  *
 *                                                                                             *
 * WARNINGS:	This routine will not return until a keypress is received							  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
KeyNumType WWKeyboardClass::Get(void)
{
    while (!Check()) {
    } // wait for key in buffer
    return (KeyNumType)(Buff_Get());
}

/***********************************************************************************************
 * WWKeyboardClass::Put -- Logic to insert a key into the keybuffer]                           *
 *                                                                                             *
 * INPUT:		int	 	- the key to insert into the buffer          								  *
 *                                                                                             *
 * OUTPUT:     bool		- true if key is sucessfuly inserted.							              *
 *                                                                                             *
 * WARNINGS:   none							                                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Put(unsigned short key)
{
    if (!Is_Buffer_Full()) {
        Put_Element(key);
        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * WWKeyboardClass::Put_Key_Message -- Translates and inserts wParam into Keyboard Buffer      *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Put_Key_Message(unsigned short vk_key, bool release)
{
    /*
    ** Get the status of all of the different keyboard modifiers.  Note, only pay attention
    ** to numlock and caps lock if we are dealing with a key that is affected by them.  Note
    ** that we do not want to set the shift, ctrl and alt bits for Mouse keypresses as this
    ** would be incompatible with the dos version.
    */
    if (!Is_Mouse_Key(vk_key)) {
        if (Down(VK_SHIFT) || Down(VK_CAPITAL) || Down(VK_NUMLOCK)) {
            vk_key |= WWKEY_SHIFT_BIT;
        }
        if (Down(VK_CONTROL)) {
            vk_key |= WWKEY_CTRL_BIT;
        }
        if (Down(VK_MENU)) {
            vk_key |= WWKEY_ALT_BIT;
        }
    }

    if (release) {
        vk_key |= WWKEY_RLS_BIT;
    }

    /*
    ** Finally use the put command to enter the key into the keyboard
    ** system.
    */
    return (Put(vk_key));
}

/***********************************************************************************************
 * WWKeyboardClass::Put_Mouse_Message -- Stores a mouse type message into the keyboard buffer. *
 *                                                                                             *
 *    This routine will store the mouse type event into the keyboard buffer. It also checks    *
 *    to ensure that there is enough room in the buffer so that partial mouse events won't     *
 *    be recorded.                                                                             *
 *                                                                                             *
 * INPUT:   vk_key   -- The mouse key message itself.                                          *
 *                                                                                             *
 *          x,y      -- The mouse coordinates at the time of the event.                        *
 *                                                                                             *
 *          release  -- Is this a mouse button release?                                        *
 *                                                                                             *
 * OUTPUT:  bool; Was the event stored sucessfully into the keyboard buffer?                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Put_Mouse_Message(unsigned short vk_key, int x, int y, bool release)
{
    if (Available_Buffer_Room() >= 3 && Is_Mouse_Key(vk_key)) {
        Put_Key_Message(vk_key, release);
        Put((unsigned short)x);
        Put((unsigned short)y);
        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * WWKeyboardClass::To_ASCII -- Convert the key value into an ASCII representation.            *
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
KeyASCIIType WWKeyboardClass::To_ASCII(unsigned short key)
{
    /*
    **	Released keys never translate into an ASCII value.
    */
    if (key & WWKEY_RLS_BIT) {
        return KA_NONE;
    }

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

    /*
    **	Ask windows to translate the key into an ASCII equivalent.
    */
    char buffer[10];
    int result = 1;
    int scancode = 0;

#if defined(SDL2_BUILD)
    key &= 0xFF; // drop all mods

    if (key > ARRAY_SIZE(sdl_keymap) / 2 - 1) {
        return KA_NONE;
    }

    if (SDL_GetModState() & KMOD_SHIFT) {
        return sdl_keymap[key + ARRAY_SIZE(sdl_keymap) / 2];
    } else {
        return sdl_keymap[key];
    }
#elif defined(_WIN32)
    scancode = MapVirtualKeyA(key & 0xFF, 0);
    result = ToAscii((UINT)(key & 0xFF), (UINT)scancode, (PBYTE)KeyState, (LPWORD)buffer, (UINT)0);
#endif

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

/***********************************************************************************************
 * WWKeyboardClass::Down -- Checks to see if the specified key is being held down.             *
 *                                                                                             *
 *    This routine will examine the key specified to see if it is currently being held down.   *
 *                                                                                             *
 * INPUT:   key   -- The key to check.                                                         *
 *                                                                                             *
 * OUTPUT:  bool; Is the specified key currently being held down?                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Down(unsigned short key)
{
    return Get_Bit(DownState, key);
}

/***********************************************************************************************
 * WWKeyboardClass::Fetch_Element -- Extract the next element in the keyboard buffer.          *
 *                                                                                             *
 *    This routine will extract the next pending element in the keyboard queue. If there is    *
 *    no element available, then NULL is returned.                                             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the element extracted from the queue. An empty queue is signified     *
 *          by a 0 return value.                                                               *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
unsigned short WWKeyboardClass::Fetch_Element(void)
{
    unsigned short val = 0;
    if (Head != Tail) {
        val = Buffer[Head];

        Head = (Head + 1) % ARRAY_SIZE(Buffer);
    }
    return (val);
}

/***********************************************************************************************
 * WWKeyboardClass::Peek_Element -- Fetches the next element in the keyboard buffer.           *
 *                                                                                             *
 *    This routine will examine and return with the next element in the keyboard buffer but    *
 *    it will not alter or remove that element. Use this routine to see what is pending in     *
 *    the keyboard queue.                                                                      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the next element in the keyboard queue. If the keyboard buffer is     *
 *          empty, then 0 is returned.                                                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
unsigned short WWKeyboardClass::Peek_Element(void) const
{
    if (!Is_Buffer_Empty()) {
        return (Buffer[Head]);
    }
    return (0);
}

/***********************************************************************************************
 * WWKeyboardClass::Put_Element -- Put a keyboard data element into the buffer.                *
 *                                                                                             *
 *    This will put one keyboard data element into the keyboard buffer. Typically, this data   *
 *    is a key code, but it might be mouse coordinates.                                        *
 *                                                                                             *
 * INPUT:   val   -- The data element to add to the keyboard buffer.                           *
 *                                                                                             *
 * OUTPUT:  bool; Was the keyboard element added successfully? A failure would indicate that   *
 *                the keyboard buffer is full.                                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Put_Element(unsigned short val)
{
    if (!Is_Buffer_Full()) {
        int temp = (Tail + 1) % ARRAY_SIZE(Buffer);
        Buffer[Tail] = val;
        Tail = temp;

        /* set cached down state for given key, remove all bits */
        if (DownSkip == 0) {
            bool isDown = !(val & KN_RLSE_BIT);
            val &= ~(KN_SHIFT_BIT | KN_CTRL_BIT | KN_ALT_BIT | KN_RLSE_BIT | KN_BUTTON);
            Set_Bit(DownState, val, isDown);

            if (Is_Mouse_Key(val)) {
                DownSkip = 2;
            }
        } else {
            DownSkip--;
        }

        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * WWKeyboardClass::Is_Buffer_Full -- Determines if the keyboard buffer is full.               *
 *                                                                                             *
 *    This routine will examine the keyboard buffer to determine if it is completely           *
 *    full of queued keyboard events.                                                          *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the keyboard buffer completely full?                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Is_Buffer_Full(void) const
{
    if ((Tail + 1) % ARRAY_SIZE(Buffer) == Head) {
        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * WWKeyboardClass::Is_Buffer_Empty -- Checks to see if the keyboard buffer is empty.          *
 *                                                                                             *
 *    This routine will examine the keyboard buffer to see if it contains no events at all.    *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the keyboard buffer currently without any pending events queued?          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool WWKeyboardClass::Is_Buffer_Empty(void) const
{
    if (Head == Tail) {
        return (true);
    }
    return (false);
}

/***********************************************************************************************
 * WWKeyboardClass::Fill_Buffer_From_Syste -- Extract and process any queued windows messages. *
 *                                                                                             *
 *    This routine will extract and process any windows messages in the windows message        *
 *    queue. It is presumed that the normal message handler will call the keyboard             *
 *    message processing function.                                                             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void Process_Network();

void WWKeyboardClass::Fill_Buffer_From_System(void)
{
#ifdef SDL2_BUILD
    Process_Network();
    SDL_Event event;

    while (!Is_Buffer_Full() && SDL_PollEvent(&event)) {
        unsigned short key;
        switch (event.type) {
        case SDL_QUIT:
            exit(0);
            break;
        case SDL_KEYDOWN:
            Put_Key_Message(event.key.keysym.scancode, false);
            break;
        case SDL_KEYUP:
            if (event.key.keysym.scancode == SDL_SCANCODE_RETURN && Down(VK_MENU)) {
                Toggle_Video_Fullscreen();
            } else {
                Put_Key_Message(event.key.keysym.scancode, true);
            }
            break;
        case SDL_MOUSEMOTION:
            Move_Video_Mouse(static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel));
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            int x, y;

            switch (event.button.button) {
            case SDL_BUTTON_LEFT:
            default:
                key = VK_LBUTTON;
                break;
            case SDL_BUTTON_RIGHT:
                key = VK_RBUTTON;
                break;
            case SDL_BUTTON_MIDDLE:
                key = VK_MBUTTON;
                break;
            }

            if (Settings.Mouse.RawInput || Is_Gamepad_Active()) {
                Get_Video_Mouse(x, y);
            } else {
                float scale_x = 1.0f, scale_y = 1.0f;
                Get_Video_Scale(scale_x, scale_y);
                x = event.button.x / scale_x;
                y = event.button.y / scale_y;
            }

            Put_Mouse_Message(key, x, y, event.type == SDL_MOUSEBUTTONDOWN ? false : true);
        } break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_EXPOSED:
            case SDL_WINDOWEVENT_RESTORED:
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                Focus_Restore();
                break;
            case SDL_WINDOWEVENT_HIDDEN:
            case SDL_WINDOWEVENT_MINIMIZED:
            case SDL_WINDOWEVENT_FOCUS_LOST:
                Focus_Loss();
                break;
            }
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (GameController != nullptr) {
                const SDL_GameController* removedController = SDL_GameControllerFromInstanceID(event.jdevice.which);
                if (removedController == GameController) {
                    SDL_GameControllerClose(GameController);
                    GameController = nullptr;
                }
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if (GameController == nullptr) {
                GameController = SDL_GameControllerOpen(event.jdevice.which);
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            Handle_Controller_Axis_Event(event.caxis);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            Handle_Controller_Button_Event(event.cbutton);
            break;
        }
    }
    if (Is_Gamepad_Active()) {
        Process_Controller_Axis_Motion();
    }
#elif defined(_WIN32)
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
#endif
}

#ifdef SDL2_BUILD
bool WWKeyboardClass::Is_Gamepad_Active()
{
    return GameController != nullptr;
}

void WWKeyboardClass::Open_Controller()
{
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            GameController = SDL_GameControllerOpen(i);
        }
    }
}

void WWKeyboardClass::Close_Controller()
{
    if (SDL_GameControllerGetAttached(GameController)) {
        SDL_GameControllerClose(GameController);
        GameController = nullptr;
    }
}

void WWKeyboardClass::Process_Controller_Axis_Motion()
{
    const uint32_t currentTime = SDL_GetTicks();
    const double deltaTime = currentTime - LastControllerTime;
    LastControllerTime = currentTime;

    if (ControllerLeftXAxis != 0 || ControllerLeftYAxis != 0) {
        const int16_t xSign = (ControllerLeftXAxis > 0) - (ControllerLeftXAxis < 0);
        const int16_t ySign = (ControllerLeftYAxis > 0) - (ControllerLeftYAxis < 0);

        float movX = std::pow(std::abs(ControllerLeftXAxis), CONTROLLER_AXIS_SPEEDUP) * xSign * deltaTime
                     * Settings.Mouse.ControllerPointerSpeed / CONTROLLER_SPEED_MOD * ControllerSpeedBoost;
        float movY = std::pow(std::abs(ControllerLeftYAxis), CONTROLLER_AXIS_SPEEDUP) * ySign * deltaTime
                     * Settings.Mouse.ControllerPointerSpeed / CONTROLLER_SPEED_MOD * ControllerSpeedBoost;

        Move_Video_Mouse(movX, movY);
    }
}

void WWKeyboardClass::Handle_Controller_Axis_Event(const SDL_ControllerAxisEvent& motion)
{
    AnalogScrollActive = false;
    ScrollDirType directionX = SDIR_NONE;
    ScrollDirType directionY = SDIR_NONE;

    if (motion.axis == SDL_CONTROLLER_AXIS_LEFTX) {
        if (std::abs(motion.value) > CONTROLLER_L_DEADZONE)
            ControllerLeftXAxis = motion.value;
        else
            ControllerLeftXAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_LEFTY) {
        if (std::abs(motion.value) > CONTROLLER_L_DEADZONE)
            ControllerLeftYAxis = motion.value;
        else
            ControllerLeftYAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_RIGHTX) {
        if (std::abs(motion.value) > CONTROLLER_R_DEADZONE)
            ControllerRightXAxis = motion.value;
        else
            ControllerRightXAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_RIGHTY) {
        if (std::abs(motion.value) > CONTROLLER_R_DEADZONE)
            ControllerRightYAxis = motion.value;
        else
            ControllerRightYAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
        if (std::abs(motion.value) > CONTROLLER_TRIGGER_R_DEADZONE)
            ControllerSpeedBoost = 1 + (static_cast<float>(motion.value) / 32767) * CONTROLLER_TRIGGER_SPEEDUP;
        else
            ControllerSpeedBoost = 1;
    }

    if (ControllerRightXAxis != 0) {
        AnalogScrollActive = true;
        directionX = ControllerRightXAxis > 0 ? SDIR_E : SDIR_W;
    }
    if (ControllerRightYAxis != 0) {
        AnalogScrollActive = true;
        directionY = ControllerRightYAxis > 0 ? SDIR_S : SDIR_N;
    }

    if (directionX == SDIR_E && directionY == SDIR_N)
        ScrollDirection = SDIR_NE;
    else if (directionX == SDIR_E && directionY == SDIR_S)
        ScrollDirection = SDIR_SE;
    else if (directionX == SDIR_W && directionY == SDIR_N)
        ScrollDirection = SDIR_NW;
    else if (directionX == SDIR_W && directionY == SDIR_S)
        ScrollDirection = SDIR_SW;
    else if (directionX == SDIR_E)
        ScrollDirection = SDIR_E;
    else if (directionX == SDIR_W)
        ScrollDirection = SDIR_W;
    else if (directionY == SDIR_S)
        ScrollDirection = SDIR_S;
    else if (directionY == SDIR_N)
        ScrollDirection = SDIR_N;
}

void WWKeyboardClass::Handle_Controller_Button_Event(const SDL_ControllerButtonEvent& button)
{
    bool keyboardPress = false;
    bool mousePress = false;
    unsigned short key;
    SDL_Scancode scancode;

    switch (button.button) {
    case SDL_CONTROLLER_BUTTON_A:
        mousePress = true;
        key = VK_LBUTTON;
        break;
    case SDL_CONTROLLER_BUTTON_B:
        mousePress = true;
        key = VK_RBUTTON;
        break;
    case SDL_CONTROLLER_BUTTON_X:
        keyboardPress = true;
        scancode = SDL_SCANCODE_G;
        break;
    case SDL_CONTROLLER_BUTTON_Y:
        keyboardPress = true;
        scancode = SDL_SCANCODE_F;
        break;
    case SDL_CONTROLLER_BUTTON_BACK:
        keyboardPress = true;
        scancode = SDL_SCANCODE_ESCAPE;
        break;
    case SDL_CONTROLLER_BUTTON_START:
        keyboardPress = true;
        scancode = SDL_SCANCODE_RETURN;
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        keyboardPress = true;
        scancode = SDL_SCANCODE_LCTRL;
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        keyboardPress = true;
        scancode = SDL_SCANCODE_LALT;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        keyboardPress = true;
        scancode = SDL_SCANCODE_1;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        keyboardPress = true;
        scancode = SDL_SCANCODE_2;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        keyboardPress = true;
        scancode = SDL_SCANCODE_3;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        keyboardPress = true;
        scancode = SDL_SCANCODE_4;
        break;
    default:
        break;
    }

    if (keyboardPress) {
        Put_Key_Message(scancode, button.state == SDL_RELEASED);
    } else if (mousePress) {
        int x, y;
        Get_Video_Mouse(x, y);
        Put_Mouse_Message(key, x, y, button.state == SDL_RELEASED);
    }
}

bool WWKeyboardClass::Is_Analog_Scroll_Active()
{
    return AnalogScrollActive;
}

unsigned char WWKeyboardClass::Get_Scroll_Direction()
{
    return ScrollDirection;
}
#endif

/***********************************************************************************************
 * WWKeyboardClass::Clear -- Clears the keyboard buffer.                                       *
 *                                                                                             *
 *    This routine will clear the keyboard buffer of all pending keyboard events.              *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void WWKeyboardClass::Clear(void)
{
    /*
    **	Extract any windows pending keyboard message events and then clear out the keyboard
    **	buffer.
    */
    Fill_Buffer_From_System();
    Head = Tail;

    /*
    **	Perform a second clear to handle the rare case of the keyboard buffer being full and there
    **	still remains keyboard related events in the windows message queue.
    */
    Fill_Buffer_From_System();
    Head = Tail;

    /*
    **  Reset all keys to not being held down to prevent stuck modifiers.
    */
    memset(DownState, '\0', sizeof(DownState));
}

/***********************************************************************************************
 * WWKeyboardClass::Message_Handler -- Process a windows message as it relates to the keyboard *
 *                                                                                             *
 *    This routine will examine the Windows message specified. If the message relates to an    *
 *    event that the keyboard input system needs to process, then it will be processed         *
 *    accordingly.                                                                             *
 *                                                                                             *
 * INPUT:   window   -- Handle to the window receiving the message.                            *
 *                                                                                             *
 *          message  -- The message number of this event.                                      *
 *                                                                                             *
 *          wParam   -- The windows specific word parameter (meaning depends on message).      *
 *                                                                                             *
 *          lParam   -- The windows specific long word parameter (meaning is message dependant)*
 *                                                                                             *
 * OUTPUT:  bool; Was this keyboard message recognized and processed? A 'false' return value   *
 *                means that the message should be processed normally.                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/30/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
#if defined(_WIN32) && !defined(SDL2_BUILD)
bool WWKeyboardClass::Message_Handler(HWND window, UINT message, UINT wParam, LONG lParam)
{
// ST - 5/13/2019
#ifndef REMASTER_BUILD // #if (0)
    bool processed = false;

    /*
    **	Examine the message to see if it is one that should be processed. Only keyboard and
    **	pertinant mouse messages are processed.
    */
    switch (message) {

    /*
    **	System key has been pressed. This is the normal keyboard event message.
    */
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        Put_Key_Message((unsigned short)wParam);
        processed = true;
        break;

    /*
    **	The key has been released. This is the normal key release message.
    */
    case WM_SYSKEYUP:
    case WM_KEYUP:
        Put_Key_Message((unsigned short)wParam, true);
        processed = true;
        break;

    /*
    **	Press of the left mouse button.
    */
    case WM_LBUTTONDOWN:
        Put_Mouse_Message(VK_LBUTTON, LOWORD(lParam), HIWORD(lParam));
        processed = true;
        break;

    /*
    **	Release of the left mouse button.
    */
    case WM_LBUTTONUP:
        Put_Mouse_Message(VK_LBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        processed = true;
        break;

    /*
    **	Double click of the left mouse button. Fake this into being
    **	just a rapid click of the left button twice.
    */
    case WM_LBUTTONDBLCLK:
        Put_Mouse_Message(VK_LBUTTON, LOWORD(lParam), HIWORD(lParam));
        Put_Mouse_Message(VK_LBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        Put_Mouse_Message(VK_LBUTTON, LOWORD(lParam), HIWORD(lParam));
        Put_Mouse_Message(VK_LBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        processed = true;
        break;

    /*
    **	Press of the middle mouse button.
    */
    case WM_MBUTTONDOWN:
        Put_Mouse_Message(VK_MBUTTON, LOWORD(lParam), HIWORD(lParam));
        processed = true;
        break;

    /*
    **	Release of the middle mouse button.
    */
    case WM_MBUTTONUP:
        Put_Mouse_Message(VK_MBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        processed = true;
        break;

    /*
    **	Middle button double click gets translated into two
    **	regular middle button clicks.
    */
    case WM_MBUTTONDBLCLK:
        Put_Mouse_Message(VK_MBUTTON, LOWORD(lParam), HIWORD(lParam));
        Put_Mouse_Message(VK_MBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        Put_Mouse_Message(VK_MBUTTON, LOWORD(lParam), HIWORD(lParam));
        Put_Mouse_Message(VK_MBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        processed = true;
        break;

    /*
    **	Right mouse button press.
    */
    case WM_RBUTTONDOWN:
        Put_Mouse_Message(VK_RBUTTON, LOWORD(lParam), HIWORD(lParam));
        processed = true;
        break;

    /*
    **	Right mouse button release.
    */
    case WM_RBUTTONUP:
        Put_Mouse_Message(VK_RBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        processed = true;
        break;

    /*
    **	Translate a double click of the right button
    **	into being just two regular right button clicks.
    */
    case WM_RBUTTONDBLCLK:
        Put_Mouse_Message(VK_RBUTTON, LOWORD(lParam), HIWORD(lParam));
        Put_Mouse_Message(VK_RBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        Put_Mouse_Message(VK_RBUTTON, LOWORD(lParam), HIWORD(lParam));
        Put_Mouse_Message(VK_RBUTTON, LOWORD(lParam), HIWORD(lParam), true);
        processed = true;
        break;

    /*
    **	If the message is not pertinant to the keyboard system,
    **	then do nothing.
    */
    default:
        break;
    }

    /*
    **	If this message has been processed, then pass it on to the system
    **	directly.
    */
    if (processed) {
        DefWindowProcA(window, message, wParam, lParam);
        return (true);
    }
#endif
    return (false);
}
#endif

/***********************************************************************************************
 * WWKeyboardClass::Available_Buffer_Room -- Fetch the quantity of free elements in the keyboa *
 *                                                                                             *
 *    This examines the keyboard buffer queue and determine how many elements are available    *
 *    for use before the buffer becomes full. Typical use of this would be when inserting      *
 *    mouse events that require more than one element. Such an event must detect when there    *
 *    would be insufficient room in the buffer and bail accordingly.                           *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the number of elements that may be stored in to the keyboard buffer   *
 *          before it becomes full and cannot accept any more elements.                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int WWKeyboardClass::Available_Buffer_Room(void) const
{
    int avail;
    if (Head == Tail) {
        avail = ARRAY_SIZE(Buffer);
    }
    if (Head < Tail) {
        avail = Tail - Head;
    }
    if (Head > Tail) {
        avail = (Tail + ARRAY_SIZE(Buffer)) - Head;
    }
    return (avail);
}
