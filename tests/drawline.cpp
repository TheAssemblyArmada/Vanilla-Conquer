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
char* _ShapeBuffer = 0;
WWKeyboardClass* WWKeyboard;

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

int test_drawline()
{
#include "drawline.inc"
    int ret = 0;
    GraphicBufferClass gb(20, 20);
    gb.Clear();

    if (gb.Lock()) {
        // Horizontal lines
        Buffer_Draw_Line(&gb, 5, 5, 10, 5, 16);
        Buffer_Draw_Line(&gb, 5, 10, 10, 10, 16);

        // Vertical lines
        Buffer_Draw_Line(&gb, 3, 5, 3, 10, 16);
        Buffer_Draw_Line(&gb, 13, 5, 13, 10, 16);

        // Test clipping.
        Buffer_Draw_Line(&gb, -5, 2, 25, 10, 16);
        Buffer_Draw_Line(&gb, 2, -5, 10, 25, 16);

        if (memcmp(gb.Get_Buffer(), test_data, test_data_length) != 0) {
            fprintf(stderr, "Buffer_Draw_Line did not generate the expected result.\n");
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

    ret |= test_drawline();

    return ret;
}
