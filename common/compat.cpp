#include "compat.h"
#include <stdio.h>
#include <assert.h>

char* itoa(int value, char *str, int base)
{
    assert(base == 10);
    sprintf(str, "%d", value);
    return str;
}

char* itoa(long value, char *str, int base)
{
    assert(base == 10);
    sprintf(str, "%ld", value);
    return str;
}
