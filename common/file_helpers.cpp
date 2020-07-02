#include "file_helpers.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>
#endif

FileSystem::FileSystem()
{
}

FileSystem::~FileSystem()
{
}

FileSystem& FileSystem::Shared()
{
    static FileSystem fileSystem;
    return fileSystem;
}

#ifdef _WIN32
time_t filetime_to_timet(const FILETIME& ft)
{
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
#endif

FileSystem::FileInfos FileSystem::ListFiles(const std::string& mask) const
{
    FileInfos result;

#ifdef _WIN32
    WIN32_FIND_DATAA ff = {0};
    HANDLE rc = FindFirstFileA(mask.c_str(), &ff);
    if (rc != INVALID_HANDLE_VALUE) {
        BOOL next = TRUE;
        while (next) {
            FileInfos::iterator info = result.emplace(result.end());
            info->name = ff.cFileName;
            info->time_write = filetime_to_timet(ff.ftLastWriteTime);
            next = FindNextFileA(rc, &ff);
        }
        FindClose(rc);
    }
#else
    struct dirent* ep = nullptr;
    DIR* dp = opendir("./");
    if (dp != nullptr) {
        while ((ep = readdir(dp)) != nullptr) {
            if (fnmatch(mask.c_str(), ep->d_name, 0) == 0) {
                FileInfos::iterator info = result.emplace(result.end());
                info->name = ep->d_name;
                struct stat st = {0};
                if (stat(ep->d_name, &st) != -1) {
                    info->time_write = (time_t)st.st_mtime;
                } else {
                    info->time_write = 0;
                }
            }
        }
        closedir(dp);
    }
#endif

    return result;
}

bool FileSystem::FileDelete(const std::string& path) const
{
    return (unlink(path.c_str()) == 0);
}
