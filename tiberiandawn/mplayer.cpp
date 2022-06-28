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

/* $Header:   F:\projects\c&c\vcs\code\mplayer.cpv   1.9   16 Oct 1995 16:51:08   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
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
 *                  Last Update : July 5, 1995 [BRR]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Select_MPlayer_Game -- prompts user for NULL-Modem, Modem, or Network game                *
 *   Read_MultiPlayer_Settings -- reads multi-player settings from conquer.ini                 *
 *   Write_MultiPlayer_Settings -- writes multi-player settings to conquer.ini                 *
 *   Read_Scenario_Descriptions -- reads multi-player scenario #'s # descriptions              *
 *   Free_Scenario_Descriptions -- frees memory for the scenario descriptions                  *
 *   Computer_Message -- "sends" a message from the computer                                   *
 *   Garble_Message -- "garbles" a message                                                     *
 *   Surrender_Dialog -- Prompts user for surrendering                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "common/irandom.h"
#include "common/tcpip.h"
#include "common/ini.h"
#include "common/framelimit.h"
#include "common/ini.h"

static void Garble_Message(char* buf);

int Choose_Internet_Game(void);
int Get_Internet_Host_Or_Join(void);
int Get_IP_Address(void);
void Show_Internet_Connection_Progress(void);

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
#ifdef REMASTER_BUILD // PG_TO_FIX
    return GAME_NORMAL;
#else
    static const char TXT_SKIRMISH[] = "Skirmish";
    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
    bool ipx_avail = false;
    int number_of_buttons;
    /*........................................................................
    Dialog & button dimensions
    ........................................................................*/
    int d_dialog_w = 190 * factor;
    int d_dialog_h = 26 * 4 * factor;
    int d_dialog_x = ((320 * factor - d_dialog_w) / 2);
    //	d_dialog_y = ((200 - d_dialog_h) / 2),
    int d_dialog_y = ((136 * factor - d_dialog_h) / 2);
    int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);

    int d_txt6_h = 11 * factor;
    int d_margin = 7 * factor;

    int d_modemserial_w = 80 * factor;
    int d_modemserial_h = 9 * factor;
    int d_modemserial_x = d_dialog_cx - d_modemserial_w / 2;
    int d_modemserial_y = d_dialog_y + d_margin + d_txt6_h + d_margin;
#if (0)
    int d_internet_w = 80 * factor;
    int d_internet_h = 9 * factor;
    int d_internet_x = d_dialog_cx - d_internet_w / 2;
    int d_internet_y = d_modemserial_y + d_modemserial_h + 2 * factor;
#endif //(0)
    int d_ipx_w = 80 * factor;
    int d_ipx_h = 9 * factor;
    int d_ipx_x = d_dialog_cx - d_ipx_w / 2;
    int d_ipx_y = d_modemserial_y + d_modemserial_h + 2 * factor;
    //	int 	d_ipx_y = d_internet_y + d_internet_h + 2*factor;

    int d_cancel_w = 60 * factor;
    int d_cancel_h = 9 * factor;
    int d_cancel_x = d_dialog_cx - d_cancel_w / 2;
    int d_cancel_y = d_ipx_y + d_ipx_h + d_margin;

    CountDownTimerClass delay;

    /*........................................................................
    Button enumerations:
    ........................................................................*/
    enum
    {
        BUTTON_SKIRMISH = 100,
#if (0)
        BUTTON_INTERNET,
#endif //(0)
        BUTTON_IPX,
        BUTTON_CANCEL,

        NUM_OF_BUTTONS = 3,
    };
    number_of_buttons = NUM_OF_BUTTONS;
    /*........................................................................
    Redraw values: in order from "top" to "bottom" layer of the dialog
    ........................................................................*/
    typedef enum
    {
        REDRAW_NONE = 0,
        REDRAW_BUTTONS,    // includes map interior & coord values
        REDRAW_BACKGROUND, // includes box, map bord, key, coord labels, btns
        REDRAW_ALL = REDRAW_BACKGROUND
    } RedrawType;
    /*........................................................................
    Dialog variables:
    ........................................................................*/
    KeyNumType input;   // input from user
    bool process;       // loop while true
    RedrawType display; // true = re-draw everything
    GameType retval;    // return value
    int selection;
    bool pressed;
    int curbutton;
    TextButtonClass* buttons[NUM_OF_BUTTONS];

    /*........................................................................
    Buttons
    ........................................................................*/
    ControlClass* commands = NULL; // the button list

    //
    // If neither IPX or winsock are active then do only the modem serial dialog
    //
    if (Ipx.Is_IPX()) {
        ipx_avail = true;
    }

    TextButtonClass skirmishbtn(BUTTON_SKIRMISH,
                                TXT_SKIRMISH,
                                TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                                d_modemserial_x,
                                d_modemserial_y,
                                d_modemserial_w,
                                d_modemserial_h);
#if (0)
    TextButtonClass internetbtn(BUTTON_INTERNET,
                                TXT_INTERNET,
                                TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                                d_internet_x,
                                d_internet_y,
                                d_internet_w,
                                d_internet_h);
#endif //(0)
    TextButtonClass ipxbtn(BUTTON_IPX,
                           TXT_NETWORK,
                           TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                           d_ipx_x,
                           d_ipx_y,
                           d_ipx_w,
                           d_ipx_h);

    TextButtonClass cancelbtn(BUTTON_CANCEL,
                              TXT_CANCEL,
                              TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                              d_cancel_x,
                              d_cancel_y,
                              d_cancel_w,
                              d_cancel_h);

    /*
    ------------------------------- Initialize -------------------------------
    */
    Set_Logic_Page(SeenBuff);

    /*
    ............................ Create the list .............................
    */
    commands = &skirmishbtn;
#if (0)
    internetbtn.Add_Tail(*commands);
#endif //(0)
    if (ipx_avail) {
        ipxbtn.Add_Tail(*commands);
    }
    cancelbtn.Add_Tail(*commands);

    /*
    ......................... Fill array of button ptrs ......................
    */
    curbutton = 0;
    buttons[0] = &skirmishbtn;
#if (0)
    buttons[1] = &internetbtn;
#endif //(0)

    if (ipx_avail) {
        buttons[1] = &ipxbtn;
        buttons[2] = &cancelbtn;
    } else {
        --number_of_buttons;
        buttons[1] = &cancelbtn;
    }

    buttons[curbutton]->Turn_On();

    Keyboard->Clear();

    Fancy_Text_Print(TXT_NONE, 0, 0, CC_GREEN, TBLACK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

    /*
    -------------------------- Main Processing Loop --------------------------
    */
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
            display = REDRAW_ALL;
        }

        /*
        ........................ Invoke game callback .........................
        */
        Call_Back();
        /*
        ...................... Refresh display if needed ......................
        */
        if (display) {
            Hide_Mouse();
            if (display >= REDRAW_BACKGROUND) {
                /*
                ..................... Refresh the backdrop ......................
                */
                Load_Title_Screen(TitlePicture, &HidPage, Palette);
                Blit_Hid_Page_To_Seen_Buff();
                /*
                ..................... Draw the background .......................
                */
                Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
                Draw_Caption(TXT_SELECT_MPLAYER_GAME, d_dialog_x, d_dialog_y, d_dialog_w);
            }
            /*
            .......................... Redraw buttons ..........................
            */
            if (display >= REDRAW_BUTTONS) {
                commands->Flag_List_To_Redraw();
            }
            Show_Mouse();
            display = REDRAW_NONE;
        }

        /*
        ........................... Get user input ............................
        */
        input = commands->Input();

        /*
        ............................ Process input ............................
        */
        switch (input) {
        case (BUTTON_SKIRMISH | KN_BUTTON):
            selection = BUTTON_SKIRMISH;
            pressed = true;
            break;

#if (0)
        case (BUTTON_INTERNET | KN_BUTTON):
            selection = BUTTON_INTERNET;
            pressed = true;
            break;
#endif //(0)

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
                curbutton = (number_of_buttons - 1);
            buttons[curbutton]->Turn_On();
            buttons[curbutton]->Flag_To_Redraw();
            break;

        case KN_DOWN:
            buttons[curbutton]->Turn_Off();
            buttons[curbutton]->Flag_To_Redraw();
            curbutton++;
            if (curbutton > (number_of_buttons - 1))
                curbutton = 0;
            buttons[curbutton]->Turn_On();
            buttons[curbutton]->Flag_To_Redraw();
            break;

        case KN_RETURN:
            selection = curbutton + BUTTON_SKIRMISH;
            if (!ipx_avail && selection > BUTTON_SKIRMISH) {
                ++selection;
            }

            pressed = true;
            break;

        default:
            break;
        }

        if (pressed) {
            //
            // to make sure the selection is correct in case they used the mouse
            //
            buttons[curbutton]->Turn_Off();
            buttons[curbutton]->Flag_To_Redraw();
            curbutton = selection - BUTTON_SKIRMISH;
            if (!ipx_avail && curbutton > 1) {
                --curbutton;
            }
            buttons[curbutton]->Turn_On();
            //			buttons[curbutton]->Flag_To_Redraw();
            buttons[curbutton]->IsPressed = true;
            buttons[curbutton]->Draw_Me(true);

            switch (selection) {
            case (BUTTON_SKIRMISH):
                GameToPlay = GAME_SKIRMISH;
                Read_MultiPlayer_Settings();
                if (Com_Scenario_Dialog()) {
                    retval = GAME_SKIRMISH;
                    process = false;
                } else {
                    buttons[curbutton]->IsPressed = false;
                    GameToPlay = GAME_NORMAL;
                    display = REDRAW_ALL;
                }

                break;

#if (0)
            case (BUTTON_INTERNET):
//#define ONLY_FOR_E3
#ifdef ONLY_FOR_E3
                Call_Back();
                Show_Internet_Connection_Progress(); // changed to do nothing
                Hide_Mouse();
                Load_Title_Screen(TitlePicture, &HidPage, Palette);
                Blit_Hid_Page_To_Seen_Buff();
                Show_Mouse();
                Call_Back();
                WWMessageBox().Process("Error! - Unable to ping KANE.WESTWOOD.COM");

                buttons[curbutton]->IsPressed = false;
                display = REDRAW_ALL;

#endif // ONLY_FOR_E3

#ifdef FORCE_WINSOCK
                if (Winsock.Init()) {
                    Read_MultiPlayer_Settings();
                    int result = Choose_Internet_Game();

                    if (!result) {
                        Winsock.Close();
                        buttons[curbutton]->IsPressed = false;
                        display = REDRAW_ALL;
                        break;
                    }

                    result = Get_Internet_Host_Or_Join();
                    if (result == 1) {
                        Winsock.Close();
                        buttons[curbutton]->IsPressed = false;
                        display = REDRAW_ALL;
                        break;
                    }
                    Server = !result;

                    if (Server) {
#if (0)
                        ModemGameToPlay = INTERNET_HOST;
                        Winsock.Start_Server();
#else
                        result = Get_IP_Address();
                        if (!result) {
                            Winsock.Close();
                            buttons[curbutton]->IsPressed = false;
                            display = REDRAW_ALL;
                            break;
                        }
                        Winsock.Set_Host_Address(PlanetWestwoodIPAddress);
                        Winsock.Start_Server();
#endif

                    } else {
                        ModemGameToPlay = INTERNET_JOIN;
                        result = Get_IP_Address();
                        if (!result) {
                            Winsock.Close();
                            buttons[curbutton]->IsPressed = false;
                            display = REDRAW_ALL;
                            break;
                        }
                        Winsock.Set_Host_Address(PlanetWestwoodIPAddress);
                        Winsock.Start_Client();
                    }

                    // CountDownTimerClass connect_timeout;
                    // connect_timeout.Set(15*60);

                    ////Show_Internet_Connection_Progress();

                    if (!Winsock.Get_Connected()) {
                        Winsock.Close();
                        return (GAME_NORMAL);
                    }

                    SerialSettingsType* settings;
                    settings = &SerialDefaults;
                    Init_Null_Modem(settings);
                    if (Server) {
                        if (Com_Scenario_Dialog()) {
                            return (GAME_INTERNET);
                        } else {
                            Winsock.Close();
                            return (GAME_NORMAL);
                        }
                    } else {
                        if (Com_Show_Scenario_Dialog()) {
                            return (GAME_INTERNET);
                        } else {
                            Winsock.Close();
                            return (GAME_NORMAL);
                        }
                    }
                }
#endif // FORCE_WINSOCK
                break;

#endif //(0)

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
#endif
}

/***********************************************************************************************
 * Read_MultiPlayer_Settings -- reads multi-player settings from conquer.ini                   *
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
void Read_MultiPlayer_Settings(void)
{
#ifndef REMASTER_BUILD
    char buf[128]; // buffer for parsing INI entry
    CELL cell;

    //	Create filename and read the file.
    INIClass ini;
    CDFileClass file(CONFIG_FILE_NAME);
    if (ini.Load(file)) {

        //	Get the player's last-used Handle
        ini.Get_String("MultiPlayer", "Handle", "Noname", MPlayerName, sizeof(MPlayerName));

        //	Get the player's last-used Color
        MPlayerPrefColor = (PlayerColorType)ini.Get_Int("MultiPlayer", "Color", 0);

        MPlayerHouse = (HousesType)ini.Get_Int("MultiPlayer", "Side", HOUSE_GOOD);

        TrapCheckHeap = ini.Get_Int("MultiPlayer", "CheckHeap", 0);

        //	Read special recording playback values, to help find sync bugs
        TrapFrame = ini.Get_Int("SyncBug", "Frame", 0x7fffffff);

        ini.Get_String("SyncBug", "Type", "NONE", buf, 80);

        if (!stricmp(buf, "AIRCRAFT"))
            TrapObjType = RTTI_AIRCRAFT;
        else if (!stricmp(buf, "ANIM"))
            TrapObjType = RTTI_ANIM;
        else if (!stricmp(buf, "BUILDING"))
            TrapObjType = RTTI_BUILDING;
        else if (!stricmp(buf, "BULLET"))
            TrapObjType = RTTI_BULLET;
        else if (!stricmp(buf, "INFANTRY"))
            TrapObjType = RTTI_INFANTRY;
        else if (!stricmp(buf, "UNIT"))
            TrapObjType = RTTI_UNIT;
        else {
            TrapObjType = RTTI_NONE;
        }

        ini.Get_String("SyncBug", "Coord", "0", buf, 80);
        sscanf(buf, "%x", &TrapCoord);

        ini.Get_String("SyncBug", "this", "0", buf, 80);
        sscanf(buf, "%p", &TrapThis);

        ini.Get_String("SyncBug", "Cell", "0", buf, 80);
        cell = atoi(buf);
        if (cell) {
            TrapCell = &(Map[cell]);
        }
    }
#endif
}

/***********************************************************************************************
 * Write_MultiPlayer_Settings -- writes multi-player settings to conquer.ini                   *
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
void Write_MultiPlayer_Settings(void)
{
#ifndef REMASTER_BUILD
    INIClass ini;
    CDFileClass file(CONFIG_FILE_NAME);
    if (ini.Load(file)) {

        //	Save the player's last-used Handle & Color
        ini.Put_Int("MultiPlayer", "Color", (int)MPlayerPrefColor);
        ini.Put_Int("MultiPlayer", "Side", MPlayerHouse);
        ini.Put_String("MultiPlayer", "Handle", MPlayerName);

        //	Write the INI data out to a file.
        ini.Save(file);
    }
#endif
}

/***********************************************************************************************
 * Read_Scenario_Descriptions -- reads multi-player scenario #'s # descriptions                *
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
void Read_Scenario_Descriptions(void)
{
    INIClass ini;
    CCFileClass file;
    int i;
    char fname[20];

    /*------------------------------------------------------------------------
    Clear the scenario description lists
    ------------------------------------------------------------------------*/
    MPlayerScenarios.Clear();
    MPlayerFilenum.Clear();

    /*------------------------------------------------------------------------
    Loop through all possible scenario numbers; if a file is available, add
    its number to the FileNum list.
    ------------------------------------------------------------------------*/
    for (i = 0; i < 100; i++) {
        Set_Scenario_Name(Scen.ScenarioName, i, SCEN_PLAYER_MPLAYER, SCEN_DIR_EAST, SCEN_VAR_A);
        sprintf(fname, "%s.INI", Scen.ScenarioName);
        file.Set_Name(fname);

        if (file.Is_Available()) {
            MPlayerFilenum.Add(i);
        }
    }

    /*------------------------------------------------------------------------
    Now, for every file in the FileNum list, read in the INI file, and extract
    its description.
    ------------------------------------------------------------------------*/
    for (i = 0; i < MPlayerFilenum.Count(); i++) {
        /*.....................................................................
        Fetch working pointer to the INI staging buffer. Make sure that the
        buffer is cleared out before proceeding.
        .....................................................................*/
        ini.Clear();

        /*.....................................................................
        Create filename and read the file.
        .....................................................................*/
        Set_Scenario_Name(Scen.ScenarioName, MPlayerFilenum[i], SCEN_PLAYER_MPLAYER, SCEN_DIR_EAST, SCEN_VAR_A);
        sprintf(fname, "%s.INI", Scen.ScenarioName);
        file.Set_Name(fname);
        ini.Load(file);

        /*.....................................................................
        Extract description & add it to the list.
        .....................................................................*/
        ini.Get_String("Basic", "Name", "Nulls-Ville", MPlayerDescriptions[i], 40);
        MPlayerScenarios.Add(MPlayerDescriptions[i]);
    }
}

/***********************************************************************************************
 * Free_Scenario_Descriptions -- frees memory for the scenario descriptions                    *
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
 *   06/05/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
void Free_Scenario_Descriptions(void)
{
    /*------------------------------------------------------------------------
    Clear the scenario descriptions & filenames
    ------------------------------------------------------------------------*/
    MPlayerScenarios.Clear();
    MPlayerFilenum.Clear();

// PG_TO_FIX
#if (0)
    int i;

    /*------------------------------------------------------------------------
    Clear the initstring entries
    ------------------------------------------------------------------------*/
    for (i = 0; i < InitStrings.Count(); i++) {
        delete InitStrings[i];
    }
    InitStrings.Clear();

    /*------------------------------------------------------------------------
    Clear the dialing entries
    ------------------------------------------------------------------------*/
    for (i = 0; i < PhoneBook.Count(); i++) {
        delete PhoneBook[i];
    }
    PhoneBook.Clear();
#endif
}

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
    int color;
    char txt[160];
    HousesType house;
    HouseClass* ptr;

    /*------------------------------------------------------------------------
    Find the computer house that the message will be from
    ------------------------------------------------------------------------*/
    for (house = HOUSE_MULTI1; house < (HOUSE_MULTI1 + MPlayerMax); house++) {
        ptr = HouseClass::As_Pointer(house);

        if (!ptr || ptr->IsHuman || ptr->IsDefeated) {
            continue;
        }

        /*.....................................................................
        Decode this house's color
        .....................................................................*/
        color = MPlayerTColors[ptr->RemapColor];

        /*.....................................................................
        We now have a 1/4 chance of echoing one of the human players' messages
        back.
        .....................................................................*/
        if (Random_Pick(0, 3) == 2) {
            /*..................................................................
            Now we have a 1/3 chance of garbling the human message.
            ..................................................................*/
            if (Random_Pick(0, 2) == 1) {
                Garble_Message(LastMessage);
            }

            /*..................................................................
            Only add the message if there is one to add.
            ..................................................................*/
            if (strlen(LastMessage)) {
                sprintf(txt, "%s %s", Text_String(TXT_FROM_COMPUTER), LastMessage);
                Messages.Add_Message(txt, color, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600, 0, 0);
            }
        } else {
            sprintf(txt, "%s %s", Text_String(TXT_FROM_COMPUTER), Text_String(TXT_COMP_MSG1 + Random_Pick(0, 12)));
            Messages.Add_Message(txt, color, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600, 0, 0);
        }

        return;
    }
}

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

    /*------------------------------------------------------------------------
    Pull off any trailing punctuation
    ------------------------------------------------------------------------*/
    p = buf + strlen(buf) - 1;
    while (true) {
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

    /*------------------------------------------------------------------------
    Copy the original buffer
    ------------------------------------------------------------------------*/
    strcpy(txt, buf);

    /*------------------------------------------------------------------------
    Split it up into words
    ------------------------------------------------------------------------*/
    p = strtok(txt, " ");
    numwords = 0;
    while (p) {
        words[numwords] = p;
        numwords++;
        p = strtok(NULL, " ");
    }

    /*------------------------------------------------------------------------
    Now randomly put the words back.  Don't use the real random-number
    generator, since different machines will have different LastMessage's,
    and will go out of sync.
    ------------------------------------------------------------------------*/
    buf[0] = 0;
    for (i = 0; i < numwords; i++) {
        j = Sim_Random_Pick(0, numwords);
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
}

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
int Surrender_Dialog(void)
{
    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
    /*........................................................................
    Dialog & button dimensions
    ........................................................................*/
    int d_dialog_w = 170 * factor;                      // dialog width
    int d_dialog_h = 53 * factor;                       // dialog height
    int d_dialog_x = ((320 * factor - d_dialog_w) / 2); // centered x-coord
    int d_dialog_y = ((200 * factor - d_dialog_h) / 2); // centered y-coord
    int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);    // coord of x-center

    int d_txt6_h = 6 * factor + 1; // ht of 6-pt text
    int d_margin = 5 * factor;     // margin width/height
    int d_topmargin = 20 * factor; // top margin

    int d_ok_w = 45 * factor;                                 // ok width
    int d_ok_h = 9 * factor;                                  // ok height
    int d_ok_x = d_dialog_cx - d_ok_w - 5 * factor;           // ok x
    int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin; // ok y

    int d_cancel_w = 45 * factor;                                     // cancel width
    int d_cancel_h = 9 * factor;                                      // cancel height
    int d_cancel_x = d_dialog_cx + 5 * factor;                        // cancel x
    int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin; // cancel y

    /*........................................................................
    Button enumerations
    ........................................................................*/
    enum
    {
        BUTTON_OK = 100,
        BUTTON_CANCEL,
    };

    /*........................................................................
    Redraw values: in order from "top" to "bottom" layer of the dialog
    ........................................................................*/
    typedef enum
    {
        REDRAW_NONE = 0,
        REDRAW_BUTTONS,
        REDRAW_BACKGROUND,
        REDRAW_ALL = REDRAW_BACKGROUND
    } RedrawType;

    /*........................................................................
    Dialog variables
    ........................................................................*/
    RedrawType display; // requested redraw level
    bool process;       // loop while true
    KeyNumType input;
    int retcode;

    /*........................................................................
    Buttons
    ........................................................................*/
    ControlClass* commands = NULL; // the button list

    TextButtonClass okbtn(
        BUTTON_OK, TXT_OK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ok_x, d_ok_y, d_ok_w, d_ok_h);

    TextButtonClass cancelbtn(BUTTON_CANCEL,
                              TXT_CANCEL,
                              TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                              d_cancel_x,
                              d_cancel_y,
                              d_cancel_w,
                              d_cancel_h);

    /*
    ------------------------------- Initialize -------------------------------
    */
    Set_Logic_Page(SeenBuff);

    /*
    ......................... Create the button list .........................
    */
    commands = &okbtn;
    cancelbtn.Add_Tail(*commands);

    /*
    -------------------------- Main Processing Loop --------------------------
    */
    display = REDRAW_ALL;
    process = true;
    while (process) {

        /*
        ** If we have just received input focus again after running in the background then
        ** we need to redraw.
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            display = REDRAW_ALL;
        }

        /*
        ........................ Invoke game callback .........................
        */
        if (Main_Loop()) {
            retcode = 0;
            process = false;
        }

        /*
        ...................... Refresh display if needed ......................
        */
        if (display) {

            /*
            ...................... Display the dialog box ......................
            */
            Hide_Mouse();
            if (display >= REDRAW_BACKGROUND) {
                Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
                Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

                /*
                ....................... Draw the captions .......................
                */
                Fancy_Text_Print(Text_String(TXT_SURRENDER),
                                 d_dialog_cx,
                                 d_dialog_y + d_topmargin,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
            }

            /*
            ........................ Redraw the buttons ........................
            */
            if (display >= REDRAW_BUTTONS) {
                commands->Flag_List_To_Redraw();
            }
            Show_Mouse();
            display = REDRAW_NONE;
        }

        /*
        ........................... Get user input ............................
        */
        input = commands->Input();

        /*
        ............................ Process input ............................
        */
        switch (input) {
        case (KN_RETURN):
        case (BUTTON_OK | KN_BUTTON):
            retcode = 1;
            process = false;
            break;

        case (KN_ESC):
        case (BUTTON_CANCEL | KN_BUTTON):
            retcode = 0;
            process = false;
            break;

        default:
            break;
        }
    }

    /*
    --------------------------- Redraw the display ---------------------------
    */
    HiddenPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();

    return (retcode);
}