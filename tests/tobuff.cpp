#include "common/drawmisc.h"
#include "common/gbuffer.h"
#include "common/wwkeyboard.h"

#include <stdint.h>
#include <string.h>
#include <iostream>

// Globals needed to compile GraphicBufferClass.
bool GameInFocus;
int ScreenWidth;
int WindowList[9][9];
char* _ShapeBuffer = 0;
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

long Read_File(int, void*, unsigned long)
{
    return 0;
}

constexpr int xor_bytes_needed(int bytes)
{
    return bytes + ((bytes / 63) * 3) + 4;
}

int test_tobuff()
{
#include "fatpixeldata.inc"
    int ret = 0;
    GraphicBufferClass gb(20, 20);
    gb.Clear();
    uint8_t buff[20 * 20];

    if (gb.Lock()) {
        // Test assumes Fat_Put_Pixel test passes, otherwise both fail.
        Fat_Put_Pixel(10, 10, 5, 5, gb);
        Buffer_To_Buffer(&gb, 0, 0, 20, 20, buff, sizeof(buff));

        if (memcmp(buff, test_data, test_data_length) != 0) {
            fprintf(stderr,
                    "Buffer_To_Buffer(&gb, 0, 0, 20, 20, buff, sizeof(buff)) did not generate the expected result.\n");
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

    ret |= test_tobuff();

    return ret;
}
