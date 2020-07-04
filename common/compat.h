#pragma once

char* itoa(int value, char *str, int base);
char* ltoa(long value, char *str, int base);

#define __cdecl
#define cdecl
#define far

#include <stdint.h>

typedef uint32_t uint32;
typedef uint64_t uint64;

#define _MAX_FNAME 256
#define _MAX_EXT 8

#define HRESULT long
#define HANDLE intptr_t
#define HWND intptr_t
#define CALLBACK

#define INVALID_HANDLE_VALUE -1

#define VOID void
#define INT int
#define UINT unsigned int
#define LONG long
#define BOOL bool
#define BYTE char
#define WORD short
#define DWORD long

#define TRUE true
#define FALSE false

#define DRIVE_CDROM 1

#define LPCTSTR char*
#define DLGPROC intptr_t

#define stricmp strcasecmp

void _makepath(char*, char*, char*, char*, char*);

struct CRITICAL_SECTION
{
};

int GetDriveTypeA(int);
