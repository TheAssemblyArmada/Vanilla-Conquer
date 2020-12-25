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
#ifndef XORDELTA_H
#define XORDELTA_H

void Apply_XOR_Delta(void* dst, const void* src);
void Apply_XOR_Delta_To_Page_Or_Viewport(void* offset, const void* delta, int width, int pitch, int copy);
int Generate_XOR_Delta(void* dst, const void* src, const void* base, int size);

#endif // XORDELTA_H
