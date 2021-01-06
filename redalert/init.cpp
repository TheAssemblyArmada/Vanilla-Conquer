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

/* $Header: /CounterStrike/INIT.CPP 8     3/14/97 5:15p Joe_b $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : INIT.CPP                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : January 20, 1992                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Anim_Init -- Initialize the VQ animation control structure.                               *
 *   Bootstrap -- Perform the initial bootstrap procedure.                                     *
 *   Calculate_CRC -- Calculates a one-way hash from a data block.                             *
 *   Init_Authorization -- Verifies that the player is authorized to play the game.            *
 *   Init_Bootstrap_Mixfiles -- Registers and caches any mixfiles needed for bootstrapping.    *
 *   Init_Bulk_Data -- Initialize the time-consuming mixfile caching.                          *
 *   Init_CDROM_Access -- Initialize the CD-ROM access handler.                                *
 *   Init_Color_Remaps -- Initialize the text remap tables.                                    *
 *   Init_Expansion_Files -- Fetch any override expansion mixfiles.                            *
 *   Init_Fonts -- Initialize all the game font pointers.                                      *
 *   Init_Game -- Main game initialization routine.                                            *
 *   Init_Heaps -- Initialize the game heaps and buffers.                                      *
 *   Init_Keys -- Initialize the cryptographic keys.                                           *
 *   Init_Mouse -- Initialize the mouse system.                                                *
 *   Init_One_Time_Systems -- Initialize internal pointers to the bulk data.                   *
 *   Init_Random -- Initializes the random-number generator                                    *
 *   Init_Secondary_Mixfiles -- Register and cache secondary mixfiles.                         *
 *   Load_Recording_Values -- Loads recording values from recording file                       *
 *   Load_Title_Page -- Load the background art for the title page.                            *
 *   Obfuscate -- Sufficiently transform parameter to thwart casual hackers.                   *
 *   Parse_Command_Line -- Parses the command line parameters.                                 *
 *   Parse_INI_File -- Parses CONQUER.INI for special options                                  *
 *   Play_Intro -- plays the introduction & logo movies                                        *
 *   Save_Recording_Values -- Saves recording values to a recording file                       *
 *   Select_Game -- The game's main menu                                                       *
 *   Load_Prolog_Page -- Loads the special pre-prolog "please wait" page.                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "loaddlg.h"

#ifdef NETWORKING
#include "wsproto.h"
#include "wspudp.h"
#include "internet.h"
#endif

#include <time.h>

#ifdef DONGLE
#include "cbn_.h"
#endif

#ifdef MPEGMOVIE // Denzil 6/25/98
#include "mpgset.h"
#endif

#include "common/vqaconfig.h"

RemapControlType SidebarScheme;

//#include "WolDebug.h"

#ifdef CHEAT_KEYS
extern bool bNoMovies;
#endif

/****************************************
**	Function prototypes for this module **
*****************************************/
static void Play_Intro(bool sequenced = false);
static void Init_Color_Remaps(void);
static void Init_Heaps(void);
static void Init_Expansion_Files(void);
static void Init_One_Time_Systems(void);
static void Init_Fonts(void);
static void Init_CDROM_Access(void);
static void Init_Bootstrap_Mixfiles(void);
static void Init_Secondary_Mixfiles(void);
static void Init_Mouse(void);
static void Bootstrap(void);
// static void Init_Authorization(void);
static void Init_Bulk_Data(void);
static void Init_Keys(void);

extern int UnitBuildPenalty;

extern unsigned long RandNumb;

// extern int SimRandIndex;
void Init_Random(void);

#define ATTRACT_MODE_TIMEOUT 3600 // timeout for attract mode

bool Load_Recording_Values(CCFileClass& file);
bool Save_Recording_Values(CCFileClass& file);

#ifdef FIXIT_VERSION_3
bool Expansion_Dialog(bool bCounterstrike);
#endif

extern bool Is_Mission_Counterstrike(char* file_name);

/***********************************************************************************************
 * Load_Prolog_Page -- Loads the special pre-prolog "please wait" page.                        *
 *                                                                                             *
 *    This loads and displays the prolog page that is displayed before the prolog movie        *
 *    is played. This page is necessary because there is much loading that occurs before       *
 *    the prolog movie is played and looking at a picture is better than looking at a blank    *
 *    screen.                                                                                  *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Load_Prolog_Page(void)
{
    Hide_Mouse();
    Load_Title_Screen("PROLOG.PCX", &HidPage, (unsigned char*)CCPalette.Get_Data());
    HidPage.Blit(SeenPage);
    CCPalette.Set();
    Show_Mouse();
}

/***********************************************************************************************
 * Init_Game -- Main game initialization routine.                                              *
 *                                                                                             *
 *    Perform all one-time game initializations here. This includes all                        *
 *    allocations and table setups. The intro and other one-time startup                       *
 *    tasks are also performed here.                                                           *
 *                                                                                             *
 * INPUT:   argc,argv   -- Command line arguments.                                             *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Only call this ONCE!                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/07/1992 JLB : Created.                                                                 *
 *=============================================================================================*/
#include "sha.h"
//#include    <locale.h>
bool Init_Game(int, char*[])
{
/*
**	Allocate the benchmark tracking objects only if the machine and
**	compile flags indicate.
*/
#if 0
    if (Processor() >= 2) {
        Benches = new Benchmark[BENCH_COUNT];
    }
#endif

    /*
    **	Initialize the encryption keys.
    */
    Init_Keys();

    /*
    **	Bootstrap as much as possible before error-prone initializations are
    **	performed. This bootstrap process will enable the error message
    **	handler to function.
    */
    Bootstrap();

    ////////////////////////////////////////
    // The editor needs to not start the mouse up. - 7/22/2019 JAS
    if (!RunningFromEditor) {
        /*
        **	Check for an initialize a working mouse pointer. Display error and bail if
        **	no mouse driver is installed.
        */
        Init_Mouse();
    }
    /*
    **	Initialize access to the CD-ROM and ensure that the CD is inserted. This can, and
    **	most likely will, result in a visible prompt.
    */
#ifndef REMASTER_BUILD
    Init_CDROM_Access();
#endif

    if (Special.IsFromInstall) {
        Load_Prolog_Page();
    }

    /*
    **	Register and cache any secondary mixfiles.
    */
    Init_Secondary_Mixfiles();

    /*
    **	This is a special hack to initialize the heaps that must be in place before the
    **	rules file is processed. These heaps should properly be allocated as a consequence
    **	of processing the rules.ini file, but that is a bit beyond the capabilities of
    **	the rule parser routine (currently).
    */
    HouseTypes.Set_Heap(HOUSE_COUNT);
    BuildingTypes.Set_Heap(STRUCT_COUNT);
    AircraftTypes.Set_Heap(AIRCRAFT_COUNT);
    InfantryTypes.Set_Heap(INFANTRY_COUNT);
    BulletTypes.Set_Heap(BULLET_COUNT);
    AnimTypes.Set_Heap(ANIM_COUNT);
    UnitTypes.Set_Heap(UNIT_COUNT);
    VesselTypes.Set_Heap(VESSEL_COUNT);
    TemplateTypes.Set_Heap(TEMPLATE_COUNT);
    TerrainTypes.Set_Heap(TERRAIN_COUNT);
    OverlayTypes.Set_Heap(OVERLAY_COUNT);
    SmudgeTypes.Set_Heap(SMUDGE_COUNT);

    HouseTypeClass::Init_Heap();
    BuildingTypeClass::Init_Heap();
    AircraftTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    BulletTypeClass::Init_Heap();
    AnimTypeClass::Init_Heap();
    UnitTypeClass::Init_Heap();
    VesselTypeClass::Init_Heap();
    TemplateTypeClass::Init_Heap();
    TerrainTypeClass::Init_Heap();
    OverlayTypeClass::Init_Heap();
    SmudgeTypeClass::Init_Heap();

    // Heap init moved here from globals.cpp. ST - 5/20/2019
    CCPtr<AircraftClass>::Set_Heap(&Aircraft);
    CCPtr<AnimClass>::Set_Heap(&Anims);
    CCPtr<BuildingClass>::Set_Heap(&Buildings);
    CCPtr<BulletClass>::Set_Heap(&Bullets);
    CCPtr<FactoryClass>::Set_Heap(&Factories);
    CCPtr<HouseClass>::Set_Heap(&Houses);
    CCPtr<InfantryClass>::Set_Heap(&Infantry);
    CCPtr<OverlayClass>::Set_Heap(&Overlays);
    CCPtr<SmudgeClass>::Set_Heap(&Smudges);
    CCPtr<TeamClass>::Set_Heap(&Teams);
    CCPtr<TeamTypeClass>::Set_Heap(&TeamTypes);
    CCPtr<TemplateClass>::Set_Heap(&Templates);
    CCPtr<TerrainClass>::Set_Heap(&Terrains);
    CCPtr<TriggerClass>::Set_Heap(&Triggers);
    CCPtr<TriggerTypeClass>::Set_Heap(&TriggerTypes);

    CCPtr<HouseTypeClass>::Set_Heap(&HouseTypes);
    CCPtr<BuildingTypeClass>::Set_Heap(&BuildingTypes);
    CCPtr<AircraftTypeClass>::Set_Heap(&AircraftTypes);
    CCPtr<InfantryTypeClass>::Set_Heap(&InfantryTypes);
    CCPtr<BulletTypeClass>::Set_Heap(&BulletTypes);
    CCPtr<AnimTypeClass>::Set_Heap(&AnimTypes);
    CCPtr<UnitTypeClass>::Set_Heap(&UnitTypes);
    CCPtr<VesselTypeClass>::Set_Heap(&VesselTypes);
    CCPtr<TemplateTypeClass>::Set_Heap(&TemplateTypes);
    CCPtr<TerrainTypeClass>::Set_Heap(&TerrainTypes);
    CCPtr<OverlayTypeClass>::Set_Heap(&OverlayTypes);
    CCPtr<SmudgeTypeClass>::Set_Heap(&SmudgeTypes);

    /*
    **	Find and process any rules for this game.
    */
    CCFileClass rulesIniFile("RULES.INI");
    if (RuleINI.Load(rulesIniFile, false)) {
        Rule.Process(RuleINI);
    }
#ifdef FIXIT_CSII //	checked - ajw 9/28/98
    //  Aftermath runtime change 9/29/98
    //	This is safe to do, as only rules for aftermath units are included in this ini.
    if (Is_Aftermath_Installed() == true) {
        CCFileClass aftermathIniFile("AFTRMATH.INI");
        if (AftermathINI.Load(aftermathIniFile, false)) {
            Rule.Process(AftermathINI);
        }
    }
#endif

    Session.MaxPlayers = Rule.MaxPlayers;

    /*
    **	Initialize the game object heaps as well as other rules-dependant buffer allocations.
    */
    Init_Heaps();

    /*
    **	Initialize the animation system.
    */
    Anim_Init();

#ifdef MPEGMOVIE // Denzil 6/15/98
    if (Using_DVD()) {
#ifdef MCIMPEG
        MciMovie = new MCIMovie(MainWindow);
#endif
        MpgSettings = new MPGSettings(NULL); // RawFileClass(CONFIG_FILE_NAME));
    }
#endif

    /*
    **	Play the startup animation.
    */
    if (!Special.IsFromInstall) {
        VisiblePage.Clear();
        //		Mono_Printf("Playing Intro\n");
        Play_Intro();
        memset(CurrentPalette, 0x01, 768);
        WhitePalette.Set();
    } else {
        memset(CurrentPalette, 0x01, 768);
    }

    /*
    **	Initialize the text remap tables.
    */
    Init_Color_Remaps();

    /*
    **	Get authorization to access the game.
    */
    //	Init_Authorization();
    //	Show_Mouse();

    /*
    **	Set the logic page to the seenpage.
    */
    Set_Logic_Page(SeenBuff);

    /*
    **	If not automatically launching into the intro, then display the title
    **	page while the bulk data is cached.
    */
    if (!Special.IsFromInstall) {
        Load_Title_Page(true);

        Hide_Mouse();
        Fancy_Text_Print(TXT_STAND_BY,
                         160 * RESFACTOR,
                         120 * RESFACTOR,
                         &ColorRemaps[PCOLOR_DIALOG_BLUE],
                         TBLACK,
                         TPF_CENTER | TPF_TEXT | TPF_DROPSHADOW);
        Show_Mouse();

        CCPalette.Set(FADE_PALETTE_SLOW);
        Call_Back();
    }

    /*
    **	Initialize the bulk data. This takes the longest time and must be performed once
    **	before the regular game starts.
    */
    Init_Bulk_Data();

    /*
    **	Initialize the multiplayer score values
    */
    Session.GamesPlayed = 0;
    Session.NumScores = 0;
    Session.CurGame = 0;
    for (int i = 0; i < MAX_MULTI_NAMES; i++) {
        Session.Score[i].Name[0] = '\0';
        Session.Score[i].Wins = 0;
        for (int j = 0; j < MAX_MULTI_GAMES; j++) {
            Session.Score[i].Kills[j] = -1; // -1 = this player didn't play this round
        }
    }

    /*
    ** Copy the title screen's palette into the GamePalette & OriginalPalette,
    ** because the options Load routine uses these palettes to set the brightness, etc.
    */
    GamePalette = CCPalette;
    //	InGamePalette = CCPalette;
    OriginalPalette = CCPalette;

    /*
    **	Read game options, so the GameSpeed is initialized when multiplayer
    ** dialogs are invoked.  (GameSpeed must be synchronized between systems.)
    */
    Options.Load_Settings();

    /*
    ** Initialise the color lookup tables for the chronal vortex
    */
    ChronalVortex.Stop();
    ChronalVortex.Setup_Remap_Tables(Scen.Theater);

    /*
    **	Clear out name overrides array
    */
#ifdef FIXIT_NAME_OVERRIDE
    for (int index = 0; index < ARRAY_SIZE(NameOverride); index++) {
        NameOverride[index] = NULL;
        NameIDOverride[index] = 0;
    }
#endif // FIXIT_NAME_OVERRIDE

    return (true);
}

/***********************************************************************************************
 * Select_Game -- The game's main menu                                                         *
 *                                                                                             *
 * INPUT:                                                                                      *
 *		fade		if true, will fade the palette in gradually												  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *		none. *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *		none. *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/05/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
bool Select_Game(bool fade)
{
    //	Enums in Select_Game() must match order of buttons in Main_Menu().
#ifdef FIXIT_VERSION_3
    enum
    {
        SEL_TIMEOUT = -1,     // main menu timeout--go into attract mode
        SEL_NEW_SCENARIO_CS,  // Expansion scenario to play.
        SEL_NEW_SCENARIO_AM,  // Expansion scenario to play.
        SEL_START_NEW_GAME,   // start a new game
        SEL_LOAD_MISSION,     // load a saved game
        SEL_MULTIPLAYER_GAME, // play modem/null-modem/network game
        SEL_INTRO,            // couch-potato mode
        SEL_EXIT,             // exit to DOS
        SEL_FAME,             // view the hall o' fame
        SEL_NONE,             // placeholder default value
    };
#else                //	FIXIT_VERSION_3
    enum
    {
        SEL_TIMEOUT = -1,   // main menu timeout--go into attract mode
        SEL_NEW_SCENARIO,   // Expansion scenario to play.
        SEL_START_NEW_GAME, // start a new game
#ifndef INTERNET_OFF // Denzil 5/1/98 - Internet play
        SEL_INTERNET,
#endif
        //#if defined(MPEGMOVIE) // Denzil 6/25/98
        //		SEL_MOVIESETTINGS,
        //#endif
        SEL_LOAD_MISSION,     // load a saved game
        SEL_MULTIPLAYER_GAME, // play modem/null-modem/network game
        SEL_INTRO,            // couch-potato mode
        SEL_EXIT,             // exit to DOS
        SEL_FAME,             // view the hall o' fame
        SEL_NONE,             // placeholder default value
    };
#endif //	FIXIT_VERSION_3

    bool gameloaded = false; // Has the game been loaded from the menu?
    int selection;           // the default selection
    bool process = true;     // false = break out of while loop
    bool display = true;

#ifdef DONGLE
    /* These where added by ColinM for the dongle checking */
    short iRet = 0;
    unsigned short iPortNr = 1; /* automatic port scan enabled */
    unsigned char cSCodeSER[] = "\x41\x42";
    unsigned long ulIdRet = 0;
    unsigned char cBoxName[] = "\x00\x00";
#endif

#ifdef FIXIT_CSII //	checked - ajw 9/28/98
    int cdcheck = 0;
    bool cs = Is_Counterstrike_Installed();
#endif

    //	#ifndef DVD // Denzil - We want the menu screen			ajw No we don't
    //	if (Special.IsFromInstall) {
    //		display = false;
    //	}
    //	#endif

    Show_Mouse();

#ifdef FIXIT_CSII //	checked - ajw 9/28/98
    NewUnitsEnabled = SecretUnitsEnabled =
        0; // Assume new units disabled, unless specifically .INI enabled or multiplayer negotiations enable it.
#endif

    /*
    **	[Re]set any globals that need it, in preparation for a new scenario
    */
    GameActive = true;
    DoList.Init();
#ifdef MIRROR_QUEUE
    MirrorList.Init();
#endif
    OutList.Init();
    Frame = 0;
    Scen.MissionTimer = 0;
    Scen.MissionTimer.Stop();
    Scen.CDifficulty = DIFF_NORMAL;
    Scen.Difficulty = DIFF_NORMAL;
    PlayerWins = false;
    PlayerLoses = false;
    Session.ObiWan = false;
    Debug_Unshroud = false;
    Map.Set_Cursor_Shape(0);
    Map.PendingObjectPtr = 0;
    Map.PendingObject = 0;
    Map.PendingHouse = HOUSE_NONE;

    Session.ProcessTicks = 0;
    Session.ProcessFrames = 0;
    Session.DesiredFrameRate = 30;
#if (TIMING_FIX)
    NewMaxAheadFrame1 = 0;
    NewMaxAheadFrame2 = 0;
#endif

/* ColinM added to check for dongle */
#ifdef DONGLE
    iRet = CbN_BoxReady(iPortNr, cBoxName);
    if (cBoxName[0] != 0xc5 && cBoxName[0] != 0xc9) {
        WWMessageBox().Process("Please ensure dongle is attached. Run the dongle batch file too.", TXT_OK);
        Emergency_Exit(EXIT_FAILURE);
    }

    iRet = CbN_ReadSER(iPortNr, cSCodeSER, &ulIdRet);
    if (ulIdRet != 0xa0095) {
        WWMessageBox().Process("Please ensure dongle is attached. Run the dongle batch file too.", TXT_OK);
        Emergency_Exit(EXIT_FAILURE);
    }
#endif

    /*
    **	Init multiplayer game scores.  Let Wins accumulate; just init the current
    ** Kills for this game.  Kills of -1 means this player didn't play this round.
    */
    for (int i = 0; i < MAX_MULTI_GAMES; i++) {
        Session.Score[i].Kills[Session.CurGame] = -1;
    }

    /*
    **	Set default mouse shape
    */
    Map.Set_Default_Mouse(MOUSE_NORMAL, false);

    /*
    **	If the last game we played was a multiplayer game, jump right to that
    **	menu by pre-setting 'selection'.
    */
    if (Session.Type == GAME_NORMAL) {
        selection = SEL_NONE;
    } else {
        selection = SEL_MULTIPLAYER_GAME;
    }

    /*
    **	Main menu processing; only do this if we're not in editor mode.
    */
    if (!Debug_Map) {

        /*
        **	Menu selection processing loop
        */
        Theme.Queue_Song(THEME_CRUS);

        /*
        ** If we're playing back a recording, load all pertinent values & skip
        ** the menu loop.  Hide the now-useless mouse pointer.
        */
        if (Session.Play && Session.RecordFile.Is_Available()) {
            if (Session.RecordFile.Open(READ)) {
                Load_Recording_Values(Session.RecordFile);
                process = false;
                Theme.Fade_Out();
            } else
                Session.Play = false;
        }

        while (process) {

            /*
            **	Redraw the title page if needed
            */
            if (display) {
                Hide_Mouse();

                /*
                **	Display the title page; fade it in if this is the first time
                **	through the loop, and the 'fade' flag is true
                */
                Load_Title_Page();
                GamePalette = CCPalette;

                HidPage.Blit(SeenPage);
                //				if (fade) {
                //					WhitePalette.Set();
                //					CCPalette.Set(FADE_PALETTE_SLOW, Call_Back);
                //					fade = false;
                //				} else {
                CCPalette.Set();
                //				}

                Set_Logic_Page(SeenBuff);
                display = false;
                Show_Mouse();
            } else {
                if (RunningAsDLL) { // PG
                    return true;
                    ;
                }
            }

            /*
            **	Display menu and fetch selection from player.
            */
            if (Special.IsFromInstall)
                selection = SEL_START_NEW_GAME;

            if (selection == SEL_NONE) {
#ifdef FIXIT_ANTS
                AntsEnabled = false;
#endif
                selection = Main_Menu(ATTRACT_MODE_TIMEOUT);
            }
            Call_Back();

            switch (selection) {

                /*
                **	Pick an expansion scenario.
                */
#ifdef FIXIT_VERSION_3
            case SEL_NEW_SCENARIO_CS:
            case SEL_NEW_SCENARIO_AM:
#else  //	FIXIT_VERSION_3
            case SEL_NEW_SCENARIO:
#endif //	FIXIT_VERSION_3
                Scen.CarryOverMoney = 0;
                IsTanyaDead = false;
                SaveTanya = false;

#ifdef FIXIT_VERSION_3
                if (selection == SEL_NEW_SCENARIO_CS) {
                    if (!Force_CD_Available(2)) {
                        selection = SEL_NONE;
                        break;
                    }
                    if (!Expansion_Dialog(true)) {
                        selection = SEL_NONE;
                        break;
                    }
                } else {
                    if (!Force_CD_Available(3)) {
                        selection = SEL_NONE;
                        break;
                    }
                    if (!Expansion_Dialog(false)) {
                        selection = SEL_NONE;
                        break;
                    }
                }
#else             //	FIXIT_VERSION_3
#ifdef FIXIT_CSII //	checked - ajw 9/28/98
                if (cs) {
                    cdcheck = 2;
                }
                if (Is_Aftermath_Installed()) {
                    if (!cdcheck) {
                        cdcheck = 3;
                    } else {
                        cdcheck = 4; // special case: means check for 3 or 4
                    }
                }
                if (!Force_CD_Available(cdcheck)) {
                    return (false);
                }
#else
                if (!Force_CD_Available(2)) {
                    return (false);
                }
#endif
                if (!Expansion_Dialog()) {
                    selection = SEL_NONE;
                    break;
                }
#endif //	FIXIT_VERSION_3

#ifdef FIXIT_DIFFICULTY
#ifdef FIXIT_CSII //	checked - ajw 9/28/98
                switch (Fetch_Difficulty(cdcheck >= 3)) {
#else
                switch (Fetch_Difficulty()) {
#endif
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
#endif
                Theme.Fade_Out();
                Theme.Queue_Song(THEME_FIRST);
                Session.Type = GAME_NORMAL;
                process = false;
                break;

            /*
            **	SEL_START_NEW_GAME: Play the game
            */
            case SEL_START_NEW_GAME:
                if (Special.IsFromInstall) {
                    Scen.CDifficulty = DIFF_NORMAL;
                    Scen.Difficulty = DIFF_NORMAL;
                } else {
                    switch (Fetch_Difficulty()) {
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
                Scen.CarryOverMoney = 0;
                BuildLevel = 10;
                IsTanyaDead = false;
                SaveTanya = false;
                Whom = HOUSE_GOOD;

                if (!Special.IsFromInstall) {
#ifdef FIXIT_ANTS
                    if (AntsEnabled) {
                        Scen.Set_Scenario_Name("SCA01EA.INI");
                    } else {
#endif
#ifdef FIXIT_VERSION_3
                        switch (WWMessageBox().Process(TXT_CHOOSE, TXT_ALLIES, TXT_CANCEL, TXT_SOVIET)) {
                        case 2:
                            Scen.Set_Scenario_Name("SCU01EA.INI");
                            break;
                        default:
                            selection = SEL_NONE;
                            continue;
#else
                    switch (WWMessageBox().Process(TXT_CHOOSE, TXT_ALLIES, TXT_SOVIET)) {
                    case 1:
                        Scen.Set_Scenario_Name("SCU01EA.INI");
                        break;
                    default:
#endif
                        case 0:
                            if (Is_Demo()) {
                                Scen.Set_Scenario_Name("SCG02EA.INI");
                            } else {
                                Scen.Set_Scenario_Name("SCG01EA.INI");
                            }
                            break;
                        }
#ifdef FIXIT_ANTS
                    }
#endif
                    Theme.Fade_Out();
                    Load_Title_Page();
                } else {
                    Theme.Fade_Out();
#ifdef DVD // Denzil			ajw Presumably a bug fix.
                    Choose_Side();
                    Hide_Mouse();
#else
                    Hide_Mouse();
                    Choose_Side();
#endif
                    if (CurrentCD == 0) {
                        Scen.Set_Scenario_Name("SCG01EA.INI");
                    } else {
                        Scen.Set_Scenario_Name("SCU01EA.INI");
                    }
                }

                Session.Type = GAME_NORMAL;
                process = false;
                break;

                //				#if defined(MPEGMOVIE) // Denzil 6/25/98
                //				case SEL_MOVIESETTINGS:
                //					MpgSettings->Dialog();
                //					display = true;
                //					selection = SEL_NONE;
                //				break;
                //				#endif

            /*
            **	Load a saved game.
            */
            case SEL_LOAD_MISSION:
                if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
                    Theme.Queue_Song(THEME_FIRST);
                    process = false;
                    gameloaded = true;
                } else {
                    display = true;
                    selection = SEL_NONE;
                }
                break;

            /*
            **	SEL_MULTIPLAYER_GAME: set 'Session.Type' to NULL-modem, modem, or
            **	network play.
            */
            case SEL_MULTIPLAYER_GAME:
#ifdef WOLAPI_INTEGRATION
                if (!pWolapi) {
#endif
                    switch (Session.Type) {

                    /*
                    **	If 'Session.Type' isn't already set up for a multiplayer game,
                    **	we must prompt the user for which type of multiplayer game
                    **	they want.
                    */
                    case GAME_NORMAL:
                        Session.Type = Select_MPlayer_Game();
                        if (Session.Type == GAME_NORMAL) { // 'Cancel'
                            display = true;
                            selection = SEL_NONE;
                        }
                        break;

                    case GAME_SKIRMISH:
#ifndef REMASTER_BUILD
                        if (!Com_Scenario_Dialog(true)) {
                            Session.Type = Select_MPlayer_Game();
                            if (Session.Type == GAME_NORMAL) { // user hit Cancel
                                display = true;
                                selection = SEL_NONE;
                            }
                        } else {
                            //	Ever hits? Session.Type set to GAME_SKIRMISH without user selecting in
                            // Select_MPlayer_Game();
#ifdef FIXIT_VERSION_3
                            //	If mission is Counterstrike, CS CD will be required. But aftermath units require AM CD.
                            bAftermathMultiplayer =
                                Is_Aftermath_Installed() && !Is_Mission_Counterstrike(Scen.ScenarioName);
                            //	ajw I'll bet this was needed before also...
                            Session.ScenarioIsOfficial =
                                Session.Scenarios[Session.Options.ScenarioIndex]->Get_Official();
#endif
                        }
#endif // PG
                        break;

#ifndef WOLAPI_INTEGRATION
                    /*
                    ** Handle being spawned from WChat. Internet play based on IPX code.
                    */
                    case GAME_INTERNET:
                        break;

#endif //	!WOLAPI_INTEGRATION
                    }
#ifdef WOLAPI_INTEGRATION
                } //	if( !pWolapi )

                if (pWolapi)
                    Session.Type = GAME_INTERNET;
#endif
                // debugprint( "Session.Type = %i\n", Session.Type );
                switch (Session.Type) {
#ifndef WOLAPI_INTEGRATION
                case GAME_INTERNET:
#endif
                case GAME_SKIRMISH:
                    Theme.Fade_Out();
                    process = false;
#ifdef FIXIT_VERSION_3
                    Options.ScoreVolume = Options.MultiScoreVolume;
#else
                    Options.ScoreVolume = 0;
#endif
                    break;

#ifdef WOLAPI_INTEGRATION //	implies also WINSOCK_IPX
                case GAME_INTERNET:
                    if (PacketTransport)
                        delete PacketTransport;
#ifdef NETWORKING
                    PacketTransport = new UDPInterfaceClass;
#endif
                    assert(PacketTransport != NULL);
                    if (PacketTransport->Init()) {
                        switch (WOL_Main()) {
                        case 1:
                            //	Start game.
#ifdef FIXIT_VERSION_3
                            Options.ScoreVolume = Options.MultiScoreVolume;
#else
                            Options.ScoreVolume = 0;
#endif
                            process = false;
                            Theme.Fade_Out();
                            break;
                        case 0:
                            //	User cancelled.
                            Session.Type = GAME_NORMAL;
                            display = true;
                            selection = SEL_MULTIPLAYER_GAME; // SEL_NONE;
                            delete PacketTransport;
                            PacketTransport = NULL;
                            break;
                        case -1:
                            //	Patch was downloaded. Exit app.
                            Theme.Fade_Out();
                            BlackPalette.Set(FADE_PALETTE_SLOW);
                            return false;
                        }
                    } else {
                        Session.Type = GAME_NORMAL;
                        display = true;
                        selection = SEL_MULTIPLAYER_GAME; // SEL_NONE;
                        delete PacketTransport;
                        PacketTransport = NULL;
                    }
                    break;
#endif

                /*
                **	Network (IPX): start a new network game.
                */
                case GAME_IPX:
                    WWDebugString("RA95 - Game type is IPX.\n");
                    /*
                    ** Init network system & remote-connect
                    */
#ifdef NETWORKING
                    if (PacketTransport)
                        delete PacketTransport;

                    PacketTransport = new UDPInterfaceClass;
                    assert(PacketTransport != NULL);
#endif

                    WWDebugString("RA95 - About to call Init_Network.\n");
                    if (Session.Type == GAME_IPX && Init_Network() && Remote_Connect()) {
#ifdef FIXIT_VERSION_3
                        Options.ScoreVolume = Options.MultiScoreVolume;
#else
                        Options.ScoreVolume = 0;
#endif
                        process = false;
                        Theme.Fade_Out();
                    } else { // user hit cancel, or init failed
                        Session.Type = GAME_NORMAL;
                        display = true;
                        selection = SEL_NONE;
#ifdef NETWORKING
                        delete PacketTransport;
                        PacketTransport = NULL;
#endif
                    }
                    break;
                }
                break;

            /*
            **	Play a VQ
            */
            case SEL_INTRO:
                Theme.Fade_Out();
                if (Debug_Flag) {
                    Play_Intro(Debug_Flag);
                } else {
                    Hide_Mouse();
                    VisiblePage.Clear();
                    Show_Mouse();
                    Play_Movie(VQ_INTRO_MOVIE, THEME_NONE, true); // no transition picture to briefing
                    Keyboard->Clear();
                    Play_Movie(VQ_SIZZLE, THEME_NONE, true);
                    Play_Movie(VQ_SIZZLE2, THEME_NONE, true);
                    //						Play_Movie(VQ_INTRO_MOVIE, THEME_NONE, false);		// has transitino picture to
                    //briefing
                }
                Theme.Queue_Song(THEME_CRUS);
                display = true;
                fade = true;
                selection = SEL_NONE;
                break;

            /*
            **	Exit to DOS.
            */
            case SEL_EXIT:
                Theme.Fade_Out();
                BlackPalette.Set(FADE_PALETTE_SLOW);
                return (false);

            /*
            **	Display the hall of fame.
            */
            case SEL_FAME:
                break;

            case SEL_TIMEOUT:
                if (Session.Attract && Session.RecordFile.Is_Available()) {
                    Session.Play = true;
                    if (Session.RecordFile.Open(READ)) {
                        Load_Recording_Values(Session.RecordFile);
                        process = false;
                        Theme.Fade_Out();
                    } else {
                        Session.Play = false;
                        selection = SEL_NONE;
                    }
                } else {
                    selection = SEL_NONE;
                }
                break;

            default:
                break;
            }
        }
    } else {

        /*
        ** For Debug_Map (editor) mode to load scenario
        */
        Scen.Set_Scenario_Name("SCG01EA.INI");
    }

    /*
    **	Don't carry stray keystrokes into game.
    */
    Keyboard->Clear();

    /*
    ** Initialize the random number generator(s)
    */
    Init_Random();

    /*
    ** Save initialization values if we're recording this game.
    */
    if (Session.Record) {
        if (Session.RecordFile.Open(WRITE)) {
            Save_Recording_Values(Session.RecordFile);
        } else {
            Session.Record = false;
        }
    }

#ifdef FIXIT_CSII //	checked - ajw 9/28/98
    switch (Session.Type) {
    case GAME_IPX:
#ifdef FIXIT_VERSION_3
        if (!bAftermathMultiplayer) {
#else
        if (PlayingAgainstVersion < VERSION_AFTERMATH_CS) {
#endif
            NewUnitsEnabled = SecretUnitsEnabled = false;
        } else {
            NewUnitsEnabled = true;
        }
        //			debugprint( "Non Internet game: NewUnitsEnabled = %i\n", NewUnitsEnabled );
        break;
    case GAME_INTERNET:
#if (0)
        if (!pWolapi) {
            //				debugprint( "pWolapi is null on internet game!" );
            Fatal("pWolapi is null on internet game!");
        }
        // if( pWolapi->bEnableNewAftermathUnits )
        if (bAftermathMultiplayer)
            NewUnitsEnabled = true;
        else
            NewUnitsEnabled = SecretUnitsEnabled = false;
//			debugprint( "Internet game: NewUnitsEnabled = %i\n", NewUnitsEnabled );
#endif
        break;
    default:
        break;
    }
#endif
    /*
    **	Load the scenario.  Specify variation 'A' for the editor; for the game,
    **	don't specify a variation, to make 'Set_Scenario_Name()' pick a random one.
    **	Skip this if we've already loaded a save-game.
    */
    if (!gameloaded && !Session.LoadGame) {
        //		if (Debug_Map) {
        //			Set_Scenario_Name(Scen.ScenarioName, Scen.Scenario, Scen.ScenPlayer, Scen.ScenDir, SCEN_VAR_A);
        //		}  else {
        //			Set_Scenario_Name(Scen.ScenarioName, Scen.Scenario, Scen.ScenPlayer, Scen.ScenDir);
        //		}

        /*
        ** Start_Scenario() changes the palette; so, fade out & clear the screen
        ** before calling it.
        */
        Hide_Mouse();

        if (selection != SEL_START_NEW_GAME) {
            BlackPalette.Set(FADE_PALETTE_MEDIUM, Call_Back);
            HiddenPage.Clear();
            VisiblePage.Clear();
        }
        Show_Mouse();
        // Mono_Printf("About to call Start Scenario with %s\n", Scen.ScenarioName);
        if (!Start_Scenario(Scen.ScenarioName)) {
            return (false);
        }
        if (Special.IsFromInstall)
            Show_Mouse();
        Special.IsFromInstall = false;
    }

    /*
    **	For multiplayer games, initialize the inter-player message system.
    **	Do this after loading the scenario, so the map's upper-left corner is
    **	properly set.
    */
    Session.Messages.Init(Map.TacPixelX,
                          Map.TacPixelY,           // x,y for messages
                          6,                       // max # msgs
                          MAX_MESSAGE_LENGTH - 14, // max msg length
                          7 * RESFACTOR,           // font height in pixels
                          -1,
                          -1,                                   // x,y for edit line (appears above msgs)
                          0,                                    // BG		1,							// enable edit overflow
                          20,                                   // min,
                          MAX_MESSAGE_LENGTH - 14,              //    max for trimming overflow
                          Lepton_To_Pixel(Map.TacLeptonWidth)); // Width in pixels of buffer

    if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH && !Session.Play) {
        Session.Create_Connections();
    }

    /*
    ** If this isnt an internet game that set the unit build rate to its default value
    */
    if (Session.Type != GAME_INTERNET) {
        UnitBuildPenalty = 100;
    }

    /*
    **	Hide the SeenPage; force the map to render one frame.  The caller can
    **	then fade the palette in.
    **	(If we loaded a game, this step will fade out the title screen.  If we
    **	started a scenario, Start_Scenario() will have played a couple of VQ
    **	movies, which will have cleared the screen to black already.)
    */
    Call_Back();
    Hide_Mouse();
    BlackPalette.Set(FADE_PALETTE_MEDIUM, Call_Back);
    //	Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
    HiddenPage.Clear();
    VisiblePage.Clear();
    Show_Mouse();

    Set_Logic_Page(SeenBuff);
    /*
    ** Sidebar is always active in hi-res.
    */
    if (!Debug_Map && !Options.ToggleSidebar) {
        Map.SidebarClass::Activate(1);
    }
    Map.Flag_To_Redraw();
    Call_Back();
    Map.Render();

    return (true);
}

/***********************************************************************************************
 * Play_Intro -- plays the introduction & logo movies                                          *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *		none. *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *		none. *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/06/1995 BRR : Created.                                                                 *
 *   05/08/1996 JLB : Modified for Red Alert and direction control.                            *
 *=============================================================================================*/
static void Play_Intro(bool sequenced)
{
    static VQType _counter = VQ_FIRST;

    Keyboard->Clear();
    if (sequenced) {
        if (_counter <= VQ_FIRST)
            _counter = VQ_COUNT;
        if (_counter == VQ_COUNT)
            _counter--;
        if (_counter == VQ_REDINTRO)
            _counter--;
        if (_counter == VQ_TITLE)
            _counter--;
        Hide_Mouse();
        VisiblePage.Clear();
        Show_Mouse();
        Play_Movie(VQType(_counter--), THEME_NONE);

        //		Show_Mouse();
    } else {
        Hide_Mouse();
        VisiblePage.Clear();
        Show_Mouse();
        Play_Movie(VQ_REDINTRO, THEME_NONE, false);
    }
}

/***********************************************************************************************
 * Anim_Init -- Initialize the VQ animation control structure.                                 *
 *                                                                                             *
 *    VQ animations are controlled by a structure passed to the VQ player. This routine        *
 *    initializes the structure to values required by C&C.                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Only need to call this routine once at the beginning of the game.               *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/20/1994 JLB : Created.                                                                 *
 *=============================================================================================*/

#ifdef DSOUND_BUILD
extern LPDIRECTSOUND SoundObject;
extern LPDIRECTSOUNDBUFFER PrimaryBufferPtr;
#endif
#ifdef MOVIE640
GraphicBufferClass VQ640(640, 400, nullptr);
#endif
void Anim_Init(void)
{
#ifndef REMASTER_BUILD
    /* Configure player with INI file */
    VQA_DefaultConfig(&AnimControl);
    AnimControl.DrawFlags = VQACFGF_TOPLEFT;
    AnimControl.DrawFlags |= VQACFGF_BUFFER;
    // AnimControl.DrawFlags |= VQACFGF_NODRAW;
    // BG - M. Grayford says turn this off	AnimControl.DrawFlags |= VQACFGF_NOSKIP;

    AnimControl.DrawFlags |= VQACFGF_NOSKIP;
    AnimControl.FrameRate = -1;
    AnimControl.DrawRate = -1;
    AnimControl.DrawerCallback = VQ_Call_Back;
    AnimControl.ImageWidth = 320;
    AnimControl.ImageHeight = 200;
    AnimControl.ImageBuf = (unsigned char*)SysMemPage.Get_Offset();
#ifdef MOVIE640
    if (IsVQ640) {
        AnimControl.ImageWidth = 640;
        AnimControl.ImageHeight = 400;
        AnimControl.ImageBuf = (unsigned char*)VQ640.Get_Offset();
    }
#endif
    AnimControl.Vmode = 0;
    AnimControl.OptionFlags |= VQAOPTF_CAPTIONS | VQAOPTF_EVA;
    if (SlowPalette) {
        AnimControl.OptionFlags |= VQAOPTF_SLOWPAL;
    }
#ifdef DSOUND_BUILD
    AnimControl.SoundObject = SoundObject;
    AnimControl.PrimaryBufferPtr = PrimaryBufferPtr;
#endif
    if (MonoClass::Is_Enabled()) {
        AnimControl.OptionFlags |= VQAOPTF_MONO;
    }
#endif
}

/***********************************************************************************************
 * Parse_Command_Line -- Parses the command line parameters.                                   *
 *                                                                                             *
 *    This routine should be called before the graphic mode is initialized. It examines the    *
 *    command line parameters and sets the appropriate globals. If there is an error, then     *
 *    it outputs a command summary and then returns false.                                     *
 *                                                                                             *
 * INPUT:   argc  -- The number of command line arguments.                                     *
 *                                                                                             *
 *          argv  -- Pointer to character string array that holds the individual arguments.    *
 *                                                                                             *
 * OUTPUT:  bool; Was the command line parsed successfully?                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/18/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Parse_Command_Line(int argc, char* argv[])
{
    /*
    **	Parse the command line and set globals to reflect the parameters
    **	passed in.
    */
    Whom = HOUSE_GOOD;
    Special.Init();

    Debug_Map = false;
    Debug_Unshroud = false;

    for (int index = 1; index < argc; index++) {
        char* string; // Pointer to argument.
        long code = 0;

        char arg_string[512];
        int str_len = strlen(argv[index]);
        char* src = argv[index];
        char* dest = arg_string;
        for (int i = 0; i < str_len; i++) {
            if (*src == '\"') {
                src++;
            } else {
                *dest++ = *src++;
            }
        }
        *dest++ = 0;
        string = arg_string;
        strupr(string);

        // string = strupr(argv[index]);

        /*
        **	Print usage text only if requested.
        */
        if (stricmp("/?", string) == 0 || stricmp("-?", string) == 0 || stricmp("-h", string) == 0
            || stricmp("/h", string) == 0) {
            /*
            **	Unrecognized command line parameter... Display usage
            **	and then exit.
            */
            puts(TEXT_OPTIONS);
            return (false);
        }

        bool processed = true;
        long ob = Obfuscate(string);

        /*
        **	Check to see if the parameter is a cheat enabling one.
        */
        long const* optr = (const long*)&CheatCodes[0];
        while (*optr) {
            if (*optr++ == ob) {
                Debug_Playtest = true;
                Debug_Flag = true;
                break;
            }
        }

        /*
        **	Check to see if the parameter is a cheat enabling one.
        */
        optr = (const long*)&PlayCodes[0];
        while (*optr) {
            if (*optr++ == ob) {
                Debug_Playtest = true;
                Debug_Flag = true;
                break;
            }
        }

        /*
        **	Check to see if the parameter is a scenario editor
        **	enabling one.
        */
        optr = (const long*)&EditorCodes[0];
        while (*optr) {
            if (*optr++ == ob) {
                Debug_Map = true;
                Debug_Unshroud = true;
                Debug_Flag = true;
                Debug_Playtest = true;
                break;
            }
        }

        switch (ob) {

#ifdef VIRGIN_CHEAT_KEYS
        case PARM_PLAYTEST:
            Debug_Playtest = true;
            break;
#endif

        /*
        ** Special flag - is C&C being run from the install program?
        */
        case PARM_INSTALL:
            Special.IsFromInstall = true;
            // If uncommented, will disable the <ESC> key during the first movie run.
            //				BreakoutAllowed = false;
            break;

        default:
            processed = false;
            break;
        }
        if (processed)
            continue;

#ifdef CHEAT_KEYS
        /*
        **	Scenario Editor Mode
        */
        if (stricmp(string, "-CHECKMAP") == 0) {
            Debug_Check_Map = true;
            continue;
        }

#endif

        /*
        **	File search path override.
        */
        if (strstr(string, "-CD")) {
            CCFileClass::Set_Search_Drives(&string[3]);
            continue;
        }

#if (0)
        /*
        ** Build speed modifier
        */
        if (strstr(string, "-UNITRATE:")) {
            int unit_rate;
            sscanf(string, "-UNITRATE:%d", &unit_rate);
            UnitBuildPenalty = unit_rate;
        }
#endif //(0)

        /*
        **	Specify destination connection for network play
        */
        if (strstr(string, "-DESTNET")) {
            NetNumType net;
            NetNodeType node;

            /*
            ** Scan the command-line string, pulling off each address piece
            */
            int i = 0;
            char* p = strtok(string + 8, ".");
            while (p) {
                int x;

                sscanf(p, "%x", &x); // convert from hex string to int
                if (i < 4) {
                    net[i] = (char)x; // fill NetNum
                } else {
                    node[i - 4] = (char)x; // fill NetNode
                }
                i++;
                p = strtok(NULL, ".");
            }

            /*
            ** If all the address components were successfully read, fill in the
            ** BridgeNet with a broadcast address to the network across the bridge.
            */
            if (i >= 4) {
                Session.IsBridge = 1;
                memset(node, 0xff, 6);
                Session.BridgeNet = IPXAddressClass(net, node);
            }
            continue;
        }

        /*
        **	Specify socket ID, as an offset from 0x4000.
        */
        if (strstr(string, "-SOCKET")) {
            unsigned short socket;

            socket = (unsigned short)(atoi(string + strlen("SOCKET")));
            socket += 0x4000;
            if (socket >= 0x4000 && socket < 0x8000) {
                Ipx.Set_Socket(socket);
            }
            continue;
        }

        /*
        **	Set the Net Stealth option
        */
        if (strstr(string, "-STEALTH")) {
            Session.NetStealth = true;
            continue;
        }

        /*
        **	Set the Net Protection option
        */
        if (strstr(string, "-MESSAGES")) {
            Session.NetProtect = false;
            continue;
        }

        /*
        **	Allow "attract" mode
        */
        if (strstr(string, "-ATTRACT")) {
            Session.Attract = true;
            continue;
        }

        /*
        ** Set screen to 640x480 instead of 640x400
        */
        if (strstr(string, "-480")) {
            ScreenHeight = 480;
            continue;
        }

#ifdef CHEAT_KEYS
        /*
        **	Specify the random number seed (for debugging)
        */
        if (strstr(string, "-SEED")) {
            CustomSeed = (unsigned short)(atoi(string + strlen("SEED")));
            continue;
        }
#endif

#ifdef NEVER
        /*
        **	Handle the prog init differently in this case.
        */
        if (strstr(string, "-V")) {
            continue;
        }
#endif

#ifdef CHEAT_KEYS
        if (strstr(string, "-NOMOVIES")) {
            bNoMovies = true;
        }
#endif

        /*
        **	Special command line control parsing.
        */
        if (strnicmp(string, "-X", strlen("-O")) == 0) {
            string += strlen("-X");
            while (*string) {
                char code = *string++;
                switch (toupper(code)) {

#ifdef CHEAT_KEYS
                /*
                **	Monochrome debug screen enable.
                */
                case 'M':
                    MonoClass::Enable();
                    break;

                /*
                **	Inert weapons -- no units take damage.
                */
                case 'I':
                    Special.IsInert = true;
                    break;

                /*
                **	Hussled recharge timer.
                */
                case 'H':
                    Special.IsSpeedBuild = true;
                    break;

                /*
                **	"Record" a multi-player game
                */
                case 'X':
                    Session.Record = 1;
                    break;

                /*
                **	"Play Back" a multi-player game
                */
                case 'Y':
                    Session.Play = 1;
                    break;

                /*
                **	Print lots of debug stuff about events & packets
                */
                case 'P':
                    Debug_Print_Events = true;
                    break;
#endif

                /*
                **	Quiet mode override control.
                */
                case 'Q':
                    Debug_Quiet = true;
                    break;

                default:
                    puts(TEXT_INVALID);
                    return (false);
                }
            }

            continue;
        }
    }
    return (true);
}

/***********************************************************************************************
 * Obfuscate -- Sufficiently transform parameter to thwart casual hackers.                     *
 *                                                                                             *
 *    This routine borrows from CRC and PGP technology to sufficiently alter the parameter     *
 *    in order to make it difficult to reverse engineer the key phrase. This is designed to    *
 *    be used for hidden game options that will be released at a later time over Westwood's    *
 *    Web page or through magazine hint articles.                                              *
 *                                                                                             *
 *    This algorithm is cryptographically categorized as a "one way hash".                     *
 *                                                                                             *
 *    Since this is a one way transformation, it becomes much more difficult to reverse        *
 *    engineer the pass phrase even if the resultant pass code is known. This has an added     *
 *    benefit of making this algorithm immune to traditional cryptographic attacks.            *
 *                                                                                             *
 *    The largest strength of this transformation algorithm lies in the restriction on the     *
 *    source vector being legal ASCII uppercase characters. This restriction alone makes even  *
 *    a simple CRC transformation practically impossible to reverse engineer. This algorithm   *
 *    uses far more than a simple CRC transformation to achieve added strength from advanced   *
 *    attack methods.                                                                          *
 *                                                                                             *
 * INPUT:   string   -- Pointer to the key phrase that will be transformed into a code.        *
 *                                                                                             *
 * OUTPUT:  Returns with the code that the key phrase is translated into.                      *
 *                                                                                             *
 * WARNINGS:   A zero length pass phrase results in a 0x00000000 result code.                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
long Obfuscate(char const* string)
{
    char buffer[1024];

    if (!string)
        return (0);
    memset(buffer, '\xA5', sizeof(buffer));

    /*
    **	Copy key phrase into a working buffer. This hides any transformation done
    **	to the string.
    */
    strncpy(buffer, string, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    int length = strlen(buffer);

    /*
    **	Only upper case letters are significant.
    */
    strupr(buffer);

    /*
    **	Ensure that only visible ASCII characters compose the key phrase. This
    **	discourages the direct forced illegal character input method of attack.
    */
    int index;
    for (index = 0; index < length; index++) {
        if (!isgraph(buffer[index])) {
            buffer[index] = 'A' + (index % 26);
        }
    }

    /*
    **	Increase the strength of even short pass phrases by extending the
    **	length to be at least a minimum number of characters. This helps prevent
    **	a weak pass phrase from compromising the obfuscation process. This
    **	process also forces the key phrase to be an even multiple of four.
    **	This is necessary to support the cypher process that occurs later.
    */
    if (length < 16 || (length & 0x03)) {
        int maxlen = 16;
        if (((length + 3) & 0x00FC) > maxlen) {
            maxlen = ((length + 3) & 0x00FC);
        }
        for (index = length; index < maxlen; index++) {
            buffer[index] = 'A' + ((('?' ^ buffer[index - length]) + index) % 26);
        }
        length = index;
        buffer[length] = '\0';
    }

    /*
    **	Transform the buffer into a number. This transformation is character
    **	order dependant.
    */
    long code = Calculate_CRC(buffer, length);

    /*
    **	Record a copy of this initial transformation to be used in a later
    **	self referential transformation.
    */
    long copy = code;

    /*
    **	Reverse the character string and combine with the previous transformation.
    **	This doubles the workload of trying to reverse engineer the CRC calculation.
    */
    strrev(buffer);
    code ^= Calculate_CRC(buffer, length);

    /*
    **	Perform a self referential transformation. This makes a reverse engineering
    **	by using a cause and effect attack more difficult.
    */
    code = code ^ copy;

    /*
    **	Unroll and combine the code value into the pass phrase and then perform
    **	another self referential transformation. Although this is a trivial cypher
    **	process, it gives the sophisticated hacker false hope since the strong
    **	cypher process occurs later.
    */
    strrev(buffer); // Restore original string order.
    for (index = 0; index < length; index++) {
        code ^= (unsigned char)buffer[index];
        unsigned char temp = (unsigned char)code;
        buffer[index] ^= temp;
        code >>= 8;
        code |= (((long)temp) << 24);
    }

    /*
    **	Introduce loss into the vector. This strengthens the key against traditional
    **	cryptographic attack engines. Since this also weakens the key against
    **	unconventional attacks, the loss is limited to less than 10%.
    */
    for (index = 0; index < length; index++) {
        static unsigned char _lossbits[] = {0x00, 0x08, 0x00, 0x20, 0x00, 0x04, 0x10, 0x00};
        static unsigned char _addbits[] = {0x10, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x04};

        buffer[index] |= _addbits[index % (sizeof(_addbits) / sizeof(_addbits[0]))];
        buffer[index] &= ~_lossbits[index % (sizeof(_lossbits) / sizeof(_lossbits[0]))];
    }

    /*
    **	Perform a general cypher transformation on the vector
    **	and use the vector itself as the cypher key. This is a variation on the
    **	cypher process used in PGP. It is a very strong cypher process with no known
    **	weaknesses. However, in this case, the cypher key is the vector itself and this
    **	opens up a weakness against attacks that have access to this transformation
    **	algorithm. The sheer workload of reversing this transformation should be enough
    **	to discourage even the most determined hackers.
    */
    for (index = 0; index < length; index += 4) {
        short key1 = buffer[index];
        short key2 = buffer[index + 1];
        short key3 = buffer[index + 2];
        short key4 = buffer[index + 3];
        short val1 = key1;
        short val2 = key2;
        short val3 = key3;
        short val4 = key4;

        val1 *= key1;
        val2 += key2;
        val3 += key3;
        val4 *= key4;

        short s3 = val3;
        val3 ^= val1;
        val3 *= key1;
        short s2 = val2;
        val2 ^= val4;
        val2 += val3;
        val2 *= key3;
        val3 += val2;

        val1 ^= val2;
        val4 ^= val3;

        val2 ^= s3;
        val3 ^= s2;

        buffer[index] = (char)val1;
        buffer[index + 1] = (char)val2;
        buffer[index + 2] = (char)val3;
        buffer[index + 3] = (char)val4;
    }

    /*
    **	Convert this final vector into a cypher key code to be
    **	returned by this routine.
    */
    code = Calculate_CRC(buffer, length);

    /*
    **	Return the final code value.
    */
    return (code);
}

/***************************************************************************
 * Init_Random -- Initializes the random-number generator                  *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
void Init_Random(void)
{
    /*
    ** Initialize RNG with the current system time in seconds.
    */
    CryptRandom.Seed_Long(time(NULL));

#ifdef FIXIT_MULTI_SAVE
    //
    // If we've loaded a multiplayer save game, return now; the random #
    // class is loaded along with ScenarioClass.
    //
    if (Session.LoadGame) {
        return;
    }

    //
    // If we're playing a recording, the Seed is loaded in
    // Load_Recording_Values().  Just init the random # and return.
    //
    if (Session.Play) {
        RandNumb = Seed;
        Scen.RandomNumber = Seed;
        return;
    }
#else
    /*
    ** Do nothing if we've loaded a multiplayer game, or we're playing back
    ** a recording; the random number generator is initialized by loading
    ** the game.
    */
    if (Session.LoadGame || Session.Play) {
        RandNumb = Seed;
        Scen.RandomNumber = Seed;
        return;
    }
#endif // FIXIT_MULTI_SAVE

    /*
    **	Initialize the random number Seed.  For multiplayer, this will have been done
    ** in the connection dialogs.  For single-player games, AND if we're not playing
    ** back a recording, init the Seed to a random value.
    */
    if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH && !Session.Play) {

        /*
        ** Set the optional user-specified seed
        */
        if (CustomSeed != 0) {
            Seed = CustomSeed;
        } else {
            srand((unsigned)time(NULL));
            Seed = rand();
        }
    }

    /*
    **	Initialize the random-number generators
    */
    Scen.RandomNumber = Seed;
    RandNumb = Seed;
}

/***********************************************************************************************
 * Load_Title_Page -- Load the background art for the title page.                              *
 *                                                                                             *
 *    This routine will load the background art in a machine independent format. There is      *
 *    different art required for the hi-res and lo-res versions of the game.                   *
 *                                                                                             *
 * INPUT:   visible  -- Should the title page art be copied to the visible page by this        *
 *                      routine?                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Be sure the mouse is hidden if the image is to be copied to the visible page.   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void Load_Title_Page(bool visible)
{
    Load_Title_Screen("TITLE.PCX", &HidPage, (unsigned char*)CCPalette.Get_Data());
    if (visible) {
        HidPage.Blit(SeenPage);
    }
}

/***********************************************************************************************
 * Init_Color_Remaps -- Initialize the text remap tables.                                      *
 *                                                                                             *
 *    There are various color scheme remap tables that are dependant upon the color remap      *
 *    information embedded within the palette control file. This routine will fetch that       *
 *    data and build the text remap tables as indicated.                                       *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *   09/11/2019 ST : The default resolution doesn't have to match the size of the palette image*
 *=============================================================================================*/
static void Init_Color_Remaps(void)
{
    /*
    **	Setup the remap tables.  PALETTE.CPS contains a special set of pixels in
    ** the upper-left corner.  Each row of 16 pixels is one range of colors.  The
    ** first row represents unity (the default color units are drawn in); rows
    ** after that are the remap colors.
    */

    GraphicBufferClass temp_page(320, 200, (void*)NULL);
    temp_page.Clear();
    Load_Picture("PALETTE.CPS", temp_page, temp_page, NULL, BM_DEFAULT);
    temp_page.Blit(HidPage);
    for (PlayerColorType pcolor = PCOLOR_FIRST; pcolor < PCOLOR_COUNT; pcolor++) {

        unsigned char* ptr = ColorRemaps[pcolor].RemapTable;

        for (int color = 0; color < 256; color++) {
            ptr[color] = color;
        }

        int index;
        for (index = 0; index < 16; index++) {
            ptr[HidPage.Get_Pixel(index, 0)] = HidPage.Get_Pixel(index, pcolor);
        }
        for (index = 0; index < 6; index++) {
            ColorRemaps[pcolor].FontRemap[10 + index] = HidPage.Get_Pixel(2 + index, pcolor);
        }
        ColorRemaps[pcolor].BrightColor = WHITE;
        //		ColorRemaps[pcolor].BrightColor = HidPage.Get_Pixel(1, pcolor);
        ColorRemaps[pcolor].Color = HidPage.Get_Pixel(4, pcolor);

        ColorRemaps[pcolor].Shadow = HidPage.Get_Pixel(10, pcolor);
        ColorRemaps[pcolor].Background = HidPage.Get_Pixel(9, pcolor);
        ColorRemaps[pcolor].Corners = HidPage.Get_Pixel(7, pcolor);
        ColorRemaps[pcolor].Highlight = HidPage.Get_Pixel(4, pcolor);
        ColorRemaps[pcolor].Bright = HidPage.Get_Pixel(0, pcolor);
        ColorRemaps[pcolor].Underline = HidPage.Get_Pixel(0, pcolor);
        ColorRemaps[pcolor].Bar = HidPage.Get_Pixel(6, pcolor);

        /*
        **	This must grab from column 4 because the multiplayer color dialog palette counts
        **	on this to be true.
        */
        ColorRemaps[pcolor].Box = HidPage.Get_Pixel(4, pcolor);
    }

    /* 12/9/2019 SKY - Swap Blue and Grey color remaps */
    {
        RemapControlType temp;
        memcpy(&temp, &ColorRemaps[PCOLOR_BLUE], sizeof(RemapControlType));
        memcpy(&ColorRemaps[PCOLOR_BLUE], &ColorRemaps[PCOLOR_GREY], sizeof(RemapControlType));
        memcpy(&ColorRemaps[PCOLOR_GREY], &temp, sizeof(RemapControlType));
    }

    /*
    ** Now do the special dim grey scheme
    */
    for (int color = 0; color < 256; color++) {
        GreyScheme.RemapTable[color] = color;
    }
    for (int index = 0; index < 6; index++) {
        GreyScheme.FontRemap[10 + index] = HidPage.Get_Pixel(9 + index, PCOLOR_GREY) & 0x00FF;
    }
    GreyScheme.BrightColor = HidPage.Get_Pixel(3, PCOLOR_GREY) & 0x00FF;
    GreyScheme.Color = HidPage.Get_Pixel(7, PCOLOR_GREY) & 0x00FF;

    GreyScheme.Shadow = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(15, PCOLOR_GREY) & 0x00FF];
    GreyScheme.Background = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(14, PCOLOR_GREY) & 0x00FF];
    GreyScheme.Corners = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(13, PCOLOR_GREY) & 0x00FF];
    GreyScheme.Highlight = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(9, PCOLOR_GREY) & 0x00FF];
    GreyScheme.Bright = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(5, PCOLOR_GREY) & 0x00FF];
    GreyScheme.Underline = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(5, PCOLOR_GREY) & 0x00FF];
    GreyScheme.Bar = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(11, PCOLOR_GREY) & 0x00FF];
    GreyScheme.Box = ColorRemaps[PCOLOR_GREY].RemapTable[HidPage.Get_Pixel(11, PCOLOR_GREY) & 0x00FF];

    /*
    ** Set up the metallic remap table for the font that prints over the tabs
    */
    memset((void*)&MetalScheme, 4, sizeof(MetalScheme));
    for (int color_counter = 0; color_counter < 16; color_counter++) {
        MetalScheme.FontRemap[color_counter] = color_counter;
    }
    MetalScheme.FontRemap[1] = 128;
    MetalScheme.FontRemap[2] = 12;
    MetalScheme.FontRemap[3] = 13;
    MetalScheme.FontRemap[4] = 14;
    MetalScheme.Color = 128;
    MetalScheme.Background = 0;
    MetalScheme.Underline = 128;

    /*
    ** Set up the font remap table for the mission briefing font
    */
    for (int colr = 0; colr < 16; colr++) {
        ColorRemaps[PCOLOR_TYPE].FontRemap[colr] = HidPage.Get_Pixel(colr, PCOLOR_TYPE);
    }

    ColorRemaps[PCOLOR_TYPE].Shadow = 11;
    ColorRemaps[PCOLOR_TYPE].Background = 10;
    ColorRemaps[PCOLOR_TYPE].Corners = 10;
    ColorRemaps[PCOLOR_TYPE].Highlight = 9;
    ColorRemaps[PCOLOR_TYPE].Bright = 15;
    ColorRemaps[PCOLOR_TYPE].Underline = 11;
    ColorRemaps[PCOLOR_TYPE].Bar = 11;
    ColorRemaps[PCOLOR_TYPE].Box = 10;
    ColorRemaps[PCOLOR_TYPE].BrightColor = 15;
    ColorRemaps[PCOLOR_TYPE].Color = 9;

    GadgetClass::Set_Color_Scheme(&ColorRemaps[PCOLOR_DIALOG_BLUE]);
    //	GadgetClass::Set_Color_Scheme(&ColorRemaps[PCOLOR_BLUE]);
}

/***********************************************************************************************
 * Init_Heaps -- Initialize the game heaps and buffers.                                        *
 *                                                                                             *
 *    This routine will allocate the game heaps and buffers. The rules file has already been   *
 *    processed by the time that this routine is called.                                       *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Heaps(void)
{
    /*
    **	Initialize the game object heaps.
    */
    Vessels.Set_Heap(Rule.VesselMax);
    Units.Set_Heap(Rule.UnitMax);
    Factories.Set_Heap(Rule.FactoryMax);
    Terrains.Set_Heap(Rule.TerrainMax);
    Templates.Set_Heap(Rule.TemplateMax);
    Smudges.Set_Heap(Rule.SmudgeMax);
    Overlays.Set_Heap(Rule.OverlayMax);
    Infantry.Set_Heap(Rule.InfantryMax);
    Bullets.Set_Heap(Rule.BulletMax);
    Buildings.Set_Heap(Rule.BuildingMax);
    Anims.Set_Heap(Rule.AnimMax);
    Aircraft.Set_Heap(Rule.AircraftMax);
    Triggers.Set_Heap(Rule.TriggerMax);
    TeamTypes.Set_Heap(Rule.TeamTypeMax);
    Teams.Set_Heap(Rule.TeamMax);
    Houses.Set_Heap(HOUSE_MAX);
    TriggerTypes.Set_Heap(Rule.TrigTypeMax);
    //	Weapons.Set_Heap(Rule.WeaponMax);

    /*
    **	Speech holding tank buffer. Since speech does not mix, it can be placed
    **	into a custom holding tank only as large as the largest speech file to
    **	be played.
    */
    for (int index = 0; index < ARRAY_SIZE(SpeechBuffer); index++) {
        SpeechBuffer[index] = new char[SPEECH_BUFFER_SIZE];
        SpeechRecord[index] = VOX_NONE;
        assert(SpeechBuffer[index] != NULL);
    }

    /*
    **	Allocate the theater buffer block.
    */
    TheaterBuffer = new Buffer(THEATER_BUFFER_SIZE);
    assert(TheaterBuffer != NULL);
}

/***********************************************************************************************
 * Init_Expansion_Files -- Fetch any override expansion mixfiles.                              *
 *                                                                                             *
 *    This routine will search for and register/cache any override mixfiles found.             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Expansion_Files(void)
{
    /*
    ** Need to search the search paths. ST - 3/15/2019 2:18PM
    */
    const char* path = ".\\";
    char search_path[_MAX_PATH];
    char scan_path[_MAX_PATH];

#ifdef _WIN32
    for (int p = 0; p < 100; p++) {

        strcpy(search_path, path);
        if (search_path[strlen(search_path) - 1] != '\\') {
            strcat(search_path, "\\");
        }

        strcpy(scan_path, search_path);
        strcat(scan_path, "SC*.MIX");

        WIN32_FIND_DATA find_data;
        memset(&find_data, 0, sizeof(find_data));
        HANDLE file_handle = FindFirstFile(scan_path, &find_data);
        if (file_handle != INVALID_HANDLE_VALUE) {
            do {
                char* ptr = strdup(find_data.cFileName);
                new MFCD(ptr, &FastKey);
            } while (FindNextFile(file_handle, &find_data));
            FindClose(file_handle);
        }

        memset(&find_data, 0, sizeof(find_data));
        strcpy(scan_path, search_path);
        strcat(scan_path, "Ss*.MIX");
        file_handle = FindFirstFile(scan_path, &find_data);
        if (file_handle != INVALID_HANDLE_VALUE) {
            do {
                char* ptr = strdup(find_data.cFileName);
                new MFCD(ptr, &FastKey);
            } while (FindNextFile(file_handle, &find_data));
            FindClose(file_handle);
        }

        path = CDFileClass::Get_Search_Path(p);

        if (path == NULL) {
            break;
        }
    }
#endif

#if (0)
    /*
    **	Before all else, cache any additional mixfiles.
    */
    struct find_t ff; // for _dos_findfirst
    if (!_dos_findfirst("SC*.MIX", _A_NORMAL, &ff)) {
        char* ptr;
        do {
            ptr = strdup(ff.name);
            new MFCD(ptr, &FastKey);
            MFCD::Cache(ptr);
        } while (!_dos_findnext(&ff));
    }
    if (!_dos_findfirst("SS*.MIX", _A_NORMAL, &ff)) {
        char* ptr;
        do {
            ptr = strdup(ff.name);
            new MFCD(ptr, &FastKey);
        } while (!_dos_findnext(&ff));
    }
#endif
}

/***********************************************************************************************
 * Init_One_Time_Systems -- Initialize internal pointers to the bulk data.                     *
 *                                                                                             *
 *    This performs the one-time processing required after the bulk data has been cached but   *
 *    before the game actually starts. Typically, this routine extracts pointers to all the    *
 *    embedded data sub-files within the main game data mixfile. This routine must be called   *
 *    AFTER the bulk data has been cached.                                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Call this routine AFTER the bulk data has been cached.                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_One_Time_Systems(void)
{
    Call_Back();
    Map.One_Time();
    Logic.One_Time();
    Options.One_Time();
    Session.One_Time();

    ObjectTypeClass::One_Time();
    BuildingTypeClass::One_Time();
    BulletTypeClass::One_Time();
    HouseTypeClass::One_Time();
    TemplateTypeClass::One_Time();
    OverlayTypeClass::One_Time();
    SmudgeTypeClass::One_Time();
    TerrainTypeClass::One_Time();
    UnitTypeClass::One_Time();
    VesselTypeClass::One_Time();
    InfantryTypeClass::One_Time();
    AnimTypeClass::One_Time();
    AircraftTypeClass::One_Time();
    HouseClass::One_Time();
}

/***********************************************************************************************
 * Init_Fonts -- Initialize all the game font pointers.                                        *
 *                                                                                             *
 *    This routine is used to fetch pointers to the game fonts. The mixfile containing these   *
 *    fonts must have been previously cached. This routine is a necessary prerequisite to      *
 *    displaying any dialogs or printing any text.                                             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Fonts(void)
{
    Metal12FontPtr = MFCD::Retrieve("12METFNT.FNT");
    MapFontPtr = MFCD::Retrieve("HELP.FNT");
    Font6Ptr = MFCD::Retrieve("6POINT.FNT");
    GradFont6Ptr = MFCD::Retrieve("GRAD6FNT.FNT");
    EditorFont = MFCD::Retrieve("EDITFNT.FNT");
    Font8Ptr = MFCD::Retrieve("8POINT.FNT");
    FontPtr = (char*)Font8Ptr;
    Set_Font(FontPtr);
    Font3Ptr = MFCD::Retrieve("3POINT.FNT");
    ScoreFontPtr = MFCD::Retrieve("SCOREFNT.FNT");
    FontLEDPtr = MFCD::Retrieve("LED.FNT");
    VCRFontPtr = MFCD::Retrieve("VCR.FNT");
    TypeFontPtr = MFCD::Retrieve("8POINT.FNT"); //("TYPE.FNT"); //VG 10/17/96
}

/***********************************************************************************************
 * Init_CDROM_Access -- Initialize the CD-ROM access handler.                                  *
 *                                                                                             *
 *    This routine is called to setup the CD-ROM access or emulation handler. It will ensure   *
 *    that the appropriate CD-ROM is present (dependant on the RequiredCD global).             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   The fonts, palettes, and other bootstrap systems must have been initialized     *
 *             prior to calling this routine since this routine will quite likely display      *
 *             a dialog box requesting the appropriate CD be inserted.                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_CDROM_Access(void)
{
    VisiblePage.Clear();
    HidPage.Clear();

    /*
    **	Always try to look at the CD-ROM for data files.
    */
    if (!CCFileClass::Is_There_Search_Drives()) {
        /*
        **	This call is needed because of a side effect of this function. It will examine the
        **	CD-ROMs attached to this computer and set the appropriate status values. Without this
        **	call, the "?:\\" could not be filled in correctly.
        */
        Force_CD_Available(-1);
        RequiredCD = -1;
    } else {

        /*
        ** If there are search drives specified then all files are to be
        ** considered local.
        */
        RequiredCD = -2;
    }
}

/***********************************************************************************************
 * Init_Bootstrap_Mixfiles -- Registers and caches any mixfiles needed for bootstrapping.      *
 *                                                                                             *
 *    This routine will register the initial mixfiles that are required to display error       *
 *    messages and get input from the player.                                                  *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Be sure to call this routine before any dialogs would be displayed to the       *
 *             player.                                                                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Bootstrap_Mixfiles(void)
{
    int temp = RequiredCD;
    RequiredCD = -2;

#ifdef WOLAPI_INTEGRATION
    CCFileClass fileWolapiMix("WOLAPI.MIX");
    if (fileWolapiMix.Is_Available()) {
        new MFCD("WOLAPI.MIX", &FastKey);
        MFCD::Cache("WOLAPI.MIX");
    }
#endif

#ifdef FIXIT_CSII //	Ok. ajw
    CCFileClass file2("EXPAND2.MIX");
    if (file2.Is_Available()) {
        new MFCD("EXPAND2.MIX", &FastKey);
        bool ok = MFCD::Cache("EXPAND2.MIX");
        assert(ok);
    }
#endif

#ifdef FIXIT_CSII //	Ok. ajw
    bool ok1;
#ifndef REMASTER_BUILD
    CCFileClass fileHires1Mix("HIRES1.MIX");
    if (fileHires1Mix.Is_Available()) {
        new MFCD("HIRES1.MIX", &FastKey);
        ok1 = MFCD::Cache("HIRES1.MIX");
        assert(ok1);
    }
#else
    new MFCD("LORES1.MIX", &FastKey);
    ok1 = MFCD::Cache("LORES1.MIX");
    assert(ok1);
#endif
#endif

#ifdef FIXIT_ANTS //	Ok. ajw
    CCFileClass file("EXPAND.MIX");
    if (file.Is_Available()) {
        new MFCD("EXPAND.MIX", &FastKey);
        bool ok = MFCD::Cache("EXPAND.MIX");
        assert(ok);
    }
#endif

    new MFCD("REDALERT.MIX", &FastKey);

    /*
    **	Bootstrap enough of the system so that the error dialog box can successfully
    **	be displayed.
    */
    new MFCD("LOCAL.MIX", &FastKey); // Cached.
    bool ok = MFCD::Cache("LOCAL.MIX");
    assert(ok);

#ifndef REMASTER_BUILD
    new MFCD("HIRES.MIX", &FastKey);
    ok = MFCD::Cache("HIRES.MIX");
    assert(ok);

    new MFCD("NCHIRES.MIX", &FastKey); //Non-cached hires stuff incl VQ palettes
#else
    new MFCD("LORES.MIX", &FastKey);
    ok = MFCD::Cache("LORES.MIX");
    assert(ok);
#endif // REMASTER_BUILD

    RequiredCD = temp;
}

/***********************************************************************************************
 * Init_Secondary_Mixfiles -- Register and cache secondary mixfiles.                           *
 *                                                                                             *
 *    This routine is used to register the mixfiles that are needed for main menu processing.  *
 *    Call this routine before the main menu is display and processed.                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Secondary_Mixfiles(void)
{
    MainMix = new MFCD("MAIN.MIX", &FastKey);
    assert(MainMix != NULL);

    /*
    **	Inform the file system of the various MIX files.
    */
    ConquerMix = new MFCD("CONQUER.MIX", &FastKey); // Cached.
                                                    //	new MFCD("TRANSIT.MIX", &FastKey);

    if (GeneralMix == NULL)
        GeneralMix = new MFCD("GENERAL.MIX", &FastKey); // Never cached.

    if (CCFileClass("MOVIES1.MIX").Is_Available()) {
        MoviesMix = new MFCD("MOVIES1.MIX", &FastKey); // Never cached.
    } else {
        MoviesMix = new MFCD("MOVIES2.MIX", &FastKey); // Never cached.
    }
    assert(MoviesMix != NULL);

    /*
    **	Register the score mixfile.
    */
    ScoresPresent = true;
    ScoreMix = new MFCD("SCORES.MIX", &FastKey);
    ThemeClass::Scan();

    /*
    **	These are sound card specific, but the install program would have
    **	copied the correct versions to the hard drive.
    */
    new MFCD("SPEECH.MIX", &FastKey);  // Never cached.
    new MFCD("SOUNDS.MIX", &FastKey);  // Cached.
    new MFCD("RUSSIAN.MIX", &FastKey); // Cached.
    new MFCD("ALLIES.MIX", &FastKey);  // Cached.
}

/***********************************************************************************************
 * Bootstrap -- Perform the initial bootstrap procedure.                                       *
 *                                                                                             *
 *    This routine will load and initialize the game engine such that a dialog box could be    *
 *    displayed. Because this is very critical, call this routine before any other game        *
 *    initialization code.                                                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Bootstrap(void)
{
    BlackPalette.Set();

    /*
    **	Be sure to short circuit the CD-ROM check if there is a CD-ROM override
    **	path.
    */
    if (CCFileClass::Is_There_Search_Drives()) {
        RequiredCD = -2;
    }

    /*
    ** Process the message loop until we are in focus. We need to be in focus to read pixels from
    ** the screen.
    */
#ifndef REMASTER_BUILD
    do {
        Keyboard->Check();
    } while (!GameInFocus);
    AllSurfaces.SurfacesRestored = false;

    /*
    **	Perform any special debug-only processing. This includes preparing the
    **	monochrome screen.
    */
    Mono_Clear_Screen();
#endif
    /*
    **	Register and make resident all local mixfiles with particular emphasis
    **	on the mixfiles that are necessary to display and error messages and
    **	process further initialization.
    */
    Init_Bootstrap_Mixfiles();

    /*
    **	Initialize the resident font pointers.
    */
    Init_Fonts();

    /*
    **	Setup the keyboard processor in preparation for the game.
    */
    Keyboard->Clear();

    /*
    **	This is the shape staging buffer. It must always be available, so it is
    **	allocated here and never freed. The library sets the globals ShapeBuffer
    **	and ShapeBufferSize to these values, so it can be accessed for other
    **	purposes.
    */
    Set_Shape_Buffer(new unsigned char[SHAPE_BUFFER_SIZE], SHAPE_BUFFER_SIZE);

    /*
    **	Fetch the language text from the hard drive first. If it cannot be
    **	found on the hard drive, then look for it in the mixfile.
    */
#ifdef STEVES_LOAD_OVERRIDE
    RawFileClass strings("CONQUER.ENG");
    if (strings.Is_Available()) {
        SystemStrings = new char[strings.Size()];
        strings.Read((void*)SystemStrings, strings.Size());
    } else {
        SystemStrings = (char const*)MFCD::Retrieve(Language_Name("CONQUER"));
    }
#else
    SystemStrings = (char const*)MFCD::Retrieve(Language_Name("CONQUER"));
#endif
    DebugStrings = (char const*)MFCD::Retrieve("DEBUG.ENG");

    /*
    **	Default palette initialization.
    */
    // PG_TO_FIX. This doesn't seem right. ST - 5/9/2019
#ifndef REMASTER_BUILD
    memmove((unsigned char*)&GamePalette[0], (void*)MFCD::Retrieve("TEMPERAT.PAL"), 768L);
    WhitePalette[0] = BlackPalette[0];
    //GamePalette.Set();
#endif
    /*
    **	Initialize expansion files (if present). Expansion files must be located
    **	in the current directory.
    */
    Init_Expansion_Files();

    SidebarScheme.Background = BLACK;
    SidebarScheme.Corners = LTGREY;
    SidebarScheme.Shadow = DKGREY;
    SidebarScheme.Highlight = WHITE;
    SidebarScheme.Color = LTGREY;
    SidebarScheme.Bright = WHITE;
    SidebarScheme.BrightColor = WHITE;
    SidebarScheme.Box = LTGREY;
    GadgetClass::Set_Color_Scheme(&SidebarScheme);
}

/***********************************************************************************************
 * Init_Mouse -- Initialize the mouse system.                                                  *
 *                                                                                             *
 *    This routine will ensure that a valid mouse driver is present and a working mouse        *
 *    pointer can be displayed. The mouse is hidden when this routine exits.                   *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Mouse(void)
{
    /*
    ** Since there is no mouse shape currently available we need
    ** to set one of our own.
    */
#if defined(_WIN32) && !defined(SDL2_BUILD)
    ShowCursor(false);
#endif
    if (MouseInstalled) {
        void const* temp_mouse_shapes = MFCD::Retrieve("MOUSE.SHP");
        if (temp_mouse_shapes) {
            Set_Mouse_Cursor(0, 0, Extract_Shape(temp_mouse_shapes, 0));
            while (Get_Mouse_State() > 1)
                Show_Mouse();
        }
    } else {
        char buffer[255];
        GamePalette.Set();
        GamePalette.Set();
        sprintf(buffer, TEXT_NO_MOUSE);
        VisiblePage.Clear();
        WWMessageBox().Process(buffer, TXT_OK);
        Prog_End("Init_Mouse", true);
        Emergency_Exit(1);
    }

    Map.Set_Default_Mouse(MOUSE_NORMAL, false);
    Show_Mouse();
    while (Get_Mouse_State() > 1)
        Show_Mouse();
    Call_Back();
    Hide_Mouse();
}

#ifdef OBSOLETE
/***********************************************************************************************
 * Init_Authorization -- Verifies that the player is authorized to play the game.              *
 *                                                                                             *
 *    This is a development routine that restricts access to the game by way of passwords.     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Authorization(void)
{
    if (Special.IsFromInstall)
        return;

    Load_Title_Page();
    Wait_Vert_Blank();

    CCPalette.Set();
    //		Set_Palette(Palette);
    HidPage.Blit(SeenPage);
    Show_Mouse();

    /*
    **	Fetch the type of game to be played. This will be either
    **	C&C:Red Alert or C&C:Plus.
    */
    // tryagain:

    bool ok = Debug_Flag;
    int counter = 3;

    if (Debug_Flag)
        ok = true;

    /*
    **	C&C:Red Alert requires a password for legal entry. Try (three times) to get a correct
    **	password. If not found, then try again.
    */
    bool skipper = false;
#ifdef NEVER
    while (!ok && counter) {
        SmartPtr<char const> str = Fetch_Password(TXT_PASSWORD_CAPTION, TXT_PASSWORD_MESSAGE, TXT_OK);
        SmartPtr<long const> lptr = &CheatCodes[0];
        while (*lptr) {
            if (Obfuscate(str) == *lptr++) {
                ok = true;
                break;
            }
        }
        lptr = &EditorCodes[0];
        while (*lptr) {
            if (Obfuscate(str) == *lptr++) {
                ok = true;
                break;
            }
        }
        lptr = &PlayCodes[0];
        while (*lptr) {
            if (Obfuscate(str) == *lptr++) {
                ok = true;
                skipper = true;
                break;
            }
        }

        if (ok)
            break;

        Hide_Mouse();
        Load_Title_Page();
        HidPage.Blit(SeenPage);
        Show_Mouse();
        Delay(TIMER_SECOND * (4 - counter) * 1);
        if (WWMessageBox().Process(TXT_PASSWORD_ERROR, TXT_TRY_AGAIN, TXT_CANCEL)) {
            goto tryagain;
        }

        counter--;
        if (counter == 0)
            goto tryagain;
    }
#endif

    if (!skipper) {
        CCPalette.Set();
    }

    Hide_Mouse();
    Load_Title_Page();
    HidPage.Blit(SeenPage);
    Show_Mouse();
    Call_Back();
}
#endif

/***********************************************************************************************
 * Init_Bulk_Data -- Initialize the time-consuming mixfile caching.                            *
 *                                                                                             *
 *    This routine is called to handle the time consuming process of game initialization.      *
 *    The title page will be displayed when this routine is called.                            *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine will take a very long time.                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Bulk_Data(void)
{
    /*
    **	Cache the main game data. This operation can take a very long time.
    */
    MFCD::Cache("CONQUER.MIX");
    if (SampleType != 0 && !Debug_Quiet) {
        MFCD::Cache("SOUNDS.MIX");
        MFCD::Cache("RUSSIAN.MIX");
        MFCD::Cache("ALLIES.MIX");
    }
    Call_Back();

    /*
    **	Fetch the tutorial message data.
    */
    INIClass ini;
    CCFileClass tutorialIniFile("TUTORIAL.INI");
    ini.Load(tutorialIniFile);
    for (int index = 0; index < ARRAY_SIZE(TutorialText); index++) {
        TutorialText[index] = NULL;

        char buffer[128];
        char num[10];
        sprintf(num, "%d", index);
        if (ini.Get_String("Tutorial", num, "", buffer, sizeof(buffer))) {
            TutorialText[index] = strdup(buffer);
        }
    }

    /*
    **	Perform one-time game system initializations.
    */
    Init_One_Time_Systems();
}

/***********************************************************************************************
 * Init_Keys -- Initialize the cryptographic keys.                                             *
 *                                                                                             *
 *    This routine will initialize the fast cryptographic key. It will also initialize the     *
 *    slow one if this is a scenario editor version of the game.                               *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/08/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static void Init_Keys(void)
{
    RAMFileClass file((void*)Keys, strlen(Keys));
    INIClass ini;
    ini.Load(file);

    FastKey = ini.Get_PKey(true);
#ifdef SCENARIO_EDITOR
    SlowKey = ini.Get_PKey(false);
#endif
}

/***************************************************************************
 * Save_Recording_Values -- Saves multiplayer-specific values              *
 *                                                                         *
 * This routine saves multiplayer values that need to be restored for a		*
 * save game.  In addition to saving the random # seed for this scenario, 	*
 * it saves the contents of the actual random number generator; this 		*
 * ensures that the random # sequencer will pick up where it left off when	*
 * the game was saved.																		*
 * This routine also saves the header for a Recording file, so it must 		*
 * save some data not needed specifically by a save-game file (ie Seed).	*
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to save to															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = success, false = failure													*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
bool Save_Recording_Values(CCFileClass& file)
{
    Session.Save(file);
    file.Write(&BuildLevel, sizeof(BuildLevel));
    file.Write(&Debug_Unshroud, sizeof(Debug_Unshroud));
    file.Write(&Seed, sizeof(Seed));
    file.Write(&Scen.Scenario, sizeof(Scen.Scenario));
    file.Write(Scen.ScenarioName, sizeof(Scen.ScenarioName));
    file.Write(&Whom, sizeof(Whom));
    file.Write(&Special, sizeof(SpecialClass));
    file.Write(&Options, sizeof(GameOptionsClass));

    return (true);
}

/***************************************************************************
 * Load_Recording_Values -- Loads multiplayer-specific values              *
 *                                                                         *
 * INPUT:                                                                  *
 *		file			file to load from														*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = success, false = failure													*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
bool Load_Recording_Values(CCFileClass& file)
{
    Session.Load(file);
    file.Read(&BuildLevel, sizeof(BuildLevel));
    file.Read(&Debug_Unshroud, sizeof(Debug_Unshroud));
    file.Read(&Seed, sizeof(Seed));
    file.Read(&Scen.Scenario, sizeof(Scen.Scenario));
    file.Read(Scen.ScenarioName, sizeof(Scen.ScenarioName));
    file.Read(&Whom, sizeof(Whom));
    file.Read(&Special, sizeof(SpecialClass));
    file.Read(&Options, sizeof(GameOptionsClass));
    return (true);
}

void __PRO(void)
{
    //	printf("_pro\n");
}

/***********************************************************************************************
 * Free_Heaps -- Clear out the heaps before exit                                               *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/18/2019 11:59AM ST : Created.                                                                 *
 *=============================================================================================*/
void Free_Heaps(void)
{
    HouseTypes.Clear();
    BuildingTypes.Clear();
    AircraftTypes.Clear();
    InfantryTypes.Clear();
    BulletTypes.Clear();
    AnimTypes.Clear();
    UnitTypes.Clear();
    VesselTypes.Clear();
    TemplateTypes.Clear();
    TerrainTypes.Clear();
    OverlayTypes.Clear();
    SmudgeTypes.Clear();
#if (0)
    HouseTypeClass::Init_Heap();
    BuildingTypeClass::Init_Heap();
    AircraftTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    BulletTypeClass::Init_Heap();
    AnimTypeClass::Init_Heap();
    UnitTypeClass::Init_Heap();
    VesselTypeClass::Init_Heap();
    TemplateTypeClass::Init_Heap();
    TerrainTypeClass::Init_Heap();
    OverlayTypeClass::Init_Heap();
    SmudgeTypeClass::Init_Heap();

    // Heap init moved here from globals.cpp. ST - 5/20/2019
    CCPtr<AircraftClass>::Set_Heap(&Aircraft);
    CCPtr<AnimClass>::Set_Heap(&Anims);
    CCPtr<BuildingClass>::Set_Heap(&Buildings);
    CCPtr<BulletClass>::Set_Heap(&Bullets);
    CCPtr<FactoryClass>::Set_Heap(&Factories);
    CCPtr<HouseClass>::Set_Heap(&Houses);
    CCPtr<InfantryClass>::Set_Heap(&Infantry);
    CCPtr<OverlayClass>::Set_Heap(&Overlays);
    CCPtr<SmudgeClass>::Set_Heap(&Smudges);
    CCPtr<TeamClass>::Set_Heap(&Teams);
    CCPtr<TeamTypeClass>::Set_Heap(&TeamTypes);
    CCPtr<TemplateClass>::Set_Heap(&Templates);
    CCPtr<TerrainClass>::Set_Heap(&Terrains);
    CCPtr<TriggerClass>::Set_Heap(&Triggers);
    CCPtr<TriggerTypeClass>::Set_Heap(&TriggerTypes);

    CCPtr<HouseTypeClass>::Set_Heap(&HouseTypes);
    CCPtr<BuildingTypeClass>::Set_Heap(&BuildingTypes);
    CCPtr<AircraftTypeClass>::Set_Heap(&AircraftTypes);
    CCPtr<InfantryTypeClass>::Set_Heap(&InfantryTypes);
    CCPtr<BulletTypeClass>::Set_Heap(&BulletTypes);
    CCPtr<AnimTypeClass>::Set_Heap(&AnimTypes);
    CCPtr<UnitTypeClass>::Set_Heap(&UnitTypes);
    CCPtr<VesselTypeClass>::Set_Heap(&VesselTypes);
    CCPtr<TemplateTypeClass>::Set_Heap(&TemplateTypes);
    CCPtr<TerrainTypeClass>::Set_Heap(&TerrainTypes);
    CCPtr<OverlayTypeClass>::Set_Heap(&OverlayTypes);
    CCPtr<SmudgeTypeClass>::Set_Heap(&SmudgeTypes);
#endif

    Vessels.Clear();
    Units.Clear();
    Factories.Clear();
    Terrains.Clear();
    Templates.Clear();
    Smudges.Clear();
    Overlays.Clear();
    Infantry.Clear();
    Bullets.Clear();
    Buildings.Clear();
    Anims.Clear();
    Aircraft.Clear();
    Triggers.Clear();
    TeamTypes.Clear();
    Teams.Clear();
    Houses.Clear();
    TriggerTypes.Clear();

    /*
    **	Speech holding tank buffer. Since speech does not mix, it can be placed
    **	into a custom holding tank only as large as the largest speech file to
    **	be played.
    */
    for (int index = 0; index < ARRAY_SIZE(SpeechBuffer); index++) {
        if (SpeechBuffer[index]) {
            delete[] SpeechBuffer[index];
            SpeechBuffer[index] = NULL;
        }
    }

    /*
    **	Allocate the theater buffer block.
    */
    if (TheaterBuffer) {
        delete TheaterBuffer;
        TheaterBuffer = NULL;
    }
}