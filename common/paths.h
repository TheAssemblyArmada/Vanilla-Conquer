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

#ifndef COMMON_PATHS_H
#define COMMON_PATHS_H

#include <string>

class PathsClass
{
public:
    PathsClass()
    {
    }

    PathsClass(PathsClass const&) = delete;
    void operator=(PathsClass const&) = delete;

    /**
    * @brief Initialise the paths from an ini file to override normal OS defaults.
    */
    void Init(const char* suffix = nullptr,
              const char* ini_name = nullptr,
              const char* data_name = nullptr,
              const char* cmd_arg = nullptr);

    const char* Program_Path();
    const char* Data_Path();
    const char* User_Path();

    static bool Create_Directory(const char* path);
    static bool Is_Absolute(const char* path);

#ifdef _WIN32
    constexpr static char SEP = '\\';
#else
    constexpr static char SEP = '/';
#endif
private:
    static std::string Argv_Path(const char* cmd_arg);

private:
    std::string Suffix;
    std::string ProgramPath;
    std::string DataPath;
    std::string UserPath;
};

extern PathsClass Paths;

#endif /* COMMON_PATHS_H */
