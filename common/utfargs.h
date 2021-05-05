// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef UTFARGS_H
#define UTFARGS_H

class UtfArgs
{
public:
    UtfArgs(int argc, char** argv)
        : ArgC(argc)
        , ArgV(argv)
    {
#ifndef REMASTER_BUILD
#ifdef _WIN32
        ArgV = CommandLineToArgvU(GetCommandLineW(), &ArgC);
#endif
#endif
    }

    int ArgC;
    char** ArgV;

private:
#ifndef REMASTER_BUILD
#ifdef _WIN32
    // Taken from https://github.com/thpatch/win32_utf8/blob/master/src/shell32_dll.c
    // Get the command line as UTF-8 as it would be on other platforms.
    static char** CommandLineToArgvU(LPCWSTR lpCmdLine, int* pNumArgs)
    {
        int cmd_line_pos; // Array "index" of the actual command line string
        // int lpCmdLine_len = wcslen(lpCmdLine) + 1;
        int lpCmdLine_len = WideCharToMultiByte(CP_UTF8, 0, lpCmdLine, -1, NULL, 0, NULL, NULL) + 1;
        char** argv_u;

        wchar_t** argv_w = CommandLineToArgvW(lpCmdLine, pNumArgs);

        if (!argv_w) {
            return NULL;
        }

        cmd_line_pos = *pNumArgs + 1;

        // argv is indeed terminated with an additional sentinel NULL pointer.
        argv_u = (char**)LocalAlloc(LMEM_FIXED, cmd_line_pos * sizeof(char*) + lpCmdLine_len);

        if (argv_u) {
            int i;
            char* cur_arg_u = (char*)&argv_u[cmd_line_pos];

            for (i = 0; i < *pNumArgs; i++) {
                size_t cur_arg_u_len;
                int conv_len;
                argv_u[i] = cur_arg_u;
                conv_len = WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1, cur_arg_u, lpCmdLine_len, NULL, NULL);

                cur_arg_u_len = argv_w[i] != NULL ? conv_len : conv_len + 1;
                cur_arg_u += cur_arg_u_len;
                lpCmdLine_len -= (int)cur_arg_u_len;
            }

            argv_u[i] = NULL;
        }

        LocalFree(argv_w);

        return argv_u;
    }
#endif
#endif
};

#endif
