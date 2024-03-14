#include "common/xordelta.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

constexpr int xor_bytes_needed(int bytes)
{
    return bytes + ((bytes / 63) * 3) + 4;
}

int test_xordelta()
{
    int ret = 0;

// Embed test image data to decode.
#include "testimage.inc"
    char basebuff[image_data_length];
    char xorbuff[xor_bytes_needed(image_data_length)];
    memset(basebuff, 0, sizeof(basebuff));

    // Generate a test xordelta data set against a null buffer.
    int xorbuff_length = Generate_XOR_Delta(xorbuff, image_data, basebuff, image_data_length);

    // Test applying to a raw buffer.
    Apply_XOR_Delta(basebuff, xorbuff);

    if (memcmp(basebuff, image_data, image_data_length) != 0) {
        fprintf(stderr, "Apply_XOR_Delta(basebuff, xorbuff) did not generate the expected result.\n");
        ret = 1;
    }

    // Test applying to a buffer with width and pitch characteristics.
    memset(basebuff, 0, sizeof(basebuff));
    Apply_XOR_Delta_To_Page_Or_Viewport(basebuff, xorbuff, 320, 320, false);

    if (memcmp(basebuff, image_data, image_data_length) != 0) {
        fprintf(stderr,
                "Apply_XOR_Delta_To_Page_Or_Viewport(basebuff, xorbuff, 320, 320, false) did not generate the expected "
                "result.\n");
        ret = 1;
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_xordelta();

    return ret;
}
