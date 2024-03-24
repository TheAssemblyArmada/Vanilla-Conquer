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

/* $Header: /CounterStrike/KEY.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Keyboard Library                                    *
 *                                                                                             *
 *                    File Name : KEYBOARD.H                                                   *
 *                                                                                             *
 *                   Programmer : Philip W. Gorrow                                             *
 *                                                                                             *
 *                   Start Date : 10/16/95                                                     *
 *                                                                                             *
 *                  Last Update : October 16, 1995 [PWG]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#if defined(_WIN32) && !defined(SDL_BUILD)
#include <windows.h>
#endif
#include <stdint.h>

#ifdef SDL_BUILD
#define SDL_MAIN_HANDLED
#include <SDL.h>
#endif

typedef enum KeyASCIIType : unsigned short
{
    //
    // Define all the KA types as variations of the VK types.  This is
    // so the KA functions will work properly under windows 95.
    //
    KA_NONE = 0,
    KA_MORE = 1,
    KA_SETBKGDCOL = 2,
    KA_SETFORECOL = 6,
    KA_FORMFEED = 12,
    KA_SPCTAB = 20,
    KA_SETX = 25,
    KA_SETY = 26,

    KA_SPACE = ' ',
    KA_EXCLAMATION = '!',
    KA_DQUOTE = '"',
    KA_HASH = '#',
    KA_DOLLAR = '$',
    KA_PERCENT = '%',
    KA_AMPER = '&',
    KA_SQUOTE = '\'',
    KA_LPAREN = '(',
    KA_RPAREN = ')',
    KA_ASTERISK = '*',
    KA_PLUS = '+',
    KA_COMMA = ',',
    KA_MINUS = '-',
    KA_PERIOD = '.',
    KA_SLASH = '/',

    KA_0 = '0',
    KA_1 = '1',
    KA_2 = '2',
    KA_3 = '3',
    KA_4 = '4',
    KA_5 = '5',
    KA_6 = '6',
    KA_7 = '7',
    KA_8 = '8',
    KA_9 = '9',
    KA_COLON = ':',
    KA_SEMICOLON = ';',
    KA_LESS_THAN = '<',
    KA_EQUAL = '=',
    KA_GREATER_THAN = '>',
    KA_QUESTION = '?',

    KA_AT = '@',
    KA_A = 'A',
    KA_B = 'B',
    KA_C = 'C',
    KA_D = 'D',
    KA_E = 'E',
    KA_F = 'F',
    KA_G = 'G',
    KA_H = 'H',
    KA_I = 'I',
    KA_J = 'J',
    KA_K = 'K',
    KA_L = 'L',
    KA_M = 'M',
    KA_N = 'N',
    KA_O = 'O',

    KA_P = 'P',
    KA_Q = 'Q',
    KA_R = 'R',
    KA_S = 'S',
    KA_T = 'T',
    KA_U = 'U',
    KA_V = 'V',
    KA_W = 'W',
    KA_X = 'X',
    KA_Y = 'Y',
    KA_Z = 'Z',
    KA_LBRACKET = '[',
    KA_BACKSLASH = '\\',
    KA_RBRACKET = ']',
    KA_CARET = '^',
    KA_UNDERLINE = '_',

    KA_GRAVE = '`',
    KA_a = 'a',
    KA_b = 'b',
    KA_c = 'c',
    KA_d = 'd',
    KA_e = 'e',
    KA_f = 'f',
    KA_g = 'g',
    KA_h = 'h',
    KA_i = 'i',
    KA_j = 'j',
    KA_k = 'k',
    KA_l = 'l',
    KA_m = 'm',
    KA_n = 'n',
    KA_o = 'o',

    KA_p = 'p',
    KA_q = 'q',
    KA_r = 'r',
    KA_s = 's',
    KA_t = 't',
    KA_u = 'u',
    KA_v = 'v',
    KA_w = 'w',
    KA_x = 'x',
    KA_y = 'y',
    KA_z = 'z',
    KA_LBRACE = '{',
    KA_BAR = '|',
    KA_RBRACE = '}',
    KA_TILDA = '~',

    KA_ESC = 0x1B,
    KA_EXTEND = 0x1B,
    KA_RETURN = 0x0D,
    KA_BACKSPACE = 0x08,
    KA_TAB = 0x09,
    KA_DELETE = 0x7F,
} KeyASCIIType;

#if defined(SDL1_BUILD)

#include <SDL_keysym.h>

typedef enum KeyNumType : unsigned short
{
    KN_NONE = SDLK_UNKNOWN,

    KN_0 = SDLK_0,
    KN_1 = SDLK_1,
    KN_2 = SDLK_2,
    KN_3 = SDLK_3,
    KN_4 = SDLK_4,
    KN_5 = SDLK_5,
    KN_6 = SDLK_6,
    KN_7 = SDLK_7,
    KN_8 = SDLK_8,
    KN_9 = SDLK_9,
    KN_A = SDLK_a,
    KN_B = SDLK_b,
    KN_BACKSLASH = SDLK_BACKSLASH,
    KN_BACKSPACE = SDLK_BACKSPACE,
    KN_C = SDLK_c,
    KN_CAPSLOCK = SDLK_CAPSLOCK,
    KN_CENTER = SDLK_CLEAR,
    KN_COMMA = SDLK_COMMA,
    KN_D = SDLK_d,
    KN_DELETE = SDLK_DELETE,
    KN_DOWN = SDLK_DOWN,
    KN_E = SDLK_e,
    KN_END = SDLK_END,
    KN_EQUAL = SDLK_EQUALS,
    KN_ESC = SDLK_ESCAPE,
    KN_E_DELETE = SDLK_KP0,
    KN_E_DOWN = SDLK_KP2,
    KN_E_END = SDLK_KP1,
    KN_E_HOME = SDLK_KP7,
    KN_E_INSERT = SDLK_KP_PERIOD,
    KN_E_LEFT = SDLK_KP4,
    KN_E_PGDN = SDLK_KP3,
    KN_E_PGUP = SDLK_KP9,
    KN_E_RIGHT = SDLK_KP6,
    KN_E_UP = SDLK_KP8,
    KN_SELECT = SDLK_KP5,
    KN_F = SDLK_f,
    KN_F1 = SDLK_F1,
    KN_F10 = SDLK_F10,
    KN_F11 = SDLK_F11,
    KN_F12 = SDLK_F12,
    KN_F2 = SDLK_F2,
    KN_F3 = SDLK_F3,
    KN_F4 = SDLK_F4,
    KN_F5 = SDLK_F5,
    KN_F6 = SDLK_F6,
    KN_F7 = SDLK_F7,
    KN_F8 = SDLK_F8,
    KN_F9 = SDLK_F9,
    KN_G = SDLK_g,
    KN_GRAVE = SDLK_BACKQUOTE,
    KN_H = SDLK_h,
    KN_HOME = SDLK_HOME,
    KN_I = SDLK_i,
    KN_INSERT = SDLK_INSERT,
    KN_J = SDLK_j,
    KN_K = SDLK_k,
    KN_KEYPAD_ASTERISK = SDLK_KP_MULTIPLY,
    KN_KEYPAD_MINUS = SDLK_KP_MINUS,
    KN_KEYPAD_PLUS = SDLK_KP_PLUS,
    KN_KEYPAD_SLASH = SDLK_KP_DIVIDE,
    KN_L = SDLK_l,
    KN_LALT = SDLK_LALT,
    KN_LBRACKET = SDLK_LEFTBRACKET,
    KN_LCTRL = SDLK_LCTRL,
    KN_LEFT = SDLK_LEFT,
    KN_LSHIFT = SDLK_LSHIFT,
    KN_M = SDLK_m,
    KN_MINUS = SDLK_MINUS,
    KN_N = SDLK_n,
    KN_NUMLOCK = SDLK_NUMLOCK,
    KN_O = SDLK_o,
    KN_P = SDLK_p,
    KN_PAUSE = SDLK_PAUSE,
    KN_PERIOD = SDLK_PERIOD,
    KN_PGDN = SDLK_PAGEDOWN,
    KN_PGUP = SDLK_PAGEUP,
    KN_PRNTSCRN = SDLK_PRINT,
    KN_Q = SDLK_q,
    KN_R = SDLK_r,
    KN_RALT = SDLK_RALT,
    KN_RBRACKET = SDLK_RIGHTBRACKET,
    KN_RCTRL = SDLK_RCTRL,
    KN_RETURN = SDLK_RETURN,
    KN_RIGHT = SDLK_RIGHT,
    KN_RSHIFT = SDLK_RSHIFT,
    KN_S = SDLK_s,
    KN_SCROLLLOCK = SDLK_SCROLLOCK,
    KN_SEMICOLON = SDLK_SEMICOLON,
    KN_SLASH = SDLK_SLASH,
    KN_SPACE = SDLK_SPACE,
    KN_SQUOTE = SDLK_QUOTE,
    KN_T = SDLK_t,
    KN_TAB = SDLK_TAB,
    KN_U = SDLK_u,
    KN_UP = SDLK_UP,
    KN_V = SDLK_v,
    KN_W = SDLK_w,
    KN_X = SDLK_x,
    KN_Y = SDLK_y,
    KN_Z = SDLK_z,

    KN_KEYPAD_RETURN = KN_RETURN,
    KN_UPLEFT = KN_HOME,
    KN_UPRIGHT = KN_PGUP,
    KN_DOWNLEFT = KN_END,
    KN_DOWNRIGHT = KN_PGDN,

    KN_LMOUSE = 0x01,
    KN_RMOUSE = 0x02,
    KN_MMOUSE = 0x03,
    KN_MOUSEWHEEL_UP = 0x04,
    KN_MOUSEWHEEL_DOWN = 0x05,

    KN_SHIFT_BIT = 0x200,
    KN_CTRL_BIT = 0x400,
    KN_ALT_BIT = 0x800,
    KN_RLSE_BIT = 0x1000,
    KN_VK_BIT = 0x4000,
    KN_BUTTON = 0x8000,
} KeyNumType;

#elif defined(SDL2_BUILD)

#include <SDL_scancode.h>

typedef enum KeyNumType : unsigned short
{
    KN_NONE = SDL_SCANCODE_UNKNOWN,

    KN_0 = SDL_SCANCODE_0,
    KN_1 = SDL_SCANCODE_1,
    KN_2 = SDL_SCANCODE_2,
    KN_3 = SDL_SCANCODE_3,
    KN_4 = SDL_SCANCODE_4,
    KN_5 = SDL_SCANCODE_5,
    KN_6 = SDL_SCANCODE_6,
    KN_7 = SDL_SCANCODE_7,
    KN_8 = SDL_SCANCODE_8,
    KN_9 = SDL_SCANCODE_9,
    KN_A = SDL_SCANCODE_A,
    KN_B = SDL_SCANCODE_B,
    KN_BACKSLASH = SDL_SCANCODE_BACKSLASH,
    KN_BACKSPACE = SDL_SCANCODE_BACKSPACE,
    KN_C = SDL_SCANCODE_C,
    KN_CAPSLOCK = SDL_SCANCODE_CAPSLOCK,
    KN_CENTER = SDL_SCANCODE_CLEAR,
    KN_COMMA = SDL_SCANCODE_COMMA,
    KN_D = SDL_SCANCODE_D,
    KN_DELETE = SDL_SCANCODE_DELETE,
    KN_DOWN = SDL_SCANCODE_DOWN,
    KN_E = SDL_SCANCODE_E,
    KN_END = SDL_SCANCODE_END,
    KN_EQUAL = SDL_SCANCODE_EQUALS,
    KN_ESC = SDL_SCANCODE_ESCAPE,
    KN_E_DELETE = SDL_SCANCODE_KP_PERIOD,
    KN_E_DOWN = SDL_SCANCODE_KP_2,
    KN_E_END = SDL_SCANCODE_KP_1,
    KN_E_HOME = SDL_SCANCODE_KP_7,
    KN_E_INSERT = SDL_SCANCODE_KP_0,
    KN_E_LEFT = SDL_SCANCODE_KP_4,
    KN_E_PGDN = SDL_SCANCODE_KP_3,
    KN_E_PGUP = SDL_SCANCODE_KP_9,
    KN_E_RIGHT = SDL_SCANCODE_KP_6,
    KN_E_UP = SDL_SCANCODE_KP_8,
    KN_SELECT = SDL_SCANCODE_SELECT,
    KN_F = SDL_SCANCODE_F,
    KN_F1 = SDL_SCANCODE_F1,
    KN_F10 = SDL_SCANCODE_F10,
    KN_F11 = SDL_SCANCODE_F11,
    KN_F12 = SDL_SCANCODE_F12,
    KN_F2 = SDL_SCANCODE_F2,
    KN_F3 = SDL_SCANCODE_F3,
    KN_F4 = SDL_SCANCODE_F4,
    KN_F5 = SDL_SCANCODE_F5,
    KN_F6 = SDL_SCANCODE_F6,
    KN_F7 = SDL_SCANCODE_F7,
    KN_F8 = SDL_SCANCODE_F8,
    KN_F9 = SDL_SCANCODE_F9,
    KN_G = SDL_SCANCODE_G,
    KN_GRAVE = SDL_SCANCODE_GRAVE,
    KN_H = SDL_SCANCODE_H,
    KN_HOME = SDL_SCANCODE_HOME,
    KN_I = SDL_SCANCODE_I,
    KN_INSERT = SDL_SCANCODE_INSERT,
    KN_J = SDL_SCANCODE_J,
    KN_K = SDL_SCANCODE_K,
    KN_KEYPAD_ASTERISK = SDL_SCANCODE_KP_MULTIPLY,
    KN_KEYPAD_MINUS = SDL_SCANCODE_KP_MINUS,
    KN_KEYPAD_PLUS = SDL_SCANCODE_KP_PLUS,
    KN_KEYPAD_SLASH = SDL_SCANCODE_KP_DIVIDE,
    KN_L = SDL_SCANCODE_L,
    KN_LALT = SDL_SCANCODE_LALT,
    KN_LBRACKET = SDL_SCANCODE_LEFTBRACKET,
    KN_LCTRL = SDL_SCANCODE_LCTRL,
    KN_LEFT = SDL_SCANCODE_LEFT,
    KN_LSHIFT = SDL_SCANCODE_LSHIFT,
    KN_M = SDL_SCANCODE_M,
    KN_MINUS = SDL_SCANCODE_MINUS,
    KN_N = SDL_SCANCODE_N,
    KN_NUMLOCK = SDL_SCANCODE_NUMLOCKCLEAR,
    KN_O = SDL_SCANCODE_O,
    KN_P = SDL_SCANCODE_P,
    KN_PAUSE = SDL_SCANCODE_PAUSE,
    KN_PERIOD = SDL_SCANCODE_PERIOD,
    KN_PGDN = SDL_SCANCODE_PAGEDOWN,
    KN_PGUP = SDL_SCANCODE_PAGEUP,
    KN_PRNTSCRN = SDL_SCANCODE_PRINTSCREEN,
    KN_Q = SDL_SCANCODE_Q,
    KN_R = SDL_SCANCODE_R,
    KN_RALT = SDL_SCANCODE_RALT,
    KN_RBRACKET = SDL_SCANCODE_RIGHTBRACKET,
    KN_RCTRL = SDL_SCANCODE_RCTRL,
    KN_RETURN = SDL_SCANCODE_RETURN,
    KN_RIGHT = SDL_SCANCODE_RIGHT,
    KN_RSHIFT = SDL_SCANCODE_RSHIFT,
    KN_S = SDL_SCANCODE_S,
    KN_SCROLLLOCK = SDL_SCANCODE_SCROLLLOCK,
    KN_SEMICOLON = SDL_SCANCODE_SEMICOLON,
    KN_SLASH = SDL_SCANCODE_SLASH,
    KN_SPACE = SDL_SCANCODE_SPACE,
    KN_SQUOTE = SDL_SCANCODE_APOSTROPHE,
    KN_T = SDL_SCANCODE_T,
    KN_TAB = SDL_SCANCODE_TAB,
    KN_U = SDL_SCANCODE_U,
    KN_UP = SDL_SCANCODE_UP,
    KN_V = SDL_SCANCODE_V,
    KN_W = SDL_SCANCODE_W,
    KN_X = SDL_SCANCODE_X,
    KN_Y = SDL_SCANCODE_Y,
    KN_Z = SDL_SCANCODE_Z,

    KN_KEYPAD_RETURN = KN_RETURN,
    KN_UPLEFT = KN_HOME,
    KN_UPRIGHT = KN_PGUP,
    KN_DOWNLEFT = KN_END,
    KN_DOWNRIGHT = KN_PGDN,

    KN_LMOUSE = 0x01,
    KN_RMOUSE = 0x02,
    KN_MMOUSE = 0x03,
    KN_MOUSEWHEEL_UP = 0xFE,
    KN_MOUSEWHEEL_DOWN = 0xFD,

    KN_SHIFT_BIT = 0x200,
    KN_CTRL_BIT = 0x400,
    KN_ALT_BIT = 0x800,
    KN_RLSE_BIT = 0x1000,
    KN_VK_BIT = 0x4000,
    KN_BUTTON = 0x8000,
} KeyNumType;

#else

// these should match VK_* values from winuser.h

typedef enum KeyNumType : unsigned short
{
    KN_NONE = 0x00,

    KN_0 = 0x30,
    KN_1 = 0x31,
    KN_2 = 0x32,
    KN_3 = 0x33,
    KN_4 = 0x34,
    KN_5 = 0x35,
    KN_6 = 0x36,
    KN_7 = 0x37,
    KN_8 = 0x38,
    KN_9 = 0x39,
    KN_A = 0x41,
    KN_B = 0x42,
    KN_BACKSLASH = 0xDC, // '\'
    KN_BACKSPACE = 0x08,
    KN_C = 0x43,
    KN_CAPSLOCK = 0x14,
    KN_CENTER = 0x0C,
    KN_COMMA = 0xBC, // ,
    KN_D = 0x44,
    KN_DELETE = 0x2E,
    KN_DOWN = 0x28,
    KN_E = 0x45,
    KN_END = 0x23,
    KN_EQUAL = 0xBB, // =
    KN_ESC = 0x1B,
    KN_E_DELETE = 0x6C,
    KN_E_DOWN = 0x62,
    KN_E_END = 0x61,
    KN_E_HOME = 0x67,
    KN_E_INSERT = 0x60,
    KN_E_LEFT = 0x64,
    KN_E_PGDN = 0x63,
    KN_E_PGUP = 0x69,
    KN_E_RIGHT = 0x66,
    KN_E_UP = 0x68,
    KN_SELECT = 0x29,
    KN_F = 0x46,
    KN_F1 = 0x70,
    KN_F2 = 0x70,
    KN_F3 = 0x72,
    KN_F4 = 0x73,
    KN_F5 = 0x74,
    KN_F6 = 0x75,
    KN_F7 = 0x76,
    KN_F8 = 0x77,
    KN_F9 = 0x78,
    KN_F10 = 0x79,
    KN_F11 = 0x7A,
    KN_F12 = 0x7B,
    KN_G = 0x47,
    KN_GRAVE = 0xC0, // `
    KN_H = 0x48,
    KN_HOME = 0x24,
    KN_I = 0x49,
    KN_INSERT = 0x2D,
    KN_J = 0x4A,
    KN_K = 0x4B,
    KN_KEYPAD_ASTERISK = 0x6A,
    KN_KEYPAD_MINUS = 0x6D,
    KN_KEYPAD_PLUS = 0x6B,
    KN_KEYPAD_SLASH = 0x6F,
    KN_L = 0x4C,
    KN_LALT = 0x12,
    KN_LBRACKET = 0xDB, // [
    KN_LCTRL = 0x11,
    KN_LEFT = 0x25,
    KN_LSHIFT = 0x10,
    KN_M = 0x4D,
    KN_MINUS = 0xBD, // -
    KN_N = 0x4E,
    KN_NUMLOCK = 0x90,
    KN_O = 0x4F,
    KN_P = 0x50,
    KN_PAUSE = 0x13,
    KN_PERIOD = 0xBE, // .
    KN_PGDN = 0x22,
    KN_PGUP = 0x21,
    KN_PRNTSCRN = 0x2A,
    KN_Q = 0x51,
    KN_R = 0x52,
    KN_RALT = 0x12,
    KN_RBRACKET = 0xDD, // ]
    KN_RCTRL = 0x11,
    KN_RETURN = 0x0D,
    KN_RIGHT = 0x27,
    KN_RSHIFT = 0x10,
    KN_S = 0x53,
    KN_SCROLLLOCK = 0x91,
    KN_SEMICOLON = 0xBA, //	;
    KN_SLASH = 0x2F,     // /
    KN_SPACE = 0x20,
    KN_SQUOTE = 0xDE, // '
    KN_T = 0x54,
    KN_TAB = 0x09,
    KN_U = 0x55,
    KN_UP = 0x26,
    KN_V = 0x56,
    KN_W = 0x57,
    KN_X = 0x58,
    KN_Y = 0x59,
    KN_Z = 0x5A,

    KN_KEYPAD_RETURN = KN_RETURN,
    KN_UPLEFT = KN_HOME,
    KN_UPRIGHT = KN_PGUP,
    KN_DOWNLEFT = KN_END,
    KN_DOWNRIGHT = KN_PGDN,

    KN_LMOUSE = 0x01,
    KN_RMOUSE = 0x02,
    KN_MMOUSE = 0x04,
    KN_MOUSEWHEEL_UP = 0xFE,
    KN_MOUSEWHEEL_DOWN = 0xFD,

    KN_SHIFT_BIT = 0x200,
    KN_CTRL_BIT = 0x400,
    KN_ALT_BIT = 0x800,
    KN_RLSE_BIT = 0x1000,
    KN_VK_BIT = 0x4000,
    KN_BUTTON = 0x8000,
} KeyNumType;

#endif

//
// scancode values from both SDL1 & SDL2 may have values up to 511
//
#define KN_SCANCODE_MASK 0x1ff

typedef enum ScrollDirType : unsigned char
{
    SDIR_N = 0,
    SDIR_NE = 1 << 5,
    SDIR_E = 2 << 5,
    SDIR_SE = 3 << 5,
    SDIR_S = 4 << 5,
    SDIR_SW = 5 << 5,
    SDIR_W = 6 << 5,
    SDIR_NW = 7 << 5,
    SDIR_NONE = 100
} ScrollDirType;

class WWKeyboardClass
{
public:
    /* Define the base constructor and destructors for the class			*/
    WWKeyboardClass();

    /* Define the functions which work with the Keyboard Class				*/
    KeyNumType Check(void) const;
    KeyNumType Get(void);
    bool Put(unsigned short key);
    void Clear(void);
    KeyASCIIType To_ASCII(unsigned short num);
    bool Down(unsigned short key);

#ifdef SDL2_BUILD
    bool Is_Gamepad_Active();
    void Open_Controller();
    void Close_Controller();
    bool Is_Analog_Scroll_Active();
    unsigned char Get_Scroll_Direction();
#elif defined(SDL1_BUILD)
    bool Is_Gamepad_Active()
    {
        return false;
    }
#endif

#if defined(_WIN32) && !defined(SDL_BUILD)
    /* Define the main hook for the message processing loop.					*/
    bool Message_Handler(HWND hwnd, UINT message, UINT wParam, LONG lParam);
#endif

    /* Define the public access variables which are used with the			*/
    /*   Keyboard Class.																	*/
    int MouseQX;
    int MouseQY;

private:
    /*
    **	This is a keyboard state array that is used to aid in translating
    **	KN_ keys into KA_ keys.
    */
#if defined(_WIN32)
    unsigned char KeyState[256];
#endif

    /*
    **	This is the circular keyboard holding buffer. It holds the VK key and
    **	the current shift state at the time the key was added to the queue.
    */
    unsigned short Buffer[256]; // buffer which holds actual keypresses

    unsigned short Buff_Get(void);
    unsigned short Fetch_Element(void);
    unsigned short Peek_Element(void) const;
    bool Put_Element(unsigned short val);
    bool Is_Buffer_Full(void) const;
    bool Is_Buffer_Empty(void) const;
    static bool Is_Mouse_Key(unsigned short key);
    void Fill_Buffer_From_System(void);
    bool Put_Key_Message(unsigned short vk_key, bool release = false);
    bool Put_Mouse_Message(unsigned short vk_key, int x, int y, bool release = false);
    int Available_Buffer_Room(void) const;

    /*
    **	These are the tracking pointers to maintain the
    **	circular keyboard list.
    */
    int Head;
    int Tail;

    /*
    ** Large bit array to hold which keys are held down
    */
    uint8_t DownState[0x2000]; // (UINT16_MAX / 8) + 1
    int DownSkip;

#ifdef SDL2_BUILD
    void Handle_Controller_Axis_Event(const SDL_ControllerAxisEvent& motion);
    void Handle_Controller_Button_Event(const SDL_ControllerButtonEvent& button);
    void Process_Controller_Axis_Motion();

    // used to convert user-friendly pointer speed values into more useable ones
    static constexpr float CONTROLLER_SPEED_MOD = 2000000.0f;
    // bigger value correndsponds to faster pointer movement speed with bigger stick axis values
    static constexpr float CONTROLLER_AXIS_SPEEDUP = 1.03f;
    // speedup value while the trigger is pressed
    static constexpr int CONTROLLER_TRIGGER_SPEEDUP = 2;

    enum
    {
        CONTROLLER_L_DEADZONE = 4000,
        CONTROLLER_R_DEADZONE = 6000,
        CONTROLLER_TRIGGER_R_DEADZONE = 3000
    };

    SDL_GameController* GameController = nullptr;
    int16_t ControllerLeftXAxis = 0;
    int16_t ControllerLeftYAxis = 0;
    int16_t ControllerRightXAxis = 0;
    int16_t ControllerRightYAxis = 0;
    uint32_t LastControllerTime = 0;
    float ControllerSpeedBoost = 1;
    bool AnalogScrollActive = false;
    ScrollDirType ScrollDirection = SDIR_NONE;
#endif
};

#endif
