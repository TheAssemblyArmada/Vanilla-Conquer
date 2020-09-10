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

#ifndef VOICETHEMES_H
#define VOICETHEMES_H

#include "common/miscasm.h"
#include "vector.h"
#include <stdint.h>
#include <string.h>

enum VoiceSoundType
{
    VOX_THEME_SND_ARMOR,
    VOX_THEME_SND_MEGAARMOR,
    VOX_THEME_SND_WEAPON,
    VOX_THEME_SND_MEGAWEAPON,
    VOX_THEME_SND_SPEED,
    VOX_THEME_SND_MEGASPEED,
    VOX_THEME_SND_RAPIDRELOAD,
    VOX_THEME_SND_MEGARAPIDRELOAD,
    VOX_THEME_SND_RANGE,
    VOX_THEME_SND_MEGARANGE,
    VOX_THEME_SND_HEAL,
    VOX_THEME_SND_STEALTHON,
    VOX_THEME_SND_STEALTHOFF,
    VOX_THEME_SND_TELEPORT,
    VOX_THEME_SND_NUKE,
    VOX_THEME_SND_ION,
    VOX_THEME_SND_UNCLOAKALL,
    VOX_THEME_SND_RESHROUD,
    VOX_THEME_SND_UNSHROUD,
    VOX_THEME_SND_RADAR,
    VOX_THEME_SND_ARMAGEDDON,
    VOX_THEME_SND_YOULOSE,
    VOX_THEME_SND_TEST,
    VOX_THEME_SND_UNIT1,
    VOX_THEME_SND_COUNT,
    VOX_THEME_SND_FIRST = 0,
    VOX_THEME_SND_NONE = -1,
};

inline VoiceSoundType operator++(VoiceSoundType& n)
{
    n = (VoiceSoundType)(((int)n) + 1);
    return n;
}

enum VoiceThemesType
{
    VOX_THEME_EVA,
    VOX_THEME_COMMANDO,
    VOX_THEME_LETS_MAKE_A_KILL,
    VOX_THEME_1_900_KILL_YOU,
    VOX_THEME_COUNT,
};

class VoiceThemeClass
{
public:
    VoiceThemeClass()
    {
        ThemeName[0] = '\0';
    }
    virtual ~VoiceThemeClass()
    {
    }
    virtual void Set_Theme_Name(const char* name)
    {
        strcpy(ThemeName, name);
    }
    virtual const char* Get_Theme_Name()
    {
        return ThemeName;
    }
    virtual void Play(VoiceSoundType type) = 0;
    virtual void Add(VoiceSoundType type, uintptr_t data, int extra) = 0;

protected:
    char ThemeName[255];
};

extern DynamicVectorClass<VoiceThemeClass*> VoiceThemes;

void Init_Voice_Themes();

#endif
