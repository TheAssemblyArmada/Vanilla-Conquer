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

/* $Header: /counterstrike/SESSION.CPP 3     3/10/97 6:23p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SESSION.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Bill R. Randolph                                             *
 *                                                                                             *
 *                   Start Date : 11/30/95                                                     *
 *                                                                                             *
 *                  Last Update : September 10, 1996 [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   SessionClass::SessionClass -- Constructor                                                 *
 *   SessionClass::~SessionClass -- Destructor                                                 *
 *   SessionClass::One_Time -- one-time initializations                                        *
 *   SessionClass::Init -- Initializes all values                                              *
 *   SessionClass::Create_Connections -- forms connections to other players                    *
 *   SessionClass::Am_I_Master -- tells if the local system is the "master"                    *
 *   SessionClass::Save -- Saves this class to a file                                          *
 *   SessionClass::Load -- Loads this class from a file                                        *
 *   SessionClass::Read_MultiPlayer_Settings -- reads settings from INI                        *
 *   SessionClass::Write_MultiPlayer_Settings -- writes settings to INI                        *
 *   SessionClass::Read_Scenario_Descriptions -- reads scen. descriptions                      *
 *   SessionClass::Free_Scenario_Descriptions -- frees scen. descriptions                      *
 *   SessionClass::Trap_Object -- searches for an object, for debugging                        *
 *   SessionClass::Compute_Unique_ID -- computes unique local ID number                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include <time.h> // for station ID computation

//#include "WolDebug.h"

/***************************** Globals *************************************/
//---------------------------------------------------------------------------
//	This is the array of remap colors.  Each player in a network game is
// assigned one of these colors.  The 'G' is for graphics drawing; the 'T'
// is for text printing (indicates a remap table for the font to use).
//---------------------------------------------------------------------------
// int SessionClass::GColors[MAX_MPLAYER_COLORS] = {
// 5, 			// Yellow
// 127, 			// Red
// 135, 			// BlueGreen
// 26,			// Orange
// 4,				// Green
// 202			// Blue-Grey
//};

// int SessionClass::TColors[MAX_MPLAYER_COLORS] = {
// CC_GDI_COLOR, 			// Yellow
// CC_NOD_COLOR, 			// Red
// CC_BLUE_GREEN, 		// BlueGreen
// CC_ORANGE,				// Orange
// CC_GREEN,				// Green
// CC_BLUE_GREY,			// Blue
//};

/*---------------------------------------------------------------------------
Min & Max unit count values; index0 = bases OFF, index1 = bases ON
---------------------------------------------------------------------------*/
int SessionClass::CountMin[2] = {1, 0};
int SessionClass::CountMax[2] = {50, 12};

//---------------------------------------------------------------------------
//	This is a list of all the names of the multiplayer scenarios
//---------------------------------------------------------------------------
char SessionClass::Descriptions[100][40];

//---------------------------------------------------------------------------
// These values are used purely for the Mono debug display.  They show the
// names of the Global Channel packet types, and the event types.
//---------------------------------------------------------------------------
const char* SessionClass::GlobalPacketNames[] = {"Game?",
                                                 "Game!",
                                                 "Player?",
                                                 "Player!",
                                                 "Join?",
                                                 "Join!",
                                                 "Reject",
                                                 "GameOptions",
                                                 "Sign Off",
                                                 "GO!",
                                                 "Message",
                                                 "Ping",
                                                 "Load"};

const char* SessionClass::SerialPacketNames[] = {
    "CONNECT",
    "GAME_OPTIONS",
    "SIGN_OFF",
    "GO",
    "MESSAGE",
    "TIMING",
    "SCORE_SCREEN",
    "LOADGAME",
    "LAST_COMMAND",
};

const char* SessionClass::DialMethodCheck[DIAL_METHODS] = {"T", "P"};

const char* SessionClass::CallWaitStrings[CALL_WAIT_STRINGS_NUM] = {"*70,",
                                                                    "70#,",
                                                                    "1170,",
                                                                    "CUSTOM -                "};

/***************************************************************************
 * SessionClass::SessionClass -- Constructor                               *
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
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
SessionClass::SessionClass(void)
{
    Type = GAME_NORMAL;
    CommProtocol = DEFAULT_COMM_PROTOCOL;

    Options.ScenarioIndex = 0;
    Options.Bases = 0;
    Options.Credits = 0;
    Options.Tiberium = 0;
    Options.Goodies = 0;
    Options.Ghosts = 0;
    Options.UnitCount = 0;

    UniqueID = 0;

    Handle[0] = 0;
    PrefColor = PCOLOR_FIRST;
    ColorIdx = PCOLOR_FIRST;
    House = HOUSE_GOOD;
    ObiWan = 0;
    Solo = 0;

    MaxPlayers = 8;
    NumPlayers = 0;

    MaxAhead = 5;
    FrameSendRate = DEFAULT_FRAME_SEND_RATE;

    LoadGame = 0;
    EmergencySave = 0;

    LastMessage[0] = 0;
    WWChat = 0;

    RecordFile.Set_Name("RECORD.BIN"); // always uses this name
    Record = 0;                        // set via command line
    Play = 0;                          // set via command line
    Attract = 0;                       // set via command line

    NetStealth = false;
    NetProtect = true;
    NetOpen = false;
    GameName[0] = 0;
    GProductID = 0;

    MetaSize = MAX_IPX_PACKET_SIZE;

    ModemService = true;
    CurPhoneIdx = 0;                             // set from INI file
    SerialDefaults.Port = 0x2f8;                 // set from INI file
    SerialDefaults.IRQ = 3;                      // set from INI file
    SerialDefaults.Baud = 9600;                  // set from INI file
    SerialDefaults.DialMethod = DIAL_TOUCH_TONE; // set from INI file
    SerialDefaults.InitStringIndex = 0;          // set from INI file
    SerialDefaults.CallWaitStringIndex = 0;      // set from INI file
    strcpy(SerialDefaults.CallWaitString, "");
    ModemType = MODEM_NULL_HOST; // set from INI file

    TrapFrame = 0x7fffffff;    // frame to start trapping object values at
    TrapObjType = RTTI_NONE;   // type of object to trap
    TrapObject.Ptr.All = NULL; // ptr to object being trapped
    TrapCoord = 0;             // COORDINATE of object to trap
    TrapTarget = TARGET_NONE;  // TARGET value of object to trap
    TrapCell = NULL;           // for trapping a cell
    TrapCheckHeap = 0;         // start checking the Heap
    TrapPrintCRC = 0;          // output CRC file

} // end of SessionClass

/***************************************************************************
 * SessionClass::~SessionClass -- Destructor                               *
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
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
SessionClass::~SessionClass(void)
{
} // end of ~SessionClass

/***************************************************************************
 * SessionClass::One_Time -- one-time initializations                      *
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
 *   12/01/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::One_Time(void)
{
    Read_MultiPlayer_Settings();
    Read_Scenario_Descriptions();

    UniqueID = Compute_Unique_ID();

} // end of One_Time

/***************************************************************************
 * SessionClass::Init -- Initializes all values                            *
 *                                                                         *
 * This function should be called for every new game played; it only sets	*
 * those variables that should be set for a new game.								*
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
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::Init(void)
{

} // end of Init

/***************************************************************************
 * SessionClass::Create_Connections -- forms connections to other players  *
 *                                                                         *
 * This routine uses the contents of the Players vector, combined with		*
 * that of the Houses array, to create connections to each other player.	*
 * It is assumed that 'Players' contains all the other players to connect	*
 * to, and that the HouseClass's have been filled in with players' data.	*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = success, 0 = failure															*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
int SessionClass::Create_Connections(void)
{
#ifndef REMASTER_BUILD
    int i;

    if (Session.Type != GAME_IPX && Session.Type != GAME_INTERNET) {
        return (0);
    }

    //------------------------------------------------------------------------
    // Loop through all entries in 'Players'.  To avoid connecting to myself,
    // skip the 1st entry.
    //------------------------------------------------------------------------
    for (i = 1; i < Players.Count(); i++) {
        //.....................................................................
        // Make sure the name matches before creating the connection
        //.....................................................................
        if (!stricmp(Players[i]->Name, HouseClass::As_Pointer(Players[i]->Player.ID)->IniName)) {
            Ipx.Create_Connection((int)Players[i]->Player.ID, Players[i]->Name, &(Players[i]->Address));
            Players[i]->Player.ProcessTime = -1;
        } else {
            return (0);
        }
    }
#endif
    return (1);

} // end of Create_Connections

/***************************************************************************
 * SessionClass::Am_I_Master -- tells if the local system is the "master"  *
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
 *   11/29/1995 BRR : Created.                                             *
 *=========================================================================*/
bool SessionClass::Am_I_Master(void)
{
    int i;
    HousesType house;
    HouseClass* hptr;

    //------------------------------------------------------------------------
    // Check every house; if PlayerPtr points to the first human house, we're
    // the master.
    //------------------------------------------------------------------------
    for (i = 0; i < Session.MaxPlayers; i++) {
        house = (HousesType)((int)HOUSE_MULTI1 + i);
        hptr = HouseClass::As_Pointer(house);
        if (hptr->IsHuman) {
            if (PlayerPtr == hptr) {
                return (true);
            } else {
                return (false);
            }
        }
    }

    return (false);

} // end of Am_I_Master

/***************************************************************************
 * SessionClass::Save -- Saves this class to a file                        *
 *                                                                         *
 * Only certain members of this class should be saved into a save-game		*
 * file; this routine saves only those members.										*
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to save to															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
int SessionClass::Save(Pipe& file) const
{
#ifdef FIXIT_MULTI_SAVE
    file.Put(&CommProtocol, sizeof(CommProtocol));
    file.Put(&MaxAhead, sizeof(MaxAhead));
    file.Put(&FrameSendRate, sizeof(FrameSendRate));
    file.Put(&DesiredFrameRate, sizeof(DesiredFrameRate));
#endif // FIXIT_MULTI_SAVE
    file.Put(&PrefColor, sizeof(PrefColor));
    file.Put(&ColorIdx, sizeof(ColorIdx));
    file.Put(&House, sizeof(House));
    file.Put(&NumPlayers, sizeof(NumPlayers));
    file.Put(&Options.Bases, sizeof(Options.Bases));
    file.Put(&Options.Credits, sizeof(Options.Credits));
    file.Put(&Options.Tiberium, sizeof(Options.Tiberium));
    file.Put(&Options.Goodies, sizeof(Options.Goodies));
    file.Put(&Options.Ghosts, sizeof(Options.Ghosts));
    file.Put(&Options.UnitCount, sizeof(Options.UnitCount));
    file.Put(&Options.AIPlayers, sizeof(Options.AIPlayers));
    file.Put(&ObiWan, sizeof(ObiWan));
    file.Put(&EmergencySave, sizeof(EmergencySave));

    return (1);

} // end of Save

/***************************************************************************
 * SessionClass::Load -- Loads this class from a file                      *
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to load from															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
int SessionClass::Load(Straw& file)
{
#ifdef FIXIT_MULTI_SAVE
    //	if(GameVersion != 0x0100616D){
    file.Get(&CommProtocol, sizeof(CommProtocol));
    file.Get(&MaxAhead, sizeof(MaxAhead));
    file.Get(&FrameSendRate, sizeof(FrameSendRate));
    file.Get(&DesiredFrameRate, sizeof(DesiredFrameRate));
//	}
#endif // FIXIT_MULTI_SAVE
    file.Get(&PrefColor, sizeof(PrefColor));
    file.Get(&ColorIdx, sizeof(ColorIdx));
    file.Get(&House, sizeof(House));
    file.Get(&NumPlayers, sizeof(NumPlayers));
    file.Get(&Options.Bases, sizeof(Options.Bases));
    file.Get(&Options.Credits, sizeof(Options.Credits));
    file.Get(&Options.Tiberium, sizeof(Options.Tiberium));
    file.Get(&Options.Goodies, sizeof(Options.Goodies));
    file.Get(&Options.Ghosts, sizeof(Options.Ghosts));
    file.Get(&Options.UnitCount, sizeof(Options.UnitCount));
    file.Get(&Options.AIPlayers, sizeof(Options.AIPlayers));
    file.Get(&ObiWan, sizeof(ObiWan));
    file.Get(&EmergencySave, sizeof(EmergencySave));

    return (1);

} // end of Load

/***************************************************************************
 * SessionClass::Save -- Saves this class to a file                        *
 *                                                                         *
 * Only certain members of this class should be saved into a save-game		*
 * file; this routine saves only those members.										*
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to save to															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
int SessionClass::Save(CCFileClass& file)
{
    int i;

    file.Write(&Type, sizeof(Type));
    file.Write(&CommProtocol, sizeof(CommProtocol));
    file.Write(&FrameSendRate, sizeof(FrameSendRate));
    file.Write(&PrefColor, sizeof(PrefColor));
    file.Write(&ColorIdx, sizeof(ColorIdx));
    file.Write(&House, sizeof(House));
    file.Write(&NumPlayers, sizeof(NumPlayers));
    file.Write(&Options.Bases, sizeof(Options.Bases));
    file.Write(&Options.Credits, sizeof(Options.Credits));
    file.Write(&Options.Tiberium, sizeof(Options.Tiberium));
    file.Write(&Options.Goodies, sizeof(Options.Goodies));
    file.Write(&Options.Ghosts, sizeof(Options.Ghosts));
    file.Write(&Options.UnitCount, sizeof(Options.UnitCount));
    file.Write(&Options.AIPlayers, sizeof(Options.AIPlayers));
    file.Write(&ObiWan, sizeof(ObiWan));
    file.Write(&EmergencySave, sizeof(EmergencySave));

    i = Players.Count();
    file.Write(&i, sizeof(i));
    for (i = 0; i < Players.Count(); i++) {
        file.Write(Players[i], sizeof(NodeNameType));
    }

    return (1);

} // end of Save

/***************************************************************************
 * SessionClass::Load -- Loads this class from a file                      *
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to load from															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
int SessionClass::Load(CCFileClass& file)
{
    int count;
    int i;
    NodeNameType* node;

    file.Read(&Type, sizeof(Type));
    file.Read(&CommProtocol, sizeof(CommProtocol));
    file.Read(&FrameSendRate, sizeof(FrameSendRate));
    file.Read(&PrefColor, sizeof(PrefColor));
    file.Read(&ColorIdx, sizeof(ColorIdx));
    file.Read(&House, sizeof(House));
    file.Read(&NumPlayers, sizeof(NumPlayers));
    file.Read(&Options.Bases, sizeof(Options.Bases));
    file.Read(&Options.Credits, sizeof(Options.Credits));
    file.Read(&Options.Tiberium, sizeof(Options.Tiberium));
    file.Read(&Options.Goodies, sizeof(Options.Goodies));
    file.Read(&Options.Ghosts, sizeof(Options.Ghosts));
    file.Read(&Options.UnitCount, sizeof(Options.UnitCount));
    file.Read(&Options.AIPlayers, sizeof(Options.AIPlayers));
    file.Read(&ObiWan, sizeof(ObiWan));
    file.Read(&EmergencySave, sizeof(EmergencySave));

    file.Read(&count, sizeof(count));
    for (i = 0; i < count; i++) {
        node = new NodeNameType;
        file.Read(node, sizeof(NodeNameType));
        Players.Add(node);
    }

    return (1);

} // end of Load

/***************************************************************************
 * SessionClass::Read_MultiPlayer_Settings -- reads settings INI           *
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
 *   02/14/1995 BR : Created.                                              *
 *=========================================================================*/
void SessionClass::Read_MultiPlayer_Settings(void)
{
#ifndef REMASTER_BUILD
    char buf[128]; // buffer for parsing INI entry
    CELL cell;

    //	Create filename and read the file.
    INIClass ini;
    CDFileClass file(CONFIG_FILE_NAME);
    if (ini.Load(file)) {

        //	Get the player's last-used Handle
        ini.Get_String("MultiPlayer", "Handle", "Noname", Handle, sizeof(Handle));

        //	Get the player's last-used Color
        PrefColor = (PlayerColorType)ini.Get_Int("MultiPlayer", "Color", 0);
#ifdef FIXIT_VERSION_3
        int iSide = ini.Get_Int("MultiPlayer", "Side", HOUSE_USSR);
        iSide = max(2, min(6, iSide));
        House = (HousesType)iSide;
#else
        House = (HousesType)ini.Get_Int("MultiPlayer", "Side", HOUSE_USSR);
#endif
        CurPhoneIdx = ini.Get_Int("MultiPlayer", "PhoneIndex", -1);
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

        ini.Get_String("SyncBug", "Target", "0", buf, 80);
        sscanf(buf, "%x", &TrapTarget);

        ini.Get_String("SyncBug", "Cell", "0", buf, 80);
        cell = atoi(buf);
        if (cell) {
            TrapCell = &(Map[cell]);
        }

        TrapPrintCRC = ini.Get_Int("SyncBug", "PrintCRC", 0x7fffffff);
    }
#endif
}

/***************************************************************************
 * SessionClass::Write_MultiPlayer_Settings -- writes settings INI         *
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
 *   02/14/1995 BR : Created.                                              *
 *=========================================================================*/
void SessionClass::Write_MultiPlayer_Settings(void)
{
#ifndef REMASTER_BUILD
    INIClass ini;
    CDFileClass file(CONFIG_FILE_NAME);
    if (ini.Load(file)) {

        //	Save the player's last-used Handle & Color
        ini.Put_Int("MultiPlayer", "PhoneIndex", CurPhoneIdx);
        ini.Put_Int("MultiPlayer", "Color", (int)PrefColor);
        ini.Put_Int("MultiPlayer", "Side", House);
        ini.Put_String("MultiPlayer", "Handle", Handle);

        //	Write the INI data out to a file.
        ini.Save(file);
    }
#endif
}

// Determine if a mission is from counterstrike or aftermath, or either.
// Multiplayer maps >24, with a numerical name, are Counterstrike.
// Multiplayer maps with an alphabetical name, like SCMJGEA.INI, are Aftermath.

bool Is_Mission_Counterstrike(char* file_name)
{
    int scenario_number = 0;

    if (isdigit(file_name[5])) {
        sscanf(file_name, "SCM%03d", &scenario_number);
    } else {
#ifdef FIXIT_CSII //	checked - ajw 9/28/98
        if (!isdigit(file_name[3]) || !isdigit(file_name[4])) {
            return (false);
        }
#endif
        sscanf(file_name, "SCM%02d", &scenario_number);
    }
    return (scenario_number > 24);
}

#ifdef FIXIT_CSII //	checked - ajw 9/28/98
bool Is_Mission_Aftermath(char* file_name)
{
    //	ajw added
    //	Must start with "scm".
    char szCopy[_MAX_PATH + 1];
    strcpy(szCopy, file_name);
    _strlwr(szCopy);
    if (strstr(szCopy, "scm") != szCopy)
        return false;

    if (isdigit(file_name[5])) {
        return (false);
    }

    if (!isdigit(file_name[3]) || !isdigit(file_name[4])) {
        return (true);
    }
    return (false);
}

/*
** Certain missions are 126x126 size, and those can't be downloaded to a
** non-Aftermath player, so this function checks to see if the map in
** question is one of those.  We'll know that by the file name: if it's
** K0 -> M9, it's 126x126.
*/
bool Is_Mission_126x126(char* file_name) //	This is no longer used. ajw
{
    if (isdigit(file_name[5])) {
        return (false);
    }

    if ((file_name[3] >= 'k' && file_name[3] <= 'm') || (file_name[3] >= 'K' && file_name[3] <= 'M')) {
        return (true);
    }
    return (false);
}

#endif

/***************************************************************************
 * SessionClass::Read_Scenario_Descriptions -- reads scen. descriptions    *
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
 *   02/14/1995 BR : Created.                                              *
 *   09/10/1996 JLB : Searches using different method.                     *
 *=========================================================================*/
void SessionClass::Read_Scenario_Descriptions(void)
{
    Find_File_Data* ffd;
    bool found;

    /*
    ** Clear the scenario description lists
    */
    Scenarios.Clear();

    /*
    ** Fetch the main multiplayer scenario packet data.
    */
    CCFileClass file("MISSIONS.PKT");
    if (file.Is_Available()) {
        INIClass ini;
        ini.Load(file);
        int count = ini.Entry_Count("Missions");
        // debugprint( "Found %i missions in Missions.pkt\n", count );
        for (int index = 0; index < count; index++) {
            char const* fname = ini.Get_Entry("Missions", index);
            char buffer[128];
            ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));
            Scenarios.Add(new MultiMission(fname, buffer, NULL, true, Is_Mission_Counterstrike((char*)fname)));
        }
    }

    /*
    ** Fetch any scenario packet lists and apply them first.
    */
    ffd = nullptr;
    found = Find_First("*.MPR", 0, &ffd);
    while (found) {
        CCFileClass file(ffd->GetName());
        INIClass ini;
        ini.Load(file);

        int count = ini.Entry_Count("Missions");
        for (int index = 0; index < count; index++) {
            char const* fname = ini.Get_Entry("Missions", index);
            char buffer[128];
            ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));
            Scenarios.Add(new MultiMission(fname, buffer, NULL, true, Is_Mission_Counterstrike((char*)fname)));
        }

        found = Find_Next(ffd);
    }
    if (ffd) {
        Find_Close(ffd);
    }

    /*
    ** Fetch the Counterstrike multiplayer scenario packet data.
    ** Load the scenarios regardless of whether counterstrike's installed,
    ** and at the point of hosting a network game, enable the counterstrike
    ** maps only if they have CS installed.  If they don't, then the maps
    ** are available as a guest, but not as a host, which fixes a multitude
    ** of problems without obviously giving the maps away to non-CS owners.
    */
    if (Is_Counterstrike_Installed()) {
        CCFileClass file2("CSTRIKE.PKT");
        if (file2.Is_Available()) {
            INIClass ini;
            ini.Load(file2);
            int count = ini.Entry_Count("Missions");
            for (int index = 0; index < count; index++) {
                char const* fname = ini.Get_Entry("Missions", index);
                char buffer[128];
                ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));
                Scenarios.Add(new MultiMission(fname, buffer, NULL, true, Is_Mission_Counterstrike((char*)fname)));
            }
        }
    }

    /*
    ** Aftermath scenarios are now in their own pkt file.
    */
    if (Is_Aftermath_Installed()) {
        CCFileClass file2("AFTMATH.PKT");
        if (file2.Is_Available()) {
            INIClass ini;
            ini.Load(file2);
            int count = ini.Entry_Count("Missions");
            for (int index = 0; index < count; index++) {
                char const* fname = ini.Get_Entry("Missions", index);
                char buffer[128];
                ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));
                Scenarios.Add(new MultiMission(fname, buffer, NULL, true, Is_Mission_Counterstrike((char*)fname)));
            }
        }
    }

    /*
    ** Scan the current directory for any loose .MPR files and build the appropriate entries
    ** into the scenario list list
    */
    ffd = nullptr;
    found = Find_First("*.MPR", 0, &ffd);
    while (found) {
        char name_buffer[128];
        char digest_buffer[32];

        CCFileClass file(ffd->GetName());
        INIClass ini;
        ini.Load(file);

        ini.Get_String("Basic", "Name", "No Name", name_buffer, sizeof(name_buffer));
        ini.Get_String("Digest", "1", "No Digest", digest_buffer, sizeof(digest_buffer));
        Scenarios.Add(new MultiMission(
            ffd->GetName(), name_buffer, digest_buffer, ini.Get_Bool("Basic", "Official", false), false));

        found = Find_Next(ffd);
    }
    if (ffd) {
        Find_Close(ffd);
    }
}

/***************************************************************************
 * SessionClass::Free_Scenario_Descriptions -- frees scen. descriptions    *
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
 *   06/05/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::Free_Scenario_Descriptions(void)
{
    int i;

    //------------------------------------------------------------------------
    //	Clear the scenario descriptions & filenames
    //------------------------------------------------------------------------
    for (int index = 0; index < Scenarios.Count(); index++) {
        delete Scenarios[index];
    }
    Scenarios.Clear();
    //	Filenum.Clear();

    //------------------------------------------------------------------------
    //	Clear the initstring entries
    //------------------------------------------------------------------------
    for (i = 0; i < InitStrings.Count(); i++) {
        delete InitStrings[i];
    }
    InitStrings.Clear();
#if (0) // PG
    //------------------------------------------------------------------------
    //	Clear the dialing entries
    //------------------------------------------------------------------------
    for (i = 0; i < PhoneBook.Count(); i++) {
        delete PhoneBook[i];
    }
    PhoneBook.Clear();
#endif
} /* end of Free_Scenario_Descriptions */

/***************************************************************************
 * SessionClass::Trap_Object -- searches for an object, for debugging		*
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
 *   06/02/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::Trap_Object(void)
{
    int i;

    //------------------------------------------------------------------------
    // Initialize
    //------------------------------------------------------------------------
    TrapObject.Ptr.All = NULL;

    //------------------------------------------------------------------------
    // Search for the object based upon its type, then its coordinate or
    // 'this' pointer value.
    //------------------------------------------------------------------------
    switch (TrapObjType) {
    case RTTI_AIRCRAFT:
        for (i = 0; i < Aircraft.Count(); i++) {
            if (Aircraft.Ptr(i)->Coord == TrapCoord || Aircraft.Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Aircraft = Aircraft.Ptr(i);
                break;
            }
        }
        break;

    case RTTI_ANIM:
        for (i = 0; i < Anims.Count(); i++) {
            if (Anims.Ptr(i)->Coord == TrapCoord || Anims.Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Anim = Anims.Ptr(i);
                break;
            }
        }
        break;

    case RTTI_BUILDING:
        for (i = 0; i < Buildings.Count(); i++) {
            if (Buildings.Ptr(i)->Coord == TrapCoord || Buildings.Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Building = Buildings.Ptr(i);
                break;
            }
        }
        break;

    case RTTI_BULLET:
        for (i = 0; i < Bullets.Count(); i++) {
            if (Bullets.Ptr(i)->Coord == TrapCoord || Bullets.Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Bullet = Bullets.Ptr(i);
                break;
            }
        }
        break;

    case RTTI_INFANTRY:
        for (i = 0; i < Infantry.Count(); i++) {
            if (Infantry.Ptr(i)->Coord == TrapCoord || Infantry.Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Infantry = Infantry.Ptr(i);
                break;
            }
        }
        break;

    case RTTI_UNIT:
        for (i = 0; i < Units.Count(); i++) {
            if (Units.Ptr(i)->Coord == TrapCoord || Units.Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Unit = Units.Ptr(i);
                break;
            }
        }
        break;

    //.....................................................................
    // Last-ditch find-the-object-right-now-darnit loop
    //.....................................................................
    case RTTI_NONE:
        for (i = 0; i < Aircraft.Count(); i++) {
            if (Aircraft.Raw_Ptr(i)->Coord == TrapCoord || Aircraft.Raw_Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Aircraft = Aircraft.Raw_Ptr(i);
                TrapObjType = RTTI_AIRCRAFT;
                return;
            }
        }
        for (i = 0; i < Anims.Count(); i++) {
            if (Anims.Raw_Ptr(i)->Coord == TrapCoord || Anims.Raw_Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Anim = Anims.Raw_Ptr(i);
                TrapObjType = RTTI_ANIM;
                return;
            }
        }
        for (i = 0; i < Buildings.Count(); i++) {
            if (Buildings.Raw_Ptr(i)->Coord == TrapCoord || Buildings.Raw_Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Building = Buildings.Raw_Ptr(i);
                TrapObjType = RTTI_BUILDING;
                return;
            }
        }
        for (i = 0; i < Bullets.Count(); i++) {
            if (Bullets.Raw_Ptr(i)->Coord == TrapCoord || Bullets.Raw_Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Bullet = Bullets.Raw_Ptr(i);
                TrapObjType = RTTI_BULLET;
                return;
            }
        }
        for (i = 0; i < Infantry.Count(); i++) {
            if (Infantry.Raw_Ptr(i)->Coord == TrapCoord || Infantry.Raw_Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Infantry = Infantry.Raw_Ptr(i);
                TrapObjType = RTTI_INFANTRY;
                return;
            }
        }
        for (i = 0; i < Units.Count(); i++) {
            if (Units.Raw_Ptr(i)->Coord == TrapCoord || Units.Raw_Ptr(i)->As_Target() == TrapTarget) {
                TrapObject.Ptr.Unit = Units.Raw_Ptr(i);
                TrapObjType = RTTI_UNIT;
                return;
            }
        }

    default:
        break;
    }
}

/***************************************************************************
 * SessionClass::Compute_Unique_ID -- computes unique local ID number      *
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
 *   12/07/1995 BRR : Created.                                             *
 *=========================================================================*/
uint32_t SessionClass::Compute_Unique_ID(void)
{
#ifdef REMASTER_BUILD
    return 1; // PG
#else         // PG
    time_t tm;
    unsigned int id;
    char* path;
    int i;

    //------------------------------------------------------------------------
    // Start with the seconds since Jan 1, 1970 (system local time)
    //------------------------------------------------------------------------
    time(&tm);
    id = (unsigned int)tm;

    //------------------------------------------------------------------------
    // Add in every byte in the user's path environment variable
    //------------------------------------------------------------------------
    path = getenv("PATH");
    if (path) {
        for (i = 0; i < strlen(path); i++) {
            Add_CRC(&id, (unsigned int)path[i]);
        }
    }

    return (id);
#endif
} // end of Compute_Unique_ID

MultiMission::MultiMission(char const* filename,
                           char const* description,
                           char const* digest,
                           bool official,
                           bool expansion)
{
    Set_Filename(filename);
    Set_Description(description);
    Set_Digest(digest);
    Set_Official(official);
    Set_Expansion(expansion);
}

void MultiMission::Draw_It(int, int x, int y, int width, int height, bool selected, TextPrintType flags) const
{
    RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
    static int _tabs[] = {35, 60, 80, 100};
    if ((flags & 0x0F) == TPF_6PT_GRAD || (flags & 0x0F) == TPF_EFNT) {

        if (selected) {
            flags = flags | TPF_BRIGHT_COLOR;
            LogicPage->Fill_Rect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
        } else {
            if (!(flags & TPF_USE_GRAD_PAL)) {
                flags = flags | TPF_MEDIUM_COLOR;
            }
        }

        Conquer_Clip_Text_Print(ScenarioDescription, x, y, scheme, TBLACK, flags, width, _tabs);
    } else {
        Conquer_Clip_Text_Print(ScenarioDescription,
                                x,
                                y,
                                (selected ? &ColorRemaps[PCOLOR_DIALOG_BLUE] : &ColorRemaps[PCOLOR_GREY]),
                                TBLACK,
                                flags,
                                width,
                                _tabs);
    }
}

void MultiMission::Set_Description(char const* description)
{
    if (description != NULL) {
        strncpy(ScenarioDescription, description, ARRAY_SIZE(ScenarioDescription));
        ScenarioDescription[ARRAY_SIZE(ScenarioDescription) - 1] = '\0';
    }
}

void MultiMission::Set_Filename(char const* filename)
{
    if (filename != NULL) {
        strncpy(Filename, filename, ARRAY_SIZE(Filename));
        Filename[ARRAY_SIZE(Filename) - 1] = '\0';
    }
}

void MultiMission::Set_Digest(char const* digest)
{
    if (digest != NULL) {
        strncpy(Digest, digest, ARRAY_SIZE(Digest));
        Digest[ARRAY_SIZE(Digest) - 1] = '\0';
    } else {
        strcpy(Digest, "NODIGEST");
    }
}

void MultiMission::Set_Official(bool official)
{
    IsOfficial = official;
}

void MultiMission::Set_Expansion(bool expansion)
{
    IsExpansion = expansion;
}

/************************** end of session.cpp *****************************/
