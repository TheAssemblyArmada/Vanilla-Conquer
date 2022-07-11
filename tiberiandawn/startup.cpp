//
// Copyright 2020 Electronic Arts Inc.
//
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

/* $Header:   F:\projects\c&c\vcs\code\startup.cpv   2.17   16 Oct 1995 16:48:12   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : STARTUP.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : October 3, 1994                                              *
 *                                                                                             *
 *                  Last Update : August 27, 1995 [JLB]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Prog_End -- Cleans up library systems in prep for game exit.                              *
 *   main -- Initial startup routine (preps library systems).                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "common/ini.h"
#include "common/paths.h"
#include "common/utfargs.h"
#include "settings.h"

bool Read_Private_Config_Struct(FileClass& file, NewConfigType* config);
void Print_Error_End_Exit(char* string);
void Print_Error_Exit(char* string);
#ifdef _WIN32
#include <direct.h>
#include "common/utf.h"
#define vc_chdir(x) _wchdir(UTF8To16(x))
extern void Create_Main_Window(HANDLE instance, int width, int height);
HINSTANCE ProgramInstance;
#else
#include <unistd.h>
#define vc_chdir(x) chdir(x)
#endif

extern int ReadyToQuit;
void Read_Setup_Options(RawFileClass* config_file);

bool VideoBackBufferAllowed = true;
void Check_From_WChat(char* wchat_name);
bool ProgEndCalled = false;

/***********************************************************************************************
 * main -- Initial startup routine (preps library systems).                                    *
 *                                                                                             *
 *    This is the routine that is first called when the program starts up. It basically        *
 *    handles the command line parsing and setting up library systems.                         *
 *                                                                                             *
 * INPUT:   argc  -- Number of command line arguments.                                         *
 *                                                                                             *
 *          argv  -- Pointer to array of comman line argument strings.                         *
 *                                                                                             *
 * OUTPUT:  Returns with execution failure code (if any).                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/

void Move_Point(short& x, short& y, register DirType dir, unsigned short distance);

void Check_Use_Compressed_Shapes(void);
extern void DLL_Shutdown(void);

#if defined REMASTER_BUILD && defined _WIN32
BOOL WINAPI DllMain(HINSTANCE instance, unsigned int fdwReason, void* lpvReserved)
{
    lpvReserved;

    switch (fdwReason) {

    case DLL_PROCESS_ATTACH:
        ProgramInstance = instance;
        break;

    case DLL_PROCESS_DETACH:
        DLL_Shutdown();

        MFCD::Free_All();

        Uninit_Game();

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return true;
}
#endif

#ifdef REMASTER_BUILD
int main(int, char*[]);

int DLL_Startup(const char* command_line_in)
{
    /* Construct argc and argv from command_line_in. Remaster build requires
    ** that first argument is a full path to DLL, not executable. Furthermore, it
    ** seems to override the argv to include extra parameters which the DLL
    ** expects. Getting the argc and argv from executable will result in
    ** a crash trying to read the font files. */

    RunningAsDLL = true;
    HINSTANCE instance = ProgramInstance;
    char command_line[1024];
    int argc = 0;
    unsigned command_scan;
    char command_char;
    char* argv[20];
    char path_to_exe[280];

    strcpy(command_line, command_line_in);
    ProgramInstance = instance;

    /*
    ** Get the full path to the .DLL
    */
    DWORD readed = GetModuleFileNameA(instance, &path_to_exe[0], 280);
    if (readed >= 280 - 1) {
        MessageBoxA(NULL, "Path to remaster is too large.", "Command & Conquer", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }

    /*
    ** First argument is supposed to be a pointer to the .EXE that is running
    ** - False. Must be a pointer to the DLL - giulianob 07/11/2021
    **
    */
    argc = 1;                  // Set argument count to 1
    argv[0] = &path_to_exe[0]; // Set 1st command line argument to point to full path

    /*
    ** Get pointers to command line arguments just like if we were in DOS
    **
    ** The command line we get is cr/zero? terminated.
    **
    */

    command_scan = 0;

    /* This certainly can be improved, but worse than this is not working :)*/

    do {
        /*
        ** Scan for non-space character on command line
        */
        do {
            command_char = *(command_line + command_scan++);
        } while (command_char == ' ');

        if (command_char != 0 && command_char != 13) {
            argv[argc++] = command_line + command_scan - 1;

            /*
            ** Scan for space character on command line
            */
            bool in_quotes = false;
            do {
                command_char = *(command_line + command_scan++);
                if (command_char == '"') {
                    in_quotes = !in_quotes;
                }
            } while ((in_quotes || command_char != ' ') && command_char != 0 && command_char != 13);

            *(command_line + command_scan - 1) = 0;
        }

    } while (command_char != 0 && command_char != 13 && argc < 20);

    if (argc >= 20) {
        MessageBoxA(NULL, "Too many arguments on command line.", "Command & Conquer", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }

    return main(argc, argv);
}
#endif // REMASTER_BUILD

int main(int argc, char** argv)
{
    UtfArgs args(argc, argv);
    CCDebugString("C&C95 - Starting up.\n");

    if (Ram_Free(MEM_NORMAL) < 5000000) {
#ifdef GERMAN
        printf("Zuwenig Hauptspeicher verf?gbar.\n");
#else
#ifdef FRENCH
        printf("M�moire vive (RAM) insuffisante.\n");
#else
        printf("Insufficient RAM available.\n");
#endif
#endif
        return (EXIT_FAILURE);
    }

#ifdef JAPANESE
    ForceEnglish = false;
#endif

    /*
    **	Remember the current working directory and drive.
    */
    Paths.Init("vanillatd", "CONQUER.INI", "CONQUER.MIX", args.ArgV[0]);

#ifdef __APPLE__
    vc_chdir(Paths.User_Path());
#else
    vc_chdir(Paths.Data_Path());
#endif

    CDFileClass::Refresh_Search_Drives();

    if (Parse_Command_Line(args.ArgC, args.ArgV)) {

        WinTimerClass::Init(60);

        CCFileClass cfile("CONQUER.INI");

        Keyboard = new WWKeyboardClass();

#ifdef JAPANESE
        //////////////////////////////////////if(!ForceEnglish) KBLanguage = 1;
#endif

        /*
        ** If there is loads of memory then use uncompressed shapes
        */
        Check_Use_Compressed_Shapes();

        /*
        ** If there is not enough disk space free, dont allow the product to run.
        */
        if (Disk_Space_Available() < INIT_FREE_DISK_SPACE) {
#if (0) // PG
#ifdef GERMAN
            char disk_space_message[512];
            sprintf(disk_space_message,
                    "Nicht genug Festplattenplatz f?r Command & Conquer.\nSie brauchen %d MByte freien Platz auf der "
                    "Festplatte.",
                    (INIT_FREE_DISK_SPACE) / (1024 * 1024));
            MessageBoxA(NULL, disk_space_message, "Command & Conquer", MB_ICONEXCLAMATION | MB_OK);
            return (EXIT_FAILURE);
#endif
#ifdef FRENCH
            char disk_space_message[512];
            sprintf(disk_space_message,
                    "Espace disque insuffisant pour lancer Command & Conquer.\nVous devez disposer de %d Mo d'espace "
                    "disponsible sur disque dur.",
                    (INIT_FREE_DISK_SPACE) / (1024 * 1024));
            MessageBoxA(NULL, disk_space_message, "Command & Conquer", MB_ICONEXCLAMATION | MB_OK);
            return (EXIT_FAILURE);
#endif
#if !(FRENCH | GERMAN)
            int reply = MessageBoxA(NULL,
                                    "Warning - you are critically low on free disk space for virtual memory and save "
                                    "games. Do you want to play C&C anyway?",
                                    "Command & Conquer",
                                    MB_ICONQUESTION | MB_YESNO);
            if (reply == IDNO) {
                return (EXIT_FAILURE);
            }

#endif
#endif
        }

        Read_Private_Config_Struct(cfile, &NewConfig);

        /*
        ** Set the options as requested by the ccsetup program
        */
        Read_Setup_Options(&cfile);

        CCDebugString("C&C95 - Creating main window.\n");

#ifndef REMASTER_BUILD
        /* If DOSMode is enabled, adjust resolution accordingly. */
        if (Settings.Video.DOSMode || Is_Demo() || Is_DOS_Files()) {
            ScreenWidth = 320;
            ScreenHeight = 200;
        }
#endif

#if defined(_WIN32) && !defined(SDL2_BUILD)
        Create_Main_Window(ProgramInstance, ScreenWidth, ScreenHeight);
#endif

        CCDebugString("C&C95 - Initialising audio.\n");

        SoundOn = Audio_Init(16, false, 11025 * 2, 0);

        Palette = new (MEM_CLEAR) unsigned char[768];

        bool video_success = false;
        CCDebugString("C&C95 - Setting video mode.\n");

        /*
        ** Set video mode.
        */
#ifdef REMASTER_BUILD
        video_success = true;
#else
        if (Set_Video_Mode(ScreenWidth, ScreenHeight, 8)) {
            video_success = true;
        }
#endif

        if (!video_success) {
            CCDebugString("C&C95 - Failed to set video mode.\n");
#ifdef _WIN32
            MessageBoxA(
                MainWindow, "Error - Unable to set the video mode.", "Command & Conquer", MB_ICONEXCLAMATION | MB_OK);
#endif
            if (Palette)
                delete[] Palette;
            return (EXIT_FAILURE);
        }

        CCDebugString("C&C95 - Initialising video surfaces.\n");

#ifdef REMASTER_BUILD

        VisiblePage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)0);
        HiddenPage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)0);

#else
        VisiblePage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)(GBC_VISIBLE | GBC_VIDEOMEM));

        /*
        ** Check that we really got a video memory page. Failure is fatal.
        */
        if (VisiblePage.IsAllocated()) {
            /*
            ** Aaaarrgghh!
            */
            CCDebugString("C&C95 - Unable to allocate primary surface.\n");
#ifdef _WIN32
            MessageBoxA(MainWindow,
                        Text_String(TXT_UNABLE_TO_ALLOCATE_PRIMARY_VIDEO_BUFFER),
                        "Command & Conquer",
                        MB_ICONEXCLAMATION | MB_OK);
#endif
            if (Palette)
                delete[] Palette;
            return (EXIT_FAILURE);
        }

        /*
        ** If we have enough left then put the hidpage in video memory unless...
        **
        ** If there is no blitter then we will get better performance with a system
        ** memory hidpage
        **
        ** Use a system memory page if the user has specified it via the ccsetup program.
        */
        CCDebugString("C&C95 - Allocating back buffer ");
        int video_memory = Get_Free_Video_Memory();
        unsigned video_capabilities = Get_Video_Hardware_Capabilities();
        if (video_memory < ScreenWidth * ScreenHeight || (!(video_capabilities & VIDEO_BLITTER))
            || (video_capabilities & VIDEO_NO_HARDWARE_ASSIST) || !VideoBackBufferAllowed) {
            CCDebugString("in system memory.\n");
            HiddenPage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)0);
        } else {
            // HiddenPage.Init (ScreenWidth , ScreenHeight , NULL , 0 , (GBC_Enum)0);
            CCDebugString("in video memory.\n");
            HiddenPage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)GBC_VIDEOMEM);

            /*
            ** Make sure we really got a video memory hid page. If we didnt then things
            ** will run very slowly.
            */
            if (HiddenPage.IsAllocated()) {
                /*
                ** Oh dear, big trub. This must be an IBM Aptiva or something similarly cruddy.
                ** We must redo the Hidden Page as system memory.
                */
                HiddenPage.Un_Init();
                HiddenPage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)0);
            } else {
                VisiblePage.Attach_DD_Surface(&HiddenPage);
            }
        }
#endif

        SeenBuff.Attach(&VisiblePage, 0, 0, GBUFF_INIT_WIDTH, GBUFF_INIT_HEIGHT);
        HidPage.Attach(&HiddenPage, 0, 0, GBUFF_INIT_WIDTH, GBUFF_INIT_HEIGHT);

        CCDebugString("C&C95 - Adjusting variables for resolution.\n");
        Options.Adjust_Variables_For_Resolution();

        CCDebugString("C&C95 - Setting palette.\n");
        /////////Set_Palette(Palette);

        WindowList[0][WINDOWWIDTH] = SeenBuff.Get_Width();
        WindowList[0][WINDOWHEIGHT] = SeenBuff.Get_Height();

        /*
        ** Install the memory error handler
        */
        Memory_Error = &Memory_Error_Handler;

        CCDebugString("C&C95 - Creating mouse class.\n");
        WWMouse = new WWMouseClass(&SeenBuff, 32, 32);
        //			MouseInstalled = Install_Mouse(32,24,320,200);
        MouseInstalled = true;

        /*
        ** See if we should run the intro
        */
        INIClass ini;
        ini.Load(cfile);

        /*
        **	Check for forced intro movie run disabling. If the conquer
        **	configuration file says "no", then don't run the intro.
        */
        if (!Special.IsFromInstall) {
            Special.IsFromInstall = ini.Get_Bool("Intro", "PlayIntro", true);
        }
        SlowPalette = ini.Get_Bool("Options", "SlowPalette", false);

        /*
        ** Regardless of whether we should run it or not, here we're
        ** gonna change it to say "no" in the future.
        */
        if (Special.IsFromInstall) {
            BreakoutAllowed = true;
            ini.Put_Bool("Intro", "PlayIntro", false);
            ini.Save(cfile);
        }

        Memory_Error_Exit = Print_Error_End_Exit;

        CCDebugString("C&C95 - Entering main game.\n");
        Main_Game(argc, argv);

        if (RunningAsDLL) {
            return (EXIT_SUCCESS);
        }

        /*
        ** Save settings if they were changed during gameplay.
        */
        ini.Load(cfile);
        Settings.Save(ini);
        ini.Save(cfile);

        VisiblePage.Clear();
        HiddenPage.Clear();

        Memory_Error_Exit = Print_Error_Exit;

        CCDebugString("C&C95 - About to exit.\n");
        ReadyToQuit = 1;

#if defined(SDL2_BUILD)
        Reset_Video_Mode();
#elif defined(_WIN32)
        PostMessageA(MainWindow, WM_DESTROY, 0, 0);
        do {
            Keyboard->Check();
        } while (ReadyToQuit == 1);
#endif

        CCDebugString("C&C95 - Returned from final message loop.\n");
        // Prog_End();
        // Invalidate_Cached_Icons();
        // VisiblePage.Un_Init();
        // HiddenPage.Un_Init();
        // AllSurfaces.Release();
        // Reset_Video_Mode();
        // Stop_Profiler();
        return (EXIT_SUCCESS);

        if (Palette) {
            delete[] Palette;
            Palette = NULL;
        }
    }

    return (EXIT_SUCCESS);
}

/***********************************************************************************************
 * Prog_End -- Cleans up library systems in prep for game exit.                                *
 *                                                                                             *
 *    This routine should be called before the game terminates. It handles cleaning up         *
 *    library systems so that a graceful return to the host operating system is achieved.      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void Prog_End(const char* why, bool fatal) // Added why and fatal parameters. ST - 6/27/2019 10:10PM
{
    GlyphX_Debug_Print("Prog_End()");

    if (why) {
        GlyphX_Debug_Print(why);
    }
    if (fatal) {
        *((int*)0) = 0;
    }

#ifndef DEMO
    if (GameToPlay == GAME_MODEM || GameToPlay == GAME_NULL_MODEM) {
        //		NullModem.Change_IRQ_Priority(0);
    }
#endif
    CCDebugString("C&C95 - About to call Sound_End.\n");
    Sound_End();
    CCDebugString("C&C95 - Returned from Sound_End.\n");
    if (WWMouse) {
        CCDebugString("C&C95 - Deleting mouse object.\n");
        delete WWMouse;
        WWMouse = NULL;
    }
    if (Palette) {
        CCDebugString("C&C95 - Deleting palette object.\n");
        delete[] Palette;
        Palette = NULL;
    }

    ProgEndCalled = true;
}

void Print_Error_End_Exit(char* string)
{
    printf("%s\n", string);
    Keyboard->Get();
    Prog_End();
    printf("%s\n", string);
    if (!RunningAsDLL) {
        exit(1);
    }
}

void Print_Error_Exit(char* string)
{
    printf("%s\n", string);
    if (!RunningAsDLL) {
        exit(1);
    }
}

/***********************************************************************************************
 * Read_Setup_Options -- Read stuff in from the INI file that we need to know sooner           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/7/96 4:09PM ST : Created                                                               *
 *=============================================================================================*/
void Read_Setup_Options(RawFileClass* config_file)
{
    INIClass ini;

    ini.Load(*config_file);

    /*
    ** Read in global settings
    */
    Settings.Load(ini);

    /*
    ** Read in the boolean options
    */
    VideoBackBufferAllowed = ini.Get_Bool("Options", "VideoBackBuffer", true);
    AllowHardwareBlitFills = ini.Get_Bool("Options", "HardwareFills", true);
}
