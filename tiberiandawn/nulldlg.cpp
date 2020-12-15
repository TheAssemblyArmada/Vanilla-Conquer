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

/* $Header:   F:\projects\c&c\vcs\code\nulldlg.cpv   1.9   16 Oct 1995 16:52:12   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLDLG.CPP                              *
 *                                                                         *
 *                   Programmer : Bill R. Randolph                         *
 *                                                                         *
 *                   Start Date : 04/29/95                                 *
 *                                                                         *
 *                  Last Update : April 29, 1995 [BRR]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Com_Scenario_Dialog -- Skirmish game scenario selection dialog        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "function.h"
#include "framelimit.h"
#include <time.h>

/***********************************************************************************************
 * Com_Scenario_Dialog -- Serial game scenario selection dialog                                *
 *                                                                                             *
 *                                                                                             *
 *    ┌────────────────────────────────────────────────────────────┐                           *
 *    │                        Serial Game                         │                           *
 *    │                                                            │                           *
 *    │     Your Name: __________          House: [GDI] [NOD]      │                           *
 *    │       Credits: ______      Desired Color: [ ][ ][ ][ ]     │                           *
 *    │      Opponent: Name                                        │                           *
 *    │                                                            │                           *
 *    │                         Scenario                           │                           *
 *    │                  ┌──────────────────┬─┐                    │                           *
 *    │                  │ Hell's Kitchen   │ │                    │                           *
 *    │                  │ Heaven's Gate    ├─┤                    │                           *
 *    │                  │      ...         │ │                    │                           *
 *    │                  │                  ├─┤                    │                           *
 *    │                  │                  │ │                    │                           *
 *    │                  └──────────────────┴─┘                    │                           *
 *    │                 [  Bases   ] [ Crates     ]                │                           *
 *    │                 [ Tiberium ] [ AI Players ]                │                           *
 *    │                                                            │                           *
 *    │                      [OK]    [Cancel]                      │                           *
 *    │   ┌────────────────────────────────────────────────────┐   │                           *
 *    │   │                                                    │   │                           *
 *    │   │                                                    │   │                           *
 *    │   └────────────────────────────────────────────────────┘   │                           *
 *    │                       [Send Message]                       │                           *
 *    └────────────────────────────────────────────────────────────┘                           *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      true = success, false = cancel                                                         *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      MPlayerName & MPlayerGameName must contain this player's name.                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
#define TXT_HOST_INTERNET_GAME 4567 + 1
#define TXT_JOIN_INTERNET_GAME 4567 + 2
int Com_Scenario_Dialog(void)
{
    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
    /*........................................................................
    Dialog & button dimensions
    ........................................................................*/
    int d_dialog_w = 290 * factor;                      // dialog width
    int d_dialog_h = 190 * factor;                      // dialog height
    int d_dialog_x = ((320 * factor - d_dialog_w) / 2); // dialog x-coord
    int d_dialog_y = ((200 * factor - d_dialog_h) / 2); // dialog y-coord
    int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);    // center x-coord

    int d_txt6_h = 6 * factor + 1; // ht of 6-pt text
    int d_margin1 = 5 * factor;    // margin width/height
    int d_margin2 = 2 * factor;    // margin width/height

    int d_name_w = 70 * factor;
    int d_name_h = 9 * factor;
    int d_name_x = d_dialog_x + 108 * factor;
    int d_name_y = d_dialog_y + d_margin1 + d_txt6_h + d_txt6_h + d_margin1;

    int d_credits_w = ((CREDITSBUF_MAX - 1) * 6 * factor) + 3 * factor;
    int d_credits_h = 9 * factor;
    int d_credits_x = d_name_x;
    int d_credits_y = d_name_y + d_name_h + d_margin2;

    int d_gdi_w = 30 * factor;
    int d_gdi_h = 9 * factor;
    int d_gdi_x = d_dialog_cx + (d_dialog_w / 4);
    int d_gdi_y = d_dialog_y + d_margin1 + d_txt6_h + d_txt6_h + d_margin1;

    int d_nod_w = 30 * factor;
    int d_nod_h = 9 * factor;
    int d_nod_x = d_gdi_x + d_gdi_w + (d_margin1 / 2);
    int d_nod_y = d_gdi_y;

    int d_color_w = 10 * factor;
    int d_color_h = 9 * factor;
    int d_color_y = d_gdi_y + d_gdi_h + d_margin2;

    int d_opponent_x = d_name_x;
    int d_opponent_y = d_color_y + d_color_h + d_margin2;

    int d_scenariolist_w = 182 * factor;
    int d_scenariolist_h = 27 * factor;
    int d_scenariolist_x = d_dialog_cx - (d_scenariolist_w / 2);
    int d_scenariolist_y = d_opponent_y + d_txt6_h + 3 * factor + d_txt6_h;

    // d_count_x is calculated below after other enums
    int d_count_w = 25 * factor;
    int d_count_h = 7 * factor;
    int d_count_y = d_scenariolist_y + d_scenariolist_h + d_margin2;

    // d_level_x is calculated below after other enums
    int d_level_w = 25 * factor;
    int d_level_h = 7 * factor;
    int d_level_y = d_count_y;

#if (GERMAN | FRENCH)
    int d_bases_w = 120 * factor; // BGA:100;
#else
    int d_bases_w = 110 * factor;
#endif
    int d_bases_h = 9 * factor;
    int d_bases_x = d_dialog_cx - d_bases_w - d_margin2;
    int d_bases_y = d_count_y + d_count_h + d_margin2;

#if (GERMAN | FRENCH)
    int d_goodies_w = 120 * factor;
#else
    int d_goodies_w = 110 * factor;
#endif
    int d_goodies_h = 9 * factor;
    int d_goodies_x = d_dialog_cx + d_margin2;
    int d_goodies_y = d_bases_y;

    int d_count_x = d_dialog_cx - d_count_w - ((2 * 6 * factor) + 3 * factor)
                    - ((d_bases_w - ((13 * 6 * factor) + 3 * factor + d_count_w)) / 2) - d_margin2;

    int d_level_x = d_dialog_cx + (11 * 6 * factor) + ((d_goodies_w - ((13 * 6 * factor) + 3 * factor + d_level_w)) / 2)
                    + d_margin2;

#if (GERMAN | FRENCH)
    int d_tiberium_w = 120 * factor;
#else
    int d_tiberium_w = 110 * factor;
#endif
    int d_tiberium_h = 9 * factor;
    int d_tiberium_x = d_dialog_cx - d_bases_w - d_margin2;
    int d_tiberium_y = d_bases_y + d_bases_h + d_margin2;

#if (GERMAN | FRENCH)
    int d_ghosts_w = 120 * factor;
#else
    int d_ghosts_w = 110 * factor;
#endif
    int d_ghosts_h = 9 * factor;
    int d_ghosts_x = d_dialog_cx + d_margin2;
    int d_ghosts_y = d_tiberium_y;

    int d_ok_w = 45 * factor;
    int d_ok_h = 9 * factor;
    int d_ok_x = d_tiberium_x + (d_tiberium_w / 2) - (d_ok_w / 2);
    int d_ok_y = d_tiberium_y + d_tiberium_h + d_margin1;

    int d_cancel_w = 45 * factor;
    int d_cancel_h = 9 * factor;
    int d_cancel_x = d_ghosts_x + (d_ghosts_w / 2) - (d_cancel_w / 2);
    int d_cancel_y = d_tiberium_y + d_tiberium_h + d_margin1;

    int d_message_w = d_dialog_w - (d_margin1 * 2);
    int d_message_h = 34 * factor;
    int d_message_x = d_dialog_x + d_margin1;
    int d_message_y = d_cancel_y + d_cancel_h + d_margin1;

    int d_send_w = 80 * factor;
    int d_send_h = 9 * factor;
    int d_send_x = d_dialog_cx - (d_send_w / 2);
    int d_send_y = d_message_y + d_message_h + d_margin2;

    /*........................................................................
    Button Enumerations
    ........................................................................*/
    enum
    {
        BUTTON_NAME = 100,
        BUTTON_GDI,
        BUTTON_NOD,
        BUTTON_CREDITS,
        BUTTON_SCENARIOLIST,
        BUTTON_COUNT,
        BUTTON_LEVEL,
        BUTTON_BASES,
        BUTTON_TIBERIUM,
        BUTTON_GOODIES,
        BUTTON_GHOSTS,
        BUTTON_OK,
        BUTTON_CANCEL,
        BUTTON_SEND,
    };

    /*........................................................................
    Redraw values: in order from "top" to "bottom" layer of the dialog
    ........................................................................*/
    typedef enum
    {
        REDRAW_NONE = 0,
        REDRAW_MESSAGE,
        REDRAW_COLORS,
        REDRAW_BUTTONS,
        REDRAW_BACKGROUND,
        REDRAW_ALL = REDRAW_BACKGROUND
    } RedrawType;

    /*........................................................................
    Dialog variables
    ........................................................................*/
    RedrawType display = REDRAW_ALL; // redraw level
    bool process = true;             // process while true
    KeyNumType input;

    char namebuf[MPLAYER_NAME_MAX] = {0}; // buffer for player's name
    char credbuf[CREDITSBUF_MAX];         // for credit edit box
    int old_cred;                         // old value in credits buffer
    int transmit;                         // 1 = re-transmit new game options
    int cbox_x[] = {d_gdi_x,
                    d_gdi_x + d_color_w,
                    d_gdi_x + (d_color_w * 2),
                    d_gdi_x + (d_color_w * 3),
                    d_gdi_x + (d_color_w * 4),
                    d_gdi_x + (d_color_w * 5)};
    int parms_received = 0; // 1 = game options received
    int changed = 0;        // 1 = user has changed an option

    int rc;
    int recsignedoff = false;
    int i;
    char txt[80];
    unsigned long timingtime;
    unsigned long lastmsgtime;
    unsigned long lastredrawtime;
    unsigned long transmittime = 0;
    unsigned long theirresponsetime;
    static bool first_time = true;
    bool oppscorescreen = false;
    bool gameoptions = GameToPlay == GAME_SKIRMISH;
    unsigned long msg_timeout = 1200; // init to 20 seconds

    bool ready_to_go = false;
    CountDownTimerClass ready_time;

    void const* up_button;
    void const* down_button;

    if (InMainLoop) {
        up_button = Hires_Retrieve("BTN-UP.SHP");
        down_button = Hires_Retrieve("BTN-DN.SHP");
    } else {
        up_button = Hires_Retrieve("BTN-UP2.SHP");
        down_button = Hires_Retrieve("BTN-DN2.SHP");
    }

    /*........................................................................
    Buttons
    ........................................................................*/
    GadgetClass* commands; // button list

    EditClass name_edt(BUTTON_NAME,
                       namebuf,
                       MPLAYER_NAME_MAX,
                       TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                       d_name_x,
                       d_name_y,
                       d_name_w,
                       d_name_h,
                       EditClass::ALPHANUMERIC);

    TextButtonClass gdibtn(BUTTON_GDI,
                           TXT_G_D_I,
                           TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                           d_gdi_x,
                           d_gdi_y,
                           d_gdi_w,
                           d_gdi_h);

    TextButtonClass nodbtn(BUTTON_NOD,
                           TXT_N_O_D,
                           TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                           d_nod_x,
                           d_nod_y,
                           d_nod_w,
                           d_nod_h);

    EditClass credit_edt(BUTTON_CREDITS,
                         credbuf,
                         CREDITSBUF_MAX,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                         d_credits_x,
                         d_credits_y,
                         d_credits_w,
                         d_credits_h,
                         EditClass::ALPHANUMERIC);

    ListClass scenariolist(BUTTON_SCENARIOLIST,
                           d_scenariolist_x,
                           d_scenariolist_y,
                           d_scenariolist_w,
                           d_scenariolist_h,
                           TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                           up_button,
                           down_button);

    GaugeClass countgauge(BUTTON_COUNT, d_count_x, d_count_y, d_count_w, d_count_h);

    GaugeClass levelgauge(BUTTON_LEVEL, d_level_x, d_level_y, d_level_w, d_level_h);

    TextButtonClass basesbtn(BUTTON_BASES,
                             TXT_BASES_OFF,
                             TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                             d_bases_x,
                             d_bases_y,
                             d_bases_w,
                             d_bases_h);

    TextButtonClass tiberiumbtn(BUTTON_TIBERIUM,
                                TXT_TIBERIUM_OFF,
                                TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                                d_tiberium_x,
                                d_tiberium_y,
                                d_tiberium_w,
                                d_tiberium_h);

    TextButtonClass goodiesbtn(BUTTON_GOODIES,
                               TXT_CRATES_OFF,
                               TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                               d_goodies_x,
                               d_goodies_y,
                               d_goodies_w,
                               d_goodies_h);

    TextButtonClass ghostsbtn(BUTTON_GHOSTS,
                              TXT_AI_PLAYERS_OFF,
                              TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                              d_ghosts_x,
                              d_ghosts_y,
                              d_ghosts_w,
                              d_ghosts_h);

    TextButtonClass okbtn(
        BUTTON_OK, TXT_OK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ok_x, d_ok_y, d_ok_w, d_ok_h);

    TextButtonClass cancelbtn(BUTTON_CANCEL,
                              TXT_CANCEL,
                              TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                              //#if (GERMAN | FRENCH)
                              //        d_cancel_x, d_cancel_y);
                              //#else
                              d_cancel_x,
                              d_cancel_y,
                              d_cancel_w,
                              d_cancel_h);

    /*
    ------------------------- Build the button list --------------------------
    */
    commands = &name_edt;
    gdibtn.Add_Tail(*commands);
    nodbtn.Add_Tail(*commands);
    credit_edt.Add_Tail(*commands);
    scenariolist.Add_Tail(*commands);
    countgauge.Add_Tail(*commands);
    levelgauge.Add_Tail(*commands);
    basesbtn.Add_Tail(*commands);
    tiberiumbtn.Add_Tail(*commands);
    goodiesbtn.Add_Tail(*commands);
    ghostsbtn.Add_Tail(*commands);
    okbtn.Add_Tail(*commands);
    cancelbtn.Add_Tail(*commands);

    /*
    ----------------------------- Various Inits ------------------------------
    */
    /*........................................................................
    Init player name & house
    ........................................................................*/
    MPlayerColorIdx = MPlayerPrefColor; // init my preferred color
    strcpy(namebuf, MPlayerName);       // set my name
    name_edt.Set_Text(namebuf, MPLAYER_NAME_MAX);
    name_edt.Set_Color(MPlayerTColors[MPlayerColorIdx]);

    if (MPlayerHouse == HOUSE_GOOD) {
        gdibtn.Turn_On();
    } else {
        nodbtn.Turn_On();
    }

    /*........................................................................
    Init scenario values, only the first time through
    ........................................................................*/
    if (first_time) {
        MPlayerCredits = 3000; // init credits & credit buffer
        MPlayerBases = 1;      // init scenario parameters
        MPlayerTiberium = 0;
        MPlayerGoodies = 0;
        MPlayerGhosts = 0;
        Special.IsCaptureTheFlag = 0;
        MPlayerUnitCount = (MPlayerCountMax[MPlayerBases] + MPlayerCountMin[MPlayerBases]) / 2;
        first_time = false;
    }

    /*........................................................................
    Init button states
    ........................................................................*/
    if (MPlayerBases) {
        basesbtn.Turn_On();
        basesbtn.Set_Text(TXT_BASES_ON);
    }
    if (MPlayerTiberium) {
        tiberiumbtn.Turn_On();
        tiberiumbtn.Set_Text(TXT_TIBERIUM_ON);
    }
    if (MPlayerGoodies) {
        goodiesbtn.Turn_On();
        goodiesbtn.Set_Text(TXT_CRATES_ON);
    }
    if (MPlayerGhosts) {
        ghostsbtn.Turn_On();
        ghostsbtn.Set_Text(TXT_AI_PLAYERS_ON);
    }
    if (Special.IsCaptureTheFlag) {
        MPlayerGhosts = 0;
        ghostsbtn.Turn_On();
        ghostsbtn.Set_Text(TXT_CAPTURE_THE_FLAG);
    }

    sprintf(credbuf, "%d", MPlayerCredits);
    credit_edt.Set_Text(credbuf, CREDITSBUF_MAX);
    old_cred = MPlayerCredits;

    levelgauge.Set_Maximum(MPLAYER_BUILD_LEVEL_MAX - 1);
    levelgauge.Set_Value(BuildLevel - 1);

    countgauge.Set_Maximum(MPlayerCountMax[MPlayerBases] - MPlayerCountMin[MPlayerBases]);
    countgauge.Set_Value(MPlayerUnitCount - MPlayerCountMin[MPlayerBases]);

    /*........................................................................
    Init other scenario parameters
    ........................................................................*/
    Special.IsTGrowth = MPlayerTiberium;
    Special.IsTSpread = MPlayerTiberium;
    transmit = 1;

    /*........................................................................
    Init scenario description list box
    ........................................................................*/
    for (i = 0; i < MPlayerScenarios.Count(); i++) {
        scenariolist.Add_Item(strupr(MPlayerScenarios[i]));
    }
    ScenarioIdx = 0; // 1st scenario is selected

    /*........................................................................
    Init random-number generator, & create a seed to be used for all random
    numbers from here on out
    ........................................................................*/
    srand((unsigned)time(NULL));
    Seed = rand();

    Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
    Blit_Hid_Page_To_Seen_Buff();
    Set_Palette(Palette);

    theirresponsetime = 10000; // initialize to an invalid value
    timingtime = lastmsgtime = lastredrawtime = WinTickCount.Time();
    while (Get_Mouse_State() > 0)
        Show_Mouse();

    while (process) {

        /*
        ** If we have just received input focus again after running in the background then
        ** we need to redraw.
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = FALSE;
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
            /*
            .................. Redraw backgound & dialog box ...................
            */
            if (display >= REDRAW_BACKGROUND) {
                Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

                // init font variables

                Fancy_Text_Print(
                    TXT_NONE, 0, 0, TBLACK, TBLACK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                /*...............................................................
                Dialog & Field labels
                ...............................................................*/
#ifdef FORCE_WINSOCK
                if (Winsock.Get_Connected()) {
                    Draw_Caption(TXT_HOST_INTERNET_GAME, d_dialog_x, d_dialog_y, d_dialog_w);
                } else {
                    Draw_Caption(TXT_HOST_SERIAL_GAME, d_dialog_x, d_dialog_y, d_dialog_w);
                }
#else
                Draw_Caption(TXT_HOST_SERIAL_GAME, d_dialog_x, d_dialog_y, d_dialog_w);
#endif // FORCE_WINSOCK

                Fancy_Text_Print(TXT_YOUR_NAME,
                                 d_name_x - 5 * factor,
                                 d_name_y + 1 * factor,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                Fancy_Text_Print(TXT_SIDE_COLON,
                                 d_gdi_x - 5 * factor,
                                 d_gdi_y + 1 * factor,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                Fancy_Text_Print(TXT_START_CREDITS_COLON,
                                 d_credits_x - 5 * factor,
                                 d_credits_y + 1 * factor,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                Fancy_Text_Print(TXT_COLOR_COLON,
                                 cbox_x[0] - 5 * factor,
                                 d_color_y + 1 * factor,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                Fancy_Text_Print(TXT_SCENARIOS,
                                 d_scenariolist_x + (d_scenariolist_w / 2),
                                 d_scenariolist_y - d_txt6_h,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                Fancy_Text_Print(TXT_COUNT,
                                 d_count_x - 3 * factor,
                                 d_count_y,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_RIGHT);

                Fancy_Text_Print(TXT_LEVEL,
                                 d_level_x - 3 * factor,
                                 d_level_y,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_RIGHT);
            }

            /*..................................................................
            Draw the color boxes
            ..................................................................*/
            if (display >= REDRAW_COLORS) {
                for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
                    LogicPage->Fill_Rect(cbox_x[i] + 1 * factor,
                                         d_color_y + 1 * factor,
                                         cbox_x[i] + 1 * factor + d_color_w - 2 * factor,
                                         d_color_y + 1 * factor + d_color_h - 2 * factor,
                                         MPlayerGColors[i]);

                    if (i == MPlayerColorIdx) {
                        Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_GREEN_DOWN, false);
                    } else {
                        Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_GREEN_RAISED, false);
                    }
                }
            }

            /*..................................................................
            Draw the message:
            - Erase an old message first
            ..................................................................*/
            if (display >= REDRAW_MESSAGE) {
                sprintf(txt, "%d ", MPlayerUnitCount);
                Fancy_Text_Print(txt,
                                 d_count_x + d_count_w + 3 * factor,
                                 d_count_y,
                                 CC_GREEN,
                                 BLACK,
                                 TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
                    sprintf(txt, "%d ", BuildLevel);
                } else {
                    sprintf(txt, "**");
                }
                Fancy_Text_Print(txt,
                                 d_level_x + d_level_w + 3 * factor,
                                 d_level_y,
                                 CC_GREEN,
                                 BLACK,
                                 TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
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
        ---------------------------- Process input ----------------------------
        */
        switch (input) {
        /*------------------------------------------------------------------
        User clicks on a color button
        ------------------------------------------------------------------*/
        case KN_LMOUSE:
            if (Keyboard->MouseQX > cbox_x[0] && Keyboard->MouseQX < (cbox_x[MAX_MPLAYER_COLORS - 1] + d_color_w)
                && Keyboard->MouseQY > d_color_y && Keyboard->MouseQY < (d_color_y + d_color_h)) {
                if (!ready_to_go) {
                    MPlayerPrefColor = (Keyboard->MouseQX - cbox_x[0]) / d_color_w;
                    MPlayerColorIdx = MPlayerPrefColor;
                    display = REDRAW_COLORS;

                    name_edt.Set_Color(MPlayerTColors[MPlayerColorIdx]);
                    name_edt.Flag_To_Redraw();
                    MPlayerCredits = atoi(credbuf);
                    strcpy(MPlayerName, namebuf);
                    transmit = 1;
                    changed = 1;
                }
            }
            break;

        /*------------------------------------------------------------------
        User edits the name field; retransmit new game options
        ------------------------------------------------------------------*/
        case (BUTTON_NAME | KN_BUTTON):
            if (!ready_to_go) {
                credit_edt.Clear_Focus();
                credit_edt.Flag_To_Redraw();
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
                changed = 1;
            }
            break;

        /*------------------------------------------------------------------
        House Buttons: set the player's desired House
        ------------------------------------------------------------------*/
        case (BUTTON_GDI | KN_BUTTON):
            if (!ready_to_go) {
                MPlayerHouse = HOUSE_GOOD;
                gdibtn.Turn_On();
                nodbtn.Turn_Off();
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
            }
            break;

        case (BUTTON_NOD | KN_BUTTON):
            if (!ready_to_go) {
                MPlayerHouse = HOUSE_BAD;
                gdibtn.Turn_Off();
                nodbtn.Turn_On();
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        User edits the credits value; retransmit new game options
        ------------------------------------------------------------------*/
        case (BUTTON_CREDITS | KN_BUTTON):
            if (!ready_to_go) {
                name_edt.Clear_Focus();
                name_edt.Flag_To_Redraw();
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        New Scenario selected.
        ------------------------------------------------------------------*/
        case (BUTTON_SCENARIOLIST | KN_BUTTON):
            if (scenariolist.Current_Index() != ScenarioIdx && !ready_to_go) {
                ScenarioIdx = scenariolist.Current_Index();
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        User adjusts max # units
        ------------------------------------------------------------------*/
        case (BUTTON_COUNT | KN_BUTTON):
            if (!ready_to_go) {
                MPlayerUnitCount = countgauge.Get_Value() + MPlayerCountMin[MPlayerBases];
                if (display < REDRAW_MESSAGE)
                    display = REDRAW_MESSAGE;
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        User adjusts build level
        ------------------------------------------------------------------*/
        case (BUTTON_LEVEL | KN_BUTTON):
            if (!ready_to_go) {
                BuildLevel = levelgauge.Get_Value() + 1;
                if (BuildLevel > MPLAYER_BUILD_LEVEL_MAX) // if it's pegged, max it out
                    BuildLevel = MPLAYER_BUILD_LEVEL_MAX;
                if (display < REDRAW_MESSAGE)
                    display = REDRAW_MESSAGE;
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        Toggle bases
        ------------------------------------------------------------------*/
        case (BUTTON_BASES | KN_BUTTON):
            if (!ready_to_go) {
                if (MPlayerBases) {
                    MPlayerBases = 0;
                    basesbtn.Turn_Off();
                    basesbtn.Set_Text(TXT_BASES_OFF);
                    MPlayerUnitCount = Fixed_To_Cardinal(MPlayerCountMax[0] - MPlayerCountMin[0],
                                                         Cardinal_To_Fixed(MPlayerCountMax[1] - MPlayerCountMin[1],
                                                                           MPlayerUnitCount - MPlayerCountMin[1]))
                                       + MPlayerCountMin[0];
                } else {
                    MPlayerBases = 1;
                    basesbtn.Turn_On();
                    basesbtn.Set_Text(TXT_BASES_ON);
                    MPlayerUnitCount = Fixed_To_Cardinal(MPlayerCountMax[1] - MPlayerCountMin[1],
                                                         Cardinal_To_Fixed(MPlayerCountMax[0] - MPlayerCountMin[0],
                                                                           MPlayerUnitCount - MPlayerCountMin[0]))
                                       + MPlayerCountMin[1];
                }
                MPlayerCredits = atoi(credbuf);
                countgauge.Set_Maximum(MPlayerCountMax[MPlayerBases] - MPlayerCountMin[MPlayerBases]);
                countgauge.Set_Value(MPlayerUnitCount - MPlayerCountMin[MPlayerBases]);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
                display = REDRAW_ALL;
            }
            break;

        /*------------------------------------------------------------------
        Toggle tiberium
        ------------------------------------------------------------------*/
        case (BUTTON_TIBERIUM | KN_BUTTON):
            if (!ready_to_go) {
                if (MPlayerTiberium) {
                    MPlayerTiberium = 0;
                    Special.IsTGrowth = 0;
                    Special.IsTSpread = 0;
                    tiberiumbtn.Turn_Off();
                    tiberiumbtn.Set_Text(TXT_TIBERIUM_OFF);
                } else {
                    MPlayerTiberium = 1;
                    Special.IsTGrowth = 1;
                    Special.IsTSpread = 1;
                    tiberiumbtn.Turn_On();
                    tiberiumbtn.Set_Text(TXT_TIBERIUM_ON);
                }
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        Toggle goodies
        ------------------------------------------------------------------*/
        case (BUTTON_GOODIES | KN_BUTTON):
            if (!ready_to_go) {
                if (MPlayerGoodies) {
                    MPlayerGoodies = 0;
                    goodiesbtn.Turn_Off();
                    goodiesbtn.Set_Text(TXT_CRATES_OFF);
                } else {
                    MPlayerGoodies = 1;
                    goodiesbtn.Turn_On();
                    goodiesbtn.Set_Text(TXT_CRATES_ON);
                }
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        Toggle ghosts
        ------------------------------------------------------------------*/
        case (BUTTON_GHOSTS | KN_BUTTON):
            if (!ready_to_go) {
                if (!MPlayerGhosts && !Special.IsCaptureTheFlag) { // ghosts OFF => ghosts ON
                    MPlayerGhosts = 1;
                    Special.IsCaptureTheFlag = 0;
                    ghostsbtn.Turn_On();
                    ghostsbtn.Set_Text(TXT_AI_PLAYERS_ON);
                } else if (MPlayerGhosts) { // ghosts ON => capture-flag
                    MPlayerGhosts = 0;
                    Special.IsCaptureTheFlag = 1;
                    ghostsbtn.Turn_On();
                    ghostsbtn.Set_Text(TXT_CAPTURE_THE_FLAG);
                } else if (Special.IsCaptureTheFlag) { // capture-flag => AI OFF
                    MPlayerGhosts = 0;
                    Special.IsCaptureTheFlag = 0;
                    ghostsbtn.Turn_Off();
                    ghostsbtn.Set_Text(TXT_AI_PLAYERS_OFF);
                }
                MPlayerCredits = atoi(credbuf);
                strcpy(MPlayerName, namebuf);
                transmit = 1;
            }
            break;

        /*------------------------------------------------------------------
        OK: exit loop with true status
        ------------------------------------------------------------------*/
        case (BUTTON_OK | KN_BUTTON):
            if (!ready_to_go) {
                //
                // make sure we got a game options packet from the other player
                //
                if (gameoptions) {
                    rc = true;
                    process = false;

                    // force transmitting of game options packet one last time

                    ready_to_go = true;

                    transmit = 1;
                    transmittime = 0;

                } else {
                    WWMessageBox().Process(TXT_ONLY_ONE, TXT_OOPS, NULL);
                    display = REDRAW_ALL;
                }
            }
            break;

        /*------------------------------------------------------------------
        CANCEL: send a SIGN_OFF, bail out with error code
        ------------------------------------------------------------------*/
        case (KN_ESC):
            if (!ready_to_go) {
                if (Messages.Get_Edit_Buf() != NULL) {
                    Messages.Input(input);
                    if (display < REDRAW_MESSAGE)
                        display = REDRAW_MESSAGE;
                    break;
                }
            }
        case (BUTTON_CANCEL | KN_BUTTON):
            if (!ready_to_go) {
                process = false;
                rc = false;
            }
            break;

        /*------------------------------------------------------------------
        Default: manage the inter-player messages
        ------------------------------------------------------------------*/
        default:
            break;

        } /* end of input processing */

        Frame_Limiter();
    } /* end of while */

    /*------------------------------------------------------------------------
    Sort player ID's, so we can execute commands in the same order on both
    machines.
    ------------------------------------------------------------------------*/
    if (rc) {
        /*.....................................................................
        Set the number of players in this game, and my ID
        .....................................................................*/
        MPlayerCount = 1;
        MPlayerLocalID = Build_MPlayerID(MPlayerColorIdx, MPlayerHouse);

        /*.....................................................................
        Store every player's ID in the MPlayerID[] array.  This array will
        determine the order of event execution, so the ID's must be stored
        in the same order on all systems.
        .....................................................................*/
        MPlayerID[0] = MPlayerLocalID;
        strcpy(MPlayerNames[0], MPlayerName);

        /*.....................................................................
        Get the scenario filename
        .....................................................................*/
        Scenario = MPlayerFilenum[ScenarioIdx];

        // TODO Add difficulty slider to menu.
    }

    /*------------------------------------------------------------------------
    Clear all lists
    ------------------------------------------------------------------------*/
    while (scenariolist.Count()) {
        scenariolist.Remove_Item(scenariolist.Get_Item(0));
    }

    /*------------------------------------------------------------------------
    Restore screen
    ------------------------------------------------------------------------*/
    Hide_Mouse();
    Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
    Blit_Hid_Page_To_Seen_Buff();
    Show_Mouse();

    /*------------------------------------------------------------------------
    Save any changes made to our options
    ------------------------------------------------------------------------*/
    if (changed) {
        Write_MultiPlayer_Settings();
    }

    return (rc);

} /* end of Com_Scenario_Dialog */
