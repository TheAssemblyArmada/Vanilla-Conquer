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
#ifndef MIXNAMEDB_H
#define MIXNAMEDB_H

#include <map>
#include <string>

class FileClass;

class MixNameDatabase
{
public:
    enum HashMethod
    {
        HASH_RACRC,
        HASH_CRC32,
        HASH_COUNT,
        HASH_ANY = HASH_COUNT,
    };

    struct NameEntry
    {
        std::string file_name;
        std::string file_desc;
    };

    struct DataEntry
    {
        std::string file_name;
        std::string file_desc;
        int32_t ra_crc;
        int32_t ts_crc;
    };

    struct DefaultDataEntry
    {
        const char* file_name;
        const char* file_desc;
        int32_t ra_crc;
        int32_t ts_crc;
    };

    MixNameDatabase()
        : m_isMapDirty(true)
    {
    }

    ~MixNameDatabase()
    {
        Save_To_Ini();
    } // Write the file to the configured file name.

    bool Read_From_Ini(const char* ini_file);
    void Read_Internal();
    const NameEntry& Get_Entry(int32_t hash, HashMethod crc_type = HASH_ANY);
    bool Add_Entry(const char* file_name, const char* comment, HashMethod crc_type = HASH_ANY);
    void Save_To_Ini(const char* ini_file = nullptr);

private:
    void Regenerate_Hash_Maps();
    static void Get_String(FileClass& fc, std::string& dst);

private:
    std::map<int, NameEntry> m_hashMaps[HASH_COUNT];
    std::map<std::string, DataEntry> m_nameMap;
    std::string m_saveName;
    bool m_isMapDirty;

    static const DefaultDataEntry s_defaultDB[];
};

#endif
