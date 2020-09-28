#include "common/drawbuff.h"
#include "common/gbuffer.h"

#include <stdint.h>
#include <string.h>
#include <iostream>

// Globals needed to compile GraphicBufferClass.
bool GameInFocus;
int ScreenWidth;
int WindowList[9][9];
extern "C" char* _ShapeBuffer = 0;

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

int test_putpixel()
{
#include "putpixel.inc"
    int ret = 0;
    GraphicBufferClass gb(10, 10);
    gb.Clear();

    if (gb.Lock()) {
        Buffer_Put_Pixel(&gb, 5, 5, 16);

        if (memcmp(gb.Get_Buffer(), test_data, test_data_length) != 0) {
            fprintf(stderr, "Buffer_Put_Pixel(&gb, 5, 5, 16) did not generate the expected result.\n");
            ret = 1;
        }

        gb.Unlock();
    } else {
        fprintf(stderr, "gb.Lock() failed.\n");
        ret = 1;
    }

    return ret;
}

int test_getpixel()
{
#include "putpixel.inc"
    int ret = 0;
    GraphicBufferClass gb(10, 10);
    gb.Clear();
    memcpy(gb.Get_Buffer(), test_data, test_data_length);

    if (gb.Lock()) {
        int pixel = Buffer_Get_Pixel(&gb, 5, 5);

        if (pixel != 16) {
            fprintf(stderr, "Buffer_Get_Pixel(&gb, 5, 5) did not generate the expected result.\n");
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

    ret |= test_putpixel();
    ret |= test_getpixel();

    return ret;
}
