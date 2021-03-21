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
#define _WIN32_WINNT 0x0600
#include "paths.h"
#include "debugstring.h"
#include "utf.h"
#include <winerror.h>
#include <shlobj.h>

namespace
{
    class FreeCoTaskMemory
    {
        LPWSTR pointer = nullptr;

    public:
        explicit FreeCoTaskMemory(LPWSTR pointer)
            : pointer(pointer){};
        ~FreeCoTaskMemory()
        {
            CoTaskMemFree(pointer);
        }
    };
} // namespace

const char* PathsClass::Program_Path()
{
    if (ProgramPath.empty()) {
        /*
        ** Adapted from https://github.com/gpakosz/whereami
        ** dual licensed under the WTFPL v2 and MIT licenses without any warranty. by Gregory Pakosz (@gpakosz)
        */
        wchar_t buffer1[MAX_PATH];
        wchar_t buffer2[MAX_PATH];
        wchar_t* path = NULL;
        int length = -1;

        while (true) {
            DWORD size;

            size = GetModuleFileNameW(nullptr, buffer1, sizeof(buffer1) / sizeof(buffer1[0]));

            if (size == 0) {
                break;
            } else if (size == (DWORD)(sizeof(buffer1) / sizeof(buffer1[0]))) {
                DWORD size_ = size;
                do {
                    wchar_t* path_ = (wchar_t*)realloc(path, sizeof(wchar_t) * size_ * 2);

                    if (path_ == nullptr) {
                        break;
                    }

                    size_ *= 2;
                    path = path_;
                    size = GetModuleFileNameW(nullptr, path, size_);
                } while (size == size_);

                if (size == size_) {
                    break;
                }
            } else {
                path = buffer1;
            }

            if (!_wfullpath(buffer2, path, MAX_PATH)) {
                break;
            }

            std::string tmp(static_cast<const char*>(UTF16To8(buffer2)));
            ProgramPath = tmp.substr(0, tmp.find_last_of("\\/"));

            break;
        }

        if (path != buffer1) {
            free(path);
        }
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

        DataPath = ProgramPath.substr(0, ProgramPath.find_last_of("\\/")) + SEP + "share";

        if (!Suffix.empty()) {
            DataPath += SEP + Suffix;
        }
    }

    return DataPath.c_str();
}

const char* PathsClass::User_Path()
{
    if (UserPath.empty()) {
        wchar_t path[MAX_PATH];
        HRESULT hr;
        hr = SHGetFolderPathW(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, path);

        if (!SUCCEEDED(hr)) {
            DBG_WARN("Failed to retrieve FOLDERID_RoamingAppData for PathsClass::User_Path()");
        }

        UserPath = static_cast<const char*>(UTF16To8(path));
        UserPath += "\\Vanilla-Conquer";

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
        pos = temp.find_first_of("\\/", pos + 1);
        if (CreateDirectoryW(UTF8To16(temp.substr(0, pos).c_str()), nullptr) == FALSE) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                ret = false;
                break;
            }
        }
    } while (pos != std::string::npos);

    return ret;
}

bool PathsClass::Is_Absolute(const char* path)
{
    if (strlen(path) < 2) {
        return false;
    }

    return path != nullptr && (path[1] == ':' || (path[0] == '\\' && path[1] == '\\'));
}

std::string PathsClass::Argv_Path(const char* cmd_arg)
{
    wchar_t base_buff[MAX_PATH];
    wchar_t* buff = base_buff;
    unsigned len = GetFullPathNameW(UTF8To16(cmd_arg), MAX_PATH, buff, nullptr);
    std::string ret;

    // If we have a path longer than the standard max path, allocate a buffer of the correct size.
    if (len >= MAX_PATH) {
        buff = new wchar_t[len];
        len = GetFullPathNameW(UTF8To16(cmd_arg), len, buff, nullptr);

        if (len > 0) {
            ret = UTF16To8(buff);
        }

        delete[] buff;
    } else if (len > 0) {
        ret = UTF16To8(buff);
    }

    return ret;
}