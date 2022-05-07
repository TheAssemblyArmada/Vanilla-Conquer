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

/* $Header:   F:\projects\c&c\vcs\code\options.cpv   2.17   16 Oct 1995 16:51:28   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : OPTIONS.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : June 8, 1994                                                 *
 *                                                                                             *
 *                  Last Update : June 30, 1995 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   OptionsClass::Adjust_Palette -- Adjusts the palette according to the settings specified.  *
 *   OptionsClass::Get_Brightness -- Fetches the current brightness setting.                   *
 *   OptionsClass::Get_Color -- Fetches the current color setting.                             *
 *   OptionsClass::Get_Contrast -- Gets the current contrast setting.                          *
 *   OptionsClass::Get_Game_Speed -- Fetches the current game speed setting.                   *
 *   OptionsClass::Get_Scroll_Rate -- Fetches the current scroll rate setting.                 *
 *   OptionsClass::Get_Tint -- Fetches the current tint setting.                               *
 *   OptionsClass::Load_Settings -- reads options settings from the INI file                   *
 *   OptionsClass::Normalize_Delay -- Normalizes delay factor to keep rate constant.           *
 *   OptionsClass::One_Time -- This performs any one time initialization for the options class.*
 *   OptionsClass::OptionsClass -- The default constructor for the options class.              *
 *   OptionsClass::Process -- Handles all the options graphic interface.                       *
 *   OptionsClass::Save_Settings -- writes options settings to the INI file                    *
 *   OptionsClass::Set -- Sets options based on current settings                               *
 *   OptionsClass::Set_Brightness -- Sets the brightness level to that specified.              *
 *   OptionsClass::Set_Color -- Sets the color to the value specified.                         *
 *   OptionsClass::Set_Contrast -- Sets the contrast to the value specified.                   *
 *   OptionsClass::Set_Game_Speed -- Sets the game speed as specified.                         *
 *   OptionsClass::Set_Repeat -- Controls the score repeat option.                             *
 *   OptionsClass::Set_Score_Volume -- Sets the global score volume to that specified.         *
 *   OptionsClass::Set_Scroll_Rate -- Sets the scroll rate as specified.                       *
 *   OptionsClass::Set_Shuffle -- Controls the play shuffle setting.                           *
 *   OptionsClass::Set_Sound_Volume -- Sets the sound effects volume level.                    *
 *   OptionsClass::Set_Tint -- Sets the tint setting.                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "options.h"
#include "common/ini.h"

#ifdef SDL_BUILD
char const* const OptionsClass::HotkeyName = "SDLHotkeys";
#else
char const* const OptionsClass::HotkeyName = "WinHotkeys";
#endif

/***********************************************************************************************
 * OptionsClass::OptionsClass -- The default constructor for the options class.                *
 *                                                                                             *
 *    This is the constructor for the options class. It handles setting up all the globals     *
 *    necessary for the options. This includes setting them to their default state.            *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/21/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
OptionsClass::OptionsClass(void)
    : KeyForceMove1(KN_LALT)
    , KeyForceMove2(KN_RALT)
    , KeyForceAttack1(KN_LCTRL)
    , KeyForceAttack2(KN_RCTRL)
    , KeySelect1(KN_LSHIFT)
    , KeySelect2(KN_RSHIFT)
    , KeyScatter(KN_X)
    , KeyStop(KN_S)
    , KeyGuard(KN_G)
    , KeyNext(KN_N)
    , KeyPrevious(KN_B)
    , KeyFormation(KN_F)
    , KeyHome1(KN_HOME)
    , KeyHome2(KN_E_HOME)
    , KeyBase(KN_H)
    , KeyResign(KN_R)
    , KeyAlliance(KN_A)
    , KeyBookmark1(KN_F9)
    , KeyBookmark2(KN_F10)
    , KeyBookmark3(KN_F11)
    , KeyBookmark4(KN_F12)
    , KeySelectView(KN_E)
    , KeyRepair(KN_T)
    , KeyRepairOn(KN_NONE)
    , KeyRepairOff(KN_NONE)
    , KeySell(KN_Y)
    , KeySellOn(KN_NONE)
    , KeySellOff(KN_NONE)
    , KeyMap(KN_U)
    , KeySidebarUp(KN_UP)
    , KeySidebarDown(KN_DOWN)
    , KeyOption1(KN_ESC)
    , KeyOption2(KN_SPACE)
    , KeyScrollLeft(KN_NONE)
    , KeyScrollRight(KN_NONE)
    , KeyScrollUp(KN_NONE)
    , KeyScrollDown(KN_NONE)
    , KeyQueueMove1(KN_Q)
    , KeyQueueMove2(KN_Q)
    , KeyTeam1(KN_1)
    , KeyTeam2(KN_2)
    , KeyTeam3(KN_3)
    , KeyTeam4(KN_4)
    , KeyTeam5(KN_5)
    , KeyTeam6(KN_6)
    , KeyTeam7(KN_7)
    , KeyTeam8(KN_8)
    , KeyTeam9(KN_9)
    , KeyTeam10(KN_0)
{
    GameSpeed = TIMER_SECOND / TICKS_PER_SECOND;
    ScrollRate = TIMER_SECOND / TICKS_PER_SECOND;
    Volume = 0xE0;
    ScoreVolume = 0x90;
    Contrast = 0x80;
    Color = 0x80;
    Contrast = 0x80;
    Tint = 0x80;
    Brightness = 0x80;
    AutoScroll = true;
#if (GERMAN | FRENCH)
    IsDeathAnnounce = true;
#else
    IsDeathAnnounce = false;
#endif
    IsScoreRepeat = false;
    IsScoreShuffle = false;
    IsFreeScroll = false;
}

/***********************************************************************************************
 * OptionsClass::One_Time -- This performs any one time initialization for the options class.  *
 *                                                                                             *
 *    This routine should be called only once and it will perform any inializations for the    *
 *    options class that is needed. This may include things like file loading and memory       *
 *    allocation.                                                                              *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Only call this routine once.                                                    *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/21/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::One_Time(void)
{
    Set_Score_Vol(ScoreVolume);
}

/***********************************************************************************************
 * OptionsClass::Process -- Handles all the options graphic interface.                         *
 *                                                                                             *
 *    This routine is the main control for the visual representation of the options            *
 *    screen. It handles the visual overlay and the player input.                              *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/21/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Process(void)
{
}

/***********************************************************************************************
 * OptionsClass::Set_Shuffle -- Controls the play shuffle setting.                             *
 *                                                                                             *
 *    This routine will control the score shuffle flag. The setting to use is provided as      *
 *    a parameter. When shuffling is on, the score play order is scrambled.                    *
 *                                                                                             *
 * INPUT:   on -- Should the shuffle option be activated?                                      *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Shuffle(int on)
{
    IsScoreShuffle = on;
}

/***********************************************************************************************
 * OptionsClass::Set_Repeat -- Controls the score repeat option.                               *
 *                                                                                             *
 *    This routine is used to control whether scores repeat or not. The setting to use for     *
 *    the repeat flag is provided as a parameter.                                              *
 *                                                                                             *
 * INPUT:   on -- Should the scores repeat?                                                    *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Repeat(int on)
{
    IsScoreRepeat = on;
}

/***********************************************************************************************
 * OptionsClass::Set_Score_Volume -- Sets the global score volume to that specified.           *
 *                                                                                             *
 *    This routine will set the global score volume to the value specified. The value ranges   *
 *    from zero to 255.                                                                        *
 *                                                                                             *
 * INPUT:   volume   -- The new volume setting to use for scores.                              *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Score_Volume(int volume)
{
    volume = Bound(volume, 0, 255);
    ScoreVolume = volume;
    Set_Score_Vol(ScoreVolume);
}

/***********************************************************************************************
 * OptionsClass::Set_Sound_Volume -- Sets the sound effects volume level.                      *
 *                                                                                             *
 *    This routine will set the sound effect volume level as indicated. It can generate a      *
 *    sound effect for feedback purposes if desired. The volume setting can range from zero    *
 *    to 255. The value of 255 is the loudest.                                                 *
 *                                                                                             *
 * INPUT:   volume   -- The volume setting to use for the new value. 0 to 255.                 *
 *                                                                                             *
 *          feedback -- Should a feedback sound effect be generated?                           *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Sound_Volume(int volume, int feedback)
{
    volume = Bound(volume, 0, 255);
    Volume = volume;
    if (feedback) {
        Sound_Effect(VOC_BLEEPY3);
    }
}

/***********************************************************************************************
 * OptionsClass::Set_Brightness -- Sets the brightness level to that specified.                *
 *                                                                                             *
 *    This routine will set the current brightness level to the value specified. This value    *
 *    can range from zero to 255, with 128 being the normal (default) brightness level.        *
 *                                                                                             *
 * INPUT:   brightness  -- The brightness level to set as current.                             *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Brightness(int brightness)
{
    Brightness = 0x40 + Fixed_To_Cardinal(0x80, brightness);
    Adjust_Palette(OriginalPalette, GamePalette, Brightness, Color, Tint, Contrast);
    if (InMainLoop) {
        Set_Palette(GamePalette);
    }
}

/***********************************************************************************************
 * OptionsClass::Get_Brightness -- Fetches the current brightness setting.                     *
 *                                                                                             *
 *    This routine will fetch the current setting for the brightness level. The value ranges   *
 *    from zero to 255, with 128 being the normal (default) value.                             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the current brightness setting.                                       *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
int OptionsClass::Get_Brightness(void) const
{
    return (Cardinal_To_Fixed(0x80, Brightness - 0x40));
}

/***********************************************************************************************
 * OptionsClass::Set_Color -- Sets the color to the value specified.                           *
 *                                                                                             *
 *    This routine will set the color value to that specified. The value specified can range   *
 *    from zero to 255. The value of 128 is the normal default color setting.                  *
 *                                                                                             *
 * INPUT:   color -- The new color value to set as current.                                    *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Color(int color)
{
    Color = color;
    Adjust_Palette(OriginalPalette, GamePalette, Brightness, Color, Tint, Contrast);
    if (InMainLoop) {
        Set_Palette(GamePalette);
    }
}

/***********************************************************************************************
 * OptionsClass::Get_Color -- Fetches the current color setting.                               *
 *                                                                                             *
 *    This routine will fetch the current color setting. This value ranges from zero to        *
 *    255.                                                                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the current color setting. The value of 128 is the normal (default)   *
 *          color setting.                                                                     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
int OptionsClass::Get_Color(void) const
{
    return (Color);
}

/***********************************************************************************************
 * OptionsClass::Set_Contrast -- Sets the contrast to the value specified.                     *
 *                                                                                             *
 *    This routine will set the constrast to the setting specified. This setting ranges from   *
 *    zero to 255. The value o 128 is the normal default value.                                *
 *                                                                                             *
 * INPUT:   contrast -- The constrast setting to make as the current setting.                  *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Contrast(int contrast)
{
    Contrast = 0x40 + Fixed_To_Cardinal(0x80, contrast);
    Adjust_Palette(OriginalPalette, GamePalette, Brightness, Color, Tint, Contrast);
    if (InMainLoop) {
        Set_Palette(GamePalette);
    }
}

/***********************************************************************************************
 * OptionsClass::Get_Contrast -- Gets the current contrast setting.                            *
 *                                                                                             *
 *    This routine will get the current contrast setting. The value returned is in the range   *
 *    of zero to 255.                                                                          *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns the current contrast setting. A setting of 128 is the normal default value.*
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
int OptionsClass::Get_Contrast(void) const
{
    return (Cardinal_To_Fixed(0x80, Contrast - 0x40));
}

/***********************************************************************************************
 * OptionsClass::Set_Tint -- Sets the tint setting.                                            *
 *                                                                                             *
 *    This routine will change the current tint setting according to the value specified.      *
 *                                                                                             *
 * INPUT:   tint  -- The desired tint setting. This value ranges from zero to 255.             *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   The value of 128 is the default (normal) tint setting.                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set_Tint(int tint)
{
    Tint = tint;
    Adjust_Palette(OriginalPalette, GamePalette, Brightness, Color, Tint, Contrast);
    if (InMainLoop) {
        Set_Palette(GamePalette);
    }
}

/***********************************************************************************************
 * OptionsClass::Get_Tint -- Fetches the current tint setting.                                 *
 *                                                                                             *
 *    This fetches the current tint setting. The value is returned as a number between         *
 *    zero and 255. This has been adjusted for the valid range allowed.                        *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the current tint setting. Normal tint setting is 128.                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
int OptionsClass::Get_Tint(void) const
{
    return (Tint);
}

/***********************************************************************************************
 * OptionsClass::Adjust_Palette -- Adjusts the palette according to the settings specified.    *
 *                                                                                             *
 *    This routine is used to adjust the palette according to the settings provided. It is     *
 *    used by the options class to monkey with the palette.                                    *
 *                                                                                             *
 * INPUT:   oldpal      -- Pointer to the original (unmodified) palette.                       *
 *                                                                                             *
 *          newpal      -- The new palette to create according to the settings provided.       *
 *                                                                                             *
 *          brightness  -- The brightness level (0..255).                                      *
 *                                                                                             *
 *          color       -- The color level (0..255).                                           *
 *                                                                                             *
 *          tint        -- The tint (hue) level (0..255).                                      *
 *                                                                                             *
 *          contrast    -- The contrast level (0..255).                                        *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/21/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Adjust_Palette(void* oldpal,
                                  void* newpal,
                                  unsigned char brightness,
                                  unsigned char color,
                                  unsigned char tint,
                                  unsigned char contrast) const
{
#ifndef REMASTER_BUILD
    int index;
    unsigned h, s, v;
    unsigned r, g, b;

    if (!oldpal || !newpal)
        return;

    /*
    **	Adjust for palette.
    */
    for (index = 0; index < 256; index++) {
        if (/*index == LTGREEN ||*/ index == 255) {
            memcpy(&((char*)newpal)[index * 3], &((char*)oldpal)[index * 3], 3);
        } else {
            r = ((char*)oldpal)[(index * 3) + 0];
            g = ((char*)oldpal)[(index * 3) + 1];
            b = ((char*)oldpal)[(index * 3) + 2];
            Convert_RGB_To_HSV(r, g, b, &h, &s, &v);

            /*
            **	Adjust contrast by moving the value toward the center according to the
            **	percentage indicated.
            */
            int temp;

            temp = (v * brightness) / 0x80; // Brightness
            temp = Bound(temp, 0, 0xFF);
            v = temp;
            temp = (((((int)v) - 0x80) * contrast) / 0x80) + 0x80; // Contrast
            temp = Bound(temp, 0, 0xFF);
            v = temp;
            temp = (s * color) / 0x80; // Color
            temp = Bound(temp, 0, 0xFF);
            s = temp;
            temp = (h * tint) / 0x80; // Tint
            temp = Bound(temp, 0, 0xFF);
            h = temp;
            Convert_HSV_To_RGB(h, s, v, &r, &g, &b);
            ((char*)newpal)[(index * 3) + 0] = r;
            ((char*)newpal)[(index * 3) + 1] = g;
            ((char*)newpal)[(index * 3) + 2] = b;
        }
    }
#endif
}

/***********************************************************************************************
 * OptionsClass::Load_Settings -- reads options settings from the INI file                     *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
void OptionsClass::Load_Settings(void)
{
    /*
    **	Create filename and read the file.
    */
    CCFileClass file(CONFIG_FILE_NAME);
    INIClass ini;
    ini.Load(file);

    /*
    **	Read in the Options values
    */
    static char const* const OPTIONS = "Options";
    GameSpeed = ini.Get_Int(OPTIONS, "GameSpeed", 4);
    ScrollRate = ini.Get_Int(OPTIONS, "ScrollRate", 4);
    Set_Brightness(ini.Get_Int(OPTIONS, "Brightness", 0x80));
    Set_Sound_Volume(ini.Get_Int(OPTIONS, "Volume", 0xA0), false);
    Set_Score_Volume(ini.Get_Int(OPTIONS, "ScoreVolume", 0xFF));
    Set_Contrast(ini.Get_Int(OPTIONS, "Contrast", 0x80));
    Set_Color(ini.Get_Int(OPTIONS, "Color", 0x80));
    Set_Tint(ini.Get_Int(OPTIONS, "Tint", 0x80));
    AutoScroll = ini.Get_Int(OPTIONS, "AutoScroll", 1);
    Set_Repeat(ini.Get_Int(OPTIONS, "IsScoreRepeat", 0));
    Set_Shuffle(ini.Get_Int(OPTIONS, "IsScoreShuffle", 0));
    IsDeathAnnounce = ini.Get_Int(OPTIONS, "DeathAnnounce", 0);
    IsFreeScroll = ini.Get_Int(OPTIONS, "FreeScrolling", 0);
    SlowPalette = ini.Get_Int(OPTIONS, "SlowPalette", 1);

    KeyForceMove1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyForceMove1", KeyForceMove1);
    KeyForceMove2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyForceMove2", KeyForceMove2);
    KeyForceAttack1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyForceAttack1", KeyForceAttack1);
    KeyForceAttack2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyForceAttack2", KeyForceAttack2);
    KeySelect1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeySelect1", KeySelect1);
    KeySelect2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeySelect2", KeySelect2);
    KeyScatter = (KeyNumType)ini.Get_Int(HotkeyName, "KeyScatter", KeyScatter);
    KeyStop = (KeyNumType)ini.Get_Int(HotkeyName, "KeyStop", KeyStop);
    KeyGuard = (KeyNumType)ini.Get_Int(HotkeyName, "KeyGuard", KeyGuard);
    KeyNext = (KeyNumType)ini.Get_Int(HotkeyName, "KeyNext", KeyNext);
    KeyPrevious = (KeyNumType)ini.Get_Int(HotkeyName, "KeyPrevious", KeyPrevious);
    KeyFormation = (KeyNumType)ini.Get_Int(HotkeyName, "KeyFormation", KeyFormation);
    KeyHome1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyHome1", KeyHome1);
    KeyHome2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyHome2", KeyHome2);
    KeyBase = (KeyNumType)ini.Get_Int(HotkeyName, "KeyBase", KeyBase);
    KeyResign = (KeyNumType)ini.Get_Int(HotkeyName, "KeyResign", KeyResign);
    KeyAlliance = (KeyNumType)ini.Get_Int(HotkeyName, "KeyAlliance", KeyAlliance);
    KeyBookmark1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyBookmark1", KeyBookmark1);
    KeyBookmark2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyBookmark2", KeyBookmark2);
    KeyBookmark3 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyBookmark3", KeyBookmark3);
    KeyBookmark4 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyBookmark4", KeyBookmark4);
    KeySelectView = (KeyNumType)ini.Get_Int(HotkeyName, "KeySelectView", KeySelectView);
    KeyRepair = (KeyNumType)ini.Get_Int(HotkeyName, "KeyRepairToggle", KeyRepair);
    KeyRepairOn = (KeyNumType)ini.Get_Int(HotkeyName, "KeyRepairOn", KeyRepairOn);
    KeyRepairOff = (KeyNumType)ini.Get_Int(HotkeyName, "KeyRepairOff", KeyRepairOff);
    KeySell = (KeyNumType)ini.Get_Int(HotkeyName, "KeySellToggle", KeySell);
    KeySellOn = (KeyNumType)ini.Get_Int(HotkeyName, "KeySellOn", KeySellOn);
    KeySellOff = (KeyNumType)ini.Get_Int(HotkeyName, "KeySellOff", KeySellOff);
    KeyMap = (KeyNumType)ini.Get_Int(HotkeyName, "KeyMapToggle", KeyMap);
    KeySidebarUp = (KeyNumType)ini.Get_Int(HotkeyName, "KeySidebarUp", KeySidebarUp);
    KeySidebarDown = (KeyNumType)ini.Get_Int(HotkeyName, "KeySidebarDown", KeySidebarDown);
    KeyOption1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyOption1", KeyOption1);
    KeyOption2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyOption2", KeyOption2);
    KeyScrollLeft = (KeyNumType)ini.Get_Int(HotkeyName, "KeyScrollLeft", KeyScrollLeft);
    KeyScrollRight = (KeyNumType)ini.Get_Int(HotkeyName, "KeyScrollRight", KeyScrollRight);
    KeyScrollUp = (KeyNumType)ini.Get_Int(HotkeyName, "KeyScrollUp", KeyScrollUp);
    KeyScrollDown = (KeyNumType)ini.Get_Int(HotkeyName, "KeyScrollDown", KeyScrollDown);
    KeyQueueMove1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyQueueMove1", KeyQueueMove1);
    KeyQueueMove2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyQueueMove2", KeyQueueMove2);
    KeyTeam1 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam1", KeyTeam1);
    KeyTeam2 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam2", KeyTeam2);
    KeyTeam3 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam3", KeyTeam3);
    KeyTeam4 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam4", KeyTeam4);
    KeyTeam5 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam5", KeyTeam5);
    KeyTeam6 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam6", KeyTeam6);
    KeyTeam7 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam7", KeyTeam7);
    KeyTeam8 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam8", KeyTeam8);
    KeyTeam9 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam9", KeyTeam9);
    KeyTeam10 = (KeyNumType)ini.Get_Int(HotkeyName, "KeyTeam10", KeyTeam10);

    KeyForceMove1 = (KeyNumType)(KeyForceMove1 & ~WWKEY_VK_BIT);
    KeyForceMove2 = (KeyNumType)(KeyForceMove2 & ~WWKEY_VK_BIT);
    KeyForceAttack1 = (KeyNumType)(KeyForceAttack1 & ~WWKEY_VK_BIT);
    KeyForceAttack2 = (KeyNumType)(KeyForceAttack2 & ~WWKEY_VK_BIT);
    KeySelect1 = (KeyNumType)(KeySelect1 & ~WWKEY_VK_BIT);
    KeySelect2 = (KeyNumType)(KeySelect2 & ~WWKEY_VK_BIT);
    KeyScatter = (KeyNumType)(KeyScatter & ~WWKEY_VK_BIT);
    KeyStop = (KeyNumType)(KeyStop & ~WWKEY_VK_BIT);
    KeyGuard = (KeyNumType)(KeyGuard & ~WWKEY_VK_BIT);
    KeyNext = (KeyNumType)(KeyNext & ~WWKEY_VK_BIT);
    KeyPrevious = (KeyNumType)(KeyPrevious & ~WWKEY_VK_BIT);
    KeyFormation = (KeyNumType)(KeyFormation & ~WWKEY_VK_BIT);
    KeyHome1 = (KeyNumType)(KeyHome1 & ~WWKEY_VK_BIT);
    KeyHome2 = (KeyNumType)(KeyHome2 & ~WWKEY_VK_BIT);
    KeyBase = (KeyNumType)(KeyBase & ~WWKEY_VK_BIT);
    KeyResign = (KeyNumType)(KeyResign & ~WWKEY_VK_BIT);
    KeyAlliance = (KeyNumType)(KeyAlliance & ~WWKEY_VK_BIT);
    KeyBookmark1 = (KeyNumType)(KeyBookmark1 & ~WWKEY_VK_BIT);
    KeyBookmark2 = (KeyNumType)(KeyBookmark2 & ~WWKEY_VK_BIT);
    KeyBookmark3 = (KeyNumType)(KeyBookmark3 & ~WWKEY_VK_BIT);
    KeyBookmark4 = (KeyNumType)(KeyBookmark4 & ~WWKEY_VK_BIT);
    KeySelectView = (KeyNumType)(KeySelectView & ~WWKEY_VK_BIT);
    KeyRepair = (KeyNumType)(KeyRepair & ~WWKEY_VK_BIT);
    KeyRepairOn = (KeyNumType)(KeyRepairOn & ~WWKEY_VK_BIT);
    KeyRepairOff = (KeyNumType)(KeyRepairOff & ~WWKEY_VK_BIT);
    KeySell = (KeyNumType)(KeySell & ~WWKEY_VK_BIT);
    KeySellOn = (KeyNumType)(KeySellOn & ~WWKEY_VK_BIT);
    KeySellOff = (KeyNumType)(KeySellOff & ~WWKEY_VK_BIT);
    KeyMap = (KeyNumType)(KeyMap & ~WWKEY_VK_BIT);
    KeySidebarUp = (KeyNumType)(KeySidebarUp & ~WWKEY_VK_BIT);
    KeySidebarDown = (KeyNumType)(KeySidebarDown & ~WWKEY_VK_BIT);
    KeyOption1 = (KeyNumType)(KeyOption1 & ~WWKEY_VK_BIT);
    KeyOption2 = (KeyNumType)(KeyOption2 & ~WWKEY_VK_BIT);
    KeyScrollLeft = (KeyNumType)(KeyScrollLeft & ~WWKEY_VK_BIT);
    KeyScrollRight = (KeyNumType)(KeyScrollRight & ~WWKEY_VK_BIT);
    KeyScrollUp = (KeyNumType)(KeyScrollUp & ~WWKEY_VK_BIT);
    KeyScrollDown = (KeyNumType)(KeyScrollDown & ~WWKEY_VK_BIT);
    KeyQueueMove1 = (KeyNumType)(KeyQueueMove1 & ~WWKEY_VK_BIT);
    KeyQueueMove2 = (KeyNumType)(KeyQueueMove2 & ~WWKEY_VK_BIT);
    KeyTeam1 = (KeyNumType)(KeyTeam1 & ~WWKEY_VK_BIT);
    KeyTeam2 = (KeyNumType)(KeyTeam2 & ~WWKEY_VK_BIT);
    KeyTeam3 = (KeyNumType)(KeyTeam3 & ~WWKEY_VK_BIT);
    KeyTeam4 = (KeyNumType)(KeyTeam4 & ~WWKEY_VK_BIT);
    KeyTeam5 = (KeyNumType)(KeyTeam5 & ~WWKEY_VK_BIT);
    KeyTeam6 = (KeyNumType)(KeyTeam6 & ~WWKEY_VK_BIT);
    KeyTeam7 = (KeyNumType)(KeyTeam7 & ~WWKEY_VK_BIT);
    KeyTeam8 = (KeyNumType)(KeyTeam8 & ~WWKEY_VK_BIT);
    KeyTeam9 = (KeyNumType)(KeyTeam9 & ~WWKEY_VK_BIT);
    KeyTeam10 = (KeyNumType)(KeyTeam10 & ~WWKEY_VK_BIT);

    char workbuf[128];

    /*
    **	Check for and possible enable true object names.
    */
    ini.Get_String(OPTIONS, "TrueNames", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_TRUENAME) {
        Special.IsNamed = true;
    }

    /*
    **	Enable 6 player games if special flag is detected.
    */
    ini.Get_String(OPTIONS, "Players", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_6PLAYER) {
        MPlayerMax = 6;
    }

    /*
    **	Enable three point turning logic as indicated.
    */
    ini.Get_String(OPTIONS, "Rotation", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_3POINT) {
        Special.IsThreePoint = true;
    }

    /*
    **	Allow purchase of the helipad separately from the helicopter.
    */
    ini.Get_String(OPTIONS, "Helipad", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_HELIPAD) {
        Special.IsSeparate = true;
    }

    /*
    **	Allow the MCV to undeploy rather than sell.
    */
    ini.Get_String(OPTIONS, "MCV", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_MCV) {
        Special.IsMCVDeploy = true;
    }

    /*
    **	Allow disabling of building bibs so that tigher building packing can occur.
    */
    ini.Get_String(OPTIONS, "Bibs", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_BIB) {
        Special.IsRoad = true;
    }

    /*
    **	Allow targeting of trees without having to hold down the shift key.
    */
    ini.Get_String(OPTIONS, "TreeTarget", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_TREETARGET) {
        Special.IsTreeTarget = true;
    }

    /*
    **	Allow infantry to fire while moving. Attacker gets advantage with this flag.
    */
    ini.Get_String(OPTIONS, "Combat", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_COMBAT) {
        Special.IsDefenderAdvantage = false;
    }

    /*
    **	Allow custom scores.
    */
    ini.Get_String(OPTIONS, "Scores", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_SCORE) {
        Special.IsVariation = true;
    }

    /*
    **	Smarter self defense logic. Tanks will try to run over adjacent infantry. Buildings
    **	will automatically return fire if they are fired upon. Infantry will run from an
    **	incoming explosive (grenade or napalm) or damage that can't be directly addressed.
    */
    ini.Get_String(OPTIONS, "CombatIQ", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_IQ) {
        Special.IsSmartDefense = true;
        Special.IsScatter = true;
    }

    /*
    **	Enable the infantry squish marks when run over by a vehicle.
    */
    ini.Get_String(OPTIONS, "Overrun", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_SQUISH) {
        Special.IsGross = true;
    }

    /*
    **	Enable the human generated sound effects.
    */
    ini.Get_String(OPTIONS, "Sounds", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_HUMAN) {
        Special.IsJuvenile = true;
    }

    /*
    **	Scrolling is disabled over the tabs with this option.
    */
    ini.Get_String(OPTIONS, "Scrolling", "", workbuf, sizeof(workbuf));
    if (Obfuscate(workbuf) == PARM_SCROLLING) {
        Special.IsScrollMod = true;
    }
}

/***********************************************************************************************
 * OptionsClass::Save_Settings -- writes options settings to the INI file                      *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
void OptionsClass::Save_Settings(void)
{
    /*
    **	Create filename and read the file.
    */
    CCFileClass file(CONFIG_FILE_NAME);
    INIClass ini;
    ini.Load(file);

    /*
    **	Save Options settings
    */
    static char const* const OPTIONS = "Options";
    ini.Put_Int(OPTIONS, "GameSpeed", GameSpeed);
    ini.Put_Int(OPTIONS, "ScrollRate", ScrollRate);
    ini.Put_Int(OPTIONS, "Brightness", Brightness);
    ini.Put_Int(OPTIONS, "Volume", Volume);
    ini.Put_Int(OPTIONS, "ScoreVolume", ScoreVolume);
    ini.Put_Int(OPTIONS, "Contrast", Contrast);
    ini.Put_Int(OPTIONS, "Color", Color);
    ini.Put_Int(OPTIONS, "Tint", Tint);
    ini.Put_Int(OPTIONS, "AutoScroll", AutoScroll);
    ini.Put_Int(OPTIONS, "IsScoreRepeat", IsScoreRepeat);
    ini.Put_Int(OPTIONS, "IsScoreShuffle", IsScoreShuffle);
    ini.Put_Int(OPTIONS, "DeathAnnounce", IsDeathAnnounce);
    ini.Put_Int(OPTIONS, "FreeScrolling", IsFreeScroll);

    ini.Put_Int(HotkeyName, "KeyForceMove1", KeyForceMove1);
    ini.Put_Int(HotkeyName, "KeyForceMove2", KeyForceMove2);
    ini.Put_Int(HotkeyName, "KeyForceAttack1", KeyForceAttack1);
    ini.Put_Int(HotkeyName, "KeyForceAttack2", KeyForceAttack2);
    ini.Put_Int(HotkeyName, "KeySelect1", KeySelect1);
    ini.Put_Int(HotkeyName, "KeySelect2", KeySelect2);
    ini.Put_Int(HotkeyName, "KeyScatter", KeyScatter);
    ini.Put_Int(HotkeyName, "KeyStop", KeyStop);
    ini.Put_Int(HotkeyName, "KeyGuard", KeyGuard);
    ini.Put_Int(HotkeyName, "KeyNext", KeyNext);
    ini.Put_Int(HotkeyName, "KeyPrevious", KeyPrevious);
    ini.Put_Int(HotkeyName, "KeyFormation", KeyFormation);
    ini.Put_Int(HotkeyName, "KeyHome1", KeyHome1);
    ini.Put_Int(HotkeyName, "KeyHome2", KeyHome2);
    ini.Put_Int(HotkeyName, "KeyBase", KeyBase);
    ini.Put_Int(HotkeyName, "KeyResign", KeyResign);
    ini.Put_Int(HotkeyName, "KeyAlliance", KeyAlliance);
    ini.Put_Int(HotkeyName, "KeyBookmark1", KeyBookmark1);
    ini.Put_Int(HotkeyName, "KeyBookmark2", KeyBookmark2);
    ini.Put_Int(HotkeyName, "KeyBookmark3", KeyBookmark3);
    ini.Put_Int(HotkeyName, "KeyBookmark4", KeyBookmark4);
    ini.Put_Int(HotkeyName, "KeySelectView", KeySelectView);
    ini.Put_Int(HotkeyName, "KeyRepairToggle", KeyRepair);
    ini.Put_Int(HotkeyName, "KeyRepairOn", KeyRepairOn);
    ini.Put_Int(HotkeyName, "KeyRepairOff", KeyRepairOff);
    ini.Put_Int(HotkeyName, "KeySellToggle", KeySell);
    ini.Put_Int(HotkeyName, "KeySellOn", KeySellOn);
    ini.Put_Int(HotkeyName, "KeySellOff", KeySellOff);
    ini.Put_Int(HotkeyName, "KeyMapToggle", KeyMap);
    ini.Put_Int(HotkeyName, "KeySidebarUp", KeySidebarUp);
    ini.Put_Int(HotkeyName, "KeySidebarDown", KeySidebarDown);
    ini.Put_Int(HotkeyName, "KeyOption1", KeyOption1);
    ini.Put_Int(HotkeyName, "KeyOption2", KeyOption2);
    ini.Put_Int(HotkeyName, "KeyScrollLeft", KeyScrollLeft);
    ini.Put_Int(HotkeyName, "KeyScrollRight", KeyScrollRight);
    ini.Put_Int(HotkeyName, "KeyScrollUp", KeyScrollUp);
    ini.Put_Int(HotkeyName, "KeyScrollDown", KeyScrollDown);
    ini.Put_Int(HotkeyName, "KeyQueueMove1", KeyQueueMove1);
    ini.Put_Int(HotkeyName, "KeyQueueMove2", KeyQueueMove2);
    ini.Put_Int(HotkeyName, "KeyTeam1", KeyTeam1);
    ini.Put_Int(HotkeyName, "KeyTeam2", KeyTeam2);
    ini.Put_Int(HotkeyName, "KeyTeam3", KeyTeam3);
    ini.Put_Int(HotkeyName, "KeyTeam4", KeyTeam4);
    ini.Put_Int(HotkeyName, "KeyTeam5", KeyTeam5);
    ini.Put_Int(HotkeyName, "KeyTeam6", KeyTeam6);
    ini.Put_Int(HotkeyName, "KeyTeam7", KeyTeam7);
    ini.Put_Int(HotkeyName, "KeyTeam8", KeyTeam8);
    ini.Put_Int(HotkeyName, "KeyTeam9", KeyTeam9);
    ini.Put_Int(HotkeyName, "KeyTeam10", KeyTeam10);

    /*
    **	Write the INI data out to a file.
    */
    ini.Save(file);
}

/***********************************************************************************************
 * OptionsClass::Set -- Sets options based on current settings                                 *
 *                                                                                             *
 * Use this routine to adjust the palette or sound settings after a fresh scenario load.       *
 * It assumes the values needed are already loaded into OptionsClass.                          *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/24/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
void OptionsClass::Set(void)
{
    Set_Brightness(Brightness);
    Set_Contrast(Contrast);
    Set_Color(Color);
    Set_Tint(Tint);
    Set_Sound_Volume(Volume, false);
    Set_Score_Volume(ScoreVolume);
    Set_Repeat(IsScoreRepeat);
    Set_Shuffle(IsScoreShuffle);
}

/***********************************************************************************************
 * OptionsClass::Normalize_Delay -- Normalizes delay factor to keep rate constant.             *
 *                                                                                             *
 *    This routine is used to adjust delay factors that MUST be synchronized on all machines   *
 *    but should maintain a speed as close to constant as possible. Building animations are    *
 *    a good example of this.                                                                  *
 *                                                                                             *
 * INPUT:   delay -- The normal delay factor.                                                  *
 *                                                                                             *
 * OUTPUT:  Returns with the delay to use that has been modified so that a reasonably constant *
 *          rate will result.                                                                  *
 *                                                                                             *
 * WARNINGS:   This calculation is crude due to the coarse resolution that a 1/15 second timer *
 *             allows.                                                                         *
 *                                                                                             *
 *             Use of this routine ASSUMES that the GameSpeed is synchronized on all machines. *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/18/1995 JLB : Created.                                                                 *
 *   06/30/1995 JLB : Handles low values in a more consistent manner.                          *
 *=============================================================================================*/
int OptionsClass::Normalize_Delay(int delay) const
{
    static int _adjust[][8] = {
        {2, 2, 1, 1, 1, 1, 1, 1}, {3, 3, 3, 2, 2, 2, 1, 1}, {5, 4, 4, 3, 3, 2, 2, 1}, {7, 6, 5, 4, 4, 4, 3, 2}};
    if (delay) {
        if (delay < 5) {
            delay = _adjust[delay - 1][GameSpeed];
        } else {
            delay = ((delay * 8) / (GameSpeed + 1));
        }
    }
    return (delay);
}

void OptionsClass::Fixup_Palette(void) const
{
    Adjust_Palette(OriginalPalette, GamePalette, Brightness, Color, Tint, Contrast);
}

int OptionsClass::Normalize_Sound(int volume) const
{
    return (Fixed_To_Cardinal(volume, Volume));
}