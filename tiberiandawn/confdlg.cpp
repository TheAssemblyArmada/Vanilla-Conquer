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

/* $Header:   F:\projects\c&c\vcs\code\confdlg.cpv   2.17   16 Oct 1995 16:49:52   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : CONFDLG.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg                                  *
 *                                  Joe L. Bostic                                              *
 *                                                                                             *
 *                   Start Date : Jan 30, 1995                                                 *
 *                                                                                             *
 *                  Last Update : Jan 30, 1995   [MML]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   ConfirmationClass::Process -- Handles all the options graphic interface.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "confdlg.h"

bool ConfirmationClass::Process(int text)
{
    return (Process(Text_String(text)));
}

/***********************************************************************************************
 * ConfirmationClass::Process -- Handles all the options graphic interface.                    *
 *                                                                                             *
 *    This dialog uses an edit box to confirm a deletion.                                      *
 *                                                                                             *
 * INPUT:   	char *string - display in edit box.                                             *
 * OUTPUT:  	none                                                                            *
 * WARNINGS:   none                                                                            *
 * HISTORY:    12/31/1994 MML : Created.                                                       *
 *=============================================================================================*/
bool ConfirmationClass::Process(char const* string)
{
    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;

    enum
    {
        NUM_OF_BUTTONS = 2
    };

    char buffer[80 * 3];
    int result = true;
    int width;
    int bwidth, bheight; // button width and height
    int height;
    int selection;
    bool pressed;
    int curbutton;
    TextButtonClass* buttons[NUM_OF_BUTTONS];

    /*
    **	Set up the window.  Window x-coords are in bytes not pixels.
    */
    strcpy(buffer, string);
    Fancy_Text_Print(TXT_NONE, 0, 0, TBLACK, TBLACK, TPF_6PT_GRAD | TPF_NOSHADOW);
    Format_Window_String(buffer, 200 * factor, width, height);
    width += 60 * factor;
    height += 60 * factor;
    int x = (320 * factor - width) / 2;
    int y = (200 * factor - height) / 2;

    Set_Logic_Page(SeenBuff);

    /*
    **	Create Buttons.  Button coords are in pixels, but are window-relative.
    */

    bheight = FontHeight + FontYSpacing + 2;
    bwidth = MAX((String_Pixel_Width(Text_String(TXT_YES)) + 8), 30U);

    TextButtonClass yesbtn(BUTTON_YES,
                           TXT_YES,
                           TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                           x + 10 * factor,
                           y + height - (bheight + 5 * factor),
                           bwidth);

    TextButtonClass nobtn(BUTTON_NO,
                          TXT_NO,
                          TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                          x + width - (bwidth + 10 * factor),
                          y + height - (bheight + 5 * factor),
                          bwidth);

    nobtn.Add_Tail(yesbtn);

    curbutton = 1;
    buttons[0] = &yesbtn;
    buttons[1] = &nobtn;
    buttons[curbutton]->Turn_On();

    /*
    **	This causes left mouse button clicking within the confines of the dialog to
    **	be ignored if it wasn't recognized by any other button or slider.
    */
    GadgetClass dialog(x, y, width, height, GadgetClass::LEFTPRESS);
    dialog.Add_Tail(yesbtn);

    /*
    **	This causes a right click anywhere or a left click outside the dialog region
    **	to be equivalent to clicking on the return to options dialog.
    */
    ControlClass background(
        BUTTON_NO, 0, 0, SeenBuff.Get_Width(), SeenBuff.Get_Height(), GadgetClass::LEFTPRESS | GadgetClass::RIGHTPRESS);
    background.Add_Tail(yesbtn);

    /*
    **	Main Processing Loop.
    */
    bool display = true;
    bool process = true;
    pressed = false;
    while (process) {

        /*
        **	Invoke game callback.
        */
        if (GameToPlay == GAME_NORMAL) {
            Call_Back();
        } else {
            if (Main_Loop()) {
                process = false;
                result = false;
            }
        }

        /*
        ** If we have just received input focus again after running in the background then
        ** we need to redraw.
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            display = true;
        }

        /*
        **	Refresh display if needed.
        */
        if (display) {
            Hide_Mouse();

            /*
            **	Draw the background.
            */
            Dialog_Box(x, y, width, height);
            Draw_Caption(TXT_CONFIRMATION, x, y, width);
            Fancy_Text_Print(buffer,
                             x + 20 * factor,
                             y + 30 * factor,
                             CC_GREEN,
                             TBLACK,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

            /*
            **	Draw the titles.
            */
            yesbtn.Draw_All();
            Show_Mouse();
            display = false;
        }

        /*
        **	Get user input.
        */
        KeyNumType input = yesbtn.Input();

        /*
        **	Process Input.
        */
        switch (input) {
        case (BUTTON_YES | KN_BUTTON):
            selection = BUTTON_YES;
            pressed = true;
            break;

        case (KN_ESC):
        case (BUTTON_NO | KN_BUTTON):
            selection = BUTTON_NO;
            pressed = true;
            break;

        case (KN_LEFT):
            buttons[curbutton]->Turn_Off();
            buttons[curbutton]->Flag_To_Redraw();

            curbutton--;
            if (curbutton < 0) {
                curbutton = NUM_OF_BUTTONS - 1;
            }

            buttons[curbutton]->Turn_On();
            buttons[curbutton]->Flag_To_Redraw();
            break;

        case (KN_RIGHT):
            buttons[curbutton]->Turn_Off();
            buttons[curbutton]->Flag_To_Redraw();

            curbutton++;
            if (curbutton > (NUM_OF_BUTTONS - 1)) {
                curbutton = 0;
            }

            buttons[curbutton]->Turn_On();
            buttons[curbutton]->Flag_To_Redraw();
            break;

        case (KN_RETURN):
            selection = curbutton + BUTTON_YES;
            pressed = true;
            break;

        default:
            break;
        }

        if (pressed) {
            switch (selection) {
            case (BUTTON_YES):
                result = true;
                process = false;
                break;

            case (BUTTON_NO):
                result = false;
                process = false;
                break;
            }

            pressed = false;
        }
    }
    return (result);
}
