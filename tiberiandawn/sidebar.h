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

/* $Header:   F:\projects\c&c\vcs\code\sidebar.h_v   2.18   16 Oct 1995 16:45:24   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SIDEBAR.H                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : October 20, 1994                                             *
 *                                                                                             *
 *                  Last Update : October 20, 1994   [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "function.h"
#include "power.h"
#include "factory.h"

class InitClass
{
};

class SidebarClass : public PowerClass
{
public:
    /*
    **	These constants are used to control the sidebar rendering. They are instantiated
    **	as enumerations since C++ cannot use "const" in this context.
    */
    int SideX;            // x position for side bar
    int SideY;            // y position for side bar
    int SideBarWidth;     // width of the sidebar
    int SideWidth;        // width of the sidebar
    int SideHeight;       // height of the sidebar
    int TopHeight;        // height of top of sidebar
    int MaxVisible;       // max production icons visible
    int ButtonOneWidth;   // Button width.
    int ButtonTwoWidth;   // Button width.
    int ButtonThreeWidth; // Button width.
    int ButtonHeight;     // Button width.

    enum SideBarClassEnums
    {
        BUTTON_ACTIVATOR = 100, // Button ID for the activator.
        SIDEBARWIDTH = 80,
#if 0
		SIDE_X=RADAR_X,					// The X position of sidebar upper left corner.
		SIDE_Y=RADAR_Y+RADAR_HEIGHT,	// The Y position of sidebar upper left corner.
		SIDE_WIDTH=320-SIDE_X,			// Width of the entire sidebar (in pixels).
		SIDE_HEIGHT=200-SIDE_Y,			// Height of the entire sidebar (in pixels).
		TOP_HEIGHT=13,					// Height of top section (with repair/sell buttons).
		COLUMN_ONE_X=SIDE_X+8,			// Sidestrip upper left coordinates...
		COLUMN_ONE_Y=SIDE_Y+TOP_HEIGHT,
		COLUMN_TWO_X=COLUMN_ONE_X+((SIDE_WIDTH-16)/2)+3,
		COLUMN_TWO_Y=SIDE_Y+TOP_HEIGHT,

//BGA: changes to all buttons
#if (GERMAN | FRENCH)
		BUTTON_ONE_WIDTH=20,			// Button width.
		BUTTON_TWO_WIDTH=27,			// Button width.
		BUTTON_THREE_WIDTH=26,			// Button width.
		BUTTON_HEIGHT=9,				// Button height.
		BUTTON_ONE_X=SIDE_X+2,			// Left button X coordinate.
		BUTTON_ONE_Y=SIDE_Y+2,			// Left button Y coordinate.
		BUTTON_TWO_X=SIDE_X+24,			// Right button X coordinate.
		BUTTON_TWO_Y=SIDE_Y+2,			// Right button Y coordinate.
		BUTTON_THREE_X=SIDE_X+53,		// Right button X coordinate.
		BUTTON_THREE_Y=SIDE_Y+2,		// Right button Y coordinate.
#else
		BUTTON_ONE_WIDTH=32,			// Button width.
		BUTTON_TWO_WIDTH=20,			// Button width.
		BUTTON_THREE_WIDTH=20,			// Button width.
		BUTTON_HEIGHT=9,				// Button height.
		BUTTON_ONE_X=SIDE_X+2,			// Left button X coordinate.
		BUTTON_ONE_Y=SIDE_Y+2,			// Left button Y coordinate.
		BUTTON_TWO_X=SIDE_X+36,			// Right button X coordinate.
		BUTTON_TWO_Y=SIDE_Y+2,			// Right button Y coordinate.
		BUTTON_THREE_X=SIDE_X+58,		// Right button X coordinate.
		BUTTON_THREE_Y=SIDE_Y+2,		// Right button Y coordinate.
#endif
		BUTTON_ONE_WIDTH=32,			// Button width.
		BUTTON_TWO_WIDTH=20,			// Button width.
		BUTTON_THREE_WIDTH=20,			// Button width.
		BUTTON_HEIGHT=9,				// Button height.
#endif

        COLUMNS = 2 // Number of side strips on sidebar.
    };

    SidebarClass(void);
    SidebarClass(NoInitClass const& x)
        : PowerClass(x)
    {
    }

    /*
    ** Initialization
    */
    virtual void One_Time(void);                    // One-time inits
    virtual void Init_Clear(void);                  // Clears all to known state
    virtual void Init_IO(void);                     // Inits button list
    virtual void Init_Theater(TheaterType theater); // Theater-specific inits

    virtual void AI(KeyNumType& input, int x, int y);
    virtual void Draw_It(bool complete);
    virtual void Refresh_Cells(CELL cell, short const* list);

    void Zoom_Mode_Control(void);
    bool Abandon_Production(RTTIType type, int factory);
    bool Activate(int control);
    bool Add(RTTIType type,
             int ID,
             bool via_capture = false); // Added via_capture for new sidebar functionality. ST - 9/24/2019 3:15PM
    bool Sidebar_Click(KeyNumType& input, int x, int y);
    void Recalc(void);
    bool Factory_Link(int factory, RTTIType type, int id);

    /*
    **	File I/O.
    */
    virtual void Code_Pointers(void);
    virtual void Decode_Pointers(void);

    /*
    **	Each side strip is managed by this class. It handles all strip specific
    **	actions.
    */
    class StripClass : public StageClass
    {
        class SelectClass : public ControlClass
        {
        public:
            SelectClass(void);

            void Set_Owner(StripClass& strip, int index);
            StripClass* Strip;
            int Index;

        protected:
            virtual int Action(unsigned flags, KeyNumType& key);
        };

    public:
        int ObjectWidth;
        int ObjectHeight;
        int StripWidth;
        int LeftEdgeOffset;
        int ButtonSpacingOffset;

        StripClass(void)
        {
        }
        StripClass(InitClass const&);
        StripClass(NoInitClass const&)
        {
        }
        bool Add(RTTIType type,
                 int ID,
                 bool via_capture); // Added via_capture for new sidebar functionality. ST - 9/24/2019 3:15PM
        bool Abandon_Production(int factory);
        bool Scroll(bool up);
        bool AI(KeyNumType& input, int x, int y);
        void Draw_It(bool complete);
        void One_Time(int id);
        void Init_Clear(void);
        void Init_IO(int id);
        void Init_Theater(TheaterType theater);
        bool Recalc(void);
        void Activate(void);
        void Deactivate(void);
        void Flag_To_Redraw(void);
        bool Factory_Link(int factory, RTTIType type, int id);
        void const* Get_Special_Cameo(int type);

        /*
        **	File I/O.
        */
        bool Load(FileClass& file);
        bool Save(FileClass& file);
        void Code_Pointers(void);
        void Decode_Pointers(void);

        /*
        **	Working numbers used when rendering and processing the side strip.
        */
        enum SideBarGeneralEnums
        {
            BUTTON_UP = 200,
            BUTTON_DOWN = 210,
            BUTTON_SELECT = 220,
            MAX_BUILDABLES = 30,       // Maximum number of object types in sidebar.
            OBJECT_HEIGHT = 24,        // Pixel height of each buildable object.
            OBJECT_WIDTH = 32,         // Pixel width of each buildable object.
            STRIP_WIDTH = 35,          // Width of strip (not counting border lines).
            MAX_VISIBLE = 4,           // Number of object slots visible at any one time.
            SCROLL_RATE = 8,           // The pixel jump while scrolling (larger is faster).
            BUTTON_SPACING_OFFSET = 4, // spacing info for buttons
            UP_X_OFFSET = 2,           // Scroll up arrow coordinates.
            UP_Y_OFFSET = MAX_VISIBLE * OBJECT_HEIGHT + 1,
            DOWN_X_OFFSET = 18, // Scroll down arrow coordinates.
            DOWN_Y_OFFSET = MAX_VISIBLE * OBJECT_HEIGHT + 1,
            BUTTON_WIDTH = 16,  // Width of the mini-scroll button.
            BUTTON_HEIGHT = 12, // Height of the mini-scroll button.
            // LEFT_EDGE_OFFSET=2,		// Offset from left edge for building shapes.
            TEXT_X_OFFSET = 18, // X offset to print "ready" text.
            TEXT_Y_OFFSET = 15, // Y offset to print "ready" text.
            TEXT_COLOR = 255,   // Color to use for the "Ready" text.
            // BUTTON_SPACING_OFFSET = 4, // spacing info for buttons
            // LEFT_EDGE_OFFSET=0,			// Offset from left edge for building shapes.
            // BUTTON_SPACING_OFFSET = 0, // spacing info for buttons

        };

        /*
        **	This is the coordinate of the upper left corner that this side strip
        **	uses for rendering.
        */
        int X, Y;

        /*
        **	This is a unique identifier for the sidebar strip. Using this identifier,
        **	it is possible to differentiate the button messages that arrive from the
        **	common input button list.  It >MUST< be equal to the strip's index into
        ** the Column[] array, because the strip uses it to access the stripclass
        ** buttons.
        */
        int ID;

        /*
        **	Shape numbers for the shapes in the STRIP.SHP file.
        */
        enum SideBarStipShapeEnums
        {
            SB_BLANK, // The blank rectangle to use if there are no objects present.
            SB_FRAME
        };

        /*
        **	If this particular side strip needs to be redrawn, then this flag
        **	will be true.
        */
        unsigned IsToRedraw : 1;

        /*
        **	If construction is in progress (no other objects in this strip can
        **	be started), then this flag will be true. It will be cleared when
        **	the strip is free to start production again.
        */
        unsigned IsBuilding : 1;

        /*
        **	This controls the sidebar slide direction. If this is true, then the sidebar
        **	will scroll downward -- revealing previous objects.
        */
        unsigned IsScrollingDown : 1;

        /*
        **	If the sidebar is scrolling, then this flag is true. Otherwise it is false.
        */
        unsigned IsScrolling : 1;

        /*
        **	This is the object (sidebar slot) that is flashing. Only one slot can be flashing
        **	at any one instant. This is usually the result of a click on the slot and construction
        **	has commenced.
        */
        int Flasher;

        /*
        **	As the sidebar scrolls up and down, this variable holds the index for the topmost
        **	visible sidebar slot.
        */
        int TopIndex;

        /*
        **	This is the queued scroll direction and amount. The sidebar
        **	will scroll the number of slots indicated by this value. This
        **	value is set according to the scroll buttons.
        */
        int Scroller;

        /*
        **	The sidebar has smooth scrolling. This is the number of pixels the sidebar
        **	has slide down. Thus, if this value were 5, then there would be 5 pixels of
        **	the TopIndex-1 sidebar object visible. When the Slid value reaches 24, then
        **	the value resets to zero and the TopIndex is decremented. For sliding in the
        **	opposite direction, change the IsScrollingDown flag.
        */
        int Slid;

        /*
        **	This is the count of the number of sidebar slots that are active.
        */
        int BuildableCount;

        /*
        **	This is the array of buildable object types. This array is sorted in the order
        **	that it is to be displayed. This array keeps track of which objects are building
        **	and ready to be placed. The very nature of this method precludes simultaneous
        **	construction of the same object type.
        */
        typedef struct BuildType
        {
            int BuildableID;
            RTTIType BuildableType;
            int Factory;              // Production manager.
            bool BuildableViaCapture; // Added for new sidebar functionality. ST - 9/24/2019 3:10PM
        } BuildType;
        BuildType Buildables[MAX_BUILDABLES];

        /*
        **	Pointer to the shape data for small versions of the logos. These are used as
        **	placeholder pieces on the side bar.
        */
        static void const* LogoShapes;

        /*
        **	This points to the animation sequence of frames used to mark the passage of time
        **	as an object is undergoing construction.
        */
        static void const* ClockShapes;

        /*
        ** This points to the animation sequence which deals with special
        ** shapes which handle non-production based icons.
        */
        static void const* SpecialShapes[3];

        /*
        **	This is the last theater that the special palette remap table was loaded
        **	for. If the current theater differs from this recorded value, then the
        **	remap tables are reloaded.
        */
        static TheaterType LastTheater;

        static ShapeButtonClass UpButton[COLUMNS];
        static ShapeButtonClass DownButton[COLUMNS];
        static SelectClass SelectButton[COLUMNS][MAX_VISIBLE];

        /*
        **	This points to the shapes that are used for the clock overlay. This displays
        **	progress of construction.
        */
        static char ClockTranslucentTable[(1 + 1) * 256];

    } Column[COLUMNS];

    /*
    **	If the sidebar is active then this flag is true.
    */
    unsigned IsSidebarActive : 1;

    /*
    **	This flag tells the rendering system that the sidebar needs to be redrawn.
    */
    unsigned IsToRedraw : 1;

    class SBGadgetClass : public GadgetClass
    {
    public:
        //				SBGadgetClass(void) : GadgetClass(SIDE_X+8, SIDE_Y, SIDE_WIDTH-1, SIDE_HEIGHT-1, LEFTUP) {};
        SBGadgetClass(void)
            : GadgetClass(0, 0, 0, 0, LEFTUP){};

    protected:
        virtual int Action(unsigned flags, KeyNumType& key);
    };

    /*
    **	This is the button that is used to collapse and expand the sidebar.
    ** These buttons must be available to derived classes, for Save/Load.
    */
    static ShapeButtonClass Repair;
    static ShapeButtonClass Upgrade;
    static ShapeButtonClass Zoom;
    static SBGadgetClass Background;

    bool Scroll(bool up, int column);

    /*
    **	Pointer to the shape data for the sidebar
    */
    static void const* SidebarShape1;
    static void const* SidebarShape2;

private:
    bool Activate_Repair(int control);
    bool Activate_Upgrade(int control);
    bool Activate_Demolish(int control);
    int Which_Column(RTTIType type);

    unsigned IsRepairActive : 1;
    unsigned IsUpgradeActive : 1;
    unsigned IsDemolishActive : 1;
};

#endif