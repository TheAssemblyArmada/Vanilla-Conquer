#include "common/fading.h"

#include <stdint.h>
#include <iostream>

int test_fading()
{
    int ret = 0;

// Embed a test palette and the result of the test ran against the ASM version.
#include "testpal.inc"
#include "faderesult.inc"

    // Build a table fading to black with a fraction of 50% (fraction is between 0 and 255)
    unsigned char fading_table[256] = {0};
    Conquer_Build_Fading_Table(test_palette, fading_table, 0, 128);

    for (int i = 0; i < 256; ++i) {
        if (fading_table[i] != test_result[i]) {
            fprintf(stderr,
                    "Conquer_Build_Fading_Table(test_palette, fading_table, 0, 128) did not generate the expected "
                    "table at position %d -> %d, expected %d.\n",
                    i,
                    fading_table[i],
                    test_result[i]);
            ret = 1;
            break;
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_fading();

    return ret;
}
