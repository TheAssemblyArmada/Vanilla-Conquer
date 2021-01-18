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

/* $Header: /CounterStrike/MPLAYER.CPP 3     3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : MPLAYER.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Bill Randolph                                                *
 *                                                                                             *
 *                   Start Date : April 14, 1995                                               *
 *                                                                                             *
 *                  Last Update : November 30, 1995 [BRR]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Select_MPlayer_Game -- prompts user for NULL-Modem, Modem, or Network game                *
 *   Clear_Listbox -- clears the given list box                                                *
 *   Clear_Vector -- clears the given NodeNameType vector                                      *
 *   Computer_Message -- "sends" a message from the computer                                   *
 *   Garble_Message -- "garbles" a message                                                     *
 *   Surrender_Dialog -- Prompts user for surrendering                                         *
 *   Abort_Dialog -- Prompts user for confirmation on aborting the mission							  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "list.h"
#include "textbtn.h"
#include "common/framelimit.h"

extern bool Is_Mission_Counterstrike(char* file_name);

/***********************************************************************************************
 * Select_MPlayer_Game -- prompts user for NULL-Modem, Modem, or Network game                  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      GAME_NORMAL, GAME_MODEM, etc.                                                          *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
GameType Select_MPlayer_Game(void)
{
    //------------------------------------------------------------------------
    //	Dialog & button dimensions
    //------------------------------------------------------------------------
    int d_dialog_w = 190 * RESFACTOR;
    int d_dialog_h = 78 * RESFACTOR;
    int d_dialog_y = 90 * RESFACTOR;
    int d_dialog_x = (((320 * RESFACTOR) - d_dialog_w) / 2);
    int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);

    int d_txt6_h = 7 * RESFACTOR;
    int d_margin = 7 * RESFACTOR;

    int d_skirmish_w = 80 * RESFACTOR;
    int d_skirmish_h = 9 * RESFACTOR;
    int d_skirmish_x = d_dialog_cx - d_skirmish_w / 2;
    int d_skirmish_y = d_dialog_y + d_margin + d_txt6_h + d_margin;

    int d_ipx_w = 80 * RESFACTOR;
    int d_ipx_h = 9 * RESFACTOR;
    int d_ipx_x = d_dialog_cx - d_ipx_w / 2;
    int d_ipx_y = d_skirmish_y + d_skirmish_h + 2 * RESFACTOR;

    int d_cancel_w = 60 * RESFACTOR;
    int d_cancel_h = 9 * RESFACTOR;
    int d_cancel_x = d_dialog_cx - d_cancel_w / 2;
    int d_cancel_y = d_ipx_y + d_ipx_h + d_margin;

    GraphicBufferClass seen_buff_save(VisiblePage.Get_Width(), VisiblePage.Get_Height(), (void*)NULL);

    //------------------------------------------------------------------------
    //	Button enumerations:
    //------------------------------------------------------------------------
    enum
    {
        BUTTON_SKIRMISH = 100,
        BUTTON_IPX,
        BUTTON_CANCEL,
        NUM_OF_BUTTONS = 3,
    };

    int num_of_buttons = NUM_OF_BUTTONS - (Ipx.Is_IPX() ? 0 : 1);
    //------------------------------------------------------------------------
    //	Redraw values: in order from "top" to "bottom" layer of the dialog
    //------------------------------------------------------------------------
    typedef enum
    {
        REDRAW_NONE = 0,
        REDRAW_BUTTONS,    // includes map interior & coord values
        REDRAW_BACKGROUND, // includes box, map bord, key, coord labels, btns
        REDRAW_ALL = REDRAW_BACKGROUND
    } RedrawType;

    //------------------------------------------------------------------------
    //	Dialog variables:
    //------------------------------------------------------------------------
    KeyNumType input;   // input from user
    bool process;       // loop while true
    RedrawType display; // true = re-draw everything
    GameType retval;    // return value
    int selection;
    bool pressed;
    int curbutton;
    TextButtonClass* buttons[NUM_OF_BUTTONS];

    //------------------------------------------------------------------------
    //	Buttons
    //------------------------------------------------------------------------
    ControlClass* commands = NULL; // the button list

    //------------------------------------------------------------------------
    // If IPX not active then do only the modem serial dialog
    //------------------------------------------------------------------------
    //	if ( !Ipx.Is_IPX() ) {
    //		return( Select_Serial_Dialog() );
    //	}

    TextButtonClass skirmishbtn(
        BUTTON_SKIRMISH, TXT_SKIRMISH, TPF_BUTTON, d_skirmish_x, d_skirmish_y, d_skirmish_w, d_skirmish_h);

    TextButtonClass ipxbtn(BUTTON_IPX, TXT_NETWORK, TPF_BUTTON, d_ipx_x, d_ipx_y, d_ipx_w, d_ipx_h);

    if (!Ipx.Is_IPX()) {
        d_cancel_y = d_ipx_y;
        d_dialog_h -= d_cancel_h;
    }

    TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_BUTTON, d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);

    //------------------------------------------------------------------------
    //	Initialize
    //------------------------------------------------------------------------
    Set_Logic_Page(SeenBuff);
    VisiblePage.Blit(seen_buff_save);
    //------------------------------------------------------------------------
    //	Create the list
    //------------------------------------------------------------------------
    commands = &skirmishbtn;
    //skirmishbtn.Add_Tail(*commands);
    if (Ipx.Is_IPX()) {
        ipxbtn.Add_Tail(*commands);
    }
    cancelbtn.Add_Tail(*commands);

    //------------------------------------------------------------------------
    //	Fill array of button ptrs
    //------------------------------------------------------------------------
    curbutton = 0;
    buttons[0] = &skirmishbtn;
    if (Ipx.Is_IPX()) {
        buttons[1] = &ipxbtn;
        buttons[2] = &cancelbtn;
    } else {
        buttons[1] = &cancelbtn;
    }
    buttons[curbutton]->Turn_On();

    Keyboard->Clear();

    Fancy_Text_Print(TXT_NONE, 0, 0, GadgetClass::Get_Color_Scheme(), TBLACK, TPF_CENTER | TPF_TEXT);

    //------------------------------------------------------------------------
    //	Main Processing Loop
    //------------------------------------------------------------------------
    display = REDRAW_ALL;
    process = true;
    pressed = false;
    while (process) {
        /*
        ** If we have just received input focus again after running in the background then
        ** we need to redraw.
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            seen_buff_save.Blit(VisiblePage);
            display = REDRAW_ALL;
        }

        //.....................................................................
        //	Invoke game callback
        //.....................................................................
        Call_Back();

        //.....................................................................
        //	Refresh display if needed
        //.....................................................................
        if (display) {
            Hide_Mouse();
            if (display >= REDRAW_BACKGROUND) {

                //...............................................................
                //	Refresh the backdrop
                //...............................................................
                Load_Title_Page(true);
                CCPalette.Set();

                //...............................................................
                //	Draw the background
                //...............................................................
                Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
                Draw_Caption(TXT_SELECT_MPLAYER_GAME, d_dialog_x, d_dialog_y, d_dialog_w);
            }

            //..................................................................
            //	Redraw buttons
            //..................................................................
            if (display >= REDRAW_BUTTONS) {
                commands->Flag_List_To_Redraw();
            }
            Show_Mouse();
            display = REDRAW_NONE;
        }

        //.....................................................................
        //	Get user input
        //.....................................................................
        input = commands->Input();

        //.....................................................................
        //	Process input
        //.....................................................................
        switch (input) {
        case (BUTTON_SKIRMISH | KN_BUTTON):
            selection = BUTTON_SKIRMISH;
            pressed = true;
            break;

        case (BUTTON_IPX | KN_BUTTON):
            selection = BUTTON_IPX;
            pressed = true;
            break;

        case (KN_ESC):
        case (BUTTON_CANCEL | KN_BUTTON):
            selection = BUTTON_CANCEL;
            pressed = true;
            break;

        case KN_UP:
            buttons[curbutton]->Turn_Off();
            buttons[curbutton]->Flag_To_Redraw();
            curbutton--;
            if (curbutton < 0)
                curbutton = (num_of_buttons - 1);
            buttons[curbutton]->Turn_On();
            buttons[curbutton]->Flag_To_Redraw();
            break;

        case KN_DOWN:
            buttons[curbutton]->Turn_Off();
            buttons[curbutton]->Flag_To_Redraw();
            curbutton++;
            if (curbutton > (num_of_buttons - 1))
                curbutton = 0;
            buttons[curbutton]->Turn_On();
            buttons[curbutton]->Flag_To_Redraw();
            break;

        case KN_RETURN:
            selection = curbutton + BUTTON_SKIRMISH;
            pressed = true;
            break;

        default:
            break;
        }

        if (pressed) {

            //..................................................................
            // to make sure the selection is correct in case they used the mouse
            //..................................................................
            buttons[curbutton]->Turn_Off();
            buttons[curbutton]->Flag_To_Redraw();
            curbutton = selection - BUTTON_SKIRMISH;
            if (selection == BUTTON_CANCEL && !Ipx.Is_IPX())
                curbutton--;
            buttons[curbutton]->Turn_On();
            buttons[curbutton]->IsPressed = true;
            buttons[curbutton]->Draw_Me(true);

            switch (selection) {
            case (BUTTON_SKIRMISH):
                Session.Type = GAME_SKIRMISH;
#ifndef REMASTER_BUILD
                if (Com_Scenario_Dialog(true)) {
                    retval = GAME_SKIRMISH;
                    process = false;
#ifdef FIXIT_VERSION_3
                    bAftermathMultiplayer = Is_Aftermath_Installed();
                    //	ajw I'll bet this was needed before also...
                    Session.ScenarioIsOfficial = Session.Scenarios[Session.Options.ScenarioIndex]->Get_Official();
#endif
                } else {
                    buttons[curbutton]->IsPressed = false;
                    Session.Type = GAME_NORMAL;
                    display = REDRAW_ALL;
                }
#endif
                break;

            case (BUTTON_IPX):
                retval = GAME_IPX;
                process = false;
                break;

            case (BUTTON_CANCEL):
                retval = GAME_NORMAL;
                process = false;
                break;
            }

            pressed = false;
        }

        Frame_Limiter();
    }
    return (retval);

} /* end of Select_MPlayer_Game */

/***************************************************************************
 * Clear_Listbox -- clears the given list box                              *
 *                                                                         *
 * This routine assumes the items in the given list box are character		*
 * buffers; it deletes each item in the list, then clears the list.			*
 *                                                                         *
 * INPUT:                                                                  *
 *		list			ptr to listbox															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1995 BRR : Created.                                             *
 *=========================================================================*/
void Clear_Listbox(ListClass* list)
{
    char* item;

    //------------------------------------------------------------------------
    //	Clear the list box
    //------------------------------------------------------------------------
    while (list->Count()) {
        item = (char*)(list->Get_Item(0));
        list->Remove_Item(item);
        delete[] item;
    }
    list->Flag_To_Redraw();

} // end of Clear_Listbox

/***************************************************************************
 * Clear_Vector -- clears the given NodeNameType vector                    *
 *                                                                         *
 * INPUT:                                                                  *
 *		vector		ptr to vector to clear												*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1995 BRR : Created.                                             *
 *=========================================================================*/
void Clear_Vector(DynamicVectorClass<NodeNameType*>* vector)
{
    int i;

    //------------------------------------------------------------------------
    //	Clear the 'Players' Vector
    //------------------------------------------------------------------------
    for (i = 0; i < vector->Count(); i++) {
        delete (*vector)[i];
    }
    vector->Clear();

} // end of Clear_Vector

/***************************************************************************
 * Computer_Message -- "sends" a message from the computer                 *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/06/1995 BRR : Created.                                             *
 *=========================================================================*/
void Computer_Message(void)
{
#ifdef NEVER
    int color;
    HousesType house;
    HouseClass* ptr;

    //------------------------------------------------------------------------
    //	Find the computer house that the message will be from
    //------------------------------------------------------------------------
    for (house = HOUSE_MULTI1; house < (HOUSE_MULTI1 + Session.MaxPlayers); house++) {
        ptr = HouseClass::As_Pointer(house);

        if (!ptr || ptr->IsHuman || ptr->IsDefeated) {
            continue;
        }

        //.....................................................................
        //	Decode this house's color
        //.....................................................................
        color = ptr->RemapColor;

        //.....................................................................
        //	We now have a 1/4 chance of echoing one of the human players'
        // messages back.
        //.....................................................................
        if (Percent_Chance(25)) {

            //..................................................................
            //	Now we have a 1/3 chance of garbling the human message.
            //..................................................................
            if (Percent_Chance(33)) {
                Garble_Message(Session.LastMessage);
            }

            //..................................................................
            //	Only add the message if there is one to add.
            //..................................................................
            if (strlen(Session.LastMessage)) {
                Session.Messages.Add_Message(Text_String(TXT_COMPUTER),
                                             0,
                                             Session.LastMessage,
                                             color,
                                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                                             Rule.MessageDelay * TICKS_PER_MINUTE);
            }
        } else {
            Session.Messages.Add_Message(Text_String(TXT_COMPUTER),
                                         0,
                                         Text_String(TXT_COMP_MSG1 + Random_Pick(0, 12)),
                                         color,
                                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                                         Rule.MessageDelay * TICKS_PER_MINUTE);
        }

        return;
    }
#endif
} /* end of Computer_Message */

#ifdef NEVER
/***************************************************************************
 * Garble_Message -- "garbles" a message                                   *
 *                                                                         *
 * INPUT:                                                                  *
 *      buf      buffer to garble; stores output message                   *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/06/1995 BRR : Created.                                             *
 *=========================================================================*/
static void Garble_Message(char* buf)
{
    char txt[80];
    char punct[20];  // for punctuation
    char* p;         // working ptr
    int numwords;    // # words in the phrase
    char* words[40]; // ptrs to various words in the phrase
    int i, j;

    //------------------------------------------------------------------------
    //	Pull off any trailing punctuation
    //------------------------------------------------------------------------
    p = buf + strlen(buf) - 1;
    while (1) {
        if (p < buf)
            break;
        if (p[0] == '!' || p[0] == '.' || p[0] == '?') {
            p--;
        } else {
            p++;
            break;
        }
        if (strlen(p) >= (sizeof(punct) - 1)) {
            break;
        }
    }
    strcpy(punct, p);
    p[0] = 0;

    for (i = 0; i < 40; i++) {
        words[i] = NULL;
    }

    //------------------------------------------------------------------------
    //	Copy the original buffer
    //------------------------------------------------------------------------
    strcpy(txt, buf);

    //------------------------------------------------------------------------
    //	Split it up into words
    //------------------------------------------------------------------------
    p = strtok(txt, " ");
    numwords = 0;
    while (p) {
        words[numwords] = p;
        numwords++;
        p = strtok(NULL, " ");
    }

    //------------------------------------------------------------------------
    //	Now randomly put the words back.  Don't use the real random-number
    //	generator, since different machines will have different LastMessage's,
    //	and will go out of sync.
    //------------------------------------------------------------------------
    buf[0] = 0;
    for (i = 0; i < numwords; i++) {
        j = Sim_IRandom(0, numwords);
        if (words[j] == NULL) { // this word has been used already
            i--;
            continue;
        }
        strcat(buf, words[j]);
        words[j] = NULL;
        if (i < numwords - 1)
            strcat(buf, " ");
    }
    strcat(buf, punct);

} /* end of Garble_Message */
#endif

/***************************************************************************
 * Surrender_Dialog -- Prompts user for surrendering                       *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = user cancels, 1 = user wants to surrender.                     *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 BRR : Created.                                             *
 *=========================================================================*/
#ifdef FIXIT_VERSION_3 //	Stalemate games.
int Surrender_Dialog(int text)
{
    return Surrender_Dialog(Text_String(text));
}
#endif

#ifdef FIXIT_VERSION_3 //	Stalemate games.
int Surrender_Dialog(const char* text)
#else
int Surrender_Dialog(int text)
#endif
{
    //------------------------------------------------------------------------
    //	Dialog & button dimensions
    //------------------------------------------------------------------------
    enum
    {
        D_DIALOG_W = 240 * RESFACTOR,                      // dialog width
        D_DIALOG_H = 63 * RESFACTOR,                       // dialog height
        D_DIALOG_X = ((320 * RESFACTOR - D_DIALOG_W) / 2), // centered x-coord
        D_DIALOG_Y = ((200 * RESFACTOR - D_DIALOG_H) / 2), // centered y-coord
        D_DIALOG_CX = D_DIALOG_X + (D_DIALOG_W / 2),       // coord of x-center

        D_TXT6_H = 7 * RESFACTOR,     // ht of 6-pt text
        D_MARGIN = 5 * RESFACTOR,     // margin width/height
        D_TOPMARGIN = 20 * RESFACTOR, // top margin

        D_OK_W = 45 * RESFACTOR,                                  // OK width
        D_OK_H = 9 * RESFACTOR,                                   // OK height
        D_OK_X = D_DIALOG_CX - D_OK_W - 5 * RESFACTOR,            // OK x
        D_OK_Y = D_DIALOG_Y + D_DIALOG_H - D_OK_H - D_MARGIN * 2, // OK y

        D_CANCEL_W = 45 * RESFACTOR,                                      // Cancel width
        D_CANCEL_H = 9 * RESFACTOR,                                       // Cancel height
        D_CANCEL_X = D_DIALOG_CX + 5 * RESFACTOR,                         // Cancel x
        D_CANCEL_Y = D_DIALOG_Y + D_DIALOG_H - D_CANCEL_H - D_MARGIN * 2, // Cancel y
    };

    //------------------------------------------------------------------------
    //	Button enumerations
    //------------------------------------------------------------------------
    enum
    {
        BUTTON_OK = 100,
        BUTTON_CANCEL,
    };

    //------------------------------------------------------------------------
    //	Buttons
    //------------------------------------------------------------------------
    ControlClass* commands = NULL; // the button list

    TextButtonClass okbtn(BUTTON_OK, TXT_OK, TPF_BUTTON, D_OK_X, D_OK_Y, D_OK_W, D_OK_H);

    TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_BUTTON, D_CANCEL_X, D_CANCEL_Y, D_CANCEL_W, D_CANCEL_H);

    int curbutton;
    TextButtonClass* buttons[2];
    curbutton = 0;

    //------------------------------------------------------------------------
    //	Initialize
    //------------------------------------------------------------------------
    Set_Logic_Page(SeenBuff);

    //------------------------------------------------------------------------
    //	Create the button list
    //------------------------------------------------------------------------
    commands = &okbtn;
    cancelbtn.Add_Tail(*commands);

    buttons[0] = &okbtn;
    buttons[1] = &cancelbtn;
    buttons[curbutton]->Turn_On();

    //------------------------------------------------------------------------
    //	Main Processing Loop
    //------------------------------------------------------------------------
    int retcode = 0;
    bool display = true;
    bool process = true;
    while (process) {

        //.....................................................................
        //	Invoke game callback
        //.....................................................................
        if (Session.Type != GAME_SKIRMISH) {
            if (Main_Loop()) {
                retcode = 0;
                process = false;
            }
        }

        //.....................................................................
        //	Refresh display if needed
        //.....................................................................
        if (display) {
            display = false;

            //..................................................................
            //	Display the dialog box
            //..................................................................
            Hide_Mouse();
            Dialog_Box(D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W, D_DIALOG_H);
            Draw_Caption(TXT_NONE, D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W);

            //...............................................................
            //	Draw the captions
            //...............................................................
#ifdef FIXIT_VERSION_3 //	Stalemate games.
            Fancy_Text_Print(text,
                             D_DIALOG_CX,
                             D_DIALOG_Y + D_TOPMARGIN,
                             GadgetClass::Get_Color_Scheme(),
                             TBLACK,
                             TPF_CENTER | TPF_TEXT);
#else
            Fancy_Text_Print(Text_String(text),
                             D_DIALOG_CX,
                             D_DIALOG_Y + D_TOPMARGIN,
                             GadgetClass::Get_Color_Scheme(),
                             TBLACK,
                             TPF_CENTER | TPF_TEXT);
#endif

            //..................................................................
            //	Redraw the buttons
            //..................................................................
            commands->Flag_List_To_Redraw();
            Show_Mouse();
        }

        //.....................................................................
        //	Get user input
        //.....................................................................
        KeyNumType input = commands->Input();

        //.....................................................................
        //	Process input
        //.....................................................................
        switch (input) {
        case (BUTTON_OK | KN_BUTTON):
            retcode = 1;
            process = false;
            break;

        case (BUTTON_CANCEL | KN_BUTTON):
            retcode = 0;
            process = false;
            break;

        case (KN_RETURN):
            if (curbutton == 0) {
                retcode = 1;
            } else {
                retcode = 0;
            }
            process = false;
            break;

        case (KN_ESC):
            retcode = 0;
            process = false;
            break;

        case (KN_RIGHT):
            buttons[curbutton]->Turn_Off();
            curbutton++;
            if (curbutton > 1) {
                curbutton = 0;
            }
            buttons[curbutton]->Turn_On();
            break;

        case (KN_LEFT):
            buttons[curbutton]->Turn_Off();
            curbutton--;
            if (curbutton < 0) {
                curbutton = 1;
            }
            buttons[curbutton]->Turn_On();
            break;

        default:
            break;
        }

        Frame_Limiter();
    }

    //------------------------------------------------------------------------
    //	Redraw the display
    //------------------------------------------------------------------------
    HidPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();

    return (retcode);
}

/***************************************************************************
 * Abort_Dialog -- Prompts user for confirmation on aborting the mission	*
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = user confirms abort, 0 = user cancels									*
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int Abort_Dialog(void)
{
    //------------------------------------------------------------------------
    //	Dialog & button dimensions
    //------------------------------------------------------------------------
    enum
    {
        D_DIALOG_W = 170 * RESFACTOR,                      // dialog width
        D_DIALOG_H = 63 * RESFACTOR,                       // dialog height
        D_DIALOG_X = ((320 * RESFACTOR - D_DIALOG_W) / 2), // centered x-coord
        D_DIALOG_Y = ((200 * RESFACTOR - D_DIALOG_H) / 2), // centered y-coord
        D_DIALOG_CX = D_DIALOG_X + (D_DIALOG_W / 2),       // coord of x-center

        D_TXT6_H = 7 * RESFACTOR,     // ht of 6-pt text
        D_MARGIN = 5 * RESFACTOR,     // margin width/height
        D_TOPMARGIN = 20 * RESFACTOR, // top margin

        D_YES_W = 45 * RESFACTOR,                                   // YES width
        D_YES_H = 9 * RESFACTOR,                                    // YES height
        D_YES_X = D_DIALOG_CX - D_YES_W - 5 * RESFACTOR,            // YES x
        D_YES_Y = D_DIALOG_Y + D_DIALOG_H - D_YES_H - D_MARGIN * 2, // YES y

        D_NO_W = 45 * RESFACTOR,                                  // Cancel width
        D_NO_H = 9 * RESFACTOR,                                   // Cancel height
        D_NO_X = D_DIALOG_CX + 5 * RESFACTOR,                     // Cancel x
        D_NO_Y = D_DIALOG_Y + D_DIALOG_H - D_NO_H - D_MARGIN * 2, // Cancel y
    };

    //------------------------------------------------------------------------
    //	Button enumerations
    //------------------------------------------------------------------------
    enum
    {
        BUTTON_YES = 100,
        BUTTON_NO,
    };

    //------------------------------------------------------------------------
    //	Buttons
    //------------------------------------------------------------------------
    ControlClass* commands = NULL; // the button list

    TextButtonClass yesbtn(BUTTON_YES, TXT_YES, TPF_BUTTON, D_YES_X, D_YES_Y, D_YES_W, D_YES_H);

    TextButtonClass nobtn(BUTTON_NO, TXT_NO, TPF_BUTTON, D_NO_X, D_NO_Y, D_NO_W, D_NO_H);

    int curbutton;
    TextButtonClass* buttons[2];
    curbutton = 0;

    //------------------------------------------------------------------------
    //	Initialize
    //------------------------------------------------------------------------
    Set_Logic_Page(SeenBuff);

    //------------------------------------------------------------------------
    //	Create the button list
    //------------------------------------------------------------------------
    commands = &yesbtn;
    nobtn.Add_Tail(*commands);

    buttons[0] = &yesbtn;
    buttons[1] = &nobtn;
    buttons[curbutton]->Turn_On();

    //------------------------------------------------------------------------
    //	Main Processing Loop
    //------------------------------------------------------------------------
    int retcode = 0;
    bool display = true;
    bool process = true;
    while (process) {

        //.....................................................................
        //	Invoke game callback
        //.....................................................................
        if (Session.Type != GAME_SKIRMISH) {
            if (Main_Loop()) {
                retcode = 0;
                process = false;
            }
        }

        //.....................................................................
        //	Refresh display if needed
        //.....................................................................
        if (display) {
            display = false;

            //..................................................................
            //	Display the dialog box
            //..................................................................
            Hide_Mouse();
            Dialog_Box(D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W, D_DIALOG_H);
            Draw_Caption(TXT_NONE, D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W);

            //...............................................................
            //	Draw the captions
            //...............................................................
            Fancy_Text_Print(Text_String(TXT_CONFIRM_EXIT),
                             D_DIALOG_CX,
                             D_DIALOG_Y + D_TOPMARGIN,
                             GadgetClass::Get_Color_Scheme(),
                             TBLACK,
                             TPF_CENTER | TPF_TEXT);

            //..................................................................
            //	Redraw the buttons
            //..................................................................
            commands->Flag_List_To_Redraw();
            Show_Mouse();
        }

        //.....................................................................
        //	Get user input
        //.....................................................................
        KeyNumType input = commands->Input();

        //.....................................................................
        //	Process input
        //.....................................................................
        switch (input) {
        case (BUTTON_YES | KN_BUTTON):
            retcode = 1;
            process = false;
            break;

        case (BUTTON_NO | KN_BUTTON):
            retcode = 0;
            process = false;
            break;

        case (KN_RETURN):
            if (curbutton == 0) {
                retcode = 1;
            } else {
                retcode = 0;
            }
            process = false;
            break;

        case (KN_ESC):
            retcode = 0;
            process = false;
            break;

        case (KN_RIGHT):
            buttons[curbutton]->Turn_Off();
            curbutton++;
            if (curbutton > 1) {
                curbutton = 0;
            }
            buttons[curbutton]->Turn_On();
            break;

        case (KN_LEFT):
            buttons[curbutton]->Turn_Off();
            curbutton--;
            if (curbutton < 0) {
                curbutton = 1;
            }
            buttons[curbutton]->Turn_On();
            break;

        default:
            break;
        }

        Frame_Limiter();
    }

    //------------------------------------------------------------------------
    //	Redraw the display
    //------------------------------------------------------------------------
    HidPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();

    return (retcode);
}

/************************** end of mplayer.cpp *****************************/
