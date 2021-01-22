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
#include "solehelp.h"
#include "conquer.h"
#include "function.h"
#include "common/ini.h"
#include "common/rect.h"
#include "common/ccfile.h"
#include "common/framelimit.h"
#include <assert.h>
#include <string.h>

class SoleHelpClass
{
public:
    struct SoleHelpBufferStruct
    {
        Rect ClipRect;
        char Tooltip[64];
        char Description[512];
    };

    SoleHelpClass(const char* filename = "SSHELP.INI")
        : HelpFile(filename)
        , EntryCount(0)
        , Entries(nullptr)
    {
    }
    ~SoleHelpClass()
    {
        Reset();
    }

    void Set_File_Name(const char* filename)
    {
        HelpFile = filename;
        Reset();
    }

    const char* Get_File_Name()
    {
        return HelpFile;
    }
    bool Load();
    bool Get_Entry(int xpos, int ypos, const char*& tooltip, const char*& description);

private:
    void Read_Entry(char* entry, int index);
    void Reset()
    {
        EntryCount = 0;
        delete Entries;
        Entries = nullptr;
    }

    const char* HelpFile;
    unsigned EntryCount;
    SoleHelpBufferStruct* Entries;
};

bool SoleHelpClass::Load()
{
    const char* section = "SSHELP";
    char buf[256];
    CCFileClass file(HelpFile);

    if (!file.Is_Available()) {
        WWMessageBox().Process("File error! (SSHELP)", TXT_OK);
        return false;
    }

    INIClass ini;
    ini.Load(file);

    Reset();
    EntryCount = ini.Entry_Count(section);

    if (EntryCount == 0) {
        return false;
    }

    Entries = new SoleHelpBufferStruct[EntryCount];

    for (unsigned index = 0; index < EntryCount; index++) {
        char const* entry = ini.Get_Entry(section, index);
        ini.Get_String(section, ini.Get_Entry(section, index), "x", buf, sizeof(buf));

        if (strcmp(buf, "x") == 0) {
            WWMessageBox().Process("Decode error! (SSHELP)", TXT_OK);
            Reset();
            return false;
        }

        Read_Entry(buf, index);
    }

    return true;
}

bool SoleHelpClass::Get_Entry(int xpos, int ypos, const char*& tooltip, const char*& description)
{
    if (Entries == nullptr) {
        return false;
    }

    for (unsigned i = 0; i < EntryCount; ++i) {
        if (ypos >= Entries[i].ClipRect.Y && Entries[i].ClipRect.Height + Entries[i].ClipRect.Y >= ypos
            && xpos >= Entries[i].ClipRect.X && Entries[i].ClipRect.Width + Entries[i].ClipRect.X >= xpos) {
            tooltip = Entries[i].Tooltip;
            description = Entries[i].Description;
            return true;
        }
    }

    return false;
}

void SoleHelpClass::Read_Entry(char* entry, int index)
{
    assert((unsigned)index < EntryCount);
    char* token = strtok(entry, ",");
    if (token) {
        Entries[index].ClipRect.X = atoi(token);
    }
    token = strtok(0, ",");
    if (token) {
        Entries[index].ClipRect.Y = atoi(token);
    }
    token = strtok(0, ",");
    if (token) {
        Entries[index].ClipRect.Width = atoi(token);
    }
    token = strtok(0, ",");
    if (token) {
        Entries[index].ClipRect.Height = atoi(token);
    }
    Entries[index].Tooltip[0] = 0;
    token = strtok(0, "\"");
    if (token) {
        strncpy(Entries[index].Tooltip, token, sizeof(Entries[index].Tooltip));
    }
    Entries[index].Description[0] = 0;
    if (strtok(0, "\"")) {
        token = strtok(0, "\"");
        if (token) {
            strncpy(Entries[index].Description, token, sizeof(Entries[index].Description));
        }
    }
}

static bool Clip_Rect(Rect& dst, Rect& src)
{
    bool clip_x = false;
    bool clip_y = false;
    int dst_x2 = dst.Width + dst.X;
    int dst_y2 = dst.Height + dst.Y;
    int src_x2 = src.Width + src.X;
    int src_y2 = src.Height + src.Y;

    if (dst.X >= src.X) {
        if (src_x2 >= dst.X) {
            clip_x = true;
        }
    } else if (dst_x2 >= src.X) {
        clip_x = true;
    }

    if (dst.Y >= src.Y) {
        if (src_y2 >= dst.Y) {
            clip_y = true;
        }
    } else if (dst_y2 >= src.Y) {
        clip_y = true;
    }

    if (!clip_x || !clip_y) {
        return false;
    }

    int x1;
    int y1;
    int x2;
    int y2;

    if (dst.X >= src.X) {
        x1 = src.X;
    } else {
        x1 = dst.X;
    }
    dst.X = x1;

    if (dst.Y >= src.Y) {
        y1 = src.Y;
    } else {
        y1 = dst.Y;
    }
    dst.Y = y1;

    if (dst_x2 <= src_x2) {
        x2 = src_x2;
    } else {
        x2 = dst_x2;
    }
    dst.Width = x2 - dst.X;

    if (dst_y2 <= src_y2) {
        y2 = src_y2;
    } else {
        y2 = dst_y2;
    }
    dst.Height = y2 - dst.Y;

    return true;
}

void Help_Menu()
{
    SoleHelpClass help;
    BufferClass buff(0x2000);
    int last_xpos = -1;
    int last_ypos = -1;
    const char* last_desc = nullptr;

    if (buff.Get_Buffer() == nullptr) {
        WWMessageBox().Process("Buffer allocation failed! (SSHELP)", TXT_OK);
        return;
    }

    if (!help.Load()) {
        WWMessageBox().Process("Configuration load failed! (SSHELP)", TXT_OK);
        return;
    }

    Fade_Palette_To(BlackPalette, 15, Call_Back);
    Hide_Mouse();
    Load_Title_Screen("SSHELP.PCX", &HidPage, Palette);
    memcpy(GamePalette, Palette, 768);
    Blit_Hid_Page_To_Seen_Buff();
    Fade_Palette_To(Palette, FADE_PALETTE_MEDIUM, Call_Back);
    Show_Mouse();

    Set_Logic_Page(&HidPage);
    Fancy_Text_Print(TXT_NONE, 0, 0, TBLACK, TBLACK, TPF_MAP | TPF_NOSHADOW);

    Rect clip_rect;
    clip_rect.X = 0;
    clip_rect.Y = 0;
    clip_rect.Width = 16;
    clip_rect.Height = 1;

    Keyboard->Clear();

    bool process = true;

    while (process) {
        int xpos = Get_Mouse_X();
        int ypos = Get_Mouse_Y();
        if (xpos != last_xpos || ypos != last_ypos) {
            last_xpos = xpos;
            last_ypos = ypos;
            const char* tool_tip;
            const char* description;

            /*
            ** Retrieve help text and prepare to display if an entry exists for where cursor is.
            */
            if (help.Get_Entry(xpos, ypos, tool_tip, description)) {
                Hide_Mouse();

                Rect txt_clip;
                txt_clip.X = xpos;
                txt_clip.Y = ypos + 16;
                txt_clip.Width = String_Pixel_Width(tool_tip) + 3;
                txt_clip.Height = FontHeight + 2;

                /*
                ** Keep the text displayed entirely within the screen boundaries.
                */
                if (txt_clip.Width + txt_clip.X > HidPage.Get_Width()) {
                    txt_clip.X = HidPage.Get_Width() - txt_clip.Width;
                }

                if (txt_clip.Height + txt_clip.Y >= HidPage.Get_Width() || ypos > 380 && ypos < 400 && xpos < 24) {
                    txt_clip.Y = ypos - FontHeight - 2;
                }

                /*
                ** Save a copy of the area without text to use to remove text later.
                */
                HidPage.To_Buffer(txt_clip.X, txt_clip.Y, txt_clip.Width, txt_clip.Height, &buff);

                Fancy_Text_Print(tool_tip, txt_clip.X + 1, txt_clip.Y + 1, YELLOW, BLACK, TPF_MAP | TPF_NOSHADOW);
                HidPage.Draw_Rect(
                    txt_clip.X, txt_clip.Y, txt_clip.Width + txt_clip.X - 1, txt_clip.Height + txt_clip.Y - 1, YELLOW);

                if (!Clip_Rect(clip_rect, txt_clip)) {
                    HidPage.Blit(
                        SeenBuff, txt_clip.X, txt_clip.Y, txt_clip.X, txt_clip.Y, txt_clip.Width, txt_clip.Height);
                }

                HidPage.Blit(
                    SeenBuff, clip_rect.X, clip_rect.Y, clip_rect.X, clip_rect.Y, clip_rect.Width, clip_rect.Height);
                buff.To_Page(txt_clip.X, txt_clip.Y, txt_clip.Width, txt_clip.Height, HidPage);
                clip_rect = txt_clip;

                if (description != last_desc) {
                    if (last_desc != nullptr || description == nullptr) {
                        HidPage.Blit(SeenBuff, 2, 402, 2, 402, 474, 76);
                    }

                    last_desc = description;

                    if (description != nullptr) {
                        char string[512];
                        int w = 470;
                        int h = 74;
                        Sound_Effect(VOC_UP, VOL_3);
                        strcpy(string, description);
                        Format_Window_String(string, 470, w, h);
                        Set_Logic_Page(&SeenBuff);
                        Fancy_Text_Print(string, 2u, 402, 5u, 0, TPF_MAP | TPF_FULLSHADOW);
                        Set_Logic_Page(&HidPage);
                    }
                }

                Show_Mouse();
            } else if (clip_rect.Width != 0 && clip_rect.Height != 0) {
                Hide_Mouse();
                HidPage.Blit(
                    SeenBuff, clip_rect.X, clip_rect.Y, clip_rect.X, clip_rect.Y, clip_rect.Width, clip_rect.Height);
                clip_rect.Width = 0;
                HidPage.Blit(SeenBuff, 2, 402, 2, 402, 476, 76);
                last_desc = nullptr;
                Show_Mouse();
            }
        }

        Call_Back();

        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            Hide_Mouse();
            Load_Title_Screen("SSHELP.PCX", &HidPage, Palette);
            memcpy(GamePalette, Palette, 0x300u);
            Blit_Hid_Page_To_Seen_Buff();
            Fade_Palette_To(Palette, FADE_PALETTE_SLOW, Call_Back);
            Show_Mouse();
            Set_Logic_Page(&HidPage);
            last_xpos = -1;
            last_ypos = -1;
            last_desc = 0;
        }

        if (Keyboard->Check() != KN_NONE) {
            KeyNumType key = Keyboard->Get();

            if (key == KN_BUTTON || key == KN_ESC || key == KN_SPACE) {
                process = false;
            }
        }

        Set_Palette(GamePalette);

        Frame_Limiter();
    }

    Set_Logic_Page(&SeenBuff);
    Hide_Mouse();
    Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
    HidPage.Clear();
    SeenBuff.Clear();
    Show_Mouse();
}
