#include "file.h"

bool Find_First(const char* fname, unsigned int mode, Find_File_Data** ffblk)
{
    if (ffblk == nullptr) {
        return false;
    }
    *ffblk = nullptr;

    *ffblk = Find_File_Data::CreateFindData();
    if ((*ffblk)->FindFirst(fname)) {
        return true;
    }

    delete *ffblk;
    *ffblk = nullptr;

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
