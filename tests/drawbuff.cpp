#include "common/drawline.h"
#include "common/gbuffer.h"
#include "common/wwkeyboard.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

// Globals needed to compile GraphicBufferClass.
bool GameInFocus;
int ScreenWidth;
int WindowList[9][9];
WWKeyboardClass* Keyboard;

void Process_Network()
{
}

void Focus_Restore()
{
}

void Focus_Loss()
{
}

int Open_File(char const*, int)
{
    return 0;
}

void Close_File(int)
{
}

int Read_File(int, void*, unsigned int)
{
    return 0;
}

#include "testimage.inc"

int test_clear()
{
    int ret = 0;
    GraphicBufferClass gb(10, 10);

    if (gb.Lock()) {
        gb.Clear(255);
        unsigned char* buff = static_cast<unsigned char*>(gb.Get_Buffer());

        for (int i = 0; i < 100; ++i) {
            if (buff[i] != 255) {
                fprintf(stderr, "Buffer_Clear(&gb, 255) did not generate the expected result.\n");
                ret = 1;
            }
        }

        gb.Unlock();
    } else {
        fprintf(stderr, "gb.Lock() failed.\n");
        ret = 1;
    }

    return ret;
}

int test_fill()
{
#include "filltest.inc"
    int ret = 0;
    GraphicBufferClass gb(10, 10);
    gb.Clear();

    if (gb.Lock()) {
        gb.Fill_Rect(3, 3, 7, 7, 15);

        if (memcmp(gb.Get_Buffer(), test_data, test_data_length) != 0) {
            fprintf(stderr, "Buffer_Fill_Rect(&gb, 3, 3, 7, 7, 15) did not generate the expected result.\n");
            ret = 1;
        }

        gb.Unlock();
    } else {
        fprintf(stderr, "gb.Lock() failed.\n");
        ret = 1;
    }

    return ret;
}

int test_frombuff()
{
#include "frombufftest.inc"
    int ret = 0;
    GraphicBufferClass gb(10, 10);
    gb.Clear();

    if (gb.Lock()) {
        Buffer_To_Page(1, 1, 8, 8, (void*)image_data, gb);

        if (memcmp(gb.Get_Buffer(), test_data, test_data_length) != 0) {
            fprintf(
                stderr,
                "Buffer_To_Page(1, 1, 8, 8, (void *)image_data, (void *)&gb) did not generate the expected result.\n");
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

    ret |= test_clear();
    ret |= test_fill();
    ret |= test_frombuff();

    return ret;
}
