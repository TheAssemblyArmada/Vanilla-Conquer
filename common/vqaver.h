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
#ifndef VQAVER_H
#define VQAVER_H

enum VQAVersion
{
    VQA_VER_1 = 0,      // 1 - First VQAs, used only in Legend of Kyrandia III.
    VQA_VER_2 = 1,      // 2 - Used in C&C, Red Alert, Lands of Lore II, Dune 2000.
    VQA_VER_HICOLOR = 2 // 3 - Lands of Lore III, Blade Runner, Nox and Tiberian Sun.
};

const char* VQA_NameString();
const char* VQA_VersionString();
const char* VQA_DateTimeString();
const char* VQA_IDString();

#endif // VQA_VER_H
