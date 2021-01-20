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

/* $Header: /CounterStrike/MAPEDIT.CPP 2     3/13/97 2:05p Steve_tall $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MAPEDIT.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : October 20, 1994                         *
 *                                                                         *
 *                  Last Update : February 2, 1995   [BR]                  *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *   Map Editor overloaded routines & utility routines                     *
 *-------------------------------------------------------------------------*
 * Map Editor modules:                                                     *
 * (Yes, they're all one huge class.)                                      *
 *      mapedit.cpp:   overloaded routines, utility routines               *
 *      mapeddlg.cpp:   map editor dialogs, most of the main menu options  *
 *      mapedplc.cpp:   object-placing routines                            *
 *      mapedsel.cpp:   object-selection & manipulation routines           *
 *      mapedtm.cpp:   team-editing routines                               *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::AI -- The map editor's main logic                       *
 *   MapEditClass::Read_INI -- overloaded Read_INI function                *
 *   MapEditClass::AI_Menu -- menu of AI options                           *
 *   MapEditClass::Add_To_List -- adds a TypeClass to the chooseable list  *
 *   MapEditClass::Clear_List -- clears the internal chooseable object list*
 *   MapEditClass::Cycle_House -- finds next valid house for object type   *
 *   MapEditClass::Draw_It -- overloaded Redraw routine                    *
 *   MapEditClass::Fatal -- exits with error message                       *
 *   MapEditClass::Main_Menu -- main menu processor for map editor         *
 *   MapEditClass::MapEditClass -- class constructor                       *
 *   MapEditClass::Mouse_Moved -- checks for mouse motion                  *
 *   MapEditClass::One_Time -- one-time initialization                     *
 *   MapEditClass::Verify_House -- sees if given house can own given obj   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "common/framelimit.h"

#ifdef SCENARIO_EDITOR
#include "dial8.h"
#include "edit.h"
#include "list.h"
#include "txtlabel.h"
#include "textbtn.h"
#include "msgbox.h"

/*
**	Array of all missions supported by the map editor
*/
MissionType MapEditClass::MapEditMissions[NUM_EDIT_MISSIONS] = {
    MISSION_GUARD,
    MISSION_STICKY,
    MISSION_HARMLESS,
    MISSION_HARVEST,
    MISSION_GUARD_AREA,
    MISSION_RETURN,
    MISSION_AMBUSH,
    MISSION_HUNT,
    MISSION_SLEEP,
};

/*
**	For menu processing
*/
extern int UnknownKey; // in menus.cpp

char MapEditClass::HealthBuf[20];

/***************************************************************************
 * MapEditClass::MapEditClass -- class constructor                         *
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
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
MapEditClass::MapEditClass(void)
{
    /*
    **	Init data members.
    */
    //	ScenVar = SCEN_VAR_A;
    ObjCount = 0;
    LastChoice = 0;
    LastHouse = HOUSE_GOOD;
    GrabbedObject = 0;
    for (int i = 0; i < NUM_EDIT_CLASSES; i++) {
        NumType[i] = 0;
        TypeOffset[i] = 0;
    }
    Scen.Waypoint[WAYPT_HOME] = 0;
    CurrentCell = 0;
    CurTeam = NULL;
    CurTrigger = NULL;
    Changed = 0;
    LMouseDown = 0;
    BaseBuilding = false;
    //	BasePercent = 100;
}

/***************************************************************************
 * MapEditClass::One_Time -- one-time initialization                       *
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
 *   02/02/1995 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::One_Time(void)
{
    MouseClass::One_Time();

    /*
    **	The map: a single large "button"
    */
    MapArea =
        new ControlClass(MAP_AREA, 0, 8, 640 - 8, 400 - 8, GadgetClass::LEFTPRESS | GadgetClass::LEFTRELEASE, false);

    /*
    **	House buttons
    */
    HouseList = new ListClass(POPUP_HOUSELIST,
                              POPUP_HOUSE_X,
                              POPUP_HOUSE_Y,
                              POPUP_HOUSE_W,
                              POPUP_HOUSE_H,
                              TPF_EFNT | TPF_NOSHADOW,
                              MFCD::Retrieve("EBTN-UP.SHP"),
                              MFCD::Retrieve("EBTN-DN.SHP"));
    for (HousesType house = HOUSE_FIRST; house < HOUSE_COUNT; house++) {
        HouseList->Add_Item(HouseTypeClass::As_Reference(house).IniName);
    }

    /*
    **	The mission list box
    */
    MissionList = new ListClass(POPUP_MISSIONLIST,
                                POPUP_MISSION_X,
                                POPUP_MISSION_Y,
                                POPUP_MISSION_W,
                                POPUP_MISSION_H,
                                TPF_EFNT | TPF_NOSHADOW,
                                MFCD::Retrieve("EBTN-UP.SHP"),
                                MFCD::Retrieve("EBTN-DN.SHP"));

    for (int i = 0; i < NUM_EDIT_MISSIONS; i++) {
        MissionList->Add_Item(MissionClass::Mission_Name(MapEditMissions[i]));
    }

    /*
    **	The health bar
    */
    HealthGauge =
        new TriColorGaugeClass(POPUP_HEALTHGAUGE, POPUP_HEALTH_X, POPUP_HEALTH_Y, POPUP_HEALTH_W, POPUP_HEALTH_H);
    HealthGauge->Use_Thumb(true);
    HealthGauge->Set_Maximum(0x100);
    HealthGauge->Set_Red_Limit(0x3f - 1);
    HealthGauge->Set_Yellow_Limit(0x7f - 1);

    /*
    **	The health text label
    */
    HealthBuf[0] = 0;
    HealthText = new TextLabelClass(HealthBuf,
                                    POPUP_HEALTH_X + POPUP_HEALTH_W / 2,
                                    POPUP_HEALTH_Y + POPUP_HEALTH_H + 1,
                                    GadgetClass::Get_Color_Scheme(),
                                    TPF_CENTER | TPF_FULLSHADOW | TPF_EFNT);

    /*
    **	Building attribute buttons.
    */
    Sellable = new TextButtonClass(POPUP_SELLABLE, TXT_SELLABLE, TPF_EBUTTON, 320 - 65, 200 - 25, 60);
    Rebuildable = new TextButtonClass(POPUP_REBUILDABLE, TXT_REBUILD, TPF_EBUTTON, 320 - 65, 200 - 15, 60);

    /*
    **	The facing dial
    */
    FacingDial = new Dial8Class(
        POPUP_FACINGDIAL, POPUP_FACEBOX_X, POPUP_FACEBOX_Y, POPUP_FACEBOX_W, POPUP_FACEBOX_H, (DirType)0);

    /*
    **	The base percent-built slider & its label
    */
    BaseGauge = new GaugeClass(POPUP_BASEPERCENT, POPUP_BASE_X, POPUP_BASE_Y, POPUP_BASE_W, POPUP_BASE_H);
    BaseLabel = new TextLabelClass(
        "Base:", POPUP_BASE_X - 3, POPUP_BASE_Y, GadgetClass::Get_Color_Scheme(), TPF_RIGHT | TPF_NOSHADOW | TPF_EFNT);
    BaseGauge->Set_Maximum(100);
    BaseGauge->Set_Value(Scen.Percent);
}

/***********************************************************************************************
 * MapeditClass::Init_IO -- Reinitializes the radar map at scenario start.                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/22/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void MapEditClass::Init_IO(void)
{
    /*
    **	For normal game mode, jump to the parent's Init routine.
    */
    if (!Debug_Map) {

        MouseClass::Init_IO();

    } else {

        /*
        **	For editor mode, add the map area to the button input list
        */
        Buttons = 0;
        Add_A_Button(*BaseGauge);
        Add_A_Button(*BaseLabel);
        Add_A_Button(*MapArea);
    }
}

/***************************************************************************
 * MapEditClass::Clear_List -- clears the internal chooseable object list   *
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
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Clear_List(void)
{
    /*
    **	Set # object type ptrs to 0, set NumType for each type to 0
    */
    ObjCount = 0;
    for (int i = 0; i < NUM_EDIT_CLASSES; i++) {
        NumType[i] = 0;
    }
}

/***************************************************************************
 * MapEditClass::Add_To_List -- adds a TypeClass to the chooseable list     *
 *                                                                         *
 * Use this routine to add an object to the game object selection list.    *
 * This list is used by the Add_Object function. All items located in the  *
 * list will appear and be chooseable by that function. Make sure to       *
 * clear the list before adding a sequence of items to it. Clearing        *
 * the list is accomplished by the Clear_List() function.                  *
 *                                                                         *
 * INPUT:                                                                  *
 *      object      ptr to ObjectTypeClass to add                          *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      bool: was the object added to the list?  A failure could occur if  *
 *      NULL were passed in or the list is full.                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/04/1994 JLB : Created.                                             *
 *=========================================================================*/
bool MapEditClass::Add_To_List(ObjectTypeClass const* object)
{
    /*
    **	Add the object if there's room.
    */
    if (object && ObjCount < MAX_EDIT_OBJECTS) {
        Objects[ObjCount++] = object;

        /*
        **	Update type counters.
        */
        switch (object->What_Am_I()) {
        case RTTI_TEMPLATETYPE:
            NumType[0]++;
            break;

        case RTTI_OVERLAYTYPE:
            NumType[1]++;
            break;

        case RTTI_SMUDGETYPE:
            NumType[2]++;
            break;

        case RTTI_TERRAINTYPE:
            NumType[3]++;
            break;

        case RTTI_UNITTYPE:
            NumType[4]++;
            break;

        case RTTI_INFANTRYTYPE:
            NumType[5]++;
            break;

        case RTTI_VESSELTYPE:
            NumType[6]++;
            break;

        case RTTI_BUILDINGTYPE:
            NumType[7]++;
            break;

        case RTTI_AIRCRAFTTYPE:
            NumType[8]++;
            break;
        }
        return (true);
    }

    return (false);
}

/***************************************************************************
 * MapEditClass::AI -- The map editor's main logic                         *
 *                                                                         *
 * This routine overloads the parent's (DisplayClass) AI function.         *
 * It checks for any input specific to map editing, and calls the parent   *
 * AI routine to handle scrolling and other mainstream map stuff.          *
 *                                                                         *
 * If this detects one of its special input keys, it sets 'input' to 0     *
 * before calling the parent AI routine; this prevents input conflict.     *
 *                                                                         *
 * SUPPORTED INPUT:                                                        *
 * General:                                                                *
 *      F2/RMOUSE:            main menu                                    *
 *      F6:                  toggles show-passable mode                    *
 *      HOME:                  go to the Home Cell (scenario's start position)*
 *      SHIFT-HOME:            set the Home Cell to the current TacticalCell*
 *      ESC:                  exits to DOS                                 *
 * Object Placement:                                                       *
 *      INSERT:               go into placement mode                       *
 *      ESC:                  exit placement mode                          *
 *      LEFT/RIGHT:          prev/next placement object                    *
 *      PGUP/PGDN:            prev/next placement category                 *
 *      HOME:                  1st placement object (clear template)       *
 *      h/H:                  toggle house of placement object             *
 *      LMOUSE:               place the placement object                   *
 *      MOUSE MOTION:         "paint" with the placement object            *
 * Object selection:                                                       *
 *      LMOUSE:               select & "grab" current object               *
 *                           If no object is present where the mouse is    *
 *                           clicked, the current object is de-selected    *
 *                           If the same object is clicked on, it stays    *
 *                           selected. Also displays the object-editing    *
 *                           gadgets.                                      *
 *      LMOUSE RLSE:         release currently-grabbed object              *
 *      MOUSE MOTION:         if an object is grabbed, moves the object    *
 *      SHIFT|ALT|ARROW:      moves object in that direction               *
 *      DELETE               deletes currently-selected object             *
 * Object-editing controls:                                                *
 *      POPUP_GDI:            makes GDI the owner of this object           *
 *      POPUP_NOD:            makes NOD the owner of this object           *
 *      POPUP_MISSIONLIST:   sets that mission for this object             *
 *      POPUP_HEALTHGAUGE:   sets that health value for this object        *
 *      POPUP_FACINGDIAL:      sets the object's facing                    *
 *                                                                         *
 * Changed is set when you:                                                *
 *      - place an object                                                  *
 *      - move a grabbed object                                            *
 *      - delete an object                                                 *
 *      - size the map                                                     *
 *      - create a new scenario                                            *
 *   Changed is cleared when you:                                          *
 *      - Save the scenario                                                *
 *      - Load a scenario                                                  *
 *      - Play the scenario                                                *
 *                                                                         *
 * INPUT:                                                                  *
 *      input      KN_ value, 0 if none                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::AI(KeyNumType& input, int x, int y)
{
    int rc;
    MissionType mission;
    int strength;
    CELL cell;
    int i;
    int found;           // for removing a waypoint label
    int waypt_idx;       // for labelling a waypoint
    BaseNodeClass* node; // for removing from an AI Base
    HousesType house;
    char wayname[4];

    /*
    **	Trap 'F2' regardless of whether we're in game or editor mode
    */
    if (Debug_Flag) {
        if ((input == KN_F2 && Session.Type == GAME_NORMAL) || input == (KN_F2 | KN_CTRL_BIT)) {
            ScenarioInit = 0;

            /*
            ** If we're in editor mode & Changed is set, prompt for saving changes
            */
            if (Debug_Map && Changed) {
                rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
                HidPage.Clear();
                Flag_To_Redraw(true);
                Render();

                /*
                **	User wants to save
                */
                if (rc == 0) {

                    /*
                    **	If save cancelled, abort game
                    */
                    if (Save_Scenario() != 0) {
                        input = KN_NONE;
                    } else {
                        Changed = 0;
                        Go_Editor(!Debug_Map);
                    }
                } else {

                    /*
                    **	User doesn't want to save
                    */
                    Go_Editor(!Debug_Map);
                }
            } else {
                /*
                ** If we're in game mode, set Changed to 0 (so if we didn't save our
                ** changes above, they won't keep coming back to haunt us with continual
                ** Save Changes? prompts!)
                */
                if (!Debug_Map) {
                    Changed = 0;
                }
                BaseGauge->Set_Value(Scen.Percent);
                Go_Editor(!Debug_Map);
            }
        }
    }

    /*
    **	For normal game mode, jump to the parent's AI routine.
    */
    if (!Debug_Map) {
        MouseClass::AI(input, x, y);
        return;
    }

    ::Frame++;

    /*
    **	Do special mouse processing if the mouse is over the map
    */
    if (Get_Mouse_X() > TacPixelX && Get_Mouse_X() < TacPixelX + Lepton_To_Pixel(TacLeptonWidth)
        && Get_Mouse_Y() > TacPixelY && Get_Mouse_Y() < TacPixelY + Lepton_To_Pixel(TacLeptonHeight)) {

        /*
        **	When the mouse moves over a scrolling edge, ScrollClass changes its
        **	shape to the appropriate arrow or NO symbol; it's our job to change it
        **	back to normal (or whatever the shape is set to by Set_Default_Mouse())
        **	when it re-enters the map area.
        */
        if (CurTrigger) {
            Override_Mouse_Shape(MOUSE_CAN_MOVE);
        } else {
            Override_Mouse_Shape(MOUSE_NORMAL);
        }
    }

    /*
    **	Set 'ZoneCell' to track the mouse cursor around over the map.  Do this
    **	even if the map is scrolling.
    */
    if (Get_Mouse_X() >= TacPixelX && Get_Mouse_X() <= TacPixelX + Lepton_To_Pixel(TacLeptonWidth)
        && Get_Mouse_Y() >= TacPixelY && Get_Mouse_Y() <= TacPixelY + Lepton_To_Pixel(TacLeptonHeight)) {

        cell = Click_Cell_Calc(Get_Mouse_X(), Get_Mouse_Y());
        if (cell != -1) {
            Set_Cursor_Pos(cell);
            if (PendingObject) {
                Flag_To_Redraw(true);
            }
        }
    }

    /*
    **	Check for mouse motion while left button is down.
    */
    rc = Mouse_Moved();
    if (LMouseDown && rc) {

        /*
        **	"Paint" mode: place current object, and restart placement
        */
        if (PendingObject) {
            Flag_To_Redraw(true);
            if (Place_Object() == 0) {
                Changed = 1;
                Start_Placement();
            }
        } else {

            /*
            **	Move the currently-grabbed object
            */
            if (GrabbedObject) {
                GrabbedObject->Mark(MARK_CHANGE);
                if (Move_Grabbed_Object() == 0) {
                    Changed = 1;
                }
            }
        }
    }

    /*
    **	Trap special editing keys; if one is detected, set 'input' to 0 to
    **	prevent a conflict with parent's AI().
    */
    switch (input) {
    /*
    ** F2/RMOUSE = pop up main menu
    */
    case KN_RMOUSE:

        /*
        **	Turn off placement mode
        */
        if (PendingObject) {
            if (BaseBuilding) {
                Cancel_Base_Building();
            } else {
                Cancel_Placement();
            }
        }

        /*
        **	Turn off trigger placement mode
        */
        if (CurTrigger) {
            Stop_Trigger_Placement();
        }

        /*
        **	Unselect object & hide popup controls
        */
        if (CurrentObject.Count()) {
            CurrentObject[0]->Unselect();
            Popup_Controls();
        }
        Main_Menu();
        input = KN_NONE;
        break;

    /*
    **	F6 = toggle passable/impassable display
    */
    case KN_F6:
        Debug_Passable = (Debug_Passable == false);
        HidPage.Clear();
        Flag_To_Redraw(true);
        input = KN_NONE;
        break;

    /*
    **	INSERT = go into object-placement mode
    */
    case KN_INSERT:
        if (!PendingObject) {
            /*
            **	Unselect current object, hide popup controls
            */
            if (CurrentObject.Count()) {
                CurrentObject[0]->Unselect();
                Popup_Controls();
            }

            /*
            **	Go into placement mode
            */
            Start_Placement();
        }
        input = KN_NONE;
        break;

    /*
    **	ESC = exit placement mode, or exit to DOS
    */
    case KN_ESC:

        /*
        **	Exit object placement mode
        */
        if (PendingObject) {
            if (BaseBuilding) {
                Cancel_Base_Building();
            } else {
                Cancel_Placement();
            }
            input = KN_NONE;
            break;
        } else {

            /*
            **	Exit trigger placement mode
            */
            if (CurTrigger) {
                Stop_Trigger_Placement();
                input = KN_NONE;
                break;
            } else {
                rc = WWMessageBox().Process("Exit Scenario Editor?", TXT_YES, TXT_NO);
                HidPage.Clear();
                Flag_To_Redraw(true);
                Render();

                /*
                **	User doesn't want to exit; return to editor
                */
                if (rc == 1) {
                    input = KN_NONE;
                    break;
                }

                /*
                **	If changed, prompt for saving
                */
                if (Changed) {
                    rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
                    HidPage.Clear();
                    Flag_To_Redraw(true);
                    Render();

                    /*
                    **	User wants to save
                    */
                    if (rc == 0) {

                        /*
                        **	If save cancelled, abort exit
                        */
                        if (Save_Scenario() != 0) {
                            input = KN_NONE;
                            break;
                        } else {
                            Changed = 0;
                        }
                    }
                }
            }
        }
        // Prog_End();
        Emergency_Exit(0);
        break;

    /*
    **	LEFT = go to previous placement object
    */
    case KN_LEFT:
        if (PendingObject) {
            Place_Prev();
        }
        input = KN_NONE;
        break;

    /*
    **	RIGHT = go to next placement object
    */
    case KN_RIGHT:
        if (PendingObject) {
            Place_Next();
        }
        input = KN_NONE;
        break;

    /*
    **	PGUP = go to previous placement category
    */
    case KN_PGUP:
        if (PendingObject) {
            Place_Prev_Category();
        }
        input = KN_NONE;
        break;

    /*
    **	PGDN = go to next placement category
    */
    case KN_PGDN:
        if (PendingObject) {
            Place_Next_Category();
        }
        input = KN_NONE;
        break;

    /*
    **	HOME = jump to first placement object, or go to Home Cell
    */
    case KN_HOME:
        if (PendingObject) {
            Place_Home();
        } else {

            /*
            **	Set map position
            */
            ScenarioInit++;
            Set_Tactical_Position(Scen.Waypoint[WAYPT_HOME]);
            ScenarioInit--;

            /*
            **	Force map to redraw
            */
            HidPage.Clear();
            Flag_To_Redraw(true);
            Render();
        }
        input = KN_NONE;
        break;

    /*
    **	SHIFT-HOME: set new Home Cell position
    */
    case ((int)KN_HOME | (int)KN_SHIFT_BIT):
        if (CurrentCell != 0) {

            /*
            ** Unflag the old Home Cell, if there are no other waypoints
            ** pointing to it
            */
            cell = Scen.Waypoint[WAYPT_HOME];

            if (cell != -1) {
                found = 0;
                for (i = 0; i < WAYPT_COUNT; i++) {
                    if (i != WAYPT_HOME && Scen.Waypoint[i] == cell) {
                        found = 1;
                    }
                }

                if (found == 0) {
                    (*this)[cell].IsWaypoint = 0;
                    Flag_Cell(cell);
                }
            }

            /*
            ** Now set the new Home cell
            */
            //			Scen.Waypoint[WAYPT_HOME] = Coord_Cell(TacticalCoord);
            //			(*this)[TacticalCoord].IsWaypoint = 1;
            //			Flag_Cell(Coord_Cell(TacticalCoord));
            Scen.Waypoint[WAYPT_HOME] = CurrentCell;
            (*this)[CurrentCell].IsWaypoint = 1;
            Flag_Cell(CurrentCell);

            Changed = 1;
            input = KN_NONE;
        }
        break;

    /*
    **	SHIFT-R: set new Reinforcement Cell position.  Don't allow setting
    **	the Reinf. Cell to the same as the Home Cell (for display purposes.)
    */
    case ((int)KN_R | (int)KN_SHIFT_BIT):
        if (CurrentCell == 0 || CurrentCell == Scen.Waypoint[WAYPT_HOME]) {
            break;
        }

        /*
        ** Unflag the old Reinforcement Cell, if there are no other waypoints
        ** pointing to it
        */
        cell = Scen.Waypoint[WAYPT_REINF];

        if (cell != -1) {
            found = 0;
            for (i = 0; i < WAYPT_COUNT; i++) {
                if (i != WAYPT_REINF && Scen.Waypoint[i] == cell) {
                    found = 1;
                }
            }

            if (found == 0) {
                (*this)[cell].IsWaypoint = 0;
                Flag_Cell(cell);
            }
        }
        /*
        ** Now set the new Reinforcement cell
        */
        Scen.Waypoint[WAYPT_REINF] = CurrentCell;
        (*this)[CurrentCell].IsWaypoint = 1;
        Flag_Cell(CurrentCell);
        Changed = 1;
        input = KN_NONE;
        break;

    /*
    **	ALT-Letter: Label a waypoint cell
    */
    case ((int)KN_A | (int)KN_ALT_BIT):
    case ((int)KN_B | (int)KN_ALT_BIT):
    case ((int)KN_C | (int)KN_ALT_BIT):
    case ((int)KN_D | (int)KN_ALT_BIT):
    case ((int)KN_E | (int)KN_ALT_BIT):
    case ((int)KN_F | (int)KN_ALT_BIT):
    case ((int)KN_G | (int)KN_ALT_BIT):
    case ((int)KN_H | (int)KN_ALT_BIT):
    case ((int)KN_I | (int)KN_ALT_BIT):
    case ((int)KN_J | (int)KN_ALT_BIT):
    case ((int)KN_K | (int)KN_ALT_BIT):
    case ((int)KN_L | (int)KN_ALT_BIT):
    case ((int)KN_M | (int)KN_ALT_BIT):
    case ((int)KN_N | (int)KN_ALT_BIT):
    case ((int)KN_O | (int)KN_ALT_BIT):
    case ((int)KN_P | (int)KN_ALT_BIT):
    case ((int)KN_Q | (int)KN_ALT_BIT):
    case ((int)KN_R | (int)KN_ALT_BIT):
    case ((int)KN_S | (int)KN_ALT_BIT):
    case ((int)KN_T | (int)KN_ALT_BIT):
    case ((int)KN_U | (int)KN_ALT_BIT):
    case ((int)KN_V | (int)KN_ALT_BIT):
    case ((int)KN_W | (int)KN_ALT_BIT):
    case ((int)KN_X | (int)KN_ALT_BIT):
    case ((int)KN_Y | (int)KN_ALT_BIT):
    case ((int)KN_Z | (int)KN_ALT_BIT):
        if (CurrentCell != 0) {
            waypt_idx = (input & ~KN_ALT_BIT) - KN_A;
            Update_Waypoint(waypt_idx);
        }
        input = KN_NONE;
        break;

    /*
    ** ALT-. : Designate an extended (2-letter) waypoint name
    */
    case KN_PERIOD:
    case ((int)KN_PERIOD | (int)KN_ALT_BIT):
        if (CurrentCell != 0 && Get_Waypoint_Name(wayname)) {
            int waynm = 0;
            if (strlen(wayname)) {
                wayname[0] = toupper(wayname[0]);
                wayname[1] = toupper(wayname[1]);
                if (wayname[0] >= 'A' && wayname[0] <= 'Z') {
                    waynm = wayname[0] - 'A';
                    if (wayname[1] >= 'A' && wayname[1] <= 'Z') {
                        waynm = (waynm + 1) * 26 + (wayname[1] - 'A');
                    }
                    if (waynm < WAYPT_HOME)
                        Update_Waypoint(waynm);
                }
            }
        }
        input = KN_NONE;
        break;

#ifdef OBSOLETE
    /*
    **	ALT-1-4: Designate a cell as a capture-the-flag cell.
    */
    case ((int)KN_1 | (int)KN_ALT_BIT):
    case ((int)KN_2 | (int)KN_ALT_BIT):
    case ((int)KN_3 | (int)KN_ALT_BIT):
    case ((int)KN_4 | (int)KN_ALT_BIT):

        /*
        **	If there's a current cell, place the flag & waypoint there.
        */
        if (CurrentCell != 0) {
            waypt_idx = (Keyboard->To_ASCII((KeyNumType)(input & 0xff)) - KA_1);
            //				waypt_idx = (KN_To_KA(input & 0xff) - KA_1);
            house = (HousesType)(HOUSE_MULTI1 + waypt_idx);
            if (HouseClass::As_Pointer(house)) {
                HouseClass::As_Pointer(house)->Flag_Attach(CurrentCell, true);
            }
        } else {

            /*
            **	If there's a current object, attach the flag to it and clear the
            **	waypoint.
            */
            if (CurrentObject[0] != 0) {
                waypt_idx = (Keyboard->To_ASCII((KeyNumType)(input & 0xff)) - KA_1);
                house = (HousesType)(HOUSE_MULTI1 + waypt_idx);
                if (HouseClass::As_Pointer(house) && CurrentObject[0]->What_Am_I() == RTTI_UNIT) {
                    HouseClass::As_Pointer(house)->Flag_Attach((UnitClass*)CurrentObject[0], true);
                }
            }
        }
        input = KN_NONE;
        break;
#endif

    /*
    **	ALT-Space: Remove a waypoint designation
    */
    case ((int)KN_SPACE | (int)KN_ALT_BIT):
        if (CurrentCell != 0) {

            /*
            **	Loop through letter waypoints; if this cell is one of them,
            **	clear that waypoint.
            */
            for (i = 0; i < WAYPT_HOME; i++) {
                if (Scen.Waypoint[i] == CurrentCell)
                    Scen.Waypoint[i] = -1;
            }

            /*
            **	Loop through flag home values; if this cell is one of them, clear
            **	that waypoint.
            */
            for (i = 0; i < MAX_PLAYERS; i++) {
                house = (HousesType)(HOUSE_MULTI1 + i);
                if (HouseClass::As_Pointer(house) && CurrentCell == HouseClass::As_Pointer(house)->FlagHome)
                    HouseClass::As_Pointer(house)->Flag_Remove(As_Target(CurrentCell), true);
            }

            /*
            **	If there are no more waypoints on this cell, clear the cell's
            **	waypoint designation.
            */
            if (Scen.Waypoint[WAYPT_HOME] != CurrentCell && Scen.Waypoint[WAYPT_REINF] != CurrentCell)
                (*this)[CurrentCell].IsWaypoint = 0;
            Changed = 1;
            Flag_Cell(CurrentCell);
        }
        input = KN_NONE;
        break;

    /*
    **	'H' = toggle current placement object's house
    */
    case KN_H:
    case ((int)KN_H | (int)KN_SHIFT_BIT):
        if (PendingObject) {
            Toggle_House();
        }
        input = KN_NONE;
        break;

    /*
    **	Left-mouse click:
    **	Button DOWN:
    **	- Toggle LMouseDown
    **	- If we're in placement mode, try to place the current object
    **	- If success, re-enter placement mode
    **	- Otherwise, try to select an object, and "grab" it if there is one
    **	- If no object, then select that cell as the "current" cell
    **	Button UP:
    **	- Toggle LMouseDown
    **	- release any grabbed object
    */
    case ((int)MAP_AREA | (int)KN_BUTTON):

        /*
        **	Left Button DOWN
        */
        if (Keyboard->Down(KN_LMOUSE)) {
            LMouseDown = 1;

            /*
            **	Placement mode: place an object
            */
            if (PendingObject) {
                if (Place_Object() == 0) {
                    Changed = 1;
                    Start_Placement();
                }
            } else {

                /*
                **	Place a trigger
                */
                if (CurTrigger) {
                    Place_Trigger();
                    Changed = 1;
                } else {

                    /*
                    **	Select an object or a cell
                    **	Check for double-click
                    */
                    if (CurrentObject.Count() && ((TickCount - LastClickTime) < 15)) {
                        ; // stub

                    } else {
                        /*
                        **	Single-click: select object
                        */
                        if (Select_Object() == 0) {
                            CurrentCell = 0;
                            Grab_Object();
                        } else {

                            /*
                            **	No object: select the cell
                            */
                            CurrentCell = Click_Cell_Calc(Keyboard->MouseQX, Keyboard->MouseQY);
                            HidPage.Clear();
                            Flag_To_Redraw(true);
                            Render();
                        }
                    }
                }
            }
            LastClickTime = WinTickCount.Time();
            input = KN_NONE;
        } else {

            /*
            **	Left Button UP
            */
            LMouseDown = 0;
            GrabbedObject = 0;
            input = KN_NONE;
        }
        break;

    /*
    **	SHIFT-ALT-Arrow: move the current object
    */
    case (int)KN_UP | (int)KN_ALT_BIT | (int)KN_SHIFT_BIT:
    case (int)KN_DOWN | (int)KN_ALT_BIT | (int)KN_SHIFT_BIT:
    case (int)KN_LEFT | (int)KN_ALT_BIT | (int)KN_SHIFT_BIT:
    case (int)KN_RIGHT | (int)KN_ALT_BIT | (int)KN_SHIFT_BIT:
        if (CurrentObject.Count()) {
            CurrentObject[0]->Move(KN_To_Facing(input));
            Changed = 1;
        }
        input = KN_NONE;
        break;

    /*
    **	DELETE: delete currently-selected object
    */
    case KN_DELETE:

        /*
        **	Delete currently-selected object's trigger, or the object
        */
        if (CurrentObject.Count()) {

            /*
            **	Delete trigger
            */
            if (CurrentObject[0]->Trigger.Is_Valid()) {
                CurrentObject[0]->Trigger = NULL;
            } else {
                /*
                ** If the current object is part of the AI's Base, remove it
                ** from the Base's Node list.
                */
                if (CurrentObject[0]->What_Am_I() == RTTI_BUILDING && Base.Is_Node((BuildingClass*)CurrentObject[0])) {
                    node = Base.Get_Node((BuildingClass*)CurrentObject[0]);
                    Base.Nodes.Delete(*node);
                }

                /*
                **	Delete current object
                */
                delete CurrentObject[0];

                /*
                **	Hide the popup controls
                */
                Popup_Controls();
            }

            /*
            **	Force a redraw
            */
            HidPage.Clear();
            Flag_To_Redraw(true);
            Changed = 1;
        } else {

            /*
            **	Remove trigger from current cell
            */
            if (CurrentCell) {
                if ((*this)[CurrentCell].Trigger.Is_Valid()) {
                    (*this)[CurrentCell].Trigger = NULL;
                    //						CellTriggers[CurrentCell] = NULL;

                    /*
                    **	Force a redraw
                    */
                    HidPage.Clear();
                    Flag_To_Redraw(true);
                    Changed = 1;
                }
            }
        }
        input = KN_NONE;
        break;

    /*
    **	TAB: select next object on the map
    */
    case KN_TAB:
        Select_Next();
        input = KN_NONE;
        break;

    /*
    **	Object-Editing button: House Button
    */
    case POPUP_HOUSELIST | KN_BUTTON:
        /*
        **	Determine the house desired by examining the currently
        **	selected index in the house list gadget.
        */
        house = HousesType(((ListClass*)Buttons->Extract_Gadget(POPUP_HOUSELIST))->Current_Index());

        /*
        **	If that house doesn't own this object, try to transfer it
        */
        if (CurrentObject[0]->Owner() != house) {
            if (Change_House(house)) {
                Changed = 1;
            }
        }
        //			Set_House_Buttons(CurrentObject[0]->Owner(), Buttons, POPUP_FIRST);
        HidPage.Clear();
        Buttons->Flag_List_To_Redraw();
        Flag_To_Redraw(true);
        input = KN_NONE;
        break;

        //		case (POPUP_GDI | KN_BUTTON):
        //		case (POPUP_NOD | KN_BUTTON):
        //		case (POPUP_NEUTRAL | KN_BUTTON):
        //		case (POPUP_MULTI1 | KN_BUTTON):
        //		case (POPUP_MULTI2 | KN_BUTTON):
        //		case (POPUP_MULTI3 | KN_BUTTON):
        //		case (POPUP_MULTI4 | KN_BUTTON):
        //
        //			/*
        //			**	Convert input value into a house value; assume HOUSE_GOOD is 0
        //			*/
        //			house = (HousesType)( (input & (~KN_BUTTON)) - POPUP_FIRST);
        //
        //			/*
        //			**	If that house doesn't own this object, try to transfer it
        //			*/
        //			if (CurrentObject[0]->Owner()!=house) {
        //				if (Change_House(house)) {
        //					Changed = 1;
        //				}
        //			}
        //			Set_House_Buttons(CurrentObject[0]->Owner(), Buttons, POPUP_FIRST);
        //			HidPage.Clear();
        //			Flag_To_Redraw(true);
        //			input = KN_NONE;
        //			break;

    case POPUP_SELLABLE | KN_BUTTON:
        if (CurrentObject[0]->What_Am_I() == RTTI_BUILDING) {
            BuildingClass* building = (BuildingClass*)CurrentObject[0];

            if (building->Class->Level != -1) {
                //				if (building->Class->IsBuildable) {
                building->IsAllowedToSell = (building->IsAllowedToSell == false);
                building->Mark(MARK_CHANGE);
            }
            if (building->IsAllowedToSell) {
                Sellable->Turn_On();
            } else {
                Sellable->Turn_Off();
            }
        }
        break;

    case POPUP_REBUILDABLE | KN_BUTTON:
        if (CurrentObject[0]->What_Am_I() == RTTI_BUILDING) {
            BuildingClass* building = (BuildingClass*)CurrentObject[0];

            if (building->Class->Level != -1) {
                //				if (building->Class->IsBuildable) {
                building->IsToRebuild = (building->IsToRebuild == false);
                building->Mark(MARK_CHANGE);
            }
            if (building->IsToRebuild) {
                Rebuildable->Turn_On();
            } else {
                Rebuildable->Turn_Off();
            }
        }
        break;

    /*
    **	Object-Editing button: Mission
    */
    case (POPUP_MISSIONLIST | KN_BUTTON):
        if (CurrentObject[0]->Is_Techno()) {
            /*
            **	Set new mission
            */
            mission = MapEditMissions[MissionList->Current_Index()];
            if (CurrentObject[0]->Get_Mission() != mission) {
                ((TechnoClass*)CurrentObject[0])->Set_Mission(mission);
                Changed = 1;
                Buttons->Flag_List_To_Redraw();
                Flag_To_Redraw(true);
            }
        }
        input = KN_NONE;
        break;

    /*
    **	Object-Editing button: Health
    */
    case (POPUP_HEALTHGAUGE | KN_BUTTON):
        if (CurrentObject[0]->Is_Techno()) {

            /*
            **	Derive strength from current gauge reading
            */
            strength = (int)CurrentObject[0]->Class_Of().MaxStrength * fixed(HealthGauge->Get_Value(), 256);
            //				strength = Fixed_To_Cardinal((unsigned)CurrentObject[0]->Class_Of().MaxStrength,
            //(unsigned)HealthGauge->Get_Value());

            /*
            **	Clip to 1
            */
            if (strength <= 0) {
                strength = 1;
            }

            /*
            **	Set new strength
            */
            if (strength != CurrentObject[0]->Strength) {
                CurrentObject[0]->Strength = strength;
                HidPage.Clear();
                Flag_To_Redraw(true);
                Changed = 1;
            }

            /*
            **	Update text label
            */
            sprintf(HealthBuf, "%d", strength);
        }
        input = KN_NONE;
        break;

    /*
    **	Object-Editing button: Facing
    */
    case (POPUP_FACINGDIAL | KN_BUTTON):
        if (CurrentObject[0]->Is_Techno()) {

            /*
            **	Set new facing
            */
            if (FacingDial->Get_Direction() != ((TechnoClass*)CurrentObject[0])->PrimaryFacing.Get()) {

                /*
                **	Set body's facing
                */
                ((TechnoClass*)CurrentObject[0])->PrimaryFacing.Set(FacingDial->Get_Direction());

                /*
                **	Set turret facing, if there is one
                */
                if (CurrentObject[0]->What_Am_I() == RTTI_UNIT) {
                    ((UnitClass*)CurrentObject[0])->SecondaryFacing.Set(FacingDial->Get_Direction());
                }

                HidPage.Clear();
                Flag_To_Redraw(true);
                Changed = 1;
            }
        }
        input = KN_NONE;
        break;

    /*
    **	Object-Editing button: Facing
    */
    case (POPUP_BASEPERCENT | KN_BUTTON):
        if (BaseGauge->Get_Value() != Scen.Percent) {
            Scen.Percent = BaseGauge->Get_Value();
            Build_Base_To(Scen.Percent);
            HidPage.Clear();
            Flag_To_Redraw(true);
        }
        input = KN_NONE;
        break;

    default:
        break;
    }

    /*
    **	Call parent's AI routine
    */
    MouseClass::AI(input, x, y);
}

/***************************************************************************
 * MapEditClass::Draw_It -- overloaded Redraw routine                      *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/17/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Draw_It(bool forced)
{
    char const* label;
    char buf[40];
    char const* tptr;

    MouseClass::Draw_It(forced);

    if (!Debug_Map) {
        return;
    }

    /*
    **	Display the total value of all Tiberium on the map.
    */
    Fancy_Text_Print(
        "Tiberium=%ld   ", 0, 0, GadgetClass::Get_Color_Scheme(), BLACK, TPF_EFNT | TPF_NOSHADOW, TotalValue);

    /*
    **	If there are no object controls displayed, just invoke parent's Redraw
    **	and return.
    */
    if (!Buttons) {
        return;
    }

    /*
    **	Otherwise, if 'display' is set, invoke the parent's Redraw to refresh
    **	the HIDPAGE; then, update the buttons & text labels onto HIDPAGE;
    **	then invoke the parent's Redraw to blit the HIDPAGE to SEENPAGE.
    */
    if (forced) {

        /*
        **	Update the text labels
        */
        if (CurrentObject.Count()) {
            /*
            **	Display the object's name & ID
            */
            label = Text_String(CurrentObject[0]->Full_Name());
            tptr = label;
            sprintf(buf, "%s (%d)", tptr, CurrentObject[0]->As_Target());

            /*
            **	print the label
            */
            Fancy_Text_Print(buf, 160, 0, &ColorRemaps[PCOLOR_BROWN], TBLACK, TPF_CENTER | TPF_NOSHADOW | TPF_EFNT);
        }
    }
}

/***************************************************************************
 * MapEditClass::Mouse_Moved -- checks for mouse motion                    *
 *                                                                         *
 * Reports whether the mouse has moved or not. This varies based on the    *
 * type of object currently selected. If there's an infantry object        *
 *   selected, mouse motion counts even within a cell; for all other types,*
 *   mouse motion counts only if the mouse changes cells.                  *
 *                                                                         *
 *   The reason this routine is needed is to prevent Paint-Mode from putting*
 *   gobs of trees and such into the same cell if the mouse moves just     *
 *   a little bit.                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/08/1994 BR : Created.                                              *
 *=========================================================================*/
bool MapEditClass::Mouse_Moved(void)
{
    static int old_mx = 0;
    static int old_my = 0;
    static CELL old_zonecell = 0;
    const ObjectTypeClass* objtype = NULL;
    bool retcode = false;

    /*
    **	Return if no motion
    */
    if (old_mx == Get_Mouse_X() && old_my == Get_Mouse_Y()) {
        return (false);
    }

    /*
    **	Get a ptr to ObjectTypeClass
    */
    if (PendingObject) {
        objtype = PendingObject;
    } else {
        if (GrabbedObject) {
            objtype = &GrabbedObject->Class_Of();
        } else {
            old_mx = Get_Mouse_X();
            old_my = Get_Mouse_Y();
            old_zonecell = ZoneCell;
            return (false);
        }
    }

    /*
    **	Infantry: mouse moved if any motion at all
    */
    if (objtype->What_Am_I() == RTTI_INFANTRYTYPE) {
        retcode = true;
    } else {
        /*
        **	Others: mouse moved only if cell changed
        */
        if (old_zonecell != ZoneCell) {
            retcode = true;
        } else {
            retcode = false;
        }
    }

    old_mx = Get_Mouse_X();
    old_my = Get_Mouse_Y();
    old_zonecell = ZoneCell;
    return (retcode);
}

/***************************************************************************
 * MapEditClass::Main_Menu -- main menu processor for map editor           *
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
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Main_Menu(void)
{
    char const* _menus[MAX_MAIN_MENU_NUM + 1];
    int selection; // option the user picks
    bool process;  // menu stays up while true
    int rc;

    /*
    **	Fill in menu items
    */
    _menus[0] = "New Scenario";
    _menus[1] = "Load Scenario";
    _menus[2] = "Save Scenario";
    _menus[3] = "Size Map";
    _menus[4] = "Add Game Object";
    _menus[5] = "Scenario Options";
    _menus[6] = "AI Options";
    _menus[7] = "Play Scenario";
    _menus[8] = NULL;

    /*
    **	Main Menu loop
    */
    Override_Mouse_Shape(MOUSE_NORMAL); // display default mouse cursor
    process = true;
    while (process) {

        /*
        **	Invoke game callback, to update music
        */
        Call_Back();

        /*
        **	Invoke menu
        */
        Hide_Mouse(); // Do_Menu assumes the mouse is already hidden
        selection = Do_Menu(&_menus[0], true);
        Show_Mouse();
        if (UnknownKey == KN_ESC || UnknownKey == KN_LMOUSE || UnknownKey == KN_RMOUSE) {
            break;
        }

        /*
        **	Process selection
        */
        switch (selection) {

        /*
        **	New scenario
        */
        case 0:
            if (Changed) {
                rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
                HidPage.Clear();
                Flag_To_Redraw(true);
                Render();
                if (rc == 0) {
                    if (Save_Scenario() != 0) {
                        break;
                    } else {
                        Changed = 0;
                    }
                }
            }
            if (New_Scenario() == 0) {
                Scen.CarryOverMoney = 0;
                Changed = 1;
            }
            process = false;
            break;

        /*
        **	Load scenario
        */
        case 1:
            if (Changed) {
                rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
                HidPage.Clear();
                Flag_To_Redraw(true);
                Render();
                if (rc == 0) {
                    if (Save_Scenario() != 0) {
                        break;
                    } else {
                        Changed = 0;
                    }
                }
            }
            if (Load_Scenario() == 0) {
                Scen.CarryOverMoney = 0;
                Changed = 0;
            }
            process = false;
            break;

        /*
        **	Save scenario
        */
        case 2:
            if (Save_Scenario() == 0) {
                Changed = 0;
            }
            process = false;
            break;

        /*
        **	Edit map size
        */
        case 3:
            if (Size_Map(MapCellX, MapCellY, MapCellWidth, MapCellHeight) == 0) {
                process = false;
                Changed = 1;
            }
            break;

        /*
        **	Add an object
        */
        case 4:
            if (Placement_Dialog() == 0) {
                Start_Placement();
                process = false;
            }
            break;

        /*
        **	Scenario options
        */
        case 5:
            if (Scenario_Dialog() == 0) {
                Changed = 1;
                process = false;
            }
            break;

        /*
        **	Other options
        */
        case 6:
            AI_Menu();
            process = false;
            break;

        /*
        **	Test-drive this scenario
        */
        case 7:
            if (Changed) {
                rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO, TXT_CANCEL);
                HidPage.Clear();
                Flag_To_Redraw(true);
                Render();
                if (rc == 2)
                    return;
                if (rc == 0) {
                    if (Save_Scenario() != 0) {
                        break;
                    } else {
                        Changed = 0;
                    }
                }
            }
            Changed = 0;
            Debug_Map = false;
            Start_Scenario(Scen.ScenarioName);
            return;
        }

        Frame_Limiter();
    }

    /*
    **	Restore the display:
    **	- Clear HIDPAGE to erase any spurious drawing done by the menu system
    **	- Invoke Flag_To_Redraw to tell DisplayClass to re-render the whole screen
    **	- Invoke Redraw() to update the display
    */
    HidPage.Clear();
    Flag_To_Redraw(true);
    Render();
}

/***************************************************************************
 * MapEditClass::AI_Menu -- menu of AI options                             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::AI_Menu(void)
{
    int selection; // option the user picks
    bool process;  // menu stays up while true
    char const* _menus[MAX_AI_MENU_NUM + 1];

    /*
    **	Fill in menu strings
    */
    _menus[0] = "Pre-Build a Base";
    _menus[1] = "Edit Triggers";
    _menus[2] = "Edit Teams";
    _menus[3] = NULL;

    /*
    **	Main Menu loop
    */
    Override_Mouse_Shape(MOUSE_NORMAL); // display default mouse cursor
    process = true;
    while (process) {

        /*
        **	Invoke game callback, to update music
        */
        Call_Back();

        /*
        **	Invoke menu
        */
        Hide_Mouse(); // Do_Menu assumes the mouse is already hidden
        selection = Do_Menu(&_menus[0], true);
        Show_Mouse();
        if (UnknownKey == KN_ESC || UnknownKey == KN_LMOUSE || UnknownKey == KN_RMOUSE) {
            break;
        }

        /*
        **	Process selection
        */
        switch (selection) {
        /*
        **	Pre-Build a Base
        */
        case 0:
            Start_Base_Building();
            process = false;
            break;

        /*
        **	Trigger Editing
        */
        case 1:
            Handle_Triggers();
            /*
            **	Go into trigger placement mode
            */
            if (CurTrigger) {
                Start_Trigger_Placement();
            }
            process = false;
            break;

        /*
        **	Team Editing
        */
        case 2:
            Handle_Teams("Teams");
            process = false;
            break;
        }

        Frame_Limiter();
    }
}

/***************************************************************************
 * MapEditClass::Verify_House -- is this objtype ownable by this house?    *
 *                                                                         *
 * INPUT:                                                                  *
 *      house         house to check                                       *
 *      objtype      ObjectTypeClass to check                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = isn't ownable, 1 = it is                                       *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/16/1994 BR : Created.                                              *
 *=========================================================================*/
bool MapEditClass::Verify_House(HousesType house, ObjectTypeClass const* objtype)
{
    /*
    **	Verify that new house can own this object
    */
    return ((objtype->Get_Ownable() & (1 << house)) != 0);
}

/***************************************************************************
 * MapEditClass::Cycle_House -- finds next valid house for object type     *
 *                                                                         *
 * INPUT:                                                                  *
 *      objtype      ObjectTypeClass ptr to get house for                  *
 *      curhouse      current house value to start with                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      HousesType that's valid for this object type                       *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/23/1994 BR : Created.                                              *
 *=========================================================================*/
HousesType MapEditClass::Cycle_House(HousesType curhouse, ObjectTypeClass const*)
{
    HousesType count; // prevents an infinite loop

    /*
    **	Loop through all house types, starting with the one after 'curhouse';
    **	return the first one that's valid
    */
    count = HOUSE_NONE;
    while (1) {

        /*
        **	Go to next house
        */
        curhouse++;
        if (curhouse == HOUSE_COUNT) {
            curhouse = HOUSE_FIRST;
        }

        /*
        **	Count # iterations; don't go forever
        */
        count++;
        if (count == HOUSE_COUNT) {
            curhouse = HOUSE_NONE;
            break;
        }

        /*
        **	Break if this is a valid house
        */
        //		if (HouseClass::As_Pointer(curhouse) && Verify_House(curhouse, objtype)) {
        break;
        //		}
    }

    return (curhouse);
}

/***************************************************************************
 * MapEditClass::Fatal -- exits with error message                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      code      tells which message to display; this minimizes the       *
 *               use of character strings in the code.                     *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/12/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Fatal(int txt)
{
    // Prog_End();
    // printf("%s\n", txt);
    Emergency_Exit(EXIT_FAILURE);
}

bool MapEditClass::Scroll_Map(DirType facing, int& distance, bool really)
{
    if (Debug_Map) {
        /*
        ** The popup gadgets require the entire map to be redrawn if we scroll.
        */
        if (really) {
            Flag_To_Redraw(true);
        }
    }
    return (MouseClass::Scroll_Map(facing, distance, really));
}

#ifdef OBSOLETE
void MapEditClass::Flag_To_Redraw(bool complete)
{
    MouseClass::Flag_To_Redraw(complete);
}
#endif

void MapEditClass::Detach(ObjectClass* object)
{
    if (GrabbedObject == object) {
        GrabbedObject = 0;
    }
}

bool MapEditClass::Get_Waypoint_Name(char wayptname[])
{
    /*
    **	Dialog & button dimensions
    */
    enum
    {
        D_DIALOG_W = 100,                            // dialog width
        D_DIALOG_H = 56,                             // dialog height
        D_DIALOG_X = ((320 - D_DIALOG_W) / 2),       // centered x-coord
        D_DIALOG_Y = ((200 - D_DIALOG_H) / 2),       // centered y-coord
        D_DIALOG_CX = D_DIALOG_X + (D_DIALOG_W / 2), // coord of x-center

        D_TXT8_H = 11, // ht of 8-pt text
        D_MARGIN = 7,  // margin width/height

        D_EDIT_W = D_DIALOG_W - (D_MARGIN * 2),
        D_EDIT_H = 13,
        D_EDIT_X = D_DIALOG_X + D_MARGIN,
        D_EDIT_Y = D_DIALOG_Y + 20,

        D_BUTTON_X = D_DIALOG_X + D_MARGIN,
        D_BUTTON_Y = D_DIALOG_Y + 40,
        D_BUTTON_W = 40,
        D_BUTTON_H = 13,

        D_CANCEL_X = D_DIALOG_X + 53,
        D_CANCEL_Y = D_DIALOG_Y + 40,
        D_CANCEL_W = 40,
        D_CANCEL_H = 13,

    };

    /*
    **	Button enumerations
    */
    enum
    {
        BUTTON_OK = 100,
        BUTTON_CANCEL,
        BUTTON_EDIT,
    };

    /*
    **	Dialog variables
    */
    bool cancel = false; // true = user cancels
    wayptname[0] = 0;

    /*
    **	Buttons
    */
    ControlClass* commands = NULL; // the button list

    TextButtonClass button(BUTTON_OK, TXT_OK, TPF_EBUTTON, D_BUTTON_X, D_BUTTON_Y, D_BUTTON_W);
    TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_EBUTTON, D_CANCEL_X, D_CANCEL_Y, D_CANCEL_W);
    EditClass editbtn(
        BUTTON_EDIT, wayptname, 3, TPF_EFNT | TPF_NOSHADOW, D_EDIT_X, D_EDIT_Y, D_EDIT_W, -1, EditClass::ALPHANUMERIC);

    /*
    **	Initialize.
    */
    Set_Logic_Page(SeenBuff);

    /*
    **	Create the button list.
    */
    commands = &button;
    cancelbtn.Add_Tail(*commands);
    editbtn.Add_Tail(*commands);
    editbtn.Set_Focus();

    /*
    **	Main Processing Loop.
    */
    bool firsttime = true;
    bool display = true;
    bool process = true;
    while (process) {

        /*
        **	Invoke game callback.
        */
        if (Session.Type == GAME_NORMAL) {
            Call_Back();
        } else if (Main_Loop()) {
            process = false;
            cancel = true;
        }

        /*
        **	Refresh display if needed.
        */
        if (display) {

            /*
            **	Display the dialog box.
            */
            Hide_Mouse();
            if (display) {
                Dialog_Box(D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W, D_DIALOG_H);
                //				Draw_Caption(caption, D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W);
            }

            /*
            **	Redraw the buttons.
            */
            if (display) {
                commands->Flag_List_To_Redraw();
            }
            Show_Mouse();
            display = false;
        }

        /*
        **	Get user input.
        */
        KeyNumType input = commands->Input();

        /*
        **	The first time through the processing loop, set the edit
        **	gadget to have the focus. The
        **	focus must be set here since the gadget list has changed
        **	and this change will cause any previous focus setting to be
        **	cleared by the input processing routine.
        */
        if (firsttime) {
            firsttime = false;
            editbtn.Set_Focus();
            editbtn.Flag_To_Redraw();
        }

        /*
        **	If the <RETURN> key was pressed, then default to the appropriate
        **	action button according to the style of this dialog box.
        */
        if (input == KN_RETURN) {
            input = (KeyNumType)(BUTTON_OK | KN_BUTTON);
        }

        /*
        **	Process input.
        */
        switch (input) {
        /*
        ** Load: if load fails, present a message, and stay in the dialog
        ** to allow the user to try another game
        */
        case (BUTTON_OK | KN_BUTTON):
            Hide_Mouse();
            SeenPage.Clear();
            GamePalette.Set();
            Show_Mouse();
            process = false;
            cancel = false;
            break;

        /*
        ** ESC/Cancel: break
        */
        case (KN_ESC):
        case (BUTTON_CANCEL | KN_BUTTON):
            Hide_Mouse();
            SeenPage.Clear();
            GamePalette.Set();
            Show_Mouse();
            cancel = true;
            process = false;
            break;

        default:
            break;
        }

        Frame_Limiter();
    }

    Map.Flag_To_Redraw(true);
    if (cancel)
        return (false);

    return (true);
}

void MapEditClass::Update_Waypoint(int waypt_idx)
{
    CELL cell;

    /*
    **	Unflag cell for this waypoint if there is one
    */
    cell = Scen.Waypoint[waypt_idx];
    if (cell != -1) {
        if (Scen.Waypoint[WAYPT_HOME] != cell && Scen.Waypoint[WAYPT_REINF] != cell) {
            (*this)[cell].IsWaypoint = 0;
        }
        Flag_Cell(cell);
    }
    Scen.Waypoint[waypt_idx] = CurrentCell;
    (*this)[CurrentCell].IsWaypoint = 1;
    Changed = 1;
    Flag_Cell(CurrentCell);
}

/***************************************************************************
 * MapEditClass::Read_INI -- overloaded Read_INI function                  *
 *                                                                         *
 * Overloading this function gives the map editor a chance to initialize   *
 * certain values every time a new INI is read.                            *
 *                                                                         *
 * INPUT:                                                                  *
 *      buffer      INI staging area                                       *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/16/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Read_INI(CCINIClass& ini)
{
    /*
    **	Invoke parent's Read_INI
    */
    Mono_Printf("We are in Read_INI\n");

    MouseClass::Read_INI(ini);
    BaseGauge->Set_Value(Scen.Percent);

    Mono_Clear_Screen();
    Mono_Printf("Scen.Percent = %d", Scen.Percent);

    //	BaseGauge->Set_Value(Scen.Percent);
}

void MapEditClass::Write_INI(CCINIClass& ini)
{
    MouseClass::Write_INI(ini);
    //	ini.Put_Int("Basic", "Percent", Scen.Percent);
}

#endif
