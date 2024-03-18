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

typedef enum
{
    WWKEY_SHIFT_BIT = 0x100,
    WWKEY_CTRL_BIT = 0x200,
    WWKEY_ALT_BIT = 0x400,
    WWKEY_RLS_BIT = 0x800,
    WWKEY_VK_BIT = 0x1000,
    WWKEY_DBL_BIT = 0x2000,
    WWKEY_BTN_BIT = 0x8000,
} WWKey_Type;

#ifdef SDL1_BUILD
#include <SDL_keysym.h>
#define VK_NONE            SDLK_UNKNOWN
#define VK_LBUTTON         0x01
#define VK_RBUTTON         0x02
#define VK_MBUTTON         0x03
#define VK_MOUSEWHEEL_UP   0x04
#define VK_MOUSEWHEEL_DOWN 0x05
#define VK_BACK            SDLK_BACKSPACE
#define VK_TAB             SDLK_TAB
#define VK_CLEAR           SDLK_CLEAR
#define VK_RETURN          SDLK_RETURN
#define VK_SHIFT           SDLK_LSHIFT
#define VK_CONTROL         SDLK_LCTRL
#define VK_MENU            SDLK_LALT
#define VK_PAUSE           SDLK_PAUSE
#define VK_CAPITAL         SDLK_CAPSLOCK
#define VK_ESCAPE          SDLK_ESCAPE
#define VK_SPACE           SDLK_SPACE
#define VK_PRIOR           SDLK_PAGEUP
#define VK_NEXT            SDLK_PAGEDOWN
#define VK_END             SDLK_END
#define VK_HOME            SDLK_HOME
#define VK_LEFT            SDLK_LEFT
#define VK_UP              SDLK_UP
#define VK_RIGHT           SDLK_RIGHT
#define VK_DOWN            SDLK_DOWN
#define VK_SELECT          SDLK_UNKNOWN //SELECT
#define VK_PRINT           SDLK_PRINT
#define VK_INSERT          SDLK_INSERT
#define VK_DELETE          SDLK_DELETE
#define VK_NUMPAD0         SDLK_KP0
#define VK_NUMPAD1         SDLK_KP1
#define VK_NUMPAD2         SDLK_KP2
#define VK_NUMPAD3         SDLK_KP3
#define VK_NUMPAD4         SDLK_KP4
#define VK_NUMPAD5         SDLK_KP5
#define VK_NUMPAD6         SDLK_KP6
#define VK_NUMPAD7         SDLK_KP7
#define VK_NUMPAD8         SDLK_KP8
#define VK_NUMPAD9         SDLK_KP9
#define VK_MULTIPLY        SDLK_KP_MULTIPLY
#define VK_ADD             SDLK_KP_PLUS
#define VK_SUBTRACT        SDLK_KP_MINUS
#define VK_DIVIDE          SDLK_KP_DIVIDE
#define VK_0               SDLK_0
#define VK_1               SDLK_1
#define VK_2               SDLK_2
#define VK_3               SDLK_3
#define VK_4               SDLK_4
#define VK_5               SDLK_5
#define VK_6               SDLK_6
#define VK_7               SDLK_7
#define VK_8               SDLK_8
#define VK_9               SDLK_9
#define VK_A               SDLK_a
#define VK_B               SDLK_b
#define VK_C               SDLK_c
#define VK_D               SDLK_d
#define VK_E               SDLK_e
#define VK_F               SDLK_f
#define VK_G               SDLK_g
#define VK_H               SDLK_h
#define VK_I               SDLK_i
#define VK_J               SDLK_j
#define VK_K               SDLK_k
#define VK_L               SDLK_l
#define VK_M               SDLK_m
#define VK_N               SDLK_n
#define VK_O               SDLK_o
#define VK_P               SDLK_p
#define VK_Q               SDLK_q
#define VK_R               SDLK_r
#define VK_S               SDLK_s
#define VK_T               SDLK_t
#define VK_U               SDLK_u
#define VK_V               SDLK_v
#define VK_W               SDLK_w
#define VK_X               SDLK_x
#define VK_Y               SDLK_y
#define VK_Z               SDLK_z
#define VK_F1              SDLK_F1
#define VK_F2              SDLK_F2
#define VK_F3              SDLK_F3
#define VK_F4              SDLK_F4
#define VK_F5              SDLK_F5
#define VK_F6              SDLK_F6
#define VK_F7              SDLK_F7
#define VK_F8              SDLK_F8
#define VK_F9              SDLK_F9
#define VK_F10             SDLK_F10
#define VK_F11             SDLK_F11
#define VK_F12             SDLK_F12
#define VK_NUMLOCK         SDLK_NUMLOCK
#define VK_SCROLL          SDLK_SCROLLOCK
#define VK_NONE_BA         SDLK_SEMICOLON
#define VK_NONE_BB         SDLK_EQUALS
#define VK_NONE_BC         SDLK_COMMA
#define VK_NONE_BD         SDLK_MINUS
#define VK_NONE_BE         SDLK_PERIOD
#define VK_NONE_BF         SDLK_SLASH
#define VK_NONE_C0         SDLK_UNKNOWN //GRAVE
#define VK_NONE_DB         SDLK_LEFTBRACKET
#define VK_NONE_DC         SDLK_BACKSLASH
#define VK_NONE_DD         SDLK_RIGHTBRACKET
#define VK_NONE_DE         SDLK_UNKNOWN //APOSTROPHE

#elif defined(SDL2_BUILD)

#include <SDL_scancode.h>

#define VK_NONE SDL_SCANCODE_UNKNOWN

#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_MBUTTON 0x03

#define VK_BACK            SDL_SCANCODE_BACKSPACE
#define VK_TAB             SDL_SCANCODE_TAB
#define VK_CLEAR           SDL_SCANCODE_CLEAR
#define VK_RETURN          SDL_SCANCODE_RETURN
#define VK_SHIFT           SDL_SCANCODE_LSHIFT
#define VK_CONTROL         SDL_SCANCODE_LCTRL
#define VK_MENU            SDL_SCANCODE_LALT
#define VK_PAUSE           SDL_SCANCODE_PAUSE
#define VK_CAPITAL         SDL_SCANCODE_CAPSLOCK
#define VK_ESCAPE          SDL_SCANCODE_ESCAPE
#define VK_SPACE           SDL_SCANCODE_SPACE
#define VK_PRIOR           SDL_SCANCODE_PAGEUP
#define VK_NEXT            SDL_SCANCODE_PAGEDOWN
#define VK_END             SDL_SCANCODE_END
#define VK_HOME            SDL_SCANCODE_HOME
#define VK_LEFT            SDL_SCANCODE_LEFT
#define VK_UP              SDL_SCANCODE_UP
#define VK_RIGHT           SDL_SCANCODE_RIGHT
#define VK_DOWN            SDL_SCANCODE_DOWN
#define VK_SELECT          SDL_SCANCODE_SELECT
#define VK_PRINT           SDL_SCANCODE_PRINTSCREEN
#define VK_INSERT          SDL_SCANCODE_INSERT
#define VK_DELETE          SDL_SCANCODE_DELETE
#define VK_NUMPAD0         SDL_SCANCODE_KP_0
#define VK_NUMPAD1         SDL_SCANCODE_KP_1
#define VK_NUMPAD2         SDL_SCANCODE_KP_2
#define VK_NUMPAD3         SDL_SCANCODE_KP_3
#define VK_NUMPAD4         SDL_SCANCODE_KP_4
#define VK_NUMPAD5         SDL_SCANCODE_KP_5
#define VK_NUMPAD6         SDL_SCANCODE_KP_6
#define VK_NUMPAD7         SDL_SCANCODE_KP_7
#define VK_NUMPAD8         SDL_SCANCODE_KP_8
#define VK_NUMPAD9         SDL_SCANCODE_KP_9
#define VK_MULTIPLY        SDL_SCANCODE_KP_MULTIPLY
#define VK_ADD             SDL_SCANCODE_KP_PLUS
#define VK_SUBTRACT        SDL_SCANCODE_KP_MINUS
#define VK_DIVIDE          SDL_SCANCODE_KP_DIVIDE
#define VK_0               SDL_SCANCODE_0
#define VK_1               SDL_SCANCODE_1
#define VK_2               SDL_SCANCODE_2
#define VK_3               SDL_SCANCODE_3
#define VK_4               SDL_SCANCODE_4
#define VK_5               SDL_SCANCODE_5
#define VK_6               SDL_SCANCODE_6
#define VK_7               SDL_SCANCODE_7
#define VK_8               SDL_SCANCODE_8
#define VK_9               SDL_SCANCODE_9
#define VK_A               SDL_SCANCODE_A
#define VK_B               SDL_SCANCODE_B
#define VK_C               SDL_SCANCODE_C
#define VK_D               SDL_SCANCODE_D
#define VK_E               SDL_SCANCODE_E
#define VK_F               SDL_SCANCODE_F
#define VK_G               SDL_SCANCODE_G
#define VK_H               SDL_SCANCODE_H
#define VK_I               SDL_SCANCODE_I
#define VK_J               SDL_SCANCODE_J
#define VK_K               SDL_SCANCODE_K
#define VK_L               SDL_SCANCODE_L
#define VK_M               SDL_SCANCODE_M
#define VK_N               SDL_SCANCODE_N
#define VK_O               SDL_SCANCODE_O
#define VK_P               SDL_SCANCODE_P
#define VK_Q               SDL_SCANCODE_Q
#define VK_R               SDL_SCANCODE_R
#define VK_S               SDL_SCANCODE_S
#define VK_T               SDL_SCANCODE_T
#define VK_U               SDL_SCANCODE_U
#define VK_V               SDL_SCANCODE_V
#define VK_W               SDL_SCANCODE_W
#define VK_X               SDL_SCANCODE_X
#define VK_Y               SDL_SCANCODE_Y
#define VK_Z               SDL_SCANCODE_Z
#define VK_F1              SDL_SCANCODE_F1
#define VK_F2              SDL_SCANCODE_F2
#define VK_F3              SDL_SCANCODE_F3
#define VK_F4              SDL_SCANCODE_F4
#define VK_F5              SDL_SCANCODE_F5
#define VK_F6              SDL_SCANCODE_F6
#define VK_F7              SDL_SCANCODE_F7
#define VK_F8              SDL_SCANCODE_F8
#define VK_F9              SDL_SCANCODE_F9
#define VK_F10             SDL_SCANCODE_F10
#define VK_F11             SDL_SCANCODE_F11
#define VK_F12             SDL_SCANCODE_F12
#define VK_NUMLOCK         SDL_SCANCODE_NUMLOCKCLEAR
#define VK_SCROLL          SDL_SCANCODE_SCROLLLOCK
#define VK_NONE_BA         SDL_SCANCODE_SEMICOLON
#define VK_NONE_BB         SDL_SCANCODE_EQUALS
#define VK_NONE_BC         SDL_SCANCODE_COMMA
#define VK_NONE_BD         SDL_SCANCODE_MINUS
#define VK_NONE_BE         SDL_SCANCODE_PERIOD
#define VK_NONE_BF         SDL_SCANCODE_SLASH
#define VK_NONE_C0         SDL_SCANCODE_GRAVE
#define VK_NONE_DB         SDL_SCANCODE_LEFTBRACKET
#define VK_NONE_DC         SDL_SCANCODE_BACKSLASH
#define VK_NONE_DD         SDL_SCANCODE_RIGHTBRACKET
#define VK_NONE_DE         SDL_SCANCODE_APOSTROPHE
#define VK_MOUSEWHEEL_DOWN 0xFD
#define VK_MOUSEWHEEL_UP   0xFE

#else

#define VK_NONE            0x00
#define VK_LBUTTON         0x01
#define VK_RBUTTON         0x02
#define VK_CANCEL          0x03
#define VK_MBUTTON         0x04
#define VK_NONE_05         0x05
#define VK_NONE_06         0x06
#define VK_NONE_07         0x07
#define VK_BACK            0x08
#define VK_TAB             0x09
#define VK_NONE_0A         0x0A
#define VK_NONE_0B         0x0B
#define VK_CLEAR           0x0C
#define VK_RETURN          0x0D
#define VK_NONE_0E         0x0E
#define VK_NONE_0F         0x0F
#define VK_SHIFT           0x10
#define VK_CONTROL         0x11
#define VK_MENU            0x12
#define VK_PAUSE           0x13
#define VK_CAPITAL         0x14
#define VK_NONE_15         0x15
#define VK_NONE_16         0x16
#define VK_NONE_17         0x17
#define VK_NONE_18         0x18
#define VK_NONE_19         0x19
#define VK_NONE_1A         0x1A
#define VK_ESCAPE          0x1B
#define VK_NONE_1C         0x1C
#define VK_NONE_1D         0x1D
#define VK_NONE_1E         0x1E
#define VK_NONE_1F         0x1F
#define VK_SPACE           0x20
#define VK_PRIOR           0x21
#define VK_NEXT            0x22
#define VK_END             0x23
#define VK_HOME            0x24
#define VK_LEFT            0x25
#define VK_UP              0x26
#define VK_RIGHT           0x27
#define VK_DOWN            0x28
#define VK_SELECT          0x29
#define VK_PRINT           0x2A
#define VK_EXECUTE         0x2B
#define VK_SNAPSHOT        0x2C
#define VK_INSERT          0x2D
#define VK_DELETE          0x2E
#define VK_HELP            0x2F
#define VK_0               0x30
#define VK_1               0x31
#define VK_2               0x32
#define VK_3               0x33
#define VK_4               0x34
#define VK_5               0x35
#define VK_6               0x36
#define VK_7               0x37
#define VK_8               0x38
#define VK_9               0x39
#define VK_NONE_3B         0x3B
#define VK_NONE_3C         0x3C
#define VK_NONE_3D         0x3D
#define VK_NONE_3E         0x3E
#define VK_NONE_3F         0x3F
#define VK_NONE_40         0x40
#define VK_A               0x41
#define VK_B               0x42
#define VK_C               0x43
#define VK_D               0x44
#define VK_E               0x45
#define VK_F               0x46
#define VK_G               0x47
#define VK_H               0x48
#define VK_I               0x49
#define VK_J               0x4A
#define VK_K               0x4B
#define VK_L               0x4C
#define VK_M               0x4D
#define VK_N               0x4E
#define VK_O               0x4F
#define VK_P               0x50
#define VK_Q               0x51
#define VK_R               0x52
#define VK_S               0x53
#define VK_T               0x54
#define VK_U               0x55
#define VK_V               0x56
#define VK_W               0x57
#define VK_X               0x58
#define VK_Y               0x59
#define VK_Z               0x5A
#define VK_NONE_5B         0x5B
#define VK_NONE_5C         0x5C
#define VK_NONE_5D         0x5D
#define VK_NONE_5E         0x5E
#define VK_NONE_5F         0x5F
#define VK_NUMPAD0         0x60
#define VK_NUMPAD1         0x61
#define VK_NUMPAD2         0x62
#define VK_NUMPAD3         0x63
#define VK_NUMPAD4         0x64
#define VK_NUMPAD5         0x65
#define VK_NUMPAD6         0x66
#define VK_NUMPAD7         0x67
#define VK_NUMPAD8         0x68
#define VK_NUMPAD9         0x69
#define VK_MULTIPLY        0x6A
#define VK_ADD             0x6B
#define VK_SEPARATOR       0x6C
#define VK_SUBTRACT        0x6D
#define VK_DECIMAL         0x6E
#define VK_DIVIDE          0x6F
#define VK_F1              0x70
#define VK_F2              0x71
#define VK_F3              0x72
#define VK_F4              0x73
#define VK_F5              0x74
#define VK_F6              0x75
#define VK_F7              0x76
#define VK_F8              0x77
#define VK_F9              0x78
#define VK_F10             0x79
#define VK_F11             0x7A
#define VK_F12             0x7B
#define VK_F13             0x7C
#define VK_F14             0x7D
#define VK_F15             0x7E
#define VK_F16             0x7F
#define VK_F17             0x80
#define VK_F18             0x81
#define VK_F19             0x82
#define VK_F20             0x83
#define VK_F21             0x84
#define VK_F22             0x85
#define VK_F23             0x86
#define VK_F24             0x87
#define VK_NONE_88         0x88
#define VK_NONE_89         0x89
#define VK_NONE_8A         0x8A
#define VK_NONE_8B         0x8B
#define VK_NONE_8C         0x8C
#define VK_NONE_8D         0x8D
#define VK_NONE_8E         0x8E
#define VK_NONE_8F         0x8F
#define VK_NUMLOCK         0x90
#define VK_SCROLL          0x91
#define VK_NONE_92         0x92
#define VK_NONE_93         0x93
#define VK_NONE_94         0x94
#define VK_NONE_95         0x95
#define VK_NONE_96         0x96
#define VK_NONE_97         0x97
#define VK_NONE_98         0x98
#define VK_NONE_99         0x99
#define VK_NONE_9A         0x9A
#define VK_NONE_9B         0x9B
#define VK_NONE_9C         0x9C
#define VK_NONE_9D         0x9D
#define VK_NONE_9E         0x9E
#define VK_NONE_9F         0x9F
#define VK_NONE_A0         0xA0
#define VK_NONE_A1         0xA1
#define VK_NONE_A2         0xA2
#define VK_NONE_A3         0xA3
#define VK_NONE_A4         0xA4
#define VK_NONE_A5         0xA5
#define VK_NONE_A6         0xA6
#define VK_NONE_A7         0xA7
#define VK_NONE_A8         0xA8
#define VK_NONE_A9         0xA9
#define VK_NONE_AA         0xAA
#define VK_NONE_AB         0xAB
#define VK_NONE_AC         0xAC
#define VK_NONE_AD         0xAD
#define VK_NONE_AE         0xAE
#define VK_NONE_AF         0xAF
#define VK_NONE_B0         0xB0
#define VK_NONE_B1         0xB1
#define VK_NONE_B2         0xB2
#define VK_NONE_B3         0xB3
#define VK_NONE_B4         0xB4
#define VK_NONE_B5         0xB5
#define VK_NONE_B6         0xB6
#define VK_NONE_B7         0xB7
#define VK_NONE_B8         0xB8
#define VK_NONE_B9         0xB9
#define VK_NONE_BA         0xBA //	;
#define VK_NONE_BB         0xBB // =
#define VK_NONE_BC         0xBC // ,
#define VK_NONE_BD         0xBD // -
#define VK_NONE_BE         0xBE // .
#define VK_NONE_BF         0xBF // /
#define VK_NONE_C0         0xC0 // `
#define VK_NONE_C1         0xC1
#define VK_NONE_C2         0xC2
#define VK_NONE_C3         0xC3
#define VK_NONE_C4         0xC4
#define VK_NONE_C5         0xC5
#define VK_NONE_C6         0xC6
#define VK_NONE_C7         0xC7
#define VK_NONE_C8         0xC8
#define VK_NONE_C9         0xC9
#define VK_NONE_CA         0xCA
#define VK_NONE_CB         0xCB
#define VK_NONE_CC         0xCC
#define VK_NONE_CD         0xCD
#define VK_NONE_CE         0xCE
#define VK_NONE_CF         0xCF
#define VK_NONE_D0         0xD0
#define VK_NONE_D1         0xD1
#define VK_NONE_D2         0xD2
#define VK_NONE_D3         0xD3
#define VK_NONE_D4         0xD4
#define VK_NONE_D5         0xD5
#define VK_NONE_D6         0xD6
#define VK_NONE_D7         0xD7
#define VK_NONE_D8         0xD8
#define VK_NONE_D9         0xD9
#define VK_NONE_DA         0xDA
#define VK_NONE_DB         0xDB // [
#define VK_NONE_DC         0xDC // '\'
#define VK_NONE_DD         0xDD // ]
#define VK_NONE_DE         0xDE // '
#define VK_NONE_DF         0xDF
#define VK_NONE_E0         0xE0
#define VK_NONE_E1         0xE1
#define VK_NONE_E2         0xE2
#define VK_NONE_E3         0xE3
#define VK_NONE_E4         0xE4
#define VK_NONE_E5         0xE5
#define VK_NONE_E6         0xE6
#define VK_NONE_E7         0xE7
#define VK_NONE_E8         0xE8
#define VK_NONE_E9         0xE9
#define VK_NONE_EA         0xEA
#define VK_NONE_EB         0xEB
#define VK_NONE_EC         0xEC
#define VK_NONE_ED         0xED
#define VK_NONE_EE         0xEE
#define VK_NONE_EF         0xEF
#define VK_NONE_F0         0xF0
#define VK_NONE_F1         0xF1
#define VK_NONE_F2         0xF2
#define VK_NONE_F3         0xF3
#define VK_NONE_F4         0xF4
#define VK_NONE_F5         0xF5
#define VK_NONE_F6         0xF6
#define VK_NONE_F7         0xF7
#define VK_NONE_F8         0xF8
#define VK_NONE_F9         0xF9
#define VK_NONE_FA         0xFA
#define VK_NONE_FB         0xFB
#define VK_NONE_FC         0xFC
#define VK_MOUSEWHEEL_DOWN 0xFD
#define VK_MOUSEWHEEL_UP   0xFE
#define VK_NONE_FF         0xFF

#endif // !SDL_BUILD

#define VK_UPLEFT    VK_HOME
#define VK_UPRIGHT   VK_PRIOR
#define VK_DOWNLEFT  VK_END
#define VK_DOWNRIGHT VK_NEXT
#define VK_ALT       VK_MENU

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

    KA_SPACE = 32,  /*   */
    KA_EXCLAMATION, /* ! */
    KA_DQUOTE,      /* " */
    KA_POUND,       /* # */
    KA_DOLLAR,      /* $ */
    KA_PERCENT,     /* % */
    KA_AMPER,       /* & */
    KA_SQUOTE,      /* ' */
    KA_LPAREN,      /* ( */
    KA_RPAREN,      /* ) */
    KA_ASTERISK,    /* * */
    KA_PLUS,        /* + */
    KA_COMMA,       /* , */
    KA_MINUS,       /* - */
    KA_PERIOD,      /* . */
    KA_SLASH,       /* / */

    KA_0,
    KA_1,
    KA_2,
    KA_3,
    KA_4,
    KA_5,
    KA_6,
    KA_7,
    KA_8,
    KA_9,
    KA_COLON,        /* : */
    KA_SEMICOLON,    /* ; */
    KA_LESS_THAN,    /* < */
    KA_EQUAL,        /* = */
    KA_GREATER_THAN, /* > */
    KA_QUESTION,     /* ? */

    KA_AT, /* @ */
    KA_A,  /* A */
    KA_B,  /* B */
    KA_C,  /* C */
    KA_D,  /* D */
    KA_E,  /* E */
    KA_F,  /* F */
    KA_G,  /* G */
    KA_H,  /* H */
    KA_I,  /* I */
    KA_J,  /* J */
    KA_K,  /* K */
    KA_L,  /* L */
    KA_M,  /* M */
    KA_N,  /* N */
    KA_O,  /* O */

    KA_P,         /* P */
    KA_Q,         /* Q */
    KA_R,         /* R */
    KA_S,         /* S */
    KA_T,         /* T */
    KA_U,         /* U */
    KA_V,         /* V */
    KA_W,         /* W */
    KA_X,         /* X */
    KA_Y,         /* Y */
    KA_Z,         /* Z */
    KA_LBRACKET,  /* [ */
    KA_BACKSLASH, /* \ */
    KA_RBRACKET,  /* ] */
    KA_CARROT,    /* ^ */
    KA_UNDERLINE, /* _ */

    KA_GRAVE, /* ` */
    KA_a,     /* a */
    KA_b,     /* b */
    KA_c,     /* c */
    KA_d,     /* d */
    KA_e,     /* e */
    KA_f,     /* f */
    KA_g,     /* g */
    KA_h,     /* h */
    KA_i,     /* i */
    KA_j,     /* j */
    KA_k,     /* k */
    KA_l,     /* l */
    KA_m,     /* m */
    KA_n,     /* n */
    KA_o,     /* o */

    KA_p,      /* p */
    KA_q,      /* q */
    KA_r,      /* r */
    KA_s,      /* s */
    KA_t,      /* t */
    KA_u,      /* u */
    KA_v,      /* v */
    KA_w,      /* w */
    KA_x,      /* x */
    KA_y,      /* y */
    KA_z,      /* z */
    KA_LBRACE, /* { */
    KA_BAR,    /* | */
    KA_RBRACE, /* ] */
    KA_TILDA,  /* ~ */

#ifdef SDL_BUILD
    KA_ESC = 0x1B,
    KA_EXTEND = 0x1B,
    KA_RETURN = 0x0D,
    KA_BACKSPACE = 0x08,
    KA_TAB = 0x09,
    KA_DELETE = 0x7F,
#else
    KA_ESC = VK_ESCAPE,
    KA_EXTEND = VK_ESCAPE,
    KA_RETURN = VK_RETURN,
    KA_BACKSPACE = VK_BACK,
    KA_TAB = VK_TAB,
    KA_DELETE = VK_DELETE, /* <DELETE> */
#endif
    KA_INSERT = VK_INSERT, /* <INSERT> */
    KA_PGDN = VK_NEXT,     /* <PAGE DOWN> */
    KA_DOWNRIGHT = VK_NEXT,
    KA_DOWN = VK_DOWN, /* <DOWN ARROW> */
    KA_END = VK_END,   /* <END> */
    KA_DOWNLEFT = VK_END,
    KA_RIGHT = VK_RIGHT,    /* <RIGHT ARROW> */
    KA_KEYPAD5 = VK_SELECT, /* NUMERIC KEY PAD <5> */
    KA_LEFT = VK_LEFT,      /* <LEFT ARROW> */
    KA_PGUP = VK_PRIOR,     /* <PAGE UP> */
    KA_UPRIGHT = VK_PRIOR,
    KA_UP = VK_UP,     /* <UP ARROW> */
    KA_HOME = VK_HOME, /* <HOME> */
    KA_UPLEFT = VK_HOME,
    KA_F12 = VK_F12,
    KA_F11 = VK_F11,
    KA_F10 = VK_F10,
    KA_F9 = VK_F9,
    KA_F8 = VK_F8,
    KA_F7 = VK_F7,
    KA_F6 = VK_F6,
    KA_F5 = VK_F5,
    KA_F4 = VK_F4,
    KA_F3 = VK_F3,
    KA_F2 = VK_F2,
    KA_F1 = VK_F1,
    KA_LMOUSE = VK_LBUTTON,
    KA_RMOUSE = VK_RBUTTON,

    KA_SHIFT_BIT = WWKEY_SHIFT_BIT,
    KA_CTRL_BIT = WWKEY_CTRL_BIT,
    KA_ALT_BIT = WWKEY_ALT_BIT,
    KA_RLSE_BIT = WWKEY_RLS_BIT,
} KeyASCIIType;

typedef enum KeyNumType : unsigned short
{
    KN_NONE = 0,

    KN_0 = VK_0,
    KN_1 = VK_1,
    KN_2 = VK_2,
    KN_3 = VK_3,
    KN_4 = VK_4,
    KN_5 = VK_5,
    KN_6 = VK_6,
    KN_7 = VK_7,
    KN_8 = VK_8,
    KN_9 = VK_9,
    KN_A = VK_A,
    KN_B = VK_B,
    KN_BACKSLASH = VK_NONE_DC,
    KN_BACKSPACE = VK_BACK,
    KN_C = VK_C,
    KN_CAPSLOCK = VK_CAPITAL,
    KN_CENTER = VK_CLEAR,
    KN_COMMA = VK_NONE_BC,
    KN_D = VK_D,
    KN_DELETE = VK_DELETE,
    KN_DOWN = VK_DOWN,
    KN_DOWNLEFT = VK_END,
    KN_DOWNRIGHT = VK_NEXT,
    KN_E = VK_E,
    KN_END = VK_END,
    KN_EQUAL = VK_NONE_BB,
    KN_ESC = VK_ESCAPE,
    KN_E_DELETE = VK_DELETE,
    KN_E_DOWN = VK_NUMPAD2,
    KN_E_END = VK_NUMPAD1,
    KN_E_HOME = VK_NUMPAD7,
    KN_E_INSERT = VK_INSERT,
    KN_E_LEFT = VK_NUMPAD4,
    KN_E_PGDN = VK_NUMPAD3,
    KN_E_PGUP = VK_NUMPAD9,
    KN_E_RIGHT = VK_NUMPAD6,
    KN_E_UP = VK_NUMPAD8,
    KN_F = VK_F,
    KN_F1 = VK_F1,
    KN_F10 = VK_F10,
    KN_F11 = VK_F11,
    KN_F12 = VK_F12,
    KN_F2 = VK_F2,
    KN_F3 = VK_F3,
    KN_F4 = VK_F4,
    KN_F5 = VK_F5,
    KN_F6 = VK_F6,
    KN_F7 = VK_F7,
    KN_F8 = VK_F8,
    KN_F9 = VK_F9,
    KN_G = VK_G,
    KN_GRAVE = VK_NONE_C0,
    KN_H = VK_H,
    KN_HOME = VK_HOME,
    KN_I = VK_I,
    KN_INSERT = VK_INSERT,
    KN_J = VK_J,
    KN_K = VK_K,
    KN_KEYPAD_ASTERISK = VK_MULTIPLY,
    KN_KEYPAD_MINUS = VK_SUBTRACT,
    KN_KEYPAD_PLUS = VK_ADD,
    KN_KEYPAD_RETURN = VK_RETURN,
    KN_KEYPAD_SLASH = VK_DIVIDE,
    KN_L = VK_L,
    KN_LALT = VK_MENU,
    KN_LBRACKET = VK_NONE_DB,
    KN_LCTRL = VK_CONTROL,
    KN_LEFT = VK_LEFT,
    KN_LMOUSE = VK_LBUTTON,
    KN_LSHIFT = VK_SHIFT,
    KN_M = VK_M,
    KN_MINUS = VK_NONE_BD,
    KN_MMOUSE = VK_MBUTTON,
    KN_N = VK_N,
    KN_NUMLOCK = VK_NUMLOCK,
    KN_O = VK_O,
    KN_P = VK_P,
    KN_PAUSE = VK_PAUSE,
    KN_PERIOD = VK_NONE_BE,
    KN_PGDN = VK_NEXT,
    KN_PGUP = VK_PRIOR,
    KN_PRNTSCRN = VK_PRINT,
    KN_Q = VK_Q,
    KN_R = VK_R,
#ifdef SDL2_BUILD
    KN_RALT = SDL_SCANCODE_RALT,
#elif defined(SDL1_BUILD)
    KN_RALT = SDLK_RALT,
#else
    KN_RALT = VK_MENU,
#endif
    KN_RBRACKET = VK_NONE_DD,
#ifdef SDL2_BUILD
    KN_RCTRL = SDL_SCANCODE_RCTRL,
#elif defined(SDL1_BUILD)
    KN_RCTRL = SDLK_RCTRL,
#else
    KN_RCTRL = VK_CONTROL,
#endif
    KN_RETURN = VK_RETURN,
    KN_RIGHT = VK_RIGHT,
    KN_RMOUSE = VK_RBUTTON,
#ifdef SDL2_BUILD
    KN_RSHIFT = SDL_SCANCODE_RSHIFT,
#elif defined(SDL1_BUILD)
    KN_RSHIFT = SDLK_RSHIFT,
#else
    KN_RSHIFT = VK_SHIFT,
#endif
    KN_S = VK_S,
    KN_SCROLLLOCK = VK_SCROLL,
    KN_SEMICOLON = VK_NONE_BA,
    KN_SLASH = VK_NONE_BF,
    KN_SPACE = VK_SPACE,
    KN_SQUOTE = VK_NONE_DE,
    KN_T = VK_T,
    KN_TAB = VK_TAB,
    KN_U = VK_U,
    KN_UP = VK_UP,
    KN_UPLEFT = VK_HOME,
    KN_UPRIGHT = VK_PRIOR,
    KN_V = VK_V,
    KN_W = VK_W,
    KN_X = VK_X,
    KN_Y = VK_Y,
    KN_Z = VK_Z,

    KN_SHIFT_BIT = WWKEY_SHIFT_BIT,
    KN_CTRL_BIT = WWKEY_CTRL_BIT,
    KN_ALT_BIT = WWKEY_ALT_BIT,
    KN_RLSE_BIT = WWKEY_RLS_BIT,
    KN_BUTTON = WWKEY_BTN_BIT,

    KN_MOUSEWHEEL_UP = VK_MOUSEWHEEL_UP,
    KN_MOUSEWHEEL_DOWN = VK_MOUSEWHEEL_DOWN,
} KeyNumType;

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
#ifdef VITA
    bool Is_Analog_Only_Scroll();
#endif
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
#ifdef VITA
        CONTROLLER_L_DEADZONE_MOUSE = 4000,
        CONTROLLER_L_DEADZONE_SCROLL = 6000,
#else
        CONTROLLER_L_DEADZONE = 4000,
#endif
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

#ifdef VITA
    bool AnalogStickMouse = true;
    void Handle_Touch_Event(const SDL_TouchFingerEvent& event);
    SDL_FingerID FirstFingerId = 0;
    int16_t NumTouches = 0;

    static constexpr int REAR_TOUCH_SPEED_MOD = 100;
    static constexpr int REAR_LMB_DELAY = 250;
    uint32_t LastRearTouchTime = 0;
    SDL_FingerID RearFirstFingerId = 0;
    int16_t RearNumTouches = 0;
#endif
#endif
};

#endif
