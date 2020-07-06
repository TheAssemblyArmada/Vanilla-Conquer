// 06/06/2020 tomsons26

unsigned int Divide_With_Round(unsigned int num, unsigned int den)
{
    return num / den + (num % den >= (den + 1) >> 1);
}

void Convert_RGB_To_HSV(unsigned int r,
                        unsigned int g,
                        unsigned int b,
                        unsigned int* h,
                        unsigned int* s,
                        unsigned int* v)
{
    /*
    ** Fetch working component values for the color guns.
    */
    int red = Divide_With_Round(255 * r, 63);
    int green = Divide_With_Round(255 * g, 63);
    int blue = Divide_With_Round(255 * b, 63);

    /*
    ** The hue defaults to none. Only if there is a saturation value will the
    ** hue be calculated.
    */
    *h = 0;

    /*
    ** Set the value (brightness) to match the brightest color gun.
    */
    *v = (red > green) ? red : green;
    if (blue > *v)
        *v = blue;

    /*
    ** Determine the amount of true white present in the color. This is the
    ** minimum color gun value. The white component is used to determine
    ** color saturation.
    */
    int white = (red < green) ? red : green;
    if (blue < white)
        white = blue;

    /*
    ** Determine the saturation (intensity) of the color by comparing the
    ** ratio of true white as a component of the overall color. The more
    ** white component, the less saturation.
    */
    *s = 0;
    if (*v != 0) {
        *s = Divide_With_Round((*v - white) * 255, *v);
    }

    /*
    ** If there is any saturation at all, then the hue must be calculated. The
    ** hue is based on a six sided color wheel.
    */
    if (*s != 0) {
        unsigned int tmp = *v - white;
        unsigned int r1 = Divide_With_Round((*v - red) * 255, tmp);
        unsigned int g1 = Divide_With_Round((*v - green) * 255, tmp);
        unsigned int b1 = Divide_With_Round((*v - blue) * 255, tmp);

        // Find effect of second most predominant color.
        // In which section of the hexagon of colors does the color lie?
        if (*v == red) {
            if (white == green) {
                tmp = 5 * 255 + b1;
            } else {
                tmp = 1 * 255 - g1;
            }
        } else {
            if (*v == green) {
                if (white == blue) {
                    tmp = 1 * 255 + r1;
                } else {
                    tmp = 3 * 255 - b1;
                }
            } else {
                if (white == red) {
                    tmp = 3 * 255 + g1;
                } else {
                    tmp = 5 * 255 - r1;
                }
            }
        }

        // Divide by six and round.
        *h = Divide_With_Round(tmp, 6);
    }
}

void Convert_HSV_To_RGB(unsigned int h,
                        unsigned int s,
                        unsigned int v,
                        unsigned int* r,
                        unsigned int* g,
                        unsigned int* b)
{
    unsigned int i;         // Integer part.
    unsigned int f;         // Fractional or remainder part.  f/HSV_BASE gives fraction.
    unsigned int tmp;       // Temporary variable to help with calculations.
    unsigned int values[7]; // Possible rgb values.  Don't use zero.

    h *= 6;
    f = h % 255;

    // Set up possible red, green and blue values.
    values[1] = values[2] = v;

    tmp = Divide_With_Round(s * f, 255);
    values[3] = Divide_With_Round(v * (255 - tmp), 255);

    values[4] = values[5] = Divide_With_Round(v * (255 - s), 255);

    tmp = 255 - Divide_With_Round(s * (255 - f), 255);
    values[6] = Divide_With_Round(v * tmp, 255);

    // This should not be rounded.
    i = h / 255;

    i += (i > 4) ? -4 : 2;
    *r = Divide_With_Round(63 * values[i], 255);

    i += (i > 4) ? -4 : 2;
    *b = Divide_With_Round(63 * values[i], 255);

    i += (i > 4) ? -4 : 2;
    *g = Divide_With_Round(63 * values[i], 255);
}
