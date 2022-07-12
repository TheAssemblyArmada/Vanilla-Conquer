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

/* $Header: /CounterStrike/MIXFILE.CPP 2     3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : MIXFILE.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : July 12, 1996 [JLB]                                          *
 *                                                                                             *
 *                                                                                             *
 *                  Modified by Vic Grippi for WwXlat Tool 10/14/96                            *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   MixFileClass::Cache -- Caches the named mixfile into RAM.                                 *
 *   MixFileClass::Cache -- Loads this particular mixfile's data into RAM.                     *
 *   MixFileClass::Finder -- Finds the mixfile object that matches the name specified.         *
 *   MixFileClass::Free -- Uncaches a cached mixfile.                                          *
 *   MixFileClass::MixFileClass -- Constructor for mixfile object.                             *
 *   MixFileClass::Offset -- Searches in mixfile for matching file and returns offset if found.*
 *   MixFileClass::Retrieve -- Retrieves a pointer to the specified data file.                 *
 *   MixFileClass::~MixFileClass -- Destructor for the mixfile object.                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "mixfile.h"
#include "ccfile.h"

template class MixFileClass<CCFileClass>;

/*
**	This is the pointer to the first mixfile in the list of mixfiles registered
**	with the mixfile system.
*/
template <class T, class TCRC> List<MixFileClass<T, TCRC>> MixFileClass<T, TCRC>::MixList;
