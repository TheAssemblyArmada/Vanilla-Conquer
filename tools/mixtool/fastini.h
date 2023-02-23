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
#ifndef FASTINI_H
#define FASTINI_H

#include "crc.h"
#include "crc32.h"
#include "search.h"
#include "listnode.h"
#include "pipe.h"
#include "straw.h"
#include <string.h>

#define SYS_NEW_LINE "\r\n"

class FileClass;
class Straw;
class Pipe;

// find out what this really is.
enum INIIntegerType
{
    INIINTEGER_AS_DECIMAL = 0,
    INIINTEGER_AS_HEX = 1,
    INIINTEGER_AS_MOTOROLA_HEX = 2
};

enum INILoadType
{
    INI_LOAD_INVALID = 0,
    INI_LOAD_OVERWRITE = 1,
    INI_LOAD_CREATE_OVERRIDES = 2
};

class FastINIClass
{
    class FastINIEntry : public VanillaNode<FastINIEntry>
    {
    public:
        // Class constructor and deconstructor.
        FastINIEntry(const char* name, const char* value)
            : EntryName(strdup(name))
            , Value(strdup(value))
        {
        }
        ~FastINIEntry();

        // int Index_ID();    //what could this be returning?

        const char* Get_Name()
        {
            return EntryName;
        }
        void Set_Name(const char* name);

        const char* Get_Value()
        {
            return Value;
        }
        void Set_Value(const char* new_value);

        int32_t const CRC(const char* value) const
        {
            return Calculate_CRC<CRC32Engine>(value, (int)strlen(value));
        }

    public:
        char* EntryName;
        char* Value;
    };

    class FastINISection : public VanillaNode<FastINISection>
    {
    public:
        FastINISection(const char* name)
            : SectionName(strdup(name))
        {
        }
        ~FastINISection();

        // int Index_ID();    //what could this be returning?

        FastINIEntry* Find_Entry(const char* entry) const;

        const char* Get_Name() const
        {
            return SectionName;
        }
        void Set_Name(const char* str);

        int Get_Entry_Count() const
        {
            return EntryIndex.Count();
        }

        int32_t const CRC(const char* value) const
        {
            return Calculate_CRC<CRC32Engine>(value, unsigned(strlen(value)));
        }

    public:
        char* SectionName;
        VanillaList<FastINIEntry> EntryList;
        IndexClass<FastINIEntry*> EntryIndex;
    };

public:
    enum
    {
        MAX_LINE_LENGTH = 128, // this is 512 in Generals, 4096 in BFME
        MAX_BUF_SIZE = 4096,
    };

public:
    FastINIClass()
    {
    }
    virtual ~FastINIClass();

    bool Clear(const char* section = nullptr, const char* entry = nullptr);
    bool Is_Loaded() const
    {
        return SectionList.First()->Is_Valid();
    }

    int Save(const char* filename) const;
    int Save(FileClass& file) const;
    virtual int Save(Pipe& pipe) const;
    int Load(const char* filename);
    int Load(FileClass& file);
    virtual int Load(Straw& straw);

    const VanillaList<FastINISection>& Section_List() const
    {
        return SectionList;
    }
    IndexClass<FastINISection*>& Section_Index()
    {
        return SectionIndex;
    }

    bool Is_Present(const char* section, const char* entry = nullptr);

    // Returns the section object if it exists.
    FastINISection* Find_Section(const char* section) const;
    bool Section_Present(const char* section) const
    {
        return Find_Section(section) != nullptr;
    }
    int Section_Count()
    {
        return SectionIndex.Count();
    }

    // Returns the entry object if it exists.
    FastINIEntry* Find_Entry(const char* section, const char* entry) const;
    int Entry_Count(const char* section) const;
    const char* Get_Entry(const char* section, int index) const;

    // Enumerate_Entries()
    //   Enumerates all entries (key/value pairs) of a given section.
    //   Returns the number of entries present or -1 upon error.
    int Enumerate_Entries(const char* section, const char* entry_prefix, uint32_t start_number, uint32_t end_number);

    bool Put_Int(const char* section, const char* entry, int value, int format = INIINTEGER_AS_DECIMAL);
    int Get_Int(const char* section, const char* entry, int defvalue = 0) const;
    bool Put_Bool(const char* section, const char* entry, bool value);
    bool const Get_Bool(const char* section, const char* entry, bool defvalue = false) const;
    bool Put_Hex(const char* section, const char* entry, int value);
    int Get_Hex(const char* section, const char* entry, int defvalue = 0) const;
    bool Put_String(const char* section, const char* entry, const char* string);
    int Get_String(const char* section,
                   const char* entry,
                   const char* defvalue = "",
                   char* buffer = nullptr,
                   int length = 0) const;

protected:
    static void Strip_Comments(char* line);
    static int32_t const CRC(const char* string);

protected:
    VanillaList<FastINISection> SectionList;
    IndexClass<FastINISection*> SectionIndex;
};

#endif // FASTINI_H
