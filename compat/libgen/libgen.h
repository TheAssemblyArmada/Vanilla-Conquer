#ifndef MINIPOSIX_LIBGEN_H
#define MINIPOSIX_LIBGEN_H

#ifndef _WIN32

#pragma message("this libgen.h implementation is for Windows only!")

#else

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char* basename(char*);
char* dirname(char*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WIN32 */

#endif /* MINIPOSIX_LIBGEN_H */
