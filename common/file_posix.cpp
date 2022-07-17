#include "file.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <fnmatch.h>

#ifdef VITA
// TODO More definitions to move to a compat lib that tests for the symbols?
#define PATH_MAX     256
#define FNM_CASEFOLD 0
#endif

class Find_File_Data_Posix : public Find_File_Data
{
public:
    Find_File_Data_Posix();
    virtual ~Find_File_Data_Posix();

    virtual const char* GetName() const;
    virtual unsigned int GetTime() const;

    virtual bool FindFirst(const char* fname);
    virtual bool FindNext();
    virtual void Close();

private:
    DIR* Directory;
    struct dirent* DirEntry;
    const char* FileFilter;
    char FullName[PATH_MAX];
    char DirName[PATH_MAX];

    bool FindNextWithFilter();
};

Find_File_Data_Posix::Find_File_Data_Posix()
    : Directory(nullptr)
    , DirEntry(nullptr)
{
}

Find_File_Data_Posix::~Find_File_Data_Posix()
{
    Close();
}

const char* Find_File_Data_Posix::GetName() const
{
    if (DirEntry == nullptr) {
        return nullptr;
    }
    return FullName;
}

unsigned int Find_File_Data_Posix::GetTime() const
{
    if (DirEntry == nullptr) {
        return 0;
    }
    struct stat buf = {0};
    if (stat(DirEntry->d_name, &buf) != 0) {
        return false;
    }
    return buf.st_mtime;
}

bool Find_File_Data_Posix::FindNextWithFilter()
{
    while (true) {
        DirEntry = readdir(Directory);
        if (DirEntry == nullptr) {
            return false;
        }
        if (fnmatch(FileFilter, DirEntry->d_name, FNM_PATHNAME | FNM_CASEFOLD) == 0) {
            strcpy(FullName, DirName);
            strcat(FullName, DirEntry->d_name);
            break;
        }
    }
    return true;
}

bool Find_File_Data_Posix::FindFirst(const char* fname)
{
    Close();
    FullName[0] = '\0';
    DirName[0] = '\0';

    // split directory and file from the path
    char* fdir = strrchr((char*)fname, '/');
    if (fdir != nullptr) {
        strncat(DirName, fname, (fdir - fname + 1));
        FileFilter = fdir + 1;
        Directory = opendir(DirName);
    } else {
        FileFilter = fname;
        Directory = opendir(".");
    }

    if (Directory == nullptr) {
        return false;
    }

    return FindNextWithFilter();
}

bool Find_File_Data_Posix::FindNext()
{
    if (Directory == nullptr) {
        return false;
    }
    return FindNextWithFilter();
}

void Find_File_Data_Posix::Close()
{
    if (Directory != nullptr) {
        closedir(Directory);
        Directory = nullptr;
    }
}

Find_File_Data* Find_File_Data::CreateFindData()
{
    return new Find_File_Data_Posix();
}

#ifdef VITA
#include <algorithm>
#include <string>
#include <vector>

#define PATH_MAX 256

std::string StringLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// fnmatch is missing from vita newlib, this is here until otherwise.
int fnmatch(const char* pattern, const char* string, int flags)
{
    //massive hackjob..
    std::string filename = string;
    std::string filter = pattern;
    filename = StringLower(filename);
    filter = StringLower(filter);
    std::vector<std::string> filter_split;
    std::string delimiter = "*";
    bool found = true;

    size_t pos = 0;
    std::string token;
    while ((pos = filter.find(delimiter)) != std::string::npos) {
        token = filter.substr(0, pos);
        filter_split.push_back(token);
        filter.erase(0, pos + delimiter.length());
    }

    if (!filter_split.empty()) {
        for (int i = 0; i < filter_split.size(); ++i) {
            if (filename.find(filter_split[i]) == std::string::npos) {
                found = false;
                break;
            }
        }
    } else {
        found = filename.find(filter) != std::string::npos;
    }

    return found ? 0 : 1;
}
#endif
