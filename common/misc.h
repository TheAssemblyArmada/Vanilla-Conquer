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

/***************************************************************************
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : 32 bit library                           *
 *                                                                         *
 *                    File Name : MISC.H                                   *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : August 3, 1994                           *
 *                                                                         *
 *                  Last Update : August 3, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef MISC_H
#define MISC_H

/*========================= C++ Routines ==================================*/

/*=========================================================================*/
/* The following prototypes are for the file: DDRAW.CPP                    */
/*=========================================================================*/

/*
** Pointer to function to call if we detect a focus loss
*/
extern void (*Misc_Focus_Loss_Function)(void);
/*
** Pointer to function to call if we detect a surface restore
*/
extern void (*Misc_Focus_Restore_Function)(void);

/*=========================================================================*/
/* The following variables are declared in: DDRAW.CPP                      */
/*=========================================================================*/

#ifdef _WIN32
#include <windows.h>
extern HWND MainWindow;
#endif
extern bool SystemToVideoBlits;
extern bool VideoToSystemBlits;
extern bool SystemToSystemBlits;
extern bool OverlappedVideoBlits; // Can video driver blit overlapped regions?

/*=========================================================================*/
/* The following prototypes are for the file: EXIT.CPP                     */
/* Prog_End Must be supplied by the user program in startup.cpp            */
/*=========================================================================*/
void Prog_End(const char* why = nullptr,
              bool fatal = false); // Added why and fatal parameters. ST - 6/27/2019 10:10PM
void Exit(int errorval, const unsigned char* message, ...);

/*=========================================================================*/
/* The following prototypes are for the file: DELAY.CPP                    */
/*=========================================================================*/
void Delay(int duration);
void Vsync(void);

/*=========================================================================*/
/* The following prototypes are for the file: FINDARGV.CPP                 */
/*=========================================================================*/

unsigned char Find_Argv(unsigned char const* str);

/*=========================================================================*/
/* The following prototypes are for the file: LIB.CPP                      */
/*=========================================================================*/
char* Find_Argv(char const* str);
void Mono_Mem_Dump(void const* databuf, int bytes, int y);
void Convert_RGB_To_HSV(unsigned int r,
                        unsigned int g,
                        unsigned int b,
                        unsigned int* h,
                        unsigned int* s,
                        unsigned int* v);
void Convert_HSV_To_RGB(unsigned int h,
                        unsigned int s,
                        unsigned int v,
                        unsigned int* r,
                        unsigned int* g,
                        unsigned int* b);

/*=========================================================================*/
/* The following prototypes are for the file: VERSION.CPP                  */
/*=========================================================================*/

unsigned char Version(void);

/*========================= Assembly Routines ==============================*/

/*=========================================================================*/
/* The following prototype is for the file: SHAKESCR.ASM                   */
/*=========================================================================*/

void Shake_Screen(int shakes);

/*=========================================================================*/
/* The following prototypes are for the file: REVERSE.ASM                  */
/*=========================================================================*/

int Reverse_Long(int number);

/*=========================================================================*/
/* The following prototypes are for the file: DETPROC.ASM                  */
/*=========================================================================*/

extern short Processor(void);
extern short Operating_System(void);

/*=========================================================================*/
/* The following prototypes are for the file: OPSYS.ASM                    */
/*=========================================================================*/

extern short OperationgSystem;

#endif // MISC_H
