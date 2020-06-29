#include <windows.h> // for windows types, sigh

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

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_calcx_calcy();

    return ret;
}
