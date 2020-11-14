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

/* $Header: /counterstrike/NULLDLG.CPP 14    3/17/97 1:05a Steve_tall $ */
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
 *                  Last Update : Jan. 21, 1997 [V.Grippi]                 *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Com_Scenario_Dialog -- Serial game scenario selection dialog		   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "framelimit.h"

// PG
GameType Select_Serial_Dialog(void)
{
    return GAME_NORMAL;
}

#ifdef FIXIT_RANDOM_GAME
#include <time.h>
#endif

#ifdef FIXIT_CSII //	checked - ajw 9/28/98
extern bool Is_Mission_126x126(char* file_name);
extern bool Is_Mission_Counterstrike(char* file_name);
extern bool Is_Mission_Aftermath(char* file_name);
#endif

extern char const* EngMisStr[];

#ifndef REMASTER_BUILD
/***********************************************************************************************
 * Com_Scenario_Dialog -- Serial game scenario selection dialog										  *
 *                                                                         						  *
 *                                                                         						  *
 * INPUT:                                                                  						  *
 *		none. *
 *                                                                         						  *
 * OUTPUT:                                                                 						  *
 *		true = success, false = cancel																			  *
 *                                                                         						  *
 * WARNINGS:                                                               						  *
 *		none. *
 *                                                                         						  *
 * HISTORY:                                                                						  *
 *   02/14/1995 BR : Created.
 *   01/21/97 V.Grippi added check for CS before sending scenario file *
 *=============================================================================================*/
int Com_Scenario_Dialog(bool skirmish)
{
    /*........................................................................
    Dialog & button dimensions
    ........................................................................*/
    int d_dialog_w = 320 * RESFACTOR;                      // dialog width
    int d_dialog_h = 200 * RESFACTOR;                      // dialog height
    int d_dialog_x = ((320 * RESFACTOR - d_dialog_w) / 2); // dialog x-coord
    int d_dialog_y = ((200 * RESFACTOR - d_dialog_h) / 2); // dialog y-coord
    int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);       // center x-coord

    int d_txt6_h = 6 * RESFACTOR + 1; // ht of 6-pt text
    int d_margin1 = 5 * RESFACTOR;    // margin width/height
    int d_margin2 = 7 * RESFACTOR;    // margin width/height

    int d_name_w = 70 * RESFACTOR;
    int d_name_h = 9 * RESFACTOR;
    int d_name_x = d_dialog_x + (d_dialog_w / 4) - (d_name_w / 2);
    int d_name_y = d_dialog_y + d_margin2 + d_txt6_h + 1 * RESFACTOR;

    int d_house_w = 60 * RESFACTOR;
    int d_house_h = (8 * 5 * RESFACTOR);
    int d_house_x = d_dialog_cx - (d_house_w / 2);
    int d_house_y = d_name_y;

    int d_color_w = 10 * RESFACTOR;
    int d_color_h = 9 * RESFACTOR;
    int d_color_y = d_name_y;
    int d_color_x = d_dialog_x + ((d_dialog_w / 4) * 3) - (d_color_w * 3);

    int d_playerlist_w = 118 * RESFACTOR;
    int d_playerlist_h = (6 * 6 * RESFACTOR) + 3 * RESFACTOR; // 6 rows high
    int d_playerlist_x = d_dialog_x + d_margin1 + d_margin1 + 5 * RESFACTOR;
    int d_playerlist_y = d_color_y + d_color_h + d_margin2 + 2 * RESFACTOR /*KO + d_txt6_h*/;

    int d_scenariolist_w = 162 * RESFACTOR;
    int d_scenariolist_h = (6 * 6 * RESFACTOR) + 3 * RESFACTOR; // 6 rows high
    d_scenariolist_h *= 2;

    int d_scenariolist_x = d_dialog_x + d_dialog_w - d_margin1 - d_margin1 - d_scenariolist_w - 5 * RESFACTOR;
    int d_scenariolist_y = d_color_y + d_color_h + d_margin2 + 2 * RESFACTOR;
    d_scenariolist_x = d_dialog_x + (d_dialog_w - d_scenariolist_w) / 2;

    int d_count_w = 25 * RESFACTOR;
    int d_count_h = d_txt6_h;
    int d_count_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * RESFACTOR; // (fudged)
    int d_count_y = d_playerlist_y + d_playerlist_h + (d_margin1 * 2) - 2 * RESFACTOR;
    d_count_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 2 * RESFACTOR;

    int d_level_w = 25 * RESFACTOR;
    int d_level_h = d_txt6_h;
    int d_level_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * RESFACTOR; // (fudged)
    int d_level_y = d_count_y + d_count_h;

    int d_credits_w = 25 * RESFACTOR;
    int d_credits_h = d_txt6_h;
    int d_credits_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * RESFACTOR; // (fudged)
    int d_credits_y = d_level_y + d_level_h;

    int d_aiplayers_w = 25 * RESFACTOR;
    int d_aiplayers_h = d_txt6_h;
    int d_aiplayers_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * RESFACTOR; // (fudged)
    int d_aiplayers_y = d_credits_y + d_credits_h;

    int d_options_w = 106 * RESFACTOR;
    int d_options_h = (5 * 6 * RESFACTOR) + 4 * RESFACTOR;
    int d_options_x = d_dialog_x + d_dialog_w - 149 * RESFACTOR;
    int d_options_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 2 * RESFACTOR;

    int d_message_w = d_dialog_w - (d_margin1 * 2) - 20 * RESFACTOR;
    int d_message_h = (8 * d_txt6_h) + 3 * RESFACTOR; // 4 rows high
    int d_message_x = d_dialog_x + d_margin1 + 10 * RESFACTOR;
    int d_message_y = d_options_y + d_options_h + 2 * RESFACTOR;

    int d_send_w = d_dialog_w - (d_margin1 * 2) - 20 * RESFACTOR;
    int d_send_h = 9 * RESFACTOR;
    int d_send_x = d_dialog_x + d_margin1 + 10 * RESFACTOR;
    int d_send_y = d_message_y + d_message_h;

    int d_ok_w = 45 * RESFACTOR;
    int d_ok_h = 9 * RESFACTOR;
    int d_ok_x = d_dialog_x + (d_dialog_w / 6) - (d_ok_w / 2);
    int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin1 - RESFACTOR * 6;

    int d_cancel_w = 45 * RESFACTOR;
    int d_cancel_h = 9 * RESFACTOR;
    int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
    int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin1 - RESFACTOR * 6;

    int d_load_w = 45 * RESFACTOR;
    int d_load_h = 9 * RESFACTOR;
    int d_load_x = d_dialog_x + ((d_dialog_w * 5) / 6) - (d_load_w / 2);
    int d_load_y = d_dialog_y + d_dialog_h - d_load_h - d_margin1 - RESFACTOR * 6;

    /*........................................................................
    Button Enumerations
    ........................................................................*/
    enum
    {
        BUTTON_NAME = 100,
        BUTTON_HOUSE,
        BUTTON_CREDITS,
        BUTTON_AIPLAYERS,
        BUTTON_OPTIONS,
        BUTTON_PLAYERLIST,
        BUTTON_SCENARIOLIST,
        BUTTON_COUNT,
        BUTTON_LEVEL,
        BUTTON_OK,
        BUTTON_LOAD,
        BUTTON_CANCEL,
        BUTTON_DIFFICULTY,
    };

    /*........................................................................
    Redraw values: in order from "top" to "bottom" layer of the dialog
    ........................................................................*/
    typedef enum
    {
        REDRAW_NONE = 0,
        REDRAW_PARMS,
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

    int playertabs[] = {77 * RESFACTOR};  // tabs for player list box
    int optiontabs[] = {8};               // tabs for player list box
    char namebuf[MPLAYER_NAME_MAX] = {0}; // buffer for player's name
    bool transmit;                        // 1 = re-transmit new game options
    int cbox_x[] = {d_color_x,
                    d_color_x + d_color_w,
                    d_color_x + (d_color_w * 2),
                    d_color_x + (d_color_w * 3),
                    d_color_x + (d_color_w * 4),
                    d_color_x + (d_color_w * 5),
                    d_color_x + (d_color_w * 6),
                    d_color_x + (d_color_w * 7)};
    bool parms_received = false; // 1 = game options received
    bool changed = false;        // 1 = user has changed an option

    int rc;
    int recsignedoff = false;
    int i;
    unsigned long timingtime;
    unsigned long lastmsgtime;
    unsigned long lastredrawtime;
    unsigned long transmittime = 0;
    unsigned long theirresponsetime;
    static bool first_time = true;
    bool oppscorescreen = false;
    bool gameoptions = Session.Type == GAME_SKIRMISH;
    unsigned long msg_timeout = 1200; // init to 20 seconds

    CCFileClass loadfile("SAVEGAME.NET");
    bool load_game = false; // 1 = load a saved game
    NodeNameType* who;      // node to add to Players
    char* item;             // for filling in lists
    RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
    bool messages_have_focus = true; // Gadget focus starts on the message system

    Set_Logic_Page(SeenBuff);

    CDTimerClass<SystemTimerClass> kludge_timer; // Timer to allow a wait after client joins
                                                 // game before game can start
    bool ok_button_added = false;

    /*........................................................................
    Buttons
    ........................................................................*/
    GadgetClass* commands; // button list

    EditClass name_edt(BUTTON_NAME,
                       namebuf,
                       MPLAYER_NAME_MAX,
                       TPF_TEXT,
                       d_name_x,
                       d_name_y,
                       d_name_w,
                       d_name_h,
                       EditClass::ALPHANUMERIC);

    char housetext[25] = "";
    Fancy_Text_Print("", 0, 0, 0, 0, TPF_TEXT);
    DropListClass housebtn(BUTTON_HOUSE,
                           housetext,
                           sizeof(housetext),
                           TPF_TEXT,
                           d_house_x,
                           d_house_y,
                           d_house_w,
                           d_house_h,
                           MFCD::Retrieve("BTN-UP.SHP"),
                           MFCD::Retrieve("BTN-DN.SHP"));
    ColorListClass playerlist(BUTTON_PLAYERLIST,
                              d_playerlist_x,
                              d_playerlist_y,
                              d_playerlist_w,
                              d_playerlist_h,
                              TPF_TEXT,
                              MFCD::Retrieve("BTN-UP.SHP"),
                              MFCD::Retrieve("BTN-DN.SHP"));
    ListClass scenariolist(BUTTON_SCENARIOLIST,
                           d_scenariolist_x,
                           d_scenariolist_y,
                           d_scenariolist_w,
                           d_scenariolist_h,
                           TPF_TEXT,
                           MFCD::Retrieve("BTN-UP.SHP"),
                           MFCD::Retrieve("BTN-DN.SHP"));
    GaugeClass countgauge(BUTTON_COUNT, d_count_x, d_count_y, d_count_w, d_count_h);

    char staticcountbuff[35];
    StaticButtonClass staticcount(0, "     ", TPF_TEXT, d_count_x + d_count_w + 3 * RESFACTOR, d_count_y);

    GaugeClass levelgauge(BUTTON_LEVEL, d_level_x, d_level_y, d_level_w, d_level_h);

    char staticlevelbuff[35];
    StaticButtonClass staticlevel(0, "     ", TPF_TEXT, d_level_x + d_level_w + 3 * RESFACTOR, d_level_y);

    GaugeClass creditsgauge(BUTTON_CREDITS, d_credits_x, d_credits_y, d_credits_w, d_credits_h);

    char staticcreditsbuff[35];
    StaticButtonClass staticcredits(0, "         ", TPF_TEXT, d_credits_x + d_credits_w + 3 * RESFACTOR, d_credits_y);

    GaugeClass aiplayersgauge(BUTTON_AIPLAYERS, d_aiplayers_x, d_aiplayers_y, d_aiplayers_w, d_aiplayers_h);

    char staticaibuff[35];
    StaticButtonClass staticai(0, "     ", TPF_TEXT, d_aiplayers_x + d_aiplayers_w + 3 * RESFACTOR, d_aiplayers_y);

    CheckListClass optionlist(BUTTON_OPTIONS,
                              d_options_x,
                              d_options_y,
                              d_options_w,
                              d_options_h,
                              TPF_TEXT,
                              MFCD::Retrieve("BTN-UP.SHP"),
                              MFCD::Retrieve("BTN-DN.SHP"));
    TextButtonClass okbtn(BUTTON_OK, TXT_OK, TPF_BUTTON, d_ok_x, d_ok_y, d_ok_w, d_ok_h);
    TextButtonClass loadbtn(BUTTON_LOAD, TXT_LOAD_BUTTON, TPF_BUTTON, d_load_x, d_load_y, d_load_w, d_load_h);
    TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_BUTTON, d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);

    SliderClass difficulty(BUTTON_DIFFICULTY,
                           d_name_x,
                           optionlist.Y + optionlist.Height + d_margin1 + d_margin1,
                           d_dialog_w - (d_name_x - d_dialog_x) * 2,
                           8 * RESFACTOR,
                           true);
    if (Rule.IsFineDifficulty) {
        difficulty.Set_Maximum(5);
        difficulty.Set_Value(2);
    } else {
        difficulty.Set_Maximum(3);
        difficulty.Set_Value(1);
    }

    /*
    ------------------------- Build the button list --------------------------
    */
    commands = &name_edt;
    staticcount.Add_Tail(*commands);
    staticcredits.Add_Tail(*commands);
    staticai.Add_Tail(*commands);
    staticlevel.Add_Tail(*commands);
    difficulty.Add_Tail(*commands);
    scenariolist.Add_Tail(*commands);
    countgauge.Add_Tail(*commands);
    levelgauge.Add_Tail(*commands);
    creditsgauge.Add_Tail(*commands);
    aiplayersgauge.Add_Tail(*commands);
    optionlist.Add_Tail(*commands);
    if (Session.Type == GAME_SKIRMISH) {
        okbtn.Add_Tail(*commands);
    }
    cancelbtn.Add_Tail(*commands);
    cancelbtn.X = loadbtn.X;
    housebtn.Add_Tail(*commands);

    /*
    ----------------------------- Various Inits ------------------------------
    */
    /*........................................................................
    Init player name & house
    ........................................................................*/
    Session.ColorIdx = Session.PrefColor; // init my preferred color
    strcpy(namebuf, Session.Handle);      // set my name
    name_edt.Set_Text(namebuf, MPLAYER_NAME_MAX);
    name_edt.Set_Color(&ColorRemaps[(Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx]);

    for (HousesType house = HOUSE_USSR; house <= HOUSE_FRANCE; house++) {
        housebtn.Add_Item(Text_String(HouseTypeClass::As_Reference(house).Full_Name()));
    }
    housebtn.Set_Selected_Index(Session.House - HOUSE_USSR);
    housebtn.Set_Read_Only(true);

    /*........................................................................
    Init scenario values, only the first time through
    ........................................................................*/
    Special.IsCaptureTheFlag = Rule.IsMPCaptureTheFlag;
    if (first_time) {
        Session.Options.Credits = Rule.MPDefaultMoney; // init credits & credit buffer
        Session.Options.Bases = Rule.IsMPBasesOn;      // init scenario parameters
        Session.Options.Tiberium = Rule.IsMPTiberiumGrow;
        Session.Options.Goodies = Rule.IsMPCrates;
        Session.Options.AIPlayers = 0;
        Special.IsShadowGrow = Rule.IsMPShadowGrow;
        Session.Options.UnitCount =
            (SessionClass::CountMax[Session.Options.Bases] + SessionClass::CountMin[Session.Options.Bases]) / 2;
        first_time = false;
    }

    /*........................................................................
    Init button states
    ........................................................................*/
    playerlist.Set_Tabs(playertabs);
    playerlist.Set_Selected_Style(ColorListClass::SELECT_NORMAL);

    optionlist.Set_Tabs(optiontabs);
    optionlist.Set_Read_Only(0);

    optionlist.Add_Item(Text_String(TXT_BASES));
    optionlist.Add_Item(Text_String(TXT_ORE_SPREADS));
    optionlist.Add_Item(Text_String(TXT_CRATES));
    optionlist.Add_Item(Text_String(TXT_SHADOW_REGROWS));

    optionlist.Check_Item(0, Session.Options.Bases);
    optionlist.Check_Item(1, Session.Options.Tiberium);
    optionlist.Check_Item(2, Session.Options.Goodies);
    optionlist.Check_Item(3, Special.IsShadowGrow);

    countgauge.Set_Maximum(SessionClass::CountMax[Session.Options.Bases]
                           - SessionClass::CountMin[Session.Options.Bases]);
    countgauge.Set_Value(Session.Options.UnitCount - SessionClass::CountMin[Session.Options.Bases]);

    levelgauge.Set_Maximum(MPLAYER_BUILD_LEVEL_MAX - 1);
    levelgauge.Set_Value(BuildLevel - 1);

    creditsgauge.Set_Maximum(Rule.MPMaxMoney);
    creditsgauge.Set_Value(Session.Options.Credits);

    int maxp = Rule.MaxPlayers - 2;
    aiplayersgauge.Set_Maximum(maxp);

    if (Session.Options.AIPlayers > 7) {
        Session.Options.AIPlayers = 7;
    }
    Session.Options.AIPlayers = max(Session.Options.AIPlayers, 1);

    aiplayersgauge.Set_Value(Session.Options.AIPlayers - 1);

    /*........................................................................
    Init other scenario parameters
    ........................................................................*/
    Special.IsTGrowth =     // Session.Options.Tiberium;
        Rule.IsTGrowth =    // Session.Options.Tiberium;
        Special.IsTSpread = // Session.Options.Tiberium;
        Rule.IsTSpread = Session.Options.Tiberium;
    transmit = true;

    /*........................................................................
    Clear the Players vector
    ........................................................................*/
    Clear_Vector(&Session.Players);

    /*........................................................................
    Init scenario description list box
    ........................................................................*/
    for (i = 0; i < Session.Scenarios.Count(); i++) {
        int j;
        for (j = 0; EngMisStr[j] != NULL; j++) {
            if (!strcmp(Session.Scenarios[i]->Description(), EngMisStr[j])) {
#ifdef FIXIT_CSII //	ajw Added Aftermath installed checks (before, it was assumed).
                //	Add mission if it's available to us.
                if (!((Is_Mission_Counterstrike((char*)(Session.Scenarios[i]->Get_Filename()))
                       && !Is_Counterstrike_Installed())
                      || (Is_Mission_Aftermath((char*)(Session.Scenarios[i]->Get_Filename()))
                          && !Is_Aftermath_Installed())))
#endif
#if defined(GERMAN) || defined(FRENCH)
                    scenariolist.Add_Item(EngMisStr[j + 1]);
#else
                scenariolist.Add_Item(EngMisStr[j]);
#endif
                break;
            }
        }
        if (EngMisStr[j] == NULL) {
#ifdef FIXIT_CSII //	ajw Added Aftermath installed checks (before, it was assumed). Added officialness check.
            //	Add mission if it's available to us.
            if (!Session.Scenarios[i]->Get_Official()
                || !((Is_Mission_Counterstrike((char*)(Session.Scenarios[i]->Get_Filename()))
                      && !Is_Counterstrike_Installed())
                     || (Is_Mission_Aftermath((char*)(Session.Scenarios[i]->Get_Filename()))
                         && !Is_Aftermath_Installed())))
#endif
                scenariolist.Add_Item(Session.Scenarios[i]->Description());
        }
    }

    Session.Options.ScenarioIndex = 0; // 1st scenario is selected

    /*........................................................................
    Init random-number generator, & create a seed to be used for all random
    numbers from here on out
    ........................................................................*/
#ifdef FIXIT_RANDOM_GAME
    srand((unsigned)time(NULL));
    Seed = rand();
#else
//	randomize();
//	Seed = rand();
#endif

    /*........................................................................
    Init version number clipping system
    ........................................................................*/
    VerNum.Init_Clipping();
    Load_Title_Page(true);
    CCPalette.Set();

    /*
    ---------------------------- Processing loop -----------------------------
    */
    theirresponsetime = 10000; // initialize to an invalid value
    timingtime = lastmsgtime = lastredrawtime = TickCount;

    while (process) {
        /*
        ........................ Invoke game callback .........................
        */
        Call_Back();

        /*
        ** If we have just received input focus again after running in the background then
        ** we need to redraw.
        */
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            display = REDRAW_ALL;
        }

        /*
        ...................... Refresh display if needed ......................
        */
        if (display) {
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            Hide_Mouse();

            /*
            .................. Redraw backgound & dialog box ...................
            */
            if (display >= REDRAW_BACKGROUND) {
                Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

                // init font variables

                Fancy_Text_Print(TXT_NONE, 0, 0, scheme, TBLACK, TPF_CENTER | TPF_TEXT);

                /*...............................................................
                Dialog & Field labels
                ...............................................................*/
                Fancy_Text_Print(TXT_YOUR_NAME,
                                 d_name_x + (d_name_w / 2),
                                 d_name_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);

                Fancy_Text_Print(TXT_SIDE_COLON,
                                 d_house_x + (d_house_w / 2),
                                 d_house_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);
                Fancy_Text_Print(TXT_COLOR_COLON,
                                 d_dialog_x + ((d_dialog_w / 4) * 3),
                                 d_color_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);

                Fancy_Text_Print(TXT_EASY, difficulty.X, difficulty.Y - 8 * RESFACTOR, scheme, TBLACK, TPF_TEXT);
                Fancy_Text_Print(TXT_HARD,
                                 difficulty.X + difficulty.Width,
                                 difficulty.Y - 8 * RESFACTOR,
                                 scheme,
                                 TBLACK,
                                 TPF_RIGHT | TPF_TEXT);
                Fancy_Text_Print(TXT_NORMAL,
                                 difficulty.X + difficulty.Width / 2,
                                 difficulty.Y - 8 * RESFACTOR,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);
                Fancy_Text_Print(TXT_SCENARIOS,
                                 d_scenariolist_x + (d_scenariolist_w / 2),
                                 d_scenariolist_y - d_txt6_h,
                                 scheme,
                                 TBLACK,
                                 TPF_CENTER | TPF_TEXT);
                Fancy_Text_Print(TXT_COUNT, d_count_x - 2, d_count_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
                Fancy_Text_Print(TXT_LEVEL, d_level_x - 2, d_level_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
                Fancy_Text_Print(TXT_CREDITS_COLON, d_credits_x - 2, d_credits_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
                Fancy_Text_Print(TXT_AI_PLAYERS_COLON,
                                 d_aiplayers_x - 2 * RESFACTOR,
                                 d_aiplayers_y,
                                 scheme,
                                 TBLACK,
                                 TPF_TEXT | TPF_RIGHT);
            }

            /*..................................................................
            Draw the color boxes
            ..................................................................*/
            if (display >= REDRAW_COLORS) {
                for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
                    LogicPage->Fill_Rect(cbox_x[i] + 1,
                                         d_color_y + 1,
                                         cbox_x[i] + 1 + d_color_w - 2,
                                         d_color_y + 1 + d_color_h - 2,
                                         ColorRemaps[i].Box);

                    if (i == Session.ColorIdx) {
                        Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_DOWN, false);
                    } else {
                        Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_RAISED, false);
                    }
                }
            }

            //..................................................................
            // Update game parameter labels
            //..................................................................
            if (display >= REDRAW_PARMS) {
                sprintf(staticcountbuff, "%d", Session.Options.UnitCount);
                staticcount.Set_Text(staticcountbuff);
                staticcount.Draw_Me();

                if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
                    sprintf(staticlevelbuff, "%d ", BuildLevel);
                } else {
                    sprintf(staticlevelbuff, "**");
                }
                staticlevel.Set_Text(staticlevelbuff);
                staticlevel.Draw_Me();

                sprintf(staticcreditsbuff, "%d", Session.Options.Credits);
                staticcredits.Set_Text(staticcreditsbuff);
                staticcredits.Draw_Me();

                sprintf(staticaibuff, "%d", Session.Options.AIPlayers);
                staticai.Set_Text(staticaibuff);
                staticai.Draw_Me();
            }

            /*
            .......................... Redraw buttons ..........................
            */
            if (display >= REDRAW_BUTTONS) {
                commands->Flag_List_To_Redraw();
                commands->Draw_All();
            }

            Show_Mouse();
            display = REDRAW_NONE;
        }

        /*
        ........................... Get user input ............................
        */
        messages_have_focus = Session.Messages.Has_Edit_Focus();
        bool droplist_is_dropped = housebtn.IsDropped;
        input = commands->Input();

        /*
        ** Redraw everything if the droplist collapsed
        */
        if (droplist_is_dropped && !housebtn.IsDropped) {
            display = REDRAW_BACKGROUND;
        }

        if (input & KN_BUTTON) {
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
        }

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

                Session.PrefColor = (PlayerColorType)((Keyboard->MouseQX - cbox_x[0]) / d_color_w);
                Session.ColorIdx = Session.PrefColor;
                if (display < REDRAW_COLORS)
                    display = REDRAW_COLORS;

                name_edt.Set_Color(
                    &ColorRemaps[(Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx]);
                name_edt.Flag_To_Redraw();
                Session.Messages.Set_Edit_Color((Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE
                                                                                         : Session.ColorIdx);
                strcpy(Session.Handle, namebuf);
                transmit = true;
                changed = true;
                if (housebtn.IsDropped) {
                    housebtn.Collapse();
                    display = REDRAW_BACKGROUND;
                }
            }
            break;

        /*------------------------------------------------------------------
        User edits the name field; retransmit new game options
        ------------------------------------------------------------------*/
        case (BUTTON_NAME | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            strcpy(Session.Handle, namebuf);
            transmit = true;
            changed = true;
            break;

        case (BUTTON_HOUSE | KN_BUTTON):
            Session.House = HousesType(housebtn.Current_Index() + HOUSE_USSR);
            strcpy(Session.Handle, namebuf);
            display = REDRAW_BACKGROUND;
            transmit = true;
            break;

            /*------------------------------------------------------------------
            New Scenario selected.
            ------------------------------------------------------------------*/
        case (BUTTON_SCENARIOLIST | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            if (scenariolist.Current_Index() != Session.Options.ScenarioIndex) {
                Session.Options.ScenarioIndex = scenariolist.Current_Index();
                strcpy(Session.Handle, namebuf);
                transmit = true;
            }
            break;

        /*------------------------------------------------------------------
        User adjusts max # units
        ------------------------------------------------------------------*/
        case (BUTTON_COUNT | KN_BUTTON):
            Session.Options.UnitCount = countgauge.Get_Value() + SessionClass::CountMin[Session.Options.Bases];
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            transmit = true;
            break;

        /*------------------------------------------------------------------
        User adjusts build level
        ------------------------------------------------------------------*/
        case (BUTTON_LEVEL | KN_BUTTON):
            BuildLevel = levelgauge.Get_Value() + 1;
            if (BuildLevel > MPLAYER_BUILD_LEVEL_MAX) // if it's pegged, max it out
                BuildLevel = MPLAYER_BUILD_LEVEL_MAX;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            transmit = true;
            break;

        /*------------------------------------------------------------------
        User adjusts max # units
        ------------------------------------------------------------------*/
        case (BUTTON_CREDITS | KN_BUTTON):
            Session.Options.Credits = creditsgauge.Get_Value();
            Session.Options.Credits = ((Session.Options.Credits + 250) / 500) * 500;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            transmit = true;
            break;

        //..................................................................
        //	User adjusts # of AI players
        //..................................................................
        case (BUTTON_AIPLAYERS | KN_BUTTON): {
            Session.Options.AIPlayers = aiplayersgauge.Get_Value();
            int humans = 1; // One humans.
            Session.Options.AIPlayers += 1;
            if (Session.Options.AIPlayers + humans >= Rule.MaxPlayers) { // if it's pegged, max it out
                Session.Options.AIPlayers = Rule.MaxPlayers - humans;
                aiplayersgauge.Set_Value(Session.Options.AIPlayers - 1);
            }
            transmit = true;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;

            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }

            break;
        }

        //------------------------------------------------------------------
        // Toggle-able options:
        // If 'Bases' gets toggled, we have to change the range of the
        // UnitCount slider.
        // Also, if Tiberium gets toggled, we have to set the flags
        // in SpecialClass.
        //------------------------------------------------------------------
        case (BUTTON_OPTIONS | KN_BUTTON):
            if (Session.Options.Bases != optionlist.Is_Checked(0)) {
                Session.Options.Bases = optionlist.Is_Checked(0);
                if (Session.Options.Bases) {
                    Session.Options.UnitCount =
                        Fixed_To_Cardinal(SessionClass::CountMax[1] - SessionClass::CountMin[1],
                                          Cardinal_To_Fixed(SessionClass::CountMax[0] - SessionClass::CountMin[0],
                                                            Session.Options.UnitCount - SessionClass::CountMin[0]))
                        + SessionClass::CountMin[1];
                } else {
                    Session.Options.UnitCount =
                        Fixed_To_Cardinal(SessionClass::CountMax[0] - SessionClass::CountMin[0],
                                          Cardinal_To_Fixed(SessionClass::CountMax[1] - SessionClass::CountMin[1],
                                                            Session.Options.UnitCount - SessionClass::CountMin[1]))
                        + SessionClass::CountMin[0];
                }
                countgauge.Set_Maximum(SessionClass::CountMax[Session.Options.Bases]
                                       - SessionClass::CountMin[Session.Options.Bases]);
                countgauge.Set_Value(Session.Options.UnitCount - SessionClass::CountMin[Session.Options.Bases]);
            }
            Session.Options.Tiberium = optionlist.Is_Checked(1);
            Special.IsTGrowth = Session.Options.Tiberium;
            Rule.IsTGrowth = Session.Options.Tiberium;
            Special.IsTSpread = Session.Options.Tiberium;
            Rule.IsTSpread = Session.Options.Tiberium;

            Session.Options.Goodies = optionlist.Is_Checked(2);
            Special.IsShadowGrow = optionlist.Is_Checked(3);

            transmit = true;
            if (display < REDRAW_PARMS)
                display = REDRAW_PARMS;
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            break;

        /*------------------------------------------------------------------
        OK: exit loop with true status
        ------------------------------------------------------------------*/
        case (BUTTON_LOAD | KN_BUTTON):
        case (BUTTON_OK | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            //
            // make sure we got a game options packet from the other player
            //
            if (gameoptions) {
                rc = true;
                process = false;

                // force transmitting of game options packet one last time

                transmit = true;
                transmittime = 0;
            } else {
                WWMessageBox().Process(TXT_ONLY_ONE, TXT_OOPS, NULL);
                display = REDRAW_ALL;
            }
            if (input == (BUTTON_LOAD | KN_BUTTON))
                load_game = true;
            break;

        /*------------------------------------------------------------------
        CANCEL: send a SIGN_OFF, bail out with error code
        ------------------------------------------------------------------*/
        case (KN_ESC):
        case (BUTTON_CANCEL | KN_BUTTON):
            if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
            }
            process = false;
            rc = false;
            break;

        /*------------------------------------------------------------------
        Default: manage the inter-player messages
        ------------------------------------------------------------------*/
        default:
            break;
        }

        /*---------------------------------------------------------------------
        Detect editing of the name buffer, transmit new values to players
        ---------------------------------------------------------------------*/
        if (strcmp(namebuf, Session.Handle)) {
            strcpy(Session.Handle, namebuf);
            transmit = true;
            changed = true;
        }

        Frame_Limiter();
    }

    /*------------------------------------------------------------------------
    Prepare to load the scenario
    ------------------------------------------------------------------------*/
    if (rc) {
        Session.NumPlayers = 1;

        Scen.Scenario = Session.Options.ScenarioIndex;
        strcpy(Scen.ScenarioName, Session.Scenarios[Session.Options.ScenarioIndex]->Get_Filename());

        /*.....................................................................
        Add both players to the Players vector; the local system is always
        index 0.
        .....................................................................*/
        who = new NodeNameType;
        strcpy(who->Name, namebuf);
        who->Player.House = Session.House;
        who->Player.Color = Session.ColorIdx;
        who->Player.ProcessTime = -1;
        Session.Players.Add(who);

        /*
        **	Fetch the difficulty setting when in skirmish mode.
        */

        int diff = difficulty.Get_Value() * (Rule.IsFineDifficulty ? 1 : 2);
        switch (diff) {
        case 0:
            Scen.CDifficulty = DIFF_HARD;
            Scen.Difficulty = DIFF_EASY;
            break;

        case 1:
            Scen.CDifficulty = DIFF_HARD;
            Scen.Difficulty = DIFF_NORMAL;
            break;

        case 2:
            Scen.CDifficulty = DIFF_NORMAL;
            Scen.Difficulty = DIFF_NORMAL;
            break;

        case 3:
            Scen.CDifficulty = DIFF_EASY;
            Scen.Difficulty = DIFF_NORMAL;
            break;

        case 4:
            Scen.CDifficulty = DIFF_EASY;
            Scen.Difficulty = DIFF_HARD;
            break;
        }
    }

    /*------------------------------------------------------------------------
    Clear all lists
    ------------------------------------------------------------------------*/
    while (scenariolist.Count()) {
        scenariolist.Remove_Item(scenariolist.Get_Item(0));
    }

    /*------------------------------------------------------------------------
    Clean up the list boxes
    ------------------------------------------------------------------------*/
    while (playerlist.Count() > 0) {
        item = (char*)playerlist.Get_Item(0);
        delete[] item;
        playerlist.Remove_Item(item);
    }

    /*------------------------------------------------------------------------
    Remove the chat edit box
    ------------------------------------------------------------------------*/
    Session.Messages.Remove_Edit();

    /*------------------------------------------------------------------------
    Restore screen
    ------------------------------------------------------------------------*/
    Hide_Mouse();
    Load_Title_Page(true);
    Show_Mouse();

    /*------------------------------------------------------------------------
    Save any changes made to our options
    ------------------------------------------------------------------------*/
    if (changed) {
        Session.Write_MultiPlayer_Settings();
    }

    return (rc);
}
#endif // REMASTER_BUILD
