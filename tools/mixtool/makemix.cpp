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
#include "crc.h"
#include "crc32.h"
#include "paths.h"
#include "ini.h"
#include "mixcreate.h"
#include "mixnamedb.h"
#include "mp.h"
#include "pk.h"
#include "ramfile.h"
#include "rawfile.h"
#include "readline.h"
#include "rndstraw.h"
#include "gitinfo.h"
#include <getopt.h>
#include <string>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#ifdef _WIN32
#include <shellapi.h>
#include <sysinfoapi.h>
#else
#include <sys/time.h>
#endif

#include "utfargs.h"

/*
**	This is the pointer to the first mixfile in the list of mixfiles registered
**	with the mixfile system.
*/
template <class T, class TCRC> List<MixFileClass<T, TCRC>> MixFileClass<T, TCRC>::MixList;

void Print_Help()
{
    char revision[12] = {0};
    const char* version = GitTag[0] == 'v' ? GitTag : GitShortSHA1;

    if (GitTag[0] != 'v') {
        snprintf(revision, sizeof(revision), "r%d ", GitRevision);
    }

    printf("\nvanillamix %s%s%s\n\n"
           "Usage:\n"
           "  vanillamix (-c | -x) [-d <path>] [-f <file>] <mixfile>\n"
           "  vanillamix -l <mixfile>\n"
           "  vanillamix -h | --help\n\n"
           "Options:\n"
           "  -c --create     Pack loose files into a mix file.\n"
           "  -x --extract    Extract files from a mix file.\n"
           "  -l --list       List files in a mix file.\n"
           "                  Unknown file names will be shown as a hash.\n"
           "  -e --encrypt    Encrypt the file header with RSA/Blowfish.\n"
           "                  Supported in Red Alert onward.\n"
           "  --checksum      Write a sha1 hash as a tailer to checksum contents.\n"
           "                  Supported in Red Alert onward.\n"
           "  --crc32         Use CRC32 to hash filenames for the header.\n"
           "                  Required for Tiberian Sun and later.\n"
           "  --align         Alignment in power of two for files. Max 16 bytes\n"
           "                  Align files in the mix to a byte boundary.\n"
           "  -v --verbose    Provide progress information.\n"
           "  -d --directory  Path to search for files or to extract files to.\n"
           "  -f --file       File to either pack or extract.\n"
           "                  Can be specified multiple times for multiple files.\n"
           "  -m --manifest   Text file containing a list of files to pack, one per line.\n"
           "  -h --help       Displays this help.\n\n",
           revision,
           GitUncommittedChanges ? "~" : "",
           version);
}

// These two stub functions are just to satisfy MixFileClass's requirements to link.
void Prog_End(char const*, bool)
{
    exit(1);
}

bool Force_CD_Available(int)
{
    return true;
}

int RequiredCD;
bool RunningAsDLL;
bool Encrypt;
bool Checksum;
bool Verbose;
bool UseCRC32;
PKey PublicKey;
PKey PrivateKey;
RandomStraw CryptRandom;

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

template <typename I> std::string Int_To_Hex(I w, size_t hex_len = sizeof(I) << 1)
{
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len, '0');

    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
        rc[i] = digits[(w >> j) & 0x0f];
    }

    return rc;
}

template <typename CRC> void List_Mix(const char* filename, MixNameDatabase& name_db, MixNameDatabase::HashMethod hash)
{
    MixFileClass<RawFileClass, CRC> mixfile(filename, &PublicKey);
    const typename MixFileClass<RawFileClass, CRC>::SubBlock* index = mixfile.Get_Index();
    printf("%-24s  %-10s  %-10s\n", "Filename", "Offset", "Size");
    printf("%-24s  %10s  %10s\n", "========================", "==========", "==========");

    for (int i = 0; i < mixfile.Get_File_Count(); ++i) {
        std::string filename = name_db.Get_Entry(index[i].CRC, hash).file_name;

        if (filename.empty()) {
            filename = Int_To_Hex(index[i].CRC);
        }

        printf("%-24s  %*d  %*d\n", filename.c_str(), 10, index[i].Offset, 10, index[i].Size);
    }
}

template <typename CRC>
void Extract_Mix(const char* filename,
                 const char* base_path,
                 MixNameDatabase& name_db,
                 MixNameDatabase::HashMethod hash,
                 const std::set<std::string>& files)
{
    const size_t BUFFER_SIZE = 2048;
    MixFileClass<RawFileClass, CRC> mixfile(filename, &PublicKey);

    if (files.empty()) {
        const typename MixFileClass<RawFileClass, CRC>::SubBlock* index = mixfile.Get_Index();
        RawFileClass reader(filename);

        if (!reader.Open(READ)) {
            return;
        }

        // Iterate the mix file index and extract the files found.
        for (int i = 0; i < mixfile.Get_File_Count(); ++i) {
            uint8_t buffer[BUFFER_SIZE];
            std::string filename;

            if (base_path != nullptr && *base_path != '\0') {
                filename = base_path;
                filename += "/";
            }

            if ((hash == MixNameDatabase::HASH_RACRC && (uint32_t)index[i].CRC == 0x54C2D545)
                || (hash == MixNameDatabase::HASH_CRC32 && (uint32_t)index[i].CRC == 0x366E051F)) {
                if (Verbose) {
                    printf("XCC local mix database extension detected, skipping entry.\n.");
                }

                continue;
            }

            // If we don't get a useable filename back, then use the hex version of the file CRC to create a filename.
            if (name_db.Get_Entry(index[i].CRC, hash).file_name.empty()) {
                filename += Int_To_Hex(index[i].CRC);
            } else {
                filename += name_db.Get_Entry(index[i].CRC, hash).file_name;
            }

            int offset;
            int size;
            MixFileClass<RawFileClass, CRC>* mp;

            if (!MixFileClass<RawFileClass, CRC>::Offset(index[i].CRC, nullptr, &mp, &offset, &size)) {
                if (Verbose) {
                    printf("Failed to find '0x%08X' in any loaded mix file.\n", index[i].CRC);
                }

                continue;
            }

            if (offset < 0 || size < 0) {
                if (Verbose) {
                    printf("A corrupt index entry was found, ignoring faulty file.");
                }

                continue;
            }

            RawFileClass writer(filename.c_str());

            // If file won't open for writing then try next file.
            if (!writer.Open(WRITE)) {
                if (Verbose) {
                    printf("Couldn't open '%s' for writing.\n", writer.File_Name());
                }

                continue;
            }

            if (Verbose) {
                printf("Extracting '%s'.\n", writer.File_Name());
            }

            // Seek to the file in question and start copying its contents.
            reader.Seek(offset, SEEK_SET);

            while (size > BUFFER_SIZE) {
                // Make sure we actually read the amount of data we expected to.
                if (reader.Read(buffer, BUFFER_SIZE) == BUFFER_SIZE) {
                    writer.Write(buffer, BUFFER_SIZE);
                    size -= BUFFER_SIZE;
                } else {
                    if (Verbose) {
                        printf("Failed to read sufficient data.\n");
                    }

                    break;
                }
            }

            if (size > 0 && size < BUFFER_SIZE) {
                if (reader.Read(buffer, size) == size) {
                    writer.Write(buffer, size);
                } else if (Verbose) {
                    printf("Failed to read sufficient data.\n");
                }
            }
        }
    } else {
        for (auto it = files.begin(); it != files.end(); ++it) {
            int offset;
            int size;
            MixFileClass<RawFileClass, CRC>* mp;

            if (!MixFileClass<RawFileClass, CRC>::Offset(it->c_str(), nullptr, &mp, &offset, &size)) {
                if (Verbose) {
                    printf("Failed to find '%s' in any loaded mix file.\n", it->c_str());
                }

                continue;
            }

            RawFileClass reader(mp->Filename);

            if (!reader.Open(READ)) {
                continue;
            }

            uint8_t buffer[BUFFER_SIZE];
            std::string filename;

            if (base_path != nullptr && *base_path != '\0') {
                filename = base_path;
                filename += "/";
            }

            filename += *it;

            RawFileClass writer(filename.c_str());

            // If file won't open for writing then try next file.
            if (!writer.Open(WRITE)) {
                if (Verbose) {
                    printf("Couldn't open '%s' for writing.\n", writer.File_Name());
                }

                continue;
            }

            // Seek to the file in question and start copying its contents.
            reader.Seek(offset, SEEK_SET);

            while (size > BUFFER_SIZE) {
                reader.Read(buffer, BUFFER_SIZE);
                writer.Write(buffer, BUFFER_SIZE);
                size -= BUFFER_SIZE;
            }

            if (size > 0) {
                reader.Read(buffer, size);
                writer.Write(buffer, size);
            }
        }
    }
}
template <typename CRC>
int Create_Mix(const char* outfile,
               unsigned alignment,
               MixNameDatabase* name_db = nullptr,
               const char* search_dir = nullptr,
               std::set<std::string>* files = nullptr,
               const char* manifest = nullptr)
{
    MixFileCreatorClass<RawFileClass, CRC> mc(outfile, alignment, Checksum, Encrypt, !Verbose, false, name_db);
    std::set<std::string> default_files;
    bool no_loose_files = false;

    if (files == nullptr) {
        files = &default_files;
    }

    // Make note that we didn't get any files passed to us.
    if (files->empty()) {
        no_loose_files = true;
    }

    // Add files from the manifest.
    if (manifest != nullptr) {
        char buffer[512];
        bool eof = false;
        RawFileClass tfc(manifest);
        tfc.Open();

        while (!eof) {
            Read_Line(tfc, buffer, sizeof(buffer), eof);

            if (strlen(buffer) > 0) {
                files->insert(buffer);
            }
        }

        tfc.Close();
    }

    // If we didn't get a manifest or any loose files, assume we want to pack the entire directory.
    if (manifest == nullptr && no_loose_files) {
        DIR* dp;
        struct dirent* dirp;
        struct stat st;

        if ((dp = opendir(search_dir)) != nullptr) {
            while ((dirp = readdir(dp)) != nullptr) {
                std::string fullpath = std::string(search_dir) + PathsClass::SEP;
                fullpath += dirp->d_name;
                stat(fullpath.c_str(), &st);

                // We don't recurse a directory if it has subdirectories, just pack the loose files in it.
                if (S_ISDIR(st.st_mode)) {
                    continue;
                }

                files->insert(dirp->d_name);
            }

            closedir(dp);
        }
    }

    // Add lose specified files.
    if (!files->empty()) {
        for (auto it : *files) {
            if (!it.empty()) {
                std::string fullpath;
                if (!Paths.Is_Absolute(it.c_str())) {
                    fullpath = std::string(search_dir) + PathsClass::SEP;
                }

                fullpath += it;
                mc.Add_File(fullpath.c_str());
            }
        }

        mc.Write_Mix();
    }

    return 0;
}

void Key_Init()
{
    static const char Keys[] = "[PublicKey]\n1=AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V\n\n"
                               "[PrivateKey]\n1=AigKVje8mROcR8QixnxUEF5b29Curkq01DNDWCdOG99XBqH79OaCiTCB\n\n";

    RAMFileClass file((void*)Keys, (int)strlen(Keys));

    INIClass ini;
    ini.Load(file);

    PublicKey = ini.Get_PKey(true);
    PrivateKey = ini.Get_PKey(false);
}

void Init_Random()
{
#ifdef _WIN32
    struct _SYSTEMTIME SystemTime;
#else  // !PLATFORM_WINDOWS
    struct tm* SystemTime;
    struct timespec MicroTime;
#endif // PLATFORM_WINDOWS

#ifdef _WIN32
    GetSystemTime(&SystemTime);
    CryptRandom.Seed_Byte((char)SystemTime.wMilliseconds);
    CryptRandom.Seed_Bit(SystemTime.wSecond);
    CryptRandom.Seed_Bit(SystemTime.wSecond >> 1);
    CryptRandom.Seed_Bit(SystemTime.wSecond >> 2);
    CryptRandom.Seed_Bit(SystemTime.wSecond >> 3);
    CryptRandom.Seed_Bit(SystemTime.wSecond >> 4);
    CryptRandom.Seed_Bit(SystemTime.wMinute);
    CryptRandom.Seed_Bit(SystemTime.wMinute >> 1);
    CryptRandom.Seed_Bit(SystemTime.wMinute >> 2);
    CryptRandom.Seed_Bit(SystemTime.wMinute >> 3);
    CryptRandom.Seed_Bit(SystemTime.wMinute >> 4);
    CryptRandom.Seed_Bit(SystemTime.wHour);
    CryptRandom.Seed_Bit(SystemTime.wDay);
    CryptRandom.Seed_Bit(SystemTime.wDayOfWeek);
    CryptRandom.Seed_Bit(SystemTime.wMonth);
    CryptRandom.Seed_Bit(SystemTime.wYear);
#else
    struct tm* sys_time;
    struct timeval curr_time;
    gettimeofday(&curr_time, nullptr);
    sys_time = localtime(&curr_time.tv_sec);

    CryptRandom.Seed_Byte(curr_time.tv_usec / 1000);
    CryptRandom.Seed_Bit(sys_time->tm_sec);
    CryptRandom.Seed_Bit(sys_time->tm_sec >> 1);
    CryptRandom.Seed_Bit(sys_time->tm_sec >> 2);
    CryptRandom.Seed_Bit(sys_time->tm_sec >> 3);
    CryptRandom.Seed_Bit(sys_time->tm_sec >> 4);
    CryptRandom.Seed_Bit(sys_time->tm_min);
    CryptRandom.Seed_Bit(sys_time->tm_min >> 1);
    CryptRandom.Seed_Bit(sys_time->tm_min >> 2);
    CryptRandom.Seed_Bit(sys_time->tm_min >> 3);
    CryptRandom.Seed_Bit(sys_time->tm_min >> 4);
    CryptRandom.Seed_Bit(sys_time->tm_hour);
    CryptRandom.Seed_Bit(sys_time->tm_mday);
    CryptRandom.Seed_Bit(sys_time->tm_wday);
    CryptRandom.Seed_Bit(sys_time->tm_mon);
    CryptRandom.Seed_Bit(sys_time->tm_year);
#endif // _WIN32
}

int main(int argc, char* argv[])
{
    const char* base_path = "";
    const char* manifest = nullptr;
    int create_file = false;
    int extracting = false;
    int listing = false;
    unsigned alignment = 1;
    std::set<std::string> files;

    if (argc <= 1) {
        Print_Help();
        return 0;
    }

    UtfArgs args(argc, argv);

    // These keys are used to handle encrypted mix file headers.
    Key_Init();

    while (true) {
        static struct option long_options[] = {
            {"create", no_argument, &create_file, 'c'},
            {"extract", no_argument, &extracting, 'x'},
            {"list", no_argument, &listing, 'l'},
            {"checksum", no_argument, 0, 0},
            {"crc32", no_argument, 0, 1},
            {"encrypt", no_argument, 0, 'e'},
            {"verbose", no_argument, 0, 'v'},
            {"align", required_argument, 0, 2},
            {"file", required_argument, 0, 'f'},
            {"directory", required_argument, 0, 'd'},
            {"manifest", required_argument, 0, 'm'},
            {"help", no_argument, 0, 'h'},
            {nullptr, no_argument, nullptr, 0},
        };

        int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        int c = getopt_long(args.ArgC, args.ArgV, "+chxelv?f:d:m:", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
        case 0:
            Checksum = true;
            break;
        case 1:
            UseCRC32 = true;
            break;
        case 2:
            alignment = atoi(optarg);
            break;
        case 'l':
            listing = true;
            break;
        case 'c':
            create_file = true;
            break;
        case 'x':
            extracting = true;
            break;
        case 'v':
            Verbose = true;
            break;
        case 'e':
            Encrypt = true;
            break;
        case 'f':
            files.insert(optarg);
            break;
        case 'd':
            base_path = optarg;
            break;
        case 'm':
            manifest = optarg;
            break;
        case '?':
            printf("\nOption not recognised.\n");
            Print_Help();
            return 0;
        case ':':
            printf("\nAn option is missing arguments.\n");
            Print_Help();
            return 0;
        case 'h':
            Print_Help();
            return 0;
        default:
            break;
        }
    }

    if (optind != argc - 1) {
        printf("You must specify a file name for your Mix file as the last parameter.\n");
        return 1;
    }

    // Load the local mix name database and if one doesn't exist, start one.
    Paths.Init("vanillatools");
    MixNameDatabase namedb;
    bool prog_dir = false;
    std::string user_db = std::string(Paths.User_Path()) + "/filenames.db";
    std::string prog_db = std::string(Paths.Program_Path()) + "/filenames.db";
    if (!namedb.Read_From_Ini(user_db.c_str())) {
        if (!namedb.Read_From_Ini(prog_db.c_str())) {
            // Saving the internal to the user directory by default so it will auto save any changes later on exit.
            if (Verbose) {
                printf("Failed to read file name database generating from internal db.\n");
            }
            namedb.Read_Internal();
            namedb.Save_To_Ini(user_db.c_str());
        } else {
            prog_dir = true;
        }
    }

    if (create_file) {
        Init_Random();
        if (Verbose) {
            printf("Creating file %s using directory %s as base.\n", argv[argc - 1], base_path);
        }

        if (UseCRC32) {
            if (Verbose) {
                printf("Creating CRC32 Mix file suitable for TS and RA2.\n");
            }
            Create_Mix<CRC32Engine>(argv[argc - 1], alignment, &namedb, base_path, &files, manifest);
        } else {
            if (Verbose) {
                printf("Creating C&C Hash Mix file suitable for TD and RA.\n");
            }
            Create_Mix<CRCEngine>(argv[argc - 1], alignment, &namedb, base_path, &files, manifest);
        }
    } else { // Extraction/listing mode.
        // Load any file names we got on the command line into the database.
        for (auto it = files.begin(); it != files.end(); ++it) {
            namedb.Add_Entry(it->c_str(), "");
        }

        // If we want a list of files in each mix, print them out here.
        if (listing) {
            if (UseCRC32) {
                List_Mix<CRC32Engine>(argv[argc - 1], namedb, MixNameDatabase::HashMethod::HASH_CRC32);
            } else {
                List_Mix<CRCEngine>(argv[argc - 1], namedb, MixNameDatabase::HashMethod::HASH_RACRC);
            }
        }

        if (extracting) {
            if (UseCRC32) {
                Extract_Mix<CRC32Engine>(
                    argv[argc - 1], base_path, namedb, MixNameDatabase::HashMethod::HASH_CRC32, files);
            } else {
                Extract_Mix<CRCEngine>(
                    argv[argc - 1], base_path, namedb, MixNameDatabase::HashMethod::HASH_RACRC, files);
            }
        }
    }

    return 0;
}
