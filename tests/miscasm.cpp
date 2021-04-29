#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include "common/miscasm.h"

int test_calcx_calcy()
{
    struct test_data
    {
        int16_t a;
        int16_t b;
        int outx;
        int outy;
    };

    int ret = 0;

    struct test_data test_data[] = {
        {0, 0, 0, 0},
        {1000, 1000, 7812, -7812},
        {3123, -827, 45358, -45358},
        {-2382, -30238, 38422, -38422},
        {-1, -1, 0, 0},
        {127, 304, 301, -301},
    };

    for (int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        struct test_data* test = &test_data[i];

        int outx = calcx(test->a, test->b);
        int outy = calcy(test->a, test->b);

        if (outx != test->outx) {
            fprintf(stderr, "calcx(%d, %d) -> %d, expected %d\n", test->a, test->b, outx, test->outx);
            ret = 1;
        }

        if (outy != test->outy) {
            fprintf(stderr, "calcy(%d, %d) -> %d, expected %d\n", test->a, test->b, outy, test->outy);
            ret = 1;
        }
    }

    return ret;
}

int test_fixed_cardinal()
{
    struct test_data
    {
        int16_t a;
        int16_t b;
        int outx;
        int outy;
    };

    int ret = 0;

    struct test_data test_data[] = {
        {0, 0, 65535, 0},
        {1000, 1000, 256, 3906},
        {3123, -827, 1375201, 65535},
        {-2382, -30238, 0, 65535},
        {-1, -1, 0, 0},
    };

    for (int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        struct test_data* test = &test_data[i];

        int outx = Cardinal_To_Fixed(test->a, test->b);
        int outy = Fixed_To_Cardinal(test->a, test->b);

        if (outx != test->outx) {
            fprintf(stderr, "Cardinal_To_Fixed(%d, %d) -> %d, expected %d\n", test->a, test->b, outx, test->outx);
            ret = 1;
        }

        if (outy != test->outy) {
            fprintf(stderr, "Fixed_To_Cardinal(%d, %d) -> %d, expected %d\n", test->a, test->b, outy, test->outy);
            ret = 1;
        }
    }

    return ret;
}

int test_bitarray()
{
    struct test_data
    {
        int16_t a;
        int16_t b;
        int outx;
        int outy;
    };

    int ret = 0;

    unsigned char test_data[][4] = {
        {255, 255, 255, 255},
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {0, 128, 0, 0},
    };

    int first = First_True_Bit(test_data[2]);

    if (first != 0) {
        fprintf(stderr, "First_True_Bit(test_data[2]) -> %d, expected 0\n", first);
        ret = 1;
    }

    first = First_False_Bit(test_data[2]);

    if (first != 1) {
        fprintf(stderr, "First_False_Bit(test_data[3]) -> %d, expected 1\n", first);
        ret = 1;
    }

    first = First_True_Bit(test_data[3]);

    if (first != 15) {
        fprintf(stderr, "First_True_Bit(test_data[2]) -> %d, expected 15\n", first);
        ret = 1;
    }

    first = First_False_Bit(test_data[3]);

    if (first != 0) {
        fprintf(stderr, "First_False_Bit(test_data[3]) -> %d, expected 0\n", first);
        ret = 1;
    }

    bool get_check = Get_Bit(test_data[0], 1);

    if (!get_check) {
        fprintf(stderr, "Get_Bit(test_data[0], 1) -> false, expected true\n");
        ret = 1;
    }

    get_check = Get_Bit(test_data[1], 1);

    if (get_check) {
        fprintf(stderr, "Get_Bit(test_data[1], 1) -> true, expected false\n");
        ret = 1;
    }

    get_check = Get_Bit(test_data[0], 16);

    if (!get_check) {
        fprintf(stderr, "Get_Bit(test_data[0], 16) -> false, expected true\n");
        ret = 1;
    }

    get_check = Get_Bit(test_data[1], 16);

    if (get_check) {
        fprintf(stderr, "Get_Bit(test_data[1], 16) -> true, expected false\n");
        ret = 1;
    }

    Set_Bit(test_data[0], 5, 0);
    Set_Bit(test_data[0], 30, 0);
    Set_Bit(test_data[1], 5, 1);
    Set_Bit(test_data[1], 30, 1);

    get_check = Get_Bit(test_data[0], 5);

    if (get_check) {
        fprintf(stderr, "Get_Bit(test_data[0], 5) -> true, expected false\n");
        ret = 1;
    }

    get_check = Get_Bit(test_data[0], 30);

    if (get_check) {
        fprintf(stderr, "Get_Bit(test_data[0], 30) -> true, expected false\n");
        ret = 1;
    }

    get_check = Get_Bit(test_data[1], 5);

    if (!get_check) {
        fprintf(stderr, "Get_Bit(test_data[1], 5) -> false, expected true\n");
        ret = 1;
    }

    get_check = Get_Bit(test_data[1], 30);

    if (!get_check) {
        fprintf(stderr, "Get_Bit(test_data[1], 30) -> false, expected true\n");
        ret = 1;
    }

    return ret;
}

int test_swap()
{
    int ret = 0;

    const int expected = 0xefbeadde;
    int actual = Reverse_Long(0xdeadbeef);

    if (actual != expected) {
        fprintf(stderr, "Reverse_Long(0xdeadbeef) -> %08x, expected %08x\n", actual, expected);
        ret = 1;
    }

    return ret;
}

int test_strtrim()
{
    int ret = 0;

    char test_data1[] = "   Lorem ipsum dolor sit amet, consectetur adipiscing elit.   ";
    const char expected1[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
    strtrim(test_data1);

    if (strcmp(test_data1, expected1) != 0) {
        fprintf(stderr, "strtrim(test_data1) -> '%s', expected '%s'\n", test_data1, expected1);
        ret = 1;
    }

    char test_data2[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.   ";
    strtrim(test_data2);

    if (strcmp(test_data2, expected1) != 0) {
        fprintf(stderr, "strtrim(test_data2) -> '%s', expected '%s'\n", test_data2, expected1);
        ret = 1;
    }

    char test_data3[] = "   Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
    strtrim(test_data3);

    if (strcmp(test_data3, expected1) != 0) {
        fprintf(stderr, "strtrim(test_data3) -> '%s', expected '%s'\n", test_data3, expected1);
        ret = 1;
    }

    char test_data4[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
    strtrim(test_data4);

    if (strcmp(test_data4, expected1) != 0) {
        fprintf(stderr, "strtrim(test_data4) -> '%s', expected '%s'\n", test_data4, expected1);
        ret = 1;
    }

    char test_data5[] = "";
    const char expected2[] = "";
    strtrim(test_data5);

    if (strcmp(test_data5, expected2) != 0) {
        fprintf(stderr, "strtrim(test_data5) -> '%s', expected '%s'\n", test_data5, expected2);
        ret = 1;
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_calcx_calcy();
    ret |= test_fixed_cardinal();
    ret |= test_bitarray();
    ret |= test_swap();
    ret |= test_strtrim();

    return ret;
}
