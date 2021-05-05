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
#include "ini2str.h"
#include "gitinfo.h"
#include <cstring>
#include <cstdio>
#include <getopt.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include "utfargs.h"

using std::printf;
using std::strcmp;

void Print_Help()
{
    const char* version = GitTag[0] == 'v' ? GitTag : GitShortSHA1;
    printf("\nVanilla Conquer StringTool %s%s\n\n"
           "Usage:\n"
           "  stringtool [-p] -i <conquer.eng> -o <strings.ini> [-l <eng>] [-b <`>]\n"
           "  stringtool -h | --help\n\n"
           "Options:\n"
           "  -p --pack      Pack an unpacked ini into a string file.\n"
           "                 The default is to unpack a string file to ini.\n"
           "  -i --input     Path to a string or ini file to operate on.\n"
           "  -o --output    Path to a string or ini file to generate.\n"
           "  -b --linebreak Sets the character to treat as a line break replacement.\n"
           "                 Defaults to '`'.\n"
           "  -l --language  Three character language code to flag the encoding to use.\n"
           "  -h --help      Displays this help.\n\n"
           "Supported Languages:\n"
           "  eng    English\n"
           "  fre    French\n"
           "  ger    German\n\n",
           GitUncommittedChanges ? "~" : "",
           version);
}

int main(int argc, char** argv)
{
    static const char table_name[] = "conquer.eng";
    static const char ini_name[] = "strings.ini";
    UtfArgs args(argc, argv);

    int pack = false;
    const char* in = nullptr;
    const char* out = nullptr;
    const char* lang = "eng";

    while (true) {
        static struct option long_options[] = {
            {"pack", no_argument, &pack, 'p'},
            {"input", required_argument, 0, 'i'},
            {"output", required_argument, 0, 'o'},
            {"linebreak", required_argument, 0, 'b'},
            {"language", required_argument, 0, 'l'},
            {"help", no_argument, 0, 'h'},
            {nullptr, no_argument, nullptr, 0},
        };

        int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        int c = getopt_long(args.ArgC, args.ArgV, "ph?i:o:b:l:", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
        case 'p':
            break;
        case 'i':
            in = optarg;
            break;
        case 'o':
            out = optarg;
            break;
        case 'b':
            LineBreak = *optarg;
            break;
        case 'l':
            lang = optarg;
            break;
        case 'h': // fallthrough.
        case '?':
            Print_Help();
            return 0;
        default:
            break;
        }
    }

    if (pack) {
        if (in == nullptr) {
            in = ini_name;
        }

        if (out == nullptr) {
            out = table_name;
        }

        return INI_To_StringTable(in, out, lang);

    } else {
        if (in == nullptr) {
            in = table_name;
        }

        if (out == nullptr) {
            out = ini_name;
        }

        return StringTable_To_INI(in, out, lang);
    }
}
