#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include "common/irandom.h"

extern unsigned long RandNumb;

int test_random()
{
    int ret = 0;
    RandNumb = 0x12349876; // Game defaults to this initial value, but randomises it normally.

    unsigned char test_data[] = {139, 61, 111, 84, 109, 56};

    for (int i = 0; i < sizeof(test_data); i++) {
        int value = Random();

        if (value != test_data[i]) {
            fprintf(stderr, "Random() -> %u, expected %u\n", value, test_data[i]);
            ret = 1;
        }
    }

    return ret;
}

int test_random_mask()
{
    struct test_data
    {
        int max_val;
        int mask;
    };

    int ret = 0;

    struct test_data test_data[] = {
        {0, 1},
        {1, 1},
        {1234, 2047},
        {54321, 65535},
        {123456, 131071},
    };

    for (int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        struct test_data* test = &test_data[i];

        int mask = Get_Random_Mask(test->max_val);

        if (mask != test->mask) {
            fprintf(stderr, "Get_Random_Mask(%d) -> %d, expected %d\n", test->max_val, mask, test->mask);
            ret = 1;
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_random();
    ret |= test_random_mask();

    return ret;
}
