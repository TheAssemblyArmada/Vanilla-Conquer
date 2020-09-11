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
#include "vqaver.h"
#include "gitinfo.h"
#include <stdio.h>

#define VQA_LIBNAME "VQALib (VanillaConquer)"

// Version numbers
#define VQA_VERSION_MAJOR    "2"
#define VQA_VERSION_MINOR    "4"
#define VQA_VERSION_REVISION "2"

// Full version strings
#define VQA_VERSION VQA_VERSION_MAJOR "." VQA_VERSION_MINOR "." VQA_VERSION_REVISION

const char* VQA_NameString()
{
    return VQA_LIBNAME;
}

const char* VQA_VersionString()
{
    return VQA_VERSION;
}

const char* VQA_DateTimeString()
{
    static char _date[50];

    if (*_date == '\0') {
        snprintf(_date, sizeof(_date), "%s", GitCommitDate);
    }

    return _date;
}

const char* VQA_IDString()
{
    static char _id[100];

    if (*_id == '\0') {
        snprintf(_id, sizeof(_id), "%s %s (%s)", VQA_LIBNAME, VQA_VERSION, GitCommitDate);
    }

    return _id;
}
