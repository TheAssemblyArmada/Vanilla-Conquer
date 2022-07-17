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

#include "paths.h"

#include <cstring>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>

#define PATH_MAX 256

static std::string VitaGamePath;

const char* PathsClass::Program_Path()
{
    if (ProgramPath.empty()) {
        ProgramPath = VitaGamePath;
    }

    return ProgramPath.c_str();
}

const char* PathsClass::Data_Path()
{
    if (DataPath.empty()) {
        if (ProgramPath.empty()) {
            // Init the program path first if it hasn't been done already.
            Program_Path();
        }

        DataPath = VitaGamePath;

        if (!Suffix.empty()) {
            DataPath += SEP + Suffix;
        }
    }

    return DataPath.c_str();
}

const char* PathsClass::User_Path()
{
    if (UserPath.empty()) {
        UserPath = VitaGamePath;

        if (!Suffix.empty()) {
            UserPath += SEP + Suffix;
        }

        Create_Directory(UserPath.c_str());
    }

    return UserPath.c_str();
}

bool PathsClass::Create_Directory(const char* dirname)
{
    bool ret = true;

    if (dirname == nullptr) {
        return ret;
    }

    std::string temp = dirname;
    size_t pos = 0;
    do {
        pos = temp.find_first_of("/", pos + 1);
        sceIoMkdir(temp.substr(0, pos).c_str(), 0700);
    } while (pos != std::string::npos);

    return ret;
}

bool PathsClass::Is_Absolute(const char* path)
{
    return path != nullptr && path[0] == 'u' && path[1] == 'x' && path[2] == '0';
}

std::string PathsClass::Concatenate_Paths(const char* path1, const char* path2)
{
    return std::string(path1) + SEP + path2;
}

std::string PathsClass::Argv_Path(const char* cmd_arg)
{
    VitaGamePath = cmd_arg;
    return VitaGamePath;
}
