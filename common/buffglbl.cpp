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
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : BUFFGLBL.CPP                             *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : January 10, 1995                         *
 *                                                                         *
 *                  Last Update : January 10, 1995   [PWG]                 *
 *                                                                         *
 * This module holds the global fixup tables for the MCGA buffer class.    *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "gbuffer.h"

/*=========================================================================*/
/* We need to keep a pointer to the logic page hanging around somewhere		*/
/*=========================================================================*/
GraphicViewPortClass* LogicPage;

bool IconCacheAllowed = true;

/*
** Pointer to a function we will call if we detect loss of focus
*/
void (*Gbuffer_Focus_Loss_Function)(void) = nullptr;
