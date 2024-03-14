#include "common/lcw.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

int test_lcw()
{
    int ret = 0;

// Embed test image data to compress/decompress.
#include "testimage.inc"
    char lcwbuff[image_data_length + (image_data_length / 128 + 1)];
    char decompbuff[image_data_length];

    LCW_Comp(image_data, lcwbuff, image_data_length);
    LCW_Uncompress(lcwbuff, decompbuff, sizeof(decompbuff));

    // This works on the assumption that the LCW_Uncompress function is reliable since its original code.
    if (memcmp(decompbuff, image_data, sizeof(decompbuff)) != 0) {
        fprintf(stderr,
                "LCW_Comp(image_data, lcwbuff, image_data_length) did not generate the expected round trip data.\n");
        ret = 1;
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_lcw();

    return ret;
}
