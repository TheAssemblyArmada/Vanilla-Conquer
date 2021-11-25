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
#include "solewdt.h"
#include "function.h"
#include "soleglobals.h"
#include "soleparams.h"
#include "common/framelimit.h"

static void Draw_Choice_Entry(RTTIType rtti, int type, int x, int y, HousesType house, int unk, bool names)
{
    ShapeFlags_Type flags = SHAPE_NORMAL;
    const TechnoTypeClass* tech = Fetch_Techno_Type(rtti, type);

    if (tech == nullptr) {
        return;
    }

    if (names) {
        Fancy_Text_Print(tech->Full_Name(),
                         x + 32,
                         y + 50,
                         BROWN,
                         TBLACK,
                         TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
    }

    const void* cameo = tech->Get_Cameo_Data();
    int draw_x;
    int draw_y;

    if (cameo != nullptr) {
        draw_x = x;
        draw_y = y;
    } else {
        cameo = tech->Get_Image_Data();
        draw_x = x + 32;
        draw_y = y + 24;
        flags = SHAPE_FADING | SHAPE_CENTER;
    }

    CC_Draw_Shape(cameo, 0, draw_x, draw_y, WINDOW_MAIN, flags, HouseTypeClass::As_Reference(house).RemapTable);
}

int Unit_Choice_Dialog(bool names)
{
    struct UnitChoice
    {
        RTTIType RTTI;
        int Type;
        int Unk;
    };

    static UnitChoice _gdi_units[16] = {
        {RTTI_INFANTRY, INFANTRY_E1, 1},
        {RTTI_INFANTRY, INFANTRY_E2, 1},
        {RTTI_INFANTRY, INFANTRY_E3, 1},
        {RTTI_INFANTRY, INFANTRY_E5, 1},
        {RTTI_INFANTRY, INFANTRY_RAMBO, 1},
        {RTTI_UNIT, UNIT_HTANK, 1},
        {RTTI_UNIT, UNIT_MTANK, 1},
        {RTTI_UNIT, UNIT_APC, 1},
        {RTTI_UNIT, UNIT_MLRS, 1},
        {RTTI_UNIT, UNIT_JEEP, 1},
        {RTTI_UNIT, UNIT_MSAM, 1},
        {RTTI_UNIT, UNIT_VICE, 1},
        {RTTI_UNIT, UNIT_TRIC, 1},
        {RTTI_UNIT, UNIT_TREX, 1},
        {RTTI_UNIT, UNIT_RAPT, 1},
        {RTTI_UNIT, UNIT_STEG, 1},
    };

    static UnitChoice _nod_units[17] = {
        {RTTI_INFANTRY, INFANTRY_E1, 1},
        {RTTI_INFANTRY, INFANTRY_E3, 1},
        {RTTI_INFANTRY, INFANTRY_E4, 1},
        {RTTI_INFANTRY, INFANTRY_E5, 1},
        {RTTI_INFANTRY, INFANTRY_RAMBO, 1},
        {RTTI_UNIT, UNIT_LTANK, 1},
        {RTTI_UNIT, UNIT_STANK, 1},
        {RTTI_UNIT, UNIT_FTANK, 1},
        {RTTI_UNIT, UNIT_BUGGY, 1},
        {RTTI_UNIT, UNIT_ARTY, 1},
        {RTTI_UNIT, UNIT_MSAM, 1},
        {RTTI_UNIT, UNIT_BIKE, 1},
        {RTTI_UNIT, UNIT_VICE, 1},
        {RTTI_UNIT, UNIT_TRIC, 1},
        {RTTI_UNIT, UNIT_TREX, 1},
        {RTTI_UNIT, UNIT_RAPT, 1},
        {RTTI_UNIT, UNIT_STEG, 1},
    };

    static UnitChoice _all_units[23] = {
        {RTTI_INFANTRY, INFANTRY_E1, 1}, {RTTI_INFANTRY, INFANTRY_E2, 1}, {RTTI_INFANTRY, INFANTRY_E3, 1},
        {RTTI_INFANTRY, INFANTRY_E4, 1}, {RTTI_INFANTRY, INFANTRY_E5, 1}, {RTTI_INFANTRY, INFANTRY_RAMBO, 1},
        {RTTI_UNIT, UNIT_HTANK, 1},      {RTTI_UNIT, UNIT_MTANK, 1},      {RTTI_UNIT, UNIT_LTANK, 1},
        {RTTI_UNIT, UNIT_STANK, 1},      {RTTI_UNIT, UNIT_FTANK, 1},      {RTTI_UNIT, UNIT_APC, 1},
        {RTTI_UNIT, UNIT_MLRS, 1},       {RTTI_UNIT, UNIT_JEEP, 1},       {RTTI_UNIT, UNIT_MSAM, 1},
        {RTTI_UNIT, UNIT_BUGGY, 1},      {RTTI_UNIT, UNIT_ARTY, 1},       {RTTI_UNIT, UNIT_BIKE, 1},
        {RTTI_UNIT, UNIT_VICE, 1},       {RTTI_UNIT, UNIT_TRIC, 1},       {RTTI_UNIT, UNIT_TREX, 1},
        {RTTI_UNIT, UNIT_RAPT, 1},       {RTTI_UNIT, UNIT_STEG, 1},
    };

    /*........................................................................
    Button enumerations:
    ........................................................................*/
    enum
    {
        BUTTON_NAME = 100,
        BUTTON_AI_SLIDER,
        BUTTON_GDI,
        BUTTON_NOD,
        BUTTON_OBSERVER,
        BUTTON_TEAM1,
        BUTTON_TEAM2,
        BUTTON_TEAM3,
        BUTTON_TEAM4,
        BUTTON_OK,
        BUTTON_CANCEL,

        NUM_OF_BUTTONS = 5,
    };

    enum RedrawType
    {
        REDRAW_NONE = 0,
        REDRAW_CHOICES,
        REDRAW_BUTTONS,
        REDRAW_BACKGROUND,
        REDRAW_ALL = REDRAW_BACKGROUND
    };

    int btn_width = 90;
    int btn_height = 18;

    int d_name_x = 270;
    int d_name_y = 42;
    int d_name_w = 100;
    int d_name_h = 18;

    /*
    ** Side button dimensions
    */
    int d_gdi_x = 185;
    int d_gdi_y = 60;
    int d_gdi_w = btn_width;
    int d_gdi_h = btn_height;

    int d_nod_x = d_gdi_x + btn_width;
    int d_nod_y = d_gdi_y;
    int d_nod_w = btn_width;
    int d_nod_h = btn_height;

    int d_obs_x = d_nod_x + btn_width;
    int d_obs_y = d_gdi_y;
    int d_obs_w = btn_width;
    int d_obs_h = btn_height;

    int d_team1_x = 140;
    int d_team1_y = 78;
    int d_team1_w = btn_width;
    int d_team1_h = btn_height;

    int d_team2_x = d_team1_x + btn_width;
    int d_team2_y = d_team1_y;
    int d_team2_w = btn_width;
    int d_team2_h = btn_height;

    int d_team3_x = d_team2_x + btn_width;
    int d_team3_y = d_team1_y;
    int d_team3_w = btn_width;
    int d_team3_h = btn_height;

    int d_team4_x = d_team3_x + btn_width;
    int d_team4_y = d_team1_y;
    int d_team4_w = btn_width;
    int d_team4_h = btn_height;

    int d_ok_x = 225;
    int d_ok_y = 368;
    int d_ok_w = btn_width;
    int d_ok_h = btn_height;

    int d_cancel_x = d_ok_x + btn_width + 10;
    int d_cancel_y = d_ok_y;
    int d_cancel_w = btn_width;
    int d_cancel_h = btn_height;

    UnitChoice gdi_units[16];
    UnitChoice nod_units[17];
    UnitChoice all_units[23];
    UnitChoice* units = all_units;
    int choice_count;

    memcpy(gdi_units, _gdi_units, sizeof(gdi_units));
    memcpy(nod_units, _nod_units, sizeof(nod_units));
    memcpy(all_units, _all_units, sizeof(all_units));

    SliderClass ai_slider(BUTTON_AI_SLIDER, 212, 89, 216, 15);

    if (OfflineMode) {
        choice_count = ARRAY_SIZE(all_units);
        units = all_units;
        Side = HOUSE_SOLE_TEAM1;
        ai_slider.Set_Maximum(11);
        ai_slider.Set_Thumb_Size(1);
        ai_slider.Set_Value(Options.AISlider);
        GameParams.AIUnitsPer10min = 20 * Options.AISlider;

        if (20 * Options.AISlider > 120) {
            GameParams.AIUnitsPer10min = 120;
        }

        GameParams.MaxAIUnits = Options.AISlider;
        GameParams.AIBuildingsPer10min = GameParams.AIUnitsPer10min;
        GameParams.MaxAIBuildings = Options.AISlider / 3;
    }

    /*........................................................................
    Buttons
    ........................................................................*/
    ControlClass* commands = nullptr; // the button list

    EditClass name_edt(BUTTON_NAME,
                       MPlayerName,
                       10,
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

    TextButtonClass obsbtn(BUTTON_OBSERVER,
                           TXT_SPECTATOR,
                           TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                           d_obs_x,
                           d_obs_y,
                           d_obs_w,
                           d_obs_h);

    TextButtonClass team1btn(BUTTON_TEAM1,
                             TXT_TEAM1,
                             TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                             d_team1_x,
                             d_team1_y,
                             d_team1_w,
                             d_team1_h);

    TextButtonClass team2btn(BUTTON_TEAM2,
                             TXT_TEAM2,
                             TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                             d_team2_x,
                             d_team2_y,
                             d_team2_w,
                             d_team2_h);

    TextButtonClass team3btn(BUTTON_TEAM3,
                             TXT_TEAM3,
                             TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                             d_team3_x,
                             d_team3_y,
                             d_team3_w,
                             d_team3_h);

    TextButtonClass team4btn(BUTTON_TEAM4,
                             TXT_TEAM4,
                             TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                             d_team4_x,
                             d_team4_y,
                             d_team4_w,
                             d_team4_h);

    TextButtonClass okbtn(
        BUTTON_OK, TXT_OK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ok_x, d_ok_y, d_ok_w, d_ok_h);

    TextButtonClass cancelbtn(BUTTON_CANCEL,
                              TXT_CANCEL,
                              TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                              d_cancel_x,
                              d_cancel_y,
                              d_cancel_w,
                              d_cancel_h);

    Set_Logic_Page(SeenBuff);
    Read_MultiPlayer_Settings();
    name_edt.Set_Text(MPlayerName, 10);

    switch (Side) {
    default:
    case HOUSE_GOOD:
        gdibtn.Turn_On();
        break;
    case HOUSE_BAD:
        nodbtn.Turn_On();
        break;
    case HOUSE_SOLE_OBSERVER:
        obsbtn.Turn_On();
        break;
    case HOUSE_SOLE_TEAM1:
        team1btn.Turn_On();
        break;
    case HOUSE_SOLE_TEAM2:
        team2btn.Turn_On();
        break;
    case HOUSE_SOLE_TEAM3:
        team3btn.Turn_On();
        break;
    case HOUSE_SOLE_TEAM4:
        team4btn.Turn_On();
        break;
    }

    commands = &name_edt;

    if (OfflineMode) {
        ai_slider.Add_Tail(*commands);
    } else {
        gdibtn.Add_Tail(*commands);
        nodbtn.Add_Tail(*commands);
        obsbtn.Add_Tail(*commands);
        team1btn.Add_Tail(*commands);
        team2btn.Add_Tail(*commands);
        team3btn.Add_Tail(*commands);
        team4btn.Add_Tail(*commands);
    }

    okbtn.Add_Tail(*commands);
    cancelbtn.Add_Tail(*commands);

    Hide_Mouse();
    Set_Palette(BlackPalette);
    VisiblePage.Clear();
    memcpy(GamePalette, MFCD::Retrieve("TEMPERAT.PAL"), 768);
    Set_Palette(GamePalette);
    UseAltArt = true;
    Show_Mouse();

    if (Map.Theater != THEATER_TEMPERATE) {
        Reset_Theater_Shapes();
    }

    Map.Theater = THEATER_TEMPERATE;
    Map.Init(THEATER_TEMPERATE);
    TerrainTypeClass::Init(THEATER_TEMPERATE);
    TemplateTypeClass::Init(THEATER_TEMPERATE);
    OverlayTypeClass::Init(THEATER_TEMPERATE);
    UnitTypeClass::Init(THEATER_TEMPERATE);
    InfantryTypeClass::Init(THEATER_TEMPERATE);
    BuildingTypeClass::Init(THEATER_TEMPERATE);
    BulletTypeClass::Init(THEATER_TEMPERATE);
    AnimTypeClass::Init(THEATER_TEMPERATE);
    AircraftTypeClass::Init(THEATER_TEMPERATE);
    SmudgeTypeClass::Init(THEATER_TEMPERATE);
    LastTheater = THEATER_TEMPERATE;

    static int _current_choice;

    bool set_focus = true;
    bool process = true;
    bool cancelled = false;
    int display = REDRAW_ALL;
    int choice_rect_x2 = 0;
    int choice_rect_y1 = 0;
    int choice_rect_y2 = 0;
    int choice_rect_x1 = 0;

    int cameos_per_row = 6;
    int pixels_per_cameo = 36;
    int choice_start_x = 100;
    int choice_start_y = 62;

    Fancy_Text_Print(TXT_NONE, 0, 0, TBLACK, TBLACK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

    while (process) {
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            display = REDRAW_BACKGROUND;
        }

        Call_Back();
        Wait_Vert_Blank();
        Set_Palette(GamePalette);

        /*
        ** .................... Refresh display if needed ......................
        */
        if (display > 0) {
            Hide_Mouse();

            if (display >= REDRAW_BACKGROUND) {
                Dialog_Box(0, 0, 640, 400);
                Draw_Caption(TXT_NONE, 0, 0, 640);
                Fancy_Text_Print(TXT_CHOOSE_IDENTITY,
                                 320,
                                 14,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

                if (OfflineMode) {
                    Fancy_Text_Print(TXT_AI_OPPONENTS_COLON,
                                     320,
                                     70,
                                     CC_GREEN,
                                     TBLACK,
                                     TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
                    Fancy_Text_Print(TXT_MIN_ZERO,
                                     207,
                                     89,
                                     CC_GREEN,
                                     TBLACK,
                                     TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
                    Fancy_Text_Print(TXT_MAX_10,
                                     String_Pixel_Width(Text_String(TXT_MAX_DOT)) + 454,
                                     89,
                                     CC_GREEN,
                                     TBLACK,
                                     TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
                }
            }

            if (display >= REDRAW_CHOICES) {
                switch (Side) {
                default:
                case HOUSE_GOOD:
                    choice_count = ARRAY_SIZE(gdi_units);
                    units = gdi_units;
                    break;
                case HOUSE_BAD:
                    choice_count = ARRAY_SIZE(nod_units);
                    units = nod_units;
                    break;
                case HOUSE_SOLE_OBSERVER:
                    choice_count = 0;
                    units = nullptr;
                    break;
                case HOUSE_SOLE_TEAM1:
                case HOUSE_SOLE_TEAM2:
                case HOUSE_SOLE_TEAM3:
                case HOUSE_SOLE_TEAM4: // Fallthrough for teams
                    choice_count = ARRAY_SIZE(all_units);
                    units = all_units;
                    break;
                }

                if (choice_rect_x2 > choice_rect_x1) {
                    LogicPage->Fill_Rect(
                        choice_rect_x1 - 3, choice_rect_y1 - 3, choice_rect_x2 + 3, choice_rect_y2 + 3, BLACK);
                }

                if (choice_count <= 0) {
                    choice_rect_x1 = 0;
                    choice_rect_y1 = 0;
                    choice_rect_x2 = 0;
                    choice_rect_y2 = 0;
                } else {
                    if (Side > HOUSE_MULTI1) {
                        cameos_per_row = 8;
                    } else {
                        cameos_per_row = 6;
                    }
                    pixels_per_cameo = (640 - (cameos_per_row * 64)) / (cameos_per_row + 1);
                    choice_start_x = pixels_per_cameo + 64;
                    choice_start_y = 62;
                    int row_count = (choice_count + cameos_per_row - 1) / cameos_per_row;
                    choice_rect_x1 = 320 - cameos_per_row * (pixels_per_cameo + 64) / 2;
                    choice_rect_x2 = cameos_per_row * (pixels_per_cameo + 64) + choice_rect_x1;
                    choice_rect_y1 = 214 - 62 * row_count / 2;
                    choice_rect_y1 += 32;
                    choice_rect_y2 = 62 * row_count + choice_rect_y1;

                    for (int i = 0; i < choice_count; ++i) {
                        int choice_x = pixels_per_cameo / 2 + choice_start_x * (i % cameos_per_row) + choice_rect_x1;
                        int choice_y = choice_start_y * (i / cameos_per_row) + choice_rect_y1;
                        Draw_Choice_Entry(units[i].RTTI, units[i].Type, choice_x, choice_y, Side, units[i].Unk, names);
                    }

                    int rect_x =
                        pixels_per_cameo / 2 + _current_choice % cameos_per_row * choice_start_x + choice_rect_x1;
                    int rect_y = choice_start_y * (_current_choice / cameos_per_row) + choice_rect_y1;
                    LogicPage->Draw_Rect(rect_x - 3, rect_y - 3, rect_x + 66, rect_y + 50, WHITE);
                    LogicPage->Draw_Rect(rect_x - 2, rect_y - 2, rect_x + 65, rect_y + 49, LTGREY);
                    LogicPage->Draw_Rect(rect_x - 1, rect_y - 1, rect_x + 64, rect_y + 48, GREY);
                }
            }

            if (display >= REDRAW_BUTTONS) {
                commands->Flag_List_To_Redraw();
            }

            Show_Mouse();
            display = REDRAW_NONE;
        }

        /*
        ........................... Get user input ............................
        */
        int input = commands->Input();

        if (set_focus) {
            name_edt.Set_Focus();
            name_edt.Flag_To_Redraw();
            input = commands->Input();
            set_focus = false;
        }

        switch (input) {
        case KN_LMOUSE: {
            int mouse_x = Get_Mouse_X();
            int mouse_y = Get_Mouse_Y();

            if (mouse_x > choice_rect_x1 && mouse_x < choice_rect_x2 && mouse_y > choice_rect_y1
                && mouse_y < choice_rect_y2) {
                int last_choice = _current_choice;
                _current_choice = (mouse_y - choice_rect_y1) / choice_start_y * cameos_per_row
                                  + (mouse_x - choice_rect_x1) / choice_start_x;

                if (_current_choice >= 0) {
                    if (choice_count - 1 < _current_choice) {
                        _current_choice = choice_count - 1;
                    }
                } else {
                    _current_choice = 0;
                }

                if (_current_choice != last_choice) {
                    Hide_Mouse();
                    mouse_x = pixels_per_cameo / 2 + choice_start_x * (last_choice % cameos_per_row) + choice_rect_x1;
                    mouse_y = choice_start_y * (last_choice / cameos_per_row) + choice_rect_y1;
                    LogicPage->Draw_Rect(mouse_x - 3, mouse_y - 3, mouse_x + 66, mouse_y + 50, 12);
                    LogicPage->Draw_Rect(mouse_x - 2, mouse_y - 2, mouse_x + 65, mouse_y + 49, 12);
                    LogicPage->Draw_Rect(mouse_x - 1, mouse_y - 1, mouse_x + 64, mouse_y + 48, 12);
                    mouse_x =
                        pixels_per_cameo / 2 + choice_start_x * (_current_choice % cameos_per_row) + choice_rect_x1;
                    mouse_y = choice_start_y * (_current_choice / cameos_per_row) + choice_rect_y1;
                    LogicPage->Draw_Rect(mouse_x - 3, mouse_y - 3, mouse_x + 66, mouse_y + 50, 15);
                    LogicPage->Draw_Rect(mouse_x - 2, mouse_y - 2, mouse_x + 65, mouse_y + 49, 14);
                    LogicPage->Draw_Rect(mouse_x - 1, mouse_y - 1, mouse_x + 64, mouse_y + 48, 13);
                    Show_Mouse();
                }
            }
        } break;
        case (BUTTON_AI_SLIDER | KN_BUTTON):
            Options.AISlider = ai_slider.Get_Value();
            GameParams.AIUnitsPer10min = 20 * Options.AISlider;
            if (20 * Options.AISlider > 120) {
                GameParams.AIUnitsPer10min = 120;
            }
            GameParams.MaxAIUnits = Options.AISlider;
            GameParams.AIBuildingsPer10min = GameParams.AIUnitsPer10min;
            GameParams.MaxAIBuildings = Options.AISlider / 3;
            break;
        case (BUTTON_GDI | KN_BUTTON):
            Side = HOUSE_GOOD;
            gdibtn.Turn_On();
            nodbtn.Turn_Off();
            obsbtn.Turn_Off();
            team1btn.Turn_Off();
            team2btn.Turn_Off();
            team3btn.Turn_Off();
            team4btn.Turn_Off();

            if (_current_choice > ARRAY_SIZE(_gdi_units)) {
                _current_choice = ARRAY_SIZE(_gdi_units);
            }

            display = REDRAW_CHOICES;
            break;
        case (BUTTON_NOD | KN_BUTTON):
            Side = HOUSE_BAD;
            gdibtn.Turn_Off();
            nodbtn.Turn_On();
            obsbtn.Turn_Off();
            team1btn.Turn_Off();
            team2btn.Turn_Off();
            team3btn.Turn_Off();
            team4btn.Turn_Off();

            if (_current_choice > ARRAY_SIZE(_nod_units)) {
                _current_choice = ARRAY_SIZE(_nod_units);
            }

            display = REDRAW_CHOICES;
            break;
        case (BUTTON_OBSERVER | KN_BUTTON):
            Side = HOUSE_SOLE_OBSERVER;
            gdibtn.Turn_Off();
            nodbtn.Turn_Off();
            obsbtn.Turn_On();
            team1btn.Turn_Off();
            team2btn.Turn_Off();
            team3btn.Turn_Off();
            team4btn.Turn_Off();
            _current_choice = 0;
            display = REDRAW_CHOICES;
            break;
        case (BUTTON_TEAM1 | KN_BUTTON):
            Side = HOUSE_SOLE_TEAM1;
            gdibtn.Turn_Off();
            nodbtn.Turn_Off();
            obsbtn.Turn_Off();
            team1btn.Turn_On();
            team2btn.Turn_Off();
            team3btn.Turn_Off();
            team4btn.Turn_Off();
            _current_choice = 0;
            display = REDRAW_CHOICES;
            break;
        case (BUTTON_TEAM2 | KN_BUTTON):
            Side = HOUSE_SOLE_TEAM2;
            gdibtn.Turn_Off();
            nodbtn.Turn_Off();
            obsbtn.Turn_Off();
            team1btn.Turn_Off();
            team2btn.Turn_On();
            team3btn.Turn_Off();
            team4btn.Turn_Off();
            _current_choice = 0;
            display = REDRAW_CHOICES;
            break;
        case (BUTTON_TEAM3 | KN_BUTTON):
            Side = HOUSE_SOLE_TEAM3;
            gdibtn.Turn_Off();
            nodbtn.Turn_Off();
            obsbtn.Turn_Off();
            team1btn.Turn_Off();
            team2btn.Turn_Off();
            team3btn.Turn_On();
            team4btn.Turn_Off();
            _current_choice = 0;
            display = REDRAW_CHOICES;
            break;
        case (BUTTON_TEAM4 | KN_BUTTON):
            Side = HOUSE_SOLE_TEAM4;
            gdibtn.Turn_Off();
            nodbtn.Turn_Off();
            obsbtn.Turn_Off();
            team1btn.Turn_Off();
            team2btn.Turn_Off();
            team3btn.Turn_Off();
            team4btn.Turn_On();
            _current_choice = 0;
            display = REDRAW_CHOICES;
            break;
        case (BUTTON_OK | KN_BUTTON):
        case KN_RETURN: // Fallthough
            cancelled = false;
            process = false;
            break;
        case (BUTTON_CANCEL | KN_BUTTON):
        case KN_ESC:
            Options.Save_Settings();
            cancelled = true;
            process = false;
            break;
        default:
            break;
        }

        Frame_Limiter();
    }

    Hide_Mouse();
    Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
    Set_Palette(BlackPalette);
    VisiblePage.Clear();

    if (cancelled) {
        Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
        Set_Palette(Palette);
        memcpy(GamePalette, Palette, 768);
        Blit_Hid_Page_To_Seen_Buff();
    }

    UseAltArt = false;
    Show_Mouse();

    if (!cancelled) {
        if (choice_count <= 0) {
            ChosenRTTI = RTTI_NONE;
            ChosenType = 0;
        } else {
            ChosenRTTI = units[_current_choice].RTTI;
            ChosenType = units[_current_choice].Type;
        }
        if (ChosenRTTI == RTTI_AIRCRAFTTYPE && (ChosenType == AIRCRAFT_ORCA || ChosenType == AIRCRAFT_HELICOPTER)) {
            ChosenRTTI = RTTI_BUILDINGTYPE;
            ChosenType = STRUCT_HELIPAD;
        }
        Write_MultiPlayer_Settings();
        return true;
    }

    return false;
}

void Add_WDT_Radar()
{
    DBG_INFO("Add_WDT_Radar\n");

    if (!WDTRadarAdded) {
        Map.Radar_Activate(1);

        if (Map.Is_Zoomed()) {
            Map.Zoom_Mode(Coord_Cell(Map.TacticalCoord));
        }

        Map.Activate(0);
        WDTRadarAdded = true;
        Map.Activate(1);
    }
}

void Remove_WDT_Radar()
{
    DBG_INFO("Remove_WDT_Radar\n");

    if (WDTRadarAdded) {
        Map.Radar_Activate(0);

        if (Map.IsSidebarActive) {
            Map.Activate(0);
            WDTRadarAdded = false;
            Map.Activate(1);
        } else {
            WDTRadarAdded = false;
        }
    }
}

void Destroy_Server_Vector()
{
    DBG_WARN("Destroy_Server_Vector not implemented yet.\n");
}
