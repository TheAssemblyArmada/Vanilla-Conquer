#include "common/rect.h"

#include <cstdint>
#include <iostream>

int test_rect()
{
    struct test_data
    {
        Rect rect;
        int width;
        int height;
        int outClip;
        Rect outClipRect;
        int outConfine;
        Rect outConfineRect;
    };

    int ret = 0;

    struct test_data test_data[] = {
        {{-5, -5, 4, 4}, 10, 10, -1, {-5, -5, 4, 4}, 1, {0, 0, 4, 4}},
        {{15, 15, 4, 4}, 10, 10, -1, {15, 15, 4, 4}, 1, {6, 6, 4, 4}},
        {{-5, -5, 10, 10}, 10, 10, 1, {0, 0, 5, 5}, 1, {0, 0, 10, 10}},
        {{5, 5, 10, 10}, 10, 10, 1, {5, 5, 5, 5}, 1, {0, 0, 10, 10}},
        {{0, 0, 10, 10}, 10, 10, 0, {0, 0, 10, 10}, 1, {0, 0, 10, 10}},
        {{-5, 0, 10, 10}, 10, 10, 1, {0, 0, 5, 10}, 1, {0, 0, 10, 10}},
        {{0, -5, 10, 10}, 10, 10, 1, {0, 0, 10, 5}, 1, {0, 0, 10, 10}},
        {{2, 2, 5, 5}, 10, 10, 0, {2, 2, 5, 5}, 0, {2, 2, 5, 5}},
        {{-2, -2, 15, 15}, 10, 10, 1, {0, 0, 10, 10}, 1, {0, 0, 15, 15}},
    };

    for (int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        struct test_data* test = &test_data[i];

        Rect outClipRect = test->rect;
        int outClip = Clip_Rect(
            &outClipRect.X, &outClipRect.Y, &outClipRect.Width, &outClipRect.Height, test->width, test->height);
        Rect outConfineRect = test->rect;
        int outConfine = Confine_Rect(&outConfineRect.X,
                                      &outConfineRect.Y,
                                      outConfineRect.Width,
                                      outConfineRect.Height,
                                      test->width,
                                      test->height);

        if ((outClip != test->outClip) || (outClipRect != test->outClipRect)) {
            fprintf(stderr,
                    "Clip_Rect(%d, %d, %d, %d, %d, %d) -> %d (%d, %d, %d, %d), expected %d (%d, %d, %d, %d)\n",
                    test->rect.X,
                    test->rect.Y,
                    test->rect.Width,
                    test->rect.Height,
                    test->width,
                    test->height,
                    outClip,
                    outClipRect.X,
                    outClipRect.Y,
                    outClipRect.Width,
                    outClipRect.Height,
                    test->outClip,
                    test->outClipRect.X,
                    test->outClipRect.Y,
                    test->outClipRect.Width,
                    test->outClipRect.Height);
            ret = 1;
        }

        if ((outConfine != test->outConfine) || (outConfineRect != test->outConfineRect)) {
            fprintf(stderr,
                    "Confine_Rect(%d, %d, %d, %d, %d, %d) -> %d (%d, %d, %d, %d), expected %d (%d, %d, %d, %d)\n",
                    test->rect.X,
                    test->rect.Y,
                    test->rect.Width,
                    test->rect.Height,
                    test->width,
                    test->height,
                    outConfine,
                    outConfineRect.X,
                    outConfineRect.Y,
                    outConfineRect.Width,
                    outConfineRect.Height,
                    test->outConfine,
                    test->outConfineRect.X,
                    test->outConfineRect.Y,
                    test->outConfineRect.Width,
                    test->outConfineRect.Height);
            ret = 1;
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;

    ret |= test_rect();

    return ret;
}
