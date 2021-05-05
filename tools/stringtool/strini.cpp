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
#include "debugstring.h"
#include "readline.h"
#include "strini.h"
#include "miscasm.h"
#include <cassert>
#include <string.h>
#include <strings.h>

#if defined BUILD_WITH_ICU
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/ustring.h>
typedef UChar unichar_t;
#elif defined _WIN32
#include <windows.h>
typedef wchar_t unichar_t;
#endif

/**
 * Overrides load from base ini class to prevent stripping of inline comments.
 */
int StrINIClass::Load(Straw &straw)
{
    char buffer[STRINI_MAX_BUF_SIZE];
    bool end_of_file = false;

    // Read all lines until we find the first section.
    while (!end_of_file) {
        Read_Line(straw, buffer, sizeof(buffer), end_of_file);

        if (end_of_file) {
            DBG_LOG("INIClass::Load() - reached end of file before finding a section");
            return false;
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
        INISection *section = new INISection(buffer);

        if (section == nullptr) {
            DBG_LOG("INIClass::Load() - failed to create section!");

            Clear();

            return false;
        }

        while (!end_of_file) {
            int count = Read_Line(straw, buffer, sizeof(buffer), end_of_file);
            // Check we don't have another section.
            if (buffer[0] == '[' && strchr(buffer, ']')) {
                break;
            }

            char *delimiter = strchr(buffer, '=');

            if (count > 0 && buffer[0] != ';' && buffer[0] != '=') {
                if (delimiter != nullptr) {
                    delimiter[0] = '\0';
                    char *entry = buffer;
                    char *value = delimiter + 1;

                    strtrim(entry);

                    if (entry[0] != '\0') {
                        INIEntry *entryptr = new INIEntry(entry, value);
                        if (!entryptr) {
                            DBG_LOG("Failed to create entry '%s = %s'.", entry, value);

                            delete section;
                            Clear();

                            return false;
                        }

                        // Is this Name, Value or something?
                        CRC(entryptr->Entry);
                        int32_t crc = CRC(entryptr->Entry);

                        if (section->EntryIndex.Is_Present(crc)) {
                            // Duplicate_CRC_Error(__FUNCTION__, section->Section, entryptr->Entry);
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
            int32_t crc = CRC(section->Section);
            SectionIndex.Add_Index(crc, section);
            SectionList.Add_Tail(section);
        }
    }

    return true;
}

bool StrINIClass::Put_Table_String(const char *section, const char *entry, const char *string, const char *lang)
{
    INISection *sectionptr;
    INIEntry *entryptr;

    if (section != nullptr && entry != nullptr) {
        if ((sectionptr = Find_Section(section)) == nullptr) {
            DBG_LOG("INIClass::Put_String() Creating new section [%s]", section);
            sectionptr = new INISection(strdup(section));
            SectionList.Add_Tail(sectionptr);
            SectionIndex.Add_Index(CRC(sectionptr->Section), sectionptr);
        }

        if ((entryptr = sectionptr->Find_Entry(entry)) != nullptr) {
            // If we already have the entry, we are replacing it so delete
            sectionptr->EntryIndex.Remove_Index(CRC(entryptr->Entry));

            if (entryptr != nullptr) {
                delete entryptr;
            }
        }

        if (string != nullptr) {
            // We will create blank entries here unlike the normal string put.
            assert(strlen(string) < STRINI_MAX_BUF_SIZE);
            char *utf8 = Codepage_To_UTF8(string, Get_Lang_Code(lang));
            entryptr = new INIEntry(strdup(entry), strdup(utf8));
            delete[] utf8;
            sectionptr->EntryList.Add_Tail(entryptr);
            sectionptr->EntryIndex.Add_Index(CRC(entryptr->Entry), entryptr);
        }

        return true;
    }

    return false;
}

int StrINIClass::Get_Table_String(
    const char *section, const char *entry, const char *defvalue, char *buffer, int length, const char *lang) const
{
    INIEntry *entryptr;
    const char *value = defvalue;

    if (buffer != nullptr && length > 0 && section != nullptr && entry != nullptr) {
        if ((entryptr = Find_Entry(section, entry)) == nullptr || (value = entryptr->Value) == nullptr) {
            if (defvalue == nullptr) {
                buffer[0] = '\0'; // nullify the first byte of char
                return 0;
            }

            value = defvalue;
        }

        // copy string to return result buffer
        char *codepaged = UTF8_To_Codepage(value, Get_Lang_Code(lang));
        strcpy(buffer, codepaged);
        delete[] codepaged;

        return int(strlen(buffer));
    }

    return 0;
}

/**
 * Helper function to convert a language string to the appropriate code page. Add new languages here.
 */
int StrINIClass::Get_Lang_Code(const char *lang)
{
    // clang-format off
    static const LangMap _LangMap[] = {
        { "eng", 437 },
        { "ger", 437 },
        { "fre", 437 },
        { nullptr, 0 } // Keep list null terminated
    };
    // clang-format on

    if (lang != nullptr) {
        const LangMap *map = _LangMap;

        while (map->lang != nullptr) {
            if (strcasecmp(map->lang, lang) == 0) {
                return map->code;
            }

            ++map;
        }
    }

    return 437;
}

char *StrINIClass::Codepage_To_UTF8(const char *string, int codepage)
{
#ifdef BUILD_WITH_ICU
    int32_t length;
    UErrorCode error = U_ZERO_ERROR;
    UConverter *conv = ucnv_openCCSID(codepage, UCNV_IBM, &error);
    length = ucnv_toUChars(conv, nullptr, 0, string, strlen(string), &error);
    unichar_t *utf16 = new unichar_t[length];

    if (U_SUCCESS(error) && length > 0) {
        ucnv_toUChars(conv, utf16, length, string, strlen(string), &error);

        if (U_FAILURE(error)) {
            delete[] utf16;
            return nullptr;
        }
    }

    ucnv_close(conv);

    u_strToUTF8(nullptr, 0, &length, utf16, -1, &error);
    char *mbyte = new char[length];

    if (U_SUCCESS(error) && length > 0) {
        u_strToUTF8(mbyte, length, nullptr, utf16, -1, &error);

        if (U_FAILURE(error)) {
            delete[] utf16;
            delete[] mbyte;
            return nullptr;
        }
    }

    delete[] utf16;
    return mbyte;
#elif defined _WIN32
    // Convert to uft16 with winapi then to desired codepage.
    int length = MultiByteToWideChar(codepage, 0, string, -1, nullptr, 0);
    unichar_t *utf16 = new unichar_t[length];

    if (length > 0) {
        MultiByteToWideChar(codepage, 0, string, -1, utf16, length);
    }

    length = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, nullptr, 0, nullptr, nullptr);
    char *mbyte = new char[length];

    if (length > 0) {
        WideCharToMultiByte(CP_UTF8, 0, utf16, -1, mbyte, length, nullptr, nullptr);
    }

    delete[] utf16;
    return mbyte;
#else
    return nullptr;
#endif
}

char *StrINIClass::UTF8_To_Codepage(const char *string, int codepage)
{
#ifdef BUILD_WITH_ICU
    int32_t oldlength;
    UErrorCode error = U_ZERO_ERROR;
    u_strFromUTF8(nullptr, 0, &oldlength, string, -1, &error);
    unichar_t *utf16 = new unichar_t[oldlength];

    if (U_SUCCESS(error) && oldlength > 0) {
        u_strFromUTF8(utf16, oldlength, nullptr, string, -1, &error);

        if (U_FAILURE(error)) {
            delete[] utf16;
            return nullptr;
        }
    }

    UConverter *conv = ucnv_openCCSID(codepage, UCNV_IBM, &error);
    int32_t length = ucnv_fromUChars(conv, nullptr, 0, utf16, oldlength, &error);
    char *mbyte = new char[length];

    if (U_SUCCESS(error) && length > 0) {
        ucnv_fromUChars(conv, mbyte, length, utf16, oldlength, &error);

        if (U_FAILURE(error)) {
            delete[] utf16;
            delete[] mbyte;
            return nullptr;
        }
    }

    ucnv_close(conv);
    delete[] utf16;
    return mbyte;
#elif defined _WIN32
    // Convert to uft16 with winapi then to desired codepage.
    int length = MultiByteToWideChar(CP_UTF8, 0, string, -1, nullptr, 0);
    unichar_t *utf16 = new unichar_t[length];

    if (length > 0) {
        MultiByteToWideChar(CP_UTF8, 0, string, -1, utf16, length);
    }

    length = WideCharToMultiByte(codepage, 0, utf16, -1, nullptr, 0, nullptr, nullptr);
    char *mbyte = new char[length];

    if (length > 0) {
        WideCharToMultiByte(codepage, 0, utf16, -1, mbyte, length, nullptr, nullptr);
    }

    delete[] utf16;
    return mbyte;
#else
    return nullptr;
#endif
}
