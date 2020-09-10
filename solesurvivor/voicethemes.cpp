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
#include "voicethemes.h"
#include "function.h"
#include "solerandom.h"

DynamicVectorClass<VoiceThemeClass*> VoiceThemes;

class AudVoiceThemeClass : public VoiceThemeClass
{
public:
    AudVoiceThemeClass();
    virtual ~AudVoiceThemeClass()
    {
    }
    virtual void Play(VoiceSoundType type) override;
    virtual void Add(VoiceSoundType type, uintptr_t data, int extra) override;

private:
    VoxType Vox[VOX_THEME_SND_COUNT];
    int Variations[VOX_THEME_SND_COUNT];
};

class WavVoiceThemeClass : public VoiceThemeClass
{
    enum
    {
        VARAIATION_COUNT = 3,
    };

public:
    WavVoiceThemeClass();
    virtual ~WavVoiceThemeClass()
    {
    }
    virtual void Play(VoiceSoundType type) override;
    virtual void Add(VoiceSoundType type, uintptr_t data, int extra) override;

private:
    char FileNames[VARAIATION_COUNT][VOX_THEME_SND_COUNT][260];
    int Variation[VOX_THEME_SND_COUNT];
};

AudVoiceThemeClass::AudVoiceThemeClass()
{
    for (VoiceSoundType i = VOX_THEME_SND_FIRST; i < VOX_THEME_SND_COUNT; ++i) {
        Vox[i] = VOX_NONE;
        Variations[i] = 0;
    }
}

void AudVoiceThemeClass::Play(VoiceSoundType type)
{
    if (Vox[type] == VOX_NONE) {
        return;
    }

    int offset = 0;

    if (Variations[type] > 1) {
        offset = Choose_Random_Val(0, Variations[type] - 1);
    }

    Speak(static_cast<VoxType>(Vox[type] + offset));
}

void AudVoiceThemeClass::Add(VoiceSoundType type, uintptr_t data, int extra)
{
    Vox[type] = static_cast<VoxType>(data);
    Variations[type] = extra;
}

WavVoiceThemeClass::WavVoiceThemeClass()
{
    for (int i = 0; i < VARAIATION_COUNT; ++i) {
        for (VoiceSoundType j = VOX_THEME_SND_FIRST; j < VOX_THEME_SND_COUNT; ++j) {
            FileNames[i][j][0] = '\0';
        }

        Variation[i] = 0;
    }
}

void WavVoiceThemeClass::Play(VoiceSoundType type)
{
    // TODO Needs Play_Wave code adding to audio engine.
}

void WavVoiceThemeClass::Add(VoiceSoundType type, uintptr_t data, int extra)
{
    if (type >= VOX_THEME_SND_FIRST && type < VOX_THEME_SND_COUNT) {
        char str[MAX_PATH];
        strcpy(str, reinterpret_cast<const char*>(data));
        char* name = strtok(str, ";");
        Variation[type] = 0;

        while (name != nullptr) {
            strcpy(this->FileNames[Variation[type]][type], name);

            if (++Variation[type] >= VARAIATION_COUNT) {
                break;
            }

            name = strtok(nullptr, ";");
        }
    }
}

void Init_Voice_Themes()
{
    // No audio theme.
    VoiceThemeClass* vtc = new AudVoiceThemeClass;
    vtc->Set_Theme_Name(Text_String(922 /*TXT_VOX_NONE*/));
    VoiceThemes.Add(vtc);

    // Eva theme.
    vtc = new AudVoiceThemeClass;
    vtc->Set_Theme_Name("EVA");
    vtc->Add(VOX_THEME_SND_ARMOR, VOX_E_ARMOR1, 1);
    vtc->Add(VOX_THEME_SND_MEGAARMOR, VOX_E_MEGAA1, 1);
    vtc->Add(VOX_THEME_SND_WEAPON, VOX_E_WEAPN1, 1);
    vtc->Add(VOX_THEME_SND_MEGAWEAPON, VOX_E_MEGAW1, 1);
    vtc->Add(VOX_THEME_SND_SPEED, VOX_E_SPEED1, 1);
    vtc->Add(VOX_THEME_SND_MEGASPEED, VOX_E_MEGAS1, 1);
    vtc->Add(VOX_THEME_SND_RAPIDRELOAD, VOX_E_RAPID1, 1);
    vtc->Add(VOX_THEME_SND_MEGARAPIDRELOAD, VOX_E_MEGARR, 1);
    vtc->Add(VOX_THEME_SND_RANGE, VOX_E_RANGE1, 1);
    vtc->Add(VOX_THEME_SND_MEGARANGE, VOX_E_MGARNG, 1);
    vtc->Add(VOX_THEME_SND_HEAL, VOX_E_HEAL1, 1);
    vtc->Add(VOX_THEME_SND_STEALTHON, VOX_E_STLTHA, 1);
    vtc->Add(VOX_THEME_SND_STEALTHOFF, VOX_E_STLTHD, 1);
    vtc->Add(VOX_THEME_SND_TELEPORT, VOX_E_TELEP1, 1);
    vtc->Add(VOX_THEME_SND_NUKE, VOX_E_NUKE1, 1);
    vtc->Add(VOX_THEME_SND_ION, VOX_E_ION1, 1);
    vtc->Add(VOX_THEME_SND_UNCLOAKALL, VOX_E_STLALL, 1);
    vtc->Add(VOX_THEME_SND_RESHROUD, VOX_E_DRK1, 1);
    vtc->Add(VOX_THEME_SND_UNSHROUD, VOX_E_MAPUP1, 1);
    vtc->Add(VOX_THEME_SND_RADAR, VOX_E_RADAR1, 1);
    vtc->Add(VOX_THEME_SND_ARMAGEDDON, VOX_E_ARMGD1, 1);
    vtc->Add(VOX_THEME_SND_YOULOSE, VOX_E_LOSER1, 2);
    vtc->Add(VOX_THEME_SND_TEST, VOX_E_HELLO1, 1);
    vtc->Add(VOX_THEME_SND_UNIT1, VOX_E_UNIT1, 1);
    VoiceThemes.Add(vtc);
}
