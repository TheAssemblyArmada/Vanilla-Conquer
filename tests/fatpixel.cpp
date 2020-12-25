#include "common/drawmisc.h"
#include "common/gbuffer.h"

#include <stdint.h>
#include <string.h>
#include <iostream>

// Globals needed to compile GraphicBufferClass.
bool GameInFocus;
int ScreenWidth;
int WindowList[9][9];
char* _ShapeBuffer = 0;

int Open_File(char const*, int)
{
    return 0;
}

void Close_File(int)
{
}

long Read_File(int, void*, unsigned long)
{
    return 0;
}

constexpr int xor_bytes_needed(int bytes)
{
    return bytes + ((bytes / 63) * 3) + 4;
}

int test_fatpixel()
{
#include "fatpixeldata.inc"
    int ret = 0;
    GraphicBufferClass gb(20, 20);
    gb.Clear();

    if (gb.Lock()) {
        Fat_Put_Pixel(10, 10, 5, 5, gb);

        if (memcmp(gb.Get_Buffer(), test_data, test_data_length) != 0) {
            fprintf(stderr, "Fat_Put_Pixel(10, 10, 5, 5, gb) did not generate the expected result.\n");
            ret = 1;
        }

        gb.Unlock();
    } else {
        fprintf(stderr, "gb.Lock() failed.\n");
        ret = 1;
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_fatpixel();

    return ret;
}
