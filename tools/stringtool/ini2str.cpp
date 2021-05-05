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
#include "crc.h"
#include "dipthong.h"
#include "xpipe.h"
#include "xstraw.h"
#include "endianness.h"
#include "rawfile.h"
#include "strini.h"

char LineBreak = '`';
const char *Language = "eng";
const char SectionName[] = "Strings";

// defined in strgen.cpp
void Print_Hint()
{
    printf("Use 'stringtool -h | --help' for correct usage.\n");
}

int INI_To_StringTable(const char *ini_name, const char *table_name, const char *lang)
{
    char line[2048];

    RawFileClass inifc(ini_name);
    RawFileClass strfc;
    StrINIClass strini;

    if (!inifc .Is_Available() || !strini.INIClass::Load(inifc)) {
        printf("ERROR: Could not load ini file '%s'.\n", ini_name);
        Print_Hint();

        return 1;
    }

    // Check if there is a section named the same as our file, if not, return.
    if (strini.Is_Present(SectionName)) {
        printf("ERROR: Cannot find [%s] section to convert to table.\n", SectionName);
        Print_Hint();

        return 1;
    }

    int str_count = strini.Entry_Count(SectionName);
    int headerpos = 0;
    uint16_t datapos = (str_count + 1) * 2;
    uint16_t header = 0;
    strfc.Open(table_name, WRITE);

    for (int i = 0; i < str_count; ++i) {
        // Seek to and write the header info.
        header = htole16(datapos);
        strfc.Seek(headerpos, SEEK_SET);
        strfc.Write(&header, sizeof(header));
        headerpos += 2;

        // Retrieve string and reformat for string table.
        strini.Get_Table_String(SectionName, strini.Get_Entry(SectionName, i), "", line, 2048);

        // Convert line breaks back.
        char *tmp = line;
        while (*tmp) {
            if (*tmp == LineBreak) {
                *tmp = '\r';
            }

            ++tmp;
        }

        // Seek to data pos and write the string.
        strfc.Seek(datapos, SEEK_SET);
        strfc.Write(line, int(strlen(line)) + 1);
        datapos += int(strlen(line)) + 1;
    }
    
    // Write the last null entry to the file.
    line[0] = '\0';
    strfc.Write(line, int(strlen(line)) + 1);
    header = htole16(datapos);
    strfc.Seek(headerpos, SEEK_SET);
    strfc.Write(&header, sizeof(header));
    strfc.Close();

    return 0;
}

int StringTable_To_INI(const char *table_name, const char *ini_name, const char *lang)
{
    // Check we have the specified string file available and if so load it.
    RawFileClass strfc(table_name);

    if (!strfc.Is_Available()) {
        printf("ERROR: Cannot find string table file '%s'.\n", table_name);
        Print_Hint();

        return 1;
    }

    char *stringfile = new char[strfc.Size()];
    strfc.Read(stringfile, strfc.Size());
    remove(ini_name);

    // Prepare the string file and if it exists already, load it.
    StrINIClass strini;

    // Check if there is a section named strings, if so, delete it.
    if (strini.Is_Present(SectionName)) {
        strini.Clear(SectionName);
    }

    // Get the string count and start adding them to the ini.
    // -1 because last entry represents end of file and is always a null byte.
    int stringcount = le16toh(*reinterpret_cast<uint16_t *>(stringfile)) / 2 - 1;
    char entry[32];

    for (int i = 0; i < stringcount; ++i) {
        sprintf(entry, "%d", i);
        // we cast away the const here as we are going to modify the string table
        // in memory. In game we wouldn't want this to happen, but we allocated
        // the space for the table so its fine here.
        char *string = const_cast<char *>(Extract_String(stringfile, i));
        char *tmp = string;

        // Walk the string and replace the line breaks with an ini friendly char
        while (*tmp != '\0') {
            if (*tmp == '\r') {
                *tmp = LineBreak;
            }

            ++tmp;
        }

        strini.Put_Table_String(SectionName, entry, string);
    }

    // Save the ini file and cleanup.
    RawFileClass inifc(ini_name);
    strini.INIClass::Save(inifc);
    delete[] stringfile;
    return 0;
}
