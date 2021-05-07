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
#ifndef CRC32_H
#define CRC32_H

#include <stdlib.h>
#include <stdint.h>

class CRC32Engine
{
public:
    CRC32Engine(int32_t initial = 0)
        : CRC(initial)
        , Index(0)
    {
        StagingBuffer.Composite = 0;
    }

    int32_t operator()(const void* buffer, unsigned length = 0);
    operator int32_t()
    {
        return Value();
    }

private:
    void operator()(char datnum);
    int32_t Value();
    bool Buffer_Needs_Data()
    {
        return Index != 0;
    }

private:
    int32_t CRC;
    int Index;

    union
    {
        int32_t Composite;
        int8_t Buffer[4];
    } StagingBuffer;
};

#endif