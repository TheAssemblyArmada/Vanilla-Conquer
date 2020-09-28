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
#include "drawbuff.h"
#include "gbuffer.h"
#include <string.h>

#ifdef NOASM

int Buffer_Get_Pixel(void* thisptr, int x, int y)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);

    if ((unsigned)x < (unsigned)vp.Get_Width() && (unsigned)y < (unsigned)vp.Get_Height()) {
        return *(reinterpret_cast<unsigned char*>(vp.Get_Offset()) + x
                 + y * (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width()));
    }

    return 0;
}

void Buffer_Put_Pixel(void* thisptr, int x, int y, unsigned char color)
{
    GraphicViewPortClass& vp = *static_cast<GraphicViewPortClass*>(thisptr);

    if ((unsigned)x < (unsigned)vp.Get_Width() && (unsigned)y < (unsigned)vp.Get_Height()) {
        *(reinterpret_cast<unsigned char*>(vp.Get_Offset()) + x
          + y * (vp.Get_Pitch() + vp.Get_XAdd() + vp.Get_Width())) = color;
    }
}

#endif
