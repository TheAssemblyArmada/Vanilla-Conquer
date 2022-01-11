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

/* $Header: /CounterStrike/SOUNDDLG.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SOUNDDLG.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready-Legg, Joe L. Bostic                   *
 *                                                                                             *
 *                   Start Date : Jan 8, 1995                                                  *
 *                                                                                             *
 *                  Last Update : September 22, 1995 [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   MusicListClass::Draw_Entry -- Draw the score line in a list box.                          *
 *   SoundControlsClass::Process -- Handles all the options graphic interface.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "list.h"
#include "textbtn.h"
#include "sounddlg.h"
#include "common/framelimit.h"

class MusicListClass : public ListClass
{
public:
    MusicListClass(int id, int x, int y, int w, int h)
        : ListClass(id,
                    x,
                    y,
                    w,
                    h,
                    TPF_6PT_GRAD | TPF_NOSHADOW,
                    MFCD::Retrieve("BTN-UP.SHP"),
                    MFCD::Retrieve("BTN-DN.SHP")){};
    virtual ~MusicListClass(void){};

protected:
    virtual void Draw_Entry(int index, int x, int y, int width, int selected);
};

int SoundControlsClass::Init(void)
{
    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
    OptionWidth = OPTION_WIDTH * factor;
    OptionHeight = 146 * factor;

    OptionX = ((SeenBuff.Get_Width() - OptionWidth) / 2);
    OptionY = (SeenBuff.Get_Height() - OptionHeight) / 2;

    ListboxX = 17 * factor;
    ListboxY = 54 * factor;
    ListboxW = OptionWidth - (ListboxX * 2);
#ifdef FIXIT_CSII //	checked - ajw 9/28/98
    ListboxH = (72 * factor) + 2;
#else
    ListboxH = 72 * factor;
#endif

    ButtonWidth = 70 * factor;
    ButtonX = OptionWidth - (ButtonWidth + (17 * factor));
    ButtonY = 128 * factor;

    StopX = 17 * factor;
    StopY = 128 * factor;

    PlayX = 35 * factor;
    PlayY = 128 * factor;

    OnOffWidth = 25 * factor;

    ShuffleX = SHUFFLE_X * factor;
    ShuffleY = 128 * factor;

    RepeatX = REPEAT_X * factor;
    RepeatY = 128 * factor;

    MSliderX = 147 * factor;
    MSliderY = 28 * factor;
    MSliderW = 108 * factor;
    MSliderHeight = 5 * factor;

    FXSliderX = 147 * factor;
    FXSliderY = 40 * factor;
    FXSliderW = 108 * factor;
    FXSliderHeight = 5 * factor;
    return (factor);
}

/***********************************************************************************************
 * SoundControlsClass::Process -- Handles all the options graphic interface.                   *
 *                                                                                             *
 *    This routine is the main control for the visual representation of the options            *
 *    screen. It handles the visual overlay and the player input.                              *
 *                                                                                             *
 * INPUT:      none                                                                            *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:    12/31/1994 MML : Created.                                                       *
 *=============================================================================================*/
void SoundControlsClass::Process(void)
{
    RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
    //	ThemeType theme;

    int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;

    Init();
    /*
    **	List box that holds the score text strings.
    */
    MusicListClass listbox(0, OptionX + ListboxX, OptionY + ListboxY, ListboxW, ListboxH);

    /*
    **	Return to options menu button.
    */
    TextButtonClass returnto(BUTTON_OPTIONS, TXT_OK, TPF_BUTTON, OptionX + ButtonX, OptionY + ButtonY, ButtonWidth);
    //	TextButtonClass returnto(BUTTON_OPTIONS, TXT_OPTIONS_MENU, TPF_BUTTON,

    /*
    **	Stop playing button.
    */
    ShapeButtonClass stopbtn(BUTTON_STOP, MFCD::Retrieve("BTN-ST.SHP"), OptionX + StopX, OptionY + StopY);

    /*
    **	Start playing button.
    */
    ShapeButtonClass playbtn(BUTTON_PLAY, MFCD::Retrieve("BTN-PL.SHP"), OptionX + PlayX, OptionY + PlayY);

    /*
    **	Shuffle control.
    */
    TextButtonClass shufflebtn(BUTTON_SHUFFLE, TXT_OFF, TPF_BUTTON, OptionX + ShuffleX, OptionY + ShuffleY, OnOffWidth);
    //	TextButtonClass shufflebtn(BUTTON_SHUFFLE, TXT_OFF, TPF_BUTTON, option_x+shuffle_x, option_y+shuffle_y,
    //ONOFF_WIDTH);

    /*
    **	Repeat control.
    */
    TextButtonClass repeatbtn(BUTTON_REPEAT, TXT_OFF, TPF_BUTTON, OptionX + RepeatX, OptionY + RepeatY, OnOffWidth);

    /*
    **	Music volume slider.
    */
    SliderClass music(SLIDER_MUSIC, OptionX + MSliderX, OptionY + MSliderY, MSliderW, MSliderHeight, true);

    /*
    **	Sound volume slider.
    */
    SliderClass sound(SLIDER_SOUND, OptionX + FXSliderX, OptionY + FXSliderY, FXSliderW, FXSliderHeight, true);

    /*
    **	Causes left mouse clicks inside the dialog area, but not on any
    **	particular button, to be ignored.
    */
    GadgetClass area(OptionX, OptionY, OptionWidth, OptionHeight, GadgetClass::LEFTPRESS);

    /*
    **	Causes right clicks anywhere or left clicks outside of the dialog
    **	box area to be the same a clicking the return to game options button.
    */
    ControlClass ctrl(BUTTON_OPTIONS,
                      0,
                      0,
                      SeenBuff.Get_Width(),
                      SeenBuff.Get_Height(),
                      GadgetClass::RIGHTPRESS | GadgetClass::LEFTPRESS);

    /*
    **	The repeat and shuffle buttons are of the toggle type. They toggle
    **	between saying "on" and "off".
    */
    shufflebtn.IsToggleType = true;
    if (Options.IsScoreShuffle) {
        shufflebtn.Turn_On();
    } else {
        shufflebtn.Turn_Off();
    }
    shufflebtn.Set_Text(shufflebtn.IsOn ? TXT_ON : TXT_OFF);

    repeatbtn.IsToggleType = true;
    if (Options.IsScoreRepeat) {
        repeatbtn.Turn_On();
    } else {
        repeatbtn.Turn_Off();
    }
    repeatbtn.Set_Text(repeatbtn.IsOn ? TXT_ON : TXT_OFF);

    /*
    **	Set the initial values of the sliders.
    */
    music.Set_Maximum(255);
    music.Set_Thumb_Size(16);
    music.Set_Value(Options.ScoreVolume * 256);
    sound.Set_Maximum(255);
    sound.Set_Thumb_Size(16);
    sound.Set_Value(Options.Volume * 256);

    /*
    **	Set up the window.  Window x-coords are in bytes not pixels.
    */
    Set_Logic_Page(SeenBuff);

    /*
    **	Create Buttons.
    */
    GadgetClass* optionsbtn = &returnto;
    listbox.Add_Tail(*optionsbtn);
    stopbtn.Add_Tail(*optionsbtn);
    playbtn.Add_Tail(*optionsbtn);
    shufflebtn.Add_Tail(*optionsbtn);
    repeatbtn.Add_Tail(*optionsbtn);
    music.Add_Tail(*optionsbtn);
    sound.Add_Tail(*optionsbtn);
    area.Add_Tail(*optionsbtn);
    ctrl.Add_Tail(*optionsbtn);

    /*
    **	Add all the themes to the list box. The list box entries are constructed
    **	and then stored into allocated EMS memory blocks.
    */
    for (ThemeType index = THEME_FIRST; index < Theme.Max_Themes(); index++) {
        if (Theme.Is_Allowed(index)) {
            int length = Theme.Track_Length(index);
            char const* fullname = Theme.Full_Name(index);

            char* ptr = new char[100];
            if (ptr != nullptr) {
                snprintf(ptr,
                         100,
                         "%cTrack %d\t%d:%02d\t%s",
                         index,
                         listbox.Count() + 1,
                         length / 60,
                         length % 60,
                         fullname);
                listbox.Add_Item(ptr);
            }

            if (Theme.What_Is_Playing() == index) {
                listbox.Set_Selected_Index(listbox.Count() - 1);
            }
        }
    }
    static int _tabs[] = {55 * factor, 72 * factor, 90 * factor};
    listbox.Set_Tabs(_tabs);

    /*
    **	Main Processing Loop.
    */
    bool display = true;
    bool process = true;

    while (process) {

        /*
        **	Invoke game callback.
        */
        if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH) {
            Call_Back();
        } else {
            if (Main_Loop()) {
                process = false;
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
            Dialog_Box(OptionX, OptionY, OptionWidth, OptionHeight);

            Draw_Caption(TXT_SOUND_CONTROLS, OptionX, OptionY, OptionWidth);

            /*
            ** Draw the Music, Speech & Sound titles.
            */
            Fancy_Text_Print(TXT_MUSIC_VOLUME,
                             OptionX + MSliderX - 10,
                             OptionY + MSliderY - 4,
                             scheme,
                             TBLACK,
                             TPF_TEXT | TPF_RIGHT);
            Fancy_Text_Print(TXT_SOUND_VOLUME,
                             OptionX + FXSliderX - 10,
                             OptionY + FXSliderY - 4,
                             scheme,
                             TBLACK,
                             TPF_TEXT | TPF_RIGHT);

#if defined(GERMAN) || defined(FRENCH)
            Fancy_Text_Print(TXT_SHUFFLE,
                             Option_X + 4 + Shuffle_X - 10,
                             Option_Y + Shuffle_Y + 2,
                             scheme,
                             TBLACK,
                             TPF_TEXT | TPF_RIGHT);
#else
            Fancy_Text_Print(
                TXT_SHUFFLE, OptionX + ShuffleX - 10, OptionY + ShuffleY + 2, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
#endif
            Fancy_Text_Print(
                TXT_REPEAT, OptionX + RepeatX - 10, OptionY + RepeatY + 2, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);

            optionsbtn->Draw_All();
            Show_Mouse();
            display = false;
        }

        /*
        **	Get user input.
        */
        KeyNumType input = optionsbtn->Input();

        /*
        **	Process Input.
        */
        switch (input) {

        case KN_ESC:
        case BUTTON_OPTIONS | KN_BUTTON:
            process = false;
            break;

        /*
        **	Control music volume.
        */
        case SLIDER_MUSIC | KN_BUTTON:
            Options.Set_Score_Volume(fixed(music.Get_Value(), 256), true);
#ifdef FIXIT_VERSION_3
            if (Session.Type != GAME_NORMAL)
                Options.MultiScoreVolume = Options.ScoreVolume;
#endif
            break;

        /*
        **	Control sound volume.
        */
        case SLIDER_SOUND | KN_BUTTON:
            Options.Set_Sound_Volume(fixed(sound.Get_Value(), 256), true);
            break;

        case BUTTON_LISTBOX | KN_BUTTON:
            break;

        /*
        **	Stop all themes from playing.
        */
        case BUTTON_STOP | KN_BUTTON:
            Theme.Stop();
            Theme.Queue_Song(THEME_QUIET);
            //				Theme.Queue_Song(THEME_NONE);
            break;

        /*
        **	Start the currently selected theme to play.
        */
        case KN_SPACE:
        case BUTTON_PLAY | KN_BUTTON:
            if (listbox.Count()) {
                Theme.Queue_Song((ThemeType) * ((unsigned char*)listbox.Current_Item()));
            }
            break;

        /*
        **	Toggle the shuffle button.
        */
        case BUTTON_SHUFFLE | KN_BUTTON:
            shufflebtn.Set_Text(shufflebtn.IsOn ? TXT_ON : TXT_OFF);
            Options.Set_Shuffle(shufflebtn.IsOn);
            break;

        /*
        **	Toggle the repeat button.
        */
        case BUTTON_REPEAT | KN_BUTTON:
            repeatbtn.Set_Text(repeatbtn.IsOn ? TXT_ON : TXT_OFF);
            Options.Set_Repeat(repeatbtn.IsOn);
            break;
        }

        Frame_Limiter();
    }

    /*
    **	If the score volume was turned all the way down, then actually
    **	stop the scores from being played.
    */
    if (Options.ScoreVolume == 0) {
        Theme.Stop();
    }

    /*
    **	Free the items from the list box.
    */
    while (listbox.Count()) {
        char const* ptr = listbox.Get_Item(0);
        listbox.Remove_Item(ptr);
        delete[] ptr;
    }
}

/***********************************************************************************************
 * MusicListClass::Draw_Entry -- Draw the score line in a list box.                            *
 *                                                                                             *
 *    This routine will display the score line in a list box. It overrides the list box        *
 *    handler for line drawing.                                                                *
 *                                                                                             *
 * INPUT:   index    -- The index within the list box that is being drawn.                     *
 *                                                                                             *
 *          x,y      -- The pixel coordinates of the upper left position of the line.          *
 *                                                                                             *
 *          width    -- The width of the line that drawing is allowed to use.                  *
 *                                                                                             *
 *          selected-- Is the current line selected?                                           *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/22/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void MusicListClass::Draw_Entry(int index, int x, int y, int width, int selected)
{
    RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

    if (TextFlags & TPF_6PT_GRAD) {
        TextPrintType flags = TextFlags;

        if (selected) {
            flags = flags | TPF_BRIGHT_COLOR;
            LogicPage->Fill_Rect(x, y, x + width - 1, y + LineHeight - 1, GadgetClass::Get_Color_Scheme()->Shadow);
        } else {
            if (!(flags & TPF_USE_GRAD_PAL)) {
                flags = flags | TPF_MEDIUM_COLOR;
            }
        }

        Conquer_Clip_Text_Print((char*)List[index] + 1, x, y, scheme, TBLACK, flags, width, Tabs);

    } else {
        Conquer_Clip_Text_Print((char*)List[index] + 1,
                                x,
                                y,
                                (selected ? &ColorRemaps[PCOLOR_DIALOG_BLUE] : &ColorRemaps[PCOLOR_GREY]),
                                TBLACK,
                                TextFlags,
                                width,
                                Tabs);
    }
}
