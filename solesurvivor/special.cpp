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

/* $Header:   F:\projects\c&c\vcs\code\special.cpv   1.4   16 Oct 1995 16:50:06   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SPECIAL.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 05/27/95                                                     *
 *                                                                                             *
 *                  Last Update : May 27, 1995 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "common/framelimit.h"

#define OPTION_WIDTH  236
#define OPTION_HEIGHT 162
#define OPTION_X      ((320 - OPTION_WIDTH) / 2)
#define OPTION_Y      (200 - OPTION_HEIGHT) / 2

void Special_Dialog(void)
{
    SpecialClass oldspecial = Special;
    GadgetClass* buttons = NULL;
    static struct
    {
        int Description;
        int Setting;
        CheckBoxClass* Button;
    } _options[] = {
        //		{TXT_DEFENDER_ADVANTAGE, 0, 0},
        {TXT_SEPARATE_HELIPAD, 0, 0},
        {TXT_VISIBLE_TARGET, 0, 0},
        {TXT_TREE_TARGET, 0, 0},
        {TXT_MCV_DEPLOY, 0, 0},
        {TXT_SMART_DEFENCE, 0, 0},
        {TXT_THREE_POINT, 0, 0},
        //		{TXT_TIBERIUM_GROWTH, 0, 0},
        //		{TXT_TIBERIUM_SPREAD, 0, 0},
        {TXT_TIBERIUM_FAST, 0, 0},
        {TXT_ROAD_PIECES, 0, 0},
        {TXT_SCATTER, 0, 0},
        {TXT_SHOW_NAMES, 0, 0},
    };

    TextButtonClass ok(200, TXT_OK, TPF_6PT_GRAD | TPF_NOSHADOW, OPTION_X + 5, OPTION_Y + OPTION_HEIGHT - 15);
    TextButtonClass cancel(
        201, TXT_CANCEL, TPF_6PT_GRAD | TPF_NOSHADOW, OPTION_X + OPTION_WIDTH - 50, OPTION_Y + OPTION_HEIGHT - 15);
    buttons = &ok;
    cancel.Add(*buttons);
    int index;
    for (index = 0; index < sizeof(_options) / sizeof(_options[0]); index++) {
        _options[index].Button = new CheckBoxClass(100 + index, OPTION_X + 7, OPTION_Y + 20 + (index * 10));
        if (_options[index].Button) {
            _options[index].Button->Add(*buttons);

            bool value = false;
            switch (_options[index].Description) {
            case TXT_SEPARATE_HELIPAD:
                value = Special.IsSeparate;
                break;

            case TXT_SHOW_NAMES:
                value = Special.IsNamed;
                break;

            case TXT_DEFENDER_ADVANTAGE:
                value = Special.IsDefenderAdvantage;
                break;

            case TXT_VISIBLE_TARGET:
                value = Special.IsVisibleTarget;
                break;

            case TXT_TREE_TARGET:
                value = Special.IsTreeTarget;
                break;

            case TXT_MCV_DEPLOY:
                value = Special.IsMCVDeploy;
                break;

            case TXT_SMART_DEFENCE:
                value = Special.IsSmartDefense;
                break;

            case TXT_THREE_POINT:
                value = Special.IsThreePoint;
                break;

            case TXT_TIBERIUM_GROWTH:
                value = Special.IsTGrowth;
                break;

            case TXT_TIBERIUM_SPREAD:
                value = Special.IsTSpread;
                break;

            case TXT_TIBERIUM_FAST:
                value = Special.IsTFast;
                break;

            case TXT_ROAD_PIECES:
                value = Special.IsRoad;
                break;

            case TXT_SCATTER:
                value = Special.IsScatter;
                break;
            }

            _options[index].Setting = value;
            if (value) {
                _options[index].Button->Turn_On();
            } else {
                _options[index].Button->Turn_Off();
            }
        }
    }

    Map.Override_Mouse_Shape(MOUSE_NORMAL);
    Set_Logic_Page(SeenBuff);
    bool recalc = true;
    bool display = true;
    bool process = true;
    while (process) {

        /*
        ** If we have just received input focus again after running in the background then
        ** we need to redraw.
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            display = true;
        }

        if (GameToPlay == GAME_NORMAL) {
            Call_Back();
        } else {
            if (Main_Loop()) {
                process = false;
            }
        }

        if (display) {
            display = false;

            Hide_Mouse();
            Dialog_Box(OPTION_X, OPTION_Y, OPTION_WIDTH, OPTION_HEIGHT);
            Draw_Caption(TXT_SPECIAL_OPTIONS, OPTION_X, OPTION_Y, OPTION_WIDTH);

            for (index = 0; index < sizeof(_options) / sizeof(_options[0]); index++) {
                Fancy_Text_Print(_options[index].Description,
                                 _options[index].Button->X + 10,
                                 _options[index].Button->Y,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
            }
            buttons->Draw_All();
            Show_Mouse();
        }

        KeyNumType input = buttons->Input();
        switch (input) {
        case KN_ESC:
        case 200 | KN_BUTTON:
            process = false;
            for (index = 0; index < sizeof(_options) / sizeof(_options[0]); index++) {
                switch (_options[index].Description) {
                case TXT_SEPARATE_HELIPAD:
                    oldspecial.IsSeparate = _options[index].Setting;
                    break;

                case TXT_SHOW_NAMES:
                    oldspecial.IsNamed = _options[index].Setting;
                    break;

                case TXT_DEFENDER_ADVANTAGE:
                    oldspecial.IsDefenderAdvantage = _options[index].Setting;
                    break;

                case TXT_VISIBLE_TARGET:
                    oldspecial.IsVisibleTarget = _options[index].Setting;
                    break;

                case TXT_TREE_TARGET:
                    oldspecial.IsTreeTarget = _options[index].Setting;
                    break;

                case TXT_MCV_DEPLOY:
                    oldspecial.IsMCVDeploy = _options[index].Setting;
                    break;

                case TXT_SMART_DEFENCE:
                    oldspecial.IsSmartDefense = _options[index].Setting;
                    break;

                case TXT_THREE_POINT:
                    oldspecial.IsThreePoint = _options[index].Setting;
                    break;

                case TXT_TIBERIUM_GROWTH:
                    oldspecial.IsTGrowth = _options[index].Setting;
                    break;

                case TXT_TIBERIUM_SPREAD:
                    oldspecial.IsTSpread = _options[index].Setting;
                    break;

                case TXT_TIBERIUM_FAST:
                    oldspecial.IsTFast = _options[index].Setting;
                    break;

                case TXT_ROAD_PIECES:
                    oldspecial.IsRoad = _options[index].Setting;
                    break;

                case TXT_SCATTER:
                    oldspecial.IsScatter = _options[index].Setting;
                    break;
                }
            }
            OutList.Add(EventClass(oldspecial));
            break;

        case 201 | KN_BUTTON:
            process = false;
            break;

        case KN_NONE:
            break;

        default:
            index = (input & ~KN_BUTTON) - 100;
            if ((unsigned)index < sizeof(_options) / sizeof(_options[0])) {
                _options[index].Setting = (_options[index].Setting == false);
                if (_options[index].Setting) {
                    _options[index].Button->Turn_On();
                } else {
                    _options[index].Button->Turn_Off();
                }
            }
            break;
        }
    }

    Map.Revert_Mouse_Shape();
    HiddenPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();
}

int Fetch_Difficulty(void)
{
    static const char TXT_EASY[] = "Easy";
    static const char TXT_NORMAL[] = "Normal";
    static const char TXT_HARD[] = "Hard";
    static const char TXT_DIFFICULTY[] = "Difficulty";

    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
    int const w = 250 * factor;
    int const h = 80 * factor;
    int const x = ((320 * factor) / 2) - w / 2;
    int const y = ((200 * factor) / 2) - h / 2;
    int const bwidth = 30 * factor;

    /*
    **	Fill the description buffer with the description text. Break
    **	the text into appropriate spacing.
    */
    char buffer[512];
    strncpy(buffer, TXT_DIFFICULTY, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    Fancy_Text_Print(TXT_NONE, 0, 0, CC_GREEN, TBLACK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
    int width;
    int height;
    Format_Window_String(buffer, w - 60 * factor, width, height);

    /*
    **	Create the OK button.
    */
    TextButtonClass okbutton(1, TXT_OK, TPF_BUTTON, (x + w) - (bwidth + 20 * factor), (y + h) - (18 * factor), bwidth);
    GadgetClass* buttonlist = &okbutton;

    /*
    **	Create the slider button.
    */
    SliderClass slider(2, x + 20 * factor, y + h - 29 * factor, w - 40 * factor, 8 * factor, true);
    if (Rule.IsFineDifficulty) {
        slider.Set_Maximum(5);
        slider.Set_Value(2);
    } else {
        slider.Set_Maximum(3);
        slider.Set_Value(1);
    }
    slider.Add(*buttonlist);

    /*
    **	Main Processing Loop.
    */
    Set_Logic_Page(SeenBuff);
    bool redraw = true;
    bool process = true;
    while (process) {

        if (redraw) {
            redraw = false;

            /*
            **	Draw the background of the dialog.
            */
            Hide_Mouse();
            Dialog_Box(x, y, w, h);
            Draw_Caption(TXT_NONE, x, y, w);

            /*
            **	Draw the body of the message.
            */
            Fancy_Text_Print(buffer,
                             w / 2 + x,
                             5 * factor + y,
                             CC_GREEN,
                             TBLACK,
                             TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

            /*
            **	Display the descripton of the slider range.
            */
            Fancy_Text_Print(TXT_HARD,
                             slider.X + slider.Width,
                             slider.Y - 9 * factor,
                             CC_GREEN,
                             TBLACK,
                             TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_DROPSHADOW);
            Fancy_Text_Print(TXT_EASY,
                             slider.X,
                             slider.Y - 9 * factor,
                             CC_GREEN,
                             TBLACK,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_DROPSHADOW);
            Fancy_Text_Print(TXT_NORMAL,
                             slider.X + (slider.Width / 2),
                             slider.Y - 9 * factor,
                             CC_GREEN,
                             TBLACK,
                             TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_DROPSHADOW);

            /*
            **	Redraw the buttons.
            */
            if (buttonlist) {
                buttonlist->Draw_All();
            }
            Show_Mouse();
        }

        /*
        **	Invoke game callback.
        */
        Call_Back();

        /*
        ** Handle possible surface loss due to a focus switch
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            redraw = true;
            continue;
        }

        /*
        **	Fetch and process input.
        */
        KeyNumType input = buttonlist->Input();

        switch (input) {
        case KN_RETURN:
        case (1 | BUTTON_FLAG):
            process = false;
            break;

        default:
            break;
        }

        Frame_Limiter();
    }

    return (slider.Get_Value() * (Rule.IsFineDifficulty ? 1 : 2));
}

unsigned SpecialClass::Pack_Bools()
{
    unsigned ret = 0;
    ret |= IsScrollMod << 0;
    ret |= IsGross << 1;
    ret |= IsEasy << 2;
    ret |= IsDifficult << 3;
    ret |= IsSpeedBuild << 4;
    ret |= IsDefenderAdvantage << 5;
    ret |= IsVisibleTarget << 6;
    ret |= IsVariation << 7;
    ret |= IsJurassic << 8;
    ret |= IsJuvenile << 9;
    ret |= IsSmartDefense << 10;
    ret |= IsTreeTarget << 11;
    ret |= IsMCVDeploy << 12;
    ret |= IsVisceroids << 13;
    ret |= IsMonoEnabled << 14;
    ret |= IsInert << 15;
    ret |= IsShowPath << 16;
    ret |= IsThreePoint << 17;
    ret |= IsTGrowth << 18;
    ret |= IsTSpread << 19;
    ret |= IsTFast << 20;
    ret |= IsRoad << 21;
    ret |= IsScatter << 22;
    ret |= IsCaptureTheFlag << 23;
    ret |= IsNamed << 24;
    ret |= IsFromInstall << 25;
    ret |= IsSeparate << 26;
    ret |= IsEarlyWin << 27;

    return ret;
}

void SpecialClass::Unpack_Bools(unsigned special)
{
    IsScrollMod = (special & (1 << 0)) != 0;
    IsGross = (special & (1 << 1)) != 0;
    IsEasy = (special & (1 << 2)) != 0;
    IsDifficult = (special & (1 << 3)) != 0;
    IsSpeedBuild = (special & (1 << 4)) != 0;
    IsDefenderAdvantage = (special & (1 << 5)) != 0;
    IsVisibleTarget = (special & (1 << 6)) != 0;
    IsVariation = (special & (1 << 7)) != 0;
    IsJurassic = (special & (1 << 8)) != 0;
    IsJuvenile = (special & (1 << 9)) != 0;
    IsSmartDefense = (special & (1 << 10)) != 0;
    IsTreeTarget = (special & (1 << 11)) != 0;
    IsMCVDeploy = (special & (1 << 12)) != 0;
    IsVisceroids = (special & (1 << 13)) != 0;
    IsMonoEnabled = (special & (1 << 14)) != 0;
    IsInert = (special & (1 << 15)) != 0;
    IsShowPath = (special & (1 << 16)) != 0;
    IsThreePoint = (special & (1 << 17)) != 0;
    IsTGrowth = (special & (1 << 18)) != 0;
    IsTSpread = (special & (1 << 19)) != 0;
    IsTFast = (special & (1 << 20)) != 0;
    IsRoad = (special & (1 << 21)) != 0;
    IsScatter = (special & (1 << 22)) != 0;
    IsCaptureTheFlag = (special & (1 << 23)) != 0;
    IsNamed = (special & (1 << 24)) != 0;
    IsFromInstall = (special & (1 << 25)) != 0;
    IsSeparate = (special & (1 << 26)) != 0;
    IsEarlyWin = (special & (1 << 27)) != 0;
}
