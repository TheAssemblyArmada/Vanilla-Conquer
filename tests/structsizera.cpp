#include "redalert/function.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "common/mixfile.h"
#include "common/vqaloader.h"
#include "common/filepcx.h"

#pragma pack(push, 2)
typedef struct
{
    int16_t count;
    int32_t size;
} FileHeader;
#pragma pack(pop)

int main(void)
{
    int ret = 0;

#define check_size(T, n)                                                                                               \
    if (sizeof(T) != n) {                                                                                              \
        fprintf(stderr, "sizeof(" #T "): should be %d, but is %d\n", n, (int)sizeof(T));                               \
        ret |= 1;                                                                                                      \
    }
#undef check_size
#define check_size(T, n)      static_assert(sizeof(T) == n, "sizeof(" #T ") != " #n)
#define check_offset(T, m, n) static_assert(__builtin_offsetof(T, m) == n, "offsetof(" #T "." #m ") != " #n)

    check_size(MFCD::SubBlock, 12);
    check_size(FileHeader, 6);
    check_size(AUDHeaderType, 12);
    check_size(COORDINATE, 4);
    check_size(COORD_COMPOSITE, 4);
    check_size(CELL, 2);
    check_size(CELL_COMPOSITE, 2);
    check_size(TARGET, 4);
    check_size(TARGET_COMPOSITE, 4);
    check_size(Cursor, 10);
    check_size(Shape_Type, 26);
    check_size(ShapeBlock_Type, 2);
    check_size(VQASND1Header, 4);
    check_size(VQASN2JHeader, 6);
    check_size(VQAHeader, 42);
    check_size(SpecialClass, 4);
    check_size(EventClass, 20);
    check_offset(EventClass, Data, 5);
    check_offset(EventClass, Data.MegaMission.Mission, 9);
    check_offset(EventClass, Data.MegaMission.Target, 10);
    check_size(SerialPacketType, 150);
    check_offset(SerialPacketType, ScenarioInfo, 17);
    check_offset(SerialPacketType, ScenarioInfo.MinVersion, 19);
    check_offset(SerialPacketType, ScenarioInfo.Credits, 71);
    check_offset(SerialPacketType, ScenarioInfo.CheatCheck, 79);
    check_offset(SerialPacketType, ScenarioInfo.FileDigest, 118);
    check_size(GlobalPacketType, 141);
    check_offset(GlobalPacketType, GameInfo, 16);
    check_size(IControl_Type, 40);
    check_size(CompHeaderType, 8);
    check_size(RGB, 3);
    check_size(PCX_HEADER, 128);
    check_size(CommHeaderType, 8);
    check_size(IPXAddressClass, 10);
    check_size(GlobalHeaderType, 12);

#undef check_size
#undef check_offset

    return ret;
}
