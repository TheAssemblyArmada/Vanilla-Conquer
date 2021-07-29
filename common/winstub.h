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
 ************************************************************************************************/

#ifndef COMMON_WINSTUB_H
#define COMMON_WINSTUB_H

/* Windows version of strncasecmp*/
#ifdef _MSC_VER /* MinGW provides strncasecmp. */
#define strncasecmp _strnicmp
#define strcasecmp  _stricmp
#endif

class GraphicViewPortClass;

/* Load Title Screen. Supports PCX and CPS files as file argument. */
void Load_Title_Screen(char* name, GraphicViewPortClass*, unsigned char*);

#endif // COMMON_WINSTUB_H
