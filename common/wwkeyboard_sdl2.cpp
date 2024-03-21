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

#include "macros.h"
#include "wwkeyboard_sdl2.h"
#include "video.h"
#include "sdl_keymap.h"
#include "settings.h"
#include <cmath>
#include <SDL.h>

void Focus_Loss();
void Focus_Restore();
void Process_Network();

WWKeyboardClassSDL2::~WWKeyboardClassSDL2()
{
}

void WWKeyboardClassSDL2::Fill_Buffer_From_System(void)
{
#ifdef NETWORKING
    Process_Network();
#endif
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
        case SDL_MOUSEWHEEL:
            if (event.wheel.y > 0) { // scroll up
                Put_Key_Message(VK_MOUSEWHEEL_UP, false);
            } else if (event.wheel.y < 0) { // scroll down
                Put_Key_Message(VK_MOUSEWHEEL_DOWN, false);
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
}

bool WWKeyboardClassSDL2::Is_Gamepad_Active()
{
    return GameController != nullptr;
}

void WWKeyboardClassSDL2::Open_Controller()
{
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            GameController = SDL_GameControllerOpen(i);
        }
    }
}

void WWKeyboardClassSDL2::Close_Controller()
{
    if (SDL_GameControllerGetAttached(GameController)) {
        SDL_GameControllerClose(GameController);
        GameController = nullptr;
    }
}

void WWKeyboardClassSDL2::Process_Controller_Axis_Motion()
{
    const uint32_t currentTime = SDL_GetTicks();
    const float deltaTime = currentTime - LastControllerTime;
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

void WWKeyboardClassSDL2::Handle_Controller_Axis_Event(const SDL_ControllerAxisEvent& motion)
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

    if (directionX == SDIR_E && directionY == SDIR_N) {
        ScrollDirection = SDIR_NE;
    } else if (directionX == SDIR_E && directionY == SDIR_S) {
        ScrollDirection = SDIR_SE;
    } else if (directionX == SDIR_W && directionY == SDIR_N) {
        ScrollDirection = SDIR_NW;
    } else if (directionX == SDIR_W && directionY == SDIR_S) {
        ScrollDirection = SDIR_SW;
    } else if (directionX == SDIR_E) {
        ScrollDirection = SDIR_E;
    } else if (directionX == SDIR_W) {
        ScrollDirection = SDIR_W;
    } else if (directionY == SDIR_S) {
        ScrollDirection = SDIR_S;
    } else if (directionY == SDIR_N) {
        ScrollDirection = SDIR_N;
    }
}

void WWKeyboardClassSDL2::Handle_Controller_Button_Event(const SDL_ControllerButtonEvent& button)
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

bool WWKeyboardClassSDL2::Is_Analog_Scroll_Active()
{
    return AnalogScrollActive;
}

unsigned char WWKeyboardClassSDL2::Get_Scroll_Direction()
{
    return ScrollDirection;
}

KeyASCIIType WWKeyboardClassSDL2::To_ASCII(unsigned short key)
{
    if (key & WWKEY_RLS_BIT) {
        return KA_NONE;
    }

    key &= 0xFF; // drop all mods

    if (key > ARRAY_SIZE(sdl_keymap) / 2 - 1) {
        return KA_NONE;
    }

    if (SDL_GetModState() & KMOD_SHIFT) {
        return sdl_keymap[key + ARRAY_SIZE(sdl_keymap) / 2];
    } else {
        return sdl_keymap[key];
    }
}

WWKeyboardClass* CreateWWKeyboardClass(void)
{
    return new WWKeyboardClassSDL2;
}
