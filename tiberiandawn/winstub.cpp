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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : WINSTUB.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : 10/04/95                                                     *
 *                                                                                             *
 *                  Last Update : October 4th 1995 [ST]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *   This file contains stubs for undefined externals when linked under Watcom for Win 95      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include "common/tcpip.h"
#include "common/vqaaudio.h"

void output(short, short)
{
}

bool InDebugger = false;
int ReadyToQuit = 0;

#ifdef _WIN32
unsigned long CCFocusMessage = WM_USER + 50; // Private message for receiving application focus
#endif

ThemeType OldTheme = THEME_NONE;

/***********************************************************************************************
 * Focus_Loss -- this function is called when a library function detects focus loss            *
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
 *    2/1/96 2:10PM ST : Created                                                               *
 *=============================================================================================*/

void Focus_Loss(void)
{
    if (SoundOn) {
        if (OldTheme == THEME_NONE) {
            OldTheme = Theme.What_Is_Playing();
        }
    }
    Theme.Stop();
    Stop_Primary_Sound_Buffer();

    if (WWMouse && Is_Video_Fullscreen()) {
        WWMouse->Clear_Cursor_Clip();
    }
}

void Focus_Restore(void)
{
    Map.Flag_To_Redraw(true);
    Start_Primary_Sound_Buffer(true);

    if (Is_Video_Fullscreen()) {
        if (WWMouse) {
            WWMouse->Set_Cursor_Clip();
        }
        VisiblePage.Clear();
        HiddenPage.Clear();
    }
}

/***********************************************************************************************
 * Check_For_Focus_Loss -- check for the end of the focus loss                                 *
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
 *    2/2/96 10:49AM ST : Created                                                              *
 *=============================================================================================*/

void Check_For_Focus_Loss(void)
{
#if !defined(REMASTER_BUILD) && defined(_WIN32)
    static BOOL focus_last_time = 1;
    MSG msg;

    if (!GameInFocus) {
        Focus_Loss();
        while (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD)) {
            if (!GetMessageA(&msg, NULL, 0, 0)) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (!focus_last_time && GameInFocus) {

        VQA_PauseAudio();
        CountDownTimerClass cd;
        cd.Set(60 * 1);

        do {
            while (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE)) {
                if (!GetMessageA(&msg, NULL, 0, 0)) {
                    return;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }

        } while (cd.Time());
        VQA_ResumeAudio();
        // AllSurfaces.Restore_Surfaces();
        // VisiblePage.Clear();
        // HiddenPage.Clear();
        // Map.Flag_To_Redraw(true);
        PostMessageA(MainWindow, CCFocusMessage, 0, 0);
    }

    focus_last_time = GameInFocus;
#endif
}

extern bool InMovie;
#if !defined(REMASTER_BUILD) && defined(_WIN32) && !defined(SDL2_BUILD)
long FAR PASCAL Windows_Procedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{

    int low_param = LOWORD(wParam);

    if (message == CCFocusMessage) {
        Start_Primary_Sound_Buffer(TRUE);
        if (!InMovie) {
            Theme.Queue_Song(OldTheme);
            OldTheme = THEME_NONE;
        }
        return (0);
    }

    /*
    **	Pass this message through to the keyboard handler. If the message
    **	was processed and requires no further action, then return with
    **	this information.
    */
    if (Keyboard->Message_Handler(hwnd, message, wParam, lParam)) {
        return (1);
    }

    switch (message) {

    case WM_DESTROY:
        CCDebugString("C&C95 - WM_DESTROY message received.\n");
        CCDebugString("C&C95 - About to call Prog_End.\n");
        Prog_End();
        CCDebugString("C&C95 - About to release the video surfaces.\n");
        VisiblePage.Un_Init();
        HiddenPage.Un_Init();
        AllSurfaces.Release();
        if (!InDebugger) {
            CCDebugString("C&C95 - About to reset the video mode.\n");
            Reset_Video_Mode();
        }
        CCDebugString("C&C95 - Posting the quit message.\n");
        PostQuitMessage(0);
        /*
        ** If we are shutting down gracefully than flag that the message loop has finished.
        ** If this is a forced shutdown (ReadyToQuit == 0) then try and close down everything
        ** before we exit.
        */
        if (ReadyToQuit) {
            CCDebugString("C&C95 - We are now ready to quit.\n");
            ReadyToQuit = 2;
        } else {
            CCDebugString("C&C95 - Emergency shutdown.\n");
            CCDebugString("C&C95 - Shut down the network stuff.\n");
#ifndef DEMO
            Shutdown_Network();
#endif
            CCDebugString("C&C95 - Kill the Winsock stuff.\n");
            if (Winsock.Get_Connected())
                Winsock.Close();
            CCDebugString("C&C95 - Call ExitProcess.\n");
            ExitProcess(0);
        }
        CCDebugString("C&C95 - Clean & ready to quit.\n");
        return (0);

    case WM_ACTIVATEAPP:
        GameInFocus = (BOOL)wParam;
        if (!GameInFocus) {
            Focus_Loss();
        }
        AllSurfaces.Set_Surface_Focus(GameInFocus);
        AllSurfaces.Restore_Surfaces();
        //			if (GameInFocus){
        //				Restore_Cached_Icons();
        //				Map.Flag_To_Redraw(true);
        //				Start_Primary_Sound_Buffer(TRUE);
        //				if (WWMouse) WWMouse->Set_Cursor_Clip();
        //			}
        return (0);
#if (0)
    case WM_ACTIVATE:
        if (low_param == WA_INACTIVE) {
            GameInFocus = FALSE;
            Focus_Loss();
        }
        return (0);
#endif //(0)

    case WM_SYSCOMMAND:
        switch (wParam) {

        case SC_CLOSE:
            /*
            ** Windows sent us a close message. Probably in response to Alt-F4. Ignore it by
            ** pretending to handle the message and returning 0;
            */
            return (0);

        case SC_SCREENSAVE:
            /*
            ** Windoze is about to start the screen saver. If we just return without passing
            ** this message to DefWindowProc then the screen saver will not be allowed to start.
            */
            return (0);
        }
        break;

#ifdef FORCE_WINSOCK
    case WM_ACCEPT:
    case WM_HOSTBYADDRESS:
    case WM_HOSTBYNAME:
    case WM_ASYNCEVENT:
    case WM_UDPASYNCEVENT:
        Winsock.Message_Handler(hwnd, message, wParam, lParam);
        return (0);
#endif // FORCE_WINSOCK
    }

    return (DefWindowProc(hwnd, message, wParam, lParam));
}

#endif

/***********************************************************************************************
 * Create_Main_Window -- opens the MainWindow for C&C                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    instance -- handle to program instance                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/10/95 4:08PM ST : Created                                                             *
 *=============================================================================================*/

#define CC_ICON 1
int ShowCommand;

#if defined(_WIN32) && !defined(SDL2_BUILD)
void Create_Main_Window(HANDLE instance, int width, int height)

{
#ifdef REMASTER_BUILD
    MainWindow = NULL;
    return;
#else
    HWND hwnd;
    WNDCLASS wndclass;

    STARTUPINFOA sinfo;
    int command_show;

    sinfo.dwFlags = 0;
    GetStartupInfoA(&sinfo);

    if (sinfo.dwFlags & STARTF_USESHOWWINDOW) {
        command_show = sinfo.wShowWindow;
    } else {
        command_show = SW_SHOWDEFAULT;
    }

    //
    // Register the window class
    //

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = Windows_Procedure;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = (HINSTANCE)instance;
    wndclass.hIcon = LoadIconA((HINSTANCE)instance, MAKEINTRESOURCE(CC_ICON));
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = "Command & Conquer"; // NULL
    wndclass.lpszClassName = "Command & Conquer";

    RegisterClass(&wndclass);

    //
    // Create our main window
    //
    hwnd = CreateWindowExA(WS_EX_TOPMOST,
                           "Command & Conquer",
                           "Command & Conquer",
                           WS_POPUP | WS_MAXIMIZE,
                           0,
                           0,
                           width,
                           height,
                           NULL,
                           NULL,
                           (HINSTANCE)instance,
                           NULL);

    ShowWindow(hwnd, command_show);
    ShowCommand = command_show;
    UpdateWindow(hwnd);
    SetFocus(hwnd);
    MainWindow = hwnd; // Save the handle to our main window

    CCFocusMessage = RegisterWindowMessageA("CC_GOT_FOCUS");

    Audio_Focus_Loss_Function = &Focus_Loss;
    Misc_Focus_Loss_Function = &Focus_Loss;
    Misc_Focus_Restore_Function = &Focus_Restore;
    Gbuffer_Focus_Loss_Function = &Focus_Loss;
#endif
}
#endif

typedef struct tColourList
{

    char Red;
    char Green;
    char Blue;
} ColourList;

ColourList ColourLookup[9] = {0,  0,  0,  63, 0, 0,  0,  63, 0,  0,  0,  63, 63, 0,
                              63, 63, 63, 0,  0, 63, 63, 32, 32, 32, 63, 63, 63};

int DebugColour = 1;

void Set_Palette_Register(int number, int red, int green, int blue);
//#pragma off (unreferenced)
void Colour_Debug(int call_number)
{
    //#if 0
    // if (DebugColour==call_number || !call_number){

    // if (call_number){
    //	Wait_Vert_Blank();
    //}

    // ST - 1/3/2019 10:43AM
    // Set_Palette_Register (0,ColourLookup[call_number].Red ,
    //								ColourLookup[call_number].Green,
    //								ColourLookup[call_number].Blue);
    //}
    //#endif
}

//#pragma on (unreferenced)

bool Any_Locked()
{
    if (SeenBuff.Get_LockCount() || HidPage.Get_LockCount()) {
        return true;
    } else {
        return false;
    }
}

#ifdef _WIN32
HANDLE DebugFile = INVALID_HANDLE_VALUE;
#endif

/***********************************************************************************************
 * CCDebugString -- sends a string to the debugger and echos it to disk                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    string                                                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/28/96 12:48PM ST : Created                                                            *
 *=============================================================================================*/
void CCDebugString(const char* string)
{
#ifndef REMASTER_BUILD

#ifdef _WIN32
    char outstr[256];

    sprintf(outstr, "%s", string);

    DWORD actual;
    if (DebugFile == INVALID_HANDLE_VALUE) {
        DebugFile = CreateFile("debug.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        DebugFile = CreateFile("debug.txt", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (DebugFile != INVALID_HANDLE_VALUE) {
        SetFilePointer(DebugFile, 0, NULL, FILE_END);
        WriteFile(DebugFile, outstr, strlen(outstr) + 1, &actual, NULL);
        CloseHandle(DebugFile);
    }

    OutputDebugString(string);
#else
    fprintf(stderr, "%s", string);
#endif

#else

    string = string;

#endif
}

//
// Miscellaneous stubs. Mainly for multi player stuff
//
//
//

// IPXAddressClass::IPXAddressClass(void) {
//	int i;
//	i++;
//}
// int IPXManagerClass::Num_Connections(void){ return (0); }
// int IPXManagerClass::Connection_ID( int ) { return (0); }
// IPXAddressClass * IPXManagerClass::Connection_Address( int ) { return ((IPXAddressClass*)0); }
// char * IPXManagerClass::Connection_Name( int ) { return ((char*)0); }
// int IPXAddressClass::Is_Broadcast() { return (0); }
// int IPXManagerClass::Send_Global_Message( void *, int, int, IPXAddressClass * ) { return (0); }
// int IPXManagerClass::Service() { return (0); }
// int IPXManagerClass::Get_Global_Message( void  *, int  *, IPXAddressClass  *, short unsigned  * ) { return (0); }
// int IPXAddressClass::operator ==( IPXAddressClass  & ) { return (0); }
// IPXManagerClass::IPXManagerClass( int, int, int, int, short unsigned, short unsigned ) {}
// IPXManagerClass::~IPXManagerClass() {
//	int i;
//	i++;
//	}
// int  IPXManagerClass::Delete_Connection( int ) { return (0); }
// IPXAddressClass::IPXAddressClass( char unsigned  *, char unsigned  * ){}
// void  IPXManagerClass::Set_Socket( short unsigned ){}
// int  IPXManagerClass::Is_IPX() { return (0); }
// int  IPXManagerClass::Init() { return (0); }
// void  IPXAddressClass::Get_Address( char unsigned  *, char unsigned  * ){}
// void  IPXManagerClass::Set_Bridge( char unsigned  * ){}
// int  IPXManagerClass::Global_Num_Send() { return (0); }
// void  IPXManagerClass::Set_Timing( long unsigned, long unsigned, long unsigned ){}
// unsigned long IPXManagerClass::Global_Response_Time() { return (0); }
// int  IPXManagerClass::Create_Connection( int, char  *, IPXAddressClass  * ) { return (0); }
// int  IPXAddressClass::operator !=( IPXAddressClass  & ) { return (0); }
// int  IPXManagerClass::Send_Private_Message( void  *, int, int, int ) { return (0); }
// int  IPXManagerClass::Get_Private_Message( void  *, int  *, int  * ) { return (0); }
// int  IPXManagerClass::Connection_Index( int ) { return (0); }
// void  IPXManagerClass::Reset_Response_Time(){}
// long unsigned  IPXManagerClass::Response_Time() { return (0); }
// int  IPXManagerClass::Private_Num_Send( int ) { return (0); }

//_VQAHandle  *  VQA_Alloc(void){ return ((_VQAHandle *)0); }
// void  VQA_Init( _VQAHandle  *, long ( *)()) {}
// long  VQA_Open( _VQAHandle  *, char const  *, _VQAConfig  * ) { return (0); }
// void  VQA_Free( _VQAHandle  * ) {}
// void  VQA_Close( _VQAHandle  * ) {}
// long  VQA_Play( _VQAHandle  *, long ) { return (0); }

// void VQA_Init(VQAHandle *, long(*)(VQAHandle *vqa, long action,	void *buffer, long nbytes)){}

// long VQA_Open(VQAHandle *, char const *, VQAConfig *)
//{
//	return (0);
//}

// void VQA_Close(VQAHandle *){}

// long VQA_Play(VQAHandle *, long)
//{
//	return (0);
//}

/***********************************************************************************************
 * Memory_Error_Handler -- Handle a possibly fatal failure to allocate memory                  *
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
 *    5/22/96 3:57PM ST : Created                                                              *
 *=============================================================================================*/
void Memory_Error_Handler(void)
{
    GlyphX_Debug_Print("Error - out of memory.");
    VisiblePage.Clear();
    Set_Palette(GamePalette);
    while (Get_Mouse_State()) {
        Show_Mouse();
    };
    WWMessageBox().Process("Error - out of memory.", "Abort", nullptr, nullptr, false);

    // Nope. ST - 1/10/2019 10:38AM
    // PostQuitMessage( 0 );
    // ExitProcess(0);
}

GraphicBufferClass* Read_PCX_File(char* name, char* Palette, void* Buff, long Size);

/***********************************************************************************************
 * Load_Title_Screen -- loads the title screen into the given video buffer                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    screen name                                                                       *
 *           video buffer                                                                      *
 *           ptr to buffer for palette                                                         *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    7/5/96 11:30AM ST : Created                                                              *
 *=============================================================================================*/

void Load_Title_Screen(char* name, GraphicViewPortClass* video_page, unsigned char* palette)
{

    GraphicBufferClass* load_buffer;

    load_buffer = Read_PCX_File(name, (char*)palette, NULL, 0);

    if (load_buffer) {
        load_buffer->Blit(*video_page);
        delete load_buffer;
    }
}

#include "common/filepcx.h"

/***************************************************************************
 * READ_PCX_FILE -- read a pcx file into a Graphic Buffer                  *
 *                                                                         *
 *	GraphicBufferClass* Read_PCX_File (char* name, char* palette ,void *Buff, long size );	*
 *  																								*
 *                                                                         *
 * INPUT: name is a NULL terminated string of the fromat [xxxx.pcx]        *
 *        palette is optional, if palette != NULL the the color palette of *
 *					 the pcx file will be place in the memory block pointed	   *
 *               by palette.																*
 *			 Buff is optinal, if Buff == NULL a new memory Buffer		 			*
 *					 will be allocated, otherwise the file will be placed 		*
 *					 at location pointd by Buffer;										*
 *			Size is the size in bytes of the memory block pointed by Buff		*
 *				  is also optional;
 *                                                                         *
 * OUTPUT: on succes a pointer to a GraphicBufferClass cointaining the     *
 *         pcx file, NULL othewise.                                        *
 *																									*
 * WARNINGS:                                                               *
 *         Appears to be a comment-free zone                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1995 JRJ : Created.                                             *
 *   04/30/1996 ST : Tidied up and modified to use CCFileClass             *
 *=========================================================================*/

#define POOL_SIZE 2048
#define READ_CHAR()                                                                                                    \
    *file_ptr++;                                                                                                       \
    if (file_ptr >= &pool[POOL_SIZE]) {                                                                                \
        file_handle.Read(pool, POOL_SIZE);                                                                             \
        file_ptr = pool;                                                                                               \
    }

/*
** The old RGB was signed, so the shift right didn't work. ST - 1/4/2019 4:49PM
*/
typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} PG_RGB;

GraphicBufferClass* Read_PCX_File(char* name, char* palette, void* Buff, long Size)
{
    int i, j;
    unsigned rle;
    unsigned color;
    int scan_pos;
    unsigned char* file_ptr;
    int width;
    int height;
    unsigned char* buffer;
    PCX_HEADER header;
    PG_RGB* pal;
    unsigned char pool[POOL_SIZE];
    GraphicBufferClass* pic;

    CCFileClass file_handle(name);

    if (!file_handle.Is_Available())
        return (NULL);

    file_handle.Open(READ);

    file_handle.Read(&header, sizeof(PCX_HEADER));

    if (header.id != 10 && header.version != 5 && header.pixelsize != 8)
        return NULL;

    width = header.width - header.x + 1;
    height = header.height - header.y + 1;

    if (Buff) {
        buffer = (unsigned char*)Buff;
        i = Size / width;
        height = MIN(i - 1, height);
        pic = new GraphicBufferClass(width, height, buffer, Size);
        if (!(pic && pic->Get_Buffer()))
            return NULL;
    } else {
        pic = new GraphicBufferClass(width, height, NULL, width * (height + 4));
        if (!(pic && pic->Get_Buffer()))
            return NULL;
    }

    buffer = (unsigned char*)pic->Get_Buffer();
    file_ptr = pool;
    file_handle.Read(pool, POOL_SIZE);

    if (header.byte_per_line != width) {

        for (scan_pos = j = 0; j < height; j++, scan_pos += width) {
            for (i = 0; i < width;) {
                rle = READ_CHAR();
                if (rle > 192) {
                    rle -= 192;
                    color = READ_CHAR();
                    ;
                    memset(buffer + scan_pos + i, color, rle);
                    i += rle;
                } else {
                    *(buffer + scan_pos + i++) = (char)rle;
                }
            }
        }

        if (i == width)
            rle = READ_CHAR();
        if (rle > 192)
            rle = READ_CHAR();

    } else {

        for (i = 0; i < width * height;) {
            rle = READ_CHAR();
            rle &= 0xff;
            if (rle > 192) {
                rle -= 192;
                color = READ_CHAR();
                memset(buffer + i, color, rle);
                i += rle;
            } else {
                *(buffer + i++) = rle;
            }
        }
    }

    if (palette) {
        file_handle.Seek(-(256 * (int)sizeof(PG_RGB)), SEEK_END);
        file_handle.Read(palette, 256L * sizeof(PG_RGB));

        pal = (PG_RGB*)palette;
        for (i = 0; i < 256; i++) {
            pal->red >>= 2;
            pal->green >>= 2;
            pal->blue >>= 2;
            pal++;
        }
    }

    file_handle.Close();
    return pic;
}