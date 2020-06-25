#include "common/face.h"

#include <stdint.h>
#include <iostream>

int test_facings()
{
    struct test_data
    {
        int l;
        int t;
        int r;
        int b;
        int out256;
        int out8;
    };

    int ret = 0;

    struct test_data test_data[] = {
        {0, 0, 0, 0, 255, 32},
        {5, 5, 20, 20, 95, 96},
        {-5, -5, 20, 20, 95, 96},
        {5, 5, -20, -20, 223, 224},
        {1000, 5, 20, 20, 191, 192},
        {5, 1000, 20, 20, 0, 0},
        {1000, 5, 130, 20, 191, 192},
        {5, 5, 5, 9, 127, 128},
        {5, 5, 9, 5, 63, 64},
        {5, 9, 5, 5, 0, 0},
        {9, 5, 5, 5, 192, 192},
        {50, 50, 50, 9, 0, 0},
        {50, 50, 9, 50, 192, 192},
        {50, 9, 50, 50, 127, 128},
        {9, 50, 50, 50, 63, 64},
    };

    for (int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        struct test_data* test = &test_data[i];

        int out256 = Desired_Facing256(test->l, test->t, test->r, test->b);
        int out8 = Desired_Facing8(test->l, test->t, test->r, test->b);

        if (out256 != test->out256) {
            fprintf(stderr,
                    "Desired_Facing256(%d, %d, %d, %d) -> %d, expected %d\n",
                    test->l,
                    test->t,
                    test->r,
                    test->b,
                    out256,
                    test->out256);
            ret = 1;
        }

        if (out8 != test->out8) {
            fprintf(stderr,
                    "Desired_Facing8(%d, %d, %d, %d) -> %d, expected %d\n",
                    test->l,
                    test->t,
                    test->r,
                    test->b,
                    out8,
                    test->out8);
            ret = 1;
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_facings();

    return ret;
}
