//
// Copyright 2020 Electronic Arts Inc.
//
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

#include "iconcach.h"

IconCacheClass::IconCacheClass(void)
{
    IsCached = FALSE;
    SurfaceLost = FALSE;
    DrawFrequency = 0;
    CacheSurface = NULL;
    IconSource = NULL;
}

IconCacheClass::~IconCacheClass(void)
{
}

IconCacheClass CachedIcons[MAX_CACHED_ICONS];

extern "C" {
IconSetType IconSetList[MAX_ICON_SETS];
short IconCacheLookup[MAX_LOOKUP_ENTRIES];
}

int CachedIconsDrawn = 0;   // Counter of number of cache hits
int UnCachedIconsDrawn = 0; // Counter of number of cache misses
BOOL CacheMemoryExhausted;  // Flag set if we have run out of video RAM

void Invalidate_Cached_Icons(void)
{
}
void Restore_Cached_Icons(void)
{
}
void Register_Icon_Set(void* icon_data, BOOL pre_cache){};

//
// Prototypes for assembly language procedures in STMPCACH.ASM
//
extern "C" void __cdecl Clear_Icon_Pointers(void){};
extern "C" void __cdecl Cache_Copy_Icon(void const* icon_ptr, void*, int){};
extern "C" int __cdecl Is_Icon_Cached(void const* icon_data, int icon)
{
    return -1;
};
extern "C" int __cdecl Get_Icon_Index(void* icon_ptr)
{
    return 0;
};
extern "C" int __cdecl Get_Free_Index(void)
{
    return 0;
};
extern "C" BOOL __cdecl Cache_New_Icon(int icon_index, void* icon_ptr)
{
    return -1;
};
extern "C" int __cdecl Get_Free_Cache_Slot(void)
{
    return -1;
}

void IconCacheClass::Draw_It(LPDIRECTDRAWSURFACE dest_surface,
                             int x_pixel,
                             int y_pixel,
                             int window_left,
                             int window_top,
                             int window_width,
                             int window_height)
{
}

extern int CachedIconsDrawn;
extern int UnCachedIconsDrawn;
