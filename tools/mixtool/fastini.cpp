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
#include "fastini.h"
#include "debugstring.h"
#include "b64pipe.h"
#include "b64straw.h"
#include "xpipe.h"
#include "xstraw.h"
#include "cstraw.h"
#include "rawfile.h"
#include "readline.h"
#include "miscasm.h"
#include <ctype.h>
#include <stdio.h>

FastINIClass::FastINIEntry::~FastINIEntry()
{
    if (EntryName != nullptr) {
        free((void*)EntryName);
    }

    EntryName = nullptr;

    if (Value != nullptr) {
        free((void*)Value);
    }

    Value = nullptr;
}

void FastINIClass::FastINIEntry::Set_Name(const char* new_name)
{
    if (EntryName != nullptr) {
        free((void*)EntryName);
    }

    EntryName = strdup(new_name);
}

void FastINIClass::FastINIEntry::Set_Value(const char* str)
{
    if (Value != nullptr) {
        free((void*)Value);
    }

    Value = strdup(str);
}

FastINIClass::FastINISection::~FastINISection()
{
    if (SectionName != nullptr) {
        free((void*)SectionName);
    }
    SectionName = nullptr;

    EntryList.Delete();
}

FastINIClass::FastINIEntry* FastINIClass::FastINISection::Find_Entry(const char* entry) const
{
    int crc;

    assert(entry != nullptr);

    // this function setup needs switching around. errors first etc.
    if (entry != nullptr && (crc = CRC(entry), EntryIndex.Is_Present(crc))) {
        // DBG_LOG("Fetching entry %s with CRC %08x", entry, crc);
        return EntryIndex.Fetch_Index(crc);
    }
    return nullptr;
}

void FastINIClass::FastINISection::Set_Name(const char* str)
{
    if (SectionName != nullptr) {
        free((void*)SectionName);
    }

    SectionName = strdup(str);
}

FastINIClass::~FastINIClass()
{
    Clear();
}

bool FastINIClass::Clear(const char* section, const char* entry)
{
    FastINISection* sectionptr;
    FastINIEntry* entryptr;

    if (section) {
        if ((sectionptr = Find_Section(section)) != nullptr) {
            if (entry) {
                if ((entryptr = sectionptr->Find_Entry(entry)) == nullptr) {
                    return true;
                }

                sectionptr->EntryIndex.Remove_Index(CRC(entryptr->Get_Name()));
                delete entryptr;

            } else {
                if (!sectionptr) {
                    return true;
                }

                SectionIndex.Remove_Index(CRC(sectionptr->Get_Name()));
                delete sectionptr;
            }

            return true;
        }

        return false;
    }

    SectionList.Delete();
    SectionIndex.Clear();

    return true;
}

int FastINIClass::Save(const char* filename) const
{
    RawFileClass fc(filename);

    return Save(fc);
}

int FastINIClass::Save(FileClass& file) const
{
    FilePipe fpipe(file);

    return Save(fpipe);
}

int FastINIClass::Save(Pipe& pipe) const
{
    char space_fill[256];

    // total amount of bytes written to the stream.
    int total = 0;

    // fill 'space_fill' with spaces.
    memset(space_fill, ' ', sizeof(space_fill));

    for (FastINISection* secptr = SectionList.First(); secptr != nullptr; secptr = secptr->Next()) {
        // If this is not a valid section node, break out of the loop.
        if (!secptr->Is_Valid()) {
            // DBG_LOG("FastINIClass::Save() - Invalid section encountered");
            break;
        }

        // Write the section name to the stream.
        total += pipe.Put("[", 1);
        total += pipe.Put(secptr->Get_Name(), (int)strlen(secptr->Get_Name()));
        total += pipe.Put("]", 1);
        total += pipe.Put(SYS_NEW_LINE, (int)strlen(SYS_NEW_LINE));

        // Loop though entries and write to stream.
        for (FastINIEntry* entryptr = secptr->EntryList.First(); entryptr != nullptr; entryptr = entryptr->Next()) {
            // If this is not a valid entry node, break out of the loop.
            if (!entryptr->Is_Valid()) {
                // DBG_LOG("FastINIClass::Save() - Invalid entry encountered");
                break;
            }

            // Write the entry name.
            total += pipe.Put(entryptr->Get_Name(), (int)strlen(entryptr->Get_Name()));
            // Next, write the Equate char.
            total += pipe.Put("=", 1);
            // Finally write the value.
            total += pipe.Put(entryptr->Get_Value(), (int)strlen(entryptr->Get_Value()));
            // Write a carriage return.
            total += pipe.Put(SYS_NEW_LINE, (int)strlen(SYS_NEW_LINE));
        }

        // Write a new line underneath the last entry of the section
        // to space out the sections neatly.
        total += pipe.Put(SYS_NEW_LINE, (int)strlen(SYS_NEW_LINE));
    }

    // End the data stream.
    total += pipe.End();

    // Return the total count of bytes we wrote to the stream.
    return total;
}

int FastINIClass::Load(const char* filename)
{
    RawFileClass fc(filename);

    return Load(fc);
}

int FastINIClass::Load(FileClass& file)
{
    FileStraw fstraw(file);

    return Load(fstraw);
}

int FastINIClass::Load(Straw& straw)
{
    char buffer[MAX_LINE_LENGTH];
    bool end_of_file = false;
    CacheStraw cstraw(0x4000);
    cstraw.Get_From(&straw);

    // Read all lines until we find the first section.
    while (!end_of_file) {
        Read_Line(cstraw, buffer, MAX_LINE_LENGTH, end_of_file);
        if (end_of_file) {
            DBG_LOG("FastINIClass::Load() - reached end of file before finding a section");
            return INI_LOAD_INVALID;
        }

        if (buffer[0] == '[' && strchr(buffer, ']') != nullptr) {
            break;
        }
    }

    while (!end_of_file) {
        assert(buffer[0] == '[' && strchr(buffer, ']')); // at start of section
        // Remove square brackets to get section name and create new section.
        buffer[0] = ' ';
        *strchr(buffer, ']') = '\0';
        strtrim(buffer);
        FastINISection* section = new FastINISection(buffer);

        if (section == nullptr) {
            DBG_LOG("FastINIClass::Load() - failed to create section!");

            Clear();

            return INI_LOAD_INVALID;
        }

        while (!end_of_file) {
            int count = Read_Line(cstraw, buffer, sizeof(buffer), end_of_file);
            // Check we don't have another section.
            if (buffer[0] == '[' && strchr(buffer, ']')) {
                break;
            }

            // Strip comments from the line.
            Strip_Comments(buffer);
            char* delimiter = strchr(buffer, '=');

            if (count > 0 && buffer[0] != ';' && buffer[0] != '=') {
                if (delimiter != nullptr) {
                    delimiter[0] = '\0';
                    char* entry = buffer;
                    char* value = delimiter + 1;

                    strtrim(entry);

                    if (entry[0] != '\0') {
                        strtrim(value);

                        if (value[0] == '\0') {
                            continue;
                        }

                        FastINIEntry* entryptr = new FastINIEntry(entry, value);
                        if (!entryptr) {
                            DBG_LOG("Failed to create entry '%s = %s'.", entry, value);

                            delete section;
                            Clear();

                            return INI_LOAD_INVALID;
                        }

                        // Is this Name, Value or something?
                        //CRC(entryptr->Get_Name());
                        int32_t crc = CRC(entryptr->Get_Name());

                        if (section->EntryIndex.Is_Present(crc)) {
                            // Duplicate_CRC_Error(__FUNCTION__, section->Get_Name(), entryptr->Get_Name());
                        }

                        section->EntryIndex.Add_Index(crc, entryptr);
                        section->EntryList.Add_Tail(entryptr);
                    }
                }
            }
        }

        if (section->EntryList.Is_Empty()) {
            delete section;
        } else {
            int32_t crc = CRC(section->Get_Name());
            SectionIndex.Add_Index(crc, section);
            SectionList.Add_Tail(section);
        }
    }

    return INI_LOAD_OVERWRITE;
}

bool FastINIClass::Is_Present(const char* section, const char* entry)
{
    assert(section != nullptr);
    //assert(entry != nullptr);

    if (section != nullptr && entry != nullptr) {
        return Find_Entry(section, entry) != nullptr;

    } else if (section != nullptr) {
        return Find_Section(section) != nullptr;
    }

    return false;
}

FastINIClass::FastINISection* FastINIClass::Find_Section(const char* section) const
{
    assert(section != nullptr);

    int crc;

    if (section != nullptr && (crc = CRC(section)) != 0 && SectionIndex.Is_Present(crc)) {
        return SectionIndex.Fetch_Index(crc);
    }

    return nullptr;
}

FastINIClass::FastINIEntry* FastINIClass::Find_Entry(const char* section, const char* entry) const
{
    assert(section != nullptr);
    assert(entry != nullptr);

    FastINISection* sectionptr = Find_Section(section);

    if (sectionptr != nullptr) {
        return sectionptr->Find_Entry(entry);
    }

    return nullptr;
}

int FastINIClass::Entry_Count(const char* section) const
{
    assert(section != nullptr);

    FastINISection* sectionptr = Find_Section(section);

    if (sectionptr != nullptr) {
        return sectionptr->Get_Entry_Count();
    }

    return 0;
}

const char* FastINIClass::Get_Entry(const char* section, int index) const
{
    assert(section != nullptr);

    FastINISection* sectionptr = Find_Section(section);

    if (sectionptr != nullptr) {
        int count = index;

        if (index < sectionptr->Get_Entry_Count()) {
            for (FastINIEntry* entryptr = sectionptr->EntryList.First(); entryptr != nullptr;
                 entryptr = entryptr->Next()) {
                if (!entryptr->Is_Valid()) {
                    break;
                }

                if (!count) {
                    return entryptr->Get_Name(); // TODO
                }

                count--;
            }
        }
    }

    return nullptr;
}

int FastINIClass::Enumerate_Entries(const char* section,
                                    const char* entry_prefix,
                                    uint32_t start_number,
                                    uint32_t end_number)
{
    char buffer[256];
    uint32_t i = start_number;

    assert(section != nullptr);
    assert(!start_number && !end_number);

    for (; i < end_number; ++i) {
        snprintf(buffer, sizeof(buffer), "%s%d", entry_prefix, i);

        if (!Find_Entry(section, buffer)) {
            break;
        }
    }

    return i - start_number;
}

bool FastINIClass::Put_Int(const char* section, const char* entry, int value, int format)
{
    char buffer[512];

    if (format == INIINTEGER_AS_HEX) {
        sprintf(buffer, "%Xh", value);
    } else if (format == INIINTEGER_AS_MOTOROLA_HEX) {
        sprintf(buffer, "$%X", value);
    } else {
        sprintf(buffer, "%d", value);
    }

    return Put_String(section, entry, buffer);
}

int FastINIClass::Get_Int(const char* section, const char* entry, int defvalue) const
{
    FastINIEntry* entryptr;
    const char* value;

    if (section != nullptr && entry != nullptr && (entryptr = Find_Entry(section, entry)) != nullptr
        && entryptr->Get_Name() && (value = entryptr->Get_Value()) != nullptr) {
        if (value[0] == '$') {
            sscanf(value, "$%x", &defvalue);
            return defvalue;
        }

        if (tolower(value[strlen(value) - 1]) == 'h') {
            sscanf(value, "%xh", &defvalue);
            return defvalue;
        }

        // Convert the value to a base 10 integer.
        return strtol(value, nullptr, 10);
    }

    return defvalue;
}

bool FastINIClass::Put_Hex(const char* section, const char* entry, int value)
{
    char buffer[32];

    sprintf(buffer, "%X", (unsigned)value);

    return Put_String(section, entry, buffer);
}

int FastINIClass::Get_Hex(const char* section, const char* entry, int defvalue) const
{
    FastINIEntry* entryptr;

    if (section && entry && (entryptr = Find_Entry(section, entry)) != nullptr && *(entryptr->Get_Name())) {
        sscanf(entryptr->Get_Value(), "%x", (unsigned*)&defvalue);
    }

    return defvalue;
}

bool FastINIClass::Put_String(const char* section, const char* entry, const char* string)
{
    assert(section != nullptr);
    assert(entry != nullptr);

    FastINISection* sectionptr;
    FastINIEntry* entryptr;

    if (section != nullptr && entry != nullptr) {
        if ((sectionptr = Find_Section(section)) == nullptr) {
            DBG_LOG("FastINIClass::Put_String() Creating new section [%s]", section);
            sectionptr = new FastINISection(section);
            SectionList.Add_Tail(sectionptr);
            SectionIndex.Add_Index(CRC(sectionptr->Get_Name()), sectionptr);
        }

        if ((entryptr = sectionptr->Find_Entry(entry)) != nullptr) {
            // TODO needs rewriting, see BMFE or Ren
            if (strcmp(entryptr->Get_Name(), entry) == 0) {
                // Duplicate_CRC_Error(__CURRENT_FUNCTION__, section, entry);
            } else {
                // Duplicate_CRC(__CURRENT_FUNCTION__, section, entry);
            }

            // If we already have the entry, we are replacing it so delete
            sectionptr->EntryIndex.Remove_Index(CRC(entryptr->Get_Name()));

            if (entryptr != nullptr) {
                delete entryptr;
            }
        }

        if (string != nullptr && strlen(string) > 0) {
            if (string[0] != '\0' /*|| KeepBlankEntries*/) {
                assert(strlen(string) < MAX_LINE_LENGTH);

                entryptr = new FastINIEntry(entry, string);
                sectionptr->EntryList.Add_Tail(entryptr);
                sectionptr->EntryIndex.Add_Index(CRC(entryptr->Get_Name()), entryptr);
            }
        }

        return true;
    }

    return false;
}

int FastINIClass::Get_String(const char* section,
                             const char* entry,
                             const char* defvalue,
                             char* buffer,
                             int length) const
{
    FastINIEntry* entryptr;
    const char* value = defvalue;

    assert(section != nullptr);
    assert(entry != nullptr);
    assert(buffer != nullptr);
    assert(length > 0);

    if (buffer != nullptr && length > 0 && section != nullptr && entry != nullptr) {
        if ((entryptr = Find_Entry(section, entry)) == nullptr || (value = entryptr->Get_Value()) == nullptr) {
            if (defvalue == nullptr) {
                buffer[0] = '\0'; // nullify the first byte of char
                return 0;
            }

            value = defvalue;
        }

        // copy string to return result buffer
        strcpy(buffer, value);
        strtrim(buffer);

        return (int)strlen(buffer);
    }

    return 0;
}

bool FastINIClass::Put_Bool(const char* section, const char* entry, bool value)
{

    if (value) {
        return Put_String(section, entry, "yes");
    }

    return Put_String(section, entry, "no");
}

bool const FastINIClass::Get_Bool(const char* section, const char* entry, bool defvalue) const
{
    FastINIEntry* entryptr;
    const char* value;

    assert(section != nullptr);
    assert(entry != nullptr);

    if (section != nullptr && entry != nullptr) {
        if ((entryptr = Find_Entry(section, entry)) != nullptr && entryptr->Get_Name()
            && (value = entryptr->Get_Value()) != nullptr) {
            switch (toupper(value[0])) {
            // 1, true, yes...
            case '1':
            case 'T':
            case 'Y':
                return true;

            // 0, false, no...
            case '0':
            case 'F':
            case 'N':
                return false;

            default:
                DBG_LOG("Invalid boolean entry in FastINIClass::Get_Bool()!");
                return false;
            }
        }
    }

    return defvalue;
}

void FastINIClass::Strip_Comments(char* line)
{
    assert(line != nullptr);

    if (line != nullptr) {
        // fine first instance of ';'
        char* comment = strchr(line, ';');

        if (comment != nullptr) {
            // insert null character at pointer to ';'
            *comment = '\0';

            // trim trailing whitespace leading up to ';'
            strtrim(line);
        }
    }
}

int32_t const FastINIClass::CRC(const char* string)
{
    return Calculate_CRC<CRC32Engine>(string, (int)strlen(string));
}
