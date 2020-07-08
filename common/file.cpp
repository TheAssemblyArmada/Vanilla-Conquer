#include "file.h"

#include <windows.h>
#include <io.h>

class Find_File_Data_Win : public Find_File_Data
{
public:
    Find_File_Data_Win();
    virtual ~Find_File_Data_Win();

    virtual const char* GetName() const;
    virtual unsigned long GetTime() const;

    virtual bool FindFirst(const char* fname);
    virtual bool FindNext();
    virtual void Close();

private:
    HANDLE FindHandle;
    WIN32_FIND_DATAA FindData;
};

Find_File_Data_Win::Find_File_Data_Win()
    : FindHandle(INVALID_HANDLE_VALUE)
    , FindData({0})
{
}

Find_File_Data_Win::~Find_File_Data_Win()
{
    Close();
}

const char* Find_File_Data_Win::GetName() const
{
    return FindData.cFileName;
}

unsigned long Find_File_Data_Win::GetTime() const
{
    ULARGE_INTEGER ull;
    ull.LowPart = FindData.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = FindData.ftLastWriteTime.dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

bool Find_File_Data_Win::FindFirst(const char* fname)
{
    FindHandle = FindFirstFileA(fname, &FindData);
    return (FindHandle != INVALID_HANDLE_VALUE);
}

bool Find_File_Data_Win::FindNext()
{
    if (FindHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    return (FindNextFileA(FindHandle, &FindData) != FALSE);
}

void Find_File_Data_Win::Close()
{
    if (FindHandle != INVALID_HANDLE_VALUE) {
        FindClose(FindHandle);
        FindHandle = INVALID_HANDLE_VALUE;
    }
}

bool Find_First(const char* fname, unsigned int mode, Find_File_Data** ffblk)
{
    if (ffblk == nullptr) {
        return false;
    }
    *ffblk = nullptr;

    Find_File_Data_Win* ff_win = new Find_File_Data_Win();
    if (ff_win->FindFirst(fname)) {
        *ffblk = ff_win;
        return true;
    }

    delete ff_win;

    return false;
}

bool Find_Next(Find_File_Data* ffblk)
{
    if (ffblk == nullptr) {
        return false;
    }

    return ffblk->FindNext();
}

void Find_Close(Find_File_Data* ffblk)
{
    if (ffblk != nullptr) {
        delete ffblk;
    }
}
