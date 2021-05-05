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
#ifndef STRINI_H
#define STRINI_H

#include "ini.h"

#define STRINI_MAX_BUF_SIZE 4096

class StrINIClass : public INIClass
{
    struct LangMap
    {
        const char *lang;
        int code;
    };

public:
    StrINIClass() {}
    virtual ~StrINIClass() {}

    int Load(Straw &straw);

    bool Put_Table_String(const char *section, const char *entry, const char *string, const char *lang = nullptr);
    int Get_Table_String(const char *section, const char *entry, const char *defvalue = "", char *buffer = nullptr,
        int length = 0, const char *lang = nullptr) const;

private:
    static int Get_Lang_Code(const char *lang);
    static char *Codepage_To_UTF8(const char *string, int codepage);
    static char *UTF8_To_Codepage(const char *string, int codepage);
};

#endif
