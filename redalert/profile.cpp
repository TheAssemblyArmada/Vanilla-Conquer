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

/* $Header: /CounterStrike/PROFILE.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : PROFILE.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : September 10, 1993                                           *
 *                                                                                             *
 *                  Last Update : September 10, 1993   [JLB]                                   *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WWGetPrivateProfileInt -- Fetches integer value from INI.                                 *
 *   WWGetPrivateProfileString -- Fetch string from INI.                                       *
 *   WWWritePrivateProfileInt -- Write a profile int to the profile data block.                *
 *   WWWritePrivateProfileString -- Write a string to the profile data block.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"

/***************************************************************************
 * Read_Private_Config_Struct -- Fetches override integer value.           *
 *                                                                         *
 * INPUT:                                                                  *
 * OUTPUT:                                                                 *
 * WARNINGS:                                                               *
 * HISTORY:                                                                *
 *   08/05/1992 JLB : Created.                                             *
 *=========================================================================*/
bool Read_Private_Config_Struct(FileClass& file, NewConfigType* config)
{
    INIClass ini;
    ini.Load(file);

    config->DigitCard = ini.Get_Hex("Sound", "Card", 0);
    config->IRQ = ini.Get_Int("Sound", "IRQ", 0);
    config->DMA = ini.Get_Int("Sound", "DMA", 0);
    config->Port = ini.Get_Hex("Sound", "Port", 0);
    config->BitsPerSample = ini.Get_Int("Sound", "BitsPerSample", 0);
    config->Channels = ini.Get_Int("Sound", "Channels", 0);
    config->Reverse = ini.Get_Int("Sound", "Reverse", 0);
    config->Speed = ini.Get_Int("Sound", "Speed", 0);
    ini.Get_String("Language", "Language", "Eng", config->Language, sizeof(config->Language));

    return ((config->DigitCard == 0) && (config->IRQ == 0) && (config->DMA == 0));
}
