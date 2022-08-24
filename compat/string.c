#include "string.h"

#ifndef HAVE_STRTRIM
char* strtrim(char* str)
{
    size_t len = 0;
    char* frontp = str;
    char* endp = NULL;

    if (str == NULL) {
        return NULL;
    }

    if (str[0] == '\0') {
        return str;
    }

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while (isspace((unsigned char)*frontp)) {
        ++frontp;
    }

    if (endp != frontp) {
        while (isspace((unsigned char)*(--endp)) && endp != frontp) {
        }
    }

    if (str + len - 1 != endp) {
        *(endp + 1) = '\0';
    } else if (frontp != str && endp == frontp) {
        *str = '\0';
    }

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if (frontp != str) {
        while (*frontp) {
            *endp++ = *frontp++;
        }

        *endp = '\0';
    }

    return str;
}
#endif

#ifndef HAVE_STRLWR
char* strlwr(char* str)
{
    char* p = str;

    while (*p) {
        *p = tolower((unsigned char)*p);
        p++;
    }

    return str;
}
#endif

#ifndef HAVE_STRUPR
char* strupr(char* str)
{
    char* p = str;

    while (*p) {
        *p = toupper((unsigned char)*p);
        p++;
    }

    return str;
}
#endif

#ifndef HAVE_STRREV
char* strrev(char* str)
{
    int len = strlen(str);

    for (int i = 0; i < len / 2; ++i) {
        char c = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = c;
    }

    return str;
}
#endif

#ifndef HAVE_STRLCAT
size_t strlcat(char* dst, const char* src, size_t dsize)
{
    const char* odst = dst;
    const char* osrc = src;
    size_t n = dsize;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end. */
    while (n-- != 0 && *dst != '\0')
        dst++;
    dlen = dst - odst;
    n = dsize - dlen;

    if (n-- == 0)
        return (dlen + strlen(src));
    while (*src != '\0') {
        if (n != 0) {
            *dst++ = *src;
            n--;
        }
        src++;
    }
    *dst = '\0';

    return (dlen + (src - osrc)); /* count does not include NUL */
}
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char* dst, const char* src, size_t dsize)
{
    const char* osrc = src;
    size_t nleft = dsize;

    /* Copy as many bytes as will fit. */
    if (nleft != 0) {
        while (--nleft != 0) {
            if ((*dst++ = *src++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src. */
    if (nleft == 0) {
        if (dsize != 0)
            *dst = '\0'; /* NUL-terminate dst */
        while (*src++)
            ;
    }

    return (src - osrc - 1); /* count does not include NUL */
}
#endif
