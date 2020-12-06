#include "file.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <fnmatch.h>

class Find_File_Data_Posix : public Find_File_Data
{
public:
    Find_File_Data_Posix();
    virtual ~Find_File_Data_Posix();

    virtual const char* GetName() const;
    virtual unsigned long GetTime() const;

    virtual bool FindFirst(const char* fname);
    virtual bool FindNext();
    virtual void Close();

private:
    DIR* Directory;
    struct dirent* DirEntry;
    const char* FileFilter;
    char FullName[PATH_MAX];

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

unsigned long Find_File_Data_Posix::GetTime() const
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

    // split directory and file from the path
    char* fdir = strrchr((char*)fname, '/');
    if (fdir != nullptr) {
        strncat(FullName, fname, (fdir - fname));
        strcat(FullName, "/");
        FileFilter = fdir + 1;
    } else {
        FileFilter = fname;
    }

    Directory = opendir(FullName);
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
